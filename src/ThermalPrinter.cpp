#include "ThermalPrinter.h"


ThermalPrinter::ThermalPrinter(Stream *s, uint8_t DTR){
    stream = s;
    dtrPin = DTR;
}

void ThermalPrinter::begin(voidFuncPtr PrintCallback){
    delay(500);
    wake();
    writeBytes(ASCII_ESC, '@'); // Init command

    // ESC 7 n1 n2 n3 Setting Control Parameter Command
    // n1 = "max heating dots" 0-255 -- max number of thermal print head
    //      elements that will fire simultaneously.  Units = 8 dots (minus 1).
    //      Printer default is 7 (64 dots, or 1/6 of 384-dot width), this code
    //      sets it to 11 (96 dots, or 1/4 of width).
    // n2 = "heating time" 3-255 -- duration that heating dots are fired.
    //      Units = 10 us.  Printer default is 80 (800 us), this code sets it
    //      to value passed (default 120, or 1.2 ms -- a little longer than
    //      the default because we've increased the max heating dots).
    // n3 = "heating interval" 0-255 -- recovery time between groups of
    //      heating dots on line; possibly a function of power supply.
    //      Units = 10 us.  Printer default is 2 (20 us), this code sets it
    //      to 40 (throttled back due to 2A supply).
    // More heating dots = more peak current, but faster printing speed.
    // More heating time = darker print, but slower printing speed and
    // possibly paper 'stiction'.  More heating interval = clearer print,
    // but slower printing speed.

    writeBytes(ASCII_ESC, '7');   // Esc 7 (print settings)
    writeBytes(10, 150, 60); // Heating dots, heat time, heat interval

    // Print density description from manual:
    // DC2 # n Set printing density
    // D4..D0 of n is used to set the printing density.  Density is
    // 50% + 5% * n(D4-D0) printing density.
    // D7..D5 of n is used to set the printing break time.  Break time
    // is n(D7-D5)*250us.
    // (Unsure of the default value for either -- not documented)

    #define printDensity   10 // 100% (? can go higher, text is darker but fuzzy)
    #define printBreakTime  2 // 500 uS

    writeBytes(ASCII_DC2, '#', (printBreakTime << 5) | printDensity);

    pinMode(dtrPin, INPUT_PULLUP);
    writeBytes(ASCII_GS, 'a', (1 << 5));

    _PrintCallback = PrintCallback;
}


void ThermalPrinter::wake() {
  writeBytes(255);
  delay(50);
  writeBytes(ASCII_ESC, '8', 0, 0); // Sleep off
  delay(50);
}

void ThermalPrinter::suppress(bool sup){
    suppressed = sup;
}

void ThermalPrinter::sleep() {
  writeBytes(ASCII_ESC, '8', 5, 5 >> 8);
}

void ThermalPrinter::offline(){
  wake();
  writeBytes(ASCII_ESC, '=', 0);
}

void ThermalPrinter::wait(){
  while(digitalRead(dtrPin) == HIGH){
    yield();
  }

}

void ThermalPrinter::writeBytes(uint8_t a) {
    wait();
    stream->write(a);
}

void ThermalPrinter::writeBytes(uint8_t a, uint8_t b) {
  wait();
  stream->write(a);
  wait();
  stream->write(b);
}

void ThermalPrinter::writeBytes(uint8_t a, uint8_t b, uint8_t c) {
  wait();
  stream->write(a);
  wait();
  stream->write(b);
  wait();
  stream->write(c);

}

void ThermalPrinter::writeBytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  wait();
  stream->write(a);
  wait();
  stream->write(b);
  wait();
  stream->write(c);
  wait();
  stream->write(d);
}

size_t ThermalPrinter::write(uint8_t c) {
  
  if(c != 0x13) { // Strip carriage returns
    wait();
    stream->write(c);
  }

  
  return 1;
}

void ThermalPrinter::output(String text){ 
    
    if(!suppressed){
      println(text);
    }
    _PrintCallback(text);
}

void ThermalPrinter::feed(uint8_t x){
    if(!suppressed){
      writeBytes(ASCII_ESC, 'd', x);
    }
    
    for(int i = 0; i < x; i++){ //Print timestamps to the console
      _PrintCallback(" ");
    }
}

//Wrap text to a specified amount of characters
String ThermalPrinter::wrap(String input, uint8_t wrapLength){

  //No need to wrap text if it's less than the wrapLength
  if(input.length() < wrapLength){
    return input;
  }

  String output = ""; //This holds the output
  uint8_t charThisLine = 1; //The character we're at on this line
  uint16_t lastSpace = 0; //The last space we processed

  //Run through this code once for each character in the input
  for(uint16_t i=0; i<input.length(); i++){
    
    
    if(charThisLine == 33){ //Once we reach the 33rd character in a line
      if(input.charAt(i) == ' '){ //If the character is a space, change it to a new line
        output.setCharAt(i, '\n');
        charThisLine = 1;
      }else if(input.charAt(i) == '\n'){ //If the character is a new line, do nothing
        charThisLine = 1;
      }else if(lastSpace <= i - wrapLength){ //If the last space was on the previous line, we're dealing with a really long word. Add a new line. 
        output.setCharAt(i, '\n');
        charThisLine = 1;
      }else{ //If it's any other situation, we change the last space to a new line and record which character we're at on the next line
        output.setCharAt(lastSpace, '\n');
        charThisLine = i - lastSpace;
      }
    }

    if(input.charAt(i) == ' '){ //If this character is a space, record it
      lastSpace = i;
    }

    if(input.charAt(i) == '\n'){
      charThisLine = 1;
    }

    output += input.charAt(i); //Append the current character to the output string
    charThisLine++; //Record that we're on the next character on this line
  }

  return output;
}

void ThermalPrinter::center(bool on){
    if(on){
        writeBytes(ASCII_ESC, 'a', 1); 
    }else{
        writeBytes(ASCII_ESC, 'a', 0); 
    }  
}

void ThermalPrinter::inverse(bool on){
    if(on){
        writeBytes(ASCII_GS, 'B', 1); 
    }else{
        writeBytes(ASCII_GS, 'B', 0); 
    }  

    writePrintMode();

}

void ThermalPrinter::doubleHeight(bool on){
    if(on){
        printMode |= (1<<4);
    }else{
        printMode &= ~(1<<4);
    }  

    writePrintMode();
}

void ThermalPrinter::doubleWidth(bool on){
    if(on){
        printMode |= (1<<5);
    }else{
        printMode &= ~(1<<5);
    }  

    writePrintMode();
}

void ThermalPrinter::bold(bool on){
    if(on){
        printMode |= (1<<3);
    }else{
        printMode &= ~(1<<3);
    }  

    writePrintMode();
}

void ThermalPrinter::writePrintMode() {
  writeBytes(ASCII_ESC, '!', printMode);
}

void ThermalPrinter::printStatus(String text, uint8_t feedAmt){
    wake();
    center(false);
    inverse(false);
    doubleHeight(false);
    doubleWidth(false);
    bold(true);

    output(wrap(text, 32));
    feed(feedAmt);
    sleep();

}

//Print a centered, large, bold, inverse title
void ThermalPrinter::printTitle(String text, uint8_t feedAmt){
    wake();
    center(true);
    inverse(true);
    doubleHeight(true);
    doubleWidth(true);
    bold(true);

  //If the text is shorter than 14 chars, add a space to either side and print. Otherwise, just print it. 
  if(text.length() <= 14){
    output(" " + text + " ");
  }else{
    output(wrap(text, 16));
  }

  feed(feedAmt);
  sleep();
}

//Print a centered, bold, medium heading
void ThermalPrinter::printHeading(String text, uint8_t feedAmt){
  wake();
  center(true);
  inverse(false);
  doubleHeight(true);
  doubleWidth(false);
  bold(true);

  output(wrap(text, 32));
  feed(feedAmt);
  sleep();

}

void ThermalPrinter::printMessage(String text, uint8_t feedAmt){
  wake();
  center(false);
  inverse(false);
  doubleHeight(true);
  doubleWidth(false);
  bold(true);

  output(wrap(text, 32));
  feed(feedAmt);
  sleep();
}

void ThermalPrinter::printError(String text, uint8_t feedAmt){
  wake();
  center(false);
  inverse(true);
  doubleHeight(false);
  doubleWidth(false);
  bold(true);

  suppressed = false;
  output(wrap("ERROR: " + text, 32));
  feed(feedAmt);
  sleep();

}

//Print a dotted line
void ThermalPrinter::printLine(uint8_t feedAmt){
  if(!suppressed){
    wake();
    writeBytes(ASCII_DC2, '*', 4, 48);

    for(int i = 0;i<48;i++){
      writeBytes(255, 255, 255, 255);
    }

    feed(feedAmt);
    sleep();
  }
  
  _PrintCallback("------------------------");

}

void ThermalPrinter::printBitmap(uint8_t feedAmt){
  wake();

  writeBytes(18, 42, 1, 24);

  for(int i = 0; i < 24; i++){
    writeBytes(170);
  }

  feed(feedAmt);
  
  sleep();

}
