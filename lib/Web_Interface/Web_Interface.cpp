/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ESP8266 library for hosting a simple web interface and web console for an ESP
    that can't be plugged in to use the serial monitor. 

    To use, initialize a Web_Interface object and call handle() every loop or as
    often as possible. Call console_print() to output a line to the console. Place
    files for server in /www/ folder in SPIFFS.

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Web_Interface.h"
#include "FS.h" //SPI File System Library
#include "ESP8266WebServer.h" //Web Server Library
#include "WebSocketsServer.h" //WebSockets Server Library

ESP8266WebServer server(80); //Create a web server listening on port 80
WebSocketsServer websockets_server = WebSocketsServer(81); //Create a websockets server listening on port 81

int8_t websockets_client = -1; //Current websockets client number connected to (-1 is none)
bool persistent_console; //Flag to persist console data when not on console page

/*  get_content_type: Returns the HTTP content type based on the extension
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

/*  handle_file_read: Read a file from SPIFFS and serve it when requested.
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

/*  websockets_event: Called when a new websockets event happens
        num: client number
        type: event type
        payload: message payload
        length: message length
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void websockets_event(uint8_t num, WStype_t type, uint8_t * payload, size_t length) { // When a WebSocket message is received

    //Take different action based on the type of event
    switch (type) {
        //When the websockets client is disconnected...
        case WStype_DISCONNECTED:             
            websockets_client = -1;
            break;

        //When the websockets client is connected...
        case WStype_CONNECTED:
            //Stpre the client number
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

/*  Web_Interface Constructor
        persistent_console_in: Set true to persist console when console page is
            not open
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
Web_Interface::Web_Interface(bool persistent_console_in){
    persistent_console = persistent_console_in;

    //If a file is requested, send it if it exists or send a generic 404 if it doesn't exist
    server.onNotFound([](){
        if(!handle_file_read(server.uri())){
            server.send(404, "text/plain", "404: Not Found");
        }
    });

    //If a websockets message comes in, call this function
    websockets_server.onEvent(websockets_event);

    SPIFFS.begin(); //Start the file system
    server.begin(); //Start the server
    websockets_server.begin(); //Start the websockets server

    //If a console.txt file exists, delete it to start with a clean console upon init
    if(SPIFFS.exists("/www/console.txt")) SPIFFS.remove("/www/console.txt"); 

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

