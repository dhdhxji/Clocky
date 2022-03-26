#include <iostream>
#include <exception>
#include <string>
#include "colproc/runtime/lua_runtime.h"
#include "colproc/variable/variable_callback.h"
#include "esp_system.h"
#include "mount_fs.h"
#include "nvs_flash.h"
#include "web_server.hpp"
#include "canvas_impl.h"
#include "cfg.hpp"
#include "wifi_handling.hpp"

extern "C" {
#include "file_server.h"
}



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
    
    Cfg cfg("config.lua");

    init_wifi_handler(cfg);

    file_server_t file_srv;
    ESP_ERROR_CHECK(build_file_server("/", &file_srv));
    WebServer srv(80);
    srv.addUriHandler("/index.html", HTTP_GET, file_srv.index_get_handler, file_srv.ctx);
    srv.addUriHandler("/favicon.ico", HTTP_GET, file_srv.favicon_get_handler, file_srv.ctx);
    srv.addUriHandler("/upload/*", HTTP_POST, file_srv.file_put_post_handler, file_srv.ctx);
    srv.addUriHandler("/delete/*", HTTP_POST, file_srv.delete_post_handler, file_srv.ctx);
    srv.addUriHandler("/*", HTTP_GET, file_srv.file_get_get_handler, file_srv.ctx);


    CanvasConsole canvas(MATRIX_W_DEFAULT, MATRIX_H_DEFAULT);
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