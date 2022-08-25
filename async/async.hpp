#ifndef ASYNC_H
#define ASYNC_H
#include <mutex>
#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include "doctest/doctest.h"
#include <chrono>
#include "../utils/queue.hpp"
#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))

/* --------------------------------------------------- */
/*                       HEADER                        */
/* --------------------------------------------------- */

template <typename Socket_Holder, typename R, typename ...Args>
class Async {
public:
    typedef std::function<R(Args...)> Queue_Fn;    
    typedef std::function<void(Socket_Holder, Queue_Fn, ...Args)> Write_Fn;
    typedef std::function<void(Socket_Holder)> Read_Fn;
    typedef std::vector<std::shared_ptr<Socket_Holder>> Socks_Vector;    

    Async(int num_connections);
    Async(Socks_Vector _soket_holders);
    void run(std::shared_ptr<TQueue<Queue_Fn>> queue, Args... args){
        wrapper_loop(queue, args);
    };

private:
    Socks_Vector socket_holders;
    std::shared_ptr<TQueue<Queue_Fn>> queue;
    void for_each_conn(std::function<void(std::shared_ptr<Socket_Holder> sock)> fn);


protected:
    virtual void wrapper_loop(std::shared_ptr<TQueue<Queue_Fn>> queue, Args... args){
        loop(queue, nullptr, args...);
    }
    void loop(std::shared_ptr<TQueue<Queue_Fn>> queue, Write_Fn write_fn, Read_Fn read_fn , Args... args);
};

/* --------------------------------------------------- */
/*                   IMPLEMENTATION                    */
/* --------------------------------------------------- */

template <typename Socket_Holder, typename R, typename ...Args>
Async<Socket_Holder, R, Args...>::Async(int num_connections)
        : socket_holders(Socks_Vector(num_connections))
        , queue()
{
    Socks_Vector::iterator iter = socket_holders.begin();
    while (iter != socket_holders.end()) {
        *iter = std::make_shared<Socket_Holder>();
        ++iter;
    }
}

template <typename Socket_Holder, typename R, typename ...Args>
Async<Socket_Holder, R, Args...>::Async(Socks_Vector _soket_holders)
        : socket_holders(_soket_holders)
        , queue()
{}


template <typename Socket_Holder, typename R, typename ...Args>
void Async<Socket_Holder, R, Args...>::for_each_conn(std::function<void(std::shared_ptr<Socket_Holder> socket_holder)> fn){
    Socks_Vector::iterator iter = socket_holders.begin();
    while (iter != socket_holders.end()) {
        const int *sock_num = (*iter)->socket();//Socket_Holder::socket()
        if (*sock_num == -1) {
            perror("socket");
        }
        else {
            fn((*iter));
        }
        ++iter;
    }
}

template <typename Socket_Holder, typename R, typename ...Args>
void Async<Socket_Holder, R, Args...>::loop(std::shared_ptr<TQueue<Queue_Fn>> queue, Write_Fn write_fn, Read_Fn read_fn, Args... args)
{
    bool queue_end = false;
    while(!queue_end){
        int nfds = 0;
        fd_set input_mask;
        fd_set output_mask;
        FD_ZERO(&input_mask);
        FD_ZERO(&output_mask);

        for_each_conn([&nfds, &input_mask, &output_mask](std::shared_ptr<Socket_Holder> socket_holder) mutable {
            const int *sock = Socket_Holder->socket();
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

        for_each_conn([&input_mask, &output_mask, _queue, &queue_end, args...](std::shared_ptr<Socket_Holder> socket_holder) mutable {
            const int *sock = Socket_Holder->socket();
            if (FD_ISSET(*sock, &output_mask)){
                Queue_Fn task = _queue->get_task(&queue_end);
                if(!queue_end){
                    if(task) {
                        if(write_fn){
                            write_fn(socket_holder, task, args...);
                        }
                        else{
                            //socket_holder.write(task(socket_holder, args...));
                            std::cout << "Write_fn not found." << std::endl;
                        }
                    }
                    else {
                        std::cout << "Task not found." << std::endl;
                    }
                }
            }
            if (FD_ISSET(*sock, &input_mask))
                if(read_fn){
                    read_fn(socket_holder);
                }
                else{
                    //socket_holder.read();
                    std::cout << "Read_fn not found." << std::endl;
                }
        });
    }
}

/* --------------------------------------------------- */
/*                       TESTS                         */
/* --------------------------------------------------- */

TEST_SUITE("Тест класса Async"){
    namespace test_async {
        int static count = 0;
        std::mutex static count_mutex;

        void count_task() {
            std::lock_guard<std::mutex> guard_mutex(count_mutex);
            count += 1;
        }

        Async<void> threads(3);
        typedef std::function<void(void)> Void_Fn;
        std::shared_ptr<TQueue<Void_Fn>> queue = std::make_shared<TQueue<Void_Fn>>(4, count_task);

        TEST_CASE("Количество вызовов и размер очереди") {
            CHECK(queue->queue_size() == 4);

            threads.run(queue);
            CHECK(count == 4);
            CHECK(queue->queue_size() == 0);
        }        
    }
}

#endif // ASYNC_H