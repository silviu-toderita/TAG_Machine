# TAG Machine Manual

## Initial Setup

Follow the [setup guide](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md). Once finished [Part 6](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md#part-6---tag-machine-settings), you should be on the settings page of the Tag Machine web interface. Read the [settings section](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md#settings) for more information on how to complete setup. 

## General Operation

The TAG (Text and Graphics) Machine is a table-top device that prints messages received on thermal receipt paper. The TAG Machine has its own phone number, and can accept these kinds of messages:
- SMS (Text Messages)
  - Emojis are automatically converted to text descriptions
  - URLs are automatically converted to QR codes
- MMS (Picture Messages)
  - Supports common image formats such as JPG, GIF and PNG
  - Images are automatically dithered and scaled down to a maximum width of 384 pixels
  
The TAG Machine connects to your local Wi-Fi network to access the internet, and establishes a connection with a relay server (by default, Silviu's server, though [you can set up your own](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md#option-2-set-up-your-own-server-advanced)). This server receives messages from [Twilio](https://www.twilio.com/), which is the phone provider we will be using. The TAG Machine can optionally reply to senders by communicating directly with Twilio, in order to request names for the contacts list. 

## Modes of Operation

### Setup Mode
This mode is indicated by a slowly flashing LED. This mode indicates that there is an error or missing field in the settings file and the TAG Machine cannot start. This is the expected mode when the TAG Machine is turned on for the first time. A hotspot is enabled:
- Wi-Fi Name: tagmachine
- Wi-Fi Password: 12345678

The [Web Interface](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md#web-interface) is enabled to allow initial setup. Go to http://tagmachine.local from a browser. Navigate to the [settings page](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md#settings) to enter initial settings or correct errors. 

### Recovery Mode
This mode is indicated by a rapidly flashing LED. To enter this mode, hold the button while plugging in the TAG Machine. This mode only allows for over-the-air firmware updates with the password "123456778". A hotspot is enabled:
- Wi-Fi Name: tagmachine
- Wi-Fi Password: 12345678

### Message Mode
The default mode, indicated by a printout of the TAG MACHINE logo, the wi-fi network it has connected to, the web interface URL, the date and time, and the current phone number. 

<img src="https://raw.githubusercontent.com/silviu-toderita/TAG_Machine/master/docs/tag_printout.jpg" width="250">

In this mode, the LED is off. You can receive messages at the phone number, and they should print automaticalls. If the LED is off but nothing prints or an error message prints, see the Error Messages section.

### Hotspot Mode
If the TAG Machine starts up in Message Mode and a known Wi-Fi network is not found, it will give you the option of entering hotspot mode by pressing a button. If you choose to enter this mode, the hotspot network name and password you have chosen will be printed out again as a reminder, and a hotspot will be created. You can then connect to the hotspot and navigate to the URL of the web interface to update Wi-Fi Network details or any other settings. Press the button again at any time to return to Message Mode.

## Using the TAG Machine

When the TAG Machine is in Message mode you can send SMS and MMS messages to it. 

### SMS Messages (Text Messages)
ASCII text is supported. Emojis that have an available text description will print that text instead. Emojis or special characters that are not supported by the printer and do not have an available text description will print a gray square instead. Text messages can be a maximum of 1600 characters long. Links will have a QR generated automatically (up to 10 links per message). 

### MMS Messages (Picture Messages)
Common image formats such as JPG, PNG, and GIF are supported. Videos are not supported for obvious reasons. Up to 10 images can be included in one message. MMS messages can also contain text. 

### Contacts
If "Send SMS Replies" is enabled in the [settings](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md#settings), the first time a phone number sends a message to the TAG Machine, the following reply will be sent:
> "Thanks for messaging User's Fax Machine! Reply with your name within 24hrs to add it to the contact list."
If the sender sends another non-empty message within 24 hours, their name will be added to the contact list. If 24 hours pass and they then send another message, the initial reply will be sent again requesting a name for the contacts list.

If a user would like to change their name, they can send this exact message: "_name" (without the quotes). The TAG Machine will reply requesting a new name, and if the sender sends another non-empty message within 24 hours, their name will be updated in the contact list. 

To import or export a contact list, go to the "Contacts" section of the web interface. 

### Photo Mode
If you send a picture message to the TAG Machine with the text "_photo" (without the quotes), the picture(s) will print without the heading or metadata, turning the TAG Machine into a rudimentary photo printer. 

### Web Interface
<img src="https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/web_interface.png?raw=true" width="400">

You can select any address (ending in .local) for the web interface in the [settings page](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md#settings). The default is http://tagmachine.local. This address will be printed at start-up. You can access the Web Interface in Message Mode (you must be connected to the same Wi-Fi Network), or from Setup Mode or Hotspot Mode (you must be connected to the TAG Machine's Hotspot). Use any browser to access the web interface. The web interface has the following pages:
- **Contacts:** Import or Export a contacts list here. Useful before uploading a new file system to the TAG Machine, to ensure you don't lose your existing contacts.
- **Settings:** Change the TAG Machine's settings on this page, or import/export a settings file. See the [Settings](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md#settings) section for more detail. 
- **Console:** A console that shows you in real-time what's happening under the hood of the TAG Machine. Will display everything printed to the printer, plus more. Useful for development as a serial monitor cannot be used at the same time as the thermal printer. 


## Error Messages

- *Unable to Find Known WiFi Networks! Check your WiFi settings and access point. Searching for Networks... <-- Press Button to Start Hotspot*: The TAG Machine was unable to find the network(s) in the settings file. The TAG Machine requires a Wi-Fi connection to access the internet and will not be able to function. Press the button if you would like to start a local hotspot to connect to the TAG Machine directly and update the Wi-Fi Settings. The TAG Machine will continue to search until a known Wi-Fi network is found.

- *Unable to Connect To Network! Check WiFi Password. Attempting to Connect... <-- Press Button to Start Hotspot*: A known Wi-Fi Network was found, however the password was incorrect. The TAG Machine requires a Wi-Fi connection to access the internet and will not be able to function. Press the button if you would like to start a local hotspot to connect to the TAG Machine directly and update the Wi-Fi Settings. The TAG Machine will attempt to reconnect to this or any other known network.

- *Lost WiFi Connection! Searching for Networks... <-- Press Button to Start Hotspot*: The TAG Machine was connected, but lost the connection. The TAG Machine requires a Wi-Fi connection to access the internet and will not be able to function. Press the button if you would like to start a local hotspot to connect to the TAG Machine directly and update the Wi-Fi Settings. The TAG Machine will attempt to reconnect to this or any other known network.

- *Unable to connect to message server! Check your internet connection. Attempting to connect...*: The TAG Machine is connected to Wi-Fi, however it could not reach the message server. This could be due to a problem with your internet connection, or with the server. If the Bridge URL was left as the default in the settings, contact Silviu to make sure his server hasn't stopped working. If you are using your own server, check the configuration. 

- If nothing is printing at all, access the [Web Interface](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md#web-interface) and go to the Console. This should give you some idea of where the error lies. 

## Settings

### General Settings

**Phone Number (Required):** Enter the phone number you bought on Twilio, starting with the country code. Numbers only, do not include any special characters such as +, -, (, ), or .

**Owner Name:** Enter your name as you would like it to appear in SMS replies. 

**Twilio Account SID:** If you would like the TAG Machine to send SMS replies, enter your Twilio Account SID. You can find this on your [Twilio Console](https://www.twilio.com/console/).

**Twilio Auth Token:** If you would like the TAG Machine to send SMS replies, enter your Twilio Auth Token. You can find this on your [Twilio Console](https://www.twilio.com/console/).

**Bridge URL:** Enter the URL of the bridge server you will use to relay messages to your TAG Machine. If you don't intend to set up your own server, you can leave it as the default.

**Send SMS Replies:** Turn this on to enable the TAG Machine to send SMS replies in order to request a name from the sender for the address book. This also allows senders to change their name, and to receive certain error messages. This is not recommended with a Twilio trial account, as each phone number will have to be independently verified. Will default to Off if Twilio Account SID or Twilio Auth Token are missing. 

**Print Images:** Turn this on to enable image printing. Turn off if image printing is too slow, or your printer doesn't support it. 

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


## Factory Reset

To reset to factory settings and delete all user data, hold the button for at least 5 seconds while plugging in TAG Machine. TAG Machine will restart in Setup Mode. 


## Firmware Updates

Firmware updates can be done by following the same instructions as [initial setup](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md#part-3---firmware). However, uploading the data (file system) again is not required unless there have been changes made to the "data" folder. If you do have to upload the data again, be sure to export your contacts and settings from the [Web Interface](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md#web-interface) before doing so, and import them again after the upload. 

### OTA (Over-The-Air) Updates

Over-the-air updates are supported. If using PlatformIO, go to the platformio.ini file in the root folder of the repo and comment out the last 3 lines. If you have changed the default OTA Password in the settings, you must also update it here after "auth=" (without the quotes). If using a different dev environment, the OTA username is "tagmachine" and OTA password is set up in the TAG Machine Settings (default: 12345678).

Please note that if performing an OTA update from Setup or Recovery mode, the OTA password will always be 12345678. 

You must be connected to the same network as the TAG Machine in order to perform an OTA update, whether that's a common Wi-Fi network in Message mode or the TAG Machine Hotspot in Hotspot, Setup, or Recovery mode. 
