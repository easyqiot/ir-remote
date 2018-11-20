#include <eagle_soc.h>
#include <c_types.h>
#include <gpio.h>

#include "ir.h"
#include "debug.h"


LOCAL uint32_t lasttime;


LOCAL struct ir_session ir;

LOCAL void irr_reset_data() {
	ir.cursor = 0;
	ir.current_byte = 0;
	memset(&ir.data[0], 0, IR_DATA_SIZE);
}


LOCAL void irr_add_bit(uint8_t b) {
	if (ir.cursor < IR_HEADER_BITS) {
		ir.cursor++;
		return;
	}
	uint8_t pos = ir.cursor % 8;
	ir.current_byte |= b << (7-pos);
	if (pos == 7) {
		ir.data[3 - (ir.cursor-IR_HEADER_BITS) / 8] = ir.current_byte;
		ir.current_byte = 0;
	}
	ir.cursor++;
}


void irr_intr_handler()
{
    uint32_t gpio_status;
	uint32_t timedelta;
	uint32_t now;

    bool hi = GPIO_INPUT_GET(GPIO_ID_PIN(IR_NUM)) ;
	now = system_get_time();
    timedelta = ir.noise + (now - lasttime);
    lasttime = now;
	if (timedelta >= IR_TRIGGER_US) {
		ir.status = IR_LEADER;
		return;
	}
	else if (timedelta < MS) {
		ir.noise += timedelta;
		return;
	}
	ir.noise = 0;
	switch (ir.status) {
		case IR_LEADER:
			if (timedelta >= IR_LEADER_US) {
				irr_reset_data();
				ir.status = IR_DATA;
			}
			break;
		case IR_DATA:
			if (ir.cursor > IR_DATA_BITS + IR_HEADER_BITS) {
				uint32_t *code = (uint32_t*)&ir.data[0];
				if (ir.callback) {
					ir.callback(*code);
				}
				return;
			}
			irr_add_bit(timedelta/1000-1);
			break;
	}
 }


void irr_register_callback(IRCallback c) {
	ir.callback = c;
}

