#include "editor_layer.hpp"
#include "panels/scene_panel.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/graphics/mesh_factory.hpp"

#include <glm/glm.hpp>
#include <nvrhi/utils.h>

#include "ignite/core/command.hpp"
#include "ignite/scene/camera.hpp"
#include "ignite/graphics/texture.hpp"

namespace ignite
{
    struct MeshRenderData
    {
        nvrhi::ShaderHandle vertexShader;
        nvrhi::ShaderHandle pixelShader;
        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;
        nvrhi::BufferHandle constantBuffer;
        nvrhi::InputLayoutHandle inputLayout;
        nvrhi::BindingLayoutHandle bindingLayout;
        nvrhi::BindingSetHandle bindingSet;
        nvrhi::GraphicsPipelineHandle pipeline;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        struct MeshPushConstant
        {
            glm::mat4 mvp;
            glm::mat4 modelMatrix;
            glm::mat4 normalMatrix;
            glm::vec4 cameraPosition;
        } pushConstant;
    };

    MeshRenderData *meshData = nullptr;
    

    EditorLayer::EditorLayer(const std::string &name)
        : Layer(name), m_DeviceManager(nullptr)
    {
    }

    void EditorLayer::OnAttach()
    {
        Layer::OnAttach();

        m_DeviceManager = Application::GetDeviceManager();
        nvrhi::IDevice *device = m_DeviceManager->GetDevice();

        meshData = new MeshRenderData();

        // create push constant
        const auto constantBufferDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(meshData->pushConstant))
            .setIsConstantBuffer(true)
            .setIsVolatile(true)
            .setInitialState(nvrhi::ResourceStates::ConstantBuffer)
            .setMaxVersions(16);

        meshData->constantBuffer = device->createBuffer(constantBufferDesc);
        LOG_ASSERT(meshData->constantBuffer, "Failed to create constant buffer");

        // create shaders
        VPShader *shaders = Renderer::GetDefaultShader("default_mesh");
        meshData->vertexShader = shaders->vertex;// Shader::Create(device, "resources/shaders/default.vertex.hlsl", ShaderMake::ShaderType::Vertex);
        meshData->pixelShader = shaders->pixel; // Shader::Create(device, "resources/shaders/default.pixel.hlsl", ShaderMake::ShaderType::Pixel);
        LOG_ASSERT(meshData->vertexShader && meshData->pixelShader, "Failed to create shaders");

        const auto attributes = VertexMesh::GetAttributes();
        meshData->inputLayout = device->createInputLayout(attributes.data(), attributes.size(), meshData->vertexShader);
        LOG_ASSERT(meshData->inputLayout, "Failed to create input layout");

        const auto layoutDesc = VertexMesh::GetBindingLayoutDesc();
        meshData->bindingLayout = device->createBindingLayout(layoutDesc);
        LOG_ASSERT(meshData->bindingLayout, "Failed to create binding layout");

        // create buffers
        const auto vbDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(MeshFactory::CubeVertices))
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Cube vertex buffer");

        meshData->vertexBuffer = device->createBuffer(vbDesc);
        LOG_ASSERT(meshData->vertexBuffer, "Failed to create vertex buffer");
        
        const auto ibDesc = nvrhi::BufferDesc()
            .setByteSize(sizeof(MeshFactory::CubeIndices))
            .setIsIndexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::IndexBuffer)
            .setKeepInitialState(true)
            .setDebugName("Cube index buffer");
        
        meshData->indexBuffer = device->createBuffer(ibDesc);
        LOG_ASSERT(meshData->indexBuffer, "Failed to create index buffer");

        // create binding set
        const auto bindingSetDesc = nvrhi::BindingSetDesc()
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, meshData->constantBuffer));

        meshData->bindingSet = device->createBindingSet(bindingSetDesc, meshData->bindingLayout);
        LOG_ASSERT(meshData->bindingSet, "Failed to create binding set");

        // write buffer with command list
        m_CommandLists[0] = device->createCommandList();
        m_CommandLists[1] = device->createCommandList();
        Renderer2D::InitQuadData(m_DeviceManager->GetDevice(), m_CommandLists[0]);

        m_Texture = Texture::Create(device, "Resources/textures/test.png");
        m_CommandLists[0]->open();
        m_Texture->Write(m_CommandLists[0]);

        m_CommandLists[0]->writeBuffer(meshData->vertexBuffer, MeshFactory::CubeVertices.data(), sizeof(MeshFactory::CubeVertices));
        m_CommandLists[0]->writeBuffer(meshData->indexBuffer, MeshFactory::CubeIndices.data(), sizeof(MeshFactory::CubeIndices));

        m_CommandLists[0]->close();
        device->executeCommandList(m_CommandLists[0]);

        m_ScenePanel = CreateRef<ScenePanel>("Scene Panel", this);
        m_ScenePanel->CreateRenderTarget(device);

        NewScene();
    }

    void EditorLayer::OnDetach()
    {
        Layer::OnDetach();
        m_CommandLists[0] = nullptr;
        m_CommandLists[1] = nullptr;

        if (meshData)
        {
            delete meshData;
        }
    }

    void EditorLayer::OnUpdate(const f32 deltaTime)
    {
        Layer::OnUpdate(deltaTime);

        // multi select entity
        m_Data.multiSelect = Input::IsKeyPressed(Key::LeftShift);

        switch (m_Data.sceneState)
        {
        case State_ScenePlay:
        {
            m_ActiveScene->OnUpdateRuntimeSimulate(deltaTime);
            break;
        }
        case State_SceneEdit:
        {
            m_ActiveScene->OnUpdateEdit(deltaTime);
            break;
        }
        }

        // update panels
        m_ScenePanel->OnUpdate(deltaTime);
    }

    void EditorLayer::OnEvent(Event &e)
    {
        Layer::OnEvent(e);
        m_ScenePanel->OnEvent(e);

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(BIND_CLASS_EVENT_FN(EditorLayer::OnKeyPressedEvent));
    }

    bool EditorLayer::OnKeyPressedEvent(KeyPressedEvent &event)
    {
        bool control = Input::IsKeyPressed(KEY_LEFT_CONTROL);
        bool shift = Input::IsKeyPressed(KEY_LEFT_SHIFT);

        switch (event.GetKeyCode())
        {
        case Key::T:
        {
            if (!Input::IsMouseButtonPressed(Mouse::ButtonRight))
                m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
            break;
        }
        case Key::R:
        {
            if (!Input::IsMouseButtonPressed(Mouse::ButtonRight))
                m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::ROTATE);
            break;
        }
        case Key::S:
        {
            if (!Input::IsMouseButtonPressed(Mouse::ButtonRight))
                m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::SCALE);
            break;
        }
        case Key::F5:
        {
            (m_Data.sceneState == State_SceneEdit || m_Data.sceneState == State_SceneSimulate)
                ? OnScenePlay()
                : OnSceneStop();

            break;
        }
        case Key::F6:
        {
            (m_Data.sceneState == State_SceneEdit || m_Data.sceneState == State_ScenePlay)
                ? OnScenePlay()
                : OnSceneStop();
            break;
        }
        case Key::D:
        {
            if (control)
                m_ScenePanel->DuplicateSelectedEntity();
            break;
        }
        case Key::Z:
        {
            if (control)
            {
                if (shift)
                    Application::GetCommandManager()->Redo();
                else
                    Application::GetCommandManager()->Undo();
            }
            break;
        }
        }
        return false;
    }

    void EditorLayer::OnRender(nvrhi::IFramebuffer *mainFramebuffer)
    {
        Layer::OnRender(mainFramebuffer);

        // create render target framebuffer

        uint32_t backBufferIndex = m_DeviceManager->GetCurrentBackBufferIndex();
        static uint32_t backBufferCount = m_DeviceManager->GetBackBufferCount();
        uint32_t width = (uint32_t)mainFramebuffer->getFramebufferInfo().getViewport().width();
        uint32_t height = (uint32_t)mainFramebuffer->getFramebufferInfo().getViewport().height();

        m_ScenePanel->GetRT()->CreateFramebuffers(backBufferCount, backBufferIndex, { width, height });

        nvrhi::IFramebuffer *viewportFramebuffer = m_ScenePanel->GetRT()->GetCurrentFramebuffer();
        nvrhi::Viewport viewport = viewportFramebuffer->getFramebufferInfo().getViewport();

        // mesh pipeline
        if (meshData->pipeline == nullptr)
        {
            // create graphics pipeline
            nvrhi::BlendState blendState;
            blendState.targets[0].setBlendEnable(true);

            auto depthStencilState = nvrhi::DepthStencilState()
                .setDepthWriteEnable(true)
                .setDepthTestEnable(true)
                .setDepthFunc(nvrhi::ComparisonFunc::LessOrEqual); // use 1.0 for far depth

            auto rasterState = nvrhi::RasterState()
                .setCullFront()
                .setFrontCounterClockwise(false)
                .setMultisampleEnable(false);

            auto renderState = nvrhi::RenderState()
                .setRasterState(rasterState)
                .setDepthStencilState(depthStencilState)
                .setBlendState(blendState);

            auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
                .setVertexShader(meshData->vertexShader)
                .setPixelShader(meshData->pixelShader)
                .setInputLayout(meshData->inputLayout)
                .addBindingLayout(meshData->bindingLayout)
                .setRenderState(renderState)
                .setPrimType(nvrhi::PrimitiveType::TriangleList);

            // create with viewportFramebuffer (the same framebuffer to render)
            meshData->pipeline = m_DeviceManager->GetDevice()->createGraphicsPipeline(pipelineDesc, viewportFramebuffer);
            LOG_ASSERT(meshData->pipeline, "Failed to create graphics pipeline");
        }

        {
            // main scene rendering
            m_CommandLists[0]->open();

            // clear main framebuffer color attachment
            nvrhi::utils::ClearColorAttachment(m_CommandLists[0], mainFramebuffer, 0, nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f));

            m_ScenePanel->GetRT()->ClearColorAttachment(m_CommandLists[0]);

            float farDepth = 1.0f; // LessOrEqual
            m_CommandLists[0]->clearDepthStencilTexture(m_ScenePanel->GetRT()->GetDepthAttachment(), nvrhi::AllSubresources, true, farDepth, true, 0);

            m_ActiveScene->OnRenderRuntimeSimulate(m_ScenePanel->GetViewportCamera(), viewportFramebuffer);
            m_CommandLists[0]->close();
            m_DeviceManager->GetDevice()->executeCommandList(m_CommandLists[0]);
        }
        
        
        // mesh command list m_CommandLists index 1
        m_CommandLists[1]->open();
        meshData->pushConstant.mvp = m_ScenePanel->GetViewportCamera()->GetViewProjectionMatrix();
        meshData->pushConstant.modelMatrix = glm::translate(glm::mat4(1.0f), meshData->position) 
            * glm::toMat4(glm::quat(meshData->rotation)) * glm::scale(meshData->scale);
        glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(meshData->pushConstant.modelMatrix)));
        meshData->pushConstant.normalMatrix = glm::mat4(normalMat3);
        meshData->pushConstant.cameraPosition = glm::vec4(m_ScenePanel->GetViewportCamera()->position, 1.0f);
        m_CommandLists[1]->writeBuffer(meshData->constantBuffer, &meshData->pushConstant, sizeof(meshData->pushConstant));

        const auto meshGraphicsState = nvrhi::GraphicsState()
            .setPipeline(meshData->pipeline)
            .setFramebuffer(m_ScenePanel->GetRT()->GetCurrentFramebuffer())
            .addBindingSet(meshData->bindingSet)
            .setViewport(nvrhi::ViewportState().addViewportAndScissorRect(viewport))
            .addVertexBuffer(nvrhi::VertexBufferBinding{meshData->vertexBuffer, 0, 0})
            .setIndexBuffer({meshData->indexBuffer, nvrhi::Format::R32_UINT});

        m_CommandLists[1]->setGraphicsState(meshGraphicsState);
        nvrhi::DrawArguments args;
        args.setVertexCount(MeshFactory::CubeIndices.size());
        args.instanceCount = 1;
        m_CommandLists[1]->drawIndexed(args);
        m_CommandLists[1]->close();
        m_DeviceManager->GetDevice()->executeCommandList(m_CommandLists[1]);
    }

    void EditorLayer::OnGuiRender()
    {
        constexpr f32 TITLE_BAR_HEIGHT = 45.0f;

        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        f32 totalTitlebarHeight = viewport->Pos.y + TITLE_BAR_HEIGHT;

        ImGui::Begin("##main_dockspace", nullptr, windowFlags);
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        window->DC.LayoutType = ImGuiLayoutType_Horizontal;
        window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        const ImVec2 minPos = viewport->Pos;
        const ImVec2 maxPos = ImVec2(viewport->Pos.x + viewport->Size.x, totalTitlebarHeight);
        draw_list->AddRectFilled(minPos, maxPos, IM_COL32(30, 30, 30, 255));

        // play stop and simulate button
        ImVec2 buttonSize = {40.0f, 25.0f};
        ImVec2 buttonCursorPos = { viewport->Pos.x, totalTitlebarHeight / 2.0f - buttonSize.y / 2.0f};
        ImGui::SetCursorScreenPos(buttonCursorPos);

        bool sceneStatePlaying = m_Data.sceneState == State_ScenePlay;
        if (ImGui::Button(sceneStatePlaying ? "Stop" : "Play", buttonSize))
        {
            if (sceneStatePlaying)
                OnSceneStop();
            else 
                OnScenePlay();
        }

        // dockspace
        ImGui::SetCursorScreenPos({viewport->Pos.x, totalTitlebarHeight});
        ImGui::DockSpace(ImGui::GetID("main_dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        {
            // scene dockspace
            m_ScenePanel->OnGuiRender();

            ImGui::Begin("Cube");
            ImGui::PushID("CubeID");
            ImGui::DragFloat3("Position", &meshData->position.x, 0.025f);
            ImGui::DragFloat3("Rotation", &meshData->rotation.x, 0.025f);
            ImGui::DragFloat3("Scale", &meshData->scale.x, 0.025f);
            ImGui::PopID();
            ImGui::End();
        }

        ImGui::End(); // end dockspace
    }

    void EditorLayer::NewScene()
    {
        // create editor scene
        m_EditorScene = CreateRef<Scene>("test scene");

        // pass to active scene
        m_ActiveScene = m_EditorScene;
        m_ScenePanel->SetActiveScene(m_ActiveScene.get(), true); // reset selected entity
    }

    void EditorLayer::OnScenePlay()
    {
        m_ScenePanel->SetGizmoOperation(ImGuizmo::OPERATION::NONE);

        if (m_Data.sceneState != State_SceneEdit)
            OnSceneStop();

        m_Data.sceneState = State_ScenePlay;

        // copy initial components to new scene
        m_ActiveScene = SceneManager::Copy(m_EditorScene);
        m_ActiveScene->OnStart();

        m_ScenePanel->SetActiveScene(m_ActiveScene.get());
    }

    void EditorLayer::OnSceneStop()
    {
        m_Data.sceneState = State_SceneEdit;
        
        m_ActiveScene->OnStop();
        m_ActiveScene = m_EditorScene;

        m_ScenePanel->SetActiveScene(m_EditorScene.get());
    }

    void  EditorLayer::OnSceneSimulate()
    {
        if (m_Data.sceneState != State_SceneEdit)
            OnSceneStop();

        m_Data.sceneState = State_SceneSimulate;
    }
}
