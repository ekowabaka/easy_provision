#include <sys/param.h>
#include <esp_http_server.h>
#include <dirent.h>

#include "private/jsportal.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "cJSON.h"

#include "private/internal.h"

static const char *TAG = "JSPORTAL";

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t script_js_start[] asm("_binary_script_js_start");
extern const uint8_t select_html_start[] asm("_binary_select_html_start");
extern const uint8_t manual_html_start[] asm("_binary_manual_html_start");

extern const uint8_t sig_0_svg_start[] asm("_binary_sig_0_svg_start");
extern const uint8_t sig_1_svg_start[] asm("_binary_sig_1_svg_start");
extern const uint8_t sig_2_svg_start[] asm("_binary_sig_2_svg_start");
extern const uint8_t sig_3_svg_start[] asm("_binary_sig_3_svg_start");
extern const uint8_t sig_4_svg_start[] asm("_binary_sig_4_svg_start");

extern const uint8_t sync_svg_start[] asm("_binary_sync_svg_start");
extern const uint8_t lock_svg_start[] asm("_binary_lock_svg_start");

/**
 * @brief Web server handle
 */
static httpd_handle_t server = NULL;

esp_err_t index_get_handler(httpd_req_t *req)
{
    char buffer[1024];
    sprintf(buffer, (const char *)index_html_start, "LED Matrix");
    httpd_resp_send(req, buffer, strlen(buffer));
    return ESP_OK;
}

esp_err_t select_get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, (const char *)select_html_start, strlen((const char *)select_html_start));
    return ESP_OK;
}

esp_err_t manual_get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, (const char *)manual_html_start, strlen((const char *)manual_html_start));
    return ESP_OK;
}

esp_err_t svg_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/svg+xml");
    ESP_LOGI(TAG, "Sending svg file: %s", req->uri);
    if (strcmp(req->uri, "/sig_0.svg") == 0)
    {
        httpd_resp_send(req, (const char *)sig_0_svg_start, strlen((const char *)sig_0_svg_start));
    }
    else if (strcmp(req->uri, "/sig_1.svg") == 0)
    {
        httpd_resp_send(req, (const char *)sig_1_svg_start, strlen((const char *)sig_1_svg_start));
    }
    else if (strcmp(req->uri, "/sig_2.svg") == 0)
    {
        httpd_resp_send(req, (const char *)sig_2_svg_start, strlen((const char *)sig_2_svg_start));
    }
    else if (strcmp(req->uri, "/sig_3.svg") == 0)
    {
        httpd_resp_send(req, (const char *)sig_3_svg_start, strlen((const char *)sig_3_svg_start));
    }
    else if (strcmp(req->uri, "/sig_4.svg") == 0)
    {
        httpd_resp_send(req, (const char *)sig_4_svg_start, strlen((const char *)sig_4_svg_start));
    }
    else if (strcmp(req->uri, "/sync.svg") == 0)
    {
        httpd_resp_send(req, (const char *)sync_svg_start, strlen((const char *)sync_svg_start));
    }
    else if (strcmp(req->uri, "/lock.svg") == 0)
    {
        httpd_resp_send(req, (const char *)lock_svg_start, strlen((const char *)lock_svg_start));
    }
    return ESP_OK;
}

esp_err_t style_css_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_css_start, strlen((const char *)style_css_start));
    return ESP_OK;
}

esp_err_t script_js_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_send(req, (const char *)script_js_start, strlen((const char *)script_js_start));
    return ESP_OK;
}

esp_err_t api_status_get_handler(httpd_req_t *req)
{
    char buffer[128];
    int connection_status = wifi_get_connection_status();

    httpd_resp_set_type(req, "application/json");
    sprintf(buffer, "{\"status\": %d}", connection_status);
    httpd_resp_send(req, buffer, strlen(buffer));
    ESP_LOGI(TAG, "Sending status: %s", buffer);

    if (connection_status == CONNECTION_STATUS_CONNECTED)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        stop_provisioning();
    }

    return ESP_OK;
}

/**
 * @brief Request handler for the ['/api/scan'] endpoint.
 *
 * @param req
 * @return esp_err_t
 */
esp_err_t api_scan_get_handler(httpd_req_t *req)
{
    char buffer[258];
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    int ap_count = wifi_scan(ap_info);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send_chunk(req, "[", 1);
    for (int i = 0; i < ap_count; i++)
    {
        sprintf(
            buffer, "{\"ssid\":\"%s\",\"rssi\":%d, \"auth\": %d}",
            ap_info[i].ssid, ap_info[i].rssi, ap_info[i].authmode);
        httpd_resp_send_chunk(req, buffer, strlen(buffer));
        if (i != (ap_count - 1))
        {
            httpd_resp_send_chunk(req, ",", 1);
        }
    }
    httpd_resp_send_chunk(req, "]", 1);
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

esp_err_t redirect_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Redirecting %s", req->uri);
    httpd_resp_set_status(req, "301 Moved Permanently");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/**
 * @brief Request handler for the ['/api/connect'] endpoint.
 *
 * @param req
 * @return esp_err_t
 */
esp_err_t api_connect_post_handler(httpd_req_t *req)
{
    size_t buf_len = req->content_len + 1;
    char *buf = malloc(buf_len);
    size_t received = 0;
    char *failure = "false";
    char *success = "true";

    // Download all the json content that was sent in post
    while (received < req->content_len)
    {
        size_t ret = httpd_req_recv(req, buf + received, buf_len - received);
        if (ret <= 0 && ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
            httpd_resp_send(req, failure, strlen(failure));
            free(buf);
            return ESP_FAIL;
        }
        received += ret;
    }

    ESP_LOGI(TAG, "Received POST request %s", buf);
    cJSON *root = cJSON_Parse(buf);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Error parsing JSON");
        httpd_resp_send_500(req);
        httpd_resp_send(req, failure, strlen(failure));
        free(buf);
        return ESP_FAIL;
    }

    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "password");
    if (!ssid || !password)
    {
        ESP_LOGE(TAG, "Error parsing JSON");
        httpd_resp_send_500(req);
        httpd_resp_send(req, failure, strlen(failure));
        cJSON_Delete(root);
        free(buf);
        return ESP_FAIL;
    }
    if (wifi_start_connecting(ssid->valuestring, password->valuestring) == ESP_OK)
    {
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_send(req, success, strlen(success));
    }
    else
    {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, failure, strlen(failure));
    }
    free(buf);
    cJSON_Delete(root);

    return ESP_OK;
}

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "Registering URI handlers");
        // register_file_urls(server);
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/api/scan",
                                               .method = HTTP_GET,
                                               .handler = api_scan_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/api/connect",
                                               .method = HTTP_POST,
                                               .handler = api_connect_post_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/api/status",
                                               .method = HTTP_GET,
                                               .handler = api_status_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/",
                                               .method = HTTP_GET,
                                               .handler = index_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/style.css",
                                               .method = HTTP_GET,
                                               .handler = style_css_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/script.js",
                                               .method = HTTP_GET,
                                               .handler = script_js_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/select",
                                               .method = HTTP_GET,
                                               .handler = select_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/manual",
                                               .method = HTTP_GET,
                                               .handler = manual_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/lock.svg",
                                               .method = HTTP_GET,
                                               .handler = svg_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/sync.svg",
                                               .method = HTTP_GET,
                                               .handler = svg_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/sig*",
                                               .method = HTTP_GET,
                                               .handler = svg_get_handler,
                                               .user_ctx = NULL});

        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/*",
                                               .method = HTTP_GET,
                                               .handler = redirect_handler,
                                               .user_ctx = NULL});
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}

void start_portal()
{
    server = start_webserver();
}
