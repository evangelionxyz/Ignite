#include "project.hpp"
#include "ignite/core/string_utils.hpp"
#include "ignite/core/logger.hpp"

#include <fstream>
#include <format>

namespace ignite
{
    std::string s_PremakeTemplate = R"(workspace "{PROJECT_NAME}"
    architecture "x64"
    configurations { "Debug", "Release", "Dist" }
    flags { "MultiProcessorCompile" }
    startproject "{PROJECT_NAME}"
    project "{PROJECT_NAME}"
    kind "SharedLib"
    language "C#"
    dotnetframework "4.8"
    location "%{wks.location}"

    targetdir ( "%{prj.location}/bin" )
    objdir    ( "%{prj.location}/bin/objs" )
    files     { "%{prj.location}/Scripts/**.cs" }

    --
    -- Tell Premake to reference the already‑built IgniteScript.dll
    -- (one line per configuration keeps the HintPath correct)
    --
    filter { "configurations:Debug" }
        local igniteBin = path.join(os.getenv("IgniteEngine"), "bin/Debug")
        links     { path.join(igniteBin, "IgniteScript.dll") }
        -- copylocal { path.join(igniteBin, "IgniteScript.dll") }   -- optional

    filter { "configurations:Release" }
        local igniteBin = path.join(os.getenv("IgniteEngine"), "bin/Release")
        links     { path.join(igniteBin, "IgniteScript.dll") }
        -- copylocal { path.join(igniteBin, "IgniteScript.dll") }

    filter { "configurations:Dist" }
        local igniteBin = path.join(os.getenv("IgniteEngine"), "bin/Dist")
        links     { path.join(igniteBin, "IgniteScript.dll") }
        -- copylocal { path.join(igniteBin, "IgniteScript.dll") }

    filter {}            -- clear filters


    filter "system:linux"
        pic "On"
            
    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        optimize "Off"
        symbols "Default"

    filter "configurations:Release"
        optimize "On"
        symbols "Default"

    filter "configurations:Dist"
        optimize "Full"
        symbols "Off"
)";

    std::string s_BatchScriptTemplate = R"(pushd %~dp0
%IgniteEngine%\scripts\premake\premake5.exe vs2022
dotnet msbuild {PROJECT_NAME}.sln
popd
)";

    std::string s_CSSharpScriptTemplate = R"(using Ignite;
using System;

namespace {PROJECT_NAME}
{
    public class Game : Entity
    {
        public void OnCreate()
        {
            // Initialize you object here
            Console.WriteLine("Hello From C#!");
        }

        public void OnUpdate(float deltaTime)
        {
            // Update loop
        }
    }
}
)";

    static Project *s_ActiveProject = nullptr;

    Project::Project(const ProjectInfo &info)
        : m_Info(info)
    {
        GenerateProject();
    }

    Project::~Project()
    {
    }

    std::filesystem::path Project::GetAssetRelativeFilepath(const std::filesystem::path &filepath) const
    {
        auto basePath = m_Info.filepath.parent_path() / m_Info.assetDirectory;
        return std::filesystem::relative(filepath, basePath);
    }

    std::filesystem::path Project::GetAssetFilepath(const std::filesystem::path &filepath) const
    {
        return m_Info.filepath.parent_path() / m_Info.assetDirectory / filepath;
    }

    std::filesystem::path Project::GetRelativeFilepath(const std::filesystem::path &filepath) const
    {
        return std::filesystem::relative(filepath, m_Info.filepath.parent_path());
    }

    void Project::SetActiveScene(const Ref<Scene> &scene)
    {
        m_ActiveScene = scene;
    }

    std::vector<std::pair<AssetHandle, AssetMetaData>> Project::ValidateAssetRegistry()
    {
        std::vector<std::pair<AssetHandle, AssetMetaData>> invalidRegistry;
        AssetRegistry &assetRegistry = GetAssetManager().GetAssetAssetRegistry();

        for (auto it = assetRegistry.begin(); it != assetRegistry.end();)
        {
            const std::filesystem::path &filepath = GetAssetFilepath(it->second.filepath);
            if (!std::filesystem::exists(filepath))
            {
                invalidRegistry.emplace_back(it->first, it->second);
                it = assetRegistry.erase(it);
            }
            else
            {
                ++it;
            }
        }

        return invalidRegistry;
    }

    Project *Project::GetActive()
    {
        return s_ActiveProject;
    }

    Ref<Project> Project::Create(const ProjectInfo &info)
    {
        Ref<Project> project = CreateRef<Project>(info);
        s_ActiveProject = project.get();

        return project;
    }

    bool Project::BuildSolution()
    {
        // execute build.batch
        std::string buildCommand = (GetDirectory() / m_Info.batchScriptFilepath).generic_string();
        std::system(buildCommand.c_str());

        bool success = std::filesystem::exists(GetDirectory() / m_Info.scriptModuleFilepath);
        // Validate .dll file
        // LOG_ASSERT(success, "[Project] Failed to build Solution");
        return success;
    }

    void Project::GenerateProject()
    {
        std::filesystem::path assetDirectory = GetAssetDirectory();
        if (!std::filesystem::exists(assetDirectory))
            std::filesystem::create_directories(assetDirectory);

        std::filesystem::path scriptDirectory = GetScriptsDirectory();
        if (!std::filesystem::exists(scriptDirectory))
            std::filesystem::create_directories(scriptDirectory);

        // generate the Visual Studio project if there is no solution file
        std::filesystem::path solutionFilepath = GetSolutionFilepath();
        if (!std::filesystem::exists(solutionFilepath))
        {
            std::filesystem::path projectDir = GetDirectory();
    
            // Create dummy c# script when there is no scripts (new project)
            std::filesystem::path defaultCSharpScriptFilepath = scriptDirectory / "Game.cs";
            if (!std::filesystem::exists(defaultCSharpScriptFilepath) || std::filesystem::is_empty(scriptDirectory))
            {
                // copy template
                std::string csharpScriptTemplate = s_CSSharpScriptTemplate;
                stringutils::ReplaceWith(csharpScriptTemplate, "{PROJECT_NAME}", m_Info.name);
                std::fstream outfile = std::fstream(defaultCSharpScriptFilepath, std::ios::out);
                outfile << csharpScriptTemplate;
                outfile.close();
            }

            // copy template
            std::string premakeTemplate = s_PremakeTemplate;
            stringutils::ReplaceWith(premakeTemplate, "{PROJECT_NAME}", m_Info.name);
            std::fstream outfile = std::fstream(GetDirectory() / m_Info.premakeFilepath, std::ios::out);
            outfile << premakeTemplate;
            outfile.close();

            // copy template
            std::string batchScriptTemplate = s_BatchScriptTemplate;
            stringutils::ReplaceWith(batchScriptTemplate, "{PROJECT_NAME}", m_Info.name);
            outfile = std::fstream(GetDirectory() / m_Info.batchScriptFilepath, std::ios::out);
            outfile << batchScriptTemplate;
            outfile.close();

            BuildSolution();
        }

        m_Info.scriptModuleFilepath = std::format("bin/{}.dll", m_Info.name);
    }

    void Project::SerializeAssetRegistry()
    {
    }
}