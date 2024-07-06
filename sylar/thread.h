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

// --------------------- Thread ---------------------
class Thread
{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string name);   // 线程的执行方法是回调函数!
    ~Thread();

    pid_t getId() const                 { return m_id; }
    const std::string& getName() const  { return m_name; }

    void join();
    static Thread* GetThis();                                   // 获得当前线程
    static const std::string& GetName();                        // 获得当前线程的名称
    static void SetName(const std::string& name);               // 给线程改名。特别是哪些不是我们创建的线程，比如主线程不是我们创建的，不能在声明的时候赋名。只能通过这个函数改名

    static void* run(void* args);                               // 线程的执行方法（pthread_create的第三个参数）

private:
    Thread(const Thread&) = delete;                             // abandon copy
    Thread(const Thread&&) = delete;                            // abandon move
    Thread& operator=(const Thread&) = delete;                  // abandon =

private:
    pid_t m_id = -1;                                            // 线程id
    pthread_t m_thread = 0;                                     // unsigned long int pthread的线程
    std::function<void()> m_callback;                           // 线程执行函数(回调函数)
    std::string m_name;                                         // 线程对象m_thread的名称 ?

    Semaphore m_semaphore;
};

/// 关于这段代码 涉及到的静态函数的SetName/GetName 和 类成员getName 的说明:
/// const std::string &Thread::GetName() { return t_thread_name; } 与 const std::string& getName() const { return m_name; }

/*  关于类中的静态函数，可以说的很多，这里顺便给自己释疑、做个总结
 * part1:
 *  1. 静态成员函数 const std::string &Thread::GetName() { return t_thread_name; }：
 *      作用：这个函数是用来获取当前执行线程的名称，它通过访问thread_local变量t_thread_name来实现。这意味着，无论哪个线程调用了这个函数，它都将返回调用该函数的那个线程所关联的名称。
 *      这种方式不依赖于任何特定的Thread实例，适用于全局上下文，包括那些并非直接由Thread类管理的线程（例如主线程）。
 *  2. 非静态成员函数 const std::string& getName() const { return m_name; }：
 *      作用：这个函数是Thread类的一个实例方法，用于获取特定Thread对象关联的线程名称，即m_name成员变量的值。调用这个函数需要有一个Thread类的实例（通过this指针隐式或显式传递），
 *      并且它返回的是该特定实例所代表的线程的名称。这意味着，它只适用于由Thread类直接管理并拥有实例的情况。
 *  简而言之，GetName()静态函数是基于线程局部存储的，能够获取当前执行线程的名称，不论该线程是否由Thread类直接创建。
 *  而getName()实例方法则是获取某个具体Thread对象关联的线程名称，需要有Thread实例作为调用的基础。两者都可用于获取线程名称，但在适用场景和获取方式上有所不同。
 *
 * part2:
 *  当然，两者各有其优势，根据不同的应用场景选择合适的方法非常重要：
 *  const std::string &Thread::GetName() { return t_thread_name; }
 *  1.全局可达性：作为静态成员函数，它不需要类的实例就可以被调用，这意味着在任何地方，只要能访问到Thread类，就可以获取到当前线程的名称，这在全局上下文或没有直接Thread实例引用的场景下非常有用。
 *  2.灵活性：它可以为非由Thread类直接创建的线程（如主线程或其他外部线程）提供命名和查询名称的能力，增加了代码的灵活性和兼容性。
 *  3.线程安全：通过thread_local变量实现，每个线程有自己的独立副本，确保了并发访问的安全性。
 *  const std::string& getName() const { return m_name; }
 *  1.精确性：直接访问特定Thread实例的m_name成员，能够准确地获取到该实例所代表的线程的名称，适合在已知线程实例且需要精确控制或查询该线程信息的场景。
 *  2.面向对象原则：遵循面向对象设计原则，通过对象实例来访问属性，使得代码更加直观和易于理解，特别是在类的其他成员函数中使用时，能够保持良好的封装性和一致性。
 *  3.控制权：由于是实例方法，可以通过实例化不同的Thread对象来创建具有不同名称的线程，便于管理和区分不同的线程任务，尤其是在需要细粒度控制线程属性的复杂应用中。
 */

}

#endif
