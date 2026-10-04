#include "hardware.h"
#include "debug.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t s_cali_handle = NULL;

static esp_err_t hardware_gpio_init(void)
{
    /* 1. Init Status LED */
    gpio_reset_pin(STATUS_LED_GPIO);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO),
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

    gpio_set_drive_capability(STATUS_LED_GPIO, GPIO_DRIVE_CAP_3);
    gpio_set_level(STATUS_LED_GPIO, 0);

    /* 2. Init Button (Active-LOW with internal pull-up) */
    gpio_reset_pin(BUTTON_GPIO);

    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ret = gpio_config(&btn_conf);
    if (ret != ESP_OK)
    {
        return ret;
    }

    return ESP_OK;
}

bool hardware_button_is_pressed(void)
{
    return (gpio_get_level(BUTTON_GPIO) == 0);
}

static esp_err_t hardware_adc_init(void)
{
    /* 1. Init ADC oneshot unit */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&init_config, &s_adc_handle);
    if (ret != ESP_OK)
    {
        return ret;
    }

    /* 2. Config ADC channel */
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    ret = adc_oneshot_config_channel(s_adc_handle, ANALOG_SENSOR_ADC_CHANNEL, &config);
    if (ret != ESP_OK)
    {
        return ret;
    }

    /* 3. Init ADC calibration (Line Fitting scheme for ESP32 classic) */
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .default_vref = 0,
    };

    ret = adc_cali_create_scheme_line_fitting(&cali_config, &s_cali_handle);
    if (ret != ESP_OK)
    {
        DEBUG_LOG("ADC calibration init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
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

esp_err_t hardware_adc_read_raw(int *out_raw)
{
    if (s_adc_handle == NULL || out_raw == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return adc_oneshot_read(s_adc_handle, ANALOG_SENSOR_ADC_CHANNEL, out_raw);
}

esp_err_t hardware_adc_raw_to_voltage(int raw, int *out_mv)
{
    if (out_mv == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_cali_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    return adc_cali_raw_to_voltage(s_cali_handle, raw, out_mv);
}
