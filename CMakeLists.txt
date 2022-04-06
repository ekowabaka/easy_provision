idf_component_register(SRCS "easy_provision.c"
                            "htportal.c"
                            "captdns.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "private"
                    REQUIRES log tcpip_adapter esp_http_server esp_event spiffs nvs_flash)

