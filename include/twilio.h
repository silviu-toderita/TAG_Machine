#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "base64.h"
#include "url_coding.h"

class Twilio {
public:
        Twilio(
                const char* account_sid_in, 
                const char* auth_token_in,
                const char* fingerprint_in
        );

        // Empty destructor
        ~Twilio() = default; 

        bool connect();

        bool send_message(
                const String& to,
                const String& from,
                const String& message,
                const String& picture_url = ""
        );

        bool delete_message(
                const String& messageID
        );

        bool check_for_messages(
                String& dateTime,
                String& from, 
                String& message, 
                String& id,
                String& media
        );

        bool get_image(
                String mediaResource,
                String& mediaURL
        );

private:
        // Account SID and Auth Token come from the Twilio console.
        // See: https://twilio.com/console for more.

        // Used for the username of the auth header
        String account_sid;
        // Used for the password of the auth header
        String auth_token;
        // To store the Twilio API SHA1 Fingerprint (get it from a browser)
        String fingerprint;

        // Utilities
        static String _get_auth_header(
                const String& user, 
                const String& password
        );
        
};
