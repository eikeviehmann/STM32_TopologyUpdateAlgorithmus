#include "rf24_debug.h"
#include "rf24_stm32f1xx/rf24_stm32f1xx_uart.h"

#ifdef RF24_DEBUG

char debug_buffer[128];
char converter_buffer[128];

static const char *rf24_debug_source_string[] = { "", "rf_module", "mac", "csma_ca", "network", "controller"};
static const char *rf24_debug_msg_type_string[] = { "", "[ -> ]", "[ <- ]", "[ TX ]", "[ RX ]", "[ TO ]", "[INFO]"};

bool rf24_debug_enabled = true;
bool rf24_debug_source_enable[5];
bool rf24_debug_msg_type_enable[5];

void rf24_debug_enable()
{
	rf24_debug_enabled = true;
	for(int i=0; i<5; i++) rf24_debug_source_enable[i] = true;
	for(int i=0; i<5; i++) rf24_debug_msg_type_enable[i] = true;
}

void rf24_debug_disable()
{
	rf24_debug_enabled = false;
}


void rf24_debug(
	rf24_debug_source source,
	rf24_debug_msg_type msg_type,
	uint8_t frame_subtype,
	uint8_t frame_subtype_reference,
	rf24_mac_addr *receiver,
	char* format, ...)
{
	if(rf24_debug_enabled /*&& rf24_debug_source_enable[source] &&  rf24_debug_msg_type_enable[msg_type]*/)
	{
		// Print source & msg_type
		rf24_printf("%-10s %-7s",
					rf24_debug_source_string[source],
					rf24_debug_msg_type_string[msg_type]);

		// Print subtype & reference
		char string[30] = "";

		if(frame_subtype > 0)
		{
			if(frame_subtype_reference > 0){
				sprintf(string, "%s(%s)",
						rf24_mac_frame_subtype_string_short[frame_subtype],
						rf24_mac_frame_subtype_string_short[frame_subtype_reference]);
			}
			else{
				sprintf(string, "%s",
						rf24_mac_frame_subtype_string_short[frame_subtype]);
			}
		}

		rf24_printf("%-20s", string);

		// Print receiver address
		string[0] = '\0';
		if(receiver != NULL) sprintf(string, "%s", decimal_to_string(receiver->bytes, 6, ':'));
		rf24_printf("%-24s", string);

		// Print vargs
		va_list args;
		va_start(args, format);
		rf24_printf_vargs(format, args);
		va_end(args);
	}
}

void rf24_printf(char* format, ...)
{
	va_list args;
	va_start(args, format);
	rf24_printf_vargs(format, args);
	va_end(args);
}

void rf24_printf_vargs(char* format, va_list args)
{
	while (*format != '\0')
	{
		if (*format == '%')
		{
			format++;

			switch(*format)
			{
				case 'l':
				{
					format++;

					switch(*format)
					{
						case 'u':
						{
							unsigned long int lu = va_arg(args, unsigned long int);
							itoa(lu, debug_buffer, 10);
							rf24_stm32f1xx_usart_write_str(debug_buffer);
							break;
						}
						case 's':
						{
							long int ls = va_arg(args, long int);
							itoa(ls, debug_buffer, 10);
							rf24_stm32f1xx_usart_write_str(debug_buffer);
							break;
						}
					}
					break;
				}
				case 'c':
				{
					char c = va_arg(args, int);
					rf24_stm32f1xx_usart_write_byte(c);
					break;
				}
				case 'd':
				{
					int d = va_arg(args, int);
					itoa(d, debug_buffer, 10);
					rf24_stm32f1xx_usart_write_str(debug_buffer);
					break;
				}
				case '-':
				{
					format++;
					uint8_t index = 0;
					char spaces_string[4];
					uint8_t spaces_number = 0;

					while(*format != 's')
					{
						if(index > 2) rf24_stm32f1xx_usart_write_str("[format: %-123s]");
						spaces_string[index++] = *format;
						format++;
					}

					spaces_string[index] = '\0';
					spaces_number = atoi(spaces_string);
					char *str = va_arg(args, char*);
					uint8_t string_length = rf24_stm32f1xx_usart_write_str(str);

					while(string_length < spaces_number)
					{
						string_length++;
						rf24_stm32f1xx_usart_write_byte(' ');
					}

					break;
				}
				case 's':
				{
					char *str = va_arg(args, char*);
					rf24_stm32f1xx_usart_write_str(str);
					break;
				}
				default:{}
			}
		}
		else rf24_stm32f1xx_usart_write_byte(*format);

		format++;
	}
}

char* decimal_to_string(uint8_t* arr, uint8_t length, char space)
{
	//char *str = (char*) malloc(length+1);

	uint8_t i = 0, k = 0;

	for (i = 0; i < (length-1); i++)
		k += sprintf(&converter_buffer[k], "%d%c", arr[i], space);

	sprintf(&converter_buffer[k], "%d", arr[i]);

	return converter_buffer;
}

char* decimal_to_binary(int n)
{
	int c, d, count;
	char *pointer;

	count = 0;
	pointer = (char*) malloc(8+1);

	if (pointer == NULL){}

	for (c = 7 ; c >= 0 ; c--)
	{
		d = n >> c;

		if (d & 1)
		*(pointer+count) = 1 + '0';
		else
		*(pointer+count) = 0 + '0';

		count++;
	}
	*(pointer+count) = '\0';

	return pointer;
}

#endif
