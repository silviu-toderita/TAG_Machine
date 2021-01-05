/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    TAG (Text And Graphics) Machine - A thermal printer that can receive SMS and 
    MMS messages.

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Arduino.h" //Arduino Library
#include "Persistent_Storage.h" //Silviu's persistent storage library

//Network Libraries
#include "WiFi_Manager.h" //Silviu's WiFi Manager Library
#include "ESP8266mDNS.h" //multicast DNS Library
#include "ArduinoOTA.h" //Over-The-Air Update Library
#include "Web_Interface.h" //Silviu's Web Interface Library
#include "WTA_Clock.h" //Silviu's clock Library

//Messaging Libraries
#include "PubSubClient.h" //MQTT Client Library
#include "Twilio.h" //Twilio Library

#include "Thermal_Printer.h" //Silviu's Thermal Printer Library


Persistent_Storage contacts("contacts"); //Storage object for the phone book
WiFi_Manager WiFi_manager; //WiFi Manager object
Web_Interface web_interface; //Web Interface Object
WTA_Clock WTA_clock; //NTP object
WiFiClient ESP_client; //Generic Client object for MQTT client
PubSubClient MQTT_client(ESP_client); //MQTT client object
Twilio twilio; //Twilio object
Thermal_Printer printer; //Printer object


String last_message_ID; //Twilio ID of the last message received
bool MQTT_connected = false; //Holds status of MQTT connection, true if MQTT has connected after Wi-Fi connection
bool WiFi_connection_failed = false; //True if the Wi-Fi Connection has failed once, false if it has not failed since last successful connect

//Settings
String phone_number; //Phone number of the Tag Machine
String owner_name; //Name of device owner
String bridge_URL; //URL of the MQTT broker

bool send_replies = true; //SMS replies are on/off default
String local_URL = "tagmachine"; //Local URL to access web interface and OTA updates default
String hotspot_SSID = "tagmachine"; //Hotspot SSID default
String hotspot_password = "12345678"; //Hotspot Password default
String OTA_password = "12345678"; //OTA password default
uint8_t LED_pin = 4; //ESP pin for LED default
uint8_t button_pin = 5; //ESP pin for button default

bool LED_on = false;


/*  console_print: Print to the web console
        input: Text to print
        printed: True if this text was printed, false if not
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void console_print(String input, bool printed){
    //Define the prefix for each line, based on whether the text was also printed or not
    String prefix = "<" + WTA_clock.get_timestamp() + ">";
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

/*  format_NA_phone_numbers: Format phone numbers as (XXX) XXX - XXXX if they are from North America
        input: raw number
    RETURNS formatted number 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String format_NA_phone_numbers(String input){
    //If the phone number starts with a 1 and is 11 digits long...
    if(input.substring(0,1) == "1" && input.length() == 11){
        //Format as (XXX) XXX - XXXX
        return "(" + input.substring(1, 4) + ") " + input.substring(4, 7) + " - " + input.substring(7,11);
    }
    
    //Otherwise, don't format it
    return input;
}

/*  process_message: Process and print an incoming message
        time: UNIX UTC time at time message received by server
        from_number: Phone number the message is from, with + removed
        message: Message body
        media: Media files (if any), separated by ","
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void process_message(String time, String from_number, String message, String media){
    //If the message is "_photo", change photo_mode to true so that the photo is printed by itself
    bool photo_mode = false;
    if(message == "_photo"){
        photo_mode = true;
    } 

    //The name is the phone number by default
    String name = from_number;

    //If send_replies is on...
    if(send_replies){
        
        //If the message is "_name", reply with a name update message
        if(message == "_name"){

            //If the reply was successfully sent...
            if(twilio.send_message(from_number, phone_number, "Please reply with a new name within 24hrs to add it to the contact list.")){
                //Store the timestamp when the request was sent out
                contacts.set(from_number, "_REQ" + time);
            }

            //Exit function before printing
            return;
        }

        //Get the name from the phone book
        name = contacts.get(from_number);
    
        //If the TAG machine should request a name, this will be true
        bool request_name = false;

        //If the number is not in the phone-book...
        if(name == ""){
            request_name = true;
        //If a name was requested already and the reply message isn't blank...
        }else if(name.substring(0,4) == "_REQ"){
            //If the request is less than 24 hours old...
            if(time.toInt() <= name.substring(4).toInt() + 86400 && message != ""){
                //Store the name in the phone book
                contacts.set(from_number, message);
                //Reply with a success message
                twilio.send_message(from_number, phone_number, "Thanks " + message + ", your name has been added to the contact list. To change your name, reply with \"_name\".");
        
                //exit the function before printing
                return;

            //If the request is more than 24 hours old...
            }else{
                request_name = true;
            }

        }

        //If request name is true...
        if(request_name){
            //Send a message asking the sender to reply with a name. If the reply is successful...
            if(twilio.send_message(from_number, phone_number, "Thanks for messaging " + owner_name + "'s Fax Machine! Reply with your name within 24hrs to add it to the contact list.")){
                //Store the timestamp when the request was sent out
                contacts.set(from_number, "_REQ" + time);
            }
            //Use the phone number as the name for this message
            name = from_number;
        }
    }

    
    //If photo mode is not on, print text
    if(!photo_mode){
        //Print the MESSAGE title
        File file = SPIFFS.open("/message.dat", "r");
        printer.print_bitmap_file(file, 1, "MESSAGE");
        file.close();

        printer.print_status(WTA_clock.get_date_time(time.toInt()), 0); //Convert Twilio's date/time to a long timestamp and print it
        printer.print_status("From: " + format_NA_phone_numbers(name), 1); //Print who the message is from
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
                if(send_replies) twilio.send_message(from_number, phone_number, "Sorry, but your message contained media in a format that's not supported by the TAG Machine. Only .jpg, .png, and .gif images are supported.");
            //Otherwise, print that particular media file
            }else{
                printer.print_bitmap_http("http://" + bridge_URL + "/img/" + media_filename[i] + ".dat", 1);
            }
        }
        
    }

    //Print a 4px line
    if(!photo_mode)printer.print_line(4, 4);
    

}


/*  remove_emojis: Removes any UTF-8 characters from a string which are not US-ASCII (includes all emojis)
        input: String to process
    RETURNS Processed string
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String remove_emojis(String input){
    //Define the character to use to replace non ASCII charactes
    const char blank_char = 178;
    //This string will hold the output
    String output = "";

    //Go through each character in the input
    for(int i = 0; i < input.length(); i++){
        //Pull the current character
        char current_char = input.charAt(i);
        //If the character is a single-byte UTF-8 character, copy it to the output
        if(current_char < 192){
            output += String(current_char);
        //If the character is a 2 byte UTF-8 character, replace it with the blank character and skip 1 byte
        }else if(current_char >= 192 && current_char < 224){
            output += blank_char;
            i++;
        //If the character is a 3 byte UTF-8 character, replace it with the blank character and skip 2 bytes
        }else if(current_char >= 224 && current_char < 240){
            output += blank_char;
            i = i + 2;
        //If the character is a 4 byte UTF-8 character, replace it with the blank character and skip 3 bytes
        }else{
            output += blank_char;
            i = i + 3;
        }
    }

    return output;
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
        process_message(time, from_number, remove_emojis(body), media);
        last_message_ID = id;
    }

}

/*  connect_to_MQTT: Connect to the MQTT broker
    RETURNS True if connected, false if not.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool connect_to_MQTT(){

    //Attempt to connect twice before giving up
    uint8_t attempt_counter = 0;
    while(!MQTT_client.connected() && attempt_counter < 2){
        attempt_counter++;
        //Attempt to connect to the MQTT server with clean-session turned off
        if(MQTT_client.connect(phone_number.c_str(),NULL,NULL,"fax",0,false,"disconnect",false)){

            //If successful, subscribe to the topic for the phone number at a QoS of 1 (!!! More security required)
            String topic = "smsin-" + phone_number;
            MQTT_client.subscribe(topic.c_str(),1);

            //If MQTT hasn't yet connected...
            if(!MQTT_connected){

                //Print the time if available
                if(WTA_clock.status()){
                    printer.print_status(WTA_clock.get_date_time(), 1);
                }else{
                    printer.feed(1);
                }

                //Print that messages can now be received
                printer.print_heading("Ready to Receive Messages!", 0);
                printer.print_heading(format_NA_phone_numbers(phone_number), 1);
                printer.print_line(4, 4); 
            }
            
            //MQTT has now connected
            MQTT_connected = true;
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
    //If the WiFi connection hasn't yet failed this time...
    if(!WiFi_connection_failed){
        //Print an error and record that the connection has failed
        printer.print_error("Unable to Connect To Network! Check WiFi Password.", 0); 
        printer.print_status("Attempting to Connect...", 1);
        printer.print_heading("<-- Press Button to Start Hotspot", 3);
        WiFi_connection_failed = true;
    }
    
}

/*  connected: Called when WiFi_manager confirms that an IP address has been assigned
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void connected(){

    //Print the current Wi-Fi Network
    printer.print_status("WiFi Connected: " + WiFi_manager.get_SSID(), 0);
    printer.print_status("Access web interface at: http://" + String(local_URL) + ".local", 1);

    //Initialize the clock
    WTA_clock.begin();

    //Initialize MDNS
    MDNS.begin(local_URL);

    //If the connection to the MQTT broker is unssuccessful, print an error and let the user know connection will continue to be attempted automatically
    if(!connect_to_MQTT()){
        printer.print_error("Unable to connect to message server! Check your internet connection.", 0);
        printer.print_status("Attempting to connect...", 2);
    } 

    //The WiFi connection has not yet failed
    WiFi_connection_failed = false;

}

/*  disconnected: Called when WiFi_manager confirms that connection to the network has been lost
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void disconnected(){
    //If available, print the time
    if(WTA_clock.status()) printer.print_status(WTA_clock.get_timestamp(), 0); 
    //Print an error
    printer.print_error("Lost WiFi Connection!", 0);
    printer.print_status("Searching for Networks...", 1);
    printer.print_heading("<-- Press Button to Start Hotspot", 3);
    //MQTT has disconnected if WiFi has disconnected
    MQTT_connected = false;

}

/*  create_hotspot: Create a hotspot using the device_name as the SSID and the device_password as the password
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void create_hotspot(){
    //Create a hotspot
    WiFi_manager.create_hotspot(hotspot_SSID, hotspot_password);

    //Print current status
    printer.print_status("Hotspot Started! ", 0);
    printer.print_status("Network: " + String(hotspot_SSID), 0);
    printer.print_status("Password: " + String(hotspot_password), 0);
    printer.print_status("Access Web Interface at: http://" + String(local_URL) + ".local", 1);
    printer.print_heading("<-- Press Button to Stop Hotspot", 3);
    //Initialize MDNS
    MDNS.begin(local_URL);
}

/*  load_settings: Load the settings from the settings file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void load_settings(){
    //Load variables
    phone_number = web_interface.load_setting("phone_number");
    owner_name = web_interface.load_setting("owner_name");
    bridge_URL = web_interface.load_setting("bridge_URL");
    if(bridge_URL.startsWith("http://")) bridge_URL = bridge_URL.substring(7);
    if(bridge_URL.startsWith("https://")) bridge_URL = bridge_URL.substring(8);
    OTA_password = web_interface.load_setting("OTA_password");
    button_pin = web_interface.load_setting("button_pin").toInt();
    LED_pin = web_interface.load_setting("LED_pin").toInt();

    //Load local_URL and filter out prefix and suffix
    local_URL = web_interface.load_setting("local_URL");
    if(local_URL.endsWith(".local")) local_URL = local_URL.substring(0,local_URL.length()-6);
    if(local_URL.startsWith("http://")) local_URL = local_URL.substring(7);
    if(local_URL.startsWith("https://")) local_URL = local_URL.substring(8);

    //Load send_replies and check if value is true or false
    String send_replies_string = web_interface.load_setting("send_replies");
    if(send_replies_string == "trueselected"){
        send_replies = true;
    }else{
        send_replies = false;
    }
    
    //Load img_photos and check if value is true or false
    String img_photos_string = web_interface.load_setting("img_photos");
    bool img_photos;
    if(img_photos_string == "trueselected"){
        img_photos = true;
    }else{
        img_photos = false;
    }
    
    //Set up printer and WiFi Manager
    printer.config(web_interface.load_setting("printer_baud").toInt(), web_interface.load_setting("printer_DTR_pin").toInt(),img_photos);
    printer.set_printing_parameters(web_interface.load_setting("printer_heating_dots").toInt(),web_interface.load_setting("printer_heating_time").toInt(),web_interface.load_setting("printer_heating_interval").toInt());

    //Set up Twilio
    String twilio_SID = web_interface.load_setting("Twilio_account_SID");
    String twilio_auth = web_interface.load_setting("Twilio_auth_token");
    if(twilio_SID == "" || twilio_auth == "") send_replies = false;
    if(send_replies) twilio.config(web_interface.load_setting("Twilio_account_SID"), web_interface.load_setting("Twilio_auth_token"));
    
    //Set up WiFi
    hotspot_SSID = web_interface.load_setting("hotspot_SSID");
    hotspot_password = web_interface.load_setting("hotspot_password");
    WiFi_manager.add_network(web_interface.load_setting("wifi_SSID_1"),web_interface.load_setting("wifi_password_1"));
    WiFi_manager.add_network(web_interface.load_setting("wifi_SSID_2"),web_interface.load_setting("wifi_password_2"));
    WiFi_manager.add_network(web_interface.load_setting("wifi_SSID_3"),web_interface.load_setting("wifi_password_3"));
    
}

/*  offline: Take SPIFFS and printer offline ahead of restart, to avoid file system corruption and garbage printer output
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void offline(){
    //Disable SPIFFS
    SPIFFS.end();
    //Disable the printer so there is no garbage output
    printer.offline();
}

/*  init_basics: Initialize basic functions of the TAG Machine
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void init_basics(){
    //Start the OTA updater using the device_password as the password
    ArduinoOTA.setHostname("tagmachine");
    ArduinoOTA.setPassword(OTA_password.c_str()); 
    //When an OTA update starts...
    ArduinoOTA.onStart([](){
        //Take the printer offline
        offline();
    });
    //Begin listening for OTA updates
    ArduinoOTA.begin();
}

/*  bootloader: Go into bootloader mode
        web_interface_on: Specifies if the web interface should be on to see the console and update settings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void bootloader(bool web_interface_on){
    uint16_t button_time = millis();
    bool button_pressed = true;
    //Initialize basic functions
    init_basics();
    //Create a hotspot
    WiFi_manager.create_hotspot(hotspot_SSID, hotspot_password);
    MDNS.begin(local_URL);
    //Loop forever
    while(true){
        if(web_interface_on){
            //Flash the LED once a second
            if(LED_on && millis() % 1000 < 500){
                digitalWrite(LED_pin, HIGH);
                LED_on = false;
            }else if(!LED_on && millis() % 1000 >= 500){
                digitalWrite(LED_pin, LOW);
                LED_on = true;
            }
        }else{
            //Flash the LED 4 times a second
            if(LED_on && millis() % 250 < 125){
                digitalWrite(LED_pin, HIGH);
                LED_on = false;
            }else if(!LED_on && millis() % 250 >= 125){
                digitalWrite(LED_pin, LOW);
                LED_on = true;
            }
        }

        // If the button continues to be held for 5 seconds continuously, do a factory reset
        if(button_pressed && digitalRead(button_pin)) button_pressed = false;
        if(button_pressed && millis() > button_time + 5000){
            SPIFFS.remove("/settings.txt");
            SPIFFS.remove("/contacts.txt");
            offline();
            ESP.restart();
        }

        //Run multicast DNS 
        MDNS.update();
        //Run OTA updater service
        ArduinoOTA.handle(); 

        //If specified, run the web interface
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
    //Start the button to listen for bootloader mode
    pinMode(button_pin, INPUT_PULLUP);
    pinMode(LED_pin, OUTPUT);
    digitalWrite(LED_pin, LOW); 
    //If the button is held down, start the bootloader without the web interface running
    if(!digitalRead(button_pin)){
        bootloader(false);
    }

    //Start the web interface, returns true if the settings file is valid and false if not
    bool settings_valid = web_interface.begin();

    console("TAG Machine Initializing...");

    //Set the callback function for taking the printer offline before restarting due to settings update
    web_interface.set_callback(offline);

    

    //If the settings are valid, load them
    if(settings_valid){
        load_settings();
        init_basics();
        console("Settings loaded successfully!");
    //If the settings file is invalid, start the bootloader with the web interface running
    }else{
        console("ERROR: Settings is missing one or more required values! Navigate to the Settings page and complete all required settings.");
        bootloader(true);
    }




    //Initialize the printer with the callback function for printing to the console
    printer.begin(console_callback);

    //Set the callback functions for the Wi-Fi Manager
    WiFi_manager.set_callbacks(connected, disconnected, connection_failed);

    //Print the title
    File file = SPIFFS.open("/logo.dat", "r");
    printer.print_bitmap_file(file, 2, "TAG MACHINE");
    file.close();

    MQTT_client.setServer(bridge_URL.c_str(), 1883);
    //Set callback for incoming message from MQTT
    MQTT_client.setCallback(new_message);
    MQTT_client.setBufferSize(2048);


    
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
            if(!digitalRead(button_pin)) create_hotspot();
            break;

        case WM_CONNECTION_SUCCESS: //Connection to a Wi-Fi Network succeeded
        case WM_CONNECTED://Connected to a Wi-Fi Network
            MDNS.update(); //Run mDNS service
            web_interface.handle(); //Update the web interface
            ArduinoOTA.handle(); //Run OTA updater service
            WTA_clock.handle(); //Update the clock

            //If the MQTT client is connected, update it.
            if(MQTT_client.connected()){
                MQTT_client.loop();
            //If the MQTT client is not connected...
            }else{
                //Attempt to connect to MQTT
                connect_to_MQTT();
            }

            break;

        //Currently running a hotspot
        case WM_HOTSPOT:
            MDNS.update(); //Run mDNS service
            web_interface.handle(); //Update the web interface
            ArduinoOTA.handle(); //Run OTA updater service
            
            //If the button is pressed, connect to the wi-fi
            if(!digitalRead(button_pin)){
                printer.print_status("Hotspot Stopped, Attempting to Connect To WiFi Network...", 2);
                begin_WiFi(); 
            } 
            break;
    }
    
}

    

 