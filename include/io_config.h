#ifndef _IO_CONFIG_H__
#define _IO_CONFIG_H__


#define EASYQ_RECV_BUFFER_SIZE  4096
#define EASYQ_SEND_BUFFER_SIZE  512 
#define EASYQ_PORT				1085

#define EASYQ_LOGIN				"ir"
#define STATUS_QUEUE			EASYQ_LOGIN":status"
#define FOTA_QUEUE				EASYQ_LOGIN":fota"
#define FOTA_STATUS_QUEUE		EASYQ_LOGIN":fota:status"

#define VOLUME_STEP				"32"
#define VOLUME_REPEAT_STEP		"16"

#define IR_QUEUE				EASYQ_LOGIN
#define AMP_SUPPLY_QUEUE		"amp:supply"
#define AMP_VOLUME_QUEUE		"amp:volume"
#define HUMIDIFIER_QUEUE		"humidifier"
#define HUMIDIFIER_LIGHT_QUEUE	HUMIDIFIER_QUEUE":light"

#define CMD_PLAY				0x04CFC17E
#define CMD_ON					0x04CFE25D
#define CMD_STOP				0x04CFD16E
#define CMD_VOLUP				0x04CFE659
#define CMD_VOLDOWN				0x04CFD669
#define CMD_HUMIDIFIER			0x04CFCC73
#define CMD_HUMIDIFIER_LIGHT	0x04CFD12E

// LED
#define LED_MUX			PERIPHS_IO_MUX_GPIO0_U
#define LED_NUM			0
#define LED_FUNC		FUNC_GPIO0

// IR
#define IR_MUX			PERIPHS_IO_MUX_GPIO2_U
#define IR_NUM			2
#define IR_FUNC			FUNC_GPIO2


#endif

