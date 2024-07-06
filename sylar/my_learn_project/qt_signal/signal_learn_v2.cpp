// 【【现代C++】函数式编程优雅实现信号槽】 https://www.bilibili.com/video/BV1nf421B7Wk/?share_source=copy_web&vd_source=7211be65ee14b6c3343765a9e8d653ef
// #pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <functional>

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

struct Input
{
    void main_loop()
    {
        int i;
        while(std::cin >> i)
        {
            foo_callback(i, 1);                 // foo->on_input(i, 1);
            bar_callback(i%3);                  // bar->on_input(3);
            no_name_callback(i+1);
        }
    }

    std::function<void(int, int)> foo_callback;     // Foo* foo;
    std::function<void(int)> bar_callback;          // Bar* bar;
    std::function<void(int)> no_name_callback;      // 传入匿名函数
};


int main()
{
    Input input;
    Foo foo;
    Bar bar;
    
    /*
    input.foo_callback = &foo;                  // 这种给function函数 传值显然是不对的, https://tongyi.aliyun.com/qianwen/share?shareId=e8f6e06c-50db-42c0-ae53-065222e8bece
    input.bar_callback = &bar;                  // 可以考虑 lambda表达式 （如下）
    */

    input.foo_callback =        [&foo](int x, int y)  { foo.on_input(x, y); };  // lamada表达式这里是按引用捕获的 [&foo]，即[&foo = foo]; 类似 auto &foo1 = foo;
                                                                                // 如果是 [foo] 或者[foo =foo] 就是按值捕获，则在lamada表达式内部是复制拷贝 , 类似 [foo1 = Bar(foo)] 发生了一次拷贝构造函数
                                                                                // [foo =&foo] 则是指针 捕获
    input.bar_callback =        [&bar](int x)         { bar.on_input(x); };     // [] w为空，就是lamada 默认为空，不捕获变量

    input.no_name_callback =    [](int i)             { std::cout << "no_name callback i=" << i << std::endl; };   // lamada 表达式的 {} 结束要加 ';'

                                                // lamada表达式 实际上就是 一个 struct 结构体 （可以看视频 5 分钟左右 ）

    input.main_loop();

    return 0;
}

/* version 2
 * 输入器类 Input；另外两个类 Foo，Bar想监听Input产生的事件 (比如 Input类中的键盘输入事件，然后这两个类负责接收并处理)
    2. 用现代c++ 函数容器来处理函数指针
    3. 学学lamada 表达式，及各种捕获...

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

