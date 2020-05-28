#include "../rf24_stm32f1xx/rf24_stm32f1xx.h"

#include "../rf24_module/rf24_module.h"

static const uint8_t RF24_STM32F1XX_TIMER2_TICK_US = 50;

volatile rf24_stm32f1xx_pin_state led_pin_state = low;
volatile rf24_stm32f1xx_event_cycle event_cycles[5];


void SysTick_Handler(void){}

void EXTI2_IRQHandler(void)
{
	if (EXTI->PR & EXTI_PR_PR2)
	{
		// clear pending bit
		EXTI->PR = EXTI_PR_PR2;

		rf24_module_irq_handler();
	}
}

void TIM2_IRQHandler(void)
{
	if(TIM2->SR & TIM_SR_UIF)
	{
		// clear pending bit
		TIM2->SR &= ~(TIM_SR_UIF);

		for(uint8_t i=0; i<5; i++)
		{
			if(event_cycles[i].enable){

				event_cycles[i].tick+= RF24_STM32F1XX_TIMER2_TICK_US;

				if(event_cycles[i].tick >= event_cycles[i].us){
					event_cycles[i].tick = 0;
					event_cycles[i].fct_ptr();
				}
			}
		}
	}
}

void EXTI15_10_IRQHandler(void)
{
	if (EXTI->PR & EXTI_PR_PR13)
	{
		// clear pending bit
		EXTI->PR = EXTI_PR_PR13;
	}
}

// FUNCTIONS

void rf24_stm32f1xx_start_stopwatch() { TIM3->EGR |= 0x0001; }

uint32_t rf24_stm32f1xx_stop_stopwatch() { return TIM3->CNT; }

uint8_t* rf24_stm32f1xx_get_uuid_md5hashed()
{
	// Read UUID from stm32f1xx memory, stored under address 0x1FFFF7E8
	uint8_t* uuid = (uint8_t*) 0x1FFFF7E8;

	// Generate md5 hash of unique device id
	MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, uuid, 12);
	MD5Final(rf24_stm32f1xx_uuid_md5hashed, &context);

	// Specify as LOCALY ADMINISTRATED address
	rf24_stm32f1xx_uuid_md5hashed[0] |= 0x2;

	// Specify as UNICAST address
	rf24_stm32f1xx_uuid_md5hashed[0] &= ~0x1;

	return rf24_stm32f1xx_uuid_md5hashed;
}

uint8_t* rf24_stm32f1xx_get_uuid()
{
	uint8_t k = 0;

    for(int i=0; i<=2; i++)
    {
    	uint32_t uuid = STM32_UUID_ADDRESS[i];

    	rf24_stm32f1xx_uuid[k++] = (uint8_t)(uuid >> 24);
    	rf24_stm32f1xx_uuid[k++] = (uint8_t)(uuid >> 16);
    	rf24_stm32f1xx_uuid[k++] = (uint8_t)(uuid >> 8);
    	rf24_stm32f1xx_uuid[k++] = (uint8_t)(uuid >> 0);
    }

    return rf24_stm32f1xx_uuid;
}

void rf24_stm32f1xx_set_system_clock_72Mhz(void)
{
    // conf system clock : 72MHz using HSE 8MHz crystal w/ PLL X 9 (8MHz x 9 = 72MHz)
    FLASH->ACR      |= FLASH_ACR_LATENCY_2; 	// Two wait states, per datasheet
    RCC->CFGR       |= RCC_CFGR_PPRE1_2;    	// prescale AHB1 = HCLK/2
    RCC->CR         |= RCC_CR_HSEON;        	// enable HSE clock
    while( !(RCC->CR & RCC_CR_HSERDY) );    	// wait for the HSEREADY flag

    RCC->CFGR       |= RCC_CFGR_PLLSRC;     	// set PLL source to HSE
    RCC->CFGR       |= RCC_CFGR_PLLMULL9;   	// multiply by 9
    RCC->CR         |= RCC_CR_PLLON;        	// enable the PLL
    while( !(RCC->CR & RCC_CR_PLLRDY) );    	// wait for the PLLRDY flag

    RCC->CFGR       |= RCC_CFGR_SW_PLL;     	// set clock source to pll

    while( !(RCC->CFGR & RCC_CFGR_SWS_PLL) );    // wait for PLL to be CLK

    SystemCoreClockUpdate();                	// calculate the SYSCLOCK value
    SysTick_Config(SystemCoreClock/1000);		// Generate interrupt each 1 ms
}

void rf24_stm32f1xx_init_tim2(uint32_t interrupt_cycle_us){

	// Enable the TIM2 clock
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// Start by making sure the timer's counter is off
	TIM2->CR1 &= ~(TIM_CR1_CEN);

	// Next, reset the peripheral. (This is where a HAL can help)
	RCC->APB1RSTR |=  (RCC_APB1RSTR_TIM2RST);
	RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST);

	// Set the timer to PRESCALER/AUTOREALOD timing registers
	TIM2->PSC = 71; // 71+1 = 72/72 = 1Mhz
	TIM2->ARR = interrupt_cycle_us;

	// Send an update event to reset the timer and apply settings
	TIM2->EGR  |= TIM_EGR_UG;

	// Enable the hardware interrupt
	TIM2->DIER |= TIM_DIER_UIE;

	// Enable the NVIC interrupt for TIM2
	NVIC_SetPriority(TIM2_IRQn, 0x00);
	NVIC_EnableIRQ(TIM2_IRQn);

	// Enable the timer
	TIM2->CR1  |= TIM_CR1_CEN;
}

void rf24_stm32f1xx_set_timer_interrupt_ms(uint8_t index, uint16_t ms, fct_ptr fct_ptr)
{
	event_cycles[index].fct_ptr = fct_ptr;
	event_cycles[index].us = ms * 1000;
	event_cycles[index].tick = 0;
	event_cycles[index].enable = true;
}

void rf24_stm32f1xx_set_timer_interrupt_us(uint8_t index, uint16_t us, fct_ptr fct_ptr)
{
	event_cycles[index].fct_ptr = fct_ptr;
	event_cycles[index].us = us;
	event_cycles[index].tick = 0;
	event_cycles[index].enable = true;
}


void rf24_stm32f1xx_init_tim3()
{

	RCC->APB1ENR |= (1 << 1);	// enable clock for that module for TIM3. Bit1 in RCC APB1ENR register

	TIM3->CR1 = 0x0000;			// reset CR1 just in case

	// fCK_PSC / (PSC[15:0] + 1)
	TIM3->PSC = 71;				// 72 Mhz / 71 + 1 = 1 Mhz timer clock speed

	// This is set to max value (0xFFFF) since we manually check if the value reach to 1000 in the delay_ms function
	TIM3->ARR = 0xFFFF;

	// Finally enable TIM3 module
	TIM3->CR1 |= (1 << 0);
}

uint32_t rf24_stm32f1xx_get_tim3_count()
{
	return TIM3->CNT;
}

void delay_ms(volatile uint32_t ms)
{
	for(ms; ms>0; ms--)
	{
		TIM3->EGR |= 0x0001;
 		while(TIM3->CNT < 1000);
	}
}

void delay_us(volatile uint32_t us)
{
	TIM3->EGR |= 0x0001;
 	while(TIM3->CNT < us);
}

void rf24_stm32f1xx_init_led(uint16_t update_cycle_ms)
{
    RCC->APB2ENR    |= RCC_APB2ENR_IOPCEN;                      // enable clock
    GPIOC->CRH      &=  ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);    // reset PC13
    GPIOC->CRH      |= (GPIO_CRH_MODE13_1 | GPIO_CRH_MODE13_0); // config PC13

    rf24_stm32f1xx_set_timer_interrupt_ms(0, update_cycle_ms, rf24_stm32f1xx_led_toggle);
}

void rf24_stm32f1xx_set_led_cycle(uint16_t update_cycle_ms)
{
	event_cycles[0].us = update_cycle_ms * 1000;
}

void rf24_stm32f1xx_led_on()
{
	GPIOC->BSRR = GPIO_BSRR_BS13;
}

void rf24_stm32f1xx_led_off()
{
	GPIOC->BSRR = GPIO_BSRR_BR13;
}

void rf24_stm32f1xx_led_toggle()
{
	if(led_pin_state){
		rf24_stm32f1xx_led_off();
		led_pin_state = low;
	}
	else
	{
		rf24_stm32f1xx_led_on();
		led_pin_state = high;
	}

}

void rf24_stm32f1xx_init_ce(void)
{
    RCC->APB2ENR    |= RCC_APB2ENR_IOPAEN;
    GPIOA->CRL      &=  ~(GPIO_CRL_MODE3 | GPIO_CRL_CNF3);
    GPIOA->CRL      |= (GPIO_CRL_MODE3_1 | GPIO_CRL_MODE3_0);
}

void rf24_stm32f1xx_ce_high()
{
	// Set PA3 high
	GPIOA->BSRR = GPIO_BSRR_BS3;
}

void rf24_stm32f1xx_ce_low()
{
	// Set PA3 low
	GPIOA->BSRR = GPIO_BSRR_BR3;
}

void rf24_nucleof103rb_irq_blue_button_init()
{
	// Reset PC13
	GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);

	// Set PC13 input pull up (CNF13_1=1, CNF13_0=0, MODE13_1=0, MODE13_0=0, ODR13=1)
	GPIOC->CRH |= GPIO_CRH_CNF13_1;
	GPIOC->CRH &= ~(GPIO_CRH_CNF13_0);
	GPIOC->CRH &= ~(GPIO_CRH_MODE13_1 | GPIO_CRH_MODE13_0);
	GPIOC->ODR |= GPIO_ODR_ODR13;

	// Enable clock port c
	RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

	// Enable clock alternate function
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

	// Map PC13 to EXTI13
	AFIO->EXTICR[3] |= AFIO_EXTICR4_EXTI13_PC;

	// Interrupt request from line 13 is not masked => enabled
	EXTI->IMR |= EXTI_IMR_MR13; //(1 << 13);

	// Enable falling edge trigger
	EXTI->FTSR |= EXTI_FTSR_TR13; //(1 << 13);
	// Disable rising edge trigger
	EXTI->RTSR &= ~(EXTI_RTSR_TR13); //(1 << 13);

	// Set EXTI15_10_IRQn to priority level 0
	NVIC_SetPriority(EXTI15_10_IRQn, 0x00);

	// Enable EXTI15_10_IRQn from NVIC
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void rf24_stm32f1xx_irq_init()
{
	// Reset PA2
	GPIOA->CRL &= ~(GPIO_CRL_MODE2 | GPIO_CRL_CNF2);

	// Set PA2 input pull up (CNF13_1=1, CNF13_0=0, MODE13_1=0, MODE13_0=0, ODR13=1)
	GPIOA->CRL |= GPIO_CRL_CNF2_1;
	GPIOA->CRL &= ~(GPIO_CRL_CNF2_0);
	GPIOA->CRL &= ~(GPIO_CRL_MODE2_1 | GPIO_CRL_MODE2_0);
	GPIOA->ODR |= GPIO_ODR_ODR2;

	// Enable clock port a
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

	// Enable clock alternate function
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

	// Map PA2 to EXTI0
	AFIO->EXTICR[0] |= AFIO_EXTICR1_EXTI2_PA;

	// Interrupt request from line 2 is not masked => enabled
	EXTI->IMR |= EXTI_IMR_MR2; //(1 << 2);

	// Enable falling edge trigger
	EXTI->FTSR |= EXTI_FTSR_TR2; //(1 << 2);
	// Disable rising edge trigger
	EXTI->RTSR &= ~(EXTI_RTSR_TR2); //(1 << 2);

	// Set EXTI2 to priority level 1
	NVIC_SetPriority(EXTI2_IRQn, 0x10);

	// Enable EXT2 IRQ from NVIC
	NVIC_EnableIRQ(EXTI2_IRQn);
}

void rf24_stm32f1xx_init()
{
	rf24_stm32f1xx_set_system_clock_72Mhz();
	rf24_stm32f1xx_init_tim2(RF24_STM32F1XX_TIMER2_TICK_US);
	rf24_stm32f1xx_init_tim3();
	rf24_stm32f1xx_init_led(1000);
	rf24_stm32f1xx_init_ce();
	rf24_stm32f1xx_ce_low();
	rf24_stm32f1xx_irq_init();
}

