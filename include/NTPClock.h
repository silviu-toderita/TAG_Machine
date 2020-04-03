#include "Arduino.h"

class NTPClock{
    public:

    NTPClock(char*, int8_t);
    bool handle();
    bool begin(uint32_t);

    uint32_t getUNIXTimeExt();
    String getYear();
    String getMonth(bool);
    String getTextMonth(bool);
    String getDayOfMonth(bool);
    String getDayOfWeek(bool);
    String getHour(bool, bool);
    String getAMPM();
    String getMinute(bool);
    String getSecond(bool);

    String getDateTime();
    String getTimestamp();

    String convertDateTime(String);

};