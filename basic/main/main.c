#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "lora.h"

TaskHandle_t myTaskHandle = NULL;
TaskHandle_t myTaskHandle2 = NULL;
QueueHandle_t queue;

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

void tempsensor_example(void *arg)
{
    char txBuffer[50];
    queue = xQueueCreate(5, sizeof(txBuffer)); 
    if (queue == 0)
    {
     printf("Failed to create queue= %p\n", queue);
    }
	
	// Initialize touch pad peripheral, it will start a timer to run a filter
    ESP_LOGI(TAG, "Initializing Temperature sensor");
    float tsens_out;
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor_get_config(&temp_sensor);
    ESP_LOGI(TAG, "default dac %d, clk_div %d", temp_sensor.dac_offset, temp_sensor.clk_div);
    temp_sensor.dac_offset = TSENS_DAC_DEFAULT; // DEFAULT: range:-10℃ ~  80℃, error < 1℃.
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
    ESP_LOGI(TAG, "Temperature sensor started");
    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
        temp_sensor_read_celsius(&tsens_out);
        ESP_LOGI(TAG, "Temperature out celsius %f°C", tsens_out);

		sprintf(txBuffer, "%.6f", tsens_out);
		xQueueSend(queue, (void*)txBuffer, (TickType_t)0);

    }
    vTaskDelete(NULL);
}


#if CONFIG_SENDER
void task_tx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[30]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		TickType_t nowTick = xTaskGetTickCount();
		vTaskDelay(pdMS_TO_TICKS(1000));

		if( xQueueReceive(queue, &(buf), (TickType_t)5)){
			printf("Received data from queue ==== %s\n", buf);
		}

		lora_send_packet(buf, 30);

	} // end while
}
#endif // CONFIG_SENDER

#if CONFIG_RECEIVER
void task_rx(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start");
	uint8_t buf[256]; // Maximum Payload size of SX1276/77/78/79 is 255
	while(1) {
		lora_receive(); // put into receive mode
		if (lora_received()) {
			int receive_len = lora_receive_packet(buf, sizeof(buf));
			ESP_LOGI(pcTaskGetName(NULL), "%d byte packet received:[%.*s]", receive_len, receive_len, buf);
		} // end if
		vTaskDelay(1); // Avoid WatchDog alerts
	} // end while
}
#endif // CONFIG_RECEIVER

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
	xTaskCreate(&task_tx, "task_tx", 1024*3, NULL, 5, &myTaskHandle2);
	xTaskCreate(tempsensor_example, "temp", 2048, NULL, 5, &myTaskHandle);
#endif
#if CONFIG_RECEIVER
	xTaskCreate(&task_rx, "task_rx", 1024*3, NULL, 5, NULL);
#endif
}
