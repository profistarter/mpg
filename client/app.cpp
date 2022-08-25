#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //INADDR_LOOPBACK
#include <stdio.h>      /* perror, printf, fopen */
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <unistd.h>     //close()
#include <iostream>     //std::string

char message[] = "Hello there!\n";
char buf[sizeof(message)];

int main()
{
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425); // или любой другой порт...
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(2);
    }

    send(sock, message, sizeof(message), 0);
    recv(sock, buf, sizeof(message), 0);
    
    std::cout << buf;
    close(sock);

    return 0;
}