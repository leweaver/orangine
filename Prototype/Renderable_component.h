#pragma once

#include "Component.h"
#include "Renderer_data.h"
#include "Material.h"

namespace oe 
{
	class Renderable_component : public Component
	{
		DECLARE_COMPONENT_TYPE;

	public:

		Renderable_component()
			: _visible(true)
			, _rendererData(nullptr)
			, _material(nullptr)
		{}
		~Renderable_component();
		
		bool visible() const { return _visible; }
		void setVisible(bool visible) { _visible = visible; }

		// Runtime, non-serializable
		const std::unique_ptr<Material>& material() const { return _material; }
		void setMaterial(std::unique_ptr<Material>& material) { _material = std::move(material); }

		const std::unique_ptr<Renderer_data>& rendererData() const { return _rendererData; }
		void setRendererData(std::unique_ptr<Renderer_data> rendererData) { _rendererData = std::move(rendererData); }

	private:

		bool _visible;

		// Runtime, non-serializable
		std::unique_ptr<Renderer_data> _rendererData;
		std::unique_ptr<Material> _material;
	};

}
