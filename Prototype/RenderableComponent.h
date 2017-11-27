#pragma once
#include "Component.h"
#include "RendererData.h"

namespace DX
{
	class DeviceResources;
}
namespace OE {
	class RenderableComponent : public Component
	{
		std::unique_ptr<RendererData> m_rendererData;

	public:

		explicit RenderableComponent(Entity& entity)
			: Component(entity)
			, m_rendererData(nullptr)
		{}

		void Initialize() override;
		void Update() override;


		RendererData* const& GetRendererData() const
		{
			return m_rendererData.get();
		}

		virtual void CreateRendererData(const DX::DeviceResources deviceResources);
	};

}