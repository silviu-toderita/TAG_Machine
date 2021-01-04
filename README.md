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


## Build Your Own TAG Machine

There is a [full setup guide](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md) available. Some electrical and embedded programming experience is required, though it should be doable for a beginner. While a server app is required to relay messages from Twilio to a TAG Machine, I have made my own server available for free to use (or you can set up your own easily). There is a [full parts list](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md#part-2---hardware) available in the setup guide.


## Dependencies

	* [knolleary/PubSubClient](https://github.com/knolleary/pubsubclient) 2.8
	* [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) 6.17.2
	* [links2004/WebSockets](https://github.com/Links2004/arduinoWebSockets) 2.3.2

## Using the TAG Machine

There is a [full manual available](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md) on how to use the TAG Machine. 


## Authors

**Silviu Toderita**


## License

This project is licensed under the MIT License.
