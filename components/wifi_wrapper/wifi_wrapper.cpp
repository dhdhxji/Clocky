#include "wifi_wrapper.hpp"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include <cstring>

#define TAG "Wifi wrapper"



esp_err_t WifiWrapper::sta_connect(const char* ssid, const char* pass) {
    if(ENABLED == get_ap_status()) {
        ESP_LOGE(TAG, "Can not connect STA: AP running");
        return ESP_FAIL;
    }

    if(ENABLED == get_sta_status()) {
        ESP_LOGE(TAG, "Can not connect STA: already connected");
        return ESP_FAIL;
    }

    netif = esp_netif_create_default_wifi_sta();
    if(netif == nullptr) {
        ESP_LOGE(TAG, "Can not connect STA, netif not initiated");
        return ESP_FAIL;
    }

    wifi_config_t wifi_config;
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
    return ESP_OK;
}

esp_err_t WifiWrapper::sta_disconnect() {
    if(ENABLED == get_sta_status()) {
        ESP_LOGE(TAG, "Can not disconnect, sta is not connected");
        return ESP_FAIL;
    }

    _sta_st = DISABLED;
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_netif_destroy(netif);

    return ESP_OK;
}

WifiWrapper::status_t WifiWrapper::get_sta_status() {
    return _sta_st;
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

esp_err_t WifiWrapper::wifi_init() {
    esp_err_t st;
    st = esp_netif_init();
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Error while netif init: %s", esp_err_to_name(st));
        return st;
    }

    st = esp_event_loop_create_default();
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Error while event loop init: %s", esp_err_to_name(st));
        return st;
    }
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    st = esp_wifi_init(&cfg);
    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Error while WIFI init: %s", esp_err_to_name(st));
        return st;
    }

    st = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL
    );

    st = esp_event_handler_instance_register(
        IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL
    );

    if(ESP_OK != st) {
        ESP_LOGE(TAG, "Error while event handler registration: %s", esp_err_to_name(st));
        return st;
    }

    return ESP_OK;
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
    } else if (event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "STA started. Connecting to AP...");
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnect event. Connecting to AP...");
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
        //s_retry_num = 0;
        //xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}