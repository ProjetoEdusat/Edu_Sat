import io
import json
import os
import socket
import struct
import threading
import wave

from dotenv import load_dotenv

from edu_core import processar_audio, processar_pergunta
from edu_sensores import atualizar_sensores
from edu_tts import gerar_pcm8_para_esp


load_dotenv()

HOST = os.getenv("EDU_HOST", "0.0.0.0")
PORTA = int(os.getenv("EDU_PORT", "5000"))

MAX_MENSAGEM = 8192
MAX_AUDIO_BYTES = 2_000_000

AUDIO_RATE = 16000
AUDIO_CHANNELS = 1
AUDIO_SAMPLE_WIDTH = 2  # PCM16 enviado pela ESP32


def _enviar_linha(conn: socket.socket, texto: str) -> None:
    texto = texto.replace("\r", " ").replace("\n", " ").strip()
    conn.sendall((texto + "\n").encode("utf-8"))


def _recv_exact(conn: socket.socket, quantidade: int) -> bytes:
    dados = bytearray()

    while len(dados) < quantidade:
        bloco = conn.recv(quantidade - len(dados))
        if not bloco:
            raise ConnectionError("Cliente desconectou durante a recepção.")
        dados.extend(bloco)

    return bytes(dados)


def _pcm16_para_wav(pcm: bytes) -> bytes:
    saida = io.BytesIO()

    with wave.open(saida, "wb") as wav:
        wav.setnchannels(AUDIO_CHANNELS)
        wav.setsampwidth(AUDIO_SAMPLE_WIDTH)
        wav.setframerate(AUDIO_RATE)
        wav.writeframes(pcm)

    return saida.getvalue()


def _receber_audio_stream(conn: socket.socket) -> bytes:
    """
    A ESP32 envia:
      uint32 tamanho_chunk (big endian)
      bytes do chunk
      ...
      uint32 0 = fim
    """
    audio = bytearray()

    while True:
        cabecalho = _recv_exact(conn, 4)
        tamanho = struct.unpack("!I", cabecalho)[0]

        if tamanho == 0:
            break

        if tamanho > 65536:
            raise ValueError(f"Chunk de áudio inválido: {tamanho}")

        if len(audio) + tamanho > MAX_AUDIO_BYTES:
            raise ValueError("Áudio maior que o limite permitido.")

        audio.extend(_recv_exact(conn, tamanho))

    return bytes(audio)


def _processar_audio_esp(conn: socket.socket, addr) -> None:
    print(f"🎤 ESP32 começou a enviar áudio: {addr}")

    pcm16 = _receber_audio_stream(conn)

    duracao = len(pcm16) / (AUDIO_RATE * AUDIO_SAMPLE_WIDTH)
    print(f"🛑 Áudio recebido: {duracao:.1f}s ({len(pcm16)} bytes)")

    if len(pcm16) < AUDIO_RATE * AUDIO_SAMPLE_WIDTH // 4:
        resposta = "Fale um pouco mais perto do meu microfone."
    else:
        wav_bytes = _pcm16_para_wav(pcm16)

        print("🧠 Enviando áudio para o Gemini...")
        resposta = processar_audio(wav_bytes)

    print(f"🤖 Resposta: {resposta}")
    print("🔊 Gerando voz para a ESP32...")

    try:
        audio_pcm8 = gerar_pcm8_para_esp(resposta)
    except Exception as e:
        print(f"⚠️ Falha ao gerar voz: {e}")
        audio_pcm8 = b""

    _enviar_linha(conn, f"TEXT:{resposta}")
    _enviar_linha(conn, f"AUDIO:{len(audio_pcm8)}")

    if audio_pcm8:
        conn.sendall(audio_pcm8)

    print(f"✅ Resposta enviada para a ESP32 ({len(audio_pcm8)} bytes de voz)")


def _processar_mensagem_texto(mensagem: str) -> str:
    """
    Compatibilidade com testes antigos:
      PING -> PONG
      SENSORES:{json} -> OK
      qualquer texto -> Gemini
    """
    mensagem = mensagem.strip()

    if not mensagem:
        return "ERRO: mensagem vazia"

    if mensagem.upper() == "PING":
        return "PONG"

    if mensagem.startswith("SENSORES:"):
        payload = mensagem[len("SENSORES:"):].strip()

        try:
            dados = json.loads(payload)
            atualizar_sensores(dados)
            print(f"📡 Sensores atualizados: {dados}")
            return "OK"
        except Exception as e:
            return f"ERRO_SENSORES: {e}"

    print(f"👤 Pergunta recebida: {mensagem}")

    resposta, _continuar = processar_pergunta(mensagem)

    print(f"🤖 Resposta: {resposta}")

    return resposta


def _atender_cliente(conn: socket.socket, addr) -> None:
    print(f"✅ ESP/cliente conectado: {addr}")

    try:
        # Longo o suficiente para o usuario falar e para o envio dos chunks.
        conn.settimeout(60)

        primeira_linha = b""

        while b"\n" not in primeira_linha and len(primeira_linha) < MAX_MENSAGEM:
            bloco = conn.recv(1)
            if not bloco:
                break
            primeira_linha += bloco

        comando = primeira_linha.decode("utf-8", errors="replace").strip()

        if comando == "AUDIO_STREAM":
            _processar_audio_esp(conn, addr)
            return

        resposta = _processar_mensagem_texto(comando)
        _enviar_linha(conn, resposta)

    except socket.timeout:
        print(f"⚠️ Timeout do cliente {addr}")
    except Exception as e:
        print(f"❌ Erro com {addr}: {e}")
        try:
            _enviar_linha(conn, "TEXT:Ocorreu um erro ao processar sua pergunta.")
            _enviar_linha(conn, "AUDIO:0")
        except Exception:
            pass
    finally:
        conn.close()


def iniciar_servidor() -> None:
    servidor = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    servidor.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    servidor.bind((HOST, PORTA))
    servidor.listen(5)

    print(f"🌐 Servidor do EDU em {HOST}:{PORTA}")
    print("Aguardando ESP32...\n")

    try:
        while True:
            conn, addr = servidor.accept()
            threading.Thread(
                target=_atender_cliente,
                args=(conn, addr),
                daemon=True,
            ).start()
    except KeyboardInterrupt:
        print("\n👋 Servidor encerrado.")
    finally:
        servidor.close()
