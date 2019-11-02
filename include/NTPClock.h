#include "Arduino.h"

class NTPClock{
    public:

    NTPClock(char*, int8_t);
    bool handle();
    bool begin(uint32_t);

    uint16_t getYear();
    uint8_t getMonth();
    uint8_t getSecond();
    String getMinute();
    uint8_t getHour();
    String getAMPM();
    uint8_t getDayOfMonth();
    String getDayOfWeek();
    String getStringMonth();
    String getLongDate();
    String getShortDate();
    String getTime();

};