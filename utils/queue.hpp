#ifndef QUEUE_H
#define QUEUE_H
#include <mutex>
#include <vector>
#include <functional>


/* --------------------------------------------------- */
/*                       HEADER                        */
/* --------------------------------------------------- */
template <typename Queue_Fn>
class TQueue {
public:
    typedef std::function<void(Queue_Fn)> Predicator;

private:
    std::mutex queue_mutex;
    std::vector<Queue_Fn> queue;

public:
    TQueue();
    TQueue(std::vector<Queue_Fn> _queue);
    TQueue(int size, Queue_Fn task);
    void add_queue(std::vector<Queue_Fn> _queue_async);
    unsigned int queue_size();
    Queue_Fn get_task(bool* queue_end);
    void for_each(Predicator predicator);
    void push(Queue_Fn  fn);
};

/* --------------------------------------------------- */
/*                   IMPLEMENTATION                    */
/* --------------------------------------------------- */

template <typename Queue_Fn>
TQueue<Queue_Fn>::TQueue()
    : queue(std::vector<Queue_Fn>()) {}

template <typename Queue_Fn>
TQueue<Queue_Fn>::TQueue(std::vector<Queue_Fn> _queue)
    : queue(_queue) {}

template <typename Queue_Fn>
TQueue<Queue_Fn>::TQueue(int size, Queue_Fn task)
    : queue(std::vector<Queue_Fn>(size, task)) {}    

template <typename Queue_Fn>
void TQueue<Queue_Fn>::add_queue(std::vector<Queue_Fn> _queue)
{
    std::lock_guard<std::mutex> guard_mutex(queue_mutex);
    queue.insert(queue.end(), _queue.begin(), _queue.end());
}

template <typename Queue_Fn>
unsigned int TQueue<Queue_Fn>::queue_size()
{
    std::lock_guard<std::mutex> guard_mutex(queue_mutex);
    return queue.size();
}

template <typename Queue_Fn>
Queue_Fn TQueue<Queue_Fn>::get_task(bool *queue_end) {
    Queue_Fn task = nullptr;
    {
        std::lock_guard<std::mutex> guard_mutex(queue_mutex);
        if (queue.size() != 0) {
            task = queue.front();
            queue.erase(queue.begin());
        } else {
            *queue_end = true;
        }
    }
    return task;
}

template <typename Queue_Fn>
void TQueue<Queue_Fn>::for_each(Predicator predicator){
    std::vector<Queue_Fn>::iterator iter = queue.begin();
    while(iter!=queue.end()) 
    {
        if(predicator)
            predicator(*iter);
        ++iter;
    }
}

template <typename Queue_Fn>
void TQueue<Queue_Fn>::push(Queue_Fn fn){
    queue.push_back(fn);
}

#endif //QUEUE_H