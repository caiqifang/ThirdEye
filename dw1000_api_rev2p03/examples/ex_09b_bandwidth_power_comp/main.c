/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   TX Bandwidth and Power Compensation over Temperature example code
 *
 *       This example application adjusts the bandwidth and power settings of the DW1000 to account for transmit bandwidth
 *       and power variations across temperature. It then enables continuous frame mode to transmit frames without
 *       interruption for 2 minutes before stopping all operation. The adjustment takes into account the temperature
 *       difference from a known reference temperature, at which the bandwidth and power of the transmit spectrum should
 *       be optimised and the PG_DELAY and TX_POWER register values read and stored. Adjusting for temperature variations
 *       relies on these reference values as well as the difference in temperature.
 *
 * @attention
 *
 * Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#include "deca_device_api.h"
#include "deca_regs.h"
#include "sleep.h"
#include "lcd.h"
#include "port.h"

/* Example application name and version to display on LCD screen. */
#define APP_NAME "BW PWR COMP v1.0"

/* Start-to-start delay between frames, expressed in quarters of the 499.2 MHz fundamental frequency (around 8 ns). See NOTE 1 below. */
#define CONT_FRAME_PERIOD 124800

/* Continuous frame duration, in milliseconds. */
#define CONT_FRAME_DURATION_MS 300000

struct ref_values {
    uint8 PGdly;
    uint32 power;
    double temperature;
    uint16 count;
};

/* Default communication configuration. See NOTE 1 below. */
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    0,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (129 + 8 - 8)   /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

/* The frame sent in this example is an 802.15.4e standard blink. It is a 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, put to 0.
 *     - byte 2 -> 9: device MAC (source) address, hard coded constant in this example for simplicity.
 *     - byte 10/11: frame check-sum, automatically set by DW1000 in a normal transmission and set to 0 here for simplicity.
 * See NOTE 1 below. */
static uint8 tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};

/**
 * Reference measurements
 * NOTE: THESE VARIABLES SHOULD BE CHANGED TO REFLECT THE REFERENCE MEASUREMENTS FROM EXAMPLE ex_09a
 * See NOTE 2 below.
 */
/* For this example the user should run the example ex_09a on their individual EVB unit and write down the values from LCD: for
 * "Temp:", "Power:", "PG_DELAY:" and ""PG_COUNT:" and then edit these values in this ex_09b to set the correct reference values
 * for the temperature compensation of power and bandwidth for their own (EVB) unit.*/
static struct ref_values reference = {
    0xC0,           /* PG_DELAY */
    0x25456585,     /* Power */
    13.29,          /* Temp */
    0x323           /* PG_COUNT */
};

/**
 * Application entry point.
 */
int main(void)
{
    double temp;

    /* Stores the adjusted bandwidth and power settings after temperature compensation */
    dwt_txconfig_t txconfig;

    /* Start with board specific hardware init. */
    peripherals_init();

    /* Display application name on LCD. */
    lcd_display_str(APP_NAME);

    /* During initialisation and continuous frame mode activation, DW1000 clocks must be set to crystal speed so SPI rate have to be lowered and will
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

    /* Read DW1000 IC temperature for temperature compensation procedure. See NOTE 4 */
    temp = 1.13 * (double) ((dwt_readtempvbat(1) & 0xFF00) >> 8) - 113.0;

    /* Compensate bandwidth and power settings for temperature */
    txconfig.PGdly = dwt_calcbandwidthtempadj(reference.count);
    txconfig.power = dwt_calcpowertempadj(config.chan, reference.power, temp, reference.temperature);

    /* Configure the TX frontend with the adjusted settings */
    dwt_configuretxrf(&txconfig);

    /* Activate continuous frame mode. */
    dwt_configcontinuousframemode(CONT_FRAME_PERIOD);

    /* Once configured, continuous frame must be started like a normal transmission. */
    dwt_writetxdata(sizeof(tx_msg), tx_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_msg), 0, 0); /* Zero offset in TX buffer, no ranging. */
    dwt_starttx(DWT_START_TX_IMMEDIATE);

    /* Wait for the required period of repeated transmission. */
    sleep_ms(CONT_FRAME_DURATION_MS);

    /* Software reset of the DW1000 to deactivate continuous frame mode and go back to default state. Initialisation and configuration should be run
     * again if one wants to get the DW1000 back to normal operation. */
    dwt_softreset();

    /* End here. */
    while (1)
    { };
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. Continuous frame mode is typically used to tune transmit power for regulatory purposes. This example has been designed to reproduce the use case
 *    of a tag blinking at a high rate: the blink frame length is around 180 µs in this configuration, emitted once per millisecond. In this configuration,
 *    the frame's transmission power can be increased while still complying with regulations. For more details about the management of TX power, the
 *    user is referred to DW1000 User Manual.
 * 2. The reference measurements are made after optimising the transmit spectrum bandwidth and power level to maximise the use of the allowed spectrum
 *    mask (the mask used was the IEEE 802.15.4a mask). This optimisation needs to be carried out once for each individual unit, perhaps in a production test environment, and
 *    the reference measurements to be stored are the temperature at which the optimisation is made, the contents of the TX_POWER [1E] register and
 *    the contents of the PG_DELAY [2A:0B] register and the contents of the PG_COUNT [2A:08] register. For more information, see App Note APS024
 * 3. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 4. The temperature can be read in a number of ways, such as using an external temperature sensor, or the temperature sensor on the host processor. Here, the temperature is 
 *    read from the DW1000 using this API call. The temperature is in the MSB, and required an adjustment to a real °C value using the formula: 
 *    Treal = (1.13 * Tdw) + 113.0
 *    Note that this is only an approximation; however its accuracy is more than good enough for the adjustments that need to be made for transmit power over temperature.
 ****************************************************************************************************************************************************/
