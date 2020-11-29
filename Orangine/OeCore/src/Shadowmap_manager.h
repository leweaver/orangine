#pragma once

#include "OeCore/IShadowmap_manager.h"
#include "OeCore/Shadow_map_texture_pool.h"

#include "D3D11/D3D_device_repository.h"

#include <memory>

namespace oe::internal {
class D3D_device_repository;
class Shadowmap_manager : public IShadowmap_manager {
 public:
  Shadowmap_manager(Scene& scene, std::shared_ptr<D3D_device_repository> deviceRepository)
      : IShadowmap_manager(scene), _texturePool(nullptr), _deviceRepository(deviceRepository) {}
  virtual ~Shadowmap_manager() = default;

  // Manager_base implementation
  void initialize() override {}
  void shutdown() override {}
  const std::string& name() const override;

  // Manager_deviceDependent implementation
  void createDeviceDependentResources() override;
  void destroyDeviceDependentResources() override;

  // IShadowmap_manager implementation
  std::shared_ptr<Texture> borrowTexture() override;
  void returnTexture(std::shared_ptr<Texture> shadowMap) override;
  std::shared_ptr<Texture> shadowMapDepthTextureArray() override;
  std::shared_ptr<Texture> shadowMapStencilTextureArray() override;

 private:
  static std::string _name;

  void verifyTexturePool() const;
  std::unique_ptr<Shadow_map_texture_pool> _texturePool;

  std::shared_ptr<D3D_device_repository> _deviceRepository;
};
} // namespace oe::internal