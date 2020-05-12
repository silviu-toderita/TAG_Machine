/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    A persistent storage library for ESP8266. Allows storage of key:value pairs of
    Strings. 

    To use, initialize an object with the filename you'd like to use. Use put() to
    store or change a key:value pair, and get() to retrieve a value based on the
    key.
    
    Keys or values may not contain line breaks or the character ";", and they are
    removed automatically when using put().
    
    For human readability, the file is stored as a .txt and // can be used to 
    comment within the value only. get() will not return anything after // or any 
    trailing spaces.

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Persistent_Storage.h"

/*  Persistent_Storage Constructor
        filename_in: The filename to use 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
Persistent_Storage::Persistent_Storage(String filename_in){
    filename = "/" + filename_in + ".txt";
    SPIFFS.begin();
}

/*  (private) remove_reserved_characters
        input: Text to process
    RETURNS a string with processed text (all : and newlines removed)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String remove_character(String input, char old_character, char new_character){

    String output = input;

    //Go through each character of the string
    for(uint8_t i=0;i < output.length();i++){

        //If this character is a ":", change it to a ";"
        if(output.charAt(i) == old_character){
            output.setCharAt(i,new_character);
        }

    }

    return output;
}

/*  (private) remove_spaces_and_comments
        input: Text to process
    RETURNS a string with processed text (everything after the first // and spaces after end of text removed)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String remove_spaces_and_comments(String input){

    //If the input text contains "//"", comments_removed contains everything up to that 
    String comments_removed;
    //If // cannot be found, no comments need to be removed
    if(input.indexOf("//") == -1 || input.substring(0, 5) == "http:"){
        comments_removed = input;
    //Otherwise, remove anything before the //
    }else{
        comments_removed = input.substring(0, input.indexOf("//")); 
    }

    //Go through every character and store the last character that wasn't a space
    int16_t last_character = -1;
    for(uint16_t i = 0; i < comments_removed.length(); i++){
        if(comments_removed.charAt(i) != ' ' && comments_removed.charAt(i) != '\t') last_character = i;
    }

    //If the string has no characters or all spaces, return ""
    if(last_character == -1){
        return "";
    //Return the characters without the spaces on the end
    }else{
        return comments_removed.substring(0,last_character + 1);
    }
    
}

/*  put: Add a new key:value pair to storage, or modify the value of an existing key
        key:
        value:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool Persistent_Storage::put(String key, String value){ 
    //If the key is blank, it can't be stored
    if(key == "") return false;

    //Process the key and the value by removing reserved characters
    String processed_key = remove_character(remove_character(key, ';', ':'), '\n', ' ');
    String processed_value = remove_character(value, '\n', ' ');

    //If the file_exists flag is false, check if the file exists
    if(!file_exists){

        //Change the file_exists flag to true
        file_exists = true;

        //If the file does not exist, create it and write the key:value pair
        if(!SPIFFS.exists(filename)){
            File file = SPIFFS.open(filename, "w");

            file.print(processed_key + ";" + processed_value + "\n");
            file.close();

            return true;
        } 

    }

    //Open the current file for reading and a new file for writing
    File current_file = SPIFFS.open(filename, "r");
    File new_file = SPIFFS.open(filename + "n", "w"); 


    bool found_key = false;
    //Loop until the current file has been read completely
    while(current_file.position() < current_file.size() - 1){
        //Read the key on this line of the current file
        String current_key = current_file.readStringUntil(';');
        //Write the current key to the new file
        new_file.print(current_key + ";");

        //If this key is the one to be modified...
        if(current_key == processed_key){
            //Write the new value to the new file
            new_file.print(processed_value + "\n");
            //Seek in the current file to the end of the line
            current_file.readStringUntil('\n');
            //Change the found_key flag to true
            found_key = true;
        //If this key is not the one to be modified
        }else{
            //Copy the value from the current file to the new file
            new_file.print(current_file.readStringUntil('\n') + "\n");
        }
        
    }

    //Close the current file and delete it
    current_file.close();
    SPIFFS.remove(filename);
    
    //If the key was not found in the file, add the new key:value pair to the end of the new file
    if(!found_key){
        new_file.print(processed_key + ";" + processed_value + "\n");
    }

    //Close the new file and rename it to the current name
    new_file.close(); 
    SPIFFS.rename(filename + "n",filename);

    return true;
}

/*  get_value: Get the value of a specific key 
        key:
    RETURNS Value of the key. If the key is not found, returns "".
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String Persistent_Storage::get_value(String key){
    //If the file does not exist, the key does not exist
    if(!SPIFFS.exists(filename)) return "";

    //Remove any reserved characters from the key
    String processed_key = remove_character(remove_character(key, ';', ':'), '\n', ' ');
    String value = "";

    //Open the file for reading
    File file = SPIFFS.open(filename, "r");
    
    //Read through each line of the file
    while(file.position() < file.size() - 1){
        //Look for the key. If found...
        if(file.readStringUntil(';') == processed_key){
            //Save the value and exit the loop
            value = file.readStringUntil('\n');
            break;
        }
        //If not found, seek to the end of the current line. 
        file.readStringUntil('\n');
    }

    //Close the file and return the value
    file.close();
    return remove_spaces_and_comments(value);

}

/*  get_sub_value: Get the subvalue of a specific key 
        key:
        id: Which subvalue to return
    RETURNS Subvalue of the key. If the key is not found, returns "".
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String Persistent_Storage::get_sub_value(String key, uint8_t id){
    //If the file does not exist, the key does not exist
    if(!SPIFFS.exists(filename)) return "";

    //Remove any reserved characters from the key
    String processed_key = remove_character(remove_character(key, ';', ':'), '\n', ' ');
    String value = "";

    //Open the file for reading
    File file = SPIFFS.open(filename, "r");
    
    //Read through each line of the file
    while(file.position() < file.size() - 1){
        //Look for the key. If found...
        if(file.readStringUntil(';') == processed_key){
            //Read through the sub-values to get to the correct id
            for(int i = 0; i < id; i++){
                file.readStringUntil(';');
            }
            //Save the sub-value from its starting point to the end of the line
            value = file.readStringUntil('\n');
            //If there is another sub-value after it, remove it
            if(value.indexOf(';') != -1){
                value = value.substring(0, value.indexOf(";"));
            }
            
            break;
        }
        //If not found, seek to the end of the current line. 
        file.readStringUntil('\n');
    }

    //Close the file and return the value
    file.close();
    return remove_spaces_and_comments(value);

}

/*  get_key: Get a specific key
        id: the index of the key
    RETURNS The key or "" if the file or key is not found
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String Persistent_Storage::get_key(uint16_t id){
    //If there is no file, return nothing
    if(!SPIFFS.exists(filename)) return "";

    File file = SPIFFS.open(filename, "r");
    
    //Skip lines until the id is reached
    for(int i = 0; i < id; i++){
        file.readStringUntil('\n');
    }
    //The key is the first part of the line, until the colon
    String key = file.readStringUntil(';');
    file.close();
    return key;
}

/*  remove: Removes a key:value pair based on the key
        key:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Persistent_Storage::remove(String key){
    //If the file does not exist, the key does not exist
    if(!SPIFFS.exists(filename)) return;

    //Remove any reserved characters from the key
    String processed_key = remove_character(remove_character(key, ';', ':'), '\n', ' ');

    //Open the current file for reading and a new file for writing
    File current_file = SPIFFS.open(filename, "r");
    File new_file = SPIFFS.open(filename + "n", "w"); 

    //Loop until the current file has been read completely
    while(current_file.position() < current_file.size() - 1){
        //Read the key on this line of the current file
        String current_key = current_file.readStringUntil(';');
        
        //If this key is the one to be deleted...
        if(current_key == processed_key){
            //Seek in the current file to the end of the line
            current_file.readStringUntil('\n');
        //If this key is not the one to be deleted...
        }else{
            //Copy the key and value from the current file to the new file
            new_file.print(current_key + ";" + current_file.readStringUntil('\n') + "\n");
        }
        
    }

    //Close the current file and delete it
    current_file.close();
    SPIFFS.remove(filename);

    //Close the new file and rename it to the current name
    new_file.close(); 
    SPIFFS.rename(filename + "n",filename);

}

/*  get_number_entries: Get the number of key:value pairs
    RETURNS number of key:value pairs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint16_t Persistent_Storage::get_number_entries(){
    //If the file does not exist, return 0
    if(!SPIFFS.exists(filename)) return 0;

    File file = SPIFFS.open(filename, "r");

    uint16_t count = 0;
    //Loop until each line has been counted
    while(file.position() < file.size() - 1){
        file.readStringUntil('\n');
        count++;
    }

    file.close();
    return count;
}

/*  get_number_values: Get the number of values for a specific key
    RETURNS the number of values for a specific key
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint8_t Persistent_Storage::get_number_values(String key){
    //If the file does not exist, the key does not exist
    if(!SPIFFS.exists(filename)) return 0;

    //Remove any reserved characters from the key
    String processed_key = remove_character(remove_character(key, ';', ':'), '\n', ' ');

    //Open the file for reading
    File file = SPIFFS.open(filename, "r");
    
    uint8_t number_values = 0;
    //Read through each line of the file
    while(file.position() < file.size() - 1){
        //Look for the key. If found...
        if(file.readStringUntil(';') == processed_key){
            //Store the values
            String value = file.readStringUntil('\n');
            number_values++;
            //Count the amount of sub-values
            for(int i = 0;i < value.length();i++){
                if(value.charAt(i) == ';') number_values++;
            }
            break;
        }
        //If not found, seek to the end of the current line. 
        file.readStringUntil('\n');
    }

    //Close the file and return the value
    file.close();
    return number_values;

}