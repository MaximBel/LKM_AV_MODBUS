#ifndef _MODBUS_
#define _MODBUS_             

extern ModBusPort av_modbus_init_struct;

//-----------------------------------------------------------------------------
extern void ModBusModeControl(ModBusPort *);
extern void ModBusInit(ModBusPort *,uint32_t Speed);

#endif
