#include "project.hpp"

namespace ignite
{
    
    static Project *s_Instance = nullptr;

    Project::Project(const ProjectInfo &info)
        : m_Info(info)
    {
    }

    Project::~Project()
    {
        s_Instance = nullptr;
    }

    std::filesystem::path Project::GetAssetFilepath(const std::filesystem::path &filepath)
    {
        auto relPath = std::filesystem::relative(filepath, m_Info.assetFilepath);
        return m_Info.filepath.parent_path() / m_Info.assetFilepath / relPath;
    }

    std::filesystem::path Project::GetRelativePath(const std::filesystem::path &filepath)
    {
        return std::filesystem::relative(filepath, m_Info.filepath.parent_path());
    }

    void Project::SetActiveScene(const Ref<Scene> &scene)
    {
        m_ActiveScene = scene;
    }

    Project *Project::GetInstance()
    {
        return s_Instance;
    }

    Ref<Project> Project::Create(const ProjectInfo &info)
    {
        Ref<Project> project = CreateRef<Project>(info);

        if (project)
            s_Instance = project.get();

        return project;
    }

    void Project::SerializeAssetRegistry()
    {

    }

}