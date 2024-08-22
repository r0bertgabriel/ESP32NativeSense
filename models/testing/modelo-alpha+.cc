// FUNCIONOU!

// Verifica se está compilando para ESP32 ou ESP8266 e inclui a biblioteca adequada
#ifdef ESP32
  #include <WiFi.h>
  #include <WiFiClientSecure.h> // Para conexão HTTPS segura no ESP32
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClientSecure.h> // Para conexão HTTPS segura no ESP8266
#endif

// Substitua pelos detalhes da sua rede Wi-Fi (MODO STATION)
const char* ssid = "REPLACE_WITH_YOUR_SSID";  // Nome da rede Wi-Fi (SSID)
const char* password = "REPLACE_WITH_YOUR_PASSWORD";  // Senha da rede Wi-Fi

// Configurações do servidor Google Forms
const char* host = "docs.google.com";  // Host do Google Forms
const int httpsPort = 443;  // Porta HTTPS
WiFiClientSecure client;  // Cria um cliente seguro (HTTPS)
String googleFormsURL = "GET /forms/d/e/1FAIpQLSfWfpqCOpTF7t7fQke-1gipVRHDhDWAI-IFWsCMsGlTv5236A/formResponse?ifq&entry.1256734567=";

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

// Função para enviar dados ao Google Sheets
void sendDataToGoogleSheets() {
    if (client.connect(host, httpsPort)) {  // Conecta ao servidor Google Docs
        String url = googleFormsURL;  // URL base do formulário
        url += WiFi.RSSI();  // Adiciona o valor de RSSI na URL para envio ao Google Forms
        url += "&submit=Submit HTTP/1.1";  // Completa o método GET para o formulário

        client.println(url);  // Envia o GET ao servidor
        client.println("Host: docs.google.com");  // Especifica o host
        client.println("User-Agent: ESP8266/ESP32");  // Identifica o cliente
        client.println("Connection: close");  // Indica o fechamento da conexão após o envio
        client.println();  // Envia uma linha em branco para indicar o final da requisição

        Serial.println("Dados enviados ao Google Sheets.");
    } else {
        Serial.println("Erro ao se conectar ao servidor.");
    }

    client.stop();  // Encerra a conexão com o servidor
}

// Função de configuração (executada uma vez quando o dispositivo é ligado ou reiniciado)
void setup() {
  Serial.begin(115200);  // Inicia a comunicação serial em 115200 bps
  initWiFi();  // Chama a função para inicializar a conexão Wi-Fi
  
  // Opcional: Verificar a autenticidade do certificado do servidor
  client.setInsecure();  // Descomente se precisar desativar a verificação de certificados (não recomendado em produção)

  // Exibe o RSSI (indicador de intensidade do sinal recebido)
  Serial.print("RRSI: ");  
  Serial.println(WiFi.RSSI());  // Imprime o valor do RSSI no monitor serial
}

// Função principal do loop (executada continuamente)
void loop() {
  // Envia os dados de RSSI para o Google Sheets a cada 5 segundos
  sendDataToGoogleSheets();
  delay(5000);  // Espera 5 segundos antes de enviar novamente
}
