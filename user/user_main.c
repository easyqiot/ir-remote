
// Internal 
#include "partition.h"
#include "wifi.h"
#include "config.h"
#include "ir.h"

// SDK
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <mem.h>
#include <user_interface.h>
#include <driver/uart.h>
#include <upgrade.h>

// LIB: EasyQ
#include "easyq.h" 
#include "debug.h"


#define VERSION				"0.1.0"


/* GPIO */

// LED
#define LED_MUX			PERIPHS_IO_MUX_GPIO2_U
#define LED_NUM			2
#define LED_FUNC		FUNC_GPIO2

//#define LED_MUX			PERIPHS_IO_MUX_MTMS_U
//#define LED_NUM			14
//#define LED_FUNC		FUNC_GPIO14

#define ON	true
#define OFF false
#define LED_SET(on)	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_NUM), on)


LOCAL EasyQSession eq;

#define CMD_PLAY	0x04CFC17E
#define CMD_ON		0x04CFE25D
#define CMD_STOP	0x04CFD16E


void ir_cmd(uint32_t code) {
	switch (code) {
		case CMD_ON:
			INFO("ON/OFF\r\n");
			irr_disable_for(1000);
			break;
		case CMD_PLAY:
			INFO("Play\r\n");
			irr_disable_for(500);
			break;
		case CMD_STOP:
			INFO("Stop\r\n");
			break;
		default:
			INFO("Unknown Command: 0x%08X\r\n", code);
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

	// LED
	PIN_FUNC_SELECT(LED_MUX, LED_FUNC);
	GPIO_OUTPUT_SET(GPIO_ID_PIN(LED_NUM), 1);

	EasyQError err = easyq_init(&eq, EASYQ_HOSTNAME, EASYQ_PORT, EASYQ_LOGIN);
	if (err != EASYQ_OK) {
		ERROR("EASYQ INIT ERROR: %d\r\n", err);
		return;
	}
	eq.onconnect = easyq_connect_cb;
	eq.ondisconnect = easyq_disconnect_cb;
	eq.onconnectionerror = easyq_connection_error_cb;
	eq.onmessage = easyq_message_cb;
	
	irr_register_callback(ir_cmd);
	irr_init();
    WIFI_Connect(WIFI_SSID, WIFI_PSK, wifi_connect_cb);
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

