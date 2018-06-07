#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/types.h>
#include <asm/div64.h>
#include <linux/timekeeping.h>

#include "crc16.h"
#include "ModBus.h"

#define IGNORE_FRAME_ERROR

#define TO_PAUSE(_sp_) (long)7*115200/(_sp_)
#define TO_FRAME(_sp_) (long)3*115200/(_sp_)

//-----------------------------------------------------------------------------
#define FORM_ERROR(_code_) {Send(Work,Work->Addr);\
                            Send(Work,Funct | 0x80 ); \
                            Send(Work,(_code_));\
                           }
//-----------------------------------------------------------------------------
// special for code 03
#define FORM_CODE_03(VAL,_MAX_) { if((VAL)<1 || (VAL)>(_MAX_)){\
                           FORM_ERROR(3); \
                        break;}\
                     }
#define FORM_CODE_02 {FORM_ERROR(2); break;}
//-----------------------------------------------------------------------------

#define REFRESH_TRGGER {}

#define READ_USER_REG \
                for (j=0; j<StCount; j++) \
                { \
                  Temp = Work->GetParam (StAddr+j); \
                  Send(Work,(Temp>>8)&0xff); \
                  Send(Work,Temp&0xff);      \
                }

#define WRITE_USER_REG \
                Work->SetParam (StAddr,  \
                                (unsigned char*)(Work->Pack_Buffer+7),\
                                StCount);

//-----------------------------------------------------------------------------
const char mID_LENGTH[4] = { 4, 16 + 2, 5, 6 };
const char ID_VALUE[4][25] = { "AV_PULT_MODBUS", "0x00", "1", "AV" };

//--------------------------------------timer functions---------------------------
static void timer_set(timer * tim, uint16_t delay);
static uint8_t timer_expired(timer * tim);
static void timer_stop(timer * tim);
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void Send(ModBusPort *Port, unsigned char val);
//-----------------------------------------------------------------------------
static void SendCRC(ModBusPort *Port);
//--------------------------------------------------------------------------
static void sendpack(void * p, unsigned char * buf, unsigned char count);
static void ModBusProcessing(ModBusPort *);
//-----------------------------------------------------------------------------
static void ReciveByte(void * Work);
//-----------------------------------------------------------------------------


void ModBusInit(ModBusPort *Work, uint32_t Speed) {
	Work->SendPackage = sendpack;
	Work->ReciveByte = ReciveByte;
	Work->TOPAUSE = TO_PAUSE(Speed);
#ifndef IGNORE_FRAME_ERROR
	Work->TOFRAME_ERR = TO_FRAME(Speed)*2;
#else
	Work->TOFRAME_ERR = TO_PAUSE(Speed) * 10;
#endif

}
//-----------------------------------------------------------------------------
void ModBusModeControl(ModBusPort *Int) {
#ifdef _DEBUG_ 
	uchar iii=0;
#endif  
	ModBusPort *Work = Int;
	switch (Work->ModBusMode) {
//-------------
	case MD_NONE:
		if (Work->rx_count() != 0) {
			Work->flush();
			break;
		}
		Work->ModBusMode = MD_PAUSE_START;
		timer_set(&Work->Pause_Timer, Work->TOPAUSE);
		break;
//-------------
	case MD_PAUSE_START:
		if (Work->rx_count() != 0)
			Work->ModBusMode = MD_NONE;
		if (!timer_expired(&Work->Pause_Timer)) {
			break;
		}
		Work->ModBusMode = MD_START;
		Work->Pack_Count = 0;
		//  break;
//-------------
	case MD_START:

		if (Work->Pack_Count && timer_expired(&Work->Pause_Timer)) {
			if (Work->Error_Frame_Count > 1) {
				Work->ModBusMode = MD_NONE;
				Work->Error = FRAME_ERROR;
			} else {
				Work->ModBusMode = MD_STOP;
			}
			Work->Error_Frame_Count = 0;
			timer_stop(&Work->Frame_Timer);
		}

		while (Work->rx_count()) {

			Work->Pack_Buffer[Work->Pack_Count] = Work->getch();
			Work->Pack_Count++;
			timer_set(&Work->Pause_Timer, Work->TOPAUSE);
		}

		if (Work->ModBusMode != MD_STOP)
			break;
//-------------
	case MD_STOP:

		if (CRC16m(Work->Pack_Buffer, Work->Pack_Count)) {
			Work->Error = NO_ERROR;

			Work->flush();
			if (Work->Master)
				Work->ProcPack(Work->Pack_Buffer, Work->Pack_Count);
			else {
				ModBusProcessing(Work);

#ifdef _DEBUG_               
				Debug_Param++;
#endif               
			}
		} else {
			Work->Error = CRC_ERROR;
		}
		Work->Pack_Count = 0;
		Work->ModBusMode = MD_NONE;
		break;
//-------------
	case MD_PAUSE_STOP:
		if (Work->tx_count() != 0)
			break;
		Work->ModBusMode = MD_NONE;
		break;
	};
}
//-----------------------------------------------------------------------------
static void timer_set(timer * tim, uint16_t delay) {

	if (tim == NULL)
		return;

	uint64_t time_ns = ktime_get_ns();

	tim->timer_start_ms = do_div(time_ns, 1000); // specific division for 64-bit number

	tim->timer_delay_ms = delay;

	tim->started = 1;

}

static uint8_t timer_expired(timer * tim) {

	if(tim->started == 0) {

		return 0;

	}


	uint64_t time_ns = ktime_get_ns();

	if ((do_div(time_ns, 1000) - tim->timer_start_ms) > tim->timer_delay_ms) {

		return 1;

	} else {

		return 0;

	}

}

static void timer_stop(timer * tim) {

	tim->started = 0;

}

//----------------------------------------------------------------------------


volatile ushort j;
//-----------------------------------------------------------------------------  
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------        
static void sendpack(void * Port, unsigned char * buf, unsigned char count) {
	if (!(ModBusPort *) Port)
		return;
	unsigned int i;
	((ModBusPort *) Port)->CRC_pack = 0xFFFF;
	for (i = 0; i < count; i++) {
		((ModBusPort *) Port)->putch(buf[i]);
		((ModBusPort *) Port)->CRC_pack = CRC16(((ModBusPort *) Port)->CRC_pack,
				buf[i]);
	}
	((ModBusPort *) Port)->putch(((ModBusPort *) Port)->CRC_pack & 0xFF);
	((ModBusPort *) Port)->putch((((ModBusPort *) Port)->CRC_pack >> 8) & 0xFF);
}
//-----------------------------------------------------------------------------
static void Send(ModBusPort *Port, unsigned char val) {
	Port->putch(val);
	Port->CRC_pack = CRC16(Port->CRC_pack, val);
}
//-----------------------------------------------------------------------------
static void SendCRC(ModBusPort *Port) {
	Port->putch(Port->CRC_pack & 0xFF);
	Port->putch((Port->CRC_pack >> 8) & 0xFF);

}
//-----------------------------------------------------------------------------
static void ModBusProcessing(ModBusPort *Work) {
	unsigned char TAddr = 0;
	unsigned char Funct = 0;
	uint16_t StAddr = 0;
	uint16_t StCount = 0;
	unsigned char cTemp = 0;
	uint16_t Temp = 0;

	Work->CRC_pack = 0xFFFF;
	TAddr = Work->Pack_Buffer[0];
	Funct = Work->Pack_Buffer[1];
	StAddr = ((uint16_t) (Work->Pack_Buffer[2]) << 8) + Work->Pack_Buffer[3];

	StCount = ((uint16_t) (Work->Pack_Buffer[4]) << 8) + Work->Pack_Buffer[5];

	if (TAddr != 0 && TAddr != Work->Addr)
		return;
	if (TAddr == 0 && Funct != 16)
		return;
	if (Funct & 0x80)
		return;

	switch (Funct) { //function NUM
// read output status
// func 1 unavailable

// read input status
// func 2 unavailable

//Read input register
// func 4 unavailable
//Read holding register

	case 3: {
		FORM_CODE_03(StCount, 0x7D);
		for (j = 0; j < StCount; j++) {

			if (Work->CheckAddrR(StAddr + j) == 0xFFFF)
				break;
		}
		if (j != StCount)
			FORM_CODE_02
		;

		Send(Work, Work->Addr);
		Send(Work, Funct);
		Send(Work, (StCount << 1));

		READ_USER_REG
		;

		break;
	}
// 5 -23 - write

// Write single coil
// func 5 unavailable

//write single register
// func 6 unavailable

// write multiple coil
// func 15 unavailable

// Write multiple register
	case 16: {

		FORM_CODE_03(StCount & 0xff, 0x007B);

		for (j = 0; j < StCount; j++) {
			if (Work->CheckAddrW(StAddr + j) == 0xFFFF)
				break;
		}
		if (j != StCount) {
			if (TAddr == 0)
				return;
			FORM_CODE_02
			;
		}

		WRITE_USER_REG
		;

		if (TAddr == 0)
			return;
		Send(Work, Work->Addr);
		Send(Work, Funct);
		Send(Work, (StAddr >> 8) & 0xFF);
		Send(Work, StAddr & 0xFF);
		Send(Work, (StCount >> 8) & 0xFF);
		Send(Work, StCount & 0xFF);

		break;
	}
//Mask write
// func 22 unavailable

// Read and Write multiple register
// func 23 unavailable

// Edentification 
	case 43: {
		StAddr = Work->Pack_Buffer[2];  //MEI type
		StCount = Work->Pack_Buffer[3]; //Dev ID code
		cTemp = Work->Pack_Buffer[4]; //Object ID
		if (cTemp > 4) {
			FORM_ERROR(2);
			break;
		}
		if (StCount != 1) {
			FORM_ERROR(3);
			break;
		}
		Send(Work, Work->Addr);
		Send(Work, Funct);
		Send(Work, 0x0E); // MEI
		Send(Work, 0x01); // Read dev code
		Send(Work, 0x01); // Conform. level
		Send(Work, ((cTemp == 3) ? 0x0 : 0xff)); // More Follows? 0-not, ff-yes
		Send(Work, ((cTemp == 3) ? 0x0 : cTemp + 1)); // Next Object
		Send(Work, 4);	       // Number of object
		Send(Work, cTemp); // Object Id
		if (cTemp < 3) {
			Send(Work, mID_LENGTH[cTemp]); // length Id
			for (j = 0; j < mID_LENGTH[cTemp]; j++) {
				Send(Work, ID_VALUE[cTemp][j]);
			}
			break;
		} else {
			Send(Work, 6); // length Id
			Send(Work, 0);
			Send(Work, 0);
			Send(Work, 0);
			Send(Work, 0);
			Send(Work, 0);
			Send(Work, 0);
			break;
		}
	}

	default: {
		FORM_ERROR(0x01);
	}
	}
	if (TAddr != 0)
		SendCRC(Work);

}

static void ReciveByte(void * Work) {
	if (timer_expired(&((ModBusPort *)Work)->Frame_Timer)) {

		((ModBusPort *)Work)->Error_Frame_Count++;
	}
	timer_set(&((ModBusPort *)Work)->Frame_Timer, ((ModBusPort *)Work)->TOFRAME_ERR);
	timer_set(&((ModBusPort *)Work)->Pause_Timer, ((ModBusPort *)Work)->TOPAUSE);

}

EXPORT_SYMBOL(ModBusModeControl);
EXPORT_SYMBOL(ModBusInit);
