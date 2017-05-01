/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   Double-sided two-way ranging (DS TWR) initiator example code
 *
 *           This is a simple code example which acts as the initiator in a DS TWR distance measurement exchange. This application sends a "poll"
 *           frame (recording the TX time-stamp of the poll), and then waits for a "response" message expected from the "DS TWR responder" example
 *           code (companion to this application). When the response is received its RX time-stamp is recorded and we send a "final" message to
 *           complete the exchange. The final message contains all the time-stamps recorded by this application, including the calculated/predicted TX
 *           time-stamp for the final message itself. The companion "DS TWR responder" example application works out the time-of-flight over-the-air
 *           and, thus, the estimated distance between the two devices.
 *
 * @attention
 *
 * Copyright 2015 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"


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


#define NUMBER_OF_BEACONS 3
#define MAX_NUM_OF_TAGS 4
/* Frames used in the ranging process. See NOTE 2 below. */
static uint8 Beacon_IDs[] = {0x03, 0x04, 0x05};
static uint8 Tag_IDs[MAX_NUM_OF_TAGS] = {0};
#define TAG_ID_OFFSET 6
#define MY_ID 0x02  // Hub ID


static uint8 tx_init_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x11, 0, 0};
#define MSG_DST_IDX 5
#define MSG_SRC_IDX 7 

static uint8 tx_invite_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x12, 0, 0};
static uint8 rx_register_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, MY_ID, 0, 0, 0, 0x13, 0, 0};
static uint8 tx_ack_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x14, 0, 0};
static uint8 tx_twr_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x20, 0, 0};
#define TWR_MSG_TGT_IDX 6
// static uint8 tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21, 0, 0};
// static uint8 rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0, 0, 0};
// static uint8 tx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8 rx_dist_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, MY_ID, 0, 0, 0, 0x24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//                                                       dst  src  to  from    64-byte double  
//static uint8 tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x10, 0x02, 0, 0, 0, 0};
//static uint8 rx_final_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, MY_ID, 0, 0, 0, 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Length of the common part of the message (up to and including the function code, see NOTE 2 below). */
#define MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX 2
#define DIST_MSG_DST_IDX 5
#define DIST_MSG_SRC_IDX 6
#define DIST_MSG_TO_IDX 7
#define DIST_MSG_FRM_IDX 8
#define MSG_FCN_IDX 9
#define DIST_MSG_DATA_IDX 10
//#define FINAL_MSG_POLL_TX_TS_IDX 10
//#define FINAL_MSG_RESP_RX_TS_IDX 14
//#define FINAL_MSG_FINAL_TX_TS_IDX 18
//#define FINAL_MSG_TS_LEN 4
/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Buffer to store received messages.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 24
static uint8 rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

static uint16 receive_timeout = 8000;


static void sendMsg(uint8 tx_msg[], size_t size);
static uint32 receiveMsg(uint16 timeout);


void printMsg(uint8 msg[], size_t size) {
    int i;
    for (i = 0; i < size; i++) {
        printf("%x ", msg[i]);
    }
    printf("\n");
}


/*! ------------------------------------------------------------------------------------------------------------------
 * @fn main()
 *
 * @brief Application entry point.
 *
 * @param  none
 *
 * @return none
 */
int main(void)
{
    openspi();

    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR)
    {
        printf("DW initialise failed\n");
        return 1;
    }

    /* Configure DW1000. See NOTE 3 below. */
    dwt_configure(&config);
    printf("DW configure succeeds\n");
    
    set_spi_rate_high();

    /* Set preamble timeout for expected frames. See NOTE 6 below. */
    //dwt_setpreambledetecttimeout(PRE_TIMEOUT);
    int i,j;
    printf("------- Init with beacons ----------\n");
    while (1)
    {
        for (i = 0; i < NUMBER_OF_BEACONS; i++) {
            tx_init_msg[MSG_DST_IDX] = Beacon_IDs[i];
            sendMsg(tx_init_msg, sizeof(tx_init_msg));
            deca_sleep(50);
        }

        deca_sleep(500);

        break;
    }

    printf("------ Entering normal loop ------\n"); 

    uint32 loopCount = 0;   
    /* Loop forever responding to ranging requests. */
    while (1)
    {
        for (i = 0; i < MAX_NUM_OF_TAGS; i++) {
            if (Tag_IDs[i] != 0) {
                for (j = 0; j < NUMBER_OF_BEACONS; j++) {
                    // send do 2wr command
                    tx_twr_msg[MSG_DST_IDX] = Tag_IDs[i];
                    tx_twr_msg[TWR_MSG_TGT_IDX] = Beacon_IDs[j];
                    sendMsg(tx_twr_msg, sizeof(tx_twr_msg));
                    // receive distance data
                    rx_dist_msg[DIST_MSG_SRC_IDX] = Beacon_IDs[j];
                    rx_dist_msg[DIST_MSG_TO_IDX] = Beacon_IDs[j];
                    rx_dist_msg[DIST_MSG_FRM_IDX] = Tag_IDs[i];
                    while (receiveMsg(receive_timeout) == 0) {
                        rx_buffer[ALL_MSG_SN_IDX] = 0;
                        
                        if (memcmp(rx_buffer, rx_dist_msg, MSG_COMMON_LEN) == 0)
                        {
                            double distance;
                            memcpy(&distance, &(rx_buffer[DIST_MSG_DATA_IDX]), sizeof(double));
                            printf("--- MSG ---\n");
                            printf("Distance between %d and %d is %f\n", rx_buffer[DIST_MSG_FRM_IDX], rx_buffer[DIST_MSG_TO_IDX], distance);
                            break;
                        } else {
                            //printf("--- memcmp failed, not sent to me ---\n");
                        }
                    } 
                    
                    
                }
            }
        }

        // Check for new tags
        if (loopCount % 3 == 0) {
            // send invite msg
            sendMsg(tx_invite_msg, sizeof(tx_invite_msg));
            // receive register msg with timeout
            while (receiveMsg(receive_timeout) == 0) {
                rx_buffer[ALL_MSG_SN_IDX] = 0;
                uint8 id = rx_buffer[MSG_SRC_IDX];
                rx_buffer[MSG_SRC_IDX] = 0;
                if (memcmp(rx_buffer, rx_register_msg, MSG_COMMON_LEN) == 0)
                {
                    Tag_IDs[id - TAG_ID_OFFSET] = id;
                
                    // send ack
                    tx_ack_msg[MSG_DST_IDX] = id;
                    sendMsg(tx_ack_msg, sizeof(tx_ack_msg));
                    break;
                }
            }
        }
        loopCount++;

        deca_sleep(500);

    }
        
}


static void sendMsg(uint8 tx_msg[], size_t size)
{
    //printf("Send msg: ");
    //printMsg(tx_msg, size);
    int res = dwt_writetxdata(size, tx_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(size, 0, 0); /* Zero offset in TX buffer, no ranging. */

    // Clear status flags
    dwt_write32bitreg(SYS_STATUS_ID, (SYS_STATUS_ALL_TX | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_GOOD));

    /* Start transmission. */
    res = dwt_starttx(DWT_START_TX_IMMEDIATE);

    /* Poll DW1000 until TX frame sent event set. See NOTE 5 below.
     * STATUS register is 5 bytes long but, as the event we are looking at is in the first byte of the register, we can use this simplest API
     * function to access it.*/
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_TXFRS))
    {}

    /* Clear TX frame sent event. */
    dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
    //printf("Send done\n");
}


static uint32 receiveMsg(uint16 timeout)
{
    /* Clear reception timeout to start next ranging process. */
    dwt_setrxtimeout(timeout);

    /* Activate reception immediately. */
    dwt_rxenable(DWT_START_RX_IMMEDIATE);
    //printf("in receiveMsg\n");
    /* Poll for reception of a frame or error/timeout. See NOTE 8 below. */
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
    {}
    //printf("while done\n");
    if (status_reg & SYS_STATUS_RXFCG)
    {
        uint32 frame_len;
        uint8 target_id;
        /* Clear good RX frame event in the DW1000 status register. */
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

        /* A frame has been received, read it into the local buffer. */
        frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
        if (frame_len <= RX_BUF_LEN)
        {
            dwt_readrxdata(rx_buffer, frame_len, 0);
            //printf("Received MSG\n");
        }
        return 0;
    }
    else
    {
         /* Clear RX error/timeout events in the DW1000 status register. */
        //dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_TX | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

        /* Reset RX to properly reinitialise LDE operation. */
        dwt_rxreset();
        //printf("Receive timeout\n");
        return 1;
    }
}

/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The sum of the values is the TX to RX antenna delay, experimentally determined by a calibration process. Here we use a hard coded typical value
 *    but, in a real application, each device should have its own antenna delay properly calibrated to get the best possible precision when performing
 *    range measurements.
 * 2. The messages here are similar to those used in the DecaRanging ARM application (shipped with EVK1000 kit). They comply with the IEEE
 *    802.15.4 standard MAC data frame encoding and they are following the ISO/IEC:24730-62:2013 standard. The messages used are:
 *     - a poll message sent by the initiator to trigger the ranging exchange.
 *     - a response message sent by the responder allowing the initiator to go on with the process
 *     - a final message sent by the initiator to complete the exchange and provide all information needed by the responder to compute the
 *       time-of-flight (distance) estimate.
 *    The first 10 bytes of those frame are common and are composed of the following fields:
 *     - byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
 *     - byte 2: sequence number, incremented for each new frame.
 *     - byte 3/4: PAN ID (0xDECA).
 *     - byte 5/6: destination address, see NOTE 3 below.
 *     - byte 7/8: source address, see NOTE 3 below.
 *     - byte 9: function code (specific values to indicate which message it is in the ranging process).
 *    The remaining bytes are specific to each message as follows:
 *    Poll message:
 *     - no more data
 *    Response message:
 *     - byte 10: activity code (0x02 to tell the initiator to go on with the ranging exchange).
 *     - byte 11/12: activity parameter, not used for activity code 0x02.
 *    Final message:
 *     - byte 10 -> 13: poll message transmission timestamp.
 *     - byte 14 -> 17: response message reception timestamp.
 *     - byte 18 -> 21: final message transmission timestamp.
 *    All messages end with a 2-byte checksum automatically set by DW1000.
 * 3. Source and destination addresses are hard coded constants in this example to keep it simple but for a real product every device should have a
 *    unique ID. Here, 16-bit addressing is used to keep the messages as short as possible but, in an actual application, this should be done only
 *    after an exchange of specific messages used to define those short addresses for each device participating to the ranging exchange.
 * 4. Delays between frames have been chosen here to ensure proper synchronisation of transmission and reception of the frames between the initiator
 *    and the responder and to ensure a correct accuracy of the computed distance. The user is referred to DecaRanging ARM Source Code Guide for more
 *    details about the timings involved in the ranging process.
 * 5. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *    is arbitrary but chosen large enough to make sure that there is enough time to receive the complete final frame sent by the responder at the
 *    110k data rate used (around 3.5 ms).
 * 6. The preamble timeout allows the receiver to stop listening in situations where preamble is not starting (which might be because the responder is
 *    out of range or did not receive the message to respond to). This saves the power waste of listening for a message that is not coming. We
 *    recommend a minimum preamble timeout of 5 PACs for short range applications and a larger value (e.g. in the range of 50% to 80% of the preamble
 *    length) for more challenging longer range, NLOS or noisy environments.
 * 7. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 8. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW1000 User Manual for more details on "interrupts". It is also to be noted that STATUS register is 5 bytes long but, as the event we
 *    use are all in the first bytes of the register, we can use the simple dwt_read32bitreg() API call to access it instead of reading the whole 5
 *    bytes.
 * 9. Timestamps and delayed transmission time are both expressed in device time units so we just have to add the desired response delay to poll RX
 *    timestamp to get response transmission time. The delayed transmission time resolution is 512 device time units which means that the lower 9 bits
 *    of the obtained value must be zeroed. This also allows to encode the 40-bit value in a 32-bit words by shifting the all-zero lower 8 bits.
 * 10. dwt_writetxdata() takes the full size of the message as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *     automatically appended by the DW1000. This means that our variable could be two bytes shorter without losing any data (but the sizeof would not
 *     work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 11. When running this example on the EVB1000 platform with the POLL_RX_TO_RESP_TX_DLY response delay provided, the dwt_starttx() is always
 *     successful. However, in cases where the delay is too short (or something else interrupts the code flow), then the dwt_starttx() might be issued
 *     too late for the configured start time. The code below provides an example of how to handle this condition: In this case it abandons the
 *     ranging exchange and simply goes back to awaiting another poll message. If this error handling code was not here, a late dwt_starttx() would
 *     result in the code flow getting stuck waiting subsequent RX event that will will never come. The companion "initiator" example (ex_05a) should
 *     timeout from awaiting the "response" and proceed to send another poll in due course to initiate another ranging exchange.
 * 12. The high order byte of each 40-bit time-stamps is discarded here. This is acceptable as, on each device, those time-stamps are not separated by
 *     more than 2**32 device time units (which is around 67 ms) which means that the calculation of the round-trip delays can be handled by a 32-bit
 *     subtraction.
 * 13. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *     DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/
