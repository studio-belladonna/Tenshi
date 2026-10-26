#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <bitset>
#include <vector>

#include "Tenshi/Entity/World.h"
#include "Tenshi/Components/Components.h"

namespace {
struct Position {
    int x;
};

struct Velocity {
    int x;
};

//struct Stuff {
//    int x;
//    uint64_t y;
//    std::vector<std::vector<uint64_t>> lotsofzs;
//    std::vector<Velocity> velocities;
//};

// simple checker
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<typename Exception, typename Function>
void checkThrows(Function&& function, const char* message) {
    try {
        function();
    } catch (const Exception&) {
        return;
    }

    throw std::runtime_error(message);
}

void testEntitiesAndComponents() {
    Tenshi::World world;

    int created = 0;
    int destroyed = 0;
    int added = 0;
    int updated = 0;
    int removed = 0;

    bool destroyEventSawDeadEntity = false;

    world.onEntityCreated([&](Tenshi::World&, Tenshi::Entity) { ++created; });
    world.onEntityDestroyed([&](Tenshi::World& owner, Tenshi::Entity entity) {
        ++destroyed;
        destroyEventSawDeadEntity = !owner.alive(entity);
    });

    const auto addedListener = world.onComponentAdded<Position>(
        [&](Tenshi::World&, Tenshi::Entity) { 
            ++added; 
        }
    );

    world.onComponentUpdated<Position>(
        [&](Tenshi::World&, Tenshi::Entity) { 
            ++updated; 
        }
    );

    world.onComponentRemoved<Position>(
        [&](Tenshi::World&, Tenshi::Entity) { 
            ++removed; 
        }
    );

    const Tenshi::Entity first = world.create();
    const Tenshi::Entity second = world.create();

    check(created == 2 && world.entityCount() == 2, "entity creation tracking failed");
    check(world.alive(first), "new entity should be alive");
    check(std::unordered_set<Tenshi::Entity>{first, second}.size() == 2,
        "entity hashing should include identity"
    );

    world.emplace<Position>(first, 3);
    world.emplace<Position>(second, 7);
    world.emplace<Velocity>(first, 4);

    check(added == 2, "component-added notification failed");
    check(world.has<Position>(first), "component should exist");
    check(world.get<Position>(first).x == 3, "component get failed");
    check(world.tryGet<Velocity>(second) == nullptr, 
        "tryGet should return null for a missing component"
    );

    checkThrows<std::logic_error>(
        [&] { world.emplace<Position>(first, 9); },
        "duplicate component insertion should fail"
    );

    checkThrows<std::logic_error>(
        [&] { (void)world.get<Velocity>(second); },
        "missing component access should fail"
    );

    world.emplaceOrReplace<Position>(first, 11);
    check(world.get<Position>(first).x == 11 && updated == 1,
        "component replacement failed"
    );

    check(world.view<Position, Velocity>().size() == 1,
        "multi-component view size failed"
    );

    //check(world.pull<Position, Velocity>().size() == 1,
    //    "multi-component asynchronous pull size failed"
    //);

    std::vector<Tenshi::Entity> matched;
    world.view<Position, Velocity>().each(
        [&](Tenshi::Entity entity, Position& position, Velocity& velocity) {
            position.x += velocity.x;
            matched.push_back(entity);
        }
    );
    
    check(matched.size() == 1 && matched.front() == first,
        "multi-component view iteration failed"
    );

    check(world.get<Position>(first).x == 15, "view should provide mutable components");

    int visited = 0;
    world.each<Position>([&](Tenshi::Entity entity, Position&) {
        ++visited;
        world.remove<Position>(entity);
    });

    check(visited == 2 && world.view<Position>().empty(),
        "itaration should remain safe when removing iterated components"
    );

    check(removed == 2, "component-removed notifications failed");

    check(world.unsubscribe(addedListener), "listener unsubscribe failed");
    world.emplace<Position>(first, 18);
    check(added == 2, "unsubscribed lifecycle callback was invoked");
    const Tenshi::World& readOnlyWorld = world;

    int readOnlyTotal = 0;
    readOnlyWorld.view<Position>().each(
        [&readOnlyTotal](Tenshi::Entity, const Position& position) {
            readOnlyTotal += position.x;
        }
    );

    check(readOnlyTotal == 18, "const views should expose const components");

    world.destroy(first);
    check(!world.alive(first) && world.entityCount() == 1,
        "destroy should invalidate the entity"
    );

    check(destroyed == 1 && destroyEventSawDeadEntity,
        "entity-destroyed callback should observe an invalid entity"
    );

    check(!world.has<Position>(first) && !world.has<Velocity>(first),
        "destroy should remove all components"
    );

    const Tenshi::Entity reused = world.create();
    check(reused.index == first.index && reused.generation != first.generation,
        "reused entity index should have a new generation"
    );

    check(!world.alive(first), "stale entity handle became alive again");
    check(world.tryGet<Position>(first) == nullptr,
        "stale entity handle should not retrieve componnets"
    );

    world.clear();
    check(world.entityCount() == 0 
    &&  !world.alive(reused) 
    &&  !world.alive(second),
        "clear should destroy all active entities"
    );
}

void testSystems() {
    Tenshi::World world;
    std::vector<int> order;
    Tenshi::World::SystemId removedSystem = 0;
    bool registeredDuringUpdate = false;

    world.addSystem("late", [&](Tenshi::World&, float) { order.push_back(3); }, 10);
    removedSystem = world.addSystem(
        "removed", [&](Tenshi::World&, float) { order.push_back(99); }, 2
    );

    const auto earlySystem = world.addSystem("early",
        [&](Tenshi::World&, float deltaTime) {
            check(deltaTime == 0.25f, "system delta time was not forwarded");
            order.push_back(1);

            if (!registeredDuringUpdate) {
                world.removeSystem(removedSystem);
                world.addSystem("deferred", [&](Tenshi::World&) { order.push_back(4); }, 3);
                registeredDuringUpdate = true;
            }
        }, -1
    );

    const auto middleSystem = world.addSystem("middle",
        [&](Tenshi::World&) { order.push_back(2); }, 1
    );

    check(world.systemCount() == 4, "system registration failed");
    check(world.setSystemEnabled(middleSystem, false), "system disabling failed");
    check(!world.systemEnabled(middleSystem), "system enabled state was not updated");
    world.update(0.25f);

    check(order == std::vector<int>({1, 3}),
        "system order, disabling, or safe mutation during update failed"
    );

    order.clear();
    check(world.setSystemEnabled(middleSystem, true), "system enabling failed");
    check(world.removeSystem(earlySystem), "system removal failed");
    world.update(0.25f);

    check(order == std::vector<int>({2, 4, 3}),
        "system priority ordering or registration timing failed"
    );

    check(!world.removeSystem(earlySystem), "removed system should not be found");
    checkThrows<std::logic_error>(
        [&] { world.addSystem("late", [](Tenshi::World&) {}); },
        "duplicate system names should fail"
    );
}

void testWorldMove() {
    Tenshi::World source;
    const Tenshi::Entity entity = source.create();
    source.emplace<Position>(entity, 5);

    Tenshi::World destination(std::move(source));
    check(destination.alive(entity) && destination.get<Position>(entity).x == 5,
        "moving a world should preserve its entities and components"
    );

    check(source.entityCount() == 0 && source.entities().empty(),
        "moved-from world should remain valid and empty"
    );

    Tenshi::World assigned;
    assigned = std::move(destination);
    check(assigned.alive(entity) && assigned.entityCount() == 1,
        "move assignment should transfer world state"
    );

    check(destination.entityCount() == 0,
        "move-assigned source should remain valid and empty"
    );
}

void testGameplayComposition() {
    Tenshi::World world;
    const Tenshi::Entity player = world.create();

    world.emplace<Tenshi::Transform>(player, 10.0, 20.0);
    world.emplace<Tenshi::Velocity>(player);
    world.emplace<Tenshi::CharacterController>(player);
    world.emplace<Tenshi::CharacterCollider>(player);
    world.emplace<Tenshi::PlayerControlled>(player);

    world.addSystem("controller", [](Tenshi::World& owner, float) {
        owner.each<Tenshi::CharacterController, Tenshi::Velocity>(
            [](Tenshi::Entity, Tenshi::CharacterController& controller,
                Tenshi::Velocity& velocity) {
                velocity.x = controller.moveAxis * controller.walkSpeed;
            }
        );
    });

    world.get<Tenshi::CharacterController>(player).moveAxis = -1.0;
    world.update(1.0f / 60.0f);

    check(world.view<Tenshi::Transform, Tenshi::Velocity,
        Tenshi::PlayerControlled>().contains(player),
        "gameplay entity composition failed"
    );
    check(world.get<Tenshi::Velocity>(player).x == -120.0,
        "gameplay controller system failed"
    );
}

} // namespace

// TODO: check MEAs too (OP and creation en masse)

// TOOD: test for and fix edgecases with large component data,
// as some kind of memory lifetime issue happens with large amounts of data in an entity

int main() {
    testEntitiesAndComponents();
    testSystems();
    testWorldMove();
    testGameplayComposition();
    
    // testMEA(500000000);
}