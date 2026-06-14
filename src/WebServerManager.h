#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include "Sensors.h"
#include "Actuators.h"
#include "StateMachine.h"
#include "Config.h"

class WebServerManager {
public:
  WebServerManager();

  void begin();
  void handleClient();

private:
  WebServer server;

  void setupRoutes();

  void handleFileRequest();
  String getContentType(String path);

  void handleStatus();
  void handleSetMode();
  void handleManualCommand();
  void handleSetThresholds();
  void handleStartAuto();
  void handleStop();
  void handleReset();
};

extern WebServerManager webServerManager;

#endif