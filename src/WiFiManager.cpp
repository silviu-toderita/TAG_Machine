#include "ESP8266WiFi.h" //WiFi Library"
#include "Counter.h" //Silviu's Counter Library
#include "WiFiManager.h"

char* networkSSID[256]; //WiFi Network Name
char* networkPassword[256]; //WiFi Network Password
uint8_t networks = 0; //Number of networks stored

WiFiEventHandler onConnected, onDisconnected; //Event handlers for getting instant connect/disconnect status

wm_status_t status = WM_IDLE; //Wifi Manager Status

uint32_t timeout; //Timeout for connecting to a found WiFi Network
uint32_t connectTimer = 0; //Stores the millis() when we first started attempting to connect

//Constructor for WiFiManager object. Requires timeout in millis for new connections
WiFiManager::WiFiManager(uint32_t inputTimeout){
  timeout = inputTimeout;

  //When we connect, update the status
  onConnected = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    status = WM_CONNECTION_SUCCESS;
  });

  //When we disconnect, update the status
  onDisconnected = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  {
    if(status == WM_CONNECTED) status = WM_CONNECTION_LOST;
  });
}

//Connect to the WiFi network based on network ID.
void connect(uint8_t ID){
  status = WM_CONNECTING; //Change the status
  WiFi.begin(networkSSID[ID], networkPassword[ID]); //Start the WiFi connection
  connectTimer = millis(); //Record the time when we started
}

//Process a finished WiFi scan, input is an integer of how many networks have been found
void processScan(int networksFound){
  status = WM_SCAN_FAILED; //Set the status

  int8_t networkID = -1; //set the network ID to an impossible number
  int32_t RSSI = -1000; //set the RSSI to an impossible number

  //For each network found...
  for(int i=0; i<networksFound; i++){
    
    //For each network in our database...
    for(int x=0; x<networks; x++){

      //See if there is a match
      if(WiFi.SSID(i) == networkSSID[x]){

        //If this match has a higher RSSI than the last match, record the network ID
        if(WiFi.RSSI(i) >= RSSI) networkID = x;
      }
    }
  }

  //If we found a network, connect to it
  if(networkID >= 0) connect(networkID);
}

//Scan for known wifi networks
void scan(){
  status = WM_SCANNING; //Update the status
  WiFi.mode(WIFI_STA); //Change the wifi mode
  WiFi.disconnect();  //Disconnect from any networks we may have automatically connected to
  WiFi.scanNetworksAsync(processScan); //Begin scan, and call processScan function when finished
}

//Initiate the WiFi Manager
bool WiFiManager::begin(){
  //Initiate a scan
  scan();

  //Wait for scan to finish
  while(status == WM_SCANNING) yield();

  //If no known networks are found, try again. On second fail, return false. Otherwise, return true. 
  if(status == WM_SCAN_FAILED){
    scan();
    while(status == WM_SCANNING) yield();
    if(status == WM_SCAN_FAILED) return false;
  }
  return true;
}

//Create a Hotspot
bool WiFiManager::createHotspot(const char* hotspotSSID, const char* hotspotPassword){
  WiFi.mode(WIFI_AP); //Set the wifi mode
  //Begin the hotspot
  if(WiFi.softAP(hotspotSSID, hotspotPassword)){
    status = WM_HOTSPOT;
    return true; //Return true if started
  }
  
  return false; //Return false if it failed to start
}

//Handler function to be run every loop
wm_status_t WiFiManager::handle(){
  //If we haven't connected to a network within the timeout, change the status to WM_CONNECTION_FAILED
  if(status == WM_CONNECTING && millis() > connectTimer + timeout){
    status = WM_CONNECTION_FAILED;

  //If status is idle or a scan just failed, initiate another scan
  }else if(status == WM_IDLE || status == WM_SCAN_FAILED){
    scan();
  }

  return status;
}

//Change the status to idle, to be called after acknowledging a failed connection or lost connection
void WiFiManager::setIdle(){
  status = WM_IDLE;
}

//Change the status to connected, after acknowledgeing a successful connection
void WiFiManager::setConnected(){
  status = WM_CONNECTED;
}

//Get the current SSID of the network we are connected to 
String WiFiManager::SSID(){
  return WiFi.SSID();
}

//Add a network to our list of networks
bool WiFiManager::addNetwork(char* SSID, char* password){
  if(networks == 255) return false; //We can't add another network if we already have 256 
  //Store the network details
  networkSSID[networks] = SSID;
  networkPassword[networks] = password;
  //Increase the number of networks
  networks++;
  return true;
}