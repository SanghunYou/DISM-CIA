#include "IOManager.h"

IOManager io;

void IOManager::begin() {
  pinMode(PIN_BED_LOW, INPUT_PULLUP);
  pinMode(PIN_BED_HIGH, INPUT_PULLUP);
  pinMode(PIN_TANK_LOW, INPUT_PULLUP);
  pinMode(PIN_TANK_HIGH, INPUT_PULLUP);

  pinMode(PIN_EMERGENCY, INPUT_PULLUP);
  pinMode(PIN_BTN_FILL, INPUT_PULLUP);
  pinMode(PIN_BTN_DRAIN, INPUT_PULLUP);
  pinMode(PIN_BTN_FAN, INPUT_PULLUP);

  pinMode(RELAY_VALVE, OUTPUT);
  pinMode(RELAY_FILL_PUMP, OUTPUT);
  pinMode(RELAY_DRAIN_PUMP, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);

  allRelaysOff();
}

/*
  LOGICA ACTUAL DEL PARO FISICO:
  INPUT_PULLUP:
  - LOW  = presionado / paro activo
  - HIGH = libre

  Si tu paro fisico es NC y trabaja al reves,
  cambia LOW por HIGH en esta funcion.
*/
bool IOManager::isEmergencyPressed() {
  return digitalRead(PIN_EMERGENCY) == LOW;
}

bool IOManager::isFillButtonPressed() {
  return digitalRead(PIN_BTN_FILL) == LOW;
}

bool IOManager::isDrainButtonPressed() {
  return digitalRead(PIN_BTN_DRAIN) == LOW;
}

bool IOManager::isFanButtonPressed() {
  return digitalRead(PIN_BTN_FAN) == LOW;
}

bool IOManager::rawBedLowClosed() {
  return digitalRead(PIN_BED_LOW) == LOW;
}

bool IOManager::rawBedHighClosed() {
  return digitalRead(PIN_BED_HIGH) == LOW;
}

bool IOManager::rawTankLowClosed() {
  return digitalRead(PIN_TANK_LOW) == LOW;
}

bool IOManager::rawTankHighClosed() {
  return digitalRead(PIN_TANK_HIGH) == LOW;
}

/*
  Relés con etapa optoacoplada activa LOW.

  ON:
    OUTPUT + LOW

  OFF:
    OUTPUT + HIGH breve, luego INPUT.
    Esto libera el GPIO y evita que el opto PC817 quede parcialmente activado.
*/
void IOManager::setRelay(int pin, bool on) {
  if (on) {
    // ON = salida LOW porque el circuito es activo LOW
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  } else {
    // OFF = liberar completamente el GPIO
    // No mandamos HIGH
    pinMode(pin, INPUT);
  }
}

void IOManager::allRelaysOff() {
  digitalWrite(RELAY_VALVE, HIGH);
  digitalWrite(RELAY_FILL_PUMP, HIGH);
  digitalWrite(RELAY_DRAIN_PUMP, HIGH);
  digitalWrite(RELAY_FAN, HIGH);
}

bool IOManager::isRelayOn(int pin) {
  return digitalRead(pin) == LOW;
}