#pragma once
#include "Component.h"

namespace DX
{
	class DeviceResources;
}
namespace OE {

	class RenderableComponent : public Component
	{
		DECLARE_COMPONENT_TYPE;

		bool m_visible;
		
		// TODO: Some kind of asset ID?
		std::string m_meshName;
		std::string m_materialName;

	public:

		explicit RenderableComponent()
			: m_visible(true)
		{}
		
		bool GetVisible() const { return m_visible; }
		void SetVisible(bool visible) { m_visible = visible; }

		const std::string &GetMeshName() const { return m_meshName; }
		void SetMeshName(const std::string &meshName) { m_meshName = meshName; }

		const std::string &GetMaterialName() const { return m_materialName; }
		void SetMaterialName(const std::string &materialName) { m_materialName = materialName; }
		
	};

}