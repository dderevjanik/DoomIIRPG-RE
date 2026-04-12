#ifndef __EVENTBUS_H__
#define __EVENTBUS_H__

#include <cstdint>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>

using EventHandle = uint32_t;

class EventBus {
public:
	// Subscribe to events of type T. Returns a handle for unsubscription.
	// Priority: lower values are called first (default 100).
	// Listeners at the same priority are called in registration order.
	template <typename T>
	EventHandle subscribe(std::function<void(const T&)> callback, int priority = 100) {
		EventHandle handle = nextHandle++;
		auto& subs = subscribers[std::type_index(typeid(T))];
		subs.push_back({handle, priority, [cb = std::move(callback)](const void* evt) {
			cb(*static_cast<const T*>(evt));
		}});
		std::stable_sort(subs.begin(), subs.end(),
			[](const Sub& a, const Sub& b) { return a.priority < b.priority; });
		return handle;
	}

	// Unsubscribe by handle. Returns true if found and removed.
	bool unsubscribe(EventHandle handle) {
		for (auto& [typeIdx, subs] : subscribers) {
			for (auto it = subs.begin(); it != subs.end(); ++it) {
				if (it->handle == handle) {
					subs.erase(it);
					return true;
				}
			}
		}
		return false;
	}

	// Synchronous emit: calls all subscribers for T immediately, in priority order.
	template <typename T>
	void emit(const T& event) {
		auto it = subscribers.find(std::type_index(typeid(T)));
		if (it == subscribers.end()) return;
		for (auto& sub : it->second) {
			sub.callback(&event);
		}
	}

	// Queue an event for deferred dispatch. Call flush() to dispatch all queued events.
	template <typename T>
	void queue(const T& event) {
		auto copied = std::make_shared<T>(event);
		pendingEvents.push_back({
			std::type_index(typeid(T)),
			std::static_pointer_cast<void>(copied)
		});
	}

	// Dispatch all queued events in FIFO order, then clear the queue.
	// Handlers that call queue() during flush are deferred to the next flush().
	void flush() {
		auto pending = std::move(pendingEvents);
		pendingEvents.clear();
		for (auto& entry : pending) {
			auto it = subscribers.find(entry.typeIdx);
			if (it == subscribers.end()) continue;
			for (auto& sub : it->second) {
				sub.callback(entry.data.get());
			}
		}
	}

	// Remove all subscribers and queued events.
	void clear() {
		subscribers.clear();
		pendingEvents.clear();
	}

private:
	struct Sub {
		EventHandle handle;
		int priority;
		std::function<void(const void*)> callback;
	};

	struct QueuedEvent {
		std::type_index typeIdx;
		std::shared_ptr<void> data;
	};

	std::unordered_map<std::type_index, std::vector<Sub>> subscribers;
	std::vector<QueuedEvent> pendingEvents;
	EventHandle nextHandle = 1;
};

#endif
