#ifndef THREADPOOL_H
#define THREAEDPOOL_H

#include <bits/stdc++.h>
#include <semaphore.h>

// class threadpool
template<class T>
class ThreadPool {
public: 
    //通过默认实参取代无参数构造函数？
    ThreadPool(int threads_nums = 8 , int requests_nums = 512 );
    ~ThreadPool();

    bool append(T *request);

private:
    //静态函数不能访问非静态成员
    static void* worker(void *arg);
    void run();

private:
    bool m_stop;
    int m_thread_number;  //nums of threads
    pthread_t *m_threads; //arry of threadpool
    sem_t m_poolSem;

    std::list<T*> m_workqueue;   //list of workqueue
    int max_requests;
    pthread_mutex_t m_queueLocker;

};

template<class T>
ThreadPool<T>::ThreadPool(int num1, int num2) :
m_thread_number(num1), max_requests(num2),
m_stop(false), m_threads(NULL) {
    if(num1 <= 0 || num2 <= 0) {
        throw std::exception();
    }

    m_threads = new pthread_t [num1];
    if(!m_threads) {
        throw std::exception();
    }

    for(int i = 0; i < num1; i++) {
        std::cout<<"Creating thread ："<<i<<" successfully"<<std::endl;
        if(pthread_create(&m_threads[i], NULL, worker, this) != 0) {
            perror("pthread_create failed");
            delete [] m_threads;
            throw std::exception();
        }

        if(pthread_detach(m_threads[i])) {
            delete [] m_threads;
            throw std::exception();
        }
    }

}

template<class T>
ThreadPool<T>::~ThreadPool() {
    delete [] m_threads;
    m_stop = true;
}

template<class T>
bool ThreadPool<T>::append(T *request) {
    pthread_mutex_lock(&m_queueLocker);
    if( m_workqueue.size() > max_requests ) {
        pthread_mutex_unlock(&m_queueLocker);
        return false;
    }

    m_workqueue.push_back(request);
    pthread_mutex_unlock(&m_queueLocker);
    sem_post(&m_poolSem);
    return true;
}

template<class T>
void* ThreadPool<T>::worker(void *arg) {
    ThreadPool *pool = (ThreadPool*) arg;
    pool->run();
    return pool;
}

template<class T>
void ThreadPool<T>::run() {
    while(!m_stop) {
        sem_wait(&m_poolSem);
        pthread_mutex_lock(&m_queueLocker);

        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        
        pthread_mutex_unlock(&m_queueLocker);

        request->process();
    }
}

#endif