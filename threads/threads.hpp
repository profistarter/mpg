#ifndef THREADS_H
#define THREADS_H
#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include "doctest/doctest.h"
#include <chrono>
#include "../utils/queue.hpp"

/* --------------------------------------------------- */
/*                       HEADER                        */
/* --------------------------------------------------- */

template <typename R, typename ...Args>
class Threads {
public:
    typedef std::function<R(Args...)> Queue_Fn;    
    typedef std::function<void(Queue_Fn)> Loop_Fn;

private:
    int num_threads;

public:
    Threads(int _num_threads = std::thread::hardware_concurrency());
    void run(std::shared_ptr<TQueue<Queue_Fn>> queue, Args... args);

protected:
    virtual void wrapper_loop(std::shared_ptr<TQueue<Queue_Fn>> queue, int num_thread, Args... args){
        loop(queue, nullptr, args...);
    }
    void loop(std::shared_ptr<TQueue<Queue_Fn>> queue, Loop_Fn _fn, Args... args);
};

/* --------------------------------------------------- */
/*                   IMPLEMENTATION                    */
/* --------------------------------------------------- */

template <typename R, typename ...Args>
Threads<R, Args...>::Threads(int _num_threads)
    : num_threads(_num_threads) {};

template <typename R, typename ...Args>
void Threads<R, Args...>::run(std::shared_ptr<TQueue<Queue_Fn>> queue, Args... args)
{
    std::vector<std::thread> threads(num_threads);
    std::vector<std::thread>::iterator iter = threads.begin();
    short i = 0;
    while(iter != threads.end()){
        *iter = std::thread(&Threads::wrapper_loop,
            std::ref(*this), queue, i++, args...);
        ++iter;
    }
    iter = threads.begin();
    while(iter != threads.end()){
        (*iter).join();
        ++iter;
    }
}

template <typename R, typename ...Args>
void Threads<R, Args...>::loop(std::shared_ptr<TQueue<Queue_Fn>> queue, Loop_Fn _fn, Args... args)
{
    bool queue_end = false;
    while (!queue_end) {
        Queue_Fn task = queue->get_task(&queue_end);
        if (!queue_end) {                
            if(task) {
                if(_fn){
                    _fn(task);
                }
                else{
                    task(args...);
                }
            }
            else {
                std::cout << "Task not found." << std::endl;
            }
        }
    }
}

/* --------------------------------------------------- */
/*                       TESTS                         */
/* --------------------------------------------------- */

TEST_SUITE("Тест класса Threads"){
    namespace test_threads {
        int static count = 0;
        std::mutex static count_mutex;

        void count_task() {
            std::lock_guard<std::mutex> guard_mutex(count_mutex);
            count += 1;
        }

        Threads<void> threads(3);
        typedef std::function<void(void)> Void_Fn;
        std::shared_ptr<TQueue<Void_Fn>> queue = std::make_shared<TQueue<Void_Fn>>(4, count_task);

        TEST_CASE("Количество вызовов и размер очереди") {
            CHECK(queue->queue_size() == 4);

            threads.run(queue);
            CHECK(count == 4);
            CHECK(queue->queue_size() == 0);
        }

        int num_threads = 3; // количество потоков
        std::vector<int> static counts(num_threads, 0);
        std::mutex static counts_mutex;

        int counts_task(int i) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(10ms);
            std::lock_guard<std::mutex> guard_mutex(counts_mutex);
            counts[i] += 1;
            return i;
        }

        class Test_Threads: public Threads<int, int> {
        public:
            typedef Threads<int, int>::Queue_Fn Test_Fn;
            Test_Threads(int num_threads)
                : Threads<int, int>(num_threads) {};
        protected:
            void wrapper_loop(std::shared_ptr<TQueue<Test_Fn>> queue, int num_thread, int arg) override{
                Threads<int, int>::loop(queue, [num_thread](Queue_Fn task) {
                    int res = task(num_thread);
                    // CHECK(res == num_thread);
                }, num_thread);
            }
        };

        const int num_tasks = 100; // количество заданий
        Test_Threads test_threads{num_threads};
        std::shared_ptr<TQueue<Test_Threads::Test_Fn>> queue2 = std::make_shared<TQueue<Test_Threads::Test_Fn>>(num_tasks, counts_task);

        TEST_CASE("Равномерность выполнения потоков и размер очереди") {
            CHECK(queue2->queue_size() == num_tasks);

            test_threads.run(queue2, num_tasks);
            for (int i = 0; i < counts.size(); ++i){
                CHECK(counts[i] > 0);
            }
            CHECK(queue2->queue_size() == 0);
        }
    }
}

#endif // THREADS_H