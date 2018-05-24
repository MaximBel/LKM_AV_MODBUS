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



extern void write_data_to_uart(struct uart_port * port, unsigned char * data, uint8_t count);

extern void read_data_from_uart(struct uart_port * port, struct circ_buf * buffer);

extern int __init uart_init(struct uart_driver *modbus_uart, struct platform_driver *modbus_serial_driver, struct uart_port *port, struct task_struct * uart_thread);

extern void __exit uart_destroy(struct uart_driver *modbus_uart, struct platform_driver *modbus_serial_driver, struct uart_port *port, struct task_struct * uart_thread);

#endif /* UART_H_ */
