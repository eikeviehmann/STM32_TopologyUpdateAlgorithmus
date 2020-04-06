#include "stm32f1xx.h"
#include <stdint.h>

#include "./rf24/rf24_stm32f1xx/rf24_stm32f1xx.h"
#include "./rf24/rf24_stm32f1xx/rf24_stm32f1xx_uart.h"
#include "./rf24/rf24_module/rf24_module.h"
#include "rf24/rf24.h"

/*-------------------------MAIN FUNCTION------------------------------*/

int main(void)
{
	rf24_init();

    while(1)
    {

    }

    return 0;
}
