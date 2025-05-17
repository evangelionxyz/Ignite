#pragma once

#include "ipanel.hpp"

#include "ignite/asset/asset.hpp"

#include <filesystem>
#include <stack>
#include <map>

namespace ignite {

    class Project;
    class EditorLayer;

    class ContentBrowserPanel : public IPanel
    {
    public:
        explicit ContentBrowserPanel(const char *windowTitle);

        void SetActiveProject(const Ref<Project> &project);

        void RenderFileTree(const std::filesystem::path &directory);

        virtual void OnGuiRender() override;

        struct TreeNode
        {
            AssetHandle handle = AssetHandle(0);

            std::filesystem::path path;
            std::map<std::filesystem::path, uint32_t> children;

            uint32_t parent = static_cast<uint32_t>(-1);
            TreeNode(const std::filesystem::path &path, AssetHandle handle)
                : path(path), handle(handle)
            {
            }
        };

    private:
        void RefreshEntryPathList();

        void RefreshAssetTree();
        void LoadAssetTree(const std::filesystem::path &directory);

        Ref<Project> m_ActiveProject;

        std::vector<TreeNode> m_TreeNodes;
        int m_ThumbnailSize = 64;

        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;
        std::filesystem::path m_SelectedFileTree;

        std::stack<std::filesystem::path> m_BackwardPathStack;
        std::stack<std::filesystem::path> m_ForwardPathStack;
        std::vector<std::filesystem::path> m_PathEntryList;
    };
}