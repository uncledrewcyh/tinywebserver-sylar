/*
 * @description: 配置系统源文件
*
 */

#include "config.h"
#include <sys/stat.h>

static pudge::Logger::ptr g_logger = PUDGE_LOG_NAME("system");

namespace pudge {

/**
 * @brief: 从yaml文件中读取, 在map中进行读取. 支持一级目录。
 * @details : A:
 *              B: 10    =>  List: ["A.B", 10] ["A.C", str]
 *              C: str  
 */
static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node> >& output) {
    if(prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")
            != std::string::npos) {
        PUDGE_LOG_ERROR(g_logger) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if(node.IsMap()) {
        for(auto it = node.begin();
                it != node.end(); ++it) {

            ListAllMember(prefix.empty() ? it->first.Scalar()
                    : prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

/**
 * @brief: 将Yaml文件中的变量载入到ConfigVarMap，
 * 仅对已存在的Config进行修改. <约定优于配置>
 */
void Config::LoadFromYaml(const YAML::Node& root) {
    std::list<std::pair<std::string, const YAML::Node> > all_nodes;
    ListAllMember("", root, all_nodes);

    for(auto& i : all_nodes) {
        std::string key = i.first;
        if(key.empty()) {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr var = LookupBase(key);

        if(var) {
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

/**
 * @brief: 从conf文件夹中读取配置信息 
 */
static pudge::Mutex s_mutex;
static std::map<std::string, uint64_t> s_file2modifytime;

void Config::LoadFromConfDir(const std::string& path, bool force) {
    std::string absoulte_path = pudge::EnvMgr::GetInstance()->getAbsolutePath(path);
    std::vector<std::string> files;
    ///找出所有的yml文件的路径
    FSUtil::ListAllFile(files, absoulte_path, ".yml"); 

    for (auto& i: files) {
        {
            struct stat st; ///文件状态
            lstat(i.c_str(), &st);
            pudge::Mutex::Lock lock(s_mutex);
            /// 上次yaml文件并未修改的话，则继续
            if(!force && s_file2modifytime[i] == (uint64_t)st.st_mtime) {
                continue;
            }
            s_file2modifytime[i] = st.st_mtime;
        }
        try {
            YAML::Node root = YAML::LoadFile(i);
            LoadFromYaml(root);
            PUDGE_LOG_INFO(g_logger) << "LoadConfFile file="
                << i << " ok";
        } catch (...) {
            PUDGE_LOG_INFO(g_logger) << "LoadConfFile file="
                << i << " failed";
        }
    }

}

/**
 * @brief: 寻找配置空间的 "name" 是否被定义
 */
ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

}