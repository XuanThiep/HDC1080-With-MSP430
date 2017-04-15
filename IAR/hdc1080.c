#include "hdc1080.h"


/* @Brief	: 	Initialize hdc1080 with configure temperature resolution and
 * 				humidity resolution
 *
 * @Para	:	+ USCI_Base_Address - This is base address of USCI Module.
 *
 * 				+ Temperature_Resolution_x_bit - This is temperature resolution
 *
 * 				+ Humidity_Resolution_x_bit - humidity resolution
 *
 * @Return	:	+ 0 - If communicate with hdc1080 OK
 * 				+ !0 - If error occur
 *
 * @Note	:   User must be call this function one time before start measuring temperature and
 * 				humidity.
 */
uint8_t hdc1080_init(uint16_t USCI_Base_Address,Temp_Reso Temperature_Resolution_x_bit,Humi_Reso Humidity_Resolution_x_bit)
{
	/* Temperature and Humidity are acquired in sequence, Temperature first
	 * Default:   Temperature resolution = 14 bit,
	 *            Humidity resolution = 14 bit
	 */

	/* Set the acquisition mode to measure both temperature and humidity by setting Bit[12] to 1 */
	uint16_t config_reg_value=0x3000; //Heater enable
	uint8_t data_send[2],return_value;

	if(Temperature_Resolution_x_bit == Temperature_Resolution_11_bit)
	{
		config_reg_value |= (1<<10); //11 bit
	}

	switch(Humidity_Resolution_x_bit)
	{
	case Humidity_Resolution_11_bit:
		config_reg_value|= (1<<8);
		break;
	case Humidity_Resolution_8_bit:
		config_reg_value|= (1<<9);
		break;
	}

	data_send[0]= (config_reg_value>>8);
	data_send[1]= (config_reg_value&0x00ff);

	return_value = I2c_Master_Send_Multibyte_To_Slave(USCI_Base_Address,
			HDC_1080_ADD,
			Configuration_register_add,
			SLAVE_MEMORY_ADDRESS_SIZE_8BIT,
			data_send,
			2);
	return return_value;
}



/* @Brief	: 	Measuring temperature and humidity
 *
 * @Para	:	+ USCI_Base_Address - This is base address of USCI Module.
 *
 * 				+ temperature - This is pointer to store temperature data
 *
 * 				+ humidity - This is pointer to store humidity data
 *
 * @Return	:	+ 0 - If communicate with hdc1080 OK
 * 				+ !0 - If error occur
 *
 * @Note	:   None
 */
uint8_t hdc1080_start_measurement(uint16_t USCI_Base_Address,float* temperature, uint8_t* humidity)
{
	uint8_t receive_data[4];
	uint16_t temp=0,humi=0;
	uint16_t time_out= 5000;

	//Specify slave address
	HWREG16(USCI_Base_Address + OFS_UCBxI2CSA) = HDC_1080_ADD;

	//Set Transmit mode and Send start condition.
	HWREG8(USCI_Base_Address + OFS_UCBxCTL1) |= UCTR + UCTXSTT;

	//Poll for transmit interrupt flag.
	while((!(HWREG8(USCI_Base_Address + OFS_UCBxIFG) & UCTXIFG)) && (--time_out));
	if(!time_out) return 1;

	//Send slave memory address
	HWREG8(USCI_Base_Address + OFS_UCBxTXBUF) = Temperature_register_add;

	//Poll for transmit interrupt flag.
	while((!(HWREG8(USCI_Base_Address + OFS_UCBxIFG) & UCTXIFG)) && (--time_out));
	if(!time_out) return 1;

	//Send stop condition.
	HWREG8(USCI_Base_Address + OFS_UCBxCTL1) |= UCTXSTP;

	//Delay here 14ms for conversion compelete
	Delay_ms(14);

	//Set receive mode
	HWREG8(USCI_Base_Address + OFS_UCBxCTL1) &= ~UCTR;

	//Send start condition.
	HWREG8(USCI_Base_Address + OFS_UCBxCTL1) |= UCTXSTT;

	//Poll for transmit interrupt flag.
	while((!(HWREG8(USCI_Base_Address + OFS_UCBxIFG) & UCTXIFG)) && (--time_out));
	if(!time_out) return 1;

	/* Request receive from slave */
	for(uint8_t i=0;i<3;i++)
	{
		//Poll for receive interrupt flag.
		while((!(HWREG8(USCI_Base_Address + OFS_UCBxIFG) & UCRXIFG)) && (--time_out));
		if(!time_out) return 1;

		// Receive data
		*(receive_data+i) = HWREG8(USCI_Base_Address + OFS_UCBxRXBUF);
	}

	//Send stop condition.
	HWREG8(USCI_Base_Address + OFS_UCBxCTL1) |= UCTXSTP;

	//Poll for receive interrupt flag
	while((!(HWREG8(USCI_Base_Address + OFS_UCBxIFG) & UCRXIFG)) && (--time_out));
	if(!time_out) return 1;

	//Read the last byte data
	*(receive_data+3) = HWREG8(USCI_Base_Address + OFS_UCBxRXBUF);

	//Wait for Stop to finish
	while((HWREG8(USCI_Base_Address + OFS_UCBxCTL1) & UCTXSTP) && (--time_out));
	if(!time_out) return 1;

	temp=((receive_data[0]<<8)|receive_data[1]);
	humi=((receive_data[2]<<8)|receive_data[3]);

	*temperature=((temp/65536.0)*165.0)-40.0;
	*humidity=(uint8_t)((humi/65536.0)*100.0);

	return 0;
}
