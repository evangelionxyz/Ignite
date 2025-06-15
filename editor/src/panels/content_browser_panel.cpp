#include "content_browser_panel.hpp"
#include "ignite/project/project.hpp"
#include "editor_layer.hpp"

#include <format>
#include <algorithm>

namespace ignite {

    ContentBrowserPanel::ContentBrowserPanel(const char *windowTitle)
        : IPanel(windowTitle)
    {

        TextureCreateInfo createInfo;
        createInfo.format = nvrhi::Format::RGBA8_UNORM;
        m_Icons["folder"] = Texture::Create("resources/ui/ic_folder.png", createInfo);
        m_Icons["unknown"] = Texture::Create("resources/ui/ic_file.png", createInfo);
    }

    void ContentBrowserPanel::SetActiveProject(const Ref<Project> &project)
    {
        m_ActiveProject = project;

        if (m_ActiveProject)
        {
            // clear directories
            m_PathEntryList.clear();
            m_TreeNodes.clear();

            m_TreeNodes.emplace_back(".", AssetHandle(0));
            
            m_BaseDirectory = m_ActiveProject->GetAssetDirectory();
            m_PathEntryList.push_back(m_BaseDirectory);

            m_CurrentDirectory = m_ActiveProject->GetAssetDirectory();
            RefreshAssetTree();
        }
    }

    void ContentBrowserPanel::RenderFileTree(FileTreeNode *node)
    {
        if (node->path.empty())
            return;

        const std::filesystem::path &filepath = m_ActiveProject->GetAssetFilepath(node->path);
        const std::string filename = filepath.filename().string();
        const bool isDirectory = std::filesystem::is_directory(filepath);

        ImGuiTreeNodeFlags flags = (m_SelectedFileTree == filepath ? ImGuiTreeNodeFlags_Selected : 0)
            | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

        if (!isDirectory)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        const bool opened = ImGui::TreeNodeEx(filename.c_str(), flags, "%s", filename.c_str());

        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (isDirectory)
                {
                    m_BackwardPathStack.push(m_CurrentDirectory);
                    m_SelectedFileTree = m_CurrentDirectory = filepath;
                }
            }
        }

        if (opened && isDirectory)
        {
            for (auto& nodeIndex : node->children | std::views::values)
            {
                RenderFileTree(&m_TreeNodes[nodeIndex]);
            }
            
            ImGui::TreePop();
        }
    }

    void ContentBrowserPanel::OnGuiRender()
    {
        ImGui::Begin("Content Browser");

        ImVec2 regionSize = ImGui::GetContentRegionAvail();
        constexpr ImVec2 navbarBtSize = ImVec2(40.0f, 30.0f);
        const ImVec2 navbarSize = ImVec2(regionSize.x, 45.0f);
        // Navigation bar
        ImGui::BeginChild("##NAV_BUTTON_BAR", navbarSize, ImGuiChildFlags_Borders);

        if (ImGui::Button("<-", navbarBtSize))
        {
            if (!m_BackwardPathStack.empty())
            {
                m_ForwardPathStack.push(m_CurrentDirectory);
                m_CurrentDirectory = m_BackwardPathStack.top();
                m_BackwardPathStack.pop();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("->", navbarBtSize))
        {
            if (!m_ForwardPathStack.empty())
            {
                m_BackwardPathStack.push(m_CurrentDirectory);
                m_CurrentDirectory = m_ForwardPathStack.top();
                m_ForwardPathStack.pop();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("R", navbarBtSize))
        {
            m_ActiveProject->ValidateAssetRegistry();
            PruneMissingNodes(0, m_ActiveProject->GetAssetDirectory());
            RefreshAssetTree();
            CompactTree();
        }

        ImGui::EndChild();

        if (m_ActiveProject)
        {
            // Left side directory tree
            ImGui::BeginChild("left_item_browser", { 300.0f, 0.0f }, ImGuiChildFlags_ResizeX);
            for (auto it = m_TreeNodes.begin() + 1; it != m_TreeNodes.end(); ++it)
            {
                if (it->parent == 0)
                    RenderFileTree(&(*it));
            }
            ImGui::EndChild();
            ImGui::SameLine();

            // Files
            ImGui::BeginChild("##FILE_LISTS", { 0.0f, 0.0f });

            // Insert path nodes
            FileTreeNode *node = m_TreeNodes.data();
            auto f = Project::GetActiveAssetDirectory();
            const auto &relativePath = std::filesystem::relative(m_CurrentDirectory, f);

            for (const auto &path : relativePath)
            {
                if (node->path == relativePath)
                {
                    break;
                }

                if (node->children.contains(path))
                {
                    node = &m_TreeNodes[node->children[path]];
                }
            }

            if (node->children.empty())
            {
                ImGui::Text("This folder is empty");
            }

            static float padding = 12.0f;
            const float cellSize = static_cast<float>(m_ThumbnailSize) + padding;
            const float &childWindowWidth = ImGui::GetContentRegionAvail().x;

            int columnCount = static_cast<int>(childWindowWidth / cellSize);
            columnCount = std::max(columnCount, 1);

            ImGui::Columns(columnCount, nullptr, false);

            for (const auto& item : node->children | std::views::keys)
            {
                std::string filenameStr = item.generic_string();
                ImGui::PushID(filenameStr.c_str());

                std::filesystem::path path = m_CurrentDirectory / item;
                bool isDirectory = std::filesystem::is_directory(path);

                // float thumbnailHeight = m_ThumbnailSize * (thumbnail->GetHeight() / thumbnail->GetWidth());
                const float thumbnailHeight = static_cast<float>(m_ThumbnailSize) * (320.0f / 540.0f);
                const float diff = static_cast<float>(m_ThumbnailSize) - thumbnailHeight;
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + diff);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                Ref<Texture> icon = isDirectory ? m_Icons["folder"] : m_Icons["unknown"];
                ImTextureID iconId = reinterpret_cast<ImTextureID>( icon->GetHandle().Get());
                ImGui::ImageButton(item.string().c_str(), iconId, { static_cast<float>(m_ThumbnailSize), static_cast<float>(m_ThumbnailSize) });


                if (ImGui::IsItemHovered())
                {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        if (isDirectory)
                        {
                            m_BackwardPathStack.push(m_CurrentDirectory);
                            m_CurrentDirectory = path;
                        }
                    }
                }

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Open"))
                    {
                        if (isDirectory)
                        {
                            m_BackwardPathStack.push(m_CurrentDirectory);
                            m_CurrentDirectory = path;
                        }
                        else
                        {
                            // Windows
                            std::string command = std::format("\"{}\"", path.generic_string());
                            std::system(command.c_str());
                        }
                    }

                    ImGui::Text("%s", filenameStr.c_str());

                    ImGui::EndPopup();
                }

                if (ImGui::BeginDragDropSource())
                {
                    if (!std::filesystem::is_directory(item))
                    {
                        const std::filesystem::path filepath = m_ActiveProject->GetAssetRelativeFilepath(m_BaseDirectory / node->path / item);
                        AssetHandle handle = m_ActiveProject->GetAssetManager().GetAssetHandle(filepath);
                        if (handle != AssetHandle(0))
                        {
                            ImGui::SetDragDropPayload("content_browser_item", &handle, sizeof(AssetHandle));
                        }
                    }

                    ImGui::EndDragDropSource();
                }
                
                ImGui::PopStyleColor();
                ImGui::TextWrapped("%s", filenameStr.c_str());

                ImGui::NextColumn();
                ImGui::PopID();
            }

            ImGui::Columns(1);

            if (ImGui::BeginPopupContextWindow("##content_browser_context_menu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoReopen | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::BeginMenu("Create"))
                {
                    if (ImGui::MenuItem("New Folder"))
                    {
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Thumbnail Size"))
                {
                    if (ImGui::MenuItem("Small")) m_ThumbnailSize = 38;
                    if (ImGui::MenuItem("Medium")) m_ThumbnailSize = 64;
                    if (ImGui::MenuItem("Large")) m_ThumbnailSize = 96;
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Open Folder in File Explorer"))
                {
                    std::string command = std::format("explorer {}", m_CurrentDirectory.string());
                    std::system(command.c_str());
                }

                ImGui::EndPopup();
            }

            ImGui::EndChild();
        }

        ImGui::End();
    }

    void ContentBrowserPanel::RefreshEntryPathList()
    {
        m_PathEntryList.erase(m_PathEntryList.begin() + 1, m_PathEntryList.end());

        const auto &relativePath = std::filesystem::relative(m_CurrentDirectory, Project::GetActiveAssetDirectory());
        auto currentDir = Project::GetActiveAssetDirectory();

        for (const auto &p : relativePath)
        {
            const std::string &pString = p.string();
            if (pString != ".")
            {
                currentDir /= p;
                m_PathEntryList.push_back(currentDir);
            }
        }
    }

    void ContentBrowserPanel::RefreshAssetTree()
    {
        const std::filesystem::path &assetPath = Project::GetActiveAssetDirectory();
        LoadAssetTree(assetPath);
    }

    void ContentBrowserPanel::LoadAssetTree(const std::filesystem::path &directory)
    {
        const std::filesystem::path assetPath = Project::GetActiveAssetDirectory();

        for (const auto &entry : std::filesystem::directory_iterator(directory))
        {
            const std::filesystem::path &relativePath = std::filesystem::relative(entry.path(), assetPath);
            uint32_t currentNodeIndex = 0;
            
            for (const std::filesystem::path &path : relativePath)
            {
                const auto it = m_TreeNodes[currentNodeIndex].children.find(path.generic_string());
                if (it != m_TreeNodes[currentNodeIndex].children.end())
                {
                    currentNodeIndex = it->second;
                }
                else
                {
                    AssetHandle assetHandle = AssetHandle(0);
                    if (!std::filesystem::is_directory(path) && path.has_extension())
                    {
                        std::string relPath = relativePath.generic_string();
                        assetHandle = m_ActiveProject->GetAssetManager().GetAssetHandle(relPath);
                        
                        // not registered yet
                        // (insert the metadata and generate the asset handle)
                        if (assetHandle == AssetHandle(0))
                        {
                            assetHandle = AssetHandle();
                            AssetMetaData metadata;
                            metadata.type = GetAssetTypeFromExtension(relativePath.extension().generic_string());
                            metadata.filepath = relPath;
                            m_ActiveProject->GetAssetManager().InsertMetaData(assetHandle, metadata);
                        }
                    }

                    FileTreeNode newNode(path, assetHandle);
                    newNode.parent = currentNodeIndex;

                    m_TreeNodes.push_back(newNode);
                    m_TreeNodes[currentNodeIndex].children[path] = static_cast<int>(m_TreeNodes.size()) - 1;
                    currentNodeIndex = static_cast<int>(m_TreeNodes.size()) - 1;
                }
            }
            
            if (entry.is_directory())
            {
                LoadAssetTree(entry.path());
            }
        }
    }

    void ContentBrowserPanel::PruneMissingNodes(uint32_t nodeIndex, const std::filesystem::path& basePath)
    {
        if (nodeIndex >= m_TreeNodes.size() || m_TreeNodes[nodeIndex].isDeleted)
            return;

        FileTreeNode &node = m_TreeNodes[nodeIndex];
        std::vector<std::string> toRemove;

        for (auto it = node.children.begin(); it != node.children.end(); )
        {
            auto &[childName, childIndex] = *it;

            if (childIndex >= m_TreeNodes.size() || m_TreeNodes[childIndex].isDeleted)
            {
                it = node.children.erase(it);
                continue;
            }

            std::filesystem::path fullPath = basePath / GetFullPath(childIndex);

            if (!std::filesystem::exists(fullPath))
            {
                toRemove.push_back((childName.string()));
            }
            else if (std::filesystem::is_directory(fullPath))
            {
                PruneMissingNodes(childIndex, fullPath);
            }
            ++it;
        }

        for (const auto& name : toRemove)
        {
            if (auto it = node.children.find(name); it != node.children.end())
            {
                const uint32_t childIndex = it->second;
                MarkNodeDeletedRecursive(childIndex);
                node.children.erase(it);
            }
        }
    }

    void ContentBrowserPanel::PruneMissingNodesAlt(uint32_t nodeIndex, const std::filesystem::path &basePath)
    {
        std::vector<uint32_t> nodesToDelete;
        CollectNodesToDelete(nodeIndex, basePath, nodesToDelete);

        // sort in descending order to delete from highest index first
        std::ranges::sort(nodesToDelete.rbegin(), nodesToDelete.rend());
        for (uint32_t nodeToDelete : nodesToDelete)
        {
            DeleteSingleNode(nodeToDelete);
        }
    }

    void ContentBrowserPanel::CollectNodesToDelete(uint32_t nodeIndex, const std::filesystem::path &basePath, std::vector<uint32_t> &nodesToDelete)
    {
        FileTreeNode &node = m_TreeNodes[nodeIndex];

        for (auto& childIndex : node.children | std::views::values)
        {
            std::filesystem::path fullPath = basePath / GetFullPath(childIndex);
            if (!std::filesystem::exists(fullPath))
            {
                CollectNodeAndDescendants(childIndex, nodesToDelete);
            }
            else if (std::filesystem::is_directory(fullPath))
            {
                CollectNodesToDelete(childIndex, fullPath, nodesToDelete);
            }
        }
    }

    void ContentBrowserPanel::CollectNodeAndDescendants(uint32_t nodeIndex, std::vector<uint32_t> &nodesToDelete)
    {
        FileTreeNode &node = m_TreeNodes[nodeIndex];

        for (auto& childIndex : node.children | std::views::values)
        {
            CollectNodeAndDescendants(childIndex, nodesToDelete);
        }

        nodesToDelete.push_back(nodeIndex);
    }

    void ContentBrowserPanel::MarkNodeDeletedRecursive(uint32_t nodeIndex)
    {
        if (nodeIndex >= m_TreeNodes.size() || m_TreeNodes[nodeIndex].isDeleted)
            return;

        FileTreeNode &node = m_TreeNodes[nodeIndex];
        node.isDeleted = true;

        for (auto& childIndex : node.children | std::views::values)
        {
            MarkNodeDeletedRecursive(childIndex);
        }

        node.children.clear();
    }

    void ContentBrowserPanel::DeleteSingleNode(uint32_t nodeIndex)
    {
        // Remove this node from its parent's children map
        if (nodeIndex < m_TreeNodes.size())
        {
            FileTreeNode &nodeToDelete = m_TreeNodes[nodeIndex];
            if (nodeToDelete.parent != static_cast<uint32_t>(-1) && nodeToDelete.parent < m_TreeNodes.size())
            {
                FileTreeNode &parent = m_TreeNodes[nodeToDelete.parent];
                for (auto it = parent.children.begin(); it != parent.children.end(); ++it)
                {
                    if (it->second == nodeIndex)
                    {
                        parent.children.erase(it);
                        break;
                    }
                }
            }
        }

        // Erase the node
        m_TreeNodes.erase(m_TreeNodes.begin() + nodeIndex);

        // Update all indices greater than nodeIndex
        UpdateIndicesAfterDeletion(nodeIndex);
    }

    void ContentBrowserPanel::UpdateIndicesAfterDeletion(uint32_t deletedIndex)
    {
        for (auto &node : m_TreeNodes)
        {
            // Update parent index
            if (node.parent > deletedIndex)
            {
                node.parent--;
            }

            // Update children indices
            for (auto& childIndex : node.children | std::views::values)
            {
                if (childIndex > deletedIndex)
                {
                    childIndex--;
                }
            }
        }
    }

    void ContentBrowserPanel::CompactTree()
    {
        std::vector<FileTreeNode> newNodes;
        std::unordered_map<uint32_t, uint32_t> indexMapping;

        // First pass: copy non-deleted nodes and create index mapping
        for (uint32_t i = 0; i < m_TreeNodes.size(); ++i)
        {
            if (!m_TreeNodes[i].isDeleted)
            {
                indexMapping[i] = static_cast<uint32_t>(newNodes.size());
                newNodes.push_back(m_TreeNodes[i]);
            }
        }

        // Second pass: update all indices
        for (auto &node : newNodes)
        {
            // Update parent index
            if (node.parent != 0)
            {
                auto it = indexMapping.find(node.parent);
                node.parent = (it != indexMapping.end()) ? it->second : 0;
            }

            // Update children indices
            for (auto& childIndex : node.children | std::views::values)
            {
                auto it = indexMapping.find(childIndex);
                if (it != indexMapping.end())
                {
                    childIndex = it->second;
                }
            }

            // Remove children that were deleted
            for (auto it = node.children.begin(); it != node.children.end();)
            {
                if (!indexMapping.contains(it->second))
                {
                    //it = node.children.erase(it);
                    ++it;
                    
                }
                else
                {
                    ++it;
                }
            }
        }

        m_TreeNodes = std::move(newNodes);
    }

    std::filesystem::path ContentBrowserPanel::GetFullPath(uint32_t nodeIndex) const
    {
        std::filesystem::path result;
        while (nodeIndex != 0)
        {
            const FileTreeNode &node = m_TreeNodes[nodeIndex];

            if (node.path.has_extension())
                return node.path;

            result = node.path / result;
            nodeIndex = node.parent;
        }

        return result;
    }
}
