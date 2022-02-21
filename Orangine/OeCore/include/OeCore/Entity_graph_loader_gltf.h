#pragma once

#include "OeCore/Entity_graph_loader.h"

#include <wrl/client.h>

struct IWICImagingFactory;

namespace oe {

class Entity_graph_loader_gltf : public Entity_graph_loader {
 public:
  Entity_graph_loader_gltf(IMaterial_manager& materialManager, ITexture_manager& textureManager);

  void getSupportedFileExtensions(std::vector<std::string>& extensions) const override;
  std::vector<std::shared_ptr<Entity>> loadFile(
          std::string_view filename, IScene_graph_manager& sceneGraphManager, IEntity_repository& entityRepository,
          IComponent_factory& componentFactory, bool calculateBounds) const override;

 private:
  Microsoft::WRL::ComPtr<IWICImagingFactory> _imagingFactory = nullptr;
  IMaterial_manager& _materialManager;
  ITexture_manager& _textureManager;
};

}// namespace oe
