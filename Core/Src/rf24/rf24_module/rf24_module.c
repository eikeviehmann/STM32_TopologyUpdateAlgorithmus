#include "rf24_module.h"

#include "nRF24L01.h"
#include "../rf24_stm32f1xx/rf24_stm32f1xx_uart.h"
#include "../rf24_stm32f1xx/rf24_stm32f1xx_spi.h"
#include "../rf24_debug.h"


//_________________________________________________________________________________________________________________________________________________________________________
// global variables

volatile rf24_module_states rf24_module_state = UNDEFINED;
volatile rf24_module_flags_t rf24_module_flags;

rf24_module_attribute_value value_in;
rf24_module_attribute_value value_out;

//_________________________________________________________________________________________________________________________________________________________________________
// rf24 attributes

/* (0) CONFIG REGISTER (ADDRESS 0) */

rf24_module_attribute prim_rx = { .type = w1_rw, .reg_addr = CONFIG, .mask = 0b11111110, .mnemonic = PRIM_RX /*0*/, .w1= { .min = false, .max = true } };

rf24_module_attribute pwr_up = { .type = w1_rw, .reg_addr = CONFIG, .mask = 0b11111101, .mnemonic = PWR_UP /*1*/, .w1= { .min = false, .max = true } };

rf24_module_attribute crc0 = { .type = w1_rw, .reg_addr = CONFIG, .mask = 0b11111011, .mnemonic = CRCO /*2*/, .w1= { .min = false, .max = true } };

rf24_module_attribute en_crc = { .type = w1_rw, .reg_addr = CONFIG, .mask = 0b11110111, .mnemonic = EN_CRC /*3*/, .w1= { .min = false, .max = true } };

rf24_module_attribute mask_max_rt = { .type = w1_rw, .reg_addr = CONFIG, .mask = 0b11101111, .mnemonic = MASK_MAX_RT /*4*/, .w1= { .min = false, .max = true } };

rf24_module_attribute mask_tx_ds = { .type = w1_rw, .reg_addr = CONFIG, .mask = 0b11011111, .mnemonic = MASK_TX_DS /*5*/, .w1= { .min = false, .max = true } };

rf24_module_attribute mask_rx_dr = { .type = w1_rw, .reg_addr = CONFIG, .mask = 0b10111111, .mnemonic = MASK_RX_DR /*6*/, .w1= { .min = false, .max = true } };

/* (1) EN_AA REGISTER (ADDRESS 1) */

rf24_module_attribute enaa_p0 = { .type = w1_rw, .reg_addr = EN_AA, .mask = 0b11111110, .mnemonic = ENAA_P0 /*0*/, .w1= { .min = false, .max = true } };

rf24_module_attribute enaa_p1 = { .type = w1_rw, .reg_addr = EN_AA, .mask = 0b11111101, .mnemonic = ENAA_P1 /*1*/, .w1= { .min = false, .max = true } };

rf24_module_attribute enaa_p2 = { .type = w1_rw, .reg_addr = EN_AA, .mask = 0b11111011, .mnemonic = ENAA_P2 /*2*/, .w1= { .min = false, .max = true } };

rf24_module_attribute enaa_p3 = { .type = w1_rw, .reg_addr = EN_AA, .mask = 0b11110111, .mnemonic = ENAA_P3 /*3*/, .w1= { .min = false, .max = true } };

rf24_module_attribute enaa_p4 = { .type = w1_rw, .reg_addr = EN_AA, .mask = 0b11101111, .mnemonic = ENAA_P4 /*4*/, .w1= { .min = false, .max = true } };

rf24_module_attribute enaa_p5 = { .type = w1_rw, .reg_addr = EN_AA, .mask = 0b11011111, .mnemonic = ENAA_P5 /*5*/, .w1= { .min = false, .max = true } };

/* (2) EN_RXADDR REGISTER (ADDRES 2) */

rf24_module_attribute erx_p0 = { .type = w1_rw, .reg_addr = EN_RXADDR, .mask = 0b11111110, .mnemonic = ERX_P0 /*0*/, .w1= { .min = false, .max = true } };

rf24_module_attribute erx_p1 = { .type = w1_rw, .reg_addr = EN_RXADDR, .mask = 0b11111101, .mnemonic = ERX_P1 /*1*/, .w1= { .min = false, .max = true } };

rf24_module_attribute erx_p2 = { .type = w1_rw, .reg_addr = EN_RXADDR, .mask = 0b11111011, .mnemonic = ERX_P2 /*2*/, .w1= { .min = false, .max = true } };

rf24_module_attribute erx_p3 = { .type = w1_rw, .reg_addr = EN_RXADDR, .mask = 0b11110111, .mnemonic = ERX_P3 /*3*/, .w1= { .min = false, .max = true } };

rf24_module_attribute erx_p4 = { .type = w1_rw, .reg_addr = EN_RXADDR, .mask = 0b11101111, .mnemonic = ERX_P4 /*4*/, .w1= { .min = false, .max = true } };

rf24_module_attribute erx_p5 = { .type = w1_rw, .reg_addr = EN_RXADDR, .mask = 0b11011111, .mnemonic = ERX_P5 /*5*/, .w1= { .min = false, .max = true } };

/* (3) SETUP_AW REGISTER (ADDRESS 3) */

rf24_module_attribute aw = { .type = w8_rw, .reg_addr = SETUP_AW, .mask = 0b11111100, .mnemonic = 0, .w8 = {.min = 1, .max = 3} };

/* (4) SETUP_RETR REGISTER (ADDRESS 4) */

rf24_module_attribute arc = { .type = w8_rw, .reg_addr = SETUP_RETR, .mask = 0b11110000, .mnemonic = ARC /*0*/, .w8 = {.min = 0, .max = 15} };

rf24_module_attribute ard = { .type = w8_rw, .reg_addr = SETUP_RETR, .mask = 0b00001111, .mnemonic = ARD /*4*/, .w8 = {.min = 0, .max = 15} };

/* (5) RF_CH REGISTER (ADDRESS 5) */

rf24_module_attribute rf_ch = { .type = w8_rw, .reg_addr = RF_CH, .mask = 0b10000000, .mnemonic = ARC /*0*/, .w8 = {.min = 0, .max = 127} };

/* (6) RF_SETUP REGISTER (ADDRESS 6) */

rf24_module_attribute rf_pwr = { .type = w8_rw, .reg_addr = RF_SETUP, .mask = 0b11111001, .mnemonic = RF_PWR /*0*/, .w8 = {.min = 0, .max = 3} };

rf24_module_attribute rf_dr_high = { .type = w1_rw, .reg_addr = RF_SETUP, .mask = 0b11110111, .mnemonic = RF_DR_HIGH /*3*/, .w1 = {.min = false, .max = true} };

rf24_module_attribute pll_lock = { .type = w1_rw, .reg_addr = RF_SETUP, .mask = 0b11101111, .mnemonic = PLL_LOCK /*4*/, .w1 = {.min = false, .max = true} };

rf24_module_attribute rf_dr_low = { .type = w1_rw, .reg_addr = RF_SETUP, .mask = 0b11011111, .mnemonic = RF_DR_LOW /*5*/, .w1 = {.min = false, .max = true} };

rf24_module_attribute cont_wave = { .type = w1_rw, .reg_addr = RF_SETUP, .mask = 0b01111111, .mnemonic = /*CONT_WAVE*/ 7, .w1 = {.min = false, .max = true} };

/* (7) STATUS REGISTER (ADDRESS 7) */

rf24_module_attribute tx_full = { .type = w1_rw, .reg_addr = STATUS, .mask = 0b11111110, .mnemonic = /*TX_FULL*/0, .w1 = {.min = false, .max = true} };

rf24_module_attribute rx_p_no = { .type = w8_rw, .reg_addr = STATUS, .mask = 0b11110001, .mnemonic = /*RX_P_NO*/1, .w8 = {.min = 0, .max = 5} };

rf24_module_attribute max_rt = { .type = w1_rw, .reg_addr = STATUS, .mask = 0b11101111, .mnemonic = /*MAX_RT*/ 4, .w1 = {.min = false, .max = true} };

rf24_module_attribute tx_ds = { .type = w1_rw, .reg_addr = STATUS, .mask = 0b11011111, .mnemonic = /*TX_DS*/5, .w1 = {.min = false, .max = true} };

rf24_module_attribute rx_dr = { .type = w1_rw, .reg_addr = STATUS, .mask = 0b10111111, .mnemonic = /*RX_DR*/6, .w1 = {.min = false, .max = true} };

/* (8) OBSERVE_TX REGISTER (ADDRESS 8) */

rf24_module_attribute arc_cnt = { .type = w8_rw, .reg_addr = /*OBSERVE_TX*/ 8, .mask = 0b11110000, .mnemonic = /*ARC_CNT*/0, .w8 = { .min = 0, .max = 15} };

rf24_module_attribute plos_cnt = { .type = w8_rw, .reg_addr = /*OBSERVE_TX*/ 8, .mask = 0b00001111, .mnemonic = /*PLOS_CNT*/4, .w8 = { .min = 0, .max = 15} };

/* (9) RPD REGISTER (ADDRESS 9) */

rf24_module_attribute rpd = { .type = w1_rw, .reg_addr = RPD, .mask = 0b11111110, .mnemonic = /*RPD*/0, .w1 = {.min = false, .max = true} };

/* (10) RX_ADDR_P0 REGISTER (ADDRESS 0A) */

rf24_module_attribute rx_addr_p0 = { .type = w40_rw, .reg_addr = RX_ADDR_P0, .w40 = { .max_length = 5 } };

/* (11) RX_ADDR_P1 REGISTER (ADDRESS 0B) */

rf24_module_attribute rx_addr_p1 = { .type = w40_rw, .reg_addr = RX_ADDR_P1, .w40 = { .max_length = 5 } };

/* (12) RX_ADDR_P2 REGISTER (ADDRESS 0C) */

rf24_module_attribute rx_addr_p2 = { .type = w8_rw, .reg_addr = RX_ADDR_P2, .mask = 0b00000000, .mnemonic = 0, .w8 = { .min = 0, .max = 255} };

/* (13) RX_ADDR_P3 REGISTER (ADDRESS 0D) */

rf24_module_attribute rx_addr_p3 = { .type = w8_rw, .reg_addr = RX_ADDR_P3, .mask = 0b00000000, .mnemonic = 0, .w8 = { .min = 0, .max = 255} };

/* (14) RX_ADDR_P4 REGISTER (ADDRESS 0E) */

rf24_module_attribute rx_addr_p4 = { .type = w8_rw, .reg_addr = RX_ADDR_P4, .mask = 0b00000000, .mnemonic = 0, .w8 = { .min = 0, .max = 255} };

/* (15) RX_ADDR_P5 REGISTER (ADDRESS 0F) */

rf24_module_attribute rx_addr_p5 = { .type = w8_rw, .reg_addr = RX_ADDR_P5, .mask = 0b00000000, .mnemonic = 0, .w8 = { .min = 0, .max = 255} };

/* (16) TX_ADDR REGISTER (ADDRESS 10) */

rf24_module_attribute tx_addr = { .type = w40_rw, .reg_addr = TX_ADDR, .w40 = { .max_length = 5 } };

/* (17) RX_PW_P0 REGISTER (ADDRESS 11) */

rf24_module_attribute rx_pw_p0 = { .type = w8_rw, .reg_addr = RX_PW_P0, .mask = 0b11000000, .mnemonic = 0, .w8 = { .min = 0, .max = 32} };

/* (18) RX_PW_P1 REGISTER (ADDRESS 12) */

rf24_module_attribute rx_pw_p1 = { .type = w8_rw, .reg_addr = RX_PW_P1, .mask = 0b11000000, .mnemonic = 0, .w8 = { .min = 0, .max = 32} };

/* (19) RX_PW_P2 REGISTER (ADDRESS 13) */

rf24_module_attribute rx_pw_p2 = { .type = w8_rw, .reg_addr = RX_PW_P2, .mask = 0b11000000, .mnemonic = 0, .w8 = { .min = 0, .max = 32} };

/* (20) RX_PW_P3 REGISTER (ADDRESS 14) */

rf24_module_attribute rx_pw_p3 = { .type = w8_rw, .reg_addr = RX_PW_P3, .mask = 0b11000000, .mnemonic = 0, .w8 = { .min = 0, .max = 32} };

/* (21) RX_PW_P4 REGISTER (ADDRESS 15) */

rf24_module_attribute rx_pw_p4 = { .type = w8_rw, .reg_addr = RX_PW_P4, .mask = 0b11000000, .mnemonic = 0, .w8 = { .min = 0, .max = 32} };

/* (22) RX_PW_P5 REGISTER (ADDRESS 16) */

rf24_module_attribute rx_pw_p5 = { .type = w8_rw, .reg_addr = RX_PW_P5, .mask = 0b11000000, .mnemonic = 0, .w8 = { .min = 0, .max = 32} };

/* (23) FIFO_STATUS REGISTER (ADDRESS 17) */

rf24_module_attribute rx_empty = { .type = w1_r, .reg_addr = FIFO_STATUS, .mask = 0b11111110, .mnemonic = /*RX_EMPTY*/0, .w1 = { .min = false, .max = true} };

rf24_module_attribute rx_full = { .type = w1_r, .reg_addr = FIFO_STATUS, .mask = 0b11111101, .mnemonic = /*RX_FULL*/1, .w1 = { .min = false, .max = true} };

rf24_module_attribute tx_empty = { .type = w1_r, .reg_addr = FIFO_STATUS, .mask = 0b11101111, .mnemonic = /*TX_EMPTY*/4, .w1 = { .min = false, .max = true} };

rf24_module_attribute tx_reuse = { .type = w1_r, .reg_addr = FIFO_STATUS, .mask = 0b10111111, .mnemonic = /*TX_REUSE*/6, .w1 = { .min = false, .max = true} };

/* (24) DYNPD REGISTER (ADDRESS 1C) */

rf24_module_attribute dpl_p0 = { .type = w1_rw, .reg_addr = /*DYNPD*/0x1C, .mask = 0b11111110, .mnemonic = /*DPL_P0*/0, .w1 = { .min = false, .max = true} };

rf24_module_attribute dpl_p1 = { .type = w1_rw, .reg_addr = /*DYNPD*/0x1C, .mask = 0b11111101, .mnemonic = /*DPL_P1*/1, .w1 = { .min = false, .max = true} };

rf24_module_attribute dpl_p2 = { .type = w1_rw, .reg_addr = /*DYNPD*/0x1C, .mask = 0b11111011, .mnemonic = /*DPL_P2*/2, .w1 = { .min = false, .max = true} };

rf24_module_attribute dpl_p3 = { .type = w1_rw, .reg_addr = /*DYNPD*/0x1C, .mask = 0b11110111, .mnemonic = /*DPL_P3*/3, .w1 = { .min = false, .max = true} };

rf24_module_attribute dpl_p4 = { .type = w1_rw, .reg_addr = /*DYNPD*/0x1C, .mask = 0b11101111, .mnemonic = /*DPL_P4*/4, .w1 = { .min = false, .max = true} };

rf24_module_attribute dpl_p5 = { .type = w1_rw, .reg_addr = /*DYNPD*/0x1C, .mask = 0b11011111, .mnemonic = /*DPL_P5*/5, .w1 = { .min = false, .max = true} };


/* (25) FEATURE REGISTER (ADDRESS 1D) */

rf24_module_attribute en_dyn_ack = { .type = w1_rw, .reg_addr = /*FEATRUE*/0x1D, .mask = 0b11111110, .mnemonic = /*EN_DYN_ACK*/0, .w1 = { .min = false, .max = true} };

rf24_module_attribute en_ack_pay = { .type = w1_rw, .reg_addr = /*FEATRUE*/0x1D, .mask = 0b11111101, .mnemonic = /*EN_ACK_PAY*/1, .w1 = { .min = false, .max = true} };

rf24_module_attribute en_dpl = { .type = w1_r, .reg_addr = /*FEATRUE*/0x1D, .mask = 0b11111011, .mnemonic = /*EN_DPL*/2, .w1 = { .min = false, .max = true} };

/* ATTRBIUTE REGISTER STRUCT*/

rf24_module_attributes_struct rf24_module_attributes = {
	.prim_rx = &prim_rx,
	.pwr_up = &pwr_up,
	.crc0 = &crc0,
	.en_crc = &en_crc,
	.mask_max_rt =&mask_max_rt,
	.mask_tx_ds =& mask_tx_ds,
	.mask_rx_dr =& mask_rx_dr,
	.enaa_p0 = &enaa_p0,
	.enaa_p1 = &enaa_p1,
	.enaa_p2 = &enaa_p2,
	.enaa_p3 = &enaa_p3,
	.enaa_p4 = &enaa_p4,
	.enaa_p5 = &enaa_p5,
	.erx_p0 = &erx_p0,
	.erx_p1 = &erx_p1,
	.erx_p2 = &erx_p2,
	.erx_p3 = &erx_p3,
	.erx_p4 = &erx_p4,
	.erx_p5 = &erx_p5,
	.aw = &aw,
	.arc = &arc,
	.ard = &ard,
	.rf_ch = &rf_ch,
	.rf_pwr = &rf_pwr,
	.rf_dr_high = &rf_dr_high,
	.pll_lock = &pll_lock,
	.rf_dr_low = &rf_dr_low,
	.cont_wave = &cont_wave,
	.tx_full = &tx_full,
	.rx_p_no = &rx_p_no,
	.max_rt = &max_rt,
	.tx_ds = &tx_ds,
	.rx_dr = &rx_dr,
	.arc_cnt = &arc_cnt,
	.plos_cnt = &plos_cnt,
	.rpd = &rpd,
	.rx_addr_p0 = &rx_addr_p0,
	.rx_addr_p1 = &rx_addr_p1,
	.rx_addr_p2 = &rx_addr_p2,
	.rx_addr_p2 = &rx_addr_p3,
	.rx_addr_p3 = &rx_addr_p4,
	.rx_addr_p5 = &rx_addr_p5,
	.tx_addr = &tx_addr,
	.rx_pw_p0 = &rx_pw_p0,
	.rx_pw_p1 = &rx_pw_p1,
	.rx_pw_p2 = &rx_pw_p2,
	.rx_pw_p3 = &rx_pw_p3,
	.rx_pw_p4= &rx_pw_p4,
	.rx_pw_p5 = &rx_pw_p5,
	.rx_empty = &rx_empty,
	.rx_full = &rx_full,
	.tx_reuse = &tx_reuse,
	.dpl_p0 = &dpl_p0,
	.dpl_p1 = &dpl_p1,
	.dpl_p2 = &dpl_p2,
	.dpl_p3 = &dpl_p3,
	.dpl_p4 = &dpl_p4,
	.dpl_p5 = &dpl_p5,
	.en_dyn_ack = &en_dyn_ack,
	.en_ack_pay = &en_ack_pay,
	.en_dpl = &en_dpl
};

//_________________________________________________________________________________________________________________________________________________________________________
//INTERRUPT SERVICE ROUTINES


//_________________________________________________________________________________________________________________________________________________________________________
// FUNCTIONS

// REGISTER ACCESS

uint8_t rf24_module_read_register(uint8_t register_name)
{
	rf24_stm32f1xx_spi_csn_low();

	rf24_stm32f1xx_spi_shift_byte(R_REGISTER | (REGISTER_MASK & register_name));

	uint8_t value = rf24_stm32f1xx_spi_shift_byte(NOP);

	rf24_stm32f1xx_spi_csn_high();

	return value;
}

uint8_t rf24_module_write_register(uint8_t register_name, uint8_t data)
{
	rf24_stm32f1xx_spi_csn_low();

	rf24_stm32f1xx_spi_shift_byte(W_REGISTER | (REGISTER_MASK & register_name));

	uint8_t status = rf24_stm32f1xx_spi_shift_byte(data);

	rf24_stm32f1xx_spi_csn_high();

	return status;
}

void rf24_module_readwrite_register(rf24_module_read_write read_write, uint8_t register_name, uint8_t *data_inout, uint8_t length)
{
	if (read_write == write && register_name != W_TX_PAYLOAD && register_name != W_TX_PAYLOAD_NO_ACK)
	{
		register_name = W_REGISTER + register_name;
	}

	rf24_stm32f1xx_spi_csn_low();

	rf24_stm32f1xx_spi_shift_byte(register_name);

	for(int i=0; i<length; i++)
	{
		if(read_write == read)
		{
			data_inout[i] = rf24_stm32f1xx_spi_shift_byte(NOP);
		}
		if(read_write == write)
		{
			rf24_stm32f1xx_spi_shift_byte(data_inout[i]);
		}
	}

	rf24_stm32f1xx_spi_csn_high();
}

// FUNCTON ACCESS

bool rf24_module_write_attribute(rf24_module_attribute *attribute, rf24_module_attribute_value *value_in)
{
	switch(attribute->type)
	{
		/* w1: set/clear bit in register */
		case w1_rw:
		{
			if(value_in->w1 >= attribute->w1.min && value_in->w1 <= attribute->w1.max)
			{
				uint8_t rf24_module_register = rf24_module_read_register(attribute->reg_addr) & attribute->mask;
				rf24_module_register |= ( (value_in->w1) << attribute->mnemonic );
				rf24_module_write_register(attribute->reg_addr, rf24_module_register);
				return true;
			}
			else
			{
				#ifdef DEBUG
					rf24_stm32f1xx_usart_write_line("[i] input of wrong value or wrong format!");
				#endif
				return false;
			}
			return true;
		}

		/* w8: set/clear bits of register */
		case w8_rw:
		{
			if(value_in->w8 >= attribute->w8.min && value_in->w8 <= attribute->w8.max)
			{
				uint8_t rf24_module_register = rf24_module_read_register(attribute->reg_addr) & attribute->mask;
				rf24_module_register |= (value_in->w8) << attribute->mnemonic;
				rf24_module_write_register(attribute->reg_addr, rf24_module_register);
				return true;
			}
			else
			{
				#ifdef DEBUG
					rf24_stm32f1xx_usart_write_line("[i] input of wrong value or wrong format!");
				#endif
				return false;
			}
		}

		/* w40: write into register w40 (5x8byte)*/
		case w40_rw:
		{
			rf24_module_readwrite_register(write, attribute->reg_addr, value_in->w40, 5);
			return true;
		}
		default:break;
	}

	return false;
}

bool rf24_module_read_attribute(rf24_module_attribute *attribute, rf24_module_attribute_value *value_out)
{
	switch(attribute->type){

		case w1_r:
		case w1_rw:{
			uint8_t reg = rf24_module_read_register(attribute->reg_addr) & ~(attribute->mask);
			uint8_t reg_value = reg >> attribute->mnemonic;
			value_out->w1 = reg_value;
			return true;
		}

		case w8_r:
		case w8_rw:
		{
			uint8_t reg = rf24_module_read_register(attribute->reg_addr) & ~(attribute->mask);
			uint8_t reg_value = reg >> attribute->mnemonic;
			value_out->w8 = reg_value;
			return true;

			//usart_write_line(decimal_to_binary(rf24_module_register_value));
		}

		case w40_r:
		case w40_rw:
		{
			rf24_module_readwrite_register(read, attribute->reg_addr, value_out->w40, 5);
			return true;
		}
	}

	return false;
}

rf24_module_attribute_value* rf24_module_value_w1(bool value){
	value_in.w1 = value;
	return &value_in;
}

rf24_module_attribute_value* rf24_module_value_w8(uint8_t value){
	value_in.w8 = value;
	return &value_in;
}

rf24_module_attribute_value* rf24_module_value_w40(uint8_t *value, uint8_t value_length){
	value_in.w40 = value;
	value_in.length = value_length;
	return &value_in;
}

// NRF24L01 INTERNAL FUNCTIONS

void rf24_module_set_ce(rf24_stm32f1xx_pin_state ce_state)
{
	switch(ce_state)
	{
		case high: rf24_stm32f1xx_ce_high(); break;
		case low: rf24_stm32f1xx_ce_low(); break;
	}
}

// NRF24L01 USER EXPORT FUNCTIONS

// REGISTER ACCESS FUNCTIONS

void rf24_module_set_prim_rx(rf24_stm32f1xx_pin_state prim_rx_state)
{
	switch(prim_rx_state)
	{
		case high:
			rf24_module_write_attribute(rf24_module_attributes.prim_rx, rf24_module_value_w1(true));
			break;
		case low:
			rf24_module_write_attribute(rf24_module_attributes.prim_rx, rf24_module_value_w1(false));
			break;
	}
}

void rf24_module_power_up()
{
	rf24_module_write_attribute(rf24_module_attributes.pwr_up, rf24_module_value_w1(true));
}

void rf24_module_power_down()
{
	rf24_module_write_attribute(rf24_module_attributes.pwr_up, rf24_module_value_w1(false));
}

void rf24_module_enable_crc()
{
	rf24_module_write_attribute(rf24_module_attributes.en_crc, rf24_module_value_w1(true));
}

void rf24_module_disable_crc()
{
	rf24_module_write_attribute(rf24_module_attributes.en_crc, rf24_module_value_w1(false));
}

bool rf24_module_crc_enabled()
{
	rf24_module_read_attribute(rf24_module_attributes.en_crc, &value_out);
	return value_out.w1;
}

void rf24_module_set_crc_length(rf24_module_crc_length crc_length)
{
	rf24_module_write_attribute(rf24_module_attributes.crc0, rf24_module_value_w1(crc_length));
}

rf24_module_crc_length rf24_module_get_crc_length()
{
	rf24_module_read_attribute(rf24_module_attributes.crc0, &value_out);
	return (rf24_module_crc_length) value_out.w1;
}

void rf24_module_enable_max_retries_interrupt()
{
	rf24_module_write_attribute(rf24_module_attributes.mask_max_rt, rf24_module_value_w1(false));
}

void rf24_module_disable_max_retries_interrupt()
{
	rf24_module_write_attribute(rf24_module_attributes.mask_max_rt, rf24_module_value_w1(true));
}

void rf24_module_enable_tx_data_sent_interrupt()
{
	rf24_module_write_attribute(rf24_module_attributes.mask_tx_ds, rf24_module_value_w1(false));
}

void rf24_module_disable_tx_data_sent_interrupt()
{
	rf24_module_write_attribute(rf24_module_attributes.mask_tx_ds, rf24_module_value_w1(true));
}

void rf24_module_enable_rx_data_read_interrupt()
{
	rf24_module_write_attribute(rf24_module_attributes.mask_rx_dr, rf24_module_value_w1(false));
}

void rf24_module_disable_rx_data_read_interrupt()
{
	rf24_module_write_attribute(rf24_module_attributes.mask_rx_dr, rf24_module_value_w1(true));
}

void rf24_module_enable_autoack_pipe(rf24_module_rx_pipes rx_pipes)
{
	uint8_t en_aa = rf24_module_read_register(EN_AA);

	if(!(en_aa & (1<<rx_pipes))){
		rf24_module_write_register(EN_AA, rf24_module_read_register(EN_AA) | (1<<rx_pipes));
	}
}

void rf24_module_disable_autoack_pipe(rf24_module_rx_pipes rx_pipes)
{
	uint8_t en_aa = rf24_module_read_register(EN_AA);

	if(en_aa & (1<<rx_pipes)){
		rf24_module_write_register(EN_AA, rf24_module_read_register(EN_AA) | (1<<rx_pipes));
	}
}

void rf24_module_enable_rx_pipe(rf24_module_rx_pipes rx_pipes)
{
	rf24_module_write_register(EN_RXADDR, rf24_module_read_register(EN_RXADDR) | (1<<rx_pipes));
}

void rf24_module_set_address_width(rf24_module_address_width address_width)
{
	rf24_module_write_attribute(rf24_module_attributes.aw, rf24_module_value_w8(address_width - rf24_module_address_width_offset));
}

rf24_module_address_width rf24_module_get_address_width()
{
	rf24_module_read_attribute(rf24_module_attributes.aw, &value_out);
	return (rf24_module_address_width) value_out.w8 + rf24_module_address_width_offset;
}

void rf24_module_set_autoretransmit_delay(rf24_module_autoretransmit_delay autoretransmit_delay)
{
	rf24_module_write_attribute(rf24_module_attributes.ard, rf24_module_value_w8(autoretransmit_delay));
}

rf24_module_autoretransmit_delay rf24_module_get_autoretransmit_delay()
{
	rf24_module_read_attribute(rf24_module_attributes.ard, &value_out);
	return (rf24_module_autoretransmit_delay) value_out.w8;
}

void rf24_module_set_autoretransmit_count(rf24_module_autoretransmit_count autoretransmit_count)
{
	rf24_module_write_attribute(rf24_module_attributes.arc, rf24_module_value_w8(autoretransmit_count));
}

rf24_module_autoretransmit_count rf24_module_get_autoretransmit_count()
{
	rf24_module_read_attribute(rf24_module_attributes.arc, &value_out);
	return (rf24_module_autoretransmit_count) value_out.w8;
}

void rf24_module_set_rf_channel(uint8_t rfchannel)
{
	rf24_module_write_attribute(rf24_module_attributes.rf_ch, rf24_module_value_w8(rfchannel));
}

uint8_t rf24_module_get_rf_channel()
{
	rf24_module_read_attribute(rf24_module_attributes.rf_ch, &value_out);
	return value_out.w8;
}

void rf24_module_set_rf_outputpower(rf24_module_rf_output_power rf_output_power)
{
	rf24_module_write_attribute(rf24_module_attributes.rf_pwr, rf24_module_value_w8(rf_output_power));
}

rf24_module_rf_output_power rf24_module_get_rf_outputpower()
{
	rf24_module_read_attribute(rf24_module_attributes.rf_pwr, &value_out);
	return (rf24_module_rf_output_power) value_out.w8;
}

void rf24_module_set_datarate(rf24_module_datarate datarate)
{
	switch(datarate)
	{
		case rf24_module_datarate_250kbps:
			rf24_module_write_attribute(rf24_module_attributes.rf_dr_low, rf24_module_value_w1(true));
			break;
		case rf24_module_datarate_1Mbps:
		case rf24_module_datarate_2Mbps:
			rf24_module_write_attribute(rf24_module_attributes.rf_dr_low, rf24_module_value_w1(false));
			rf24_module_write_attribute(rf24_module_attributes.rf_dr_high, rf24_module_value_w1(datarate));
			break;
	}
}

rf24_module_datarate rf24_module_get_datarate()
{
	rf24_module_read_attribute(rf24_module_attributes.rf_dr_low, &value_out);
	bool rf_dr_low = value_out.w1;

	if(rf_dr_low) return rf24_module_datarate_250kbps;

	rf24_module_read_attribute(rf24_module_attributes.rf_dr_high, &value_out);
	bool rf_dr_high = value_out.w1;

	return (rf24_module_datarate) rf_dr_high;
}

void rf24_module_set_cont_wave(rf24_stm32f1xx_pin_state bit_state)
{
	rf24_module_write_attribute(rf24_module_attributes.cont_wave, rf24_module_value_w1(bit_state));
}

// STAUTS REGISTER FUNCTIONS

bool rf24_module_tx_fifo_full()
{
	rf24_module_read_attribute(rf24_module_attributes.tx_full, &value_out);
	return (value_out.w1 == 1);
}

rf24_module_rx_pipes rf24_module_get_rx_available_pipe()
{
	rf24_module_read_attribute(rf24_module_attributes.rx_p_no, &value_out);
	return (rf24_module_rx_pipes) value_out.w8;
}

bool rf24_module_max_retransmits()
{
	rf24_module_read_attribute(rf24_module_attributes.max_rt, &value_out);
	return (value_out.w1 == 1);
}

void rf24_module_clear_max_retransmit_interrupt()
{
	rf24_module_read_attribute(rf24_module_attributes.max_rt, rf24_module_value_w1(true));
}

bool rf24_module_package_transmitted()
{
	rf24_module_read_attribute(rf24_module_attributes.tx_ds, &value_out);
	return (value_out.w1 == 1);
}

void rf24_module_clear_package_transmitted_interrupt()
{
	//rf24_module_read_attribute(rf24_module_attributes.tx_ds, rf24_module_value_w1(true));
	uint8_t status = rf24_module_read_register(STATUS);
	rf24_module_write_register(STATUS, status & (1<<TX_DS) );
}

bool rf24_module_rx_data_available()
{
	rf24_module_read_attribute(rf24_module_attributes.rx_dr, &value_out);
	return (value_out.w1 == 1);
}

void rf24_module_clear_rx_data_interrupt()
{
	//rf24_module_read_attribute(rf24_module_attributes.rx_dr, rf24_module_value_w1(true));
	uint8_t status = rf24_module_read_register(STATUS);
	rf24_module_write_register(STATUS, status & (1<<RX_DR) );
}

void rf24_module_clear_all_interrupts()
{
	uint8_t status = rf24_module_read_register(STATUS);
	rf24_module_write_register(STATUS, status & ( (1<<MAX_RT) | (1<<TX_DS) | (1<<RX_DR) ) );
}

uint8_t rf24_module_get_retransmitted_count(){
	rf24_module_read_attribute(rf24_module_attributes.arc, &value_out);
	return value_out.w8;
}

uint8_t rf24_module_get_packagelost_count(){
	rf24_module_read_attribute(rf24_module_attributes.plos_cnt, &value_out);
	return value_out.w8;
}

uint8_t rf24_module_get_received_power_detector(){
	return 0;
}

void rf24_module_set_rx_address_pipe0(uint8_t *rx_address /*3-5 bytes*/, uint8_t rx_address_length){
	rf24_module_write_attribute(rf24_module_attributes.rx_addr_p0, rf24_module_value_w40(rx_address, rx_address_length));
}

void rf24_module_set_rx_address_pipe1(uint8_t *rx_address /*3-5 bytes*/, uint8_t rx_address_length){
	rf24_module_write_attribute(rf24_module_attributes.rx_addr_p1, rf24_module_value_w40(rx_address, rx_address_length));
}

void rf24_module_set_rx_address_pipe2(uint8_t rx_address /*1 byte*/){
	rf24_module_write_attribute(rf24_module_attributes.rx_addr_p2, rf24_module_value_w8(rx_address));
}

void rf24_module_set_rx_address_pipe3(uint8_t rx_address /*1 byte*/){
	rf24_module_write_attribute(rf24_module_attributes.rx_addr_p3, rf24_module_value_w8(rx_address));
}

void rf24_module_set_rx_address_pipe4(uint8_t rx_address /*1 byte*/){
	rf24_module_write_attribute(rf24_module_attributes.rx_addr_p4, rf24_module_value_w8(rx_address));
}

void rf24_module_set_rx_address_pipe5(uint8_t rx_address /*1 byte*/){
	rf24_module_write_attribute(rf24_module_attributes.rx_addr_p5, rf24_module_value_w8(rx_address));
}

void rf24_module_set_tx_address(uint8_t *tx_address /*3-5 bytes*/, uint8_t tx_address_length){
	rf24_module_write_attribute(rf24_module_attributes.tx_addr, rf24_module_value_w40(tx_address, tx_address_length));
}

void rf24_module_get_tx_address(uint8_t *tx_address_out /*3-5 bytes*/){
	value_out.w40 = tx_address_out;
	rf24_module_read_attribute(rf24_module_attributes.tx_addr, &value_out);
}

void rf24_module_set_payload_width_pipe(rf24_module_rx_pipes rx_pipe, uint8_t payload_width){

	//rf24_module_write_register(RX_PW_P0 + rx_pipe, payload_width & 0b00111111);

	switch(rx_pipe)
	{
		case 0: rf24_module_write_attribute(rf24_module_attributes.rx_pw_p0, rf24_module_value_w8(payload_width)); break;
		case 1: rf24_module_write_attribute(rf24_module_attributes.rx_pw_p1, rf24_module_value_w8(payload_width)); break;
		case 2: rf24_module_write_attribute(rf24_module_attributes.rx_pw_p2, rf24_module_value_w8(payload_width)); break;
		case 3: rf24_module_write_attribute(rf24_module_attributes.rx_pw_p3, rf24_module_value_w8(payload_width)); break;
		case 4: rf24_module_write_attribute(rf24_module_attributes.rx_pw_p4, rf24_module_value_w8(payload_width)); break;
		case 5: rf24_module_write_attribute(rf24_module_attributes.rx_pw_p5, rf24_module_value_w8(payload_width)); break;
	}
}

uint8_t rf24_module_get_payload_width_pipe(rf24_module_rx_pipes rx_pipe)
{
	return rf24_module_read_register(RX_PW_P0 + rx_pipe);
}

bool rf24_module_rx_fifo_empty(){
	rf24_module_read_attribute(rf24_module_attributes.rx_empty, &value_out);
	return value_out.w1;
}

bool rf24_module_rx_fifo_full(){
	rf24_module_read_attribute(rf24_module_attributes.rx_full, &value_out);
	return value_out.w1;
}

bool rf24_module_tx_fifo_empty(){
	rf24_module_read_attribute(rf24_module_attributes.tx_empty, &value_out);
	return value_out.w1;
}

bool rf24_module_tx_reuse(){
	rf24_module_read_attribute(rf24_module_attributes.tx_reuse, &value_out);
	return value_out.w1;
}

void rf24_module_enable_dynamic_payload_pipe(rf24_module_rx_pipes rx_pipe){
	rf24_module_write_register(/*DYNPD*/ 0x1C, rf24_module_read_register(/*DYNPD*/ 0x1C) | (1<<rx_pipe));
}

void rf24_module_disable_dynamic_payload_pipe(rf24_module_rx_pipes rx_pipe){
	rf24_module_write_register(/*DYNPD*/ 0x1C, rf24_module_read_register(/*DYNPD*/ 0x1C) & ~(1<<rx_pipe));
}

void rf24_module_enable_dynamic_payload(){
	rf24_module_write_attribute(rf24_module_attributes.en_dpl, rf24_module_value_w1(true));
}

void rf24_module_disable_dynamic_payload(){
	rf24_module_write_attribute(rf24_module_attributes.en_dpl, rf24_module_value_w1(false));
}

void rf24_module_enable_autoack_with_payload(){
	rf24_module_write_attribute(rf24_module_attributes.en_ack_pay, rf24_module_value_w1(true));
}

void rf24_module_disable_autoack_with_payload(){
	rf24_module_write_attribute(rf24_module_attributes.en_ack_pay, rf24_module_value_w1(false));
}

void rf24_module_enable_transmission_without_ack(){
	rf24_module_write_attribute(rf24_module_attributes.en_dyn_ack, rf24_module_value_w1(true));
}

void rf24_module_disable_transmission_without_ack(){
	rf24_module_write_attribute(rf24_module_attributes.en_dyn_ack, rf24_module_value_w1(false));
}

void rf24_module_flush_tx()
{
	rf24_stm32f1xx_spi_csn_low();
	rf24_stm32f1xx_spi_shift_byte(FLUSH_TX);
	rf24_stm32f1xx_spi_csn_high();
}

void rf24_module_flush_rx()
{
	rf24_stm32f1xx_spi_csn_low();
	rf24_stm32f1xx_spi_shift_byte(FLUSH_RX);
	rf24_stm32f1xx_spi_csn_high();
}

void rf24_module_collect_rx_data(rf24_module_rx_data *rx_data)
{
	rf24_module_readwrite_register(read, R_RX_PAYLOAD, rx_data->payload, 32);
	rx_data->length = 32;
	rx_data->rx_pipe = rf24_module_get_rx_available_pipe();
}

// USER FUNCTIONS

void rf24_module_goto_state(rf24_module_states goto_state)
{
	#ifdef RF24_MODULE_DEBUG_STATE_MACHINE
		usart_write_str("[rf24]: switched state from ");
		usart_write_str(rf24_module_states_string[rf24_module_state]);
	#endif

	switch(rf24_module_state){

		/* UNDEFINED -> POWER_DOWN */
		case UNDEFINED:{
			switch(goto_state){
				case POWER_DOWN:
					delay_ms(20); //10.3ms
					rf24_module_state = POWER_DOWN;
					break;
				default:break;
			}
			break;
		}

		/* POWER_DOWN -> STAND_BY_I */
		case POWER_DOWN:{
			switch(goto_state){
				case STAND_BY_I:{
					rf24_module_power_up();
					rf24_module_state = START_UP;
					delay_ms(2); //1.5ms
					rf24_module_state = STAND_BY_I;
					break;
				}
				default:break;
			}
			break;
		}

		/* STAND_BY_I -> TX_MODE, RX_MODE */
		case STAND_BY_I:{
			switch(goto_state){
				case RX_MODE:{
					rf24_module_set_prim_rx(high);
					rf24_module_set_ce(high);
					rf24_module_state = RX_SETTING;
					delay_us(130); //130us
					rf24_module_state = RX_MODE;
					break;
				}
				case TX_MODE:{
					rf24_module_set_prim_rx(low);
					rf24_module_set_ce(low);
					// ce = 1 for more than 10us
					rf24_module_set_ce(high);
					delay_us(20); // > 10us
					rf24_module_set_ce(low);
					// tx setting 130us
					rf24_module_state = TX_SETTING;
					delay_us(130); //130us
					rf24_module_state = TX_MODE;
					break;
				}
				default:break;
			}
		}

		/* TX_MODE -> STAND_BY_I */
		case TX_MODE:{
			switch(goto_state){
				case STAND_BY_I:{
					rf24_module_set_ce(low);
					rf24_module_state = STAND_BY_I;
					break;
				}
				default:break;
			}
			break;
		}

		/* RX_MODE -> STAND_BY_I */
		case RX_MODE:{
			switch(goto_state){
				case STAND_BY_I:{
					rf24_module_set_ce(low);
					rf24_module_state = STAND_BY_I;
				}
				default:break;
			}
			break;
		}

		default: break;
	}

	#ifdef RF24_MODULE_DEBUG_STATE_MACHINE
		usart_write_str(" to ");
		usart_write_line(rf24_module_states_string[rf24_module_state]);
	#endif

}

bool rf24_module_connected(){
	rf24_module_read_attribute(rf24_module_attributes.aw, &value_in);
	return (value_in.w8 >= 1 && value_in.w8 <= 3);
}

bool rf24_module_init(uint32_t baudrate)
{
	// init hardware: system clock, timers, interrupts, pin out-/inputs, etc.
	rf24_stm32f1xx_init();
	// init usart
	rf24_stm32f1xx_usart_init(baudrate);
	// init spi
	rf24_stm32f1xx_spi_init();

	// check if rf module is responding / connected
	/*uint8_t attempts = 0;
	while(!rf24_module_connected() && attempts < 3){
		usart_write_line("rf24: module not responding, try again..");
		delay_ms(500);
		attempts++;
	}
	if(attempts >= 3){
		usart_write_line("rf24: module not responding, check pin connections");
		return false;
	}*/

	// switch to state power down (from undefined)
	rf24_module_goto_state(POWER_DOWN);

	// configure rf module
	rf24_module_config();

	// switch to rx mode over stand_by mode
	rf24_module_goto_state(STAND_BY_I);
	rf24_module_goto_state(RX_MODE);


	rf24_debug(RF_MODULE, INFO, VOID, VOID, NULL, "Initialization finished\n", "");


	return true;
}

void rf24_module_disable_autoack()
{
	rf24_module_write_register(EN_AA, 0x00);
	rf24_module_set_autoretransmit_count(rf24_module_autoretransmit_disable);
}

void rf24_module_config()
{
	rf24_module_set_rf_channel(5);
	rf24_module_set_rf_outputpower(rf24_module_rf_output_power3);
	rf24_module_set_datarate(rf24_module_datarate_250kbps);
	//rf24_module_enable_autoack_pipe(rf24_module_rx_pipe0);
	rf24_module_enable_rx_pipe(rf24_module_rx_pipe0);

	rf24_module_disable_autoack();

	rf24_module_enable_crc();
	//rf24_module_disable_crc();
	rf24_module_set_crc_length(rf24_module_crc_length_2bytes);
	rf24_module_enable_max_retries_interrupt();
	rf24_module_enable_tx_data_sent_interrupt();
	rf24_module_enable_rx_data_read_interrupt();

	rf24_module_set_address_width(rf24_module_address_width_5bytes);
	rf24_module_set_payload_width_pipe(rf24_module_rx_pipe0, 32);

	//rf24_module_set_autoretransmit_delay(rf24_module_autoretransmit_delay_750us);
	//rf24_module_set_autoretransmit_count(rf24_module_autoretransmit_count_15);
	rf24_module_set_autoretransmit_count(rf24_module_autoretransmit_disable);

	//rf24_module_enable_dynamic_payload();
	//rf24_module_enable_dynamic_payload_pipe(rf24_module_rx_pipe0);
	//rf24_module_enable_dynamic_payload_with_ack();
	//rf24_module_enable_transmission_without_ack();

	uint8_t address[5] = {'n','R','F','2','4'};
	uint8_t address_width = rf24_module_get_address_width();
	rf24_module_set_tx_address(address, address_width);
	rf24_module_set_rx_address_pipe0(address, address_width);

	rf24_module_flush_tx();
	rf24_module_flush_rx();
	rf24_module_clear_all_interrupts();
}

uint8_t rf24_module_transmit(rf24_module_tx_data *tx_data){

	// 1) flush tx___________________________________________________________________________________________
		rf24_stm32f1xx_spi_shift_byte(FLUSH_TX);

	// 2) Goto STAND_BY_I________________________________________________________________________________
		// Switch state from "RX_MODE" to "STAND_BY_I"
		rf24_module_goto_state(STAND_BY_I);

	// 3) Fill tx fifo___________________________________________________________________________________
		// Write payload into tx buffer

		uint8_t payload[32];

		for(int i=0; i<32; i++){
			if(i < tx_data->length) payload[i] = tx_data->payload[i];
			else payload[i] = '\0';
		}

		rf24_module_readwrite_register(write, W_TX_PAYLOAD, payload, 32);

	// 4) Start transmit_________________________________________________________________________________
		// Switch state from "STAND_BY_I" to "TX_MODE"
		rf24_module_goto_state(TX_MODE);

	return 1;
}

void rf24_module_attach_notify_data_received(rf24_module_rx_data_fct_ptr fct_ptr)
{
	rx_data_received_notify_fct_ptr = fct_ptr;
}

void rf24_module_attach_notify_data_transmitted(rf24_module_fct_ptr fct_ptr)
{
	tx_data_transmitted_notify_fct_ptr = fct_ptr;
}

rf24_module_rx_data* rf24_module_get_rx_data()
{
	return &rx_data;
}

void rf24_module_print_rx_data(rf24_module_rx_data *rx_data)
{
	char str[128];

	sprintf(str,
			"\nrf24_module: package with %d bytes received on pipe %d \n--------------------------------------------------------------------",
			rx_data->length,
			rx_data->rx_pipe);

	rf24_stm32f1xx_usart_write_line(str);

	int index = 0;

	for (int i=0; i<rx_data->length; i++)
		index += sprintf(&str[index], "%d ", rx_data->payload[i]);

	rf24_stm32f1xx_usart_write_str("[int]: ");
	rf24_stm32f1xx_usart_write_line(str);

	/*index = 0;
	for (int i=0; i<rx_data->length; i++)
		index += sprintf(&str[index], "%c", rx_data->payload[i]);

	rf24_stm32f1xx_usart_write_str("[ascii]: ");
	rf24_stm32f1xx_usart_write_line(str);*/
	rf24_stm32f1xx_usart_write_str("\n");
}

void rf24_module_irq_handler()
{
	if(rf24_module_max_retransmits())
	{
		rf24_module_flags.max_retransmits = true;

		// switch back to receiver mode
		rf24_module_goto_state(STAND_BY_I);
		rf24_module_goto_state(RX_MODE);

		#ifdef RF24_MODULE_DEBUG_IRQ
			rf24_debug("max retransmits");
		#endif
	}

	if(rf24_module_package_transmitted())
	{
		rf24_module_flags.package_transmitted = true;

		// switch back to receiver mode
		rf24_module_goto_state(STAND_BY_I);
		rf24_module_goto_state(RX_MODE);

		// if a callback function attached, notify
		// [WARNING:] DONT ATTACH ANY LONG PROCEEDINGS TO THIS NOTIFY
		if(tx_data_transmitted_notify_fct_ptr)
			tx_data_transmitted_notify_fct_ptr();

		#ifdef RF24_MODULE_DEBUG_IRQ
			rf24_debug("package transmitted");
		#endif
	}

	if(rf24_module_rx_data_available())
	{
		rf24_module_flags.package_received = true;

		// read rx data from chip, store it into global struct rx_data
		rf24_module_collect_rx_data(&rx_data);

		// if a callback function attached, notify
		// [WARNING:] DONT ATTACH ANY LONG PROCEEDINGS TO THIS NOTIFY
		if(rx_data_received_notify_fct_ptr)
			rx_data_received_notify_fct_ptr(&rx_data);

		#ifdef RF24_MODULE_DEBUG_IRQ
			rf24_module_print_rx_data(&rx_data);
		#endif
	}

	rf24_module_clear_all_interrupts();
}
