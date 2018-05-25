/**
  ******************************************************************************
  * @file    crc16.h
  * @author
  * @version
  * @date
  * @brief   Header file of CRC16.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 LOT Ltd.</center></h2>
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CRC16__
#define __CRC16__

extern uint16_t CRC16(uint16_t oldCRC, uint8_t val);
extern uint16_t CRC16_buf(uint16_t oldCRC, uint8_t *val, uint16_t DataLen);
extern uint8_t CRC16m (uint8_t *Msg, uint16_t DataLen);
extern uint16_t CRC16short (uint8_t *Msg, uint16_t DataLen);



#endif	/* __CRC16__ */

/************************ (C) COPYRIGHT LOT Ltd. *********END OF FILE**************/


