#pragma once
#include "Component.h"
#include "RendererData.h"
#include "Material.h"

namespace DX
{
	class DeviceResources;
}
namespace OE {
	class RenderableComponent : public Component
	{
		bool m_visible;
		std::unique_ptr<RendererData> m_rendererData;
		std::shared_ptr<Material> m_material;

	public:

		explicit RenderableComponent(Entity &entity)
			: Component(entity)
			, m_visible(true)
			, m_rendererData(nullptr)
			, m_material(nullptr)
		{}

		void Initialize() override;
		void Update() override;

		bool GetVisible() const { return m_visible; }
		void SetVisible(bool visible) { m_visible = visible; }

		const RendererData *GetRendererData() const
		{
			return m_rendererData.get();
		}

		virtual RendererData *CreateRendererData(const DX::DeviceResources deviceResources);

		void SetMaterial(std::shared_ptr<Material> material) { m_material = material; }
		std::shared_ptr<Material> GetMaterial() const { return m_material; }
		
	};

}