#include  "config.h"

#include <list>
#include "log.h"
#include <iostream>

namespace sylar
{
// Config::ConfigVarMap Config::s_datas = std::map<std::string, ConfigVarBase::ptr>();  
// Config::ConfigVarMap Config::s_datas = Config::ConfigVarMap();
Config::ConfigVarMap Config::s_datas;

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

ConfigVarBase::ptr Config::LookupBase(const std::string &name)
{
    auto it = s_datas.find(name);
    return it == s_datas.end() ? nullptr : it->second;
}

// 整个LoadFromYaml 只处理 map 结构 内容， 即 Sequence Null 和 Scalar 统一当做 Scalar 处理 ，
// 且 key 与 value的关系 已经确定（数据按照已经确定的关系去处理）
void Config:: LoadFromYaml(const YAML::Node &root)                          // 该方法 将yaml中的配置覆盖到原有配置的核心方法
{
    std::list<std::pair<std::string, const YAML::Node> > all_nodes;         // 节点类型<string,YAML::Node>
    ListAllMember("", root, all_nodes);                                     // root中可以获取到日志里面配置的内容，ListAllMember则是把里面的内容分解成一个个节点

    for(auto& i : all_nodes) 
    {
        std::string key = i.first;
        if(key.empty())  continue;

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);

        if(var) 
        {
            if(i.second.IsScalar()) {
                var->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                var->fromString(ss.str());
            }
        }
    }
}

} // namespace sylar
