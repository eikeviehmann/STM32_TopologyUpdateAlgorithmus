#include "rf24_network.h"
#include "rf24_worker.h"
#include "rf24_csma_ca.h"
#include "../rf24_debug.h"

bool 								controller = false;				// Node is or is not controller
uint8_t								topology_cycle_id = 0;			// Broadcast topology id
uint8_t								hops_to_controller = 0;			// Number of hops to controller
rf24_network_flags					network_flags = { false, false, false, false, false, false, false, false };

struct rf24_neighbor*				predecessor;					// Predecessor
struct rf24_neighbor* volatile 		neighbors = NULL;				// Neighbors
uint8_t 							neighbors_length = 0;			// number of local neighbors
struct rf24_topology* volatile 		topology = NULL;				// Topologies
uint8_t								topology_length = 0; 			// Number of topologies
uint8_t								topology_neighbors_length = 0;	// Number of neighbors in topologies

struct rf24_neighbor_collection		neighbor_collection;			// Collection to filtered neighbors

static const char 					*rf24_neighbor_link_states_string[] = { "AVAILABLE", "CONNECTED", "TIMED_OUT", "NO_LINK",};
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

	if(rf24_mac_addr_equal(&predecessor->mac_addr, &receiver)) mac_frame.topology.successor = true;

	// Transfer TRM (CSMA/CA)
	rf24_mac_transfer_frame(UNICAST, &mac_frame);
}

void rf24_network_mark_as_controller()
{
	controller = true;
	rf24_debug(NETWORK, INFO, VOID, VOID, NULL, "Node has been classified as CONTROLLER\n");

}

void rf24_network_mark_as_node()
{
	controller = false;
	rf24_debug(NETWORK, INFO, VOID, VOID, NULL, "Node has been classified as NETWORK NODE\n");
}

rf24_network_flags* rf24_network_get_flags()
{
	return &network_flags;
}

struct rf24_topology* rf24_network_get_topology()
{
	return topology;
}

struct rf24_neighbor* rf24_network_get_neighbors()
{
	return neighbors;
}

void rf24_network_frame_received_handler(rf24_mac_frame *mac_frame)
{
	switch(mac_frame->frame_control.subtype)
	{
		/* Neighbor Update Message (NUM) */
		case TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE:
		{
			// Received a NUM with a new topology cycle id
			if(mac_frame->topology.id != rf24_network_get_topology_cycle_id() && !controller)
			{
				// Reset Routine
				rf24_network_reset();

				rf24_module_flush_tx();
				rf24_module_flush_rx();

				// Mark as NETWORK NODE
				rf24_network_mark_as_node();

				// Update topology cycle id
				rf24_network_set_topology_cycle_id(mac_frame->topology.id);

				// Set hop count
				rf24_network_set_hopcount(mac_frame->topology.hop_count + 1);

				// Set predecessor to transmitter of NUM frame
				predecessor = rf24_network_add_neighbor(&mac_frame->transmitter, 0, PREDECESSOR, AVAILABLE);

				rf24_debug(	NETWORK, RECEIVE, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, &mac_frame->transmitter,
							"PREDECESSOR evaluated, broadcast NUM (id: %d, hop-count: %d)\n",
							mac_frame->topology.id, mac_frame->topology.hop_count);

				// Broadcast NUM(predecessor)
				rf24_network_broadcast_num();

				// Start TRM timeout
			}
			else
			{
				// Case 1: NUM from predecessor (transmitter == predecessor)
				//_________________________________________________________________________________________________________
				bool from_predecessor = rf24_mac_addr_equal(&mac_frame->transmitter, &predecessor->mac_addr);

				if(from_predecessor)
				{

				}

				// Case 2: NUM from successor (transmitter = successor && receiver == this)
				//_________________________________________________________________________________________________________
				bool addressed_to_me = rf24_mac_addr_equal(&mac_frame->receiver, rf24_mac_get_address());

				if(addressed_to_me)
				{
					// Append neighbor as SUCCESSOR with link state AVAILABLE
					rf24_network_add_neighbor(&mac_frame->transmitter, 0, SUCCESSOR, AVAILABLE);

					// Transmit TRM in case node is not busy
					if(!rf24_mac_transmission_is_active())
					{
						// Transmit TRM in case predecessor link state has already been verified
						if(predecessor->link_state == CONNECTED || controller)
						{
							rf24_debug(	NETWORK, RECEIVE, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, &mac_frame->transmitter,
										"SUCCESSOR evaluated, transfer TRM (id: %d, hop-count: %d)\n",
										mac_frame->topology.id, mac_frame->topology.hop_count);

							rf24_network_transfer_trm(mac_frame->transmitter);
						}
						else rf24_debug(NETWORK, RECEIVE, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, &mac_frame->transmitter,
										"SUCCESSOR evaluated, wait for predecessor TRM (id: %d, hop-count: %d)\n",
										mac_frame->topology.id, mac_frame->topology.hop_count);
					}
					else rf24_debug(NETWORK, RECEIVE, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, &mac_frame->transmitter,
									"SUCCESSOR evaluated, query ignored while busy (id: %d, hop-count: %d)\n",
									mac_frame->topology.id, mac_frame->topology.hop_count);
				}

				// Case 3: NUM from neighbor (transmitter != predecessor/successor && receiver != this)
				//_________________________________________________________________________________________________________
				if(!addressed_to_me)
				{
					// Append neighbor
					rf24_network_add_neighbor(&mac_frame->transmitter, 0, NEIGHBOR, AVAILABLE);

					rf24_debug(	NETWORK, RECEIVE, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, &mac_frame->transmitter,
								"NEIGHBOR evaluated (id: %d, hop-count: %d\n",
								mac_frame->topology.id, mac_frame->topology.hop_count);
				}
			}
			break;
		}
	}
}

void rf24_network_trm_timeout_handler()
{
	if(!controller)
	{
		if(rf24_mac_transmission_is_active())
		{
			//rf24_debug(CONTROLLER, TIMEOUT, VOID, VOID, NULL, "[TRM-Timeout] (I AM BUSY)\n");
			return;
		}
		else
		{
			rf24_debug(CONTROLLER, TIMEOUT, TOPOLOGY_REPLY_MESSAGE, VOID, NULL, "retransmit NUM\n");

			// Re-send NUM(-Reply) to predecessor
			rf24_network_broadcast_num();
		}
	}
}

void rf24_network_num_timeout_handler()
{
	uint8_t n_successors = rf24_network_count_successors();

	if(n_successors > 0)
	{
		rf24_debug(NETWORK, TIMEOUT, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, NULL, "wait for successor topology data\n");
		// Wait for topology data
		// Start an topology timeout timer
	}
	else
	{
		if(!controller)
		{
			rf24_debug(NETWORK, TIMEOUT, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, NULL, "identified as leaf, transfer topology\n");
			rf24_network_transfer_topology(&predecessor->mac_addr);
		}
	}
}

void rf24_network_topology_timeout_handler()
{

}

void rf24_network_reception_successfull_handler()
{
	switch(rf24_mac_get_transmission()->frame_subtype)
	{
		case TOPOLOGY_REPLY_MESSAGE:
		{
			bool from_predecessor = rf24_mac_addr_equal(&rf24_mac_get_transmission()->transmitter, &predecessor->mac_addr);

			if(from_predecessor)
			{
				predecessor->link_state = CONNECTED;

				// Stop TRM timeout
				rf24_worker_stop_timer(trm_timeout);

				// Start NUM timeout timer
				struct rf24_timer *timer = rf24_worker_get_timer(num_timeout);

				if(timer)
					rf24_worker_reset_timer(num_timeout);
				else
					timer = rf24_worker_start_timer(num_timeout, us, T_NUM_TIMEOUT_US, rf24_network_num_timeout_handler);

				rf24_timespan timespan = rf24_worker_us_to_timespan(T_NUM_TIMEOUT_US);

				rf24_debug(NETWORK, INFO, VOID, VOID, NULL, "NUM-Timeout started (%ds %dms %dus)\n", timespan.s, timespan.ms, timespan.us);
			}

			break;
		}
		case DATA_TOPOLOGY:
		{
			rf24_network_topology_received_handler();
			break;
		}
		default: break;
	}
}

void rf24_network_transmission_successfull_handler()
{
	switch(rf24_mac_get_transmission()->frame_subtype)
	{
		case TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE:
		{
			if(!controller)
			{
				rf24_worker_start_timer(trm_timeout, us, T_TRM_TIMEOUT_US, rf24_network_trm_timeout_handler);

				rf24_timespan timespan = rf24_worker_us_to_timespan(T_TRM_TIMEOUT_US);

				rf24_debug(NETWORK, INFO, VOID, VOID, NULL, "TRM-Timeout started (%ds %dms %dus)\n", timespan.s, timespan.ms, timespan.us);
			}

			break;
		}

		case TOPOLOGY_REPLY_MESSAGE:
		{
			rf24_network_set_neighbor_link_state(rf24_mac_get_transmission()->receiver, CONNECTED);

			break;
		}

		case DATA_TOPOLOGY:
		{
			/*rf24_printf("%-10s Subtree transferred to %s\n\n", "network", decimal_to_string(predecessor->mac_addr.bytes, 6, ':'));
			rf24_network_print_neighbors(rf24_network_get_neighbors());
			rf24_printf("%-10s %s", "", "---------------------------------------------------------\n");
			rf24_network_print_topology(rf24_network_get_topology());*/
			break;
		}
		default: break;
	}
}

void rf24_network_transmission_failed_handler()
{
	switch(rf24_mac_get_transmission()->frame_subtype)
	{
		case TOPOLOGY_REPLY_MESSAGE:
		{

			break;
		}
		default: break;
	}
}

void rf24_network_topology_received_handler()
{
	// Convert topology byte stream into topology struct and attach to local topology
	struct rf24_topology* successor_topology =
			rf24_network_rx_data_to_topology(rf24_mac_get_transmission()->payload, rf24_mac_get_transmission()->payload_length);

	// Add successor topology to local topology
	rf24_network_add_successor_topology(successor_topology);

	// Notice successor topology received
	struct rf24_neighbor* successor = rf24_network_get_neighbor(&successor_topology->neighbor->mac_addr);

	if(successor)
	{
		successor->topology = topology;
		successor->topology_received = true;
	}

	if(predecessor->link_state == CONNECTED || controller)
	{
		// Check if all successors transferred valid topologies
		bool topology_integrity = rf24_network_check_topology_integrity();

		if(topology_integrity)
		{
			if(!controller)
			{
				rf24_debug(	NETWORK, INFO, VOID, VOID, NULL,
							"Sub-topologies complete, transfer topology to %s\n",
							decimal_to_string(predecessor->mac_addr.bytes, 6, ':'));

				// Transfer topology to predecessor
				rf24_network_transfer_topology(&predecessor->mac_addr);
			}
			else
			{
				// Stop timer
				uint32_t t_us_topology_timeout_remaining = rf24_worker_stop_timer(topology_timeout);
				rf24_timespan timespan = rf24_worker_us_to_timespan(t_us_topology_timeout_remaining);

				rf24_printf("%-10s Topology update cycle %d terminated after %ds %dms %dus\n\n", "controller", topology_cycle_id, timespan.s, timespan.ms, timespan.us);
				rf24_network_print_neighbors(rf24_network_get_neighbors());
				rf24_printf("%-10s %s", "", "---------------------------------------------------------\n");
				rf24_network_print_topology(rf24_network_get_topology());
			}
		}
	}
}

void rf24_network_init()
{
	rf24_module_init(115200);
	rf24_worker_init();
	rf24_mac_init();

	/*rf24_mac_addr addr1 = { .bytes = { 1, 2, 3, 4, 5, 6 } };
	rf24_mac_addr addr2 = { .bytes = { 2, 2, 3, 4, 5, 6 } };
	rf24_mac_addr addr3 = { .bytes = { 3, 2, 3, 4, 5, 6 } };
	rf24_mac_addr addr4 = { .bytes = { 4, 2, 3, 4, 5, 6 } };

	rf24_network_add_neighbor(&addr1, 0, NEIGHBOR, AVAILABLE);
	rf24_network_add_neighbor(&addr2, 0, SUCCESSOR, AVAILABLE);
	rf24_network_add_neighbor(&addr3, 0, SUCCESSOR, CONNECTED);
	rf24_network_add_neighbor(&addr4, 0, SUCCESSOR, AVAILABLE);

	rf24_network_print_neighbors();

	/*struct rf24_neighbor_collection* available_successors = rf24_network_collect_neighbors(SUCCESSOR, AVAILABLE);
	rf24_printf("available successors: %d \n", available_successors->length);

	rf24_network_print_neighbor_collection(available_successors);*/
}

void rf24_network_reset_topology()
{
	// Reset topology list
	free(topology);
	topology = NULL;

	topology_length = 0;
	topology_neighbors_length = 0;
}

void rf24_network_reset()
{
	predecessor = NULL;
	topology_cycle_id = 0;
	hops_to_controller = 0;

	memset(&network_flags, 0, sizeof(rf24_network_flags));

	rf24_network_reset_neighbors();
	rf24_network_reset_topology();
}

void rf24_network_start_topology_update()
{
	rf24_network_reset_neighbors();
	rf24_network_reset_topology();

	rf24_network_mark_as_controller();

	rf24_network_set_topology_cycle_id(topology_cycle_id + 1);

	rf24_module_flush_tx();
	rf24_module_flush_rx();

	rf24_network_broadcast_num();

	// Start topology timeout
	struct rf24_timer *topology_timeout_timer = rf24_worker_get_timer(topology_timeout);

	if(topology_timeout_timer)
		rf24_worker_reset_timer(topology_timeout_timer);
	else
		topology_timeout_timer = rf24_worker_start_timer(topology_timeout, s, 100, rf24_network_topology_timeout_handler);

	//rf24_debug_attach_timer(topology_timeout_timer);
}

void rf24_network_broadcast_num()
{
	rf24_mac_frame mac_frame;

	mac_frame.frame_control.type = TOPOLOGY;
	mac_frame.frame_control.subtype = TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE;
	mac_frame.transmitter = *rf24_mac_get_address();
	mac_frame.duration = 0;
	mac_frame.topology.id = topology_cycle_id;
	mac_frame.topology.hop_count = hops_to_controller;

	if(controller)
	{
		mac_frame.receiver = *rf24_mac_get_broadcast_address();
		mac_frame.frame_control.from_distribution = true;
	}
	else
	{
		mac_frame.receiver = predecessor->mac_addr;
	}

	rf24_mac_transfer_frame(BROADCAST, &mac_frame);
}

uint8_t	rf24_network_get_topology_cycle_id()
{
	return topology_cycle_id;
}

void rf24_network_set_topology_cycle_id(uint8_t cycle_id)
{
	topology_cycle_id = cycle_id;
}

void rf24_network_set_hopcount(uint8_t hopcount)
{
	hops_to_controller = hopcount;
}

uint8_t	rf24_network_get_hopcount()
{
	return hops_to_controller;
}

struct rf24_neighbor* rf24_network_add_neighbor(rf24_mac_addr *mac_addr_neighbor, uint32_t t_response_us, rf24_neighbor_relation relation, rf24_neighbor_link_state link_state)
{
	// Create a new node
	struct rf24_neighbor *new_node = (struct rf24_neighbor*) malloc(sizeof(struct rf24_neighbor));

	memcpy(new_node->mac_addr.bytes, mac_addr_neighbor->bytes, 6);
  	new_node->link_state = link_state;
  	new_node->relation = relation;
  	new_node->t_response_us = t_response_us;
  	new_node->t_last_updated_ms = 0;
  	new_node->hops_to_controller = 0;
  	new_node->topology = NULL;
  	new_node->topology_received = false;
  	new_node->next = NULL;

  	// List is empty, new node becomes head node
  	if(neighbors == NULL){
  		neighbors = new_node;
  		neighbors_length++;
  		return new_node;
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
  			return new_node;
  		}

  		// [2] Check if new node value (mac_addr) is equal
  		if(cmp_current == 0){
  			// Node is already in list, therefore update metric
  			free(new_node);
  			return current_node;
  		}

  		// [3] check if current node is last node
  		if(cmp_current > 0 && current_node->next == NULL){
  			// append new node at end of list
  			current_node->next = new_node;
  			neighbors_length++;
  			return new_node;
  		}

  		// [4] check if node value (mac_addr) is between two nodes
  		int cmp_next = memcmp(new_node->mac_addr.bytes, current_node->next->mac_addr.bytes, 6);

  		if(cmp_current > 0 && cmp_next < 0){
  			// append new node between current and next
  			new_node->next = current_node->next;
  			current_node->next = new_node;
  			neighbors_length++;
  			return new_node;
  		}

  		// Goto next node
  		current_node = current_node->next;
  	}

  	return NULL;
}

uint8_t rf24_network_count_neighbors(rf24_neighbor_relation relation, rf24_neighbor_link_state link_state)
{
	struct rf24_neighbor *current_node = neighbors;
	uint8_t count = 0;

	while(current_node != NULL)
	{
		if(current_node->link_state == link_state && current_node->relation == relation) count++;
		current_node = current_node->next;
	}

	return count;
}

struct rf24_neighbor_collection* rf24_network_collect_neighbors(rf24_neighbor_relation relation, rf24_neighbor_link_state link_state)
{
	// Start from the first node
	struct rf24_neighbor *current_node = neighbors;
	uint8_t count = 0;

	// Count matching neighbors
	while(current_node != NULL)
	{
		if(current_node->link_state == link_state && current_node->relation == relation) count++;
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
		if(current_node->link_state == link_state && current_node->relation == relation) neighbor_collection.nodes[count++] = current_node;
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
			rf24_neighbor_link_states_string[collection->nodes[i]->link_state]);
	}
}

struct rf24_neighbor* rf24_network_get_neighbor(rf24_mac_addr *mac_addr)
{
	// start from the first node
	struct rf24_neighbor *current_node = neighbors;

	// iterate over list
	while(current_node != NULL)
	{
		if(rf24_mac_addr_equal(&current_node->mac_addr, mac_addr)) return current_node;
		current_node = current_node->next;
	}

	return NULL;
}

void rf24_network_set_neighbor_link_state(rf24_mac_addr mac_addr, rf24_neighbor_link_state link_state)
{
	struct rf24_neighbor *neighbor = rf24_network_get_neighbor(&mac_addr);
	if(neighbor) neighbor->link_state = link_state;
}

rf24_neighbor_link_state rf24_network_get_neighbor_link_state(rf24_mac_addr *mac_addr)
{
	struct rf24_neighbor *neighbor = rf24_network_get_neighbor(mac_addr);

	if(neighbor)
		return neighbor->link_state;
	else
		return NO_LINK;
}

void rf24_network_reset_neighbors()
{
	free(neighbors);
	neighbors = NULL;
	neighbors_length = 0;
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

void rf24_network_print_neighbors(struct rf24_neighbor *neighbors)
{
	if(neighbors_length > 0)
	{
		// Start from the first node
		struct rf24_neighbor *current_node = neighbors;
		uint8_t index = 1;

		// Iterate over list
		while(current_node != NULL)
		{
			rf24_printf("%-10s %d: %d:%d:%d:%d:%d:%d (rel: %s, link: %s)\n", "network",
				index++,
				current_node->mac_addr.bytes[0],
				current_node->mac_addr.bytes[1],
				current_node->mac_addr.bytes[2],
				current_node->mac_addr.bytes[3],
				current_node->mac_addr.bytes[4],
				current_node->mac_addr.bytes[5],
				rf24_neighbor_relation_string[current_node->relation],
				rf24_neighbor_link_states_string[current_node->link_state]);

			current_node = current_node->next;
		}
	}
	else rf24_printf("%-10s no neighbors available\n", "network");
}


bool rf24_network_check_topology_integrity()
{
	if(neighbors_length > 0)
	{
		struct rf24_neighbor *current_neighbor = rf24_network_get_neighbors();

		// Iterate over neighbors
		while(current_neighbor != NULL)
		{
			if(current_neighbor->relation == SUCCESSOR && !current_neighbor->topology_received) return false;
			current_neighbor = current_neighbor->next;
		}

		return true;
	}

	return false;
}

struct rf24_topology* rf24_network_topology_get_last()
{
	struct rf24_topology *current_topology = rf24_network_get_topology();

	while(current_topology->next != NULL) current_topology = current_topology->next;

	return current_topology;
}

bool rf24_network_validate_topology()
{

}

struct rf24_topology* rf24_network_rx_data_to_topology(uint8_t *rx_data, uint8_t length)
{
	// Initialize a topology to hold sub topology
	struct rf24_topology *sub_topology = (struct rf24_topology*) malloc(sizeof(struct rf24_topology));
	sub_topology->neighbor = NULL;
	sub_topology->next = NULL;

	topology_length++;

	uint8_t index = 0;
	struct rf24_topology *current_topology = sub_topology;

	while(index < length)
	{
		if(rx_data[index] == 0x1C)
		{
			// Create a new topology
			struct rf24_topology *new_topology = (struct rf24_topology*) malloc(sizeof(struct rf24_topology));
			new_topology->neighbor = NULL;
			new_topology->next = NULL;

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

			// If current topology is empty, insert new neighbor at head
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

	return sub_topology;
}

void rf24_network_add_successor_topology(struct rf24_topology* successor_topology)
{
	if(topology == NULL)
		topology = successor_topology;
	else
		rf24_network_topology_get_last()->next = successor_topology;
}

void rf24_network_topology_to_tx_data(struct rf24_topology *topology, uint8_t *tx_data, uint8_t length)
{
	uint8_t index = 0;

	struct rf24_topology *current_topology = topology;

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

void rf24_network_print_topology(struct rf24_topology *topology)
{
	bool head = true;
	uint8_t topology_count = 1;
	uint8_t neighbor_count = 0;

	//rf24_printf("%-10s %d topology/ies with %d neighbor/s \n", "topology", topology_length, topology_neighbors_length);

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

void rf24_network_transfer_topology(rf24_mac_addr *receiver)
{
	// (1) 	Insert myself (as a neighbor node) at head position of neighbors list
	//_____________________________________________________________________________________________________

	struct rf24_neighbor *new_neighbor = (struct rf24_neighbor*) malloc(sizeof(struct rf24_neighbor));

	new_neighbor->mac_addr = *rf24_mac_get_address();

	new_neighbor->next = neighbors;
	neighbors = new_neighbor;

	neighbors_length++;

	// (2) Insert my neighbors into topology at head position (before topologies of successors)
	//_____________________________________________________________________________________________________

	struct rf24_topology *new_topology = (struct rf24_topology*) malloc(sizeof(struct rf24_topology));

	new_topology->neighbor = neighbors;
	new_topology->next = topology;
	topology = new_topology;

	topology_length++;
	topology_neighbors_length += neighbors_length;

	// (3) Covert topology into byte stream
	//_____________________________________________________________________________________________________

	// Calc amount of separator symbols needed
	uint8_t n_separators = 0;
	if(topology_length > 0) n_separators = topology_length - 1;

	// Overall neighbors in (all) topologies * 6 bytes for each MAC address */
	uint8_t tx_data_length = topology_neighbors_length * 6 + n_separators;

	uint8_t tx_data[tx_data_length];
	rf24_network_topology_to_tx_data(topology, tx_data, tx_data_length);

	// (4) Transfer topology to predecessor
	//_____________________________________________________________________________________________________

	rf24_mac_transfer_data(UNICAST, receiver, DATA_TOPOLOGY, tx_data, tx_data_length);
}




















