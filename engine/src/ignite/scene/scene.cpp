 #include "scene.hpp"

#include "camera.hpp"
#include "ignite/graphics/renderer_2d.hpp"

namespace ignite
{
    Scene::Scene(const std::string &_name)
        : name(_name)
    {
        registry = new entt::registry();
    }

    Scene::~Scene()
    {
        if (registry)
            delete registry;
    }

    void Scene::OnUpdate(f32 deltaTime)
    {
    }

    void Scene::OnRender(Camera *camera, nvrhi::IFramebuffer *framebuffer)
    {
        Renderer2D::Begin(camera, framebuffer);

        for (auto &e : entities)
        {
            Transform &tr = EntityGetComponent<Transform>(e.second);
            Sprite2D &sprite = EntityGetComponent<Sprite2D>(e.second);
            Renderer2D::DrawQuad(tr.translation, tr.scale, sprite.color);
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
        registry->destroy(entity);
        entities.erase(std::ranges::find_if(entities, [entity](const auto &pair)
        {
            return pair.second == entity;
        }));
    }
}

