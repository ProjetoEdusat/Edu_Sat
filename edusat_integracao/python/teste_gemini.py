from edu_core import processar_pergunta


if __name__ == "__main__":
    resposta, continuar = processar_pergunta("Explique o que é um satélite.")
    print("Resposta:", resposta)
    print("Continuar ativo:", continuar)
