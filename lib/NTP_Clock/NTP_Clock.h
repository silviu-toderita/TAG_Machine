/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    An NTP library for the ESP8266. Able to get accurate time from 
    an NTP server down to the second. Will calculate for leap 
    years to year 2100. Able to get timezone automatically based 
    on IP Address location.

    To use, initialize an NTP_Clock object. Call the 
    NTP_Clock.begin() function after connecting to a network, and 
    then call the NTP_Clock.handle() function in every loop. 

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Arduino.h"
#define NTP_CLOCK_AUTO -1

class NTP_Clock{
    public:

        NTP_Clock(char*, uint16_t, int16_t);
        NTP_Clock();

        void
            handle();

        bool 
            status(),
            begin(uint32_t),
            begin(),
            get_timezone();

        uint16_t 
            get_year();
    
        String 
            get_month_number(bool),
            get_month_text(bool),
            get_day_of_month(bool),
            get_day_of_week(bool),
            get_hour(bool, bool),
            get_AM_PM(),
            get_minute(bool),
            get_second(bool),
            get_date_time(),
            get_date_time(uint32_t),
            get_timestamp();
            

    private:

        void 
            send_NTP_packet();

        uint8_t 
            get_leap_years();

        uint16_t 
            get_day_of_year();

        uint32_t 
            get_UNIX_time();

};