#pragma once

#include "ipanel.hpp"

#include "ignite/asset/asset.hpp"

#include <filesystem>
#include <stack>
#include <map>

#include "ignite/graphics/texture.hpp"

namespace ignite {

    class Project;
    class EditorLayer;

    struct FileTreeNode
    {
        AssetHandle handle = AssetHandle(0);
        std::filesystem::path path;
        // path, index
        std::map<std::filesystem::path, uint32_t> children;
        uint32_t parent = static_cast<uint32_t>(-1);
        bool isDeleted = false;

        FileTreeNode(const std::filesystem::path &path, AssetHandle handle)
            : path(path), handle(handle)
        {
        }
    };

    struct FileThumbnail
    {
        Ref<Texture> thumbnail;
        uint64_t timestamp = 0;
    };

    class ContentBrowserPanel : public IPanel
    {
    public:
        explicit ContentBrowserPanel(const char *windowTitle);
        void SetActiveProject(const Ref<Project> &project);
        virtual void OnGuiRender() override;

    private:
        void RenderFileTree(FileTreeNode *node);
        void RefreshEntryPathList();
        void RefreshAssetTree();
        void LoadAssetTree(const std::filesystem::path &directory);

        void PruneMissingNodes(uint32_t nodeIndex, const std::filesystem::path &basePath);
        void PruneMissingNodesAlt(uint32_t nodeIndex, const std::filesystem::path &basePath);
        void CollectNodesToDelete(uint32_t nodeIndex, const std::filesystem::path &basePath, std::vector<uint32_t> &nodesToDelete);
        void CollectNodeAndDescendants(uint32_t nodeIndex, std::vector<uint32_t> &nodesToDelete);
        void MarkNodeDeletedRecursive(uint32_t nodeIndex);
        void DeleteSingleNode(uint32_t nodeIndex);
        void UpdateIndicesAfterDeletion(uint32_t deletedIndex);
        void CompactTree();
        std::filesystem::path GetFullPath(uint32_t nodeIndex) const;

        Ref<Project> m_ActiveProject;

        std::vector<FileTreeNode> m_TreeNodes;
        int m_ThumbnailSize = 64;

        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;
        std::filesystem::path m_SelectedFileTree;

        std::stack<std::filesystem::path> m_BackwardPathStack;
        std::stack<std::filesystem::path> m_ForwardPathStack;
        std::vector<std::filesystem::path> m_PathEntryList;

        Ref<Texture> m_Icon;
    };
}
