#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "util.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");


// static thread_local 修饰的线程局部变量会为每个程序中的线程都分配这个内存，并初始化为nullptr
// 只有这个调度器下setThis 以后，才会将当前线程的 t_scheduler 初始化为该实例调度器。
// 这样相当于每个调度线程初始化的时候就被指定了调度器（父亲指定了儿子的所属），以后在线程中就可以直接获取本线程的调度器是谁（是谁所属的）
static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_scheduler_fiber = nullptr; // 每个线程专属的调度循环协程; 协程调度器所属的各个线程的调度循环协程（usercaller = true 时的主线程的为 m_rootFiber， 其他线程都是主协程充当调度循环协程；而usercaller = false时，全部协程调度器所属的各个线程的调度循环协程 都是线程的主协程）

Scheduler::Scheduler(size_t threadsize, bool use_caller, const std::string& name)
    : m_name(name)
{
    SYLAR_ASSERT(threadsize > 0);               // 断言输入的线程数自然要大于0
    m_rootThread = sylar::GetThreadId();        // 记录调用线程ID
    m_use_caller = use_caller;

    if(use_caller)
    {
        initWithCaller();                       // 创建该调度器的线程扮演“创建者”和“管理者”的角色, 也参与任务调度. (在use_caller=true时，caller线程内存在三类协程：主协程、调度协程和任务协程。在use_caller=true时，caller线程内存在三类协程：主协程、调度协程和任务协程。主协程负责创建调度器和最终停止调度器，调度协程负责执行调度循环，任务协程是具体的工作单元。)
        threadsize--;                           // 就是使用了调用线程，把它也加入到调度器的线程组中来了，所以可以少申请一个线程
    }
    else
    {
        initWithoutCaller();                    // 创建该调度器的线程仅仅是扮演“创建者”和“管理者”的角色, 本身不参与任务调度
    }
    m_threadCount = threadsize;
}

Scheduler::~Scheduler()
{
    SYLAR_ASSERT2(m_is_stopping, "m_is_stopping 应该为true, 出现问题大概率是因为你没有正确调用stop函数");
    if(GetCurrentScheduler() == this)
        t_scheduler = nullptr;
}

Scheduler* Scheduler::GetCurrentScheduler()     { return t_scheduler; } //
void Scheduler::setCurrentScheduler()           { t_scheduler = this; }
Fiber* Scheduler::GetSchedulerFiber()           { return t_scheduler_fiber; }
void Scheduler::setSchedulerFiber(Fiber* fiber) { t_scheduler_fiber = fiber; }

void Scheduler::start()
{
    MutexType::Lock lock(m_mutex);      // 这个锁是非常必要的，它体现了框架设计中对线程安全和状态一致性的严格考量。 (锁的核心作用：防御性编程与状态保护)

    if(!m_is_stopping)
        return;                         // 1. m_is_stopping == true 表示还没有启动; == false 表示它启动了，那就不执行下面的了
    m_is_stopping = false;              // 2. 正式开始start了，先改个状态先，

    SYLAR_ASSERT(m_threads.empty());    //    m_is_stopping = false表示第一次启动，此时线程池应为空，很显然，没运行过，还加入线程

    m_threads.resize(m_threadCount);    // 3. 3.1给线程池设置大小(位置)，3.2并给每个位置创建线程，3.3 将线程的id保存到线程id数组中
    for(size_t i = 0; i < m_threadCount; ++i)
    {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();                      // lock.unlock()也绝对不是多余的。它是保证调度器从启动阶段平滑、安全地过渡到运行阶段的关键操作，是并发编程中一项重要的防御性措施。
                                        // "接着前面的对话，关于sylar 调度器源..."点击查看元宝的回答 https://yb.tencent.com/s/ApaRLWNmXn6c

}

/**
 * @brief stop()函数的核心作用是安全、有序地停止调度器的所有工作。它不会立即终止线程，而是确保：
 *          1. 所有已提交的任务都被执行完毕
 *          2. 不再接受新任务
 *          3. 所有工作线程安全退出
 *      参考资料 ： "stop 函数是什么作用？工作机理是什么..."元宝回答 https://yb.tencent.com/s/Kelg8asSF9PL
 *
 * stop()函数的设计非常精巧，它确保了调度器能够平滑、安全地关闭，并妥善处理了 use_caller参数带来的两种不同线程模型。
 */
void Scheduler::stop()
{
    m_is_autostop = true;
    if(m_rootFiber &&               // use_caller为true时有效, 调度协程
       m_threadCount == 0 &&
       (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
    {
        MYLOG_INFO(g_logger) << this << " stopped";
        m_is_stopping = true;

        if(stopping())
            return;
    }

    // 无论哪种模式，都验证是否在创建线程中调用
    int current_tid = sylar::GetThreadId();
    if (current_tid != m_rootThread) {
        std::stringstream ss;
        ss << "Scheduler::stop() 必须在创建线程中调用. "
           << "创建线程: " << m_rootThread /*<< ", 创建线程名: " << m_rootThread->getName()*/
           << ", 当前线程: " << current_tid;
        throw std::logic_error(ss.str());
    }

    if (m_use_caller) {
        stopWithCaller();       // 使用调用线程的模式，额外验证协程身份
    } else {
        stopWithoutCaller();    // 不使用调用线程的模式
    }
}

void Scheduler::initWithCaller()
{
    MYLOG_INFO(g_logger) << " 初始化 scheduler with caller thread ";
    sylar::Fiber::GetThis();                        // (初始化主协程, 当前线程的主协程)如果线程没有协程的话，GetThis()给它初始化一个主协程: (获取当前线程正在执行的协程) --- test_fiber.cc 中有fiber使用 案列。 这就是说某个线程使用fiber 就是要先GetThis 一下

    SYLAR_ASSERT(GetCurrentScheduler() == nullptr); // 在创建协程调度器的时候，getCurrentScheduler() 返回当前线程的协程调度器，应该是nullptr，如果不是空，就表明已经有了一个线程调度器。就报错 (本线程已经有人在调度我了)
    setCurrentScheduler();                          // setCurrentScheduler()  t_scheduler = this; 指向当前实例，就是当前调度器

    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true)); // 创建调用线程的调度协程m_rootFiber, 调用线程的主协程是属于管理 调度器对象的（创建它和stop它/delete它）, 需要专门的任务调度的协程。 == 其他线程的t_scheduler_fiber(调度协程(执行run循环))
    sylar::Thread::SetName(m_name);
    setSchedulerFiber(m_rootFiber.get());
    m_threadIds.push_back(m_rootThread);
}

void Scheduler::initWithoutCaller()
{
    MYLOG_INFO(g_logger) << " 初始化 scheduler without caller thread ";
    // 该创建调度器线程不参与调度，它仅仅作为调度器的管理线程。所以这里不用实现
}

void Scheduler::stopWithCaller()
{
    MYLOG_INFO(g_logger) << "stopWithCaller() 在线程 " << sylar::GetThreadId() << " 中调用";

    // 验证是否在正确的协程中调用（必须是管理者协程，不能是调度协程或任务协程）
    // 在 use_caller=true 模式下，GetThis() == this 表示当前在管理者协程中
    std::stringstream ss;
    ss << "stopWithCaller() 必须在创建线程的管理者协程中调用. " << "当前上下文: " << (GetCurrentScheduler() ? "调度器上下文" : "非调度器上下文");
    SYLAR_ASSERT2(GetCurrentScheduler() == this, ss.str());  // 必须在创建调度器的线程调用

    m_is_stopping = true;
    // 通知所有纯工作线程(总的线程数 - 调用协程调度器的一个线程), 让他们运行调度协程 run 循环处理任务.没有任务时, 直接退出线程 (纯工作线程是"线程入口即调度循环")
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    // 通知调用线程   : "对于caller = true 。 这里..."点击查看元宝的回答 https://yb.tencent.com/s/U6Qlbgwlwp4s
    tickle();
    // 让调用线程的调度协程处理剩余任务
    if (m_rootFiber && !stopping()) {
        std::cout << "切换到调用线程的调度协程处理剩余任务" << std::endl;
        m_rootFiber->call();    // 执行流从主协程（管理者）​ 主动切换到调度协程（工作者）。调度协程开始执行 Scheduler::run()循环，直到任务队列为空且收到停止信号为止; 完成后，执行流会切回主协程。
    }
    // m_rootFiber 调度协程, 需要返回主协程继续执行收尾逻辑-> 继续执行stop() 函数的后续逻辑. (调用线程是"主协程托管调度循环", 即 m_rootFiber 协程的入口是 调用线程的主协程)

    waitForWorkerThreads();     // 本质就是 (=true) 调用线程的主协程（管理协程，此时依然是协程） 或者 （= false）调用线程本身 ，接着管理这些协程调度器中的线程的退出。 相当于 前面的执行又回到了 调用stop所在的管理协程/线程中，让它接着管理那些线程的退出及线程的清理。
    // waitForWorkerThreads()的本质正是让调用 stop()的管理者（无论是 use_caller=true模式下的主协程，还是 use_caller=false模式下的调用线程）来负责等待并清理它之前创建的所有工作线程。这个过程是调度器停止序列中的资源回收阶段。 https://yb.tencent.com/s/up3gMC0w1VOx
}


void Scheduler::stopWithoutCaller()
{
    MYLOG_INFO(g_logger) << "stopWithoutCaller() 在线程 " << sylar::GetThreadId() << " 中调用";

    SYLAR_ASSERT2(GetCurrentScheduler() != this, "警告: stopWithoutCaller() 在工作线程中调用，可能导致死锁");  // 检查是否在工作线程中调用（潜在死锁）. 就是不能再工作线程中调用。只能在创建调度器的线程中（管理线程中）调用
                                              // 避免这种: 一个任务协程（运行在某个工作线程上）的代码中，错误地写了这样一行： Scheduler::GetThis()->stop(); // 这是一个严重的错误！

    m_is_stopping = true;
    // 通知所有工作线程
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    // 管理线程等待工作线程结束
    waitForWorkerThreads();
}

void Scheduler::waitForWorkerThreads()
{
    MYLOG_INFO(g_logger) << "等待 " << m_threads.size() << " 个工作线程结束...";
//    // 方案1: (在简单、单线程控制的场景下可以工作，但存在并发风险，不适合作为基础库的核心逻辑。)
//    for (auto& thread : m_threads) {
//        if (thread) {
//            thread->join();
//        }
//    }
//    m_threads.clear();

    // 方案2（sylar原来的方案）:  方案2更优, 点击查看元宝的回答 https://yb.tencent.com/s/iM8pF5Wg01NK
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
    MYLOG_INFO(g_logger) << "所有工作线程已结束!";


}


/**
 *   这个run函数虽然是Scheduler类中的函数，但是它的实际执行者是 "协程调度器下每个参与调度的线程"。
 *   就是线程创建时通过指定该函数来在对应线程中运行该run函数，
 *   有点类似QThread的exec()函数，是一个事件循环，只不过 QThread 的exec()的事件循环访问的是线程内的事件，
 *   而Scheduler的run函数是其关联的线程运行run函数事件循环来从 Scheduler 队列中取任务。本质就是任务线程的主协程自己while 循环从Scheduler 实例中取任务。取到了任务
 *   就创建一个任务协程执行，执行完了就回到 主协程继续while 循环等待（阻塞等待）。
 *   doc: "这个run 函数虽然是 Schedule..."点击查看元宝的回答 https://yb.tencent.com/s/i9bZGxeixqio
 */
void Scheduler::run()   // 实际在run 函数中运行的都是各自的线程中，所以setCurrentScheduler() 都是设置的各自线程的参数
{
    MYLOG_INFO(g_logger) << m_name << " run";
//    set_hook_enable(true);
    setCurrentScheduler();                          /*1. 设置当前线程的scheduler, 就是运行任务线程的线程局部变量都有t_scheduler参数，都把每个线程的t_scheduler设置为 调度器实例*/
    if(sylar::GetThreadId() != m_rootThread)        //   非caller=true创建调度器线程执行run时是协程
        setSchedulerFiber(Fiber::GetThis().get());  /*2. 设置当前线程的执行run方法的那个协程fiber*/ // 此时这些纯任务线程运行run函数时是线程模式。运行到GetThis() 将任务线程变成主协程，并设置为调度协程

    /**
     * 运行到这里。就是所有的参与调度器的协程了。剩下的就是 调度器协程执行了：t_scheduler_fiber
     * 调度协程就是执行下面的run循环调度函数（从队列中取协程->切换到任务协程运行->回到循环调度协程）
     * 但是呢，如果调度协程while 循环中一直运行取到任务协程运行还好。就没有浪费时间，但是如果任务队列没有呢？while循环就一直占用CPU。怎么办呢？
     * 所以就需要一个idle协程 ，让没有任务协程的时候，调度协程切换到这个idle协程，idle协程处理该做的事情，
     * 比如epoll_wait() 让出CPU等待；或者占用CPU 等待（比如自旋锁等待）等等。
     *
     * 我们可以将调度器的运行看作三个核心“角色”的协作：
     *      1. 调度协程（Scheduler Fiber）：即执行 Scheduler::run()函数的协程。它的核心职责是一个 “决策者”​ 和 “切换器”，不断循环执行以下逻辑：
     *          决策：从任务队列中取出一个符合条件的任务（协程或回调函数）。
     *          切换：从调度协程切换到任务协程执行。
     *          回收：任务执行完毕或让出后，切回调度协程，并根据任务状态决定其后续安排（例如，重新调度、挂起等）。
     *      2. 任务协程（Task Fiber）：就是需要被执行的具体任务。
     *      3. idle协程（Idle Fiber）：这是解决你提出的“CPU空转”问题的关键。当任务队列为空时，调度协程不会持续空转消耗CPU，而是主动切换到idle协程。
     *         idle协程的核心职责是 “高效等待”，它会阻塞在一个特定的操作上，让出CPU时间片，直到有新任务到来或被唤醒。
     * idle协程的根本目的是在无任务可调度时，将线程置于一种低功耗的阻塞状态，避免忙等待（busy-waiting），从而极大降低CPU占用率。
     * 查看元宝的回答 https://yb.tencent.com/s/kbpIBrJ3czyh
     */
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;

    /* 3. 协程调度循环 while(true)
     *      3.1. 检查协程消息队列中是否有任务，有任务的话就执行任务
     *      3.2. 没有任务执行的话，就执行idle方法
     */
    while(true)
    {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {   // 从协程的任务队列中取出一个协程; 所有等待执行的协程任务都在这个m_fibers里面 (所有的线程都共享这个协程任务队列)
            // -> 可以像go一样优化？ 多任务队列
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end())
            {
                if(it->thread_id != -1 && it->thread_id != sylar::GetThreadId())
                {   // 任务指定了其他线程，跳过并通知
                    ++it;
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC)
                {   // 任务正在其他线程执行，跳过
                    ++it;
                    continue;
                }

                // 找到适合当前线程的任务，取出
                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            } // 这个过程实现了任务窃取和线程亲和性。调度器并非简单的先进先出，它会尊重任务的“意愿”（指定线程），并避免一个协程同时在多个线程上执行
            tickle_me |= it != m_fibers.end();
        }

        if(tickle_me)
            tickle();

        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT))
        {
            // swapIn()的本质是调度协程将CPU执行权交给任务协程，并阻塞等待其“归还”的过程。
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY)    // Ready状态是要进消息队列的，Hold状态不进
            {   // READY：表示协程主动让出并希望尽快被再次调度。例如，一个协程执行了 YieldToReady()，通常是因为它执行了一个非阻塞的操作但暂时需要让出CPU，或者它希望与其他协程公平地交替执行。
                schedule(ft.fiber);
            }
            else if(ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
            {   // 当一个任务协程执行权交还给调度器后，如果其状态既不是终止(TERM)也不是异常(EXCEPT)，同时也不是就绪(READY)，那么调度器会主动将其状态设置为挂起（HOLD）。
                // 这行代码是协程状态管理的关键。要理解它，我们需要明确 READY和 HOLD状态的区别：
                //     READY：表示协程主动让出并希望尽快被再次调度。例如，一个协程执行了 YieldToReady()，通常是因为它执行了一个非阻塞的操作但暂时需要让出CPU，或者它希望与其他协程公平地交替执行。
                //     HOLD ：表示协程被动挂起，需要等待某个外部条件满足后才能继续。这个条件可能是一个I/O操作完成、一个定时器到期，或者某个锁可用。
                //            处于 HOLD状态的协程不会被调度器自动重新加入队列，必须由外部机制（如I/O管理器、定时器事件）在条件满足后再次调用 schedule()方法将其状态置为 READY并重新调度。
                // --> HOLD状态： 程被动挂起，调度器将其标记为 HOLD后便不再主动管理，直到外部事件将其重新激活。===> 当然，这是Scheduler 的继承者中要执行的逻辑。
                //     对于Scheduler本身协程调度器来说，是没有做任何处理的，单单这个调度器本身逻辑来说，它就是等同于我只设置了状态，"然后就像 ERM/EXCEPT状态 一样，资源回收了(默认协程消费掉了)" ? 错误，
                //     然后调度器不管它了, ft.reset()操作和协程对象的资源回收是两个不同层面的概念. 当协程被置为 HOLD状态后，虽然 ft.reset()解除了调度循环的临时持有，但这个协程的智能指针通常已经被某个外部模块（如 IOManager）所持有。
                //     例如，一个协程因为等待IO事件而挂起，IOManager会在其内部的数据结构中保存这个协程的指针，并监听相应事件。当事件发生时，IOManager会再次调用 schedule方法将该协程重新放回调度队列 。
                //     只要外部模块还持有它，它的引用计数就不为零，对象就会一直存活在内存中，等待被唤醒。 take over by
                ft.fiber->setstate(Fiber::HOLD);
            }
            // 其他TERM/EXCEPT状态：协程任务已完成，资源被回收，相当于被“消费”掉了。
            ft.reset();
        }
        else if(ft.cb)
        {   // 从m_fibers队列取出ft，确认ft.cb不为空, 识别这是一个函数回调任务，函数任务被cb_fiber协程"接管"执行 (这段代码的作用是执行一个函数任务，并管理其状态和资源。)
            // 我们需要将这个回调函数包装成一个协程来执行，因为调度器最终是通过协程的切换来执行任务的。
            if(cb_fiber)    // 如果当前已经有一个回调协程（cb_fiber存在），则重置它，即用新的回调函数重新初始化这个协程
                cb_fiber->reset(ft.cb);
            else            // 否则，创建一个新的协程，其执行体为回调函数ft.cb
                cb_fiber.reset(new Fiber(ft.cb));
            ft.reset();     // 重置ft（将ft.cb置为空，ft.fiber置为空，ft.thread置为-1），表示当前任务已经被取出并处理, 防止重复执行。

            cb_fiber->swapIn();     // 执行回调协程：切换到cb_fiber执行
            --m_activeThreadCount;  // 执行完毕，活跃线程数减一

            // 此时协程回到调度协程 : 并根据cb_fiber回调协程执行后的状态进行后续处理
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber); // 如果回调协程的状态为READY，表示它主动让出（比如调用了yieldToReady）并希望再次被调度，所以将它重新加入调度队列
                cb_fiber.reset();   // 重置cb_fiber（释放对回调协程的持有，因为已经交给调度队列了）
            }
            else if(cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);   // 如果回调协程执行出现异常或结束，则重置回调协程（将协程的执行体置为空，相当于释放回调函数）
            }
            else {//if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->setstate(Fiber::HOLD);// 其他状态（通常是HOLD状态），将回调协程的状态设置为HOLD，HOLD状态表示协程被挂起，需要外部事件来唤醒它（例如,等待I/O操作完成）。在这种情况下，我们不会将它重新加入调度队列，而是等待外部事件来重新调度它。
                                                // 这个协程被挂起了，一般在挂起的时候注册到了外部事件管理器中，这里就不用管这个协程了，让那个事件去管理了
                cb_fiber.reset();               // 然后重置cb_fiber（释放对回调协程的持有）
            }
        }
        else {      // 它处理的正是当任务队列为空或取到的任务无效时，调度器进入空闲状态并执行idle协程的逻辑。
                    // 对于这段代码：https://yb.tencent.com/s/Kisr8A6DggBK
            if(is_active)
            {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM)
             {
                MYLOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();   // idle_fiber->swapIn()会将执行权从调度协程切换到 idle协程。idle协程的核心任务是调用 epoll_wait系统调用. 这个调用会请求内核将当前线程挂起
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT)
            {
                idle_fiber->setstate(Fiber::HOLD);
            }
        }
    }
}

void Scheduler::tickle() { MYLOG_INFO(g_logger) << "tickle"; }

bool Scheduler::stopping()
{
    MutexType::Lock lock(m_mutex);
    return m_is_autostop && m_is_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle()
{
    MYLOG_INFO(g_logger) << "idle";
    while(!stopping())
        sylar::Fiber::YieldToHold();
}

void Scheduler::switchTo(int thread)
{
    SYLAR_ASSERT(Scheduler::GetCurrentScheduler() != nullptr);
    if(Scheduler::GetCurrentScheduler() == this)
    {
        if(thread == -1 || thread == sylar::GetThreadId())
            return;
    }
    schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os)
{
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " is stopping=" << m_is_stopping
       << " ]" << std::endl << "    ";
    for(size_t i = 0; i < m_threadIds.size(); ++i)
    {
        if(i)
        {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler* target)
{
    m_caller = Scheduler::GetCurrentScheduler();
    if(target) {
        target->switchTo();
    }
}

SchedulerSwitcher::~SchedulerSwitcher()
{
    if(m_caller) {
        m_caller->switchTo();
    }
}

}
