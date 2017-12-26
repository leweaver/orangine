#pragma once

#include "Component.h"
#include "RendererData.h"
#include "Material.h"

namespace OE 
{
	class RenderableComponent : public Component
	{
		DECLARE_COMPONENT_TYPE;

		bool m_visible;

		// Runtime, non-serializable
		std::unique_ptr<RendererData> m_rendererData;
		std::unique_ptr<Material> m_material;

	public:

		RenderableComponent()
			: m_visible(true)
			, m_rendererData(nullptr)
			, m_material(nullptr)
		{}
		
		bool GetVisible() const { return m_visible; }
		void SetVisible(bool visible) { m_visible = visible; }

		// Runtime, non-serializable
		const std::unique_ptr<Material>& GetMaterial() const { return m_material; }
		void SetMaterial(std::unique_ptr<Material> &material) { m_material = std::move(material); }

		const std::unique_ptr<RendererData> &GetRendererData() const { return m_rendererData; }
		void SetRendererData(std::unique_ptr<RendererData> &rendererData) { m_rendererData = std::move(rendererData); }

	};

}
