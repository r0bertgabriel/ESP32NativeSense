#include <ESP8266WiFi.h> // Para ESP8266
// #include <WiFi.h> // Para ESP32, descomente esta linha e comente a de cima
// #include <WiFiClientSecure.h> // Incluir para ESP32, se necessário

const char* ssid = "Convidados";
const char* password = "11223344";

const char* host = "docs.google.com";
const int httpsPort = 443;

WiFiClientSecure client; // Cria um cliente seguro (para HTTPS)
String textFix = "GET /forms/d/e/1FAIpQLSfWfpqCOpTF7t7fQke-1gipVRHDhDWAI-IFWsCMsGlTv5236A/formResponse?ifq&entry.1256734567=";

void setup()
{
    Serial.begin(115200); // Inicia a comunicação serial

    connectToWiFi();

    // Opcional: Verificar a autenticidade do certificado do servidor
    client.setInsecure(); // Descomente se precisar desativar a verificação de certificados (não recomendado em produção)
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        sendData();
    }
    else
    {
        Serial.println("WiFi desconectado. Tentando reconectar...");
        connectToWiFi();
    }

    delay(3000); // Aguardar 5 segundos antes de enviar novamente
}

void connectToWiFi()
{
    WiFi.mode(WIFI_STA); // Habilita o modo estação
    WiFi.begin(ssid, password); // Conecta na rede Wi-Fi

    Serial.print("Conectando ao WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado ao WiFi");
}

void sendData()
{
    if (client.connect(host, httpsPort)) // Conecta ao servidor Google Docs
    {
        String toSend = textFix; // Atribui a string auxiliar na nova string que será enviada
        toSend += random(0, 501); // Adiciona um valor aleatório
        toSend += "&submit=Submit HTTP/1.1"; // Completa o método GET para o formulário

        client.println(toSend); // Envia o GET ao servidor
        client.println("Host: docs.google.com");
        client.println("User-Agent: ESP8266/ESP32"); // Identifica o cliente
        client.println("Connection: close");
        client.println(); // Envia uma linha em branco para indicar o final da requisição

        Serial.println("Dados enviados.");
    }
    else
    {
        Serial.println("Erro ao se conectar ao servidor.");
    }

    client.stop(); // Encerra a conexão com o servidor
}
