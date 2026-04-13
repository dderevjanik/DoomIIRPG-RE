#pragma once
#include <typeindex>

// Base class for entity components (built-in and mod-defined).
// Derive from this to create custom components that can be attached to entities.
//
// Example:
//   struct HealthRegen : Component {
//       float rate = 1.0f;
//       int delay = 3;
//       std::type_index typeId() const override { return componentTypeId<HealthRegen>(); }
//   };
//
//   entity->addComponent<HealthRegen>(1.5f, 2);
//   auto* regen = entity->getComponent<HealthRegen>();
//
class Component {
public:
	virtual ~Component() = default;
	virtual std::type_index typeId() const = 0;
};

// Compile-time type ID for component type lookups
template<typename T>
std::type_index componentTypeId() {
	return std::type_index(typeid(T));
}
