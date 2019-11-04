#include "FS.h" //SPI File System Library
#include "WebInterface.h"

ESP8266WebServer server(80); //Create a web server for the local web interface

String getContentType(String filename){ //This function returns the HTTP content type for a given extension
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){  //This function returns the requested file from the local web server
  if(path.endsWith("/")) path += "index.html"; 
  String contentType = getContentType(path);          
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){  
    if(SPIFFS.exists(pathWithGz))                          
      path += ".gz";                                         
    File file = SPIFFS.open(path, "r");                    
    server.streamFile(file, contentType);    
    file.close();                                          
    return true;
  }
  return false;                                         
}

WebInterface::WebInterface(){

    server.onNotFound([](){
        if(!handleFileRead("/www" + server.uri()))
        server.send(404, "text/plain", "404: Not Found");
    });

    SPIFFS.begin();
    server.begin();
    
}

void WebInterface::handle(){
    server.handleClient();
}