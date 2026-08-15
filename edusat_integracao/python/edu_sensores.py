import random
from threading import Lock


_lock = Lock()
_dados_reais = None


def atualizar_sensores(dados: dict) -> None:
    """
    Recebe dados reais enviados pela ESP32 e os mantém em memória.
    Campos desconhecidos são aceitos para permitir expansão futura.
    """
    global _dados_reais

    if not isinstance(dados, dict):
        raise TypeError("Os dados dos sensores precisam ser um dicionário.")

    with _lock:
        _dados_reais = dict(dados)


def _simular_sensores() -> dict:
    """
    Fallback para desenvolvimento enquanto os sensores reais
    ainda não estiverem enviando dados pela ESP32.
    """
    return {
        "bateria": random.randint(20, 100),
        "solar": random.choice([True, False]),
        "temperatura": round(random.uniform(15, 35), 1),
        "luz": random.randint(100, 1000),
        "servo_1": random.choice(["parado", "girando"]),
        "servo_2": random.choice(["parado", "girando"]),
        "botao": random.choice([True, False]),
    }


def ler_sensores() -> dict:
    """
    Retorna os últimos dados reais recebidos.
    Se nenhum dado real chegou ainda, usa a simulação.
    """
    with _lock:
        if _dados_reais is not None:
            return dict(_dados_reais)

    return _simular_sensores()


def explicar_sensores() -> str:
    dados = ler_sensores()
    frases = []

    temperatura = dados.get("temperatura")
    if temperatura is not None:
        if temperatura > 30:
            frases.append("Está quente no satélite.")
        else:
            frases.append("A temperatura está boa.")

    bateria = dados.get("bateria")
    if bateria is not None:
        if bateria < 30:
            frases.append("A bateria está fraca.")
        else:
            frases.append("A bateria está carregada.")

    solar = dados.get("solar")
    if solar is not None:
        if solar:
            frases.append("Os painéis solares estão funcionando.")
        else:
            frases.append("Os painéis solares não estão recebendo luz.")

    servo_1 = dados.get("servo_1")
    servo_2 = dados.get("servo_2")
    if servo_1 is not None or servo_2 is not None:
        if servo_1 == "girando" or servo_2 == "girando":
            frases.append("O robô está se movendo.")
        else:
            frases.append("O robô está parado.")

    if dados.get("botao") is True:
        frases.append("O botão foi pressionado.")

    if not frases:
        return "Ainda não recebi dados dos sensores."

    return " ".join(frases)
