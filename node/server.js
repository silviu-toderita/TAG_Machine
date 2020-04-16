const http = require('http');
const express = require('express');
const {urlencoded} = require('body-parser');
const Jimp = require("jimp");
var mqtt = require('mqtt');
var floydSteinberg = require('floyd-steinberg');
var fs = require('fs');
var PNG = require('pngjs').PNG;

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
                var ditherImage = floydSteinberg(this);

                var size = ditherImage.width * ditherImage.height;

                var buffer = Buffer.alloc(size/8 + 4);

                buffer.writeUInt8(Math.floor(ditherImage.width/128), 0);
                buffer.writeUInt8(ditherImage.width%128, 1);
                buffer.writeUInt8(Math.floor(ditherImage.height/128), 2);
                buffer.writeUInt8(ditherImage.height%128, 3);
                
                var byte = 0;
                var bitCounter = 7;
                var byteCounter = 4;
                for(var i = 0; i < size * 4; i = i + 4){
                    if(ditherImage.data[i] == 0){
                        byte |= (1<<bitCounter);
                    }else{
                        byte |= (0<<bitCounter);
                    }

                    if(bitCounter == 0){
                        bitCounter = 7;
                        buffer.writeUInt8(byte, byteCounter);
                        byte = 0;
                        byteCounter++;
                    }else{
                        bitCounter = bitCounter - 1;
                    }
                }

                fs.writeFileSync("/var/www/silviutoderita-com/output.txt", buffer, function(err){
                    if(err){
                        return console.log(err);
                    }
                    console.log("File Saved!");
                });  

            var mqtt_message = `id:${id}\nfrom:${from}\nbody:${body}\nmedia:${media}\ntime:${time}`;

            mqtt_client.publish(`smsin-${to}`, mqtt_message, {qos:1});
            console.log(`Published MQTT Message!\nTopic: smsin-${to} \nMessage:\n----\n${mqtt_message}\n----------------`);
            });

            
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

