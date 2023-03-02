#include "mprpcconfig.h"

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

void MprpcConfig::LoadConfigFile(const char* config_file)
{
    std::ifstream in(config_file);
    if(!in.is_open())
    {
        std::cout << config_file << " is not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    while(getline(in, line))
    {
        Trim(line);
        if(line.empty() || *line.begin() == '#' )
            continue;
        size_t idx = line.find("=");
        std::string key = line.substr(0, idx);
        std::string value = line.substr(idx + 1);
        m_configMap.insert({key, value});
    }
}

std::string MprpcConfig::Load(const std::string& key)
{
    auto it = m_configMap.find(key);
    if(it == m_configMap.end())
    {
        return "";
    }
    return it->second;
}

void MprpcConfig::Trim(std::string &str)
{
    if(str.empty()) return;
    // remove_if是将满足谓词的字符移到后面，然后返回第一个满足谓词的迭代器
    str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());
}
