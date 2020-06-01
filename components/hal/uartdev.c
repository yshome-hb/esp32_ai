#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "uartdev.h"

#if (CONFIG_LOG_DEFAULT_LEVEL>0)
    #define CONFIG_UART_PORT        1
#else
    #define CONFIG_UART_PORT        0
#endif

#define CONFIG_UART_BAUDRATE    9600
#define CONFIG_BUF_SIZE         200


static QueueHandle_t esp_uart_queue = NULL;
static parse_callback_t parse_callback = NULL;
static parse_callback_t send_callback = NULL;
static uint8_t uart_baudrate = 0;


int uart_sendlen(const char* data, int len)
{
    return uart_write_bytes(CONFIG_UART_PORT, data, len);
} 

int uart_send(const char* data)
{
    return uart_write_bytes(CONFIG_UART_PORT, data, strlen(data));
}  


static void uart_task(void *pvParameters)
{
    uart_event_t event;
    int pattern_pos = -1;
    uint8_t* dtmp = (uint8_t*) malloc(CONFIG_BUF_SIZE);
    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(esp_uart_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
            switch (event.type) {
                //Event of UART receving data
            case UART_DATA:
                while(event.size){
                    pattern_pos = event.size > 20 ? 20 : event.size;
                    uart_read_bytes(CONFIG_UART_PORT, dtmp, pattern_pos, portMAX_DELAY);
                    if(send_callback){
                        send_callback(dtmp, pattern_pos);
                    }
                    event.size -= pattern_pos;
                }
                break;
            case UART_FIFO_OVF:
            case UART_BUFFER_FULL:
                uart_flush_input(CONFIG_UART_PORT);
                xQueueReset(esp_uart_queue);
                break;    
            case UART_PATTERN_DET:
                pattern_pos = uart_pattern_pop_pos(CONFIG_UART_PORT);
                if(pattern_pos == -1){
                    uart_flush_input(CONFIG_UART_PORT);
                }else{
                    uart_read_bytes(CONFIG_UART_PORT, dtmp, pattern_pos+1, 100 / portTICK_PERIOD_MS);
                    dtmp[pattern_pos] = '\0';
                    if(parse_callback)
                        parse_callback(dtmp, pattern_pos-1);
                }        
                break;
            //Others
            default:
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;    
    vTaskDelete(NULL);
}


void uart_dev_init(int pin_tx, int pin_rx, parse_callback_t parse_cb)
{
    uart_config_t uart_config = {
        .baud_rate = CONFIG_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    parse_callback = parse_cb;
    //Set UART parameters
    uart_param_config(CONFIG_UART_PORT, &uart_config);
    //Set UART pins,(-1: default pin, no change.)
    uart_set_pin(CONFIG_UART_PORT, pin_tx, pin_rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //Install UART driver, and get the queue.
    uart_driver_install(CONFIG_UART_PORT, CONFIG_BUF_SIZE, CONFIG_BUF_SIZE, 10, &esp_uart_queue, 0);  
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(CONFIG_UART_PORT, 20);
    uart_dev_intr_disable();

    xTaskCreate(uart_task, "uart_task", 2304, NULL, 5, NULL);
}


void uart_dev_intr_enable(parse_callback_t send_cb)
{	
    send_callback = send_cb;
    uart_disable_pattern_det_intr(CONFIG_UART_PORT);    
    uart_enable_rx_intr(CONFIG_UART_PORT);
}


void uart_dev_intr_disable()
{	 
    uart_disable_rx_intr(CONFIG_UART_PORT);
    uart_enable_pattern_det_intr(CONFIG_UART_PORT, '\n', 1, 5000, 10, 10);
    send_callback = NULL;
}


uint8_t uart_dev_getBaud()
{	
    return uart_baudrate;
}


void uart_dev_baudrate(uint8_t baud)
{	
    if(baud == uart_baudrate)
        return;

    uart_wait_tx_done(CONFIG_UART_PORT, 50/portTICK_PERIOD_MS);	
	switch(baud){				
		case 0:
			uart_set_baudrate(CONFIG_UART_PORT, 9600);
			break;	
		case 1:
			uart_set_baudrate(CONFIG_UART_PORT, 19200);
			break;	
		case 2:
			uart_set_baudrate(CONFIG_UART_PORT, 38400);
			break;	
		case 3:
			uart_set_baudrate(CONFIG_UART_PORT, 57600);
			break;	
		case 4:
			uart_set_baudrate(CONFIG_UART_PORT, 115200);
			break;
		default:
			return;
	}
    uart_baudrate = baud;
    uart_flush(CONFIG_UART_PORT);
}


// 参数说明:  
// in， 源字符串  
// src，要替换的字符串  
// dst，替换成什么字符串  
void strrpl(char* pDstOut, char* pSrcIn, const char* pSrcRpl, const char* pDstRpl)
{ 
	char* pi = pSrcIn; 
	char* po = pDstOut; 

	int nSrcRplLen = strlen(pSrcRpl); 
	int nDstRplLen = strlen(pDstRpl); 

	char *p = NULL; 
	int nLen = 0; 

	while(1)
	{
		p = strstr(pi, pSrcRpl); 
		if(p == NULL) 
		{ 
			strcpy(po, pi); 
			break;
		} 
		nLen = p - pi; 
		memcpy(po, pi, nLen);
		memcpy(po + nLen, pDstRpl, nDstRplLen); 

		pi = p + nSrcRplLen; 
		po = po + nLen + nDstRplLen; 
	}
}


