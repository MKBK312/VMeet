#include "threadpool.h"
#include <unistd.h>

ThreadPool::ThreadPool(int minThreads, int maxThreads, int queueMax)
    : m_minThreads(minThreads), m_maxThreads(maxThreads), m_queueMax(queueMax) {
    
    m_running = true;
    m_workerThreads.resize(m_maxThreads);
    
    createMinThreads();
    
    if (pthread_create(&m_managerThread, nullptr, [](void* arg) -> void* {
        ((ThreadPool*)arg)->managerThreadFunc();
        return nullptr;
    }, this) != 0) {
        std::cerr << "Failed to create manager thread" << std::endl;
    }
    
    std::cout << "ThreadPool created: min=" << m_minThreads 
              << ", max=" << m_maxThreads 
              << ", queue=" << m_queueMax << std::endl;
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::createMinThreads() {
    for (int i = 0; i < m_minThreads; ++i) {
        if (pthread_create(&m_workerThreads[i], nullptr, [](void* arg) -> void* {
            ((ThreadPool*)arg)->workerThreadFunc();
            return nullptr;
        }, this) != 0) {
            std::cerr << "Failed to create worker thread" << std::endl;
        } else {
            ++m_aliveThreadCount;
        }
    }
}

void ThreadPool::managerThreadFunc() {
    while (m_running) {
        sleep(2);

        int alive = m_aliveThreadCount;
        int busy = m_busyThreadCount;
        int queueSize = 0;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            queueSize = m_taskQueue.size();
        }
        int idle = alive - busy;

        // Scale up: queue has backlog or busy rate > 70%, and not at max
        if ((queueSize > idle || (alive > 0 && busy * 100 / alive > 70))
            && alive < m_maxThreads) {

            int toAdd = m_minThreads;
            if (alive + toAdd > m_maxThreads)
                toAdd = m_maxThreads - alive;

            std::cout << "[Manager] Scaling UP: +" << toAdd
                      << " threads (alive=" << alive << " busy=" << busy
                      << " queue=" << queueSize << ")" << std::endl;

            for (int i = 0; i < toAdd; ++i) {
                size_t idx = m_workerThreads.size();
                m_workerThreads.push_back(0);
                if (pthread_create(&m_workerThreads[idx], nullptr,
                    [](void* arg) -> void* {
                        ((ThreadPool*)arg)->workerThreadFunc();
                        return nullptr;
                    }, this) == 0) {
                    ++m_aliveThreadCount;
                }
            }
        }

        // Scale down: idle threads >= 2x busy, and above min threshold
        if (idle >= busy * 2 && alive > m_minThreads
            && alive - m_minThreads >= m_minThreads) {

            std::cout << "[Manager] Scaling DOWN: -" << m_minThreads
                      << " threads (alive=" << alive << " busy=" << busy
                      << " idle=" << idle << ")" << std::endl;

            m_exitThreads = m_minThreads;
            for (int i = 0; i < m_minThreads; ++i)
                m_exitCV.notify_one();
        }
    }
}

void ThreadPool::workerThreadFunc() {
    while (m_running) {
        Task task;
        task.func = nullptr;
        task.arg = nullptr;
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            
            while (m_taskQueue.empty() && m_running) {
                m_taskCV.wait(lock);
            }
            
            if (!m_running) {
                break;
            }
            
            if (m_taskQueue.empty()) {
                continue;
            }
            
            task = m_taskQueue.front();
            m_taskQueue.pop();
            
            if (m_taskQueue.size() == (size_t)(m_queueMax - 1)) {
                m_fullCV.notify_one();
            }
        }
        
        if (task.func != nullptr) {
            ++m_busyThreadCount;
            task.func(task.arg);
            --m_busyThreadCount;
        }

        // Scale-down: exit if manager requested
        if (m_exitThreads > 0 && m_aliveThreadCount > m_minThreads) {
            --m_exitThreads;
            --m_aliveThreadCount;
            pthread_exit(nullptr);
        }
    }
    
    --m_aliveThreadCount;
    pthread_exit(nullptr);
}

bool ThreadPool::addTask(TaskFunc func, void* arg) {
    if (!m_running || func == nullptr) {
        return false;
    }
    
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        
        while ((int)m_taskQueue.size() >= m_queueMax && m_running) {
            m_fullCV.wait(lock);
        }
        
        if (!m_running) {
            return false;
        }
        
        Task task;
        task.func = func;
        task.arg = arg;
        
        m_taskQueue.push(task);
        
        std::cout << "[Producer] Task added, queue size: " << m_taskQueue.size() << std::endl;
        
        if (m_taskQueue.size() == 1) {
            m_taskCV.notify_one();
        }
    }
    
    return true;
}

void ThreadPool::shutdown() {
    if (!m_running.exchange(false)) {
        return;
    }
    
    m_taskCV.notify_all();
    
    if (m_managerThread != 0) {
        pthread_join(m_managerThread, nullptr);
    }
    
    for (auto& tid : m_workerThreads) {
        if (tid != 0) {
            pthread_join(tid, nullptr);
        }
    }
    
    std::cout << "ThreadPool shutdown complete" << std::endl;
}
