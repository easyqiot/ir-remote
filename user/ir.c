#include <eagle_soc.h>
#include <c_types.h>
#include <gpio.h>

#include "ir.h"
#include "debug.h"


LOCAL uint32_t lasttime;

#define MS				1000
#define IR_TRIGGER_US	20000
#define IR_LEADER_US	13000
#define IR_DATA_SIZE	4
#define IR_HEADER_BITS	8
#define IR_DATA_BITS	32	
#define IR_FOOTER_BITS	2


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
};

LOCAL struct ir_session ir;

LOCAL void reset_data() {
	ir.cursor = 0;
	ir.current_byte = 0;
	memset(&ir.data[0], 0, IR_DATA_SIZE);
}


LOCAL void add_bit(uint8_t b) {
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


LOCAL void ir_intr_handler()
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
				reset_data();
				ir.status = IR_DATA;
			}
			break;
		case IR_DATA:
			if (ir.cursor > IR_DATA_BITS + IR_HEADER_BITS) {
				uint32_t *code = (uint32_t*)&ir.data[0];
				INFO("%u\r\n", *code);
				ir.status = IR_IDLE;
				return;
			}
			add_bit(timedelta/1000-1);
			break;
	}
 }



void interrupt_serv()
{
    s32 gpio_status;
    gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
    if( (gpio_status>>IR_NUM)& BIT0 ){
        ir_intr_handler();
    }  
    //add else if for other gpio intr task
    else{
        INFO("gpio num mismached   \n");
    }
}


void ICACHE_FLASH_ATTR
 irr_init()
{
    PIN_FUNC_SELECT(IR_MUX, IR_FUNC);
    GPIO_DIS_OUTPUT(IR_NUM);
    PIN_PULLUP_DIS(IR_MUX);
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(IR_NUM));
    gpio_pin_intr_state_set(IR_NUM, GPIO_PIN_INTR_NEGEDGE);
	ETS_GPIO_INTR_ATTACH(interrupt_serv, NULL);
    ETS_GPIO_INTR_ENABLE();
	ir.status = IR_IDLE;
	reset_data();
}

void ICACHE_FLASH_ATTR
 irr_disable()
{
    gpio_pin_intr_state_set(GPIO_ID_PIN(IR_NUM), GPIO_PIN_INTR_DISABLE);
}

void ICACHE_FLASH_ATTR
 irr_enable()
{
    gpio_pin_intr_state_set(GPIO_ID_PIN(IR_NUM), GPIO_PIN_INTR_NEGEDGE);
}

