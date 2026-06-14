#include <Arduino.h>

#include "IOManager.h"
#include "Sensors.h"
#include "Actuators.h"
#include "StateMachine.h"
#include "WebServerManager.h"

unsigned long lastSerialPrint = 0;

void printSerialStatus() {
  SensorData data = sensors.getData();
  ActuatorStatus actuatorStatus = actuators.getStatus();

  Serial.println();
  Serial.println("========== ESTADO SISTEMA ==========");

  Serial.print("Modo: ");
  Serial.println(stateMachine.getModeName());

  Serial.print("Estado: ");
  Serial.println(stateMachine.getStateName());

  Serial.print("Siguiente paso manual: ");
  Serial.println(stateMachine.getManualStepName());

  Serial.print("Alerta: ");
  Serial.println(stateMachine.getAlertMessage());

  Serial.print("Humedad promedio suelo: ");
  Serial.print(data.soilAveragePercent);
  Serial.println("%");

  Serial.print("Auto - Inicio de llenado: ");
  Serial.print(stateMachine.getAutoFillStartHumidity());
  Serial.println("%");

  Serial.print("Auto - Objetivo de riego: ");
  Serial.print(stateMachine.getAutoIrrigationTargetHumidity());
  Serial.println("%");

  Serial.print("Auto - Fin de ventilacion: ");
  Serial.print(stateMachine.getAutoVentEndHumidity());
  Serial.println("%");

  for (int i = 0; i < SOIL_SENSOR_COUNT; i++) {
    Serial.print("Sensor D");
    Serial.print(SOIL_SENSOR_PINS[i]);
    Serial.print(" raw=");
    Serial.print(data.soilRaw[i]);
    Serial.print(" percent=");
    Serial.print(data.soilPercent[i]);
    Serial.println("%");
  }

  Serial.print("Cama nivel bajo D32: ");
  Serial.println(data.bedLow ? "ACTIVO" : "INACTIVO");

  Serial.print("Cama nivel alto D33: ");
  Serial.println(data.bedHigh ? "ACTIVO" : "INACTIVO");

  Serial.print("Deposito minimo D18: ");
  Serial.println(data.tankHasMinimumWater ? "CON AGUA" : "SIN AGUA");

  Serial.print("Deposito alto D13: ");
  Serial.println(data.tankHigh ? "ACTIVO" : "INACTIVO");

  Serial.print("Boton emergencia D25: ");
  Serial.println(io.isEmergencyPressed() ? "PRESIONADO" : "LIBRE");

  Serial.print("Boton llenado D26: ");
  Serial.println(io.isFillButtonPressed() ? "ENCLAVADO" : "LIBRE");

  Serial.print("Boton drenado D27: ");
  Serial.println(io.isDrainButtonPressed() ? "ENCLAVADO" : "LIBRE");

  Serial.print("Boton ventilacion D14: ");
  Serial.println(io.isFanButtonPressed() ? "ENCLAVADO" : "LIBRE");

  Serial.print("Rele valvula D23: ");
  Serial.println(actuatorStatus.valveOn ? "ON" : "OFF");

  Serial.print("Rele bomba llenado D22: ");
  Serial.println(actuatorStatus.fillPumpOn ? "ON" : "OFF");

  Serial.print("Rele bomba vaciado D21: ");
  Serial.println(actuatorStatus.drainPumpOn ? "ON" : "OFF");

  Serial.print("Rele ventilador D19: ");
  Serial.println(actuatorStatus.fanOn ? "ON" : "OFF");

  if (data.dhtError) {
    Serial.println("DHT22: ERROR");
  } else {
    Serial.print("DHT22 Temp: ");
    Serial.print(data.airTemperature);
    Serial.println(" C");

    Serial.print("DHT22 Humedad: ");
    Serial.print(data.airHumidity);
    Serial.println(" %");
  }

  Serial.println("====================================");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Iniciando Sistema Automatizado de Cama de Inmersion...");

  io.begin();
  sensors.begin();
  actuators.begin();
  stateMachine.begin();
  webServerManager.begin();

  Serial.println("Sistema listo.");
}

void loop() {
  sensors.update();

  stateMachine.update();

  webServerManager.handleClient();

  if (millis() - lastSerialPrint >= SERIAL_PRINT_INTERVAL_MS) {
    lastSerialPrint = millis();
    printSerialStatus();
  }
}