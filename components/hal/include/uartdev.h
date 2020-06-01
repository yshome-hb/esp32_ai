#ifndef _UART_DEV_H_
#define _UART_DEV_H_


typedef void (*parse_callback_t)    (uint8_t* data, int len);

int uart_sendlen(const char* data, int len);
int uart_send(const char* data);
void uart_dev_init(int pin_tx, int pin_rx, parse_callback_t parse_cb);
void uart_dev_intr_enable(parse_callback_t send_cb);
void uart_dev_intr_disable();
uint8_t uart_dev_getBaud();
void uart_dev_baudrate(uint8_t baud);
void strrpl(char* pDstOut, char* pSrcIn, const char* pSrcRpl, const char* pDstRpl);


#endif /* _UART_DEV_H_ */