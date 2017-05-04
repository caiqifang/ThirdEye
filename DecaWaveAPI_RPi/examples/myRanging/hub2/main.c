#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"

#define MAX_SIZE 1024
#define MIN_SIZE 32

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
static uint32 num_tags_now = 0;
#define TAG_ID_OFFSET 6
#define MY_ID 0x02  // Hub ID


static uint8 tx_init_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x11, 0, 0};
#define MSG_DST_IDX 5
#define MSG_SRC_IDX 7 
#define TWR_MSG_TGT_IDX 6

#define MSG_ALERTTAG_IDX 5
#define MSG_ALERTVAL_IDX 6
    
static uint8 tx_invite_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x12, 0, 0};
static uint8 rx_register_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, MY_ID, 0, 0, 0, 0x13, 0, 0};
static uint8 tx_ack_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x14, 0, 0};
static uint8 tx_twr_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x20, 0, 0};

static uint8 tx_alert_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 0, 0, MY_ID, 0, 0x25, 0, 0};//0x25??

static uint8 rx_dist_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, MY_ID, 0, 0, 0, 0x24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//                                                       dst  src  to  from    64-byte double  
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


void error(const char *msg)
{
    perror(msg);
    exit(0);
}


void check_new_tags(int loopCount) {
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
                num_tags_now++;
                break;
            }
        }
    }
}


// TODO: collecting DW information and put into buffer
void pull_info(char *buffer){
    memset(buffer, 0, MAX_SIZE);
    int responseCount, i, j;
    double distances[NUMBER_OF_BEACONS];
    for (i = 0; i < MAX_NUM_OF_TAGS; i++) {
        if (Tag_IDs[i] != 0) {
            responseCount = 0;
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
                        distances[responseCount] = distance;
                        responseCount++;
                        break;
                    } else {
                        //printf("--- memcmp failed, not sent to me ---\n");
                    }
                }
                  
            }
            if (responseCount == NUMBER_OF_BEACONS) {
                for (j = 0; j < NUMBER_OF_BEACONS; j++) {
                    sprintf(buffer, "%s,777:%d:%d:%d:%d:%d:%f", buffer, 5, num_tags_now, 
                        NUMBER_OF_BEACONS, Tag_IDs[i], Beacon_IDs[j], distances[j]);
                }
            }
        }
    }

    // Write the information to buffer
}

// TODO: send out alert to tags
// char* reply in the format of "ID:0 or 1,ID:0 or 1,..." comma seperated string
// "0" -> no alert
// "1" -> has alert
// Default reply -> ""   empty string, no need to process
void send_alert(char* reply){
    size_t str_len = MIN_SIZE;
    // TODO: implement this function
    //Form "ID:1,ID:0"
    printf("==== send alert ====\n");
    printf("reply is => %s\n",reply);
    char replyBuf[100];
    strcpy(replyBuf,reply);
    int i = 0;
    char *alertMsg = strtok(replyBuf,",");
    char *alertArr[MAX_NUM_OF_TAGS];
    //split ,
    while(alertMsg != NULL) {
        printf("ID with alert: %s\n", alertMsg);
        alertArr[i++] = alertMsg;
        alertMsg = strtok(NULL,",");
    }
    //split :
    for (i = 0; i < MAX_NUM_OF_TAGS; i++) {
        int j = 0;
        int *tagInfo[2];
        char *c = strtok(alertArr[i],":");
        while (c != NULL) {
            tagInfo[j++] = atoi(c);
            printf("c => %s\n", c);
            c = strtok(NULL,":");
        }
        //Send alert message to Tag
        int tagID = tagInfo[0];
        int alertVal = tagInfo[1];
        printf("tagID => %d, alertVal => %d\n", tagID, alertVal);
        tx_alert_msg[MSG_ALERTTAG_IDX] = tagID;
        tx_alert_msg[MSG_ALERTVAL_IDX] = alertVal;
        sendMsg(tx_alert_msg,sizeof(tx_alert_msg));

    }
}

int send_info(char *msg, int sockfd, char *reply){
    int n;
    /* char* msg contains the location information
     * here we send the msg to calculation in python */
    n = write(sockfd, msg, MAX_SIZE);
    if (n < 0){
        error("ERROR writing to socket");
        return 1;
    }
    //Wait for some time?
    /* we have reply from server, if alert occur, send alert to tags */
    n = read(sockfd, reply, MIN_SIZE -1);
    if (n <= 0){
        error("ERROR reading from socket");
        return 1;
    }
    printf("Return message: %s\n", reply);
    /* we have reply from server, if alert occur, send alert to tags */
    send_alert(reply);
    return 0;
}


int main(int argc, char *argv[])
{
    
    char *buffer, *reply;
    buffer = (char *) malloc(MAX_SIZE);
    reply = (char *) malloc(MIN_SIZE);
    memset(reply, 0, MIN_SIZE); 

    /* Global Socket */
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        error("ERROR opening socket");
        exit(0);
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        error("ERROR connecting");
        exit(0);
    }


    /* DecaWave related setup */
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
    int i;
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

    int loopCount = 0;   



    while(1){
        check_new_tags(loopCount);

        pull_info(buffer); // collect information
        

        if(send_info(buffer, sockfd, reply)!= 0){
            close(sockfd);
            fprintf(stderr, "Not Send\n");
        }

        loopCount++;
        deca_sleep(500);
    }
    //free(buffer);
    //free(reply);
    return 1;
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
