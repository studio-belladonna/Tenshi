#include "Tenshi/Entity/World.h"

#include <limits>

namespace Tenshi {

World::World() = default;
World::~World() = default;

World::World(World&& other) noexcept {
	swap(other);
}

World& World::operator=(World&& other) noexcept {
	if (this != &other) {
		World moved(std::move(other));
		swap(moved);
	}
	return *this;
}

Entity World::create() {
	ensureStructuralChangeAllowed();

	Entity::Index index;
	if (!freeIndices.empty()) {
		index = freeIndices.back();
		freeIndices.pop_back();
	} else {
		if (generations.size() >= Entity::InvalidIndex)
			throw std::runtime_error("Entity limit reached");

		index = static_cast<Entity::Index>(generations.size());
		generations.push_back(0);
		try {
			aliveFlags.push_back(false);
		} catch (...) {
			generations.pop_back();
			throw;
		}
	}

	aliveFlags[index] = true;
	++aliveCount;

	const Entity entity{index, generations[index]};
	emit(LifecycleEvent::EntityCreated, entity);

	return entity;
}

bool World::alive(Entity entity) const noexcept {
	return entity.valid()
		&& entity.index < generations.size()
		&& aliveFlags[entity.index]
		&& generations[entity.index] == entity.generation;
}

void World::destroy(Entity entity) {
	requireAlive(entity);
	ensureStructuralChangeAllowed();

	std::vector<std::pair<std::type_index, IComponentPool*>> affectedPools;
	affectedPools.reserve(pools.size());
	for (auto& [type, storage] : pools)
		affectedPools.emplace_back(type, storage.get());

	const bool canReuse = generations[entity.index] != (std::numeric_limits<std::uint32_t>::max)();
	if (canReuse)
		freeIndices.reserve(freeIndices.size() + 1);

	aliveFlags[entity.index] = false;
	--aliveCount;

	std::vector<std::type_index> removedComponents;
	removedComponents.reserve(affectedPools.size());
	for (const auto& [type, storage] : affectedPools) {
		if (storage->remove(entity))
			removedComponents.push_back(type);
	}

	if (canReuse) {
		++generations[entity.index];
		freeIndices.push_back(entity.index);
	}

	for (const auto type : removedComponents) {
		emit(LifecycleEvent::ComponentRemoved, entity, type);
	}

	emit(LifecycleEvent::EntityDestroyed, entity);
}

std::vector<Entity> World::entities() const {
	std::vector<Entity> result;
	result.reserve(aliveCount);
	for (std::size_t index = 0; index < generations.size(); ++index) {
		if (aliveFlags[index])
			result.push_back(Entity{static_cast<Entity::Index>(index), generations[index]});
	}

	return result;
}

std::size_t World::entityCount() const noexcept {
	return aliveCount;
}

void World::clear() {
	ensureStructuralChangeAllowed();
	const auto currentEntities = entities();
	for (Entity entity : currentEntities) {
		if (alive(entity))
			destroy(entity);
	}

	flushCommands();
}

World::ListenerId World::onEntityCreated(std::function<void(World&, Entity)> callback) {
	return subscribe(LifecycleEvent::EntityCreated, typeid(void), std::move(callback));
}

World::ListenerId World::onEntityDestroyed(std::function<void(World&, Entity)> callback) {
	return subscribe(LifecycleEvent::EntityDestroyed, typeid(void), std::move(callback));
}

bool World::unsubscribe(ListenerId listener) {
	const auto it = std::find_if(listeners.begin(), listeners.end(),
		[listener](const Listener& item) { return item.id == listener; });
	if (it == listeners.end())
		return false;
	listeners.erase(it);
	return true;
}

bool World::removeSystem(SystemId id) {
	const auto it = std::find_if(systems.begin(), systems.end(),
		[id](const System& system) { return system.id == id; });
	if (it == systems.end())
		return false;
	systems.erase(it);
	return true;
}

bool World::setSystemEnabled(SystemId id, bool enabled) {
	const auto it = std::find_if(systems.begin(), systems.end(),
		[id](const System& system) { return system.id == id; });
	if (it == systems.end())
		return false;
	it->enabled = enabled;
	return true;
}

bool World::systemEnabled(SystemId id) const {
	const auto it = std::find_if(systems.begin(), systems.end(),
		[id](const System& system) { return system.id == id; });
	return it != systems.end() && it->enabled;
}

std::size_t World::systemCount() const noexcept {
	return systems.size();
}

void World::update(float deltaTime) {
	if (updating)
		throw std::logic_error("World::update cannot be called recursively");

	updating = true;
	try {
		std::vector<SystemId> order;
		order.reserve(systems.size());
		for (const System& system : systems)
			order.push_back(system.id);

		for (SystemId id : order) {
			const auto it = std::find_if(systems.begin(), systems.end(),
				[id](const System& system) { return system.id == id; });
			if (it == systems.end() || !it->enabled)
				continue;

			auto callback = it->callback;
			callback(*this, deltaTime);
		}
		updating = false;
		flushCommands();
	} catch (...) {
		updating = false;
		flushCommands();
		throw;
	}
}

void World::requireAlive(Entity entity) const {
	if (!alive(entity))
		throw std::invalid_argument("Invalid entity");
}

void World::ensureStructuralChangeAllowed() const {
	if (iterationDepth != 0)
		throw std::logic_error("Structural changes are not allowed during World::each; queue a command instead");
}

[[noreturn]] void World::throwComponentNotFound() {
	throw std::logic_error("Component not found");
}

World::ListenerId World::subscribe(
	LifecycleEvent event,
	std::type_index componentType,
	std::function<void(World&, Entity)> callback
) {
	if (!callback)
		throw std::invalid_argument("Lifecycle callback cannot be empty");

	const ListenerId id = nextListenerId++;
	listeners.push_back(Listener{id, event, componentType, std::move(callback)});
	return id;
}

void World::emit(LifecycleEvent event, Entity entity, std::type_index componentType) {
	std::vector<ListenerId> matching;
	for (const Listener& listener : listeners) {
		if (listener.event == event && listener.componentType == componentType)
			matching.push_back(listener.id);
	}

	for (ListenerId id : matching) {
		const auto it = std::find_if(listeners.begin(), listeners.end(),
			[id](const Listener& listener) { return listener.id == id; });
		if (it != listeners.end()) {
			auto callback = it->callback;
			callback(*this, entity);
		}
	}
}

World::SystemId World::addSystemImpl(
	std::string name,
	std::function<void(World&, float)> callback,
	int priority
) {
	if (name.empty())
		throw std::invalid_argument("System name cannot be empty");
	if (std::any_of(systems.begin(), systems.end(),
		[&name](const System& system) { return system.name == name; }))
		throw std::logic_error("System name already exists");
	if (!callback)
		throw std::invalid_argument("System callback cannot be empty");

	const SystemId id = nextSystemId++;
	systems.push_back(System{id, std::move(name), priority, true, std::move(callback)});
	std::stable_sort(systems.begin(), systems.end(),
		[](const System& left, const System& right) {
			return left.priority < right.priority;
		});
	return id;
}

void World::queueCommand(std::function<void(World&)> command) {
	if (!command)
		throw std::invalid_argument("Command cannot be empty");
	commandQueue.push_back(std::move(command));
}

void World::Commands::destroy(Entity entity) {
	world.queueCommand([entity](World& owner) {
		if (owner.alive(entity))
			owner.destroy(entity);
	});
}

void World::Commands::flush() {
	world.flushCommands();
}

void World::flushCommands() {
	if (iterationDepth != 0 || updating)
		return;

	std::size_t processed = 0;
	while (!commandQueue.empty()) {
		if (++processed > 100000)
			throw std::runtime_error("Command queue limit exceeded; possible command loop");

		auto commands = std::move(commandQueue);
		commandQueue.clear();
		for (auto& command : commands)
			command(*this);
	}
}

void World::swap(World& other) noexcept {
	using std::swap;
	swap(generations, other.generations);
	swap(aliveFlags, other.aliveFlags);
	swap(freeIndices, other.freeIndices);
	swap(aliveCount, other.aliveCount);
	swap(pools, other.pools);
	swap(listeners, other.listeners);
	swap(nextListenerId, other.nextListenerId);
	swap(systems, other.systems);
	swap(nextSystemId, other.nextSystemId);
	swap(commandQueue, other.commandQueue);
	swap(iterationDepth, other.iterationDepth);
	swap(updating, other.updating);
}

} // namespace Tenshi