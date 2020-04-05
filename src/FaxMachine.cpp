#include "Arduino.h" //Arduino Library
#include "ESP8266mDNS.h" //multicast DNS Library
#include "ArduinoOTA.h" //Over-The-Air Update Library
#include "EEPROM.h" //EEPROM Library
#include "ESP8266WebServer.h" //Web Server Library
#include "WiFiClient.h" //WiFi Client Library
#include "twilio.h" //Twilio Library
#include "Adafruit_Thermal.h" //Thermal Printer Library
#include "NTPClock.h" //Silviu's NTP Library
#include "WiFiManager.h" //Silviu's WiFi Manager Library
#include "WebInterface.h" //Silviu's Web Interface Library
#include "storage.h" //Silviu's local storage library
#include "Counter.h" //Silviu's Counter Library
#include "PubSubClient.h" //MQTT Client Library
#include "ESP8266HTTPClient.h" //For Downloading JPGs...
#include "TJpg_Decoder.h" //For Decoding JPGs...


NTPClock ntpClock((char*)("time.google.com"), -7); //Create an NTP object
Adafruit_Thermal printer(&Serial); //Create a Thermal Printer object
WiFiManager wifiManager(10000); //Create a WiFi Manager object with a 10 second connection timeout
WebInterface webInterface; //Create a Web Interface Object
Storage phoneBook("phoneBook", 32); //Create a storage object for the phone book
WiFiClient espClient; //Create a generic client
PubSubClient MQTT_client(espClient); //Create an MQTT client

//Twilio SID, Twilio Authentication Token, Twilio fingerprint to authenticate connection
Twilio twilio("AC6e91a7c91c69b11d1d621f9f09ce0858", "9cc8633f6e84411fd69e2a17b3e82258", "06 86 86 C0 A0 ED 02 20 7A 55 CC F0 75 BB CF 24 B1 D9 C0 49");

const char* deviceName = "faxmachine"; //Device Name
const char* devicePassword = "sebastian"; //Device Password

bool updated = false; //Did the device just do an OTA update?

String lastMessageID;

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
      }else if(lastSpace <= i - wrapLength){ //If the last space was on the previous line, we're dealing with a really long word. Add a new line. 
        output.setCharAt(i, '\n');
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

//Output text to the web console but not to the printer
void consolePrint(String input, bool printed){

  String output = "<" + ntpClock.getTimestamp(); //Start with the timestamp

  if(printed){ //If this was not printed, append tildes
    output += "> ";
  }else{
    output += ">~~~~ ";
  }

  output += wrapText(input, 32); //Add the input text, wrapped to 32 characters 

  //Any time there is a new line, add a time stamp
  if(printed){
    output.replace("\n","\n<" + ntpClock.getTimestamp() + "> "); 
  }else{
    output.replace("\n","\n<" + ntpClock.getTimestamp() + ">~~~~ "); 
  }
  
  webInterface.println(output); //Print the text
}

//Output text to the web console but not to the printer, without having to specify it's for the console with a second argument
void consolePrint(String input){
  consolePrint(input, false);
}

//Output text to the web console and to the printer. Feed designates the amount of lines to feed after the text. Rest are parameters for printing. Size 1 = small, 2 = medium, 3 = large
void printerPrint(String input, uint8_t feed, bool bold, bool inverse, bool center, uint8_t size){
  
  if(!updated){//If firmware was not just updated
    printer.wake();
    //Set the printing parameters
    if(bold) printer.boldOn();
    if(inverse) printer.inverseOn();
    if(center) printer.justify('C');
    if(size == 2) printer.setSize('M');
    if(size == 3) printer.setSize('L');
    //Print
    printer.println(wrapText(input, 32)); 
  } 
  

  consolePrint(input, true); //Output to the console


  //If there is a feed specified
  if(feed != 0){
    if(!updated) printer.feed(feed); //Feed the printer if we haven't just updated
    for(int i = 0; i < feed; i++){ //Print timestamps to the console
      consolePrint(" ", true);
    }
  }

  //Put the printer to sleep
  printer.setDefault();
  printer.sleep();
}

//Print a centered, large, bold, inverse title
void printTitle(String text, uint8_t feed){

  //If the text is shorter than 14 chars, add a space to either side and print. Otherwise, just print it. 
  if(text.length() <= 14){
    printerPrint(" " + text + " ", feed, true, true, true, 3);
  }else{
    printerPrint(wrapText(text, 16), feed, true, true, true, 3);
  }
}

//Print a centered, bold, medium heading
void printHeading(String text, uint8_t feed){
  printerPrint(text, feed, true, false, true, 2);
}

//Print a medium, bold message
void printMessage(String text, uint8_t feed){
  printerPrint(text, feed, true, false, false, 2);
}

//Print a small, bold status
void printStatus(String text, uint8_t feed){
  printerPrint(text, feed, true, false, false, 1);
}

//Print a small, bold, inverse error
void printError(String text, uint8_t feed){
  updated = false; //If we just updated, resume printing to the printer
  printerPrint("Error: " + text, feed, true, true, false, 1);
}

//Print a timestamp
void printTimestamp(uint8_t feed, bool longDate){
  //If we have a time, print it
  if(ntpClock.handle()){
    //Print a long date/time or just a short timestamp
    if(longDate){
      printerPrint(ntpClock.getDateTime(), feed, false, false, false, 1);
    }else{
      printerPrint(ntpClock.getTimestamp(), feed, false, false, false, 1);
    }
  }
}

//Print a dotted line
void printLine(uint8_t feed){
  printerPrint("------------------------", feed, false, false, true, 1);
}

void downloadImage(String URL){

  HTTPClient http;

  // Configure server and url
  http.begin(URL);

  // Start connection and send HTTP header
  int httpCode = http.GET();
  if (httpCode > 0) {
    fs::File f = SPIFFS.open("/www/test.jpg", "w+");

    // File found at server
    if (httpCode == HTTP_CODE_OK) {

      // Get length of document (is -1 when Server sends no Content-Length header)
      int total = http.getSize();
      int len = total;

      // Create buffer for read
      uint8_t buff[128] = { 0 };

      // Get tcp stream
      WiFiClient * stream = http.getStreamPtr();

      // Read all data from server
      while (http.connected() && (len > 0 || len == -1)) {
        // Get available data size
        size_t size = stream->available();

        if (size) {
          // Read up to 128 bytes
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

          // Write it to file
          f.write(buff, c);

          // Calculate remaining bytes
          if (len > 0) {
            len -= c;
          }
        }
        yield();
      }
    }
    f.close();
  }
  http.end();
}

void processMessage(String time, String from, String message, String id, String media){

  if(message == "$help"){
    twilio.send_message(from, "+16043739569", "This fax machine is operated by Silviu. For any issues, contact silviu.toderita@gmail.com.\n##FEATURES\nText: Supported\nEmojis: Not Supported\nPhotos: Coming Soon\nVideos: Not Supported\n##COMMANDS\n$help - Information\n$name - Change Your Name");
    return;
  }

  if(message == "$name"){
    if(twilio.send_message(from, "+16043739569", "Please reply with a new name within 24hrs to add it to the phone book. For more options, reply with \"$help\".")){
      //Store the timestamp when we requested a name from the sender
      phoneBook.put(from, "%REQ" + String(ntpClock.getUNIXTimeExt()) );
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
      phoneBook.put(from, "%REQ" + String(ntpClock.getUNIXTimeExt()) );
    }

  //If we requested a name already...
  }else if(name.substring(0,4) == "%REQ"){
    //If the request is less than 24 hours old...
    if(int(ntpClock.getUNIXTimeExt()) <= name.substring(4).toInt() + 86400){

      //Let the sender know their name has been stored and store it. 
      if(phoneBook.put(from, message)){
        twilio.send_message(from, "+16043739569", "Thanks " + message + ", your name and number has been added to the phone book. To change your name, reply with \"$name\". For more options, reply with \"$help\".");
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
        phoneBook.put(from, "%REQ" + String(ntpClock.getUNIXTimeExt()) );
      }
    }

  //If we have a name stored, use that
  }else{
    from = name;
  }

  if(from.substring(0,1) == "1" && name.substring(0,1) != "1"){
    from = "(" + from.substring(1, 4) + ") " + from.substring(4, 7) + " - " + from.substring(7,11);
  }

  if(media != ""){
    message += "\nIMAGE ATTACHED";
    downloadImage("http://ackwxpcapo.cloudimg.io/v7/https://api.twilio.com" + media + "?p=fax");
  }

  printTitle("MESSAGE", 1); //Print a heading

  printStatus(ntpClock.convertUnixTime(time.toInt()), 0); //Convert Twilio's date/time to a long timestamp and print it

  printStatus("From: " + from, 1);

  printMessage(message, 0); //Print the message
  printLine(4); //Print a line

}

void MQTT_receive_message(char* topic, byte* payload, unsigned int length){
  String message;
  for(unsigned int i=0;i<length;i++){
    message += (char)payload[i];
  }

  String body = message.substring(message.indexOf("body:") + 5, message.indexOf("\nmedia:"));
  String from = message.substring(message.indexOf("from:") + 5, message.indexOf("\nbody:"));
  String id = message.substring(message.indexOf("id:") + 3, message.indexOf("\nfrom:"));
  String media = message.substring(message.indexOf("media:") + 6, message.indexOf("\ntime:"));
  String time = message.substring(message.indexOf("time:") + 5, message.length());

  if(media != "0"){
    media = "/2010-04-01/Accounts/AC6e91a7c91c69b11d1d621f9f09ce0858/Messages/" + id + "/Media/" + media;
  }else{
    media = "";
  }

  if(id != lastMessageID){
    processMessage(time, from, body, id, media);
    lastMessageID = id;
  }

}

//Set the WiFiManager status to idle to acknowledge that we've processed the disconnection
void setIdle(){
  printStatus("Searching for Networks...", 1);
  printHeading("<-- Press Button to Start Hotspot", 3);
  wifiManager.setIdle();
}

//Start a WiFi Hotspot
void createHotspot(){
  //If we can successfully create a hotspot, output the details
  if(wifiManager.createHotspot(deviceName, devicePassword)){
    printStatus("Hotspot Started! ", 0);
    printStatus("Network: " + String(deviceName), 0);
    printStatus("Password: " + String(devicePassword), 1);
    printHeading("<-- Press Button to Stop Hotspot", 3);
  //If we can't create a hotspot, output an error
  }else{
    printError("Unable to Initialize Hotspot!", 0); //Output an error if we can't create the hotspot (not sure when this would ever happen)
    printHeading("<-- Press Button to Retry", 3);
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
  printStatus("WiFi Connected: " + wifiManager.SSID(), 0); //Output the network

  //Attempt to connect to the NTP server for 2 seconds
  if(ntpClock.begin(2000)){
    printTimestamp(1, true); //Print a timestamp if it succeeds
  }else{
    printError("Unable to Connect To Time Server!", 1); //Print an error if it fails
  }
  
  wifiManager.setConnected(); //Let the wifiManager object know we have acknowledged a connection

  if(connectMQTT()){
    printHeading("Ready to Receive Messages!", 0);
    printHeading("(604) 373 - 9569", 4);
  }

  updated = false; //If we just did an OTA update and printing was inhibited, now printing is re-enabled


  if(!SPIFFS.exists("/www/test.jpg")){
    consolePrint("File not found!");
  }else{
    consolePrint("File Found!");
  } 

  /*
  TJpgDec.setJpgScale(1);

  TJpgDec.setCallback(jpg_output);

  uint32_t t = millis();

  uint16_t w = 0, h = 0;
  TJpgDec.getFsJpgSize(&w, &h, filename); 

  consolePrint("JPG Width: " + String(w) + ", JPG Height: " + String(h));


  //TJpgDec.drawFsJpg(0, 0, "/www/test.jpg");

  consolePrint("Time to Decode JPG: " + String(millis() - t) + "ms");
  */


}

//#######################
//######## SETUP ########
//#######################

void setup() {
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
    EEPROM.write(0,0);
    EEPROM.commit();
    SPIFFS.end(); //End SPIFFS if we're about to update the firmware
  });
  ArduinoOTA.begin();

  MDNS.begin(deviceName); //Start the mDNS

  //Bootloader code
  if(!digitalRead(D1)){
    wifiManager.createHotspot(deviceName, devicePassword);
    Counter counter(60000);
    while(!counter.status()){
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
    updated = true; //Set updated flag to true
    EEPROM.write(0,1); //Reset EEPROM at 0 address
    EEPROM.commit();
    consolePrint("OTA Update Successful, Rebooted!");
  } 

  //Begin the serial connection to the printer
  Serial.begin(9600);
  printer.begin();
  printer.sleep();
  //delay(100);

  //Print the title
  printTitle("FAX MACHINE", 2);

  //Add the network to the WiFi Manager
  wifiManager.addNetwork((char*)"Azaviu 2.4GHz", (char*)"sebastian");

  //Set MQTT settings
  MQTT_client.setServer("silviutoderita.com", 1883);
  MQTT_client.setCallback(MQTT_receive_message);
  
  //If we can't establish a WiFi connection in the alloted timeout, print an error
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
  ntpClock.handle(); //Update the clock
  
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
        printError("Unable to Connect To Network!", 0);  
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
        printTimestamp(0, true); //Print the time and an error
        printError("Lost WiFi Connection!", 0);
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

    

 