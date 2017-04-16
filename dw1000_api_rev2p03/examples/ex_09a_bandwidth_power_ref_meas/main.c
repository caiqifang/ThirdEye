/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   TX Bandwidth and Power Compensation Reference Measurement example code
 *
 *       This example application takes reference measurements from the DW1000 for the bandwidth and power settings, to be
 *       used for the example 09b (bandwidth and power compensation). These reference measurements are used as a base for
 *       the adjustments done during compensation. The measurements to be taken are the temperature and the contents of
 *       the TX_POWER, PG_DELAY and PGC_STATUS registers. These measurements will be output on the LCD screen.
 *
 * @attention
 *
 * Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#include <stdio.h>
#include <string.h>

#include "deca_device_api.h"
#include "deca_regs.h"
#include "sleep.h"
#include "lcd.h"
#include "port.h"

/* Example application name and version to display on LCD screen. */
#define APP_NAME "BW PWR REF v1.0"

/* String to display on LCD string */
char disp_str[16] = {0};

/* Default communication configuration. See NOTE 1 below. */
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,       /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    0,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (129 + 8 - 8)   /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power of the spectrum at the current
 * temperature. These values can be calibrated prior to taking reference measurements. See note 2 below. */
static dwt_txconfig_t txconfig = {
    0xC0,            /* PG delay. */
    0x25456585,      /* TX power. */
};

/**
 * Application entry point.
 */
int main(void)
{
    uint16 ref_pgcount;
    double ref_temp;
    uint32 ref_power;
    uint8 ref_pgdelay;

    /* Start with board specific hardware init. */
    peripherals_init();

    /* Display application name on LCD. */
    lcd_display_str(APP_NAME);

    /* During initialisation and continuous frame mode activation, DW1000 clocks must be set to crystal speed so SPI rate has to be lowered and will
     * not be increased again in this example. */
    spi_set_rate_low();

    /* Reset and initialise DW1000. See NOTE 3 below. */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR)
    {
        lcd_display_str("INIT FAILED");
        while (1)
        { };
    }

    /* Configure DW1000. */
    dwt_configure(&config);
    /* Configure the TX frontend with the desired operational settings */
    dwt_configuretxrf(&txconfig);

    /* Read DW1000 IC temperature for temperature compensation procedure. See NOTE 4 */
    ref_temp = 1.13 * (double)((dwt_readtempvbat(1) & 0xFF00) >> 8) - 113.0;

    ref_pgcount = dwt_calcpgcount(txconfig.PGdly);
    ref_power = txconfig.power;
    ref_pgdelay = txconfig.PGdly;

    /* Software reset of the DW1000 to deactivate continuous frame mode and go back to default state. Initialisation and configuration should be run
     * again if one wants to get the DW1000 back to normal operation. */
    dwt_softreset();

    /* End here. */
    /* Display the temperature, power register, PG_DELAY register and PGC_STATUS register on the LCD screen */
    while (1)
    {
        sprintf(disp_str, "Temp: %.2fC", ref_temp);
        lcd_display_str(disp_str);
        deca_sleep(2000);
        sprintf(disp_str, "Power: %x", (unsigned)ref_power);
        lcd_display_str(disp_str);
        deca_sleep(2000);
        sprintf(disp_str, "PG_DELAY: %02x", (unsigned)ref_pgdelay);
        lcd_display_str(disp_str);
        deca_sleep(2000);
        sprintf(disp_str, "PG_COUNT: %x", (unsigned)ref_pgcount);
        lcd_display_str(disp_str);
        deca_sleep(2000);

    };
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The reference measurements are made after optimising the transmit spectrum bandwidth and power level to maximise the use of the allowed spectrum
 *    mask (the mask used was the IEEE 802.15.4a mask). This optimisation needs to be carried out once, perhaps in a production test environment, and
 *    the reference measurements to be stored are the temperature at which the optimisation is made, the contents of the TX_POWER [1E] register and
 *    the contents of the PG_DELAY [2A:0B] register and the contents of the PG_COUNT [2A:08] register. For more information, see App Note APS024
 * 2. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 3. The temperature is read from the DW1000 using this API call. The temperature is in the MSB, and required an adjustment to a real °C value using
 *    the formula: Treal = (1.13 * Tdw) + 113.0
 ****************************************************************************************************************************************************/
