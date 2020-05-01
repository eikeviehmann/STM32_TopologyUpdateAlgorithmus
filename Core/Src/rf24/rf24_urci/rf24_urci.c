#include "rf24_urci.h"
#include "../rf24_network/rf24_csma_ca.h"
#include "../rf24_network/rf24_network.h"
#include "../rf24_debug.h"

static urci_flags flags;
static uint8_t  counter;
static uint8_t attribute_pos;

const urci_cmd urci_cmds[] = {

	{	.name = "register",
		.description = "blabla",
		.details = "blabla",
		.set_type = rf24_urci_string,
		.fct_ptr_set_string = rf24_urci_print_register },

	{	.name = "help",
		.description = "returns page of all available commands or presents information to given command",
		.details = "R/W: [R,W], INPUT-FORMAT (string) [\"name of command\"]",
		.set_type = rf24_urci_string,
		.get_type = rf24_urci_void,
		.fct_ptr_set_string = rf24_urci_help,
		.fct_ptr = rf24_urci_print_help},

	{	.name = "channel",
		.description = "set or get rf channel",
		.details = "R/W: [R+W], INPUT-FORMAT (int): [0,1]",
		.set_type = rf24_urci_uint8,
		.get_type = rf24_urci_uint8,
		.fct_ptr_set_uint8 = rf24_module_set_rf_channel,
		.fct_ptr_get_uint8 = rf24_module_get_rf_channel },

	{	.name = "output-power",
		.description = "set or get rf channel",
		.details = "R/W: [R+W], INPUT-FORMAT (int): [0,1]",
		.set_type = rf24_urci_uint8,
		.get_type = rf24_urci_uint8,
		.fct_ptr_set_uint8 = rf24_module_set_rf_outputpower,
		.fct_ptr_get_uint8 = rf24_module_get_rf_outputpower },

	{	.name = "crc-length",
		.description = "prim_rx",
		.details = "R/W:[R+W], INPUT-FORMAT (int): [0,1]",
		.set_type = rf24_urci_uint8,
		.get_type = rf24_urci_uint8,
		.fct_ptr_set_uint8 = rf24_urci_set_crc_length,
		.fct_ptr_get_uint8 = rf24_urci_get_crc_length},

	{	.name = "crc.enable",
		.description = "enable crc",
		.details = "R/W:[R+W], INPUT-FORMAT (int): [0,1]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_module_enable_crc },

	{	.name = "crc.disable",
		.description = "disable crc",
		.details = "R/W:[R+W], INPUT-FORMAT (int): [0,1]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_module_disable_crc },

	{	.name = "tx-address",
		.description = "prim_rx",
		.details = "R/W:[R+W], INPUT-FORMAT (int): [0,1]",
		.set_type = rf24_urci_string, .get_type = rf24_urci_string,
		.fct_ptr_set_string = rf24_urci_set_tx_address,
		.fct_ptr_get_string = rf24_urci_get_tx_address },

	{	.name = "rx-address.pipe",
		.set_type = rf24_urci_string, .get_type = rf24_urci_string,
		.fct_ptr_set_string = rf24_urci_set_rx_address,
		.fct_ptr_get_string = rf24_urci_get_rx_address,
		.description = "set/get the address of rx pipe 0",
		.details = "R/W:[R,W], IN:[string], FORMAT: \"n:R:F:2:4\"" },

	{	.name = "autoretransmit.count",
		.description = "",
		.set_type = rf24_urci_uint8,
		.get_type = rf24_urci_uint8,
		.fct_ptr_set_uint8 = rf24_module_set_autoretransmit_count,
		.fct_ptr_get_uint8 = rf24_module_get_autoretransmit_count },

	{	.name = "autoretransmit.delay",
		.description = "",
		.set_type = rf24_urci_uint8,
		.get_type = rf24_urci_uint8,
		.fct_ptr_set_uint8 = rf24_module_set_autoretransmit_delay,
		.fct_ptr_get_uint8 = rf24_module_get_autoretransmit_delay },

	{	.name = "datarate",
		.description = "set/get the datarate",
		.details = "R/W:[R,W], allowed inputs: (string) {\"250kbps\", \"1Mbps\", \"2Mbps\"}",
		.set_type = rf24_urci_string,
		.get_type = rf24_urci_string,
		.fct_ptr_set_string = rf24_urci_set_datarate,
		.fct_ptr_get_string = rf24_urci_get_datarate },

	{	.name = "address-width",
		.description = "set/get address width",
		.details = "R/W:[R,W], allowed inputs: (int) {3,4,5}",
		.set_type = rf24_urci_uint8,
		.get_type = rf24_urci_uint8,
		.fct_ptr_set_uint8 = rf24_module_set_address_width,
		.fct_ptr_get_uint8 = rf24_module_get_address_width },

	{	.name = "settings",
		.description = "prints settings of rf module",
		.details = "R/W:[R]",
		.get_type = rf24_urci_void,
		.fct_ptr_get_uint8 = rf24_module_get_address_width },


	{	.name = "payload-width.pipe",
		.description = "set/get the address of rx pipe 0",
		.details = "R/W:[R,W], IN:[string], FORMAT: \"n:R:F:2:4\"",
		.set_type = rf24_urci_uint8, .get_type = rf24_urci_uint8,
		.fct_ptr_set_uint8 = rf24_urci_set_payload_width,
		.fct_ptr_get_uint8 = rf24_urci_get_payload_width
	},

	{	.name = "flush-tx",
		.description = "",
		.details = "R/W:[R]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_module_flush_tx },

	// MAC FUNCTIONS

	{	.name = "transmit",
		.description = "",
		.details = "",
		.set_type = rf24_urci_string,
		.fct_ptr_set_string = rf24_urci_transmit },

	{	.name = "ping",
		.description = "",
		.details = "R/W:[W]",
		.set_type = rf24_urci_string,
		.fct_ptr_set_string = rf24_urci_ping },

	{	.name = "update-topology",
		.description = "",
		.details = "R/W:[R]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_network_start_topology_update },

	{	.name = "print-topology",
		.description = "",
		.details = "R/W:[R]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_network_print_topology },

	{	.name = "print-neighbors",
		.description = "",
		.details = "R/W:[R]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_network_print_neighbors },

	{	.name = "mac-address",
		.description = "",
		.details = "R/W:[R]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_urci_print_mac_address },

	/*{	.name = "transmit-num",
		.description = "blabla",
		.details = "blabla",
		.set_type = rf24_urci_string,
		.fct_ptr_set_string = rf24_urci_send_num },*/

	{	.name = "transmit-tum",
		.description = "",
		.details = "",
		.set_type = rf24_urci_string,
		.fct_ptr_set_string = rf24_urci_transfer_topology },

	{	.name = "transmit-mac-frame",
		.description = "",
		.details = "",
		.set_type = rf24_urci_string,
		.fct_ptr_set_string = rf24_urci_transmit_mac_frame },

	{	.name = "transfer-topology",
		.description = "",
		.details = "R/W:[R]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_network_transfer_topology },

	{	.name = "enable-debug",
		.description = "",
		.details = "R/W:[R]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_debug_enable },

	{	.name = "disable-debug",
		.description = "",
		.details = "R/W:[R]",
		.get_type = rf24_urci_void,
		.fct_ptr = rf24_debug_disable },


	/*{	.name = "reset",
		.description = "adasdasdasdasdasdasd",
		.val_type = VOID,
		.fct_ptr = rf24_config },

	{	.name = "config",
		.description = "adsadasdasdasdasdas",
		.val_type = VOID,
		.fct_ptr = NULL },

	{	.name = "autoack.disable",
		.description = "adsadasdasdasdasdas",
		.val_type = VOID,
		.fct_ptr = rf24_disable_autoack },

	{	.name = "autoretransmit.disable",
		.description = "adsadasdasdasdasdas",
		.val_type = VOID,
		.fct_ptr = rf24_disable_autoretransmit },

	{	.name = "autoack.pipes",
		.description = "adsadasdasdasdasdas",
		.val_type = STRING,
		.fct_ptr_string = rf24_set_autoack_pipes_string,
		.fct_ptr_ret_string = rf24_get_autoack_pipes_string},*/
};

void rf24_urci_ping(char* str)
{
	/*rf24_mac_addr mac_addr;
	string_to_bytes(str, ".,:;", mac_addr.bytes, 6);

	uint8_t payload[255];
	for(int i=0; i < 255; i++) payload[i] = i;
	rf24_mac_transfer_data(&mac_addr, DATA_DATA, payload, 255);*/

	/*rf24_mac_frame mac_frame;

	mac_frame.receiver = mac_addr;
	mac_frame.frame_control.type = MANAGEMENT;
	mac_frame.frame_control.sub_type = MANAGEMENT_NEIGHBOR_ANSWER_MESSAGE;

	rf24_mac_transfer_frame(&mac_frame, BROADCAST);*/


}

void rf24_urci_print_mac_address()
{
	rf24_printf("%-10s mac-address: %s\n", "mac", decimal_to_string(rf24_mac_get_address()->bytes, 6, ':'));
}

void rf24_urci_print_register(char* str){

	for(int i=0; i<sizeof(rf24_module_register_names); i++)
	{
		if(strcmp(rf24_module_register_names[i], str) == 0)
		{
			// get register_address vom register mnemonic enumeration
			uint8_t register_address = (rf24_module_register_mnemonics) i;

			// read register value and write it to usart
			rf24_stm32f1xx_usart_write_line(decimal_to_binary(rf24_module_read_register(register_address)));
		}
	}
}

/*
void rf24_urci_transfer_nam(char* str)
{
	rf24_mac_addr mac_addr;
	string_to_bytes(str, ".,:;", mac_addr.bytes, 6);

	rf24_mac_frame mac_frame;
	mac_frame.transmitter = *rf24_mac_get_address();
	mac_frame.receiver = mac_addr;
	mac_frame.frame_control.type = TOPOLOGY;
	mac_frame.frame_control.subtype = TOPOLOGY_NEIGHBOR_ANSWER_MESSAGE;

	rf24_mac_transfer_frame(UNICAST, &mac_frame);
}*/

void rf24_urci_transmit_num(char* str)
{
	rf24_mac_addr mac_addr;
	string_to_bytes(str, ".,:;", mac_addr.bytes, 6);

	rf24_mac_frame mac_frame;
	mac_frame.transmitter = *rf24_mac_get_address();
	mac_frame.receiver = mac_addr;
	mac_frame.frame_control.type = TOPOLOGY;
	mac_frame.frame_control.subtype = TOPOLOGY_NEIGHBOR_UPDATE_MESSAGE;

	rf24_module_tx_data tx_data;
	rf24_mac_frame_to_tx_data(&mac_frame, &tx_data);
	rf24_module_transmit(&tx_data);
}

void rf24_urci_transmit_mac_frame(char* str)
{
	rf24_mac_frame mac_frame;

	string_to_bytes(str, ".,:;", mac_frame.receiver.bytes, 6);
	mac_frame.frame_control.type = MANAGEMENT;
	//mac_frame.frame_control.subtype = MANAGEMENT_TEST;

	rf24_mac_transfer_frame(UNICAST, &mac_frame);
}

void rf24_urci_transfer_topology(char* str)
{
	rf24_mac_addr mac_addr;
	string_to_bytes(str, ".,:;", mac_addr.bytes, 6);

	rf24_network_transfer_topology(mac_addr);
}

void rf24_urci_transmit(char* str)
{
	rf24_mac_addr mac_addr;
	string_to_bytes(str, ".,:;", mac_addr.bytes, 6);

	uint8_t payload[255];
	for(int i=0; i < 255; i++) payload[i] = i;
	rf24_mac_transfer_data(UNICAST, &mac_addr, DATA_DATA, payload, 255);
}

void rf24_urci_set_datarate(char* str){
	if(strcmp(str, "250KBPS") == 0) rf24_module_set_datarate(rf24_module_datarate_250kbps);
	if(strcmp(str, "1MBPS") == 0) rf24_module_set_datarate(rf24_module_datarate_1Mbps);
	if(strcmp(str, "2MBPS") == 0) rf24_module_set_datarate(rf24_module_datarate_2Mbps);
}

char* rf24_urci_get_datarate(){

	rf24_module_datarate datarate = rf24_module_get_datarate();
	char* str = "";

	switch(datarate)
	{
		case rf24_module_datarate_250kbps:
			str = "250KBPS";
			return str;
		case rf24_module_datarate_1Mbps:
			str = "1MBPS";
			return str;
		case rf24_module_datarate_2Mbps:
			str = "2MBPS";
			return str;
	}

	return str;
}

void rf24_urci_print_settings(){

	//itoa(rf24_module_get_rf_channel(), urci_buffer_32, 10);
	//sprintf(urci_buffer_128, "%-25s %s",  "", urci_buffer_32);
	//usart_write_line(urci_buffer_128);

	/*rf24_set_rf_outputpower(rf24_rf_output_power3);
	rf24_set_datarate(rf24_datarate_250kbps);
	rf24_enable_rx_pipe(rf24_rx_pipe0);
	rf24_enable_crc();
	rf24_set_crc_length(rf24_crc_length_1byte);
	rf24_set_address_width(rf24_address_width_5bytes);
	rf24_set_payload_width_pipe(rf24_rx_pipe0, 32);
	rf24_set_autoretransmit_delay(rf24_autoretransmit_delay_750us);
	rf24_set_autoretransmit_count(rf24_autoretransmit_count_15);*/
}

void rf24_urci_print_help(void)
{
	for(int i=0; i < sizeof(urci_cmds)/sizeof(urci_cmds[0]); i++)
	{
		if(urci_cmds[i].description){
			sprintf(urci_buffer_128, "%-25s %s",  urci_cmds[i].name, urci_cmds[i].description);
			rf24_stm32f1xx_usart_write_line(urci_buffer_128);
		}

		if(urci_cmds[i].details){
			urci_buffer_128[0] = '\0';
			sprintf(urci_buffer_128, "%-25s %s", "", urci_cmds[i].details);
			rf24_stm32f1xx_usart_write_line(urci_buffer_128);
		}

		rf24_stm32f1xx_usart_write_line("");
	}
}

void rf24_urci_help(char *str)
{
	for(int i=0; i < sizeof(urci_cmds)/sizeof(urci_cmds[0]); i++)
	{
		if(strcmp(urci_cmds[i].name, str) == 0)
		{
			sprintf(urci_buffer_128, "%-25s %s",  urci_cmds[i].name, urci_cmds[i].description);
			rf24_stm32f1xx_usart_write_line(urci_buffer_128);
		}
	}
}

void rf24_urci_print_status(void)
{
	uint8_t status = rf24_module_read_register(STATUS);
	rf24_stm32f1xx_usart_write_line(decimal_to_binary(status));
}

void rf24_urci_print_config(void)
{
	uint8_t config = rf24_module_read_register(CONFIG);
	rf24_stm32f1xx_usart_write_line(decimal_to_binary(config));
}

void rf24_urci_set_crc_length(uint8_t crc_length)
{
	switch(crc_length){
		case 0: rf24_module_disable_crc(); break;
		case 8: rf24_module_set_crc_length(rf24_module_crc_length_1byte); break;
		case 16: rf24_module_set_crc_length(rf24_module_crc_length_2bytes); break;
		default:break;
	}
}

uint8_t rf24_urci_get_crc_length()
{
	if(rf24_module_crc_enabled())
	{
		return ( rf24_module_get_crc_length() + 1) * 8;
	}
	else return 0;
}

void rf24_urci_set_tx_address(char *str)
{
	// read configured address width from chip
	uint8_t address_width = rf24_module_get_address_width();

	// array to hold converted string address into byte array
	uint8_t address[address_width];

	// convert string address into byte address
	string_to_bytes(str, ".,:;", address, address_width);

	// set tx address
	rf24_module_set_tx_address(address, address_width);
}

char* rf24_urci_get_tx_address(void)
{
	// read configured address width from chip
	uint8_t address_width = rf24_module_get_address_width();

	// array to hold converted string address into byte array
	uint8_t address[address_width];

	// read tx address from chip
	rf24_module_get_tx_address(address);

	// convert byte array into string and store it in urci_buffer_32
	bytes_to_string(address, address_width, ':', urci_buffer_32);

	// return pointer to urci_buffer_32
	return urci_buffer_32;
}

void rf24_urci_get_attribute_value(char* attribute_identifier, char* attribute_value){

	int i = 0, k = 0;

	while(urci_buffer_32[attribute_pos + i] != '\0' && ((attribute_pos+i) < sizeof(urci_buffer_32)))
	{
		//usart_write_byte(urci_buffer_32[attribute_pos+i]);

		if(i <= sizeof(attribute_identifier))
		{
			if(urci_buffer_32[attribute_pos + i] != attribute_identifier[i])
			{
				//fuck: no attribute_identifier in input string found
				return;
			}
		}
		else
		{
			attribute_value[k++] = urci_buffer_32[attribute_pos + i];
		}

		i++;
	}

	attribute_value[k] = '\0';
}


void rf24_urci_set_rx_address(char *str)
{
	// read pipe number from input string
	char attribute_value[5];
	rf24_urci_get_attribute_value(".pipe", attribute_value);
	uint8_t rx_pipe = atoi(attribute_value);

	switch(rx_pipe)
	{
		// rx address pipe 0 and pipe 1 are 5 byte registers
		case 0: case 1:
		{
			// read configured address width from chip
			uint8_t address_width = rf24_module_get_address_width();

			// array to hold converted string address into byte array
			uint8_t address[address_width];

			// convert string address into byte address
			string_to_bytes(str, ".,:;", address, address_width);

			// write address
			rf24_module_readwrite_register(write, RX_ADDR_P0 + rx_pipe, address, address_width);
			return;
		}

		// rx address pipe 2 - 5 are 8 bit registers
		case 2: case 3: case 4: case 5:
		{
			uint8_t address = atoi(str);
			rf24_module_write_register(RX_ADDR_P0 + rx_pipe, address);
			return;
		}
	}
}

char* rf24_urci_get_rx_address()
{
	// read pipe number from input string
	char attribute_value[5];
	rf24_urci_get_attribute_value(".pipe", attribute_value);
	uint8_t rx_pipe = atoi(attribute_value);

	// get current address width from chip
	uint8_t address_width = rf24_module_get_address_width();

	// create an array to hold rx address
	uint8_t address[address_width];

	switch(rx_pipe)
	{
		case 0: case 1:
		{
			rf24_module_readwrite_register(read, RX_ADDR_P0 + rx_pipe, address, address_width);
			break;
		}
		case 2: case 3: case 4: case 5:
		{
			rf24_module_readwrite_register(read, RX_ADDR_P1, address, address_width);
			address[address_width-1] = rf24_module_read_register(RX_ADDR_P0 + rx_pipe);
			break;
		}
	}

	// convert address into string and store in urci_buffer_128
	bytes_to_string(address, address_width, ":", urci_buffer_128);

	// return pointer of urci_buffer_128
	return urci_buffer_128;
}

void rf24_urci_set_payload_width(uint8_t payload_size)
{
	char attribute_value[5];

	// read pipe number from input string
	rf24_urci_get_attribute_value(".pipe", attribute_value);
	uint8_t rx_pipe = atoi(attribute_value);

	rf24_module_write_register(RX_PW_P0 + rx_pipe, payload_size);
}

uint8_t rf24_urci_get_payload_width()
{
	char attribute_value[5];

	// read pipe number from input string
	rf24_urci_get_attribute_value(".pipe", attribute_value);
	uint8_t rx_pipe = atoi(attribute_value);

	return rf24_module_read_register(RX_PW_P0 + rx_pipe);
}


// creates commands, has to be called in main function for cmd set initiation of uart remote controll interface
void rf24_urci_init(){
	// init flags
	flags.enable = false;
	flags.start = false;
	flags.separator = false;
	flags.end = false;
	flags.cmd_get = false;
	flags.cmd_set = false;
}

// assigns given command and values through uart to rf24 get and set functions
void rf24_urci_call(urci_cmd cmd){

	uint8_t value;
	char str[50];

	// convert value array as defined type in command struct
	if(flags.cmd_set)
	{
		switch(cmd.set_type)
		{
			case rf24_urci_uint8:
			{
				value = atoi(urci_buffer_128);
				cmd.fct_ptr_set_uint8(value);
				break;
			}
			case rf24_urci_string:
			{
				cmd.fct_ptr_set_string(urci_buffer_128);
				break;
			}
			default: break;
		}
		flags.cmd_set = false;
	}

	if(flags.cmd_get)
	{
		switch(cmd.get_type)
		{
			case rf24_urci_uint8:
			{
				rf24_stm32f1xx_usart_write_str(cmd.name);
				if(flags.attribute){
					rf24_urci_get_attribute_value(".pipe", str);
					rf24_stm32f1xx_usart_write_str(str);
				}
				rf24_stm32f1xx_usart_write_byte(rf24_urci_separator_symbol);
				value = cmd.fct_ptr_get_uint8();
				itoa(value, str, 10);
				rf24_stm32f1xx_usart_write_line(str);
				break;
			}
			case rf24_urci_string:
			{
				rf24_stm32f1xx_usart_write_str(cmd.name);
				if(flags.attribute){
					rf24_urci_get_attribute_value(".pipe", str);
					rf24_stm32f1xx_usart_write_str(str);
				}
				rf24_stm32f1xx_usart_write_byte(rf24_urci_separator_symbol);
				rf24_stm32f1xx_usart_write_line(cmd.fct_ptr_get_string());
				break;
			}
			case rf24_urci_void:
			{
				cmd.fct_ptr();
				rf24_stm32f1xx_usart_write_line("OK");
				break;
			}
		}
		flags.cmd_get = false;
	}
}

void rf24_urci_match(){

	#ifdef DEBUG
		/*usart_write_str("urci_buffer_32: ");
		usart_write_line(urci_buffer_32);
		usart_write_str("urci_buffer_128: ");
		usart_write_line(urci_buffer_128);*/
	#endif

	for(int i=0; i < sizeof(urci_cmds) / sizeof(urci_cmds[0]); i++)
	{
		if(strcmp(urci_buffer_32, urci_cmds[i].name) == 0)
		{
			#ifdef DEBUG
				//usart_write_str("URCI_MATCH: ");
				//usart_write_line(URCI_CMDS[i].name);
			#endif

			rf24_urci_call(urci_cmds[i]);
		}
	}
}

/*
rf24_urci_putc(const char data)

- Takes single chars from uart buffer and writes them into the arrays RF24_URCI_COMMAND and RF24_URCI_VALUE.
- Converts uart inputs of the form "<rfchannel=1>" into RF24_URCI_COMMAND = "rfchannel", RF24_URCI_VALUE = "1".
- Command name is stored in buffer RF24_URCI_COMMAND, command value is stored in RF24_URCI_VALUE
*/
void rf24_urci_putc(char data){

	switch(data){
		// data is start symbol: '<'
		case rf24_urci_start_symbol:
			flags.start = true;												// remember start symbol detected to fill command array
			break;

		// data is separator symbol: '='
		case rf24_urci_separator_symbol:
			flags.start = false;											// remember end of command array, beginning of value array
			flags.separator = true;											// remember separator found
			urci_buffer_32[counter] = '\0';									// null terminate command array
			counter = 0;													// reset counter
			break;

		// data is termination symbol, reset flags and process command+value to a generic function: '>'
		case rf24_urci_end_symbol:
			if(flags.separator) flags.cmd_set = true;						// if a separator symbol is found, command is a SET type (write into rf-module register)
			else flags.cmd_get = true;										// if no separator detected, command is a GET type (read from rf-module register)

			if(!flags.separator) urci_buffer_32[counter] = '\0';			// null terminate command array in case no separator was found
			urci_buffer_128[counter] = '\0';								// null terminate value array

			flags.enable = false;											// stop reading chars into uart remote control routine (prevents calling rf24_urci_putc)
			flags.start = false;											// reset start flag
			flags.separator = false;										// reset end flag
			flags.attribute = false;
			counter = 0;													// reset counter

			rf24_urci_match();												// call rf24_urci_match which fetches the assigned rf24 function to the input command
			break;

		// data is either part of command name or its value, proceed filling arrays
		default:
			if(flags.start) urci_buffer_32[counter] = data;					// fill command buffer
			if(flags.separator) urci_buffer_128[counter] = data;			// fill value buffer
			if(data == rf24_urci_attribute_symbol){
				flags.attribute = true;
				attribute_pos = counter;
			}
			counter++;														// increment counter
			break;
	}
}

// HELPER FUNCTIONS

uint8_t string_to_bytes(char *str_in, char* delimiters, uint8_t *array_out, uint8_t array_out_length){

	int count = 0;

	char *ptr = strtok(str_in, delimiters);

	while(ptr != NULL /*&& count<(array_out_length)*/) {
		array_out[count++] = atoi(ptr);
		ptr = strtok(NULL, delimiters);
	}

	return true;
}

void bytes_to_string(uint8_t *address_in, uint8_t address_length, const char delimiter, char *str_out){

	bool first = true;

	for(int i=0; i < address_length; i++){

		char ascii_digits[4];
		itoa(address_in[i], ascii_digits, 10);

		if(first){
			strcpy(str_out, ascii_digits);
			first = false;
		}
		else{
			strcat(str_out, ":");
			strcat(str_out, ascii_digits);
		}
	}
}


