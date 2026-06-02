#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqttServer = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

const int sensorPin = 35;
const int buzzerPin = 25;

const int limiteRuido = 2000;

void conectaWifi() {

  Serial.print("Conectando WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
}

void conectaMQTT() {

  while (!client.connected()) {

    Serial.println("Conectando MQTT...");

    String clientId = "ManuESP32";
    clientId += String(random(1000));

    if (client.connect(clientId.c_str())) {

      Serial.println("MQTT conectado!");

    } else {

      Serial.println("Falha MQTT");
      delay(2000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);

  conectaWifi();

  client.setServer(mqttServer, 1883);
}

void loop() {

  if (!client.connected()) {
    conectaMQTT();
  }

  client.loop();

  int valorRuido = analogRead(sensorPin);

  Serial.print("Nivel de ruido: ");
  Serial.println(valorRuido);

  char msg[10];
  sprintf(msg, "%d", valorRuido);

  client.publish("mackenzie/ruido", msg);

  if (valorRuido > limiteRuido) {

    digitalWrite(buzzerPin, HIGH);

    client.publish(
      "mackenzie/alerta",
      "RUIDO ACIMA DO LIMITE"
    );

  } else {

    digitalWrite(buzzerPin, LOW);

    client.publish(
      "mackenzie/alerta",
      "RUIDO NORMAL"
    );
  }

  delay(1000);
}
