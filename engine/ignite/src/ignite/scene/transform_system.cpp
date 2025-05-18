#include "transform_system.hpp"

#include "scene.hpp"
#include "component.hpp"
#include "ignite/math/math.hpp"
#include "scene_manager.hpp"

namespace ignite {

    void TransformSystem::UpdateTransforms(Scene *scene)
    {
        // Update all local matrices
        auto view = scene->registry->view<Transform>();
        for (auto entity : view)
        {
            Transform &transform = view.get<Transform>(entity);
            if (transform.dirty)
                transform.UpdateLocalMatrix();
        }

        // Then update world matrices in hierarchical manner
        // Process entities with no parent first
        auto idView = scene->registry->view<ID, Transform>();
        for (auto entity : idView)
        {
            auto [id, transform] = idView.get<ID, Transform>(entity);
            if (id.parent == UUID(0))
            {
                // Root entity, world transformation equals local transform
                transform.worldMatrix = transform.localMatrix;

                // Update world transform components
                Math::DecomposeTransform(transform.worldMatrix,
                    transform.translation,
                    transform.rotation,
                    transform.scale);

                // Recursively update all childdrenn
                UpdateChildTransform(scene, id.uuid);

                // Clear dirty flag
                transform.dirty = false;
            }
        }
    }

    void TransformSystem::UpdateChildTransform(Scene *scene, UUID parentId)
    {
        Entity parentEntity = SceneManager::GetEntity(scene, parentId);
        ID &parentIdComp = parentEntity.GetComponent<ID>();
        Transform &parentTransformComp = parentEntity.GetComponent<Transform>();

        // Process all children
        for (UUID childId : parentIdComp.children)
        {
            Entity childEntity = SceneManager::GetEntity(scene, childId);
            Transform &childTransformComp = childEntity.GetComponent<Transform>();
            ID &childIdComp = childEntity.GetComponent<ID>();

            if (childEntity.HasComponent<SkinnedMeshRenderer>())
            {
                childTransformComp.worldMatrix = parentTransformComp.worldMatrix;
            }
            else
            {
                // Calculate world matrix using parent's world matrix
                childTransformComp.worldMatrix = parentTransformComp.worldMatrix * childTransformComp.localMatrix;

                // Update world matrix using parent's world matrix
                Math::DecomposeTransform(childTransformComp.worldMatrix,
                    childTransformComp.translation,
                    childTransformComp.rotation,
                    childTransformComp.scale);

                // Mark as clean
                childTransformComp.dirty = false;

            }

            // Recursively update this child's children
            UpdateChildTransform(scene, childIdComp.uuid);
        }
    }
}