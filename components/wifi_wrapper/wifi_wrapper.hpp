#ifndef WIFI_WRAPPER_H
#define WIFI_WRAPPER_H

#include "esp_err.h"
#include "esp_netif.h"
#include "esp_event.h"

class WifiWrapper {
public:
    //WifiWrapper(); // Init netif, NVS, etc
    //~WifiWrapper();

    enum sta_status {
        CONNECTED,
        DISCONNECTED
    };

    esp_err_t sta_connect(const char* ssid, const char* pass);
    esp_err_t sta_disconnect();
    sta_status get_sta_status();

    
    enum ap_status {
        ENABLED,
        DISABLED
    };
    
    esp_err_t ap_start(const char* ssid, const char* pass);
    esp_err_t ap_stop();
    ap_status get_ap_status();

    static esp_err_t wifi_init();

protected:
    static void wifi_event_handler(
            void* arg, esp_event_base_t event_base,
            int32_t event_id, void* event_data
    );

protected:
    esp_netif_t* netif;
    
    sta_status _sta_st = DISCONNECTED;
    ap_status  _ap_st = DISABLED;
};

#endif // WIFI_WRAPPER_H
