#include <WiFi.h>
#include <PubSubClient.h>
// Credenciales de red
const char* ssid = "AFGC";
const char* password = "mxcf15132310";
// Datos cliente MQTT 
const char* mqtt_server = "iot.ceisufro.cl";
const int mqtt_port = 1883;
const char* token = "odpuzt2u87zlfmizuzwp";
// Declaracion clientes para comunicacion WiFi + MQTT
WiFiClient arduinoClient;
PubSubClient client(arduinoClient);
// Declaracion pines/conexiones en la placa
const int PinLED = D4;
const int PinPIR = D7;
const int PinPhotoResistor = A0; 
const int PinServo = D3;
// Constante para determinar el valor de luz necesario para determinar dia/noche
const int ligthValueForDay = 300;
// Inicializacion de variables
bool ledOn;
bool servoOpen;
int ligthValue;
bool motionDetected;
bool dayTime;
// Configuracion conexion WiFi + Indicador de conexion exitosa
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
}
// Conexion a ThingsBoard
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando a ThingsBoard... ");
    if (client.connect("arduinoClient", token, NULL)) {
      Serial.println("Conectado");
    } else {
      Serial.println("Fallo, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  
  pinMode(PinPIR, INPUT);
  pinMode(PinPhotoResistor, INPUT);

  pinMode(PinLED, OUTPUT);
  pinMode(PinServo, OUTPUT);

  ledOn = false;
  servoOpen = false;
  ligthValue = 0;
  motionDetected = false;
  dayTime = false;
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  Serial.begin(115200);
}

void openWindow() {
  servoOpen = true;
  digitalWrite(PinServo, HIGH);
}
void closeWindow() {
  servoOpen = false;
  digitalWrite(PinServo, LOW);
}
void turnOnLed() {
  ledOn = true;
  digitalWrite(PinLED, HIGH);
}
void turnOffLed() {
  ledOn = false;
  digitalWrite(PinLED, LOW);
}
// Generacion JSON con lecturas/estados del sistema
String generatePayload() {
  return
    "{"
      "\"nivelLuz\":" + String(ligthValue) + "," +
      "\"esDia\":" + String(dayTime ? "true" : "false") + "," +
      "\"movDetectado\":" + String(motionDetected ? "true" : "false") + "," +
      "\"ledPrendido\":" + String(ledOn ? "true" : "false") + "," +
      "\"ventanaAbierta\":" + String(servoOpen ? "true" : "false") +
    "}";
}
void loop() {
  // Conexion a cliente MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // Lectura de los sensores
  ligthValue = analogRead(PinPhotoResistor);
  motionDetected = digitalRead(PinPIR) == HIGH;
  dayTime = ligthValue > ligthValueForDay;
  // Control logico del comportamiento segun lectura de sensores
  dayTime ? openWindow() : closeWindow();
  (motionDetected && !dayTime) ? turnOnLed() : turnOffLed();
  // Envio de JSON con lecturas/estados a ThingsBoard e impresion por serial
  client.publish("v1/devices/me/telemetry", generatePayload().c_str());
  Serial.println(generatePayload());
  
  delay(1000);
}
