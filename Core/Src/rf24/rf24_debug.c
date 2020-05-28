#include "rf24_debug.h"
#include "rf24_stm32f1xx/rf24_stm32f1xx_uart.h"

#ifdef RF24_DEBUG

static const char *rf24_debug_source_string[] = { "", "rf_module", "mac", "csma_ca", "network", "controller"};
static const char *rf24_debug_msg_type_string[] = { "", "[ -> ]", "[ <- ]", "[ TX ]", "[ RX ]", "[ TO ]", "[INFO]"};

struct rf24_timer *debug_timer;

bool enable = false;

void rf24_debug_enable()
{
	enable = true;
}

void rf24_debug_disable()
{
	enable = false;
}

void rf24_debug_attach_timer(struct rf24_timer *timer)
{
	debug_timer = timer;
}

void rf24_debug(rf24_debug_source source, rf24_debug_msg_type msg_type, uint8_t frame_subtype, uint8_t frame_subtype_reference, rf24_mac_addr *mac_addr, char* format, ...)
{
	if(enable)
	{
		char string[30];

		// (1) Print time stamp
		//__________________________________________________________________________________________________________________________________________________
		if(debug_timer){
			rf24_timespan timespan = rf24_worker_us_to_timespan(debug_timer->t_count_us);
			sprintf(string, "%d:%d:%d ", timespan.s, timespan.ms, timespan.us);
			rf24_printf("%-10s", string);
		}

		// (2) Print source & message type (class)
		//__________________________________________________________________________________________________________________________________________________
		rf24_printf("%-10s %-7s", rf24_debug_source_string[source], rf24_debug_msg_type_string[msg_type]);

		// (3) Print mac frame sub type & reference
		//__________________________________________________________________________________________________________________________________________________
		string[0] = '\0';
		if(frame_subtype > 0)
		{
			if(frame_subtype_reference > 0)
				sprintf(string, "%s(%s)", rf24_mac_frame_subtype_string_short[frame_subtype], rf24_mac_frame_subtype_string_short[frame_subtype_reference]);
			else
				sprintf(string, "%s", rf24_mac_frame_subtype_string_short[frame_subtype]);
		}
		rf24_printf("%-20s", string);

		// (4) Print transmitter / receiver
		//__________________________________________________________________________________________________________________________________________________
		string[0] = '\0';
		if(mac_addr) sprintf(string, "%d:%d:%d:%d:%d:%d", mac_addr->bytes[0], mac_addr->bytes[1], mac_addr->bytes[2], mac_addr->bytes[3], mac_addr->bytes[4], mac_addr->bytes[5]);
		rf24_printf("%-24s", string);

		// (5) Print optional VARGS
		//__________________________________________________________________________________________________________________________________________________
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
							char str[10];
							unsigned long int lu = va_arg(args, unsigned long int);
							itoa(lu, str, 10);
							rf24_stm32f1xx_usart_write_str(str);
							break;
						}
						case 's':
						{
							char str[10];
							long int ls = va_arg(args, long int);
							itoa(ls, str, 10);
							rf24_stm32f1xx_usart_write_str(str);
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
					char str[10];
					int d = va_arg(args, int);
					itoa(d, str, 10);
					rf24_stm32f1xx_usart_write_str(str);
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

char buffer[100];

char* decimal_to_string(uint8_t* arr, uint8_t length, char space)
{
	uint8_t i = 0, k = 0;

	for (i = 0; i < (length-1); i++) k += sprintf(&buffer[k], "%d%c", arr[i], space);

	sprintf(&buffer[k], "%d", arr[i]);

	return buffer;
}

char* decimal_to_binary(int n)
{
	int c, d, count = 0;
	char *ptr = (char*) malloc(8+1);

	for (c = 7 ; c >= 0 ; c--)
	{
		d = n >> c;

		if (d & 1)
		*(ptr+count) = 1 + '0';
		else
		*(ptr+count) = 0 + '0';

		count++;
	}
	*(ptr+count) = '\0';

	return ptr;
}

#endif
