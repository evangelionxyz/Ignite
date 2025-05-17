#include "project.hpp"

namespace ignite
{
    
    static Project *s_Instance = nullptr;

    Project::Project(const ProjectInfo &info)
        : m_Info(info)
    {
        s_Instance = this;
    }

    Ref<Project> Project::Create(const ProjectInfo &info)
    {
        return CreateRef<Project>(info);
    }

    void Project::SerializeAssetRegistry()
    {

    }

}