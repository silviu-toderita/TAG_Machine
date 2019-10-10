#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "ESP8266WiFiMulti.h"
#include "ESP8266mDNS.h"
#include "ESP8266WebServer.h"

ESP8266WiFiMulti wifiMulti;

ESP8266WebServer server(80);

void setupWifi();
void setupMDNS();
void setupServer();

void handleRoot();
void handleNotFound();

const String localAddress = "faxmachine";

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("\n");
  Serial.println("~~~~//// FAX 2.0 INIT \\\\~~~~");

  setupWifi();
  setupMDNS();
  setupServer();

}

void loop() {
  server.handleClient();
  MDNS.update();
}

void setupWifi(){
  wifiMulti.addAP("Azaviu 2.4GHz", "sebastian");
  wifiMulti.addAP("Azaviu", "sebastian");

  Serial.print("Connecting");

  while(wifiMulti.run() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected to ");
  Serial.print(WiFi.SSID());
  Serial.print("! IP Address: ");
  Serial.println(WiFi.localIP());
}

void setupMDNS(){
  if(MDNS.begin(localAddress)){
    Serial.print("mDNS Responder Started! Access console at ");
    Serial.print(localAddress);
    Serial.println(".local");
  }else{
    Serial.println("Error setting up mDNS responder!");
  }
}

void setupServer(){
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("HTTP Server Started");
}

void handleRoot(){
  server.send(200, "text/plain", "Hello World!");
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not Found");
}