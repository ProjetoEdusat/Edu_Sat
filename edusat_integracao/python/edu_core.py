import os
import time

from dotenv import load_dotenv
from google import genai
from google.genai import types

from edu_sensores import explicar_sensores


load_dotenv()

NOME = "EDU"
CRIADORES = "Felipe e Thiago"

# Modelo principal e fallback.
# Se o .env tiver GEMINI_MODEL=..., ele vira o principal.
MODELO = os.getenv("GEMINI_MODEL", "gemini-3.6-flash")
MODELO_FALLBACK = os.getenv("GEMINI_FALLBACK_MODEL", "gemini-3.5-flash")

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
        raise RuntimeError("GEMINI_API_KEY não encontrada. Confira o arquivo .env.")

    _client = genai.Client(api_key=api_key)
    return _client


def _extrair_texto(resposta) -> str:
    texto = (getattr(resposta, "text", None) or "").strip()

    if not texto:
        return "Não consegui responder agora."

    # O protocolo ESP32/Python usa uma linha para o texto.
    return texto.replace("\r", " ").replace("\n", " ").strip()


def _config_gemini():
    # O EDU não usa ferramentas/function calling nesta etapa.
    # Desabilitar AFC evita o aviso do SDK ao usar models.generate_content.
    return types.GenerateContentConfig(
        system_instruction=SYSTEM_INSTRUCTION,
        automatic_function_calling={"disable": True},
    )


def _eh_erro_temporario(exc: Exception) -> bool:
    texto = str(exc).upper()
    return (
        "503" in texto
        or "UNAVAILABLE" in texto
        or "HIGH DEMAND" in texto
        or "429" in texto
        or "RESOURCE_EXHAUSTED" in texto
    )


def _gerar_com_fallback(contents):
    """
    Tenta o modelo principal.
    Se houver sobrecarga temporária (503/429), tenta novamente e depois
    troca automaticamente para o modelo fallback.
    """
    client = _obter_cliente()

    modelos = []
    for modelo in (MODELO, MODELO_FALLBACK):
        if modelo and modelo not in modelos:
            modelos.append(modelo)

    ultimo_erro = None

    for modelo in modelos:
        # Duas tentativas por modelo. O próprio SDK também possui retries internos.
        for tentativa in range(2):
            try:
                if tentativa == 0:
                    print(f"🧠 Gemini: tentando {modelo}...")
                else:
                    print(f"🔁 Gemini: nova tentativa em {modelo}...")

                return client.models.generate_content(
                    model=modelo,
                    contents=contents,
                    config=_config_gemini(),
                )

            except Exception as e:
                ultimo_erro = e

                if not _eh_erro_temporario(e):
                    raise

                print(f"⚠️ {modelo} temporariamente indisponível: {e}")

                if tentativa == 0:
                    time.sleep(1.5)

        if modelo != modelos[-1]:
            print(f"↪️ Tentando modelo alternativo: {modelos[modelos.index(modelo) + 1]}")

    if ultimo_erro:
        raise ultimo_erro

    raise RuntimeError("Nenhum modelo Gemini configurado.")


def processar_pergunta(pergunta: str) -> tuple[str, bool]:
    """Mantém os testes antigos de pergunta em texto funcionando."""
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
        resposta = _gerar_com_fallback(pergunta_limpa)
        return _extrair_texto(resposta), True

    except Exception as e:
        print(f"❌ Erro Gemini: {e}")
        return "Não consegui responder agora.", True


def processar_audio(audio_wav: bytes) -> str:
    """
    Recebe o WAV montado pelo edu_servidor.py a partir do áudio da ESP32.
    O Gemini ouve a pergunta e devolve a resposta do EDU em texto.
    """
    if not audio_wav:
        return "Não consegui ouvir a pergunta."

    try:
        prompt = (
            "Ouça a pergunta falada neste áudio e responda diretamente como o EDU. "
            "Não diga que recebeu um áudio e não mostre uma transcrição separada. "
            "Se não conseguir entender a fala, diga isso de forma curta."
        )

        resposta = _gerar_com_fallback(
            [
                types.Part.from_text(text=prompt),
                types.Part.from_bytes(
                    data=audio_wav,
                    mime_type="audio/wav",
                ),
            ]
        )

        return _extrair_texto(resposta)

    except Exception as e:
        print(f"❌ Erro Gemini com áudio: {e}")
        return "Não consegui entender sua pergunta agora."