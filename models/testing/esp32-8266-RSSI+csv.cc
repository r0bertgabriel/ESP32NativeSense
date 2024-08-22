#ifdef ESP32
  #include <WiFi.h>
  #include <WiFiClientSecure.h> // Para conexão HTTPS segura
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClientSecure.h> // Para conexão HTTPS segura
#endif

#include <time.h>
#include <stdio.h>

// Credenciais Wi-Fi
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Configurações do servidor Google Forms
const char* host = "docs.google.com";
const int httpsPort = 443;
WiFiClientSecure client; // Cria um cliente seguro (HTTPS)
String googleFormsURL = "GET /forms/d/e/1FAIpQLSfWfpqCOpTF7t7fQke-1gipVRHDhDWAI-IFWsCMsGlTv5236A/formResponse?ifq&entry.1256734567=";

// Função para inicializar a conexão Wi-Fi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

// Função para enviar dados ao Google Sheets
void sendDataToGoogleSheets(float temperature, float humidity, float pressure) {
    if (client.connect(host, httpsPort)) {
        String url = googleFormsURL;
        url += temperature;
        url += "&entry.987654321=" + String(humidity);
        url += "&entry.123456789=" + String(pressure);
        url += "&submit=Submit HTTP/1.1";

        client.println(url);
        client.println("Host: docs.google.com");
        client.println("User-Agent: ESP8266/ESP32");
        client.println("Connection: close");
        client.println();

        Serial.println("Dados enviados ao Google Sheets.");
    } else {
        Serial.println("Erro ao se conectar ao servidor.");
    }

    client.stop();
}

// Função principal para gerar os dados meteorológicos e enviar para Google Sheets
void generateWeatherDataAndSend() {
  float temperature = 28 + 4 * sin(millis() * 0.0001);
  float humidity = 80 + 5 * sin(millis() * 0.00005) + random(-20, 20) * 0.1;
  float pressure = 1010 + 2 * sin(millis() * 0.00002) + random(-3, 3) * 0.1;

  // Exibe os dados no monitor serial
  Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("Pressure: "); Serial.print(pressure); Serial.println(" hPa");

  // Envia os dados ao Google Sheets
  sendDataToGoogleSheets(temperature, humidity, pressure);
}

void setup() {
  Serial.begin(115200);
  initWiFi();

  // Opcional: Verificar a autenticidade do certificado do servidor
  client.setInsecure(); // Descomente se precisar desativar a verificação de certificados (não recomendado em produção)
}

void loop() {
  generateWeatherDataAndSend();
  delay(5000); // Aguardar 5 segundos antes de enviar novamente
}
