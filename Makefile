PROJECT_NAME := ph_iot_monitor

# Tự động nhận diện IDF_PATH nếu chưa export trong terminal
IDF_PATH ?= $(CURDIR)/esp/ESP8266_RTOS_SDK

EXTRA_COMPONENT_DIRS := $(CURDIR)/src

include $(IDF_PATH)/make/project.mk
