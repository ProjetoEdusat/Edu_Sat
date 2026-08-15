import os

from dotenv import load_dotenv
from google import genai
from google.genai import types

from edu_sensores import explicar_sensores


load_dotenv()

NOME = "EDU"
CRIADORES = "Felipe e Thiago"
MODELO = os.getenv("GEMINI_MODEL", "gemini-3.5-flash-lite")

PALAVRAS_DORMIR = [
    "dormir",
    "descansar",
    "pode dormir",
    "até logo",
    "tchau",
    "tchau edu",
    "boa noite",
    "descansa",
]

PALAVRAS_SENSORES = [
    "sensor",
    "sensores",
    "robô",
    "robo",
    "bateria",
    "energia",
    "temperatura",
    "luz",
    "painel solar",
]

SYSTEM_INSTRUCTION = (
    "Você é o EDU, um robô educacional criado por Felipe e Thiago. "
    "Ensine crianças sobre satélites e a Olimpíada Brasileira de Satélites (OBSat). "
    "Responda sempre com frases curtas, diretas e simples. "
    "Use no máximo 2 frases. "
    "Use linguagem infantil, clara e objetiva."
)

_client = None


def _obter_cliente():
    global _client

    if _client is not None:
        return _client

    api_key = os.getenv("GEMINI_API_KEY")
    if not api_key:
        raise RuntimeError(
            "GEMINI_API_KEY não encontrada. "
            "Crie o arquivo .env a partir de .env.example."
        )

    _client = genai.Client(api_key=api_key)
    return _client


def processar_pergunta(pergunta: str) -> tuple[str, bool]:
    """
    Retorna:
        (resposta, continuar_ativo)

    Essa função não captura microfone e não controla hardware.
    Ela é apenas o cérebro do EDU.
    """
    if not pergunta:
        return "Não consegui entender a pergunta.", True

    pergunta_limpa = pergunta.strip()
    pergunta_lower = pergunta_limpa.lower()

    for palavra in PALAVRAS_DORMIR:
        if palavra in pergunta_lower:
            return "Tudo bem! Vou descansar agora. Até logo!", False

    if any(palavra in pergunta_lower for palavra in PALAVRAS_SENSORES):
        return explicar_sensores(), True

    try:
        client = _obter_cliente()

        resposta = client.models.generate_content(
            model=MODELO,
            contents=pergunta_limpa,
            config=types.GenerateContentConfig(
                system_instruction=SYSTEM_INSTRUCTION,
                temperature=0.4,
            ),
        )

        texto = (resposta.text or "").strip()

        if not texto:
            return "Não consegui responder agora.", True

        return texto, True

    except Exception as e:
        print(f"❌ Erro Gemini: {e}")
        return "Não consegui responder agora.", True
