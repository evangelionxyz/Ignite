#pragma once

#include <string>
#include <filesystem>

namespace ignite
{
    struct ProjectInfo
    {
        std::string name;
        std::filesystem::path filepath;
        std::filesystem::path assetFilepath;
    };

    class Project
    {
    public:
        Project();
        
    private:
        ProjectInfo m_Info;
    };
}