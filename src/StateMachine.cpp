#include "StateMachine.h"

StateMachine stateMachine;

void StateMachine::begin() {
  currentMode = MODE_MANUAL;
  currentState = STATE_MONITORING;
  manualStep = MANUAL_WAIT_FILL;

  stateStartTime = millis();
  alertMessage = "";

  webManualCommand = MANUAL_CMD_NONE;
  webStopRequested = false;
  autoStarted = false;
  emergencyActive = false;
  errorActive = false;

  waitingAllManualReleased = false;

  autoFillStartHumidity = DEFAULT_FILL_START_HUMIDITY;
  autoIrrigationTargetHumidity = DEFAULT_IRRIGATION_TARGET;
  autoVentEndHumidity = DEFAULT_VENT_END_HUMIDITY;

  actuators.allOff();
}

void StateMachine::update() {
  SensorData data = sensors.getData();

  // Paro fisico: pausa el sistema sin cambiar modo ni estado.
  if (io.isEmergencyPressed()) {
    enterEmergency("Paro de emergencia fisico activo. Modo y estado conservados.");
    return;
  }

  // Paro web: pausa el sistema sin cambiar modo ni estado.
  if (webStopRequested) {
    enterEmergency("Paro solicitado desde pagina web. Modo y estado conservados.");
    return;
  }

  // Si el paro ya fue liberado, se permite continuar desde el mismo modo/estado.
  if (emergencyActive) {
    emergencyActive = false;
    waitingAllManualReleased = true;
    alertMessage = "Paro liberado. Se conserva el modo y estado previo.";
  }

  // Error de nivel: pausa el sistema sin cambiar modo ni estado.
  if (data.bedLevelError) {
    enterError("Error: sensores de nivel de cama inconsistentes. Modo y estado conservados.");
    return;
  }

  if (data.tankLevelError) {
    enterError("Error: sensores de nivel de deposito inconsistentes. Modo y estado conservados.");
    return;
  }

  // Si el error ya desaparecio, se permite continuar desde el mismo modo/estado.
  if (errorActive) {
    errorActive = false;
    waitingAllManualReleased = true;
    alertMessage = "Error corregido. Se conserva el modo y estado previo.";
  }

  if (currentMode == MODE_AUTO) {
    runAutoMode(data);
  } else {
    runManualMode(data);
  }
}

void StateMachine::setMode(OperationMode mode) {
  actuators.allOff();

  currentMode = mode;
  webManualCommand = MANUAL_CMD_NONE;
  webStopRequested = false;
  emergencyActive = false;
  errorActive = false;
  waitingAllManualReleased = true;

  if (mode == MODE_AUTO) {
    autoStarted = false;
    currentState = STATE_MONITORING;
    stateStartTime = millis();
    alertMessage = "Modo automatico seleccionado. Presiona EMPEZAR para iniciar.";
  } else {
    autoStarted = false;
    currentState = STATE_MONITORING;
    manualStep = MANUAL_WAIT_FILL;
    stateStartTime = millis();
    alertMessage = "Modo manual activado.";
  }
}

OperationMode StateMachine::getMode() {
  return currentMode;
}

SystemState StateMachine::getState() {
  return currentState;
}

ManualStep StateMachine::getManualStep() {
  return manualStep;
}

const char* StateMachine::getStateName() {
  return stateToString(currentState);
}

const char* StateMachine::getModeName() {
  return modeToString(currentMode);
}

const char* StateMachine::getManualStepName() {
  return manualStepToString(manualStep);
}

String StateMachine::getAlertMessage() {
  return alertMessage;
}

void StateMachine::resetSystem() {
  actuators.allOff();
  drainExtraTimeActive = false;

  webManualCommand = MANUAL_CMD_NONE;
  webStopRequested = false;
  emergencyActive = false;
  errorActive = false;
  waitingAllManualReleased = true;

  // Importante: no se cambia currentMode, currentState ni manualStep.
  alertMessage = "Reset aplicado. Se conserva el modo y estado previo.";

  Serial.println("Reset aplicado sin reiniciar modo ni estado.");
}

void StateMachine::startAutoMode() {
  actuators.allOff();

  currentMode = MODE_AUTO;
  autoStarted = true;
  webManualCommand = MANUAL_CMD_NONE;
  webStopRequested = false;
  emergencyActive = false;
  errorActive = false;
  waitingAllManualReleased = true;

  alertMessage = "Modo automatico iniciado.";
  Serial.println("Modo automatico iniciado por boton START.");
}

void StateMachine::setWebManualCommand(ManualCommand command) {
  if (currentMode != MODE_MANUAL) {
    alertMessage = "Los comandos manuales web solo funcionan en modo manual.";
    return;
  }

  if (emergencyActive || errorActive || webStopRequested) {
    alertMessage = "No se aceptan comandos manuales durante paro o error.";
    return;
  }

  if (webManualCommand == command) {
    webManualCommand = MANUAL_CMD_NONE;
  } else {
    webManualCommand = command;
  }
}

void StateMachine::clearWebManualCommand() {
  webManualCommand = MANUAL_CMD_NONE;
}

void StateMachine::requestWebStop() {
  webStopRequested = true;
}

ManualCommand StateMachine::getWebManualCommand() {
  return webManualCommand;
}

bool StateMachine::isWebStopRequested() {
  return webStopRequested;
}

bool StateMachine::isAutoStarted() {
  return autoStarted;
}

bool StateMachine::isEmergencyActive() {
  return emergencyActive;
}

bool StateMachine::isErrorActive() {
  return errorActive;
}

bool StateMachine::isFaultActive() {
  return emergencyActive || errorActive || webStopRequested;
}

void StateMachine::setAutoThresholds(int fillStart, int irrigationTarget, int ventEnd) {
  fillStart = constrain(fillStart, 0, 100);
  irrigationTarget = constrain(irrigationTarget, 0, 100);
  ventEnd = constrain(ventEnd, 0, 100);

  if (fillStart >= irrigationTarget) {
    alertMessage = "Error: inicio de llenado debe ser menor que objetivo de riego.";
    return;
  }

  if (ventEnd >= irrigationTarget) {
    alertMessage = "Error: fin de ventilacion debe ser menor que objetivo de riego.";
    return;
  }

  autoFillStartHumidity = fillStart;
  autoIrrigationTargetHumidity = irrigationTarget;
  autoVentEndHumidity = ventEnd;

  alertMessage = "Umbrales automaticos actualizados.";
}

int StateMachine::getAutoFillStartHumidity() {
  return autoFillStartHumidity;
}

int StateMachine::getAutoIrrigationTargetHumidity() {
  return autoIrrigationTargetHumidity;
}

int StateMachine::getAutoVentEndHumidity() {
  return autoVentEndHumidity;
}

void StateMachine::changeState(SystemState newState) {
  actuators.allOff();
  currentState = newState;
  stateStartTime = millis();

  if (newState != STATE_DRAINING) {
  drainExtraTimeActive = false;
  }

  Serial.print("Cambio de estado: ");
  Serial.println(getStateName());
}

void StateMachine::enterEmergency(const String& message) {
  actuators.allOff();

  emergencyActive = true;
  errorActive = false;
  drainExtraTimeActive = false;
  webManualCommand = MANUAL_CMD_NONE;
  waitingAllManualReleased = true;

  alertMessage = message;
  Serial.println(message);
}

void StateMachine::enterError(const String& message) {
  actuators.allOff();

  errorActive = true;
  drainExtraTimeActive = false;
  webManualCommand = MANUAL_CMD_NONE;
  waitingAllManualReleased = true;

  alertMessage = message;
  Serial.println(message);
}

bool StateMachine::timeoutExceeded(unsigned long maxTime) {
  return millis() - stateStartTime >= maxTime;
}

bool StateMachine::physicalManualFill() {
  return io.isFillButtonPressed();
}

bool StateMachine::physicalManualDrain() {
  return io.isDrainButtonPressed();
}

bool StateMachine::physicalManualFan() {
  return io.isFanButtonPressed();
}

bool StateMachine::anyPhysicalManualButtonPressed() {
  return physicalManualFill() || physicalManualDrain() || physicalManualFan();
}

int StateMachine::physicalManualButtonCount() {
  int count = 0;

  if (physicalManualFill()) count++;
  if (physicalManualDrain()) count++;
  if (physicalManualFan()) count++;

  return count;
}

ManualCommand StateMachine::getActiveManualCommand() {
  if (physicalManualButtonCount() > 1) {
    return MANUAL_CMD_NONE;
  }

  if (physicalManualFill()) return MANUAL_CMD_FILL;
  if (physicalManualDrain()) return MANUAL_CMD_DRAIN;
  if (physicalManualFan()) return MANUAL_CMD_FAN;

  return webManualCommand;
}

void StateMachine::finishManualStepAndWaitRelease(ManualStep nextStep) {
  actuators.allOff();
  webManualCommand = MANUAL_CMD_NONE;

  manualStep = nextStep;
  waitingAllManualReleased = true;

  changeState(STATE_MONITORING);
}

void StateMachine::runManualMode(const SensorData& data) {
  ManualCommand command = getActiveManualCommand();

  if (physicalManualButtonCount() > 1) {
    actuators.allOff();
    currentState = STATE_MONITORING;
    alertMessage = "Manual: hay mas de un boton enclavado. Desenclava todos.";
    return;
  }

  if (waitingAllManualReleased) {
    if (anyPhysicalManualButtonPressed() || webManualCommand != MANUAL_CMD_NONE) {
      actuators.allOff();
      currentState = STATE_MONITORING;
      alertMessage = "Manual: desenclava todos los botones antes de continuar.";
      return;
    }

    waitingAllManualReleased = false;
    alertMessage = "";
  }

  // Ventilacion auxiliar libre cuando esta esperando llenar.
  if (manualStep == MANUAL_WAIT_FILL && command == MANUAL_CMD_FAN) {
    currentState = STATE_VENTILATING;
    alertMessage = "Manual: ventilacion auxiliar activa.";
    actuators.startVentilating();
    return;
  }

  if (manualStep == MANUAL_WAIT_FILL &&
      currentState == STATE_VENTILATING &&
      command == MANUAL_CMD_NONE) {
    actuators.allOff();
    currentState = STATE_MONITORING;
    alertMessage = "Manual: ventilacion auxiliar apagada.";
    return;
  }

  switch (manualStep) {
    case MANUAL_WAIT_FILL:
      if (command == MANUAL_CMD_NONE) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: esperando llenar cama.";
        return;
      }

      if (command != MANUAL_CMD_FILL) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: la siguiente accion permitida es LLENAR.";
        return;
      }

      if (!data.tankHasMinimumWater) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: nivel de deposito insuficiente.";
        return;
      }

      if (data.bedHigh) {
        finishManualStepAndWaitRelease(MANUAL_WAIT_DRAIN);
        alertMessage = "Manual: cama llena. Ahora corresponde DRENAR.";
        return;
      }

      currentState = STATE_FILLING;
      alertMessage = "";
      actuators.startFilling();
      return;

    case MANUAL_WAIT_DRAIN:
      if (command == MANUAL_CMD_NONE) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: esperando drenar.";
        drainExtraTimeActive = false;
        return;
      }

      if (command != MANUAL_CMD_DRAIN) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: la siguiente accion permitida es DRENAR.";
        drainExtraTimeActive = false;
        return;
      }

      currentState = STATE_DRAINING;
      actuators.startDraining();

      if (data.bedLow) {
        if (!drainExtraTimeActive) {
          drainExtraTimeActive = true;
          drainExtraTimeStart = millis();
          alertMessage = "Manual: nivel bajo alcanzado. Drenando tiempo extra.";
          return;
        }

        if (millis() - drainExtraTimeStart >= EXTRA_DRAIN_TIME_MS) {
          drainExtraTimeActive = false;
          finishManualStepAndWaitRelease(MANUAL_WAIT_FAN);
          alertMessage = "Manual: drenado extra completo. Ahora corresponde VENTILAR.";
          return;
        }

        alertMessage = "Manual: drenado extra en proceso.";
        return;
      }

      drainExtraTimeActive = false;
      alertMessage = "";
      return;

    case MANUAL_WAIT_FAN:
      if (command == MANUAL_CMD_NONE) {
        actuators.allOff();

        if (currentState == STATE_VENTILATING) {
          webManualCommand = MANUAL_CMD_NONE;
          manualStep = MANUAL_WAIT_FILL;
          waitingAllManualReleased = false;

          changeState(STATE_MONITORING);
          alertMessage = "Manual: ventilacion desenclavada. Secuencia completa.";
          return;
        }

        currentState = STATE_MONITORING;
        alertMessage = "Manual: esperando ventilar.";
        return;
      }

      if (command != MANUAL_CMD_FAN) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: la siguiente accion permitida es VENTILAR.";
        return;
      }

      currentState = STATE_VENTILATING;
      alertMessage = "";
      actuators.startVentilating();
      return;
  }
}

void StateMachine::runAutoMode(const SensorData& data) {
  if (!autoStarted) {
    actuators.allOff();
    alertMessage = "Auto: modo seleccionado. Presiona EMPEZAR para iniciar.";
    return;
  }

  switch (currentState) {
    case STATE_MONITORING:
      actuators.allOff();

      if (data.soilAveragePercent < autoFillStartHumidity) {
        if (!data.tankHasMinimumWater) {
          alertMessage = "Auto: nivel de deposito insuficiente.";
          return;
        }

        if (data.bedHigh) {
          alertMessage = "Auto: cama ya esta en nivel alto, no se puede llenar.";
          return;
        }

        alertMessage = "";
        changeState(STATE_FILLING);
      }
      break;

    case STATE_FILLING:
      actuators.startFilling();

      if (!data.tankHasMinimumWater) {
        enterError("Error: deposito sin agua durante llenado. Modo y estado conservados.");
        return;
      }

      if (data.bedHigh) {
        changeState(STATE_CAPILLARY_IRRIGATION);
        return;
      }

      if (timeoutExceeded(MAX_FILL_TIME_MS)) {
        enterError("Error: tiempo maximo de llenado excedido. Modo y estado conservados.");
        return;
      }
      break;

    case STATE_CAPILLARY_IRRIGATION:
      actuators.allOff();

      if (data.soilAveragePercent >= autoIrrigationTargetHumidity) {
        changeState(STATE_DRAINING);
        return;
      }

      if (timeoutExceeded(MAX_CAPILLARY_TIME_MS)) {
        alertMessage = "Alerta: humedad no alcanzo objetivo; se drena por seguridad.";
        changeState(STATE_DRAINING);
        return;
      }
      break;

    case STATE_DRAINING:
      actuators.startDraining();

      if (data.bedLow) {
        if (!drainExtraTimeActive) {
          drainExtraTimeActive = true;
          drainExtraTimeStart = millis();
          alertMessage = "Auto: nivel bajo alcanzado. Drenando tiempo extra.";
          return;
        }

        if (millis() - drainExtraTimeStart >= EXTRA_DRAIN_TIME_MS) {
          drainExtraTimeActive = false;
          alertMessage = "";
          changeState(STATE_VENTILATING);
          return;
        }

        alertMessage = "Auto: drenado extra en proceso.";
        return;
      }

      drainExtraTimeActive = false;

      if (timeoutExceeded(MAX_DRAIN_TIME_MS)) {
        enterError("Error: tiempo maximo de drenado excedido. Modo y estado conservados.");
        return;
      }

      break;

    case STATE_EMERGENCY:
    case STATE_ERROR:
    default:
      actuators.allOff();
      break;
  }
}

const char* StateMachine::stateToString(SystemState state) {
  switch (state) {
    case STATE_MONITORING:
      return "Monitoreando humedad";

    case STATE_FILLING:
      return "Llenando";

    case STATE_CAPILLARY_IRRIGATION:
      return "Regando por capilaridad";

    case STATE_DRAINING:
      return "Drenando";

    case STATE_VENTILATING:
      return "Ventilando";

    case STATE_EMERGENCY:
      return "Paro";

    case STATE_ERROR:
      return "Error";

    default:
      return "Desconocido";
  }
}

const char* StateMachine::modeToString(OperationMode mode) {
  switch (mode) {
    case MODE_MANUAL:
      return "Manual";

    case MODE_AUTO:
      return "Automatico";

    default:
      return "Desconocido";
  }
}

const char* StateMachine::manualStepToString(ManualStep step) {
  switch (step) {
    case MANUAL_WAIT_FILL:
      return "Llenar cama";

    case MANUAL_WAIT_DRAIN:
      return "Drenar agua";

    case MANUAL_WAIT_FAN:
      return "Ventilar";

    default:
      return "Desconocido";
  }
}
