#include "web_server.hpp"
#include "esp_err.h"
#include <stdexcept>

struct user_context_t {
    std::function<esp_err_t(httpd_req_t*)> cb;
    WebServer* server;
    void* ctx;
};

static esp_err_t req_handler(httpd_req_t* req) {
    user_context_t* user_ctx = (user_context_t*)req->user_ctx;
    req->user_ctx = user_ctx->ctx;

    return user_ctx->cb(req);
}

WebServer::WebServer(uint16_t port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.global_user_ctx = this;
    config.global_transport_ctx_free_fn = nullptr;
    config.uri_match_fn = httpd_uri_match_wildcard;
    
    esp_err_t st = httpd_start(&httpd, &config);
    if(ESP_OK != st) {
        throw std::runtime_error(
            std::string("Unable to start web server: ") + esp_err_to_name(st)
        );
    }
}

WebServer::~WebServer() {
    httpd_stop(&httpd);

    for(auto ctx: userContexts) {
        delete (user_context_t*)ctx;
    }
}

void WebServer::addUriHandler(
    const std::string& uri, 
    httpd_method_t method,
    std::function<esp_err_t(httpd_req_t*)> cb,
    void* ctx
) {
    user_context_t* user_ctx = new user_context_t;
    user_ctx->cb = cb;
    user_ctx->ctx = ctx;
    user_ctx->server = this;

    userContexts.push_back(user_ctx);

    httpd_uri_t uri_cfg = {
        .uri       = uri.c_str(),
        .method    = method,
        .handler   = req_handler,
        .user_ctx  = user_ctx
    };

    httpd_register_uri_handler(&httpd, &uri_cfg);
}