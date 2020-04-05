const http = require('http');
const express = require('express');
const {urlencoded} = require('body-parser');
var mqtt = require('mqtt');
//process.env.TZ = 'America/Vancouver';


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

    var num_media = req.body.NumMedia;
    var media = '0';

    console.log(`Webhook received from Twilio to: ${to}`);

    //console.log(req.body);

    if(num_media != '0'){
        media = req.body.MediaUrl0.split('/Media/')[1];
    }


    var mqtt_message = `id:${id}\nfrom:${from}\nbody:${body}\nmedia:${media}\ntime:${time}`;

    mqtt_client.publish(`smsin-${to}`, mqtt_message, {qos:1});
    console.log(`Published MQTT Message!\nTopic: smsin-${to} \nMessage:\n----\n${mqtt_message}\n----------------`);

    res.writeHead(200, {'Content-Type': 'text/xml'});
    res.end('<Response></Response>');

});

http.createServer(app).listen(3000, () => {
    console.log('Server listening for Twilio webhooks');
});

