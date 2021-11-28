#include "htportal.h"
#include <esp_http_server.h>
#include "esp_log.h"

static const char *TAG = "HTPORTAL";

/**
 * @brief Web server handle
 */
static httpd_handle_t server = NULL;

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

void stream_page_head(httpd_req_t * req)
{
    stream_file("/spiffs/header.html", req);
}

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
