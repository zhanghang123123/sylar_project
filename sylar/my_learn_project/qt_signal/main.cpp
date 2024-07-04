/*
 * 1. 为了使信号和槽机制支持可变数量的参数: C++标准库并不直接支持变长模板参数列表的传递，但我们可以通过使用完美转发和模板元编程来实现这一目标。(支持任意数量参数的信号和槽系统)
 */


#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct Foo
{
    void on_input(int i, int j) const { std::cout << "Foo age=" << age << "; i=" << i << "; j=" << j << std::endl; }
    ~Foo()  { std::cout << "~Foo" << std::endl; }

    int age = 15;
};

struct Bar
{
    void on_input(int i) const { std::cout << "Bar age=" << age << " i=" << i << '\n'; }
    ~Bar()  { std::cout << "~Bar" << std::endl; }

    int age = 100;
};

// **********定义一个支持可变参数的信号: ...Args 可以理解为 参数打包（传递时）； Args... 是参数展开（使用时）**********
template<typename ...Args>
class Signal;

/* 这段代码定义了一个名为SlotBase的模板类，它是所有具体槽（slot）类的基类。
 * SlotBase的设计目的是为了提供一个统一的接口，使得不同的槽类可以被以相同的方式调用，尽管它们可能接受不同类型的参数并返回不同的结果。
 *
 * https://tongyi.aliyun.com/qianwen/share?shareId=e1840779-e74d-402d-ad67-419283ff29a0
 *
 * old version:
 *  template<class TParam>
    class SlotBase
    {
    public:
        virtual ~SlotBase() = default;
        virtual void slotFunction(TParam param) = 0;
    };

*/
template<class RetType, typename ...Args>
class SlotBase
{
public:
    virtual ~SlotBase() = default;
    virtual RetType operator()(Args&&... args) = 0;
};

template<typename RetType, typename Reciver, typename Func, typename ...Args>
class Slot final : public SlotBase<RetType, Args...>
{
public:
    Slot(std::shared_ptr<Reciver> receiver, Func func)
        : m_reciver(receiver),
          m_func(func)
    {}

    RetType operator()(Args&& ...args) override
    {   // 这是一个重载的函数调用运算符，它接受Args...类型的参数，并返回RetType类型的值。override关键字表明这个函数覆盖了基类SlotBase中的纯虚函数。
        auto ptr = m_reciver.lock();
        if (ptr) { return (ptr.get()->*m_func)(std::forward<Args>(args)...); }
        // 如果接收者存在，这里会调用接收者中的槽函数func_，并将std::forward<Args>(args)...转发给该函数。std::forward确保参数以正确的引用类型传递。
        return RetType(); // Return default-constructed return type.
    }

private:
    std::weak_ptr<Reciver> m_reciver;  // 用于存储指向接收者的指针。std::weak_ptr与std::shared_ptr配合使用，可以避免循环引用的问题，确保当接收者不再被引用时，Slot对象可以正确清理。
    Func m_func;                        // m_func是一个成员变量，用于存储指向接收者中槽函数的函数指针。
};

template<typename ...Args>
class Signal
{
public:
    typedef std::shared_ptr<SlotBase<void, Args...>> SlotPtr;

    template<typename Reciver, typename Func>
    void connect(std::shared_ptr<Reciver> receiver, Func func)
    {
        m_slots.emplace_back(std::make_shared<Slot<void, Reciver, Func, Args...>>(receiver, func));
    }
//    void addSlot(Reciver* pObj, void (Reciver::*func)(TParam))
//    {
//        m_signal_vector.push_back(new Slot<Reciver, TParam>(pObj, func));
//    }

    template<typename ...CallArgs>
    void operator()(CallArgs&&... call_args)
    {
        for(auto& slot : m_slots)
//            (*slot)(std::forward<CallArgs>(call_args)...);
            (*slot)(std::forward<CallArgs>(call_args)...);
    }

    template<class ...EmitArgs>
    void emit(EmitArgs&&... emit_args)
    {
        for (auto& slot : m_slots)
            (*slot)(std::forward<EmitArgs>(emit_args)...);
    }

private:
    std::vector<SlotPtr> m_slots;
};

//#define connect(sender, signal, reciver, method) (sender)->signal.addSlot(reciver, method)

class Reciver_test1
{
public:
    void func(int param1, double param2)
    {
        std::cout <<"recive param1=" << param1 <<"; param2=" << param2 << std::endl;
    }
};

class Reciver_test2
{
public:
    void func(int param1, float param2)
    {
        std::cout <<"recive param1=" << param1 <<"; param2=" << param2 << std::endl;
    }
};

//class SendObj
//{
//public:
//    Signal<int> valueChanged;

//public:
//    void testSignal(int value)
//    {
//        valueChanged(value);
//    }
//};

int main(int argc, char *argv[])
{

//    ReciverPosition* p1 = new ReciverPosition;
//    ReciverCanvas* c2 = new ReciverCanvas;

//    SendObj* sd = new SendObj;

//    connect(sd, SendObj::valueChanged, p1, &ReciverPosition::func);
//    connect(sd, SendObj::valueChanged, c2, &ReciverCanvas::func);

//    sd->testSignal(1000);

    auto receiver = std::make_shared<Reciver_test1>();
    Signal<int, double> signal;
    signal.connect(receiver, &Reciver_test1::func);

    signal.emit(10, 3.14); // This will invoke the connected function.

    signal(100,444.4);

    return 0;
}

