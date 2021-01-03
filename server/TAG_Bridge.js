/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    TAG_Bridge is a Twilio to MQTT bridge running on Node.js. It can receive new 
    messages at a Webhook that Twilio can point to, and send them to a local MQTT
    broker (running on the same server). 

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

const http = require('http'); //HTTP Library
const express = require('express'); //Express framework
const {urlencoded} = require('body-parser'); //URL Decoding library
const mqtt = require('mqtt'); //MQTT Library
const dither = require('floyd-steinberg'); //Dithering Library
const fs = require('fs'); //File System Library
const JIMP = require('jimp'); //Javascript Image Manipulation Program
const storage = require('node-persist'); //Persistent Storage Library
const twilio = require('twilio'); //Twilio library

//Load settings from config file
var config = JSON.parse(fs.readFileSync('config.json'));
var MQTT_broker_username = config.MQTT_broker_username;
var MQTT_broker_password = config.MQTT_broker_password;
var www_root_folder = config.www_root_folder;
var emoji = require('node-emoji');
var getUrls = require ('get-urls');
var QRCode = require ('qrcode');

//Initialize the storage object
storage.init();

//Connect to the local MQTT Server
var mqtt_client = mqtt.connect('http://localhost', {
    port: 1883,
    username: MQTT_broker_username,
    password: MQTT_broker_password
});

//Use express library
const app = express();
app.use(urlencoded({ extended: false}));

/*  bit_buffer: Buffer object that can be assigned 1 byte or 1 bit at a time. 
        size: Size in bytes
        FUNCTIONS:
            set_bit: Set the next bit
                value: True or False
            set_byte: Set the next byte. If the last byte was incomplete (ie. only 3 bits written), it will write to the last byte and overwrite the set bits. 
                value: Byte to set
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
function bit_buffer(size){
    this.buffer = Buffer.alloc(Math.floor(size/8)); //Initialize a byte buffer
    var current_bit = 0; //Start at bit 0

    //Function for setting a single bit
    this.set_bit = function(value) {
        
        var byte = Math.floor(current_bit / 8); 
        //Determine the current bit
        var bit = 7 - (current_bit % 8); 
    
        //If the bit is zero, set the next bit to 0
        if(value == 0){
            this.buffer[byte] &=  ~(1 << bit);
        //Otherwise, set the next bit to 1
        }else{
            this.buffer[byte] |= (1 << bit);
        }
        //Increment the current bit by one
        current_bit++;
    }

    //Function for setting a single byte
    this.set_byte = function(value){
        //Determine the current byte
        var byte = Math.floor(current_bit / 8);
        //Set the next byte
        this.buffer[byte] = value;

        //Increment the current bit by 8
        current_bit = current_bit + 8; 
    }

}

/*  save_image: Download the specified image and save it as a 1-bit bitmap in a custom format that the TAG Machine can read
        URL: URL of the image to download
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
function save_image(URL){
    var dither_flag = false;
    if(URL.indexOf("qr") == -1) dither_flag = true;

    //Wrap function in promise to only return once processing is done
    return new Promise(function(resolve, reject){
        //Read the image
        JIMP.read(URL, async (err, image) => {
            if(err) throw err;

            //If the image is not a supported format, return "NS"
            if(!image){
                resolve("NS");
                return;
            }
            
            //Scale the image to fit the maximum width of 384
            image.scaleToFit(384, JIMP.AUTO);

            //Calculate the amount of pixels in the image
            var size = image.bitmap.width * image.bitmap.height; 
    
            //Iterate through each pixel and check for transparency
            for(var i = 0; i < size * 4; i = i + 4){
                //If there is any transparency and the colour is black...
                if(image.bitmap.data[i+3] < 255 && image.bitmap.data[i] == 0 && image.bitmap.data[i+1] == 0 && image.bitmap.data[i+2] == 0){
                    //Change the pixel to white
                    image.bitmap.data[i] = 255;
                    image.bitmap.data[i+1] = 255;
                    image.bitmap.data[i+2] = 255;
                }
            }
    
            var processed_image;
            //Dither the image, turning it into a 1-bit bitmap
            if(dither_flag){
                processed_image = dither(image.bitmap);
            }else{
                processed_image = image.bitmap;
            }
            //Create new bit buffer with the size of the image plus 2 bytes for height information
            let buffer = new bit_buffer(size + 16);
            
            //Assign the first 2 bytes of the buffer to the height
            buffer.set_byte(Math.floor(processed_image.height/256));
            buffer.set_byte(processed_image.height%256);
    
            //Write each pixel to the buffer. Only write every 4th pixel, as the bitmap is still in 32-bit format. 
            for(var i = 0; i < size * 4; i = i + 4){
                if(processed_image.data[i] == 0){
                    buffer.set_bit(1);
                }else{
                    buffer.set_bit(0);
                }
    
            }
            
            //Get the current image number based on the last image number
            var image_number = 1;
            var last_image_number = await storage.getItem('image_number');
            if(last_image_number < 255){
                image_number = last_image_number + 1;
            }

            //Output the file
            fs.writeFileSync(www_root_folder + "img/" + image_number + ".dat", buffer.buffer);

            //Store this imagenumber
            await storage.setItem('image_number', image_number);

            //Resolve the promise and return the image number
            resolve(image_number);
        });
    });
    
}

/*  publish_MQTT_message: Publish a message to the MQTT server
        data: Data to publish in the message
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
async function publish_MQTT_message(data){
    //This variable will store any image numbers in this message
    var media; 
    //If there are no images, send 0
    if(data.image_count == 0){
        media = '0';
    //If there are images...
    }else{
        //For each image...
        for(var i = 0; i < data.image_count; i++){
            //Store the image number after the image is processed
            var current_image = String(await save_image(data.media[i]));
            
            //Add the image number to the media var
            if(media){
                media = media + current_image;
            }else{
                media = current_image;
            }

            //If it's not the last image, add a comma to media string
            if(i != data.image_count - 1){
                media = media + ',';
            }
        }
        
    }

    //Form the MQTT message
    mqtt_message = `id:${data.id}\nfrom:${data.from}\nbody:${emoji.unemojify(data.body)}\nmedia:${media}\ntime:${data.time}`;

    //Publish the MQTT message
    mqtt_client.publish(`smsin-${data.to}`, mqtt_message, {qos:1});
    console.log(`Published MQTT Message!\nTopic: smsin-${data.to} \nMessage:\n----\n${mqtt_message}\n----------------`); 
}

//Do this if a POST request is received to /sms
app.post( '/', (req, res)  => {
    console.log(`Webhook received from Twilio...`);

    //Respond to Twilio with a message to acknowledge receipt
    res.writeHead(200, {'Content-Type': 'text/xml'});
    res.end('<Response></Response>');

    var image_num = parseInt(req.body.NumMedia);
    var images = [
        req.body.MediaUrl0,
        req.body.MediaUrl1,
        req.body.MediaUrl2,
        req.body.MediaUrl3,
        req.body.MediaUrl4,
        req.body.MediaUrl5,
        req.body.MediaUrl6,
        req.body.MediaUrl7,
        req.body.MediaUrl8,
        req.body.MediaUrl9];

    var URLs = Array.from(getUrls(req.body.Body));

    for(var i = 0; i < URLs.length; i++){
        var path = www_root_folder + "qr/" + i + ".png";
        (async () => {
            await QRCode.toFile(path,URLs[i]);
            console.log("Generated QR Code...");
            images[image_num] = path;
            image_num++;
        })()

        while(images[image_num-1] === undefined){
            require('deasync').runLoopOnce();
        }
    }
    
    //Send a message to the MQTT broker, pass all relevant data from Twilio's webhook
    publish_MQTT_message({
        to: req.body.To.slice(1),
        from: req.body.From.slice(1),
        id: req.body.MessageSid,
        body: req.body.Body,
        image_count: image_num,
        media: images,
        time: Math.floor(Date.now()/1000)
    });
        
});

//Set up the server to listen on port 3000
http.createServer(app).listen(3000, () => {
    console.log('Server listening for Twilio webhooks');
});

