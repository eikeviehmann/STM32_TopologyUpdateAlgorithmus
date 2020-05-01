#include "rf24_network.h"
#include "rf24_worker.h"
#include "rf24_csma_ca.h"
#include "../rf24_debug.h"

bool 								controller = false;				// Node is or is not controller
uint8_t								topology_cycle_id = 0;		// Broadcast topology id
rf24_mac_addr						topology_predecessor;			// MAC address of first NUM sender (topology predecessor)

struct rf24_neighbor* volatile 		neighbors = NULL;
struct rf24_topology* volatile 		topology = NULL;

uint8_t 							neighbors_length = 0;			// number of local neighbors
uint8_t								topology_length = 0; 			// number of topologies
uint8_t								topology_neighbors_length = 0;	// number of neighbors in topologies

uint8_t								hops_to_controller = 0;		// number of hops to controller

bool								topology_broadcasted = false;

rf24_network_flags					network_flags = { false, false, false, false, false, false, false, false };

rf24_network_predecessor_linkstate	predecessor_linkstate;

struct rf24_neighbor_collection		neighbor_collection;

static const char 					*rf24_neighbor_states_string[] = { "AVAILABLE", "CONNECTED", "TIMED_OUT", "NO_LINK",};
static const char 					*rf24_neighbor_relation_string[] = { "SUCCESSOR", "PREDECESSOR", "NEIGHBOR" };

void rf24_network_transfer_trm(rf24_mac_addr receiver)
{
	// Build a NAM frame
	rf24_mac_frame mac_frame;
	mac_frame.frame_control.type = TOPOLOGY;
	mac_frame.frame_control.subtype = TOPOLOGY_REPLY_MESSAGE;
	mac_frame.transmitter = *rf24_mac_get_address();
	mac_frame.receiver = receiver;
	mac_frame.duration = T_NAV_FRAG_MS;
	mac_frame.id = 1;
	mac_frame.topology.successor = false;

	if(rf24_mac_addr_equal(&topology_predecessor, &receiver)) mac_frame.topology.successor = true;

	// Transfer TRM (CSMA/CA)
	rf24_mac_transfer_frame(UNICAST, &mac_frame);

	rf24_debug(	NETWORK, TRANSMIT, mac_frame.frame_control.subtype, VOID, &receiver, "\n");

}

void rf24_network_set_predecessor(rf24_mac_addr *predecessor)
{
	topology_predecessor = *predecessor;

	rf24_debug(	NETWORK, INFO, VOID, VOID, VOID,
				"Predecessor set to %s\n", decimal_to_string(topology_predecessor.bytes, 6, ':'));
}

rf24_mac_addr* rf24_network_get_predecessor()
{
	// If node is controller, return its own MAC address
	if(controller) return rf24_mac_get_address();

	return &topology_predecessor;
}

rf24_network_flags* rf24_network_get_flags()
{
	return &network_flags;
}

void rf24_network_frame_received_handler(rf24_mac_frame *mac_frame)
{
	switch(mac_frame->frame_control.subtype)
	{
		/* Neighbor Update Message (NUM) */
		case TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE:
		{
			bool addressed_to_me = rf24_mac_addr_equal(&mac_frame->receiver, rf24_mac_get_address());

			if(addressed_to_me)
			{
				rf24_network_add_neighbor(mac_frame->transmitter, 0, SUCCESSOR, AVAILABLE);

				rf24_network_transfer_trm(mac_frame->transmitter);
			}
			else
			{
				if(mac_frame->topology.id != rf24_network_get_topology_cycle_id())
				{
					rf24_network_reset_topology();

					controller = false;
					topology_cycle_id = mac_frame->topology.id;
					hops_to_controller = mac_frame->topology.hop_count + 1;
					rf24_network_set_predecessor(&mac_frame->transmitter);
					predecessor_linkstate = EVALUATED;

					rf24_network_add_neighbor(mac_frame->transmitter, 0, PREDECESSOR, AVAILABLE);

					rf24_network_broadcast_num();
				}
				else
				{
					rf24_network_add_neighbor(mac_frame->transmitter, 0, NEIGHBOR, AVAILABLE);

					rf24_debug(	NETWORK, RECEIVE, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, &mac_frame->transmitter,
								"ID: %d, hop-count: %d (node attached to neighbors)\n",
								mac_frame->topology.id,
								mac_frame->topology.hop_count);
				}
			}

			break;
		}

		/* Topology Reply Message (TR) */
		case TOPOLOGY_REPLY_MESSAGE:
		{
			bool addressed_to_me = rf24_mac_addr_equal(&mac_frame->receiver, rf24_mac_get_address());
			//bool from_predecessor = rf24_mac_addr_equal(&mac_frame->transmitter, &topology_predecessor);

			if(addressed_to_me)
			{
				// Notice predecessor TR received
				predecessor_linkstate = CONNECTED;

				// Send ACK
				// MAKE SURE TASK PIPE IS EMPTY (DISCARD ALL OTHER TRANSMISSIONS)
				rf24_mac_send_ack(mac_frame);

				// Stop TRM timeout timer
				uint32_t t_us_trm_timeout_remaining = rf24_worker_stop_timer(trm_timeout);

				// Start NUM timeout timer
				rf24_worker_start_timer(num_timeout, us, t_us_trm_timeout_remaining, num_timeout_handler);


				uint8_t n_successors = rf24_network_count_successors();

				if(n_successors > 0)
				{
					struct rf24_neighbor_collection* available_successors = rf24_network_collect_neighbors(SUCCESSOR, AVAILABLE);

					if(available_successors->length > 0)
					{
						// get_next_successor
					}
				}
				else
				{
					// Start NUM-Timeout

				}
			}

			break;
		}
	}
}

void trm_timeout_handler()
{
	if(!controller)
	{
		rf24_debug(CONTROLLER, TIMEOUT, VOID, VOID, NULL, "TR-Timeout, ID: %d\n", topology_cycle_id);

		// Re-send NUM(-Reply) to predessor
		rf24_network_broadcast_num();



	}
}

void num_timeout_handler()
{
	// Check if predecessor link state is established
	if(predecessor_linkstate == CONNECTED)
	{
		rf24_debug(CONTROLLER, TIMEOUT, VOID, VOID, NULL, "TR-Timeout, ID: %d\n", topology_cycle_id);
		//rf24_network_transfer_topology();
	}
}

void rf24_network_transmission_successfull()
{
	switch(rf24_mac_get_transmission()->frame_subtype)
	{
		case TOPOLOGY_REPLY_MESSAGE:
		{
			// Notice TRM transmission successfull
			rf24_network_set_neighbor_state(rf24_mac_get_transmission()->transmitter, CONNECTED);
		}
		default: break;
	}
}

void rf24_network_transmission_failed()
{
	switch(rf24_mac_get_transmission()->frame_subtype)
	{
		case TOPOLOGY_REPLY_MESSAGE:
		{
			//Re-send TRM

			//rf24_network_set_neighbor_state(rf24_mac_get_transmission()->transmitter, NO_LINK);
		}
		default: break;
	}
}


void rf24_network_topology_received()
{
	if(predecessor_linkstate != CONNECTED) return;

	// Check if all successors transferred valid topologies
	bool successor_integrity = rf24_network_check_successor_integrity();

	if(successor_integrity)
	{
		// Stop timer
		uint32_t t_us_num_timeout_remaining = rf24_worker_stop_timer(num_timeout);

		if(!controller)
		{
			rf24_debug(NETWORK, INFO, VOID, VOID, NULL,
				"Sub-topologies complete, transfer topology to %s\n",
				decimal_to_string(topology_predecessor.bytes, 6, ':'));

			// Transfer topology to predecessor
			rf24_network_transfer_topology(topology_predecessor);
		}
		else
		{
			struct rf24_timespan timespan = rf24_worker_us_to_timespan(t_us_num_timeout_remaining);

			rf24_printf("%-10s Topology update cycle %d terminated after %ds %dms %dus\n\n", "controller",
						topology_cycle_id,timespan.s, timespan.ms, timespan.us);

			rf24_network_print_neighbors();
			rf24_printf("%-10s %s", "", "---------------------------------------------------------\n");
			rf24_network_print_topology();
		}
	}
}

void rf24_network_init()
{
	rf24_module_init(115200);
	rf24_worker_init();
	rf24_mac_init();

	rf24_mac_addr addr1 = { .bytes = { 1, 2, 3, 4, 5, 6 } };
	rf24_mac_addr addr2 = { .bytes = { 2, 2, 3, 4, 5, 6 } };
	rf24_mac_addr addr3 = { .bytes = { 3, 2, 3, 4, 5, 6 } };
	rf24_mac_addr addr4 = { .bytes = { 4, 2, 3, 4, 5, 6 } };

	rf24_network_add_neighbor(addr1, 0, NEIGHBOR, AVAILABLE);
	rf24_network_add_neighbor(addr2, 0, SUCCESSOR, AVAILABLE);
	rf24_network_add_neighbor(addr3, 0, SUCCESSOR, CONNECTED);
	rf24_network_add_neighbor(addr4, 0, SUCCESSOR, AVAILABLE);

	rf24_network_print_neighbors();

	struct rf24_neighbor_collection* available_successors = rf24_network_collect_neighbors(SUCCESSOR, AVAILABLE);
	rf24_printf("available successors: %d \n", available_successors->length);

	rf24_network_print_neighbor_collection(available_successors);

}

void rf24_network_reset_topology()
{
	// Reset flags
	memset(&network_flags, 0, sizeof(rf24_network_flags));

	// Reset neighbor list
	rf24_network_reset_neighbors();

	// Reset topology list
	free(topology);
	topology = NULL;

	topology_length = 0;
	topology_neighbors_length = 0;
}

void rf24_network_start_topology_update()
{
	rf24_network_reset_topology();

	topology_cycle_id++;

	hops_to_controller = 0;

	controller = true;

	rf24_network_broadcast_num();
}

void rf24_network_broadcast_num()
{
	rf24_mac_transmission *transmission = rf24_mac_setup_transmission(
			BROADCAST,
			TOPOLOGY,
			TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE,
			rf24_mac_get_broadcast_address(),
			NULL,
			0);

	// Get access to medium
	rf24_csma_ca_access_medium(transmission, 0, rf24_network_send_num);
}

// CALLBACK
void rf24_network_send_num()
{
	rf24_module_tx_data tx_data;
	rf24_mac_frame mac_frame;
	rf24_mac_addr *receiver;

	if(!controller)
		receiver = &topology_predecessor;
	else
		receiver = rf24_mac_get_broadcast_address();

	// Construct MAC frame
	mac_frame.frame_control.type = TOPOLOGY;
	mac_frame.frame_control.subtype = TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE;
	mac_frame.frame_control.from_distribution = true;
	mac_frame.transmitter = *rf24_mac_get_address();
	mac_frame.receiver = *receiver;
	mac_frame.duration = 0;
	mac_frame.topology.id = topology_cycle_id;
	mac_frame.topology.hop_count = hops_to_controller;

	// Convert mac frame into byte stream
	rf24_mac_frame_to_tx_data(&mac_frame, &tx_data);

	// Transmit
	rf24_module_transmit(&tx_data);

	// Start timeouts
	rf24_worker_start_timer(trm_timeout, ms, 2000, trm_timeout_handler);
	rf24_worker_start_timer(num_timeout, ms, 1000, num_timeout_handler);

	//struct rf24_timespan timespan = rf24_worker_us_to_timespan(topology_timeout_us);

	rf24_debug(	NETWORK, TRANSMIT, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, rf24_mac_get_broadcast_address(),
				"ID: %d, hop-count: %d (TR-Timeout: %dms)\n",
				topology_cycle_id, hops_to_controller, 1000);

	/*rf24_debug(	NETWORK, TRANSMIT, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, rf24_mac_get_broadcast_address(),
				"ID: %d, hop-count: %d (NUM-Timeout: %ds %dms %dus)\n",
				broadcast_topology_id, hops_to_controller, timespan.s, timespan.ms, timespan.us);*/
}
uint8_t	rf24_network_get_topology_cycle_id()
{
	return topology_cycle_id;
}

void rf24_network_add_neighbor(rf24_mac_addr mac_addr_neighbor, uint32_t t_response_us, rf24_neighbor_relation relation, rf24_neighbor_state state)
{
	// Create a new node
	struct rf24_neighbor *new_node = (struct rf24_neighbor*) malloc(sizeof(struct rf24_neighbor));

	memcpy (new_node->mac_addr.bytes, mac_addr_neighbor.bytes, 6);
  	new_node->t_response_us = t_response_us;
  	new_node->t_last_updated_ms = 0;
  	new_node->state = state;
  	new_node->relation = relation;
  	new_node->next = NULL;

  	// List is empty, new node becomes head node
  	if(neighbors == NULL){
  		neighbors = new_node;
  		neighbors_length++;
  		return;
  	}

  	// Start from the first node
  	struct rf24_neighbor *current_node = neighbors;

  	// Iterate over linked list
  	while(current_node != NULL)
  	{
  		// [1] Check if new node value (mac_addr) is lower than current node
  		int cmp_current = memcmp(new_node->mac_addr.bytes, current_node->mac_addr.bytes, 6);

  		if(cmp_current < 0){
  			// New node becomes head node
  			new_node->next = current_node;
  			neighbors = new_node;
  			neighbors_length++;
  			return;
  		}

  		// [2] Check if new node value (mac_addr) is equal
  		if(cmp_current == 0)
  		{
  			// Node is already in list, therefore update metric
  			current_node->t_response_us = new_node->t_response_us;
  			current_node->state = state;
  			current_node->relation = relation;
  			current_node->t_last_updated_ms = 0;
  			free(new_node);
  			return;
  		}

  		// [3] check if current node is last node
  		if(cmp_current > 0 && current_node->next == NULL){
  			// append new node at end of list
  			current_node->next = new_node;
  			neighbors_length++;
  			return;
  		}

  		// [4] check if node value (mac_addr) is between two nodes
  		int cmp_next = memcmp(new_node->mac_addr.bytes, current_node->next->mac_addr.bytes, 6);

  		if(cmp_current > 0 && cmp_next < 0){
  			// append new node between current and next
  			new_node->next = current_node->next;
  			current_node->next = new_node;
  			neighbors_length++;
  			return;
  		}

  		// goto next node
  		current_node = current_node->next;
   }
}

uint8_t rf24_network_count_neighbors(rf24_neighbor_relation relation, rf24_neighbor_state state)
{
	struct rf24_neighbor *current_node = neighbors;
	uint8_t count = 0;

	while(current_node != NULL)
	{
		if(current_node->state == state && current_node->relation == relation) count++;
		current_node = current_node->next;
	}

	return count;
}

struct rf24_neighbor_collection* rf24_network_collect_neighbors(rf24_neighbor_relation relation, rf24_neighbor_state state)
{
	// Start from the first node
	struct rf24_neighbor *current_node = neighbors;
	uint8_t count = 0;

	// Count matching neighbors
	while(current_node != NULL)
	{
		if(current_node->state == state && current_node->relation == relation) count++;
		current_node = current_node->next;
	}

	// Malloc array for pointers to neighbors
	neighbor_collection.nodes = malloc(count * sizeof(struct rf24_neighbor*));
	neighbor_collection.length = count;

	current_node = neighbors;
	count = 0;

	// Copy pointer of matching neighbors into result set
	while(current_node != NULL)
	{
		if(current_node->state == state && current_node->relation == relation) neighbor_collection.nodes[count++] = current_node;
		current_node = current_node->next;
	}

	return &neighbor_collection;
}

void rf24_network_print_neighbor_collection(struct rf24_neighbor_collection *collection)
{
	for(int i=0; i < neighbor_collection.length; i++)
	{
		rf24_printf("%-10s %d: %d:%d:%d:%d:%d:%d (%s, %s)\n", "neighbor",
			i+1,
			collection->nodes[i]->mac_addr.bytes[0],
			collection->nodes[i]->mac_addr.bytes[1],
			collection->nodes[i]->mac_addr.bytes[2],
			collection->nodes[i]->mac_addr.bytes[3],
			collection->nodes[i]->mac_addr.bytes[4],
			collection->nodes[i]->mac_addr.bytes[5],
			rf24_neighbor_relation_string[collection->nodes[i]->relation],
			rf24_neighbor_states_string[collection->nodes[i]->state]);
	}
}

struct rf24_neighbor* rf24_network_get_neighbor(rf24_mac_addr *mac_addr)
{
	// start from the first node
	struct rf24_neighbor *current_node = neighbors;

	// iterate over list
	while(current_node != NULL)
	{
		uint8_t cmp_current = memcmp(current_node->mac_addr.bytes, mac_addr->bytes, 6);

		if(cmp_current == 0) return current_node;

		current_node = current_node->next;
	}

	return NULL;
}

void rf24_network_set_neighbor_state(rf24_mac_addr mac_addr, rf24_neighbor_state state)
{
	struct rf24_neighbor *neighbor = rf24_network_get_neighbor(&mac_addr);

	if(neighbor) neighbor->state = state;
}

rf24_neighbor_state rf24_network_get_neighbor_state(rf24_mac_addr *mac_addr)
{
	struct rf24_neighbor *neighbor = rf24_network_get_neighbor(mac_addr);

	if(neighbor)
		return neighbor->state;
	else
		return NO_LINK;
}

void rf24_network_reset_neighbors()
{
	free(neighbors);
	neighbors = NULL;
	neighbors_length = 0;
}

void rf24_network_update_neighbors()
{
	struct rf24_neighbor *current_node = neighbors;

	while(current_node != NULL)
	{
		current_node->t_last_updated_ms += T_BROADCAST_TOPOGLOY_MS;

		if(current_node->t_last_updated_ms >= T_NEIGHBOUR_OUTDATED_MS) current_node->state = TIMED_OUT;

		current_node = current_node->next;
	}
}

uint8_t rf24_network_count_successors()
{
	struct rf24_neighbor *current_node = neighbors;
	uint8_t count = 0;

	while(current_node != NULL)
	{
		if(current_node->relation == SUCCESSOR) count++;
		current_node = current_node->next;
	}

	return count;
}

void rf24_network_print_neighbors()
{
	// Start from the first node
	struct rf24_neighbor *current_node = neighbors;
	uint8_t index = 1;

	// Iterate over list
	while(current_node != NULL){

		rf24_printf("%-10s %d: %d:%d:%d:%d:%d:%d (%s, %s)\n", "neighbor",
			index++,
			current_node->mac_addr.bytes[0],
			current_node->mac_addr.bytes[1],
			current_node->mac_addr.bytes[2],
			current_node->mac_addr.bytes[3],
			current_node->mac_addr.bytes[4],
			current_node->mac_addr.bytes[5],
			rf24_neighbor_relation_string[current_node->relation],
			rf24_neighbor_states_string[current_node->state]);

		current_node = current_node->next;
	}
}


bool rf24_network_check_successor_integrity()
{
	bool topology_received;
	struct rf24_neighbor *current_neighbor = neighbors;

	// Iterate over neighbors
	while(current_neighbor != NULL)
	{
		struct rf24_topology *current_topology = topology;
		topology_received = false;

		if(current_neighbor->relation == SUCCESSOR && current_neighbor->state == CONNECTED)
		{
			// Iterate over topologies
			while(current_topology != NULL)
			{
				if(memcmp(current_neighbor->mac_addr.bytes, &current_topology->neighbor->mac_addr, 6) == 0)
				{
					topology_received = true;
				}

				current_topology = current_topology->next;
			}

			if(!topology_received) return false;
		}

		current_neighbor = current_neighbor->next;
	}

	return true;
}

struct rf24_topology* rf24_network_topology_get_last()
{
	struct rf24_topology *current_topology = topology;

	while(current_topology->next != NULL) current_topology = current_topology->next;

	return current_topology;
}

void rf24_network_rx_data_to_topology(uint8_t *rx_data, uint8_t length)
{
	// Initialize a topology to hold sub topology
	struct rf24_topology *sub_topology = (struct rf24_topology*) malloc(sizeof(struct rf24_topology));
	sub_topology->neighbor = NULL;
	sub_topology->next = NULL;

	uint8_t index = 0;
	struct rf24_topology *current_topology = sub_topology;

	while(index < length)
	{
		if(rx_data[index] == 0x1C)
		{
			// Create a new topology
			struct rf24_topology *new_topology = (struct rf24_topology*) malloc(sizeof(struct rf24_topology));
			new_topology->neighbor = NULL;

			// Attach new topology
			current_topology->next = new_topology;
			current_topology = new_topology;

			topology_length++;
			index++;
		}
		else
		{
			// Create a new neighbor
			struct rf24_neighbor *new_neighbor = (struct rf24_neighbor*) malloc(sizeof(struct rf24_neighbor));
			new_neighbor->next = NULL;

			// Copy MAC address
			memcpy(&new_neighbor->mac_addr, &rx_data[index], 6);

			// If current topology is empty, insert new new neighbor at head
			if(current_topology->neighbor == NULL) current_topology->neighbor = new_neighbor;
			else
			{
				struct rf24_neighbor *current_neighbor = current_topology->neighbor;

				// Goto end of neighbor list
				while(current_neighbor->next != NULL) current_neighbor = current_neighbor->next;

				// Attach new neighbor to and of neighbor list
				current_neighbor->next = new_neighbor;
			}

			topology_neighbors_length++;
			index += 6;
		}
	}

	// Close sub topology
	current_topology->next = NULL;

	// Attach sub topology to overall topology
	if(topology == NULL) topology = sub_topology;
	else rf24_network_topology_get_last()->next = sub_topology;
}

void rf24_network_topology_to_tx_data(uint8_t *tx_data, uint8_t length)
{
	uint8_t index = 0;

	struct rf24_topology *current_topology = topology ;

	while(current_topology != NULL)
	{
		struct rf24_neighbor *current_neighbor = current_topology->neighbor;

		while(current_neighbor != NULL)
		{
			// Copy MAC address of current node into rx_data array
			memcpy(&tx_data[index], &current_neighbor->mac_addr, 6);
			// Increment index
			index += 6;
			// Next neighbor
			current_neighbor = current_neighbor->next;
		}

		// Insert ASCII file separator(s) between topologies
		if(index < length) tx_data[index++] = 0x1C;
		// Next topology
		current_topology = current_topology->next;
	}
}

void rf24_network_print_topology()
{
	bool head = true;
	uint8_t topology_count = 1;
	uint8_t neighbor_count = 0;

	struct rf24_topology *current_topology = topology;

	while(current_topology != NULL)
	{
		struct rf24_neighbor *current_neighbor = current_topology->neighbor;
		neighbor_count = 0;

		while(current_neighbor != NULL)
		{
			if(!head) rf24_printf("%-13s %d: ", "", neighbor_count);
			else rf24_printf("%-10s %d: ", "topology", topology_count);

			head = false;

			rf24_printf("%d:%d:%d:%d:%d:%d\n",
				current_neighbor->mac_addr.bytes[0],
				current_neighbor->mac_addr.bytes[1],
				current_neighbor->mac_addr.bytes[2],
				current_neighbor->mac_addr.bytes[3],
				current_neighbor->mac_addr.bytes[4],
				current_neighbor->mac_addr.bytes[5]);

			current_neighbor = current_neighbor->next;
			neighbor_count++;
		}

		current_topology = current_topology->next;
		topology_count++;
		head = true;
	}
}

void rf24_network_transfer_topology()
{
	//rf24_worker_stop_timer();

	// (1) 	Insert myself (as a neighbor node) at head position of neighbors list

	struct rf24_neighbor *new_neighbor = (struct rf24_neighbor*) malloc(sizeof(struct rf24_neighbor));

	new_neighbor->mac_addr = *rf24_mac_get_address();

	new_neighbor->next = neighbors;
	neighbors = new_neighbor;

	neighbors_length++;

	// (2) Insert my neighbors into topology at head position (before topologies of successors)

	struct rf24_topology *new_topology = (struct rf24_topology*) malloc(sizeof(struct rf24_topology));

	new_topology->neighbor = neighbors;
	new_topology->next = topology;
	topology = new_topology;

	topology_length++;
	topology_neighbors_length += neighbors_length;

	// (3) Covert topology into byte stream

	//  number of bytes required to hold topology
	uint8_t tx_data_length =
			topology_neighbors_length * 6 + // number of neighbors in topology * 6 Bytes for MAC address */
			topology_length - 1; 			// (number of topologies - 1) Bytes for separator symbols*/

	uint8_t tx_data[tx_data_length];
	rf24_network_topology_to_tx_data(tx_data, tx_data_length);

	// (4) Transfer topology to topology predecessor

	rf24_mac_transfer_data(UNICAST, &topology_predecessor, DATA_TOPOLOGY, tx_data, tx_data_length);
}



















