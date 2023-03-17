#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lora.h"

/* Note: ESP32 don't support temperature sensor */

#if CONFIG_IDF_TARGET_ESP32C3
#include "driver/temp_sensor.h"

#elif CONFIG_IDF_TARGET_ESP32
void app_main(void)
{
    printf("ESP32 don't support temperature sensor\n");
}

#endif

static const char *TAG = "TempSensor";

// void tempsensor_example(void *arg)
// {
//     // Initialize touch pad peripheral, it will start a timer to run a filter
//     ESP_LOGI(TAG, "Initializing Temperature sensor");
//     float tsens_out;
//     temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
//     temp_sensor_get_config(&temp_sensor);
//     ESP_LOGI(TAG, "default dac %d, clk_div %d", temp_sensor.dac_offset, temp_sensor.clk_div);
//     temp_sensor.dac_offset = TSENS_DAC_DEFAULT; // DEFAULT: range:-10℃ ~  80℃, error < 1℃.
//     temp_sensor_set_config(temp_sensor);
//     temp_sensor_start();
//     ESP_LOGI(TAG, "Temperature sensor started");
//     while (1) {
//         vTaskDelay(1000 / portTICK_RATE_MS);
//         temp_sensor_read_celsius(&tsens_out);
//         ESP_LOGI(TAG, "Temperature out celsius %f°C", tsens_out);
//     }
//     vTaskDelete(NULL);
// }


#if CONFIG_SENDER
void task_tx(void *pvParameters)
{
	// Initialize touch pad peripheral, it will start a timer to run a filter
    ESP_LOGI(TAG, "Initializing Temperature sensor....");
    float tsens_out;
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor_get_config(&temp_sensor);
    ESP_LOGI(TAG, "default dac %d, clk_div %d", temp_sensor.dac_offset, temp_sensor.clk_div);
    temp_sensor.dac_offset = TSENS_DAC_DEFAULT; // DEFAULT: range:-10℃ ~  80℃, error < 1℃.
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
	ESP_LOGI(TAG, "Temperature sensor started...");

	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		temp_sensor_read_celsius(&tsens_out);
    	ESP_LOGI(TAG, "Temperature out celsius %f°C", tsens_out);
		TickType_t nowTick = xTaskGetTickCount();
		int send_len = sprintf((char *)buf, "%.6f", tsens_out, nowTick);
		lora_send_packet(buf, send_len);
		ESP_LOGI(pcTaskGetName(NULL), "%d byte packet sent...", send_len);
		vTaskDelay(pdMS_TO_TICKS(2000));
	} // end while
}
#endif // CONFIG_SENDER

void app_main()
{
	if (lora_init() == 0) {
		ESP_LOGE(pcTaskGetName(NULL), "Does not recognize the module");
		while(1) {
			vTaskDelay(1);
		}
	}

#if CONFIG_169MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 169MHz");
	lora_set_frequency(169e6); // 169MHz
#elif CONFIG_433MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz");
	lora_set_frequency(433e6); // 433MHz
#elif CONFIG_470MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 470MHz");
	lora_set_frequency(470e6); // 470MHz
#elif CONFIG_866MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
	lora_set_frequency(866e6); // 866MHz
#elif CONFIG_915MHZ
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is 915MHz");
	lora_set_frequency(915e6); // 915MHz
#elif CONFIG_OTHER
	ESP_LOGI(pcTaskGetName(NULL), "Frequency is %dMHz", CONFIG_OTHER_FREQUENCY);
	long frequency = CONFIG_OTHER_FREQUENCY * 1000000;
	lora_set_frequency(frequency);
#endif

	lora_enable_crc();

	int cr = 1;
	int bw = 7;
	int sf = 7;
#if CONFIF_ADVANCED
	cr = CONFIG_CODING_RATE
	bw = CONFIG_BANDWIDTH;
	sf = CONFIG_SF_RATE;
#endif

	lora_set_coding_rate(cr);
	//lora_set_coding_rate(CONFIG_CODING_RATE);
	//cr = lora_get_coding_rate();
	ESP_LOGI(pcTaskGetName(NULL), "coding_rate=%d", cr);

	lora_set_bandwidth(bw);
	//lora_set_bandwidth(CONFIG_BANDWIDTH);
	//int bw = lora_get_bandwidth();
	ESP_LOGI(pcTaskGetName(NULL), "bandwidth=%d", bw);

	lora_set_spreading_factor(sf);
	//lora_set_spreading_factor(CONFIG_SF_RATE);
	//int sf = lora_get_spreading_factor();
	ESP_LOGI(pcTaskGetName(NULL), "spreading_factor=%d", sf);

#if CONFIG_SENDER
	xTaskCreate(&task_tx, "task_tx", 1024*3, NULL, 5, NULL);
	// xTaskCreate(tempsensor_example, "temp", 2048, NULL, 5, NULL);
#endif
#if CONFIG_RECEIVER
	xTaskCreate(&task_rx, "task_rx", 1024*3, NULL, 5, NULL);
#endif
}
