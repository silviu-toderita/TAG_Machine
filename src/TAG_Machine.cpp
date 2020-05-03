/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    TAG (Text And Graphics) Machine - A thermal printer that can receive SMS and 
    MMS messages.

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "config.h"
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
Persistent_Storage config("config"); //Storage object for config file

WiFi_Manager WiFi_manager; //WiFi Manager object
Web_Interface web_interface; //Web Interface Object
NTP_Clock NTP_clock; //NTP object

WiFiClient ESP_client; //Generic Client object for MQTT client
PubSubClient MQTT_client(ESP_client); //MQTT client object
Twilio twilio; //Twilio object

Thermal_Printer printer; //Printer object

String last_message_ID;

String phone_number;
String device_name = "tagmachine";
String device_password = "12345678";
String owner_name = "User";

String MQTT_address;
bool connected_to_MQTT = false;

uint8_t LED_pin = 4;
uint8_t button_pin = 5;


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

    //Add a new line to the end of the output
    output += "\n";

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
    MQTT_client.setServer(MQTT_address.c_str(), 1883);
    //Set callback for incoming message
    MQTT_client.setCallback(new_message);

    //Attempt to connect twice before giving up
    uint8_t attempt_counter = 0;
    while(!MQTT_client.connected() && attempt_counter < 2){
        attempt_counter++;
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

String load_config(){
    String error = "";
    if(config.exists()){
        phone_number = config.get("phone_number");
            if(phone_number == "") error += "Missing phone number!\n";
        device_name = config.get("device_name");
            if(device_name == "") device_name = "tagmachine";
        device_password = config.get("device_password");
            if(device_password == "") device_password = "12345678";
        owner_name = config.get("owner_name");
            if(owner_name == "") owner_name = "User";

        String printer_baud = config.get("printer_baud");
            if(printer_baud == "") printer_baud = "9600";
        String printer_DTR_pin = config.get("printer_DTR_pin");
            if(printer_DTR_pin == "") printer_DTR_pin = "13";
        printer.config(printer_baud.toInt(), printer_DTR_pin.toInt());
        
        String button_pin_string = config.get("button_pin");
        if(button_pin_string != ""){
            button_pin = button_pin_string.toInt();
        }

        String LED_pin_string = config.get("LED_pin"); 
        if(LED_pin_string != ""){
            LED_pin = LED_pin_string.toInt();
        }

        //Add the network to the WiFi Manager
        String WiFi_SSID = config.get("WiFi_SSID");
        if(WiFi_SSID != ""){
            String WiFi_password = config.get("WiFi_password");
            if(WiFi_password == "" || WiFi_password == "none") WiFi_password = "";
            WiFi_manager.add_network(config.get("WiFi_SSID"),config.get("WiFi_password"));
        }else{
            error += "Missing WiFi SSID!\n";
        }

        if(config.get("WiFi_timeout") != ""){
            WiFi_manager.config(config.get("WiFi_timeout").toInt());
        }
        
        if(config.get("Twilio_account_sid") != "" && config.get("Twilio_auth_token") != ""){
            String Twilio_fingerprint = config.get("Twilio_fingerprint");
                if(Twilio_fingerprint == "") Twilio_fingerprint = "BC B0 1A 32 80 5D E6 E4 A2 29 66 2B 08 C8 E0 4C 45 29 3F D0";
            twilio.config(config.get("Twilio_account_sid"), config.get("Twilio_auth_token"), Twilio_fingerprint);
        }else{
            error += "Missing Twilio account SID or auth token!\n";
        }

        if(config.get("MQTT_address") != ""){
            MQTT_address = config.get("MQTT_address");
        }else{
            error += "Missing MQTT broker address!\n";
        }
        

        String pcv = config.get("persistent_console");
        if(pcv == "FALSE" || pcv == "False" || pcv == "false" || pcv == "NO" || pcv == "No" || pcv == "no" || pcv == "N" || pcv == "n" || pcv == "0"){
            web_interface.config(false);
        }else{
            web_interface.config(true);
        }

        String NTP_server = config.get("NTP_server");
            if(NTP_server == "") NTP_server = "time.google.com";
        String NTP_interval = config.get("NTP_interval");
            if(NTP_interval == "") NTP_interval = 600;
        String NTP_timezone = config.get("NTP_timezone");
            if(NTP_timezone = "" || NTP_timezone == "AUTO" || NTP_timezone == "Auto" || NTP_timezone == "auto") NTP_timezone = NTP_CLOCK_AUTO;
        String NTP_timeout = config.get("NTP_timeout");
            if(NTP_timeout = "") NTP_timeout = 5000;

        NTP_clock.config(NTP_server, NTP_interval.toInt(), NTP_timezone.toInt(), NTP_timeout.toInt());
    }else{
        error = "Missing config file!";
        config.put("phone_number", "\t\t//REQUIRED - Phone number including country code(numbers only)");
        config.put("device_name", "\t\t//Device name (no spaces) - Default: tagmachine");
        config.put("device_password", "\t\t//Device password - Default: 12345678");
        config.put("owner_name", "\t\t//Your name - Default: User");
        config.put("printer_baud", "\t\t//Baud rate of thermal printer. To get this, hold the button on the printer while plugging it in. Default: 9600");
        config.put("printer_DTR_pin", "\t\t//ESP8266 pin that printer DTR is connected to (GPIO number) - Default: 13");
        config.put("button_pin", "\t\t//ESP8266 pin that button is connected to (GPIO number) - Default: 5");
        config.put("LED_pin", "\t\t//ESP8266 pin that LED is connected to (GPIO number) - Default: 4");
        config.put("WiFi_SSID", "\t\t//REQUIRED - SSID of local WiFi Network");
        config.put("WiFi_password", "\t\t//Password of local WiFi Network - Default: none");
        config.put("WiFi_timeout", "\t\t//Timeout in ms to attempt to connect to WiFi Network - Default: 10000");
        config.put("Twilio_account_sid", "\t\t//REQUIRED - Twilio Account SID, get this from Twilio dashboard");
        config.put("Twilio_auth_token", "\t\t//REQUIRED - Twilio Auth Token, get this from Twilio dashboard");
        config.put("Twilio_fingerprint", "\t\t//Twilio SHA1 fingerprint, get this from SSL certificate of api.twilio.gom - Default: BC B0 1A 32 80 5D E6 E4 A2 29 66 2B 08 C8 E0 4C 45 29 3F D0");
        config.put("MQTT_address", "\t\t//REQUIRED - URL of the MQTT broker used for relaying Twilio messages");
        config.put("persistent_console", "\t\t//Persist console contents when console isn't open - Default: false");
        config.put("NTP_server", "\t\t//URL of NTP server to use for time - Default: time.google.com");
        config.put("NTP_interval", "\t\t//Interval in s to update time using NTP server - Default: 600");
        config.put("NTP_timezone", "\t\t//Timezone offset from UTC in minutes (optionally -) - Default: auto");
        config.put("NTP_timeout", "\t\t//Timeout in ms to wait for NTP connection - Default: 5000");
    }

    return error;
}

void bootloader(bool web_interface_on){
    //Create a hotspot
    WiFi_manager.create_hotspot(device_name, device_password);

    uint32_t start = millis();
    bool LED_on = false;
    while(true){
        uint32_t time_elapsed = millis() - start;
        if(LED_on && time_elapsed % 250 < 125){
            digitalWrite(LED_pin, HIGH);
            LED_on = false;
        }else if(!LED_on && time_elapsed % 250 >= 125){
            digitalWrite(LED_pin, LOW);
            LED_on = true;
        }
        //Run multicast DNS 
        MDNS.update();
        //Run OTA updater service
        ArduinoOTA.handle(); 

        if(web_interface_on){
            web_interface.handle();
        }

        yield();
    }

}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ####  SETUP  ####
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void setup() {
    //Start SPI Flash File System
    SPIFFS.begin(); 
    SPIFFS.gc();

    pinMode(button_pin, INPUT_PULLUP); //default 5
    pinMode(LED_pin, OUTPUT); // default 4
    digitalWrite(LED_pin, LOW); 

    String error = load_config();

    //Start the multicast DNS with the device_name as the address
    MDNS.begin(device_name);

    //Initialize the EEPROM with 4 bits
    EEPROM.begin(4);
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

    web_interface.begin();

    //Start the OTA updater using the device_name as the hostname and device_password as the password
    ArduinoOTA.setHostname(device_name.c_str()); 
    ArduinoOTA.setPassword(device_password.c_str()); 

    //Begin listening for OTA updates
    ArduinoOTA.begin();

    if(error != ""){
        console("BOOT ERROR - Config File Error:");
        console(error);
        bootloader(true);
    }else if(!digitalRead(D1)){
        bootloader(false);
    }



    //Initialize the printer with the callback function for printing to the console
    printer.begin(console_callback);
    //Set the callback functions for the Wi-Fi Manager
    WiFi_manager.set_callbacks(connected, disconnected, connection_failed);

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

    

 