import socket


HOST = "127.0.0.1"
PORTA = 5000


def enviar(mensagem: str) -> str:
    with socket.create_connection((HOST, PORTA), timeout=10) as cliente:
        cliente.sendall((mensagem + "\n").encode("utf-8"))

        dados = b""
        while b"\n" not in dados:
            bloco = cliente.recv(1024)
            if not bloco:
                break
            dados += bloco

    return dados.decode("utf-8", errors="replace").strip()


if __name__ == "__main__":
    print("1) PING:")
    print(enviar("PING"))

    print("\n2) Sensores simulando ESP32:")
    print(
        enviar(
            'SENSORES:{"bateria":82,"solar":true,"temperatura":27.4,'
            '"luz":600,"servo_1":"parado","servo_2":"parado","botao":false}'
        )
    )

    print("\n3) Pergunta sobre sensores:")
    print(enviar("Como estão seus sensores e sua bateria?"))

    print("\n4) Pergunta para Gemini:")
    print(enviar("O que é um satélite?"))
