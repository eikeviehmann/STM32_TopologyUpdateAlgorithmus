#ifndef __RF24_MAC_H__
#define __RF24_MAC_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include "../rf24_stm32f1xx/rf24_stm32f1xx.h"
#include "../rf24_stm32f1xx/rf24_stm32f1xx_uart.h"
#include "../rf24_module/rf24_module.h"

//_________________________________________________________________________________________________________________________________________________________________________
// Preprocessor & Constants

#ifdef DEBUG
	#define RF24_MAC_DEBUG
	#define RF24_CSMA_CA_DEBUG
#endif

#define T_SLOT_US 				500
#define CONTENTION_WINDOW		32

#ifdef DEBUG
	#undef T_SLOT_US
	#undef CONTENTION_WINDOW
	#define T_SLOT_US 			10000
	#define T_SLOS_MS			T_SLOT_US / 1000
	#define CONTENTION_WINDOW	32
#endif

#define T_PROCESSING_US			150
#define T_SIFS_US  				1 * T_SLOT_US
#define T_PIFS_US 				2 * T_SLOT_US
#define T_DIFS_US  				3 * T_SLOT_US
#define T_NAV_RTS_US			( 3 * T_SIFS_US + 3 * T_PROCESSING_US )
#define T_NAV_RTS_MS			T_NAV_RTS_US / 1000
#define T_NAV_CTS_US			( 2 * T_SIFS_US + 2 * T_PROCESSING_US )
#define T_NAV_CTS_MS			T_NAV_CTS_US / 1000
#define T_NAV_FRAG_US 			( 3 * T_SIFS_US + 3 * T_PROCESSING_US )
#define T_NAV_FRAG_MS 			T_NAV_FRAG_US / 1000
#define T_NAV_ACK_US 			( 2 * T_SIFS_US + 2 * T_PROCESSING_US )
#define T_NAV_ACK_MS 			T_NAV_ACK_US / 1000

#define T_CTS_TIMEOUT_US  		3 * T_SLOT_US
#define T_ACK_TIMEOUT_US 		3 * T_SLOT_US

#define N_MAX_RETRANSMITS		15
#define N_MAX_RTS_RETRANSMITS	15

#define T_BROADCAST_TOPOGLOY_MS	1000
#define T_NEIGHBOUR_OUTDATED_MS	2000

#define T_MAX_RANDOM_BACKOFF_US CONTENTION_WINDOW * T_SLOT_US

#define T_LED_CYCLE_TRANSMITTING_MS 100
#define T_LED_CYCLE_IDLE_MS 1000

//_________________________________________________________________________________________________________________________________________________________________________
// Structs & Enums

typedef struct {
	uint8_t bytes[6];
} rf24_mac_addr;

typedef struct {
	uint16_t ms;
} rf24_mac_slot_time;

static const char *rf24_mac_frame_type_string[] = {
		"",
		"MANAGEMENT",
		"CONTROL",
		"DATA",
		"TOPOLOGY",
};

typedef enum {
	MANAGEMENT=1,
	CONTROL,
	DATA,
	TOPOLOGY
} rf24_mac_frame_type;

static const char *rf24_mac_frame_subtype_string[] = {
		"",
		"TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE",
		"TOPOLOGY_NEIGHBOR_ANSWER_MESSAGE",
		"CONTROL_RTS",
		"CONTROL_CTS",
		"CONTROL_ACK",
		"DATA_DATA",
		"DATA_TOPOLOGY",
		"DATA_NULL"};

static const char *rf24_mac_frame_subtype_string_short[] = {
		"",
		"NUM",
		"NAM",
		"RTS",
		"CTS",
		"ACK",
		"DATA",
		"DATA_TOPOLOGY",
		"DATA_NULL"};

typedef enum {
	TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE=1,
	TOPOLOGY_NEIGHBOR_ANSWER_MESSAGE,
	CONTROL_RTS,
	CONTROL_CTS,
	CONTROL_ACK,
	DATA_DATA,
	DATA_TOPOLOGY,
	DATA_NULL
}
rf24_mac_frame_subtype;

typedef struct __attribute__ ((__packed__)) {
	uint8_t								id;
	uint8_t 							hop_count;
	bool								successor;
} rf24_mac_frame_topology;

typedef struct __attribute__ ((__packed__)) {
	unsigned int 	type : 4;
	unsigned int 	subtype : 4;
	uint16_t 		length;
} rf24_mac_frame_rts;

typedef struct __attribute__ ((__packed__)) {
	unsigned int 	type : 4;
	unsigned int 	subtype : 4;
	uint8_t	 		id;
} rf24_mac_frame_ack;

typedef struct __attribute__ ((__packed__)) rf24_mac_frame_data {
	struct __attribute__ ((__packed__)) {
		uint8_t  	length;
	} header;
	uint8_t 		payload[13];
} rf24_mac_frame_data;

typedef struct __attribute__((__packed__))
{
	struct __attribute__((__packed__)) frame_control
	{
		unsigned int 	protocol_version : 2; 	// Bits [1,0]
		unsigned int 	type : 4;				// Bits [3,2]
		unsigned int 	subtype : 4;			// Bits [7,6,5,4]
		unsigned int 	to_distribution : 1;	// Bit [0]
		unsigned int 	from_distribution : 1; 	// Bit [1]
		unsigned int 	more_fragments : 1;		// Bit [2]
		unsigned int 	retry : 1;				// Bit [3]
		unsigned int 	reply : 1;				// Bit [4]
		unsigned int 	more_data : 1;			// Bit [5]
		//unsigned int 	wep : 1;				// Bit [6]
		//unsigned int 	order : 1;				// Bit [7]
	}
	frame_control; 								// 2 Byte

	uint16_t 			duration;				// 2 Byte
	rf24_mac_addr 		receiver; 				// 6 Byte
	rf24_mac_addr 		transmitter; 			// 6 Byte
	uint8_t				id;

	union __attribute__((__packed__))
	{
		rf24_mac_frame_ack 		ack;
		rf24_mac_frame_rts	 	rts;
		rf24_mac_frame_data 	data;
		rf24_mac_frame_topology topology;
	};
}
rf24_mac_frame;

#define MAC_FRAME_PAYLOAD_LENGTH 13

typedef enum {
	transmission = 0,
	reception
} rf24_mac_transmission_type;

typedef enum {
	UNICAST,
	MULTICAST,
	BROADCAST
}
rf24_mac_communication_type;

typedef enum {
	ACTIVE = 0,
	COMPLETED,
	FINALIZED
} rf24_mac_transmission_state;

typedef struct {
	rf24_mac_transmission_state	state;
	rf24_mac_transmission_type	transmission_type;
	rf24_mac_communication_type	communication_type;
	rf24_mac_frame_type 		frame_type;
	rf24_mac_frame_subtype		frame_subtype;
	rf24_mac_addr 				receiver;
	rf24_mac_addr				transmitter;
	rf24_mac_frame 				mac_frame;
	uint8_t* 					payload;
	uint8_t						length;
	uint8_t						index;
	uint8_t						frame_count;
	uint8_t 					attemps;
}
rf24_mac_transmission;

typedef struct {
	unsigned int ack_received : 1;
	unsigned int max_retransmits : 1;
	unsigned int free2 : 1;
	unsigned int free3 : 1;
	unsigned int free4 : 1;
	unsigned int free5 : 1;
	unsigned int free6 : 1;
	unsigned int free7 : 1;
}
rf24_mac_flags;

//_________________________________________________________________________________________________________________________________________________________________________
// Function Definitions

// Interrupts / Callbacks

bool 							rf24_mac_addr_equal(rf24_mac_addr *mac_addr1, rf24_mac_addr *mac_addr2);

void 							rf24_mac_frame_received_handler(rf24_mac_frame *mac_frame);
void 							rf24_mac_ack_timeout();
void 							rf24_mac_transmission_successfull();
void 							rf24_mac_reception_successfull();

// User Functions
void 							rf24_mac_init();
void 							rf24_mac_build_address();
rf24_mac_addr* 					rf24_mac_get_address();
rf24_mac_addr*					rf24_mac_get_broadcast_address();
rf24_mac_transmission* 			rf24_mac_get_transmission();

void 							rf24_mac_transfer_frame(rf24_mac_communication_type transmission_type, rf24_mac_frame *mac_frame);
void 							rf24_mac_transfer_data(rf24_mac_communication_type, rf24_mac_addr*, rf24_mac_frame_subtype ,uint8_t* , uint8_t);
void 							rf24_mac_print_neighbours();

// Internal Functions
void 							rf24_mac_send_ack(rf24_mac_frame *mac_frame);

void 							_send_mac_frame();

void 							_start_transmission();
rf24_mac_transmission* 			rf24_mac_setup_transmission(rf24_mac_communication_type, rf24_mac_frame_type,rf24_mac_frame_subtype,rf24_mac_addr*,uint8_t*,uint8_t);
void 							_process_transmission();

void							 rf24_mac_setup_reception(rf24_mac_frame *mac_frame);
void							_process_reception();

void 							rf24_mac_frame_to_tx_data(rf24_mac_frame* , rf24_module_tx_data*);
void 							rf24_mac_rx_data_to_mac_frame(rf24_module_rx_data*, rf24_mac_frame*);

void 							_payload_to_data_frame(rf24_mac_transmission* transmission, rf24_mac_frame* mac_frame);
void 							_data_frame_to_payload(rf24_mac_frame* mac_frame, rf24_mac_transmission* transmission);

void 							rf24_mac_print_ping_result();
void 							rf24_mac_print_frame(rf24_mac_frame *mac_frame);
void 							rf24_mac_print_payload(rf24_mac_transmission *transmission);
void 							rf24_mac_print_timings();

#endif /* RF24_MAC */
