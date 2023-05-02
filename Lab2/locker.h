#ifndef LOCKER_H
#define LOCKER_H

#include <bits/stdc++.h>
#include <semaphore.h>

// 线程同步类的封装

// class locker
class Locker{
public:
    Locker();
    ~Locker();
    bool lock();
    bool unlock();
    pthread_mutex_t* get();
private:
    pthread_mutex_t m_mutex;
};

// class cond
class Cond{
public:
    Cond();
    ~Cond();
    bool wait(pthread_mutex_t *);
    bool timedwait(pthread_mutex_t *, struct timespec);
    bool signal();
    bool broadcast();

private:
    pthread_cond_t m_cond;
};
#endif

//class Sem
class Sem{
public:
    Sem();
    Sem(int);
    ~Sem();
    bool wait();
    bool post();

private:
    sem_t m_sem;
};