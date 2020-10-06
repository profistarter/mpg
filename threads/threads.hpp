#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include "doctest/doctest.h"
#include <chrono>

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
    std::mutex queue_mutex;
    std::vector<Queue_Fn> queue;

    Queue_Fn get_task(bool* queue_end);

public:
    Threads(int _num_threads);
    void add_queue(std::vector<Queue_Fn> _queue_async);
    void run(Args... args);
    unsigned int queue_size();

protected:
    virtual void wrapper_loop(int num_thread, Args... args){
        loop(nullptr, args...);
    }
    void loop(Loop_Fn _fn, Args... args);
};

/* --------------------------------------------------- */
/*                   IMPLEMENTATION                    */
/* --------------------------------------------------- */

template <typename R, typename ...Args>
Threads<R, Args...>::Threads(int _num_threads = std::thread::hardware_concurrency())
    : queue()
    , num_threads(_num_threads) {};

template <typename R, typename ...Args>
void Threads<R, Args...>::add_queue(std::vector<Queue_Fn> _queue)
{
    queue_mutex.lock();
    queue.insert(queue.end(), _queue.begin(), _queue.end());
    queue_mutex.unlock();
}

template <typename R, typename ...Args>
void Threads<R, Args...>::run(Args... args)
{
    std::vector<std::thread> threads(num_threads);
    std::vector<std::thread>::iterator iter = threads.begin();
    short i = 0;
    while(iter != threads.end()){
        *iter = std::thread(&Threads::wrapper_loop,
            std::ref(*this), i++, args...);
        ++iter;
    }
    iter = threads.begin();
    while(iter != threads.end()){
        (*iter).join();
        ++iter;
    }
}

template <typename R, typename ...Args>
unsigned int Threads<R, Args...>::queue_size()
{
    std::lock_guard<std::mutex> guard_mutex(queue_mutex);
    return queue.size();
}

template <typename R, typename ...Args>
void Threads<R, Args...>::loop(Loop_Fn _fn, Args... args)
{
    Queue_Fn task;
    bool queue_end = false;
    while (!queue_end) {
        task = get_task(&queue_end);
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

template <typename R, typename ...Args>
std::function<R(Args...)> Threads<R, Args...>::get_task(bool *queue_end){
    Queue_Fn task = nullptr;
    {
        queue_mutex.lock();
        if (queue.size() != 0) {
            task = queue.front();
            queue.erase(queue.begin());
        } else {
            *queue_end = true;
        }
        queue_mutex.unlock();
    }
    return task;
}

/* --------------------------------------------------- */
/*                       TESTS                         */
/* --------------------------------------------------- */

TEST_SUITE("Тест класса Threads"){
    int static count = 0;
    std::mutex static count_mutex;
    void count_task() {
        count_mutex.lock();
        count += 1;
        count_mutex.unlock();
    }
    Threads<void> threads(3);
    std::vector<Threads<void>::Queue_Fn> queue (4, count_task);

    TEST_CASE("Количество вызовов и размер очереди") {
        threads.add_queue(queue);
        CHECK(threads.queue_size() == 4);

        threads.run();
        CHECK(count == 4);
        CHECK(threads.queue_size() == 0);
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
        Test_Threads(int num_threads)
            : Threads<int, int>(num_threads) {};
    protected:
        void wrapper_loop(int num_thread, int arg) override{
            Threads<int, int>::loop([num_thread](Queue_Fn task) {
                int res = task(num_thread);
                // CHECK(res == num_thread);
            }, num_thread);
        }
    };

    const int num_tasks = 100; // количество заданий
    Test_Threads test_threads{num_threads};
    std::vector<Test_Threads::Queue_Fn> queue2(num_tasks, counts_task);

    TEST_CASE("Равномерность выполнения потоков и размер очереди") {
        test_threads.add_queue(queue2);
        CHECK(test_threads.queue_size() == num_tasks);// Проверяем размер очереди перед выполнением

        test_threads.run(num_tasks);
        for (int i = 0; i < counts.size(); ++i){
            CHECK(counts[i] > 0);// Проверяем счетчик выполненных заданий каждого потока
        }
        CHECK(test_threads.queue_size() == 0);// Проверяем размер очереди после выполнения
    }
}