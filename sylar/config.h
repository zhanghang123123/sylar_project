/**
 * @file config.h
 * @brief 配置模块
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-05-22
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <iostream>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>   // 系统自带的
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>

#include "log.h"
#include "util.h"

#include <yaml-cpp/yaml.h>

namespace sylar {

/// 模板类不能实现在cpp，因为模板类在运行的时候才实现，所以连接的时候如果分离，就会报错，

/* ******************** 一条配置信息： name: value # 注释信息（description）********************
 * ******************** 配置变量的基类(该基类 包含 name 和 注释) ********************
 */
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name)                                                           // name 配置参数名称[0-9a-z_.]
        ,m_description(description)                                             // description 配置参数描述
    {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);// 将名称变成大小写不明感的(只有小写)
    }
    virtual ~ConfigVarBase() {}                                                 // 有具体类型的子类，所以需要变成 虚析构

    const std::string& getName() const          { return m_name; }              // 返回配置参数名称
    const std::string& getDescription() const   { return m_description; }       // 返回配置参数的描述

    virtual std::string toString() = 0;                                         // 转成字符串
    virtual bool fromString(const std::string& value) = 0;                      // 从字符串初始化值
//    virtual std::string getTypeName() const = 0;                                // 返回配置参数值的类型名称

protected:
    std::string m_name;                                                         // 配置参数的名称
    std::string m_description;                                                  // 配置参数的描述
};


template<class F, class T>                  // F from_type, T to_type(基础类型) 把F 转成T
class LexicalCast{
public:
    T operator()(const F& v){
        return boost::lexical_cast<T>(v);   // 基础类型的转换度可以
    }
};
/// ******************** 偏特化 ********************
template<class T>
class LexicalCast<std::string, std::vector<T> >{        // str to vector 这里只考虑一维vector  输入int_vector: [[10, 20, 30], [11, 22, 33]]是不可以的
public:
    std::vector<T> operator()(const std::string& v){    // v = "vector1: [11, 22, 33]\nvector2: [101, 202, 303]"
        std::cout << " LexicalCast before YAML::Load v : "  << typeid(v).name() << "; "   << v << std::endl;
        YAML::Node node = YAML::Load(v);                // YAML::Load(v) 将字符串转换为 YAML对应的YAMLType (vector对应的就是Sequence)
        std::cout << " LexicalCast after YAML::Load node : "<< node.Type() << "; " << node << std::endl;  //YAML::Sequence
        typename std::vector<T> vec;                                            // 定义的返回类，这个时候用typename
        std::stringstream ss;
        std::cout << " LexicalCast after YAML::Load node.size() : " << node.size() << std::endl;
        for (size_t i = 0; i < node.size(); i++)
        {
            ss.str("");
            ss << node[i];
            std::cout << " LexicalCast after YAML::Load ss : " << ss.str() << std::endl;
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::vector<T>, std::string>{         // vector to str 这里只考虑一维vector
public:
    std::string operator()(const std::vector<T>& v){
//        std::cout << " LexicalCast before operator v : "  << typeid(v).name() << "; "   << v << std::endl;
//        std::string ret = YAML::Dump(v);
//        MYLOG_ERROR(SYLAR_LOG_ROOT()) << " LexicalCast(vector to str) : " << ret;

        YAML::Node node;
//        std::cout << " LexicalCast push back v : "  << v << std::endl;
        for (auto& i : v)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            std::cout << " LexicalCast push back node : "  << node << std::endl;
        }
        std::stringstream ss;
        ss << node;
        MYLOG_ERROR(SYLAR_LOG_ROOT()) << " LexicalCast(vector to str) : " << ss.str();
        return ss.str();
    }
};



/* ******************** 配置变量类(继承 配置变量的基类) ********************
 * FromStr类中有方法将str转换成T类型 : T operator() (const std::string&)
 * ToStr类中有方法将T转换成str类型 : std::string operator() (const T&)
 *
 *  构造函数: 构造函数初始化了配置项的名称、描述和默认值。
 *      name：配置项的名称。
 *      default_value：配置项的默认值。
 *      description：配置项的描述信息。
 *
 *  toString(): 将配置项的值转换为字符串。
 *  fromString(const std::string& value):
 *      功能：将字符串转换为配置项的值，并更新 m_value
 *  setValue(const T& value):
 *      功能：更新配置项的值，并触发所有注册的回调函数。
 *      实现：如果新值与旧值相同，直接返回。
 *           否则，遍历 m_callbacks，依次调用回调函数，传入旧值和新值。
 *           最后更新 m_value。

这个模板声明是 C++ 中非常典型的**模板参数默认值**和**策略模式**的结合。它的目的是为 `ConfigVar` 类提供灵活的类型转换机制。以下是对这段代码的详细解释：

---

### 1. **模板声明**
```cpp
template<class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
```
https://blog.csdn.net/qq_35858961/article/details/145195418?fromshare=blogdetail&sharetype=blogdetail&sharerId=145195418&sharerefer=PC&sharesource=qq_35858961&sharefrom=from_link


 */

// 使用模板类来实现不同值类型的子类
template<class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>    // 值为默认的基础类型的转换函数
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void (const T& old_value, const T& new_value)> on_change_callback;

    ConfigVar(const std::string& name, const T& default_value, const std::string& description)
        :ConfigVarBase(name, description)
        ,m_value(default_value)
    {
    }
    // ~ConfigVar() {}  // 不用

    std::string toString() override     // 原本想把此函数提出去放在公共函数中，但是看到 catch中的输出信息， 它是专有的
    {
        try{
            // return boost::lexical_cast<std::string>(m_value);    // 此方法只对简单Scalar类型有用
            return ToStr()(m_value);
        } catch(const std::exception& e){
            MYLOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception" << e.what() << " convert: " << typeid(m_value).name() << " to string";
            // std::cerr << e.what() << '\n';
        }
        return "";
    }

    bool fromString(const std::string& value) override
    {   // "vector1: [11, 22, 33]\nvector2: [101, 202, 303]"
        try{
            // m_value = boost::lexical_cast<T>(value);                             // 此方法只对简单Scalar类型有用
            setValue(FromStr()(value));     // m_value = FromStr()(value);          // 仿函数 实现
        }
        catch(const std::exception& e)
        {
            MYLOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception " << e.what() << " convert: string to " << typeid(m_value).name();
            // std::cerr << e.what() << '\n';
        }
        return false;
    }

    const T getValue() const                                { return m_value; }
    void setValue(const T& value) {
        if(value == m_value)
            return;
        for(auto& i : m_callbacks) {
            i.second(m_value, value);
        }
        m_value = value;
    }
    void addListener(uint64_t key, on_change_callback cb)   { m_callbacks[key] = cb; }
    void deleteListener(uint64_t key)                       { m_callbacks.erase(key); }
    void clearListener()                                    { m_callbacks.clear(); }
    on_change_callback getListener(uint64_t key) {
        auto it = m_callbacks.find(key);
        return it == m_callbacks.end() ? nullptr : it->second;
    }

private:
    T m_value;                                              // 存储配置项的当前值，类型为 T
    std::map<uint64_t, on_change_callback> m_callbacks;     // 存储配置项值变化时的回调函数，键为 uint64_t，值为 on_change_callback
};



/*  ******************** 配置管理类 ********************
 *  Config 类是一个单例类,是配置系统的核心管理类:
 *      1. 负责注册、查找和管理所有的配置项: 用于管理所有的配置项, 提供了查找和注册配置项的功能
 *      2. 并支持从 YAML 文件加载配置。
 *  它的设计目标是提供一个统一的接口来操作配置项，并支持从外部数据源（如 YAML 文件）加载配置。就是这个类管理配置的读取解析，以及存放每一条配置
 */
class Config {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap; // std::unordered_map

    // 查找
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name)
    {
        auto it =  s_datas.find(name);
        if(it == s_datas.end()) return nullptr;                             // 未找到
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);         // 找到转换成智能指针
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description = "")
    {                                               // 诚实的讲，我觉得这个函数叫做 register()更好。 注册默认（default）配置
        auto tmp = Lookup<T>(name);
        if(tmp)  MYLOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name = " << name << " exists";
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") != std::string::npos)
        {                                                                   // 发现异常
            MYLOG_ERROR(SYLAR_LOG_ROOT()) <<"Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }
        typename ConfigVar<T>::ptr value(new ConfigVar<T>(name, default_value, description));
        s_datas[name] = value;
        return value;
    }

    // YAML与日志的整合
    // ConfigVarBase::ptr Config::LookupBase(const std::string& name)
    // {
    //     auto it = s_datas.find(name);
    //     return it == s_datas.end() ? nullptr : it->second;
    // }        // 一个方法 只被这个类使用，就写在这个类中

    static void LoadFromYaml(const YAML::Node& root);               // (static方法)从YAML配置文件中加载配置，并将其应用到内存中的配置变量中
    static ConfigVarBase::ptr LookupBase(const std::string& name);  // 查找配置参数,返回配置参数的基类(name 配置参数名称)

private:
    static ConfigVarMap s_datas;
};


/// @brief 在这里声明 s_datas 会导致释放s_datas的时候，多次释放。报：error *** Error in `./bin/test_config': double free or corruption (fasttop): 0x0000000001be0720 ***
// Config::ConfigVarMap Config::s_datas = std::map<std::string, ConfigVarBase::ptr>();  
// Config::ConfigVarMap Config::s_datas = Config::ConfigVarMap();
// Config::ConfigVarMap Config::s_datas;




/// ******************** 配置变量的基类（实现2） ********************
/* 这里是独立的配置系统的实现，不使用上面的基类ConfigVarBase(多一层结构)， ==> not good
 *  为了实现一个功能强大的配置系统，支持分层配置、动态加载、类型安全、多格式支持和分布式配置中心，我们可以设计一个模块化的配置管理系统。
    以下是一个基于 C++ 的实现示例，结合了上述所有特点。
        1. 设计目标
            分层配置：支持默认配置、环境配置和用户配置的优先级合并。
            动态加载：支持配置的热更新，并通过回调函数通知变化。
            类型安全：使用模板和强类型表示配置项。
            多格式支持：支持 YAML、JSON、TOML 等多种格式。
            分布式配置中心：支持从分布式配置中心（如 etcd）加载配置。
        2. 实现代码
            (1) 核心类设计
                ConfigManager：配置管理器，负责加载、合并和提供配置。
                ConfigSource：配置源接口，支持文件、环境变量、命令行参数和分布式配置中心。
                ConfigVar：配置项，封装类型安全的配置值，并支持回调函数。
            (2) 代码实现 如下
 *
 */
 template<class T>
 class ConfigVar2 {
 public:
     typedef std::shared_ptr<ConfigVar2> ptr;

     ConfigVar2(const std::string& name, const T& default_value, const std::string& description = "")
         :m_name(name)                  //param[in] name 配置参数名称[0-9a-z_.]
         ,m_value(default_value)
         ,m_description(description)    //param[in] description 配置参数描述
     {
         std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);// 将名称变成大小写不明感的
     }
     virtual ~ConfigVar2() {}

     const std::string& getName() const          { return m_name; }         // 返回配置参数名称
     const std::string& getDescription() const   { return m_description; }  // 返回配置参数的描述
     const T getValue() const                    { std::lock_guard<std::mutex> lock(m_mutex); return m_value; }
     void setValue(const T& v)                   { m_value = v; }

     virtual std::string toString() = 0;                                     /// 转成字符串
     virtual bool fromString(const std::string& val) = 0;                    /// 从字符串初始化值
     // virtual std::string getTypeName() const = 0;                            /// 返回配置参数值的类型名称

 private:
     std::string m_name;                                                     /// 配置参数的名称
     T m_value;                                                              /// 配置参数的值
     std::string m_description;                                              /// 配置参数的描述
     mutable std::mutex m_mutex;
 };

}

#endif
