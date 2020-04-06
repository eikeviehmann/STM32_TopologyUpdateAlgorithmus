#ifndef RF24_WORKER_H
#define RF24_WORKER_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include "../rf24_module/rf24_module.h"
#include "rf24_mac.h"

#define TIMER_CYCLE_US 1000
#define TASK_CYCLE_US 100
#define CYCLIC_TASK_CYCLE_MS 100

typedef enum {
	wait_sifs,
	wait_difs,
	wait_pifs,
	wait_nav,
	wait_random_backoff,
	send_rts,
	send_cts,
	send_ack,
	wait_for_cts,
	send_mac_frame,
	wait_for_ack,
	transmission_successfull,
	reception_successfull
} rf24_task_names;

static const char *rf24_task_names_string[] = {
	"wait_sifs",
	"wait_difs",
	"wait_pifs",
	"wait_nav",
	"wait_random_backoff",
	"send_rts",
	"send_cts",
	"send_ack",
	"wait_for_cts",
	"send_mac_frame",
	"wait_for_ack"
};

typedef union {
	//struct rf24_mac_task_ping ping;
	//struct rf24_mac_task_ping_reply ping_reply;
	//struct rf24_mac_task_broacast_topology_reply topology_reply;
	struct rf24_csma_task {
		rf24_mac_addr receiver;
	} csma;
	struct rf24_mac_task {
		rf24_mac_frame mac_frame;
	} mac;
} rf24_task_data;

typedef void (*rf24_worker_fct_ptr) (void);
typedef bool (*rf24_worker_fct_ptr_ret_bool) (void);

struct rf24_task {
	rf24_task_names					task;
	uint8_t 						cycles;
	uint8_t 						cycle_count;
	uint32_t 						t_cycle_us;
	volatile uint32_t 				t_cycle_count_us;
	rf24_task_data 					data;
	bool							precondition;
	rf24_worker_fct_ptr_ret_bool	fct_ptr_precondition;
	rf24_worker_fct_ptr 			fct_ptr_execution;
	struct rf24_task 				*next_task;
};

typedef enum {
	send_broadcast_topology,
	send_topology_reply,
	update_local_neighbours
} rf24_cyclic_tasks_t;

struct rf24_cyclic_task {
	rf24_cyclic_tasks_t				task;
	uint32_t 						t_cycle_ms;
	uint32_t 						t_cycle_count_ms;
	fct_ptr 						fct_ptr_execution;
	struct rf24_cyclic_task 		*next_task;
};

typedef enum {
	num_timeout
} rf24_timer_names;

typedef enum {
	s,
	ms,
	us
} rf24_timer_units;

struct rf24_timer {
	rf24_timer_names				name;
	uint32_t 						t_us;
	volatile uint32_t 				t_count_us;
	rf24_worker_fct_ptr 			fct_ptr_timeout;
};

typedef struct rf24_timespan {
	uint8_t s;
	uint16_t ms;
	uint16_t us;
};

// INTERRUPTS
void 					rf24_worker_data_transmitted_handler();
void 					rf24_worker_data_received_handler(rf24_module_rx_data*);

void 					rf24_worker_cyclic_task_worker();
void 					rf24_worker_process_tasks();
void 					rf24_worker_process_cyclic_tasks();
void					rf24_worker_process_timers();

// USER FUNCTIONS

struct 					rf24_thread_t* rf24_worker_build_thread(rf24_worker_fct_ptr fct_ptr);
void 					rf24_worker_delete_thread();


void 					rf24_worker_init();
void 					rf24_worker_print_tasks();
bool 					rf24_worker_tasks_available();

void 					rf24_worker_print_tasks();

void					rf24_worker_start();
void 					rf24_worker_stop();

void 					rf24_worker_start_timer(rf24_timer_names name, rf24_timer_units unit, uint32_t duration, rf24_worker_fct_ptr fct_ptr_timeout);
struct rf24_timespan 	rf24_worker_stop_timer();
struct rf24_timespan 	rf24_worker_us_to_timespan(uint32_t us);

// INTERNAL FUNCTIONS

void 					rf24_worker_attach_wait(rf24_task_names task, uint32_t t_cycle_us);
void 					rf24_worker_attach(struct rf24_task* task, rf24_worker_fct_ptr fct_ptr);
void 					rf24_worker_push_wait(rf24_task_names task, uint32_t t_cycle_us);
void 					rf24_worker_push(struct rf24_task* task, rf24_worker_fct_ptr fct_ptr);
void 					rf24_worker_add_precondition(struct rf24_task* task, rf24_worker_fct_ptr_ret_bool fct_ptr_precondition);
void 					rf24_worker_add_cyclic_task(rf24_cyclic_tasks_t cyclic_task, uint32_t t_cycle_us, rf24_worker_fct_ptr_ret_bool fct_ptr_precondition);

struct rf24_task* 		rf24_worker_build_task(rf24_task_names task, uint8_t cycles, uint32_t t_cycle_us, bool immediate);
extern void 			rf24_worker_attach_task(struct rf24_task*);
extern void 			rf24_worker_push_task(struct rf24_task*);
void 					rf24_worker_pop_task();
void 					rf24_worker_reset_tasks();

struct rf24_task* 		rf24_worker_current_task();

#endif
