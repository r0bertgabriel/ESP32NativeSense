/*
  Usei como referencia: https://RandomNerdTutorials.com/esp32-useful-wi-fi-functions-arduino/
  
  Este exemplo demonstra como conectar o ESP32 a uma rede Wi-Fi usando credenciais especificadas
  e como exibir o endereço IP local e o RSSI (Received Signal Strength Indicator) no monitor serial.
*/

#include <WiFi.h>  // Inclui a biblioteca WiFi para o ESP32

// Substitua pelos detalhes da sua rede (MODO STATION)
const char* ssid = "REPLACE_WITH_YOUR_SSID";  // Nome da rede Wi-Fi (SSID)
const char* password = "REPLACE_WITH_YOUR_PASSWORD";  // Senha da rede Wi-Fi

// Função para inicializar a conexão Wi-Fi
void initWiFi() {
  WiFi.mode(WIFI_STA);  // Define o modo Wi-Fi como Station (STA)
  WiFi.begin(ssid, password);  // Inicia a conexão com a rede Wi-Fi usando o SSID e a senha fornecidos
  Serial.print("Connecting to WiFi ..");  // Exibe uma mensagem enquanto tenta conectar
  
  // Continua tentando conectar até que a conexão seja estabelecida
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');  // Exibe um ponto a cada segundo enquanto a conexão não é estabelecida
    delay(1000);  // Espera 1 segundo antes de tentar novamente
  }
  
  // Quando a conexão é estabelecida, exibe o endereço IP local
  Serial.println(WiFi.localIP());  // Imprime o endereço IP atribuído ao ESP32 na rede
}

// Função de configuração (executada uma vez quando o dispositivo é ligado ou reiniciado)
void setup() {
  Serial.begin(115200);  // Inicia a comunicação serial em 115200 bps
  initWiFi();  // Chama a função para inicializar a conexão Wi-Fi
  
  // Exibe o RSSI (indicador de intensidade do sinal recebido)
  Serial.print("RRSI: ");  
  Serial.println(WiFi.RSSI());  // Imprime o valor do RSSI no monitor serial
}

// Função principal do loop (executada continuamente)
void loop() {
  // Coloque seu código principal aqui, para ser executado repetidamente:
}
