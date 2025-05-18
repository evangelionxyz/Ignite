 #include "scene.hpp"

#include <ranges>

#include <entt/entt.hpp>
#include "camera.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/physics/2d/physics_2d.hpp"

#include "entity.hpp"

#include "scene_manager.hpp"

#include "transform_system.hpp"

namespace ignite
{
    Scene::Scene(const std::string &_name)
        : name(_name)
    {
        registry = new entt::registry();
        physics2D = CreateScope<Physics2D>(this);
    }

    Scene::~Scene()
    {
        if (registry)
            delete registry;
    }

    void Scene::OnStart()
    {
        m_Playing = true;

        physics2D->SimulationStart();
    }

    void Scene::OnStop()
    {
        m_Playing = false;
        
        physics2D->SimulationStop();
    }

    void Scene::OnUpdateEdit(f32 deltaTime)
    {
        TransformSystem::UpdateTransforms(this);
    }

    void Scene::OnUpdateRuntimeSimulate(f32 deltaTime)
    {
        physics2D->Simulate(deltaTime);
    }

    void Scene::OnRenderRuntime(nvrhi::IFramebuffer *framebuffer)
    {
        // TODO: render with camera component
    }

    Ref<Scene> Scene::Create(const std::string &name)
    {
        return CreateRef<Scene>(name);
    }

    void Scene::OnRenderRuntimeSimulate(Camera *camera, nvrhi::IFramebuffer *framebuffer)
    {
        Renderer2D::Begin(camera, framebuffer);

        for (entt::entity e: entities | std::views::values)
        {
            Entity entity = { e, this };
            auto &tr = entity.GetComponent<Transform>();
            
            if (!tr.visible)
                continue;

            if (entity.HasComponent<Sprite2D>())
            {
                auto &sprite = entity.GetComponent<Sprite2D>();
                Renderer2D::DrawQuad(tr.worldMatrix, sprite.color, sprite.texture);
            }
        }

        Renderer2D::Flush();
        Renderer2D::End();
    }
}

