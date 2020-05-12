#include "Arduino.h"
#include "ESP8266WebServer.h" //Web Server Library
#include "WebSocketsServer.h" //WebSockets Server Library
#include "Persistent_Storage.h"

//Callback function type (no args)
typedef void (*void_function_pointer)();

class Web_Interface{
    public:
    
        Web_Interface();
    
        void 
            set_callback(void_function_pointer),
            handle(),
            console_print(String);

        bool
            begin();

        String
            load_setting(String);

};