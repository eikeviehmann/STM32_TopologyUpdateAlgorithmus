#ifndef RF24_STM32F1XX_SPI_H
#define RF24_STM32F1XX_SPI_H

#include "stm32f1xx.h"
#include <stdint.h>

extern void 	rf24_stm32f1xx_spi_init();
extern void 	rf24_stm32f1xx_spi_csn_low();
extern void 	rf24_stm32f1xx_spi_csn_high();
extern void 	rf24_stm32f1xx_spi_write_byte(uint8_t data);
extern uint8_t 	rf24_stm32f1xx_spi_shift_byte(uint8_t data);

#endif
