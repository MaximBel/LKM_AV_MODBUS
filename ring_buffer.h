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

// WARNING! See source file to know size of buffers


void Init_Ring_Buffer(struct circ_buf *buff);

void Destroy_Ring_Buffer(struct circ_buf *buff);

void InsertDataToRing(struct circ_buf *buff, char data);

char GetDataFromRing(struct circ_buf *buff);

u8 GetSpaceInRing(struct circ_buf *buff);

u8 GetDataCountInRing(struct circ_buf *buff);

void FlushBuffer(struct circ_buf *buff);

#endif /* RING_BUFFER_H_ */
