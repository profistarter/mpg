#define DOCTEST_CONFIG_DISABLE
#include "doctest/doctest.h"
#include <vector>
#include <functional>
#include <iostream>
#include "pg_pool_threads.hpp"
#include "../utils/queue.hpp"
#include "../utils/utils.h"
#include "pg_pool_async.hpp"

//https://ravesli.com/urok-129-tajming-koda-vremya-vypolneniya-programmy/
class Timer {
private:
    using clock_t = std::chrono::high_resolution_clock;
    using second_t = std::chrono::duration<double, std::ratio<1>>;

    std::chrono::time_point<clock_t> m_beg;

public:
    Timer() 
        : m_beg(clock_t::now()) {}

    void reset() {
        m_beg = clock_t::now();
    }

    double elapsed() const {
        return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
    }
};

enum class Weight {
    little = 1,
    medium = 1000,
    large = 2000
};

std::string str_weght(const Weight &weight) {
    std::string weight_str;
    switch (weight) {
        case Weight::little: weight_str.append("small");
            break;  
        case Weight::medium: weight_str.append("medium");
            break;
        case Weight::large: weight_str.append("large");
            break;            
    }
    return weight_str;
}

std::string myFunc(const Weight &weight) {
    std::string str = str_weght(weight);
    return "SELECT SUM(a_field) + SUM(b_field) FROM " + str + "_table";
}

typedef std::function<std::string(Weight)> String_Fn;
std::shared_ptr<TQueue<String_Fn>> get_queue(const int &num){
    std::shared_ptr<TQueue<String_Fn>> queue = std::make_shared<TQueue<String_Fn>>();
    int i = 0;
    while (i < num) {
        queue->push(myFunc);
        ++i;
    }
    return queue;
}

void prepare(const Weight &weight){
    std::string weight_str = str_weght(weight);
    PGConnection conn;
    conn.exec(std::string("DROP TABLE IF EXISTS " + weight_str + "_table").c_str());
    conn.exec(std::string("CREATE TABLE IF NOT EXISTS " + weight_str + "_table (       \
                    a_field bigint,  \
                    b_field bigint   \
                )").c_str());
    for (int i = 0; i < static_cast<int>(weight); i++) {
        conn.exec(std::string("INSERT INTO " + weight_str + "_table (a_field, b_field) VALUES   \
                    (1000000, 1000000),    \
                    (2000000, 2000000),    \
                    (3000000, 3000000),    \
                    (1000000, 1000000),    \
                    (2000000, 2000000),    \
                    (3000000, 3000000),    \
                    (1000000, 1000000),    \
                    (2000000, 2000000),    \
                    (3000000, 3000000),    \
                    (1000000, 1000000);    \
                ").c_str());
    }
}

void sync(const int &num_query, const Weight &weight){
    PGConnection conn;
    std::shared_ptr<TQueue<String_Fn>> queue = get_queue(num_query);
    Timer t1;
    queue->for_each([&conn, weight](String_Fn task) {
        conn.exec(task(weight).c_str());
    });
    std::cout << " " << t1.elapsed() << " \t |";
}

void treads(const int &num_query, const Weight &weight, const int &num_threads){
    std::shared_ptr<TQueue<String_Fn>> queue = get_queue(num_query);
    PGPool_Threads<Weight> pool_threads{num_threads};
    Timer t2;
    pool_threads.run(queue, weight);
    std::cout << " " << t2.elapsed() << " \t |";
}

void async(const int &num_query, const Weight &weight, const int &num_connections){
    std::shared_ptr<TQueue<String_Fn>> queue = get_queue(num_query);
    PGPool_Async<std::string, Weight> pool_async{num_connections};
    Timer t2;
    pool_async.run(queue, weight);
    std::cout << " " << t2.elapsed() << " \t |";
}

void async_threads(const int &num_query, const Weight &weight, const int &num_connections){
    std::shared_ptr<TQueue<String_Fn>> queue = get_queue(num_query);
    PGPool_Async<std::string, Weight> pool_async{num_connections};
    PGPool_Async<std::string, Weight> pool_async2{num_connections};
    Timer t3;
    {
        std::vector<std::thread> threads(2);
        threads[0] = std::thread(&PGPool_Async<std::string, Weight>::run,
            std::ref(pool_async), std::ref(queue), weight);
        threads[1] = std::thread(&PGPool_Async<std::string, Weight>::run,
            std::ref(pool_async2), std::ref(queue), weight);

        std::vector<std::thread>::iterator iter = threads.begin();
        iter = threads.begin();
        while(iter != threads.end()){
            (*iter).join();
            ++iter;
        }
    }
    std::cout << " " << t3.elapsed() << " \t |";
}

int main()
{
    #ifdef _WIN32
    SetConsoleOutputCP(1251);
    #endif

    //Подготовка
    prepare(Weight::little);
    prepare(Weight::medium);
    prepare(Weight::large);

    const int num_query = 100;

    //==============Последовательно

    std::cout << utils::cpt("| выполнение \\ объем таблицы\t | малый \t | средний \t | большой \t |") << std::endl;
    std::cout << "| -------------------------------------------------------------------------" << std::endl;
    std::cout << utils::cpt("| последовательно \t\t |");
    sync(num_query, Weight::little);
    sync(num_query, Weight::medium);
    sync(num_query, Weight::large);
    std::cout << std::endl;

    ///=============Параллельно
    std::cout << "| -------------------------------------------------------------------------" << std::endl;
    for (int i = 1; i < 6; ++i){
        std::cout << utils::cpt("| параллельно ") << i << utils::cpt(" потоков \t |");
        treads(num_query, Weight::little, i);
        treads(num_query, Weight::medium, i);
        treads(num_query, Weight::large, i);
        std::cout << std::endl;
    }
    
    //=============Асинхронно
    std::cout << "| ------------------------------------------------------------------------" << std::endl;
    for (int i = 1; i < 6; ++i){
        std::cout << utils::cpt("| асинхронно ") << i << utils::cpt(" соединений \t |");
        async(num_query, Weight::little, i);
        async(num_query, Weight::medium, i);
        async(num_query, Weight::large, i);
        std::cout << std::endl;
    }

    //=============Асинхронно 2 потока
    std::cout << "| ------------------------------------------------------------------------" << std::endl;
    for (int i = 1; i < 6; ++i){
        std::cout << utils::cpt("| асинх (2 потока) ") << i << utils::cpt(" соедин. \t |");
        async_threads(num_query, Weight::little, i);
        async_threads(num_query, Weight::medium, i);
        async_threads(num_query, Weight::large, i);
        std::cout << std::endl;
    }
    return 0;
}