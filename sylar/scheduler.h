/**
 * @file scheduler.h
 * @brief 协程调度器封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-05-28
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include <iostream>
#include <atomic>
#include "fiber.h"
#include "thread.h"
#include "noncopyable.h"

namespace sylar {

/** @brief   协程调度器 : Scheduler的纯粹性：它的职责非常纯粹和核心多线程协程调度。它不关心任务是因为I/O、定时器还是单纯的计算而产生的。它只负责接收任务，并高效、公平地分配出去。
 * @details  封装的是N-M的协程调度器
 *           内部有一个线程池,支持协程在线程池里面切换
 */
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    /** @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否使用当前调用线程
     * @param[in] name 协程调度器名称
     *
     *  工作原理： 一个（主）线程创建一个调度器（此时系统中没有协程调度器，自然也就没有协程了。所以此时创建调度器的都是线程）
     *  use_caller = true模式:
     *      立即协程化： 在调度器构造函数中同步执行 Fiber::GetThis()，将当前线程主协程化。所以创建完该该协程调度器后，创建该调度器的线程变成了协程：将原先线程执行的部分虚拟化为主协程 -> m_rootFiber; 本线程 -> m_rootThread
     *      指明调度协程：生成 m_rootFiber 作为该线程的专用调度器
     *      身份标记：   记录 m_rootThread 标识这是"有特殊身份的调度线程"
     *      结果：      本线程在调度器创建完成后即具备完整的协程调度能力
     *
     *  use_caller=false模式:
     *      保持原样：   线程不进行任何协程化转换
     *      无特殊资源： 不创建 m_rootFiber，m_rootThread为 -1
     *      纯粹管理者： 线程仅负责调度器的创建和管理，不参与具体任务执行
     *      结果：      线程保持普通线程身份，调度任务由其他工作线程处理
     *
     *  最终
     *  use_caller = true 时:
     *                                       |线程主协程t_thread_fiber                             || t_scheduler_fiber(当前线程的调度协程)|
     *      线程1(t_scheduler = scheduler) ： 创建调度器, 线程虚拟化成线程主协程(管理调度器，不参与调度任务)， m_rootFiber调度协程(运行run调度函数)  ， 协程1-3， 协程1-4， 协程1-5
     *                                                                            |t_scheduler_fiber=t_thread_fiber|
     *      线程2(t_scheduler = scheduler) ：                                          线程主协程(运行run调度函数)                         ， 协程2-2， 协程2-3， 协程2-4， 协程2-5
     *      线程3(t_scheduler = scheduler) ：                                          线程主协程(运行run调度函数)                         ， 协程3-2， 协程3-3， 协程3-4， 协程3-5
     *      因为所有线程都参与 调度，所以， 标记线程1 为 m_rootThread； 标记创建调度器 的协程1-1 为  m_rootFiber；
     *  use_caller = false时:
     *      线程1 ：创建调度器scheduler（就是线程1本身创建）
     *                                      |t_scheduler_fiber=t_thread_fiber|
     *      线程2(t_scheduler = scheduler) ： 线程主协程(运行run调度函数)         ， 协程2-2， 协程2-3， 协程2-4， 协程2-5
     *      线程3(t_scheduler = scheduler) ： 线程主协程(运行run调度函数)         ， 协程3-2， 协程3-3， 协程3-4， 协程3-5
     *      线程4(t_scheduler = scheduler) ： 线程主协程(运行run调度函数)         ， 协程4-2， 协程4-3， 协程4-4， 协程4-5
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name;}    // 返回协程调度器名称
    static Scheduler* GetCurrentScheduler();                // 返回当前(线程所属的)协程调度器
    static Fiber* GetSchedulerFiber();                      // 返回当前线程中的调度循环协程
    static void setSchedulerFiber(Fiber*);                  // 设置当前线程中的调度循环协程
    void start();                                           // 启动协程调度器
    void stop();                                            // 停止协程调度器

    void switchTo(int thread = -1);
    std::ostream& dump(std::ostream& os);

    /// 调度协程, param: fc协程或函数; thread_id 协程执行的线程id(-1标识任意线程)   --> addJobToSchedule
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread_id = -1)         // use : schedule(func/fiber) 就是把一个函数扔进Schedule中,让它以协程的方式运行
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread_id);
        }
        if(need_tickle)
            tickle();
    }

    /// 批量调度协程, param :begin协程数组的开始; end协程数组的结束         --> addJobsToSchedule
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;   // &*begin是地址，begin是迭代器把 ，  这里用的是它的指针（取的地址），就是调用的swap对应的函数。
                ++begin;
            }
        }
        if(need_tickle)
            tickle();
    }

protected:
    virtual void tickle();                                  // 通知协程调度器有任务了
    void run();                                             // 协程调度函数
    virtual bool stopping();                                // 返回是否可以停止
    virtual void idle();                                    // 协程无任务可调度时执行idle协程
    void setCurrentScheduler();                             // 设置当前(线程所属的)的协程调度器
    bool hasIdleThreads() { return m_idleThreadCount > 0;}  // 是否有空闲线程

private:
    /// 协程调度启动(无锁) --- 协程调度器提供的方法Schedule()：将任务(协程/function)添加到任务队列
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread)
    {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb)
            m_fibers.push_back(ft);
        return need_tickle;
    }

private:
    struct FiberAndThread                           ///  这里是我们要执行的协程，对其封装, 且其可以不仅仅支持对象是协程: 支持协程/函数/线程组 (定义协程可以支持的参数(就是运行的对象))
    {
        Fiber::ptr fiber;                           // 协程
        std::function<void()> cb;                   // 协程执行函数
        int thread_id;                              // 线程id         --- 这个是为了支持协程调度器支持指定协程在那个线程上运行，就是指定运行线程; thread_id = -1 就是任意现场可以执行

        // FiberAndThread 的构造函数中分别为两个值（赋值传值）和 指针（交换指针，接管所有权）；可能会有疑问问什么不用std::move -> 这种设计与STL容器的兼容性非常好。类似std::vector::push_back这样的操作，在C++11之前没有移动语义，通过swap可以高效地转移外部对象资源而不需要拷贝。
        FiberAndThread(Fiber::ptr f, int thread_id) : fiber(f), thread_id(thread_id) {}         // (构造函数) param: f协程, thread_id线程id: 传入是协程和线程id（说明这个对象是协程对象, 且指定了运行所在的线程），
        FiberAndThread(Fiber::ptr* f, int thr) : thread_id(thr) { fiber.swap(*f); }             // 构造函数 param[in] f 协程指针; param[in] thr 线程id; post *f = nullptr   ???
        FiberAndThread(std::function<void()> f, int thread_id) : cb(f), thread_id(thread_id) {} // (构造函数) param: f协程执行函数, thread_id线程id
        FiberAndThread(std::function<void()>* f, int thr) : thread_id(thr) { cb.swap(*f); }     // 构造函数 param[in] f 协程执行函数指针; param[in] thr 线程id; post *f = nullptr

        FiberAndThread() : thread_id(-1) {}                                                     // 无参构造函数 (STL容器必须)
        void reset() { fiber = nullptr; cb = nullptr; thread_id = -1; }                         // 重置数据
    };

private:
    void initWithCaller();                          // 初始化 - 使用调用线程的模式
    void initWithoutCaller();                       // 初始化 - 不使用调用线程的模式
    void stopWithCaller();                          // 停止调度器（使用调用线程的版本）
    void stopWithoutCaller();                       // 停止调度器（不使用调用线程的版本）
    void waitForWorkerThreads();                    // 等待工作线程结束

private:
    MutexType m_mutex;                              // Mutex
    std::vector<Thread::ptr> m_threads;             // 协程调度器的线程池
    std::list<FiberAndThread> m_fibers;             // 待执行的协程队列 (可以理解为待执行的任务队列，它可以是协程，也可以就是单纯的function函数， 以外，给这个任务绑定一个threadid 已表示指定的线程)-- 通过schedule()函数添加任务
    std::string m_name;                             // 协程调度器名称

protected:
    std::vector<int> m_threadIds;                   // 协程下的线程id数组
    size_t m_threadCount = 0;                       // 线程数量
    std::atomic<size_t> m_activeThreadCount = {0};  // 工作线程数量
    std::atomic<size_t> m_idleThreadCount = {0};    // 空闲线程数量
    bool m_is_stopping = true;                      // 是否正在停止 (协程调度器停止整个协程池)
    bool m_is_autostop = false;                     // 是否自动停止

    bool m_use_caller = true;                       // use_caller为true/false
    Fiber::ptr m_rootFiber;                         // use_caller为true时有效, caller线程的调度协程
    int m_rootThread = 0;                           // m_creatorThread创建调度器的线程，也被指定为管理调度器的线程start()/stop()等, 即调度器的主线程id(use_caller)
};

class SchedulerSwitcher : public sylar::Noncopyable {
public:
    SchedulerSwitcher(Scheduler* target = nullptr);
    ~SchedulerSwitcher();
private:
    Scheduler* m_caller;
};

}

#endif
