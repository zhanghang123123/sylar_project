// 【【现代C++】函数式编程优雅实现信号槽】 https://www.bilibili.com/video/BV1nf421B7Wk/?share_source=copy_web&vd_source=7211be65ee14b6c3343765a9e8d653ef
// #pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <functional>
#include <vector>

// 形参包语法: ...T 省略号在左边表示 define，定义一个形参包(打包); T... 省略号在右边表示 use，使用一个形参包(拆包)

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


// 致敬QT ，用Signal 记录、管理 监听函数集合(qt 用信号在多个对象之间传递消息，)
struct Signal
{
    // 将监听的函数 的保存和访问放在vector 中, 就目前而言，要求所有的 参数都是一致的（参数类型和参数个数），后面会改为支持不同参数；暂时只能支持相同变量
    std::vector<std::function<void(int)> > callbacks;

    void connect(std::function<void(int)> callback)     //  致敬QT, add_callback() 就是往vector中添加 要监听的 callback函数 
    {
        callbacks.push_back(callback);                  // 更高效的添加可以用  callbacks.push_back(std::move(callback));
    }

    void emit(int i)                        //  致敬QT, emit 发出信号，实际上就是将执行emit 函数的对象的function函数 （全部）执行一次 
    {
        for(auto&& callback : callbacks)    // for循环 函数容器， 可以用万能引用
        {
            callback(i);
        }
    }
};

struct Input
{
    void main_loop()
    {
        int i;
        while(std::cin >> i)
        {
            signal_input.emit(i);
            // for(auto&& callback : callbacks)    // for循环 函数容器， 可以用万能引用
            // {
            //     callback(i);
            // }
        }
    }

    Signal signal_input;
    // // 将监听的函数 的保存和访问放在vector 中, 就目前而言，要求所有的 参数都是一致的（参数类型和参数个数），后面会改为支持不同参数；暂时只能支持相同变量
    // std::vector<std::function<void(int)> > callbacks;

};


int main()
{
    Input input;
    Foo foo;
    Bar bar;
    
    /* version 1
    input.foo_callback = &foo;                  // 这种给function函数 传值显然是不对的, https://tongyi.aliyun.com/qianwen/share?shareId=e8f6e06c-50db-42c0-ae53-065222e8bece
    input.bar_callback = &bar;                  // 可以考虑 lambda表达式 （如下）
    */

    /* version 2
    input.foo_callback =        [&foo](int x, int y)  { foo.on_input(x, y); };  // lamada表达式这里是按引用捕获的 [&foo]，即[&foo = foo]; 类似 auto &foo1 = foo;
                                                                                // 如果是 [foo] 或者[foo =foo] 就是按值捕获，则在lamada表达式内部是复制拷贝 , 类似 [foo1 = Bar(foo)] 发生了一次拷贝构造函数
                                                                                // [foo =&foo] 则是指针 捕获
    input.bar_callback =        [&bar](int x)         { bar.on_input(x); };     // [] w为空，就是lamada 默认为空，不捕获变量

    input.no_name_callback =    [](int i)             { std::cout << "no_name callback i=" << i << std::endl; };   // lamada 表达式的 {} 结束要加 ';'
    */

    /*
    input.callbacks.push_back([&foo](int x)  { foo.on_input(x, 5); });          // 参数只能有一个和 std::vector<std::function<void(int)> > callbacks; 匹配的参数了
    input.callbacks.push_back([&bar](int x)  { bar.on_input(x); });
    input.callbacks.push_back([](int i)  { std::cout << "no_name callback i=" << i << std::endl; });
    */
    input.signal_input.connect([&foo](int x)  { foo.on_input(x, 5); });
    input.signal_input.connect([&bar](int x)  { bar.on_input(x); });
    input.signal_input.connect([](int i)  { std::cout << "no_name callback i=" << i << std::endl; });


    input.main_loop();

    return 0;
}

/* version 3
 * 输入器类 Input；另外两个类 Foo，Bar想监听Input产生的事件 (比如 Input类中的键盘输入事件，然后这两个类负责接收并处理)
    2. 用现代c++ 函数容器来处理函数指针
    3. 学学lamada 表达式，及各种捕获...

      |
      v
  version 4 升级:
    1. 将 监听函数集合单独封装
    2. 解决参数只能固定的缺点。引入模版

    terminal output:
        1
        Foo age=15; i=1; j=1
        Bar age=100 i=1
        no_name callback i=2
        2
        Foo age=15; i=2; j=1
        Bar age=100 i=2
        no_name callback i=3
        4
        Foo age=15; i=4; j=1
        Bar age=100 i=1
        no_name callback i=5
        
        6
        Foo age=15; i=6; j=1
        Bar age=100 i=0
        no_name callback i=7
        7
        Foo age=15; i=7; j=1
        Bar age=100 i=1
        no_name callback i=8
        
        q
        ~Bar
        ~Foo

*/

