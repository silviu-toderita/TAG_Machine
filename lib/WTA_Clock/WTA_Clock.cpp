/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    A clock library for the ESP8266. Able to get accurate time from 
    worldtimeapi.org down to the second. Will calculate for leap years to year 
    2100. Able to get timezone automatically based on IP Address location.

    To use, initialize an WTA_Clock object. Call the 
    WTA_Clock.begin() function after connecting to a network, and 
    then call the WTA_Clock.handle() function in every loop. 

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "WTA_Clock.h"

/*  WTA_Clock Constructor (with defaults)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
WTA_Clock::WTA_Clock(){ 
    config(600);
}

/*  config
        interval: Time Update Interval in seconds (default: 600)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void WTA_Clock::config(uint16_t interval){
    request_interval = interval;
}

/*  get_UNIX_time: Calculate the current UNIX time based on the 
    last NTP response OR based on the external UNIX time passed 
    from another source
    RETURNS Current UNIX time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint32_t WTA_Clock::get_UNIX_time(){ 
    //If there is no external time defined...
    if(external_UNIX_time == 0){
        //Current time is the time we received at the last NTP response, plus how much time has elapsed since then
        return time_at_last_response + ((millis() - last_response_millis)/1000);

    //If there is an external time defined...
    }else{
        //Return that time adjusted by the timezone_offset
        return external_UNIX_time + timezone_offset;
    }
}


/*  handle: Should be run as often as possible, preferably in 
    every loop to update time if needed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void WTA_Clock::handle(){ 

    //If it's been more than the interval since the last API request, send another.
    if(millis() > last_response_millis + request_interval){ 
        WiFiClient client; //Create a client object
        //If connection to the URL established...
        if(client.connect("worldtimeapi.org", 80)){
            //Send a request for the current timezone based on this device's IP
            client.print("GET /api/ip.txt HTTP/1.1\r\nHost: worldtimeapi.org\r\nConnection: close\r\n\r\n");

            //Read the response
            String response;
            while (client.available() || client.connected()){
                    if(client.available()){
                        String line = client.readStringUntil('\n');
                        response += line + "\n";
                        if(line.indexOf("week_number:") != -1) break;
                    }
                    yield();
            }

            //Parse response to get the UTC UNIX timestamp
            String unix_time = response.substring(response.indexOf("unixtime: ") + 10, response.indexOf("\nutc_datetime:"));
            //Parse response to get the timezone offset from UTC
            String utc_offset = response.substring(response.indexOf("utc_offset: ") + 12, response.indexOf("\nweek_number:"));

            //Timezone offset is equal to the hours of UTC offset * 3600 seconds...
            timezone_offset = utc_offset.substring(0, 3).toInt() * 3600;
            //Plus the minutes of UTC offset * 60 seconds. Make the minutes + or - depending on sign of hours
            if(timezone_offset >= 0){
                timezone_offset += utc_offset.substring(4,6).toInt() * 60;
            }else{
                timezone_offset = timezone_offset - utc_offset.substring(4,6).toInt() * 60;
            }

            //Store the current time
            time_at_last_response = unix_time.toInt() + timezone_offset;
            //Store the internal time at which the current time was last received 
            last_response_millis = millis();
        }
    }

}

/*  status:
    RETURNS True if there is a valid time, false if not.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool WTA_Clock::status(){
    if(!time_at_last_response) return false;
    return true;
}

/*  begin: Should be run after a network connection is established and will attempt to get the time twice
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void WTA_Clock::begin(){ 
    if(status()) return;
    handle();
    if(status()) return;
    handle();

}

/*  get_leap_years
    RETURNS number of leap years since 2019
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint8_t WTA_Clock::get_leap_years(){ 
    return (get_UNIX_time() - 1483228800) / 126230400; //(Current time - time@Jan 1, 2017) / Seconds in 4 years
}

/*  get_year
    RETURNS current year
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint16_t WTA_Clock::get_year(){
    return ((get_UNIX_time() - 1546300800 - (get_leap_years() * 86400)) / 31536000) + 2019; //((Current time - Time Jan 1, 2019 - leap day differential) / seconds in a year) + 2019
}

/*  get_month_number
        add_zero: Add a zero before single-digit number
    RETURNS current month as a number (String)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String WTA_Clock::get_month_number(bool add_zero){
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
String WTA_Clock::get_month_text(bool short_month){
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
uint16_t WTA_Clock::get_day_of_year(){
    //Time at the start of the year = time at start of 2019 + ((current year - 2019) * time in a year) + (number of leap years since 2019 * time in a day)
    uint32_t time_at_start_of_year = 1546300800 + ((get_year() - 2019) * 31536000 + (get_leap_years() * 86400));
    //return current time - time at the start of this year / time in a day + 1
    return (get_UNIX_time() - time_at_start_of_year) / 86400 + 1;
}

/*  get_day_of_month
        add_zero: Add a zero before single-digit number
    RETURNS currrent day of the month
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String WTA_Clock::get_day_of_month(bool add_zero){
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
String WTA_Clock::get_day_of_week(bool short_day){

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
String WTA_Clock::get_AM_PM(){
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
String WTA_Clock::get_hour(bool add_zero, bool format_24_hour){ 
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
String WTA_Clock::get_minute(bool add_zero){
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
String WTA_Clock::get_second(bool add_zero){
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
String WTA_Clock::get_date_time(){
    //If current time is unknown, return blank date_time
    if(!status()) return "### ### ##, #### - ##:####";

    return get_day_of_week(true) + " " + get_month_text(true) + " " + get_day_of_month(false) + ", " + String(get_year()) + " - " + get_hour(false, false) + ":" + get_minute(true) + get_AM_PM();
}

/*  get_date_time
        external_time: current UNIX time from external source
    RETURNS currrent date and time formatted as: 
    "DAY MON DD, YEAR - HH:MMam"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String WTA_Clock::get_date_time(uint32_t external_time){
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
String WTA_Clock::get_timestamp(){
    //If time is unknown, return a blank timestamp
    if(!status()) return "####/##/##-##:##:##";
    
    return String(get_year()) + "/" + get_month_number(true) + "/" + get_day_of_month(true) + "-" + get_hour(true, true) + ":" + get_minute(true) + ":" + get_second(true);
}

