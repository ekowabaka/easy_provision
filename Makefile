#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ledmatrix

include $(IDF_PATH)/make/project.mk

FSFILES=$(wildcard fs/*)

.PHONY: flash_spiffs

spiffs.bin: $(FSFILES)
	mkspiffs -c fs --size 458752 --page 256 --block 4096 spiffs.bin

flash_spiffs: spiffs.bin
	python $(IDF_PATH)/components/esptool_py/esptool/esptool.py \
		--chip esp8266 \
		--port /dev/ttyUSB0 \
		--baud 460800 \
		--before default_reset \
		--after hard_reset \
		write_flash 0x90000 storage.bin