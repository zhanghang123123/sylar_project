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

#include "log.h"
#include "util.h"

#include <yaml-cpp/yaml.h>

namespace sylar {

/// ******************** 一条配置信息： name: value # 注释信息（description）
/// ******************** 配置变量的基类 ********************
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name),                                                          /// name 配置参数名称[0-9a-z_.]
        m_description(description)                                              /// description 配置参数描述
    {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() {}                                         

    const std::string& getName() const          { return m_name; }              /// 返回配置参数名称
    const std::string& getDescription() const   { return m_description; }       /// 返回配置参数的描述
    virtual std::string toString() = 0;                                         /// 转成字符串
    virtual bool fromString(const std::string& value) = 0;                      /// 从字符串初始化值
    // virtual std::string getTypeName() const = 0;                                /// 返回配置参数值的类型名称

protected:
    std::string m_name;                                                         /// 配置参数的名称
    std::string m_description;                                                  /// 配置参数的描述
};

/// ******************** 配置变量类(继承 配置变量的基类) ********************
template<class T>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    ConfigVar(const std::string& name, const T& default_value, const std::string& description)
        :ConfigVarBase(name, description),
         m_value(default_value)
    {
    }
    // ~ConfigVar() {}                                                          /// 不用
    std::string toString() override                                             /// 原本想把此函数提出去放在公共函数中，但是看到 catch中的输出信息， 它是专有的
    {
        try
        {
            return boost::lexical_cast<std::string>(m_value);
        }
        catch(const std::exception& e)
        {
            MYLOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception" << e.what() 
                                          << "convert: " << typeid(m_value).name() << " to string"; 
            // std::cerr << e.what() << '\n';
        }
        return "";
    }

    bool fromString(const std::string& value) override
    {
        try
        {
            m_value = boost::lexical_cast<T>(value);
        }
        catch(const std::exception& e)
        {
            MYLOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception " << e.what() 
                                          << " convert: string to " << typeid(m_value).name();
            // std::cerr << e.what() << '\n';
        }
        return false;
    }

    const T getValue() const            { return m_value; }
    void setValue(const T& value)       { m_value = value; }
private:
    T m_value;
};



/// ******************** 配置变量的基类（实现2） ********************
// template<class T>
// class ConfigVar2 {
// public:
//     typedef std::shared_ptr<ConfigVar2> ptr;
//     /** @brief 构造函数
//      * @param[in] name 配置参数名称[0-9a-z_.]
//      * @param[in] description 配置参数描述
//      */
//     ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
//         :m_name(name),
//         m_value(default_value),
//         m_description(description) 
//     {
//         std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
//     }
//     virtual ~ConfigVar() {}                                         

//     const std::string& getName() const          { return m_name;}           /// 返回配置参数名称
//     const std::string& getDescription() const   { return m_description;}    /// 返回配置参数的描述
//     const T getValue() const                    { return m_value; }
//     void setValue(const T& v)                   { m_value = v; }

//     virtual std::string toString() = 0;                                     /// 转成字符串
//     virtual bool fromString(const std::string& val) = 0;                    /// 从字符串初始化值
//     // virtual std::string getTypeName() const = 0;                            /// 返回配置参数值的类型名称

// protected:
//     std::string m_name;                                                     /// 配置参数的名称
//     T m_value;                                                              /// 配置参数的值
//     std::string m_description;                                              /// 配置参数的描述
// };



/// ******************** 配置管理类 ********************
class Config {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

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
    {
        auto tmp = Lookup<T>(name);
        if(tmp)  MYLOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name = " << name << " exists";
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._012345678") != std::string::npos)
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

    static void LoadFromYaml(const YAML::Node& root);                       // 使用YAML::Node初始化配置模块
    static ConfigVarBase::ptr LookupBase(const std::string& name);          // 查找配置参数,返回配置参数的基类(name 配置参数名称)

private:
    static ConfigVarMap s_datas;
};


/// @brief 在这里声明 s_datas 会导致释放s_datas的时候，多次释放。报：error *** Error in `./bin/test_config': double free or corruption (fasttop): 0x0000000001be0720 ***
// Config::ConfigVarMap Config::s_datas = std::map<std::string, ConfigVarBase::ptr>();  
// Config::ConfigVarMap Config::s_datas = Config::ConfigVarMap();
// Config::ConfigVarMap Config::s_datas;            

}

#endif