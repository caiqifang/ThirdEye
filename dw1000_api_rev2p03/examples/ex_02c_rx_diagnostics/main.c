/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   Simple RX with diagnostics example code
 *
 *           This application waits for reception of a frame. After each frame received with a good CRC it reads some data provided by DW1000:
 *               - Diagnostics data (e.g. first path index, first path amplitude, channel impulse response, etc.). See dwt_rxdiag_t structure for more
 *                 details on the data read.
 *               - Accumulator values around the first path.
 *           It also reads event counters (e.g. CRC good, CRC error, PHY header error, etc.) after any event, be it a good frame or an RX error. See
 *           dwt_deviceentcnts_t structure for more details on the counters read.
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
#include "lcd.h"
#include "port.h"

/* Example application name and version to display on LCD screen. */
#define APP_NAME "RX DIAG v1.0"

/* Default communication configuration. We use here EVK1000's default mode (mode 3). */
static dwt_config_t config = {
    2,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_1024,   /* Preamble length. Used in TX only. */
    DWT_PAC32,       /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_110K,     /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (1025 + 64 - 32) /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

/* Buffer to store received frame. See NOTE 1 below. */
#define FRAME_LEN_MAX 127
static uint8 rx_buffer[FRAME_LEN_MAX];

/* Hold copy of status register state here for reference, so reader can examine it at a breakpoint. */
static uint32 status_reg = 0;

/* Hold copy of frame length of frame received (if good), so reader can examine it at a breakpoint. */
static uint16 frame_len = 0;

/* Hold copy of event counters so that it can be examined at a debug breakpoint. */
static dwt_deviceentcnts_t event_cnt;

/* Hold copy of diagnostics data so that it can be examined at a debug breakpoint. */
static dwt_rxdiag_t rx_diag;

/* Hold copy of accumulator data so that it can be examined at a debug breakpoint. See NOTE 2. */
#define ACCUM_DATA_LEN (2 * 2 * (3 + 3) + 1)
static uint8 accum_data[ACCUM_DATA_LEN];

/**
 * Application entry point.
 */
int main(void)
{
    /* Start with board specific hardware init. */
    peripherals_init();

    /* Display application name on LCD. */
    lcd_display_str(APP_NAME);

    /* Reset and initialise DW1000. See NOTE 3 below.
     * For initialisation, DW1000 clocks must be temporarily set to crystal speed. After initialisation SPI rate can be increased for optimum
     * performance. */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    spi_set_rate_low();
    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR)
    {
        lcd_display_str("INIT FAILED");
        while (1)
        { };
    }
    spi_set_rate_high();

    /* Configure DW1000. */
    dwt_configure(&config);

    /* Activate event counters. */
    dwt_configeventcounters(1);

    /* Loop forever receiving frames. */
    while (1)
    {
        int i;

        /* TESTING BREAKPOINT LOCATION #1 */

        /* Clear local RX buffer, rx_diag structure and accumulator values to avoid having leftovers from previous receptions  This is not necessary
         * but is included here to aid reading the data for each new frame.
         * This is a good place to put a breakpoint. Here (after first time through the loop) the local status register will be set for last event
         * and if a good receive has happened the data buffer will have the data in it, and frame_len will be set to the length of the RX frame. All
         * diagnostics data will also be available. */
        for (i = 0 ; i < FRAME_LEN_MAX; i++ )
        {
            rx_buffer[i] = 0;
        }
        for (i = 0 ; i < ACCUM_DATA_LEN; i++ )
        {
           accum_data[i] = 0;
        }
        rx_diag.firstPath = 0;
        rx_diag.firstPathAmp1 = 0;
        rx_diag.firstPathAmp2 = 0;
        rx_diag.firstPathAmp3 = 0;
        rx_diag.maxGrowthCIR = 0;
        rx_diag.rxPreamCount = 0;
        rx_diag.maxNoise = 0;
        rx_diag.stdNoise = 0;

        /* Activate reception immediately. See NOTE 4 below. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll until a frame is properly received or an error/timeout occurs. See NOTE 5 below.
         * STATUS register is 5 bytes long but, as the event we are looking at is in the first byte of the register, we can use this simplest API
         * function to access it. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR)))
        { };

        if (status_reg & SYS_STATUS_RXFCG)
        {
            /* Clear good RX frame event in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

            /* A frame has been received, copy it to our local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
            if (frame_len <= FRAME_LEN_MAX)
            {
                dwt_readrxdata(rx_buffer, frame_len, 0);
            }

            /* Read diagnostics data. */
            dwt_readdiagnostics(&rx_diag);

            /* Read accumulator. See NOTES 2 and 6. */
            uint16 fp_int = rx_diag.firstPath / 64;
            dwt_readaccdata(accum_data, ACCUM_DATA_LEN, (fp_int - 2) * 4);
        }
        else
        {
            /* Clear RX error events in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);

            /* Reset RX to properly reinitialise LDE operation. */
            dwt_rxreset();
        }

        /* Read event counters. See NOTE 7. */
        dwt_readeventcounters(&event_cnt);
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. In this example, maximum frame length is set to 127 bytes which is 802.15.4 UWB standard maximum frame length. DW1000 supports an extended
 *    frame length (up to 1023 bytes long) mode which is not used in this example.
 * 2. Accumulator values are complex numbers: one 16-bit integer for real part and one 16-bit value for imaginary part, for each sample. In this
 *    example, we chose to read 3 values below the first path index and 3 values above. It must be noted that the first byte read when accessing the
 *    accumulator memory is always garbage and must be discarded, that is why the data length to read is increased by one byte here.
 * 3. In this example, LDE microcode is loaded even if timestamps are not used because diagnostics values are computed during LDE execution. If LDE is
 *    not loaded and running, dwt_readdiagnostics will return all 0 values.
 * 4. Manual reception activation is performed here but DW1000 offers several features that can be used to handle more complex scenarios or to
 *    optimise system's overall performance (e.g. timeout after a given time, automatic re-enabling of reception in case of errors, etc.).
 * 5. We use polled mode of operation here to keep the example as simple as possible but RXFCG and error/timeout status events can be used to generate
 *    interrupts. Please refer to DW1000 User Manual for more details on "interrupts".
 * 6. Here we chose to read only a few values around the first path index but it is possible and can be useful to get all accumulator values, using
 *    the relevant offset and length parameters. Reading the whole accumulator will require 4064 bytes of memory. First path value gotten from
 *    dwt_readdiagnostics is a 10.6 bits fixed point value calculated by the DW1000. By dividing this value by 64, we end up with the integer part of
 *    it. This value can be used to access the accumulator samples around the calculated first path index as it is done here.
 * 7. Event counters are never reset in this example but this can be done by re-enabling them (i.e. calling again dwt_configeventcounters with
 *    "enable" parameter set).
 * 8. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/
