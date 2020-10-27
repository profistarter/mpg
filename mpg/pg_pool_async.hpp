#include <mutex>
#include <vector>
#include <queue>
#ifdef WIN32
#include <windows.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#endif
#include "pg_connection.h"
#include "../utils/queue.hpp"
#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#include <cstring>// linux for strerror()

/* --------------------------------------------------- */
/*                       HEADER                        */
/* --------------------------------------------------- */

template <typename R, typename ...Args>
class PGPool_Async {
public:
    typedef std::function<R(Args...)> Queue_Fn; 
    typedef std::vector<std::shared_ptr<PGConnection>> Conns_Vector;

private:
    Conns_Vector conns;
    TQueue<Queue_Fn> queue;
    void for_each_conn(std::function<void(std::shared_ptr<PGConnection> conn, const int* sock)> fn);
    void loop(std::shared_ptr<TQueue<Queue_Fn>> _queue, Args... args);

public:
    PGPool_Async(int num_connections);
    void run(std::shared_ptr<TQueue<Queue_Fn>> _queue, Args... args){
        loop(_queue, args...);
    }
};

/* --------------------------------------------------- */
/*                   IMPLEMENTATION                    */
/* --------------------------------------------------- */

template <typename R, typename ...Args>
PGPool_Async<R, Args...>::PGPool_Async(int num_connections)
        : conns(Conns_Vector(num_connections))
        , queue()
{
    Conns_Vector::iterator iter = conns.begin();
    while (iter != conns.end()) {
        *iter = std::make_shared<PGConnection>();
        ++iter;
    }
}

template <typename R, typename ...Args>
void PGPool_Async<R, Args...>::for_each_conn(std::function<void(std::shared_ptr<PGConnection> conn, const int* sock)> fn){
    Conns_Vector::iterator iter = conns.begin();
    while (iter != conns.end()) {
        const int *sock = (*iter)->socket();//PGConnection::socket()
        if (*sock == -1) {
            perror("socket");
        }
        else {
            fn((*iter), sock);
        }
        ++iter;
    }
}

template <typename R, typename ...Args>
void PGPool_Async<R, Args...>::loop(std::shared_ptr<TQueue<Queue_Fn>> _queue, Args... args){
    bool queue_end = false;
    while(!queue_end){
        int nfds = 0;
        fd_set input_mask;
        fd_set output_mask;
        FD_ZERO(&input_mask);
        FD_ZERO(&output_mask);

        for_each_conn([&nfds, &input_mask, &output_mask](std::shared_ptr<PGConnection> conn, const int *sock) mutable {
            FD_SET(*sock, &input_mask);
            FD_SET(*sock, &output_mask);
            nfds = max(nfds, *sock);
        });
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 1;
        if (select(nfds+1, &input_mask, &output_mask, NULL, &timeout) < 0)
        {
            fprintf(stderr, "select() failed: %s\n", strerror(errno));
        }

        for_each_conn([&input_mask, &output_mask, _queue, &queue_end, args...](std::shared_ptr<PGConnection> conn, const int* sock) mutable {
            if (FD_ISSET(*sock, &output_mask)){
                if(conn->is_ready()){
                    Queue_Fn task = _queue->get_task(&queue_end);
                    if(!queue_end){
                        std::string res = task(args...);
                        conn->send(res.c_str(), [](std::vector<std::shared_ptr<PGresult>> results) { //PGConnection::send();
                            auto iter = results.begin();
                            while(iter != results.end()){
                                if((*iter).get() != NULL)
                                {
                                    PQntuples((*iter).get());
                                }
                                ++iter;
                            }
                        });
                    }
                }
            }
            if (FD_ISSET(*sock, &input_mask))
                conn->receive(); //PGConnection::recive();
        });
    }
}