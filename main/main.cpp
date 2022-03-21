#include <iostream>
#include <exception>
#include <string>
#include "colproc/runtime/lua_runtime.h"
#include "colproc/variable/variable_callback.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "mount_fs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_wrapper.hpp"
#include "web_server.hpp"

extern "C" {
#include "file_server.h"
}

#define WIFI_SSID   ""
#define WIFI_PASS   ""


#define REFRESH_RATE_HZ 30
#define MATRIX_W        19
#define MATRIX_H        7



class CanvasConsole: public Canvas
{
public:
    CanvasConsole(size_t w, size_t h): Canvas(w, h) {
        _canvas = new ColRGB[w*h];

        for(size_t i = 0; i < w*h; ++i) {
            _canvas[i].r = 0;
            _canvas[i].g = 0;
            _canvas[i].b = 0;
        } 
    }

    ~CanvasConsole() override {
        delete[] _canvas;
    }

    void setPix(size_t x, size_t y, ColRGB col) override {
        if(x >= getW() || y >= getH()) {
            throw std::runtime_error(
                std::string("Out of bounds in setPix(") + 
                std::to_string(x) + 
                ";" + 
                std::to_string(y) + 
                ")"
            );
            return;
        }
        size_t index = y*getW() + x;

        _canvas[index] = col;
    }

    ColRGB getPix(size_t x, size_t y) const override {
        if(x >= getW() || y >= getH()) { 
            throw std::runtime_error(
                std::string("Out of bounds in getPix(") + 
                std::to_string(x) + 
                ";" + 
                std::to_string(y) + 
                ")"
            );
            return ColRGB(0,0,0);
        }
        size_t index = y*getW() + x;

        return _canvas[index];
    }

    void display() const override {
        printf("\033[0;0H");
        for(size_t y = 0; y < getH(); ++y) {
            for(size_t x = 0; x < getW(); ++x) {
                ColRGB col = getPix(x, y);
                printf("\033[48;2;%d;%d;%dm ", col.r, col.g, col.b);
                printf("\033[48;2;%d;%d;%dm ", col.r, col.g, col.b);
            }
            printf("\n");
        }
    }

private:
    ColRGB* _canvas;
};


void print_system_info() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Free heap: %d\n", esp_get_free_heap_size());
}

extern "C" void app_main() {
    print_system_info();
    ESP_ERROR_CHECK(init_fs());
    ESP_ERROR_CHECK(nvs_flash_init());
    
    WifiWrapper* w = WifiWrapper::getInstanse();
    w->sta_connect(WIFI_SSID, WIFI_PASS, 5/* , true */);

    file_server_t file_srv;
    ESP_ERROR_CHECK(build_file_server("/", &file_srv));
    WebServer srv(80);
    srv.addUriHandler("/index.html", HTTP_GET, file_srv.index_get_handler, file_srv.ctx);
    srv.addUriHandler("/favicon.ico", HTTP_GET, file_srv.favicon_get_handler, file_srv.ctx);
    srv.addUriHandler("/upload/*", HTTP_POST, file_srv.file_put_post_handler, file_srv.ctx);
    srv.addUriHandler("/delete/*", HTTP_POST, file_srv.delete_post_handler, file_srv.ctx);
    srv.addUriHandler("/*", HTTP_GET, file_srv.file_get_get_handler, file_srv.ctx);


    vTaskDelay(portMAX_DELAY);

    CanvasConsole canvas(MATRIX_W, MATRIX_H);
    try {
        LuaRuntime rt(&canvas, 30);

        rt.getVariableManager().addVariable(
            "hrs",
            new VariableCallback<std::string>([](){
                time_t ttime = time(0);
                struct tm* tm_time = localtime(&ttime);
                return  std::to_string(tm_time->tm_hour);
            })
        );

        rt.getVariableManager().addVariable(
            "mins",
            new VariableCallback<std::string>([](){
                time_t ttime = time(0);
                struct tm* tm_time = localtime(&ttime);
                return  std::to_string(tm_time->tm_min);
            })
        );

        rt.loadScript("../init.lua");

        rt.runRenderLoop();
    } catch(std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    } catch(...) {
        std::cerr << "Unexpected error" << std::endl;
    }
}