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
#include "ESP8266WiFi.h" //WiFi Library for hostname resolution
#include "WiFiClient.h" //Web Client Library

class WTA_Clock{
    public:

        WTA_Clock();

        void
            config(uint16_t),
            begin(),
            handle();

        bool 
            status(),
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

    uint16_t request_interval; //Frequency of NTP requests in seconds
    uint64_t last_response_millis; //Time of last NTP response
    uint32_t time_at_last_response = 0; //UNIX Time at last NTP response

    int16_t timezone_offset = 0; //Offset from UTC in minutes

    uint32_t external_UNIX_time = 0; //Holds value of UNIX timestamp passed to class from external source

};