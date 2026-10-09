#pragma once

#include <esp_http_server.h>

#define MAX_CONTENT_LEN 128

httpd_handle_t start_webserver();
void register_uris(httpd_handle_t server);