const http = require('http');
const express = require('express');
const {urlencoded} = require('body-parser');
const Jimp = require("jimp");
var mqtt = require('mqtt');
var floydSteinberg = require('floyd-steinberg');
var fs = require('fs');
var PNG = require('pngjs').PNG;

function setBit(number, bitPosition) {
  return number | (1 << bitPosition);
}

var mqtt_options = {
    port: 1883,
    username: 'silviu',
    password: 'sebastian'
};

var mqtt_client = mqtt.connect('http://localhost',mqtt_options);

mqtt_client.on('connect', function(){
    console.log('Connected to MQTT Broker\n----------------');
});


const app = express();

app.use(urlencoded({ extended: false}));

app.post( '/sms', ( req, res ) => {

    var id = req.body.MessageSid;
    var from = req.body.From.slice(1);
    var to = req.body.To.slice(1);
    var body = req.body.Body;
    var time = Math.floor(Date.now()/1000);
    var media = req.body.NumMedia;

    console.log(`Webhook received from Twilio to: ${to}`);

    res.writeHead(200, {'Content-Type': 'text/xml'});
    res.end('<Response></Response>');

    if(media != "0"){

        const request = http.get("http://ackwxpcapo.cloudimg.io/v7/" + req.body.MediaUrl0 + "?p=fax", function(response){
            response.pipe(new PNG()).on('parsed', function(){
                floydSteinberg(this).pack().pipe(fs.createWriteStream('test.png'));
            });
        });

        
        
        Jimp.read('test.png', function(err, image){
            if(err){
                console.log(err);
            }else{
                media += "w" + image.bitmap.width + "h" + image.bitmap.height;
                console.log(media);
                var size = image.bitmap.width * image.bitmap.height;
      
                var buffer = Buffer.alloc(size/8);
                var bitCounter = 1;
                var byteCounter = 0;
                for(var i=1; i<=size; i++){
                    if(image.bitmap.data[i] == 0){
                        buffer[byteCounter] = setBit(0,bitCounter-1);
                    }else{
                        buffer[byteCounter] = setBit(1,bitCounter-1);
                    }

                    if(bitCounter != 8){
                        bitCounter++
                    }else{
                        bitCounter = 1;
                        byteCounter++;
                    }

                }

                fs.writeFile("/var/www/silviutoderita-com/output.txt", buffer, function(err){
                    if(err){
                        return console.log(err);
                    }
                    console.log("File Saved!");
                })  

            }

            var mqtt_message = `id:${id}\nfrom:${from}\nbody:${body}\nmedia:${media}\ntime:${time}`;

            mqtt_client.publish(`smsin-${to}`, mqtt_message, {qos:1});
            console.log(`Published MQTT Message!\nTopic: smsin-${to} \nMessage:\n----\n${mqtt_message}\n----------------`);

        });

    }else{
        var mqtt_message = `id:${id}\nfrom:${from}\nbody:${body}\nmedia:${media}\ntime:${time}`;

        mqtt_client.publish(`smsin-${to}`, mqtt_message, {qos:1});
        console.log(`Published MQTT Message!\nTopic: smsin-${to} \nMessage:\n----\n${mqtt_message}\n----------------`); 
    }






});

http.createServer(app).listen(3000, () => {
    console.log('Server listening for Twilio webhooks');
});

