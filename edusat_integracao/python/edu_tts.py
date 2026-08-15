import asyncio
import os
import tempfile
from pathlib import Path

import edge_tts
import miniaudio


VOICE = os.getenv("TTS_VOICE", "pt-BR-AntonioNeural")
TTS_RATE = os.getenv("TTS_RATE", "+0%")
AUDIO_RATE_ESP = 16000


async def _gerar_mp3(texto: str, caminho: str) -> None:
    communicate = edge_tts.Communicate(
        texto,
        voice=VOICE,
        rate=TTS_RATE,
    )
    await communicate.save(caminho)


def gerar_pcm8_para_esp(texto: str) -> bytes:
    """
    Gera a mesma voz do Edge TTS usada no projeto e converte para:
    - mono
    - 16 kHz
    - PCM unsigned 8-bit

    Esse formato e enviado diretamente para o DAC GPIO25 da ESP32.
    """
    if not texto:
        return b""

    caminho = None

    try:
        with tempfile.NamedTemporaryFile(
            suffix=".mp3",
            delete=False,
        ) as temp:
            caminho = temp.name

        asyncio.run(_gerar_mp3(texto, caminho))

        decodificado = miniaudio.decode_file(
            caminho,
            output_format=miniaudio.SampleFormat.UNSIGNED8,
            nchannels=1,
            sample_rate=AUDIO_RATE_ESP,
        )

        return decodificado.samples.tobytes()

    finally:
        if caminho:
            try:
                Path(caminho).unlink(missing_ok=True)
            except Exception:
                pass


def falar_local(texto: str) -> None:
    """
    Mantido apenas para compatibilidade.
    Na V3 a voz principal e enviada para a ESP32.
    """
    print(f"🔊 EDU: {texto}")
