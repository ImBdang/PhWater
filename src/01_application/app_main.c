#include "app_main.h"
#include "m_led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void app_task(void *pvParameters)
{
    while (1)
    {
        status_offline();
    }
}

void app_start(void)
{
    xTaskCreate(app_task, "app_task", 2048, NULL, 5, NULL);
}
