#include "wifi_wrapper.hpp"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include <cstring>
#include <stdexcept>

#define TAG "Wifi wrapper"


#define STA_CONNECTED_BIT               BIT(0)
#define STA_DISCONNECTED_BIT            BIT(1)  //Currently not implemented
#define STA_CONN_LIMIT_REACHED_BIT      BIT(2)


esp_err_t WifiWrapper::sta_connect(
    const char* ssid, 
    const char* pass,
    int max_reconnect_count, 
    bool is_async
) {
    if(ENABLED == get_ap_status()) {
        ESP_LOGE(TAG, "Can not connect STA: AP running");
        return ESP_FAIL;
    }

    if(ENABLED == get_sta_status()) {
        ESP_LOGE(TAG, "Can not connect STA: already connected");
        return ESP_FAIL;
    }

    xEventGroupClearBits(
        sta_event_group, 
        STA_CONNECTED_BIT | STA_DISCONNECTED_BIT | STA_CONN_LIMIT_REACHED_BIT
    );

    xEventGroupSetBits(sta_event_group, STA_DISCONNECTED_BIT);
    sta_max_reconnect_count = max_reconnect_count;

    netif = esp_netif_create_default_wifi_sta();
    if(netif == nullptr) {
        ESP_LOGE(TAG, "Can not connect STA, netif not initiated");
        return ESP_FAIL;
    }

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid)+1);
    memcpy(wifi_config.sta.password, pass, strlen(pass)+1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.required = false;

    esp_err_t st;
    st = esp_wifi_set_mode(WIFI_MODE_STA);
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Can not set wifi mode");
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_netif_destroy(netif);
        return st;
    }

    st = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Can not configure wifi");
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_netif_destroy(netif);
        return st;
    }

    st = esp_wifi_start();
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Can not start AP");
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_netif_destroy(netif);
        return st;
    }

    _sta_st = ENABLED;

    if(is_async) {
        return ESP_OK;
    }

    EventBits_t bits =  xEventGroupWaitBits(
        sta_event_group, 
        STA_CONNECTED_BIT | STA_CONN_LIMIT_REACHED_BIT,
        pdFALSE, pdFALSE,
        portMAX_DELAY
    );

    if(bits & STA_CONNECTED_BIT) {
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }

}

esp_err_t WifiWrapper::sta_disconnect() {
    if(DISABLED == get_sta_status()) {
        ESP_LOGE(TAG, "Can not disconnect, sta is not connected");
        return ESP_FAIL;
    }

    _sta_st = DISABLED;
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_netif_destroy(netif);

    xEventGroupClearBits(
        sta_event_group, 
        STA_CONNECTED_BIT | STA_DISCONNECTED_BIT | STA_CONN_LIMIT_REACHED_BIT
    );

    return ESP_OK;
}

WifiWrapper::status_t WifiWrapper::get_sta_status() {
    return _sta_st;
}

bool WifiWrapper::is_sta_connected() {
    return xEventGroupGetBits(sta_event_group) & STA_CONNECTED_BIT;
}


esp_err_t WifiWrapper::ap_start(const char* ssid, const char* pass) {
    if(ENABLED == get_ap_status()) {
        ESP_LOGE(TAG, "Can not start AP: already running");
        return ESP_FAIL;
    }

    if(ENABLED == get_sta_status()) {
        ESP_LOGE(TAG, "Can not start AP: STA running");
        return ESP_FAIL;
    }

    netif = esp_netif_create_default_wifi_ap();
    if(netif == nullptr) {
        ESP_LOGE(TAG, "Can not create AP, netif not initiated");
        return ESP_FAIL;
    }

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    memcpy(wifi_config.ap.ssid, ssid, strlen(ssid));
    wifi_config.ap.ssid_len = strlen(ssid);
    memcpy(wifi_config.ap.password, pass, strlen(pass)+1);
    wifi_config.ap.max_connection = 10;
    wifi_config.ap.authmode = (strlen(pass) == 0) ? WIFI_AUTH_OPEN:WIFI_AUTH_WPA_WPA2_PSK;
    wifi_config.ap.ssid_hidden = false;

    esp_err_t st;
    st = esp_wifi_set_mode(WIFI_MODE_AP);
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Can not set wifi mode");
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_netif_destroy(netif);
        return st;
    }

    st = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Can not configure wifi");
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_netif_destroy(netif);
        return st;
    }

    st = esp_wifi_start();
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Can not start AP");
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_netif_destroy(netif);
        return st;
    }

    ESP_LOGI(TAG, "SoftAP started. SSID:%s password:%s channel:%d",
             ssid, pass, wifi_config.ap.channel);

    _ap_st = ENABLED;
    return ESP_OK;
}

esp_err_t WifiWrapper::ap_stop() {
    if(DISABLED == get_ap_status()) {
        ESP_LOGE(TAG, "Can not stop AP, it is already stopped");
        return ESP_FAIL;
    }

    _ap_st = DISABLED;
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_netif_destroy(netif);

    return ESP_OK;
}

WifiWrapper::status_t WifiWrapper::get_ap_status() {
    return _ap_st;
}

void WifiWrapper::add_sta_connection_handler(
    std::function<void(WifiWrapper*, bool)> cb
) {
    sta_conn_handlers.push_back(cb);
}

void WifiWrapper::add_sta_disconnect_handler(
    std::function<void(WifiWrapper*)> cb
) {
    sta_disconnect_handlers.push_back(cb);
}

extern "C" void WifiWrapper::wifi_event_handler(
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data
) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (
        event_id == WIFI_EVENT_STA_START || 
        event_id == WIFI_EVENT_STA_DISCONNECTED
    ) {
        WifiWrapper* w = WifiWrapper::getInstanse();
        EventBits_t bits = xEventGroupClearBits(
            w->sta_event_group, STA_CONNECTED_BIT
        );

        if(bits & STA_CONNECTED_BIT) {
            w->sta_disconnect();
            w->notify_sta_disconnect();
            return;
        }

        if(
            (w->sta_reconnect_count >= w->sta_max_reconnect_count) && 
            (w->sta_max_reconnect_count != -1)
        ) {
            ESP_LOGE(
                TAG, "STA: Reconnect limit (%d) reached", w->sta_max_reconnect_count
            );
            xEventGroupSetBits(
                w->sta_event_group, STA_CONN_LIMIT_REACHED_BIT
            );
            w->notify_sta_connect(false);
            return;
        }

        ESP_LOGI(TAG, "Connecting to AP (%d)...", w->sta_reconnect_count);
        w->sta_reconnect_count++;

        esp_wifi_connect();
    }
}

extern "C" void WifiWrapper::ip_event_handler(
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data
) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

        WifiWrapper* w = WifiWrapper::getInstanse();
        xEventGroupSetBits(
            w->sta_event_group,
            STA_CONNECTED_BIT
        );

        w->notify_sta_connect(true);
        w->sta_reconnect_count = 0;
    }
}

void WifiWrapper::notify_sta_connect(bool is_connected) {
    for(auto cb: sta_conn_handlers) {
        cb(this, is_connected);
    }
}


void WifiWrapper::notify_sta_disconnect() {
    for(auto cb: sta_disconnect_handlers) {
        cb(this);
    }
}


WifiWrapper* WifiWrapper::_instance = nullptr;

WifiWrapper::WifiWrapper() {
    esp_err_t st;
    st = esp_netif_init();
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Error while netif init: %s", esp_err_to_name(st));
        throw std::runtime_error(esp_err_to_name(st));
    }

    st = esp_event_loop_create_default();
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Error while event loop init: %s", esp_err_to_name(st));
        throw std::runtime_error(esp_err_to_name(st));
    }
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    st = esp_wifi_init(&cfg);
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Error while WIFI init: %s", esp_err_to_name(st));
        throw std::runtime_error(esp_err_to_name(st));
    }

    st = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL
    );

    st = esp_event_handler_instance_register(
        IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL
    );

    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Error while event handler registration: %s", esp_err_to_name(st));
        throw std::runtime_error(esp_err_to_name(st));
    }

    sta_event_group = xEventGroupCreate();
}

WifiWrapper* WifiWrapper::getInstanse() {
    if(_instance == nullptr) {
        _instance = new WifiWrapper();
    }

    return _instance;
}