#ifndef WIFI_HANDLING_H
#define WIFI_HANDLING_H

#include "cfg.hpp"
#include "wifi_wrapper.hpp"
#include "event_loop.hpp"
#include "esp_system.h"
#include "esp_log.h"
#include <string>

#define TAG "WIFI_Handler"

#define AP_SSID_DEFAULT "Clocky"s
#define AP_PASS_DEFAULT ""s

#define STA_RECONN_COUNT_DEFAULT 10

using namespace std::literals;

DECLARE_BASE_ID(TESTID);


DEFINE_BASE_ID(APP_WIFI_EVT);
#define WIFI_START_EVT          0
#define WIFI_STOP_EVT           1
#define WIFI_CONF_CHANGED_EVT   2
#define WIFI_STA_START_EVT      3
#define WIFI_STA_FAILED_EVT     4
#define WIFI_AP_START_EVT       5
#define WIFI_CFG_UPDATED_EVT    6

/**
 * @brief Wifi module should behave in the next way: when sta mode is 
 * configured, it should connect to the AP, but if connection is 
 * unsuccessfull (it can not connect fo 10 times) or ap mode configured, 
 * it should start in ap mode.
 * 
 * @param cfg 
 */
void init_wifi_handler(Cfg& cfg, EventLoop& ev) {
    WifiWrapper* w = WifiWrapper::getInstanse();
    w->add_sta_connection_handler([&ev](WifiWrapper* w, bool isConnected){
        if(!isConnected) {
            w->sta_disconnect();
            ev.post(APP_WIFI_EVT, WIFI_STA_FAILED_EVT, nullptr, 0);
        }
    });

    ev.handlerRegister(APP_WIFI_EVT, WIFI_START_EVT, [&cfg](EventLoop& ev, event_base_t, int32_t, void*){
        ESP_LOGD(TAG, "Start event");

        bool apMode = cfg.get("wifi.isAP", true); 
        int32_t targetEvt = (apMode) ? WIFI_AP_START_EVT:WIFI_STA_START_EVT;

        ESP_LOGI(TAG, "Starting AP mode");
        ev.post(APP_WIFI_EVT, WIFI_STOP_EVT, (void*)&targetEvt, 4);
    });

    ev.handlerRegister(APP_WIFI_EVT, WIFI_STOP_EVT, [](EventLoop& ev, event_base_t, int32_t, void* nextEvent){
        if(nextEvent) {
            ESP_LOGD(TAG, "Stop event with next event chained: %d", *(int32_t*)nextEvent);
        } else {
            ESP_LOGD(TAG, "Stop event without next event chained");
        }

        WifiWrapper* w = WifiWrapper::getInstanse();
        if(w->get_sta_status() != WifiWrapper::DISABLED) {w->sta_disconnect();}
        if(w->get_ap_status() != WifiWrapper::DISABLED) {w->ap_stop();}

        if(nextEvent) {
            ev.post(APP_WIFI_EVT, *(int32_t*)nextEvent, nullptr, 0);
        }
    });

    ev.handlerRegister(APP_WIFI_EVT, WIFI_STA_START_EVT, [&cfg](EventLoop& ev, event_base_t, int32_t, void*){
        ESP_LOGD(TAG, "STA Start event");
        WifiWrapper* w = WifiWrapper::getInstanse();
        std::string ssid;
        std::string pass;
        int reconnect_count = cfg.get<int>("wifi.sta.reconnect_count", STA_RECONN_COUNT_DEFAULT);
        try {
            ssid = cfg.get<std::string>("wifi.sta.ssid");
            pass = cfg.get<std::string>("wifi.sta.pass");
        } catch(...) {
            cfg.put("wifi.isAP", true);
            cfg.save();

            // Restart wifi flow
            ev.post(APP_WIFI_EVT, WIFI_START_EVT, nullptr, 0);
        }

        ESP_LOGI(TAG, "Connecting to %s", ssid.c_str());
        w->sta_connect(ssid, pass, reconnect_count, true);
    });

    ev.handlerRegister(APP_WIFI_EVT, WIFI_STA_FAILED_EVT, [&cfg](EventLoop& ev, event_base_t, int32_t, void*){
        ESP_LOGI(TAG, "STA connection failed");
        int32_t target_evt = WIFI_AP_START_EVT;
        ev.post(APP_WIFI_EVT, WIFI_STOP_EVT, (void*)&target_evt, 4); //Call stop, then ap start
    });

    ev.handlerRegister(APP_WIFI_EVT, WIFI_AP_START_EVT, [&cfg](EventLoop& ev, event_base_t, int32_t, void*){
        WifiWrapper* w = WifiWrapper::getInstanse();

        std::string ssid = cfg.get("wifi.ap.ssid", AP_SSID_DEFAULT);
        std::string pass = cfg.get("wifi.ap.pass", AP_PASS_DEFAULT);

        ESP_LOGI(TAG, "Starting AP %s", ssid.c_str());
        w->ap_start(ssid, pass);
    });

    ev.handlerRegister(APP_WIFI_EVT, WIFI_CFG_UPDATED_EVT, [](EventLoop& ev, event_base_t, int32_t, void*){
        ESP_LOGI(TAG, "Wifi configs updated, restarting wifi");
        ev.post(APP_WIFI_EVT, WIFI_START_EVT, nullptr, 0);
    });
}

#undef TAG

#endif // WIFI_HANDLING_H
