#ifndef RF24_STM32F1XX_UART
#define RF24_STM32F1XX_UART

#include "stm32f1xx.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define RF24_DEBUG


//_________________________________________________________________________________________________________________________________________________________________________

#ifdef RF24_DEBUG

/*
#define __NARG__(...)  __NARG_I_(__VA_ARGS__,__RSEQ_N())
#define __NARG_I_(...) __ARG_N(__VA_ARGS__)
#define __ARG_N( \
      _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
     _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
     _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
     _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
     _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
     _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
     _61,_62,_63,N,...) N
#define __RSEQ_N() \
     63,62,61,60,                   \
     59,58,57,56,55,54,53,52,51,50, \
     49,48,47,46,45,44,43,42,41,40, \
     39,38,37,36,35,34,33,32,31,30, \
     29,28,27,26,25,24,23,22,21,20, \
     19,18,17,16,15,14,13,12,11,10, \
     9,8,7,6,5,4,3,2,1,0
// general definition for any function name
#define _VFUNC_(name, n) name##n
#define _VFUNC(name, n) _VFUNC_(name, n)
#define VFUNC(func, ...) _VFUNC(func, __NARG__(__VA_ARGS__)) (__VA_ARGS__)
// definition for FOO
#define RF24_DEBUG(...) VFUNC(RF24_DEBUG, __VA_ARGS__)
//#define RF24_DEBUG1(fmt) do { sprintf(USART_BUFFER, "%s > %s(): %s", __FILENAME__, __FUNCTION__, fmt); rf24_stm32f1xx_usart_write_line(USART_BUFFER); } while (0)
//#define RF24_DEBUG2(fmt, args...) do { sprintf(USART_BUFFER, "%s > %s(): "fmt, __FILENAME__, __FUNCTION__, args); rf24_stm32f1xx_usart_write_line(USART_BUFFER); } while (0)
#define RF24_DEBUG1(fmt) do { sprintf(USART_BUFFER, "%s", fmt); rf24_stm32f1xx_usart_write_line(USART_BUFFER); } while (0)
#define RF24_DEBUG2(fmt, args...) do { sprintf(USART_BUFFER, ""fmt, args); rf24_stm32f1xx_usart_write_line(USART_BUFFER); } while (0)
char USART_BUFFER[255];

char USART_BUFFER[255];

#define debug_print(fmt, ...) \
            do { if (DEBUG) sprintf(USART_BUFFER, fmt, __VA_ARGS__); rf24_stm32f1xx_usart_write_line(USART_BUFFER);} while (0)

#define rf24_print(fmt) \
			do {	\
				sprintf(USART_BUFFER, "%s:%s:%d: "fmt, __FILE__, __FUNCTION__, __LINE__); \
				rf24_stm32f1xx_usart_write_line(USART_BUFFER); \
			} while (0)


#define rf24_debug(fmt, args...) \
			do { if (DEBUG) sprintf(USART_BUFFER, ""fmt, args); rf24_stm32f1xx_usart_write_line(USART_BUFFER);} while (0)

#endif

*/

#endif



//_________________________________________________________________________________________________________________________________________________________________________

#define USART_BUFFER_SIZE   16
#define USART_DELAY_MS      5

//_________________________________________________________________________________________________________________________________________________________________________

typedef struct {
    uint8_t buffer[USART_BUFFER_SIZE];
    uint8_t head_pos;
    uint8_t tail_pos;
} rf24_stm32f1xx_usart_rx_buffer_t;

typedef enum{
	usart_idle = 0,
	usart_send
} rf24_stm32f1xx_usart_state_t;

//_________________________________________________________________________________________________________________________________________________________________________

extern volatile rf24_stm32f1xx_usart_state_t usart_state;
extern volatile rf24_stm32f1xx_usart_rx_buffer_t uart_buffer;

extern void rf24_stm32f1xx_usart_init(uint32_t baudrate);
extern void rf24_stm32f1xx_usart_write_byte(uint8_t byte);
extern int rf24_stm32f1xx_usart_write_str(char *str);
extern int rf24_stm32f1xx_usart_write_line(char *str);

#endif
