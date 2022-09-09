#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <set>
#include <stdio.h>      /* perror, printf, fopen */
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <unistd.h>     //close()
#include <iostream>     //std::string
#include <vector>
#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))

using namespace std;

void print_buf(char *buf, int buf_len){
    std::cout << "recieve " << buf_len << " '";
    for(int i=0;i<13;++i){
        std::cout << buf[i]; 
    }                
    std::cout << "'" << "\n";
}

int main()
{
    int listener;
    struct sockaddr_in addr;
    char buf[1024];
    int bytes_read;
    int mx = 0;
    bool is_close;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        perror("socket");
        exit(1);
    }
    
    fcntl(listener, F_SETFL, O_NONBLOCK);
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }

    if(listen(listener, 2) < 0)
    {
        perror("listen");
        exit(2);
    }
    
    std::vector<int> clients(0);
    while(1)
    {        
        // Заполняем множество сокетов
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(listener, &readset);

        // std::cout << "1 Before while(iter != clients.end())\n";
        mx = max(mx, listener);
        std::vector<int>::iterator iter = clients.begin();
        while (iter != clients.end()) {
            FD_SET(*iter, &readset);
            mx = max(mx, *iter);
            ++iter;
        }
        // std::cout << "1 After while(iter != clients.end())\n";

        // Задаём таймаут
        timeval timeout;
        timeout.tv_sec = 1; 
        timeout.tv_usec = 0;

        // Ждём события в одном из сокетов
        if(select(mx+1, &readset, NULL, NULL, &timeout) < 0)
        {
            perror("select");
            exit(3);
        }
        
        iter = clients.begin();    
        while (iter != clients.end()){
            is_close = false;
            if(FD_ISSET(*iter, &readset))
            {
                // Поступили данные от клиента, читаем их
                print_buf(buf, bytes_read);
                bytes_read = recv(*iter, buf, 13, 0);
                print_buf(buf, bytes_read);

                if(bytes_read <= 0)
                {
                    // Соединение разорвано, удаляем сокет из множества
                    is_close = true;
                }
                // Отправляем данные обратно клиенту
                std::cout << "send " << bytes_read << " " << " '" << buf << "'" << "\n";
                send(*iter, buf, bytes_read, 0);
            }
            if(is_close) {
                close(*iter);
                iter = clients.erase(iter);
                std::cout << "close connection\n";
                is_close = false;
            }
            else {
                ++iter;
            }
        }

        // Определяем тип события и выполняем соответствующие действия
        if(FD_ISSET(listener, &readset))
        {
            // Поступил новый запрос на соединение, используем accept
            int sock = accept(listener, NULL, NULL);
            if(sock < 0)
            {
                perror("accept");
                exit(3);
            }
            
            fcntl(sock, F_SETFL, O_NONBLOCK);
            std::cout << "new connection\n\n";
            clients.push_back(sock);
        }
    }
    
    return 0;
}
