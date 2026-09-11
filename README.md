# Hệ thống IoT giám sát độ pH sử dụng ESP32 và MQTT

Dự án giám sát chất lượng nước (đo độ pH) ứng dụng kiến trúc Hierarchical State Machine (HSM), Event-driven trên nền tảng vi điều khiển ESP32 và ESP-IDF v5.3.

---

## 1. Cấu hình phần cứng (Hardware Pinout)

- **Vi điều khiển:** ESP32 (ESP32 DevKit / ESP32-WROOM-32)
- **Status LED:** `GPIO 2` (Onboard LED - Active HIGH)
- **Cảm biến pH (Analog Input):** `GPIO 34` (ADC1 Channel 6, độ phân giải 12-bit: 0 - 4095)

---

## 2. Cài đặt môi trường ESP-IDF v5.3

### Clone ESP-IDF SDK (nhánh v5.3)

```bash
mkdir -p esp
cd esp
git clone -b release/v5.3 --recursive --depth 1 https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
cd ../..
```

### Kích hoạt môi trường (Environment Export)

Trước khi thực hiện các lệnh build/flash, chạy lệnh export môi trường:

```bash
. esp/esp-idf/export.sh
```

---

## 3. Biên dịch và Nạp chương trình

Dự án hỗ trợ cả công cụ chuẩn `idf.py` và `Makefile` wrapper tiện lợi:

### Thiết lập target ESP32 (lần đầu tiên)

```bash
idf.py set-target esp32
# hoặc
make set-target
```

### Biên dịch dự án (Build)

```bash
idf.py build
# hoặc
make build
```

### Nạp vào ESP32 (Flash)

```bash
idf.py -p /dev/ttyUSB0 flash
# hoặc
make flash
```

### Xem Log Serial Monitor

```bash
idf.py -p /dev/ttyUSB0 monitor
# hoặc
make monitor
```

*Nhấn `Ctrl + ]` để thoát monitor.*

---

## 4. Cấu trúc thư mục mã nguồn

```
├── CMakeLists.txt              # File cấu hình build ESP-IDF cấp root
├── Makefile                    # Wrapper lệnh build/flash/monitor nhanh
├── esp/esp-idf                 # ESP-IDF SDK v5.3
├── xtensa-esp-elf              # Toolchain compiler Xtensa GCC 13.2 cho ESP32
└── src/
    ├── CMakeLists.txt          # Đăng ký component src với ESP-IDF
    ├── main.c                  # Hàm app_main() khởi tạo NVS, hardware và app
    ├── STATE.h                 # Định nghĩa các state của Application HSM
    ├── 01_application/         # Tầng ứng dụng (App task, HSM, Event router)
    │   ├── app_main.c/.h
    │   ├── event/              # Định nghĩa event ID và struct
    │   └── hsm/                # Core HSM engine và WiFi HSM
    ├── 02_middleware/          # Tầng middleware
    │   ├── m_debug/            # Module log DEBUG_LOG với prefix [USER]
    │   ├── m_event/            # Ring buffer hàng đợi sự kiện
    │   ├── m_led/              # Điều khiển LED báo trạng thái
    │   └── m_wifi/             # Module WiFi kết nối và ICMP Ping
    └── 03_hardware/            # Tầng giao tiếp phần cứng ESP32
        ├── hardware.c/.h       # Khởi tạo GPIO và ADC Oneshot
```
