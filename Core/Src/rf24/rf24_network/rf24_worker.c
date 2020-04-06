#include "rf24_worker.h"
#include "rf24_network.h"
#include "rf24_csma_ca.h"

struct rf24_timer		*timer1 = NULL;
struct rf24_task 		*rf24_tasks = NULL;
struct rf24_cyclic_task *rf24_cyclic_tasks = NULL;

void rf24_worker_init()
{
	// attach callback function to rf24 module to notify tx transmitted event

	rf24_module_attach_notify_data_transmitted(rf24_worker_data_transmitted_handler);

	// attach callback function to rf24 module to notify rx received event

	rf24_module_attach_notify_data_received(rf24_worker_data_received_handler);

	rf24_stm32f1xx_set_timer_interrupt_us(2, TASK_CYCLE_US, rf24_worker_process_tasks);

	rf24_stm32f1xx_set_timer_interrupt_us(3, TIMER_CYCLE_US, rf24_worker_process_timers);

	//rf24_stm32f1xx_set_timer_interrupt_ms(3, 20*CYCLIC_TASK_CYCLE_MS, rf24_worker_process_cyclic_tasks);
}

void rf24_worker_data_transmitted_handler()
{

}

void rf24_worker_data_received_handler(rf24_module_rx_data *rx_data)
{
	// cast rx_data (byte array) to mac frame (struct)
	rf24_mac_frame *mac_frame = (rf24_mac_frame*) rx_data->payload;

	//rf24_mac_print_frame(mac_frame);
	//return;

	// if frame is csma ca control message call csma ca frame handler
	switch(mac_frame->frame_control.type)
	{
		case TOPOLOGY:
			rf24_network_frame_received_handler(mac_frame);
			break;
	}

	// Call CSMA/CA frame received handler
	rf24_csma_ca_frame_received_handler(mac_frame);

	// Call MAC frame received handler
	rf24_mac_frame_received_handler(mac_frame);
}

void rf24_worker_process_cyclic_tasks()
{
	if(rf24_tasks == NULL){};

	// start from beginning
	struct rf24_cyclic_task *current_task = rf24_cyclic_tasks;

	// iterate over linked list (cyclic tasks)
	while(current_task != NULL)
	{
		// update wait state
		current_task->t_cycle_count_ms += CYCLIC_TASK_CYCLE_MS;

		// one cycle past?
		if(current_task->t_cycle_count_ms >= current_task->t_cycle_ms)
		{
			// reset t_cycle_count_us
			current_task->t_cycle_count_ms = 0;

			// execute tasks function
			if(current_task->fct_ptr_execution) current_task->fct_ptr_execution();
		}

		current_task = current_task->next_task;
	}
}

void rf24_worker_process_timers()
{
	if(timer1 == NULL) return;

	if(timer1->t_count_us < timer1->t_us)
	{
		timer1->t_count_us += TIMER_CYCLE_US;

		if(timer1->t_count_us >= timer1->t_us)
		{
			if(timer1->fct_ptr_timeout)
			{
				timer1->fct_ptr_timeout();
			}
		}
	}
	else
	{
		free(timer1);
		timer1 = NULL;
	}
}

void rf24_worker_process_tasks()
{
	// tasks available?
	if(rf24_tasks == NULL) return;

	// wait for precondition
	/*if(!rf24_tasks->precondition)
	{
		if(rf24_tasks->fct_ptr_precondition())
		{
			rf24_tasks->precondition = true;
		}
		else return;
	}*/

	// as long as counter is less than repeat - process current task
	if(rf24_tasks->cycle_count < rf24_tasks->cycles)
	{
		// add us
		rf24_tasks->t_cycle_count_us += TASK_CYCLE_US;

		// one cycle past?
		if(rf24_tasks->t_cycle_count_us >= rf24_tasks->t_cycle_us)
		{
			// one cycle past - reset t_wait_count
			rf24_tasks->t_cycle_count_us = 0;

			// increment counter
			rf24_tasks->cycle_count++;

			//call task function
			if(rf24_tasks->fct_ptr_execution) rf24_tasks->fct_ptr_execution();
		}
	}
	// task has been terminated, pop it
	else
	{
		/*rf24_debug(
				"pop id: %d | task:%s | cycle_count_ms: %lu | cycle_ms: %lu",
				rf24_tasks->task,
				rf24_task_names[rf24_tasks->task],
				rf24_tasks->t_cycle_count_us/1000,
				rf24_tasks->t_cycle_us/1000);*/

		//rf24_worker_print_tasks();
		//rf24_debug("---------------");

		rf24_worker_pop_task();
	}
}

//_________________________________________________________________________________________________________________________________________________________________________
// User Functions

void rf24_worker_add_precondition(struct rf24_task* task, rf24_worker_fct_ptr_ret_bool fct_ptr_precondition)
{
	task->precondition = false;
	task->fct_ptr_precondition = fct_ptr_precondition;
}

void rf24_worker_attach_wait(rf24_task_names task, uint32_t t_cycle_us)
{
	rf24_worker_attach_task(rf24_worker_build_task(task, 1, t_cycle_us, false));
}

void rf24_worker_push_wait(rf24_task_names task, uint32_t t_cycle_us)
{
	rf24_worker_push_task(rf24_worker_build_task(task, 1, t_cycle_us, false));
}

void rf24_worker_attach(struct rf24_task* task, rf24_worker_fct_ptr fct_ptr_execution)
{
	task->fct_ptr_execution = fct_ptr_execution;
	rf24_worker_attach_task(task);
}

void rf24_worker_push(struct rf24_task* task, rf24_worker_fct_ptr fct_ptr_execution)
{
	task->fct_ptr_execution = fct_ptr_execution;
	rf24_worker_push_task(task);
}

void rf24_worker_start(){};
void rf24_worker_stop(){};

//_________________________________________________________________________________________________________________________________________________________________________
// Internal Functions

void rf24_worker_add_cyclic_task(rf24_cyclic_tasks_t cyclic_task, uint32_t t_cycle_ms, rf24_worker_fct_ptr_ret_bool fct_ptr_precondition)
{
	// build new cyclic task
	struct rf24_cyclic_task *new_cyclic_task = (struct rf24_cyclic_task*) malloc(sizeof(struct rf24_cyclic_task));

	new_cyclic_task->task = cyclic_task;
	new_cyclic_task->t_cycle_count_ms = 0;
	new_cyclic_task->t_cycle_ms = t_cycle_ms;
	new_cyclic_task->fct_ptr_execution = fct_ptr_precondition;
	new_cyclic_task->next_task = NULL;

	// append cyclic task to linked list

	// if list is empty, new task becomes head node
	if(rf24_cyclic_tasks == NULL)
	{
		rf24_cyclic_tasks = new_cyclic_task;
		return;
	}

	// start from beginning
	struct rf24_cyclic_task *current_task = rf24_cyclic_tasks;

	// iterate to end of list
	while(current_task->next_task != NULL) current_task = current_task->next_task;

	// insert new node at the end
	current_task->next_task = new_cyclic_task;
}

struct rf24_task* rf24_worker_build_task(rf24_task_names task, uint8_t cycles, uint32_t t_cycle_us, bool immediate)
{
	struct rf24_task *new_task = (struct rf24_task*) malloc(sizeof(struct rf24_task));

	if(new_task == NULL)
	{
		return NULL;
	}

	new_task->task = task;
	new_task->cycle_count = 0;
	new_task->cycles = cycles;
	new_task->t_cycle_us = t_cycle_us;
	new_task->precondition = true;
	new_task->fct_ptr_precondition = NULL;
	new_task->fct_ptr_execution = NULL;

	new_task->next_task = NULL;

	// if task has to be executed immediate, set cycle counter to cycle time
	if(immediate)
		new_task->t_cycle_count_us = new_task->t_cycle_us;
	else
		new_task->t_cycle_count_us = 0;

	return new_task;
}

void rf24_worker_attach_task(struct rf24_task *new_task)
{
	// if list is empty, new task becomes head node
	if(rf24_tasks == NULL)
	{
		rf24_tasks = new_task;
		return;
	}

	// start from beginning
	struct rf24_task *current_task = rf24_tasks;

	// iterate to end of list
	while(current_task->next_task != NULL) current_task = current_task->next_task;

	// insert new node at the end
	current_task->next_task = new_task;
}

void rf24_worker_push_task(struct rf24_task *new_task)
{
	// If list is empty, new task becomes head node
	if(rf24_tasks == NULL)
	{
		rf24_tasks = new_task;
		return;
	}

	// Otherwise insert new task at beginning of list

	// (1) Former head node becomes next node of new task
	new_task->next_task = rf24_tasks;

	// (2) New task becomes new head node
	rf24_tasks = new_task;

}

struct rf24_task* rf24_worker_current_task()
{
	return rf24_tasks;
}

bool rf24_worker_tasks_available()
{
	return rf24_tasks == NULL;
}

void rf24_worker_pop_task()
{
	// save a reference to current task
	struct rf24_task *current_task = rf24_tasks;

	// remove first (current) task -> 2nd task becomes new head node
	rf24_tasks = rf24_tasks->next_task;

	//free current task
	free(current_task);
}

void rf24_worker_reset_tasks()
{
	free(rf24_tasks);
	rf24_tasks = NULL;
}

void rf24_worker_print_tasks()
{
	// start from the first node
	struct rf24_task *current_task = rf24_tasks;

	// iterate over list
	while(current_task != NULL)
	{
		rf24_printf("%s", rf24_task_names_string[current_task->task]);
		current_task = current_task->next_task;
	}
}

void rf24_worker_start_timer(rf24_timer_names name, rf24_timer_units unit, uint32_t duration, rf24_worker_fct_ptr fct_ptr_timeout)
{
	struct rf24_timer *timer = (struct rf24_timer*) malloc(sizeof(struct rf24_timer));

	timer->name = name;
	timer->t_count_us = 0;
	timer->fct_ptr_timeout = fct_ptr_timeout;

	switch(unit)
	{
		case s: timer->t_us = duration * 1000000; break;
		case ms: timer->t_us = duration * 1000; break;
		case us: timer->t_us = duration; break;
	}

	timer1 = timer;
}

struct rf24_timespan rf24_worker_stop_timer()
{
	struct rf24_timespan timespan = rf24_worker_us_to_timespan(timer1->t_count_us);

	free(timer1);
	timer1 = NULL;

	return timespan;
}

struct rf24_timespan rf24_worker_us_to_timespan(uint32_t us)
{
	struct rf24_timespan timespan;

	uint32_t remaining_us = us;

	timespan.s = remaining_us / 1000000;
	remaining_us = remaining_us % 1000000;
	timespan.ms = remaining_us / 1000;
	remaining_us = remaining_us % 1000;
	timespan.us = remaining_us;

	return timespan;
}
