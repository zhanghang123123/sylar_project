// #pragma once

#include <memory>
#include <vector>
#include <iostream>

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
            foo->on_input(i, 1);
            bar->on_input(3);
        }
    }

    Foo* foo;
    Bar* bar;
};


int main()
{
    Input input;
    Foo foo;
    Bar bar;
    input.foo = &foo;
    input.bar = &bar;
    input.main_loop();

    return 0;
}

/* version 1
 * 输入器类 Input；另外两个类 Foo，Bar想监听Input产生的事件 (比如 Input类中的键盘输入事件，然后这两个类负责接收并处理)
    1、 实现上就要把 这两个类的指针传到 Input 类中，（必须是指针，不能是普通变量，因为传普通变量是拷贝）

    terminal output:
        1
        Foo age=15; i=1; j=1
        Bar age=100 i=3
        2
        Foo age=15; i=2; j=1
        Bar age=100 i=3
        4
        Foo age=15; i=4; j=1
        Bar age=100 i=3
        5555
        Foo age=15; i=5555; j=1
        Bar age=100 i=3

        q
        ~Bar
        ~Foo
*/

