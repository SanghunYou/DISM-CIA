#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>
#include "Config.h"
#include "IOManager.h"

struct ActuatorStatus {
  bool valveOn;
  bool fillPumpOn;
  bool drainPumpOn;
  bool fanOn;
};

class Actuators {
public:
  void begin();

  void allOff();

  void setValve(bool on);
  void setFillPump(bool on);
  void setDrainPump(bool on);
  void setFan(bool on);

  void startFilling();
  void startDraining();
  void startVentilating();

  ActuatorStatus getStatus();

private:
  bool valveOn = false;
  bool fillPumpOn = false;
  bool drainPumpOn = false;
  bool fanOn = false;
};

extern Actuators actuators;

#endif
