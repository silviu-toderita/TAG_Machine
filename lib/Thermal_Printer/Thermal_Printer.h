#include "Arduino.h"
#include "FS.h"
#include "WiFiClient.h"
#include "ESP8266HTTPClient.h"

//Define control characters
#define ASCII_TAB '\t'
#define ASCII_DC2  18 
#define ASCII_ESC  27
#define ASCII_GS   29 

class Thermal_Printer{
    public:

        Thermal_Printer(bool);

        void    
            config(uint32_t, uint8_t, bool),
            begin(),
            set_printing_parameters(uint8_t, uint8_t, uint8_t),
            offline(),
            
            print_status(String, uint8_t),
            print_title(String, uint8_t),
            print_heading(String, uint8_t),
            print_message(String, uint8_t),
            print_error(String, uint8_t),
            print_line(uint8_t, uint8_t),
            print_bitmap_file(File, uint8_t),
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
            write_bytes(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t),
            write_bytes(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t),
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

        WiFiClient wifiClient; 

        uint32_t baud_rate = 0; //Baud rate of printer
        uint8_t DTR_pin = 0; //ESP8266 pin to use to detect DTR

        uint8_t printMode = 0; //printMode byte holds inverse, double height, double width, and bold font status
        
        bool 
            debugMode,
            img_web; // Print images from web
};