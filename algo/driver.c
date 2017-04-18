#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_SIZE 1024
#define MIN_SIZE 256

void error(const char *msg)
{
    perror(msg);
    exit(0);
}


// TODO: collecting DW information and put into buffer
void pull_info(char *buffer){
    memset(buffer, 0, MAX_SIZE);
    // change this test
    fgets(buffer,255,stdin);
}

// TODO: area alert

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
    n = write(sockfd, msg, MAX_SIZE);
    if (n < 0)
        error("ERROR writing to socket");
    n = read(sockfd, reply, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("Return message: %s\n", reply);
    close(sockfd);
    return;
}


int main(int argc, char *argv[])
{
    char *buffer, *reply;
    buffer = (char *) malloc(MAX_SIZE);
    reply = (char *) malloc(256);
    while(1){
        pull_info(buffer);
        send_info(buffer, argc, argv, reply);
        // TODO: process server reply, might have to fire alert to tag
    }
    free(buffer);
    free(reply);
}


