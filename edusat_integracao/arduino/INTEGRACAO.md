# Integração com o Python

As funções de olhos, asas, animação, touch e comportamento já existentes devem permanecer intactas.

## 1. Configuração

O `codigo_edu.ino` usa:

```cpp
#include "segredos.h"
```

Crie `segredos.h` a partir de `segredos.h.example`.

## 2. Teste da conexão TCP

A função `enviarParaPython()` envia uma linha de texto ao servidor e espera uma linha de resposta.

Enquanto o INMP441 ainda não estiver implementado, a pergunta fixa pode continuar:

```cpp
String pergunta = "O que e o EduSat";
```

Isso permite validar primeiro:

```text
ESP32 -> Wi-Fi -> Python -> Gemini -> ESP32
```

## 3. Microfone INMP441

O código-fonte recebido ainda NÃO informa os pinos BCLK/SCK, WS/LRCLK e SD do INMP441.

Não é seguro preencher esses pinos por suposição.

Quando o pinout físico for confirmado, a pergunta fixa será substituída por:

```text
INMP441 -> I2S -> buffer de áudio -> Wi-Fi -> servidor Python
```

Isso será uma segunda etapa, sem alterar as funções de olhos/asas.
