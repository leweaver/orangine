#pragma once

namespace oe {

// You shouldn't try and use this class from the interface directly.
// If you need it inside a manager, it should have been passed in as a concrete class to the
// constructor. It is stored on the scene only as an authority for reference counting
class IDevice_repository {

  virtual void createDeviceDependentResources() = 0;
  virtual void destroyDeviceDependentResources() = 0;
};
} // namespace oe