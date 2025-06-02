#include "content_browser_panel.hpp"
#include "ignite/project/project.hpp"
#include "editor_layer.hpp"

namespace ignite {

    ContentBrowserPanel::ContentBrowserPanel(const char *windowTitle)
        : IPanel(windowTitle)
    {
        m_TreeNodes.push_back(TreeNode(".", AssetHandle(0)));

        TextureCreateInfo createInfo;
        createInfo.format = nvrhi::Format::RGBA8_UNORM;
        m_Icon = Texture::Create("resources/ui/ic_folder.png", createInfo);

        nvrhi::IDevice *device = Application::GetRenderDevice();

        nvrhi::CommandListHandle commandList = device->createCommandList();

        commandList->open();
        m_Icon->Write(commandList);
        commandList->close();
        
        device->executeCommandList(commandList);
    }

    void ContentBrowserPanel::SetActiveProject(const Ref<Project> &project)
    {
        m_ActiveProject = project;

        if (m_ActiveProject)
        {
            auto &info = m_ActiveProject->GetInfo();
            m_BaseDirectory = m_ActiveProject->GetAssetDirectory();
            
            m_PathEntryList.push_back(m_BaseDirectory);
            m_CurrentDirectory = m_ActiveProject->GetAssetDirectory();
            RefreshAssetTree();
        }

    }

    void ContentBrowserPanel::RenderFileTree(const std::filesystem::path &directory)
    {
        for (const auto &entry : std::filesystem::directory_iterator(directory))
        {
            const std::filesystem::path &path = entry.path();
            std::string filename = path.filename().string();

            ImGuiTreeNodeFlags flags = (m_SelectedFileTree == entry.path() ? ImGuiTreeNodeFlags_Selected : 0)
                | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth;

            if (!entry.is_directory())
            {
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }

            bool opened = ImGui::TreeNodeEx(filename.c_str(), flags, filename.c_str());

            if (ImGui::IsItemHovered())
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (entry.is_directory())
                    {
                        m_BackwardPathStack.push(m_CurrentDirectory);
                        m_SelectedFileTree = entry.path();
                        m_CurrentDirectory = entry.path();
                    }
                }
            }

            if (opened && entry.is_directory())
            {
                RenderFileTree(path);
                ImGui::TreePop();
            }
        }
    }

    void ContentBrowserPanel::OnGuiRender()
    {
        ImGui::Begin("Content Browser");

        ImGuiWindow *window = ImGui::GetCurrentWindow();
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
            RefreshAssetTree();
        }

        ImGui::SameLine();
        ImGui::SliderInt("Thumbnail Size", &m_ThumbnailSize, 1, 100);

        ImGui::EndChild();

        if (m_ActiveProject)
        {
            // Left side directory tree
            ImGui::BeginChild("left_item_browser", { 300.0f, 0.0f }, ImGuiChildFlags_ResizeX);
            RenderFileTree(m_BaseDirectory);
            ImGui::EndChild();
            ImGui::SameLine();

            // Files
            ImGui::BeginChild("##FILE_LISTS", { 0.0f, 0.0f });

            // Insert path nodes
            TreeNode *node = m_TreeNodes.data();
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

            for (auto &[item, nodeIndex] : node->children)
            {
                std::string filenameStr = item.generic_string();
                ImGui::PushID(filenameStr.c_str());

                // float thumbnailHeight = m_ThumbnailSize * (thumbnail->GetHeight() / thumbnail->GetWidth());
                const float thumbnailHeight = static_cast<float>(m_ThumbnailSize) * (320.0f / 540.0f);
                const float diff = static_cast<float>(m_ThumbnailSize) - thumbnailHeight;
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + diff);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                ImTextureID iconId = reinterpret_cast<ImTextureID>(m_Icon->GetHandle().Get());
                ImGui::ImageButton(item.string().c_str(), iconId, { static_cast<float>(m_ThumbnailSize), static_cast<float>(m_ThumbnailSize) });

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    std::filesystem::path openPath = m_CurrentDirectory / item;
                    if (std::filesystem::is_directory(openPath))
                    {
                        m_BackwardPathStack.push(m_CurrentDirectory);
                        m_CurrentDirectory = openPath;
                    }
                }
                
                ImGui::PopStyleColor();
                ImGui::TextWrapped(filenameStr.c_str());

                ImGui::NextColumn();
                ImGui::PopID();
            }

            ImGui::Columns(1);

            ImGui::EndChild();
        }

        ImGui::End();
    }

    void ContentBrowserPanel::RefreshEntryPathList()
    {
        m_PathEntryList.erase(m_PathEntryList.begin() + 1, m_PathEntryList.end());

        const auto &relativePath = std::filesystem::relative(m_CurrentDirectory, Project::GetActiveAssetDirectory());
        auto currentDir = Project::GetActiveAssetDirectory();

        for (auto p : relativePath)
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
        const AssetRegistry &assetRegistry = Project::GetInstance()->GetAssetManager().GetAssetAssetRegistry();

        for (const auto &entry : std::filesystem::directory_iterator(directory))
        {
            const std::filesystem::path &currentRelativePath = std::filesystem::relative(entry.path(), assetPath);

            uint32_t currentNodeIndex = 0;
            for (const std::filesystem::path &path : currentRelativePath)
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
                        std::string relativeAssetPathStr = currentRelativePath.generic_string();
                        assetHandle = Project::GetInstance()->GetAssetManager().ImportAsset(relativeAssetPathStr);
                    }

                    TreeNode newNode(path, assetHandle);
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

}