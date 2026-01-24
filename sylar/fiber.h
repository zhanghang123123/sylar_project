/**
 * @file fiber.h
 * @brief 协程封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-05-24
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

namespace sylar {

//class Scheduler;

// @brief 协程类 (io密集型 有优势 计算型没有)
class Fiber : public std::enable_shared_from_this<Fiber> {
//friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {        /// 协程状态
        INIT,           /// 初始化状态
        HOLD,           /// 暂停状态
        EXEC,           /// 执行中状态
        TERM,           /// 结束状态 DONE
        READY,          /// 可执行状态
        EXCEPT          /// 异常状态
    };
private:
    Fiber();            /// @brief 无参构造函数   @attention 每个线程第一个协程的构造

public:
    /**
     * @brief 构造函数
     * @param[in] cb 协程执行的函数
     * @param[in] stacksize 协程栈大小
     * @param[in] use_caller 是否在MainFiber上调度
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    void reset(std::function<void()> cb);               /// @brief 重置协程执行函数,并设置状态  @pre getState() 为 INIT, TERM, EXCEPT  @post getState() = INIT
    void swapIn();                                      /// 将当前协程切换到运行状态(就是这个线程上，之前是另一个协程在运行，现在将切换到该协程运行) @pre getState() != EXEC @post getState() = EXEC
    void swapOut();                                     /// 将当前协程切换到后台
    void call();                                        /// @brief 将当前线程切换到执行状态 @pre 执行的为当前线程的主协程
    void back();                                        /// @brief 将当前线程切换到后台   @pre 执行的为该协程    @post 返回到线程的主协程

    uint64_t getId() const      { return m_id; }        /// @brief 返回协程id
    State getState() const      { return m_state; }     /// @brief 返回协程状态
    void setstate(State state)  { m_state = state; }

public:
    static void SetThis(Fiber* f);                      /// 设置当前线程的运行协程  @param[in] f 运行协程
    static Fiber::ptr GetThis();                        /// @brief 返回当前所在的协程
    static void YieldToReady();                         /// @brief 将当前协程切换到后台,并设置为READY状态   @post getState() = READY
    static void YieldToHold();                          /// @brief 将当前协程切换到后台,并设置为HOLD状态    @post getState() = HOLD
    static uint64_t TotalFibers();                      /// @brief 当前协程的总数量

    static void MainFunc();                             /// 协程执行函数   @post 执行完成返回到线程主协程
    static void CallerMainFunc();                       /// @brief 协程执行函数   @post 执行完成返回到线程调度协程
    static uint64_t GetFiberId();                       /// @brief 获取当前协程的id

private:
    uint64_t m_id = 0;                                  /// 协程id (m_fiber_id)
    uint32_t m_stacksize = 0;                           /// 协程运行栈大小
    State m_state = INIT;                               /// 协程状态    (@@@@枚举定义必须初始化，不然 忘记了会导致一些不明行为，特别是对枚举值判断switch or ifelse)
    ucontext_t m_ctx;                                   /// 协程上下文
    void* m_stack = nullptr;                            /// 协程运行栈指针 指向(栈的内存空间)
    std::function<void()> m_cb;                         /// 协程运行函数
};

}

#endif
