#include <WiFi.h>
#include <PubSubClient.h>

// Credenciales de red
const char* ssid = "poiute8";
const char* password = "12345678";

// Datos cliente MQTT 
const char* mqtt_server = "iot.ceisufro.cl";
const int mqtt_port = 1883;
const char* token = "odpuzt2u87zlfmizuzwp";

// Declaracion clientes para comunicacion WiFi + MQTT
WiFiClient arduinoClient;
PubSubClient client(arduinoClient);

// Declaracion pines/conexiones en la placa
const int PinPhotoResistor = A0; 

const int PinWindowStatusLed = D3;
const int PinLED = D4;

const int PinPIR = D7;

const int PinMotor1 = D9; 
const int PinMotor2 = D10;

// Constante para determinar el valor de luz necesario para determinar dia/noche
const int ligthValueForDay = 300;

// Constante para el tiempo de giro del motor en ms
const int motorSpinTime = 2000;

// Inicializacion de variables
bool ledOn;
bool windowOpen;
int ligthValue;
bool motionDetected;
bool dayTime;
bool prevDayTime;  

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

// Control LED integrado placa para ver estado de conexion
void handleLEDStatus(bool connected) {
  if (connected) {
    digitalWrite(LED_BUILTIN, HIGH);  
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  
    delay(500);                  
    digitalWrite(LED_BUILTIN, LOW);   
    delay(500);                  
  }
}

// Conexion a ThingsBoard
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando a ThingsBoard");
      handleLEDStatus(false);
    if (client.connect("arduinoClient", token, NULL)) {
      Serial.println("Conectado");
      handleLEDStatus(true);
    } else {
      Serial.println("Fallo, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(PinPIR, INPUT);
  pinMode(PinPhotoResistor, INPUT);

  pinMode(PinLED, OUTPUT);
  pinMode(PinWindowStatusLed, OUTPUT);

  pinMode(PinMotor1, OUTPUT);
  pinMode(PinMotor2, OUTPUT);

  ledOn = false;
  windowOpen = false;
  ligthValue = 0;
  motionDetected = false;
  dayTime = false;
  prevDayTime = false;
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  Serial.begin(115200);
}

void stopMotorSpin() {
  digitalWrite(PinMotor1, LOW);
  digitalWrite(PinMotor2, LOW);
}

void openWindow() {
  digitalWrite(PinMotor1, LOW);
  digitalWrite(PinMotor2, HIGH);
  delay(motorSpinTime);

  windowOpen = true;
  digitalWrite(PinWindowStatusLed, HIGH);
  stopMotorSpin();  
}

void closeWindow() {
  digitalWrite(PinMotor1, HIGH);
  digitalWrite(PinMotor2, LOW);
  delay(motorSpinTime);

  windowOpen = false;
  digitalWrite(PinWindowStatusLed, LOW);
  stopMotorSpin();  
}

void turnOnLed() {
  ledOn = true;
  digitalWrite(PinLED, HIGH);
}

void turnOffLed() {
  ledOn = false;
  digitalWrite(PinLED, LOW);
}

void readSensors() {
  ligthValue = analogRead(PinPhotoResistor);
  motionDetected = digitalRead(PinPIR) == HIGH;
  dayTime = ligthValue > ligthValueForDay;
}

void systemControl() {
  if (dayTime != prevDayTime) {
    (dayTime) ? openWindow() : closeWindow();
    prevDayTime = dayTime;  // actualizar estado anterior
  }
  (motionDetected && !dayTime) ? turnOnLed() : turnOffLed();
}

// Generacion JSON con lecturas/estados del sistema
String generatePayload() {
  return
    "{"
      "\"nivelLuz\":" + String(ligthValue) + "," +
      "\"esDia\":" + String(dayTime ? "true" : "false") + "," +
      "\"movDetectado\":" + String(motionDetected ? "true" : "false") + "," +
      "\"ledPrendido\":" + String(ledOn ? "true" : "false") + "," +
      "\"ventanaAbierta\":" + String(windowOpen ? "true" : "false") +
    "}";
}

void loop() {
  // Conexion a cliente MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Lectura de sensores y actualizacion de su estado
  readSensors();

  // Control logico del comportamiento segun lectura de sensores
  systemControl();
  
  // Envio de JSON con lecturas/estados a ThingsBoard e impresion por serial
  client.publish("v1/devices/me/telemetry", generatePayload().c_str());

  delay(1000);
}
