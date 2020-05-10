#include "Arduino.h"
#include "FS.h"
#include "ESP8266HTTPClient.h"

//Define control characters
#define ASCII_TAB '\t'
#define ASCII_DC2  18 
#define ASCII_ESC  27
#define ASCII_GS   29 

//Define void callback function type with a single String arg
typedef void (*voidFuncPtrStr)(const String input);

class Thermal_Printer{
    public:

        Thermal_Printer();

        void    
            config(uint32_t, uint8_t),
            begin(voidFuncPtrStr),
            begin(),
            set_printing_parameters(uint8_t, uint8_t, uint8_t),
            set_printing_density(uint8_t, uint8_t),
            suppress(bool),
            offline(),
            
            print_status(String, uint8_t),
            print_title(String, uint8_t),
            print_heading(String, uint8_t),
            print_message(String, uint8_t),
            print_error(String, uint8_t),
            print_line(uint8_t, uint8_t),
            print_bitmap_file(File, uint8_t, String),
            print_bitmap_http(String, uint8_t),
            feed(uint8_t);

    private:

        void
            wake(),
            sleep(),
            write_bytes(uint8_t),
            write_bytes(uint8_t, uint8_t),
            write_bytes(uint8_t, uint8_t, uint8_t),
            write_bytes(uint8_t, uint8_t, uint8_t, uint8_t),
            font_center(bool),
            font_inverse(bool),
            font_double_height(bool),
            font_double_width(bool),
            font_bold(bool),
            font_write_print_mode(),
            output(String),
            wait();

        String  
            wrap(String, uint8_t);

        uint32_t baud_rate = 0; //Baud rate of printer
        uint8_t DTR_pin = 0; //ESP8266 pin to use to detect DTR

        uint8_t printMode = 0; //printMode byte holds inverse, double height, double width, and bold font status
        
        bool suppressed = false; //When printer is suppressed, it won't print but callback will still be called

};