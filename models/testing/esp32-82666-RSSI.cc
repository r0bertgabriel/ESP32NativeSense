// Verifica se está compilando para ESP32 ou ESP8266 e inclui a biblioteca adequada
#ifdef ESP32
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

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
  Serial.println(WiFi.localIP());  // Imprime o endereço IP atribuído ao ESP na rede
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
