/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ESP8266 library for hosting a simple web interface. Features:
    -Web console for an ESP that can't be plugged in to use the serial monitor. 
    -Dynamic settings page

    To use, initialize a Web_Interface object and call handle() every loop or as
    often as possible. Call console_print() to output a line to the console. Place
    files for server in /www/ folder in SPIFFS. 

    To use the settings function, place a settings_template.txt file in the root 
    of the SPIFFS. Fill the file with settings with the format:
        setting1:default value //Default value if none specified
        setting2: //Optional setting
        setting3:REQ //Required setting with no default
    begin() will return false if there are any required settings that are missing.
    load_setting() allows you to load a setting based on its name. 

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Web_Interface.h"

//Storage objects for settings and settings_template
Persistent_Storage settings("settings");
Persistent_Storage settings_template("settings_template");

ESP8266WebServer server(80); //Create a web server listening on port 80
WebSocketsServer websockets_server = WebSocketsServer(81); //Create a websockets server listening on port 81

static void_function_pointer _offline; //Callback function when connected

int8_t websockets_client = -1; //Current websockets client number connected to (-1 is none)
bool persistent_console; //Flag to persist console data when not on console page

/*  (private) get_content_type: Returns the HTTP content type based on the extension
        filename: 
    RETURNS HTTP content type as a string
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String get_content_type(String filename){ 
    if(filename.endsWith(".htm")) return "text/html";
    else if(filename.endsWith(".html")) return "text/html";
    else if(filename.endsWith(".css")) return "text/css";
    else if(filename.endsWith(".js")) return "application/javascript";
    else if(filename.endsWith(".png")) return "image/png";
    else if(filename.endsWith(".gif")) return "image/gif";
    else if(filename.endsWith(".jpg")) return "image/jpeg";
    else if(filename.endsWith(".bmp")) return "image/bmp";
    else if(filename.endsWith(".ico")) return "image/x-icon";
    else if(filename.endsWith(".xml")) return "text/xml";
    else if(filename.endsWith(".pdf")) return "application/x-pdf"; //Compressed file
    else if(filename.endsWith(".zip")) return "application/x-zip"; //Compressed file
    else if(filename.endsWith(".gz")) return "application/x-gzip"; //Compressed file
    return "text/plain"; //If none of the above, assume file is plain text
}

/*  (private)handle_file_read: Read a file from SPIFFS and serve it when requested.
        path: The requested URI
    RETURNS true if the file exists, false if it does not exist
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool handle_file_read(String path){  
    //Append /www to the beginning of the path
    path = "/www" + path;

    //If only a folder is specified, attempt to return index.html
    if(path.endsWith("/")) path += "index.html"; 

    //Get the content type 
    String content_type = get_content_type(path); 

    //If the compressed file exists, stream it to the client
    if(SPIFFS.exists(path + ".gz")){
        File file = SPIFFS.open(path + ".gz", "r");                
        server.streamFile(file, content_type);
        file.close();                                    
        return true;
    }

    //If the file exists, stream it to the client
    if(SPIFFS.exists(path)){
        File file = SPIFFS.open(path, "r");                
        server.streamFile(file, content_type);
        file.close();                                    
        return true;
    }

    return false;                                         
}

/*  (private)websockets_event: Called when a new websockets event happens
        num: client number
        type: event type
        payload: message payload
        length: message length
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void websockets_event(uint8_t num, WStype_t type, uint8_t * payload, size_t length) { // When a WebSocket message is received

    //Take different action based on the type of event
    switch (type) {
        //When the websockets client disconnects...
        case WStype_DISCONNECTED:  
            //Erase the client number           
            websockets_client = -1;
            break;

        //When the websockets client connects...
        case WStype_CONNECTED:
            //Store the client number
            websockets_client = num;

            //If persistent_console is true and the console file exists...
            if(persistent_console && SPIFFS.exists("/www/console.txt")){
                //Open the console.txt file in read mode and send it to the client
                File console = SPIFFS.open("/www/console.txt", "r");
                String console_text = console.readString();
                websockets_server.sendTXT(num, console_text);
                //Close the file
                console.close();
            }

            break;

        //For all other cases, do nothing
        default:           
            break;
    }

}

/*  (private)handle_settings_get: Send the settings to the browser as an HTML form
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void handle_settings_get(){
    //Get the number of settings from the template
    uint16_t settings_template_entries = settings_template.get_number_entries();
    String response;

    //Respond with no settings defined if there are none
    if(settings_template_entries == 0){
        response = "<h3>No Settings Defined!</h3>";
    }
    
    //For each setting...
    for(int i = 0; i < settings_template_entries; i++){
        //Get the setting name from the settings template
        String key = settings_template.get_key(i);
        //Get the setting value
        String value = settings.get_value(key);

        //Form the HTML response
        response += "<div class=\"form-group\">";
        response +=     "<label for=\"" + key + "\">" + key + "</label>";
        response +=     "<input type=\"text\" class=\"form-control\" id=\"" + key + "\" name=\"" + key + "\" value=\"" + value + "\">";
        response += "</div>";
        
    }

    //Send the response to the browser
    server.send(200, "text/html", response);
}

/*  (private)handle_settings_pose: Receive new settings from the browser
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void handle_settings_post(){
    //Confirm that the settings have been received
    server.send(200);
    
    //For each server argument except the last one...
    for(int i = 0; i < server.args() - 1; i++){
        //Store the settings
        settings.put(server.argName(i), server.arg(i));
    }

    //Take the tag machine offline and restart
    _offline();
    ESP.restart();
}

/*  Web_Interface Constructor (with defaults)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
Web_Interface::Web_Interface(){
    config(true);
}

/*  config
        persistent_console_in: Set true to persist console when console page is
            not open (default: false)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Web_Interface::config(bool persistent_console_in){
    persistent_console = persistent_console_in; 
}

/*  set_callback: Set the callback function to take tag machine offline safely before restarting
        offline: offline function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Web_Interface::set_callback(void_function_pointer offline){
    _offline = offline;
}

/*  (private)check_settings_file: Check the settings file to ensure all the required parameters are present
    RETURNS true if there is no blank parameter that's required, false if there is a blank parameter that's required
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool check_settings_file(){
    //Start off by assuming the file is valid
    bool is_valid = true;

    //The number of settings in the settings template
    uint8_t template_entries = settings_template.get_number_entries();

    //For each setting in the setting template...
    for(int i = 0; i < template_entries; i++){
        //Get the setting name from the template
        String key = settings_template.get_key(i);
        //If this particular setting is blank...
        if(settings.get_value(key) == ""){
            //Get the value of the default
            String template_value = settings_template.get_value(key);
            //If there is no default but the setting is required, it's not valid
            if(template_value == "REQ"){
                is_valid = false;
            //Otherwise, just store the default value
            }else{
                settings.put(key, template_value);
            }
        }
    }

    return is_valid;
}

/*  begin: Start the web interface
    RETURNS True if the settings file is good, false if it's missing anything
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool Web_Interface::begin(){

    //When the settings file is requested or posted, call the corresponding function
    server.on("/settings.txt", HTTP_POST, handle_settings_post);
    server.on("/settings.txt", HTTP_GET, handle_settings_get);

    //If any other file is requested, send it if it exists or send a generic 404 if it doesn't exist
    server.onNotFound([](){
        if(!handle_file_read(server.uri())){
            server.send(404, "text/plain", "404: Not Found");
        }
    });

    //If a websockets message comes in, call this function
    websockets_server.onEvent(websockets_event);

    server.begin(); //Start the server
    websockets_server.begin(); //Start the websockets server

    //If a console.txt file exists, delete it to start with a clean console upon init
    if(SPIFFS.exists("/www/console.txt")) SPIFFS.remove("/www/console.txt"); 

    //If the settings file is good, return true. Otherwise, return false. 
    if(check_settings_file()){
        return true;
    }
    return false;
}

/*  handle: Check for incoming requests to the server and to the websockets server
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Web_Interface::handle(){
    server.handleClient();
    websockets_server.loop();
}

/*  console_print: Print to the web console
        output: Text to print
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Web_Interface::console_print(String output){
    //If there is an active websockets connection, send the text to the client
    if(websockets_client != -1){
        websockets_server.sendTXT(websockets_client, output);
    }
    
    //If persistent_console is true, append the text to the console file
    if(persistent_console){
        //Open/create the console.txt file in append mode
        File console = SPIFFS.open("/www/console.txt", "a");

        //If the console file is over 10kb, delete it and create a new one. 
        if(console.size() > 10000){
            console.close();
            SPIFFS.remove("/www/console.txt");
            console = SPIFFS.open("/www/console.txt", "w");
        }

        //output the current string to the end of the file
        console.print(output); 
        //Close the file
        console.close();
    }
    

}

/*  load_setting: 
    RETURNS the specified setting
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String Web_Interface::load_setting(String setting){
    return settings.get_value(setting);
}