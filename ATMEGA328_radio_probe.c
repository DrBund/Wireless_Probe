                                            /* SPI ATMEGA radio */

// ------- Preamble -------- //
#include <avr/io.h>
#include <util/delay.h>
#include "ATMEGA-328-pinDefines.h"
#include "USART.h"
#include "nRF24L01.h"
#include "ATMEGA-328-pinDefines.h"



NRF24L01pClass * myRadio;

// Interrupt variable
volatile unsigned char IRQ_state = 0x00;

// Used to check the status of a given bit in a variable
#define CHECK_BIT(var,pos) ((var & (1 << pos)) == (1 << pos))

// GLOBALS >> GLOBALS  >> GLOBALS  >> GLOBALS  >> GLOBALS 
// Set if the radio is transmitter (TX) or receiver (RX)
int radioMode = 0; // radioMode = 1 for RX, 0 for TX
int rxDataFLAG = 0; // Indicates that there is data in the RX FIFO buffer
// GLOBALS << GLOBALS  << GLOBALS  << GLOBALS  << GLOBALS 


void setup()
{
	/* add setup code here */
  initUSART();
	printString("Begin startup");
	
	// Begin SPI communication
	initSPImaster();
	
	// int airDataRate = 250; //kBps, can be 250, 1000 or 2000 section 6.3.2
	//int rfChannelFreq = 0x02; // = 2400 + RF_CH(MHz) section 6.3.3 0x02 is the default
	//  RF_CH can be set from 0-83. Any channel higher than 83 is off limits in US by FCC law
	//SETUP_AW: AW=11-5 byte address width
	myRadio = new NRF24L01pClass;
	myRadio->init(SPI_CE,SPI_CSN);
	
	// Start radio
	myRadio->begin();
	
	// Setup data pipes, addresses etc
	//
	// Use default addresses for now _ CHANGE ADDRESSES HERE IN FUTURE
	unsigned char pipesOn [] = {0x03}; // which pipes to turn on for receiving
	unsigned char fixedPayloadWidth [] = {0x05}; // number of bytes for payload width
	myRadio->setup_data_pipes(pipesOn, fixedPayloadWidth);
	
	//DEBUG - change RX_ADDR_P0 to see if I am reading the right value
	unsigned char tmpArr [] = {0xE7,0xE7,0xE7,0xE7,0xE7};
	myRadio->writeRegister(RX_ADDR_P0,tmpArr, 5);
	
	
	// Configure radio to be TX (transmitter) or RX (receiver)
	printString("Configure Radio\r\n");
	if (radioMode)
	{myRadio->rMode();}// Configure radio to be a receiver
	else
	{myRadio->txMode();}// Configure radio to be a receiver

	// Clear any interrupts
	clear_interrupts();

	unsigned char tmp_state [] = {0x00};
	tmp_state[0] = *myRadio->readRegister(STATUS, 0);
	printString("STATUS: ");
	printBinaryByte(tmp_state[0]);
  printString("\r\n");

	
	myRadio->setDebugVal(123);
	printString("**************************************  debug_val  = ");
	printWord(myRadio->getDebugVal());

	// Attach interrupt 0 (digital pin 2 on the Arduino Uno)
	//	This interrupt monitors the interrupt pin (IRQ) from the nRF24L01 Radio
	//  The IRQ is normally high, and active low
	//  The IRQ is triggered at:
	delay(100); // Make sure all the configuration is completed before attaching the interrupt
	//attachInterrupt(0, IRQ_resolve, FALLING);
  // The nRF24L01p chip should pullup the pin when not interrupting
  initInterrupt0();
}

void initInterrupt0(void)
{
  EIMSK |= (1 << INT0);                  /* Enable INT0 */
  EICRA |= (1<< ISC01);         /* trigger when falling */
  sei();             /* set global interrupt enable bit */
}

int main(void) {

  // -------- Inits --------- //
  setup();
  
  // ------ Event loop ------ //
  while (1) {

	if (IRQ_state == 1)
	{
		IRQ_reset_and_respond();
	}
	
	// Radio is in TX mode
	if (radioMode == 0) 
	{
		// Send data
		printString("Transmit code abc go\r\n");
		unsigned char tmpData [] = {1,2,3,4,26}; // Data needs to be the same size as the fixedDataWidth set in setup
		myRadio->txData(tmpData, 5); // This is currently sending data to pipe 0 at the default address. Change this once the radio is working
		delay(2000);
	}
	// Radio is in RX mode
	else
	{
		if (rxDataFLAG == 1)
		{
			// Receive data and print
			
			unsigned char * tmpRxData = myRadio->rData(5);
			printString("RX Data: \r\n");
			for (int x=0; x<5; x++)
			{
				printString("Element ");
			  printWord(x);
				printString(": ");
				printWord(*(tmpRxData+x));
        printString("\r\n");
			}
			
			myRadio->flushRX();
			rxDataFLAG = 0; // reset rxDataFLAG
		}
		
		
	}
	
	delay(50); // Short delay to keep everything running well. Make sure the IRQ's get cleared before next loop. etc...

  }                                                  /* End event loop */
  return (0);                            /* This line is never reached */
}




unsigned char setBit(unsigned char byteIn, int bitNum, boolean setClear)
{
	if(setClear == 1)
	byteIn |= (1<<bitNum);
	else
	byteIn &= ~(1<<bitNum);
	
	return byteIn;
}



//******* INTERRUPTS **************** INTERRUPTS ***************** INTERRUPTS ****************************

/* IRQ_resolve
Resolve the attachInterrupt function quickly
*/
ISR(INT0_vect)
{
	// Get the IRQ code from the receiver and assign it to IRQ_state variable
	//unsigned char * p_tmp;
	//printString("IRQ");
	//IRQ_state = * myRadio->readRegister(STATUS,1); // this returns a pointer, so I dereferenced it to the unsigned char for IRQ_state
	IRQ_state = 1;
}



/* IRQ_reset_and_respond
Reset the IRQ in the radio STATUS register
Also resolve the condition which triggered the interrupt
*/
void IRQ_reset_and_respond(void)
{
	printString(" ------------------ RESPOND TO IRQ --------------------- \r\n");
	unsigned char tmp_state [] = {0x00};
	unsigned char tmp_status = * myRadio->readRegister(STATUS,1);
	
	if CHECK_BIT(tmp_status,0) // TX_FIFO full
	{
		printString("TX_FIFO Full\r\n");
	}
	if (CHECK_BIT(tmp_status,1)|CHECK_BIT(tmp_status,2)|CHECK_BIT(tmp_status,3)) // TX_FIFO full
	{
		printString("Pipe Number Changed\r\n");
	}
	if CHECK_BIT(tmp_status,4) // Maximum number of TX retries interrupt
	{
		printString("Max TX retries IRQ\r\n");
		myRadio->flushTX();
	}
	if CHECK_BIT(tmp_status,5) // Data sent TX FIFO interrupt
	{
		printString("Data Sent TX FIFO IRQ\r\n");
	}
	if CHECK_BIT(tmp_status,6) // Data ready RX FIFO interrupt
	{
		printString("Data ready RX FIFO IRQ\r\n");
		// Read the data from the R_RX_PAYLOAD
		// RX_P_NO bits 3:1 tell what pipe number the payload is available in 000-101: Data Pipe Number, 110: Not Used, 111: RX_FIFO Empty
		// Get bits 3:1 and right shift to get pipe number
		//pipeNumber = (tmp_status & 0xE) >> 1;
		rxDataFLAG = 1; //Set Rx Data FLAG
	}
	
	clear_interrupts();
	IRQ_state = 0; //reset IRQ_state
	
}

void clear_interrupts(void)
{
	// Clear any interrupts
	unsigned char tmp_state [] = {1<<RX_DR};
	myRadio->writeRegister(STATUS, tmp_state, 1);
	tmp_state [0] = 1<<TX_DS;
	myRadio->writeRegister(STATUS, tmp_state, 1);
	tmp_state [0] = 1<<MAX_RT;
	myRadio->writeRegister(STATUS, tmp_state, 1);
	// Flush the TX register
	myRadio->flushTX();
}



