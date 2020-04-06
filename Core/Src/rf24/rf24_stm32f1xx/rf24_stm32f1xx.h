#ifndef RF24_STM32F1XX
#define RF24_STM32F1XX

#include "stm32f1xx.h"
#include <stdint.h>
#include <stdbool.h>

#include "../inc/stm32f10x_md5.h"

#define STM32_UUID_ADDRESS ( (uint32_t *) 0x1FFFF7E8 ) //0x1FFFF7E8 //0x1FFF7A10
uint8_t rf24_stm32f1xx_uuid[12];
uint8_t rf24_stm32f1xx_uuid_md5hashed[16];

typedef enum {
	low = 0,
	high = 1
} rf24_stm32f1xx_pin_state;

typedef void (*fct_ptr) (void);

typedef struct {
	fct_ptr fct_ptr;
	uint32_t us;
	uint32_t tick;
	bool enable;
}
rf24_stm32f1xx_event_cycle;

extern void 	rf24_stm32f1xx_start_stopwatch();
extern uint32_t rf24_stm32f1xx_stop_stopwatch();

extern void 	rf24_stm32f1xx_set_timer_interrupt_ms(uint8_t index, uint16_t ms, fct_ptr fct_ptr);
extern void 	rf24_stm32f1xx_set_timer_interrupt_us(uint8_t index, uint16_t us, fct_ptr fct_ptr);

extern uint8_t* rf24_stm32f1xx_get_uuid();
extern uint8_t* rf24_stm32f1xx_get_uuid_md5hashed();

extern void 	rf24_stm32f1xx_led_on();
extern void 	rf24_stm32f1xx_led_off();
extern void 	rf24_stm32f1xx_led_toggle();
extern void 	rf24_stm32f1xx_set_led_cycle(uint16_t update_cycle_ms);

extern void 	rf24_stm32f1xx_ce_high();
extern void 	rf24_stm32f1xx_ce_low();

extern uint32_t rf24_stm32f1xx_get_tim3_count();

extern void 	delay_ms(volatile uint32_t ms);
extern void 	delay_us(volatile uint32_t us);

extern void 	rf24_stm32f1xx_init();

#endif
