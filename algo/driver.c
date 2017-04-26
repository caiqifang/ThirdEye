#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_SIZE 1024
#define MIN_SIZE 32

void error(const char *msg)
{
    perror(msg);
    exit(0);
}


// TODO: collecting DW information and put into buffer
void pull_info(char *buffer){
    memset(buffer, 0, MAX_SIZE);
    // TODO: rewrite this function
    // TODO: remove this, this is for test purpose
    fgets(buffer,255,stdin);
}

// TODO: send out alert to tags
// server return 0 -> no alert
// server return 1 -> has alert
void send_alert(char* reply){
    size_t str_len = MIN_SIZE;
    // TODO: implement this function
    // if alert is true, send alert
    // else ignore reply
}

void send_info(char *msg, int argc, char *argv[], char *reply){
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    memset(reply, 0, MIN_SIZE);
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
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
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    /* char* msg contains the location information
     * here we send the msg to calculation in python */
    n = write(sockfd, msg, MAX_SIZE);

    if (n < 0)
        error("ERROR writing to socket");

    /* we have reply from server, if alert occur, send alert to tags */
    n = read(sockfd, reply, MIN_SIZE -1);
    if (n < 0)
        error("ERROR reading from socket");
    printf("Return message: %s\n", reply);
    /* we have reply from server, if alert occur, send alert to tags */
    send_alert(reply);

    close(sockfd);
    return;
}


int main(int argc, char *argv[])
{
    char *buffer, *reply;
    buffer = (char *) malloc(MAX_SIZE);
    reply = (char *) malloc(MIN_SIZE);
    while(1){
        pull_info(buffer); // collect information
        send_info(buffer, argc, argv, reply);
    }
    free(buffer);
    free(reply);
}


