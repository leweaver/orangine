#pragma once

namespace oe {
class Entity;
class Behavior_base {
 public:
  explicit Behavior_base(std::string&& name) : _name(std::move(name)) {}
  virtual ~Behavior_base() = default;
  Behavior_base(const Behavior_base&) = default;
  Behavior_base& operator=(const Behavior_base&) = default;
  Behavior_base(Behavior_base&&) = default;
  Behavior_base& operator=(Behavior_base&&) = default;

  const std::string& name() const { return _name; }
  virtual void handleEntity(Entity& entity) = 0;

 private:
  std::string _name;
};
} // namespace oe