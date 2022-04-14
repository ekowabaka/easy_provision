#ifndef EASY_PROVISION_H
#define EASY_PROVISION_H

/**
 * @brief Easy Provisioning
 */
typedef struct {
    char path[32];
    char ssid[32];
    char password[64];
    char app_name[32];
} easy_provision_config_t;

esp_err_t easy_provision_start(easy_provision_config_t *config);
esp_err_t easy_provision_reset();
esp_err_t easy_provision_stop();

#endif