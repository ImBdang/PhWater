#include "hardware.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

static adc_oneshot_unit_handle_t s_adc_handle = NULL;

static esp_err_t hardware_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO) | (1ULL << 0),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        return ret;
    }

    gpio_set_level(STATUS_LED_GPIO, 0);
    gpio_set_level(0, 0);
    return ESP_OK;
}

static esp_err_t hardware_adc_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&init_config, &s_adc_handle);
    if (ret != ESP_OK)
    {
        return ret;
    }

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    return adc_oneshot_config_channel(s_adc_handle, PH_ADC_CHANNEL, &config);
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
