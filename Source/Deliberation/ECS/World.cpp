#include <Deliberation/ECS/World.h>

#include <iostream>
#include <sstream>

#include <Deliberation/Core/Assert.h>
#include <Deliberation/Core/ScopeProfiler.h>

#define VERBOSE 0

namespace deliberation
{

World::World():
    m_entityIDCounter(1)
{

}

World::~World()
{
#if VERBOSE
    std::cout << "World::~World()" << std::endl;
#endif

    for (auto & entity : m_entities)
    {
        if (isValid(entity.id))
        {
            removeEntity(entity.id);
        }
    }
}

EventManager & World::eventManager()
{
    return m_eventManager;
}

const WorldProfiler & World::profiler() const
{
    return m_profiler;
}

EntityData & World::entityData(EntityId id)
{
    Assert(isValid(id), "");

    return m_entities[entityIndex(id)];
}

Entity World::entity(EntityId id)
{
    return Entity(*this, id);
}

Entity World::entityByIndex(size_t index)
{
    return Entity(*this, m_entities[index].id);
}

Entity World::entityById(EntityId id)
{
    return Entity(*this, id);
}

Entity World::createEntity(const std::string & name, EntityId parent)
{
    auto id = m_entityIDCounter++;
    auto index = m_entities.emplace(id, name, parent);
    m_entityIndexByID[id] = index;

#if VERBOSE
    std::cout << "World::createEntity(): Allocated index=" << index << " for Entity '" << name << "'; parent=" << (int)parent << std::endl;
#endif

    auto & entity = entityData(id);

    if (isValid(parent))
    {
        m_entities[entityIndex(parent)].children.push_back(id);
    }

    entity.componentSetup = componentSetup(entity.componentBits);

    return Entity(*this, id);
}

void World::frameBegin()
{
    /**
     * For all entity removals, schedule all the entities components for removal
     */
    for (const auto & entityId : m_entityRemovals)
    {
        auto entity = this->entityData(entityId);
        auto * componentSetup = entity.componentSetup;

        for (const auto & componentTypeId : componentSetup->componentTypeIds)
        {
            scheduleComponentRemoval(entityId, componentTypeId);
        }
    }

    /**
     * Process Component removals
     */
    for (const auto & componentRemoval : m_componentRemovals)
    {
        removeComponent(componentRemoval);
    }
    m_componentRemovals.clear();

    /**
     * Process Entity removals
     */
    for (const auto & entityId : m_entityRemovals)
    {
        removeEntity(entityId);
    }
    m_entityRemovals.clear();

    for (auto & pair : m_systems)
    {
        auto & system = *pair.second;

        ScopeProfiler profiler;
        system.frameBegin();
        const auto micros = profiler.stop();

        m_profiler.addScope({system, "FrameBegin", micros});
    }

}

void World::update(float seconds)
{

    for (auto & pair : m_systems)
    {
        auto & system = *pair.second;

        ScopeProfiler profiler;
        system.beforeUpdate();
        const auto micros = profiler.stop();

        m_profiler.addScope({system, "BeforeUpdate", micros});
    }

    for (auto & pair : m_systems)
    {
        auto & system = *pair.second;

        ScopeProfiler profiler;
        system.update(seconds);
        const auto micros = profiler.stop();

        m_profiler.addScope({system, "Update", micros});
    }
}

void World::prePhysicsUpdate(float seconds)
{
    for (auto & pair : m_systems)
    {
        auto & system = *pair.second;

        ScopeProfiler profiler;
        system.prePhysicsUpdate(seconds);
        const auto micros = profiler.stop();

        m_profiler.addScope({system, "PrePhysicsUpdate", micros});
    }
}

void World::frameComplete()
{
    m_profiler.frameComplete();
}

std::string World::toString() const
{
    std::stringstream stream;

    for (auto & entity : m_entities)
    {
        if (!isValid(entity.id))
        {
            continue;
        }

        stream << "Entity " << entity.id << "/" << entity.name;
        if (isValid(entity.parent))
        {
            stream << "; parent = " << entity.parent << "/" << m_entities[entity.parent].name;
        }
        stream << std::endl;

        for (auto & pair : m_components)
        {
            auto & components = pair.second;

            if (components.contains(entity.id))
            {
                stream << "  Component " << components.at(entity.id)->name() << std::endl;
            }
        }
    }

    return stream.str();
}

void World::emit(size_t entityIndex, TypeID::value_t eventType, const void * event)
{
    const auto * const entityComponentSetup = m_entities[entityIndex].componentSetup;

    const auto iter = entityComponentSetup->componentIndicesByEventType.find(eventType);
    if (iter == entityComponentSetup->componentIndicesByEventType.end()) return;

    for (const auto & index : iter->second)
    {
        m_components[index][entityIndex]->dispatchEvent(eventType, event);
    }
}

bool World::isValid(EntityId id) const
{
    return m_entityIndexByID.find(id) != m_entityIndexByID.end();
}

void World::scheduleEntityRemoval(EntityId id)
{
    Assert(isValid(id), "");

    auto & entity = entityData(id);
    Assert(entity.phase == EntityPhase::Active || entity.phase == EntityPhase::ScheduledForRemoval,
           "Can't schedule non-active entity for removal");

    if (entity.phase == EntityPhase::Active)
    {
        m_entityRemovals.emplace_back(id);
        entityData(id).phase = EntityPhase::ScheduledForRemoval;
    }
}

void World::removeEntity(EntityId entityId)
{
    Assert(isValid(entityId), "");

    auto i = entityIndex(entityId);
    auto & entity = m_entities[i];
    auto * componentSetup = entity.componentSetup;

#if VERBOSE
    std::cout << "World::remove(): id=" << id << "; name=" << entity.name << std::endl;
    std::cout << "  num children=" << entity.children.size() << std::endl;
#endif

    for (auto child : entity.children) removeEntity(child);

    for (auto componentIndex : componentSetup->componentTypeIds) removeComponent({entityId, componentIndex});

    m_entities.erase(i);
    m_entityIndexByID.erase(entityId);
}

std::shared_ptr<ComponentBase> World::component(EntityId id, TypeID::value_t index)
{
    Assert(isValid(id), "");

    auto i = entityIndex(id);

    if (!m_components.contains(index)) return nullptr;
    if (!m_components.at(index).contains(i)) return nullptr;

    return m_components.at(index).at(i);
}

std::shared_ptr<const ComponentBase> World::component(EntityId id, TypeID::value_t index) const
{
    Assert(isValid(id), "");

    auto i = entityIndex(id);

    if (!m_components.contains(index))
    {
        return nullptr;
    }

    if (!m_components.at(index).contains(i))
    {
        return nullptr;
    }

    return m_components.at(index).at(i);
}

void World::addComponent(EntityId id, TypeID::value_t index, std::shared_ptr<ComponentBase> component)
{
    Assert(isValid(id), "");

    auto i = entityIndex(id);
    auto & entity = m_entities[i];
    auto * prevComponentSetup = entity.componentSetup;

    component->m_world = this;
    component->m_entityIndex = i;
    component->m_entityId = id;

    entity.componentBits.set(index);
    entity.componentSetup = componentSetup(entity.componentBits);

    Assert(!m_components[index][i], "Entity already had this component");

    m_components[index][i] = component;

#if VERBOSE
    std::cout << "World::addComponent()" << std::endl;
#endif

    for (auto systemIndex : entity.componentSetup->systemIndices)
    {
#if VERBOSE
    std::cout << "  Checking system " << systemIndex << " (" << prevComponentSetup->systemBits.test(systemIndex) << ")" << std::endl;
#endif
        if (!prevComponentSetup->systemBits.test(systemIndex))
        {
#if VERBOSE
    std::cout << "  Adding entity to system" << std::endl;
#endif
            auto & system = *m_systems[systemIndex];
            Entity entity(*this, id);
            system.addEntity(entity);
        }
    }
}

void World::scheduleComponentRemoval(EntityId id, ComponentTypeId index)
{
    auto component = this->component(id, index);

    if (component->phase() == ComponentPhase::Active)
    {
        m_componentRemovals.emplace_back(id, index);
        component->setPhase(ComponentPhase::ScheduledForRemoval);
    }
}

void World::removeComponent(const ComponentRemoval & componentRemoval)
{
    auto i = entityIndex(componentRemoval.entityId);
    auto & entity = m_entities[i];
    auto * prevComponentSetup = entity.componentSetup;

    entity.componentBits.reset(componentRemoval.componentTypeId);
    entity.componentSetup = componentSetup(entity.componentBits);

    for (auto systemIndex : prevComponentSetup->systemIndices)
    {
        if (!entity.componentSetup->systemBits.test(systemIndex))
        {
            auto & system = *m_systems[systemIndex];
            Entity entity(*this, componentRemoval.entityId);
            system.removeEntity(entity);
        }
    }

    m_components[componentRemoval.componentTypeId].erase(i);
}

std::size_t World::entityIndex(EntityId id) const
{
    auto i = m_entityIndexByID.find(id);
    Assert(i != m_entityIndexByID.end(), "");

    return i->second;
}

EntityComponentSetup * World::componentSetup(const ComponentBitset & componentBits)
{
    auto i = m_entityComponentSetups.find(componentBits);
    if (i != m_entityComponentSetups.end())
    {
        return &i->second;
    }

#if VERBOSE
    std::cout << "World: New component setup for bits " << componentBits.to_string() << std::endl;
#endif

    auto & setup = m_entityComponentSetups[componentBits];



    for (auto b = 0; b < ECS_MAX_NUM_COMPONENTS; b++)
    {
        if (componentBits.test(b))
        {
#if VERBOSE
            std::cout << "  Contains component " << b << std::endl;
#endif
            setup.componentTypeIds.push_back(b);
        }
    }

    for (const auto & componentIndex : setup.componentTypeIds)
    {
        const auto & componentSubscriptions = ComponentSubscriptionsBase::subscriptionsByComponentType[componentIndex];
        for (const auto & componentSubscription : componentSubscriptions)
        {
            setup.componentIndicesByEventType[componentSubscription].emplace_back(componentIndex);
        }
    }

    for (std::size_t s = 0; s < m_systems.keyUpperBound(); s++)
    {
        if (!m_systems.contains(s))
        {
            continue;
        }

        auto & system = *m_systems[s];
        if (system.filter().accepts(componentBits))
        {
#if VERBOSE
            std::cout << "  In system " << s << std::endl;
#endif
            setup.systemIndices.push_back(s);
            setup.systemBits.set(s);
        }
    }

    return &setup;
}

}
