#include "rf24_csma_ca.h"
#include "../rf24_debug.h"

rf24_csma_ca_flags					csma_ca_flags = { false, false, false, false, false, false, false, false };
rf24_csma_ca_order					csma_ca_order;
rf24_csma_ca_fct_ptr_rts_received	csma_ca_fct_ptr_rts_received;

// INTERRUPTS

void rf24_csma_ca_frame_received_handler(rf24_mac_frame *mac_frame)
{
	bool addressed_to_me = memcmp(mac_frame->receiver.bytes, rf24_mac_get_address()->bytes, 6) == 0;

	switch(mac_frame->frame_control.subtype)
	{
		case CONTROL_RTS:
		{
			if(addressed_to_me)
			{
				// Attach send CTS task, wait a SIFS before transmitting CTS
				struct rf24_task *task = rf24_worker_build_task(send_cts, 1, T_SIFS_US, true);

				// Set destination & slot time (duration) for CTS reply
				task->data.csma.receiver = mac_frame->transmitter;

				// Bind send CTS function to task
				rf24_worker_attach(task, rf24_csma_ca_send_cts);

				// Set flag RTS received
				csma_ca_flags.rts_received = true;

				// Call RTS received fct ptr (callback to receiving thread)
				if(csma_ca_fct_ptr_rts_received) csma_ca_fct_ptr_rts_received(mac_frame);

				rf24_debug(CSMA_CA, RECEIVE, CONTROL_RTS, mac_frame->rts.subtype, &mac_frame->transmitter, "addressed to me\n", "");
			}
			else
			{
				// Wait NAV
				rf24_csma_ca_wait_nav(mac_frame);

				rf24_debug(CSMA_CA, RECEIVE, CONTROL_RTS, mac_frame->rts.subtype, &mac_frame->transmitter, "jammed\n", "");
			}

			break;
		}

		/***************************************************************************************/

		case CONTROL_CTS:
		{
			// Check if CTS frame is addressed to me
			bool addressed_to_me = memcmp(mac_frame->receiver.bytes, rf24_mac_get_address()->bytes, 6) == 0;

			if(addressed_to_me)
			{
				// Notice CTS received
				csma_ca_flags.cts_received = true;

				// Stop stop-watch
				csma_ca_order.t_cts_response_us = rf24_stm32f1xx_stop_stopwatch();

				// Pop CTS timeout from task stack
				if(rf24_worker_current_task()->task == wait_for_cts) rf24_worker_pop_task();

				// Reset CSMA/CA routine
				//rf24_csma_ca_reset();

				// Call CTS received function pointer (callback function to ordering thread)
				if(csma_ca_order.fct_ptr_access_medium) csma_ca_order.fct_ptr_access_medium();

				rf24_debug(CSMA_CA, RECEIVE, CONTROL_CTS, mac_frame->rts.subtype, &mac_frame->transmitter, "access to medium\n", "");
			}
			else
			{
				rf24_debug(CSMA_CA, RECEIVE, CONTROL_CTS, mac_frame->rts.subtype, &mac_frame->transmitter, "jammed\n", "");

				// Wait NAV
				rf24_csma_ca_wait_nav(mac_frame);
			}

			break;
		}

		/***************************************************************************************/

		default:
		{
			// All frames which aren't addressed to me and have a valid NAV attached
			if(!addressed_to_me)
			{
				// Wait NAV
				if(mac_frame->duration > 0) rf24_csma_ca_wait_nav(mac_frame);
			}

			break;
		}
	}
}

void rf24_csma_ca_wait_nav(rf24_mac_frame *mac_frame)
{
	uint32_t t_nav_us = mac_frame->duration * 1000;

	// Pointer to current (working) tasks
	struct rf24_task *task = rf24_worker_current_task();

	// Am i trying to access medium? -> current task == wait random back-off
	if(task->task == wait_random_backoff)
	{
		// Push a new SIFS before remaining back-off timer (task) into task-pipe
		rf24_worker_push_wait(wait_sifs, T_SIFS_US);

		// Set flag back off competition lost
		csma_ca_flags.backoff_competition_lost = true;

		// If debug enabled print message
		#ifdef RF24_CSMA_CA_DEBUG
			// MESSAGE CAUSES DELAY IN TIMER PROCEEDING !!! DONT COMMENT OUT
			//rf24_debug("CSMA/CA: [BACK-OFF] competition lost against %s", decimal_to_string(mac_frame->transmitter.bytes, 6, ':'));
		#endif
	}

	// Am I already waiting?
	if(task->task == wait_nav)
	{
		switch(mac_frame->frame_control.subtype)
		{
			// If RTS or CTS received a new transmission starts, therefore reset wait NAV task
			case CONTROL_RTS: case CONTROL_CTS:
			{
				// Pop current wait NAV task
				rf24_worker_pop_task();

				// Build a new wait NAV task
				task = rf24_worker_build_task(wait_nav, 1, t_nav_us, false);

				// Attach it to NAV expired function
				rf24_worker_push(task, rf24_csma_ca_nav_expired);

				break;
			}
			// Otherwise update wait NAV task (add duration of new MAC frame on top of current task)
			default:
				task->t_cycle_us += t_nav_us;
				break;
		}

		// If debug enabled print message
		#ifdef RF24_CSMA_CA_DEBUG
			// MESSAGE CAUSES DELAY IN TIMER PROCEEDING !!! DONT COMMENT OUT
			/*rf24_debug(
					"CSMA/CA: [NAV] delay updated (%lu ms by %s from %s)",
					task->t_cycle_us / 1000,
					rf24_mac_frame_subtype_str[mac_frame->frame_control.subtype],
					decimal_to_string(mac_frame->transmitter.bytes, 6, ':'));*/
		#endif
	}
	else
	{
		// Build a new wait NAV task
		task = rf24_worker_build_task(wait_nav, 1, t_nav_us /*1s test*/, false);

		// Attach it to NAV expired function
		rf24_worker_push(task, rf24_csma_ca_nav_expired);

		#ifdef RF24_CSMA_CA_DEBUG
			// MESSAGE CAUSES DELAY IN TIMER PROCEEDING !!! DONT COMMENT OUT
			/*rf24_debug(
					"CMSA/CA: [NAV] delay started (%lu ms by %s from %s)",
					t_nav_us / 1000,
					rf24_mac_frame_subtype_str[mac_frame->frame_control.subtype],
					decimal_to_string(mac_frame->transmitter.bytes, 6, ':'));*/
		#endif
	}

}

void rf24_cmsa_ca_init(rf24_csma_ca_fct_ptr_rts_received fct_ptr_rts_received)
{
	// CTS received (start of reception) callback function
	csma_ca_fct_ptr_rts_received = fct_ptr_rts_received;

	// Reset routine
	rf24_csma_ca_reset();
}

void rf24_csma_ca_reset()
{
	// Reset csma_ca_flags
	memset(&csma_ca_flags, 0, sizeof(rf24_csma_ca_flags));

	// Reset variables
	csma_ca_order.rts_transmits = 0;
	csma_ca_order.t_random_backoff_us = 0;

	// Reset task pipeline
	rf24_worker_reset_tasks();
}

void rf24_csma_ca_send_rts()
{
	rf24_mac_frame mac_frame;

	mac_frame.frame_control.type = CONTROL;
	mac_frame.frame_control.subtype = CONTROL_RTS;
	mac_frame.duration = T_NAV_RTS_MS;
	mac_frame.transmitter = *rf24_mac_get_address();
	mac_frame.receiver = csma_ca_order.transmission->receiver;
	mac_frame.rts.type = csma_ca_order.transmission->frame_type;
	mac_frame.rts.subtype = csma_ca_order.transmission->frame_subtype;
	mac_frame.rts.length = csma_ca_order.transmission->length;

	rf24_module_tx_data tx_data;
	rf24_mac_frame_to_tx_data(&mac_frame, &tx_data);
	rf24_module_transmit(&tx_data);

	// notice attempt
	csma_ca_order.rts_transmits++;

	// start stop watch to measure time between RTS and CTS replies
	rf24_stm32f1xx_start_stopwatch();

	rf24_debug(CSMA_CA, TRANSMIT, CONTROL_RTS, rf24_mac_get_transmission()->frame_subtype, &mac_frame.receiver, "\n", "");
}

void rf24_csma_ca_send_cts()
{
	rf24_mac_frame mac_frame;

	mac_frame.frame_control.type = CONTROL;
	mac_frame.frame_control.subtype = CONTROL_CTS;
	mac_frame.duration = T_NAV_CTS_MS;
	mac_frame.transmitter = *rf24_mac_get_address();
	mac_frame.rts.type = csma_ca_order.transmission->frame_type;
	mac_frame.rts.subtype = csma_ca_order.transmission->frame_subtype;
	mac_frame.rts.length = csma_ca_order.transmission->length;
	mac_frame.receiver = rf24_worker_current_task()->data.csma.receiver;

	rf24_module_tx_data tx_data;
	rf24_mac_frame_to_tx_data(&mac_frame, &tx_data);
	rf24_module_transmit(&tx_data);

	rf24_debug(CSMA_CA, TRANSMIT, CONTROL_CTS, rf24_mac_get_transmission()->frame_subtype, &mac_frame.receiver, "\n","");
}

void rf24_csma_ca_nav_expired()
{
	rf24_debug(CSMA_CA, INFO, VOID, VOID, NULL, "NAV expired, wake up & proceed\n","");
}

void rf24_csma_ca_random_backoff_expired()
{
	// If access type is a broadcast, allow access to medium after a random back off
	switch(csma_ca_order.transmission->communication_type)
	{
		case BROADCAST:
		{
			// Random back off expired without disruption, call callback function
			if(csma_ca_order.fct_ptr_access_medium) csma_ca_order.fct_ptr_access_medium();

			break;
		}
		default: break;
	}
}

void rf24_csma_ca_cts_timeout()
{
	// Wait for CTS timeout

	// If CTS timeout reached & CTS not received
	if(!csma_ca_flags.cts_received)
	{
		if(csma_ca_order.rts_transmits >= N_MAX_RTS_RETRANSMITS)
		{
			rf24_debug(CSMA_CA, TIMEOUT, VOID, VOID, NULL,"[CTS-Timeout] max retransmits reached -> transmission cancelled\n", "");

			// Reset routine
			rf24_csma_ca_reset();
		}
		else
		{
			rf24_debug(CSMA_CA, TIMEOUT, VOID, VOID, NULL,
					"[CTS-Timeout] %d/%d attempt(s) -> start again\n", csma_ca_order.rts_transmits, N_MAX_RTS_RETRANSMITS);

			// Start a new CSMA/CA cycle (recursive call)
			rf24_csma_ca_access_medium(
					csma_ca_order.transmission,
					csma_ca_order.rts_transmits++,
					csma_ca_order.fct_ptr_access_medium);
		}
	}
}

uint32_t rf24_csma_ca_compute_random_backoff(uint16_t contention_window)
{
	// Use timer count 3 to seed rand for a pseudo random number
	srand(rf24_stm32f1xx_get_tim3_count());

	// Compute random back off with in given contention window
	return ( rand() % contention_window ) * T_SLOT_US;
}

// User Functions

void rf24_csma_ca_access_medium(
		rf24_mac_transmission*				transmission,
		uint8_t								rts_transmits,
		rf24_csma_ca_fct_ptr_access_medium 	fct_ptr_access_medium)
	{

	// Reset CSMA_CA routine
	rf24_csma_ca_reset();

	csma_ca_order.transmission = transmission;
	csma_ca_order.rts_transmits = rts_transmits;
	csma_ca_order.fct_ptr_access_medium = fct_ptr_access_medium;

	// 1)___________________________________________________________________________________________
	// Wait a DIFS

	rf24_worker_attach_wait(wait_difs, T_DIFS_US);

	// 2)___________________________________________________________________________________________
	// Generate & wait random back-off

	uint32_t t_ramdom_backoff_us = rf24_csma_ca_compute_random_backoff(CONTENTION_WINDOW);
	csma_ca_order.t_random_backoff_us = t_ramdom_backoff_us;

	struct rf24_task *task = rf24_worker_build_task(wait_random_backoff, 1, csma_ca_order.t_random_backoff_us, false);
	rf24_worker_attach(task, rf24_csma_ca_random_backoff_expired);

	// only send RTS in case of a directed (UNICAST or MULTICAST) CMSA/CA access (to a receiver node)
	switch(csma_ca_order.transmission->communication_type)
	{
		case UNICAST: case MULTICAST:
		{
			// 3)___________________________________________________________________________________________
			// Send RTS after a SIFS

			struct rf24_task *task = rf24_worker_build_task(send_rts, 1, T_SIFS_US, false);
			rf24_worker_attach(task, rf24_csma_ca_send_rts);

			// 4)___________________________________________________________________________________________
			// Wait for CTS (timeout)

			task = rf24_worker_build_task(wait_for_cts, 1, T_CTS_TIMEOUT_US, false);
			rf24_worker_attach(task, rf24_csma_ca_cts_timeout);

			break;
		}
		case BROADCAST:
		{
			break;
		};
		default: break;
	}

	rf24_debug(	CSMA_CA, INFO, csma_ca_order.transmission->frame_subtype, VOID, &csma_ca_order.transmission->receiver,
				"Access medium (Random-Backoff: %dms)\n", t_ramdom_backoff_us/1000);
}




