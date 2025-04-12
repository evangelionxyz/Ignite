 #include "scene.hpp"

#include "camera.hpp"
#include "ignite/graphics/renderer_2d.hpp"

#include "ignite/physics/2d/physics_2d.hpp"

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
        physics2D->SimulationStart();
    }

    void Scene::OnStop()
    {
        physics2D->SimulationStop();
    }

    void Scene::OnUpdate(f32 deltaTime)
    {
        physics2D->Simulate(deltaTime);
    }

    void Scene::OnRender(Camera *camera, nvrhi::IFramebuffer *framebuffer)
    {
        Renderer2D::Begin(camera, framebuffer);

        for (auto &e : entities)
        {
            Transform &tr = GetComponent<Transform>(e.second);
            Sprite2D &sprite = GetComponent<Sprite2D>(e.second);
            Renderer2D::DrawQuad(tr.translation, tr.scale, sprite.color, sprite.texture);
        }

        Renderer2D::Flush();
        Renderer2D::End();
    }

    entt::entity Scene::CreateEntity(const std::string &name, UUID uuid)
    {
        entt::entity entity = registry->create();
        registry->emplace<ID>(entity, ID(name, uuid));
        registry->emplace<Transform>(entity, Transform({0.0f, 0.0f, 0.0f}));
        entities[uuid] = entity;
        return entity;
    }

    entt::entity Scene::GetEntity(UUID uuid)
    {
        if (entities.contains(uuid))
            return entities[uuid];

        return entt::null;
    }

    void Scene::DestroyEntity(UUID uuid)
    {
        entt::entity entity = GetEntity(uuid);
        registry->destroy(entity);
    }

    void Scene::DestroyEntity(entt::entity entity)
    {
        if (!registry->valid(entity))
            return;
            
        physics2D->DestroyBody(entity);
        registry->destroy(entity);
        auto it = std::find_if(entities.begin(), entities.end(),
        [entity](const auto& pair) {
            return pair.second == entity;
        });

        if (it != entities.end())
            entities.erase(it);
    }
}

