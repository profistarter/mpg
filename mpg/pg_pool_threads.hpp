#ifndef PG_POOL_THREADS_H
#define PG_POOL_THREADS_H
#include <mutex>
#include <thread>
#include <vector>
#include "fakeit/fakeit.hpp"
#include "doctest/doctest.h"
#include <chrono>
#include "../threads/threads.hpp"
#include "pg_connection.h"

template <typename ...Args>
class PGPool_Threads: public Threads<std::string, Args...>
{
private:
    std::vector<std::shared_ptr<PGConnection>> conns;

public:
    typedef std::function<std::string(Args...)> Queue_Fn; 

    PGPool_Threads(int num_threads = std::thread::hardware_concurrency())
        : conns(std::vector<std::shared_ptr<PGConnection>>(num_threads))
        , Threads<std::string, Args...>(num_threads)
    {
        for (int i = 0; i < num_threads; i++) {
            conns[i] = std::make_shared<PGConnection>();
        }
    }

    PGPool_Threads(std::vector<std::shared_ptr<PGConnection>> _conns)
        : conns(_conns)
        , Threads<std::string, Args...>(_conns.size())
    {}

    void wrapper_loop(std::shared_ptr<TQueue<Queue_Fn>> queue, int num_thread, Args... args) override{
        std::shared_ptr<PGConnection> conn = conns[num_thread];

        Threads<std::string, Args...>::loop(queue, [conn, num_thread, args...](Queue_Fn task) {
            // std::cout << std::string(num_thread, '\t') << num_thread << std::endl;
            conn->exec(task(args...).c_str());
        }, args...);
    }
};

/* --------------------------------------------------- */
/*                       TESTS                         */
/* --------------------------------------------------- */

TEST_SUITE("Тест класса PGPool_Threads")
{
    namespace test_pg_pool_threads {
        int num_conns = 3; // Количество соединений.
        int num_tasks = 10; // Количество заданий.
        //----Создаем счетчик вызовов функции-задания и саму функцию-задание
        int static count = 0;
        std::mutex static count_mutex;

        std::string test_task() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(10ms);
            std::lock_guard<std::mutex> guard_mutex(count_mutex);
            count += 1;
            return "";
        }

        TEST_CASE("Количество вызовов и размер очереди") {
            //----Создаем фейковые соединения и помещаем их в вектор
            std::vector<std::shared_ptr<PGConnection>> conns(num_conns);
            fakeit::Mock<PGConnection> conn1;
            fakeit::Fake(Method(conn1, exec));
            //https://github.com/eranpeer/FakeIt/issues/60
            conns[0] = std::shared_ptr<PGConnection>(&conn1.get(), [](...) {});
            
            fakeit::Mock<PGConnection> conn2;
            fakeit::Fake(Method(conn2, exec));
            conns[1] = std::shared_ptr<PGConnection>(&conn2.get(), [](...) {});

            fakeit::Mock<PGConnection> conn3;
            fakeit::Fake(Method(conn3, exec));
            conns[2] = std::shared_ptr<PGConnection>(&conn3.get(), [](...) {});

            //----Создаем объект класса PGPool_Threads и очередь TQueue
            PGPool_Threads pg_pool_threads(conns);
            typedef std::function<std::string(void)> String_Fn;
            std::shared_ptr<TQueue<String_Fn>> queue = std::make_shared<TQueue<String_Fn>>(num_tasks, test_task);


            CHECK(queue->queue_size() == num_tasks);
            pg_pool_threads.run(queue);
            CHECK(count == num_tasks);
            CHECK(queue->queue_size() == 0);
            fakeit::Verify(Method(conn1,exec)).AtLeast(1);
            fakeit::Verify(Method(conn2,exec)).AtLeast(1);
            fakeit::Verify(Method(conn3,exec)).AtLeast(1);
        }

    }
}

#endif // PG_POOL_THREADS_H