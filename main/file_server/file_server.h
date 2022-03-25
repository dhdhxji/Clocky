#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

typedef struct {
    esp_err_t (*index_get_handler)(httpd_req_t*);
    esp_err_t (*favicon_get_handler)(httpd_req_t*);
    esp_err_t (*file_get_get_handler)(httpd_req_t*);
    esp_err_t (*file_put_post_handler)(httpd_req_t*);
    esp_err_t (*delete_post_handler)(httpd_req_t*);
    void* ctx;
} file_server_t;

esp_err_t build_file_server(const char* base_path, file_server_t* fs_data);

#endif // FILE_SERVER_H
