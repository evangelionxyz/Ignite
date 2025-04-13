#include "entity_manager.hpp"
#include "scene.hpp"
#include "entity.hpp"

#include <string>
#include <unordered_map>

namespace ignite
{    
    std::string GenerateUniqueName(const std::string &name, const std::vector<std::string> &names, StringCounterMap &strMap)
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

        // generate a unieuq key if the name already exists
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

        // update the counter to reflect the highes number used
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


    Entity EntityManager::CreateEntity(Scene *scene, const std::string &name, EntityType type, UUID uuid)
    {
        Entity entity = Entity { scene->registry->create(), scene };
        
        std::string uniqueName = GenerateUniqueName(name, scene->entityNames, scene->entityNamesMapCounter);
        
        entity.AddComponent<ID>(uniqueName, uuid);
        entity.AddComponent<Transform>(Transform({0.0f, 0.0f, 0.0f}));
        
        scene->entities[uuid] = entity;
        scene->entityNames.push_back(uniqueName);

        return entity;
    }

    Entity EntityManager::CreateSprite(Scene *scene, const std::string &name, UUID uuid)
    {
        Entity entity = CreateEntity(scene, name, EntityType_Common, uuid);
        entity.AddComponent<Sprite2D>();
        return entity;
    }

    void EntityManager::RenameEntity(Scene *scene, Entity entity, const std::string &newName)
    {
        if (newName.empty())
            return;

        // check registed names
        bool foundSameName = false;
        for (auto &n : scene->entityNames)
        {
            foundSameName = n == newName;
            if (foundSameName)
                break;
        }

        ID &idComp = entity.GetComponent<ID>();
        if (foundSameName)
            idComp.name = GenerateUniqueName(newName, scene->entityNames, scene->entityNamesMapCounter);
        else 
            idComp.name = newName;
    }

    void EntityManager::DestroyEntity(Scene *scene, Entity entity)
    {
        if (!scene->registry->valid(entity))
            return;

        ID idComp = entity.GetComponent<ID>();

        scene->registeredComps.erase(entity);
        scene->physics2D->DestroyBody(entity);
        scene->registry->destroy(entity);

        auto it = std::ranges::find_if(scene->entities, [entity](auto pair) { return pair.second == entity; });
        if (it != scene->entities.end())
        {
            scene->entities.erase(it);

            bool found = false;
            i32 count = 0;
            for (size_t i = 0; i < scene->entityNames.size(); ++i)
            {
                if (idComp.name == scene->entityNames[i])
                {
                    found = true;
                    count = i;
                    break;
                }
            }

            if (found)
            {
                scene->entityNames.erase(scene->entityNames.begin() + count);
            }
        }
    }

    void EntityManager::DestroyEntity(Scene *scene, UUID uuid)
    {
        Entity entity = GetEntity(scene, uuid);
        scene->registry->destroy(entity);
    }

    Entity EntityManager::GetEntity(Scene *scene, UUID uuid)
    {
        if (scene->entities.contains(uuid))
            return Entity { scene->entities[uuid], scene };

        return Entity{};
    }

} // namespace ignite
