IDF_PATH ?= $(CURDIR)/esp/esp-idf
PORT ?= /dev/ttyUSB0

.PHONY: all set-target build flash monitor clean

all: build

set-target:
	. $(IDF_PATH)/export.sh && idf.py set-target esp32

build:
	. $(IDF_PATH)/export.sh && idf.py build

flash:
	. $(IDF_PATH)/export.sh && idf.py -p $(PORT) flash

monitor:
	. $(IDF_PATH)/export.sh && idf.py -p $(PORT) monitor

clean:
	. $(IDF_PATH)/export.sh && idf.py fullclean
