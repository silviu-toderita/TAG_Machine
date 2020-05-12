#include "Arduino.h"
#include "FS.h" //SPI Flash File System (SPIFFS) Library

class Persistent_Storage{
    
    public:

        Persistent_Storage(String);

        void 
            remove(String);
    
        bool
            put(String, String);

        String 
            get_key(uint16_t),
            get_value(String),
            get_sub_value(String, uint8_t);

        uint8_t 
            get_number_values(String);

        uint16_t
            get_number_entries();

    private:

        String filename; //The filename for this object
        bool file_exists = false; //True if the file exists, false if it does not   

};