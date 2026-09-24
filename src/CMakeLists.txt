file(GLOB_RECURSE SOURCES "*.c")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        "."
        "01_application"
        "01_application/event"
        "01_application/hsm"
        "01_application/hsm/provision"
        "01_application/hsm/sensor"
        "01_application/hsm/wifi"
        "02_middleware/m_ble"
        "02_middleware/m_database"
        "02_middleware/m_debug"
        "02_middleware/m_event"
        "02_middleware/m_led"
        "02_middleware/m_sensor"
        "02_middleware/m_wifi"
        "03_hardware"
    REQUIRES
        esp_adc
        driver
        freertos
        esp_wifi
        esp_event
        esp_netif
        lwip
        nvs_flash
        log
        bt
)
