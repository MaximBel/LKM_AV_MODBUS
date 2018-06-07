/*
 * uart.h
 *
 *  Created on: 24 May 2018
 *      Author: pi
 */

#ifndef UART_H_
#define UART_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/serial_core.h>

typedef struct {
	struct circ_buf rx_buffer;
	struct circ_buf tx_buffer;
	struct uart_port port;
} uart_control_t;


void write_data_to_uart(uart_control_t * port, char data);

// can return 0 if there is no data in buffer
char receive_data_from_uart(uart_control_t * port);

int rx_data_count(uart_control_t * port);

int tx_data_count(uart_control_t * port);

void flush_buffers(uart_control_t * port);

int __init uart_init(struct uart_driver *modbus_uart, struct platform_driver *modbus_serial_driver, uart_control_t *port);

void uart_destroy(struct uart_driver *modbus_uart, struct platform_driver *modbus_serial_driver, uart_control_t *port);

void uart_proc(uart_control_t *uart_control);

#endif /* UART_H_ */
