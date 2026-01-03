#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

static thread_local Thread* t_thread = nullptr;                     // 用这个静态局部变量指向当前线程  t_thread 就是自己
static thread_local std::string t_thread_name = "UNKNOW";           // 用这个当前线程全局变量表示当前线程name (better than name in thread class)

Thread::Thread(std::function<void ()> cb, const std::string name)   // 构造线程对象，传入回调函数
    : m_callback(cb),
      m_name(name)
{
    if(name.empty()) m_name = "UNKNOW";                                 // (2).
    int ret = pthread_create(&m_thread, nullptr, &Thread::run, this);   // 线程构造之后线程开始运行;这里新线程的创建就交给操作系统了，主线程继续执行。就和主线程分离了。
    if(ret){
        MYLOG_ERROR(SYLAR_LOG_ROOT()) << "pthread_create thread fail, ret=" << ret << " name=" << m_name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();         // 写在这里，阻塞等待, 确保线程类在（完整）生成之前，线程就已经跑起来了
}

Thread::~Thread()
{
    if(m_thread) pthread_detach(m_thread);                  // 析构时线程分离，保证其自我销毁
}

void Thread::join()
{
    if(m_thread) {
        int ret = pthread_join(m_thread, nullptr);          // 对于pthread_create创建的线程来说，join的时候获取其返回值（就是pthread_join的第二个参数）
        if(ret) {
            MYLOG_ERROR(SYLAR_LOG_ROOT()) << "pthread_join thread fail, ret=" << ret << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

Thread *Thread::GetThis()               { return t_thread; }
const std::string &Thread::GetName()    { return t_thread_name; }

void Thread::SetName(const std::string &name)
{
    if(t_thread) t_thread->m_name = name;
    t_thread_name = name;                               // 说明下，这里封装的对象Thread 是我们管理 pthread 创建的具体线程及其属性。它本身的名字 和 具体线程 是分开的，也应该是相同的。
}

void* Thread::run(void *args)                           // 参数就是 pthread_create() 创建具体线程时 传入的第四个参数 this; 此处代码中的run是静态成员，所以this指针必须显式地传入
{
    Thread* thread = (Thread*)args;                     // https://yb.tencent.com/s/17jGNcYMfxGt
    t_thread = thread;                                  // 将 Thread对象的指针存入线程局部变量​ t_thread。
    t_thread_name = thread->m_name;                     // (2). t_thread_name = thread->m_name; 不放在构造函数中，为什么？
    thread->m_id = sylar::getThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0,15).c_str());    // 给这个线程pthread_t的线程命名，修改完了线程名以后。top命令中显示的名就变了

    std::function<void()> cb;
    cb.swap(thread->m_callback);

    thread->m_semaphore.notify();

    cb();
    MYLOG_INFO(SYLAR_LOG_ROOT()) << "name=" << sylar::getThreadId() << "; = " << pthread_self();
    return 0;
}

Semaphore::Semaphore(uint32_t count)
{
    if(sem_init(&m_semaphore, 0, count))
        throw std::logic_error("sem_init error");
}

Semaphore::~Semaphore()
{
    sem_destroy(&m_semaphore);
}

void Semaphore::wait()
{
    if(sem_wait(&m_semaphore))
        throw std::logic_error("sem_wait error");
}

void Semaphore::notify()
{
    if(sem_post(&m_semaphore))
        throw std::logic_error("sem_post error");
}

//#include "sylar/sylar.h"

//void func1(){
//    MYLOG_INFO(SYLAR_LOG_ROOT()) << "name=" << sylar::Thread::GetName()
//                                 << "this.name=" << sylar::Thread::GetThis()->getName()
//                                 << "id=" << sylar::getThreadId()
//                                 << "this.id=" << sylar::Thread::GetThis()->getId();
//}
//void func2(){

//}

//int main(int argc, char** argv){
//    MYLOG_INFO(SYLAR_LOG_ROOT()) << " test thread begin--";
//    std::vector<sylar::Thread::ptr> threads;
//    for(int i = 0; i < 10; i++){
//        sylar::Thread::ptr thrd(new sylar::Thread(&func1, "name_" + std::to_string(i)));
//        threads.push_back(thrd);
//    }

//    for(int i = 0; i < 10; i++){
//        threads[i]->join();
//    }
//    MYLOG_INFO(SYLAR_LOG_ROOT()) << " test thread end--";
//    return 0;
//}

}
