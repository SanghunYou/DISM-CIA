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
  drainExtraTimeActive = false;

  autoFillStartHumidity = DEFAULT_FILL_START_HUMIDITY;
  autoIrrigationTargetHumidity = DEFAULT_IRRIGATION_TARGET;
  autoVentEndHumidity = DEFAULT_VENT_END_HUMIDITY;

  actuators.allOff();
}

void StateMachine::update() {
  SensorData data = sensors.getData();

  if (io.isEmergencyPressed()) {
    enterEmergency("Paro de emergencia fisico activo. Desenclava el paro fisico para continuar. Modo y estado conservados.");
    return;
  }

  if (webStopRequested) {
    enterEmergency("Paro solicitado desde pagina web. Presiona Reset/Liberar paro web para continuar. Modo y estado conservados.");
    return;
  }

  if (emergencyActive) {
    emergencyActive = false;
    waitingAllManualReleased = true;
    alertMessage = "Paro liberado. Desenclava todos los botones fisicos antes de continuar. Se conserva el modo y estado previo.";
  }

  if (data.bedLevelError) {
    enterError("Error: sensores de nivel de cama inconsistentes. Revisa flotadores. Modo y estado conservados.");
    return;
  }

  if (data.tankLevelError) {
    enterError("Error: sensores de nivel de deposito inconsistentes. Revisa flotadores. Modo y estado conservados.");
    return;
  }

  if (errorActive) {
    errorActive = false;
    waitingAllManualReleased = true;
    alertMessage = "Error corregido. Desenclava todos los botones fisicos antes de continuar. Se conserva el modo y estado previo.";
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
  drainExtraTimeActive = false;

  if (mode == MODE_AUTO) {
    autoStarted = false;
    currentState = STATE_MONITORING;
    stateStartTime = millis();
    alertMessage = "Modo automatico seleccionado. Presiona EMPEZAR para iniciar. Desenclava todos los botones fisicos.";
  } else {
    autoStarted = false;
    currentState = STATE_MONITORING;
    manualStep = MANUAL_WAIT_FILL;
    stateStartTime = millis();
    alertMessage = "Modo manual activado. Drenado manual disponible sin restriccion de secuencia. Desenclava todos los botones fisicos.";
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

  alertMessage = "Reset aplicado. Desenclava todos los botones fisicos antes de continuar. Se conserva el modo y estado previo.";
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
  drainExtraTimeActive = false;

  alertMessage = "Modo automatico iniciado. Desenclava todos los botones fisicos.";
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
    alertMessage = "Comando web desenclavado.";
  } else {
    webManualCommand = command;

    if (command == MANUAL_CMD_FILL) {
      alertMessage = "Comando web: LLENAR enclavado.";
    } else if (command == MANUAL_CMD_DRAIN) {
      alertMessage = "Comando web: DRENAR enclavado. Drenado manual sin restriccion de secuencia.";
    } else if (command == MANUAL_CMD_FAN) {
      alertMessage = "Comando web: VENTILAR enclavado.";
    }
  }
}

void StateMachine::clearWebManualCommand() {
  webManualCommand = MANUAL_CMD_NONE;
  drainExtraTimeActive = false;
  alertMessage = "Comando manual web liberado.";
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

void StateMachine::runManualDrainUnrestricted(const SensorData& data) {
  currentState = STATE_DRAINING;
  actuators.startDraining();

  if (data.bedLow) {
    if (!drainExtraTimeActive) {
      drainExtraTimeActive = true;
      drainExtraTimeStart = millis();
      alertMessage = "Manual: nivel bajo detectado o cama sin agua. Drenando 1 minuto extra.";
      return;
    }

    unsigned long elapsed = millis() - drainExtraTimeStart;

    if (elapsed >= EXTRA_DRAIN_TIME_MS) {
      drainExtraTimeActive = false;
      finishManualStepAndWaitRelease(MANUAL_WAIT_FAN);
      alertMessage = "Manual: drenado extra completo. Ahora corresponde VENTILAR. Desenclava todos los botones fisicos.";
      return;
    }

    unsigned long remaining = (EXTRA_DRAIN_TIME_MS - elapsed) / 1000;
    alertMessage = "Manual: drenado extra en proceso. Restan aprox. " + String(remaining) + " s.";
    return;
  }

  drainExtraTimeActive = false;
  alertMessage = "Manual: drenando sin restriccion. Esperando sensor de nivel bajo para iniciar el minuto extra.";
}

void StateMachine::runManualMode(const SensorData& data) {
  ManualCommand command = getActiveManualCommand();

  if (physicalManualButtonCount() > 1) {
    actuators.allOff();
    currentState = STATE_MONITORING;
    drainExtraTimeActive = false;
    alertMessage = "Manual: hay mas de un boton fisico enclavado. Desenclava todos los botones fisicos.";
    return;
  }

  if (waitingAllManualReleased) {
    if (anyPhysicalManualButtonPressed() || webManualCommand != MANUAL_CMD_NONE) {
      actuators.allOff();
      currentState = STATE_MONITORING;
      drainExtraTimeActive = false;
      alertMessage = "Manual: desenclava todos los botones fisicos y libera comandos web antes de continuar.";
      return;
    }

    waitingAllManualReleased = false;
    alertMessage = "Manual listo.";
  }

  // DRENADO MANUAL SIN RESTRICCION DE SECUENCIA.
  // Puede activarse desde cualquier paso manual.
  if (command == MANUAL_CMD_DRAIN) {
    runManualDrainUnrestricted(data);
    return;
  }

  // Si se solto el drenado durante el minuto extra, se cancela el contador.
  drainExtraTimeActive = false;

  // Ventilacion auxiliar libre cuando esta esperando llenar.
  if (manualStep == MANUAL_WAIT_FILL && command == MANUAL_CMD_FAN) {
    currentState = STATE_VENTILATING;
    alertMessage = "Manual: ventilacion auxiliar activa.";
    actuators.startVentilating();
    return;
  }

  if (manualStep == MANUAL_WAIT_FILL && currentState == STATE_VENTILATING && command == MANUAL_CMD_NONE) {
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
        alertMessage = "Manual: esperando llenar cama. Drenado disponible sin restriccion.";
        return;
      }

      if (command != MANUAL_CMD_FILL) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: la siguiente accion recomendada es LLENAR. El drenado manual queda permitido sin restriccion.";
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
        alertMessage = "Manual: cama llena. Ahora corresponde DRENAR. Desenclava todos los botones fisicos.";
        return;
      }

      currentState = STATE_FILLING;
      alertMessage = "Manual: llenando cama.";
      actuators.startFilling();
      return;

    case MANUAL_WAIT_DRAIN:
      if (command == MANUAL_CMD_NONE) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: esperando drenar.";
        return;
      }

      if (command != MANUAL_CMD_DRAIN) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: la siguiente accion recomendada es DRENAR.";
        return;
      }
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
        alertMessage = "Manual: esperando ventilar. Drenado disponible sin restriccion.";
        return;
      }

      if (command != MANUAL_CMD_FAN) {
        actuators.allOff();
        currentState = STATE_MONITORING;
        alertMessage = "Manual: la siguiente accion recomendada es VENTILAR. Drenado manual sigue disponible.";
        return;
      }

      currentState = STATE_VENTILATING;
      alertMessage = "Manual: ventilando.";
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
    case STATE_MONITORING: return "Monitoreando humedad";
    case STATE_FILLING: return "Llenando";
    case STATE_CAPILLARY_IRRIGATION: return "Regando por capilaridad";
    case STATE_DRAINING: return "Drenando";
    case STATE_VENTILATING: return "Ventilando";
    case STATE_EMERGENCY: return "Paro";
    case STATE_ERROR: return "Error";
    default: return "Desconocido";
  }
}

const char* StateMachine::modeToString(OperationMode mode) {
  switch (mode) {
    case MODE_MANUAL: return "Manual";
    case MODE_AUTO: return "Automatico";
    default: return "Desconocido";
  }
}

const char* StateMachine::manualStepToString(ManualStep step) {
  switch (step) {
    case MANUAL_WAIT_FILL: return "Llenar cama";
    case MANUAL_WAIT_DRAIN: return "Drenar agua";
    case MANUAL_WAIT_FAN: return "Ventilar";
    default: return "Desconocido";
  }
}
