#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include <Arduino.h>
#include "Config.h"

class IOManager {
public:
  void begin();

  bool isEmergencyPressed();
  bool isFillButtonPressed();
  bool isDrainButtonPressed();
  bool isFanButtonPressed();

  bool rawBedLowClosed();
  bool rawBedHighClosed();
  bool rawTankLowClosed();
  bool rawTankHighClosed();

  void setRelay(int pin, bool on);
  void allRelaysOff();

  bool isRelayOn(int pin);
};

extern IOManager io;

#endif