#ifndef RF24_NETWORK
#define RF24_NETWORK

#include "rf24_mac.h"

#define RF24_NETWORK_DEBUG

#define PWRTWO(x) (1 << (x))

typedef struct {
	unsigned int num_transmitted : 1;	// NUM received (first NUM transmitter is topology predecessor)
	unsigned int free1: 1;				// NUM broadcasted (forward topology update cycle)
	unsigned int free2 : 1;
	unsigned int free3 : 1;
	unsigned int free4 : 1;
	unsigned int free5 : 1;
	unsigned int free6 : 1;
	unsigned int free7 : 1;
}
rf24_network_flags;

typedef enum {
	UNKNOWN = 0,
	EVALUATED,
	ESTABLISHED
}
rf24_network_predecessor_linkstate;

typedef enum {
	SUCCESSOR = 0,
	PREDECESSOR,
	NEIGHBOR
}
rf24_neighbor_relation;

typedef enum {
	AVAILABLE = 0, // neighbor is know from a broadcast but never replied to a brodcast message -> inactive link
	CONNECTED = 1, // neighbor replied a broadcast message -> active link
	TIMED_OUT = 2, // active link timed out because no message has replied within timeout timer
	NO_LINK = 3
}
rf24_neighbor_link_state;

struct rf24_neighbor {
	rf24_mac_addr 				mac_addr;
	rf24_neighbor_link_state 	link_state;
	rf24_neighbor_relation 		relation;
	uint32_t 					t_response_us;
	uint16_t 					t_last_updated_ms;
	uint8_t 					hops_to_controller;
	struct rf24_topology*		topology;
	bool						topology_received;
	struct 						rf24_neighbor *next;
};

struct rf24_neighbor_collection {
	uint8_t length;
	struct rf24_neighbor **nodes;
};

struct rf24_topology {
	struct rf24_neighbor *neighbor;
	struct rf24_topology *next;
};

/* CALLBACK HANDLER / NOTIFICATION HANDLER */
void 								rf24_network_frame_received_handler(rf24_mac_frame*);
void 								rf24_network_transmission_successfull_handler(void);
void 								rf24_network_reception_successfull_handler(void);
void 								rf24_network_transmission_failed_handler(void);
void 								rf24_network_reception_failed_handler(void);
void 								rf24_network_topology_received_handler(void);
/* USER FUNCTIONS */
void 								rf24_network_init(void);
void 								rf24_network_reset(void);
void 								rf24_network_reset_topology(void);
void 								rf24_network_start_topology_update(void);
void 								rf24_network_broadcast_num(void);
void 								rf24_network_send_num(void);
/* GET & SET*/
struct rf24_neighbor* 				rf24_network_get_neighbors();
struct rf24_topology*				rf24_network_get_topology();
rf24_network_flags*					rf24_network_get_flags(void);
void 								rf24_network_set_broadcast_topology_id(uint8_t);
void 								rf24_network_set_topology_predecessor(rf24_mac_addr);
void 								rf24_network_set_topology_cycle_id(uint8_t);
uint8_t								rf24_network_get_topology_cycle_id(void);
rf24_mac_addr* 						rf24_network_get_topology_predecessor(void);
void 								rf24_network_set_topology_cycle_id(uint8_t);
void 								rf24_network_set_hopcount(uint8_t);
/* FUNCTIONAL */
void 								rf24_network_add_successor_topology(struct rf24_topology*);
void 								rf24_network_topology_timeout_handler(void);
void 								rf24_network_trm_timeout_handler(void);
void 								rf24_network_num_timeout_handler(void);

uint8_t 							rf24_network_count_successors();

/* INTERNAL */
uint32_t 							rf24_network_calculate_topology_timeout(void);
bool 								rf24_network_check_topology_integrity(void);
void 								rf24_network_new_topology_cycle(rf24_mac_frame*);
struct rf24_neighbor* 				rf24_network_add_neighbor(rf24_mac_addr*, uint32_t, rf24_neighbor_relation, rf24_neighbor_link_state);
struct rf24_neighbor* 				rf24_network_get_neighbor(rf24_mac_addr*);
void 								rf24_network_set_neighbor_link_state(rf24_mac_addr, rf24_neighbor_link_state);
rf24_neighbor_link_state 			rf24_network_get_neighbor_link_state(rf24_mac_addr*);
struct rf24_neighbor_collection* 	rf24_network_collect_neighbors(rf24_neighbor_relation, rf24_neighbor_link_state);
void 								rf24_network_print_neighbor_collection(struct rf24_neighbor_collection *);
void 								rf24_network_reset_neighbors();
void 								rf24_network_update_neighbors();
void 								rf24_network_topology_to_tx_data(struct rf24_topology *topology, uint8_t *rx_data, uint8_t length);
struct rf24_topology* 				rf24_network_rx_data_to_topology(uint8_t *rx_data, uint8_t length);
/* PRINTER */
void 								rf24_network_print_neighbors(struct rf24_neighbor *neighbors);
void 								rf24_network_print_topology(struct rf24_topology *topology);

void 								rf24_network_print_topology(struct rf24_topology *topology);
void 								rf24_network_transfer_topology(rf24_mac_addr *receiver);



#endif /* RF24_NETWORK */
