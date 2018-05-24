/*
 * ring_buffer.c
 *
 *  Created on: 24 May 2018
 *      Author: pi
 */

//-----------ring buffer-------//

#include <linux/circ_buf.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define OUT_BUF_SIZE 128 // ACHTUNG!!! Must be the digit with all 1 in binary plus 1(example: 0b01111111 + 1 -> 0b10000000)

void __init Init_Ring_Buffer(struct circ_buf *buff) {

	buff->buf = (char *) kmalloc(OUT_BUF_SIZE, GFP_KERNEL);
	buff->head = 0;
	buff->tail = 0;

}

void __exit Destroy_Ring_Buffer(struct circ_buf *buff) {

	kfree((void *)buff->buf);

}

void InsertDataToRing(struct circ_buf *buff, char data) {

	unsigned long head = buff->head;
	unsigned long tail = ACCESS_ONCE(buff->tail);

	if (CIRC_SPACE(head, tail, OUT_BUF_SIZE) >= 1) {

		buff->buf[head] = data;

		buff->head = (head + 1) & (OUT_BUF_SIZE - 1);

	}

}

char GetDataFromRing(struct circ_buf *buff) {

	char outputData = 0;
	unsigned long head = ACCESS_ONCE(buff->head);
	unsigned long tail = buff->tail;

	if (CIRC_CNT(head, tail, OUT_BUF_SIZE) >= 1) {

		outputData = buff->buf[tail];

		buff->tail = (tail + 1) & (OUT_BUF_SIZE - 1);

	}

	return outputData;

}

u8 GetSpaceInRing(struct circ_buf *buff) {

	return CIRC_SPACE(buff->head, ACCESS_ONCE(buff->tail), OUT_BUF_SIZE);

}

u8 GetDataCountInRing(struct circ_buf *buff) {

	return CIRC_CNT(ACCESS_ONCE(buff->head),
			ACCESS_ONCE(buff->tail), OUT_BUF_SIZE);

}

EXPORT_SYMBOL(Init_Ring_Buffer);
EXPORT_SYMBOL(Destroy_Ring_Buffer);
EXPORT_SYMBOL(InsertDataToRing);
EXPORT_SYMBOL(GetDataFromRing);
EXPORT_SYMBOL(GetSpaceInRing);
EXPORT_SYMBOL(GetDataCountInRing);

//------------------------------------//
