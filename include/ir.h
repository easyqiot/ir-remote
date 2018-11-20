
#ifndef _IR_TEST_H
#define _IR_TEST_H

#include <gpio.h>
#include <eagle_soc.h>


// IR
#define IR_MUX			PERIPHS_IO_MUX_GPIO5_U
#define IR_NUM			5
#define IR_FUNC			FUNC_GPIO5



void irr_init();
void irr_disable();
void irr_enable();

#endif
