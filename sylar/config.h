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
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {


/// @brief 配置变量的基类
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    /** @brief 构造函数
     * @param[in] name 配置参数名称[0-9a-z_.]
     * @param[in] description 配置参数描述
     */
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name),
        m_description(description) 
    {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() {}                                         

    const std::string& getName() const          { return m_name;}           /// 返回配置参数名称
    const std::string& getDescription() const   { return m_description;}    /// 返回配置参数的描述
    virtual std::string toString() = 0;                                     /// 转成字符串
    virtual bool fromString(const std::string& val) = 0;                    /// 从字符串初始化值
    virtual std::string getTypeName() const = 0;                            /// 返回配置参数值的类型名称

protected:
    std::string m_name;                                                     /// 配置参数的名称
    std::string m_description;                                              /// 配置参数的描述
};


/// @brief 配置变量的基类
template<class T>
class ConfigVar {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    /** @brief 构造函数
     * @param[in] name 配置参数名称[0-9a-z_.]
     * @param[in] description 配置参数描述
     */
    ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
        :m_name(name),
        m_value(default_value),
        m_description(description) 
    {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVar() {}                                         

    const std::string& getName() const          { return m_name;}           /// 返回配置参数名称
    const std::string& getDescription() const   { return m_description;}    /// 返回配置参数的描述
    const T getValue() const                    { return m_value; }
    void setValue(const T& v)                   { m_value = v; }

    virtual std::string toString() = 0;                                     /// 转成字符串
    virtual bool fromString(const std::string& val) = 0;                    /// 从字符串初始化值
    // virtual std::string getTypeName() const = 0;                            /// 返回配置参数值的类型名称

protected:
    std::string m_name;                                                     /// 配置参数的名称
    T m_value;                                                              /// 配置参数的值
    std::string m_description;                                              /// 配置参数的描述
};

}