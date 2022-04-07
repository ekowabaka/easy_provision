if(CONFIG_EASY_PROVISION_HTPORTAL)
        set(portal_src "htportal.c")
elseif(CONFIG_EASY_PROVISION_JSPORTAL)
        set(portal_src "jsportal.c")
endif()

idf_component_register(SRCS "easy_provision.c"
                            ${portal_src}
                            "captdns.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "private"
                    REQUIRES log tcpip_adapter esp_http_server esp_event spiffs nvs_flash)

