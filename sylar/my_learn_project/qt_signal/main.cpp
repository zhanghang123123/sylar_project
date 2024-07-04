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
template<class Ret, typename ...Args>
class SlotBase
{
public:
    virtual ~SlotBase() = default;
    virtual Ret operator()(Args&&... args) = 0;
};



template<class TReciver, class TParam>
class Slot : public SlotBase<TParam>
{
private:
    TReciver* m_pReciver;   // 定义一个接收者的指针和成员函数指针
    void (TReciver::*m_func)(TParam param); // 成员函数指针用来存放 槽函数的函数指针

public:
    Slot(TReciver* pObj, void(TReciver::*func)(TParam))
    {
        this->m_pReciver = pObj;
        this->m_func = func;
    }
};

template<class TParam>
class Signal
{
private:
    std::vector<SlotBase<TParam>* > m_signal_vector;
public:
    template<class TReciver>
    void addSlot(TReciver* pObj, void (TReciver::*func)(TParam))
    {
        m_signal_vector.push_back(new Slot<TReciver, TParam>(pObj, func));
    }
    void operator()(TParam param)
    {
        for(SlotBase<TParam>* p : m_signal_vector)
            p->slotFunction(param);
    }
};

#define connect(sender, signal, reciver, method) (sender)->signal.addSlot(reciver, method)

class ReciverPosition
{
public:
    void func(int param1)
    {
        std::cout <<"recive param1=" << param1 << std::endl;
    }
};

class ReciverCanvas
{
public:
    void func(int param1)
    {
        std::cout <<"recive param2=" << param1 << std::endl;
    }
};

class SendObj
{
public:
    Signal<int> valueChanged;

public:
    void testSignal(int value)
    {
        valueChanged(value);
    }
};

int main(int argc, char *argv[])
{

    ReciverPosition* p1 = new ReciverPosition;
    ReciverCanvas* c2 = new ReciverCanvas;

    SendObj* sd = new SendObj;

    connect(sd, SendObj::valueChanged, p1, &ReciverPosition::func);
    connect(sd, SendObj::valueChanged, c2, &ReciverCanvas::func);

    sd->testSignal(1000);

    return 0;
}

