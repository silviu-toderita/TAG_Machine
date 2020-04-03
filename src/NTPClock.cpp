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

uint16_t intervalNTP = 600; //Frequency of NTP requests in seconds
uint64_t lastNTPRequest = 0; //Time of last NTP request
uint64_t lastNTPResponse; //Time of last NTP response

const uint32_t seventyYears = 2208988800UL; //Seventy Years offset
int8_t zoneOffset = 0; //Offset from UTC in hours
uint32_t lastUNIXTime = 0; //UNIX Time at last NTP response

//Initialize object, requires server and hour offset from UTC
NTPClock::NTPClock(char* server, int8_t offset){ 
    NTPServer = server;
    zoneOffset = offset;
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
    return lastUNIXTime + ((millis() - lastNTPResponse)/1000); 
}

uint32_t NTPClock::getUNIXTimeExt(){
    return getUNIXTime();
} 

//Handle function should be run as frequently as possible to update time via UDP. Returns true if we have a time, false if not. 
bool NTPClock::handle(){ 
    uint32_t interval;

    if(lastUNIXTime == 0){ //If we've not received any time yet, the interval is 2s. Otherwise the interval is normal. 
        interval = 2000;
    }else{
        interval = intervalNTP * 1000;
    }

    //If we don't have an IP address for the timeserver, get one
    if(!timeServerIP) WiFi.hostByName(NTPServer, timeServerIP);
    
    if(millis() - lastNTPRequest > interval){ //If it's been more than the interval since the last NTP Request, send another.
        lastNTPRequest = millis();
        sendNTPpacket(timeServerIP);
    }

    if(UDP.parsePacket() != 0){ //If there is any data in the UDP buffer, process it
        UDP.read(NTPBuffer, NTP_PACKET_SIZE); //Read the buffer
        uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43]; //Extract NTP time from the bytes
        lastUNIXTime = NTPTime - seventyYears + zoneOffset*3600; //Convert NTP time to UNIX Time, and take into account time zone
        lastNTPResponse = millis(); //Update the time of last NTP response
    }

    if(!lastUNIXTime) return false;
    return true;
}

//Begin the clock, specify timeout. Returns true if it could get an update from the time server within timeout, false if not.
bool NTPClock::begin(uint32_t timeout){ 
    
    //Begin UDP connection
    UDP.begin(123);


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

//Returns the year as an integer
uint16_t intGetYear(){
    //((Current time - Time Jan 1, 2019 - leap day differential) / seconds in a year) + 2019
    return ((getUNIXTime() - 1546300800 - (getLeapYears() * 86400)) / 31536000) + 2019;
}

//Returns the year as a string
String NTPClock::getYear(){
    return String(intGetYear());
}

//Returns the day of the year
uint16_t getDayOfYear(){
    uint32_t timeAtStartOfYear = 1546300800 + ((intGetYear() - 2019) * 31536000 + (getLeapYears() * 86400));
    return (getUNIXTime() - timeAtStartOfYear) / 86400 + 1;
}

//Returns the Month as a String. If addZero is true, a zero will be added to single-digit months.
String NTPClock::getMonth(bool addZero){
    uint16_t adjustedDayOfYear = getDayOfYear();

    //If it's a leap year, adjust the day of the year by -1 for all months except January, as each month will end one day later. 
    if(intGetYear() % 4 == 0){
        adjustedDayOfYear = adjustedDayOfYear - 1;
    }

    uint8_t month;
    if(getDayOfYear() <= 31){
        month = 1;
    }else if(adjustedDayOfYear <= 59){
        month = 2;
    }else if(adjustedDayOfYear <= 90){
        month = 3;
    }else if(adjustedDayOfYear <= 120){
        month = 4;
    }else if(adjustedDayOfYear <= 151){
        month = 5;
    }else if(adjustedDayOfYear <= 181){
        month = 6;
    }else if(adjustedDayOfYear <= 212){
        month = 7;
    }else if(adjustedDayOfYear <= 243){
        month = 8;
    }else if(adjustedDayOfYear <= 273){
        month = 9;
    }else if(adjustedDayOfYear <= 304){
        month = 10;
    }else if(adjustedDayOfYear <= 334){
        month = 11;
    }else{
        month = 12;
    }

    //If we should add a zero to the start of the output, add it if the month is a single digit
    if(addZero && month <= 9) return "0" + String(month);

    //Otherwise, return the month as-is
    return String(month);
   
}

//Returns the text month as a string. If shortMonth is true, return a 3-character month only. 
String NTPClock::getTextMonth(bool shortMonth){
    if(getMonth(false) == "1"){
        if(!shortMonth) return "January";
        return "Jan";
    }else if(getMonth(false) == "2"){
        if(!shortMonth) return "February";
        return "Feb";
    }else if(getMonth(false) == "3"){
        if(!shortMonth) return "March";
        return "Mar";
    }else if(getMonth(false) == "4"){
        if(!shortMonth) return "April";
        return "Apr";
    }else if(getMonth(false) == "5"){
        return "May";
    }else if(getMonth(false) == "6"){
        if(!shortMonth) return "June";
        return "Jun";
    }else if(getMonth(false) == "7"){
        if(!shortMonth) return "July";
        return "Jul";
    }else if(getMonth(false) == "8"){
        if(!shortMonth) return "August";
        return "Aug";
    }else if(getMonth(false) == "9"){
        if(!shortMonth) return "September";
        return "Sep";
    }else if(getMonth(false) == "10"){
        if(!shortMonth) return "October";
        return "Oct";
    }else if(getMonth(false) == "11"){
        if(!shortMonth) return "November";
        return "Nov";
    }
    if(!shortMonth) return "January";
    return "December";
}

//Returns the day of the month as a string. If addZero is true, a zero will be added to single-digit days. 
String NTPClock::getDayOfMonth(bool addZero){
    uint16_t adjustedDayOfYear = getDayOfYear();

    //If it's a leap year, adjust the day of the year by -1 for all months except January and February, as each month will start one day later. 
    if(intGetYear() % 4 == 0){
        adjustedDayOfYear = adjustedDayOfYear - 1;
    }
    
    uint8_t dayOfMonth;
    if(getMonth(false) == "1"){
        dayOfMonth = getDayOfYear();
    }else if(getMonth(false) == "2"){
        dayOfMonth = getDayOfYear() - 31;
    }else if(getMonth(false) == "3"){
        dayOfMonth = adjustedDayOfYear - 59;
    }else if(getMonth(false) == "4"){
        dayOfMonth = adjustedDayOfYear - 90;
    }else if(getMonth(false) == "5"){
        dayOfMonth = adjustedDayOfYear - 120;
    }else if(getMonth(false) == "6"){
        dayOfMonth = adjustedDayOfYear - 151;
    }else if(getMonth(false) == "7"){
        dayOfMonth = adjustedDayOfYear - 181;
    }else if(getMonth(false) == "8"){
        dayOfMonth = adjustedDayOfYear - 212;
    }else if(getMonth(false) == "9"){
        dayOfMonth = adjustedDayOfYear - 243;
    }else if(getMonth(false) == "10"){
        dayOfMonth = adjustedDayOfYear - 273;
    }else if(getMonth(false) == "11"){
        dayOfMonth = adjustedDayOfYear - 304;
    }else{
        dayOfMonth = adjustedDayOfYear - 334;
    }

    //If we should add a zero to the start of the output, add it if the day is a single-digit
    if(addZero && dayOfMonth <=9 ) return "0" + String(dayOfMonth);

    //Otherwise, return the day as-is
    return String(dayOfMonth); 
        
}

//Returns the day of the week as a string. If shortDay is true, return a 3-character day only.
String NTPClock::getDayOfWeek(bool shortDay){

    //We know that the first day of 2019 was a Tuesday, so calculate based on that. 
    uint16_t daysSinceStartOf2019 = (getUNIXTime() - 1546300800) / 86400;

    if(daysSinceStartOf2019 % 7 == 0){
        if(!shortDay) return "Tuesday";
        return "Tue";
    }else if(daysSinceStartOf2019 % 7 == 1){
        if(!shortDay) return "Wednesday";
        return "Wed";
    }else if(daysSinceStartOf2019 % 7 == 2){
        if(!shortDay) return "Thursday";
        return "Thu";
    }else if(daysSinceStartOf2019 % 7 == 3){
        if(!shortDay) return "Friday";
        return "Fri";
    }else if(daysSinceStartOf2019 % 7 == 4){
        if(!shortDay) return "Saturday";
        return "Sat";
    }else if(daysSinceStartOf2019 % 7 == 5){
        if(!shortDay) return "Sunday";
        return "Sun";
    }
    if(!shortDay) return "Monday";
    return "Mon";
}

//Returns the hour as a string. If addZero is true, a zero will be added to single-digit hours. If format24Hour is true, hour will be returned in 24 hour time. 
String NTPClock::getHour(bool addZero, bool format24Hour){ 
    uint8_t hour = getUNIXTime() / 3600 % 24;

    uint8_t output;

    if(format24Hour){ //If 24-hour format, don't modify the hour
        output = hour;
    }else if(hour == 0){ //If not 24-hour format and the hour is zero, change it to 12
        output = 12;
    }else if(hour <= 12){ //If it's less than or equal 12, keep it as-is
        output = hour;
    }else{  //If it's between 13 and 23, subtract 12
        output = hour - 12;
    }

    //If we should add a zero to the start of the output, add it if the hour is a single-digit
    if(addZero && output <= 9) return "0" + String(output);
    //Otherwise, return the hour as-is
    return String(output);
    
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

//Returns the minute as a String. If addZero is true, a zero will be added to single-digit minutes.
String NTPClock::getMinute(bool addZero){
    uint8_t minute = getUNIXTime() / 60 % 60;

    //If we should add a zero to the start of the output, add it if the minute is a single digit
    if(minute <= 9 && addZero) return "0" + String(minute);

    //Otherwise, return the minute as-is
    return String(minute);
}

//Returns the second as a String. If addZero is true, a zero will be added to single-digit seconds.
String NTPClock::getSecond(bool addZero){
    uint8_t second = getUNIXTime() % 60;

    //If we should add a zero to the start of the output, add it if the second is a single digit
    if(addZero && second <= 9) return "0" + String(second);

    //Otherwise, return the second as-is
    return String(second);
}

//Returns the date/time formatted as: "DAY MON DD, YEAR - HH:MMam"
String NTPClock::getDateTime(){
    //If we don't have a timestamp, return a blank timestamp
    if(!handle()) return "### ### ##, #### - ##:####";

    return getDayOfWeek(true) + " " + getTextMonth(true) + " " + getDayOfMonth(false) + ", " + getYear() + " - " + getHour(false, false) + ":" + getMinute(true) + getAMPM();
}

//Returns the date/time formatted as: "YEAR/MM/DD-HH:MM:SS"
String NTPClock::getTimestamp(){

    //If we don't have a time, return a blank timestamp
    if(!handle()) return "####/##/##-##:##:##";
    
    return String(getYear()) + "/" + getMonth(true) + "/" + getDayOfMonth(true) + "-" + getHour(true, true) + ":" + getMinute(true) + ":" + getSecond(true);
}

//Convert the incoming date/time from Twilio into a long timestamp
String NTPClock::convertDateTime(String input){

  //Parse the dateTime input
  String dayString = input.substring(0, 3);
  String day = input.substring(5, 7);
  String month = input.substring(8, 11);
  String year = input.substring(12, 16);
  uint8_t hour = input.substring(17, 19).toInt();
  uint8_t minute = input.substring(20, 22).toInt();

  //Adjust the timezone
  if(hour + zoneOffset < 0){
    hour = hour + zoneOffset + 24;
  }else if(hour + zoneOffset >= 24){
    hour = hour + zoneOffset - 24;
  }else{
    hour = hour + zoneOffset;
  }

  //Is the hour am or pm?
  String amPM;
  if(hour < 12){
    amPM = "am";
  }else{
    amPM = "pm";
  }

  //Change from 24 hour to 12 hour time
  if(hour == 0){ //If the hour is zero, change it to 12
        hour = 12;
  }else if(hour > 12){ //If it's greater than 12, subtract 12
        hour = hour - 12;
  }

  //If the minute is a single digit, add a zero to the beginning
  String minuteString;
  if(minute <= 9){
    minuteString = "0" + String(minute);
  }else{
    minuteString = String(minute);
  }

  //Output a long timestamp
  return dayString + " " + month + " " + day + ", " + year + " - " + String(hour) + ":" + minuteString + amPM;
}