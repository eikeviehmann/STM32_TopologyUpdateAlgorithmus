#include "../rf24_network/rf24_mac.h"

#include "rf24_csma_ca.h"
#include "rf24_worker.h"
#include "rf24_network.h"
#include "../rf24_debug.h"

//_________________________________________________________________________________________________________________________________________________________________________
// Global variables & Constants

//volatile rf24_mac_flags				mac_flags;

rf24_mac_addr 							mac_addr;
rf24_mac_addr 							mac_addr_broadcast = { .bytes = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
struct rf24_mac_neighbour* volatile 	mac_neighbours = NULL;
rf24_mac_transmission 					mac_transmission;
const char								*rf24_mac_neigbour_status_string[] = { "AVAILABLE", "CONNECTED", "TIMED_OUT"};
uint8_t									test_counter = 0;

//_________________________________________________________________________________________________________________________________________________________________________
// Function declarations
// Interrupt service routines / callback functions

void rf24_mac_frame_received_handler(rf24_mac_frame *mac_frame)
{
	bool addressed_to_me = rf24_mac_addr_equal(&mac_frame->receiver, &mac_addr);

	switch(mac_frame->frame_control.type)
	{
		case DATA:
		{
			if(addressed_to_me)
			{
				_data_frame_to_payload(mac_frame, &mac_transmission);

				rf24_mac_send_ack(mac_frame);
			}
			break;
		}

		case CONTROL:
		{
			switch(mac_frame->frame_control.subtype)
			{
				case CONTROL_ACK:
				{
					if(addressed_to_me)
					{
						// Pop "wait_for_ack" task from task stack
						if(rf24_worker_current_task()->task == wait_for_ack) rf24_worker_pop_task();

						rf24_debug(
							MAC, RECEIVE, CONTROL_ACK, mac_frame->ack.subtype, &mac_frame->transmitter,
							"frame %d\n", mac_transmission.frame_count);

						if(mac_transmission.transmission_type == transmission) _process_transmission();
					}
				}
			}
		}
	}
}

void rf24_mac_reception_successfull()
{
	// Slow down LED - show that I am back in IDLE
	rf24_stm32f1xx_set_led_cycle(T_LED_CYCLE_IDLE_MS);

	switch(mac_transmission.frame_subtype)
	{
		case DATA_TOPOLOGY:
		{
			// Convert byte stream into topology and attach to local topology
			rf24_network_rx_data_to_topology(mac_transmission.payload, mac_transmission.length);

			// Call topology received handler
			rf24_network_topology_received();

			break;
		}
		case DATA_DATA:
		{
			rf24_mac_print_payload(&mac_transmission);
			break;
		}

		default: break;
	}
}

void rf24_mac_transmission_successfull()
{
	// Slow down LED - show that I am back in IDLE
	rf24_stm32f1xx_set_led_cycle(T_LED_CYCLE_IDLE_MS);

	switch(mac_transmission.frame_type)
	{
		case TOPOLOGY:
			rf24_network_transmission_successfull();
			break;
		default:
			break;
	}
}

void rf24_mac_ack_timeout()
{
	// Only retransmit frame again if attempts is lower than maximum retransmits
	if(mac_transmission.attemps < N_MAX_RETRANSMITS)
	{
		// Build a new "send_mac_frame" task
		struct rf24_task *task = rf24_worker_build_task(send_mac_frame, 1, T_SIFS_US, false);

		// Attach mac_frame to task
		task->data.mac.mac_frame = mac_transmission.mac_frame;

		// Attach function rf24_mac_send_frame to task
		rf24_worker_attach(task, _send_mac_frame);

		// Create a "wait_for_ack" task
		task = rf24_worker_build_task(wait_for_ack, 1, T_ACK_TIMEOUT_US, false);
		rf24_worker_attach(task, rf24_mac_ack_timeout);

		// Notice one more attempt
		mac_transmission.attemps++;

		rf24_debug(
			MAC, TIMEOUT, VOID, VOID, NULL,
			"[ACK-Timeout] retransmit frame %d (%d/%d attempt/s)\n",
			mac_transmission.frame_count,
			mac_transmission.attemps,
			N_MAX_RETRANSMITS);
	}
	else
	{
		rf24_debug(MAC, INFO, VOID, VOID, NULL, "[ACK-Timeout] max retries reached -> transmission cancelled\n");

		switch(mac_transmission.frame_type)
		{
			case TOPOLOGY:
				rf24_network_transmission_failed();
				return;
			default:
				break;
		}
	}
}


// FUNCTIONS

void rf24_mac_test_timer_timeout()
{
	rf24_printf("TIMEOUT\n");
}

void rf24_mac_init()
{
	// csma ca init
	rf24_cmsa_ca_init(rf24_mac_setup_reception);

	// build mac adress from stm32 uuid
	rf24_mac_build_address();

	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %s\n", "MAC_ADDRESS", decimal_to_string(mac_addr.bytes, 6, ':'));

	rf24_mac_print_timings();

	/*rf24_worker_start_timer(trm_timeout, ms, 5000, rf24_mac_test_timer_timeout);
	rf24_worker_start_timer(num_timeout, ms, 1000, rf24_mac_test_timer_timeout);
	rf24_worker_start_timer(trm_timeout, ms, 3000, rf24_mac_test_timer_timeout);

	rf24_worker_stop_timer(num_timeout);

	rf24_worker_print_timers();*/
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

void _process_transmission()
{
	// Transmission has not transferred bytes left?
	if(mac_transmission.index < mac_transmission.length)
	{
		// Reset attempts (each frame can be retransmitted 15 times)
		mac_transmission.attemps = 0;

		// Create a "send mac frame" task, wait a SIFS before transmission
		struct rf24_task *task = rf24_worker_build_task(send_mac_frame, 1, T_SIFS_US, false);

		// Assemble next data frame
		_payload_to_data_frame(&mac_transmission, &task->data.mac.mac_frame);

		// Bind send mac frame function to task
		rf24_worker_attach(task, _send_mac_frame);

		// Create a "wait for ack" task
		task = rf24_worker_build_task(wait_for_ack, 1, T_ACK_TIMEOUT_US, false);
		rf24_worker_attach(task, rf24_mac_ack_timeout);
	}
	else
	{
		mac_transmission.state = COMPLETED;

		rf24_debug(	MAC, TRANSMISSION, mac_transmission.frame_subtype, VOID, &mac_transmission.receiver,
					"Transmission completed (%d frames)\n\n", mac_transmission.frame_count);

		if(mac_transmission.state != FINALIZED)
		{
			mac_transmission.state = FINALIZED;
			rf24_mac_transmission_successfull();
		}
	}
}

void _process_reception()
{
	if(mac_transmission.index >= mac_transmission.length)
	{
		mac_transmission.state = COMPLETED;

		rf24_debug(	MAC, RECEPTION, mac_transmission.frame_subtype, VOID, &mac_transmission.transmitter,
					"Reception completed (%d frames)\n\n", mac_transmission.frame_count);

		if(mac_transmission.state != FINALIZED)
		{
			mac_transmission.state = FINALIZED;
			rf24_mac_reception_successfull();
		}
	}
}

void _send_mac_frame()
{
	// Create TX data container
	rf24_module_tx_data tx_data;
	rf24_mac_frame mac_frame = rf24_worker_current_task()->data.mac.mac_frame;

	// Covert MAC frame into TX data
	rf24_mac_frame_to_tx_data(&mac_frame, &tx_data);

	// Transmit MAC frame (TX data)
	rf24_module_transmit(&tx_data);


	// Stop stop-watch in case it has been started
	//rf24_stm32f1xx_start_stopwatch();
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
	rf24_worker_push(task, _send_mac_frame);

	rf24_debug(	MAC, TRANSMIT, CONTROL_ACK, mac_frame->frame_control.subtype, &mac_frame->transmitter,
				"frame %d\n", mac_frame->id);

	if(mac_transmission.transmission_type == reception) _process_reception();
}


//uint16_t rf24_mac_calculate_nav(rf24_mac_transmission* transmission)
//{
	// calculate transmission time ms

//	transmission->nav_data.ms =
//			(	T_SIFS_US + 			/* SIFS (before DATA) */
//				T_PROCESSING_US + 		/* DATA */
//				T_SIFS_US + 			/* SIFS (before ACK) */
//				T_PROCESSING_US			/* ACK */
//			) 	* transmission->length	/* multiplied with number of data packages*/
//				/ 1000;					/* us -> ms */
//
	// calculate nav rts ms
//
//	uint16_t nav_rts_ms =
//			(	T_SIFS_US + 			/* SIFS (before CTS) */
//				T_PROCESSING_US) 		/* CTS  */
//			/ 1000 +					/* us -> ms*/
//			transmission->nav_data.ms;
//
//	//
//	//
	//	return nav_rts_ms;
//}
//

void _payload_to_data_frame(rf24_mac_transmission* transmission, rf24_mac_frame* mac_frame)
{
	// Calculate remaining bytes to transfer
	uint8_t bytes_to_transfer = transmission->length - transmission->index;

	if( !(bytes_to_transfer > 0) ) return;

	mac_frame->frame_control.type = transmission->frame_type;
	mac_frame->frame_control.subtype = transmission->frame_subtype;
	mac_frame->transmitter = mac_addr;
	mac_frame->receiver = transmission->receiver;
	mac_frame->duration = T_NAV_FRAG_MS;

	// increment frame counter
	transmission->frame_count++;
	mac_frame->id = transmission->frame_count;

	// More than 15 bytes to transfer?
	if(bytes_to_transfer >= MAC_FRAME_PAYLOAD_LENGTH)
	{
		// Inform receiver more frames to come
		mac_frame->frame_control.more_data = true;
		// Add PAYLOAD length to data header
		mac_frame->data.header.length = MAC_FRAME_PAYLOAD_LENGTH;

		// Copy PAYLOAD data into MAC frame
		memcpy(mac_frame->data.payload, &transmission->payload[transmission->index], MAC_FRAME_PAYLOAD_LENGTH);

		transmission->index += MAC_FRAME_PAYLOAD_LENGTH;
	}
	else
	{
		// Inform receiver last MAC frame
		mac_frame->frame_control.more_data = false;
		// Add remaining PAYLOAD length to data header
		mac_frame->data.header.length = bytes_to_transfer;

		// Copy remaining bytes of PAYLOAD into MAC frame
		memcpy(mac_frame->data.payload, &transmission->payload[transmission->index], bytes_to_transfer);

		// -> index = length -> transmission completed
		transmission->index += bytes_to_transfer;
	}

	// Store a local copy for retransmission (if not ACK received)
	transmission->mac_frame = *mac_frame;
}

void _data_frame_to_payload(rf24_mac_frame* mac_frame, rf24_mac_transmission* transmission)
{
	// Packet is a new frame?
	if(mac_frame->id > transmission->frame_count)
	{
		switch(mac_frame->frame_control.subtype)
		{
			case DATA_DATA: case DATA_TOPOLOGY:
			{
				memcpy(	&transmission->payload[transmission->index],
						mac_frame->data.payload,
						mac_frame->data.header.length);

				transmission->index += mac_frame->data.header.length;

				break;
			}
			default: break;
		}

		transmission->frame_count = mac_frame->id;
	}
}

void rf24_mac_setup_reception(rf24_mac_frame *mac_frame)
{
	// Speed up LED - show that I am receiving
	rf24_stm32f1xx_set_led_cycle(T_LED_CYCLE_TRANSMITTING_MS);

	// Check, if RTS frame contains any information of following data
	switch(mac_frame->rts.subtype)
	{
		case DATA_DATA:
		case DATA_TOPOLOGY:
		{
			// Create an PAYLOAD array based on the given RTS frame meta information
			mac_transmission.payload = (uint8_t*) malloc(mac_frame->rts.length * sizeof(uint8_t));
			break;
		}
		default: break;
	}

	// Initialize mac_transmission for reception based on the given RTS meta information
	mac_transmission.transmission_type = reception;
	mac_transmission.state = ACTIVE;
	mac_transmission.frame_type = mac_frame->rts.type;
	mac_transmission.frame_subtype = mac_frame->rts.subtype;
	mac_transmission.transmitter = mac_frame->transmitter;
	mac_transmission.length = mac_frame->rts.length;
	mac_transmission.index = 0;
	mac_transmission.frame_count = 0;
}

rf24_mac_transmission* rf24_mac_setup_transmission(
	rf24_mac_communication_type communication_type,
	rf24_mac_frame_type frame_type,
	rf24_mac_frame_subtype frame_subtype,
	rf24_mac_addr *receiver,
	uint8_t *payload,
	uint8_t length){

	// Setup MAC transmission
	mac_transmission.state = ACTIVE;
	mac_transmission.transmission_type = transmission;
	mac_transmission.communication_type = communication_type;
	mac_transmission.frame_type = frame_type;
	mac_transmission.frame_subtype = frame_subtype;
	mac_transmission.receiver = *receiver;
	mac_transmission.length = length;
	mac_transmission.index = 0;
	mac_transmission.frame_count = 0;
	mac_transmission.attemps = 0;

	if(length > 0)
	{
		// Free PAYLOAD (of former transmission)
		free(mac_transmission.payload);

		// Allocate memory for PAYLOAD
		mac_transmission.payload = (uint8_t*) malloc(length * sizeof(uint8_t));

		// Copy PAYLOAD to MAC transmission
		memcpy(mac_transmission.payload, payload, length);
	}

	return &mac_transmission;
}

void _start_transmission()
{
	// Speed up LED - show that I am transmitting
	rf24_stm32f1xx_set_led_cycle(T_LED_CYCLE_TRANSMITTING_MS);

	//1)_________________________________________________________
	// (Build &) Send MAC frame

	struct rf24_task *task = rf24_worker_build_task(send_mac_frame, 1, T_SIFS_US, false);

	switch(mac_transmission.frame_type)
	{
		//If transmission is of type DATA, convert PAYLOAD into DATA_DATA frames
		case DATA:
			_payload_to_data_frame(&mac_transmission, &task->data.mac.mac_frame);
			break;

		// Otherwise take given MAC frame and proceed
		default:
			task->data.mac.mac_frame = mac_transmission.mac_frame;
			break;
	}

	rf24_worker_attach(task, _send_mac_frame);

	// Increase attempts to notice retries
	mac_transmission.attemps++;

	// 2)_________________________________________________________
	// Wait for ACK(s)

	task = rf24_worker_build_task(wait_for_ack, 1, T_ACK_TIMEOUT_US, false);
	rf24_worker_attach(task, rf24_mac_ack_timeout);
}

void rf24_mac_transfer_frame(rf24_mac_communication_type communication_type, rf24_mac_frame *mac_frame)
{
	// In case transmitter is not already set
	mac_frame->transmitter = mac_addr;
	mac_frame->id = 1;

	// 1)_________________________________________________________
	// Setup a MAC transmission

	rf24_mac_setup_transmission(
			communication_type,					/* uni-/multi-/broadcast*/
			mac_frame->frame_control.type,		/* NUM/NAM/DATA/..*/
			mac_frame->frame_control.subtype,	/* DATA/DATA_TOPOLOGY/..*/
			&mac_frame->receiver,				/* receiver mac address*/
			NULL, 								/* no payload*/
			0 									/* no length*/);

	// 2)_________________________________________________________
	// Copy MAC frame
	mac_transmission.mac_frame = *mac_frame;

	// 3)_________________________________________________________
	// Get access to medium (CSMA/CA)

	// Call CSMA/CA access medium function & attach start transmission function (callback function)
	rf24_csma_ca_access_medium(&mac_transmission, 0,_start_transmission);
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

	// Call CSMA/CA routines & start transmission after obtain access to medium (callback)
	rf24_csma_ca_access_medium(&mac_transmission, 0, _start_transmission);
}

void rf24_mac_print_payload(rf24_mac_transmission *transmission)
{
	char str[256];
	uint8_t index_string = 0;
	uint8_t index_payload = 0;

	for (int i=0; i < transmission->length; i++)
	{
		index_string += sprintf(&str[index_string], "%d ", transmission->payload[i]);
		index_payload++;

		if(index_payload >= 15)
		{
			index_string = 0;
			index_payload = 0;
			rf24_stm32f1xx_usart_write_line(str);
		}
	}
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
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_SLOT_US", T_SLOT_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_SIFS_US", T_SIFS_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_PIFS_US", T_PIFS_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_DIFS_US", T_DIFS_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_NAV_RTS_US", T_NAV_RTS_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_NAV_CTS_US", T_NAV_CTS_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_NAV_FRAG_US", T_NAV_FRAG_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_NAV_ACK_US", T_NAV_ACK_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_CTS_TIMEOUT_US", T_CTS_TIMEOUT_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_ACK_TIMEOUT_US", T_ACK_TIMEOUT_US);
	rf24_debug(MAC, INFO, VOID, VOID, NULL, "%-25s %dus\n", "T_NUM_NAM_HANDSHAKE_US", T_NUM_NAM_HANDSHAKE_US);
}
