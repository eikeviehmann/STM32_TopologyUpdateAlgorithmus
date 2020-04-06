#ifndef RF24_MODULE
#define RF24_MODULE

#include "stm32f1xx.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "../rf24_stm32f1xx/rf24_stm32f1xx.h"

#define RF24_USART_MESSAGES

#ifdef DEBUG
	//#define RF24_MODULE_DEBUG_IRQ
	//#define RF24_MODULE_DEBUG_STATE_MACHINE
#endif


//_________________________________________________________________________________________________________________________________________________________________________
// TYPE & ENUM DEFINITIONS

typedef enum {
	_config = 		0x00,
	_en_aa = 		0x01,
	_en_rxaddr = 	0x02,
	_setup_aw = 	0x03,
	_setup_retr = 	0x04,
	_rf_ch = 		0x05,
	_rf_setup = 	0x06,
	_status = 		0x07,
	_observe_tx = 	0x08,
	_rpd = 			0x09,
	_rx_addr_p0 = 	0x0A,
	_rx_addr_p1 = 	0x0B,
	_rx_addr_p2 = 	0x0C,
	_rx_addr_p3 = 	0x0D,
	_rx_addr_p4 = 	0x0E,
	_rx_addr_p5 = 	0x0F,
	_tx_addr = 		0x10,
	_rx_pw_p0 = 	0x11,
	_rx_pw_p1 = 	0x12,
	_rx_pw_p2 = 	0x13,
	_rx_pw_p3 = 	0x14,
	_rx_pw_p4 = 	0x15,
	_rx_pw_p5 = 	0x16,
	_fifo_status = 	0x17,
	_dynpd = 		0x1C,
	_feature =		0x1D
} rf24_module_register_mnemonics;

static const char *rf24_module_register_names[] = { "config", "en_aa", "en_rxaddr", "setup_aw", "setup_retr", "rf_ch", "rf_setup", "status", "observe_tx", "rpd", "rx_addr_p0", "x_addr_p1", "rx_addr_p2", "rx_addr_p3", "rx_addr_p4", "rx_addr_p5", "tx_addr", "rx_pw_p0", "rx_pw_p1", "rx_pw_p2", "rx_pw_p3", "rx_pw_p4", "rx_pw_p5", "fifo_status", "dynpd", "feature" };

typedef enum {
	read = 0,
	write = 1
} rf24_module_read_write;


typedef enum {
	UNDEFINED = 0,
	POWER_ON_RESET,
	POWER_DOWN,
	START_UP,
	STAND_BY_I,
	STAND_BY_II,
	RX_SETTING,
	RX_MODE,
	TX_SETTING,
	TX_MODE,
} rf24_module_states;

static const char *rf24_module_states_string[] = { "UNDEFINED", "POWER_ON_RESET", "POWER_DOWN", "START_UP", "STAND_BY_I", "STAND_BY_II", "RX_SETTING", "RX_MODE", "TX_SETTING", "TX_MODE"};

typedef enum {
	rf24_module_crc_length_1byte = 0,
	rf24_module_crc_length_2bytes = 1
}
rf24_module_crc_length;

typedef enum {
	rf24_module_rx_pipe0 = 0,
	rf24_module_rx_pipe1 = 1,
	rf24_module_rx_pipe2 = 2,
	rf24_module_rx_pipe3 = 3,
	rf24_module_rx_pipe4 = 4,
	rf24_module_rx_pipe5 = 5
}
rf24_module_rx_pipes;

typedef enum {
	rf24_module_address_width_offset = 	2,
	rf24_module_address_width_3bytes = 	3,
	rf24_module_address_width_bytes = 	4,
	rf24_module_address_width_5bytes = 	5,
}
rf24_module_address_width;

typedef enum {
	rf24_module_autoretransmit_delay_250us = 	0,
	rf24_module_autoretransmit_delay_500us = 	1,
	rf24_module_autoretransmit_delay_750us = 	2,
	rf24_module_autoretransmit_delay_1000us = 	3,
	rf24_module_autoretransmit_delay_1250us = 	4,
	rf24_module_autoretransmit_delay_1500us = 	5,
	rf24_module_autoretransmit_delay_1750us = 	6,
	rf24_module_autoretransmit_delay_2000us = 	7,
	rf24_module_autoretransmit_delay_2250us = 	8,
	rf24_module_autoretransmit_delay_2500us = 	9,
	rf24_module_autoretransmit_delay_2750us = 	10,
	rf24_module_autoretransmit_delay_3000us = 	11,
	rf24_module_autoretransmit_delay_3250us = 	12,
	rf24_module_autoretransmit_delay_3500us = 	13,
	rf24_module_autoretransmit_delay_3750us = 	14,
	rf24_module_autoretransmit_delay_4000us = 	15,
}
rf24_module_autoretransmit_delay;

typedef enum {
	rf24_module_autoretransmit_disable = 	0,
	rf24_module_autoretransmit_count_1 = 	1,
	rf24_module_autoretransmit_count_2 = 	2,
	rf24_module_autoretransmit_count_3 = 	3,
	rf24_module_autoretransmit_count_4 = 	4,
	rf24_module_autoretransmit_count_5 = 	5,
	rf24_module_autoretransmit_count_6 = 	6,
	rf24_module_autoretransmit_count_7 = 	7,
	rf24_module_autoretransmit_count_8 = 	8,
	rf24_module_autoretransmit_count_9 = 	9,
	rf24_module_autoretransmit_count_10 = 	10,
	rf24_module_autoretransmit_count_11 = 	11,
	rf24_module_autoretransmit_count_12 = 	12,
	rf24_module_autoretransmit_count_13 = 	13,
	rf24_module_autoretransmit_count_14 = 	14,
	rf24_module_autoretransmit_count_15 = 	15,
}
rf24_module_autoretransmit_count;

typedef enum {
	rf24_module_rf_output_power1 = 0,
	rf24_module_rf_output_power2 = 1,
	rf24_module_rf_output_power3 = 2,
	rf24_module_rf_output_power4 = 3
}
rf24_module_rf_output_power;

typedef enum {
	rf24_module_datarate_1Mbps = 	0,
	rf24_module_datarate_2Mbps = 	1,
	rf24_module_datarate_250kbps = 	2,
}
rf24_module_datarate;

typedef enum {
	w1_r,
	w1_rw,
	w8_r,
	w8_rw,
	w40_r,
	w40_rw
}
rf24_module_attribute_types;

typedef struct {
	bool w1;
	uint8_t w8;
	uint8_t *w40;
	uint8_t length;
}
rf24_module_attribute_value;

typedef struct {
	bool min;
	bool max;
}
r24_attribute_w1;

typedef struct {
	uint8_t min;
	uint8_t max;
}
r24_attribute_w8;

typedef struct {
	uint8_t max_length;
}
r24_attribute_w40;

typedef struct {
	const rf24_module_attribute_types type;
	const uint8_t reg_addr;
	const uint8_t mask;
	const uint8_t mnemonic;
	union {
		r24_attribute_w1 w1;
		r24_attribute_w8 w8;
		r24_attribute_w40 w40;
	};
}
rf24_module_attribute;

typedef struct {
	/*CONFIG */
	rf24_module_attribute* prim_rx;
	rf24_module_attribute* pwr_up;
	rf24_module_attribute* crc0;
	rf24_module_attribute* en_crc;
	rf24_module_attribute* mask_max_rt;
	rf24_module_attribute* mask_tx_ds;
	rf24_module_attribute* mask_rx_dr;
	/*EN_AA*/
	rf24_module_attribute* enaa_p0;
	rf24_module_attribute* enaa_p1;
	rf24_module_attribute* enaa_p2;
	rf24_module_attribute* enaa_p3;
	rf24_module_attribute* enaa_p4;
	rf24_module_attribute* enaa_p5;
	/*EN_RXADDR*/
	rf24_module_attribute* erx_p0;
	rf24_module_attribute* erx_p1;
	rf24_module_attribute* erx_p2;
	rf24_module_attribute* erx_p3;
	rf24_module_attribute* erx_p4;
	rf24_module_attribute* erx_p5;
	/*SETUP_AW*/
	rf24_module_attribute* aw;
	/*SETUP_RETR*/
	rf24_module_attribute* ard;
	rf24_module_attribute* arc;
	/*RF_CHANNEL*/
	rf24_module_attribute* rf_ch;
	/*RF_SETUP*/
	rf24_module_attribute* rf_pwr;
	rf24_module_attribute* rf_dr_high;
	rf24_module_attribute* pll_lock;
	rf24_module_attribute* rf_dr_low;
	rf24_module_attribute* cont_wave;
	/*STATUS*/
	rf24_module_attribute* tx_full;
	rf24_module_attribute* rx_p_no;
	rf24_module_attribute* max_rt;
	rf24_module_attribute* tx_ds;
	rf24_module_attribute* rx_dr;
	/*OBSERVE_TX*/
	rf24_module_attribute* arc_cnt;
	rf24_module_attribute* plos_cnt;
	/*PRD*/
	rf24_module_attribute* rpd;
	/*RX_ADDR_P0*/
	rf24_module_attribute* rx_addr_p0;
	/*RX_ADDR_P1*/
	rf24_module_attribute* rx_addr_p1;
	/*RX_ADDR_P2*/
	rf24_module_attribute* rx_addr_p2;
	/*RX_ADDR_P3*/
	rf24_module_attribute* rx_addr_p3;
	/*RX_ADDR_P4*/
	rf24_module_attribute* rx_addr_p4;
	/*RX_ADDR_P5*/
	rf24_module_attribute* rx_addr_p5;
	/*TX_ADDR*/
	rf24_module_attribute* tx_addr;
	/*RX_PW_P0*/
	rf24_module_attribute* rx_pw_p0;
	/*RX_PW_P1*/
	rf24_module_attribute* rx_pw_p1;
	/*RX_PW_P2*/
	rf24_module_attribute* rx_pw_p2;
	/*RX_PW_P3*/
	rf24_module_attribute* rx_pw_p3;
	/*RX_PW_P4*/
	rf24_module_attribute* rx_pw_p4;
	/*RX_PW_P5*/
	rf24_module_attribute* rx_pw_p5;
	/*FIFO STATUS*/
	rf24_module_attribute* rx_empty;
	rf24_module_attribute* rx_full;
	rf24_module_attribute* tx_empty;
	rf24_module_attribute* tx_reuse;
	/*DYNPD*/
	rf24_module_attribute* dpl_p0;
	rf24_module_attribute* dpl_p1;
	rf24_module_attribute* dpl_p2;
	rf24_module_attribute* dpl_p3;
	rf24_module_attribute* dpl_p4;
	rf24_module_attribute* dpl_p5;
	/*FEATURE*/
	rf24_module_attribute* en_dyn_ack;
	rf24_module_attribute* en_ack_pay;
	rf24_module_attribute* en_dpl;
}
rf24_module_attributes_struct;

typedef struct {
	uint8_t 					rf_channel;
	rf24_module_rf_output_power rf_output_power;
	rf24_module_datarate		datarate;

	// addresses
	rf24_module_address_width	address_width;
	uint8_t 					tx_address[5];
	uint8_t						rx_address_pipe0[5];
	uint8_t						rx_address_pipe1[5];
	uint8_t						rx_address_pipe2;
	uint8_t						rx_address_pipe3;
	uint8_t						rx_address_pipe4;
	uint8_t						rx_address_pipe5;

	// crc
	rf24_module_crc_length		crc_length; // = 0 (crc off)

	// autoretransmit
	rf24_module_autoretransmit_count 	autoretransmit_count; // = 0 (autoretransmit off)
	rf24_module_autoretransmit_delay	autoretransmit_delay;

	uint8_t						payload_width_pipe0;
	uint8_t						payload_width_pipe1;
	uint8_t						payload_width_pipe2;
	uint8_t						payload_width_pipe3;
	uint8_t						payload_width_pipe4;
	uint8_t						payload_width_pipe5;
}
rf24_module_settings_struct;

typedef struct {
	uint8_t payload[32];
	uint8_t length;
	rf24_module_rx_pipes rx_pipe;
} rf24_module_rx_data;

typedef struct {
	uint8_t payload[32];
	uint8_t length;
} rf24_module_tx_data;

typedef struct {
	bool package_transmitted;
	bool max_retransmits;
	bool package_received;
} rf24_module_flags_t;

typedef void (*rf24_module_rx_data_fct_ptr) (rf24_module_rx_data*);
typedef void (*rf24_module_fct_ptr) (void);


//_________________________________________________________________________________________________________________________________________________________________________
// GLOBALS

rf24_module_attributes_struct 	rf24_module_attributes;
rf24_module_attribute_value 	value_in;
rf24_module_attribute_value 	value_out;

rf24_module_rx_data 			rx_data;
rf24_module_rx_data_fct_ptr 	rx_data_received_notify_fct_ptr;
rf24_module_fct_ptr				tx_data_transmitted_notify_fct_ptr;

//_________________________________________________________________________________________________________________________________________________________________________
// FUNCTION DEFINITIONS

extern uint8_t 						rf24_module_read_register						(uint8_t register_name);
extern uint8_t 						rf24_module_write_register						(uint8_t register_name, uint8_t data);
extern void 						rf24_module_readwrite_register					(rf24_module_read_write read_write, uint8_t register_name, uint8_t *data_inout, uint8_t length);
extern bool 						rf24_module_write_attribute						(rf24_module_attribute *attribute, rf24_module_attribute_value *value_in);
extern bool 						rf24_module_read_attribute						(rf24_module_attribute *attribute, rf24_module_attribute_value *value_out);
extern rf24_module_attribute_value* rf24_module_value_w1							(bool value);
extern rf24_module_attribute_value* rf24_module_value_w8							(uint8_t value);
extern rf24_module_attribute_value* rf24_module_value_w40							(uint8_t* value, uint8_t value_length);
void 								rf24_module_set_ce								(rf24_stm32f1xx_pin_state);
extern void 						rf24_module_irq_handler							(void);
void 								rf24_module_set_prim_rx							(rf24_stm32f1xx_pin_state);
void 								rf24_module_power_up							(void);
void 								rf24_module_power_down							(void);
void 								rf24_module_enable_crc							(void);
void 								rf24_module_disable_crc							(void);
extern bool 						rf24_module_crc_enabled							(void);
void 								rf24_module_set_crc_length						(rf24_module_crc_length crc_length);
extern rf24_module_crc_length 		rf24_module_get_crc_length						(void);
void 								rf24_module_enable_autoack_pipe					(rf24_module_rx_pipes rx_pipes);
void 								rf24_module_enable_rx_pipe						(rf24_module_rx_pipes rx_pipes);
void 								rf24_module_disable_autoack_pipe				(rf24_module_rx_pipes rx_pipes);
void 								rf24_module_set_address_width					(rf24_module_address_width address_width);
rf24_module_address_width 			rf24_module_get_address_width					(void);
void 								rf24_module_set_autoretransmit_delay			(rf24_module_autoretransmit_delay autoretransmit_delay);
rf24_module_autoretransmit_delay 	rf24_module_get_autoretransmit_delay			(void);
void 								rf24_module_set_autoretransmit_count			(rf24_module_autoretransmit_count autoretransmit_count);
rf24_module_autoretransmit_count 	rf24_module_get_autoretransmit_count			(void);
void 								rf24_module_set_rf_channel						(uint8_t rfchannel);
uint8_t 							rf24_module_get_rf_channel						(void);
void 								rf24_module_set_rf_outputpower					(rf24_module_rf_output_power rf_output_power);
rf24_module_rf_output_power 		rf24_module_get_rf_outputpower					(void);
void 								rf24_module_set_datarate						(rf24_module_datarate datarate);
rf24_module_datarate 				rf24_module_get_datarate						(void);
void 								rf24_set_cont_wave								(rf24_stm32f1xx_pin_state bit_state);
bool 								rf24_module_tx_fifo_full						(void);
rf24_module_rx_pipes 				rf24_module_get_rx_available_pipe				(void);
bool 								rf24_module_max_retransmits						(void);
bool 								rf24_module_package_transmitted					(void);
bool 								rf24_module_rx_data_available					(void);
void 								rf24_module_clear_all_interrupts				(void);
uint8_t 							rf24_module_get_retransmitted_count				(void);
uint8_t 							rf24_module_get_packagelost_count				(void);
uint8_t 							rf24_module_get_received_power_detector			(void);
void 								rf24_module_set_rx_address_pipe0				(uint8_t *rx_address /*3-5 bytes*/, uint8_t rx_address_length);
void 								rf24_module_set_rx_address_pipe1				(uint8_t *rx_address /*3-5 bytes*/, uint8_t rx_address_length);
void 								rf24_module_set_rx_address_pipe2				(uint8_t rx_address /*1 byte*/);
void 								rf24_module_set_rx_address_pipe3				(uint8_t rx_address /*1 byte*/);
void 								rf24_module_set_rx_address_pipe4				(uint8_t rx_address /*1 byte*/);
void 								rf24_module_set_rx_address_pipe5				(uint8_t rx_address /*1 byte*/);
void 								rf24_module_set_tx_address						(uint8_t *tx_address /*3-5 bytewrites*/, uint8_t tx_address_length);
void 								rf24_module_get_tx_address						(uint8_t *tx_address_out /*3-5 bytewrites*/);
void 								rf24_module_set_payload_width_pipe				(rf24_module_rx_pipes rx_pipe, uint8_t payload_width);
bool 								rf24_module_rx_fifo_empty						(void);
bool 								rf24_module_rx_fifo_full						(void);
bool 								rf24_module_tx_fifo_empty						(void);
bool 								rf24_module_tx_reuse							(void);
void 								rf24_module_enable_dynamic_payload_pipe			(rf24_module_rx_pipes rx_pipe);
void 								rf24_module_disable_dynamic_payload_pipe		(rf24_module_rx_pipes rx_pipe);
void 								rf24_module_enable_dynamic_payload				(void);
void 								rf24_module_disable_dynamic_payload				(void);
void 								rf24_module_enable_autoack_with_payload			(void);
void 								rf24_module_disable_autoack_with_payload		(void);
void 								rf24_module_enable_transmission_without_ack		(void);
void 								rf24_module_disable_transmission_without_ack	(void);
void 								rf24_module_flush_tx							(void);
void 								rf24_module_flush_rx							(void);
void 								rf24_module_collect_rx_data						(rf24_module_rx_data*);
extern bool 						rf24_module_init								(uint32_t baudrate);
extern void 						rf24_module_config								(void);
extern uint8_t 						rf24_module_transmit							(rf24_module_tx_data *);
void  								rf24_module_print_rx_data						(rf24_module_rx_data*);
rf24_module_rx_data* 				rf24_module_get_rx_data							(void);
extern void 	 					rf24_module_attach_notify_data_transmitted		(rf24_module_fct_ptr fct_ptr);
extern void 						rf24_module_attach_notify_data_received			(rf24_module_rx_data_fct_ptr fct_ptr);

#endif /* RF24_MODULE */
