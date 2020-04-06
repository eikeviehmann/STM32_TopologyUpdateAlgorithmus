#include "rf24_stm32f1xx_spi.h"

void rf24_stm32f1xx_spi_init(void)
{
    RCC->APB2ENR    |= RCC_APB2ENR_IOPAEN;  // Enable GPIOA clock
    RCC->APB2ENR    |= RCC_APB2ENR_SPI1EN;  // Enable SPI1 clock

    // Reset PA4, PA5, PA, PA7 MODE and CNF to 0b00
    GPIOA->CRL &=	~(	(GPIO_CRL_MODE4 | GPIO_CRL_CNF4) |
    					(GPIO_CRL_MODE5 | GPIO_CRL_CNF5) |
						(GPIO_CRL_MODE6 | GPIO_CRL_CNF6) |
						(GPIO_CRL_MODE7 | GPIO_CRL_CNF7)	);

    // Init Pin PA4 NSS  - Mode = 11 (50Mhz) - CNF = 0b00
    GPIOA->CRL |= GPIO_CRL_MODE4_1 | GPIO_CRL_MODE4_0;

    // Init Pin PA5 SCK  - Mode = 11 (50Mhz) - CNF = 0b10 (Alt Function PP)
    GPIOA->CRL |= GPIO_CRL_MODE5_1 | GPIO_CRL_MODE5_0 | GPIO_CRL_CNF5_1;

    // Init Pin PA6 MISO - Mode = 11 (50Mhz) - CNF = 0b10 (Alt Function PP)
    GPIOA->CRL |= GPIO_CRL_MODE6_1 | GPIO_CRL_MODE6_0 | GPIO_CRL_CNF6_1;

    // Init Pin PA7 MOSI - Mode = 11 (50Mhz) - CNF = 0b10 (Alt Function PP)
    GPIOA->CRL |= GPIO_CRL_MODE7_1 | GPIO_CRL_MODE7_0 | GPIO_CRL_CNF7_1;

    // Set the SS (CSN) pin high
    GPIOA->BSRR = GPIO_BSRR_BS4;

    // 1. __________________________________________________________________________
    // Configuring the SPI in master mode (Source: ST RM0008 Reference manual)

    uint32_t spi_config = 0;

    spi_config |= SPI_CR1_SPE;				// Enable SPI Interface

    // 1.1 Select the BR[2:0] bits to define the serial clock baud rate (see SPI_CR1 register).
    spi_config |= SPI_CR1_BR_2;				// SPI-Baudrate = F_PCLK/32

    // 1.2 Select the CPOL and CPHA bits to define one of the four relationships between the data transfer and the serial clock (see Figure 240).
    spi_config &= ~(SPI_CR1_CPHA);			// SPI clock to 0 when idle
    //spi_config |= SPI_CR1_CPHA;			// SPI clock to 1 when idle
    spi_config &= ~(SPI_CR1_CPOL);			// First clock transition is the first data capture edge
    //spi_config |= SPI_CR1_CPOL;			// Second clock transition is the first data capture edge

    // 1.3 Set the DFF bit to define 8- or 16-bit data frame format
    //spi_config |= SPI_CR1_DFF; 			// 16-bit data frame
    spi_config &= ~(SPI_CR1_DFF);			// 8-bit data frame

    // 1.4 Configure the LSBFIRST bit in the SPI_CR1 register to define the frame format.
    //spi_config |= SPI_CR1_LSBFIRST; 		// LSB transmitted first
    spi_config &= ~(SPI_CR1_LSBFIRST);		// MSB transmitted first

    // 1.5 If the NSS pin is required in input mode, in hardware mode, connect the NSS pin to a high-level signal during the complete byte transmit sequence. In NSS software mode,
	//		set the SSM and SSI bits in the SPI_CR1 register. If the NSS pin is required in output mode, the SSOE bit only should be set.
    spi_config |= SPI_CR1_SSM;				// Software slave management enabled
    spi_config |= SPI_CR1_SSI;				// Internal slave select

    // 1.6 The MSTR and SPE bits must be set (they remain set only if the NSS pin is connected to a high-level signal)
    spi_config |= SPI_CR1_MSTR;				// Configure as Master

    SPI1->CR1  |= spi_config;
}

void rf24_stm32f1xx_spi_csn_low(){
	GPIOA->BSRR = GPIO_BSRR_BR4;
}

void rf24_stm32f1xx_spi_csn_high(){
	GPIOA->BSRR = GPIO_BSRR_BS4;
}

uint8_t rf24_stm32f1xx_spi_shift_byte(uint8_t data){
	//while (!(SPI1->SR & SPI_SR_TXE)){};
	*(uint8_t*)&(SPI1->DR) = data;
	while(!(SPI1->SR & SPI_SR_RXNE)){};
	return SPI1->DR;
}

void rf24_stm32f1xx_spi_write_byte(uint8_t data){
	*(uint8_t*)&(SPI1->DR) = data;
}


