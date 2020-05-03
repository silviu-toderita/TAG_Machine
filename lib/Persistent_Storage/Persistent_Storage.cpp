/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    A persistent storage library for ESP8266. Allows storage of key:value pairs of
    Strings. 

    To use, initialize an object with the filename you'd like to use. Use put() to
    store or change a key:value pair, and get() to retrieve a value based on the
    key. Use remove() to delete a key:value pair. 
    
    Keys or values may not contain line breaks or the character ":", and they are
    removed automatically when using put().
    
    For human readability, the file is stored as a .txt and // can be used to 
    comment within the value only. get() will not return anything after // or any 
    spaces after the value.

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Persistent_Storage.h"
#include "FS.h" //SPI Flash File System (SPIFFS) Library

String filename; //The filename for this object
bool file_exists = false; //True if the file exists, false if it does not

/*  Persistent_Storage Constructor
        filename_in: The filename to use 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
Persistent_Storage::Persistent_Storage(String filename_in){
    filename = filename_in;
}

/*  (private) remove_reserved_characters
        input: Text to process
    RETURNS a string with processed text (all : and newlines removed)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String remove_reserved_characters(String input){

    String output = input;

    //Go through each character of the string
    for(uint8_t i=0;i < output.length();i++){

        //If this character is a ":", change it to a ";"
        if(output.charAt(i) == char(':')){
            output.setCharAt(i,';');
        }

        //If this character is a newline, change it to a space
        if(output.charAt(i) == char('\n')){
            output.setCharAt(i,' ');
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
    if(input.indexOf("//") == -1){
        comments_removed = input;
    }else{
        comments_removed = input.substring(0, input.indexOf("//")); 
    }

    //Go through every character and store the last character that wasn't a space
    uint16_t last_character = 0;
    for(uint16_t i = 0; i < comments_removed.length(); i++){
        if(comments_removed.charAt(i) != ' ' && comments_removed.charAt(i) != '\t') last_character = i;
    }

    //If the string has no characters or all spaces, return ""
    if(last_character == 0){
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
void Persistent_Storage::put(String key, String value){
    //If the key is blank, it can't be stored
    if(key == "") return;

    //Process the key and the value by removing reserved characters
    String processed_key = remove_reserved_characters(key);
    String processed_value = remove_reserved_characters(value);

    //If the file_exists flag is false, check if the file exists
    if(!file_exists){

        //Change the file_exists flag to true
        file_exists = true;

        //If the file does not exist, create it and write the key:value pair
        if(!SPIFFS.exists("/" + filename + ".txt")){
            File file = SPIFFS.open("/" + filename + ".txt", "w");

            file.print(processed_key + ":" + processed_value + "\n");
            file.close();

            return;
        } 

    }

    //Open the current file for reading and a new file for writing
    File current_file = SPIFFS.open("/" + filename + ".txt", "r");
    File new_file = SPIFFS.open("/" + filename + "NEW.txt", "w"); 


    bool found_key = false;
    //Loop until the current file has been read completely
    while(current_file.position() < current_file.size() - 1){
        //Read the key on this line of the current file
        String current_key = current_file.readStringUntil(':');
        //Write the current key to the new file
        new_file.print(current_key + ":");

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
    SPIFFS.remove("/" + filename + ".txt");
    
    //If the key was not found in the file, add the new key:value pair to the end of the new file
    if(!found_key){
        new_file.print(processed_key + ":" + processed_value + "\n");
    }

    //Close the new file and rename it to the current name
    new_file.close(); 
    SPIFFS.rename("/" + filename + "NEW.txt","/" + filename + ".txt");
}

/*  get: Get the value of a specific key 
        key:
    RETURNS Value of the key. If the key is not found, returns "%NF".
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String Persistent_Storage::get(String key){
    //If the file does not exist, the key does not exist
    if(!SPIFFS.exists("/" + filename + ".txt")) return "";

    //Remove any reserved characters from the key
    String processed_key = remove_reserved_characters(key);
    String value = "";

    //Open the file for reading
    File file = SPIFFS.open("/" + filename + ".txt", "r");
    
    //Read through each line of the file
    while(file.position() < file.size() - 1){
        //Look for the key. If found...
        if(file.readStringUntil(':') == processed_key){
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

/*  delete: Removes a key:value pair based on the key
        key:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Persistent_Storage::remove(String key){
    //If the file does not exist, the key does not exist
    if(!SPIFFS.exists("/" + filename + ".txt")) return;

    //Remove any reserved characters from the key
    String processed_key = remove_reserved_characters(key);

    //Open the current file for reading and a new file for writing
    File current_file = SPIFFS.open("/" + filename + ".txt", "r");
    File new_file = SPIFFS.open("/" + filename + "NEW.txt", "w"); 

    //Loop until the current file has been read completely
    while(current_file.position() < current_file.size() - 1){
        //Read the key on this line of the current file
        String current_key = current_file.readStringUntil(':');
        
        //If this key is the one to be deleted...
        if(current_key == processed_key){
            //Seek in the current file to the end of the line
            current_file.readStringUntil('\n');
        //If this key is not the one to be deleted...
        }else{
            //Copy the key and value from the current file to the new file
            new_file.print(current_key + ":" + current_file.readStringUntil('\n') + "\n");
        }
        
    }

    //Close the current file and delete it
    current_file.close();
    SPIFFS.remove("/" + filename + ".txt");

    //Close the new file and rename it to the current name
    new_file.close(); 
    SPIFFS.rename("/" + filename + "NEW.txt","/" + filename + ".txt");

}

bool Persistent_Storage::exists(){
    return SPIFFS.exists("/" + filename + ".txt");
}