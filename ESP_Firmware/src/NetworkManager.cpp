#include "NetworkManager.h"
#include <Preferences.h>
#include <esp_system.h>

namespace {
bool hasNetWizardCredentials() {
  Preferences preferences;
  if (!preferences.begin("nw_creds", true)) {
    return false;
  }
  const String ssid = preferences.getString("ssid", "");
  preferences.end();
  return ssid.length() > 0;
}
} // namespace

ScapeSenseNetworkManager::ScapeSenseNetworkManager(ConfigManager &config,
                                                   AsyncWebServer &server)
    : config_(config), server_(server), netWizard_(&server_),
      apPassword_("scapesense"), connected_(false) {}

void ScapeSenseNetworkManager::begin() {
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X",
           static_cast<unsigned>(ESP.getEfuseMac() >> 32) & 0xFFFF);
  apSsid_ = String("ScapeSense-") + suffix;

  netWizard_.setHostname("scapesense");
  netWizard_.setStrategy(NetWizardStrategy::NON_BLOCKING);
  netWizard_.setConnectTimeout(15000);
  netWizard_.onConnectionStatus([this](NetWizardConnectionStatus status) {
    connected_ = status == NetWizardConnectionStatus::CONNECTED;
    if (connected_) {
      Serial.printf("Connected to Wi-Fi; IP: %s\n",
                    WiFi.localIP().toString().c_str());
    }
  });
  netWizard_.onPortalState([](NetWizardPortalState state) {
    if (state == NetWizardPortalState::SUCCESS) {
      Serial.println("Wi-Fi credentials saved by NetWizard");
    } else if (state == NetWizardPortalState::FAILED ||
               state == NetWizardPortalState::TIMEOUT) {
      Serial.println("NetWizard Wi-Fi setup did not complete");
    }
  });

  if (!hasNetWizardCredentials() && config_.hasWifiCredentials()) {
    netWizard_.setCredentials(config_.wifiSsid(), config_.wifiPassword());
  }
  netWizard_.autoConnect(apSsid_.c_str(), apPassword_.c_str());
}

void ScapeSenseNetworkManager::update() { netWizard_.loop(); }

bool ScapeSenseNetworkManager::isConnected() const {
  return connected_ || WiFi.status() == WL_CONNECTED;
}

bool ScapeSenseNetworkManager::isProvisioning() const {
  const wifi_mode_t mode = WiFi.getMode();
  return mode == WIFI_AP || mode == WIFI_AP_STA;
}

String ScapeSenseNetworkManager::apSsid() const { return apSsid_; }
String ScapeSenseNetworkManager::apPassword() const { return apPassword_; }

int ScapeSenseNetworkManager::apClientCount() const {
  return isProvisioning() ? WiFi.softAPgetStationNum() : 0;
}

String ScapeSenseNetworkManager::localIP() const {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }
  return isProvisioning() ? WiFi.softAPIP().toString() : String();
}

void ScapeSenseNetworkManager::clearWifiCredentials() {
  config_.clearWifiCredentials();
  netWizard_.erase();
}
