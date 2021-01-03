# TAG (Text And Graphics) Machine
![Animated GIF of TAG Machine](https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/Animated.gif?raw=true)

A table-top device that can receive text (SMS) and picture (MMS) messages at its own phone number. Built around an ESP8266 (NodeMCU 1.0 or equivalent) microcontroller. Written in C++ using the Arduino framework. Under active development as of Jan 2021. 

## Features

- Prints ASCII text received via SMS
- Prints dithered black and white images received via MMS
- Prints text description of emojis (this model of printer does not support text inline with images)
- Detects links and generates QR Code automatically
- Automatically responds to new phone numbers to request a name for the address book
- Web interface for changing settings available on local network (mobile-friendly too!)
- Connects to chosen Wi-Fi Network(s) automatically
- Generates its own Wi-Fi hotspot for initial setup
- Uses a remote server to relay messages and convert images nearly instantly

## Getting Started (Software)

Download the data, lib and src folders to your computer and compile them using the [Arduino IDE](https://www.arduino.cc/en/Main/Software) or a compatible IDE such as [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO](https://platformio.org/) Extension. Configure your IDE for the ESP8266 board with at least 2MB of flash memory and upload the code to your board.  

Upload the files in the server folder to a server that you control. Edit the config.json with your own settings, and then run TAG_Bridge.js using node. Install an MQTT Broker such as [Mosquitto](https://mosquitto.org/) on the same server and run it. 

Create a Twilio account and get a phone number. Point Twilio to your server webhook. 

## Getting Started (Hardware)

* [NodeMCU 1.0 or equivalent](https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20200607165641&SearchText=nodemcu) (ESP8266 module with 4mb of flash memory).
* [AdaFruit Mini Thermal Printer](https://www.adafruit.com/product/597), [Sparkfun Thermal Printer](https://www.sparkfun.com/products/14970), or [equivalent](https://www.aliexpress.com/item/4000670706301.html?spm=a2g0o.productlist.0.0.6f64c49bIuD0cf&algo_pvid=21e71930-b292-4d6e-b394-f049cdb62eff&algo_expid=21e71930-b292-4d6e-b394-f049cdb62eff-9&btsid=0ab50f6115915778964662640e4af4&ws_ab_test=searchweb0_0,searchweb201602_,searchweb201603_).
* LED & current-limiting resistor
* Momentary push button
* 9v 3A Power Supply

![TAG Machine Schematic](https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/Schematic.png?raw=true)

## Authors

**Silviu Toderita**

## License

This project is licensed under the MIT License.
