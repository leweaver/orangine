﻿#pragma once

#include "OeCore/Entity_graph_loader.h"

#include <wrl/client.h>

struct IWICImagingFactory;

namespace oe {
class ITexture_manager;
class IMaterial_manager;
class Entity;

class Entity_graph_loader_gltf : public Entity_graph_loader {
 public:
  Entity_graph_loader_gltf();

  void getSupportedFileExtensions(std::vector<std::string>& extensions) const override;
  std::vector<std::shared_ptr<Entity>> loadFile(
      std::wstring_view filename,
      IEntity_repository& entityRepository,
      IMaterial_manager& materialRepository,
      ITexture_manager& textureManager,
      bool calculateBounds) const override;

 private:
  Microsoft::WRL::ComPtr<IWICImagingFactory> _imagingFactory = nullptr;
};

} // namespace oe
