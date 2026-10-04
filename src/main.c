#include "app_main.h"
#include "hardware.h"
#include "m_led.h"
#include "m_button.h"
#include "debug.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) 
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    hardware_init();
    DEBUG_LOG("Hardware initialized");

    button_init();

    xTaskCreate(led_task, "led_task", 2048, NULL, 1, NULL);
    xTaskCreate(button_task, "button_task", 2048, NULL, 3, NULL);
    xTaskCreate(app_task, "app_task", 4096, NULL, 5, NULL);
}
