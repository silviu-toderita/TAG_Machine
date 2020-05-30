const http = require('http'); 
const express = require('express');
const {urlencoded} = require('body-parser');
const mqtt = require('mqtt'); //MQTT Library
const floydSteinberg = require('floyd-steinberg'); //Dithering Library
const fs = require('fs'); //File System Library
const Jimp = require('jimp'); //Javascript Image Manipulation Program
const storage = require('node-persist');


//Buffer object that can be assigned 1 byte or 1 bit at a time
function BitBuffer(size){
    this.buffer = Buffer.alloc(Math.floor(size/8)); //Initialize a byte buffer
    var currentBit = 0; //Start us off at bit 0

    //Function for setting a single bit
    this.setBit = function(value) {
        var byte = Math.floor(currentBit / 8); //Determine which byte we're on
        var bit = 7 - (currentBit % 8); //Determine which bit of the current byte we're on
    
        //If the bit should be zero, set it to 0
        if(value == 0){
            this.buffer[byte] &=  ~(1 << bit);
        //Otherwise, set it to 1
        }else{
            this.buffer[byte] |= (1 << bit);
        }
        //Increment the current bit by one
        currentBit++;
    }

    //Function for setting a single byte. Note that this function will overwrite any bits written to the current byte already
    this.setByte = function(value){
        var byte = Math.floor(currentBit / 8); //Determine which byte we're on
        this.buffer[byte] = value; //Set the value

        currentBit = currentBit + 8; //Increment the current bit by 8
    }

}

//This function saves the image as a 1-bit bitmap text file
function saveImage(URL){

    //Wrap function in promise to only return once processing is done
    return new Promise(function(resolve, reject){
        //Read the image
        Jimp.read(URL, async (err, image) => {
            if(!image){
                resolve("NS");
                return;
            }
            
            //Scale it to fit the maximum width of 384
            image.scaleToFit(384, Jimp.AUTO);

            //The amount of pixels in this image
            var size = image.bitmap.width * image.bitmap.height; 
    
            //Go through each pixel. If any pixel has slight transparency and is black, change it to white. 
            for(var i = 0; i < size * 4; i = i + 4){
                if(image.bitmap.data[i+3] < 255 && image.bitmap.data[i] == 0 && image.bitmap.data[i+1] == 0 && image.bitmap.data[i+2] == 0){
                    image.bitmap.data[i] = 255;
                    image.bitmap.data[i+1] = 255;
                    image.bitmap.data[i+2] = 255;
                }
            }
    
            //Dither the image, turning it into a 1-bit bitmap
            var ditherImage = floydSteinberg(image.bitmap);
    
            //Create new bit buffer
            let buffer = new BitBuffer(size + 16);
            
            //Assign the first 2 bytes of the buffer to the height
            buffer.setByte(Math.floor(ditherImage.height/256));
            buffer.setByte(ditherImage.height%256);
    
            //Write each pixel to the buffer. We only write every 4th pixel, as the bitmap is still in 32-bit format. 
            for(var i = 0; i < size * 4; i = i + 4){
                if(ditherImage.data[i] == 0){
                    buffer.setBit(1);
                }else{
                    buffer.setBit(0);
                }
    
            }
            
            //Get the current image number based on the last image number, so that we can store up to 256 images at once in case the tag machine is offline
            var imageNum = 1;
            var storedImageNum = await storage.getItem('imageNum');
            if(storedImageNum < 255){
                imageNum = storedImageNum + 1;
            }

            //Output the file
            fs.writeFileSync("/var/www/silviutoderita-com/img/" + imageNum + ".dat", buffer.buffer);

            //Store this imagenumber
            await storage.setItem('imageNum', imageNum);

            //Resolve the promise once all the image processing is done
            resolve(imageNum);
        });
    });
    
}

//Initialize the storage object
storage.init();

//Connect to the local MQTT Server
var mqtt_client = mqtt.connect('http://localhost', {
    port: 1883,
    username: 'silviu',
    password: 'sebastian'
});


//Publish MQTT message
async function sendMessage(data){
    var media; //This variable will store any image numbers in this message
    //If there are no images, send 0
    if(data.numMedia == 0){
        media = '0';
    }else{
        //If there are images, save each one and add its number to the media variable
        for(var i = 0; i < data.numMedia; i++){
            //Store the image number after the image is processed
            var currentImg = String(await saveImage(data.media[i]));
            
            //Add the image number to the media var
            if(media){
                media = media + currentImg;
            }else{
                media = currentImg;
            }

            //If it's not the last image, add a comma to media
            if(i != data.numMedia - 1){
                media = media + ',';
            }
        }
        
    }

    //Form the MQTT message
    mqtt_message = `id:${data.id}\nfrom:${data.from}\nbody:${data.body}\nmedia:${media}\ntime:${data.time}`;

    //Publish the MQTT message
    mqtt_client.publish(`smsin-${data.to}`, mqtt_message, {qos:1});
    console.log(`Published MQTT Message!\nTopic: smsin-${data.to} \nMessage:\n----\n${mqtt_message}\n----------------`); 
}


//Use express library
const app = express();
app.use(urlencoded({ extended: false}));

//Do this if a POST request is received
app.post( '/sms', (req, res)  => {

    console.log(`Webhook received from Twilio...`);

    //Respond to Twilio so it's happy
    res.writeHead(200, {'Content-Type': 'text/xml'});
    res.end('<Response></Response>');

    //Send a message using MQTT, pass all relevant data from Twilio's webhook
    sendMessage({
        to: req.body.To.slice(1),
        from: req.body.From.slice(1),
        id: req.body.MessageSid,
        body: req.body.Body,
        numMedia: parseInt(req.body.NumMedia),
        media: [
            req.body.MediaUrl0,
            req.body.MediaUrl1,
            req.body.MediaUrl2,
            req.body.MediaUrl3,
            req.body.MediaUrl4,
            req.body.MediaUrl5,
            req.body.MediaUrl6,
            req.body.MediaUrl7,
            req.body.MediaUrl8,
            req.body.MediaUrl9
        ],
        time: Math.floor(Date.now()/1000)
    });
        
});

//Set up the server to listen on port 3000
http.createServer(app).listen(3000, () => {
    console.log('Server listening for Twilio webhooks');
});

