/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ESP8266 library for hosting a simple web interface. Features:
    -Web console for an ESP that can't be plugged in to use the serial monitor. 
    -Dynamic settings page

    To use, initialize a Web_Interface setting and call handle() every loop or as
    often as possible. Call console_print() to output a line to the console. Place
    files for server in /www/ folder in SPIFFS. 

    To use the settings function, place a settings.txt file in the root 
    of the SPIFFS. Settings must be in the format of a JSON array, with each object
    in the array having these possible attributes:
    "id" (required): a string with no spaces to name each setting
    "type" (required): text, pass, bool, or multi
    "name" (required): Human readable name
    "req" (required): True or False if this setting is required to be filled in.
    "val": Default value
    "optx": For multi type, each option must be listed as optx where x is a number
        from 0 to inifinity in sequence.

    begin() will return false if there are any required settings that are missing.
    load_setting() allows you to load a setting based on its name. 

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Web_Interface.h"

ESP8266WebServer server(80); //Create a web server listening on port 80
WebSocketsServer websockets_server = WebSocketsServer(81); //Create a websockets server listening on port 81

static void_function_pointer _offline; //Callback function when connected

int8_t websockets_client = -1; //Current websockets client number connected to (-1 is none)

const String settings_path = "/settings.txt"; //Path to settings file

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

    bool cache = false;
    if(path.endsWith(".js") || path.endsWith(".css") || path.endsWith(".ico")){
        cache = true;
    } 

    //If the compressed file exists, stream it to the client
    if(SPIFFS.exists(path + ".gz")){
        File file = SPIFFS.open(path + ".gz", "r"); 
        if(cache) server.sendHeader("Cache-Control", "max-age=2592000");          
        server.streamFile(file, content_type);
        file.close();                                    
        return true;
    }

    //If the file exists, stream it to the client
    if(SPIFFS.exists(path)){
        File file = SPIFFS.open(path, "r");
        if(cache) server.sendHeader("Cache-Control", "max-age=2592000"); 
        server.streamFile(file, content_type);
        file.close();                                    
        return true;
    }

    //If the file exists in the root folder instead of the /www/ folder, stream it to the client
    if(SPIFFS.exists(path.substring(4))){
        File file = SPIFFS.open(path.substring(4), "r");                
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

            //If the console file exists...
            if(SPIFFS.exists("/www/console.txt")){
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

String text_input_HTML(String id, String val, String type){
    String response = "<input type=\"";

    if(type == "num"){
        response += "number";
    }else if(type == "pass"){
        response += "password";
    }else{
        response += "text";
    }
    
    response += "\" class=\"form-control\" id=\"" + id + "\" name=\"" + id + "\" aria-describedby=\"" + id +"help\" value=\"" + val + "\">";

    return response;
}

String multi_input_html(String id, String val, String type, JsonArray opt){
    String response = "<select class=\"form-control\" id=\"" + id + "\" name=\"" + id + "\" aria-describedby=\"" + id +"help\">";

    if(type == "multi"){
        
        for(int i = 0; i < opt.size(); i++){
            
            String this_option = opt[i];
            response += "<option value=\"" + this_option + "\"";
            //If the current option is the current value or default, pre-select it on the form 
            if(this_option == val) response += "selected";
            response +=">" + this_option + "</option>"; 

        }
    }else{
        //Create the Yes option
        response += "<option value=true";
        //If the current value is true, pre-select the Yes option
        if(val){
            response += "selected";
        }
        response +=">On</option>";

        //Create the No option
        response += "<option value=false";
        //If the current value is false, pre-select the No option
        if(!val){
            response += "selected";
        }
        response +=">Off</option>";
    
    }

    response += "</select>";

    return response;
}

/*  (private)handle_settings_get: Send the settings to the browser as an HTML form
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void handle_settings_get(){
    //Open the settings file
    File file = SPIFFS.open(settings_path, "r");
    //Set aside enough memory for a JSON document
    DynamicJsonDocument doc(file.size() * 2);
    //Number of settings present in the file
    uint8_t number_settings = 0;

    //Parse JSON from file
    DeserializationError error = deserializeJson(doc, file);
    if(!error){
        //If there is no issue with the parsing, save the number of settings
        number_settings = doc.size();
    } 

    //Close the file
    file.close();

    //This will hold the HTML response to the browser
    String response;

    //Respond with no settings defined if there are none
    if(number_settings == 0){
        response = "<h3>No Settings Defined!</h3>";
        
    }
    
    //For each setting...
    for(int i = 0; i < number_settings; i++){
        //Save the setting attributes
        String id = doc[i]["id"];
        String type = doc[i]["type"];
        String name = doc[i]["name"];
        String desc = "";
        if(doc[i].containsKey("desc")) desc = doc[i]["desc"].as<String>();
        String val = "";
        if(doc[i].containsKey("val")) val = doc[i]["val"].as<String>();

        //Form the HTML response
        response += "<div class=\"form-group\">";
        response += "<label for=\"" + id + "\">" + name + "</label>";
        //Based on the type of setting, create the input or select object in HTML

        if(type == "multi" || type == "bool"){
            response += multi_input_html(id, val, type, doc[i]["opt"]);
        }else{
            response += text_input_HTML(id, val, type);
        }
        
        //Add help text if the description is defined
        response +=     "<small id=\"" + id + "help\" class=\"form-text text-muted\">" + desc + "</small>";
        response += "</div>";
  
    }
    
    //Send the response to the browser
    server.send(200, "text/html", response);
}

/*  (private)handle_settings_post: Receive new settings from the browser
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void handle_settings_post(){
    //Open the file for reading
    File file = SPIFFS.open(settings_path, "r");
    //Set aside enough memory for a JSON document
    DynamicJsonDocument doc(file.size() * 2);

    //Parse JSON from file 
    deserializeJson(doc, file);
    //Close the file
    file.close();

    //For each server argument except the last one...
    for(int i = 0; i < server.args() - 1; i++){
        //Store the setting
        doc[i]["val"] = server.arg(i);
    }

    //Open the file for writing
    file = SPIFFS.open(settings_path, "w");
    //Encode the JSON in the file
    serializeJson(doc, file);
    //Close the file
    file.close();

    //Confirm that the settings have been received
    server.send(200);

    //Take the tag machine offline and restart
    _offline();
    ESP.restart();
}

/*  Web_Interface Constructor (with defaults)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
Web_Interface::Web_Interface(){
    SPIFFS.begin();
    SPIFFS.gc();
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
    //Open the file
    File file = SPIFFS.open(settings_path, "r");
    //Set aside enough memory for a JSON document
    DynamicJsonDocument doc(file.size() * 2);
    //This will hold the number of settings
    uint8_t number_settings = 0;

    //Parse JSON from file
    DeserializationError error = deserializeJson(doc, file);
    //If there is no error, save the number of settings
    if(!error){
        number_settings = doc.size();
    } 
    //Close the file
    file.close();

    //If there are no settings, return false
    if(number_settings == 0) return false;

    //For each setting in the setting template...
    for(int i = 0; i < number_settings; i++){
        //If this setting is required...
        if(doc[i]["req"] == true){
            //If this setting has a value...
            if(doc[i].containsKey("val")){
                //If this setting has a blank value, return false
                if(doc[i]["val"] == "") return false;
            //If this setting doesn't have a value, return false
            }else{
                return false;
            }

        }
    }

    return true;
}

/*  begin: Start the web interface
    RETURNS True if the settings file is good, false if it's missing anything
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool Web_Interface::begin(){

    //When the settings file is requested or posted, call the corresponding function
    server.on("/settings_data", HTTP_POST, handle_settings_post);
    server.on("/settings_data", HTTP_GET, handle_settings_get);

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

/*  load_setting: 
    RETURNS the specified setting
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String Web_Interface::load_setting(String setting){
    //Open the file for reading
    File file = SPIFFS.open(settings_path, "r");
    //Set aside enough memory for a JSON document
    DynamicJsonDocument doc(file.size() * 2);
    //This will store the number of settings
    uint8_t number_settings = 0;

    //Parse JSON from file
    DeserializationError error = deserializeJson(doc, file);
    //If there is no error, save the number of settings
    if(!error){
        number_settings = doc.size();
    } 
    //Close the file
    file.close();

    //For each setting in the setting template...
    for(int i = 0; i < number_settings; i++){
        //If the current setting is the one specified, return its value
        if(doc[i]["id"] == setting){
            return doc[i]["val"];
        }
    }

    //If nothin has been returned so far, return an empty string
    return "";     

}