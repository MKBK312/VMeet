#pragma once

#include "task.h"
#include <pthread.h>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>

class ThreadPool {
public:
    ThreadPool(int minThreads, int maxThreads, int queueMax);
    ~ThreadPool();
    
    bool addTask(TaskFunc func, void* arg);
    void shutdown();
    
    int getBusyThreadCount() { return m_busyThreadCount; }
    int getAliveThreadCount() { return m_aliveThreadCount; }
    bool isRunning() { return m_running; }

private:
    void createMinThreads();
    void managerThreadFunc();
    void workerThreadFunc();
    
    std::queue<Task> m_taskQueue;
    std::vector<pthread_t> m_workerThreads;
    pthread_t m_managerThread;
    
    int m_minThreads;
    int m_maxThreads;
    int m_queueMax;
    
    std::atomic<int> m_busyThreadCount{0};
    std::atomic<int> m_aliveThreadCount{0};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shutdown{false};
    
    std::atomic<int> m_exitThreads{0};

    std::mutex m_queueMutex;
    std::condition_variable m_taskCV;
    std::condition_variable m_fullCV;
    std::condition_variable m_exitCV;
    
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
};
