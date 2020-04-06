#include "../rf24_stm32f1xx/rf24_stm32f1xx_uart.h"

#include "../rf24_urci/rf24_urci.h"
#include <inttypes.h>

volatile rf24_stm32f1xx_usart_state_t  usart_state = usart_idle;
volatile rf24_stm32f1xx_usart_rx_buffer_t uart_buffer = { {0}, 0, 0 };

void rf24_stm32f1xx_usart_init(uint32_t baudrate)
{
	// enable GPIOA clock
    RCC->APB2ENR    |= RCC_APB2ENR_IOPAEN;
    // enable USART1 clock
    RCC->APB2ENR    |= RCC_APB2ENR_USART1EN;

    // reset PA9
    GPIOA->CRH &= ~(GPIO_CRH_CNF9  | GPIO_CRH_MODE9);
    // reset PA10
    GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);

    // 0b11 50MHz output
    GPIOA->CRH |= GPIO_CRH_MODE9_1 | GPIO_CRH_MODE9_0;
    // PA9: output @ 50MHz - alt-function push-pull
    GPIOA->CRH |= GPIO_CRH_CNF9_1;
    // PA10 rx - mode = 0b00 (input) - CNF = 0b01 (input floating)
    GPIOA->CRH |= GPIO_CRH_CNF10_0;

    // configure USART1 registers
    uint32_t baud = (uint32_t)(SystemCoreClock/baudrate);
    USART1->BRR  = baud;
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    // configure NVIC
    NVIC_EnableIRQ(USART1_IRQn);
}

void rf24_stm32f1xx_usart_write_byte(uint8_t byte)
{
	if(byte == '\n') rf24_stm32f1xx_usart_write_byte('\r');

    USART1->DR = (int)(byte);
    while (!(USART1->SR & USART_SR_TXE));
}

int rf24_stm32f1xx_usart_write_str(char *str)
{
    int count = 0;

    while(*str){
    	rf24_stm32f1xx_usart_write_byte(*str);
        str++;
        count++;
    }

    return count;
}

int rf24_stm32f1xx_usart_write_line(char *str)
{
    int count = 0;
    count = rf24_stm32f1xx_usart_write_str(str);
    rf24_stm32f1xx_usart_write_byte('\n');
    return(count);
}

void rf24_stm32f1xx_usart_buffer_char(uint8_t c)
{
    int i = (uart_buffer.head_pos + 1) % USART_BUFFER_SIZE;

    if (i != uart_buffer.tail_pos)
    {
        uart_buffer.buffer[uart_buffer.head_pos] = c;
        uart_buffer.head_pos = i;
    }
}

void USART1_IRQHandler(void)
{
    if(USART1->SR & USART_SR_ORE)
    {
        // process overrun error if needed
    }

    uint8_t in_char = (USART1->DR & 0xFF);

    // if input is escape, send clear screen command
    if(in_char == '\x1B')
    {
    	rf24_stm32f1xx_usart_write_str("---------------------------------------------------------");
    	rf24_stm32f1xx_usart_write_str("\033c");
    	return;
    }

    rf24_stm32f1xx_usart_buffer_char(in_char);
    rf24_urci_putc(in_char);
}
