/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ESP8266 library for connecting to strongest of multiple WiFi networks, 
    automatic scanning, and creating APs.

    To use, initialize a WiFi_Manager object. Call add_networks() function at 
    least once to add one or more networks, then call begin() function to attempt 
    to connect. Call handle() function in the loop, and call create_hotspot()
    function to create an access point. Call begin() again to try and connect to 
    WiFi networks. 

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "ESP8266WiFi.h" //WiFi Library"
#include "WiFi_Manager.h"

String SSID_list[32]; //WiFi Network Name
String password_list[32]; //WiFi Network Password
uint8_t networks = 0; //Number of networks stored

WiFiEventHandler connected_handler;
WiFiEventHandler disconnected_handler;

static void_function_pointer _connected_callback; //Callback function when connected
static void_function_pointer _disconnected_callback; //Callback function when disconnected
static void_function_pointer _connection_failed_callback; //Callback function when disconnected

wm_status status = WM_IDLE; //Wifi Manager Status

uint32_t timeout; //Timeout for connecting to a found WiFi Network
uint32_t connect_start = 0; //Stores the millis() when we first started attempting to connect

/*  WiFi_Manager constructor (with defaults)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
WiFi_Manager::WiFi_Manager(){
    config(10000);

    //When connected, change status to WM_CONNECTION SUCCESS
    connected_handler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event){
        status = WM_CONNECTION_SUCCESS;
    });

    //When disconnecteed, change status to connection_lost if previous status was connected 
    disconnected_handler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event){
        if(status == WM_CONNECTED) status = WM_CONNECTION_LOST;
    });
}

/*  config
        timeout_in: Timeout in ms before connection considered failed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void WiFi_Manager::config(uint32_t timeout_in){
    timeout = timeout_in;
}

/*  set_callbacks:
        connected_callback: Function to call when successfully connected
        disconnected_callback: Function to call when disconnected
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void WiFi_Manager::set_callbacks(void_function_pointer connected_callback, void_function_pointer disconnected_callback, void_function_pointer connection_failed_callback){
    _connected_callback = connected_callback;
    _disconnected_callback = disconnected_callback;
    _connection_failed_callback = connection_failed_callback;
}

/*  (private) connect: Attempt to connect to a network
        ID: ID of network to attempt connection to
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void connect(uint8_t ID){
    //Change status to WM_CONNECTING
    status = WM_CONNECTING;
    //Attempt to connect to network
    if(password_list[ID] == ""){
        WiFi.begin(SSID_list[ID]);
    }else{
        WiFi.begin(SSID_list[ID], password_list[ID]);
    }
    
    //Record the time connection attempt started
    connect_start = millis(); 
}

/*  (private) process_scan: Process a completed scan for Wi-Fi Networks and
        connect if a nown network is found
        networks_found: Number of networks found
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void process_scan(int networks_found){
    //Change status to idle in case the scan failes
    status = WM_IDLE;

    //set the network ID to an impossible number
    int8_t network_ID = -1; 
    //set the RSSI to an impossible number
    int32_t RSSI = -1000; 

    //For each network found...
    for(int i=0; i<networks_found; i++){
        
        //For each network in our database...
        for(int x=0; x<networks; x++){

            //See if there is a match
            if(WiFi.SSID(i) == SSID_list[x]){

                //If this match has a higher RSSI than the last match, record the network ID
                if(WiFi.RSSI(i) >= RSSI) network_ID = x;

            }
        }
    }

    //If a matching network has been found, connect to the strongest one
    if(network_ID >= 0) connect(network_ID);

}

/*  (private) scan: Begin scanning for known networks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void scan(){
    //Update the status
    status = WM_SCANNING; 
    //Change WiFi to station mode
    WiFi.mode(WIFI_STA);
    //Disconnect from any current connections
    WiFi.disconnect();  
    //Begin background scan, call process_scan function when complete
    WiFi.scanNetworksAsync(process_scan);
}

/*  begin: Start the WiFi Manager and attempt to connect to a network.
    RETURNS True if known network is found, false if not.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool WiFi_Manager::begin(){
    //Initiate a scan
    scan();
    //Wait for scan or connection to finish
    while(status == WM_SCANNING) yield();

    //If no known networks are found, try again. On second fail, return false. Otherwise, return true. 
    if(status != WM_CONNECTING){
        //Initiate a scan
        scan();
        //Wait for scan or connection to finish
        while(status == WM_SCANNING) yield();

        if(status != WM_CONNECTING) return false;
    }

    return true;
}

/*  create_hotspot: Create a WiFi network.
        hotspot_SSID: Network name
        hotspot_password: Network password
    RETURNS True if hotspot established, false if not.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool WiFi_Manager::create_hotspot(String hotspot_SSID, String hotspot_password){
    //Switch to Access Point mode
    WiFi.mode(WIFI_AP);
    //Begin the hotspot
    if(WiFi.softAP(hotspot_SSID, hotspot_password)){
        status = WM_HOTSPOT;
        return true; //Return true if started
    }
    
    return false; //Return false if it failed to start
}

/*  handle: Handler function, run every loop or as often as possible
    RETURNS current status of WiFi as wm_status
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
wm_status WiFi_Manager::handle(){
    //If status is idle, initiate another scan
    if(status == WM_IDLE){
        scan();
    //If a connection hasn't been established in the timeout, call the connection_failed_callback and set status to idle
    }else if(status == WM_CONNECTING && millis() > connect_start + timeout){
        _connection_failed_callback();
        status = WM_IDLE;
    //If a connection has succeeded, call the connected callback and change the status to connected
    }else if(status == WM_CONNECTION_SUCCESS){
        _connected_callback();
        status = WM_CONNECTED;
    //If the connection has been lost, call the disconnected callback and change the status to idle
    }else if(status == WM_CONNECTION_LOST){
        _disconnected_callback();
        status = WM_IDLE;
    }

    return status;
}

/*  get_SSID: 
    RETURNS SSID of current network
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String WiFi_Manager::get_SSID(){
  return WiFi.SSID();
}

/*  add_network: Add Network to list of networks
        SSID: Network name
        password: Network password
    RETURNS True if successfully added, False if not
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool WiFi_Manager::add_network(String SSID, String password){
  //Return false if maximum number of networks reached
  if(networks == 32) return false; 

  //Store the network details
  SSID_list[networks] = SSID;
  password_list[networks] = password;
  
  //Increase the number of networks
  networks++;
  return true;
}



