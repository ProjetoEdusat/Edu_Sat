// =============================================
// EDUSAT — MASCOTE EDU
// Versao 3.0 — botao grava/para + voz na ESP32
//
// FLUXO:
// 1 clique  -> comeca a ouvir o INMP441
// 2 clique  -> para, envia audio ao Python/Gemini
// Python    -> devolve texto + voz PCM
// ESP32     -> mostra no OLED + toca a voz no GPIO 25 (DAC)
//
// IMPORTANTE SOBRE O SOM:
// GPIO 25 e uma SAIDA DE AUDIO ANALOGICA (DAC).
// Use GPIO25 -> entrada de um amplificador -> alto-falante.
// NAO ligue um alto-falante comum diretamente no GPIO25.
//
// INMP441 (padrao desta versao):
// SCK/BCLK -> GPIO 26
// WS/LRCL  -> GPIO 27
// SD       -> GPIO 32
// L/R      -> GND (canal esquerdo)
// VDD      -> 3V3
// GND      -> GND
// =============================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <ESP_I2S.h>

#include "segredos.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo servoEsq;
Servo servoDir;

// Hardware que ja existia
#define PINO_BOTAO  4
#define PINO_TOUCH  5
#define PINO_SOM    25  // DAC analogico do ESP32

// INMP441 — altere SOMENTE se seus fios estiverem em outros GPIOs
#define MIC_BCLK    26
#define MIC_WS      27
#define MIC_SD      32

#define AUDIO_RATE  16000
#define MAX_GRAVACAO_MS 20000UL

const char* ssid       = WIFI_SSID;
const char* senha      = WIFI_PASSWORD;
const char* ipNotebook = PYTHON_HOST;
const int porta        = PYTHON_PORT;

bool wifiOK = false;
bool internetOK = false;
bool micOK = false;
int contadorIdle = 0;

I2SClass MicI2S(I2S_NUM_1);

String respostasOffline[] = {
  "Ola! Eu sou o Edu!",
  "Sou o mascote do EduSat!",
  "Toda crianca pode ser um cientista!",
  "O EduSat foi criado pelo Tiago e pelo Felipe!",
  "Voce sabia que eu fui ao espaco?",
  "Pergunte sobre satelites!",
  "A ciencia e para todos!"
};

int totalRespostas = 7;
int ultimaResposta = -1;


// =============================================
// UTILITARIOS DO DISPLAY
// =============================================

void mostrarMensagem(String linha1, String linha2 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setTextWrap(true);

  display.setCursor(0, 18);
  display.println(linha1);

  if (linha2 != "") {
    display.setCursor(0, 34);
    display.println(linha2);
  }

  display.display();
}


// =============================================
// OLHINHOS
// =============================================

void animacaoOlhos() {
  int mov_x = random(-20, 21);
  int mov_y = random(-10, 11);
  int tipo  = random(0, 10);

  display.clearDisplay();

  if (tipo < 7) {
    display.fillCircle(45 + mov_x, 32 + mov_y, 13, WHITE);
    display.fillCircle(83 + mov_x, 32 + mov_y, 13, WHITE);
  } else if (tipo == 7 || tipo == 8) {
    display.fillCircle(45 + mov_x, 32 + mov_y, 13, WHITE);
    display.fillCircle(83 + mov_x, 32 + mov_y, 13, WHITE);
    display.fillRect(0, 0, 128, 28 + mov_y, BLACK);
  } else {
    display.fillRect(32 + mov_x, 32 + mov_y, 25, 4, WHITE);
    display.fillRect(71 + mov_x, 32 + mov_y, 25, 4, WHITE);
  }

  display.display();
}

void olhosEscutando() {
  display.clearDisplay();
  display.fillCircle(45, 32, 18, WHITE);
  display.fillCircle(83, 32, 18, WHITE);
  display.fillCircle(45, 32, 7, BLACK);
  display.fillCircle(83, 32, 7, BLACK);
  display.display();
}

void olhosFalando() {
  display.clearDisplay();
  display.fillCircle(45, 32, 13, WHITE);
  display.fillCircle(83, 32, 13, WHITE);
  display.fillCircle(45, 32, 5, BLACK);
  display.fillCircle(83, 32, 5, BLACK);
  display.fillRect(0, 0, 128, 24, BLACK);
  display.display();
}

void olhosCoracao() {
  display.clearDisplay();

  display.fillCircle(35, 28, 9, WHITE);
  display.fillCircle(49, 28, 9, WHITE);
  display.fillTriangle(26, 32, 58, 32, 42, 46, WHITE);

  display.fillCircle(73, 28, 9, WHITE);
  display.fillCircle(87, 28, 9, WHITE);
  display.fillTriangle(64, 32, 96, 32, 80, 46, WHITE);

  display.display();
}

void olhosEstrelas() {
  display.clearDisplay();

  display.fillTriangle(32, 20, 45, 44, 58, 20, WHITE);
  display.fillTriangle(32, 44, 45, 20, 58, 44, WHITE);

  display.fillTriangle(70, 20, 83, 44, 96, 20, WHITE);
  display.fillTriangle(70, 44, 83, 20, 96, 44, WHITE);

  display.display();
}


// =============================================
// ASAS
// =============================================

void asasNeutras() {
  servoEsq.write(90);
  servoDir.write(90);
}

void asasAbertas() {
  servoEsq.write(30);
  servoDir.write(150);
}

void asasFechadas() {
  servoEsq.write(150);
  servoDir.write(30);
}

void asasBatendo(int vezes) {
  for (int v = 0; v < vezes; v++) {
    for (int i = 90; i >= 40; i -= 8) {
      servoEsq.write(i);
      servoDir.write(180 - i);
      delay(20);
    }

    for (int i = 40; i <= 90; i += 8) {
      servoEsq.write(i);
      servoDir.write(180 - i);
      delay(20);
    }
  }

  asasNeutras();
}

void asasRespirar() {
  for (int i = 90; i >= 75; i -= 3) {
    servoEsq.write(i);
    servoDir.write(180 - i);
    delay(25);
  }

  for (int i = 75; i <= 90; i += 3) {
    servoEsq.write(i);
    servoDir.write(180 - i);
    delay(25);
  }
}


// =============================================
// SOM / DAC GPIO25
// =============================================

void somSilencio() {
  dacWrite(PINO_SOM, 128);
}

void beepDAC(int frequencia, int duracaoMs) {
  if (frequencia <= 0 || duracaoMs <= 0) return;

  unsigned long meioPeriodo = 500000UL / frequencia;
  unsigned long fim = micros() + (unsigned long)duracaoMs * 1000UL;

  while ((int32_t)(fim - micros()) > 0) {
    dacWrite(PINO_SOM, 180);
    delayMicroseconds(meioPeriodo);
    dacWrite(PINO_SOM, 76);
    delayMicroseconds(meioPeriodo);
  }

  somSilencio();
}

void somBotao() {
  beepDAC(800, 70);
  delay(50);
  beepDAC(1200, 70);
}

void somCarinho() {
  beepDAC(1000, 100);
  delay(70);
  beepDAC(1300, 100);
  delay(70);
  beepDAC(1600, 150);
}


// =============================================
// REDE / INTERNET
// =============================================

bool testarInternet() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClient teste;
  teste.setTimeout(3000);

  Serial.println("Testando Internet...");

  // Testa DNS + conexao TCP externa.
  if (teste.connect("example.com", 80)) {
    teste.stop();
    Serial.println("Internet OK!");
    return true;
  }

  teste.stop();
  Serial.println("WiFi conectado, mas sem Internet.");
  return false;
}

bool lerLinhaComTimeout(WiFiClient &client, String &linha, unsigned long timeoutMs) {
  unsigned long inicio = millis();

  while (millis() - inicio < timeoutMs) {
    if (client.available()) {
      linha = client.readStringUntil('\n');
      linha.trim();
      return true;
    }

    if (!client.connected()) {
      return false;
    }

    delay(10);
  }

  return false;
}

void enviarU32(WiFiClient &client, uint32_t valor) {
  uint8_t b[4];
  b[0] = (valor >> 24) & 0xFF;
  b[1] = (valor >> 16) & 0xFF;
  b[2] = (valor >> 8) & 0xFF;
  b[3] = valor & 0xFF;
  client.write(b, 4);
}


// =============================================
// MICROFONE INMP441
// =============================================

bool iniciarMicrofone() {
  MicI2S.setPins(MIC_BCLK, MIC_WS, -1, MIC_SD);

  bool ok = MicI2S.begin(
    I2S_MODE_STD,
    AUDIO_RATE,
    I2S_DATA_BIT_WIDTH_32BIT,
    I2S_SLOT_MODE_MONO,
    I2S_STD_SLOT_LEFT
  );

  if (ok) {
    Serial.println("Microfone I2S iniciado.");
  } else {
    Serial.println("ERRO ao iniciar microfone I2S.");
  }

  return ok;
}


// =============================================
// VOZ RECEBIDA DO PYTHON
// Formato: PCM unsigned 8-bit, mono, 16 kHz
// =============================================

bool tocarAudioDoPython(WiFiClient &client, uint32_t tamanho) {
  if (tamanho == 0) {
    Serial.println("Python nao enviou audio.");
    return false;
  }

  Serial.printf("Recebendo voz: %u bytes\n", tamanho);

  uint8_t buffer[512];
  uint32_t restante = tamanho;

  // 16 kHz = 62,5 us/amostra.
  // Acumulador alterna 62/63 us para manter a media correta.
  uint32_t frac = 0;
  unsigned long proxima = micros();

  while (restante > 0) {
    size_t alvo = restante > sizeof(buffer) ? sizeof(buffer) : restante;
    size_t recebidos = 0;
    unsigned long inicio = millis();

    while (recebidos < alvo && millis() - inicio < 10000) {
      int disponivel = client.available();

      if (disponivel > 0) {
        size_t pedir = alvo - recebidos;
        if ((size_t)disponivel < pedir) pedir = disponivel;

        int n = client.read(buffer + recebidos, pedir);
        if (n > 0) {
          recebidos += n;
        }
      } else {
        delay(1);
      }
    }

    if (recebidos == 0) {
      Serial.println("Audio interrompido.");
      somSilencio();
      return false;
    }

    for (size_t i = 0; i < recebidos; i++) {
      // volume moderado: reduz a amplitude para evitar distorcao
      int amostra = 128 + ((int)buffer[i] - 128) * 3 / 4;
      if (amostra < 0) amostra = 0;
      if (amostra > 255) amostra = 255;

      dacWrite(PINO_SOM, (uint8_t)amostra);

      proxima += 62;
      frac += 8000;
      if (frac >= 16000) {
        proxima += 1;
        frac -= 16000;
      }

      while ((int32_t)(proxima - micros()) > 0) {
        delayMicroseconds(1);
      }
    }

    restante -= recebidos;
    yield();
  }

  somSilencio();
  Serial.println("Voz reproduzida.");
  return true;
}


// =============================================
// FLUXO PRINCIPAL DE VOZ
// Primeiro clique ja entrou aqui.
// Segundo clique para a gravacao.
// =============================================

void ouvirPerguntaEResponder() {
  if (!wifiOK || WiFi.status() != WL_CONNECTED) {
    mostrarMensagem("Sem WiFi", "Modo offline");
    delay(1200);
    return;
  }

  if (!micOK) {
    mostrarMensagem("Microfone", "nao iniciado");
    Serial.println("Microfone nao iniciado.");
    delay(1500);
    return;
  }

  WiFiClient client;
  client.setTimeout(60000);

  Serial.println("Conectando ao servidor Python para audio...");

  if (!client.connect(ipNotebook, porta)) {
    Serial.println("Sem conexao com Python!");
    mostrarMensagem("Servidor Python", "sem conexao");
    delay(1500);
    return;
  }

  // Inicia o novo protocolo binario.
  client.print("AUDIO_STREAM\n");

  somBotao();
  olhosEscutando();
  asasAbertas();
  mostrarMensagem("Estou ouvindo...", "Aperte p/ parar");

  Serial.println("GRAVACAO INICIADA");
  Serial.println("Aperte o botao novamente para parar.");

  // Evita interpretar o primeiro clique como segundo.
  while (digitalRead(PINO_BOTAO) == LOW) {
    delay(10);
  }
  delay(80);

  bool parar = false;
  bool anterior = HIGH;
  unsigned long inicioGravacao = millis();

  int32_t bruto[128];
  int16_t pcm16[128];

  while (!parar) {
    // Segundo clique
    int atual = digitalRead(PINO_BOTAO);

    if (anterior == HIGH && atual == LOW) {
      delay(35);
      if (digitalRead(PINO_BOTAO) == LOW) {
        parar = true;
      }
    }
    anterior = atual;

    if (millis() - inicioGravacao >= MAX_GRAVACAO_MS) {
      Serial.println("Tempo maximo de gravacao atingido.");
      parar = true;
    }

    if (parar) {
      break;
    }

    size_t bytesLidos = MicI2S.readBytes((char*)bruto, sizeof(bruto));

    if (bytesLidos == 0) {
      delay(1);
      continue;
    }

    size_t amostras = bytesLidos / sizeof(int32_t);

    for (size_t i = 0; i < amostras; i++) {
      // INMP441 entrega audio em slot de 32 bits.
      // O shift converte e aplica um ganho simples.
      int32_t v = bruto[i] >> 14;

      if (v > 32767) v = 32767;
      if (v < -32768) v = -32768;

      pcm16[i] = (int16_t)v;
    }

    uint32_t tamanhoChunk = amostras * sizeof(int16_t);
    enviarU32(client, tamanhoChunk);
    client.write((uint8_t*)pcm16, tamanhoChunk);

    if (!client.connected()) {
      Serial.println("Conexao com Python caiu durante a gravacao.");
      mostrarMensagem("Conexao caiu", "Tente novamente");
      client.stop();
      return;
    }
  }

  // Marca fim do stream.
  enviarU32(client, 0);

  while (digitalRead(PINO_BOTAO) == LOW) {
    delay(10);
  }

  Serial.println("GRAVACAO FINALIZADA");
  Serial.println("Processando no Python/Gemini...");

  asasNeutras();
  mostrarMensagem("Processando...", "aguarde");

  // Resposta do Python:
  // TEXT:<texto>\n
  // AUDIO:<quantidade de bytes>\n
  // <PCM unsigned 8 bit>
  String linhaTexto;
  if (!lerLinhaComTimeout(client, linhaTexto, 60000)) {
    Serial.println("Python nao respondeu.");
    mostrarMensagem("Python", "nao respondeu");
    client.stop();
    delay(1500);
    return;
  }

  if (!linhaTexto.startsWith("TEXT:")) {
    Serial.println("Resposta invalida: " + linhaTexto);
    mostrarMensagem("Resposta", "invalida");
    client.stop();
    delay(1500);
    return;
  }

  String resposta = linhaTexto.substring(5);
  resposta.trim();

  Serial.println("Resposta do Python: " + resposta);

  olhosFalando();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setTextWrap(true);
  display.setCursor(0, 5);
  display.println(resposta);
  display.display();

  String linhaAudio;
  if (!lerLinhaComTimeout(client, linhaAudio, 30000)) {
    Serial.println("Cabecalho de audio nao recebido.");
    client.stop();
    delay(1500);
    return;
  }

  uint32_t tamanhoAudio = 0;

  if (linhaAudio.startsWith("AUDIO:")) {
    tamanhoAudio = (uint32_t)linhaAudio.substring(6).toInt();
  }

  if (tamanhoAudio > 0) {
    tocarAudioDoPython(client, tamanhoAudio);
  } else {
    Serial.println("Sem audio de voz; texto foi recebido.");
  }

  client.stop();

  delay(700);
  olhosEstrelas();
  asasBatendo(1);
  delay(500);
  asasNeutras();
}


// =============================================
// ANIMACAO IDLE (mais responsiva ao botao)
// =============================================

void animacaoIdle() {
  static unsigned long ultimaAnimacao = 0;
  static unsigned long ultimaAsa = 0;

  unsigned long agora = millis();

  if (agora - ultimaAnimacao >= 1800) {
    contadorIdle++;
    animacaoOlhos();
    ultimaAnimacao = agora;
  }

  if (agora - ultimaAsa >= 12000) {
    asasRespirar();
    ultimaAsa = millis();
  }

  delay(10);
}


// =============================================
// FALA OFFLINE
// =============================================

void falarOffline() {
  int indice;

  do {
    indice = random(0, totalRespostas);
  } while (indice == ultimaResposta);

  ultimaResposta = indice;

  String texto = respostasOffline[indice];

  olhosFalando();
  asasBatendo(1);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.setTextWrap(true);
  display.println(texto);
  display.display();

  // Offline nao ha TTS do notebook. Faz apenas um aviso sonoro curto.
  beepDAC(700, 80);

  delay(1800);
  olhosEstrelas();
  delay(500);
  asasNeutras();

  Serial.println("Edu offline: " + texto);
}


// =============================================
// REACAO AO TOUCH
// =============================================

void reagirCarinho() {
  Serial.println("Carinho recebido!");

  olhosCoracao();
  somCarinho();

  for (int i = 0; i < 3; i++) {
    asasAbertas();
    delay(250);
    asasNeutras();
    delay(250);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 20);
  display.println("Obrigado pelo");
  display.setCursor(30, 35);
  display.println("carinho! <3");
  display.display();

  delay(1400);
  asasNeutras();
}


// =============================================
// SETUP
// =============================================

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED nao encontrado!");
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(35, 15);
  display.println("HELLO");
  display.setTextSize(1);
  display.setCursor(45, 40);
  display.println("EDUSAT");
  display.display();

  delay(1800);

  servoEsq.attach(18);
  servoDir.attach(19);

  asasNeutras();

  pinMode(PINO_BOTAO, INPUT_PULLUP);
  pinMode(PINO_TOUCH, INPUT);

  somSilencio();

  // Microfone pode iniciar mesmo antes da rede.
  micOK = iniciarMicrofone();

  mostrarMensagem("Conectando", "WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, senha);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    tentativas++;
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiOK = true;

    Serial.println("\nWiFi OK!");
    Serial.print("IP da ESP32: ");
    Serial.println(WiFi.localIP());

    mostrarMensagem("WiFi conectado!", WiFi.localIP().toString());
    delay(900);

    internetOK = testarInternet();

    if (internetOK) {
      mostrarMensagem("Internet OK!", "EDU online");
    } else {
      mostrarMensagem("WiFi OK", "Sem Internet");
    }

    delay(1000);

  } else {
    wifiOK = false;
    internetOK = false;

    Serial.println("\nModo offline");
    mostrarMensagem("Modo offline", "Sem WiFi");
    delay(1200);
  }

  Serial.println("Edu pronto!");
  Serial.print("Microfone: ");
  Serial.println(micOK ? "OK" : "ERRO");
  Serial.print("Internet: ");
  Serial.println(internetOK ? "OK" : "SEM ACESSO");

  animacaoOlhos();
}


// =============================================
// LOOP PRINCIPAL
// =============================================

void loop() {
  static bool botaoAnterior = HIGH;

  int botaoAtual = digitalRead(PINO_BOTAO);

  // Detecta somente a borda do clique, evitando repeticoes.
  if (botaoAnterior == HIGH && botaoAtual == LOW) {
    delay(35);

    if (digitalRead(PINO_BOTAO) == LOW) {
      Serial.println("Botao pressionado!");

      if (wifiOK) {
        ouvirPerguntaEResponder();
      } else {
        falarOffline();
      }

      // Espera soltar antes de voltar ao loop.
      while (digitalRead(PINO_BOTAO) == LOW) {
        delay(10);
      }
    }
  }

  botaoAnterior = digitalRead(PINO_BOTAO);

  if (digitalRead(PINO_TOUCH) == HIGH) {
    delay(35);

    if (digitalRead(PINO_TOUCH) == HIGH) {
      reagirCarinho();

      while (digitalRead(PINO_TOUCH) == HIGH) {
        delay(10);
      }
    }
  }

  animacaoIdle();
}
