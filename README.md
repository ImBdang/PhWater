# Hệ thống IoT giám sát độ pH sử dụng ESP8266 và MQTT

## Clone sdk

```bash
mkdir -p esp
cd esp
git clone -b release/v3.4 --recursive --depth 1 https://github.com/espressif/ESP8266_RTOS_SDK.git
cd ..
```

## Download compiler

```bash
wget https://dl.espressif.com/dl/xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
tar -xzf xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
rm xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
```

## Install requirement python

```bash
python3 -m pip install --user -r ./esp/ESP8266_RTOS_SDK/requirements.txt
```

## Test build hello world

```bash
export IDF_PATH=$(pwd)/esp/ESP8266_RTOS_SDK
export PATH="$PATH:$(pwd)/xtensa-lx106-elf/bin"

cd ./esp/ESP8266_RTOS_SDK/examples/get-started/hello_world
make defconfig
make -j$(nproc)
make flash

# Mở serial monitor
make monitor
# hoặc
picocom -b 74880 /dev/ttyUSB0
```
