# TAG (Text And Graphics) Machine
![Animated GIF of TAG Machine](https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/Animated.gif?raw=true)

A table-top device that can receive text (SMS) and picture (MMS) messages at its own phone number. Built around an ESP8266 (NodeMCU 1.0 or equivalent) microcontroller. Written in C++ using the Arduino framework. Under active development as of Jan 2021. 

## Features

- Prints ASCII text received via SMS
- Prints dithered black and white images received via MMS
- Prints text description of received emojis
- Detects links and generates QR Codes for them automatically
- Automatically responds to new phone numbers to request a name for the address book
- Web interface for changing settings available on local network (mobile-friendly too!)
- Connects to chosen Wi-Fi Network(s) automatically
- Generates its own Wi-Fi hotspot for initial setup
- Uses a remote server to relay messages and convert images nearly instantly

## Setup Part 1 - Firmware

*NOTE: Before uploading the firmware to the NodeMCU, ensure it's plugged into your computer via USB, and that it's not connected to the power supply or the printer.*

### Install Drivers

Have a look at [this guide](https://cityos-air.readme.io/docs/1-usb-drivers-for-nodemcu-v10) on how to identify which serial chip you have. You may also find this information on the product listing on the site you purchased the NodeMCU board from. Drivers for the CP2102 chip can be found [here](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers), and drivers for the CH341 chip can be found [here](https://github.com/nodemcu/nodemcu-devkit/tree/master/Drivers).

If you have issues with getting your computer to recognize your NodeMCU, make sure you are using the cable supplied with your board, or a Micro-USB cable that is data-capable. Most Micro-USB cables that are included with consumer devices for the purposes of charging are not real USB cables, and only have power connections (no data).

### Option 1: PlatformIO (Easy)

1. Download and install [Github Desktop](https://desktop.github.com/) or your favourite Git client

2. Clone this repository locally on your computer

3. Install [Visual Studio Code](https://code.visualstudio.com/), and then install the [PlatformIO Extension](https://platformio.org/)

4. Open the local folder with the Tag Machine repository in Visual Studio Code. 

![Build & Upload PlatformIO](https://docs.platformio.org/en/latest/_images/platformio-ide-vscode-build-project.png)

5. Press the Build button in the blue bar at the bottom of the Visual Studio Code window. You should see a green "SUCCESS" message at the bottom of the Terminal pane.

6. Press the Upload button in the blue bar at the bottom of the window. You should see the upload progress, and a green "SUCCESS" message at the bottom of the Terminal pane. This has uploaded the firmware. 

7. In the Terminal pane, type "pio run -t uploadfs" without the quotes. You should see the upload progress, and a green "SUCCESS" message at the bottom of the Terminal pane. This has uploaded the data.

### Option 2: Your Preferred Development Environment (Advanced)

1. Clone the repository or download the data, lib and src folders to your computer. 

2. Configure your preferred development environment for the NodeMCU or an ESP8266 with 4MB of flash memory. 

3. Configure at least 1MB of flash memory for file storage using SPIFFS. 

4. Install the following dependencies:
	* knolleary/PubSubClient@^2.8
	* bblanchon/ArduinoJson@^6.17.2
	* links2004/WebSockets@^2.3.2
  
5. Compile and upload to the board using USB. 

6. Upload the contents of the data folder to the SPIFFS using USB. 


## Setup Part 2 - Hardware

### Shopping List
* [NodeMCU 1.0 or equivalent](https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20200607165641&SearchText=nodemcu) (ESP8266 module with 4mb of flash memory)
* [AdaFruit Mini Thermal Printer](https://www.adafruit.com/product/597), [Sparkfun Thermal Printer](https://www.sparkfun.com/products/14970), or [equivalent](https://www.aliexpress.com/item/4000670706301.html?spm=a2g0o.productlist.0.0.6f64c49bIuD0cf&algo_pvid=21e71930-b292-4d6e-b394-f049cdb62eff&algo_expid=21e71930-b292-4d6e-b394-f049cdb62eff-9&btsid=0ab50f6115915778964662640e4af4&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_)
* LED & current-limiting resistor
  * *or combined LED button*
* Momentary push button
  * *or combined LED button*
* Power Supply 5-8v, 1A+ (8v 3A recommended) 
  * *Note that while both the NodeMCU and Thermal Printer support input voltages up to 9v, some models of printers are extremely sensitive to voltages slightly above 9.0v. Power Supplies with outputs of 9v may drift above 9v at times, causing the printer to freeze.*
* DC Jack
* Capacitor 220uF, 15v+
* Case or Box (Simple DIY Case Design coming soon!)

### Connections Schematic:

![TAG Machine Schematic](https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/Schematic.png?raw=true)

### Connections Notes:
- Thermal Printer wire colours can vary according to model, be sure to double-check label on the printer itself
- Capacitor values do not have to be exact, but the voltage should be higher than your power supply
- Resistor value will depend on the LED used. 470-1k Ohm should work for most LEDs
- Be sure to leave the USB port on the NodeMCU exposed for initial programming
- A combination LED button can be used and looks cleaner


## Setup Part 3 - Server

Upload the files in the server folder to a server that you control. Edit the config.json with your own settings, and then run TAG_Bridge.js using node. Install an MQTT Broker such as [Mosquitto](https://mosquitto.org/) on the same server and run it. 

Create a Twilio account and get a phone number. Point Twilio to your server webhook. 


## Authors

**Silviu Toderita**

## License

This project is licensed under the MIT License.
