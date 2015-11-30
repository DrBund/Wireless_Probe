#include "SPI.h"


// Variables
bool LED_latch_FLAG = false; // 0-not latched, 1-latched


void initSPImaster(void) {
  // set pin modes
  SPI_SS_DDR  |= (1 << SPI_SS);                       /* set SS Output */
  SPI_SS_PORT |= (1 << SPI_SS);       /* Start off not selected (high) */

  SPI_MISO_DDR   &= ~ (1 << SPI_MISO);                /* input on MISO */
  SPI_MISO_PORT  |= (1 << SPI_MISO);                 /* pullup on MISO */
  SPI_MOSI_DDR   |= (1 << SPI_MOSI);                   /* output on DO */
  SPI_SCK_DDR |= (1 << SPI_SCK);                      /* output on SCK */

}

void initSPIslave(void) {
  // set pin modes
  SPI_SS_DDR  &= ~(1 << SPI_SS);                        /* set SS Input */
          
  SPI_MISO_DDR  &= ~(1 << SPI_MISO);                   /* input on MOSI */
  SPI_MOSI_DDR |= (1 << SPI_MOSI);                    /* output on MISO */
  SPI_SCK_DDR   &= ~(1 << SPI_SCK);                     /* input on SCK */
}


uint8_t SPI_tradeByte(uint8_t tData) {
  SPIDR = tData;                        /* set SPI Data Register to tData */
  SPISR = _BV(SPIOIF);           /* clear Counter Overflow Interrupt Flag */
                                /* Interrupt is cleared when written to 1 */

  while ( (SPISR & _BV(SPIOIF)) == 0 ) {            /* While the 4-bit timer 
                                                       has not overflowed */

                         /* Set to 3-wire mode, software clock positive edge
                                           start the clock (SPITC toggle) */
   SPICR = (1<<SPIWM0)|(1<<SPICS1)|(1<<SPICLK)|(1<<SPITC);
  }
  return SPIDR;    /* Return the byte recieved from the SPI Data Register */
}

uint8_t SPI_readByte(void) {
  __LATCH_LOW;         /* Every new command must be started by a high to low 
                                   transition on CSN pin on the NRF24L01p */
  uint8_t tmp_rx = SPI_tradeByte(0);
  __LATCH_HIGH;
  return (tmp_rx);
}

uint8_t SPI_writeByte(uint8_t wData) {
  __LATCH_LOW;         /* Every new command must be started by a high to low 
                                   transition on CSN pin on the NRF24L01p */
  uint8_t ack_rx = SPI_tradeByte(wData);
  __LATCH_HIGH; 
  return (ack_rx);
}



void SPI_turnOnLED() {
    SLAVE_SELECT;
    delay(30);

    uint8_t tmp_ack = 0;
    do {
      // Send serial byte to SPI slave
      tmp_ack = SPI_writeByte(SPI_LED_ON);
      //SERIAL_ECHO("ACK Recieved: ");
      //SERIAL_ECHOLN((int)tmp_ack);
    } while (!(tmp_ack == ACK_SPI));

    delay(30);
    SLAVE_DESELECT;

}

void SPI_turnOffLED() {
    SLAVE_SELECT;
    delay(30); // 50 works well Note: play with this delay to see how low it can be

    uint8_t tmp_ack = 0;
    do {
      // Send serial byte to SPI slave
      tmp_ack = SPI_writeByte(SPI_LED_OFF);
      //SERIAL_ECHO("ACK Recieved: ");
      //SERIAL_ECHOLN((int)tmp_ack);
    } while (!(tmp_ack == ACK_SPI));

    delay(30); // Delay to allow the transmissions to complete before deselecting the slave device 
    SLAVE_DESELECT;

    //SERIAL_ECHO("** SPI LED OFF ** ");

    //Unlatch LED_latch_FLAG
    LED_latch_FLAG = false;
    
}


void SPI_latchOnLED() {
    // If latch flag is not set, latch on
    if (!LED_latch_FLAG) {
      LED_latch_FLAG = true;

      SLAVE_SELECT;
      delay(30);

      uint8_t tmp_ack = 0;
      do {
        // Send serial byte to SPI slave
        tmp_ack = SPI_writeByte(SPI_LED_ON);
        //SERIAL_ECHO("ACK Recieved: ");
        //SERIAL_ECHOLN((int)tmp_ack);
      } while (!(tmp_ack == ACK_SPI));

      delay(30);
      SLAVE_DESELECT;

    }
    // else just leave it latched, unlatch with SPI_turnOffLED()

}
