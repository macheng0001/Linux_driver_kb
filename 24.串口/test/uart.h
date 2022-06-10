#ifndef _UART_H
#define _UART_H
int set_uart_opt(int fd, int baud_rate, int data_bits, char parity,
                 int stop_bits);
#endif