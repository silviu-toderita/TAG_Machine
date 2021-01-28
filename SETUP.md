# TAG Machine Setup


## Part 1 - Hardware

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
* Electrolytic Capacitor 220uF, 15v+
* 1N4001 Diode
* Thermal Receipt Paper - 58mm wide, max thickness 40mm
* [Case](https://github.com/silviu-toderita/TAG_Machine/blob/master/SETUP.md#part-2---case)

### Optional - Configure Printer

You can optionally configure the printer to increase the Baud Rate form the defauly 9600 or 19200 to as high as 115200. This will increase photo printing speed significantly, although it will make no difference for text. [This configuration utility](http://www.dkia.at/downloads/csn-a2-t-tool.zip) (Windows only) works with some models of Thermal Printers, but not with others (your mileage may vary). 

### Connections Schematic:

<img src="https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/Schematic.png?raw=true" width="600">

### Connections Notes:
- Thermal Printer wire colours can vary according to model, be sure to double-check label on the printer itself
- Capacitor values do not have to be exact, but the voltage should be higher than your power supply
- Resistor value will depend on the LED used. 470-1k Ohm should work for most LEDs


## Part 2 - Case

<img src="https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/tag_pic.JPG?raw=true" width="600">

The cardboard case design for the TAG Machine is available in the /case/ folder. It is designed to be laser-cut and laser-engraved from 1.3mm cardboard, and has been tested with [Ponoko](https://www.ponoko.com/), although other laser-cutting services will likely also work. The blue stroke is the cut path, and the red stroke is the engrave path. 

<img src="https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/case_design.png?raw=true" width="500">

To assemble the case, fold it at the short slots and insert the tabs into the long slots. The tabs will act as the feet of the case once it's put together. Please note that the design has been updated and may differ slightly from these photos. 

<img src="https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/case_2.JPG?raw=true" width="600">

There are several holes in the design for a button, an LED, and a DC jack. Feel free to modify these as needed for your own use. The large cutout for the thermal printer module should fit any model of printer listed in the [shopping list](case_2.JPG). In addition to laser-cutting, you can always attempt to cut out the design by hand: It's recommended you enlarge the tabs and use glue if you decide to go this route. 

<img src="https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/case_4.JPG?raw=true" width="600">


## Part 3 - Firmware

*NOTE: Before uploading the firmware to the NodeMCU, ensure it's plugged into your computer via USB, and that it's not connected to the power supply or the printer.*

### Install Drivers

Have a look at [this guide](https://cityos-air.readme.io/docs/1-usb-drivers-for-nodemcu-v10) on how to identify which serial chip you have. You may also find this information on the product listing on the site you purchased the NodeMCU board from. Drivers for the CP2102 chip can be found [here](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers), and drivers for the CH341 chip can be found [here](https://github.com/nodemcu/nodemcu-devkit/tree/master/Drivers).

If you have issues with getting your computer to recognize your NodeMCU, make sure you are using the cable supplied with your board, or a Micro-USB cable that is data-capable. Most Micro-USB cables that are included with consumer devices for the purposes of charging are not real USB cables, and only have power connections (no data).

### Option 1: PlatformIO (Easy)

1. Download and install [Github Desktop](https://desktop.github.com/) or your favourite Git client

2. Clone this repository locally on your computer

3. Install [Visual Studio Code](https://code.visualstudio.com/), and then install the [PlatformIO Extension](https://platformio.org/)

4. Open the local folder with the Tag Machine repository in Visual Studio Code. 

<img src="https://docs.platformio.org/en/latest/_images/platformio-ide-vscode-build-project.png" width="600">

5. Press the Build button in the blue bar at the bottom of the Visual Studio Code window. You should see a green "SUCCESS" message at the bottom of the Terminal pane.

6. Press the Upload button in the blue bar at the bottom of the window. You should see the upload progress, and a green "SUCCESS" message at the bottom of the Terminal pane. This has uploaded the firmware. 

7. In the Terminal pane, type "pio run -t uploadfs" without the quotes. You should see the upload progress, and a green "SUCCESS" message at the bottom of the Terminal pane. This has uploaded the data.

### Option 2: Your Preferred Development Environment (Advanced)

1. Clone the repository or download the data, lib and src folders to your computer. 

2. Configure your preferred development environment for the NodeMCU or an ESP8266 with 4MB of flash memory. 

3. Configure at least 1MB of flash memory for file storage using SPIFFS. 

4. Install the following dependencies:
	* [knolleary/PubSubClient](https://github.com/knolleary/pubsubclient) 2.8
	* [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) 6.17.2
	* [links2004/WebSockets](https://github.com/Links2004/arduinoWebSockets) 2.3.2
  
5. Compile and upload to the board using USB. 

6. Upload the contents of the data folder to the SPIFFS using USB. 


## Part 4 - Server

### Option 1: Use Silviu's Server (Easy)

For this option, there is no setup required. When entering your TAG Machine settings later, use the default Bridge URL. When setting up Twilio later, use "https://silviutoderita.com/twilio/" as the Webhook address and "HTTP POST" as the request method.

### Option 2: Set Up Your Own Server (Advanced)

1. Install the [Mosquitto MQTT Broker](https://mosquitto.org/) on your server.

2. Create a new file called acl_file.conf and add it to /etc/mosquitto/ or wherever the configuration settings for Mosquitto are stored on your OS. Add the following lines:

> topic read #
> user USERNAME
> pattern readwrite #

Replace USERNAME with any desired MQTT broker username. Write down this username for later. These settings allow any user to subscribe to the broker, and only a particular user to publish and subscribe. 

3. In this same directory, run the command "sudo mosquitto_passwd -c passwd USERNAME" without the quotes and replace USERNAME with your MQTT broker username. It will then ask you to set a password. Write down this password for later. 

4. In this same directory, edit the file mosquitto.conf and add the following lines:

> listener 1883
> allow_anonymous true

This allows external connections to subscribe anonymously. 

5. Run the following command: "sudo service mosquitto restart"

6. Enable port-forwarding for port 1883. 

7. Install your prefered web server (ie. [Apache](https://httpd.apache.org/)) and enable port forwarding for HTTP/HTTPS.

8. Install [Node.js](https://nodejs.org/en/)

9. Upload the contents of the server folder from this repo to your server. 

10. Edit config.json and add the MQTT broker username and password that you created earlier, and the root directory of your web server. 

11. Run TAG_Bridge.js using node, preferably using a process manager such as pm2 to ensure it restarts upon reboot. 

## Part 5 - Twilio

1. Create a [Twilio](https://www.twilio.com/) account. New users can sign up for a trial without a credit card, which will give you a small amount of funds in your account for sending and receiving SMS and MMS messages. Sending messages is limited to verified numbers only, so it's recommended that you turn off "Send SMS Replies" in the TAG Machine settings later if you're planning on using a trial account. 

2. [Choose a phone-number and "buy" it](https://support.twilio.com/hc/en-us/articles/223135247-How-to-Search-for-and-Buy-a-Twilio-Phone-Number-from-Console) using either your trial funds or real money you have added to your account. 

3. Navigate to the [Phone Numbers page](https://www.twilio.com/console/phone-numbers/incoming) of your [Twilio Console](https://www.twilio.com/console/), click on your number and scroll down to the "Messaging" section. Under "Configure With", select "Webhooks, TwiML Bins, Functions, Studio, or Proxy" from the dropdown box.

<img src="https://github.com/silviu-toderita/TAG_Machine/blob/master/docs/Twilio_Settings.png?raw=true" width="600">

4. Under "A Message Comes In", select "Webhook" from the dropdown box. Next to that, type in "https://silviutoderita.com/twilio/" if using Silviu's server, or "http://youraddress.com/twilio" if using your own server. Next to that, make sure "HTTP Post" is selected as the request method. Press the "Save" button at the bottom when done. 

## Part 6 - TAG Machine Settings

1. Plug in your TAG Machine. The LED should start blinking rapidly almost immediately. This indicates that an incomplete settings file was detected, and it has automatically entered hotspot mode. 

2. Connect your computer or phone to the Wi-Fi network "tagmachine" with default password "12345678". 

3. Navigate to [http://tagmachine.local](http://tagmachine.local). Note that some Android devices may lack support for .local domains, it is recommended you use a different device if the page cannot load on your Android device. 

4. In order to get the Tag Machine to start, the following settings are required at a minimum (though you may want to change some defaults also): Phone Number, WiFi Network Name 1. Please read the [Manual](https://github.com/silviu-toderita/TAG_Machine/blob/master/MANUAL.md) for help with the Settings page. 
