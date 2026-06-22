#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include "Sensors.h"
#include "Actuators.h"
#include "IOManager.h"
#include "Config.h"

enum OperationMode {
  MODE_MANUAL,
  MODE_AUTO
};

enum SystemState {
  STATE_MONITORING,
  STATE_FILLING,
  STATE_CAPILLARY_IRRIGATION,
  STATE_DRAINING,
  STATE_VENTILATING,
  STATE_EMERGENCY,
  STATE_ERROR
};

enum ManualStep {
  MANUAL_WAIT_FILL,
  MANUAL_WAIT_DRAIN,
  MANUAL_WAIT_FAN
};

enum ManualCommand {
  MANUAL_CMD_NONE,
  MANUAL_CMD_FILL,
  MANUAL_CMD_DRAIN,
  MANUAL_CMD_FAN
};

class StateMachine {
public:
  void begin();
  void update();

  void setMode(OperationMode mode);
  OperationMode getMode();
  SystemState getState();
  ManualStep getManualStep();

  const char* getStateName();
  const char* getModeName();
  const char* getManualStepName();
  String getAlertMessage();

  void resetSystem();
  void startAutoMode();

  void setWebManualCommand(ManualCommand command);
  void clearWebManualCommand();
  void requestWebStop();
  ManualCommand getWebManualCommand();

  bool isWebStopRequested();
  bool isAutoStarted();
  bool isEmergencyActive();
  bool isErrorActive();
  bool isFaultActive();

  void setAutoThresholds(int fillStart, int irrigationTarget, int ventEnd);
  int getAutoFillStartHumidity();
  int getAutoIrrigationTargetHumidity();
  int getAutoVentEndHumidity();

private:
  OperationMode currentMode = MODE_MANUAL;
  SystemState currentState = STATE_MONITORING;
  ManualStep manualStep = MANUAL_WAIT_FILL;
  ManualCommand webManualCommand = MANUAL_CMD_NONE;

  bool webStopRequested = false;
  bool autoStarted = false;
  bool emergencyActive = false;
  bool errorActive = false;

  bool drainExtraTimeActive = false;
  unsigned long drainExtraTimeStart = 0;

  int autoFillStartHumidity = DEFAULT_FILL_START_HUMIDITY;
  int autoIrrigationTargetHumidity = DEFAULT_IRRIGATION_TARGET;
  int autoVentEndHumidity = DEFAULT_VENT_END_HUMIDITY;

  unsigned long stateStartTime = 0;
  String alertMessage = "";
  bool waitingAllManualReleased = false;

  void changeState(SystemState newState);
  void enterEmergency(const String& message);
  void enterError(const String& message);

  void runManualMode(const SensorData& data);
  void runAutoMode(const SensorData& data);
  void runManualDrainUnrestricted(const SensorData& data);

  bool timeoutExceeded(unsigned long maxTime);

  bool physicalManualFill();
  bool physicalManualDrain();
  bool physicalManualFan();
  bool anyPhysicalManualButtonPressed();
  int physicalManualButtonCount();
  ManualCommand getActiveManualCommand();

  void finishManualStepAndWaitRelease(ManualStep nextStep);

  const char* stateToString(SystemState state);
  const char* modeToString(OperationMode mode);
  const char* manualStepToString(ManualStep step);
};

extern StateMachine stateMachine;

#endif
