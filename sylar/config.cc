#include  "config.h"

#include <list>
#include "log.h"
#include <iostream>

namespace sylar
{
// Config::ConfigVarMap Config::s_datas = std::map<std::string, ConfigVarBase::ptr>();  
// Config::ConfigVarMap Config::s_datas = Config::ConfigVarMap();
Config::ConfigVarMap Config::s_datas;


/*  ListAllMember 函数的作用是递归地遍历YAML节点，并将每个节点的路径和节点本身存储到一个列表中。
 *  这个函数主要用于将YAML文件中的嵌套结构扁平化，方便后续处理。
 *
 */
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

/*  这个函数的主要作用是从'YAML配置文件'中加载配置，并将其存储到内存中
 *      (这里传入的是刚从文件中读取的yaml::Node, root变量名就指的是这个文件yaml的那个根节点): 用法:
 *          YAML::Node root = YAML::LoadFile("xxx.yml");
            sylar::Config::LoadFromYaml(root);
 *  整个LoadFromYaml只处理 yaml层级的map结构 内容， 即 Sequence Null 和 Scalar 统一当做 Scalar 处理
 *  且 key 与 value的关系 已经确定（数据按照已经确定的关系去处理）
 */
void Config:: LoadFromYaml(const YAML::Node &root)                          // 该方法 将yaml中的配置覆盖到原有配置的核心方法
{
    std::list<std::pair<std::string, const YAML::Node> > all_nodes;         // 节点类型<string,YAML::Node>
    ListAllMember("", root, all_nodes);                                     // ListAllMember 函数将YAML文件中的嵌套结构扁平化

    for (auto it = all_nodes.begin(); it != all_nodes.end(); it++){
        std::cout << it->first << std::endl;
    }
    for (auto it = all_nodes.begin(); it != all_nodes.end(); it++){
        std::cout << it->second << std::endl;
    }
    for(auto& i : all_nodes) 
    {
        std::string key = i.first;
        if(key.empty())
            continue;

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);   // 没找到 约定的配置item Node就遗弃了

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
