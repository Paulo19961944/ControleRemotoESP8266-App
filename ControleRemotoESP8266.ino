// INCLUSÃO DE BIBLIOTECAS
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRsend.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// DADOS DA SUA REDE WIFI
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

// DEFINIÇÃO DE PINOS
const byte buttonPin = 0; // Pin 0 (D3)
const byte irSensor = 4; // Pin 4 (D2)

// DEFINIÇÃO DE ESTADOS
bool buttonPressed = false;
bool learningMode = false;

// CONFIGURAÇÕES DO CONTROLE
unsigned long codigoControle = 0;
String tipoControle = "";
bool marcaControleEncontrada = false;

// CÓDIGOS CONTROLE
unsigned long panasonicAddress = 0x4004;
unsigned long panasonicCodes[] = {0x100BCBD, 0x1004C4D, 0x1002C2D, 0x100ACAD, 0x1000405, 0x1008485};
unsigned long SonyCodes[] = {0xA90, 0x290, 0x090, 0x890, 0x490, 0xC90};
unsigned long LGNECCodes[] = {0x20DF10EF, 0x20DF906F, 0x20DF00FF, 0x20DF807F, 0x20DF40BF, 0x20DFC03F};
unsigned long GenericNECCodes[] = {0x00FEA857, 0x00FE6897, 0x00FE9867, 0x00FE18E7, 0x00FED827, 0x00FE58A7};

// Índices para as ações
enum ActionIndex { POWER, MUTE, CHANNEL_UP, CHANNEL_DOWN, VOL_UP, VOL_DOWN };

// Mapeamento de códigos para ações baseado no protocolo e marca
const unsigned long actionCodes[4][6] = {
  // Panasonic
  {0x100BCBD, 0x1004C4D, 0x1002C2D, 0x100ACAD, 0x1000405, 0x1008485},
  // Sony
  {0xA90, 0x290, 0x090, 0x890, 0x490, 0xC90},
  // LG NEC
  {0x20DF10EF, 0x20DF906F, 0x20DF00FF, 0x20DF807F, 0x20DF40BF, 0x20DFC03F},
  // Generic NEC
  {0x00FEA857, 0x00FE6897, 0x00FE9867, 0x00FE18E7, 0x00FED827, 0x00FE58A7}
};

// Protocolo associado a cada marca
enum Protocol { PROTOCOL_PANASONIC, PROTOCOL_SONY, PROTOCOL_LG, PROTOCOL_GENERIC };

// Define o pino do receptor IR
uint16_t RECV_PIN = 2;
IRrecv irrecv(RECV_PIN);
decode_results results;

// Define o sinal de envio
IRsend irsend(irSensor);  // Define o GPIO para enviar a mensagem.

// CONFIGURAÇÃO DO IP ESTÁTICO
IPAddress staticIP(192, 168, 0, 100); // IP estático desejado
IPAddress gateway(192, 168, 0, 1); // IP do gateway (geralmente o IP do roteador)
IPAddress subnet(255, 255, 255, 0); // Máscara de sub-rede

// Inicializa o servidor
ESP8266WebServer server(80);

// Código HTML
void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="pt-br">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Controle Remoto - ESP8266</title>
        <style>
            @import url('https://fonts.googleapis.com/css2?family=Chakra+Petch:ital,wght@0,300;0,400;0,500;0,600;0,700;1,300;1,400;1,500;1,600;1,700&display=swap');
            *{ margin: 0; padding: 0; box-sizing: border-box; font-family: "Chakra Petch", sans-serif; }
            html, body{ width: 100vw; min-height: 100vh; }
            html{ font-size: 62.5%; }
            body{ background-color: #ddd; position: relative; }
            h1{ margin: 64px 0; text-align: center; font-size: 1.9rem; }
            p{ font-size: 1.2rem; }
            .container{ display: flex; justify-content: center; }
            .container:last-child{ margin-bottom: 96px; }
            #ligar-tv, #mute, #channel-up, #channel-down, #vol-up, #vol-down{
                cursor: pointer; border-radius: 8px; display: flex; justify-content: center; align-items: center;
                width: 144px; height: 120px; margin: 0 8px 16px; 
            } 
            #ligar-tv { background-color: #9E2A2B; color: #FFFFFF; }
            #mute { background-color: #950952; color: #FFFFFF; }
            #channel-up  { background-color: #086375; color: #FFFFFF; }
            #channel-down{ background-color: #03414e; color: #FFFFFF; }
            #vol-up { background-color: #058C42; color: #FFFFFF; }
            #vol-down{ background-color: #116537; color: #FFFFFF; }
            footer{ background-color: #333; color: #e0e0e0; width: 100%; display: flex; justify-content: center; align-items: center; position: absolute; bottom: -192px; padding: 48px 0; }
            footer p{ font-size: 1rem; }
            @media(max-width: 320px){ #ligar-tv, #mute, #channel-up, #channel-down, #vol-up, #vol-down{ width: 128px; height: 120px; } }
            @media (min-width: 720px) { h1{ font-size: 3.2rem; margin: 80px 0; } p, footer p{ font-size: 1.6rem; } #ligar-tv, #mute, #channel-up, #channel-down, #vol-up, #vol-down{ width: 200px; height: 160px; margin: 0 16px 16px; } #ligar-tv:hover, #mute:hover, #channel-up:hover, #channel-down:hover, #vol-up:hover, #vol-down:hover{ box-shadow: 0.3px 0.3px 4px 0.3px #888; } footer{ padding: 64px 0; } }
            @media (min-width: 1024px){ h1{ font-size: 3.2rem; } #ligar-tv, #mute, #channel-up, #channel-down, #vol-up, #vol-down{ width: 240px; height: 200px; margin: 0 32px 32px; } }
        </style>
    </head>
    <body>
        <header>
            <h1>Controle Remoto - ESP8266</h1>
        </header>
        <main>
            <section class="container">
                <article id="ligar-tv" onclick="sendCommand('ligar-tv')">
                    <p>Ligar TV</p>
                </article>
                <article id="mute" onclick="sendCommand('mute')">
                    <p>Mute</p>
                </article>
            </section>
            <section class="container">
                <article id="channel-up" onclick="sendCommand('channel-up')">
                    <p>Ch (+)</p>
                </article>
                <article id="channel-down" onclick="sendCommand('channel-down')">
                    <p>Ch (-)</p>
                </article>
            </section>
            <section class="container">
                <article id="vol-up" onclick="sendCommand('vol-up')">
                    <p>Vol (+)</p>
                </article>
                <article id="vol-down" onclick="sendCommand('vol-down')">
                    <p>Vol (-)</p>
                </article>
            </section>
        </main>
        <footer>
            <p>Criado por Paulo Henrique Azevedo do Nascimento</p>
        </footer>
        <script>
            function sendCommand(command) {
                fetch('/' + command, {
                    method: 'GET',
                })
                .then(response => {
                    if (response.ok) {
                        return response.text();
                    } else {
                        throw new Error('Erro na requisição: ' + response.status);
                    }
                })
                .then(data => {
                    console.log('Resposta do servidor:', data);
                })
                .catch(error => {
                    console.error('Erro:', error);
                });
            }
        </script>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

Protocol getProtocolIndex(const String& protocol) {
  if (protocol == "PANASONIC") return PROTOCOL_PANASONIC;
  if (protocol == "SONY") return PROTOCOL_SONY;
  if (protocol == "LG") return PROTOCOL_LG;
  if (protocol == "GENERIC") return PROTOCOL_GENERIC;
  return PROTOCOL_GENERIC; // Default
}

void handleCommand() {
  String command = server.uri();
  command.remove(0, 1); // Remove o '/' do início da string

  ActionIndex action;
  if (command == "ligar-tv") {
    action = POWER;
  } else if (command == "mute") {
    action = MUTE;
  } else if (command == "channel-up") {
    action = CHANNEL_UP;
  } else if (command == "channel-down") {
    action = CHANNEL_DOWN;
  } else if (command == "vol-up") {
    action = VOL_UP;
  } else if (command == "vol-down") {
    action = VOL_DOWN;
  } else {
    server.send(404, "text/plain", "Comando desconhecido");
    return;
  }

  if (marcaControleEncontrada) {
    unsigned long codeToSend = actionCodes[getProtocolIndex(tipoControle)][action];
    sendIRCode(tipoControle, codeToSend);
  } else {
    server.send(404, "text/plain", "Controle não reconhecido");
  }
  
  server.send(200, "text/plain", "Comando enviado: " + command);
}

void sendIRCode(const String& protocol, unsigned long code) {
  if (protocol == "PANASONIC") {
    irsend.sendPanasonic(panasonicAddress, code);
  } else if (protocol == "SONY") {
    irsend.sendSony(code, 12); // Ajuste o número de bits se necessário
  } else if (protocol == "LG") {
    irsend.sendNEC(code, 32);
  } else if (protocol == "GENERIC") {
    irsend.sendNEC(code, 32);
  }
}

void findControlLabel(unsigned long codeControl) {
  marcaControleEncontrada = false; // Reset the flag before searching

  for (int i = 0; i < sizeof(panasonicCodes) / sizeof(panasonicCodes[0]); i++) {
    if (panasonicCodes[i] == codeControl) {
      Serial.println("Controle Panasonic Encontrado!");
      marcaControleEncontrada = true;
      tipoControle = "PANASONIC";
      return;
    }
  }
  for (int i = 0; i < sizeof(SonyCodes) / sizeof(SonyCodes[0]); i++) {
    if (SonyCodes[i] == codeControl) {
      Serial.println("Controle Sony Encontrado!");
      marcaControleEncontrada = true;
      tipoControle = "SONY";
      return;
    }
  }
  for (int i = 0; i < sizeof(LGNECCodes) / sizeof(LGNECCodes[0]); i++) {
    if (LGNECCodes[i] == codeControl) {
      Serial.println("Controle LG Encontrado");
      marcaControleEncontrada = true;
      tipoControle = "LG";
      return;
    }
  }
  for (int i = 0; i < sizeof(GenericNECCodes) / sizeof(GenericNECCodes[0]); i++) {
    if (GenericNECCodes[i] == codeControl) {
      Serial.println("Controle Genérico Encontrado!");
      marcaControleEncontrada = true;
      tipoControle = "GENERIC";
      return;
    }
  }

  // CASO NÃO ENCONTRE
  if (!marcaControleEncontrada) {
    Serial.println("Marca de Controle Não Suportada!"); // Envia mensagem ao Usuário
  }
}

// SETUP INICIAL
void setup() {
  // CONFIGURAÇÕES BÁSICAS
  Serial.begin(115200); // Inicializa a Serial
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password); // Inicializa o WiFi
  irrecv.enableIRIn();  // Start the receiver
  Serial.println(); // Pula uma Linha

  // CONFIGURAÇÃO DO HARDWARE
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(irSensor, OUTPUT);

  // ENQUANTO NÃO ESTIVER CONECTADO
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200); // Espera 200 milisegundos
  }

  // MOSTRA OS DADOS NO SERIAL
  Serial.println(""); // Pula uma Linha
  Serial.print("Conectado no IP: "); // Exibe uma Mensagem que está conectado
  Serial.println(WiFi.localIP()); // Exibe o IP na Tela

  // CONFIGURAÇÕES DO SERVIDOR
  server.on("/", HTTP_GET, handleRoot); // Define a Rota Principal do Servidor
  server.on("/ligar-tv", HTTP_GET, handleCommand);
  server.on("/mute", HTTP_GET, handleCommand);
  server.on("/channel-up", HTTP_GET, handleCommand);
  server.on("/channel-down", HTTP_GET, handleCommand);
  server.on("/vol-up", HTTP_GET, handleCommand);
  server.on("/vol-down", HTTP_GET, handleCommand);
  server.begin(); // Inicializa o Servidor
}

// FUNÇÃO LOOP
void loop() {
  bool buttonState = digitalRead(buttonPin); // Armazena o estado do botão

  // VERIFICA O ESTADO DO BOTÃO
  if (buttonState == LOW) {
    if (!buttonPressed) {
      delay(3000); // Espera 3 segundos
      if (digitalRead(buttonPin) == LOW) { // Verifica se o botão ainda está pressionado
        if (learningMode) {
          Serial.println("Modo de Aprendizagem Desativado.");
          learningMode = false;
        } else {
          Serial.println("Modo de Aprendizagem Ativado. Digite uma tecla do controle para gravar!");
          learningMode = true;
        }
      }
      buttonPressed = true;
    }
  } else {
    buttonPressed = false;
  }

  if (learningMode && irrecv.decode(&results)) {
    codigoControle = results.value; // Corrigir a atribuição
    Serial.print("Código Recebido: ");
    Serial.println(codigoControle, HEX); // Mostrar código em hexadecimal
    findControlLabel(codigoControle);
    irrecv.resume();  // Receive the next value
  }

  server.handleClient(); // Chama a função Cliente do Servidor
}
