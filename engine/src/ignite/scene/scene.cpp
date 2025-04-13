 #include "scene.hpp"

#include <ranges>

#include <entt/entt.hpp>
#include "camera.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/physics/2d/physics_2d.hpp"

#include "entity.hpp"

#include "entity_manager.hpp"

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
        // NOTE: currently we are not updating transformation with edit mode
        // TODO: calulcate parent & child transformation
    }

    void Scene::OnUpdateRuntimeSimulate(f32 deltaTime)
    {
        physics2D->Simulate(deltaTime);
    }

    void Scene::OnRenderRuntime(nvrhi::IFramebuffer *framebuffer)
    {
        // TODO: render with camera component
    }

    void Scene::OnRenderRuntimeSimulate(Camera *camera, nvrhi::IFramebuffer *framebuffer)
    {
        Renderer2D::Begin(camera, framebuffer);

        for (auto e: entities | std::views::values)
        {
            Entity entity = { e, this };
            auto &tr = entity.GetComponent<Transform>();
            
            if (!tr.visible)
                continue;

            if (entity.HasComponent<Sprite2D>())
            {
                auto &sprite = entity.GetComponent<Sprite2D>();
                Renderer2D::DrawQuad(tr.WorldTransform(), sprite.color, sprite.texture);
            }
        }

        Renderer2D::Flush();
        Renderer2D::End();
    }

    Ref<Scene> Scene::Copy(const Ref<Scene> &other)
    {
        // create new scene with other's name
        Ref<Scene> newScene = CreateRef<Scene>(other->name);

        // create source and destination registry
        auto srcRegistry = other->registry;
        auto destRegistry = newScene->registry;

        EntityMap entityMap;
        Entity newEntity = Entity{ };

        // create entities for new new scene
        auto view = srcRegistry->view<ID>();
        for (auto e : view)
        {
            // get src entity component
            Entity srcEntity = { e, other.get() };
            const ID &srcIdComp = srcEntity.GetComponent<ID>();

            // store src entity component to new entity (destination entity)
            newEntity = EntityManager::CreateEntity(newScene.get(), srcIdComp.name, srcIdComp.type, srcIdComp.uuid);

            // TODO: store parent and child
            entityMap[srcIdComp.uuid] = newEntity;
        }

        EntityManager::CopyComponent(AllComponents{}, destRegistry, srcRegistry, entityMap);

        return newScene;
    }
}

