#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "I2Cdev.h"
#include "at_cmd.h"
#include "led.h"
#include "camera_photo.h"


#define TAG "main:"

#if (CONFIG_LOG_DEFAULT_LEVEL>0)
    #define AT_UART_TX_PIN  21
    #define AT_UART_RX_PIN  22
#else
    #define AT_UART_TX_PIN  1
    #define AT_UART_RX_PIN  3
#endif

#define MINI58_RESET_PIN	14

void app_main()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

	gpio_pad_select_gpio(MINI58_RESET_PIN);
    gpio_set_direction(MINI58_RESET_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(MINI58_RESET_PIN, 0);
    vTaskDelay(20 / portTICK_PERIOD_MS);  
	gpio_set_level(MINI58_RESET_PIN, 1);   
    vTaskDelay(50 / portTICK_PERIOD_MS); 

    I2C_init(CONFIG_I2C_SDA, CONFIG_I2C_SCL, CONFIG_I2C_FREQ);    
    at_cmd_init(AT_UART_TX_PIN, AT_UART_RX_PIN);

    Ui_Step_E ui_step = UI_IDLE;
    Ui_Step_E ui_temp = UI_IDLE;
    Led_setLedMode(ui_step);   
    while(1){
        ui_temp = at_process();
        if(ui_step != ui_temp){
            ui_step = ui_temp;
            //模式灯效添加处
            Led_setLedMode(ui_step);        
        }

        if(ui_step == UI_FACTORY){
            vTaskDelay(4 / portTICK_PERIOD_MS);  
        }else{      
#if (CONFIG_LOG_DEFAULT_LEVEL>0)
        ESP_LOGI(TAG, "Free ram: %d", esp_get_free_heap_size());
        // size_t free8start=heap_caps_get_free_size(MALLOC_CAP_8BIT);
        // size_t free32start=heap_caps_get_free_size(MALLOC_CAP_32BIT);
        // ESP_LOGI(TAG,"free mem8bit: %d mem32bit: %d\n",free8start,free32start);
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
#else
        vTaskDelay(50 / portTICK_PERIOD_MS);  
#endif
        }

// unsigned portBASE_TYPE uxHighWaterMark; 
// uxHighWaterMark=uxTaskGetStackHighWaterMark(NULL); 
// printf("uxHighWaterMark: %d\r\n", uxHighWaterMark);
    }
}

