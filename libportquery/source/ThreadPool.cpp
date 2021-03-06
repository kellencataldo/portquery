#include "ThreadPool.h"

namespace PortQuery {

    void ThreadSafeWorkQueue::blockingPush(const WorkType&& work) {
        std::unique_lock lock(m_mutex);
        m_queue.emplace(work);
        m_ready.notify_one();
    }


    bool ThreadSafeWorkQueue::nonBlockingPush(const WorkType&& work) {
        std::unique_lock lock(m_mutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            return false;
        }

        m_queue.emplace(work);
        m_ready.notify_one();
        return true;
    }


    bool ThreadSafeWorkQueue::blockingPop(WorkType& work) {
        std::unique_lock lock(m_mutex);
        while(m_queue.empty() && !m_done) {
            m_ready.wait(lock); 
        }

        if (m_queue.empty()) {
            return false;
        }

        work = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }


    bool ThreadSafeWorkQueue::nonBlockingPop(WorkType& work) { 
        std::unique_lock lock(m_mutex, std::try_to_lock);
        if(!lock.owns_lock() || m_queue.empty()) {
            return false;
        }

        work = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }


    void ThreadSafeWorkQueue::setDone() {
        std::unique_lock lock(m_mutex);
        m_done = true;
        m_ready.notify_all();
    }


    bool ThreadSafeWorkQueue::empty() const {
        std::unique_lock lock(m_mutex);
        return m_queue.empty();
    }

    ThreadPool::ThreadPool(const int threadCount) : m_nextQueue{0} {

        m_threadCount = threadCount != 0 ? threadCount : std::thread::hardware_concurrency();
        m_queues = std::vector<ThreadSafeWorkQueue>(m_threadCount);
        for (unsigned int startQueue = 0; startQueue < m_threadCount; startQueue++) {
            m_threads.emplace_back([&, startQueue] { workerLoop(startQueue); });
        }
    }

    ThreadPool::~ThreadPool() {
        for(auto& q : m_queues) {
            q.setDone();
        }

        for(auto& t : m_threads) {
            t.join();
        }
    }
 


    void ThreadPool::workerLoop(const unsigned int startQueue) {

        while(true) {

            std::function<void(void)> functor = nullptr;
            for(unsigned int index = 0; index < m_threadCount; index++) {
                if(m_queues[(startQueue + index) % m_threadCount].nonBlockingPop(functor)) {
                    break;
                }
            }

            if (nullptr == functor && !m_queues[startQueue].blockingPop(functor)) {
                break;
            }

            functor();
        }
    }

/*
    template<typename Function, typename... Args> 
    std::future<std::invoke_result_t<Function, Args...>> ThreadPool::submitWork(Function&& f, Args&&... args) {
        using WorkType = std::packaged_task<std::invoke_result_t<Function, Args...>()>;
        auto workPtr = std::make_shared<WorkType>(std::bind(std::forward<Function>(f), std::forward<Args>(args)...));
        auto workLambda = [workPtr] () { (*workPtr)(); };
        unsigned int currentQueue = m_nextQueue++;
        for (unsigned int loopIndex = 0; loopIndex < m_threadCount * MAX_LOOPS; loopIndex++) {
            if (m_queues[(currentQueue + loopIndex) % m_threadCount].nonBlockingPush(workLambda)) {
                return workPtr->get_future();
            }
        }

        m_queues[currentQueue % m_threadCount].blockingPush(workLambda);
        return workPtr->get_future();
    }
    */
}


