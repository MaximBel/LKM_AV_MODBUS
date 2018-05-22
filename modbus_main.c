#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/slab.h>
#include <linux/gfp.h>

#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#include <linux/delay.h>

//-------------UART-----------------
#include <asm/io.h>
#include <linux/serial_core.h>
#include <linux/kdev_t.h>
#include <linux/platform_device.h>

#define PL011_MEM_BASE 0x3F201000
#define PL011_REG_SIZE 0x90

#define REG_OFFS_DR 0x00
#define REG_OFFS_FR 0x18
#define REG_OFFS_LCRH 0x2C

static struct uart_driver modbus_uart = {
		.owner = THIS_MODULE,
		.driver_name = "ModBus_UART",
		.dev_name = "ttyMB",
		.major = MAJOR(22),
		.minor = MINOR(22),
		.nr = 1,
		.cons = NULL, };

static struct uart_ops modbus_uart_ops;


struct uart_port port;

static int modbus_serial_probe(struct platform_device *pdev){	return 0;}

static int modbus_serial_remove(struct platform_device* pdev) {	return 0;}

static struct platform_driver modbus_serial_driver = {
		.probe = modbus_serial_probe,
		.remove = modbus_serial_remove,
		.driver = {
				.name =	"ModBus_USART_platform",
				.owner = THIS_MODULE,
		},
};

static struct device dev;

static void tiny_stop_tx(struct uart_port *port){}
static void tiny_stop_rx(struct uart_port *port){}
static void tiny_enable_ms(struct uart_port *port){}
static void tiny_tx_chars(struct uart_port *port){}
static void tiny_start_tx(struct uart_port *port){}
static void tiny_timer (unsigned long data){}
static unsigned int tiny_tx_empty(struct uart_port *port){	return 0;}
static unsigned int tiny_get_mctrl(struct uart_port *port) {	return 0;}
static void tiny_set_mctrl(struct uart_port *port, unsigned int mctrl){ }
static void tiny_break_ctl(struct uart_port *port, int break_state){ }
static void tiny_change_speed(struct uart_port *port, unsigned int cflag, unsigned int iflag, unsigned int quot){}
static int tiny_startup(struct uart_port *port){	return 0;}
static void tiny_shutdown(struct uart_port *port){}
static const char *tiny_type(struct uart_port *port){	return "modbus_tty";}
static void tiny_release_port(struct uart_port *port){}
static int tiny_request_port(struct uart_port *port){	return 0;}
static void tiny_config_port(struct uart_port *port, int flags){}
static int tiny_verify_port(struct uart_port *port, struct serial_struct *ser){	return 0;}

static struct uart_ops mb_uart_ops = {
	.tx_empty	= tiny_tx_empty,
	.set_mctrl	= tiny_set_mctrl,
	.get_mctrl	= tiny_get_mctrl,
	.stop_tx	= tiny_stop_tx,
	.start_tx	= tiny_start_tx,
	.stop_rx	= tiny_stop_rx,
	.enable_ms	= tiny_enable_ms,
	.break_ctl	= tiny_break_ctl,
	.startup	= tiny_startup,
	.shutdown	= tiny_shutdown,
	//.change_speed	= tiny_change_speed,
	.type		= tiny_type,
	.release_port	= tiny_release_port,
	.request_port	= tiny_request_port,
	.config_port	= tiny_config_port,
	.verify_port	= tiny_verify_port,
};

void write_data_to_uart(unsigned char * data, uint8_t count) {
	uint8_t counter;

	for(counter = 0; counter < count; counter++) {

		while(__raw_readl(port.membase + REG_OFFS_FR) & 0x20) {
			cpu_relax(); //maybe we can use sleep instead of this?
		}

		__raw_writel((uint32_t)data[counter], port.membase + REG_OFFS_DR);

	}

}

int __init uart_init(void) {

	volatile uint32_t * base_addr;

	uart_register_driver(&modbus_uart);
	platform_driver_register(&modbus_serial_driver);

	// check that we can remap memory to work with uart
	// disabled checking, because we have already use this mem on BT driver? so check that we disabled BT by using overlay
	///if(!request_mem_region(0x3F201000, 0x90, "modbus_serial")) {
	///	printk(KERN_ERR "Cant requst memory!\r\n");
	///	return -1;
	///}

	base_addr = (volatile uint32_t *)ioremap(PL011_MEM_BASE, PL011_REG_SIZE);

	port.iotype = UPIO_MEM;
	port.flags = UPF_BOOT_AUTOCONF;
	port.ops = &mb_uart_ops;
	port.fifosize = 1;
	port.line = 0;// temp
	//port.dev = &dev;// temp
	port.mapbase = PL011_MEM_BASE;
	port.mapsize = PL011_REG_SIZE;
	port.membase = (unsigned char __iomem *)base_addr;
	port.irq = 87;

	uart_add_one_port(&modbus_uart, &port);

	//---enable FIFO mode---//

	uint32_t reg_temp = __raw_readl(port.membase + REG_OFFS_LCRH);
	__raw_writel(reg_temp | 0x10, port.membase + REG_OFFS_LCRH);

	//--------------------//

	return 0;

}

void __exit uart_destroy(void) {

	uart_remove_one_port(&modbus_uart, &port);

	platform_driver_unregister(&modbus_serial_driver);
	uart_unregister_driver(&modbus_uart);

	iounmap((volatile uint32_t *)port.membase);


}


//----------------------------------




static struct task_struct * usThread = NULL;

static int us_Task(void* Params) {

return -1;
}

//----------------------------------------------------------------------------------------

static int __init modbus_init(void) {

	uart_init();

return 0;
}

static void __exit modbus_exit(void) {

	uart_destroy();


}

module_init( modbus_init);
module_exit( modbus_exit);

MODULE_AUTHOR("Maxim Belei");
MODULE_DESCRIPTION("ModBus RTU LKM driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
