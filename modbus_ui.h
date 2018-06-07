/*
 * modbus_ui.h
 *
 *  Created on: 29 May 2018
 *      Author: pi
 */

#ifndef MODBUS_UI_H_
#define MODBUS_UI_H_

#include <linux/module.h>
#include "uart.h"

int __init m_ui_init(uart_control_t * port);

void m_ui_exit(void);

void m_ui_proc(void);

#endif /* MODBUS_UI_H_ */
