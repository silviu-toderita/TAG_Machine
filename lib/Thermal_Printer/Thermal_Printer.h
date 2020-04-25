#include "Arduino.h"
#include "FS.h"
#include "ESP8266HTTPClient.h"

#define ASCII_TAB '\t' // Horizontal tab
#define ASCII_LF  '\n' // Line feed
#define ASCII_FF  '\f' // Form feed
#define ASCII_CR  '\r' // Carriage return
#define ASCII_DC2  18  // Device control 2
#define ASCII_ESC  27  // Escape
#define ASCII_FS   28  // Field separator
#define ASCII_GS   29  // Group separator

typedef void (*voidFuncPtr)(const String input);

class Thermal_Printer{
    public:

    Thermal_Printer(uint32_t baudIn, uint8_t DTRin, bool port2In);
    Thermal_Printer(uint32_t baudIn, uint8_t DTRin);

    void    
        begin(voidFuncPtr),
        suppress(bool),
        offline(),
        
        printStatus(String, uint8_t),
        printTitle(String, uint8_t),
        printHeading(String, uint8_t),
        printMessage(String, uint8_t),
        printError(String, uint8_t),
        printLine(uint8_t, uint8_t),
        printBitmap(const char*, uint8_t),
        feed(uint8_t);

    private:

    uint8_t 
        DTR,
        printMode = 0;
    uint32_t
        baud;
    bool
        suppressed = false,
        port2;
    void
        wake(),
        sleep(),
        writeBytes(uint8_t),
        writeBytes(uint8_t, uint8_t),
        writeBytes(uint8_t, uint8_t, uint8_t),
        writeBytes(uint8_t, uint8_t, uint8_t, uint8_t),
        center(bool),
        inverse(bool),
        doubleHeight(bool),
        doubleWidth(bool),
        bold(bool),
        writePrintMode(),
        output(String),
        wait();

    String  
        wrap(String, uint8_t);
        

};