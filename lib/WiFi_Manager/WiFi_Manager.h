//wm_status type for holding WiFi Manager status
typedef enum {
    WM_IDLE                 = 0,
    WM_SCANNING             = 1,
    WM_CONNECTING           = 2,
    WM_CONNECTION_SUCCESS   = 3,
    WM_CONNECTED            = 4,
    WM_CONNECTION_LOST      = 5,
    WM_HOTSPOT              = 6
} wm_status;

//Callback function type (no args)
typedef void (*void_function_pointer)();

class WiFi_Manager{
    public:

        WiFi_Manager(uint32_t);
        
        void 
            set_callbacks(void_function_pointer, void_function_pointer, void_function_pointer);
        
        bool 
            begin(),
            create_hotspot(const char*, const char*),
            add_network(char*, char*);
        
        wm_status 
            handle();

        String 
            get_SSID();

};