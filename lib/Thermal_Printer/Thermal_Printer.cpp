/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ESP8266 library for thermal printers such as the Adafruit Mini Thermal Receipt
    Printer or the Sparkfun Thermal Printer. Supports printing text with different
    fonts, printing bitmaps from a custom file type obtained via HTTP or stored 
    locally as a file.

    Hardware requirements:
		-RX pin of printer connected to GPIO2
		-DTR_pin pin of printer connected to any GPIO pin
		-Known printer baud_rate rate (higher is better for printing bitmaps)

    To use, initialize a Thermal_Printer object. Print using print_... functions.

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#include "Thermal_Printer.h"

static voidFuncPtrStr _print_callback; //Callback function when printing

/*  Thermal_Printer constructor (with defaults)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
Thermal_Printer::Thermal_Printer(){
    config(9600, 13);
}

/*  config
        baud_rate_in: Baud rate of printer, usually 9600 or 19200 but can go as high 
        as 115200 (default: 9600)
        DTR_pin_in: ESP8266 pin that DTR pin of printer is connected to (default: 13)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::config(uint32_t baud_rate_in, uint8_t DTR_pin_in){
    baud_rate = baud_rate_in;
    DTR_pin = DTR_pin_in;
}

/*	begin: Starts the printer, should be called during setup before printing.
      	print_callback: Callback function to be called anytime printing happens, 
        that should accept a single String arg.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::begin(voidFuncPtrStr print_callback){
	//Begin the serial connection to the printer after a 150ms delay to allow for OTA update serial garbage to finish
	delay(150);
	Serial.begin(baud_rate);
	Serial.set_tx(2); //Set the TX port to GPIO2
	
	//Wake the printer
	delay(350); 
	wake();
	write_bytes(ASCII_ESC, '@'); // Initialize printer

	//Set default printing parameters
	set_printing_parameters(11, 120, 60);
    write_bytes(ASCII_DC2, '#', (2 << 5) | 10);

	//Set DTR pin and enable printer flow control
	pinMode(DTR_pin, INPUT_PULLUP);
	write_bytes(ASCII_GS, 'a', (1 << 5));

	//Set the print callback
	_print_callback = print_callback;
}

/*	begin: Starts the printer, should be called during setup before printing. No 
      	printer callback defined. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::begin(){
  	begin(NULL);
}

/*	set_printing_parameters
		heating_dots: (0-47) Maximum number of heating dots to fire simultaneously 
			from 8-384. Default is 11. Higher = faster print speed, higher current 
			draw.
		heating_time: (3-255) Amount of time to heat each line (x10uS). Default is 
			120. Higher = darker print, slower print, possibility of sticking paper.
		heating_interval: (0-255) Amount of time between heating each line (x10uS). 
			Default is 60. Higher = Clearer print, slower print speed, less current 
			draw.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::set_printing_parameters(uint8_t heating_dots, uint8_t heating_time, uint8_t heating_interval){
	write_bytes(ASCII_ESC, '7');
	write_bytes(heating_dots, heating_time, heating_interval);
}

/*	offline: Turns off the printer, to be called before a function that might spit
      	out serial garbage like the start of an OTA update.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::offline(){
	wake();
	write_bytes(ASCII_ESC, '=', 0);
}

/*	suppress: Suppresses all printing, but still calls callback function when
		printing. 
		suppressed_in: Set suppressed status
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::suppress(bool suppressed_in){
    suppressed = suppressed_in;
}

/*	feed: Advance the paper roll.
      	feed_amount: Amount of lines to advance the paper roll by. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::feed(uint8_t feed_amount){
    if(!suppressed){
      	write_bytes(ASCII_ESC, 'd', feed_amount);
    }
    
    //Print blank lines to the console
    for(int i = 0; i < feed_amount; i++){ 
      	_print_callback(" ");
    }
}

/*	print_status: print small text
		text: text to print
		feed_amount: Amount to feed after text
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::print_status(String text, uint8_t feed_amount){
    wake();
    font_center(false);
    font_inverse(false);
    font_double_height(false);
    font_double_width(false);
    font_bold(true);

    output(wrap(text, 32));
    feed(feed_amount);
    sleep();

}

/*	print_title: Print a large, centered white title on a black background
		text: text to print
		feed_amount: Amount to feed after text
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::print_title(String text, uint8_t feed_amount){
    wake();
    font_center(true);
    font_inverse(true);
    font_double_height(true);
    font_double_width(true);
    font_bold(true);

	//If the text is shorter than 14 chars, add a space to either side and print. Otherwise, print it wrapped to 16 chars. 
	if(text.length() <= 14){
		output(" " + text + " ");
	}else{
		output(wrap(text, 16));
	}

	feed(feed_amount);
	sleep();
}

/*	print_heading: Print large, centered text
		text: text to print
		feed_amount: Amount to feed after text
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::print_heading(String text, uint8_t feed_amount){
	wake();
	font_center(true);
	font_inverse(false);
	font_double_height(true);
	font_double_width(false);
	font_bold(true);

	output(wrap(text, 32));
	feed(feed_amount);
	sleep();
}

/*	print_message: Print large text
		text: text to print
		feed_amount: Amount to feed after text
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::print_message(String text, uint8_t feed_amount){
	wake();
	font_center(false);
	font_inverse(false);
	font_double_height(true);
	font_double_width(false);
	font_bold(true);

	output(wrap(text, 32));
	feed(feed_amount);
	sleep();
}

/*	print_error: Print small white text on black background with the prefix 
		"ERROR: "
		text: text to print
		feed_amount: Amount to feed after text
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::print_error(String text, uint8_t feed_amount){
	wake();
	font_center(false);
	font_inverse(true);
	font_double_height(false);
	font_double_width(false);
	font_bold(false);

	suppressed = false;
	output(wrap("ERROR: " + text, 32));
	feed(feed_amount);
	sleep();
}

/*	print_line: Print a solid horizontal line
		thickness: Thickness in pixels
		feed_amount: Amount to feed after line
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::print_line(uint8_t thickness, uint8_t feed_amount){
	_print_callback("------------------------");

	if(!suppressed){
		wake();
		//Write full-width bitmap
		write_bytes(ASCII_DC2, '*', thickness, 48);

		//Write 48 bytes per line of black pixels
		for(int i = 0;i<(48*thickness);i++){
		write_bytes(255);
		}

		feed(feed_amount);
		sleep();
	}
}

/*	print_bitmap_file: Print a bitmap from a file
		file: The file to read from. First byte is height in pixels x 256, second 
			byte is height (ie. 384 pixel height will be 1, 128). Starting from third 
			byte, 1-bit bitmap with each byte being MSB. Width must be exactly 384 to
			match printer width.
		feed_amount: Amount to feed after image.
        description: Text describing the photo to be sent back to printer callback
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::print_bitmap_file(File file, uint8_t feed_amount, String description){
	
    //Print the description to the callback
	_print_callback(description);

    //Only print if the printing has not been suppressed
    if(!suppressed){
        uint16_t height = file.read() * 256; //First byte is height * 256
        height += file.read(); //Second byte is more height
        wake();
        //While there are still lines to print
        while(height != 0){
            //Print up to 255 lines per chunk
            uint8_t chunk_height = 255;
            //If there are less than 255 lines left, the chunk will be exactly the height remaining
            if(height < 255){
            chunk_height = height;
            }

            //Write full-width bitmap
            write_bytes(ASCII_DC2, '*', chunk_height, 48);

            //For each line, write the next 48 bytes
            for(int i = 0; i < (chunk_height * 48); i++){
            write_bytes(file.read());
            }

            //Height remaining = last height remaining - how much we printed this chunk
            height = height - chunk_height;
        }
        feed(feed_amount);
	    sleep();

    }else{
        //Print blank lines to the callback for each feed amount
        for(int i = 0; i < feed_amount; i++){
            _print_callback("");
        }

    }

}

void Thermal_Printer::print_bitmap_file_test(File file){    
    wake();
    
    write_bytes(ASCII_GS, '*', 1, 3);
    for(int i = 0; i < 24; i++){
        write_bytes(127);
    }

    output("Before");
    write_bytes(ASCII_GS, 'L', 72, 0);
    write_bytes(ASCII_GS, '/', 2);
    write_bytes(ASCII_GS, 'L', 0, 0);
    output("After");


    /*
    Serial.print("Before");

    write_bytes(ASCII_ESC, '*', 32, 12, 0);
    for(int i = 0; i < 36; i++){
        write_bytes(127);
    }

    output("After");*/

    sleep();
}

/*	print_bitmap_http: Print a bitmap from web
		URL: The URL of the file to read from. First byte is height in pixels x 256, 
			second byte is height (ie. 384 pixel height will be 1, 128). Starting from 
			third byte, 1-bit bitmap with each byte being MSB. Width must be exactly 
			384 to match printer width.
		feed_amount: Amount to feed after image.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::print_bitmap_http(String URL, uint8_t feed_amount){
	HTTPClient http; //Create a client object

	http.begin(URL); //Begin connection to address

	//Get HTTP code
	int HTTP_code = http.GET();
	//If there is any valid HTTP code...
	if (HTTP_code > 0) {

		//If the HTTP code says there is a file found...
		if (HTTP_code == HTTP_CODE_OK) {

            // Get TCP stream
            WiFiClient * stream = http.getStreamPtr();

            wake();

            uint16_t height;
            uint8_t byte_counter = 0;
            while(byte_counter < 2){
                //Wait for a byte to be available
                while(!stream->available()) yield();
                //The first byte is height * 256
                if(byte_counter == 0){
                height = stream->read() * 256;
                byte_counter = 1;
                //The second byte is additional height
                }else{
                height += stream->read();
                byte_counter = 2;
                }
                
            }

            //While there are still lines to print
            while(height != 0){
                //Print up to 255 lines per chunk
                uint8_t chunk_height = 255;
                //If there are less than 255 lines left, the chunk will be exactly the height remaining
                if(height < 255){
                chunk_height = height;
                }

                //Write full-width bitmap
                write_bytes(ASCII_DC2, '*', chunk_height, 48);

                //For each line, write the next 48 bytes
                for(int i = 0; i < (chunk_height * 48); i++){
                //Wait for a byte to be available
                while(!stream->available()) yield();
                //Write the byte
                write_bytes(stream->read());
                }
                //Height remaining = last height remaining - how much we printed this chunk
                height = height - chunk_height;
            }
            _print_callback("<IMAGE>");

        //If the HTTP status was anything other than 200 OK, print an error with the status
		}else{
            print_error("Image Download Failed with HTTP Status: " + String(HTTP_code), 0);
        }
    //If there was no valid HTTP status, print an error
	}else{
        print_message("Image Download Failed", 0);

    }

	feed(feed_amount);
	sleep();

	//Close the connection
	http.end();
  
}

/*	(private) wake: Wake up the printer before printing.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::wake() {
	write_bytes(ASCII_ESC, '8', 0, 0);
	delay(50); //If we don't wait a bit, the printer won't be ready to print
}

/*	(private) sleep: Put the printer to sleep after printing.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::sleep() {
  	write_bytes(ASCII_ESC, '8', 1, 1 >> 8);
}

/*	(private) wait: Check the printer buffer and wait while it's full.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::wait(){
  	while(digitalRead(DTR_pin) == HIGH) yield();
}

/*	(private) write_bytes: Write instructions as bytes to the printer.
      	a, b, c, d: Bytes to write.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::write_bytes(uint8_t a) {
    wait();
    Serial.write(a);
}
void Thermal_Printer::write_bytes(uint8_t a, uint8_t b) {
	wait();
	Serial.write(a);
	Serial.write(b);
}
void Thermal_Printer::write_bytes(uint8_t a, uint8_t b, uint8_t c) {
	wait();
	Serial.write(a);
	Serial.write(b);
    wait();
	Serial.write(c);
}
void Thermal_Printer::write_bytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
	wait();
	Serial.write(a);
	Serial.write(b);
    wait();
	Serial.write(c);
	Serial.write(d);
}
void Thermal_Printer::write_bytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e) {
	wait();
	Serial.write(a);
	Serial.write(b);
    wait();
	Serial.write(c);
	Serial.write(d);
    wait();
    Serial.write(e);
}
void Thermal_Printer::write_bytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) {
	wait();
	Serial.write(a);
	Serial.write(b);
    wait();
	Serial.write(c);
	Serial.write(d);
    wait();
    Serial.write(e);
	Serial.write(f);
}

/*	(private) output: Write text to the printer.
      	text: Text to write.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::output(String text){ 
    if(!suppressed){
		wait();
		Serial.println(text);
    }
    _print_callback(text);
}

/*	(private) wrap: Wrap text.
		input: String to wrap.
		wrap_length: Length to wrap string to.
    RETURNS wrapped string
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String Thermal_Printer::wrap(String input, uint8_t wrap_length){
	//No need to wrap text if it's less than the wrap_length
	if(input.length() < wrap_length){
		return input;
	}

	String output = ""; //This holds the output
	uint8_t char_current_line = 1; //The character currently being processed on this line
	uint16_t last_space = 0; //The last space processed

	//Run through this code once for each character in the input
	for(uint16_t i=0; i<input.length(); i++){
		
		//If the current character is the 1st in the next line...
		if(char_current_line == wrap_length + 1){
		if(input.charAt(i) == ' '){ //If the character is a space, change it to a new line
			output.setCharAt(i, '\n');
			char_current_line = 1;
		}else if(input.charAt(i) == '\n'){ //If the character is a new line, do nothing
			char_current_line = 1;
		}else if(last_space <= i - wrap_length){ //If the last space was on the second-last line, this is a really long word and should be split with a new line.
			output.setCharAt(i, '\n');
			char_current_line = 1;
		}else{ //If it's any other situation, change the last space to a new line and record the current character being processed on this line
			output.setCharAt(last_space, '\n');
			char_current_line = i - last_space;
		}
		}

		if(input.charAt(i) == ' '){ //If this character is a space, record it
		last_space = i;
		}

		//If this character is a new line, reset the char_current_line counter
		if(input.charAt(i) == '\n'){
		char_current_line = 1;
		}

		output += input.charAt(i); //Append the current character to the output string
		char_current_line++; //Increment the char_current_line coutner
	}

	return output;
}

/*	(private) font_center:
      on: Center the font on/off
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::font_center(bool on){
    if(on){
        write_bytes(ASCII_ESC, 'a', 1); 
    }else{
        write_bytes(ASCII_ESC, 'a', 0); 
    }  
}

/*	(private) font_inverse:
      	on: Print white on black on/off
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::font_inverse(bool on){
    if(on){
        write_bytes(ASCII_GS, 'B', 1); 
    }else{
        write_bytes(ASCII_GS, 'B', 0); 
    }  

    font_write_print_mode();

}

/*	(private) font_double_height:
      	on: Set double height font on/off
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::font_double_height(bool on){
    if(on){
        printMode |= (1<<4);
    }else{
        printMode &= ~(1<<4);
    }  

    font_write_print_mode();
}

/*	(private) font_double_width:
      	on: Set double width font on/off
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::font_double_width(bool on){
    if(on){
        printMode |= (1<<5);
    }else{
        printMode &= ~(1<<5);
    }  

    font_write_print_mode();
}

/*	(private) font_bold:
     	on: Set font bold on/off
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::font_bold(bool on){
    if(on){
        printMode |= (1<<3);
    }else{
        printMode &= ~(1<<3);
    }  

    font_write_print_mode();
}

/*	(private) font_write_print_mode: Sets the print mode for inverse, double
      	height, double width, and bold font. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Thermal_Printer::font_write_print_mode() {
  	write_bytes(ASCII_ESC, '!', printMode);
}