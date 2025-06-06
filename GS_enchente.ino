#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Definições do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Definições dos pinos do ultrassônico
const int trigPin = 19;  // D19
const int echoPin = 18;  // D18

// Definição do pino do buzzer
const int buzzerPin = 15;

// Variáveis para medição
long duration;
float distance;

// Faixas de risco (ajuste conforme seu cenário)
const float limiteSeguro = 100;   // Até 100 cm
const float limiteAlerta = 50;    // De 50 a 100 cm
// Abaixo de 50 cm é crítico

// === Configurações de rede e mqtt === //
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// === Configurações MQTT ===
const char* broker = "20.64.236.3";
const int brokerPort = 1883;
const char* topicoPublish = "/TEF/stack-enchente001/attrs/e";
const char* idMQTT = "stack-enchente001";

// === Instâncias dos clientes Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// === Conectar à rede Wi-Fi ===
void initWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao Wi-Fi!");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao Broker MQTT...");
    if (client.connect(idMQTT)) {
      Serial.println("Conectado ao broker!");
    } else {
      Serial.print("Falha, rc=");
      Serial.println(client.state());
      Serial.println("Tentando novamente em 2s");
      delay(2000);
    }
  }
}

void setupMQTT() {
  client.setServer(broker, brokerPort);
}

void setup() {
  Serial.begin(115200);

  initWiFi();
  setupMQTT();

  // Inicializa o display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Falha no display SSD1306"));
    for(;;); // Travar
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Monitor de Nivel");
  display.println("Iniciando...");
  display.display();
  delay(2000);

  // Configura pinos do sensor e buzzer
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
}

void enviarResultado() {
  client.publish(topicoPublish, String(distance).c_str());
}

void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  // Envia pulso
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Mede o tempo de retorno
  duration = pulseIn(echoPin, HIGH);

  // Calcula a distancia (em cm)
  distance = duration * 0.034 / 2;
  enviarResultado();

  // Exibe no Serial Monitor
  Serial.print("Distancia: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Determina o status do nível
  String status = "";

  if(distance > limiteSeguro) {
    status = "SEGURO";
    noTone(buzzerPin);  // buzzer desligado
  } else if(distance <= limiteSeguro && distance > limiteAlerta) {
    status = "ALERTA";
    // Buzzer intermitente: liga 200ms, desliga 800ms
    tone(buzzerPin, 1000);  // tom 1000Hz
    delay(200);
    noTone(buzzerPin);
    delay(800);
  } else {
    status = "CRITICO";
    // Buzzer ligado contínuo com tom mais alto
    tone(buzzerPin, 2000); // tom 2000Hz
    delay(1000);
  }

  // Mostra no display OLED
  display.clearDisplay();

  // Texto do nível
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println("Nivel:");

  // Distancia
  display.setTextSize(2);
  display.setCursor(0,20);
  display.print(distance, 0);
  display.println(" cm");

  // Status
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.setTextColor(SSD1306_WHITE);
  display.println(status);

  display.display();
}
