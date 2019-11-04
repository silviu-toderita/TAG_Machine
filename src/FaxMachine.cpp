#include "Arduino.h" //Arduino Library
#include "ArduinoOTA.h" //Over-The-Air Update Library
#include "twilio.h" //Twilio Library
#include "Adafruit_Thermal.h" //Thermal Printer Library
#include "NTPClock.h" //NTP Library
#include "ESP8266WebServer.h" //Web Server Library
#include "ESP8266mDNS.h" //multicast DNS Library
#include "WiFiManager.h"
#include "WiFiClient.h"
#include "WebInterface.h"

NTPClock ntpClock((char*)("time.google.com"), -8); //Create an NTP object with a -7 hour offset
Twilio *twilio; //Create a twilio object
ESP8266WebServer twilio_server(8000); //Create a web server for Twilio API Communication
Adafruit_Thermal printer(&Serial); //Create a Thermal Printer object
WiFiManager wifiManager(10000);
WebInterface webInterface;

const char fingerprint[] = "06 86 86 C0 A0 ED 02 20 7A 55 CC F0 75 BB CF 24 B1 D9 C0 49"; //Twilio fingerprint to authenticate connection
const char* account_sid = "AC6e91a7c91c69b11d1d621f9f09ce0858"; //Twilio SID
const char* auth_token = "9cc8633f6e84411fd69e2a17b3e82258"; //Twilio Authentication Token

const char* deviceName = "faxmachine"; //Device Name
const char* devicePassword = "sebastian"; //Device Password

bool statusTwilio = false;

//Wrap text to a specified amount of characters
String wrapText(String input, uint8_t wrapLength){

  //No need to wrap text if it's less than the wrapLength
  if(input.length() < wrapLength){
    return input;
  }

  String output = ""; //This holds the output
  uint8_t charThisLine = 1; //The character we're at on this line
  uint16_t lastSpace = 0; //The last space we processed

  //Run through this code once for each character in the input
  for(uint16_t i=0; i<input.length(); i++){
    
    
    if(charThisLine == 33){ //Once we reach the 33rd character in a line
      if(input.charAt(i) == ' '){ //If the character is a space, change it to a new line
        output.setCharAt(i, '\n');
        charThisLine = 1;
      }else if(input.charAt(i) == '\n'){ //If the character is a new line, do nothing
        charThisLine = 1;
      }else if(lastSpace <= i - wrapLength){ //If the last space was on the previous line, we're dealing with a really long word and we won't introduce a space
        charThisLine = 1;
      }else{ //If it's any other situation, we change the last space to a new line and record which character we're at on the next line
        output.setCharAt(lastSpace, '\n');
        charThisLine = i - lastSpace;
      }
    }

    if(input.charAt(i) == ' '){ //If this character is a space, record it
      lastSpace = i;
    }

    output += input.charAt(i); //Append the current character to the output string
    charThisLine++; //Record that we're on the next character on this line
  }

  return output;
}

//Print a centered, large, bold, inverse title
void printTitle(String text, uint8_t feed){
  printer.wake();
  printer.justify('C'); 
  printer.boldOn();
  printer.setSize('L'); 
  printer.inverseOn();
  if(text.length() <= 14){
    printer.println(" " + text + " ");
  }else{
    printer.println(wrapText(text, 16));
  }
  if(feed != 0) printer.feed(feed); 
  printer.setDefault();
  printer.sleep();
}

//Print a centered, bold, medium heading
void printHeading(String text, uint8_t feed){
  printer.wake();
  printer.boldOn();
  printer.setSize('M'); 
  printer.justify('C');
  printer.println(wrapText(text, 32));
  if(feed != 0) printer.feed(feed); 
  printer.setDefault();
  printer.sleep();
}

//Print a medium, bold message
void printMessage(String text, uint8_t feed){
  printer.wake();
  printer.boldOn();
  printer.setSize('M'); 
  printer.println(wrapText(text, 32));
  if(feed != 0) printer.feed(feed); 
  printer.setDefault();
  printer.sleep();
}

//Print a small, bold status
void printStatus(String text, uint8_t feed){
  printer.wake();
  printer.boldOn();
  printer.println(wrapText(text, 32));
  if(feed != 0) printer.feed(feed);  
  printer.setDefault();
  printer.sleep();
}

//Print a small, bold, inverse error
void printError(String text, uint8_t feed){
  printer.wake();
  printer.boldOn();
  printer.inverseOn();
  String output = "Error: " + text;
  printer.println(wrapText(output, 32));
  if(feed != 0) printer.feed(feed);  
  printer.setDefault();
  printer.sleep();
}

//Print a timestamp
void printTimestamp(uint8_t feed, bool longDate){
  if(ntpClock.handle()){
    printer.wake();
    //Print a long date or a short date
    if(longDate){
      printer.print(ntpClock.getLongDate()); printer.print(F(" - ")); printer.println(ntpClock.getTime());
    }else{
      printer.print(ntpClock.getShortDate()); printer.print(F(" - ")); printer.println(ntpClock.getTime());
    }
    if(feed != 0) printer.feed(feed); 
    printer.setDefault();
    printer.sleep();
  }
}

//Print a dotted line
void printLine(uint8_t feed){
  printer.wake();
  printer.justify('C'); 
  printer.setSize('S');
  printer.println(F("------------------------"));
  if(feed != 0) printer.feed(feed); 
  printer.setDefault();
  printer.sleep();
}


//Test the Connection to Twilio
bool testTwilio(){
  WiFiClient client;
  if(client.connect("api.twilio.com", 443)){ //If we can resolve Twilio's IP Address, return true
    statusTwilio = true;
    if(ntpClock.begin(1000)){
      printTimestamp(1, true);
    }else{
      printError("Unable to Connect To Time Server!", 1);
    }
    printHeading("Ready to Receive Messages!",0);
    printHeading("(604) 373 - 9569",4);
    return true;
  } //Otherwise, return false
  statusTwilio = false;
  return false;
}

void receiveMessage(){ //This function processes incoming SMS messages
  String number; //Store the number the message is from
  String message; //Store the message body

  for(int i=0; i < twilio_server.args(); ++i){ //Run through the arguments received in the message
    if(twilio_server.argName(i) == "From"){ //Once we reach the "From" Argument, store the number
      number = twilio_server.arg(i);
    }else if(twilio_server.argName(i) == "Body"){ //Once we reach the "Body" Argument, store the message
      message = twilio_server.arg(i);
    }
  }

  //Respond to Twilio to let them know the message was received
  twilio_server.send(200, "application/xml", "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Response></Response>");
  
  //Print the message
  printTitle("MESSAGE",1);

  printTimestamp(0, true);

  printStatus("From: " + number, 1);

  printMessage(message, 0);
  
  printLine(4);
} 

void setIdle(){
  printStatus("Searching for Networks...", 1);
  printHeading("<-- Press Button to Start Hotspot", 3);
  wifiManager.setIdle();
}

void createHotspot(){
  if(wifiManager.createHotspot(deviceName, devicePassword)){
    printStatus("Hotspot Started! ", 0); //Output our status
    printStatus("Network: " + String(deviceName), 0);
    printStatus("Password: " + String(devicePassword), 1);
    printHeading("<-- Press Button to Stop Hotspot", 3);
  }else{
    printError("Unable to Initialize Hotspot!", 0); //Output an error if we can't create the hotspot (not sure when this would ever happen)
    printHeading("<-- Press Button to Retry", 3);
  }
}

void connected(){
  printStatus("WiFi Connected: " + wifiManager.SSID(), 0); //Print the network

  if(!testTwilio()){ //If we establish a connection to Twilio, print out a message
    if(ntpClock.begin(1000)){
      printTimestamp(1, true);
    }else{
      printError("Unable to Connect To Time Server!", 1);
    }
    printError("Unable to Connect To Message Server, Check Internet Connection!", 3);
  }

  wifiManager.setConnected();
}

//#######################
//######## SETUP ########
//#######################

void setup() {
  //Initialize the button and LED
  pinMode(D1, INPUT_PULLUP);
  pinMode(D2, OUTPUT);

  MDNS.begin(deviceName); //Start the mDNS

  //Start the OTA updater
  ArduinoOTA.setHostname(deviceName); 
  ArduinoOTA.setPassword(devicePassword); 
  ArduinoOTA.begin();

  //Create a twilio object
  //twilio = new Twilio(account_sid, auth_token, fingerprint);

  //Create a web server for comunicating with Twilio. Call the function receiveMessage when anything is received at /message
  twilio_server.on("/message", receiveMessage);
  twilio_server.begin();

  //Begin the serial connection to the printer
  Serial.begin(9600);
  printer.begin();
  printer.sleep();
  delay(100);

  //Print the title
  printTitle("FAX MACHINE", 1);

  wifiManager.addNetwork("Azaviu 2.4GHz", "sebastian");
  
  if(!wifiManager.begin()){
    printError("Unable to Find Known Networks!", 0);  
    printStatus("Searching for Networks...", 1);
    printHeading("<-- Press Button to Start Hotspot", 3);
  }
}

//######################
//######## LOOP ########
//######################

void loop() {

  switch(wifiManager.handle()){

    case WM_IDLE: 
        if(!digitalRead(D1)) createHotspot(); //If the button is pressed, create an access point
        break;

    case WM_SCANNING: 
        if(!digitalRead(D1)) createHotspot(); //If the button is pressed, create an access point
        break;
    
    case WM_SCAN_FAILED: break;
    case WM_CONNECTING: break;
          
    case WM_CONNECTION_SUCCESS:
        connected();
        break;

    case WM_CONNECTION_FAILED:
        printError("Unable to Connect To Network!", 0);  
        setIdle();
        break;

    case WM_CONNECTED:
        twilio_server.handleClient(); //Update the Twilio server
        ntpClock.handle(); //Update the clock
        webInterface.handle();
        MDNS.update(); //Run mDNS
        ArduinoOTA.handle(); //Run OTA updater service
    
        if(!statusTwilio) testTwilio(); //If we don't have a connection to Twilio, test it
        break;

    case WM_CONNECTION_LOST:
        statusTwilio = false; //Change the Twilio status
        printTimestamp(0, true); //Print the time and an error
        printError("Lost WiFi Connection!", 0);
        setIdle();
        break;

    case WM_HOTSPOT:
        webInterface.handle();
        MDNS.update(); //Run mDNS
        ArduinoOTA.handle(); //Run OTA updater service
        
        if(!digitalRead(D1)) setIdle();
        break;
  }
}

    

 