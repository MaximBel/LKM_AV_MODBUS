#ifndef _MODBUS_
#define _MODBUS_             

//-----------------------------------------------------------------------------
typedef enum {
	MD_NONE = 0x0, MD_START, MD_STOP, MD_PAUSE_START, MD_PAUSE_STOP
} ModeBus_Mode;
//-----------------------------------------------------------------------------
typedef enum {
	NO_ERROR = 0, FRAME_ERROR, START_ERROR, CRC_ERROR
} ModeBus_Error;

#define BUFFER_SIZE 128
//-----------------------------------------------------------------------------

typedef struct {
	uint32_t timer_start_ms;
	uint32_t timer_delay_ms;
	uint8_t started;
} timer;

typedef struct {
	ModeBus_Mode ModBusMode;
	unsigned char Addr;
	uint16_t ReInitTime;
	uint16_t CRC_pack;
	uint16_t TOPAUSE;
	uint16_t TOFRAME_ERR;
	ModeBus_Error Error;
	unsigned char Pack_Buffer[BUFFER_SIZE];
	unsigned char TimerWork;
	uint16_t Pack_Count;
	unsigned char Master;
	timer Pause_Timer;
	timer Frame_Timer;
	uint16_t Error_Frame_Count;
	void (*ChangeMode)(char);
	void (*putch)(char);
	char (*getch)(void);
	void (*flush)(void);
	int (*tx_count)(void);
	int (*rx_count)(void);
	void (*ProcPack)(unsigned char *, uint16_t);
	void (*SetParam)(uint16_t Addr, unsigned char *Val, uint16_t Size);
	uint16_t (*GetParam)(uint16_t Addr);
	uint16_t (*CheckAddrR)(uint16_t Addr);
	uint16_t (*CheckAddrW)(uint16_t Addr);
	void (*ReciveByte)(void *);
	void (*SendPackage)(void *, unsigned char *, unsigned char);
} ModBusPort;

//-----------------------------------------------------------------------------
void ModBusModeControl(ModBusPort * Int);
void ModBusInit(ModBusPort * Work,uint32_t Speed);

#endif
