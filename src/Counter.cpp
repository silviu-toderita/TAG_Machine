//The counter object can be used as a simple way to check if a specific amount of time has passed
#include "Counter.h"

uint32_t counterLength; //Holds the length of the counter
uint32_t start; //Time at which the counter started

Counter::Counter(uint32_t length){ //Create a counter object with length in milliseconds
  counterLength = length; 
  start = millis(); 
}

bool Counter::status(){ //Check on the counter status, return true if counter has finished, false if it hasn't
    if(millis() >= start + counterLength) return true;
    return false;
}