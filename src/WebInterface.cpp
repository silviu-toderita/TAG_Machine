#include "FS.h" //SPI File System Library
#include "ESP8266WebServer.h" //Web Server Library
#include "WebInterface.h"

ESP8266WebServer server(80); //Create a web server for the local web interface

//This function returns the HTTP content type for a given extension
String getContentType(String filename){ 
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
  return "text/plain"; //If none of the above, it's plain text
}

//This function returns the requested file from the local web server
bool handleFileRead(String path){  
  //Append /www to the beginning of the path
  if(path != "/phoneBook.sto") path = "/www" + path;

  if(path.endsWith("/")) path += "index.html"; //If only a folder is specified, return index.html

  String contentType = getContentType(path); //Figure out the content type

  String pathWithGz = path + ".gz"; //Save path with .gz extension in case we have a compressed file

  //If the file exists, stream it to the client
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){  
    if(SPIFFS.exists(pathWithGz)) path += ".gz"; //If the compressed version of the file exists, stream that one instead   

    File file = SPIFFS.open(path, "r"); //Open the file                     
    server.streamFile(file, contentType); //Stream the file   
    file.close(); //Close the file                                        
    return true;
  }
  return false;                                         
}

//Create the WebInterface object
WebInterface::WebInterface(){

    //If a file is requested, send it if it exists or send a generic 404 if it doesn't exist
    server.onNotFound([](){
        if(!handleFileRead(server.uri())){
          server.send(404, "text/plain", "404: Not Found");
        }
    });

    SPIFFS.begin(); //Start the file system
    server.begin(); //Start the server

    if(SPIFFS.exists("/www/console.txt")) SPIFFS.remove("/www/console.txt"); //If a console.txt file exists, delete it

}

//Handler function to be run in the loop
void WebInterface::handle(){
    server.handleClient();
}

//Output a line to the web console
void WebInterface::println(String output){
  //Open/create the console.txt file in append mode
  File console = SPIFFS.open("/www/console.txt", "a");
  console.println(output); //output the current string to the end of the file
  console.close(); //Close the file
}