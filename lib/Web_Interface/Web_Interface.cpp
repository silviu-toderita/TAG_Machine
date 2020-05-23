/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ESP8266 library for hosting a simple web interface. Features:
    -Web console for an ESP that can't be plugged in to use the serial monitor. 
    -Dynamic settings page

    To use, initialize a Web_Interface setting and call handle() every loop or as
    often as possible. Call console_print() to output a line to the console. Place
    files for server in /www/ folder in SPIFFS. 

    To use the settings function, place a settings.txt file in the root 
    of the SPIFFS. Settings must be in the following JSON format:
    [
        {"category":"1st Settings Category",
        "settings":[
            {"id":"",
            "type":"",
            "name":"",
            "desc":"",
            "req":true,
            "val":""}
            ]
        },
        {"category":"2nd Settings Category",
        "settings":[

            ]
        }
    ]

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

    //If the file exists in the root folder instead of the /www/ folder, stream it to the client (this is for debugging non-server files)
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

/*  (private)text_input_HTML: Create the html for a text form input
        id: setting id
        val: current or default setting value
        type: Setting type, valid inputs are "num", "pass", or "text". Anything else defaults to "text"
    RETURNS complete HTML
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String text_input_HTML(String id, String val, String type, bool req){
    //Create the string that will hold the response
    String response = "<input type=\"";

    //Specify the type
    if(type == "num"){
        response += "number";
    }else if(type == "pass"){
        response += "password";
    }else{
        response += "text";
    }
    
    //Create the input field
    response += "\" class=\"form-control\" id=\"" + id + "\" name=\"" + id + "\" aria-describedby=\"" + id +"help\" value=\"" + val + "\"";
    //If this setting is required, make it a required field 
    if(req){
        response += "required";
    }

    response += ">";

    return response;
}

/*  (private)multi_input_HTML: Create the html for a multiple choice input
        id: setting id
        val: current or default setting value
        type: Setting type, valid inputs are "multi" or "bool"
        opt: a JsonArray of possible options, only required for "multi" type
    RETURNS complete HTML
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String multi_input_html(String id, String val, String type, JsonArray opt){

    //Create the string that will hold the response
    String response = "<select class=\"form-control\" id=\"" + id + "\" name=\"" + id + "\" aria-describedby=\"" + id +"help\">";

    //If this is a multiple-choice setting...
    if(type == "multi"){
        
        //For each option...
        for(int i = 0; i < opt.size(); i++){
            
            //This string holds the option name
            String this_option = opt[i];
            response += "<option value=\"" + this_option + "\"";
            //If the current option is the current value or default, pre-select it on the form 
            if(this_option == val) response += "selected";
            response +=">" + this_option + "</option>"; 
        }
    //Otherwise, this is a boolean setting...
    }else{
        //Create the On option
        response += "<option value=true";
        //If the current value is true, pre-select the Yes option
        if(val){
            response += "selected";
        }
        response +=">On</option>";

        //Create the Off option
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

    //Parse JSON from file
    DeserializationError error = deserializeJson(doc, file);

    //Close the file
    file.close();

    //If there is an error, send the response to the browser and exit the function
    if(error){
        server.send(200, "text/html", "<h3>Invalid settings file or no settings defined!</h3>");
        return;
    } 

    //This will hold the status of whether or not there will be a tabbed interface
    bool tabbed_interface = false;
    //If there are no settings categories, send the response to the browser and exit the function
    if(doc.size() == 0){
        server.send(200, "text/html", "<h3>No settings defined!</h3>");
        return;
    //If there is more than 1 settings category, there will need to be a tabbed interface
    }else if(doc.size() > 1){
        tabbed_interface = true;
    }

    //These will hold the HTML response to the browser
    String response_header = "";
    String response;

    //If there is a tabbed interface, create the nav area
    if(tabbed_interface){
        response_header = "<ul class=\"nav nav-tabs\" id=\"settings_nav\" role=\"tablist\">";
        response = "<div class=\"tab-content\" id=\"settings_nav_content\">";
    }

    //For each setting category...
    for(int x = 0; x < doc.size(); x++){
        
        //If there is a tabbed interface, create this tab
        if(tabbed_interface){
            //Get the category name
            String category_name = doc[x]["category"];

            //Add the HTML for the list
            response_header += "<li class=\"nav-item\">";

            //Add the HTML for the link
            response_header += "<a class=\"nav-link"; 
            //If this is the first category, make it active
            if(x == 0) response_header += " active"; 
            response_header += "\" id=\"category" + String(x) + "-tab\" data-toggle=\"tab\" href=\"#category" + String(x) + "\" role\"tab\" aria-controls=\"category" + String(x) + "\" aria-selected=\"";
            //If this is the first category, make it active
            if(x == 0){
                response_header += "true";
            }else{
                response_header += "false";
            }
            response_header += "\">" + category_name + "</a>";
            response_header += "</li>";

            //Add the HTML for the content of the tab
            response += "<div class=\"tab-pane fade";
            //If this is the first category, make it active
            if(x == 0) response += " show active";
            response += "\" id=\"category" + String(x) + "\" role=\"tabpanel\" aria-labelledby=\"category" + String(x) + "-tab\">";
        }

        //The array of settings for this category
        JsonArray settings = doc[x]["settings"];

        //For each setting...
        for(int i = 0; i < settings.size(); i++){
            //Save the setting attributes
            String id = settings[i]["id"];
            String type = settings[i]["type"];
            String name = settings[i]["name"];
            String desc = "";
            if(settings[i].containsKey("desc")) desc = settings[i]["desc"].as<String>();
            String val = "";
            if(settings[i].containsKey("val")) val = settings[i]["val"].as<String>();
            bool req = settings[i]["req"];

            //Form the HTML response
            response += "<div class=\"form-group\">";
            response += "<label for=\"" + id + "\">" + name + "</label>";

            //Based on the type of setting, get the HTML
            if(type == "multi" || type == "bool"){
                response += multi_input_html(id, val, type, settings[i]["opt"]);
            }else{
                response += text_input_HTML(id, val, type, req);
            }
            
            //Add help text if the description is defined
            response +=     "<small id=\"" + id + "help\" class=\"form-text text-muted\">" + desc + "</small>";
            response += "</div>";

        }

        //If there is a tabbed interface, close out this tab
        if(tabbed_interface) response += "</div>";

    }

    //If there is a tabbed interface, close out the tab list
    if(tabbed_interface){
        response_header += "</ul><br>";
        response += "</div>";
    }

    //Send the response to the browser
    server.send(200, "text/html", response_header + response);
}

/*  (private)handle_settings_post: Receive new settings from the browser
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void handle_settings_post(){
    //Confirm that the settings have been received
    server.send(200);

    //Open the file for reading
    File file = SPIFFS.open(settings_path, "r");
    //Set aside enough memory for a JSON document
    DynamicJsonDocument doc(file.size() * 2);

    //Parse JSON from file 
    deserializeJson(doc, file);
    //Close the file
    file.close();

    uint8_t current_server_arg = 0;

    //For each setting category...
    for(int y = 0; y < doc.size(); y++){
        //Array of settings in this category
        JsonArray settings = doc[y]["settings"];
        //For each setting...
        for(int x = 0; x < settings.size(); x++){
            //Copy the server argument to the current value and increment the server argument
            settings[x]["val"] = server.arg(current_server_arg);
            current_server_arg++;

        }
    }

    //Open the file for writing
    file = SPIFFS.open(settings_path, "w");
    //Encode the JSON in the file
    serializeJson(doc, file);
    //Close the file
    file.close();

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

    //Parse JSON from file
    DeserializationError error = deserializeJson(doc, file);

    //Close the file
    file.close();

    //If there is no error, save the number of settings
    if(error){
        return false;
    } 
    
    //For each setting category...
    for(int x = 0; x < doc.size(); x++){

        //Array of settings in this category
        JsonArray settings = doc[x]["settings"];
        //For each setting...
        for(int i = 0; i < settings.size(); i++){
            //If this setting is required...
            if(settings[i]["req"] == true){
                //If this setting has a value...
                if(settings[i].containsKey("val")){
                    //If this setting has a blank value, return false
                    if(settings[i]["val"] == "") return false;
                //If this setting doesn't have a value, return false
                }else{
                    return false;
                }
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

    //Parse JSON from file
    DeserializationError error = deserializeJson(doc, file);
    //Close the file
    file.close();
    //If there is an error, return a blank response
    if(error){
        return "";
    } 

    //For each setting category...
    for(int x = 0; x < doc.size(); x++){

        //Array of settings in this category
        JsonArray settings = doc[x]["settings"];
        //For each setting...
        for(int i = 0; i < settings.size(); i++){
            //If the current setting is the one specified, return its value
            if(settings[i]["id"] == setting){
                return settings[i]["val"];
            }
        }
    }

    //If nothin has been returned so far, return an empty string
    return "";     

}