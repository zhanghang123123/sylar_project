#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <pthread.h>
#include <memory>
#include <functional>
#include <thread>
#include <string>


namespace sylar {

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
};

}

#endif
