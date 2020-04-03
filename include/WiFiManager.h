typedef enum {
    WM_IDLE                 = 0,
    WM_SCANNING             = 1,
    WM_SCAN_FAILED          = 2,
    WM_CONNECTING           = 3,
    WM_CONNECTION_FAILED    = 4,
    WM_CONNECTION_SUCCESS   = 5,
    WM_CONNECTED            = 6,
    WM_CONNECTION_LOST      = 7,
    WM_HOTSPOT              = 8
} wm_status_t;

class WiFiManager{
    public:

    WiFiManager(uint32_t);
    
    bool begin();
    bool createHotspot(const char*, const char*);
    wm_status_t handle();
    void setIdle();
    void setConnected();
    String SSID();
    bool addNetwork(char*, char*);

};