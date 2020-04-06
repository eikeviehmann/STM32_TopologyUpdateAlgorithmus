#ifndef RF24_NETWORK
#define RF24_NETWORK

#include "rf24_mac.h"

#define RF24_NETWORK_DEBUG

#define PWRTWO(x) (1 << (x))

#define N_NODES	5

#define T_NUM_NAM_HANDSHAKE_US \
	T_PROCESSING_US /*NUM*/ + T_MAX_RANDOM_BACKOFF_US /*MAX RND-BACKOFF NUM*/ + \
	T_PROCESSING_US /*RTS*/ + T_MAX_RANDOM_BACKOFF_US /*MAX RND-BACKOFF RTS*/ + T_DIFS_US /* DIFS RTS*/ + \
	T_PROCESSING_US /*CTS*/ + T_SIFS_US /* SIFS CTS*/ + \
	T_PROCESSING_US /*NAM*/ + T_SIFS_US /* SIFS NAM*/ + \
	T_PROCESSING_US /*ACK*/ + T_SIFS_US /*SIFS ACK*/

typedef struct {
	unsigned int num_transmitted : 1;		// NUM received (first NUM transmitter is topology predecessor)
	unsigned int free1: 1;	// NUM broadcasted (forward topology update cycle)
	unsigned int free2 : 1;
	unsigned int free3 : 1;
	unsigned int free4 : 1;
	unsigned int free5 : 1;
	unsigned int free6 : 1;
	unsigned int free7 : 1;
}
rf24_network_flags;

static const char *rf24_neighbor_states_string[] = { "AVAILABLE", "CONNECTED", "TIMED_OUT", "NO_LINK",};

typedef enum {
	UNKNOWN,
	EVALUATED,
	ESTABLISHED
}
rf24_network_predecessor_linkstate;

typedef enum {
	SUCCESSOR,
	PREDECESSOR,
	NEIGHBOR
}
rf24_neighbor_relation;

static const char *rf24_neighbor_relation_string[] = { "SUCCESSOR", "PREDECESSOR", "NEIGHBOR"};

typedef enum {
	AVAILABLE = 0, // neighbor is know from a broadcast but never replied to a brodcast message -> inactive link
	CONNECTED = 1, // neighbor replied a broadcast message -> active link
	TIMED_OUT = 2, // active link timed out because no message has replied within timeout timer
	NO_LINK = 3
}
rf24_neighbor_state;

struct rf24_neighbor {
	rf24_mac_addr 			mac_addr;
	rf24_neighbor_state 	state;
	rf24_neighbor_relation 	relation;
	uint32_t 				t_response_us;
	uint16_t 				t_last_updated_ms;
	uint8_t 				hops_to_controller;
	struct 					rf24_neighbor *next;
};

struct rf24_topology {
	struct rf24_neighbor *neighbor;
	struct rf24_topology *next;
};

void 							rf24_network_frame_received_handler(rf24_mac_frame *mac_frame);
void 							rf24_network_transmission_successfull();


void 							rf24_network_init();
rf24_network_flags*				rf24_network_get_flags();

void 							rf24_network_topology_received();
void 							rf24_network_reset_topology();

void 							rf24_network_start_broadcast_topology();
void 							rf24_network_forward_broadcast_topology();
void 							rf24_network_send_broadcast_topology();
void 							rf24_network_set_broadcast_topology_id(uint8_t broadcast_topology_id);
void 							rf24_network_set_topology_predecessor(rf24_mac_addr topology_predecessor);
uint8_t							rf24_network_get_broadcast_topology_id();
rf24_mac_addr* 					rf24_network_get_topology_predecessor(void);
void 							rf24_network_wait_for_topology_reply();
void 							rf24_network_topology_timeout();
uint32_t 						rf24_network_calculate_topology_timeout();

bool 							rf24_network_check_successor_integrity();

void 							rf24_network_transfer_nam(rf24_mac_addr receiver);
void 							rf24_network_send_broadcast_topology_reply();
void 							rf24_network_new_topology_cycle(rf24_mac_frame *mac_frame);

void 							rf24_network_add_neighbor(rf24_mac_addr, uint32_t, rf24_neighbor_relation , rf24_neighbor_state);
struct rf24_neighbor* 			rf24_network_get_neighbor(rf24_mac_addr mac_addr);
rf24_neighbor_state 			rf24_network_get_neighbor_state(rf24_mac_addr *mac_addr);

void 							rf24_network_reset_neighbors();
void 							rf24_network_update_neighbors();
void 							rf24_network_print_neighbors();

void 							rf24_network_topology_to_tx_data(uint8_t *rx_data, uint8_t length);
void 							rf24_network_rx_data_to_topology(uint8_t *rx_data, uint8_t length);
void 							rf24_network_print_topology();
void 							rf24_network_transfer_topology();



#endif /* RF24_NETWORK */
