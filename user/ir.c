#include <eagle_soc.h>
#include <c_types.h>
#include <gpio.h>

#include "ir.h"
#include "debug.h"


LOCAL uint32_t lasttime;
LOCAL ETSTimer suspend_timer;
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


void interrupt_dispatch()
{
    s32 gpio_status;
    gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
    if( (gpio_status>>IR_NUM)& BIT0 ){
        irr_intr_handler();
    }  
    else{
        INFO("gpio num mismached   \n");
    }
}

void irr_register_callback(IRCallback c) {
	ir.callback = c;
}


void irr_enable() {
	gpio_pin_intr_state_set(GPIO_ID_PIN(IR_NUM), 
			GPIO_PIN_INTR_NEGEDGE);
	LED_SET(OFF);
    os_timer_disarm(&suspend_timer);
}



void irr_disable_for(uint16_t ms) {
	LED_SET(ON);
	gpio_pin_intr_state_set(GPIO_ID_PIN(IR_NUM), 
			GPIO_PIN_INTR_DISABLE);
    os_timer_disarm(&suspend_timer);
    os_timer_setfn(&suspend_timer, (os_timer_func_t *)irr_enable, NULL);
    os_timer_arm(&suspend_timer, ms, 0);
}


void irr_init() {
	// LED
	PIN_FUNC_SELECT(LED_MUX, LED_FUNC);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_NUM), 1);
	
	// IR Diode
	PIN_FUNC_SELECT(IR_MUX, IR_FUNC);
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(IR_NUM));

	PIN_PULLUP_DIS(IR_MUX);
	ETS_GPIO_INTR_DISABLE();
	ETS_GPIO_INTR_ATTACH(interrupt_dispatch, NULL);
	gpio_pin_intr_state_set(GPIO_ID_PIN(IR_NUM), GPIO_PIN_INTR_NEGEDGE);
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(IR_NUM));
	ETS_GPIO_INTR_ENABLE();
}


