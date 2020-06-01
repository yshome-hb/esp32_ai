#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32delay.h"

portMUX_TYPE microsMux = portMUX_INITIALIZER_UNLOCKED;

uint32_t IRAM_ATTR micros()
{
    static uint32_t lccount = 0;
    static uint32_t overflow = 0;
    uint32_t ccount;
    portENTER_CRITICAL_ISR(&microsMux);
    __asm__ __volatile__ ( "rsr     %0, ccount" : "=a" (ccount) );
    if(ccount < lccount){
        overflow += UINT32_MAX / CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ;
    }
    lccount = ccount;
    portEXIT_CRITICAL_ISR(&microsMux);
    return overflow + (ccount / CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ);
}

uint32_t IRAM_ATTR millis()
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

inline void delay(int millis) 
{
	vTaskDelay(millis / portTICK_PERIOD_MS);
}


