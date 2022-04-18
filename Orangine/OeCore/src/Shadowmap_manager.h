#pragma once

#include "OeCore/IShadowmap_manager.h"
#include "OeCore/ITexture_manager.h"
#include "OeCore/Shadow_map_texture_pool.h"

#include <memory>


namespace oe {
class Shadowmap_manager : public IShadowmap_manager
    , public Manager_base
    , public Manager_deviceDependent {
 public:
  explicit Shadowmap_manager(ITexture_manager& textureManager)
      : IShadowmap_manager()
      , Manager_base()
      , Manager_deviceDependent()
      , _texturePool(nullptr)
      , _textureManager(textureManager)
  {}
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

  bool verifyTexturePool() const;
  std::unique_ptr<Shadow_map_texture_pool> _texturePool;
  ITexture_manager& _textureManager;
};
} // namespace oe::internal