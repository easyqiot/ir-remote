
#ifndef _IR_TEST_H
#define _IR_TEST_H

#include <gpio.h>
#include <eagle_soc.h>


// IR
#define IR_MUX			PERIPHS_IO_MUX_GPIO5_U
#define IR_NUM			5
#define IR_FUNC			FUNC_GPIO5


#define MS				1000
#define IR_TRIGGER_US	20000
#define IR_LEADER_US	13000
#define IR_DATA_SIZE	4
#define IR_HEADER_BITS	8
#define IR_DATA_BITS	32	
#define IR_FOOTER_BITS	2


typedef void (*IRCallback)(uint32_t);


enum ir_status {
	IR_IDLE,
	IR_LEADER,			// ~ 9 + 4 ms
	IR_DATA				// 72 bits
};


struct ir_session {
	enum ir_status status;
	uint8_t cursor;
	uint16_t noise;
	char data[IR_DATA_SIZE];
	char current_byte;
	IRCallback callback;
};



void irr_init();
void irr_intr_handler();
void irr_register_callback(IRCallback);
void irr_disable_for(uint16_t);
#endif
