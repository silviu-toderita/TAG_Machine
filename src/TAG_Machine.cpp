/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    TAG (Text And Graphics) Machine - A thermal printer that can receive SMS and 
    MMS messages.

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Arduino.h" //Arduino Library
#include "EEPROM.h" //EEPROM Library
#include "Persistent_Storage.h" //Silviu's Persistent Storage library

//Network Libraries
#include "WiFi_Manager.h" //Silviu's WiFi Manager Library
#include "ESP8266mDNS.h" //multicast DNS Library
#include "ArduinoOTA.h" //Over-The-Air Update Library
#include "Web_Interface.h" //Silviu's Web Interface Library
#include "NTP_Clock.h" //Silviu's NTP Library

//Messaging Libraries
#include "PubSubClient.h" //MQTT Client Library
#include "Twilio.h" //Twilio Library

#include "Thermal_Printer.h" //Silviu's Thermal Printer Library


Persistent_Storage phone_book("phone_book"); //Storage object for the phone book

WiFi_Manager WiFi_manager(10000); //WiFi Manager object with a 10 second connection timeout
Web_Interface web_interface; //Web Interface Object
NTP_Clock NTP_clock; //NTP object

WiFiClient ESP_client; //Generic Client object for MQTT client
PubSubClient MQTT_client(ESP_client); //MQTT client object
Twilio twilio("AC6e91a7c91c69b11d1d621f9f09ce0858", "9cc8633f6e84411fd69e2a17b3e82258", "BC B0 1A 32 80 5D E6 E4 A2 29 66 2B 08 C8 E0 4C 45 29 3F D0"); //Twilio object

Thermal_Printer printer(115200, D7); //Printer object

const char* device_name = "tagmachine"; //Device Name
const char* device_password = "sebastian"; //Device Password

String last_message_ID;

bool connected_to_MQTT = false;


/*  console_print: Print to the web console
        input: Text to print
        printed: True if this text was printed, false if not
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void console_print(String input, bool printed){
    //Define the prefix for each line, based on whether the text was also printed or not
    String prefix = "<" + NTP_clock.get_timestamp() + ">";
    if(printed){
        prefix += " ";
    }else{
        prefix += "~~~~";
    }

    //Initialize the output string with the prefix
    String output = prefix;

    //Append the input to the output, then replace every new line with the prefix.
    output += input;
    output.replace("\n","\n" + prefix); 
    
    //Print the text
    web_interface.console_print(output); 
}

/*  console: Print to the web console without printing to the printer
        input: Text to print
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void console(String input){
  console_print(input, false);
}

/*  console_callback: Print to the web console after printing to the printer
        input: Text to print
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void console_callback(String input){
  console_print(input, true);
}

/*  process_message: Process and print an incoming message
        time: UNIX UTC time at time message received by server
        from_number: Phone number the message is from, with + removed
        message: Message body
        media: Media files (if any), separated by ","
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void process_message(String time, String from_number, String message, String media){
    //Get the name from the phone book
    String name = phone_book.get(from_number);

    //If the message is ".photo", change photo_mode to true so that the photo is printed by itself
    bool photo_mode = false;
    if(message == ".photo"){
        photo_mode = true;
    
    //If the message is ".help", reply with a help message.
    }else if(message == ".help"){
        const char* help_message = 
            "This fax machine is operated by Silviu. For any issues, contact silviu.toderita@gmail.com.\n"
            "##FEATURES\n"
            "Text: Supported\n"
            "Emojis: Coming Soon\n"
            "Photos: Supported\n"
            "Videos: Not Supported\n"
            "##COMMANDS\n"
            ".help - Information\n"
            ".name - Change Your Name\n"
            ".photo - Print Photo Only";

        twilio.send_message(from_number, "16043739569", help_message);

        //Exit function before printing
        return;
    
    //If the message is ".name", reply with a name update message
    }else if(message == ".name"){

        //If the reply was successfully sent...
        if(twilio.send_message(from_number, "16043739569", "Please reply with a new name within 24hrs to add it to the phone book.")){
            //Store the timestamp when the request was sent out
            phone_book.put(from_number, "%REQ" + time);
        }

        //Exit function before printing
        return;
    }

    //If the number is not in the phone-book...
    if(name == ""){

        //Send a message asking the sender to reply with a name. If the reply is successful...
        if(twilio.send_message(from_number, "16043739569", "Thanks for messaging Silviu's Fax Machine! Reply with your name within 24hrs to add it to the phone book.")){
            //Store the timestamp when the request was sent out
            phone_book.put(from_number, "%REQ" + time);
        }

        //Use the phone number as the name for this message
        name = from_number;

    //If a name was requested already and the reply message isn't blank...
    }else if(name.substring(0,4) == "%REQ" && message != ""){

        //If the request is less than 24 hours old...
        if(time.toInt() <= name.substring(4).toInt() + 86400){
            //Store the name in the phone book
            phone_book.put(from_number, message);
            //Reply with a success message
            twilio.send_message(from_number, "16043739569", "Thanks " + message + ", your name has been added to the phone book. To change your name, reply with \".name\". For more options, reply with \".help\".");
    
            //exit the function before printing
            return;

        //If the request is more than 24 hours old...
        }else{
            //Send a message asking the sender to reply with a name. If the reply is successful...
            if(twilio.send_message(from_number, "16043739569", "Thanks for messaging Silviu's Fax Machine! Reply with your name within 24hrs to add it to the phone book.")){
                //Store the timestamp when the request was sent out
                phone_book.put(from_number, "%REQ" + time);
            }

            //Use the phone number as the name for this message
            name = from_number;
        }
    }

    //If the phone number is the name and it starts with a "1", change to North American number format
    if(from_number.substring(0,1) == "1" && name == from_number){
        name = "(" + from_number.substring(1, 4) + ") " + from_number.substring(4, 7) + " - " + from_number.substring(7,11);
    }

    
    if(!photo_mode){
        printer.print_title("MESSAGE", 1); //Print the title
        printer.print_status(NTP_clock.get_date_time(time.toInt()), 0); //Convert Twilio's date/time to a long timestamp and print it
        printer.print_status("From: " + name, 1); //Print who the message is from
        printer.print_message(message, 1); //Print the message
    } 

    //If there are any photos
    if(media != "0"){

        //Parse media string
        uint8_t media_count = 1; //The number of media files in this message
        String media_filename[10]; //The filename of each media file
        for(uint8_t i = 0; i < media.length(); i++){
            //Every time there is a comma, increment the media_count by 1
            if(media.charAt(i) == ','){
                media_count++;
            //Append the current character to the current media_filename
            }else{
                media_filename[media_count-1] += media.charAt(i);
            }
        }

        //For every media object...
        for(uint8_t i = 0; i < media_count; i++){
            //If it's an unsupported attachment, print that out and reply to the sender
            if(media_filename[i] == "NS"){
                printer.print_message("<UNSUPPORTED ATTACHMENT>", 1);
                twilio.send_message(from_number, "16043739569", "Sorry, but your message contained media in a format that's not supported by the TAG Machine. Only .jpg, .png, and .gif images are supported.");
            //Otherwise, print that particular media file
            }else{
                printer.print_bitmap_http("http://silviutoderita.com/img/" + media_filename[i] + ".bbf", 1);
            }
        }
        
    }

    //Print a 4px line
    if(!photo_mode)printer.print_line(4, 4);

}

/*  new_message: Called when a new message is received via MQTT
        topic: MQTT topic the message is received on (not currently used)
        payload: Byte array of the message
        length: Length of byte array
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void new_message(char* topic, byte* payload, unsigned int length){
    //Put payload into String
    String message;
    for(unsigned int i=0;i<length;i++){
        message += (char)payload[i];
    }

    //Parse message 
    String id = message.substring(message.indexOf("id:") + 3, message.indexOf("\nfrom:"));
    String from_number = message.substring(message.indexOf("from:") + 5, message.indexOf("\nbody:"));
    String body = message.substring(message.indexOf("body:") + 5, message.indexOf("\nmedia:"));
    String media = message.substring(message.indexOf("media:") + 6, message.indexOf("\ntime:"));
    String time = message.substring(message.indexOf("time:") + 5, message.length());

    //Sometimes duplicates come in, so always save ID of last message so it isn't processed twice
    if(id != last_message_ID){
        process_message(time, from_number, body, media);
        last_message_ID = id;
    }

}

/*  connect_to_MQTT: Connect to the MQTT broker
    RETURNS True if connected, false if not.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool connect_to_MQTT(){
    //Set MQTT server
    MQTT_client.setServer("silviutoderita.com", 1883);
    //Set callback for incoming message
    MQTT_client.setCallback(new_message);

    //Attempt to connect to the MQTT server with clean-session turned off
    if(MQTT_client.connect("tag-machine",NULL,NULL,"fax",0,false,"disconnect",false)){
        //If successful, subscribe to the topic for the phone number at a QoS of 1
        MQTT_client.subscribe("smsin-16043739569",1);

        //Attempt to connect to the NTP server
        if(NTP_clock.status()){
            printer.print_status(NTP_clock.get_date_time(), 1); //Print a timestamp if it succeeds
        }else{
            printer.feed(1);
        }

        //Print that messages can now be received
        printer.print_heading("Ready to Receive Messages!\n(604) 373 - 9569", 1);
        printer.print_line(4, 4); 
        connected_to_MQTT = true;
        return true;
    }
    return false;

}

/*  begin_WiFi: Attempt to connect to a Wi-Fi network
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void begin_WiFi(){
    //If a known WiFi network can't be found, print an error message
    if(!WiFi_manager.begin()){
        printer.print_error("Unable to Find Known WiFi Networks! Check your WiFi settings and access point.", 0);  
        printer.print_status("Searching for Networks...", 1);
        printer.print_heading("<-- Press Button to Start Hotspot", 3);
    }
}

/*  connection_failed: Called when WiFi_Manager confirms that connection attempt has failed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void connection_failed(){
    printer.print_error("Unable to Connect To Network!", 0); 
}

/*  connected: Called when WiFi_manager confirms that an IP address has been assigned
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void connected(){
    //Print the current Wi-Fi Network
    printer.print_status("WiFi Connected: " + WiFi_manager.get_SSID(), 0);
    printer.print_status("Access web console at: http://" + String(device_name) + ".local", 1);

    //Begin the NTP connection
    NTP_clock.begin();

    //If the connection to the MQTT broker is unssuccessful, print an error and let the user know connection will continue to be attempted automatically
    if(!connect_to_MQTT()){
        if(NTP_clock.status()) printer.print_status(NTP_clock.get_timestamp(), 0); 
        printer.print_error("Unable to connect to message server! Check your internet connection.", 0);
        printer.print_status("Attempting to connect...", 2);
    } 

    //If an OTA update was just completed, printing can now restart
    printer.suppress(false); 
}

/*  disconnected: Called when WiFi_manager confirms that connection to the network has been lost
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void disconnected(){
    //If available, print the time
    if(NTP_clock.status()) printer.print_status(NTP_clock.get_timestamp(), 0); 
    //Print an error
    printer.print_error("Lost WiFi Connection!", 0);
    printer.print_status("Searching for Networks...", 1);
    printer.print_heading("<-- Press Button to Start Hotspot", 3);
}

/*  create_hotspot: Create a hotspot using the device_name as the SSID and the device_password as the password
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void create_hotspot(){
    //Create a hotspot
    WiFi_manager.create_hotspot(device_name, device_password);

    //Print current status
    printer.print_status("Hotspot Started! ", 0);
    printer.print_status("Network: " + String(device_name), 0);
    printer.print_status("Password: " + String(device_password), 0);
    printer.print_status("Access web console at: http://" + String(device_name) + ".local", 1);
    printer.print_heading("<-- Press Button to Stop Hotspot", 3);

}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ####  SETUP  ####
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void setup() {
    //Initialize the printer with the callback function for printing to the console
    printer.begin(console_callback);
    //Set the callback functions for the Wi-Fi Manager
    WiFi_manager.set_callbacks(connected, disconnected, connection_failed);

    //Initialize the button and LED
    pinMode(D1, INPUT_PULLUP);
    pinMode(D2, OUTPUT);
    digitalWrite(D2, LOW);

    //Start SPI Flash File System
    SPIFFS.begin(); 
    SPIFFS.gc();

    //Start the OTA updater using the device_name as the hostname and device_password as the password
    ArduinoOTA.setHostname(device_name); 
    ArduinoOTA.setPassword(device_password); 

    //When an OTA update starts...
    ArduinoOTA.onStart([](){
        //Disable SPIFFS
        SPIFFS.end();
        //Disable the printer so there is no garbage output
        printer.offline();
        //Write a 0 to taddress 0 of EEPROM
        EEPROM.write(0,0);
        EEPROM.commit();
    });

    //Begin listening for OTA updates
    ArduinoOTA.begin();

    //Start the multicast DNS with the device_name as the address
    MDNS.begin(device_name);

    //If the button is pressed down during boot, enter the bootloader
    if(!digitalRead(D1)){
        //Create a hotspot
        WiFi_manager.create_hotspot(device_name, device_password);

        //For 90 seconds...
        uint32_t start = millis();
        while(millis() < start + 90000){
            //Flash the LED
            digitalWrite(D2, LOW); delay(250); digitalWrite(D2, HIGH); delay(250);
            //Run multicast DNS 
            MDNS.update();
            //Run OTA updater service
            ArduinoOTA.handle(); 
        }

    }

    //Initialize the EEPROM with 4 bits
    EEPROM.begin(4);

    //Read the EEPROM to see if an OTA update was just completed...
    if(EEPROM.read(0) == 0){
        //Suppress printing until boot finishes
        printer.suppress(true); 
        //Write a 1 to 0 address of EEPROM
        EEPROM.write(0,1); 
        EEPROM.commit();

        console("OTA Update Successful, Rebooted!");
    } 

    //Print the title
    printer.print_title("TAG MACHINE", 2);

    //Add the network to the WiFi Manager
    WiFi_manager.add_network((char*)"Azaviu 2.4GHz", (char*)"sebastian");
    
    //Start the Wi-Fi
    begin_WiFi();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ####  LOOP  ####
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void loop() {
    //Handle the WiFiManager every loop and pull the status
    switch(WiFi_manager.handle()){

        case WM_IDLE: //No active connection
        case WM_SCANNING: //Scanning for networks
        case WM_CONNECTING: //Connecting to a network
        case WM_CONNECTION_LOST: //Lost connection to a network
            //If the button is pressed, create a hotspot
            if(!digitalRead(D1)) create_hotspot();
            break;

        case WM_CONNECTION_SUCCESS: //Connection to a Wi-Fi Network succeeded
        case WM_CONNECTED://Connected to a Wi-Fi Network
            web_interface.handle(); //Update the web interface
            MDNS.update(); //Run mDNS service
            ArduinoOTA.handle(); //Run OTA updater service
            NTP_clock.handle(); //Update the clock

            //If the MQTT client is connected, update it.
            if(MQTT_client.connected()){
                MQTT_client.loop();
            
            //If the MQTT client is not connected...
            }else{
                
                //If the connection was just lost, print an error message
                if(connected_to_MQTT){
                    //If available, print the time
                    if(NTP_clock.status()) printer.print_status(NTP_clock.get_timestamp(), 0); 
                    printer.print_error("Disconnected from message server! Check your internet connection.", 0);
                    printer.print_status("Attempting to reconnect...", 2);
                    connected_to_MQTT = false;
                }
                
                //Attempt to connect to MQTT
                connect_to_MQTT();
            }

            break;

        //Currently running a hotspot
        case WM_HOTSPOT:
            web_interface.handle(); //Update the web interface
            MDNS.update(); //Run mDNS service
            ArduinoOTA.handle(); //Run OTA updater service
            
            //If the button is pressed, connect to the wi-fi
            if(!digitalRead(D1)){
                printer.print_status("Hotspot Stopped, Attempting to Connect To WiFi Network...", 2);
                begin_WiFi(); 
            } 
            break;
    }
}

    

 