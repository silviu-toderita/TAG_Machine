//The counter object can be used as a simple way to check if a specific amount of time has passed
#include "Arduino.h"

class Counter{
  public:

  Counter(uint32_t);
  bool status();
};