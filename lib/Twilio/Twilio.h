#include "Arduino.h"
#include "WiFiClientSecure.h"
#include "base64.h"
#include "url_coding.h"

class Twilio {

    public:
        Twilio();

        void 
            config (String, String, String);

        bool 
            send_message(String, String, String);
    
    private:

        String 
            get_auth_header(String, String);
            
};
