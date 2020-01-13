#pragma once

namespace oe {
class Entity;
class Entity_behavior {
 public:
  explicit Entity_behavior(std::string&& name) : _name(std::move(name)) {}
  virtual ~Entity_behavior() = default;
  Entity_behavior(const Entity_behavior&) = default;
  Entity_behavior& operator=(const Entity_behavior&) = default;
  Entity_behavior(Entity_behavior&&) = default;
  Entity_behavior& operator=(Entity_behavior&&) = default;

  const std::string& name() const { return _name; }
  virtual void handleEntity(Entity& entity) = 0;

 private:
  std::string _name;
};
} // namespace oe