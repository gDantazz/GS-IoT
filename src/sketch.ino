#include <WiFi.h>
#include <PubSubClient.h>

// =====================================
//             CONFIGURAÇÕES
// =====================================

// 📡 WiFi
const char* ssid = "Wokwi-GUEST"; // Seu SSID da rede
const char* password = "";        // Sua senha (vazio para Wokwi-GUEST)

// 🔑 Credenciais ThingSpeak
const char* mqtt_server = "52.202.164.249"; // IP do ThingSpeak
const int mqtt_port = 1883;

// SEU CHANNEL ID
long channelID = 3177550; 

// Sua API Key de Escrita (Usada para o TÓPICO e como CREDENCIAL)
const char* thingspeak_api_key = "AE97SFQEH1TH1PID"; 

// 🔒 Credencial MQTT Simplificada (Apenas o Client ID será mantido)
// Nota: O Username e Password serão substituídos pela thingspeak_api_key
const char* mqtt_client_id = "BQMoEjYWNCgfFhoFAyE0BjY"; 

// 📌 Pins
const int buttonPin = 2;
const int ledPin = 4;

// ⚙️ Estados
bool lastButtonState = HIGH;
bool pauseActive = false;
unsigned long pauseStartTime = 0;
int dailyPauseCount = 0;

// =====================================
//         OBJETOS E VARIÁVEIS GLOBAIS
// =====================================

WiFiClient espClient;
PubSubClient client(espClient);
String mqttTopic;

// =====================================
//            FUNÇÕES
// =====================================

/**
 * Tenta conectar (ou reconectar) o cliente MQTT ao broker ThingSpeak.
 */
void reconnect_mqtt() {
  // Loop até estarmos conectados
  while (!client.connected()) {
    Serial.print("🔌 Tentando conexão MQTT...");
    
    // 🎯 MUDANÇA CRÍTICA: Usando API_KEY como Username e Password.
    // Este formato é compatível com versões antigas da PubSubClient e aceito pelo ThingSpeak.
    if (client.connect(mqtt_client_id, thingspeak_api_key, thingspeak_api_key)) {
      Serial.println("✅ CONECTADO!");
    } else {
      Serial.print("❌ FALHOU! Código erro: ");
      Serial.println(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// =====================================
//              SETUP
// =====================================

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  
  Serial.println("==================================");
  Serial.println("🚀 WORKTIME ASSIST - CREDENCIAL SIMPLIFICADA");
  Serial.println("==================================");
  
  // 1. Conectar WiFi
  Serial.print("📡 Conectando WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Conectado!");
  } else {
    Serial.println("\n❌ WiFi Falhou! Reinicie o ESP32.");
    return;
  }
  
  // 2. Configurar MQTT
  client.setServer(mqtt_server, mqtt_port);
  
  // ❌ REMOVIDO: client.setProtocolVersion(4); (Causava erro de compilação)
  
  // O Tópico de publicação usa a API Key de Escrita
  mqttTopic = "channels/" + String(channelID) + "/publish/" + String(thingspeak_api_key);
  
  // 3. Conectar MQTT
  reconnect_mqtt();
  
  Serial.println("==================================");
  Serial.println("✅ Sistema Pronto! Pressione o botão.");
  Serial.println("==================================");
}

// =====================================
//               LOOP
// =====================================

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      // Tentar reconectar se desconectou
      reconnect_mqtt();
    }
    client.loop(); // Processa mensagens pendentes do MQTT
  }

  // Controle do botão
  bool buttonState = digitalRead(buttonPin);
  
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50); // Debounce
    
    if (digitalRead(buttonPin) == LOW) {
      if (!pauseActive) {
        // --- INICIAR PAUSA ---
        pauseActive = true;
        pauseStartTime = millis();
        dailyPauseCount++;
        digitalWrite(ledPin, HIGH); // LED LIGADO

        Serial.println("⏸️  PAUSA INICIADA");
        
        // Enviar para ThingSpeak (Field 1: 1 = Pausa Iniciada)
        if (client.connected()) {
          String payload = "field1=1&field2=0&field3=" + String(pauseStartTime) + "&field4=" + String(dailyPauseCount);
          if (client.publish(mqttTopic.c_str(), payload.c_str())) {
            Serial.println("📤 Dados de INÍCIO enviados!");
          }
        }
        
      } else {
        // --- ENCERRAR PAUSA ---
        pauseActive = false;
        unsigned long duration = (millis() - pauseStartTime) / 1000;
        digitalWrite(ledPin, LOW); // LED DESLIGADO

        Serial.print("▶️  PAUSA ENCERRADA - ");
        Serial.print(duration);
        Serial.println("s");
        
        // Enviar para ThingSpeak (Field 1: 0 = Pausa Encerrada, Field 2: Duração)
        if (client.connected()) {
          String payload = "field1=0&field2=" + String(duration) + "&field3=" + String(pauseStartTime) + "&field4=" + String(dailyPauseCount);
          if (client.publish(mqttTopic.c_str(), payload.c_str())) {
            Serial.println("📤 Dados de FIM enviados!");
          }
        }
      }
    }
  }
  
  lastButtonState = buttonState;
  delay(100);
}