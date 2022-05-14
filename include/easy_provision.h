#ifndef EASY_PROVISION_H
#define EASY_PROVISION_H

#include "esp_event.h"

extern esp_event_loop_handle_t easy_provision_event_loop;

enum {
    EASY_PROVISION_EVENT_STARTED,
    EASY_PROVISION_EVENT_PROVISIONING,
    EASY_PROVISION_EVENT_CONNECTED,
    EASY_PROVISION_EVENT_DISCONNECTED
};

ESP_EVENT_DECLARE_BASE(EASY_PROVISION);

/**
 * @brief Easy Provisioning
 */
typedef struct {
    char path[32];
    char ssid[32];
    char password[64];
    char app_name[32];
} easy_provision_config_t;

esp_err_t easy_provision_init(easy_provision_config_t *config);
esp_err_t easy_provision_start();
void easy_provision_reset(bool restart);
esp_err_t easy_provision_stop();

#endif