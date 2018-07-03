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
			, _wireframe(false)
			, _rendererData(nullptr)
			, _material(nullptr)
		{}
		~Renderable_component();
		
		bool visible() const { return _visible; }
		void setVisible(bool visible) { _visible = visible; }
		
		bool wireframe() const { return _wireframe; }
		void setWireframe(bool wireframe) { _wireframe = wireframe; }

		// Runtime, non-serializable
		const std::unique_ptr<Material>	& material() const { return _material; }
		void setMaterial(std::unique_ptr<Material>&& material) { _material = std::move(material); }

		const std::unique_ptr<Renderer_data>& rendererData() const { return _rendererData; }
		void setRendererData(std::unique_ptr<Renderer_data>&& rendererData) { _rendererData = std::move(rendererData); }

	private:

		bool _visible;
		bool _wireframe;

		// Runtime, non-serializable
		std::unique_ptr<Renderer_data> _rendererData;
		std::unique_ptr<Material> _material;
	};

}
