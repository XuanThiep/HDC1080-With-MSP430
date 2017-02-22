#include "driverlib.h"
#include "mymsp430.h"
#include "hdc1080.h"


/* Declaring Global Variables */
float temp;
uint8_t humi;
uint8_t buffer[20];

/* Declaring Function Prototype */
void uart_send_string(uint8_t* string);

/* Interrupt Service Rounties */
#pragma vector=TIMER2_A1_VECTOR
__interrupt void TimerA2_ISR(void)
{
	Delay_Using_TimerA2_ISR();
}



void main( void )
{
	/* Stop watchdog timer */
	WDT_A_hold(WDT_A_BASE);

	/* Initialize clock using DCO */
	Clk_Using_DCO_Init(16000,4000,SMCLK_CLOCK_DIVIDER_2);

	/* Initialize delay function using timerA2 */
	Delay_Using_TimerA2_Init();

	/* Initialize I2C peripheral */
	I2c_Init(USCI_B0_BASE,
			GPIO_PORT_P3,GPIO_PIN0,//SDA - P3.0
			GPIO_PORT_P3,GPIO_PIN1//SCL - P3.1
	);

	/* Initialize HDC1080 sensor */
	hdc1080_init(USCI_B0_BASE,Temperature_Resolution_14_bit,Humidity_Resolution_14_bit);

	/* Initialize GPIO as output for led */
	GPIO_setAsOutputPin(GPIO_PORT_P4,GPIO_PIN7);

	/* Initialize Uart for transmit data to pc */
	UART_Initialize(USCI_A0_BASE,
			GPIO_PORT_P3,GPIO_PIN3, //TxD
			GPIO_PORT_P3,GPIO_PIN4 //RxD
	);

	uart_send_string((uint8_t*)"Hello World\r\n");

	while(1)
	{
		hdc1080_start_measurement(USCI_B0_BASE,(float*)&temp,(uint8_t*)&humi);
		GPIO_toggleOutputOnPin(GPIO_PORT_P4,GPIO_PIN7);
		sprintf((char*)buffer,"Temp = %5.2f  Humi = %02d\r\n",temp,humi);
		uart_send_string(buffer);

		Delay_ms(500);
	}
}


void uart_send_string(uint8_t* string)
{
	for(uint8_t i=0; i< strlen((const char*)string);i++)
	{
		while(!USCI_A_UART_getInterruptStatus(USCI_A0_BASE,USCI_A_UART_TRANSMIT_INTERRUPT_FLAG));
		USCI_A_UART_transmitData(USCI_A0_BASE,*(string+i));
	}
}
