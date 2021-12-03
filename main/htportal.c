#include "htportal.h"
#include <esp_http_server.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "wifi_main.h"
#include <math.h>

static const char *TAG = "HTPORTAL";

/**
 * @brief Web server handle
 */
static httpd_handle_t server = NULL;

/**
 * @brief Stream the contents of a file to the client.
 * @param filename 
 * @param req 
 */
void stream_file(char * filename, httpd_req_t * req)
{
    FILE * stream = fopen(filename, "r");
    if (stream == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s for reading", filename);
        return;
    }
    char buffer[100] = {0};
    while(!feof(stream)) {
        size_t len = fread(buffer, 1, 100, stream);
        httpd_resp_send_chunk(req, buffer, len);
    }
    fclose(stream);
}

/**
 * @brief Stream the page header to the client.
 * @param req 
 */
void stream_page_head(httpd_req_t * req)
{
    stream_file("/spiffs/header.html", req);
}

/**
 * @brief Stream the page footer to the client.
 * @param req 
 */
void stream_page_foot(httpd_req_t * req)
{
    stream_file("/spiffs/footer.html", req);
}

/**
 * @brief Substitute variables in a string. 
 * Note, this will replace only a single instance of the variable for a given string.
 * 
 * @param string 
 * @param vars 
 * @param values 
 * @param count 
 * @return char* 
 */
char * substitute_vars(const char *string, const char ** vars, const char ** values, int count) {
    bool allocated = false;
    char * result = (char *)string;

    for (int i = 0; i < count; i++) {
        char * var = (char *)vars[i];
        char * value = (char *)values[i];
        char * pos = strstr(result, var);

        if (pos) {
            int len = strlen(value);
            int len_var = strlen(var);
            int len_string = strlen(result);
            int new_len = len_string - len_var + len;

            char * new_string = (char *)malloc(new_len + 1);
            memset(new_string, 0, new_len + 1);
            strncpy(new_string, result, pos - result);
            strcat(new_string, value);
            strcat(new_string, pos + len_var);
            if(allocated) {
                free(result);
            }
            allocated = true;
            result = new_string;
        }
    }
    return (char *)result;
}

void render_and_stream_content(char *file, httpd_req_t *req, const char **vars, const char **values, int count) {
    FILE * stream = fopen(file, "r");
    if (stream == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s for reading", file);
        return;
    }
    char buffer[512];
    while(!feof(stream)) {
        fgets(buffer, 512, stream) ;
        char * new_buffer = substitute_vars(buffer, vars, values, count);
        httpd_resp_send_chunk(req, new_buffer, strlen(new_buffer));
        free(new_buffer);
    }
    fclose(stream);
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

esp_err_t index_get_handler(httpd_req_t *req)
{
    const char * vars[] = {"<!--app_name-->"};
    const char * values[] = {"RGB LED Matrix"};

    stream_page_head(req);
    render_and_stream_content("/spiffs/index.html", req, vars, values, 1);
    stream_page_foot(req);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t scan_get_handler(httpd_req_t *req)
{
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    int ap_count = wifi_scan(ap_info);    
    char template[512]; 

    FILE * stream = fopen("/spiffs/network.html", "r");
    if (stream == NULL) {
        ESP_LOGE(TAG, "Failed to open file %s for reading", "/spiffs/network.html");
        return ESP_FAIL;
    }
    fgets(template, 512, stream) ;
    fclose(stream);

    stream_page_head(req);
    render_and_stream_content("/spiffs/scan_head.html", req, NULL, NULL, 0);

    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        const char * vars[] = {
            "{{ssid2}}", 
            "{{ssid1}}", 
            "{{auth2}}", 
            "{{auth1}}", 
            "{{rssi}}"
        };
        char rssi_level[3];
        sprintf(rssi_level, "%d", (int) floor(((ap_info[i].rssi + 100.0) / 70.0) * 4.0));
        char *auth = ap_info[i].authmode == WIFI_AUTH_OPEN ? "open" : "lock";
        const char * vals[] = {
            (char *)ap_info[i].ssid, 
            (char *)ap_info[i].ssid, 
            auth, auth, 
            rssi_level
        };
        char * buffer = substitute_vars(template, vars, vals, 5);

        httpd_resp_send_chunk(req, buffer, strlen(buffer));
        free(buffer);
    }

    stream_page_foot(req);
    render_and_stream_content("/spiffs/scan_foot.html", req, NULL, NULL, 0);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t connect_lock_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Attempting to connect to %s", req->uri);
    size_t query_len = httpd_req_get_url_query_len(req);
    if (query_len == 0) {
        //@todo make this print some nice message.
        return ESP_FAIL;
    }
    
    char * query_str = calloc(1, query_len + 1);
    httpd_req_get_url_query_str(req, query_str, query_len + 1);
    
    char ssid[32];
    httpd_query_key_value(query_str, "ssid", ssid, sizeof(ssid));
    stream_page_head(req);

    const char * vars[] = {"%%ssid1%%", "%%ssid2%%", "%%ssid3%%"};
    const char * values[] = {ssid, ssid, ssid};
    render_and_stream_content("/spiffs/password.html", req, vars, values, 3);

    stream_page_foot(req);
    httpd_resp_send_chunk(req, NULL, 0);

    free(query_str);
    return ESP_OK;
}

esp_err_t connect_post_handler(httpd_req_t *req)
{
    size_t buf_len = req->content_len + 1;
    char * buf = malloc(buf_len);
    size_t received = 0;

    // Download all the HTTP content
    while(received < req->content_len) {
        size_t ret = httpd_req_recv(req, buf + received, buf_len - received);
        if(ret <= 0 && ret == HTTPD_SOCK_ERR_TIMEOUT) {
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

    ESP_LOGI(TAG, "Attempting to connect to %s", ssid);
    if(wifi_start_station(ssid, password) == ESP_OK) {
        int connection_status = wifi_get_connection_status();
        while(connection_status == CONNECTION_STATUS_CONNECTING || connection_status == CONNECTION_STATUS_WAITING) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            connection_status = wifi_get_connection_status();
        }
    }

    if (wifi_get_connection_status() == CONNECTION_STATUS_CONNECTED) {
        const char * vars[] = {"{{ssid}}"};
        const char * values[] = {ssid};
        stream_page_head(req);
        render_and_stream_content("/spiffs/connected.html", req, vars, values, 1);
        wifi_end_ap();
        stream_page_foot(req);
    } else {
        char url[64];
        sprintf(url, "/connect_lock?ssid=%s", ssid);
        httpd_resp_set_status(req, "303 See Other");
        httpd_resp_set_hdr(req, "Location", url);
    }

    httpd_resp_send_chunk(req, NULL, 0);

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
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_get_handler,
            .user_ctx = NULL
        });
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/scan",
            .method = HTTP_GET,
            .handler = scan_get_handler,
            .user_ctx = NULL
        });
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/connect",
            .method = HTTP_POST,
            .handler = connect_post_handler,
            .user_ctx = NULL
        });
        httpd_register_uri_handler(server, &(httpd_uri_t) {
            .uri = "/connect_lock",
            .method = HTTP_GET,
            .handler = connect_lock_get_handler,
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
