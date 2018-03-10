#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := bluetooth_speaker

include $(IDF_PATH)/make/project.mk

sdkconfig: sdkconfig.defaults
	$(Q) cp $< $@

menuconfig: sdkconfig
defconfig:  sdkconfig

spiffs_bin:
	mkspiffs -c files -b 4096 -p 256 -s 0x10000 spiffs.bin

spiffs_flash: spiffs_bin
	esptool.py --chip esp32 --baud 2000000 write_flash -z 0x3f0000 spiffs.bin
