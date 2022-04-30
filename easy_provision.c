#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#ifdef CONFIG_EASY_PROVISION_HTPORTAL
#include "private/htportal.h"
#endif

#ifdef CONFIG_EASY_PROVISION_JSPORTAL
#include "private/jsportal.h"
#endif


#include "private/captdns.h"
#include "private/internal.h"
#include "easy_provision.h"

static const char *TAG = "EasProvision";
static int connection_status = CONNECTION_STATUS_DISCONNECTED ;
static int state = STATE_CONNECTING;
static wifi_config_t wifi_config;
static easy_provision_config_t * easy_provision_config;

/**
 * @brief Receive and handle events on the statuses of the different wifi connections. Because this relies on the ESP's
 *        internal wifi event loop, some events may originate from other tasks.
 * 
 * @param arg A custom argument passed to the event handler. Currently contains the pointer to the config struct.
 * @param event_base 
 * @param event_id 
 * @param event_data 
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    easy_provision_config_t * config = (easy_provision_config_t *) arg;

    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        // When a station connects to the access point.
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        // When a station disconnects from the access point.
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        // When the station connects to the external access point.
        ESP_LOGI(TAG, "station connected");
        connection_status = CONNECTION_STATUS_CONNECTED;
        if(state == STATE_CONNECTING) {
            state = STATE_CONNECTED;
        } 
        // else if (state == STATE_SETTING_UP) {
        //     state = STATE_CONNECTED;
        //     stop_provisioning();
        // }
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // When the station is disconnected from the external access point.
        ESP_LOGI(TAG, "station disconnected");
        if (state == STATE_CONNECTING) {
            state = STATE_SETTING_UP;
            start_provisioning(config);
        }
        if(connection_status==CONNECTION_STATUS_CONNECTING) {
            connection_status = CONNECTION_STATUS_FAILED;
        } else {
            connection_status = CONNECTION_STATUS_DISCONNECTED;
        }
    }
}

/**
 * @brief Return the wifi connection status.
 * 
 * @return int 
 */
int wifi_get_connection_status()
{
    return connection_status;
}

/**
 * @brief Scan and return the list of available wifi networks.
 * 
 * @param ap_info 
 * @return int 
 */
int wifi_scan(wifi_ap_record_t *ap_info)
{
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    uint16_t ap_count = 0;

    memset(ap_info, 0, sizeof(wifi_ap_record_t) * DEFAULT_SCAN_LIST_SIZE);    
    esp_wifi_scan_start(NULL, true);
    esp_wifi_scan_get_ap_records(&number, ap_info);
    esp_wifi_scan_get_ap_num(&ap_count);

    return ap_count;
}

/**
 * @brief Initialize the wifi adapter.
 * 
 * This function initially attempts to read the wifi credentials from flash memory. If it finds any, it attempts to 
 * connect to the wifi network. If it fails, it starts the portal. If it succeeds, it establishes a connection to the
 * WiFi network and returns control to the main application. In the case where there are no credentials stored in flash
 * memory, it starts the portal.
 * 
 */
void init_wifi(easy_provision_config_t * config)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, config));
    
    // Read persisted credentials and attempt to connect
    ESP_ERROR_CHECK(esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config));
    if (strlen((const char *)wifi_config.sta.ssid) == 0) {
        ESP_LOGI(TAG, "No stored credentials found");
        start_provisioning(config);
    } else {
        ESP_LOGI(TAG, "Connecting to [%s]", wifi_config.sta.ssid);
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        esp_wifi_connect();
    }
}


void start_provisioning(easy_provision_config_t * config)
{
    esp_wifi_stop();

    state = STATE_SETTING_UP;
    wifi_config.ap.ssid_len = strlen(config->ssid);
    wifi_config.ap.max_connection = 2;
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    strncpy((char *)&wifi_config.ap.password, config->password, strlen(config->password));
    strncpy((char *)&wifi_config.sta.ssid, config->ssid, strlen(config->ssid));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    captdnsInit();
    start_portal();
}

esp_err_t wifi_start_connecting(char * ssid, char * password)
{
    strncpy((char *)&wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)&wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    connection_status = CONNECTION_STATUS_CONNECTING;

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "Connecting to WiFi with ssid:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
    esp_wifi_start();
    return esp_wifi_connect();
}

void stop_provisioning()
{
    ESP_LOGI(TAG, "Disabling AP");
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    endCaptDnsTask();
}

/**
 * @brief Start the easy provision system.
 * 
 * @param config Configuration details for the portal.
 * 
 * @return esp_err_t 
 */
esp_err_t easy_provision_start(easy_provision_config_t * config)
{
    ESP_LOGI(TAG, "Starting Easy Provision ...");
    nvs_flash_init();
    easy_provision_config = (easy_provision_config_t *) malloc(sizeof(easy_provision_config_t));
    memcpy(easy_provision_config, config, sizeof(easy_provision_config_t));
    init_wifi(config);

    return ESP_OK;
}

void easy_provision_reset(bool restart)
{
    ESP_LOGI(TAG, "Resetting Easy Provision ...");
    if(nvs_flash_deinit() == ESP_ERR_NVS_NOT_INITIALIZED) {
        nvs_flash_init();
    }
    nvs_flash_erase();
    if(restart) {
        esp_restart();
    } 
}