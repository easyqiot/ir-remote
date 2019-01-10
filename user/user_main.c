
// Internal 
#include "partition.h"
#include "wifi.h"
#include "io_config.h"
#include "ir.h"
#include "params.h"

// SDK
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <mem.h>
#include <user_interface.h>
#include <driver/uart.h>
#include <upgrade.h>
#include <string.h>
#include <osapi.h>

// LIB: EasyQ
#include "easyq.h" 
#include "debug.h"


#define VERSION		"0.1.0"


/* GPIO */


static EasyQSession eq;
static ETSTimer wakeup_timer;
static uint32_t last_code;
static uint32_t last_time;
static Params params;


typedef struct {
	uint32_t code;
	bool repeat;
} IREvent;


void wokenup(void *arg) {
	char a[32];
	IREvent *e = (IREvent*) arg;
	switch (e->code) {
		case CMD_ON:
			INFO("ON/OFF\r\n");
			irr_disable_for(500);
			easyq_push(&eq, AMP_SUPPLY_QUEUE, "toggle");
			break;

		case CMD_PLAY:
			INFO("Play\r\n");
			irr_disable_for(100);
			break;

		case CMD_STOP:
			INFO("Stop\r\n");
			irr_disable_for(500);
			break;

		case CMD_VOLUP:
			if (e->repeat) {
				irr_disable_for(10);
			}
			else {
				irr_disable_for(5);
			}
			easyq_push(&eq, AMP_VOLUME_QUEUE, 
					e->repeat? "+"VOLUME_REPEAT_STEP: "+"VOLUME_STEP
				);
			break;

		case CMD_VOLDOWN:
			if (e->repeat) {
				irr_disable_for(10);
			}
			else {
				irr_disable_for(5);
			}
			easyq_push(&eq, AMP_VOLUME_QUEUE, 
					e->repeat? "-"VOLUME_REPEAT_STEP: "-"VOLUME_STEP
				);
			break;

		case CMD_HUMIDIFIER:
			irr_disable_for(500);
			easyq_push(&eq, HUMIDIFIER_QUEUE, "toggle");
			break;

		case CMD_HUMIDIFIER_LIGHT:
			irr_disable_for(500);
			easyq_push(&eq, HUMIDIFIER_LIGHT_QUEUE, "toggle");
			break;

		default:
			os_sprintf(&a[0], "Unknown Command: 0x%08X", e->code);
			easyq_push(&eq, IR_QUEUE, a);
			//INFO("Unknown Command: 0x%08X\r\n", code);
	}
	os_free(e);
}


void ir_cmd(uint32_t code) {
	uint32_t time = system_get_time();
	IREvent *e = (IREvent*) os_zalloc(sizeof(IREvent));
	e->repeat = (code == last_code) && ((time - last_time) < 300000);
	e->code = code;
	last_code = code;
	last_time = time;
	
	if (e->repeat) {
		wokenup(e);
	}
	else {
		os_timer_disarm(&wakeup_timer);
		// Wait for system to wake up.
		os_timer_setfn(&wakeup_timer, (os_timer_func_t *)wokenup, e);
		os_timer_arm(&wakeup_timer, 5, 0);
	}
}


void
fota_report_status(const char *q) {
	char str[50];
	float vdd = system_get_vdd33() / 1024.0;

	uint8_t image = system_upgrade_userbin_check();
	os_sprintf(str, "Image: %s Version: "VERSION" VDD: %d.%03d", 
			(UPGRADE_FW_BIN1 == image)? "FOTA": "APP",
			(int)vdd, 
			(int)(vdd*1000)%1000
		);
	easyq_push(&eq, q, str);
}


void ICACHE_FLASH_ATTR
easyq_message_cb(void *arg, const char *queue, const char *msg, 
		uint16_t message_len) {
	if (strcmp(queue, FOTA_QUEUE) == 0) {
		if (msg[0] == 'R') {
			INFO("Rebooting to FOTA ROM\r\n");
			system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
			system_upgrade_reboot();
		}
		else if (msg[0] == 'I') {
			fota_report_status(FOTA_STATUS_QUEUE);
		}
	}
}


void ICACHE_FLASH_ATTR
easyq_connect_cb(void *arg) {
	INFO("EASYQ: Connected to %s:%d\r\n", eq.hostname, eq.port);
	INFO("\r\n***** InfraRed Receiver "VERSION"****\r\n");
	const char * queues[] = {FOTA_QUEUE};
	easyq_pull_all(&eq, queues, 1);
}


void ICACHE_FLASH_ATTR
easyq_connection_error_cb(void *arg) {
	EasyQSession *e = (EasyQSession*) arg;
	INFO("EASYQ: Connection error: %s:%d\r\n", e->hostname, e->port);
	INFO("EASYQ: Reconnecting to %s:%d\r\n", e->hostname, e->port);
}


void easyq_disconnect_cb(void *arg)
{
	EasyQSession *e = (EasyQSession*) arg;
	INFO("EASYQ: Disconnected from %s:%d\r\n", e->hostname, e->port);
	easyq_delete(&eq);
}


void setup_easyq() {
	EasyQError err = \
			easyq_init(&eq, params.easyq_host, EASYQ_PORT, EASYQ_LOGIN);
	if (err != EASYQ_OK) {
		ERROR("EASYQ INIT ERROR: %d\r\n", err);
		return;
	}
	eq.onconnect = easyq_connect_cb;
	eq.ondisconnect = easyq_disconnect_cb;
	eq.onconnectionerror = easyq_connection_error_cb;
	eq.onmessage = easyq_message_cb;
}


void wifi_connect_cb(uint8_t status) {
    if(status == STATION_GOT_IP) {
        easyq_connect(&eq);
    } else {
        easyq_disconnect(&eq);
    }
}


void user_init(void) {
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
	bool ok = params_load(&params);
	if (!ok) {
		ERROR("Cannot load Params\r\n");
		system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
		system_upgrade_reboot();
		return;
	}
	INFO("Params loaded sucessfully: ssid: %s psk: %s easyq: %s\r\n",
			params.wifi_ssid, 
			params.wifi_psk,
			params.easyq_host
		);
	setup_easyq();
	irr_register_callback(ir_cmd);
	irr_init();
    wifi_connect(params.wifi_ssid, params.wifi_psk, wifi_connect_cb);
    INFO("System started ...\r\n");
}


void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, 
				sizeof(at_partition_table)/sizeof(at_partition_table[0]),
				SPI_FLASH_SIZE_MAP)) {
		FATAL("system_partition_table_regist fail\r\n");
		while(1);
	}
}

