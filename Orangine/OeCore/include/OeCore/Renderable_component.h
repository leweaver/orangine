#pragma once

#include "Component.h"
#include "Renderer_data.h"
#include "Material.h"
#include "Material_context.h"

namespace oe 
{
	class Renderable_component : public Component
	{
		DECLARE_COMPONENT_TYPE;

	public:

		Renderable_component(Entity& entity)
			: Component(entity)
			, _visible(true)
			, _wireframe(false)
			, _castShadow(true)
			, _rendererData(nullptr)
            , _materialContext(nullptr)
			, _material(nullptr)
		{}
		~Renderable_component();
		
		bool visible() const { return _visible; }
		void setVisible(bool visible) { _visible = visible; }
		
		bool wireframe() const { return _wireframe; }
		void setWireframe(bool wireframe) { _wireframe = wireframe; }

		bool castShadow() const { return _castShadow; }
		void setCastShadow(bool castShadow) { _castShadow = castShadow; }

		// Runtime, non-serializable
		const std::shared_ptr<Material>	& material() const { return _material; }
		void setMaterial(std::shared_ptr<Material> material) { _material = material; }

		const std::unique_ptr<Renderer_data>& rendererData() const { return _rendererData; }
		void setRendererData(std::unique_ptr<Renderer_data>&& rendererData) { _rendererData = std::move(rendererData); }

        const std::unique_ptr<Material_context>& materialContext() const { return _materialContext; }
        void setMaterialContext(std::unique_ptr<Material_context>&& materialContext) { _materialContext = std::move(materialContext); }

	private:

		bool _visible;
		bool _wireframe;
		bool _castShadow;

		// Runtime, non-serializable
		std::unique_ptr<Renderer_data> _rendererData;
        std::unique_ptr<Material_context> _materialContext;
		std::shared_ptr<Material> _material;
	};

}
