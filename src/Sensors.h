#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <DHT.h>
#include "Config.h"
#include "IOManager.h"

struct SensorData {
  int soilRaw[SOIL_SENSOR_COUNT];
  int soilPercent[SOIL_SENSOR_COUNT];
  int soilAveragePercent;

  bool bedLow;
  bool bedHigh;
  bool tankHasMinimumWater;
  bool tankHigh;

  bool bedLevelError;
  bool tankLevelError;

  float airHumidity;
  float airTemperature;
  bool dhtError;
};

class Sensors {
public:
  void begin();
  void update();
  SensorData getData();

private:
  SensorData data;

  unsigned long lastSensorUpdate = 0;
  unsigned long lastDhtUpdate = 0;

  DHT dht = DHT(DHT_PIN, DHT22);

  int soilRawToPercent(int raw, int sensorIndex);
  void updateSoilSensors();
  void updateLevelSensors();
  void updateDHTSensor();
};

extern Sensors sensors;

#endif