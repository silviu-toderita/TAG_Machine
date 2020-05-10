/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    ESP8266 library for connecting to Twilio and sending SMS messages.

    To use, initialize a Twilio object. Send a new message with the send_message()
    function.

    Created by Silviu Toderita in 2020.
    silviu.toderita@gmail.com
    silviutoderita.com
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Twilio.h"

//Twilio API address and port
const char* host = "api.twilio.com";
const int   httpsPort = 443;

/*  Twilio constructor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
Twilio::Twilio(){
}

/*  config: Set configuration parameters
        account_sid_in: Twilio account ID
        auth_token_in: Twilio account auth token
        fingerprint_in: Twilio SHA1 fingerprint
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void Twilio::config(String account_sid_in, String auth_token_in, String fingerprint_in){
        account_sid = account_sid_in;
        fingerprint = fingerprint_in;
        auth_header = get_auth_header(account_sid, auth_token_in);
}

/*  send_message: Send a message using Twilio
        to: Phone number to send the message to, including country code
        from: Phone number to send the message from, including country code. Must
            be a number owned by the account on Twilio.
        message: Message body to send
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
bool Twilio::send_message(String to, String from, String message)
{
    // Check the body is less than 1600 characters in length
    if (message.length() > 1600) return false;
    // URL encode the message body to escape special chars such as '&' and '='
    String encoded_message = urlencode(message);

    // Use WiFiClientSecure class to create TLS 1.2 connection
    WiFiClientSecure client;
    //Set the current Twilio fingerprint
    client.setFingerprint(fingerprint.c_str());

    // Connect to Twilio's REST API and verify the fingerprint
    if (!client.connect(host, httpsPort)) return false;
    if (!client.verify(fingerprint.c_str(), host)) return false;

    //Create the post data String
    String post_data = "To=" + urlencode(to) + "&From=" + urlencode(from) + \
    "&Body=" + encoded_message;

    // Construct headers and post body manually
    String http_request = "POST /2010-04-01/Accounts/" +
                            String(account_sid) + "/Messages HTTP/1.1\r\n" +
                            auth_header + "\r\n" + "Host: " + host + "\r\n" +
                            "Cache-control: no-cache\r\n" +
                            "User-Agent: ESP8266 Twilio Example\r\n" +
                            "Content-Type: " +
                            "application/x-www-form-urlencoded\r\n" +
                            "Content-Length: " + post_data.length() +"\r\n" +
                            "Connection: close\r\n" +
                            "\r\n" + post_data + "\r\n";

    //Sent the request to Twilio
    client.println(http_request);
    client.stop();
    return true;
}

/*  get_auth_header: Generate an auth header from a username and password
        user:
        password:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
String Twilio::get_auth_header(String user, String password) {
    //Create a to_encode char array with a length equal to the length of the username + password + 2
    size_t to_encode_length = user.length() + password.length() + 2;
    char to_encode[to_encode_length];

    //Fill the to_encode array with the username and password
    memset(to_encode, 0, to_encode_length);
    snprintf(to_encode, to_encode_length, "%s:%s", user.c_str(), password.c_str());

    //Encode the array in base64
    String encoded = base64::encode((uint8_t*)to_encode, to_encode_length-1);

    //Remove line breaks
    uint16_t i = 0;
    while (i < encoded.length()) {
        i = encoded.indexOf('\n', i);
        if (i == -1) {
                break;
        }
        encoded.remove(i, 1);
    }

    //Return "Authorization: Basic " plus the encoded string
    return "Authorization: Basic " + encoded;
}