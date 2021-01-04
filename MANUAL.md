# TAG Machine Manual

## Initial Setup

Follow the [setup guide](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md). Once finished [Part 5](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md#part-5---tag-machine-settings), you should be on the settings page of the Tag Machine web interface. 

## Settings

### General Settings

**Phone Number (Required):** Enter the phone number you bought on Twilio, starting with the country code. Numbers only, do not include any special characters such as +, -, (, ), or .

**Owner Name:** Enter your name as you would like it to appear in SMS replies. 

**Twilio Account SID:** If you would like the TAG Machine to send SMS replies, enter your Twilio Account SID. You can find this on your [Twilio Console](https://www.twilio.com/console/).

**Twilio Auth Token:** If you would like the TAG Machine to send SMS replies, enter your Twilio Auth Token. You can find this on your [Twilio Console](https://www.twilio.com/console/).

**Bridge URL:** Enter the URL of the bridge server you will use to relay messages to your TAG Machine. If you don't intend to set up your own server, you can leave it as the default.

**Send SMS Replies:** Turn this on to enable the TAG Machine to send SMS replies in order to request a name from the sender for the address book. This also allows senders to change their name, and to receive certain error messages. This is not recommended with a Twilio trial account, as each phone number will have to be independently verified. Will default to Off if Twilio Account SID or Twilio Auth Token are missing. 

**Print Photos:** Turn this on to enable image printing. Turn off if image printing is too slow, or your printer doesn't support it. 

### Advanced Settings

**Printer Baud Rate:** Select your thermal printer's baud rate. This can be found on a label on the printer, or by holding the printer's button while powering on. Most printers are 9600, while some are 19200. If you were able to [configure your printer for a higher baud rate](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md#optional---configure-printer), be sure to also change it here. 

**Local URL:** The URL for the printer's web interface. 

**Password:** Password for over-the-air firmware updates.

**Printer Heating Dots (Default: 11):** Maximum number of heating dots to use simultaneously (0-47). Higher = faster print speed, but higher current draw and possible crashing.

**Printer Heating Time (Default: 120):** Amount of time to heat each line in 10us increments (3-255). Higher = darker print, but slower print speed and possible sticking paper. 

**Printer Heating Interval (Default: 60):** Amount of time between heating each line in 10us increments (0-255). Higher = Clearer print, but slower print speed.

**Printer DTR Pin:** The ESP8266 pin used for the printer DTR (Data Terminal Ready) signal.

**Button Pin:** The ESP8266 pin used for the button (ground).

**LED Pin:** The ESP8266 pin used for the LED (anode).

### Wi-Fi Settings

**Hotspot Name:** The network name for the wi-fi hotspot created by the TAG Machine. 

**Hotspot Password:** The network password for the wi-fi hotspot created by the TAG Machine.

**WiFi Network Name 1-3 (1 Required):** The network password for a wi-fi network that the TAG Machine should attempt to connect to.

**Wifi Network Password 1-3:** The corresponding network password for a wi-fi network that the TAG Machine should attempt to connect to.


