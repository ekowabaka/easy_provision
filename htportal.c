#include "private/htportal.h"
#include <esp_http_server.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "private/internal.h"
#include <math.h>
#include <cJSON.h>
//#include "private/mustach-cjson.h"

static const char *TAG = "HTPORTAL";

/**
 * Content strings for the various pages
 */
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t connected_html_start[] asm("_binary_connected_html_start");
extern const uint8_t connected_html_end[] asm("_binary_connected_html_end");
extern const uint8_t footer_html_start[] asm("_binary_footer_html_start");
extern const uint8_t footer_html_end[] asm("_binary_footer_html_end");
extern const uint8_t header_html_start[] asm("_binary_header_html_start");
extern const uint8_t header_html_end[] asm("_binary_header_html_end");
extern const uint8_t manual_html_start[] asm("_binary_manual_html_start");
extern const uint8_t manual_html_end[] asm("_binary_manual_html_end");
extern const uint8_t network_html_start[] asm("_binary_network_html_start");
extern const uint8_t network_html_end[] asm("_binary_network_html_end");
extern const uint8_t password_html_start[] asm("_binary_password_html_start");
extern const uint8_t password_html_end[] asm("_binary_password_html_end");
extern const uint8_t scan_head_html_start[] asm("_binary_scan_head_html_start");
extern const uint8_t scan_head_html_end[] asm("_binary_scan_head_html_end");
extern const uint8_t scan_foot_html_start[] asm("_binary_scan_foot_html_start");
extern const uint8_t scan_foot_html_end[] asm("_binary_scan_foot_html_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_style_css_end");
extern const uint8_t auth_lock_png_start[] asm("_binary_auth_lock_png_start");
extern const uint8_t auth_lock_png_end[] asm("_binary_auth_lock_png_end");
extern const uint8_t auth_open_png_start[] asm("_binary_auth_open_png_start");
extern const uint8_t auth_open_png_end[] asm("_binary_auth_open_png_end");

extern const uint8_t sig_0_png_start[] asm("_binary_sig_0_png_start");
extern const uint8_t sig_0_png_end[] asm("_binary_sig_0_png_end");
extern const uint8_t sig_1_png_start[] asm("_binary_sig_1_png_start");
extern const uint8_t sig_1_png_end[] asm("_binary_sig_1_png_end");
extern const uint8_t sig_2_png_start[] asm("_binary_sig_2_png_start");
extern const uint8_t sig_2_png_end[] asm("_binary_sig_2_png_end");
extern const uint8_t sig_3_png_start[] asm("_binary_sig_3_png_start");
extern const uint8_t sig_3_png_end[] asm("_binary_sig_3_png_end");
extern const uint8_t sig_4_png_start[] asm("_binary_sig_4_png_start");
extern const uint8_t sig_4_png_end[] asm("_binary_sig_4_png_end");


/**
 * @brief Web server handle
 */
static httpd_handle_t server = NULL;

/**
 * @brief Stream the page header to the client.
 * @param req
 */
void stream_page_head(httpd_req_t *req)
{
    httpd_resp_send_chunk(req, (const char *)header_html_start, header_html_end - header_html_start);
}

/**
 * @brief Stream the page footer to the client.
 * @param req
 */
void stream_page_foot(httpd_req_t *req)
{
    httpd_resp_send_chunk(req, (const char *)footer_html_start, footer_html_end - footer_html_start);
}

/**
 * @brief Redirect any unknown requests to the index page.
 * This is used to force clients to display a captive portal popup.
 *
 * @param req
 * @return esp_err_t
 */
esp_err_t redirect_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Redirecting %s", req->uri);
    httpd_resp_set_status(req, "301 Moved Permanently");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/**
 * @brief Make a connection.
 *
 * @param req
 * @param ssid
 * @param password
 */
void make_connection(httpd_req_t *req, char *ssid, char *password)
{
    ESP_LOGI(TAG, "Attempting to connect to %s", ssid);
    if (wifi_start_connecting(ssid, password) == ESP_OK)
    {
        int connection_status = wifi_get_connection_status();
        while (connection_status == CONNECTION_STATUS_CONNECTING || connection_status == CONNECTION_STATUS_WAITING)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            connection_status = wifi_get_connection_status();
        }
        ESP_LOGI(TAG, "Wifi Connected");
    }

    if (wifi_get_connection_status() == CONNECTION_STATUS_CONNECTED)
    {
        char buffer[256];
        stream_page_head(req);
        sprintf(buffer, (const char *)connected_html_start, ssid);
        httpd_resp_send_chunk(req, buffer, strlen(buffer));
        stop_provisioning();
        stream_page_foot(req);
    }
    else
    {
        char url[64];
        size_t query_len = httpd_req_get_url_query_len(req);
        char *query_str = calloc(1, query_len + 1);
        char redirect[16];

        httpd_req_get_url_query_str(req, query_str, query_len + 1);
        esp_err_t error = httpd_query_key_value(query_str, "redirect", redirect, sizeof(redirect));
        if (error == ESP_ERR_NOT_FOUND)
        {
            sprintf(url, "./?ssid=%s&e=pw", ssid);
        }
        else
        {
            sprintf(url, "/%s?ssid=%s&e=pw", redirect, ssid);
        }
        httpd_resp_set_status(req, "303 See Other");
        httpd_resp_set_hdr(req, "Location", url);

        free(query_str);
    }

    httpd_resp_send_chunk(req, NULL, 0);
}

esp_err_t index_get_handler(httpd_req_t *req)
{
    char buffer[512];
    stream_page_head(req);

    sprintf(buffer, (const char *) index_html_start, "LED Matrix");
    httpd_resp_send_chunk(req, buffer, strlen(buffer));

    stream_page_foot(req);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t style_css_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_css_start, style_css_end - style_css_start);
    return ESP_OK;
}

esp_err_t png_image_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/png");
    if (strcmp(req->uri, "/auth-lock.png") == 0)
    {
        httpd_resp_send(req, (const char *)auth_lock_png_start, auth_lock_png_end - auth_lock_png_start);
    }
    else if (strcmp(req->uri, "/auth-open.png") == 0)
    {
        httpd_resp_send(req, (const char *)auth_open_png_start, auth_open_png_end - auth_open_png_start);
    }
    else if (strcmp(req->uri, "/sig-0.png") == 0)
    {
        httpd_resp_send(req, (const char *)sig_0_png_start, sig_0_png_end - sig_0_png_start);
    }
    else if (strcmp(req->uri, "/sig-1.png") == 0)
    {
        httpd_resp_send(req, (const char *)sig_1_png_start, sig_1_png_end - sig_1_png_start);    
    }
    else if (strcmp(req->uri, "/sig-2.png") == 0)
    {
        httpd_resp_send(req, (const char *)sig_2_png_start, sig_2_png_end - sig_2_png_start);
    }
    else if (strcmp(req->uri, "/sig-3.png") == 0)
    {
        httpd_resp_send(req, (const char *)sig_3_png_start, sig_3_png_end - sig_3_png_start);
    }
    else if (strcmp(req->uri, "/sig-4.png") == 0)
    {
        httpd_resp_send(req, (const char *)sig_4_png_start, sig_4_png_end - sig_4_png_start);
    }
    return ESP_OK;
}

esp_err_t manual_get_handler(httpd_req_t *req)
{
    size_t query_len = httpd_req_get_url_query_len(req);
    char *query_str = calloc(1, query_len + 1);
    char pw[3];
    esp_err_t key_found = httpd_query_key_value(query_str, "e", pw, 3);

    // const char * vars[] = {"%%error%%"};
    // const char * values[] = {key_found == ESP_OK ? "Failed to connect. Please try again.": ""};

    stream_page_head(req);
    // render_and_stream_content("/spiffs/manual.html", req, vars, values, 1);
    httpd_resp_send_chunk(req, (const char *)manual_html_start, manual_html_end - manual_html_start);
    stream_page_foot(req);
    httpd_resp_send_chunk(req, NULL, 0);

    free(query_str);
    return ESP_OK;
}

esp_err_t scan_get_handler(httpd_req_t *req)
{
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    int ap_count = wifi_scan(ap_info);
    stream_page_head(req);
    httpd_resp_send_chunk(req, (const char *)scan_head_html_start, scan_head_html_end - scan_head_html_start);

    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++)
    {
        int rssi_level = round(((ap_info[i].rssi + 100.0) / 50.0) * 4.0);
        rssi_level = rssi_level > 4 ? 4 : rssi_level;
        rssi_level = rssi_level < 0 ? 0 : rssi_level;
        char srssi[4];
        char buffer[512];
        char *auth = ap_info[i].authmode == WIFI_AUTH_OPEN ? "open" : "lock";

        sprintf(srssi, "%d", rssi_level);
        sprintf(buffer, (const char *)network_html_start, auth, (const char *) ap_info[i].ssid, (const char *)ap_info[i].ssid, auth, srssi);
        httpd_resp_send_chunk(req, buffer, strlen(buffer));
    }

    httpd_resp_send_chunk(req, (const char *)scan_foot_html_start, scan_foot_html_end - scan_foot_html_start);
    stream_page_foot(req);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t connect_open_get_handler(httpd_req_t *req)
{
    size_t query_len = httpd_req_get_url_query_len(req);
    char *query_str = calloc(1, query_len + 1);
    httpd_req_get_url_query_str(req, query_str, query_len + 1);
    char ssid[33];
    httpd_query_key_value(query_str, "ssid", ssid, sizeof(ssid));

    ESP_LOGI(TAG, "Connecting to %s", ssid);
    make_connection(req, ssid, "");

    free(query_str);
    return ESP_OK;
}

esp_err_t connect_lock_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Attempting to connect to %s", req->uri);
    size_t query_len = httpd_req_get_url_query_len(req);

    if (query_len == 0)
    {
        return ESP_FAIL;
    }

    char *query_str = calloc(1, query_len + 1);
    httpd_req_get_url_query_str(req, query_str, query_len + 1);

    char ssid[32];
    httpd_query_key_value(query_str, "ssid", ssid, sizeof(ssid));

    stream_page_head(req);

    char pw[3];
    esp_err_t key_found = httpd_query_key_value(query_str, "e", pw, 3);

    char buffer[512];
    sprintf(buffer, (const char *)password_html_start, ssid, key_found == ESP_OK ? "Failed to connect. Please try again." : "", ssid);
    httpd_resp_send_chunk(req, buffer, strlen(buffer));

    stream_page_foot(req);
    httpd_resp_send_chunk(req, NULL, 0);

    free(query_str);
    return ESP_OK;
}

esp_err_t connect_post_handler(httpd_req_t *req)
{
    size_t buf_len = req->content_len + 1;
    char *buf = malloc(buf_len);
    size_t received = 0;

    // Download all the HTTP content
    while (received < req->content_len)
    {
        size_t ret = httpd_req_recv(req, buf + received, buf_len - received);
        if (ret <= 0 && ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
            free(buf);
            return ESP_FAIL;
        }
        received += ret;
    }

    char ssid[32];
    char password[64];

    httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
    httpd_query_key_value(buf, "password", password, sizeof(password));

    make_connection(req, ssid, password);
    free(buf);
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
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/",
                                               .method = HTTP_GET,
                                               .handler = index_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/scan",
                                               .method = HTTP_GET,
                                               .handler = scan_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/manual",
                                               .method = HTTP_GET,
                                               .handler = manual_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/connect",
                                               .method = HTTP_POST,
                                               .handler = connect_post_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/connect_lock",
                                               .method = HTTP_GET,
                                               .handler = connect_lock_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/connect_open",
                                               .method = HTTP_GET,
                                               .handler = connect_open_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/style.css",
                                               .method = HTTP_GET,
                                               .handler = style_css_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/auth-lock.png",
                                               .method = HTTP_GET,
                                               .handler = png_image_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/auth-open.png",
                                               .method = HTTP_GET,
                                               .handler = png_image_get_handler,
                                               .user_ctx = NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){
                                               .uri = "/sig-*",
                                               .method = HTTP_GET,
                                               .handler = png_image_get_handler,
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
