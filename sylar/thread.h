#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <pthread.h>
#include <memory>
#include <functional>
#include <thread>
#include <string>
#include <semaphore.h>
#include <stdint.h>


namespace sylar {

class Semaphore
{
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();
private:
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

private:
    sem_t m_semaphore;
};

template<class T>
struct ScopedLockImpl
{
public:
    ScopedLockImpl(T& mutext)
        :m_mutex(mutext)
    {
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLockImpl()
    {
        m_mutex.unlock();
//        m_locked = false;     error
    }

    void lock()
    {
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock()
    {
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct ReadScopedLockImpl{
public:
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex)
    {
        m_mutex.readlock();
        m_locked = true;
    }
    ~ReadScopedLockImpl()   { m_mutex.unlock(); }

    void lock(){
        if(!m_locked){
            m_mutex.readlock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct WriteScopedLockImpl{
public:
    WriteScopedLockImpl(T& mutext)
        :m_mutex(mutext)
    {
        m_mutex.writelock();
        m_locked = true;
    }
    ~WriteScopedLockImpl()  { m_mutex.unlock(); }

    void lock(){
        if(!m_locked){
            m_mutex.writelock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

class Mutex
{
public:
    typedef ScopedLockImpl<Mutex> Lock;     // ReadScopedLockImpl error
    Mutex()         { pthread_mutex_init(&m_mutex, nullptr); }
    ~Mutex()        { pthread_mutex_destroy(&m_mutex); }
    void lock()     { pthread_mutex_lock(&m_mutex); }
    void unlock()   { pthread_mutex_unlock(&m_mutex); }
private:
    pthread_mutex_t m_mutex;
};

class RWMutex
{
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;
    RWMutex()
    {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWMutex()
    {
        pthread_rwlock_destroy(&m_lock);            // RAII
    }

    void readlock()     { pthread_rwlock_rdlock(&m_lock); }
    void writelock()    { pthread_rwlock_wrlock(&m_lock); }
    void unlock()       { pthread_rwlock_unlock(&m_lock); }

private:
    pthread_rwlock_t m_lock;
};


class Thread
{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string name);   // 线程的执行方法是回调函数?
    ~Thread();

    pid_t getId() const                 { return m_id; }
    const std::string& getName() const  { return m_name; }

    void join();
    static Thread* GetThis();                                   // 获得当前线程
    static const std::string& GetName();                        // 获得当前线程的名称
    static void setName(const std::string& name);               // 给线程改名。特别是哪些不是我们创建的线程，比如主线程不是我们创建的，不能在声明的时候赋名。只能通过这个函数改名

    static void* run(void* args);                               // 线程的执行方法（pthread_create的第三个参数）

private:
    Thread(const Thread&) = delete;                             // abandon copy
    Thread(const Thread&&) = delete;                            // abandon move
    Thread& operator=(const Thread&) = delete;                  // abandon =

private:
    pid_t m_id = -1;                                            // 线程id
    pthread_t m_thread = 0;                                     // unsigned long int pthread的线程
    std::function<void()> m_callback;                           //
    std::string m_name;                                         // 线程对象m_thread的名称 ?

    Semaphore m_semaphore;
};

}

#endif
