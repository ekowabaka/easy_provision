#include <sys/param.h>
#include <esp_http_server.h>
#include <dirent.h>

#include "private/jsportal.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "cJSON.h"

#include "easy_provision.h"

static const char *TAG = "JSPORTAL";

/**
 * @brief Web server handle
 */
static httpd_handle_t server = NULL;

esp_err_t api_status_get_handler(httpd_req_t *req)
{
    char buffer[128];

    httpd_resp_set_type(req, "application/json");
    sprintf(buffer, "{\"status\": %d}", wifi_get_connection_status());
    httpd_resp_send(req, buffer, strlen(buffer));

    ESP_LOGI(TAG, "Sending status: %s", buffer);

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
    char buffer[128];
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    int ap_count = wifi_scan(ap_info);    

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send_chunk(req, "[", 1);
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        sprintf(
            buffer, "{\"ssid\":\"%s\",\"rssi\":%d, \"auth\": %d}", 
            ap_info[i].ssid, ap_info[i].rssi, ap_info[i].authmode
        );
        httpd_resp_send_chunk(req, buffer, strlen(buffer));
        if (i != (ap_count - 1)) {
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
 * @brief Request handler for serving files endpoint. 
 * 
 * All files served must be prefixed with 'portal_'. In the special case of the '/' endpoint, the 'portal_index.html' 
 * file is served. 
 * 
 * @param req 
 * @return esp_err_t 
 */
esp_err_t file_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Attempting to serve file for request %s", req->uri);
    char filename[32];
    strcpy(filename, "/spiffs");
    strcat(filename, strcmp(req->uri, "/") == 0 ? "/portal_index.html" : req->uri);
    char * extension = strrchr(filename, '.') + 1;
    if(extension) {
        if(strcmp(extension, "html") == 0) {
            httpd_resp_set_type(req, "text/html");
        } else if(strcmp(extension, "css") == 0) {
            httpd_resp_set_type(req, "text/css");
        } else if(strcmp(extension, "js") == 0) {
            httpd_resp_set_type(req, "application/javascript");
        } else if(strcmp(extension, "svg") == 0) {
            httpd_resp_set_type(req, "image/svg+xml");
        } else if(strcmp(extension, "ico") == 0) {
            httpd_resp_set_type(req, "image/x-icon");
        }
    }
    FILE * f = fopen(filename, "r");
    if (f == NULL) {
        return ESP_FAIL;
    }  
    char buffer[100] = {0};
    while(!feof(f)) {
        size_t len = fread(buffer, 1, 100, f);
        httpd_resp_send_chunk(req, buffer, len);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
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
    char * buf = malloc(buf_len);
    size_t received = 0;
    char * failure = "false";
    char * success = "true";

    // Download all the json content that was sent in post
    while(received < req->content_len) {
        size_t ret = httpd_req_recv(req, buf + received, buf_len - received);
        if(ret <= 0 && ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
            httpd_resp_send(req, failure, strlen(failure));
            free(buf);
            return ESP_FAIL;
        }
        received += ret;
    }

    ESP_LOGI(TAG, "Received POST request %s", buf);
    cJSON *root = cJSON_Parse(buf);
    if(root == NULL) {
        ESP_LOGE(TAG, "Error parsing JSON");
        httpd_resp_send_500(req);
        httpd_resp_send(req, failure, strlen(failure));
        free(buf);
        return ESP_FAIL;
    }

    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "password");
    if(!ssid || !password) {
        ESP_LOGE(TAG, "Error parsing JSON");
        httpd_resp_send_500(req);
        httpd_resp_send(req, failure, strlen(failure));
        cJSON_Delete(root);
        free(buf);
        return ESP_FAIL;
    }
    if(wifi_start_station(ssid->valuestring, password->valuestring) == ESP_OK) {
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_send(req, success, strlen(success));
        stop_provisioning();
    } else {
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
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        //register_file_urls(server);
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/api/scan",
            .method = HTTP_GET,
            .handler = api_scan_get_handler,
            .user_ctx = NULL
        });
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/api/connect",
            .method = HTTP_POST,
            .handler = api_connect_post_handler,
            .user_ctx = NULL
        });
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/api/status",
            .method = HTTP_GET,
            .handler = api_status_get_handler,
            .user_ctx = NULL
        });
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/",
            .method = HTTP_GET,
            .handler = file_get_handler,
            .user_ctx = NULL
        });
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/portal_*",
            .method = HTTP_GET,
            .handler = file_get_handler,
            .user_ctx = NULL
        });
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = redirect_handler,
            .user_ctx = NULL
        });        
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

