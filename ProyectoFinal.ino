//ULTIMA ENTRE ARQUITECTURA COMPUTACIONAL
//INTEGRANTES:  BRAYAN BENAVIDES
//              ALEXIS SICLOS
//              BAIRON PALECHOR


// Incluye las bibliotecas necesarias
#include <StateMachine.h>
#include "AsyncTaskLib.h"
#include <LiquidCrystal.h>
#include <Keypad.h>
#include "DHTStable.h"


// Inicialización del objeto LiquidCrystal
// Nota: Configura los pines para controlar la pantalla LCD
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

// Configuración del teclado
// Nota: Define los pines del teclado matricial y las teclas correspondientes

const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
byte rowPins[KEYPAD_ROWS] = {41, 43, 45, 47}; 
byte colPins[KEYPAD_COLS] = {A3, A2, A1, A0};
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', '+'},
  {'4', '5', '6', '-'},
  {'7', '8', '9', '*'},
  {'.', '0', '=', '/'}
};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

#define DHT11_PIN 6

// Inicialización del objeto DHTStable
// Nota: Crea un objeto para leer datos del sensor DHT11

DHTStable DHT;

//tenemos definición de constantes y pines
#define BEZ_SENSOR 13
const int photocellPin = A7;
const int Led_rojo = 49;
const int Led_verde = 51;
const int Led_azul = 53;
#define PULSADOR 8

// tenemos variables de contraseña y bloqueo

char password[6];
char realpass[6] = "55555";
int idx = 0;
int bloq = 3;
boolean enter = false;
int outputValue = 0;

// Declaración de funciones de los estados

void securityState(void);
void eventDoorsWindowsState(void);
void environmentalMonitoringState(void);
void environmentalAlarmState(void);
void handleSecurityAlert(void);

// Declaración de funciones auxiliares para lectura de sensores

void readPhotoresistor(void);
void readTemperature(void);

// Tenemos las declaraciónes de tareas asíncronas

AsyncTask taskEnvironmentalMonitoring(2000, true, environmentalMonitoringState);
AsyncTask taskBeforeEnvironmentalMonitoring(3000, true, eventDoorsWindowsState);
AsyncTask taskAlarmAlert(3000, true, handleSecurityAlert);
AsyncTask taskEventAlarm(3000, true, eventDoorsWindowsState);

// Tenemos la declaración de la máquina de estados y sus estados

StateMachine machine = StateMachine();
State* stSecurityEntry = machine.addState(&securityState);
State* stEventDoorsAndWindows = machine.addState(&eventDoorsWindowsState);
State* stEnvironmentalMonitoring = machine.addState(&environmentalMonitoringState);
State* stEnvironmentalAlarm = machine.addState(&environmentalAlarmState);
State* stHandleSecurityAlert = machine.addState(&handleSecurityAlert);

  // Configuración inicial

void setup() {
  Serial.begin(115200);
  pinMode(Led_rojo, OUTPUT);
  pinMode(Led_verde, OUTPUT);
  pinMode(Led_azul, OUTPUT);

  // Agregamos las transiciones entre estados

  stSecurityEntry->addTransition(&checkCorrectPassword, stEventDoorsAndWindows);
  stEventDoorsAndWindows->addTransition(&checkTimeout2sec, stEnvironmentalMonitoring);
  stHandleSecurityAlert->addTransition(&checkTimeoutSec, stEventDoorsAndWindows);
  stEnvironmentalMonitoring->addTransition(&checkTemperatureGCel, stEnvironmentalAlarm);
  stEnvironmentalMonitoring->addTransition(&checkTimeout1Secu, stEventDoorsAndWindows);
  stEnvironmentalAlarm->addTransition(&checkTemperatureCS, stHandleSecurityAlert);
  stEnvironmentalAlarm->addTransition(&checkTemperatureBl, stEnvironmentalMonitoring);

  // Inicializar el objeto lcd y mostrar mensaje de inicio

  lcd.begin(16, 2);

  lcd.setCursor(3, 0);
  lcd.print("Sistema de");
  lcd.setCursor(3, 1);
  lcd.print("seguridad");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ingrese la clave");
  lcd.setCursor(0, 1);
}

void loop() {
    // Actualizar el estado actual de la máquina de estados

  machine.run();
}
// Implementación de funciones de los estados
// Estado de ingreso de seguridad

void securityState() {
  Serial.println("Ingreso de Seguridad");
    // Se ejecuta cuando se ingresa a este estado

  char key = keypad.getKey();

      // Verificar la contraseña ingresada

  if (key) {
    password[idx] = key;
    idx++;
    lcd.print("*");
  }

  if (idx == 5) {
    bool isCorrectPassword = true;
    for (int i = 0; i < 5; i++) {
      if (password[i] != realpass[i]) {
        isCorrectPassword = false;
        break;
      }
    }

    lcd.clear();
    delay(300);
    lcd.clear();

    if (isCorrectPassword) {
      digitalWrite(Led_verde, HIGH);
      lcd.setCursor(5, 0);
      lcd.print("Holi");
      lcd.setCursor(3, 0);
      lcd.print("Monitoreo");
      lcd.clear();
      digitalWrite(Led_verde, HIGH);
    } else {
      bloq--;
      if (bloq > 0) {
        digitalWrite(Led_rojo, HIGH);
        lcd.setCursor(5, 0);
        lcd.print("Clave");
        lcd.setCursor(3, 1);
        lcd.print("Incorrecta");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Ingrese la clave");
        delay(1000);
        lcd.setCursor(0, 1);
      } else {
        digitalWrite(Led_azul, HIGH);
        lcd.setCursor(0, 0);
        lcd.println("Sistema");
        lcd.setCursor(0, 1);
        lcd.println("Bloqueado");
        digitalWrite(Led_azul, HIGH);
        exit(0);
      }
    }

    idx = 0;
  }
}

// Tenemos la función para verificar la contraseña ingresada

bool checkCorrectPassword() {
  if (strcmp(password, realpass) == 0) {
    taskEnvironmentalMonitoring.Start();
    return true;
  }
  return false;
}

// Estado de puertas y ventanas
// Se ejecuta cuando se ingresa a este estado
// Realizar acciones relacionadas con puertas y ventanas

void eventDoorsWindowsState() {
  Serial.println("Puertas y Ventanas");
  taskEnvironmentalMonitoring.Update();
}

// Tenemos la función para verificar el tiempo de espera en segundos

bool checkTimeout2sec() {
  if (!taskEnvironmentalMonitoring.IsActive()) {
    taskEnvironmentalMonitoring.Stop();
    taskEnvironmentalMonitoring.Reset();
    return true;
  }
  return false;
}

// Tenemos la función para verificar si se activó una puerta o ventana

bool checkDoorWindowActivated() {
  return true;
}

//Tenemos el estado de monitoreo ambiental

void environmentalMonitoringState() {
  Serial.println("Monitor Ambiental");
  readTemperaturePhotoresistor();
  taskBeforeEnvironmentalMonitoring.Stop();
  taskBeforeEnvironmentalMonitoring.Reset();
}

// Tenemos la función para verificar si la temperatura supera los 32 grados Celsius

bool checkTemperatureGCel() {
  if (DHT.getTemperature() > 26) {
    taskBeforeEnvironmentalMonitoring.Stop();
    taskBeforeEnvironmentalMonitoring.Reset();
    taskAlarmAlert.Start();
    return true;
  }
  return false;
}

//Tenemos la función para verificar el tiempo de espera en segundos

bool checkTimeout1Secu() {
  if (!taskBeforeEnvironmentalMonitoring.IsActive()) {
    taskBeforeEnvironmentalMonitoring.Stop();
    taskBeforeEnvironmentalMonitoring.Reset();
    taskEnvironmentalMonitoring.Start();
    return true;
  }
  return false;
}

//Tenemos la función para leer la temperatura y el valor del fotoresistor

void readTemperaturePhotoresistor() {
  readTemperature();
  readPhotoresistor();
}

// Tenemos el estado de alarma ambiental

void environmentalAlarmState() {
  Serial.println("Alarma Ambiental");
  tone(BEZ_SENSOR, 262, 250);
  taskAlarmAlert.Update();
}

// Tenemos la función para verificar si la temperatura supera los 32 grados Celsius durante segundos

bool checkTemperatureCS() {
  if (!taskAlarmAlert.IsActive()) {
    taskAlarmAlert.Stop();
    taskAlarmAlert.Reset();
    taskEventAlarm.Reset();
    return true;
  }
  return false;
}

// Tenemos la función para verificar si la temperatura está por debajo de los 30 grados Celsius

bool checkTemperatureBl() {
  if (DHT.getTemperature() < 30) {
    taskAlarmAlert.Reset();
    return true;
  }
  return false;
}

// Tenemos estado de manejo de la alerta de seguridad

void handleSecurityAlert() {
  Serial.println("Alerta de Seguridad");
  taskEventAlarm.Update();
}

// Tenemos función para verificar el tiempo de espera en segundos

bool checkTimeoutSec() {
  if (!taskEventAlarm.IsActive()) {
    taskEventAlarm.Stop();
    taskEventAlarm.Reset();
    taskEnvironmentalMonitoring.Start();
    return true;
  }
  return false;
}

// Función para mostrar mensaje en la pantalla LCD

void processing() {
  for (int i = 0; i < 3; i++) {
    lcd.setCursor(2, 0);
    lcd.print("Procesando");
    lcd.setCursor(12, 0);
    delay(200);
    lcd.print(".");
    lcd.setCursor(13, 0);
    delay(200);
    lcd.print(".");
    lcd.setCursor(14, 0);
    delay(200);
    lcd.print(".");
    delay(200);
    lcd.clear();
  }
}

// Función para leer la temperatura

void readTemperature() {
  int chk = DHT.read11(DHT11_PIN);

  const char* errorMessages[] = {
    "OK",
    "Checksum error",
    "Time out error",
    "Unknown error"
  };

  if (chk >= 0 && chk < sizeof(errorMessages) / sizeof(errorMessages[0])) {
    Serial.print(errorMessages[chk]);
  } else {
    Serial.print("Invalido error codigo");
  }

  float humidity = DHT.getHumidity();
  float temperature = DHT.getTemperature();

  lcd.setCursor(0, 1);
  lcd.print("H%:");
  lcd.setCursor(4, 1);
  lcd.print(humidity, 1);
  Serial.print(",\t");
  Serial.print(humidity, 1);
  lcd.setCursor(9, 1);
  lcd.print("T:");
  lcd.setCursor(12, 1);
  lcd.print(temperature, 1);
  Serial.print(temperature, 1);
  Serial.println();
}

// Tenemos la función para leer el valor del fotoresistor

void readPhotoresistor() {
  int lightIntensity = analogRead(photocellPin);
  lcd.setCursor(0, 0);
  lcd.print("Luz:");
  lcd.setCursor(5, 0);
  lcd.print(lightIntensity);
  Serial.print("Luz = ");
  Serial.println(lightIntensity);

  if (lightIntensity > 100) {
    digitalWrite(Led_azul, HIGH);
  } else {
    digitalWrite(Led_azul, LOW);
  }
}

