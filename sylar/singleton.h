/// 单例模式封装

#ifndef __SYLAR_SINGLETON_H_
#define __SYLAR_SINGLETON_H_

#include <memory>

namespace sylar
{

template<class T, class X = void, int N = 0>  // T 类型 X 为了创造多个实例对应的Tag N 同一个Tag创造多个实例索引
class Singleton
{
public:
    static T* getInstance()
    {
        static T v;
        return &v;
    }
};

template<class T, class X = void, int N = 0>
class SingletonPtr
{
public:
    static std::shared_ptr<T> getInstance()
    {
        static std::shared_ptr<T> v(new T);
        return v; 
    }
};
    
} // namespace sylar


#endif