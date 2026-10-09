#include <esp_log.h>
#include <sys/param.h>
#include <esp_err.h>
#include <esp_check.h>

#include "http_server.h"

#define TAG "http_server"

QueueHandle_t serverQueue;

#pragma region // Index page
extern const char index_start[] asm("_binary_index_html_start");
extern const char index_end[] asm("_binary_index_html_end");

static esp_err_t index_get_handler(httpd_req_t *req)
{
    const uint64_t index_len = index_end - index_start;

    ESP_LOGI(TAG, "serving index page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_start, index_len);

    return ESP_OK;
}

static const httpd_uri_t index_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_get_handler};

#pragma endregion // Index page

#pragma region // All post requests
static esp_err_t all_post_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len = req->content_len + 1;

    int total_len = req->content_len;
    int cur_len = 0, received = 0;

    ESP_LOGI(TAG, "Post request received. buf_len = %i", buf_len);
    if (buf_len > 1)
    {
        buf = malloc(1 + buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");

        while (cur_len < total_len)
        {
            received = httpd_req_recv(req, buf + cur_len, total_len);
            if (received <= 0)
            {
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
                return ESP_FAIL;
            }
            cur_len += received;
        }
        buf[req->content_len] = '\0';

        /* Log data received */
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%s", buf);
        ESP_LOGI(TAG, "====================================");

        if (xQueueSend(serverQueue, (void *)&buf, 0) == pdFAIL)
        {
            ESP_LOGI(TAG, "Failed to post message to server queue");
        }
    }

    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t all_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = all_post_handler,
};

#pragma endregion // All post requests

#pragma region // 404 Redirects to home
esp_err_t home_redirect_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "303 See Other");
    // Redirect all requests
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting...\nrequest url: %s, err: %i", req->uri, err);
    return ESP_OK;
}
#pragma endregion


httpd_handle_t start_webserver()
{
    httpd_handle_t server = NULL;
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", cfg.server_port);

    esp_err_t err = httpd_start(&server, &cfg);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start server!.\n%s", esp_err_to_name(err));
    }

    // initialize queue for storing data sent in requests and throw if it doesn't work
    assert((serverQueue = xQueueCreate(10, sizeof(char *))) != NULL);

    ESP_LOGI(TAG, "http server started successfully!");
    return server;
}



void register_uris(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers for station mode.");
    httpd_register_uri_handler(server, &index_uri);
    httpd_register_uri_handler(server, &all_post);
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, home_redirect_handler);
}