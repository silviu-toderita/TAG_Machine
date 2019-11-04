#include "ESP8266WiFi.h" //WiFi Library"
#include "WiFiManager.h"
#include "Counter.h"

char* networkSSID[256]; //WiFi Network Name
char* networkPassword[256]; //WiFi Network Password
uint8_t networks = 0;

WiFiEventHandler onConnected, onDisconnected;

wm_status_t status = WM_IDLE;

uint32_t timeout;
uint32_t connectTimer = 0;

WiFiManager::WiFiManager(uint32_t inputTimeout){
  timeout = inputTimeout;

  onConnected = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    status = WM_CONNECTION_SUCCESS;
  });

  onDisconnected = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  {
    if(status == WM_CONNECTED) status = WM_CONNECTION_LOST;
  });
}

//Connect to the WiFi network. Returns true if connection can be made within the specified timeout.
void connect(uint8_t ID){
  status = WM_CONNECTING;
  WiFi.begin(networkSSID[ID], networkPassword[ID]); //Start the WiFi connection
  connectTimer = millis();
}

void processScan(int networksFound){
  status = WM_SCAN_FAILED;

  int8_t networkID = -1;
  int32_t RSSI = -1000;
  for(int i=0; i<networksFound; i++){

    for(int x=0; x<networks; x++){

      if(WiFi.SSID(i) == networkSSID[x]){

        if(WiFi.RSSI(i) >= RSSI) networkID = x;
      }
    }
  }

  if(networkID >= 0) connect(networkID);
}

//Scan for known wifi networks
void scan(){
  status = WM_SCANNING;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.scanNetworksAsync(processScan);
}

bool WiFiManager::begin(){
  scan();
  while(status == WM_SCANNING){
    yield();
  }
  if(status == WM_SCAN_FAILED) return false;
  return true;
}

//Create a Hotspot
bool WiFiManager::createHotspot(const char* hotspotSSID, const char* hotspotPassword){
  WiFi.mode(WIFI_AP);
  if(WiFi.softAP(hotspotSSID, hotspotPassword)){
    status = WM_HOTSPOT;
    return true; //Start the access point
  }
  
  return false;
}

wm_status_t WiFiManager::handle(){
  if(status == WM_CONNECTING && millis() > connectTimer + timeout){
    status = WM_CONNECTION_FAILED;
  }else if(status == WM_IDLE || status == WM_SCAN_FAILED){
    scan();
  }

  return status;
}

void WiFiManager::setIdle(){
  status = WM_IDLE;
}

void WiFiManager::setConnected(){
  status = WM_CONNECTED;
}

String WiFiManager::SSID(){
  return WiFi.SSID();
}

bool WiFiManager::addNetwork(char* SSID, char* password){
  if(networks == 255) return false;
  networkSSID[networks] = SSID;
  networkPassword[networks] = password;
  networks++;
  return true;
}