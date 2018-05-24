obj-m := modbus_main.o
av_monitor_modbus-objs := modbus_main.o

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
