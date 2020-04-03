#include "Arduino.h"

class Storage{
    public:

    Storage(String, uint8_t length);

    bool put(String, String);
    String get(String);
};