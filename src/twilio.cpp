#include "twilio.h"


const char* host = "api.twilio.com";
const int   httpsPort = 443;
char* account_sid;
char* fingerprint;
String auth_header;

Twilio::Twilio(const char* account_sid_in, const char* auth_token_in, const char* fingerprint_in){
        account_sid = account_sid_in;
        fingerprint = fingerprint_in;
        auth_header = _get_auth_header(account_sid, auth_token_in);
}

//Test Twilio connection
bool Twilio::connect(){
        // Use WiFiClient class to create connection
        WiFiClient client;

        // Connect to Twilio's REST API
        if (!client.connect(host, httpsPort)) return false;

        client.stop();

        return true;
}

//Send a message
bool Twilio::send_message(
        const String& to,
        const String& from,
        const String& message,
        const String& picture_url)
{
        // Check the body is less than 1600 characters in length.
        if (message.length() > 1600) return false;

        // URL encode our message body & picture URL to escape special chars such as '&' and '='
        String encoded_message = urlencode(message);

        // Use WiFiClientSecure class to create TLS 1.2 connection
        WiFiClientSecure client;
        //client.setNoDelay(true);
        client.setFingerprint(fingerprint.c_str());

        // Connect to Twilio's REST API
        if (!client.connect(host, httpsPort)) return false;
        //client.setTimeout(1000);

        // Check the SHA1 Fingerprint (We will watch for CA verification)
        if (!client.verify(fingerprint.c_str(), host)) return false;

        // Attempt to send an SMS or MMS, depending on picture URL
        String post_data = "To=" + urlencode(to) + "&From=" + urlencode(from) + \
        "&Body=" + encoded_message;
        if (picture_url.length() > 0) {
                String encoded_image = urlencode(picture_url);
                post_data += "&MediaUrl=" + encoded_image;
        }

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

        client.println(http_request);
        client.stop();
        return true;
        
}

//Delete a message
bool Twilio::delete_message(const String& messageID)
{
        // Use WiFiClientSecure class to create TLS 1.2 connection
        WiFiClientSecure client;
        client.setFingerprint(fingerprint.c_str());
        
        // Connect to Twilio's REST API
        if (!client.connect(host, httpsPort)) return false;
        client.setTimeout(1000);

        // Check the SHA1 Fingerprint (We will watch for CA verification)
        if (!client.verify(fingerprint.c_str(), host)) return false; 

        // Construct headers and post body manually
        String http_request = "DELETE /2010-04-01/Accounts/" +
                              String(account_sid) + 
                              "/Messages/" + messageID + 
                              ".json HTTP/1.1\r\n" +
                              auth_header + "\r\n" + "Host: " + host + "\r\n" +
                              "Cache-control: no-cache\r\n" +
                              "User-Agent: ESP8266 Twilio Example\r\n" +
                              "Content-Type: " +
                              "application/x-www-form-urlencoded\r\n" +
                              "Connection: close\r\n" +
                              "\r\n" + "\r\n";

        client.println(http_request);

        return true;
}



//Check for messages
String Twilio::check_for_messages(uint16_t maxMessages)
{
        // Use WiFiClientSecure class to create TLS 1.2 connection
        WiFiClientSecure client;
        client.setFingerprint(fingerprint.c_str());
        client.setTimeout(1000);
        
        // Connect to Twilio's REST API
        if (!client.connect(host, httpsPort)) return "";
        // Check the SHA1 Fingerprint (We will watch for CA verification)
        if (!client.verify(fingerprint.c_str(), host)) return ""; 

        // Construct headers and post body manually
        String http_request = "GET /2010-04-01/Accounts/" + String(account_sid) + "/Messages?To=%2B16043739569&PageSize=" + String(maxMessages) + " HTTP/1.1\r\n" +
                              auth_header + "\r\n" + 
                              "Host: " + host + "\r\n" +
                              "Cache-control: no-cache\r\n" +
                              "User-Agent: Fax Machine\r\n" +
                              "Content-Type: " +
                              "application/x-www-form-urlencoded\r\n" +
                              "Connection: close\r\n" +
                              "\r\n" + 
                              "\r\n";

        client.println(http_request);

        String response;
        while (client.connected() || client.available()){
                if(client.available()){
                        response += client.readStringUntil('>') + ">";
                        if(response.indexOf("</Messages>") != -1){
                                return response;
                        }
                }
                yield();
        }

        return "";



        
}

bool Twilio::get_image(String mediaResource, String& mediaURL)
{
        // Use WiFiClientSecure class to create TLS 1.2 connection
        WiFiClientSecure client;
        client.setFingerprint(fingerprint.c_str());
        
        // Connect to Twilio's REST API
        if (!client.connect(host, httpsPort)) return false;
        //client.setTimeout(5000);

        // Check the SHA1 Fingerprint (We will watch for CA verification)
        if (!client.verify(fingerprint.c_str(), host)) return false; 

        // Construct headers and post body manually
        String http_request = "GET " + mediaResource + " HTTP/1.1\r\n" +
                              auth_header + "\r\n" + "Host: " + host + "\r\n" +
                              "Cache-control: no-cache\r\n" +
                              "User-Agent: Fax Machine\r\n" +
                              "Content-Type: " +
                              "application/x-www-form-urlencoded\r\n" +
                              "Connection: close\r\n" +
                              "\r\n" + "\r\n";

        client.println(http_request);

        String response;
        while (client.connected()) {  
                if(client.available()){
                        response += client.readStringUntil('\n') + "\n";
                        if(response.indexOf("</Uri>") != -1){
                                client.stop();
                                break;
                        }
                }
        }

        if(response.indexOf("</Uri>") == -1){
                return false;
        } 
        
        mediaURL = response.substring(response.indexOf("<Uri>") + 5, response.indexOf("</Uri>"));

        return true;
}



/* Private function to create a Basic Auth field and parameter */
String Twilio::_get_auth_header(const String& user, const String& password) {
        size_t toencodeLen = user.length() + password.length() + 2;
        char toencode[toencodeLen];
        memset(toencode, 0, toencodeLen);
        snprintf(
                toencode,
                toencodeLen,
                "%s:%s",
                user.c_str(),
                password.c_str()
        );

        String encoded = base64::encode((uint8_t*)toencode, toencodeLen-1);
        String encoded_string = String(encoded);
        std::string::size_type i = 0;

        // Strip newlines (after every 72 characters in spec)
        while (i < encoded_string.length()) {
                i = encoded_string.indexOf('\n', i);
                if (i == -1) {
                        break;
                }
                encoded_string.remove(i, 1);
        }
        return "Authorization: Basic " + encoded_string;
}
