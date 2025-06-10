#include "scene_manager.hpp"
#include "scene.hpp"
#include "entity.hpp"
#include "entity_command_manager.hpp"

#include "ignite/core/application.hpp"
#include "ignite/core/uuid.hpp"

#include "ignite/graphics/scene_renderer.hpp"
#include "ignite/graphics/environment.hpp"
#include "ignite/graphics/renderer.hpp"

#include "ignite/math/math.hpp"

#include <string>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace ignite
{    
    static std::string GenerateUniqueName(const std::string &name, const std::vector<std::string> &names, std::unordered_map<std::string, uint32_t> &strMap)
    {
        // iterate names for the first time
        // check if the name already exists
        bool found = false;
        for (auto &n : names)
        {
            found = n == name;
            if (found)
                break;
        }

        // if the name does not exists, return it as the unique key
        if (!found)
            return name;

        // generate a unique key if the name already exists
        i32 counter = 1;
        std::string uniqueKey;

        bool unique = false;
        while (!unique)
        {
            uniqueKey = fmt::format("{} ({})", name, counter);
            unique = true;

            // iterate once again
            // check if they are already unique
            for (auto &n : names)
            {
                if (n == uniqueKey) // not unique continue search
                {
                    unique = false;
                    break;
                }
            }
            counter++; // increment the counter if names are not unique
        }

        // update the counter to reflect the highest number used
        strMap[name] = counter - 1;

        // cleanup counters for names that no longer exist
        for (auto it = strMap.begin(); it != strMap.end();)
        {
            bool exists = false;
            for (auto &n : names)
            {
                exists = name.find(it->first) == 0;
                if (exists)
                    break;
            }

            if (!exists)
                it = strMap.erase(it);
            else
                ++it;
        }

        return uniqueKey;
    }

    Entity SceneManager::CreateEntity(Scene *scene, const std::string &name, EntityType type, UUID uuid)
    {
        scene->SetDirtyFlag(true);
        Entity entity = Entity { scene->registry->create(), scene };
        entity.AddComponent<ID>(name, type, uuid);
        entity.AddComponent<Transform>(Transform({0.0f, 0.0f, 0.0f}));
        scene->entities[uuid] = entity;
        return entity;
    }

    Entity SceneManager::CreateSprite(Scene *scene, const std::string &name, UUID uuid)
    {
        // create local storage for entity data
        Entity createdEntity;

        // prepare entity creation logic
        std::function<void()> createFunc = [=, &createdEntity]() mutable
        {
            createdEntity = CreateEntity(scene, name, EntityType_Node, uuid);
            createdEntity.AddComponent<Sprite2D>();
        };

        // immediately call createFunc to initialize createdEntity
        createFunc();

        // capture scene and entity by value to preserve the for undo
        Scene *capturedScene = scene;
        UUID capturedUUID = createdEntity.GetComponent<ID>().uuid;

        std::function<void()> destroyFunc = [capturedScene, capturedUUID]()
        {
            Entity entityToDestroy = SceneManager::GetEntity(capturedScene, capturedUUID);
            if (entityToDestroy)
            {
                DestroyEntity(capturedScene, entityToDestroy);
            }
        };

        CommandManager::AddCommand(
            CreateScope<EntityManagerCommand>(
                createFunc, 
                destroyFunc, 
                CommandState_Create
            )
        );
       
        return createdEntity;
    }

    Entity SceneManager::CreateMesh(Scene *scene, const std::string &name, UUID uuid)
    {
        scene->SetDirtyFlag(true);

        Entity entity = Entity { scene->registry->create(), scene };
        entity.AddComponent<ID>(name, EntityType_Node, uuid);
        entity.AddComponent<Transform>(Transform({ 0.0f, 0.0f, 0.0f }));

        scene->entities[uuid] = entity;

        return entity;
    }

    void SceneManager::WriteMeshBuffer(Scene *scene, MeshRenderer &meshRenderer, uint32_t entityID)
    {
        nvrhi::IDevice *device = Application::GetRenderDevice();
        nvrhi::CommandListHandle commandList = device->createCommandList();

        commandList->open();

        commandList->writeBuffer(meshRenderer.mesh->indexBuffer, meshRenderer.mesh->data.indices.data(), sizeof(uint32_t) * meshRenderer.mesh->data.indices.size());

        // default
        for (auto &vertex : meshRenderer.mesh->data.vertices)
        {
            vertex.entityID = entityID;
        }

        commandList->writeBuffer(meshRenderer.mesh->vertexBuffer, meshRenderer.mesh->data.vertices.data(), sizeof(VertexMesh) * meshRenderer.mesh->data.vertices.size());
        
        // outline
        commandList->writeBuffer(meshRenderer.mesh->outlineVertexBuffer, meshRenderer.mesh->outlineVertices.data(), sizeof(VertexMeshOutline) * meshRenderer.mesh->outlineVertices.size());

        // write textures
        if (meshRenderer.mesh->material.ShouldWriteTexture())
        {
            meshRenderer.mesh->material.WriteBuffer(commandList);
        }

        auto desc = nvrhi::BindingSetDesc();
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, Renderer::GetCameraBufferHandle()));
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, meshRenderer.mesh->objectBufferHandle));
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, scene->sceneRenderer->GetEnvironment()->GetDirLightBuffer()));
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(3, scene->sceneRenderer->GetEnvironment()->GetParamsBuffer()));
        desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(4, meshRenderer.mesh->materialBufferHandle));

        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(0, meshRenderer.mesh->material.textures[aiTextureType_DIFFUSE].handle));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(1, meshRenderer.mesh->material.textures[aiTextureType_SPECULAR].handle));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(2, meshRenderer.mesh->material.textures[aiTextureType_EMISSIVE].handle));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(3, meshRenderer.mesh->material.textures[aiTextureType_DIFFUSE_ROUGHNESS].handle));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(4, meshRenderer.mesh->material.textures[aiTextureType_NORMALS].handle));
        desc.addItem(nvrhi::BindingSetItem::Texture_SRV(5, scene->sceneRenderer->GetEnvironment()->GetHDRTexture()));
        desc.addItem(nvrhi::BindingSetItem::Sampler(0, meshRenderer.mesh->material.sampler));

        meshRenderer.mesh->bindingSets[GPipeline::MESH] = device->createBindingSet(desc, Renderer::GetBindingLayout(GPipeline::MESH));
        LOG_ASSERT(meshRenderer.mesh->bindingSets[GPipeline::MESH], "Failed to create binding set");

        {
            // Outline

            desc = nvrhi::BindingSetDesc();
            desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, Renderer::GetCameraBufferHandle()));
            desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(1, meshRenderer.mesh->objectBufferHandle));
            meshRenderer.mesh->bindingSets[GPipeline::MESH_OUTLINE] = device->createBindingSet(desc, Renderer::GetBindingLayout(GPipeline::MESH_OUTLINE));
            LOG_ASSERT(meshRenderer.mesh->bindingSets[GPipeline::MESH_OUTLINE], "Failed to create binding set");
        }

        commandList->close();
        device->executeCommandList(commandList);
    }

    void SceneManager::RenameEntity(Scene *scene, Entity entity, const std::string &newName)
    {
        scene->SetDirtyFlag(true);

        if (newName.empty())
            return;

        ID &idComp = entity.GetComponent<ID>();
        idComp.name = newName;
    }

    void SceneManager::DestroyEntity(Scene *scene, Entity entity)
    {
        scene->SetDirtyFlag(true);

        if (!scene || !scene->registry->valid(entity))
            return;

        ID idComp = entity.GetComponent<ID>();

        // recursively destroy children
        for (UUID childId : idComp.children)
        {
            entity.GetComponent<ID>().RemoveChild(childId);
            DestroyEntity(scene, GetEntity(scene, childId));
        }

        scene->registry->destroy(entity);
        scene->registeredComps.erase(entity);
        scene->physics2D->DestroyBody(entity);
        scene->entities.erase(idComp.uuid);
        
        // remove from parent
        if (idComp.parent != UUID(0))
        {
            Entity parent = SceneManager::GetEntity(scene, idComp.parent);
            parent.GetComponent<ID>().RemoveChild(idComp.uuid);
        }
    }

    void SceneManager::DestroyEntity(Scene *scene, UUID uuid)
    {
        DestroyEntity(scene, GetEntity(scene, uuid));
    }

    Entity SceneManager::DuplicateEntity(Scene *scene, Entity entity, bool addToParent)
    {
        scene->SetDirtyFlag(true);

        // first, get current entity's ID Component
        ID &idComp = entity.GetComponent<ID>();

        Entity newEntity = SceneManager::CreateEntity(scene, idComp.name, idComp.type);

        // copy current entity's components to new entity
        SceneManager::CopyComponentIfExists(AllComponents{}, newEntity, entity);

        if (newEntity.HasComponent<MeshRenderer>())
        {
            MeshRenderer &mr = newEntity.GetComponent<MeshRenderer>();
            SceneManager::WriteMeshBuffer(scene, mr, newEntity);
        }

        // get new entity's ID Component
        ID &newEntityIDComp = newEntity.GetComponent<ID>();

        // create its children
        for (UUID cid : idComp.children)
        {
            Entity newChildEntity = DuplicateEntity(scene, GetEntity(scene, cid), false); // add to parent false

            ID &childId = newChildEntity.GetComponent<ID>();
            
            // add this child to new entity
            newEntityIDComp.AddChild(childId.uuid);

            // set child parent to new entity
            childId.parent = newEntityIDComp.uuid;
        }

        // check if current entity has a parent
        if (idComp.parent != 0 && addToParent)
        {
            // get the current entity's parent
            Entity parent = GetEntity(scene, idComp.parent);
            ID &parentIDComp = parent.GetComponent<ID>();

            // set new entity parent to this parent
            newEntityIDComp.parent = parentIDComp.uuid;

            // add child to parent
            parentIDComp.AddChild(newEntityIDComp.uuid);
        }

        return newEntity;
    }

    Entity SceneManager::GetEntity(Scene *scene, UUID uuid)
    {
        if (scene->entities.contains(uuid))
            return Entity { scene->entities[uuid], scene };

        return Entity{};
    }

    void SceneManager::AddChild(Scene *scene, Entity destination, Entity source)
    {
        scene->SetDirtyFlag(true);

        ID &destIDComp = destination.GetComponent<ID>();
        ID &sourceIDComp = source.GetComponent<ID>();

        if (!IsParent(scene, destIDComp.uuid, sourceIDComp.uuid))
        {
            // remove from current parent
            if (sourceIDComp.parent != 0)
            {
                Entity currentParent = GetEntity(scene, sourceIDComp.parent);
                currentParent.GetComponent<ID>().RemoveChild(sourceIDComp.uuid);
            }

            // add to target parent
            destIDComp.AddChild(sourceIDComp.uuid);
            sourceIDComp.parent = destIDComp.uuid;
        }
    }

    bool SceneManager::ChildExists(Scene *scene, Entity destination, Entity source)
    {
        ID &destIDComp = destination.GetComponent<ID>();
        ID &sourceIDComp = source.GetComponent<ID>();

        // source parent is the destination
        if (sourceIDComp.parent == destIDComp.uuid)
        {
            return true;
        }

        // recursively search
        Entity nextParent = GetEntity(scene, sourceIDComp.parent);
        if (ChildExists(scene, nextParent, source)) // find until it is not source's parent
            return true;

        return false;
    }

    bool SceneManager::IsParent(Scene *scene, UUID target, UUID source)
    {
        Entity destParent = GetEntity(scene, target);
        if (!destParent.IsValid())
            return false;

        const ID &destIDComp = destParent.GetComponent<ID>();

        if (target == source)
            return true;

        if (destIDComp.parent && IsParent(scene, destIDComp.parent, source))
            return true;

        return false;
    }

    Entity SceneManager::FindChild(Scene *scene, Entity parent, UUID uuid)
    {
        UUID parentUUID = parent.GetComponent<ID>().uuid;
        if (IsParent(scene, parentUUID, uuid))
            return GetEntity(scene, uuid);

        return {entt::null, nullptr};
    }

    Ref<Scene> SceneManager::Copy(Ref<Scene> &other)
    {
        // create new scene with other's name
        Ref<Scene> newScene = CreateRef<Scene>(other->name);

        // create source and destination registry
        auto srcRegistry = other->registry;
        auto destRegistry = newScene->registry;

        EntityMap entityMap;

        // create entities for new new scene
        auto view = srcRegistry->view<ID>();
        for (auto e : view)
        {
            // get src entity component
            Entity srcEntity = { e, other.get() };
            ID &srcIdComp = srcEntity.GetComponent<ID>();

            // store src entity component to new entity (destination entity)
            Entity newEntity = SceneManager::CreateEntity(newScene.get(), srcIdComp.name, srcIdComp.type, srcIdComp.uuid);
            ID &newEntityIdComp = newEntity.GetComponent<ID>();
            newEntityIdComp.parent = srcIdComp.parent;
            newEntityIdComp.children = srcIdComp.children;

            entityMap[srcIdComp.uuid] = newEntity;
        }

        SceneManager::CopyComponent(AllComponents{}, destRegistry, srcRegistry, entityMap, newScene->registeredComps);

        // copy scene extra data
        newScene->handle = other->handle;
        newScene->registeredComps = other->registeredComps;
        newScene->sceneRenderer = other->sceneRenderer;

        // Do not copy entities (it is created when creating entity)
        // newScene->entities = other->entities; 

        auto mrView = destRegistry->view<MeshRenderer>();
        for (entt::entity e : mrView)
        {
            MeshRenderer &meshRenderer = mrView.get<MeshRenderer>(e);
            WriteMeshBuffer(newScene.get(), meshRenderer, static_cast<uint32_t>(e));
        }

        return newScene;
    }


} // namespace ignite
