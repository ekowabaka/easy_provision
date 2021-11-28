#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ledmatrix

include $(IDF_PATH)/make/project.mk

JSPORTAL_FILES=$(wildcard jsportal/*)
HTPORTAL_FILES=$(wildcard htportal/*)

.PHONY: flash_jsportal flash_htportal

jsportal.bin: $(JSPORTAL_FILES)
	mkspiffs -c jsportal --size 0x19000 --page 256 --block 4096 jsportal.bin -d 5

htportal.bin: $(HTPORTAL_FILES)
	mkspiffs -c htportal --size 0x19000 --page 256 --block 4096 htportal.bin -d 5

flash_jsportal: jsportal.bin
	python $(IDF_PATH)/components/esptool_py/esptool/esptool.py \
		--chip esp8266 \
		--port $(CONFIG_ESPTOOLPY_PORT) \
		--baud 460800 \
		--before default_reset \
		--after hard_reset \
		write_flash 0xe7000 spiffs.bin

flash_htportal: htportal.bin
	python $(IDF_PATH)/components/esptool_py/esptool/esptool.py \
		--chip esp8266 \
		--port $(CONFIG_ESPTOOLPY_PORT) \
		--baud 460800 \
		--before default_reset \
		--after hard_reset \
		write_flash 0xe7000 htportal.bin		