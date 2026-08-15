# EduSat — Integração EDU + Python/IA

Projeto de integração do **EDU** com **ESP32 + Python + Gemini**, mantendo as funções já existentes do robô, como olhos, asas e animações, e adicionando comunicação com IA, captura de voz e reprodução de áudio.

A versão atual descrita neste README é a **V3 — Teste de Voz**.

---

## 🚀 Visão geral

Na V3, o funcionamento esperado é:

```text
Botão da ESP32
    ↓
Inicia gravação pelo INMP441
    ↓
Botão novamente
    ↓
Finaliza gravação
    ↓
ESP32 envia o áudio pela rede
    ↓
Python recebe o áudio
    ↓
Gemini processa e gera a resposta
    ↓
Python gera a voz
    ↓
ESP32 recebe a resposta
    ↓
Texto no OLED + áudio no alto-falante
```

A integração foi feita para preservar as funções já prontas de **olhos, asas e animações** do EDU.

---

## 📁 Estrutura do projeto

```text
edusat_integracao/
├── .gitignore
├── README.md
└── edu/
    ├── arduino/
    │   └── codigo_edu/
    │       ├── codigo_edu_v3.ino
    │       ├── segredos.h
    │       ├── segredos.h.example
    │       └── INTEGRACAO.md
    └── python/
        ├── .env.example
        ├── main.py
        ├── edu_core.py
        ├── edu_sensores.py
        ├── edu_servidor.py
        ├── edu_tts.py
        ├── teste_cliente.py
        ├── teste_gemini.py
        └── requirements.txt
```

> O arquivo `segredos.h` deve permanecer apenas localmente e não deve ser enviado ao GitHub caso contenha senha do Wi-Fi ou outros dados privados.

---

# 🔧 Configuração da V3

## 1. Arduino / ESP32

Use:

```text
codigo_edu_v3.ino
```

como o sketch da ESP32.

Mantenha o arquivo:

```text
segredos.h
```

na mesma pasta do sketch.

Antes de substituir o firmware, é recomendado manter uma cópia do código atual da placa.

Também evite alterar funções de olhos, asas e animações que já estejam funcionando corretamente.

---

## 2. Arquivos Python da V3

Substitua pelos arquivos da V3:

```text
edu_core.py
edu_servidor.py
edu_tts.py
```

Não é necessário alterar:

```text
main.py
edu_sensores.py
```

---

# 🐍 Configuração do Python

Abra o PowerShell dentro da pasta:

```text
edu/python
```

Crie um ambiente virtual:

```powershell
py -m venv .venv
```

Ative o ambiente:

```powershell
.\.venv\Scripts\Activate.ps1
```

Atualize o `pip`:

```powershell
py -m pip install --upgrade pip
```

Instale as dependências do projeto:

```powershell
pip install -r requirements.txt
```

Para garantir especificamente as dependências utilizadas pela V3:

```powershell
pip install -U google-genai edge-tts miniaudio python-dotenv
```

Depois, copie o arquivo de exemplo:

```powershell
copy .env.example .env
```

Edite o `.env` e adicione sua chave:

```env
GEMINI_API_KEY=SUA_CHAVE_AQUI
```

---

# 🤖 Modelo Gemini

A V3 está configurada por padrão para usar:

```text
gemini-3.6-flash
```

Caso seu `.env` tenha:

```env
GEMINI_MODEL=nome-do-modelo
```

o valor definido no `.env` terá prioridade.

Use um modelo compatível com entrada de áudio.

---

# 🌐 Configuração de rede

No arquivo `segredos.h`, configure o Wi-Fi e o endereço do computador que estará executando o servidor Python.

Exemplo:

```cpp
PYTHON_HOST = "192.168.1.200";
PYTHON_PORT = 5000;
```

O `PYTHON_HOST` deve ser o **IP atual do notebook/computador na mesma rede da ESP32**.

A porta utilizada no projeto é:

```text
5000
```

Além disso, configure corretamente no `segredos.h`:

```text
SSID do Wi-Fi
Senha do Wi-Fi
IP do computador
Porta do servidor Python
```

---

# 🎙️ Microfone INMP441

Pinagem padrão utilizada pela V3:

| INMP441 | ESP32 |
|---|---|
| VDD | 3V3 |
| GND | GND |
| SCK / BCLK | GPIO26 |
| WS / LRCL | GPIO27 |
| SD | GPIO32 |
| L/R | GND |

Caso o seu INMP441 esteja ligado em outros GPIOs, altere somente as definições correspondentes no topo do arquivo `.ino`:

```text
MIC_BCLK
MIC_WS
MIC_SD
```

### Canal do microfone

Com o pino `L/R` ligado ao **GND**, a configuração utiliza:

```cpp
I2S_STD_SLOT_LEFT
```

Se o pino `L/R` estiver ligado ao **3V3**, utilize:

```cpp
I2S_STD_SLOT_RIGHT
```

---

# 🔊 Saída de voz

Na V3, a voz é enviada pela saída DAC analógica da ESP32 através do:

```text
GPIO25
```

Ligação esperada:

```text
GPIO25 da ESP32
      ↓
Entrada do amplificador
      ↓
Alto-falante
```

Também conecte:

```text
GND ESP32 → GND do amplificador
```

> ⚠️ **Não conecte um alto-falante comum diretamente ao GPIO25.**
>
> Utilize um amplificador entre a ESP32 e o alto-falante.
>
> Caso exista apenas um buzzer conectado ao GPIO25, ele não conseguirá reproduzir fala de maneira inteligível.

---

# ▶️ Como executar

Primeiro, envie o arquivo:

```text
codigo_edu_v3.ino
```

para a ESP32 usando o Arduino IDE.

Depois, abra o **Serial Monitor** em:

```text
115200 baud
```

Ao iniciar corretamente, devem aparecer mensagens semelhantes a:

```text
WiFi OK!
Internet OK!
Microfone: OK
Edu pronto!
```

No computador, dentro da pasta Python, execute:

```powershell
python main.py
```

---

# 🧪 Teste completo da voz

### 1. Primeiro clique no botão

A ESP32 deve iniciar a gravação:

```text
GRAVACAO INICIADA
```

### 2. Fale normalmente

O áudio será capturado pelo microfone INMP441.

### 3. Segundo clique no botão

A gravação deve parar:

```text
GRAVACAO FINALIZADA
Processando...
```

### 4. Verifique o terminal Python

Devem aparecer mensagens semelhantes a:

```text
Audio recebido
Enviando audio para o Gemini
Resposta
Gerando voz
```

### 5. Resposta do EDU

Ao final do processamento:

- a resposta deve aparecer no **OLED**;
- o áudio da resposta deve ser enviado de volta para a **ESP32**;
- a voz deve sair pelo sistema conectado ao **GPIO25 + amplificador + alto-falante**.

---

# 🧪 Teste sem a ESP32

Para testar somente o servidor Python, utilize:

```powershell
python teste_cliente.py
```

Também existe o teste específico do Gemini:

```powershell
python teste_gemini.py
```

---

# 🔄 Fluxo original da integração

A primeira versão utilizava o fluxo:

```text
ESP32 -> Wi-Fi/TCP -> Python -> Gemini -> texto de resposta -> ESP32
```

A V3 expande esse fluxo para permitir também **captura de voz e retorno de áudio**, mantendo a comunicação entre ESP32 e Python.

---

# ✅ Checklist rápido

Antes de testar, confirme:

- [ ] `codigo_edu_v3.ino` foi enviado para a ESP32.
- [ ] `segredos.h` está na mesma pasta do sketch.
- [ ] SSID e senha do Wi-Fi estão corretos.
- [ ] `PYTHON_HOST` contém o IP atual do computador.
- [ ] `PYTHON_PORT` está configurado como `5000`.
- [ ] ESP32 e computador estão na mesma rede.
- [ ] INMP441 está conectado corretamente.
- [ ] O canal LEFT/RIGHT do INMP441 corresponde à configuração do código.
- [ ] Amplificador está conectado ao GPIO25.
- [ ] Alto-falante está conectado ao amplificador, e não diretamente ao GPIO25.
- [ ] Ambiente virtual Python está ativo.
- [ ] Dependências Python estão instaladas.
- [ ] `.env` contém a `GEMINI_API_KEY`.
- [ ] `python main.py` está rodando.
- [ ] Serial Monitor está aberto em `115200`.

---

# 📌 Observações importantes

- Preserve uma cópia do firmware anterior antes de realizar alterações.
- Não envie o arquivo `segredos.h` com credenciais para um repositório público.
- Não envie o arquivo `.env` com sua chave da API para o GitHub.
- Mantenha as funções de olhos, asas e animações já validadas sem alterações desnecessárias.
- O computador que executa o Python deve estar acessível pela ESP32 através da rede local.

---

## EduSat V3

**ESP32 + Python + Gemini + INMP441 + OLED + saída de voz**
