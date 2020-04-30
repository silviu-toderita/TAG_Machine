#include "Arduino.h"

class Persistent_Storage{
    public:

        Persistent_Storage(String);

        void 
            put(String, String),
            remove(String);
    
        String 
            get(String);
};