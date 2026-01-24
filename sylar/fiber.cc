#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
//#include "scheduler.h"
#include <atomic>

namespace sylar {

//static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};             // 协程数量

static thread_local Fiber* t_fiber = nullptr;               // 当前线程的协程(运行协程)
static thread_local Fiber::ptr t_threadFiber = nullptr;     // 当前线程的主协程
static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");  // 协程的栈大小（初始化默认128K）


class MallocStackAllocator
{
public:
    static void* Alloc(size_t size)             { return malloc(size); }
    static void Dealloc(void* vp, size_t size)  { return free(vp); }
};
using StackAllocator = MallocStackAllocator;


uint64_t Fiber::GetFiberId()    /// ??? 有 协程的就返回协程id. 没有的就是原来的线程，就返回0
{
    if(t_fiber) {return t_fiber->getId(); }
    return 0;
}

Fiber::Fiber()                  /// 每个线程第一个协程的构造(也就是主协程的构造函数，是私有的)  ----- 主协程是没有栈，没有回调函数的
{
    m_state = EXEC;             // 主协程m_id = 0; m_stacksize = 0; INIT state变为执行中EXEC
    SetThis(this);              // 当前初始化de协程就是当前线程的main协程(运行协程)
    if(getcontext(&m_ctx)) { SYLAR_ASSERT2(false, "getcontext"); }  //  当前的协程获取当前线程的上下文context(cpu寄存器栈状态等)初始化给 当前的协程的m_ctx上下文变量。
    ++s_fiber_count;            // 协程数量+1
    MYLOG_DEBUG(SYLAR_LOG_ROOT()) << "Fiber::Fiber mainfiber initize";
}                               // 这个函数实际上并没有创建 额外的协程。它只是把原始的线程虚拟化成了一个协程（就是把原始线程的上下文context函数栈(cpu寄存器栈状态等)存到一个自定义的协程变量中），

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)   /// 这个函数创建真正的一个协程。
    : m_id(++s_fiber_id),
      m_cb(cb)
{
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();   /// 这里设置函数栈大小，如果你传参数为0，则我用配置的栈大小。不为0，就以你给的为准
    m_stack = StackAllocator::Alloc(m_stacksize);                           // 为当前（新的）协程申请对应的上下文context栈空间
    if(getcontext(&m_ctx)) { SYLAR_ASSERT2(false, "getcontext"); }          // 该函数会将当前线程的上下文保存到该结构体m_ctx中, 就是将线程中的上下文信息（处于正在运行的协程的上下文）保存到这个新协程的ucontext_t变量中(初始化它)
                                                                            // 就是用当前线程的上下文来给这个新协程赋值上下文信息，让后再去修改，使其符合自己协程的上下文信息
    m_ctx.uc_link = nullptr;                 // ??? uc_link 为啥不设置为 线程主协程？  这样本线程运行完了自动就回到主协程了，不用后续自己管理了 (我直接在带参数的构造函数内，把uc_link指向&t_threadFiber->m_ctx ? 设置为主协程，如果没有主协程，可以提前判断出来)
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if(!use_caller) { makecontext(&m_ctx, &Fiber::MainFunc, 0); }       // makecontext就是将上面设置好的上下文和这个函数方法 关联好
    else { makecontext(&m_ctx, &Fiber::CallerMainFunc, 0); }

    MYLOG_DEBUG(SYLAR_LOG_ROOT()) << "      Fiber::Fiber create task fiber id = " << m_id;
}

Fiber::~Fiber()
{
    --s_fiber_count;            /// 析构，协程数量-1
    if(m_stack)                 // 主协程没有栈 (每个线程第一个协程的构造, 主协程的栈其实就是线程的栈堆，主协程外的其他协程 都需要另外申请对应的函数栈)
    {                           // 该判断条件是有栈（非主协程），此时就应该能够被析构释放申请的栈空间，所谓能，就是该协程的状态应该是结束了/还在初始化，对它进行assert,然后释放
        SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    else {                      // 没有栈，就是主协程，确认下是不是，1:没有 callback(因为主协程的默认构造函数，没有给它设置callback函数); 2:线程在执行的时候，主协程一直在执行，不会有其他状态, 所以m_state == EXEC
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC); // 线程在执行的时候，主协程一直在执行，不会有其他状态

        Fiber* cur = t_fiber;   // 就是主协程, 就是自己。把主协程 置为nullptr
        if(cur == this) { SetThis(nullptr); }
    }
    MYLOG_DEBUG(SYLAR_LOG_ROOT()) << "      Fiber::~Fiber id = " << m_id << " total=" << s_fiber_count;
}

//重置协程函数，并重置状态
//INIT，TERM, EXCEPT
void Fiber::reset(std::function<void()> cb)
{   /// 重置协程，为了充分利用内存，就是一个协程运行完了，分配的内存还没有释放，我可以基于这个协程的内存重新初始化, 让它成为一个新的协程的执行栈 (基于这个内存，创建一个新的协程)
    SYLAR_ASSERT(m_stack);                                                  // 1. 要想reset，就要第一步判断，当前栈是可用的。
    SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);  // 2. 要想reset, 必须是 非运行状态或者hold状态
    m_cb = cb;                                                              // 3. 更换新的回调函数

    if(getcontext(&m_ctx)) { SYLAR_ASSERT2(false, "getcontext"); }          // 4.  重新初始化ucontext对象
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::call()
{
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back()
{
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

/// 切换到当前协程执行 (从线程的主协程swap到当前协程) : 主协程 -> 任务协程（当前协程）
void Fiber::swapIn()
{   // 将目标协程唤醒到当前执行状态。就是将该协程（正在操作的协程）和当前正在运行的协程进行交换. 把当前正在运行的协程切换到后台。把本协程放进去（执行）
    SetThis(this);                  // 操作的对象协程一定是子协程而非主协程（必须）。所以先将当前线程的协程(运行协程)设置为此协程
//    SYLAR_ASSERT(m_state != EXEC);  // 当前协程显然不能正在运行，  ???
    m_state = EXEC;                 // 当前协程状态在这里改为 执行状态
//    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {  // 主协程上的上下文，就是当前正在运行的的上下文. 把它和当前线程的上下文进行swap
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {  // 主协程上的上下文，就是当前正在运行的的上下文. 把它和当前线程的上下文进行swap
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

/// 切换到后台执行 (把当前协程yeild到后台，(下一步是把main协程唤醒)) : 任务协程（当前协程）-> 主协程
void Fiber::swapOut()
{
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) { SYLAR_ASSERT2(false, "swapcontext"); }
}

/// 设置当前协程 (就是设置当前线程中正在执行的协程（很多个协程，只有一个是当前线程正要执行的）)
void Fiber::SetThis(Fiber* f)   { t_fiber = f; }

/// 返回当前协程 (获取当前线程正在执行的协程)  t_fiber就是指向当前线程正在执行的协程
Fiber::ptr Fiber::GetThis()
{
    if(t_fiber) { return t_fiber->shared_from_this(); }     // 返回的是该协程的智能指针（继承自shared_from_this就可以直接获得其智能指针了--没有申明智能指针）
    Fiber::ptr main_fiber(new Fiber);                       // 若t_fiber不存在，说明现在没有协程，就是没有主协程；这里就创建主协程
    SYLAR_ASSERT(t_fiber == main_fiber.get());              // 主协程创建的时候（调用Fiber()），给t_fiber赋值了，这里就是做一个判断
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

/// 协程切换到后台，并且设置为Ready状态  (YieldToReady()是静态static方法，它的操作对象就是当前正在执行的协程; 它的目的是把当前正在执行的协程设置为Ready状态，切换到后台，[方便后续]切换到主协程)
void Fiber::YieldToReady()
{                                                           // 主协程没必要切到主协程，应该要作判断 assert(GetThis() != t_threadFiber)
    Fiber::ptr cur = GetThis();                             // 拿到当前（正在执行的）协程
    SYLAR_ASSERT(cur->getState() == EXEC);
    cur->setstate(READY);                                   // 1. 把当前协程的状态设置为READY,
    cur->swapOut();                                         // 2. 把当前协程切出去（到后台）
}

/// 协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold()
{                                                           // 主协程没必要切到主协程，应该要作判断 assert(GetThis() != t_threadFiber)
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur->getState() == EXEC);
    //cur->setstate(HOLD);
    cur->swapOut();
}

/// 总协程数
uint64_t Fiber::TotalFibers()   { return s_fiber_count; }

void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();                                             // 拿到当前（正在执行的）协程
    SYLAR_ASSERT(cur);                                                      // 当前协程不能为空
    try {
        cur->m_cb();                                                        // 执行当前协程的函数
        cur->m_cb = nullptr;                                                // 当前协程的函数执行完毕后，执行的回调函数置为nullptr, 因为用的是智能指针，不用的东西置为nullptr，使得引用计数减1，能及时释放
        cur->setstate(TERM);                                                // 当前协程的函数执行完毕后, 修改当前协程状态为TERM (DONE)
    } catch (std::exception& ex) {
        cur->setstate(EXCEPT);                                              // 非正常结束时）当出了问题时的异常结束，则 当前协程的状态为异常（表示结束，但是不正常的状态）
        MYLOG_DEBUG(SYLAR_LOG_ROOT()) << "Fiber Except: " << ex.what()      // 如果捕捉到异常的话，就打印出日志  (getthis是默认构造  创建的是主协程，主协程没有回调函数，就会产生异常)
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString(0, 0, "    ");
    } catch (...) {                                                         // 如果没有捕捉到异常的话，就打印出日志
        cur->setstate(EXCEPT);
        MYLOG_DEBUG(SYLAR_LOG_ROOT()) << "Fiber Except"
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString(0, 0, "    ");
    }

    // return to main fiber (可以在含参构造里面将uc_link指向主协程也可以完成)  不用裸指针，直接用t_fiber swap_out，然后释放cur
    auto raw_ptr = cur.get();
    cur.reset();                                        // 如果没有这一局。那么在下面swap()出去以后，就会没有出作用域。也就意味着 智能指针没有减一。导致后面释放不了。所以这里提前将这个指针释放掉 reset() == 智能指针-1
    raw_ptr->swapOut();                                 // swapout() 是当前协程执行完毕后退出，因为要人工指定退出到哪里。需要用它来指定回到主协程。用裸指针来切换

    // swapout()以后，这个MainFunc()函数后面的内容就不会再执行了。因为它不会再回来了
    // 要不要给协程取名称呢？可以。很多时候协程只是完成了一个任务，它自己就结束了，协程的复用程度低，它的名字可能就不是很重要
    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc()
{
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->setstate(TERM);
    } catch (std::exception& ex) {
        cur->setstate(EXCEPT);
        MYLOG_DEBUG(SYLAR_LOG_ROOT()) << "Fiber Except: " << ex.what()
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString(0, 0, "    ");
    } catch (...) {
        cur->setstate(EXCEPT);
        MYLOG_DEBUG(SYLAR_LOG_ROOT()) << "Fiber Except"
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString(0, 0, "    ");
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    SYLAR_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}
