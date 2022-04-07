#ifndef EASY_PROVISION_H
#define EASY_PROVISION_H

typedef struct {
    char ssid[32];
    char password[64];
    char bssid[18];
    int channel;
    int authmode;
    int rssi;
} ep_config_t;

esp_err_t ep_start(ep_config_t *config);
esp_err_t ep_reset();
esp_err_t ep_stop();

#endif