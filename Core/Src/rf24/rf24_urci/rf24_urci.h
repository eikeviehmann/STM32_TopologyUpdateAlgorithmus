#ifndef RF24_URCI_H
#define RF24_URCI_H

#include <stdint.h>
#include <stdbool.h>

#include "../rf24_module/rf24_module.h"
#include "../rf24_module/nRF24L01.h"

#define rf24_urci_start_symbol		'<'									//0x05;	/* ASCII 'ENQ' */
#define rf24_urci_end_symbol		'>'									//* ASCII 'ETX' */
#define rf24_urci_separator_symbol	'='									//0x3D;		/* ASCII '=' */
#define rf24_urci_attribute_symbol	'.'

char urci_buffer_32[32];
char urci_buffer_128[128];

//_________________________________________________________________________________________________________________________________________________________________________
// TYPES DEFINITIONS

typedef 	void 	(*fct_ptr) 				(void);
typedef 	void 	(*fct_ptr_set_uint8) 	(uint8_t);
typedef 	void 	(*fct_ptr_set_string) 	(char*);
typedef 	uint8_t (*fct_ptr_get_uint8) 	(void);
typedef 	char* 	(*fct_ptr_get_string) 	(void);

typedef struct {
	uint8_t enable:1;
	uint8_t start:1;
	uint8_t end:1;
	uint8_t separator:1;
	uint8_t attribute:1;
	uint8_t cmd_set:1;
	uint8_t cmd_get:1;
} urci_flags;

typedef enum { rf24_urci_uint8, rf24_urci_string, rf24_urci_void } urci_val_types;

typedef struct {
	char* 				name;							// name of command, used for matching
	char*				description;					// description
	char*				details;
	urci_val_types		set_type;
	urci_val_types		get_type;
	fct_ptr 			fct_ptr;
	fct_ptr_set_uint8 	fct_ptr_set_uint8;
	fct_ptr_set_string 	fct_ptr_set_string;
	fct_ptr_get_uint8   fct_ptr_get_uint8;
	fct_ptr_get_string 	fct_ptr_get_string;
} urci_cmd;


//_________________________________________________________________________________________________________________________________________________________________________
// FUNCTION DEFINITIONS

extern void rf24_urci_init();
extern void rf24_urci_putc(char data);

void rf24_urci_help(char* str);
void rf24_urci_print_help(void);
void rf24_urci_print_register(char* str);

void rf24_urci_set_datarate(char* str);
char* rf24_urci_get_datarate();

void rf24_urci_print_status(void);
void rf24_urci_set_crc_length(uint8_t crc_length);
uint8_t rf24_urci_get_crc_length();

void rf24_urci_set_tx_address(char *str);
char* rf24_urci_get_tx_address(void);

void rf24_urci_set_rx_address(char *address);
char* rf24_urci_get_rx_address();

void rf24_urci_set_payload_width(uint8_t payload_size);
uint8_t rf24_urci_get_payload_width();

void rf24_urci_transmit(char* str);
void rf24_urci_transfer_topology(char* str);
void rf24_urci_transmit_mac_frame(char* str);
void rf24_urci_transfer_nam(char* str);

void rf24_urci_print_mac_address();

void rf24_urci_ping(char* str);
void rf24_urci_ping_reply(void);

//void rf24_urci_set_rxaddress_pipe(rf24_rx_pipes rx_pipe, uint8_t *address, uint8_t address_width);
//char* rf24_urci_get_rx_address_pipe(rf24_rx_pipes rx_pipe);

uint8_t string_to_bytes(char *str_in, char* delimiters, uint8_t *array_out, uint8_t array_out_length);
void bytes_to_string(uint8_t *address_in, uint8_t address_length, const char delimiter, char *str_out);

#endif
