#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/serial_core.h>
#include <linux/kdev_t.h>
#include <linux/platform_device.h>

#include "uart.h"
#include "modbus_ui.h"

#define PL011_MEM_BASE 0x3F201000
#define PL011_REG_SIZE 0x90

#define DEV_NUM 70618

//------------//

static int modbus_pratform_serial_probe(struct platform_device *pdev){	return 0;}
static int modbus_platform_serial_remove(struct platform_device* pdev) {	return 0;}

static void uart_port_stop_tx(struct uart_port *port) {}
static void uart_port_stop_rx(struct uart_port *port) {}
static void uart_port_enable_ms(struct uart_port *port) {}
static void uart_port_start_tx(struct uart_port *port){}
static unsigned int uart_port_tx_empty(struct uart_port *port) {	return 0;}
static unsigned int uart_port_get_mctrl(struct uart_port *port) {	return 0;}
static void uart_port_set_mctrl(struct uart_port *port, unsigned int mctrl) {}
static void uart_port_break_ctl(struct uart_port *port, int break_state) {}
static int uart_port_startup(struct uart_port *port) {	return 0;}
static void uart_port_shutdown(struct uart_port *port) {}
static const char *tiny_type(struct uart_port *port) {	return "modbus_tty";}
static void uart_port_release_port(struct uart_port *port) {}
static int uart_port_request_port(struct uart_port *port) {	return 0;}
static void uart_port_config_port(struct uart_port *port, int flags) {}
static int uart_port_verify_port(struct uart_port *port, struct serial_struct *ser) {	return 0;}

static struct uart_ops mb_uart_ops = {
	.tx_empty	= uart_port_tx_empty,
	.set_mctrl	= uart_port_set_mctrl,
	.get_mctrl	= uart_port_get_mctrl,
	.stop_tx	= uart_port_stop_tx,
	.start_tx	= uart_port_start_tx,
	.stop_rx	= uart_port_stop_rx,
	.enable_ms	= uart_port_enable_ms,
	.break_ctl	= uart_port_break_ctl,
	.startup	= uart_port_startup,
	.shutdown	= uart_port_shutdown,
	.type		= tiny_type,
	.release_port	= uart_port_release_port,
	.request_port	= uart_port_request_port,
	.config_port	= uart_port_config_port,
	.verify_port	= uart_port_verify_port,
};

static struct uart_driver modbus_uart = {
		.owner = THIS_MODULE,
		.driver_name = "ModBus_UART",
		.dev_name = "ttyMB",
		.major = MAJOR(DEV_NUM),
		.minor = 0, // because revision cant support only one driver
		.nr = 1,
		.cons = NULL, };

static struct platform_driver modbus_serial_driver = {
		.probe = modbus_pratform_serial_probe,
		.remove = modbus_platform_serial_remove,
		.driver = {
				.name =	"ModBus_USART_platform",
				.owner = THIS_MODULE,
		},
};

static uart_control_t uart_control = {
	.port = {
		.iotype = UPIO_MEM,
		.flags = UPF_BOOT_AUTOCONF,
		.ops = &mb_uart_ops,
		.fifosize = 1,
		.line = 0,		// temp
		//port.dev = &dev;// temp // no any devices
		.mapbase = PL011_MEM_BASE,
		.mapsize = PL011_REG_SIZE,
		//.irq = 87, // no any irq's
	},

};

// MB task handler
static struct task_struct * mb_thread = NULL;

static int mb_Task(void* Params);

//----------------------------------------------------------------------------------------


static int __init modbus_init(void) {
	int err = 0;

	err = uart_init(&modbus_uart, &modbus_serial_driver, &uart_control);

	if(err < 0) {

		return err;

	}

	err = m_ui_init(&uart_control);

	if(err < 0) {

		uart_destroy(&modbus_uart, &modbus_serial_driver, &uart_control);

		return err;

	}

	//-------------------//

	mb_thread = kthread_run(mb_Task, (void *)&uart_control, "ModBus UART thread");

	if(!mb_thread || mb_thread == ERR_PTR(-ENOMEM)) {

		m_ui_exit();

		uart_destroy(&modbus_uart, &modbus_serial_driver, &uart_control);

		printk(KERN_ERR "AV_MONITOR_MODBUS: Can't init programm thread\r\n");

		return -((int)mb_thread);

	}

	//--------------------//

return 0;
}

static void __exit modbus_exit(void) {

	if(mb_thread) {
		kthread_stop(mb_thread);
	}

	m_ui_exit();

	uart_destroy(&modbus_uart, &modbus_serial_driver, &uart_control);

}

//---STATIC---//

static int mb_Task(void* Params) {
	uart_control_t *uart_control = Params;

	while (!kthread_should_stop()) {

		uart_proc(uart_control); // uart driver

		m_ui_proc(); // modbus and user interface

		msleep(10);

	}

return 0;
}

//---------------//

module_init( modbus_init);
module_exit( modbus_exit);

MODULE_AUTHOR("Maxim Belei");
MODULE_DESCRIPTION("ModBus RTU LKM slave driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
