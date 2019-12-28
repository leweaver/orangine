#pragma once

#include <memory>
#include <unordered_set>

namespace oe {
class Entity;

class Entity_filter {
 public:
  struct Entity_filter_listener {
    std::function<void(Entity*)> onAdd = [](Entity*) {};
    std::function<void(Entity*)> onRemove = [](Entity*) {};
  };

  using Container = std::unordered_set<std::shared_ptr<Entity>>;
  using Iterator = Container::iterator;
  using Const_iterator = Container::const_iterator;

  Entity_filter() = default;
  virtual ~Entity_filter() = default;
  Entity_filter(const Entity_filter&) = default;
  Entity_filter& operator=(const Entity_filter&) = default;
  Entity_filter(Entity_filter&&) = default;
  Entity_filter& operator=(Entity_filter&&) = default;

  bool empty() const { return _entities.empty(); }

  Iterator begin() { return _entities.begin(); }
  Iterator end() { return _entities.end(); }

  Const_iterator begin() const { return _entities.begin(); }
  Const_iterator end() const { return _entities.end(); }

  virtual void add_listener(std::weak_ptr<Entity_filter_listener> callback) = 0;

 protected:
  Container _entities;
};
} // namespace oe
