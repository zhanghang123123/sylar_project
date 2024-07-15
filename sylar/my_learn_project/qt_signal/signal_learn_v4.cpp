// 【【现代C++】函数式编程优雅实现信号槽】 https://www.bilibili.com/video/BV1nf421B7Wk/?share_source=copy_web&vd_source=7211be65ee14b6c3343765a9e8d653ef
// #pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <functional>
#include <vector>
#include <type_traits>
#include <utility>
#include <string>

/// 形参包语法: ...T 省略号在左边表示 define，定义一个形参包(打包); T... 省略号在右边表示 use，使用一个形参包(拆包)

struct Foo
{
    void on_input(int i, std::string j) const   { std::cout << "Foo age=" << age << "; i=" << i << "; j=" << j << std::endl; }
    void myEmptySlot()                          { std::cout << "myEmptySlot called" << std::endl; }
    ~Foo()                                      { std::cout << "~Foo" << std::endl; }

    int age = 15;
};

struct Bar
{
    void on_input(int i) const  { std::cout << "Bar age=" << age << " i=" << i << '\n'; }
    void on_exit() const        { std::cout << "Bar on exit>>>" << '\n'; }
    ~Bar()                      { std::cout << "~Bar" << std::endl; }

    int age = 100;
};


// 致敬QT ，用Signal 记录、管理 监听函数集合(qt 用信号在多个对象之间传递消息，)
template<typename ...T>
struct Signal
{
    // 将监听的函数 的保存和访问放在vector 中, 就目前而言，要求所有的 参数都是一致的（参数类型和参数个数），后面会改为支持不同参数；暂时只能支持相同变量
    std::vector<std::function<void(T...)> > callbacks;

    void connect(std::function<void(T...)> callback)     //  致敬QT, add_callback() 就是往vector中添加 要监听的 callback函数 
    {
        callbacks.push_back(callback);                  // 更高效的添加可以用  callbacks.push_back(std::move(callback));
    }

    // void emit(T... t)                        //  致敬QT, emit 发出信号，实际上就是将执行emit 函数的对象的function函数 （全部）执行一次  t 是一个 具有形参包（T...）的变量
    // {
    //     for(auto&& callback : callbacks)    // for循环 函数容器， 可以用万能引用
    //     {
    //         callback(t...);
    //     }
    // }

    template<typename... Args>
    void emit(Args&&... args)    
    {
        for(auto& callback : callbacks)    
        {
            callback(std::forward<Args>(args)...);
        }
    }
};


template <typename... Args>
class Signal_C11 
{
public:
    using SlotType = std::function<void(Args...)>;

    // 连接槽函数
    template <typename T, typename Func>
    auto connect(T* instance, Func T::*func) -> std::shared_ptr<SlotType> 
    {
        auto slot = std::make_shared<SlotType>([instance, func](Args... new_args) mutable {
                    (instance->*func)(std::forward<Args>(new_args)...);
                    });
        m_slots.emplace_back(slot);
        return slot;
    }

    // 发射信号
    template <typename... NewArgs>
    void emit(NewArgs&&... new_args) const 
    {
        for (const auto& slot : m_slots) 
        {
            (*slot)(std::forward<NewArgs>(new_args)...);
        }
    }

 private:
  std::vector<std::shared_ptr<SlotType>> m_slots;
};

struct Input
{
    Signal_C11<int, std::string> signal_input;      //为啥 引用 不行。  ？？？  需要提升
    Signal_C11<> signal_exit;   // 支持空的参数
};

//  // C++17 VERSION
// template<class Self, class MemFuc>
// auto bind(Self* self, MemFuc memfuc) 
// {
//     return [self, memfuc](auto ...t) { (self->*memfuc)(t...); };
// }


int main()
{
    Input input;
    Foo foo;
    Bar bar;
    
    input.signal_input.connect(&foo, &Foo::on_input);
    input.signal_exit.connect(&foo, &Foo::myEmptySlot);
    // input.signal_input.connect([](int i, std::string& j)  { std::cout << "no_name callback i=" << i << std::endl; } );  // 封装以后 不在支持这种 了

    input.signal_input.emit(42, "hello world");
    input.signal_exit.emit();

    return 0;
}

/* version 3

  version 5 升级:
    1. 将 支持的变量 多样化 模版实现变参
        假设 Bar 函数中要有一个函数 void on_exit(); 事件，需要监听 main_loop() 中 输入结束事件。这时候变量为空。 这时候支持变参就很重要了(不然 你还要指定一些垃圾类型来占位，不然编译不过)
    2. 解决参数只能固定的缺点。引入模版

      | （version 5 就挺好的了，qt4 的信号槽 差不多就这个样子， 但是它有个缺陷就是 比如使用 input.signal_input.connect([&foo](int x, int y)  { foo.on_input(x, y); }); 中，输入参数我都需要
      |   固定， 也就是说 使用时必须写明，如果我想改<增加或者删除> ，则所有相关的都需要改，） 使用bind 函数来进行绑定
      v
  version 6 升级：
    1. bind() 用法， bind(&bar, &Bar::on_input)  实现  动态参数绑定。 --> 将 槽函数 添加 全部继承到 connect() 函数中

*/

