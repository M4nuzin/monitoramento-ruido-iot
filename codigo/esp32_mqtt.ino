#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid       = "Wokwi-GUEST";
const char* password   = "";
const char* mqttServer = "test.mosquitto.org";
const int   mqttPort   = 1883;

WiFiClient   espClient;
PubSubClient client(espClient);

const int buzzerPin        = 25;
const int potenciometroPin = 34;
const int limiteRuido      = 2000;

unsigned long tempoAnterior   = 0;
unsigned long mqttConectadoEm = 0;
const long intervaloMedicao   = 1000;

int contadorMedicoes = 0;
int contadorAlertas  = 0;
int totalPublicados  = 0;

// ── Leitura do potenciômetro ──────────────────────────────────────────────

int lerRuido() {
  int leituraADC = analogRead(potenciometroPin);

  // Converte 0-4095 para 0-4000
  int ruido = map(leituraADC, 0, 4095, 0, 4000);

  return ruido;
}

// ── Utilitários ───────────────────────────────────────────────────────────

String formatarTempo(unsigned long ms) {
  unsigned long seg = ms / 1000;
  int h = seg / 3600;
  int m = (seg % 3600) / 60;
  int s = seg % 60;

  char buf[12];
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
  return String(buf);
}

String barraVisual(int valor) {
  int blocos = map(valor, 0, 4000, 0, 24);

  String b = "|";

  for (int i = 0; i < 24; i++) {
    b += (i < blocos) ? "#" : "-";
  }

  b += "|";
  return b;
}

void linha(char c = '-', int n = 50) {
  for (int i = 0; i < n; i++) {
    Serial.print(c);
  }
  Serial.println();
}

// ── MQTT ──────────────────────────────────────────────────────────────────

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
  Serial.print("  [RX] ");
  Serial.print(topic);
  Serial.print(" : ");

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
}

bool publicarMQTT(const char* topico, const char* mensagem) {
  bool ok = client.publish(topico, mensagem);

  Serial.print(ok ? "    [TX OK]   " : "    [TX FAIL] ");
  Serial.print(topico);
  Serial.print("  ->  ");
  Serial.println(mensagem);

  if (ok) {
    totalPublicados++;
  }

  return ok;
}

// ── WiFi ──────────────────────────────────────────────────────────────────

void conectaWifi() {
  linha('=');

  Serial.println("  WIFI  Iniciando conexao...");
  Serial.print("  SSID  : ");
  Serial.println(ssid);

  Serial.println();
  Serial.println("  ATENCAO: certifique-se que o arquivo");
  Serial.println("  wokwi.toml existe com [net] enabled = true");

  linha('-');

  WiFi.disconnect(true);
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long inicio = millis();
  const unsigned long timeout = 30000;

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED) {

    if (millis() - inicio > timeout) {

      linha('!');

      Serial.println("  ERRO: WiFi nao conectou em 30s!");
      Serial.println("  Verifique o arquivo wokwi.toml:");

      Serial.println();
      Serial.println("    [wokwi]");
      Serial.println("    version = 1");
      Serial.println();
      Serial.println("    [net]");
      Serial.println("    enabled = true");

      linha('!');

      Serial.println("  Reiniciando em 5s...");
      delay(5000);
      ESP.restart();
    }

    delay(500);
    tentativas++;

    Serial.printf(
      "  [%3d] WiFi status: %d (aguardando...)\n",
      tentativas,
      WiFi.status()
    );
  }

  linha('-');

  Serial.println("  WIFI  Conectado com sucesso!");
  Serial.print("  IP    : ");
  Serial.println(WiFi.localIP());

  Serial.print("  RSSI  : ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  Serial.print("  MAC   : ");
  Serial.println(WiFi.macAddress());

  linha('=');
}

// ── MQTT conexão ──────────────────────────────────────────────────────────

void conectaMQTT() {
  linha('=');

  Serial.println("  MQTT  Iniciando conexao...");
  Serial.print("  Broker : ");
  Serial.println(mqttServer);

  Serial.print("  Porta  : ");
  Serial.println(mqttPort);

  linha('-');

  int tentativas = 0;

  while (!client.connected()) {

    tentativas++;

    String clientId = "ESP32_" + String(random(0xFFFF), HEX);

    Serial.printf(
      "  [%d] Tentando ID: %s\n",
      tentativas,
      clientId.c_str()
    );

    if (client.connect(clientId.c_str())) {

      mqttConectadoEm = millis();

      linha('-');

      Serial.println("  MQTT  Conectado com sucesso!");
      Serial.print("  Client ID : ");
      Serial.println(clientId);

      Serial.println("  QoS       : 0  |  Retain: false");

      Serial.println();
      Serial.println("  Topicos:");

      Serial.println("    PUB sensor/ruido");
      Serial.println("    PUB sensor/alerta");
      Serial.println("    PUB sensor/tempo");

      linha('=');

    } else {

      int rc = client.state();

      Serial.print("  Falha! rc=");
      Serial.println(rc);

      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("  WiFi perdido! Reconectando WiFi...");
        conectaWifi();
      }

      Serial.println("  Aguardando 3s...");
      delay(3000);
    }
  }
}

// ── Setup ─────────────────────────────────────────────────────────────────

void setup() {

  Serial.begin(115200);
  delay(500);

  pinMode(buzzerPin, OUTPUT);
  pinMode(potenciometroPin, INPUT);

  linha('*');

  Serial.println("  MONITORAMENTO DE RUIDO v3.0");
  Serial.println("  ESP32 + MQTT + Potenciometro");

  linha('*');
  Serial.println();

  conectaWifi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callbackMQTT);
  client.setSocketTimeout(15);
  client.setKeepAlive(60);

  conectaMQTT();
}

// ── Loop ──────────────────────────────────────────────────────────────────

void loop() {

  if (!client.connected()) {

    Serial.println();

    linha('!');

    Serial.println("  MQTT DESCONECTADO! Reconectando...");

    linha('!');

    conectaMQTT();
  }

  client.loop();

  unsigned long agora = millis();

  if (agora - tempoAnterior >= intervaloMedicao) {

    tempoAnterior = agora;
    contadorMedicoes++;

    int leituraADC = analogRead(potenciometroPin);
    int valorRuido = map(leituraADC, 0, 4095, 0, 4000);

    String timestamp = formatarTempo(agora);

    bool alerta = valorRuido > limiteRuido;

    if (alerta) {
      contadorAlertas++;
    }

    unsigned long uptime =
      (millis() - mqttConectadoEm) / 1000;

    linha('-');

    Serial.printf(
      "  #%-4d  %s   uptime MQTT: %lus\n",
      contadorMedicoes,
      timestamp.c_str(),
      uptime
    );

    linha('-');

    Serial.printf(
      "  ADC   : %4d / 4095\n",
      leituraADC
    );

    Serial.printf(
      "  Ruido : %4d / 4000  %s\n",
      valorRuido,
      barraVisual(valorRuido).c_str()
    );

    float pct = valorRuido / 4000.0 * 100.0;

    Serial.printf(
      "  Nivel : %.1f%%  |  Status: %s\n",
      pct,
      alerta ? "*** ACIMA DO LIMITE ***" : "Normal"
    );

    Serial.printf(
      "  Stats : %d medicoes | %d alertas | %d pub\n",
      contadorMedicoes,
      contadorAlertas,
      totalPublicados
    );

    Serial.println();
    Serial.println("  [MQTT] Publicando...");

    char msgRuido[10];
    sprintf(msgRuido, "%d", valorRuido);

    publicarMQTT("sensor/ruido", msgRuido);

    publicarMQTT(
      "sensor/alerta",
      alerta ? "RUIDO ACIMA DO LIMITE" : "RUIDO NORMAL"
    );

    publicarMQTT(
      "sensor/tempo",
      timestamp.c_str()
    );

    digitalWrite(
      buzzerPin,
      alerta ? HIGH : LOW
    );

    if (alerta) {
      Serial.println("  [BUZZER] ACIONADO");
    }
  }
}
