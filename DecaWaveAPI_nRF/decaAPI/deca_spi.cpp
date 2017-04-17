/*! ----------------------------------------------------------------------------
 * @file	deca_spi.c
 * @brief	SPI access functions
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */
#include <stdio.h>
#include <stdlib.h>
#include "mbed.h"
//#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "deca_spi.h"
#include "deca_device_api.h"

int channel = 0;
volatile int SPI_write_done = 0;
volatile int SPI_read_done = 0;
extern Serial pc;
SPI device(SPI_PSELMOSI0, SPI_PSELMISO0, SPI_PSELSCK0, SPI_PSELSS0);
DigitalInOut rst(P0_3); 

void writeCallback(int event) {
    SPI_write_done = 1;
}
void readCallback(int event) {
    SPI_read_done = 1;
}
/*! ------------------------------------------------------------------------------------------------------------------
 * Function: openspi()
 *
 * Low level abstract function to open and initialise access to the SPI device.
 * returns 0 for success, or -1 for error
 */
int openspi(/*SPI_TypeDef* SPIx*/)
{
    
    device.frequency(2000000);
    /*
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128);   // 1.95MHz on Pi2
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
    
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_24, BCM2835_GPIO_FSEL_ALT0); // CE0 alt0
    bcm2835_gpio_set(RPI_V2_GPIO_P1_24); // CE0 high

    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_11, BCM2835_GPIO_FSEL_OUTP); // RSTn output
    bcm2835_gpio_clr(RPI_V2_GPIO_P1_11); // RSTn low 
    deca_sleep(10); // delay 10ms
    //bcm2835_gpio_set(RPI_V2_GPIO_P1_11); // RSTn high
    bcm2835_gpio_set_pud(RPI_V2_GPIO_P1_11, BCM2835_GPIO_PUD_OFF);  // RSTn float
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_11, BCM2835_GPIO_FSEL_INPT); // RSTn input / open drain*/

    //printf("Open SPI succeeds\n");
    rst.output();
    rst.write(0);
    deca_sleep(10);
    rst.mode(PullNone);
    rst.input();
    rst.mode(PullNone);
	return 0;

} // end openspi()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: closespi()
 *
 * Low level abstract function to close the the SPI device.
 * returns 0 for success, or -1 for error
 */
int closespi(void)
{
	return 0;

} // end closespi()

void printUint8(uint8 *buffer, uint32 blen) {
    int i = 0;
    for(i=0;i<blen;i++) {
        printf("In char %x\n",buffer[i]);
        printf("In uint8 %u\n",buffer[i]);
    }
}




/*! ------------------------------------------------------------------------------------------------------------------
 * Function: writetospi()
 *
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success, or -1 for error
 */
//#pragma GCC optimize ("O3")
int writetospi(uint16 headerLength, uint8 *headerBuffer, uint32 bodylength, uint8 *bodyBuffer)
{
    uint16 totalLength = headerLength + bodylength;
    uint8 buffer[totalLength];
    uint8 rvBuffer[totalLength];
    memcpy(buffer, headerBuffer, headerLength);
    memcpy(&(buffer[headerLength]), bodyBuffer, bodylength);
    //bcm2835_spi_transfern(buffer, totalLength);
    int s = device.transfer(&buffer[0], totalLength, &rvBuffer[0], totalLength, writeCallback);
    //pc.printf("write res %d\r\n", s);
    while (SPI_write_done == 0);
    //pc.printf("SPI write done\r\n");
    SPI_write_done = 0; // set SPI_wirte_done back to 0 for next transfer
    return 0;
} // end writetospi()


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: readfromspi()
 *
 * Low level abstract function to read from the SPI
 * Takes two separate byte buffers for write header and read data
 * returns the offset into read buffer where first byte of read data may be found,
 * or returns -1 if there was an error
 */
//#pragma GCC optimize ("O3")
int readfromspi(uint16 headerLength, uint8 *headerBuffer, uint32 readlength, uint8 *readBuffer)
{
    uint16 totalLength = headerLength + readlength;
    uint8 buffer[totalLength];
    uint8 rvBuffer[totalLength];
    memcpy(buffer, headerBuffer, headerLength);
    
    //for (int i = 0; i < totalLength; i++)
//    	pc.printf("%x ", rvBuffer[i]);
//    pc.printf("\r\n");
    int s = device.transfer(&buffer[0], totalLength, &rvBuffer[0], totalLength, readCallback);
    //pc.printf("read res %d, headerL %d readL %d\r\n", s, headerLength, readlength);
    while (SPI_read_done == 0);
    //pc.printf("SPI read done\r\n");
    SPI_read_done = 0; // set SPI_wirte_done back to 0 for next transfer
    //for (int i = 0; i < totalLength; i++)
//    	pc.printf("%x ", rvBuffer[i]);
//    pc.printf("\r\n");
    memcpy(readBuffer, &(rvBuffer[headerLength]), readlength);
    
    return 0;
} // end readfromspi()



void set_spi_rate_high() {
	device.frequency(16000000);
    //bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16);  // 15.625MHz
}


