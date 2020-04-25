#include "Arduino.h" //Arduino Library
#include "ArduinoOTA.h" //Over-The-Air Update Library
#include "EEPROM.h" //EEPROM Library
#include "Storage.h" //Silviu's Persistent Storage library

//Network Libraries
#include "WiFiManager.h" //Silviu's WiFi Manager Library
#include "ESP8266mDNS.h" //multicast DNS Library
#include "WebInterface.h" //Silviu's Web Interface Library
#include "Twilio.h" //Twilio Library
#include "NTP_Clock.h" //Silviu's NTP Library
#include "PubSubClient.h" //MQTT Client Library

#include "Thermal_Printer.h" //Silviu's Thermal Printer Library


NTP_Clock ntp_clock; //Create an NTP object
Thermal_Printer printer(115200, D7); //Create a printer object
WiFiManager wifiManager(10000); //Create a WiFi Manager object with a 10 second connection timeout
WebInterface webInterface; //Create a Web Interface Object
Storage phoneBook("phoneBook", 32); //Create a storage object for the phone book
WiFiClient espClient; //Create a generic client
PubSubClient MQTT_client(espClient); //Create an MQTT client

//Twilio SID, Twilio Authentication Token, Twilio fingerprint to authenticate connection
Twilio twilio("AC6e91a7c91c69b11d1d621f9f09ce0858", "9cc8633f6e84411fd69e2a17b3e82258", "06 86 86 C0 A0 ED 02 20 7A 55 CC F0 75 BB CF 24 B1 D9 C0 49");

const char* deviceName = "faxmachine"; //Device Name
const char* devicePassword = "sebastian"; //Device Password

String lastMessageID;


//Output text to the web console but not to the printer
void consolePrintln(String input, bool printed){

  String output = "<" + ntp_clock.get_timestamp(); //Start with the timestamp

  if(printed){ //If this was not printed, append tildes
    output += "> ";
  }else{
    output += ">~~~~ ";
  }

  output += input;

  //Any time there is a new line, add a time stamp
  if(printed){
    output.replace("\n","\n<" + ntp_clock.get_timestamp() + "> "); 
  }else{
    output.replace("\n","\n<" + ntp_clock.get_timestamp() + ">~~~~ "); 
  }
  
  webInterface.println(output); //Print the text
}

//Output text to the web console but not to the printer, without having to specify it's for the console with a second argument
void console(String input){
  consolePrintln(input, false);
}

void consoleCallback(String input){
  consolePrintln(input, true);
}

void processMessage(String time, String from, String message, String media){
  bool photo = false;

  if(message == "$photo"){
    photo = true;
    message = "";
  }
  

  if(message == "$help"){
    twilio.send_message(from, "+16043739569", "This fax machine is operated by Silviu. For any issues, contact silviu.toderita@gmail.com.\n##FEATURES\nText: Supported\nEmojis: Coming Soon\nPhotos: Supported\nVideos: Not Supported\n##COMMANDS\n$help - Information\n$name - Change Your Name\n$photo - Photo Mode (No Info)");
    return;
  }

  if(message == "$name"){
    if(twilio.send_message(from, "+16043739569", "Please reply with a new name within 24hrs to add it to the phone book. For more options, reply with \"$help\".")){
      //Store the timestamp when we requested a name from the sender
      phoneBook.put(from, "%REQ" + time);
    }
    return;
  }

  //Get the name from the phone book
  String name = phoneBook.get(from);

  //If the number is not in the phone-book...
  if(name == ""){
    //Send a message asking the sender to reply with a name
    if(twilio.send_message(from, "+16043739569", "Thanks for messaging Silviu's Fax Machine! Reply with your name within 24hrs to add it to the phone book.")){
      //Store the timestamp when we requested a name from the sender
      phoneBook.put(from, "%REQ" + time);
    }

  name = from;
  //If we requested a name already...
  }else if(name.substring(0,4) == "%REQ" && message != ""){
    //If the request is less than 24 hours old...
    if(time.toInt() <= name.substring(4).toInt() + 86400){

      //Let the sender know their name has been stored and store it. 
      if(phoneBook.put(from, message)){
        twilio.send_message(from, "+16043739569", "Thanks " + message + ", your name and number has been added to the phone book. To change your name, reply with \"$name\". For more options, reply with \"$help\".");
      name = message;
      }else{
        twilio.send_message(from, "+16043739569", "Sorry " + message + ", the phone book doesn't support names longer than 32 characters. Please reply with a shorter name.");
      }

      //exit the function before printing
      return;

    //If the request is more than 24 hours old...
    }else{
      //Send a message asking the sender to reply with a name
      if(twilio.send_message(from, "+16043739569", "Thanks for messaging Silviu's Fax Machine! Reply with your name within 24hrs to add it to the phone book.")){
        //Store the timestamp when we requested a name from the sender
        phoneBook.put(from, "%REQ" + time);
      }
    }
  name = from;
  //If we have a name stored, use that
  }

  if(from.substring(0,1) == "1" && name == from){
    name = "(" + from.substring(1, 4) + ") " + from.substring(4, 7) + " - " + from.substring(7,11);
  }

  if(!photo) printer.print_title("MESSAGE", 1); //Print a heading

  if(!photo) printer.print_status(ntp_clock.get_date_time(time.toInt()), 0); //Convert Twilio's date/time to a long timestamp and print it

  if(!photo) printer.print_status("From: " + name, 1);

  if(!photo) printer.print_message(message, 1); //Print the message

  if(media != "0"){
    uint8_t mediaCount = 1;
    String mediaNum[10];
    for(uint8_t i = 0; i < media.length(); i++){
      if(media.charAt(i) == ','){
        mediaCount++;
      }else{
        mediaNum[mediaCount-1] += media.charAt(i);
      }
    }

    for(uint8_t i = 0; i < mediaCount; i++){
      if(mediaNum[i] == "NS"){
        printer.print_message("<UNSUPPORTED ATTACHMENT>", 1);
        twilio.send_message(from, "+16043739569", "Sorry " + name + ", but your message contained media in a format that's not supported. Only .jpg, .png, and .gif images are supported.");
      }else{
        printer.print_bitmap_http("http://silviutoderita.com/img/" + mediaNum[i] + ".bbf", 1);
      }
      
    }
    
  }

  
  if(!photo)printer.print_line(4, 4); //Print a line

}

void newMessage(char* topic, byte* payload, unsigned int length){
  String message;
  for(unsigned int i=0;i<length;i++){
    message += (char)payload[i];
  }

  String id = message.substring(message.indexOf("id:") + 3, message.indexOf("\nfrom:"));
  String from = message.substring(message.indexOf("from:") + 5, message.indexOf("\nbody:"));
  String body = message.substring(message.indexOf("body:") + 5, message.indexOf("\nmedia:"));
  String media = message.substring(message.indexOf("media:") + 6, message.indexOf("\ntime:"));
  String time = message.substring(message.indexOf("time:") + 5, message.length());

  if(id != lastMessageID){
    processMessage(time, from, body, media);
    lastMessageID = id;
  }

}

//Set the WiFiManager status to idle to acknowledge that we've processed the disconnection
void setIdle(){
  printer.print_status("Searching for Networks...", 1);
  printer.print_heading("<-- Press Button to Start Hotspot", 3);
  wifiManager.setIdle();
}

//Start a WiFi Hotspot
void createHotspot(){
  //If we can successfully create a hotspot, output the details
  if(wifiManager.createHotspot(deviceName, devicePassword)){
    printer.print_status("Hotspot Started! ", 0);
    printer.print_status("Network: " + String(deviceName), 0);
    printer.print_status("Password: " + String(devicePassword), 1);
    printer.print_heading("<-- Press Button to Stop Hotspot", 3);
  //If we can't create a hotspot, output an error
  }else{
    printer.print_error("Unable to Initialize Hotspot!", 0); //Output an error if we can't create the hotspot (not sure when this would ever happen)
    printer.print_heading("<-- Press Button to Retry", 3);
  }
}

bool connectMQTT(){

  if(MQTT_client.connect("fax-machine",NULL,NULL,"fax",0,false,"disconnect",false)){
    MQTT_client.subscribe("smsin-16043739569",1);
    
    return true;
  }

  return false;

}


//Set the WiFiManager status to connected to acknowledge that we've processed the connection
void connected(){
  printer.print_status("WiFi Connected: " + wifiManager.SSID(), 0); //Output the network

  //Attempt to connect to the NTP server for 2 seconds
  if(ntp_clock.begin()){
    printer.print_status(ntp_clock.get_date_time(), true); //Print a timestamp if it succeeds
  }else{
    printer.print_error("Unable to Connect To Time Server!", 1); //Print an error if it fails
  }
  
  wifiManager.setConnected(); //Let the wifiManager object know we have acknowledged a connection

  if(connectMQTT()){
    printer.print_heading("Ready to Receive Messages!\n(604) 373 - 9569", 1);
  }

  printer.print_line(4, 4);

  printer.suppress(false); //If we just did an OTA update and printing was inhibited, now printing is re-enabled



}

//#######################
//######## SETUP ########
//#######################

void setup() {
  printer.begin(consoleCallback);

  //Initialize the button and LED
  pinMode(D1, INPUT_PULLUP);
  pinMode(D2, OUTPUT);
  digitalWrite(D2, LOW);

  SPIFFS.begin(); //Start SPI Flash File System

  //Start the OTA updater
  ArduinoOTA.setHostname(deviceName); 
  ArduinoOTA.setPassword(devicePassword); 
  //When an OTA update starts, update address 0 of EEPROM
  ArduinoOTA.onStart([](){
    printer.offline();
    EEPROM.write(0,0);
    EEPROM.commit();
    SPIFFS.end(); //End SPIFFS if we're about to update the firmware
  });

  ArduinoOTA.begin();

  MDNS.begin(deviceName); //Start the mDNS

  //Bootloader code
  if(!digitalRead(D1)){
    wifiManager.createHotspot(deviceName, devicePassword);
    uint32_t start = millis();
    while(millis() < start + 60000){
      digitalWrite(D2, LOW);
      delay(250);
      digitalWrite(D2, HIGH);
      delay(250);
      MDNS.update(); //Run mDNS
      ArduinoOTA.handle(); //Run OTA updater service
    }
  }

  //Initialize the EEPROM with 4 bits
  EEPROM.begin(4);
  //Read the EEPROM to see if we have just completed an OTA update:
  if(EEPROM.read(0) == 0){ //If we have...
    printer.suppress(true); //Set updated flag to true
    EEPROM.write(0,1); //Reset EEPROM at 0 address
    EEPROM.commit();
    console("OTA Update Successful, Rebooted!");
  } 

  //Print the title
  printer.print_title("FAX MACHINE", 2);

  //Add the network to the WiFi Manager
  wifiManager.addNetwork((char*)"Azaviu 2.4GHz", (char*)"sebastian");

  //Set MQTT settings
  MQTT_client.setServer("silviutoderita.com", 1883);
  MQTT_client.setCallback(newMessage);
  
  //If we can't establish a WiFi connection in the alloted timeout, print an error
  if(!wifiManager.begin()){
    printer.print_error("Unable to Find Known Networks!", 0);  
    printer.print_status("Searching for Networks...", 1);
    printer.print_heading("<-- Press Button to Start Hotspot", 3);
  }

}

//######################
//######## LOOP ########
//######################

void loop() {
  ntp_clock.handle(); //Update the clock
  
  //Handle the WiFiManager every loop and pull the status
  switch(wifiManager.handle()){

    //WM_IDLE indicates no active connection, listen for button press to create hotspot
    case WM_IDLE: 
        if(!digitalRead(D1)) createHotspot();
        break;

    //WM_SCANNING indicates we are scanning for known networks, listen for button press to create hotspot
    case WM_SCANNING: 
        if(!digitalRead(D1)) createHotspot();
        break;
    
    //WM_SCAN_FAILED indicates the last scan failed to find a known network
    case WM_SCAN_FAILED: break;
    //WM_CONNECTING indicates we are attempting to connect to a known network
    case WM_CONNECTING: break;

    //WM_CONNECTION_SUCCESS indicates we have successfully connected to a network
    case WM_CONNECTION_SUCCESS:
        connected();
        break;

    //WM_CONNECTION_FAILED indicates we have failed to connect to a network
    case WM_CONNECTION_FAILED:
        printer.print_error("Unable to Connect To Network!", 0);  
        setIdle(); //Set the status to Idle
        break;

    //WM_CONNECTED indicates we are connected to a wi-fi network
    case WM_CONNECTED:
        webInterface.handle(); //Update the web interface
        MDNS.update(); //Run mDNS
        ArduinoOTA.handle(); //Run OTA updater service

        if(MQTT_client.connected()){
          MQTT_client.loop();
        }else{
          connectMQTT();
        }

        break;

    //WM_CONNECTION_LOST indicates we have just lost the wi-fi connection
    case WM_CONNECTION_LOST:
        printer.print_status(ntp_clock.get_timestamp(), true); //Print the time and an error
        printer.print_error("Lost WiFi Connection!", 0);
        setIdle(); //Set the status to idle
        break;

    //WM_HOTSPOT indicates we have an active hotspot, listen for button press to end hotspot
    case WM_HOTSPOT:
        webInterface.handle(); //Update the web interface
        MDNS.update(); //Run mDNS
        ArduinoOTA.handle(); //Run OTA updater service
        
        if(!digitalRead(D1)) setIdle(); 
        break;
  }
}

    

 