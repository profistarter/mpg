#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //INADDR_LOOPBACK
#include <stdio.h>      /* perror, printf, fopen */
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <unistd.h>     //close()
#include <iostream>     //std::string

char buf[1024];

int main()
{
    int sock;
    struct sockaddr_in addr;
    int bytes_read;

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

    char message1[] = "First";
    std::cout << "send 1\n";
    send(sock, message1, sizeof(message1), 0);
    char message2[] = "Second";
    std::cout << "send 2\n";
    send(sock, message2, sizeof(message2), 0);
    std::cout << "recv 1: ";
    bytes_read = recv(sock, buf, sizeof(message1), 0);
    std::cout << bytes_read << " " << buf << "\n";
    std::cout << "recv 2: ";
    bytes_read = recv(sock, buf, sizeof(message2), 0);
    std::cout << bytes_read << " " << buf << "\n";
    
    close(sock);

    return 0;
}