#obj-m := av_monitor_modbus.o
#av_monitor_modbus-objs := modbus_main.o

EXTRA_CFLAGS += -O3 -std=gnu89 --no-warning
OBJS= modbus_main.o modbus_ui.o ring_buffer.o uart.o crc16.o ModBus.o 

TARGET=av_monitor_modbus

obj-m := $(TARGET).o
$(TARGET)-objs := $(OBJS)


all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

	$(TARGET).o:$(OBJS)
		$(LD) -r -o $@ $(OBJS)