#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/mutex.h>

#include "modbus_ui.h"
#include "ModBus.h"
#include "uart.h"

#define sEMPTY 0xFFFF // can't read or write register

#define MB_OWN_ADDRESS 0x10

#define MAX_REG_COUNT 52 // maximum count of registers that can be exported
#define REG_PERM_R 1 // permission mask to read 0b01
#define REG_PERM_W 2 // permission makkkk to write 0b10
#define KOBJ_DATA_EXPORTER 0 // export file index on kobj_data
#define KOBJ_DATA_UNEXPORTER 1 // unexport file index on kobj_data

typedef struct {
	struct kobj_attribute reg_attribute;
	uint8_t rw; // read/write permissions
	uint16_t regnum; // register number
	uint16_t regval; // register value
} reg_data_t;

static struct kobject *mb_kobject;
static volatile reg_data_t kobj_data[MAX_REG_COUNT];
static ModBusPort modbus_port;
static uart_control_t *uart_port;

// mutex to protect data write from mistakes
static struct mutex write_data_mutex;

// fops to registers files
static ssize_t file_show(struct kobject *kobj, struct kobj_attribute *attr,	char *buf);
static ssize_t file_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count);

static void Init_Reg(uint16_t regnum, uint8_t rw) ;
static uint8_t Deinit_Reg(uint16_t regnum) ;

static reg_data_t * find_empty_reg(void);
static reg_data_t * find_existed_reg(uint16_t regnum);
static void unexport_all_regs(void);

// bm driver operation functions
static uint16_t CheckAddrRead(ushort Addr);
static uint16_t CheckAddrWrite(ushort Addr);
static void SetParamOut(uint16_t Addr, unsigned char *data, uint16_t Cnt);
static uint16_t GetParamOut(uint16_t Addr);
static void mb_master_proc(unsigned char *Buff, uint16_t size);

// covers for mb driver
static void putchar_cover(char ch);
static char getchar_cover(void);
static void flush_cover(void);
static int tx_count_cover(void);
static int rx_count_cover(void);

static void __init ModBus_Protocol_Init(void);



int __init m_ui_init (uart_control_t * port) {
	int error = 0;

	if(!port) {
		printk(KERN_ERR "AV_MONITOR_MODBUS: uart port pointer is NULL! \n");
		return -EINVAL;
	}

	uart_port = port;

	//----------------

	mutex_init(&write_data_mutex);

	//-------------------

	ModBus_Protocol_Init();

	//---------------------

	mb_kobject = kobject_create_and_add("ModBus", kernel_kobj->parent);

	if(!mb_kobject) {
		printk(KERN_ERR "AV_MONITOR_MODBUS: can't create modbus kobject! \n");
		return -ENOMEM;
	}

	// init interface of registers exporting
	kobj_data[KOBJ_DATA_EXPORTER].reg_attribute.attr.name = "export";
	kobj_data[KOBJ_DATA_EXPORTER].reg_attribute.attr.mode = 0777;
	kobj_data[KOBJ_DATA_EXPORTER].reg_attribute.show = file_show;
	kobj_data[KOBJ_DATA_EXPORTER].reg_attribute.store = file_store;
	kobj_data[KOBJ_DATA_EXPORTER].regnum = 65535;
	error = sysfs_create_file(mb_kobject, &kobj_data[KOBJ_DATA_EXPORTER].reg_attribute.attr);

	if(error) {

		printk(KERN_ERR "AV_MONITOR_MODBUS: can't create modbus reg exporter! \n");

		goto file_create_error;
	}


	// init interface of registers unexporting
	kobj_data[KOBJ_DATA_UNEXPORTER].reg_attribute.attr.name = "unexport";
	kobj_data[KOBJ_DATA_UNEXPORTER].reg_attribute.attr.mode = 0777;
	kobj_data[KOBJ_DATA_UNEXPORTER].reg_attribute.show = file_show;
	kobj_data[KOBJ_DATA_UNEXPORTER].reg_attribute.store = file_store;
	kobj_data[KOBJ_DATA_UNEXPORTER].regnum = 65535;
	error = sysfs_create_file(mb_kobject, &kobj_data[KOBJ_DATA_UNEXPORTER].reg_attribute.attr);

	if(error) {

		sysfs_remove_file(mb_kobject, &kobj_data[KOBJ_DATA_EXPORTER].reg_attribute.attr);

		printk(KERN_ERR "AV_MONITOR_MODBUS: can't create modbus reg unexporter! \n");

		goto file_create_error;
	}

	printk(KERN_INFO "AV_MONITOR_MODBUS: ModBus and user interface initialised\r\n");

	return 0;

	//-----ERRORS-----------

	file_create_error:

	kobject_put(mb_kobject);

	return -ENOMEM;

}

void m_ui_exit (void) {

	unexport_all_regs();

	kobject_put(mb_kobject);
}

void m_ui_proc(void) {

	ModBusModeControl(&modbus_port);

}

static ssize_t file_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	int regnum = 0;
	reg_data_t * reg_data = 0;

	if(strcmp(attr->attr.name, kobj_data[KOBJ_DATA_EXPORTER].reg_attribute.attr.name) == 0 ||
			strcmp(attr->attr.name, kobj_data[KOBJ_DATA_UNEXPORTER].reg_attribute.attr.name) == 0) {

		return sprintf(buf, "no eny info here\r\n");

	}

	if(sscanf(attr->attr.name, "%d", &regnum)) {

		reg_data = find_existed_reg(regnum);

		if(reg_data) {

			return sprintf(buf, "%d\r\n", reg_data->regval);

		}

	}

	return 0;

}

static ssize_t file_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count) {
	int regnum = 0;
	reg_data_t * reg_data = 0;



	if(strcmp(attr->attr.name, kobj_data[KOBJ_DATA_EXPORTER].reg_attribute.attr.name) == 0) { //reg export

		mutex_lock(&write_data_mutex);

		int rw_perm = 0;

		sscanf(buf, "%d %d", &regnum, &rw_perm);

		Init_Reg(regnum, rw_perm);

		mutex_unlock(&write_data_mutex);

		return count;

	}

	if(strcmp(attr->attr.name, kobj_data[KOBJ_DATA_UNEXPORTER].reg_attribute.attr.name) == 0) { // reg unexport

		mutex_lock(&write_data_mutex);

		if(sscanf(buf, "%d", &regnum)) {

			Deinit_Reg(regnum);

		}

		mutex_unlock(&write_data_mutex);

		return count;

	}

	if(sscanf(attr->attr.name, "%d", &regnum)) {

		mutex_lock(&write_data_mutex);

		int reg_value = 0;

		reg_data = find_existed_reg(regnum);

		if(reg_data && sscanf(buf, "%d", &reg_value)) {

			reg_data->regval = (uint16_t)(reg_value & 0xFFFF);

		}

		mutex_unlock(&write_data_mutex);

		return count;

	}

	return 0;
}

static reg_data_t * find_empty_reg(void) {

	int i;

	for(i = 0; i < MAX_REG_COUNT; i++) {

		if(kobj_data[i].regnum == 0) {

			return &kobj_data[i];

		}

	}

	return 0;

}

static reg_data_t * find_existed_reg(uint16_t regnum) {

	int i;

	for(i = 0; i < MAX_REG_COUNT; i++) {

		if(kobj_data[i].regnum == regnum) {

			return &kobj_data[i];

		}

	}

	return 0;

}

static void unexport_all_regs(void) {
	int i;

	sysfs_remove_file(mb_kobject, &kobj_data[KOBJ_DATA_EXPORTER].reg_attribute.attr);
	sysfs_remove_file(mb_kobject, &kobj_data[KOBJ_DATA_UNEXPORTER].reg_attribute.attr);

	for(i = 2; i < MAX_REG_COUNT; i++) {

		if(kobj_data[i].regnum != 0) {

			sysfs_remove_file(mb_kobject, &kobj_data[i].reg_attribute.attr);

			kfree(kobj_data[i].reg_attribute.attr.name);

		}

	}

}

static void Init_Reg(uint16_t regnum, uint8_t rw) {
	int error = 0;

	reg_data_t * empty_reg = find_empty_reg();

	if (empty_reg == 0) {
		printk(KERN_INFO"AV_MONITOR_MODBUS: no more empty place to export register! \n");

		return;
	}

	if(rw > (REG_PERM_W | REG_PERM_R)) {

		printk(KERN_INFO"AV_MONITOR_MODBUS: bad permissions to register! \n");

		return;

	}

	empty_reg->reg_attribute.attr.name = kmalloc(7, GFP_KERNEL);

	sprintf(empty_reg->reg_attribute.attr.name, "%d", regnum);

	empty_reg->reg_attribute.attr.mode = 0777;
	empty_reg->reg_attribute.show = file_show;
	empty_reg->reg_attribute.store = file_store;
	empty_reg->rw = rw;
	empty_reg->regnum = regnum;
	empty_reg->regval = 0;

	error = sysfs_create_file(mb_kobject, &empty_reg->reg_attribute.attr);

	if (error)  {

		printk(KERN_INFO"AV_MONITOR_MODBUS: failed to create the register file \n");

	}

}

static uint8_t Deinit_Reg(uint16_t regnum) {
	int error = 0;

	reg_data_t * existed_reg = find_existed_reg(regnum);

	if (existed_reg == 0) {
		return 0;
	}

	existed_reg->regnum = 0;
	existed_reg->regval = 0;
	existed_reg->rw = 0;

	sysfs_remove_file(mb_kobject, &existed_reg->reg_attribute.attr);

	kfree(existed_reg->reg_attribute.attr.name);



	return 1;
}

static void __init ModBus_Protocol_Init(void) {

	modbus_port.ModBusMode = MD_NONE;
	modbus_port.Addr = MB_OWN_ADDRESS;
	modbus_port.CRC_pack = 0xffff;
	modbus_port.Error = NO_ERROR;
	modbus_port.putch = putchar_cover;
	modbus_port.getch = getchar_cover;
	modbus_port.flush = flush_cover;
	modbus_port.tx_count = tx_count_cover;
	modbus_port.rx_count = rx_count_cover;
	modbus_port.Master = 0; // slave
	modbus_port.ProcPack = mb_master_proc;
	modbus_port.CheckAddrR = CheckAddrRead;
	modbus_port.CheckAddrW = CheckAddrWrite;
	modbus_port.SetParam = SetParamOut;
	modbus_port.GetParam = GetParamOut;

	ModBusInit(&modbus_port, 115200);

}
//-----------------------------------------------------------------------------

//(ushort Addr, ushort data)
static void SetParamOut(uint16_t Addr, unsigned char *data, uint16_t Cnt) {
	uint16_t value;
	reg_data_t * reg_data;
	int i;
	mutex_lock(&write_data_mutex);

	for (i = 0; i < Cnt; i++, Addr++) {
		value = ((unsigned short) (data[i * 2]) << 8) + data[i * 2 + 1];

		reg_data = find_existed_reg(Addr);

		if (!reg_data) {
			continue;
		}

		reg_data->regval = value;


	}

	mutex_unlock(&write_data_mutex);
}

//-----------------------------------------------------------------------------
static uint16_t GetParamOut(uint16_t Addr) {
	reg_data_t * reg_data = 0;

	reg_data = find_existed_reg(Addr);

	if(!reg_data) {
		return 0;
	}

	return reg_data->regval;

}
//-----------------------------------------------------------------------------
static uint16_t CheckAddrRead(uint16_t Addr) {
	reg_data_t * reg_data = 0;

	reg_data = find_existed_reg(Addr);

	if (!reg_data) {
		return sEMPTY;
	}

	if ((reg_data->rw & REG_PERM_R)) {

		return !sEMPTY;

	}

	return sEMPTY;
}
//-----------------------------------------------------------------------------
static uint16_t CheckAddrWrite(uint16_t Addr) {
	reg_data_t * reg_data = 0;

	reg_data = find_existed_reg(Addr);

	if (!reg_data) {
		return sEMPTY;
	}

	if ((reg_data->rw & REG_PERM_W)) {

		return !sEMPTY;

	}

	return sEMPTY;
}

void mb_master_proc(unsigned char *Buff, uint16_t size) {


}


static void putchar_cover(char ch) {

	write_data_to_uart(uart_port, ch);

}

static char getchar_cover(void) {

	return receive_data_from_uart(uart_port);

}

static void flush_cover(void) {

	flush_buffers(uart_port);

}

static int tx_count_cover(void) {

	return tx_data_count(uart_port);

}

static int rx_count_cover(void) {

	return rx_data_count(uart_port);

}


EXPORT_SYMBOL(m_ui_init);
EXPORT_SYMBOL(m_ui_exit);
EXPORT_SYMBOL(m_ui_proc);
