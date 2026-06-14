#include "WebServerManager.h"

WebServerManager webServerManager;

WebServerManager::WebServerManager()
  : server(80)
{
}

void WebServerManager::begin() {
  if (!LittleFS.begin(true)) {
    Serial.println("Error montando LittleFS");
  } else {
    Serial.println("LittleFS montado correctamente");
  }

  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("WiFi AP iniciado");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("Password: ");
  Serial.println(WIFI_PASSWORD);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  setupRoutes();
  server.begin();

  Serial.println("Servidor web iniciado");
}

void WebServerManager::handleClient() {
  server.handleClient();
}

void WebServerManager::setupRoutes() {
  server.on("/api/status", HTTP_GET, [this]() {
    handleStatus();
  });

  server.on("/api/setMode", HTTP_GET, [this]() {
    handleSetMode();
  });

  server.on("/api/manual", HTTP_GET, [this]() {
    handleManualCommand();
  });

  server.on("/api/setThresholds", HTTP_GET, [this]() {
    handleSetThresholds();
  });

  server.on("/api/startAuto", HTTP_GET, [this]() {
    handleStartAuto();
  });

  server.on("/api/stop", HTTP_GET, [this]() {
    handleStop();
  });

  server.on("/api/reset", HTTP_GET, [this]() {
    handleReset();
  });

  server.onNotFound([this]() {
    handleFileRequest();
  });
}

void WebServerManager::handleFileRequest() {
  String path = server.uri();

  if (path == "/") {
    path = "/index.html";
  }

  if (!LittleFS.exists(path)) {
    server.send(404, "text/plain", "Archivo no encontrado");
    return;
  }

  File file = LittleFS.open(path, "r");
  server.streamFile(file, getContentType(path));
  file.close();
}

String WebServerManager::getContentType(String path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".jpg")) return "image/jpeg";
  if (path.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

void WebServerManager::handleStatus() {
  SensorData data = sensors.getData();
  ActuatorStatus actuatorStatus = actuators.getStatus();

  String json = "{";

  json += "\"mode\":\"";
  json += stateMachine.getModeName();
  json += "\",";

  json += "\"state\":\"";
  json += stateMachine.getStateName();
  json += "\",";

  json += "\"manualStep\":\"";
  json += stateMachine.getManualStepName();
  json += "\",";

  json += "\"alert\":\"";
  json += stateMachine.getAlertMessage();
  json += "\",";

  json += "\"autoStarted\":";
  json += stateMachine.isAutoStarted() ? "true" : "false";
  json += ",";

  json += "\"isPhysicalEmergency\":";
  json += io.isEmergencyPressed() ? "true" : "false";
  json += ",";

  json += "\"isWebStopRequested\":";
  json += stateMachine.isWebStopRequested() ? "true" : "false";
  json += ",";

  json += "\"isEmergencyActive\":";
  json += stateMachine.isEmergencyActive() ? "true" : "false";
  json += ",";

  json += "\"isErrorActive\":";
  json += stateMachine.isErrorActive() ? "true" : "false";
  json += ",";

  json += "\"emergencySource\":\"";
  if (io.isEmergencyPressed()) {
    json += "physical";
  } else if (stateMachine.isWebStopRequested()) {
    json += "web";
  } else {
    json += "none";
  }
  json += "\",";

  json += "\"soilAverage\":";
  json += data.soilAveragePercent;
  json += ",";

  json += "\"thresholds\":{";
  json += "\"fillStart\":";
  json += stateMachine.getAutoFillStartHumidity();
  json += ",\"irrigationTarget\":";
  json += stateMachine.getAutoIrrigationTargetHumidity();
  json += ",\"ventEnd\":";
  json += stateMachine.getAutoVentEndHumidity();
  json += "},";

  json += "\"soil\":[";
    for (int i = 0; i < SOIL_SENSOR_COUNT; i++) {
      if (i > 0) {
        json += ",";
      }
      json += "{";
      json += "\"pin\":";
      json += SOIL_SENSOR_PINS[i];
      json += ",\"raw\":";
      json += data.soilRaw[i];
      json += ",\"percent\":";
      json += data.soilPercent[i];
      json += "}";
    }
    json += "],";

  json += "\"levels\":{";
  json += "\"bedLow\":";
  json += data.bedLow ? "true" : "false";
  json += ",\"bedHigh\":";
  json += data.bedHigh ? "true" : "false";
  json += ",\"tankHasMinimumWater\":";
  json += data.tankHasMinimumWater ? "true" : "false";
  json += ",\"tankHigh\":";
  json += data.tankHigh ? "true" : "false";
  json += ",\"bedLevelError\":";
  json += data.bedLevelError ? "true" : "false";
  json += ",\"tankLevelError\":";
  json += data.tankLevelError ? "true" : "false";
  json += "},";

  json += "\"buttons\":{";
  json += "\"emergency\":";
  json += io.isEmergencyPressed() ? "true" : "false";
  json += ",\"fill\":";
  json += io.isFillButtonPressed() ? "true" : "false";
  json += ",\"drain\":";
  json += io.isDrainButtonPressed() ? "true" : "false";
  json += ",\"fan\":";
  json += io.isFanButtonPressed() ? "true" : "false";
  json += "},";

  json += "\"actuators\":{";
  json += "\"valve\":";
  json += actuatorStatus.valveOn ? "true" : "false";
  json += ",\"fillPump\":";
  json += actuatorStatus.fillPumpOn ? "true" : "false";
  json += ",\"drainPump\":";
  json += actuatorStatus.drainPumpOn ? "true" : "false";
  json += ",\"fan\":";
  json += actuatorStatus.fanOn ? "true" : "false";
  json += "},";

  json += "\"dht\":{";
  json += "\"error\":";
  json += data.dhtError ? "true" : "false";
  json += ",\"humidity\":";
  json += data.airHumidity;
  json += ",\"temperature\":";
  json += data.airTemperature;
  json += "}";

  json += "}";

  server.send(200, "application/json", json);
}

void WebServerManager::handleSetMode() {
  if (!server.hasArg("mode")) {
    server.send(400, "text/plain", "Falta parametro mode");
    return;
  }

  String mode = server.arg("mode");

  if (mode == "auto") {
    stateMachine.setMode(MODE_AUTO);
    server.send(200, "text/plain", "Modo automatico activado");
    return;
  }

  if (mode == "manual") {
    stateMachine.setMode(MODE_MANUAL);
    server.send(200, "text/plain", "Modo manual activado");
    return;
  }

  server.send(400, "text/plain", "Modo no valido");
}

void WebServerManager::handleManualCommand() {
  if (!server.hasArg("action")) {
    server.send(400, "text/plain", "Falta parametro action");
    return;
  }

  String action = server.arg("action");

  if (action == "fill") {
    stateMachine.setWebManualCommand(MANUAL_CMD_FILL);
    server.send(200, "text/plain", "Comando manual: llenar");
    return;
  }

  if (action == "drain") {
    stateMachine.setWebManualCommand(MANUAL_CMD_DRAIN);
    server.send(200, "text/plain", "Comando manual: drenar");
    return;
  }

  if (action == "fan") {
    stateMachine.setWebManualCommand(MANUAL_CMD_FAN);
    server.send(200, "text/plain", "Comando manual: ventilar");
    return;
  }

  if (action == "clear") {
    stateMachine.clearWebManualCommand();
    server.send(200, "text/plain", "Comando manual liberado");
    return;
  }

  server.send(400, "text/plain", "Accion no valida");
}

void WebServerManager::handleSetThresholds() {
  if (!server.hasArg("fillStart") ||
      !server.hasArg("irrigationTarget") ||
      !server.hasArg("ventEnd")) {
    server.send(400, "text/plain", "Faltan parametros de umbrales");
    return;
  }

  int fillStart = server.arg("fillStart").toInt();
  int irrigationTarget = server.arg("irrigationTarget").toInt();
  int ventEnd = server.arg("ventEnd").toInt();

  if (fillStart < 0 || fillStart > 100 ||
      irrigationTarget < 0 || irrigationTarget > 100 ||
      ventEnd < 0 || ventEnd > 100) {
    server.send(400, "text/plain", "Los umbrales deben estar entre 0 y 100");
    return;
  }

  if (fillStart >= irrigationTarget) {
    server.send(400, "text/plain", "Inicio de llenado debe ser menor que objetivo de riego");
    return;
  }

  if (ventEnd >= irrigationTarget) {
    server.send(400, "text/plain", "Fin de ventilacion debe ser menor que objetivo de riego");
    return;
  }

  stateMachine.setAutoThresholds(fillStart, irrigationTarget, ventEnd);
  server.send(200, "text/plain", "Umbrales actualizados");
}


void WebServerManager::handleStartAuto() {
  stateMachine.startAutoMode();
  server.send(200, "text/plain", "Modo automatico iniciado");
}

void WebServerManager::handleStop() {
  stateMachine.requestWebStop();
  server.send(200, "text/plain", "Paro solicitado");
}

void WebServerManager::handleReset() {
  stateMachine.resetSystem();
  server.send(200, "text/plain", "Sistema reiniciado");
}