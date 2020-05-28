#include "../rf24_network/rf24_mac.h"

#include "rf24_csma_ca.h"
#include "rf24_worker.h"
#include "rf24_network.h"
#include "../rf24_debug.h"

//_________________________________________________________________________________________________________________________________________________________________________
// Global variables & Constants

//volatile rf24_mac_flags	mac_flags;
rf24_mac_addr 				mac_addr;
rf24_mac_addr 				mac_addr_broadcast = { .bytes = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
rf24_mac_transmission 		mac_transmission;
rf24_mac_ping				ping;

rf24_mac_addr 				blacklist[5];
uint8_t						blacklist_length = 0;

#ifdef RF24_MAC_DEBUG

#endif

//_________________________________________________________________________________________________________________________________________________________________________
// Function declarations
// Interrupt service routines / callback functions


rf24_mac_addr* rf24_mac_get_blacklist()
{
	return blacklist;
}

uint8_t rf24_mac_get_blacklist_length()
{
	return blacklist_length;
}

void rf24_mac_blacklist_add(rf24_mac_addr *mac_addr)
{
	if(blacklist_length < 5)
	{
		blacklist[blacklist_length++] = *mac_addr;

		rf24_printf("%-10 %s has been blacklisted\n", "network", decimal_to_string(mac_addr->bytes, 6, ':'));
	}
}

void rf24_mac_frame_received_handler(rf24_mac_frame *mac_frame)
{
	bool addressed_to_me = rf24_mac_addr_equal(&mac_frame->receiver, &mac_addr);

	if(!addressed_to_me) return;

	switch(mac_frame->frame_control.type)
	{
		/* PING FRAME TREATMENT */
		case PING:
		{
			switch(mac_frame->frame_control.subtype)
			{
				case PING_REQUEST:
				{
					struct rf24_task *task = rf24_worker_build_task(send_ping, 1, 0, true);
					task->data.mac.mac_frame.id = mac_frame->id;
					task->data.mac.mac_frame.frame_control.type = PING;
					task->data.mac.mac_frame.frame_control.subtype = PING_REPLY;
					task->data.mac.mac_frame.transmitter = *rf24_mac_get_address();
					task->data.mac.mac_frame.receiver = mac_frame->transmitter;
					task->data.mac.mac_frame.duration = 0;
					rf24_worker_push(task, rf24_mac_send_mac_frame);

					break;
				}

				case PING_REPLY:
				{
					if(mac_frame->id == ping.requests->id)
					{
						ping.requests->state = REPLY_RECEIVED;
						ping.requests->t_response_us = rf24_worker_stop_timer(stopwatch);

						rf24_printf("%-10s ping reply %d received after %d \n", "ping", ping.requests->id, ping.requests->t_response_us);

						rf24_mac_process_ping();
					}
					break;
				}
			}
			break;
		}

		case TOPOLOGY:
		{
			switch(mac_frame->frame_control.subtype)
			{
				case TOPOLOGY_REPLY_MESSAGE:
				{
					rf24_debug(	MAC, RECEIVE, mac_transmission.frame_subtype, VOID, &mac_transmission.receiver,
								"(%d/%d)\n", mac_frame->id, mac_transmission.n_frames);

					// Attach payload
					rf24_mac_frame_to_payload(mac_frame, &mac_transmission);

					// Finalize reception if frame is last frame
					rf24_mac_process_reception();

					// Send ACK
					rf24_mac_send_ack(mac_frame);

					break;
				}
			}
			break;
		}
		case DATA:
		{
			rf24_debug(	MAC, RECEIVE, mac_transmission.frame_subtype, VOID, &mac_transmission.receiver,
						"(%d/%d) \"%s\"\n", mac_frame->id, mac_transmission.n_frames,
						decimal_to_string(mac_frame->data.payload, mac_frame->data.header.length, ' '));

			// Validate if MAC frame is part of an active transmission
			bool validation = rf24_mac_addr_equal(&mac_frame->transmitter, &rf24_mac_get_transmission()->transmitter);

			if(validation)
			{
				// Attach payload
				rf24_mac_frame_to_payload(mac_frame, &mac_transmission);

				// Finalize reception if frame is last frame
				rf24_mac_process_reception();
			}

			// Send ACK
			rf24_mac_send_ack(mac_frame);

			break;
		}

		case CONTROL: switch(mac_frame->frame_control.subtype) case CONTROL_ACK:
		{
			if(addressed_to_me)
			{
				bool in_transmission = rf24_mac_get_transmission()->transmission_type == transmission;
				// ACK frame is from tranmission partner (receiver)?
				bool from_receiver = rf24_mac_addr_equal(&mac_frame->transmitter, &rf24_mac_get_transmission()->receiver);
				// ACK frame ID matches ID of mac frame preciously sent
				bool id_match = mac_frame->ack.id == rf24_mac_get_transmission()->mac_frame.id;

				if(in_transmission && from_receiver && id_match)
				{
					rf24_debug(	MAC, RECEIVE, CONTROL_ACK, mac_frame->ack.subtype, &mac_frame->transmitter,
								"(%d/%d)\n", mac_frame->ack.id, mac_transmission.n_frames);

					// Pop "wait_for_ack" task from task stack
					if(rf24_worker_current_task()->task == wait_for_ack) rf24_worker_pop_task();

					// Send next data frame
					rf24_mac_process_transmission();
				}
			}

			break;
		}
	}
}

void rf24_mac_reception_successfull()
{
	switch(mac_transmission.frame_subtype)
	{
		case DATA_TOPOLOGY:
		case TOPOLOGY_REPLY_MESSAGE:
			rf24_network_reception_successfull_handler(); break;
		case DATA_DATA:
			rf24_mac_print_payload(&mac_transmission); break;
		default:
			break;
	}
}

void rf24_mac_transmission_successfull()
{
	switch(mac_transmission.frame_subtype)
	{
		case DATA_TOPOLOGY:
		case TOPOLOGY_REPLY_MESSAGE:
		case TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE:
			rf24_network_transmission_successfull_handler(); break;
		default:
			break;
	}
}

void rf24_mac_ack_timeout()
{
	// Only retransmit frame if attempts are lower than maximum retransmits
	if(mac_transmission.attempts < N_MAX_RETRANSMITS)
	{
		// Build a new "send_mac_frame" task
		struct rf24_task *task = rf24_worker_build_task(send_mac_frame, 1, T_SIFS_US, false);

		// Attach mac_frame to task
		task->data.mac.mac_frame = mac_transmission.mac_frame;

		// Attach function rf24_mac_send_frame to task
		rf24_worker_attach(task, rf24_mac_send_mac_frame);

		// Create a "wait_for_ack" task
		task = rf24_worker_build_task(wait_for_ack, 1, T_ACK_TIMEOUT_US, false);
		rf24_worker_attach(task, rf24_mac_ack_timeout);

		// Notice one more attempt
		mac_transmission.attempts++;

		rf24_debug(MAC, TIMEOUT, CONTROL_ACK, VOID, NULL, "retransmit frame with id %d (%d/%d)\n", mac_transmission.frame_count, mac_transmission.attempts, N_MAX_RETRANSMITS);
	}
	else
	{
		rf24_debug(MAC, TIMEOUT, CONTROL_ACK, VOID, NULL, "max retries reached, transmission cancelled\n");

		// Set transmission state to CANCELLED
		rf24_mac_set_transmission_state(CANCELLED);

		// Call notifications
		switch(mac_transmission.frame_type)
		{
			case TOPOLOGY:
				rf24_network_transmission_failed_handler();
				return;
			default:
				break;
		}
	}
}

// FUNCTIONS

void rf24_mac_init()
{
	// csma ca init
	rf24_cmsa_ca_init(rf24_mac_setup_reception);

	// build mac adress from stm32 uuid
	rf24_mac_build_address();

	rf24_printf("%-10s %s %s\n", "mac", "MAC_ADDRESS", decimal_to_string(mac_addr.bytes, 6, ':'));

	rf24_mac_print_timings();

	/*rf24_mac_addr mac_addr_142 = { .bytes = { 142, 186, 194, 248, 183, 183 } };
	rf24_mac_addr mac_addr_226 = { .bytes = { 226, 3, 199, 111, 67, 158 } };
	rf24_mac_addr mac_addr_238 = { .bytes = { 238, 2, 224, 165, 68, 145 } };
	rf24_mac_addr mac_addr_126 = { .bytes = { 126, 48, 81, 162, 173, 74 } };
	rf24_mac_addr mac_addr_38 = { .bytes = { 38, 199, 20, 119, 91, 227 } };

	rf24_mac_blacklist_add(&mac_addr_142);
	//rf24_mac_blacklist_add(&mac_addr_226);
	//rf24_mac_blacklist_add(&mac_addr_238);
	//rf24_mac_blacklist_add(&mac_addr_126);
	rf24_mac_blacklist_add(&mac_addr_38);*/
}

void rf24_mac_build_address()
{
	// Read md5 hashed UUID from stm32 UC
	uint8_t* uuid_md5hashed = rf24_stm32f1xx_get_uuid_md5hashed();

	// Take first 6 byte of md5 hashed UUID as MAC address
	for(int i=0; i<6; i++) mac_addr.bytes[i] = uuid_md5hashed[i];
}

bool rf24_mac_addr_equal(rf24_mac_addr *mac_addr1, rf24_mac_addr *mac_addr2)
{
	for(int i = 0; i <= 5; i++)
	{
		if (mac_addr1->bytes[i] != mac_addr2->bytes[i]) return false;
	}

	return true;
}

bool rf24_mac_transmission_is_active()
{
	return ( (mac_transmission.state == INITIALIZED) || (mac_transmission.state == ACTIVE) );
}

rf24_mac_transmission_state rf24_mac_get_transmission_state()
{
	return mac_transmission.state;
}

void rf24_mac_set_transmission_state(rf24_mac_transmission_state state)
{
	mac_transmission.state = state;

	if(mac_transmission.state == INITIALIZED)
		rf24_stm32f1xx_set_led_cycle(T_LED_CYCLE_TX_INITIALIZED_MS);

	if(mac_transmission.state == ACTIVE)
		rf24_stm32f1xx_set_led_cycle(T_LED_CYCLE_TX_ACTIVE_MS);

	rf24_stm32f1xx_set_led_cycle(T_LED_CYCLE_TX_IDLE_MS);
}

rf24_mac_transmission* rf24_mac_get_transmission()
{
	return &mac_transmission;
}

rf24_mac_addr* rf24_mac_get_address()
{
	return &mac_addr;
}

rf24_mac_addr* rf24_mac_get_broadcast_address()
{
	return &mac_addr_broadcast;
}

void rf24_mac_frame_to_tx_data(rf24_mac_frame *mac_frame, rf24_module_tx_data *tx_data)
{
	// Copy MAC frame (struct) into PAYLOAD (byte array)
	memcpy(tx_data->payload, (uint8_t*) mac_frame , 32);

	// PAYLOAD of nRF24L01 has a fixed 32 bit frame
	tx_data->length = 32;
}

void rf24_mac_rx_data_to_mac_frame(rf24_module_rx_data *rx_data, rf24_mac_frame *mac_frame)
{
	mac_frame = (rf24_mac_frame*) rx_data->payload;
}

void rf24_mac_process_transmission()
{
	// Transmission has not transferred bytes left?
	if(mac_transmission.payload_index < mac_transmission.payload_length)
	{
		// Reset attempts (each frame can be retransmitted 15 times)
		mac_transmission.attempts = 0;

		// Increase frame count
		mac_transmission.frame_count++;

		// Create a "send mac frame" task, wait a SIFS before transmission
		struct rf24_task *task = rf24_worker_build_task(send_mac_frame, 1, T_SIFS_US, false);

		// Assemble next data frame
		rf24_mac_payload_to_data_frame(&mac_transmission, &task->data.mac.mac_frame);

		// Bind send mac frame function to task
		rf24_worker_attach(task, rf24_mac_send_mac_frame);

		// Create a "wait for ack" task
		task = rf24_worker_build_task(wait_for_ack, 1, T_ACK_TIMEOUT_US, false);
		rf24_worker_attach(task, rf24_mac_ack_timeout);
	}
	else
	{
		if(mac_transmission.state != COMPLETED)
		{
			rf24_debug(	MAC, TRANSMISSION, mac_transmission.frame_subtype, VOID, &mac_transmission.receiver,
						"Transmission completed (%d frame/s, %d byte/s)\n\n", mac_transmission.frame_count, mac_transmission.payload_length);

			rf24_mac_transmission_successfull();
		}

		rf24_mac_set_transmission_state(COMPLETED);
	}
}

void rf24_mac_process_reception()
{
	if(mac_transmission.frame_count >= mac_transmission.n_frames)
	{
		if(mac_transmission.state != COMPLETED)
		{
			rf24_debug(	MAC, RECEPTION, mac_transmission.frame_subtype, VOID, &mac_transmission.transmitter,
						"Reception completed (%d frame/s, %d byte/s)\n\n", mac_transmission.frame_count, mac_transmission.payload_length);

			rf24_mac_reception_successfull();
		}

		rf24_mac_set_transmission_state(COMPLETED);
	}
}

void rf24_mac_send_mac_frame()
{
	// Create TX data container
	rf24_module_tx_data tx_data;

	// Collect mac_frame from task
	rf24_mac_frame mac_frame = rf24_worker_current_task()->data.mac.mac_frame;

	// Covert MAC frame into TX data
	rf24_mac_frame_to_tx_data(&mac_frame, &tx_data);

	// Transmit MAC frame (TX data)
	rf24_module_transmit(&tx_data);

	// Transmission (BROADCAST) is COMPLETED, when mac_frame was transmitted
	switch(mac_transmission.communication_type){
		case BROADCAST:
			rf24_mac_set_transmission_state(COMPLETED);
			rf24_mac_transmission_successfull();
			break;
		default:
			break;
	}

	if(mac_frame.frame_control.type != CONTROL)
	{
		if(mac_frame.frame_control.type == DATA)
			rf24_debug(	MAC, TRANSMIT, mac_transmission.frame_subtype, VOID, &mac_transmission.receiver,
						"(%d/%d) \"%s\"\n", mac_frame.id, mac_transmission.n_frames,
						decimal_to_string(mac_frame.data.payload, mac_frame.data.header.length, ' '));
		else rf24_debug(MAC, TRANSMIT, mac_transmission.frame_subtype, VOID, &mac_transmission.receiver,
						"(%d/%d)\n", mac_frame.id, mac_transmission.n_frames);
	}
}

void rf24_mac_send_ack(rf24_mac_frame *mac_frame)
{
	// Create send ACK task
	struct rf24_task *task = rf24_worker_build_task(send_ack, 1, T_SIFS_US, false);

	task->data.mac.mac_frame.frame_control.type = CONTROL;
	task->data.mac.mac_frame.frame_control.subtype = CONTROL_ACK;
	task->data.mac.mac_frame.transmitter = mac_addr;
	task->data.mac.mac_frame.receiver = mac_frame->transmitter;
	task->data.mac.mac_frame.duration = T_NAV_ACK_MS;
	task->data.mac.mac_frame.ack.type = mac_frame->frame_control.type;
	task->data.mac.mac_frame.ack.subtype = mac_frame->frame_control.subtype;
	task->data.mac.mac_frame.ack.id = mac_frame->id;

	// Bind send MAC frame function to task
	rf24_worker_push(task, rf24_mac_send_mac_frame);

	rf24_debug(	MAC, TRANSMIT, CONTROL_ACK, mac_frame->frame_control.subtype, &mac_frame->transmitter,
				"(%d/%d)\n", mac_frame->id, mac_transmission.n_frames);
}

void rf24_mac_payload_to_data_frame(rf24_mac_transmission* transmission, rf24_mac_frame* mac_frame)
{
	// Calculate remaining bytes to transfer
	uint8_t bytes_to_transfer = transmission->payload_length - transmission->payload_index;

	if( !(bytes_to_transfer > 0) ) return;

	mac_frame->frame_control.type = transmission->frame_type;
	mac_frame->frame_control.subtype = transmission->frame_subtype;
	mac_frame->transmitter = mac_addr;
	mac_frame->receiver = transmission->receiver;
	mac_frame->duration = T_NAV_FRAG_MS;
	mac_frame->id = transmission->frame_count;

	// More than MAC_FRAME_PAYLOAD_LENGTH bytes to transfer?
	if(bytes_to_transfer >= MAC_FRAME_PAYLOAD_LENGTH)
	{
		// Inform receiver more frames to come
		mac_frame->frame_control.more_data = true;
		// Add PAYLOAD length to data header
		mac_frame->data.header.length = MAC_FRAME_PAYLOAD_LENGTH;

		// Copy PAYLOAD data into MAC frame
		memcpy(mac_frame->data.payload, &transmission->payload[transmission->payload_index], MAC_FRAME_PAYLOAD_LENGTH);

		transmission->payload_index += MAC_FRAME_PAYLOAD_LENGTH;
	}
	else
	{
		// Inform receiver last MAC frame
		mac_frame->frame_control.more_data = false;
		// Add remaining PAYLOAD length to data header
		mac_frame->data.header.length = bytes_to_transfer;

		// Copy remaining bytes of PAYLOAD into MAC frame
		memcpy(mac_frame->data.payload, &transmission->payload[transmission->payload_index], bytes_to_transfer);

		// -> index = length -> transmission completed
		transmission->payload_index += bytes_to_transfer;
	}

	// Store a local copy for retransmission
	transmission->mac_frame = *mac_frame;
}

void rf24_mac_frame_to_payload(rf24_mac_frame* mac_frame, rf24_mac_transmission* transmission)
{
	// Packet is a new frame?
	if(mac_frame->id > transmission->frame_count)
	{
		switch(mac_frame->frame_control.subtype)
		{
			case DATA_DATA: case DATA_TOPOLOGY:
			{
				memcpy(	&transmission->payload[transmission->payload_index],
						mac_frame->data.payload,
						mac_frame->data.header.length);

				transmission->payload_index += mac_frame->data.header.length;

				break;
			}
			default: break;
		}

		transmission->frame_count = mac_frame->id;
	}
}

void rf24_mac_setup_reception(rf24_mac_frame *mac_frame)
{
	// Check, if RTS frame contains any information of following data
	switch(mac_frame->rts.type)
	{
		case DATA:
		{
			if(mac_frame->rts.n_bytes_payload > 0)
			{
				// Calculate number of frames needed for transmission
				uint8_t n_frames = mac_frame->rts.n_bytes_payload / MAC_FRAME_PAYLOAD_LENGTH;
				if(mac_frame->rts.n_bytes_payload % MAC_FRAME_PAYLOAD_LENGTH > 0) n_frames++;
				mac_transmission.n_frames = n_frames;

				// Malloc space for payload
				mac_transmission.payload = (uint8_t*) malloc(mac_frame->rts.n_bytes_payload * sizeof(uint8_t));
			}
			else mac_transmission.n_frames = 1;

			break;
		}
		default: break;
	}

	// Initialize mac_transmission for reception based on the given RTS meta information
	mac_transmission.transmission_type = reception;
	mac_transmission.frame_type = mac_frame->rts.type;
	mac_transmission.frame_subtype = mac_frame->rts.subtype;
	mac_transmission.frame_count = 0;
	mac_transmission.transmitter = mac_frame->transmitter;
	mac_transmission.receiver = *rf24_mac_get_address();
	mac_transmission.payload_length = mac_frame->rts.n_bytes_payload;
	mac_transmission.payload_index = 0;
	mac_transmission.attempts = 0;

	// Set transmission to state INITIALIZED
	rf24_mac_set_transmission_state(INITIALIZED);
}

rf24_mac_transmission* rf24_mac_setup_transmission(
	rf24_mac_communication_type communication_type,
	rf24_mac_frame_type frame_type,
	rf24_mac_frame_subtype frame_subtype,
	rf24_mac_addr *receiver,
	uint8_t *payload,
	uint8_t payload_length){

	// Setup MAC transmission
	mac_transmission.transmission_type = transmission;
	mac_transmission.communication_type = communication_type;
	mac_transmission.frame_type = frame_type;
	mac_transmission.frame_subtype = frame_subtype;
	mac_transmission.frame_count = 0;
	mac_transmission.transmitter = *rf24_mac_get_address();
	mac_transmission.receiver = *receiver;
	mac_transmission.payload_length = payload_length;
	mac_transmission.payload_index = 0;
	mac_transmission.attempts = 0;

	if(payload_length > 0)
	{
		// Calculate number of frames needed for transmission
		uint8_t n_frames = payload_length / MAC_FRAME_PAYLOAD_LENGTH;
		if(payload_length % MAC_FRAME_PAYLOAD_LENGTH > 0) n_frames++;
		mac_transmission.n_frames = n_frames;

		// Free PAYLOAD (of former transmission)
		free(mac_transmission.payload);

		// Allocate memory for PAYLOAD
		mac_transmission.payload = (uint8_t*) malloc(payload_length * sizeof(uint8_t));

		// Copy PAYLOAD to MAC transmission
		memcpy(mac_transmission.payload, payload, payload_length);
	}
	else mac_transmission.n_frames = 1;

	// Set transmission to state INITIALIZED
	rf24_mac_set_transmission_state(INITIALIZED);

	return &mac_transmission;
}

void rf24_mac_start_transmission()
{
	mac_transmission.attempts++;
	mac_transmission.frame_count++;

	//1)______________________________________________________________________________
	// Build "send_mac_frame" task
	struct rf24_task *task = rf24_worker_build_task(send_mac_frame, 1, T_SIFS_US, false);

	// In case transmission has payload, convert payload into mac_frame
	switch(mac_transmission.frame_type)
	{
		// If transmission is of type DATA, convert PAYLOAD into DATA_DATA frames
		case DATA:
			rf24_mac_payload_to_data_frame(&mac_transmission, &task->data.mac.mac_frame);
			break;
		// Otherwise take given MAC frame and proceed
		default:
			task->data.mac.mac_frame = mac_transmission.mac_frame;
			break;
	}

	// Attach "send_mac_frame" task to task pipe
	rf24_worker_attach(task, rf24_mac_send_mac_frame);

	// Notice transmission ACTIVE
	rf24_mac_set_transmission_state(ACTIVE);

	//2)______________________________________________________________________________
	// In case transmission is UNICAST, attach "wait_for_ack" task to task pipe
	switch(mac_transmission.communication_type)
	{
		case UNICAST:
			// Build "wait_for_ack" task
			task = rf24_worker_build_task(wait_for_ack, 1, T_ACK_TIMEOUT_US, false);
			// Attach to task pipe
			rf24_worker_attach(task, rf24_mac_ack_timeout);
			break;
		default:
			break;
	}
}

void rf24_mac_transfer_frame(rf24_mac_communication_type communication_type, rf24_mac_frame *mac_frame)
{
	// In case transmitter is not already set
	mac_frame->transmitter = mac_addr;
	mac_frame->id = 1;

	// 1)_________________________________________________________
	// Setup a MAC transmission

	rf24_mac_setup_transmission(
		communication_type,					/* UNICAST/BROADCAST*/
		mac_frame->frame_control.type,		/* TOPOLOGY/DATA/..*/
		mac_frame->frame_control.subtype,	/* DATA/DATA_TOPOLOGY/..*/
		&mac_frame->receiver,				/* RECEIVER MAC ADDRESS*/
		NULL, 								/* PAYLOAD*/
		0 									/* PAYLOAD_LENGTH */);

	// 2)_________________________________________________________
	// Copy MAC frame
	mac_transmission.mac_frame = *mac_frame;

	// 3)_________________________________________________________
	// Get access to medium (CSMA/CA)
	rf24_csma_ca_access_medium(&mac_transmission, 0, rf24_mac_start_transmission);
}

void rf24_mac_transfer_data(
	rf24_mac_communication_type communication_type,
	rf24_mac_addr *receiver,
	rf24_mac_frame_subtype mac_frame_subtype,
	uint8_t *payload,
	uint8_t length){

	// 1)_________________________________________________________
	// Setup a new MAC transmission

	rf24_mac_setup_transmission(
			communication_type,
			DATA,
			mac_frame_subtype,
			receiver,
			payload,
			length);

	// 2)_________________________________________________________
	// Get access to medium (CSMA/CA)

	rf24_csma_ca_access_medium(&mac_transmission, 0, rf24_mac_start_transmission);
}

void rf24_mac_print_payload(rf24_mac_transmission *transmission)
{
	uint8_t i;

	for (i = 0; i < transmission->payload_length; i++)
	{
		rf24_printf("%d ",transmission->payload[i]);
	}

	rf24_printf("\n");

}

void rf24_mac_print_frame(rf24_mac_frame *mac_frame)
{
	rf24_printf("%d:%d:%d:%d:%d:%d",
		mac_frame->transmitter.bytes[0],
		mac_frame->transmitter.bytes[1],
		mac_frame->transmitter.bytes[2],
		mac_frame->transmitter.bytes[3],
		mac_frame->transmitter.bytes[4],
		mac_frame->transmitter.bytes[5]);

	rf24_printf(" %d:%d:%d:%d:%d:%d",
		mac_frame->receiver.bytes[0],
		mac_frame->receiver.bytes[1],
		mac_frame->receiver.bytes[2],
		mac_frame->receiver.bytes[3],
		mac_frame->receiver.bytes[4],
		mac_frame->receiver.bytes[5]);

	rf24_printf(" %-14s %-14s",
				rf24_mac_frame_type_string[mac_frame->frame_control.type],
				rf24_mac_frame_subtype_string_short[mac_frame->frame_control.subtype]);

	switch(mac_frame->frame_control.type)
	{
		case DATA:
		{
			for(int i=0; i<mac_frame->data.header.length; i++) rf24_printf("%d ", mac_frame->data.payload[i]);
			break;
		}
		default: break;
	}

	rf24_printf("\n");
}

void rf24_mac_print_timings()
{
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_PROCESSING_US", T_PROCESSING_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dmsm %dus\n", "T_SLOT_US", T_SLOT_MS, T_SLOT_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dms, %dus\n", "T_SIFS_US", T_SIFS_MS, T_SIFS_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dms, %dus\n", "T_DIFS_US", T_DIFS_MS, T_DIFS_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dms, %dus\n", "T_NAV_RTS_US", T_NAV_RTS_MS, T_NAV_RTS_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dms, %dus\n", "T_NAV_CTS_US", T_NAV_CTS_MS, T_NAV_CTS_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dms, %dus\n", "T_NAV_FRAG_US", T_NAV_FRAG_MS, T_NAV_FRAG_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dms, %dus\n", "T_NAV_ACK_US", T_NAV_ACK_MS, T_NAV_ACK_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dms, %dus\n", "T_CTS_TIMEOUT_US", T_CTS_TIMEOUT_MS, T_CTS_TIMEOUT_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dms, %dus\n", "T_ACK_TIMEOUT_US", T_ACK_TIMEOUT_MS, T_ACK_TIMEOUT_US);
}

void rf24_mac_print_transmission()
{
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %s\n", "state", rf24_mac_transmission_states_string[mac_transmission.state]);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %s\n", "transmission_type", rf24_mac_transmission_type_string[mac_transmission.transmission_type]);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %s\n", "communication_type", rf24_mac_communication_type_string[mac_transmission.communication_type]);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %s\n", "frame_type", rf24_mac_frame_type_string[mac_transmission.frame_type]);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %s\n", "frame_subtype", rf24_mac_frame_subtype_string[mac_transmission.frame_subtype]);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %s\n", "transmitter", decimal_to_string(mac_transmission.transmitter.bytes, 6, ':'));
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %s\n", "receiver", decimal_to_string(mac_transmission.receiver.bytes, 6, ':'));
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %d\n", "length", mac_transmission.payload_length);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %d\n", "index", mac_transmission.payload_index);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %d\n", "frame_count", mac_transmission.frame_count);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %d\n", "attempts", mac_transmission.attempts);

	rf24_mac_print_frame(&mac_transmission.mac_frame);
	rf24_mac_print_payload(&mac_transmission);
}


void rf24_mac_ping_timeout_handler()
{
	ping.requests->t_response_us = 0;
	ping.requests->state = PACKAGE_LOST;

	rf24_printf("%-10s %s", "ping", "ping %d timed out or got lost\n", ping.requests->id);

	rf24_mac_process_ping();
}

void rf24_mac_send_ping()
{
	// Create a task to send ping request
	struct rf24_task *task = rf24_worker_build_task(send_ping, 1, 0, true);
	task->data.mac.mac_frame.id = ping.index;
	task->data.mac.mac_frame.frame_control.type = PING;
	task->data.mac.mac_frame.frame_control.subtype = PING_REQUEST;
	task->data.mac.mac_frame.transmitter = *rf24_mac_get_address();
	task->data.mac.mac_frame.receiver = ping.receiver;
	task->data.mac.mac_frame.duration = 0;

	// Start stop watch
	rf24_worker_start_timer(stopwatch, ms, 100, rf24_mac_ping_timeout_handler);

	// Bind send MAC frame function to task (sends ping immediately)
	rf24_worker_push(task, rf24_mac_send_mac_frame);

	ping.requests->state = REQUEST_TRANSMITTED;
}

void rf24_mac_start_ping(rf24_mac_addr *receiver, uint8_t pings)
{
	ping.receiver = *receiver;
	ping.count = pings;
	ping.index = 1;

	ping.requests = (struct rf24_mac_ping_request*) malloc(sizeof(struct rf24_mac_ping_request));
	ping.requests->id = ping.index;
	ping.requests->next = NULL;

	rf24_printf("%-10s send %d pings to %s\n", "ping", ping.count, decimal_to_string(receiver->bytes, 6, ':'));

	rf24_mac_send_ping();
}

void rf24_mac_process_ping()
{
	if(ping.index < ping.count)
	{
		ping.index++;

		struct rf24_mac_ping_request* ping_request = ping.requests;
		ping.requests = (struct rf24_mac_ping_request*) malloc(sizeof(struct rf24_mac_ping_request));
		ping.requests->next = ping_request;
		ping.requests->id = ping.index;

		rf24_mac_send_ping();
	}
	else
	{
		struct rf24_mac_ping_request *request = ping.requests;
		uint8_t n_successfull_replies = 0;
		uint32_t sum_t_response_us = 0;

		// Iterate over list
		while(request != NULL)
		{
			if(request->state == REPLY_RECEIVED)
			{
				n_successfull_replies++;
				sum_t_response_us = request->t_response_us;
			}

			request = request->next;
		}

		rf24_timespan timespan = rf24_worker_us_to_timespan(sum_t_response_us);

		rf24_printf("%-10s %d/%d packages received, average response time %ds %dms %dus \n", "ping", n_successfull_replies, ping.count, timespan.s, timespan.ms, timespan.us);
	}
}
