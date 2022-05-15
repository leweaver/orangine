#pragma once

namespace oe {

// Provides an interface for an application that owns DeviceResources to be notified of the device
// being lost or created.
struct IDevice_notify {
  virtual ~IDevice_notify() = default;
  virtual void onDeviceLost() = 0;
  virtual void onDeviceRestored() = 0;
};

class IDevice_resources {
 public:
  virtual ~IDevice_resources() = default;

  // Public interface
  virtual bool checkSystemSupport(bool logFailures) = 0;
  virtual bool getWindowSize(int& width, int& height) = 0;

  // Events that are invoked from the windows event loop
  virtual void createDeviceDependentResources() = 0;
  virtual void recreateWindowSizeDependentResources() = 0;

  virtual void beginResourcesUploadStep() = 0;
  virtual void endResourcesUploadStep() = 0;

  virtual void destroyDeviceDependentResources() = 0;

  virtual bool setWindowSize(int width, int height) = 0;
};
} // namespace oe