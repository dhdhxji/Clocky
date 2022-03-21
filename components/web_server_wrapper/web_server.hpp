#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <string>
#include <vector>
#include <functional>

#include "esp_http_server.h"

class WebServer {
public:
    WebServer(uint16_t port);
    ~WebServer();

    void addUriHandler(
        const std::string& uri, 
        httpd_method_t method, 
        std::function<esp_err_t(httpd_req_t*)> cb,
        void* ctx
    );

protected:
    httpd_handle_t httpd;
    std::vector<void*> userContexts;
};

#endif // WEB_SERVER_H
