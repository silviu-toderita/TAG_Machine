//Library for storing String variables in a file on ESP8266 Flash

#include "Storage.h"
#include "fs.h" //SPI Flash File System (SPIFFS) Library

String filename;
uint8_t maxLength; //The maximum character length of any key or value


Storage::Storage(String filenameIn, uint8_t maxLengthIn){
    filename = filenameIn;
    maxLength = maxLengthIn;

    //If the file doesn't already exist, create it
    if(!SPIFFS.exists("/" + filename + ".sto")){
        File file = SPIFFS.open("/" + filename + ".sto","w");
        file.close();
    }

}

//Remove spaces from the end of a string
String removeSpaces(String input){
    uint8_t length = 1;

    //Go through each character from the input String starting with the first
    for(uint8_t i=0;i < input.length();i++){

        //If the current character is not a space...
        if(input.charAt(i) != char(' ')){
            //Record how long the String is so far
            length = i + 1;
        }
    }

    //Return a substring of the input, starting from the beginning and ending with the last non-space character
    return input.substring(0,length);
}

//Add spaces to the end of a string to reach a prescribed length
String addSpaces(String input, uint8_t length){

    //If the string is longer than the prescribed length, shorten it and return the result
    if(input.length() > length) return input.substring(0,length);

    //Calculate the amount of spaces needed to bring the String up to the prescribed length
    uint8_t spaces = length - input.length();

    String output = input;

    //Add the amount of spaces necessary to the string
    for(int i=0; i < spaces;i++){
        output += " ";
    }

    return output;
}

String removeSpecialChars(String input){

    String output = input;

    for(uint8_t i=0;i < output.length();i++){

        if(output.charAt(i) == char(':')){
            output.setCharAt(i,';');
        }

        if(output.charAt(i) == char('\n')){
            output.setCharAt(i,' ');
        }

    }

    return output;
}

//Add or replace a value to the file. Return true if successful, false if not
bool Storage::put(String key, String value){

    //If either the key or value are longer than the max length, return false
    if(key.length() > maxLength) return false;
    if(value.length() > maxLength) return false;

    String processedKey = removeSpecialChars(key);
    String processedValue = removeSpecialChars(value);

    //Get the existing value of the key, if any
    String prevValue = get(processedKey);

    //If the new value is the same as the old value, return true
    if(prevValue == processedValue) return true;

    //Open the file for reading and writing
    File file = SPIFFS.open("/" + filename + ".sto", "r+");

    //If the key didn't already exist in the file...
    if(prevValue == ""){
        
        file.seek(file.size());//Go to the end of the file

        file.print(addSpaces(processedKey, maxLength)); //Write the key 
        file.print(":"); //Write a colon
        file.print(addSpaces(processedValue, maxLength)); //Write the value
        file.print("\n"); //Write a line break

    //If the key already existed...
    }else{

        //While we haven't reached the end of the file...
        while(file.position() < file.size() - 1){
            //When we find the key, break out of the loop
            if(removeSpaces(file.readStringUntil(':')) == processedKey) break;
            //If we didn't find the key this time, go to the end of the line
            file.readStringUntil('\n');
        }

        //Write the value
        file.print(addSpaces(processedValue, maxLength));

    }

    //Close the file and return true
    file.close();
    return true;

}

//Read a value from the file based on the key
String Storage::get(String key){

    String processedKey = removeSpecialChars(key);

    //Open the file for reading
    File file = SPIFFS.open("/" + filename + ".sto", "r");

    //If the file is smaller than the length of one key and one value, return an empty string
    if(file.size() < maxLength * 2) return "";
    
    //While we haven't reached the end of the file...
    while(file.position() < file.size() - 1){
        //Look for the key. If found...
        if(removeSpaces(file.readStringUntil(':')) == processedKey){
            //Close the file and return the value
            String value = removeSpaces(file.readStringUntil('\n'));
            file.close();
            return value;
        }
        //If not found, go to the end of the current line. 
        file.readStringUntil('\n');
    }

    //Close the file and return an empty string if the key couldn't be found
    file.close();
    return "";

}