#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name.a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

COMPONENT_OBJS := easy_provision.o captdns.o mustach.o mustach-wrap.o mustach-cjson.o
COMPONENT_DEPENDS := log tcpip_adapter esp_http_server esp_event nvs_flash
COMPONENT_PRIV_INCLUDEDIRS := private

$(call compile_only_if,$(CONFIG_EASY_PROVISION_HTPORTAL),htportal.o)
$(call compile_only_if,$(CONFIG_EASY_PROVISION_JSPORTAL),jsportal.o)

ifdef CONFIG_EASY_PROVISION_HTPORTAL
	COMPONENT_EMBED_FILES := htportal/connected.html htportal/footer.html htportal/header.html htportal/index.html \
			htportal/manual.html htportal/network.html htportal/password.html htportal/scan_head.html htportal/scan_foot.html
endif

ifdef CONFIG_EASY_PROVISION_JSPORTAL
	COMPONENT_EMBED_FILES := $(wildcard jsportal/*.html)
endif
