#pragma once

#include "ComponentPool.h"
#include "Entity.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace Tenshi {

class World {
public:
	using ListenerId = std::uint64_t;
	using SystemId = std::uint64_t;

	World();
	~World();
	World(const World&) = delete;
	World& operator=(const World&) = delete;
	World(World&& other) noexcept;
	World& operator=(World&& other) noexcept;

	Entity create();
	bool alive(Entity entity) const noexcept;
	void destroy(Entity entity);

	template<typename T, typename... Args>
	T& emplace(Entity entity, Args&&... args) {
		requireAlive(entity);
		ensureStructuralChangeAllowed();
		T& component = pool<T>().emplace(entity, std::forward<Args>(args)...);
		emit(LifecycleEvent::ComponentAdded, entity, typeid(T));
		return component;
	}

	template<typename T, typename... Args>
	T& emplaceOrReplace(Entity entity, Args&&... args) {
		requireAlive(entity);
		ensureStructuralChangeAllowed();

		auto* storage = findPool<T>();
		if (storage && storage->has(entity)) {
			T& component = storage->emplaceOrReplace(entity, std::forward<Args>(args)...);
			emit(LifecycleEvent::ComponentUpdated, entity, typeid(T));
			return component;
		}

		return emplace<T>(entity, std::forward<Args>(args)...);
	}

	template<typename T>
	bool has(Entity entity) const {
		if (!alive(entity))
			return false;
		const auto* storage = findPool<T>();
		return storage && storage->has(entity);
	}

	template<typename T>
	T& get(Entity entity) {
		requireAlive(entity);
		auto* storage = findPool<T>();
		auto* component = storage ? storage->get(entity) : nullptr;
		if (!component)
			throwComponentNotFound();
		return *component;
	}

	template<typename T>
	const T& get(Entity entity) const {
		requireAlive(entity);
		const auto* storage = findPool<T>();
		const auto* component = storage ? storage->get(entity) : nullptr;
		if (!component)
			throwComponentNotFound();
		return *component;
	}

	template<typename T>
	T* tryGet(Entity entity) {
		if (!alive(entity))
			return nullptr;
		auto* storage = findPool<T>();
		return storage ? storage->get(entity) : nullptr;
	}

	template<typename T>
	const T* tryGet(Entity entity) const {
		if (!alive(entity))
			return nullptr;
		const auto* storage = findPool<T>();
		return storage ? storage->get(entity) : nullptr;
	}

	template<typename T>
	bool remove(Entity entity) {
		requireAlive(entity);
		ensureStructuralChangeAllowed();
		auto* storage = findPool<T>();
		if (!storage || !storage->remove(entity))
			return false;
		emit(LifecycleEvent::ComponentRemoved, entity, typeid(T));
		return true;
	}

	template<typename... Components, typename Function>
	void each(Function&& function) {
		static_assert(sizeof...(Components) > 0, "A query must include at least one component");
		auto poolsForQuery = std::tuple{findPool<Components>()...};
		bool missingPool = false;
		std::apply([&](auto*... storage) {
			missingPool = ((storage == nullptr) || ...);
		}, poolsForQuery);
		if (missingPool)
			return;

		const IComponentPool* smallest = nullptr;
		std::apply([&](auto*... storage) {
			((smallest = (!smallest || storage->size() < smallest->size())
				? static_cast<const IComponentPool*>(storage) : smallest), ...);
		}, poolsForQuery);

		const std::vector<Entity> candidates = smallest->entityList();
		IterationGuard guard(*this);
		for (Entity entity : candidates) {
			if (!alive(entity))
				continue;
			if ((findPool<Components>()->has(entity) && ...))
				std::invoke(function, entity, *findPool<Components>()->get(entity)...);
		}
	}

	template<typename... Components, typename Function>
	void each(Function&& function) const {
		static_assert(sizeof...(Components) > 0, "A query must include at least one component");

		auto poolsForQuery = std::tuple{findPool<Components>()...};
		bool missingPool = false;
		std::apply([&](auto*... storage) {
			missingPool = ((storage == nullptr) || ...);
		}, poolsForQuery);
		if (missingPool)
			return;

		const IComponentPool* smallest = nullptr;
		std::apply([&](auto*... storage) {
			((smallest = (!smallest || storage->size() < smallest->size())
				? static_cast<const IComponentPool*>(storage) : smallest), ...);
		}, poolsForQuery);

		const std::vector<Entity> candidates = smallest->entityList();
		for (Entity entity : candidates) {
			if (!alive(entity))
				continue;
			if ((findPool<Components>()->has(entity) && ...))
				std::invoke(function, entity, *findPool<Components>()->get(entity)...);
		}
	}

	template<typename... Components>
	class View {
	public:
		explicit View(World& owner) : world(owner) {}

		template<typename Function>
		void each(Function&& function) const {
			world.template each<Components...>(std::forward<Function>(function));
		}

		bool contains(Entity entity) const {
			return world.alive(entity) && (world.template has<Components>(entity) && ...);
		}

		std::size_t size() const {
			std::size_t result = 0;
			each([&result](Entity, auto&...) { ++result; });
			return result;
		}

		bool empty() const {
			return size() == 0;
		}

		std::vector<Entity> entities() const {
			std::vector<Entity> result;
			each([&result](Entity entity, auto&...) { result.push_back(entity); });
			return result;
		}

	private:
		World& world;
	};

	template<typename... Components>
	class ConstView {
	public:
		explicit ConstView(const World& owner) : world(owner) {}

		template<typename Function>
		void each(Function&& function) const {
			world.template each<Components...>(std::forward<Function>(function));
		}

		bool contains(Entity entity) const {
			return world.alive(entity) && (world.template has<Components>(entity) && ...);
		}

		std::size_t size() const {
			std::size_t result = 0;
			each([&result](Entity, const auto&...) { ++result; });
			return result;
		}

		bool empty() const {
			return size() == 0;
		}

		std::vector<Entity> entities() const {
			std::vector<Entity> result;
			each([&result](Entity entity, const auto&...) {
				result.push_back(entity);
			});
			return result;
		}

	private:
		const World& world;
	};

	template<typename... Components>
	View<Components...> view() {
		static_assert(sizeof...(Components) > 0, "A view must include at least one component");
		return View<Components...>(*this);
	}

	template<typename... Components>
	ConstView<Components...> view() const {
		static_assert(sizeof...(Components) > 0, "A view must include at least one component");
		return ConstView<Components...>(*this);
	}

	std::vector<Entity> entities() const;
	std::size_t entityCount() const noexcept;
	void clear();

	ListenerId onEntityCreated(std::function<void(World&, Entity)> callback);
	ListenerId onEntityDestroyed(std::function<void(World&, Entity)> callback);

	template<typename T>
	ListenerId onComponentAdded(std::function<void(World&, Entity)> callback) {
		return subscribe(LifecycleEvent::ComponentAdded, typeid(T), std::move(callback));
	}

	template<typename T>
	ListenerId onComponentUpdated(std::function<void(World&, Entity)> callback) {
		return subscribe(LifecycleEvent::ComponentUpdated, typeid(T), std::move(callback));
	}

	template<typename T>
	ListenerId onComponentRemoved(std::function<void(World&, Entity)> callback) {
		return subscribe(LifecycleEvent::ComponentRemoved, typeid(T), std::move(callback));
	}

	bool unsubscribe(ListenerId listener);

	template<typename Function>
	SystemId addSystem(std::string name, Function&& function, int priority = 0) {
		using Callable = std::decay_t<Function>;
		static_assert(
			std::is_invocable_v<Callable&, World&, float> 
		||	std::is_invocable_v<Callable&, World&>,
			"A system must be callable with (World&, float) or (World&)"
		);

		std::function<void(World&, float)> callback =
			[callable = Callable(std::forward<Function>(function))]
			(World& owner, float deltaTime) mutable {
				if constexpr (std::is_invocable_v<Callable&, World&, float>)
					std::invoke(callable, owner, deltaTime);
				else
					std::invoke(callable, owner);
			};

		return addSystemImpl(std::move(name), std::move(callback), priority);
	}

	bool removeSystem(SystemId id);
	bool setSystemEnabled(SystemId id, bool enabled);
	bool systemEnabled(SystemId id) const;
	std::size_t systemCount() const noexcept;
	void update(float deltaTime);

	class Commands {
	public:
		explicit Commands(World& owner) : world(owner) {}

		void destroy(Entity entity);

		template<typename T, typename... Args>
		void emplace(Entity entity, Args&&... args) {
			world.queueCommand([entity, values = std::make_tuple(std::forward<Args>(args)...)]
				(World& owner) mutable {
					if (!owner.alive(entity) || owner.has<T>(entity))
						return;
					std::apply([&](auto&&... unpacked) {
						owner.emplace<T>(entity, std::forward<decltype(unpacked)>(unpacked)...);
					}, std::move(values));
				}
			);
		}

		template<typename T>
		void remove(Entity entity) {
			world.queueCommand([entity](World& owner) {
				if (owner.alive(entity))
					owner.remove<T>(entity);
			});
		}

		void flush();

	private:
		World& world;
	};

	Commands commands() { return Commands(*this); }
	void flushCommands();

private:
	enum class LifecycleEvent {
		EntityCreated,
		EntityDestroyed,
		ComponentAdded,
		ComponentUpdated,
		ComponentRemoved
	};

	struct Listener {
		ListenerId id;
		LifecycleEvent event;
		std::type_index componentType;
		std::function<void(World&, Entity)> callback;
	};

	struct System {
		SystemId id;
		std::string name;
		int priority;
		bool enabled;
		std::function<void(World&, float)> callback;
	};

	class IterationGuard {
	public:
		explicit IterationGuard(World& owner) : world(owner) { ++world.iterationDepth; }
		~IterationGuard() { --world.iterationDepth; }
		IterationGuard(const IterationGuard&) = delete;
		IterationGuard& operator=(const IterationGuard&) = delete;
	private:
		World& world;
	};

	template<typename T>
	ComponentPool<T>& pool() {
		static_assert(std::is_same_v<T, std::remove_cv_t<T>>, "Component types must be unqualified");
		const std::type_index key(typeid(T));
		auto it = pools.find(key);
		if (it == pools.end()) {
			auto storage = std::make_unique<ComponentPool<T>>();
			auto* result = storage.get();
			pools.emplace(key, std::move(storage));
			return *result;
		}

		return *static_cast<ComponentPool<T>*>(it->second.get());
	}

	template<typename T>
	ComponentPool<T>* findPool() {
		static_assert(std::is_same_v<T, std::remove_cv_t<T>>, "Component types must be unqualified");

		const auto it = pools.find(std::type_index(typeid(T)));
		return it == pools.end()
			?	nullptr
			:	static_cast<ComponentPool<T>*>(it->second.get());
	}

	template<typename T>
	const ComponentPool<T>* findPool() const {
		static_assert(std::is_same_v<T, std::remove_cv_t<T>>, "Component types must be unqualified");

		const auto it = pools.find(std::type_index(typeid(T)));
		return it == pools.end()
			?	nullptr
			:	static_cast<const ComponentPool<T>*>(it->second.get());
	}

	void requireAlive(Entity entity) const;
	void ensureStructuralChangeAllowed() const;
	[[noreturn]] static void throwComponentNotFound();

	ListenerId subscribe(LifecycleEvent event, std::type_index componentType, std::function<void(World&, Entity)> callback);
	void emit(LifecycleEvent event, Entity entity, std::type_index componentType = typeid(void));
	SystemId addSystemImpl(std::string name, std::function<void(World&, float)> callback, int priority);
	void queueCommand(std::function<void(World&)> command);
	void swap(World& other) noexcept;

	std::vector<std::uint32_t> generations;
	std::vector<bool> aliveFlags;
	std::vector<Entity::Index> freeIndices;
	std::size_t aliveCount = 0;

	std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools;
	std::vector<Listener> listeners;
	ListenerId nextListenerId = 1;
	std::vector<System> systems;
	SystemId nextSystemId = 1;
	std::vector<std::function<void(World&)>> commandQueue;
	std::size_t iterationDepth = 0;
	bool updating = false;
};

} // namespace Tenshi
