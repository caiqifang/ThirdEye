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
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "deca_spi.h"
#include "deca_device_api.h"
//#include <wiringPi.h>
//#include <wiringPiSPI.h>
#include <bcm2835.h>

int channel = 0;

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: openspi()
 *
 * Low level abstract function to open and initialise access to the SPI device.
 * returns 0 for success, or -1 for error
 */
int openspi(/*SPI_TypeDef* SPIx*/)
{
/*  wiring Pi impementation
	wiringPiSetup () ;
        if ((fd = wiringPiSPISetup (channel, 2000000)) < 0) // channel 0, freq 2MHz
	{
	  fprintf (stderr, "Can't open the SPI bus: %s\n", strerror (errno)) ;
	  exit (EXIT_FAILURE) ;
        }
*/

    if (!bcm2835_init())
    {
      printf("bcm2835_init failed. Are you running as root??\n");
      return 1;
    }
    if (!bcm2835_spi_begin())
    {
      printf("bcm2835_spi_begin failed. Are you running as root??\n");
      return 1;
    }
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
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_11, BCM2835_GPIO_FSEL_INPT); // RSTn input / open drain

    printf("Open SPI succeeds\n");
    
    deca_sleep(10);
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
	//while (port_SPIx_busy_sending()); //wait for tx buffer to empty

	//port_SPIx_disable();

        //close(fd);
        bcm2835_gpio_set(RPI_V2_GPIO_P1_24); // CE0 high
        bcm2835_spi_end();
        bcm2835_close();

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
#pragma GCC optimize ("O3")
int writetospi(uint16 headerLength, uint8 *headerBuffer, uint32 bodylength, uint8 *bodyBuffer)
{
    uint16 totalLength = headerLength + bodylength;
    uint8 buffer[totalLength];
    memcpy(buffer, headerBuffer, headerLength);
    memcpy(&(buffer[headerLength]), bodyBuffer, bodylength);
    bcm2835_spi_transfern(buffer, totalLength);


    //int i=0;
/*  wiringPi implementation
    int res;
    //digitalWrite(10, 0); // set wpin 10 (CE0) low to assert CSn
    printf("Before hB : %s\n",headerBuffer);
    res = wiringPiSPIDataRW(channel, headerBuffer, headerLength);
    printf("in headerBuffer:%s\n",headerBuffer);
    printUint8(headerBuffer,headerLength);
    if (res < 0) {
        fprintf (stderr, "Can't write SPI header: %s\n", strerror (errno)) ;
	return -1;
    }
    printf("Before bB %s\n",bodyBuffer);
    printUint8(bodyBuffer,bodylength);
    res = wiringPiSPIDataRW(channel, bodyBuffer, bodylength);
    printf("After bodyBuffer %s\n",bodyBuffer);
    if (res < 0) {
        fprintf (stderr, "Can't write SPI body: %s\n", strerror (errno)) ;
	return -1;
    }
    //digitalWrite(10, 1); // set wpin 10 (CE0) hight to deassert CSn
*/
/*    decaIrqStatus_t  stat ;

    stat = decamutexon() ;

    SPIx_CS_GPIO->BRR = SPIx_CS;

    for(i=0; i<headerLength; i++)
    {
    	SPIx->DR = headerBuffer[i];

    	while ((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);

    	SPIx->DR ;
    }

    for(i=0; i<bodylength; i++)
    {
     	SPIx->DR = bodyBuffer[i];

    	while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);

		SPIx->DR ;
	}

    SPIx_CS_GPIO->BSRR = SPIx_CS;

    decamutexoff(stat) ;
*/
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
#pragma GCC optimize ("O3")
int readfromspi(uint16 headerLength, uint8 *headerBuffer, uint32 readlength, uint8 *readBuffer)
{
    uint16 totalLength = headerLength + readlength;
    uint8 buffer[totalLength];
    memcpy(buffer, headerBuffer, headerLength);
    //printf("Before buffer spi length:%u\n",headerLength+readlength);
    //printUint8(buffer,headerLength+readlength);
    
    bcm2835_spi_transfern(buffer, totalLength);
    //printf("After buffer spi\n");
    //printUint8(buffer, headerLength+readlength);
    
    memcpy(readBuffer, &(buffer[headerLength]), readlength);
    //printf("ReadBuffer\n");
    //printUint8(readBuffer,readlength);

/*    
    printf("Before HeaderBuffer spi length:%u\n",headerLength);
    printUint8(headerBuffer,headerLength);
    bcm2835_spi_transfern(headerBuffer, headerLength);
    printf("After HeaderBuffer spi\n");
    printUint8(headerBuffer,headerLength);

    printf("Before ReadBuffer spi length:%u\n",readlength);
    printUint8(readBuffer,readlength);
    bcm2835_spi_transfern(readBuffer, readlength);
    printf("After readBuffer\n");
    printUint8(readBuffer,readlength);
*/
    //int i=0;
/*  wiringPi implementation
    int res;
    digitalWrite(10, 0); // set wpin 10 (CE0) low to assert CSn
    printf("Before HeaderBuffer spi length:%u\n",headerLength);
    printUint8(headerBuffer,headerLength);
    
    res = wiringPiSPIDataRW(channel, headerBuffer, headerLength);
    
    printf("After HeaderBuffer spi\n");
    printUint8(headerBuffer,headerLength);
    if (res < 0) {
        fprintf (stderr, "Can't write SPI header for read: %s\n", strerror (errno)) ;
	return -1;
    }
    printf("Before ReadBuffer spi length:%u\n",readlength);
    printUint8(readBuffer,readlength);
   
    res = wiringPiSPIDataRW(channel, readBuffer, readlength);
    
    printf("After readBuffer\n");
    printUint8(readBuffer,readlength);
    if (res < 0) {
        fprintf (stderr, "Can't read SPI body: %s\n", strerror (errno)) ;
	return -1;
    }
    digitalWrite(10, 1); // set wpin 10 (CE0) high to deassert CSn
*/
/*    decaIrqStatus_t  stat ;

    stat = decamutexon() ;

    // Wait for SPIx Tx buffer empty
    //while (port_SPIx_busy_sending());

    SPIx_CS_GPIO->BRR = SPIx_CS;

    for(i=0; i<headerLength; i++)
    {
    	SPIx->DR = headerBuffer[i];

     	while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);

     	readBuffer[0] = SPIx->DR ; // Dummy read as we write the header
    }

    for(i=0; i<readlength; i++)
    {
    	SPIx->DR = 0;  // Dummy write as we read the message body

    	while((SPIx->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
 
	   	readBuffer[i] = SPIx->DR ;//port_SPIx_receive_data(); //this clears RXNE bit
    }

    SPIx_CS_GPIO->BSRR = SPIx_CS;

    decamutexoff(stat) ;
*/
    return 0;
} // end readfromspi()



void set_spi_rate_high() {

    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_16);  // 15.625MHz
}

