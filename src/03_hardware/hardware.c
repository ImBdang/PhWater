#include "hardware.h"

#include "driver/gpio.h"
#include "driver/adc.h"


static esp_err_t hardware_gpio_init(void)
{
    esp_err_t ret;

    ret = gpio_set_direction(
        STATUS_LED_GPIO,
        GPIO_MODE_OUTPUT
    );

    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = gpio_set_level(
        STATUS_LED_GPIO,
        1
    );

    return ret;
}


static esp_err_t hardware_adc_init(void)
{
    adc_config_t config = {
        .mode = ADC_READ_TOUT_MODE,
        .clk_div = 8,
    };

    return adc_init(&config);
}


esp_err_t hardware_init(void)
{
    esp_err_t ret;

    ret = hardware_gpio_init();

    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = hardware_adc_init();

    if (ret != ESP_OK)
    {
        return ret;
    }

    return ESP_OK;
}
