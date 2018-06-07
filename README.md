# LKM_AV_MODBUS
Modbus RTU LKM for RPi3

Uart baud: 115200.

ModBus address: 0x10

Notice: to work with uart You should disable bluetooth module(include appropriate overlay file) and disable console shell in raspi-config

User manual:

To use this module you should load it to kernel with command insmod. 
Then, if everything ok, in the root directory of sysfs must be created a new folder, named ModBus. 
In this folder you can see two files: export and unexport. 
To make some registers be able to use, you should write to export file reg number and rw permissions. 
If export was seccessfull, in ModBus folder you will see new file named like register number. To read or write register value, read or write to its file.
To unregister it, write to unexport file reg number.

Export exapmle:

sudo echo "30 3" > export --> means we export 30 register with permission 1 + 2(read + write).

Unexport example:

sudo echo "30" > unexport --> means we unexport 30 register
