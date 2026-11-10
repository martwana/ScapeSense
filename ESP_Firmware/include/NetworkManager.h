#pragma once

#include "ConfigManager.h"
#include <ESPAsyncWebServer.h>
#include <NetWizard.h>
#include <WiFi.h>

class ScapeSenseNetworkManager {
public:
  ScapeSenseNetworkManager(ConfigManager &config, AsyncWebServer &server);

  void begin();
  void update();

  bool isConnected() const;
  bool isProvisioning() const;
  String localIP() const;
  String apSsid() const;
  String apPassword() const;
  int apClientCount() const;
  void clearWifiCredentials();

private:
  ConfigManager &config_;
  AsyncWebServer &server_;
  NetWizard netWizard_;
  String apSsid_;
  String apPassword_;
  bool connected_;
};
