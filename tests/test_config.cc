#include "../sylar/config.h"
#include <iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"


#include "yaml-cpp/yaml.h"


sylar::ConfigVar<int>::ptr g_int_value_config = sylar::Config::Lookup("system.port", (int)8080, "system port");


void print_yaml(const YAML::Node& node, int level)  // 该函数
{
    // MYLOG_INFO(SYLAR_LOG_ROOT()) << "node.Type() : " << node.Type();
    if(node.IsScalar()) 
    {
        MYLOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    } 
    else if(node.IsNull()) 
    {
        MYLOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ')  << "NULL - " << node.Type() << " - " << level;
    } 
    else if(node.IsMap()) 
    {
        for(auto it = node.begin(); it != node.end(); ++ it)
        {
            MYLOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } 
    else if(node.IsSequence()) 
    {
        for(size_t i = 0; i < node.size(); ++ i) 
        {
            MYLOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() 
{
    YAML::Node root = YAML::LoadFile("/home/hangzhang/projects/sylar_project/bin/config/log.yml");
    print_yaml(root, 0);
}  

void test_config()
{
    MYLOG_INFO(SYLAR_LOG_ROOT()) << "before:" << g_int_value_config->getValue(); 
    // MYLOG_INFO(SYLAR_LOG_ROOT()) << "before:" << g_float_value_config->toString();
    YAML::Node root = YAML::LoadFile("/home/hangzhang/projects/sylar_project/bin/config/log.yml");
    sylar::Config::LoadFromYaml(root);
    MYLOG_INFO(SYLAR_LOG_ROOT()) << "after:" << g_int_value_config->getValue(); 
    // MYLOG_INFO(SYLAR_LOG_ROOT()) << "after:" << g_float_value_config->toString();
}

int main(int argc, char* argv[])
{
    MYLOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    MYLOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();

    // test_yaml();

    test_config();

    return 0;   // 高标准代码 所有非空函数必须有稳定的返回值（if else容易 遗漏）
}