#include "m_sensor.h"
#include "hardware.h"
#include "event.h"
#include "debug.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 
 * Chế độ chẩn đoán:
 * 1: Đọc và log raw ADC liên tục mỗi 100ms (thu thập 30-50 mẫu raw phân tích nhiễu/spike)
 * 0: Chế độ bình thường (lấy mẫu 40 lần, lọc saturation, chu kỳ 2s)
 */
#define SENSOR_DIAGNOSTIC_RAW_STREAM   1

#if SENSOR_DIAGNOSTIC_RAW_STREAM
#define SENSOR_SAMPLE_INTERVAL_MS      100
#else
#define SENSOR_SAMPLE_INTERVAL_MS      2000
#endif

void sensor_task(void *pvParameters)
{
    (void)pvParameters;

    DEBUG_LOG("Sensor task started (raw_stream=%d)", SENSOR_DIAGNOSTIC_RAW_STREAM);

    while (1)
    {
#if SENSOR_DIAGNOSTIC_RAW_STREAM
        int raw = 0;
        esp_err_t ret = hardware_adc_read_raw(&raw);
        if (ret == ESP_OK)
        {
            DEBUG_LOG("ADC RAW = %d", raw);
        }
        else
        {
            DEBUG_LOG("ADC RAW read error: %s", esp_err_to_name(ret));
        }
#else
        int adc_mv = 0;
        esp_err_t ret = hardware_adc_read_voltage(&adc_mv);
        if (ret == ESP_OK)
        {
            float po_voltage = ((float)adc_mv / 1000.0f) * 2.0f;
            DEBUG_LOG("ADC voltage = %d mV | PH-4502C PO: %.2f V", adc_mv, po_voltage);

            event_t event = {
                .id = EVT_PH_UPDATE,
                .data.ph = po_voltage,
            };
            event_post(&event);
        }
        else
        {
            DEBUG_LOG("PH-4502C read error / saturated");
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(SENSOR_SAMPLE_INTERVAL_MS));
    }
}
