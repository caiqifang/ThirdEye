#include <stdio.h>
#include <errno.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#define MAXBUF      1024

int main( int argc, char *argv[]){
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[MAXBUF];

    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    /*---Create streaming socket---*/
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("Socket");
        exit(errno);
    }

    /*---Initialize address/port structure---*/
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
    serv_addr.sin_port = htons(portno);

    /*---Assign a port number to the socket---*/
    if ( bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0 )
    {
        perror("socket--bind");
        exit(errno);
    }

    /*---Make it a "listening socket"---*/
    if ( listen(sockfd, 20) != 0 )
    {
        perror("socket--listen");
        exit(errno);
    }

    /*---Forever... ---*/
    while (1)
    {   int clientfd;
        struct sockaddr_in client_addr;
        int addrlen=sizeof(client_addr);
        size_t r;
        /*---accept a connection (creating a data pipe)---*/
        clientfd = accept(sockfd, (struct sockaddr*)&client_addr,
                                (socklen_t*) &addrlen);
        printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr),
                                        ntohs(client_addr.sin_port));
        r = recv(clientfd, buffer, MAXBUF, 0);

        /*---Echo back anything sent---*/
        send(clientfd, buffer, r, 0);

        /*---Close data connection---*/
        close(clientfd);
    }

    /*---Clean up (should never get here!)---*/
    close(sockfd);
    return 0;
}
