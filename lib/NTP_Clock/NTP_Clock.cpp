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

#include "NTP_Clock.h"
#include "WiFiUdp.h" //UDP Library
#include "ESP8266WiFi.h" //WiFi Library for hostname resolution
#include "WiFiClient.h" //Web Client Library

WiFiUDP UDP; //Create a UDP object

char* server_address; //Web Address for time server
IPAddress server_address_IP; //IP Address for time server
const uint8_t NTP_packet_size = 48; //Packet size of NTP messages
byte NTP_buffer[NTP_packet_size]; //NTP packet buffer

uint16_t request_interval; //Frequency of NTP requests in seconds
uint64_t last_request_millis = 0; //Time of last NTP request
uint64_t last_response_millis; //Time of last NTP response
uint32_t time_at_last_response = 0; //UNIX Time at last NTP response

int16_t timezone_offset = 0; //Offset from UTC in minutes

bool timezone_valid; //Is the current timezone valid
bool timezone_auto; //Timezone obtained automatically

uint32_t external_UNIX_time = 0;

/*  NTP_Clock Constructor
        server: NTP server address
        interval: NTP Update Interval in seconds
        timezone: Timezone offset from UTC in minutes (- or +). Can optionally be
            NTP_CLOCK_AUTO to get automatic timezone
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
NTP_Clock::NTP_Clock(char* server, uint16_t interval, int16_t timezone){ 
    server_address = server;
    request_interval = interval;
    timezone_offset = timezone;

    //If the timezone is automatic, set the valid status to false as it must be obtained later
    if(timezone == NTP_CLOCK_AUTO){
        timezone_valid = false;
        timezone_auto = true;
    //If a timezone is defined, set validity to true
    }else{
        timezone_valid = true;
        timezone_auto = false;
        timezone_offset = timezone;
    }
}

/*  NTP_Clock Constructor (with defaults)
        server: time.google.com
        interval: 10 minutes
        timezone: auto
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
NTP_Clock::NTP_Clock(){ 
    NTP_Clock((char*)"time.google.com", 600, NTP_CLOCK_AUTO);
}

/*  send_NTP_Packet: Send a request packet to the NTP server
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NTP_Clock::send_NTP_packet(){ 
    memset(NTP_buffer, 0, NTP_packet_size); //Clear NTP_buffer
    NTP_buffer[0] = 0b11100011; //Set first byte of NTP_buffer
    UDP.beginPacket(server_address_IP, 123); //Open a connection to the NTP server on port 123
    UDP.write(NTP_buffer, NTP_packet_size); //Write the NTP_buffer
    UDP.endPacket(); //Close the connection
}

/*  get_UNIX_time: Calculate the current UNIX time based on the 
    last NTP response OR based on the external UNIX time passed 
    from another source
    RETURNS Current UNIX time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint32_t NTP_Clock::get_UNIX_time(){ 
    //If there is no external time defined...
    if(external_UNIX_time == 0){
        //Current time is the time we received at the last NTP response, plus how much time has elapsed since then
        return time_at_last_response + ((millis() - last_response_millis)/1000);
    //If there is an external time defined...
    }else{
        //Return that time adjusted by the timezone_offset
        return external_UNIX_time + timezone_offset*60;
    }
}

/*  handle: Should be run as often as possible, preferably in 
    every loop. Checks if UDP packet should be sent and if UDP 
    packet has been received.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void NTP_Clock::handle(){ 
    //If there is no time yet, attempt to update every 5 seconds. Otherwise the interval is normal. 
    uint32_t interval;
    if(time_at_last_response == 0){ 
        interval = 5000;
    }else{
        interval = request_interval * 1000;
    }

    //If there is no IP address for the timeserver, attempt to get one
    if(!server_address_IP) WiFi.hostByName(server_address, server_address_IP);

    //If there is no valid timezone, attempt to get one
    if(!timezone_valid){
        if(get_timezone()){
            timezone_valid = true;
        }
    }
    
    //If it's been more than the interval since the last NTP Request, send another.
    if(millis() - last_request_millis > interval){ 
        last_request_millis = millis();
        send_NTP_packet();
        //If the timezone offset is 0, there is a high chance that the timezone offset is wrong. Attempt to get another one
        if(timezone_auto && timezone_offset == 0){
            get_timezone();
        }
    }

     //If there is any data in the UDP buffer, process it
    if(UDP.parsePacket() != 0){
        UDP.read(NTP_buffer, NTP_packet_size); //Read the UDP buffer into the NTP_buffer
        uint32_t NTP_time = (NTP_buffer[40] << 24) | (NTP_buffer[41] << 16) | (NTP_buffer[42] << 8) | NTP_buffer[43]; //Extract NTP time from the bytes
        time_at_last_response = NTP_time - 2208988800UL + timezone_offset*60; //Convert NTP time to UNIX Time, and take into account time zone
        last_response_millis = millis(); //Update the time of last NTP response
    }
}

/*  status:
    RETURNS True if there is a valid time, false if not.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool NTP_Clock::status(){
    if(!time_at_last_response) return false;
    return true;
}

/*  begin: Should be run after a network connection is 
    established. 
        timeout: How long to wait for an initial connection
    RETURNS 
        True if there is a valid time
        False if there is not a valid time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool NTP_Clock::begin(uint32_t timeout){ 
    if(status()) return true;
    
    UDP.begin(123); //Begin UDP connection

    //Try and get initial time within the specified timeout. If that fails, return false. 
    uint32_t start = millis();
    while(millis() < start + timeout){
        handle();
        if(status()) return true;
        yield();
    }

    return false;
    
}

/*  begin (with defaults)
        timeout: 5 seconds
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool NTP_Clock::begin(){
    //Run begin function with a timeout of 5s
    return begin(5000);
}

/*  get_timezone: Get the timezone from worldtimeAPI.org based on current IP, and
        save it if successful
    RETURNS True if successful, false if not
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool NTP_Clock::get_timezone(){
    WiFiClient client; //Create a client object
    client.setTimeout(5000);
    //If connection to the URL established...
    if(client.connect("worldtimeapi.org", 80)){
        //Send a request for the current timezone based on our IP
        client.println("GET /api/ip.txt HTTP/1.1\r\nHost: worldtimeapi.org\r\nConnection: close\r\n\r\n");

        //Read the response
        String response;
        while (client.connected() || client.available()){
                if(client.available()){
                        response += client.readStringUntil('\n') + "\n";
                }
                yield();
        }

        //Parse response to get utc_offset
        String utc_offset = response.substring(response.indexOf("utc_offset: ") + 12, response.indexOf("\nweek_number:"));
        //Timezone offset is equal to the hours + minutes of UTC offset
        timezone_offset = utc_offset.substring(0, 3).toInt() * 60 + utc_offset.substring(4,6).toInt();

        return true;
    }

    //If connection to the URL is not established, return false
    return false;

}

/*  get_leap_years
    RETURNS number of leap years since 2019
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint8_t NTP_Clock::get_leap_years(){ 
    return (get_UNIX_time() - 1483228800) / 126230400; //(Current time - time@Jan 1, 2017) / Seconds in 4 years
}


/*  get_year
    RETURNS current year
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint16_t NTP_Clock::get_year(){
    return ((get_UNIX_time() - 1546300800 - (get_leap_years() * 86400)) / 31536000) + 2019; //((Current time - Time Jan 1, 2019 - leap day differential) / seconds in a year) + 2019
}

/*  get_month_number
        add_zero: Add a zero before single-digit number
    RETURNS current month as a number (String)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_month_number(bool add_zero){
    uint16_t adjusted_day_of_year = get_day_of_year();

    //If it's a leap year, adjust the day of the year by -1 for all months except January, as each month will end one day later. 
    if(get_year() % 4 == 0){
        adjusted_day_of_year = adjusted_day_of_year - 1;
    }

    uint8_t month;
    if(get_day_of_year() <= 31){
        month = 1;
    }else if(adjusted_day_of_year <= 59){
        month = 2;
    }else if(adjusted_day_of_year <= 90){
        month = 3;
    }else if(adjusted_day_of_year <= 120){
        month = 4;
    }else if(adjusted_day_of_year <= 151){
        month = 5;
    }else if(adjusted_day_of_year <= 181){
        month = 6;
    }else if(adjusted_day_of_year <= 212){
        month = 7;
    }else if(adjusted_day_of_year <= 243){
        month = 8;
    }else if(adjusted_day_of_year <= 273){
        month = 9;
    }else if(adjusted_day_of_year <= 304){
        month = 10;
    }else if(adjusted_day_of_year <= 334){
        month = 11;
    }else{
        month = 12;
    }

    //If add_zero is true, add a 0 before a single digit
    if(add_zero && month <= 9) return "0" + String(month);

    //Otherwise, return the month as-is
    return String(month);
   
}

/*  get_month_text
        short_month: Shorten month name to 3 letters
    RETURNS current month as a word
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_month_text(bool short_month){
    uint8_t month_number = get_month_number(false).toInt();
    
    String month;
    if(month_number == 1){
        month = "January";
    }else if(month_number == 2){
        month = "February";
    }else if(month_number == 3){
        month = "March";
    }else if(month_number == 4){
        month = "April";
    }else if(month_number == 5){
        month = "May";
    }else if(month_number == 6){
        month = "June";
    }else if(month_number == 7){
        month = "July";
    }else if(month_number == 8){
        month = "August";
    }else if(month_number == 9){
        month = "September";
    }else if(month_number == 10){
        month = "October";
    }else if(month_number == 11){
        month = "November";
    }else{
        month = "December";
    }

    //If short_month is true, return the first 3 letters of the month only
    if(short_month) return month.substring(0,3); 
    return month;
}

/*  get_day_of_year
    RETURNS current day of the year
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint16_t NTP_Clock::get_day_of_year(){
    //Time at the start of the year = time at start of 2019 + ((current year - 2019) * time in a year) + (number of leap years since 2019 * time in a day)
    uint32_t time_at_start_of_year = 1546300800 + ((get_year() - 2019) * 31536000 + (get_leap_years() * 86400));
    //return current time - time at the start of this year / time in a day + 1
    return (get_UNIX_time() - time_at_start_of_year) / 86400 + 1;
}

/*  get_day_of_month
        add_zero: Add a zero before single-digit number
    RETURNS currrent day of the month
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_day_of_month(bool add_zero){
    uint16_t adjusted_day_of_year = get_day_of_year();

    //If it's a leap year, adjust the day of the year by -1 for all months except January and February, as each month will start one day later. 
    if(get_year() % 4 == 0){
        adjusted_day_of_year = adjusted_day_of_year - 1;
    }
    
    uint8_t month_number = get_month_number(false).toInt();

    uint8_t day_of_month;
    if(month_number == 1){
        day_of_month = get_day_of_year();
    }else if(month_number == 2){
        day_of_month = get_day_of_year() - 31;
    }else if(month_number == 3){
        day_of_month = adjusted_day_of_year - 59;
    }else if(month_number == 4){
        day_of_month = adjusted_day_of_year - 90;
    }else if(month_number == 5){
        day_of_month = adjusted_day_of_year - 120;
    }else if(month_number == 6){
        day_of_month = adjusted_day_of_year - 151;
    }else if(month_number == 7){
        day_of_month = adjusted_day_of_year - 181;
    }else if(month_number == 8){
        day_of_month = adjusted_day_of_year - 212;
    }else if(month_number == 9){
        day_of_month = adjusted_day_of_year - 243;
    }else if(month_number == 10){
        day_of_month = adjusted_day_of_year - 273;
    }else if(month_number == 11){
        day_of_month = adjusted_day_of_year - 304;
    }else{
        day_of_month = adjusted_day_of_year - 334;
    }

    //If add_zero is true, add it before a single-digit number
    if(add_zero && day_of_month <=9 ) return "0" + String(day_of_month);

    //Otherwise, return the day as-is
    return String(day_of_month); 
        
}

/*  get_day_of_week
        short_day: Shorten day name to 3 letters
    RETURNS currrent day of the week
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_day_of_week(bool short_day){

    //days_since_start_of_2019 = (current time - time at start of 2019) / time in a day
    uint16_t days_since_start_of_2019 = (get_UNIX_time() - 1546300800) / 86400;

    String day;
    if(days_since_start_of_2019 % 7 == 0){
        day = "Tuesday";
    }else if(days_since_start_of_2019 % 7 == 1){
        day = "Wednesday";
    }else if(days_since_start_of_2019 % 7 == 2){
        day = "Thursday";
    }else if(days_since_start_of_2019 % 7 == 3){
        day = "Friday";
    }else if(days_since_start_of_2019 % 7 == 4){
        day = "Saturday";
    }else if(days_since_start_of_2019 % 7 == 5){
        day = "Sunday";
    }else{
        day = "Monday";
    }

    //If short_day is true, return the first three letters of the day only
    if(short_day) return day.substring(0,3);
    return day;

}

/*  get_AM_PM
    RETURNS AM or PM
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_AM_PM(){
    //Current hour = (current time / time in an hour) remainder / 24
    uint8_t hour = get_UNIX_time() / 3600 % 24;
    if(hour <= 11){
        return "AM";
    }
    return "PM";
}

/*  get_hour
        add_zero: Add a zero before single-digit number
        format_24_hour: 24 hour format
    RETURNS currrent hour
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_hour(bool add_zero, bool format_24_hour){ 
    //Current hour = (current time / time in an hour) remainder / 24
    uint8_t hour = get_UNIX_time() / 3600 % 24;

    uint8_t output;

    if(format_24_hour){ //If 24-hour format, don't modify the hour
        output = hour;
    }else if(hour == 0){ //If not 24-hour format and the hour is zero, change it to 12
        output = 12;
    }else if(hour <= 12){ //If it's less than or equal 12, keep it as-is
        output = hour;
    }else{  //If it's between 13 and 23, subtract 12
        output = hour - 12;
    }

    //add_zero is true, add 0 before a single-digit number
    if(add_zero && output <= 9) return "0" + String(output);
    //Otherwise, return the hour as-is
    return String(output);
    
}

/*  get_minute
        add_zero: Add a zero before single-digit number
    RETURNS currrent minute
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_minute(bool add_zero){
    //minute = (current time / 60) remainder of / 60
    uint8_t minute = get_UNIX_time() / 60 % 60;

    //If add_zero is true, add a 0 before a single-digit minute
    if(minute <= 9 && add_zero) return "0" + String(minute);

    //Otherwise, return the minute as-is
    return String(minute);
}

/*  get_second
        add_zero: Add a zero before single-digit number
    RETURNS currrent second
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_second(bool add_zero){
    //Second = (current time) remainder / 60
    uint8_t second = get_UNIX_time() % 60;

    //If add_zero is true, add 0 before a single digit second
    if(add_zero && second <= 9) return "0" + String(second);

    //Otherwise, return the second as-is
    return String(second);
}

/*  get_date_time
    RETURNS currrent date and time formatted as: 
    "DAY MON DD, YEAR - HH:MMam"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_date_time(){
    //If current time is unknown, return blank date_time
    if(!status()) return "### ### ##, #### - ##:####";

    return get_day_of_week(true) + " " + get_month_text(true) + " " + get_day_of_month(false) + ", " + String(get_year()) + " - " + get_hour(false, false) + ":" + get_minute(true) + get_AM_PM();
}

/*  get_date_time
        external_time: current UNIX time from external source
    RETURNS currrent date and time formatted as: 
    "DAY MON DD, YEAR - HH:MMam"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_date_time(uint32_t external_time){
    //Set external_UNIX_time based on input
    external_UNIX_time = external_time;
    //Get date/time string
    String date_time = get_day_of_week(true) + " " + get_month_text(true) + " " + get_day_of_month(false) + ", " + String(get_year()) + " - " + get_hour(false, false) + ":" + get_minute(true) + get_AM_PM();;
    //Delete external_UNIX_time
    external_UNIX_time = 0;

    return date_time;
}

/*  get_timestamp
    RETURNS currrent date and time formatted as: 
    "YEAR/MM/DD-HH:MM:SS"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String NTP_Clock::get_timestamp(){
    //If time is unknown, return a blank timestamp
    if(!status()) return "####/##/##-##:##:##";
    
    return String(get_year()) + "/" + get_month_number(true) + "/" + get_day_of_month(true) + "-" + get_hour(true, true) + ":" + get_minute(true) + ":" + get_second(true);
}

