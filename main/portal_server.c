#include <sys/param.h>
#include <esp_http_server.h>
#include <dirent.h>

#include "portal_server.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"

#include "cJSON.h"

static const char *TAG = "UI-Server";

/**
 * @brief Web server handle
 */
static httpd_handle_t server = NULL;

/**
 * @brief Request handler for the ['/api/scan'] endpoint.
 * 
 * @param req 
 * @return esp_err_t 
 */
esp_err_t api_scan_get_handler(httpd_req_t *req)
{
    char buffer[128];
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;

    memset(ap_info, 0, sizeof(ap_info));    
    esp_wifi_scan_start(NULL, true);
    esp_wifi_scan_get_ap_records(&number, ap_info);
    esp_wifi_scan_get_ap_num(&ap_count);
    
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



// /**
//  * @brief Register individual request handlers for all files prefixed with 'portal_' in the SPIFFS.
//  * 
//  * A wildcard request handler could have been used, but it seems the ESP8266 version of the ESP-IDF library does not 
//  * have this implemented.
//  * 
//  * @param server 
//  */
// void register_file_urls(httpd_handle_t server) 
// {
//     DIR *dir_handle = opendir("/spiffs");
//     struct dirent *dir;
//     char * prefix = malloc(8);

//     if(dir_handle != NULL) {
//         while((dir = readdir(dir_handle)) != NULL) {
            
//             if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
//                 continue;
//             }
//             strncpy(prefix, dir->d_name, 7);
//             prefix[7] = '\0';
//             ESP_LOGI(TAG, "Prefixes %s %s", prefix, dir->d_name);
//             if(strcmp(prefix, "portal_") != 0) {
//                 continue;
//             }

//             char * uri = malloc(strlen(dir->d_name) + 2);
//             strcpy(uri, "/");
//             if(strcmp("portal_index.html", dir->d_name) != 0) {
//                 strcat(uri, dir->d_name);
//             } 
//             ESP_LOGI(TAG, "Registering URI handler for %s with %s", uri, dir->d_name);
//             httpd_uri_t * route  = malloc(sizeof(httpd_uri_t));
//             route->uri = uri;
//             route->method = HTTP_GET;
//             route->handler = file_get_handler;
//             route->user_ctx = NULL;            
//             httpd_register_uri_handler(server, route);
//         }
//     }
//     free(prefix);
// }

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

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) 
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void run_ui()
{
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    server = start_webserver();
}

