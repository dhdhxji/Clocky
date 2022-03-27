#ifndef WIFI_HANDLING_H
#define WIFI_HANDLING_H

#include "cfg.hpp"
#include "wifi_wrapper.hpp"
#include "esp_system.h"
#include <string>


#define AP_SSID_DEFAULT "Clocky" 
#define AP_PASS_DEFAULT ""

using namespace std::literals;


/**
 * @brief Wifi module should behave in the next way: when sta mode is 
 * configured, it should connect to the AP, but if connection is 
 * unsuccessfull (it can not connect fo 10 times) or ap mode configured, 
 * it should start in ap mode.
 * 
 * @param cfg 
 */
void init_wifi_handler(Cfg& cfg) {
    WifiWrapper* w = WifiWrapper::getInstanse();
    bool apMode = cfg.get("wifi.isAP", true);
    if(apMode) {
        std::string ssid = cfg.get("wifi.ap.ssid", "Clocky"s);
        std::string pass = cfg.get("wifi.ap.pass", ""s);
        w->ap_start(ssid, pass);
    } else {
        try {
            std::string ssid = cfg.get<std::string>("wifi.sta.ssid");
            std::string pass = cfg.get<std::string>("wifi.sta.pass");
            w->sta_connect(ssid, pass, 10, true);
            w->add_sta_connection_handler([](WifiWrapper* w, bool connected){

            });
        } catch(...) {
            cfg.put("wifi.isAP", true);
            cfg.save();
            esp_restart();
        }
    }
}

#endif // WIFI_HANDLING_H
