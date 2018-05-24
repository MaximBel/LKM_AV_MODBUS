/*
 * uart.c
 *
 *  Created on: 24 May 2018
 *      Author: pi
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/delay.h>

#include <linux/kthread.h>

//-------------------uart----------------//

#include <asm/io.h>
#include <linux/serial_core.h>
#include <linux/kdev_t.h>
#include <linux/platform_device.h>

#include "ring_buffer.h"

struct circ_buf modbus_uart_buffer; // size is 128 byte, see the ring_buffer.c

#define REG_OFFS_DR 0x00
#define REG_OFFS_FR 0x18
#define REG_OFFS_LCRH 0x2C

static uint8_t initState = 0; // there is one uart, so we can only init one driver.


void write_data_to_uart(struct uart_port * port, unsigned char * data, uint8_t count) {
	uint8_t counter;

	for(counter = 0; counter < count; counter++) {

		while(__raw_readl(port->membase + REG_OFFS_FR) & 0x20) {
			cpu_relax(); //maybe we can use sleep instead of this?
		}

		__raw_writel((uint32_t)data[counter], port->membase + REG_OFFS_DR);

	}

}

void read_data_from_uart(struct uart_port * port, struct circ_buf * buffer) {

	while(!(__raw_readl(port->membase + REG_OFFS_FR) & 0x10) &&
			(GetSpaceInRing(buffer) > 0)) {

		InsertDataToRing(buffer, (char) (__raw_readl(port->membase + REG_OFFS_DR) & 0xFF));

	}

}

static int uart_Task(void* Params) {
	struct uart_port *port = Params;

	while (!kthread_should_stop()) {

		if (!(__raw_readl(port->membase + REG_OFFS_FR) & 0x10)) {

			read_data_from_uart(port, &modbus_uart_buffer);

		}

		if (GetDataCountInRing(&modbus_uart_buffer) > 0) {

			int i, datacount;

			datacount = GetDataCountInRing(&modbus_uart_buffer);

			char * str = kmalloc(datacount, GFP_KERNEL);

			if (str != NULL) {

				for (i = 0; i < datacount; i++) {

					str[i] = GetDataFromRing(&modbus_uart_buffer);

				}

				write_data_to_uart(port, str, datacount);

				kfree(str);

			}

		}


		msleep(10);

	}

return 0;
}

int __init uart_init(struct uart_driver *modbus_uart, struct platform_driver *modbus_serial_driver, struct uart_port *port, struct task_struct * uart_thread) {

	if(initState == 1) {

		return -1; //convert this to linux error code

	}


	volatile uint32_t * base_addr;

	uart_register_driver(modbus_uart);
	platform_driver_register(modbus_serial_driver);

	// check that we can remap memory to work with uart
	// disabled checking, because we have already use this mem on BT driver? so check that we disabled BT by using overlay
	///if(!request_mem_region(0x3F201000, 0x90, "modbus_serial")) {
	///	printk(KERN_ERR "Cant requst memory!\r\n");
	///	return -1;
	///}

	base_addr = (volatile uint32_t *)ioremap(port->mapbase, port->mapsize);
	port->membase = (unsigned char __iomem *)base_addr;

	uart_add_one_port(modbus_uart, &port);

	//---enable FIFO mode---//

	uint32_t reg_temp;
	reg_temp = __raw_readl(port->membase + REG_OFFS_LCRH);
	__raw_writel(reg_temp | 0x10, port->membase + REG_OFFS_LCRH);

	//--------------------//

	Init_Ring_Buffer(&modbus_uart_buffer);

	//-------------------//

	uart_thread = kthread_run(uart_Task, (void *)port, "ModBus uart thread");

	if(!uart_thread || uart_thread == ERR_PTR(-ENOMEM)) {

		return -((int)uart_thread);

	}

	//--------------------//

	initState = 1;

	return 0;

}

void __exit uart_destroy(struct uart_driver *modbus_uart, struct platform_driver *modbus_serial_driver, struct uart_port *port, struct task_struct * uart_thread) {

	if(uart_thread) {
		kthread_stop(uart_thread);
	}

	Destroy_Ring_Buffer(&modbus_uart_buffer);

	uart_remove_one_port(modbus_uart, &port);

	platform_driver_unregister(modbus_serial_driver);
	uart_unregister_driver(modbus_uart);

	iounmap((volatile uint32_t *)port->membase);

	initState = 0;

}

EXPORT_SYMBOL(write_data_to_uart);

EXPORT_SYMBOL(read_data_from_uart);

EXPORT_SYMBOL(uart_init);

EXPORT_SYMBOL(uart_destroy);

//----------------------------------//
