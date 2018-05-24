/*
 * ring_buffer.h
 *
 *  Created on: 24 May 2018
 *      Author: pi
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <linux/circ_buf.h>
#include <linux/kernel.h>
#include <linux/module.h>


extern void __init Init_Ring_Buffer(struct circ_buf *buff);

extern void __exit Destroy_Ring_Buffer(struct circ_buf *buff);

extern void InsertDataToRing(struct circ_buf *buff, char data);

extern char GetDataFromRing(struct circ_buf *buff);

extern u8 GetSpaceInRing(struct circ_buf *buff);

extern u8 GetDataCountInRing(struct circ_buf *buff);

#endif /* RING_BUFFER_H_ */
