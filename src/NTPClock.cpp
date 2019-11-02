//The NTPClock object can be used to get accurate time from an NTP server down to the second. Will calculate for leap years until the year 2100. 

#include "NTPClock.h"
#include "WiFiUdp.h" //UDP Library
#include "ESP8266WiFi.h" //WiFi Library for hostname resolution
#include "Counter.h" //Counter library

WiFiUDP UDP; //Create a UDP object

IPAddress timeServerIP; //IP Address for time server (to be defined upon init)
char* NTPServer; //Web Address for time server (to be defined upon init)
const uint8_t NTP_PACKET_SIZE = 48; //Packet size of NTP messages
byte NTPBuffer[NTP_PACKET_SIZE]; //NTP packet bugger

uint16_t intervalNTP = 60000; //Frequency of NTP requests in ms
uint32_t lastNTPRequest = 0; //Time of last NTP request
uint32_t lastNTPResponse; //Time of last NTP response

const uint32_t seventyYears = 2208988800UL; //Seventy Years offset
int8_t hourOffset = 0; //Offset from UTC in hours
uint32_t lastUNIXTime = 0; //UNIX Time at last NTP response

//Initialize object, requires server and hour offset from UTC
NTPClock::NTPClock(char* server, int8_t offset){ 
    NTPServer = server;
    hourOffset = offset;
}

//Send a request to the NTP server via UDP, requires IP address of server
void sendNTPpacket(IPAddress& address){ 
    memset(NTPBuffer, 0, NTP_PACKET_SIZE);
    NTPBuffer[0] = 0b11100011;
    UDP.beginPacket(address, 123);
    UDP.write(NTPBuffer, NTP_PACKET_SIZE);
    UDP.endPacket();
}

//Calculate the current UNIX Time based on the last NTP response
uint32_t getUNIXTime(){ 
    return lastUNIXTime + (millis() - lastNTPResponse)/1000; 
}

//Handle function should be run as frequently as possible to update time via UDP. Returns true if we have a time, false if not. 
bool NTPClock::handle(){ 
    uint32_t currentMillis = millis(); //Store the current millisecond so that we only have to call this function once
    uint16_t interval;

    if(lastUNIXTime == 0){ //If we've not received any time yet, the interval is 5s. Otherwise the interval is normal. 
        interval = 5000;
    }else{
        interval = intervalNTP;
    }

    //If we don't have an IP address for the timeserver, get one
    if(!timeServerIP) WiFi.hostByName(NTPServer, timeServerIP);
    
    if(currentMillis - lastNTPRequest > interval){ //If it's been more than the interval since the last NTP Request, send another.
        lastNTPRequest = currentMillis;
        sendNTPpacket(timeServerIP);
    }

    if(UDP.parsePacket() != 0){ //If there is any data in the UDP buffer, process it
        UDP.read(NTPBuffer, NTP_PACKET_SIZE); //Read the buffer
        uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43]; //Extract NTP time from the bytes
        lastUNIXTime = NTPTime - seventyYears + hourOffset*3600; //Convert NTP time to UNIX Time, and take into account time zone
        lastNTPResponse = currentMillis; //Update the time of last NTP response
    }

    if(!lastUNIXTime) return false;
    return true;
}

//Begin the clock, specify timeout. Returns true if it could get an update from the time server within timeout, false if not.
bool NTPClock::begin(uint32_t timeout){ 
    
    //Begin UDP connection
    UDP.begin(123);

    //Get an IP address for the timeserver
    WiFi.hostByName(NTPServer, timeServerIP);

    Counter counter(timeout);
    while(!counter.status()){
        if(handle()) return true;
        yield();
    }

    return false;
    
}

//Returns number of completed leap years since 2019
uint8_t getLeapYears(){ 
    //(Current time - time@Jan 1, 2017) / Seconds in 4 years
    return (getUNIXTime() - 1483228800) / 126230400;
}

//Returns the year (internal function)
uint16_t intGetYear(){
    //((Current time - Time Jan 1, 2019 - leap day differential) / seconds in a year) + 2019
    return ((getUNIXTime() - 1546300800 - (getLeapYears() * 86400)) / 31536000) + 2019;
}

//Returns the year
uint16_t NTPClock::getYear(){
    return intGetYear();
}

//Returns the day of the year
uint16_t getDayOfYear(){
    uint32_t timeAtStartOfYear = 1546300800 + ((intGetYear() - 2019) * 31536000 + (getLeapYears() * 86400));
    return (getUNIXTime() - timeAtStartOfYear) / 86400 + 1;
}

//Returns the Month as an 8-bit unsigned int based on the day of the year
uint8_t NTPClock::getMonth(){
    uint16_t adjustedDayOfYear = getDayOfYear();

    //If it's a leap year, adjust the day of the year by -1 for all months except January, as each month will end one day later. 
    if(getYear() % 4 == 0){
        adjustedDayOfYear = adjustedDayOfYear - 1;
    }

    if(getDayOfYear() <= 31){
        return 1;
    }else if(adjustedDayOfYear <= 59){
        return 2;
    }else if(adjustedDayOfYear <= 90){
        return 3;
    }else if(adjustedDayOfYear <= 120){
        return 4;
    }else if(adjustedDayOfYear <= 151){
        return 5;
    }else if(adjustedDayOfYear <= 181){
        return 6;
    }else if(adjustedDayOfYear <= 212){
        return 7;
    }else if(adjustedDayOfYear <= 243){
        return 8;
    }else if(adjustedDayOfYear <= 273){
        return 9;
    }else if(adjustedDayOfYear <= 304){
        return 10;
    }else if(adjustedDayOfYear <= 334){
        return 11;
    }
    return 12;
}

//Returns the second
uint8_t NTPClock::getSecond(){
    return getUNIXTime() % 60;
}

//Returns the minute as a string, with a 0 appended to the beginning if it's a single digit
String NTPClock::getMinute(){
    uint8_t minute = getUNIXTime() / 60 % 60;

    if(minute <= 9){
        return "0" + String(minute);
    }else{
        return String(minute);
    }

}

//Returns the hour in 12-hour format
uint8_t NTPClock::getHour(){ 
    uint8_t hour = getUNIXTime() / 3600 % 24;
    if(hour == 0){
        return 12;
    }else if(hour <= 12){
        return hour;
    }else{
        return hour - 12;
    }
    
}

//Returns AM or PM as a string
String NTPClock::getAMPM(){
    uint8_t hour = getUNIXTime() / 3600 % 24;
    if(hour <= 11){
        return "AM";
    }else{
        return "PM";
    }
}

//Returns the day of the month
uint8_t NTPClock::getDayOfMonth(){
    uint16_t adjustedDayOfYear = getDayOfYear();

    //If it's a leap year, adjust the day of the year by -1 for all months except January and February, as each month will start one day later. 
    if(getYear() % 4 == 0){
        adjustedDayOfYear = adjustedDayOfYear - 1;
    }
    
    if(getMonth() == 1){
        return getDayOfYear();
    }else if(getMonth() == 2){
        return getDayOfYear() - 31;
    }else if(getMonth() == 3){
        return adjustedDayOfYear - 59;
    }else if(getMonth() == 4){
        return adjustedDayOfYear - 90;
    }else if(getMonth() == 5){
        return adjustedDayOfYear - 120;
    }else if(getMonth() == 6){
        return adjustedDayOfYear - 151;
    }else if(getMonth() == 7){
        return adjustedDayOfYear - 181;
    }else if(getMonth() == 8){
        return adjustedDayOfYear - 212;
    }else if(getMonth() == 9){
        return adjustedDayOfYear - 243;
    }else if(getMonth() == 10){
        return adjustedDayOfYear - 273;
    }else if(getMonth() == 11){
        return adjustedDayOfYear - 304;
    }
        
    return adjustedDayOfYear - 334;

}

//Returns the day of the week as a string
String NTPClock::getDayOfWeek(){

    //We know that the first day of 2019 was a Tuesday, so calculate based on that. 
    uint16_t daysSinceStartOf2019 = (getUNIXTime() - 1546300800) / 86400;

    if(daysSinceStartOf2019 % 7 == 0){
        return "Tue";
    }else if(daysSinceStartOf2019 % 7 == 1){
        return "Wed";
    }else if(daysSinceStartOf2019 % 7 == 2){
        return "Thu";
    }else if(daysSinceStartOf2019 % 7 == 3){
        return "Fri";
    }else if(daysSinceStartOf2019 % 7 == 4){
        return "Sat";
    }else if(daysSinceStartOf2019 % 7 == 5){
        return "Sun";
    }
        
    return "Mon";
}

//Returns the month as a string
String NTPClock::getStringMonth(){
    if(getMonth() == 1){
        return "Jan";
    }else if(getMonth() == 2){
        return "Feb";
    }else if(getMonth() == 3){
        return "Mar";
    }else if(getMonth() == 4){
        return "Apr";
    }else if(getMonth() == 5){
        return "May";
    }else if(getMonth() == 6){
        return "Jun";
    }else if(getMonth() == 7){
        return "Jul";
    }else if(getMonth() == 8){
        return "Aug";
    }else if(getMonth() == 9){
        return "Sep";
    }else if(getMonth() == 10){
        return "Oct";
    }else if(getMonth() == 11){
        return "Nov";
    }
        
    return "Dec";
}

//Returns a date formatted as: "DAY MON DD, YEAR"
String NTPClock::getLongDate(){
    return getDayOfWeek() + " " + getStringMonth() + " " + String(getDayOfMonth()) + ", " + String(getYear());
}

//Returns a date formatted as: "DAY MON DD, YEAR"
String NTPClock::getShortDate(){
    return String(getYear()) + "/" + String(getMonth()) + "/" + String(getDayOfMonth());
}

//Returns a time formatted as: "HH:MMam/pm"
String NTPClock::getTime(){
    return String(getHour()) + ":" + getMinute() + getAMPM();
}