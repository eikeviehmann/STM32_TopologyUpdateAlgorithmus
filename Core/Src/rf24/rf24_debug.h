#include "rf24_network/rf24_mac.h"
#include "rf24_network/rf24_worker.h"

#define VOID 0

typedef enum{
	TRANSMIT = 1,
	RECEIVE,
	TRANSMISSION,
	RECEPTION,
	TIMEOUT,
	INFO
} rf24_debug_msg_type;

typedef enum{
	RF_MODULE = 1,
	MAC ,
	CSMA_CA,
	NETWORK,
	CONTROLLER,
} rf24_debug_source;

extern void rf24_debug(
		rf24_debug_source source,
		rf24_debug_msg_type msg_type,
		rf24_mac_frame_subtype frame_subtype,
		rf24_mac_frame_subtype frame_subtype_reference,
		rf24_mac_addr *receiver,
		char* format, ...);

extern void rf24_debug_enable();
extern void rf24_debug_disable();

extern void rf24_printf(char* format, ...);
extern void rf24_printf_vargs(char* format, va_list args);

extern void rf24_debug_attach_timer(struct rf24_timer *timer);

char* decimal_to_string(uint8_t* arr, uint8_t length, char space);
char* decimal_to_binary(int n);
