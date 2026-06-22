#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===============================
// WiFi AP
// ===============================
const char* const WIFI_SSID = "ESP32-CIA";
const char* const WIFI_PASSWORD = "12345678";

// ===============================
// Sensores de humedad analogica
// ===============================
const int SOIL_SENSOR_COUNT = 4;
const int REAL_SOIL_SENSOR_COUNT = 3;

// SH1, SH2, SH3, SH4
const int SOIL_SENSOR_PINS[SOIL_SENSOR_COUNT] = {34, 35, 36, 39};

// Calibración individual
const int SOIL_DRY_RAW[SOIL_SENSOR_COUNT] = {
  3080,  // GPIO34 real
  3170,  // GPIO35 real
  3170,  // GPIO36 real
  3170   // GPIO39 real
};

const int SOIL_WET_RAW[SOIL_SENSOR_COUNT] = {
  1690,  // GPIO34 real
  1600,  // GPIO35 real
  1610,  // GPIO36 real
  1610   // GPIO39 real
};

// ===============================
// Sensores de nivel
// INPUT_PULLUP
// Abierto = HIGH
// Cerrado = LOW
// ===============================
const int PIN_BED_LOW = 32;     // Nivel bajo cama
const int PIN_BED_HIGH = 33;    // Nivel alto cama
const int PIN_TANK_LOW = 18;    // Nivel bajo depósito
const int PIN_TANK_HIGH = 13;   // Nivel alto depósito

// ===============================
// Botones físicos
// INPUT_PULLUP
// Presionado = LOW
// ===============================
const int PIN_EMERGENCY = 25;
const int PIN_BTN_FILL = 26;
const int PIN_BTN_DRAIN = 27;
const int PIN_BTN_FAN = 14;

// ===============================
// Relés activos en LOW
// LOW  = ON
// HIGH = OFF
// ===============================
const int RELAY_VALVE = 23;
const int RELAY_FILL_PUMP = 22;
const int RELAY_DRAIN_PUMP = 21;
const int RELAY_FAN = 19;

// ===============================
// DHT22
// RX2 = GPIO16
// ===============================
const int DHT_PIN = 16;

// ===============================
// Umbrales de operación
// ===============================
const int DEFAULT_FILL_START_HUMIDITY = 30;
const int DEFAULT_IRRIGATION_TARGET = 80;
const int DEFAULT_VENT_END_HUMIDITY = 75;

// ===============================
// Timeouts de seguridad
// Ajustar después de pruebas reales
// ===============================
const unsigned long MAX_FILL_TIME_MS = 7000000;       
const unsigned long MAX_CAPILLARY_TIME_MS = 7000000;  
const unsigned long MAX_DRAIN_TIME_MS = 7000000;  
const unsigned long MAX_VENT_TIME_MS = 7000000;   

// ===============================
// Intervalos
// ===============================
const unsigned long SENSOR_UPDATE_INTERVAL_MS = 500;
const unsigned long DHT_UPDATE_INTERVAL_MS = 2500;
const unsigned long SERIAL_PRINT_INTERVAL_MS = 2000;


const unsigned long EXTRA_DRAIN_TIME_MS = 60000;

#endif