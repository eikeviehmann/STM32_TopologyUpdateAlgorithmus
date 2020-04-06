#ifndef RF24_CSMA_CA_H
#define RF24_CSMA_CA_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include "rf24_mac.h"
#include "rf24_worker.h"

//_________________________________________________________________________________________________________________________________________________________________________
// Structs & Enums


typedef void (*rf24_csma_ca_fct_ptr_rts_received) (rf24_mac_frame*);

typedef void (*rf24_csma_ca_fct_ptr_access_medium) (void);

typedef struct {
	rf24_mac_transmission*				transmission;
	uint32_t							t_random_backoff_us;
	uint32_t							t_cts_response_us;
	uint8_t								rts_transmits;
	rf24_csma_ca_fct_ptr_access_medium	fct_ptr_access_medium;
} rf24_csma_ca_order;

typedef struct {
	unsigned int backoff_competition_lost : 1;
	unsigned int cts_timeout : 1;
	unsigned int rts_received : 1;
	unsigned int cts_received : 1;
	unsigned int free1 : 1;
	unsigned int free2: 1;
	unsigned int free3 : 1;
	unsigned int free4 : 1;
} rf24_csma_ca_flags;

typedef enum {
	idle,
	processing,
	time_out,
	access_to_medium
} rf24_csma_ca_states;

//_________________________________________________________________________________________________________________________________________________________________________
// Function definitions

extern void rf24_csma_ca_frame_received_handler(rf24_mac_frame *mac_frame);

void rf24_csma_ca_wait_nav(rf24_mac_frame *mac_frame);

void rf24_csma_ca_send_rts();
void rf24_csma_ca_send_cts();
void rf24_csma_ca_nav_expired();
void rf24_csma_ca_cts_timeout();
void rf24_csma_ca_random_backoff_expired();

uint32_t rf24_csma_ca_compute_random_backoff(uint16_t contention_window);

void rf24_cmsa_ca_init(rf24_csma_ca_fct_ptr_rts_received rts_received_fct_ptr);
void rf24_csma_ca_access_medium(rf24_mac_transmission*, uint8_t, rf24_csma_ca_fct_ptr_access_medium);

bool rf24_csma_ca_access();

void rf24_csma_ca_reset();
rf24_mac_frame* rf24_csma_ca_get_rts_frame();


#endif /* RF24_CSMA_CA */
