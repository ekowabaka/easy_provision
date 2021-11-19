#include <sys/param.h>
#include <esp_http_server.h>
#include "ui_server.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"

static const char *TAG = "UI-Server";

static httpd_handle_t server = NULL;

// /* An HTTP GET handler */
// esp_err_t hello_get_handler(httpd_req_t *req)
// {
//     char*  buf;
//     size_t buf_len;

//     /* Get header value string length and allocate memory for length + 1,
//      * extra byte for null termination */
//     buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
//     if (buf_len > 1) {
//         buf = malloc(buf_len);
//         /* Copy null terminated value string into buffer */
//         if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
//             ESP_LOGI(TAG, "Found header => Host: %s", buf);
//         }
//         free(buf);
//     }

//     buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
//     if (buf_len > 1) {
//         buf = malloc(buf_len);
//         if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
//             ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
//         }
//         free(buf);
//     }

//     buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
//     if (buf_len > 1) {
//         buf = malloc(buf_len);
//         if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
//             ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
//         }
//         free(buf);
//     }

//     /* Read URL query string length and allocate memory for length + 1,
//      * extra byte for null termination */
//     buf_len = httpd_req_get_url_query_len(req) + 1;
//     if (buf_len > 1) {
//         buf = malloc(buf_len);
//         if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
//             ESP_LOGI(TAG, "Found URL query => %s", buf);
//             char param[32];
//             /* Get value of expected key from query string */
//             if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
//                 ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
//             }
//             if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
//                 ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
//             }
//             if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
//                 ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
//             }
//         }
//         free(buf);
//     }

//     /* Set some custom headers */
//     httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
//     httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

//     /* Send response with custom headers and body set as the
//      * string passed in user context*/
//     const char* resp_str = (const char*) req->user_ctx;
//     httpd_resp_send(req, resp_str, strlen(resp_str));

//     /* After sending the HTTP response the old HTTP request
//      * headers are lost. Check if HTTP request headers can be read now. */
//     if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
//         ESP_LOGI(TAG, "Request headers lost");
//     }
//     return ESP_OK;
// }

// httpd_uri_t hello = {
//     .uri       = "/hello",
//     .method    = HTTP_GET,
//     .handler   = hello_get_handler,
//     /* Let's pass response string in user
//      * context to demonstrate it's usage */
//     .user_ctx  = "Hello World!"
// };

// /* An HTTP POST handler */
// esp_err_t echo_post_handler(httpd_req_t *req)
// {
//     char buf[100];
//     int ret, remaining = req->content_len;

//     while (remaining > 0) {
//         /* Read the data for the request */
//         if ((ret = httpd_req_recv(req, buf,
//                         MIN(remaining, sizeof(buf)))) <= 0) {
//             if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
//                 /* Retry receiving if timeout occurred */
//                 continue;
//             }
//             return ESP_FAIL;
//         }

//         /* Send back the same data */
//         httpd_resp_send_chunk(req, buf, ret);
//         remaining -= ret;

//         /* Log data received */
//         ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
//         ESP_LOGI(TAG, "%.*s", ret, buf);
//         ESP_LOGI(TAG, "====================================");
//     }

//     // End response
//     httpd_resp_send_chunk(req, NULL, 0);
//     return ESP_OK;
// }

// httpd_uri_t echo = {
//     .uri       = "/echo",
//     .method    = HTTP_POST,
//     .handler   = echo_post_handler,
//     .user_ctx  = NULL
// };

// /* An HTTP PUT handler. This demonstrates realtime
//  * registration and deregistration of URI handlers
//  */
// esp_err_t ctrl_put_handler(httpd_req_t *req)
// {
//     char buf;
//     int ret;

//     if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
//         if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
//             httpd_resp_send_408(req);
//         }
//         return ESP_FAIL;
//     }

//     if (buf == '0') {
//         /* Handler can be unregistered using the uri string */
//         ESP_LOGI(TAG, "Unregistering /hello and /echo URIs");
//         httpd_unregister_uri(req->handle, "/hello");
//         httpd_unregister_uri(req->handle, "/echo");
//     }
//     else {
//         ESP_LOGI(TAG, "Registering /hello and /echo URIs");
//         httpd_register_uri_handler(req->handle, &hello);
//         httpd_register_uri_handler(req->handle, &echo);
//     }

//     /* Respond with empty body */
//     httpd_resp_send(req, NULL, 0);
//     return ESP_OK;
// }

// httpd_uri_t ctrl = {
//     .uri       = "/ctrl",
//     .method    = HTTP_PUT,
//     .handler   = ctrl_put_handler,
//     .user_ctx  = NULL
// };

esp_err_t index_get_handler(httpd_req_t *req)
{
        ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    } 



    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file");
    
    FILE * f = fopen("/spiffs/index.htm", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_OK;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    httpd_resp_send(req, line, strlen(line));
    return ESP_OK;
}


httpd_uri_t index_route = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_route);
        // httpd_register_uri_handler(server, &echo);
        // httpd_register_uri_handler(server, &ctrl);
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

