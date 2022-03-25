#ifndef WIFI_WRAPPER_H
#define WIFI_WRAPPER_H

#include "esp_err.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include <vector>
#include <functional>

class WifiWrapper {
public:
    enum status_t {
        ENABLED,
        DISABLED
    };

    esp_err_t sta_connect(const char* ssid, const char* pass, 
                          int max_reconnect_count = -1,
                          bool is_async = false);
    esp_err_t sta_disconnect();
    status_t get_sta_status();
    bool is_sta_connected();

    
    esp_err_t ap_start(const char* ssid, const char* pass);
    esp_err_t ap_stop();
    status_t get_ap_status();

    void add_sta_connection_handler(std::function<void(WifiWrapper*, bool)> cb);
    void add_sta_disconnect_handler(std::function<void(WifiWrapper*)> cb);
    
    static WifiWrapper* getInstanse();

protected:
    static void wifi_event_handler(
            void* arg, esp_event_base_t event_base,
            int32_t event_id, void* event_data
    );

    static void ip_event_handler(
            void* arg, esp_event_base_t event_base,
            int32_t event_id, void* event_data
    );

    void notify_sta_connect(bool is_connected);
    void notify_sta_disconnect();

private: 
    WifiWrapper();
    static WifiWrapper* _instance;

protected:
    esp_netif_t* netif;
    
    status_t _sta_st = DISABLED;
    status_t  _ap_st = DISABLED;

    int sta_max_reconnect_count = 0;
    int sta_reconnect_count = 0;
    EventGroupHandle_t sta_event_group;

    std::vector<std::function<void(WifiWrapper*)>> sta_disconnect_handlers;
    std::vector<std::function<void(WifiWrapper*, bool)>> sta_conn_handlers; 

};

#endif // WIFI_WRAPPER_H
