 #include "scene.hpp"

#include <ranges>

#include <entt/entt.hpp>
#include "camera.hpp"
#include "ignite/graphics/renderer_2d.hpp"
#include "ignite/physics/2d/physics_2d.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "entity.hpp"

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

        for (entt::entity e: entities | std::views::values)
        {
            const ID &id = registry->get<ID>(e);
            Transform &tr = registry->get<Transform>(e);

            // calculate transform from entity's parent
            if (id.parent != 0)
            {
                entt::entity parent = entities[id.parent];
                Transform &parentTr = registry->get<Transform>(parent);

                const glm::mat4 &transformedMatrix = parentTr.WorldTransform() * tr.LocalTransform();

                static glm::vec3 skew(1.0f);
                static glm::vec4 perspective(1.0f);

                glm::decompose(transformedMatrix, tr.scale, tr.rotation, tr.translation,
                    skew, perspective); // unused
            }
            else
            {
                tr.translation = tr.local_translation;
                tr.rotation = tr.local_rotation;
                tr.scale = tr.local_scale;
            }
        }
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

        for (entt::entity e: entities | std::views::values)
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
}

