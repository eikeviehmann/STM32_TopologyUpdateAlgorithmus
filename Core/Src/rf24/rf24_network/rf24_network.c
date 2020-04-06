#include "rf24_network.h"
#include "rf24_worker.h"
#include "rf24_csma_ca.h"
#include "../rf24_debug.h"

bool 							controller = false;				// Node is or is not controller
uint8_t							broadcast_topology_id = 0;		// Broadcast topology id
rf24_mac_addr					topology_predecessor;			// MAC address of first NUM sender (topology predecessor)

struct rf24_neighbor* volatile 	neighbors = NULL;
struct rf24_topology* volatile 	topology = NULL;

uint8_t 						neighbors_length = 0;			// number of local neighbors
uint8_t							topology_length = 0; 			// number of topologies
uint8_t							topology_neighbors_length = 0;	// number of neighbors in topologies

uint8_t							hops_to_controller = 0;		// number of hops to controller

bool							topology_broadcasted = false;

rf24_network_flags				network_flags = { false, false, false, false, false, false, false, false };

rf24_network_predecessor_linkstate	predecessor_linkstate;


void rf24_network_transfer_nam(rf24_mac_addr receiver)
{
	// Build a NAM frame
	rf24_mac_frame mac_frame_reply;
	mac_frame_reply.frame_control.type = TOPOLOGY;
	mac_frame_reply.frame_control.subtype = TOPOLOGY_NEIGHBOR_ANSWER_MESSAGE;
	mac_frame_reply.transmitter = *rf24_mac_get_address();
	mac_frame_reply.receiver = receiver;
	mac_frame_reply.duration = T_NAV_FRAG_MS;
	mac_frame_reply.id = 1;
	mac_frame_reply.topology.successor = false;

	if(rf24_mac_addr_equal(&topology_predecessor, &receiver)) mac_frame_reply.topology.successor = true;

	// Transfer NAM (CSMA/CA)
	rf24_mac_transfer_frame(UNICAST, &mac_frame_reply);
}

void rf24_network_frame_received_handler(rf24_mac_frame *mac_frame)
{
	switch(mac_frame->frame_control.subtype)
	{
		/*  Neighbor Update Message (NUM) */
		case TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE:
		{
			// Check if NUM is of a new topology update cycle?
			if(mac_frame->topology.id != rf24_network_get_broadcast_topology_id())
			{
				rf24_network_reset_topology();

				controller = false;

				broadcast_topology_id = mac_frame->topology.id;

				hops_to_controller = mac_frame->topology.hop_count + 1;

				predecessor_linkstate = EVALUATED;
				topology_predecessor = mac_frame->transmitter;

				rf24_network_add_neighbor(mac_frame->transmitter, 0, PREDECESSOR, AVAILABLE);

				// Transfer NAM
				rf24_network_transfer_nam(mac_frame->transmitter);
			}
			else if(rf24_network_get_neighbor_state(&mac_frame->transmitter) != CONNECTED)
			{
				// Insert NUM transmitter to neighbor list
				rf24_network_add_neighbor(mac_frame->transmitter, 0, NEIGHBOR, AVAILABLE);

				// Transfer NAM
				rf24_network_transfer_nam(mac_frame->transmitter);

				rf24_debug(	NETWORK, RECEIVE, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, &mac_frame->transmitter,
							"ID: %d, hop-count: %d (reply NAM)\n",
							mac_frame->topology.id,
							mac_frame->topology.hop_count);
			}
			else
			{
				rf24_debug( NETWORK, RECEIVE, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, &mac_frame->transmitter,
							"ID: %d, hop-count: %d (ignore)\n",
							mac_frame->topology.id,
							mac_frame->topology.hop_count);
			}

			break;
		}

		case TOPOLOGY_NEIGHBOR_ANSWER_MESSAGE:
		{
			bool addressed_to_me = rf24_mac_addr_equal(&mac_frame->receiver, rf24_mac_get_address());

			if(addressed_to_me)
			{
				// ONLY TAKE RESPONSE TIME IF ID REPLY MATCHES ID SEND (FEHLT NOCH)!!
				uint32_t t_response_us = rf24_stm32f1xx_stop_stopwatch();

				if(mac_frame->topology.successor)
					rf24_network_add_neighbor(mac_frame->transmitter, t_response_us, SUCCESSOR, CONNECTED);
				else
					rf24_network_add_neighbor(mac_frame->transmitter, t_response_us, NEIGHBOR, CONNECTED);

				rf24_debug(NETWORK, RECEIVE, TOPOLOGY_NEIGHBOR_ANSWER_MESSAGE, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, &mac_frame->transmitter, "\n", "");

				// Send ACK
				rf24_mac_send_ack(mac_frame);
			}

			break;
		}
	}
}

void rf24_network_transmission_successfull()
{
	switch(rf24_mac_get_transmission()->frame_subtype)
	{
		// MAC frame is a ACK for a MANAGEMENT_NEIGHBOR_ANSWER_MESSAGE message
		case TOPOLOGY_NEIGHBOR_ANSWER_MESSAGE:
		{
			if(rf24_mac_addr_equal(&rf24_mac_get_transmission()->receiver, &topology_predecessor))
			{
				rf24_network_add_neighbor(rf24_mac_get_transmission()->receiver, 0, PREDECESSOR, CONNECTED);
				predecessor_linkstate = ESTABLISHED;
				rf24_network_forward_broadcast_topology();
			}
			else
			{
				rf24_network_add_neighbor(rf24_mac_get_transmission()->receiver, 0, NEIGHBOR, CONNECTED);

				if(predecessor_linkstate == EVALUATED)
				{
					rf24_network_transfer_nam(topology_predecessor);
				}
				else if(!network_flags.num_transmitted)
				{
					rf24_network_forward_broadcast_topology();
				}
			}

			/*// If ACK(NAM) is from NUM transmitter (topology_predecessor) proceed broadcasting NUM
			if(rf24_mac_addr_equal(&rf24_mac_get_transmission()->transmitter, &topology_predecessor))
			{
				// Notice, that I received an ACK(NAM) from NUM transmitter (topology_predecessor)
				rf24_network_get_flags()->nam_transmitted = true;

				// Update Predecessor to CONNECTED State
				rf24_network_add_neighbor(rf24_mac_get_transmission()->transmitter, 0, PREDECESSOR, CONNECTED);

				// Broadcast update topology (NUM)
				rf24_network_forward_broadcast_topology();
			}
			// ACK(NAM) is from another node
			else
			{
				rf24_network_flags *network_flags = rf24_network_get_flags();

				// In case I received a NUM but haven't replied with a NAM yet, transfer NAM (to topology predecessor)
				if(network_flags->num_received & !network_flags->nam_transmitted)
				{
					// Build a broadcast topology reply (NAM) frame
					rf24_network_transfer_nam(topology_predecessor);
				}

				// Priority 2: Check if I still need to transfer a NUM broadcast

				// In case I received a NUM but didn't broadcasted it myself, broadcast NUM

				if(network_flags->nam_transmitted & network_flags->num_received & !network_flags->num_transmitted)
				{
					// Broadcast update topology (NUM)
					rf24_network_forward_broadcast_topology();
				}
			}*/
		}
		default: break;
	}
}

void rf24_network_topology_received()
{
	// Check if all successors transferred valid topologies
	bool successor_integrity = rf24_network_check_successor_integrity();

	if(successor_integrity)
	{
		// Stop timer
		struct rf24_timespan timespan = rf24_worker_stop_timer();

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
			rf24_printf("%-10s Topology update cycle %d terminated after %ds %dms %dus\n\n", "controller",
						broadcast_topology_id,timespan.s, timespan.ms, timespan.us);

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

	/*
	rf24_network_test_topology();
	rf24_network_test_neighbors();

	uint8_t length = topology_neighbors_length * 6 + (topology_length - 1);
	uint8_t array[length];
	rf24_network_topology_to_tx_data(array, length);

	free(topology);
	topology = NULL;

	topology_length = 0;
	topology_neighbors_length = 0;

	rf24_network_rx_data_to_topology(array, length);
	rf24_network_rx_data_to_topology(array, length);

	length = 0;*/

	/*rf24_mac_addr addr1 = { .bytes = { 0x1, 0x1, 0x1, 0x1, 0x1, 0x1 } };
	rf24_mac_addr addr2 = { .bytes = { 0x2, 0x2, 0x2, 0x2, 0x2, 0x2 } };
	rf24_mac_addr addr3 = { .bytes = { 0x3, 0x3, 0x3, 0x3, 0x3, 0x3 } };

	rf24_network_add_neighbor(addr1, 0 , CONNECTED);
	rf24_network_add_neighbor(addr1, 0 , CONNECTED);
	rf24_network_add_neighbor(addr3, 0 , CONNECTED);*/
}

rf24_network_flags* rf24_network_get_flags()
{
	return &network_flags;
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

void rf24_network_start_broadcast_topology()
{
	// Reset topology
	rf24_network_reset_topology();

	// Update broadcast topology id
	broadcast_topology_id++;
	hops_to_controller = 0;
	controller = true;

	// Broadcast NUM
	rf24_network_forward_broadcast_topology();
}

void rf24_network_forward_broadcast_topology()
{
	rf24_mac_transmission *transmission = rf24_mac_setup_transmission(
			BROADCAST,
			TOPOLOGY,
			TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE,
			rf24_mac_get_broadcast_address(),
			NULL,
			0);

	// Get access to medium
	rf24_csma_ca_access_medium(
			transmission,
			0,
			rf24_network_send_broadcast_topology);
}

uint32_t rf24_network_calculate_topology_timeout()
{
	// t=(2^x -1) * st + DIFS + fd(SIZE_NeighborUpdateMessage) + fd(SIZE_NeighborAckMessage)

	uint32_t topology_timeout_us = T_NUM_NAM_HANDSHAKE_US;

	topology_timeout_us = topology_timeout_us * PWRTWO(N_NODES - hops_to_controller);
	//topology_timeout_us = topology_timeout_us * (N_NODES - hops_to_controller);

	return topology_timeout_us;
}

// CALLBACK
void rf24_network_send_broadcast_topology()
{
	rf24_module_tx_data tx_data;
	rf24_mac_frame mac_frame;

	// Construct MAC frame
	mac_frame.frame_control.type = TOPOLOGY;
	mac_frame.frame_control.subtype = TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE;
	mac_frame.frame_control.from_distribution = true;
	mac_frame.transmitter = *rf24_mac_get_address();
	mac_frame.receiver = *rf24_mac_get_broadcast_address();
	mac_frame.duration = 0;
	mac_frame.topology.id = broadcast_topology_id;
	mac_frame.topology.hop_count = hops_to_controller;

	// Convert frame into byte stream & transmit
	rf24_mac_frame_to_tx_data(&mac_frame, &tx_data);
	rf24_module_transmit(&tx_data);

	network_flags.num_transmitted = true;

	// Calculate topology timeout
	uint32_t topology_timeout_us = rf24_network_calculate_topology_timeout();

	rf24_worker_start_timer(
			num_timeout,
			us,
			topology_timeout_us,
			rf24_network_topology_timeout);

	struct rf24_timespan timespan = rf24_worker_us_to_timespan(topology_timeout_us);

	rf24_debug(	NETWORK, TRANSMIT, TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE, VOID, rf24_mac_get_broadcast_address(),
				"ID: %d, hop-count: %d (NUM-Timeout: %ds %dms %dus)\n",
				broadcast_topology_id, hops_to_controller,timespan.s, timespan.ms, timespan.us);
}


void rf24_network_topology_timeout()
{
	if(!controller)
	{
		rf24_debug(	NETWORK, TIMEOUT, VOID, VOID, NULL,
					"NUM-Timeout, transmit topology to predecessor %s\n",
					decimal_to_string(topology_predecessor.bytes, 6, ':'));

		rf24_network_transfer_topology();
	}
	else
	{
		rf24_debug(	CONTROLLER, TIMEOUT, VOID, VOID, NULL,
					"NUM-Timeout, cycle %d\n",
					broadcast_topology_id);
	}
}

uint8_t	rf24_network_get_broadcast_topology_id()
{
	return broadcast_topology_id;
}

rf24_mac_addr* rf24_network_get_topology_predecessor(void)
{
	// If node is controller, return its own MAC address
	if(controller) return rf24_mac_get_address();

	return &topology_predecessor;
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

struct rf24_neighbor* rf24_network_get_neighbor(rf24_mac_addr mac_addr)
{
	// start from the first node
	struct rf24_neighbor *current_node = neighbors;

	// iterate over list
	while(current_node != NULL)
	{
		uint8_t cmp_current = memcmp(current_node->mac_addr.bytes, mac_addr.bytes, 6);

		if(cmp_current == 0) return current_node;

		current_node = current_node->next;
	}

	return NULL;
}

rf24_neighbor_state rf24_network_get_neighbor_state(rf24_mac_addr *mac_addr)
{
	// Reference to head node of linked neighbor list
	struct rf24_neighbor *current_node = neighbors;

	// Iterate over linked neighbor list
	while(current_node != NULL)
	{
		// Compare MAC addresses
		uint8_t cmp_current = memcmp(current_node->mac_addr.bytes, mac_addr->bytes, 6);

		// If MAC address equal, return state of neighbor
		if(cmp_current == 0) return current_node->state;

		// Goto next node
		current_node = current_node->next;
	}

	// No match found, return no link state
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
			rf24_neighbor_states_string[current_node->state],
			rf24_neighbor_relation_string[current_node->relation]);

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

		// Is neighbor's state CONNECTED? (CONNECTED means node replied with a NAM to a NUM)
		//if(current_neighbor->state == CONNECTED)
		if(current_neighbor->relation == SUCCESSOR)
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
	rf24_worker_stop_timer();

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



















