/*
 * uart.c
 *
 *  Created on: 24 May 2018
 *      Author: pi
 */

// UART driver for Raspberry Pi 3. For more details see tech, documentation to BCM2837 processor.

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <asm/io.h>
#include <linux/serial_core.h>
#include <linux/kdev_t.h>
#include <linux/platform_device.h>

#include "ring_buffer.h"
#include "uart.h"

#define REG_OFFS_DR 0x00 // DR register of UART
#define REG_OFFS_FR 0x18 // FR register of UART
#define REG_OFFS_LCRH 0x2C // LCRH register of UART

static void read_data_from_uart(uart_control_t * port);


void write_data_to_uart(uart_control_t * port, char data) {

	if (GetSpaceInRing(&port->tx_buffer) > 0) {

		InsertDataToRing(&port->tx_buffer, data);

	}

}

char receive_data_from_uart(uart_control_t * port) {

	if (GetDataCountInRing(&port->rx_buffer) > 0) {

		return GetDataFromRing(&port->rx_buffer);

	}

	return 0;
}

int rx_data_count(uart_control_t * port) {
	return GetDataCountInRing(&port->rx_buffer);
}

int tx_data_count(uart_control_t * port) {
	return GetDataCountInRing(&port->tx_buffer);
}

void flush_buffers(uart_control_t * port) {
	FlushBuffer(&port->tx_buffer);
	FlushBuffer(&port->rx_buffer);
}

int __init uart_init(struct uart_driver *modbus_uart, struct platform_driver *modbus_serial_driver, uart_control_t *port) {
	volatile uint32_t * base_addr;

	uart_register_driver(modbus_uart);

	platform_driver_register(modbus_serial_driver);

	// check that we can remap memory to work with uart
	// disabled checking, because we have already use this mem on BT driver? so check that we disabled BT by using overlay
	///if(!request_mem_region(0x3F201000, 0x90, "modbus_serial")) {
	///	printk(KERN_ERR "Cant requst memory!\r\n");
	///	return -1;
	///}

	// remap hard address or uart regs to kernel space
	base_addr = (volatile uint32_t *)ioremap(port->port.mapbase, port->port.mapsize);

	port->port.membase = (unsigned char __iomem *)base_addr;

	uart_add_one_port(modbus_uart, &port->port);

	//---enable FIFO mode---//

	uint32_t reg_temp;
	reg_temp = __raw_readl(port->port.membase + REG_OFFS_LCRH);
	__raw_writel(reg_temp | 0x10, port->port.membase + REG_OFFS_LCRH);

	//---init data buffers---//

	Init_Ring_Buffer(&port->rx_buffer);
	Init_Ring_Buffer(&port->tx_buffer);

	//-------------------//

	printk(KERN_INFO "AV_MONITOR_MODBUS: uart driver initialised\r\n");

	return 0;

}

void uart_destroy(struct uart_driver *modbus_uart, struct platform_driver *modbus_serial_driver, uart_control_t *port) {

	Destroy_Ring_Buffer(&port->rx_buffer);
	Destroy_Ring_Buffer(&port->tx_buffer);

	uart_remove_one_port(modbus_uart, &port->port);

	platform_driver_unregister(modbus_serial_driver);
	uart_unregister_driver(modbus_uart);

	iounmap((volatile uint32_t *)port->port.membase);

	printk(KERN_INFO "AV_MONITOR_MODBUS: uart driver unloaded\r\n");

}

void uart_proc(uart_control_t *uart_control) {

	if (!(__raw_readl(uart_control->port.membase + REG_OFFS_FR) & 0x10)) {

		read_data_from_uart(uart_control);

	}

	if (GetDataCountInRing(&uart_control->tx_buffer) > 0) {

		uint8_t counter;

		for (counter = 0; (counter < 20) && (GetDataCountInRing(&uart_control->tx_buffer) > 0) && !(__raw_readl(uart_control->port.membase + REG_OFFS_FR) & 0x20); counter++) {

			__raw_writel((uint32_t) GetDataFromRing(&uart_control->tx_buffer), uart_control->port.membase + REG_OFFS_DR);

		}

	}

	/*if (GetDataCountInRing(&uart_control->rx_buffer) > 0) {

		while ((GetDataCountInRing(&uart_control->rx_buffer) > 0)) {

			__raw_writel((uint32_t) GetDataFromRing(&uart_control->rx_buffer), uart_control->port.membase + REG_OFFS_DR);

		}


	}*/


}

//-----------------STATIC-----------------//

static void read_data_from_uart(uart_control_t * port) {

	while(!(__raw_readl(port->port.membase + REG_OFFS_FR) & 0x10) &&
			(GetSpaceInRing(&port->rx_buffer) > 0)) {

		char ch = (char) (__raw_readl(port->port.membase + REG_OFFS_DR) & 0xFF);

		InsertDataToRing(&port->rx_buffer, ch);

	}

}


//------------------------------------//


EXPORT_SYMBOL(write_data_to_uart);
EXPORT_SYMBOL(uart_init);
EXPORT_SYMBOL(uart_destroy);
