#include "Arduino.h" //Arduino Library
#include "ESP8266WiFi.h" //WiFi Library
#include "ArduinoOTA.h" //Over-The-Air Update Library
#include "twilio.h" //Twilio Library
#include "Adafruit_Thermal.h" //Thermal Printer Library
#include "NTPClock.h" //NTP Library
#include "ESP8266WebServer.h" //Web Server Library
#include "ESP8266mDNS.h" //multicast DNS Library
#include "Counter.h" //Counter Library

NTPClock ntpClock((char*)("time.google.com"), -7); //Create an NTP object with a -7 hour offset
Twilio *twilio; //Create a twilio object
ESP8266WebServer twilio_server(8000); //Create a web server for Twilio API Communication
Adafruit_Thermal printer(&Serial); //Create a Thermal Printer object

const char fingerprint[] = "06 86 86 C0 A0 ED 02 20 7A 55 CC F0 75 BB CF 24 B1 D9 C0 49"; //Twilio fingerprint to authenticate connection
const char* account_sid = "AC6e91a7c91c69b11d1d621f9f09ce0858"; //Twilio SID
const char* auth_token = "9cc8633f6e84411fd69e2a17b3e82258"; //Twilio Authentication Token

const char* deviceName = "faxmachine"; //Device Name
const char* devicePassword = "sebastian"; //Device Password

uint8_t statusWiFi = 0; //0=not connected, 1=connected to network, 2=hotspot mode
bool statusTwilio = false; //Are we connected to Twilio?

const char* wifiNetwork = "Azaviu 2.4GHz"; //WiFi Network Name
const char* wifiPassword = "sebastian"; //WiFi Network Password

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
  }else{
    //print an error message if we don't have a time yet
    printError("Unable to Connect To Time Server!", feed);
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
  IPAddress twilioIP;
  if(WiFi.hostByName("api.twilio.com", twilioIP) == 1){ //If we can resolve Twilio's IP Address, return true
    statusTwilio = true;
    return true;
  } //Otherwise, return false
  statusTwilio = false;
  return false;
}

//Scan for known wifi networks
bool scanWiFi(){
  uint8_t networksFound = WiFi.scanNetworks();

  for(int i=0; i<networksFound; i++){
    if(WiFi.SSID(i) == wifiNetwork){
      return true;
    }
  }
  return false;
}

//Connect to the WiFi network. Returns true if connection can be made within the specified timeout.
bool connectWiFi(uint32_t timeout){
  statusWiFi = 0; //Reset the WiFI Status
  WiFi.mode(WIFI_STA); //Enter Station Mode
  WiFi.begin(wifiNetwork, wifiPassword); //Start the WiFi connection

  Counter counter(timeout);
  while(!counter.status()){ //While within timeout periud
    yield();
    if(WiFi.status() == WL_CONNECTED){ //If we're connected
      statusWiFi = 1; //Change the WiFi Status
      return true; // Return true
    }
  }
  return false; //If we're not connected by the timeout, return false
}

//Create a Hotspot
bool createAP(){
  statusWiFi = 0; //Reset the WiFi Status
  WiFi.mode(WIFI_AP); //Enter Access point mode
  if(WiFi.softAP(deviceName, devicePassword)){ //Start the access point
    printStatus("Hotspot Started! ", 0); //Output our status
    printStatus("Network: " + String(deviceName), 0);
    printStatus("Password: " + String(devicePassword), 1);
    printHeading("<-- Hold Button to Reconnect To WiFi", 3);
    statusWiFi = 2;
    return true;
  }
  printError("Unable to Initialize Hotspot!", 0); //Output an error if we can't create the hotspot (not sure when this would ever happen)
  printHeading("<-- Hold Button to Retry", 3);
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
  twilio = new Twilio(account_sid, auth_token, fingerprint);

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

  //Attempt to connect to WiFi
  if(connectWiFi(10000)){ //If we connect within 10 seconds
    printStatus("WiFi Connected: " + WiFi.SSID(), 0); //Print the network
    ntpClock.begin(1000); //Initialize the NTP Clock
    printTimestamp(1, true); //Print a Time Stamp
    if(testTwilio()){ //Test the twilio connection
      printHeading("Ready to Receive Messages!",0);
      printHeading("(604) 373 - 9569",4);
    }else{
      printError("Unable to Connect To Message Server, Check Internet Connection!", 3);
    }
  }else{ //If Timeout passes and we're not connected, print an error
    printError("Unable to Connect To WiFi!", 0);  
    printStatus("Searching for Networks...", 1);
    printHeading("<-- Hold Button to Start Hotspot", 3);
  }

}

//######################
//######## LOOP ########
//######################

void loop() {
  MDNS.update(); //Run mDNS
  ArduinoOTA.handle(); //Run OTA updater service

  if(WiFi.status() == WL_DISCONNECTED && statusWiFi == 1){ //If we lose connection to the WiFi
    statusWiFi = 0; //Change the WiFi status
    statusTwilio = 0; //Change the Twilio status
    printTimestamp(0, false); //Print the time and an error
    printError("Lost WiFi Connection!", 0);
    printStatus("Searching for Networks...", 1);
    printHeading("<-- Hold Button to Start Hotspot", 3);
  }

  if(statusWiFi == 0){ //If we have no connection
    if(!digitalRead(D1)){ //If the button is pressed, create an access point
      createAP();
    }else if(scanWiFi()){ //If we find a known network, attempt to connect to it
      if(connectWiFi(10000)){ //If we connect within 10 seconds...
        printStatus("WiFi Connected: " + WiFi.SSID(), 0); //Print the network
        ntpClock.begin(1000); //Initialize the NTP Clock
        printTimestamp(1, true); //Print a Time Stamp
        if(testTwilio()){ //Test the twilio connection
          printHeading("Ready to Receive Messages!",0);
          printHeading("(604) 373 - 9569",4);
        }else{
          printError("Unable to Connect To Message Server, Check Internet Connection!", 3);
        }
      }else{ //If we don't connect within 10 seconds, print some errors
        printError("Unable to Connect To WiFi!", 0);  
        printStatus("Searching for Networks...", 1);
        printHeading("<-- Hold Button to Start Hotspot", 3);
      }
    }

  }else if(statusWiFi == 1 ){ //If we're connected to WiFi...
    twilio_server.handleClient(); //Update the Twilio server
    ntpClock.handle(); //Update the clock
    
    if(!statusTwilio){ //If we don't have a connection to Twilio, test it
      if(testTwilio()){ //If we establish a connection to Twilio, print out a message
        ntpClock.begin(1000);
        printTimestamp(1, true);
        printHeading("Ready to Receive Messages!",0);
        printHeading("(604) 373 - 9569",4);
      }
    }

  }else if(statusWiFi == 2){ //If a hotspot has been set up
    if(!digitalRead(D1)){ //If a button is being held down for over a second
      delay(1000);
      if(!digitalRead(D1)){
        printStatus("Connecting to WiFi...", 2); //Connect to the WiFi
        if(connectWiFi(10000)){ //If we connect within 10 seconds...
          printStatus("WiFi Connected: " + WiFi.SSID(), 0); //Print the network
          ntpClock.begin(1000); //Initialize the NTP Clock
          printTimestamp(1, true); //Print a Time Stamp
          if(testTwilio()){ //Test the twilio connection
            printHeading("Ready to Receive Messages!",0);
            printHeading("(604) 373 - 9569",4);
          }else{
            printError("Unable to Connect To Message Server, Check Internet Connection!", 3);
          }
        }else{ //If we don't connect within 10 seconds, print some errors
          printError("Unable to Connect To WiFi!", 0);  
          printStatus("Searching for Networks...", 1);
          printHeading("<-- Hold Button to Start Hotspot", 3);
        }
      }
    }
  }
}

