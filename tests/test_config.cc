#include "../sylar/config.h"
#include <iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"


#include "yaml-cpp/yaml.h"


sylar::ConfigVar<int>::ptr g_int_value_config = sylar::Config::Lookup("system.port", (int)8080, "system port");

// sylar::ConfigVar<int>::ptr g_float_value_config = sylar::Config::Lookup("system.value", (float)10.2f, "system value");

sylar::ConfigVar<std::vector<int> >::ptr g_vector_int_value_config = sylar::Config::Lookup("system.int_vector", std::vector<int>{1,2}, "system int vector");  //ListAllMember() 解析完以后就是 system.port 这种key


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

static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node> >& output) 
{
    if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) 
    {
        MYLOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if(node.IsMap()) 
    {
        for(auto it = node.begin(); it != node.end(); ++it) 
        {
            ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

void test_yaml() 
{
    YAML::Node root = YAML::LoadFile("/home/hangzhang/projects/sylar_project/bin/config/log.yml");

    MYLOG_INFO(SYLAR_LOG_ROOT()) << root;
    print_yaml(root, 0);
}

void test_listAllMember()
{
    YAML::Node root = YAML::LoadFile("/home/hangzhang/projects/sylar_project/bin/config/log.yml");
    std::list<std::pair<std::string, const YAML::Node> > all_nodes;             // 将root中的结点进行解析，存放到all_nodes中
    ListAllMember("", root, all_nodes);
    for (auto it = all_nodes.begin(); it != all_nodes.end(); it++){             // 遍历输出all_nodes
        std::cout << it->first << std::endl;
    }
}

void test_config()
{
    MYLOG_INFO(SYLAR_LOG_ROOT()) << "before:" << g_int_value_config->getValue(); 
    // MYLOG_INFO(SYLAR_LOG_ROOT()) << "before:" << g_float_value_config->toString();
    auto v = g_vector_int_value_config->getValue();                             // auto& v = xxx; 后面如果在用到这个变量v .就会报错（getValue()返回的是常量,不能够再次赋值了）
    for(auto& i : v){
        MYLOG_INFO(SYLAR_LOG_ROOT()) << "before int_vector: " << i;
    }
//    MYLOG_INFO(SYLAR_LOG_ROOT()) << "before int_vector to str: " << g_vector_int_value_config->toString();

    YAML::Node root = YAML::LoadFile("/home/hangzhang/projects/sylar_project/bin/config/log.yml");
    sylar::Config::LoadFromYaml(root);
    MYLOG_INFO(SYLAR_LOG_ROOT()) << "after:" << g_int_value_config->getValue(); 
    // MYLOG_INFO(SYLAR_LOG_ROOT()) << "after:" << g_float_value_config->toString();

    v = g_vector_int_value_config->getValue();
    for(auto& i : v){
        MYLOG_INFO(SYLAR_LOG_ROOT()) << "after int_vector: " << i;
    }
//    MYLOG_INFO(SYLAR_LOG_ROOT()) << "after int_vector to str: " << g_vector_int_value_config->toString();
}

int main(int argc, char* argv[])
{
    MYLOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    MYLOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();

    // test_yaml();

    test_config();
    // test_listAllMember();

    return 0;   // 高标准代码 所有非空函数必须有稳定的返回值（if else容易 遗漏）
}
