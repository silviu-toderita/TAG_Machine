# TAG (Text And Graphics) Machine
<p align="center">
<img src="https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/tag_gif.gif?raw=true">
</p>

A table-top device that can receive text (SMS) and picture (MMS) messages at its own phone number. Built around an ESP8266 microcontroller. Written in C++ using the Arduino framework, using Twilio for SMS communication. Development has finished as of Jan 2021, however please submit any bugs using the Issues page. 


## Features

- Prints text and images!
- Emojis are automatically converted to text descriptions
- Links are automatically converted to QR codes
- Contacts list requests name from the sender directly
- Mobile-friendly web interface to easily update settings
- Offline message caching


## Build Your Own TAG Machine

There is a [full setup guide](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md) available. Some electrical and embedded programming experience is required, though it should be doable for a beginner. While a server app is required to relay messages from Twilio to a TAG Machine, I have made my own server available for free to use (or you can set up your own easily). There is a [full parts list](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md#shopping-list) available in the setup guide.


## Dependencies

* [knolleary/PubSubClient](https://github.com/knolleary/pubsubclient) 2.8
* [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) 6.17.2
* [links2004/WebSockets](https://github.com/Links2004/arduinoWebSockets) 2.3.2

## Using the TAG Machine

There is a [full manual available](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md) on how to use the TAG Machine. 


## License

This project is licensed under the MIT License.
