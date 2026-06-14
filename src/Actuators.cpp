#include "Actuators.h"

Actuators actuators;

void Actuators::begin() {
  allOff();
}

void Actuators::allOff() {
  setValve(false);
  setFillPump(false);
  setDrainPump(false);
  setFan(false);
}

void Actuators::setValve(bool on) {
  valveOn = on;
  io.setRelay(RELAY_VALVE, on);
}

void Actuators::setFillPump(bool on) {
  fillPumpOn = on;
  io.setRelay(RELAY_FILL_PUMP, on);
}

void Actuators::setDrainPump(bool on) {
  drainPumpOn = on;
  io.setRelay(RELAY_DRAIN_PUMP, on);
}

void Actuators::setFan(bool on) {
  fanOn = on;
  io.setRelay(RELAY_FAN, on);
}

void Actuators::startFilling() {
  // Llenado: solo bomba de llenado.
  setValve(false);
  setDrainPump(false);
  setFan(false);
  setFillPump(true);
}

void Actuators::startDraining() {
  // Drenado: válvula + bomba de vaciado.
  setFillPump(false);
  setFan(false);
  setValve(true);
  setDrainPump(true);
}

void Actuators::startVentilating() {
  // Ventilación: solo ventilador.
  setValve(false);
  setFillPump(false);
  setDrainPump(false);
  setFan(true);
}

ActuatorStatus Actuators::getStatus() {
  ActuatorStatus status;
  status.valveOn = valveOn;
  status.fillPumpOn = fillPumpOn;
  status.drainPumpOn = drainPumpOn;
  status.fanOn = fanOn;
  return status;
}
