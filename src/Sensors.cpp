#include "Sensors.h"

Sensors sensors;

void Sensors::begin() {
  analogReadResolution(12);

  // Inicializar sensores reales: GPIO34, GPIO35, GPIO36
  for (int i = 0; i < REAL_SOIL_SENSOR_COUNT; i++) {
    pinMode(SOIL_SENSOR_PINS[i], INPUT);
  }

  // Inicializar todos los valores, incluyendo el simulado GPIO39
  for (int i = 0; i < SOIL_SENSOR_COUNT; i++) {
    data.soilRaw[i] = 0;
    data.soilPercent[i] = 0;
  }

  data.soilAveragePercent = 0;
  data.airHumidity = 0;
  data.airTemperature = 0;
  data.dhtError = true;

  randomSeed(micros());

  dht.begin();
}

void Sensors::update() {
  unsigned long now = millis();

  if (now - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL_MS) {
    lastSensorUpdate = now;
    updateSoilSensors();
    updateLevelSensors();
  }

  if (now - lastDhtUpdate >= DHT_UPDATE_INTERVAL_MS) {
    lastDhtUpdate = now;
    updateDHTSensor();
  }
}

SensorData Sensors::getData() {
  return data;
}

int Sensors::soilRawToPercent(int raw, int sensorIndex) {
  int percent = map(
    raw,
    SOIL_DRY_RAW[sensorIndex],
    SOIL_WET_RAW[sensorIndex],
    0,
    100
  );

  return constrain(percent, 0, 100);
}

void Sensors::updateSoilSensors() {
  static int currentSensorIndex = 0;

  // ===============================
  // Leer solo UN sensor real por ciclo
  // GPIO34 -> GPIO35 -> GPIO36 -> GPIO34 ...
  // ===============================

  int pin = SOIL_SENSOR_PINS[currentSensorIndex];

  // Lectura doble para estabilizar un poco el ADC
  analogRead(pin);
  delayMicroseconds(80);

  int raw = analogRead(pin);
  int percent = soilRawToPercent(raw, currentSensorIndex);

  data.soilRaw[currentSensorIndex] = raw;
  data.soilPercent[currentSensorIndex] = percent;

  currentSensorIndex++;
  if (currentSensorIndex >= REAL_SOIL_SENSOR_COUNT) {
    currentSensorIndex = 0;
  }

  // ===============================
  // Sensor 4 simulado: GPIO39
  // No se lee físicamente.
  // Se calcula a partir de los 3 sensores reales.
  // ===============================

  int sumRawReal = 0;
  int sumPercentReal = 0;

  for (int i = 0; i < REAL_SOIL_SENSOR_COUNT; i++) {
    sumRawReal += data.soilRaw[i];
    sumPercentReal += data.soilPercent[i];
  }

  int baseRaw = sumRawReal / REAL_SOIL_SENSOR_COUNT;
  int basePercent = sumPercentReal / REAL_SOIL_SENSOR_COUNT;

  int simulatedRaw = constrain(baseRaw + random(-60, 61), 0, 4095);
  int simulatedPercent = constrain(basePercent + random(-3, 4), 0, 100);

  data.soilRaw[3] = simulatedRaw;
  data.soilPercent[3] = simulatedPercent;

  // ===============================
  // Promedio total: 3 reales + 1 simulado
  // ===============================

  int sumPercentTotal = 0;

  for (int i = 0; i < SOIL_SENSOR_COUNT; i++) {
    data.soilPercent[i] = constrain(data.soilPercent[i], 0, 100);
    sumPercentTotal += data.soilPercent[i];
  }

  data.soilAveragePercent = constrain(sumPercentTotal / SOIL_SENSOR_COUNT, 0, 100);
}


void Sensors::updateLevelSensors() {
  /*
    Según tu tabla:

    D32 nivel bajo cama:
      Ahora está invertido respecto a antes.
      rawBedLowClosed() devuelve true cuando D32 está LOW / cerrado.
      Como este sensor quedó invertido:
      bedLow = true cuando D32 está abierto / HIGH.

    D33 nivel alto cama:
      Sin agua = abierta = HIGH
      Con agua = cerrada = LOW
      Entonces:
      bedHigh = true cuando D33 está cerrada / LOW

    D18 nivel bajo depósito:
      Sin agua = cerrada = LOW
      Con agua = abierta = HIGH
      Entonces:
      tankHasMinimumWater = true cuando D18 está abierta / HIGH

    D13 nivel alto depósito:
      Sin agua = abierta = HIGH
      Con agua = cerrada = LOW
      Entonces:
      tankHigh = true cuando D13 está cerrada / LOW
  */

  data.bedLow = !io.rawBedLowClosed();
  data.bedHigh = io.rawBedHighClosed();

  data.tankHasMinimumWater = !io.rawTankLowClosed();
  data.tankHigh = io.rawTankHighClosed();

  // Inconsistencia física: cama vacía y llena al mismo tiempo
  data.bedLevelError = data.bedLow && data.bedHigh;

  // Inconsistencia física: depósito sin mínimo pero con nivel alto
  data.tankLevelError = (!data.tankHasMinimumWater) && data.tankHigh;
}

void Sensors::updateDHTSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    data.dhtError = true;
    return;
  }

  data.dhtError = false;
  data.airHumidity = h;
  data.airTemperature = t;
}