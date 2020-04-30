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

ESP8266WebServer server(80); //Create a web server listening on port 80

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

    //If the file exists, stream it to the client
    if(SPIFFS.exists(path)){
        File file = SPIFFS.open(path, "r");                
        server.streamFile(file, content_type);
        file.close();                                    
        return true;
    }

    return false;                                         
}

/*  Web_Interface Constructor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
Web_Interface::Web_Interface(){

    //If a file is requested, send it if it exists or send a generic 404 if it doesn't exist
    server.onNotFound([](){
        if(!handle_file_read(server.uri())){
            server.send(404, "text/plain", "404: Not Found");
        }
    });

    SPIFFS.begin(); //Start the file system
    server.begin(); //Start the server

    //If a console.txt file exists, delete it to start with a clean console upon init
    if(SPIFFS.exists("/www/console.txt")) SPIFFS.remove("/www/console.txt"); 

}

/*  handle: Check for incoming requests to the server
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Web_Interface::handle(){
    server.handleClient();
}

/*  console_print: Print a line to the web console
        output: Text to print
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Web_Interface::console_print(String output){
    //Open/create the console.txt file in append mode
    File console = SPIFFS.open("/www/console.txt", "a");
    //output the current string to the end of the file
    console.println(output); 
    //Close the file
    console.close(); 
}

