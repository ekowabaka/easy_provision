# if(CONFIG_EASY_PROVISION_HTPORTAL)
#         set(portal_src "htportal.c")
# elseif(CONFIG_EASY_PROVISION_JSPORTAL)
#         set(portal_src "jsportal.c")
# endif()

if(CONFIG_EASY_PROVISION_HTPORTAL)
    file(GLOB TXTFILES htportal/*.html htportal/*.css)
    file(GLOB FILES htportal/*.png)
elseif(CONFIG_EASY_PROVISION_JSPORTAL)
    file(GLOB TXTFILES jsportal/*.html jsportal/*.css jsportal/*.js)
    file(GLOB FILES jsportal/*.svg)        
endif()

idf_component_register(SRCS "easy_provision.c" 
                            "htportal.c"
                            "captdns.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "private"
                    REQUIRES log esp_wifi esp_netif esp_http_server esp_event spiffs nvs_flash json
                    EMBED_TXTFILES ${TXTFILES}
                    EMBED_FILES ${FILES}
                )


# if(CONFIG_EASY_PROVISION_HTPORTAL)
#         idf_component_register(EMBED_TXTFILES htportal/connected.html)
#         idf_component_register(EMBED_TXTFILES htportal/footer.html)
#         idf_component_register(EMBED_TXTFILES htportal/header.html)
#         idf_component_register(EMBED_TXTFILES htportal/index.html)
#         idf_component_register(EMBED_TXTFILES htportal/manual.html)
#         idf_component_register(EMBED_TXTFILES htportal/network.html)
#         idf_component_register(EMBED_TXTFILES htportal/password.html)
# elseif(CONFIG_EASY_PROVISION_JSPORTAL)
#         idf_component_register(EMBED_FILES htportal/portal_index.html)
#         idf_component_register(EMBED_FILES htportal/portal_connected.html)
#         idf_component_register(EMBED_FILES htportal/portal_manual.html)
#         idf_component_register(EMBED_FILES htportal/portal_select.html)
# endif()
