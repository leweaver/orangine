#pragma once

#include "Component.h"

#include "Mesh_data.h"

/*
 * CPU side mesh vertex, index or animation buffer. Used to create a new mesh. 
 */
namespace oe 
{

	class Mesh_data_component : public Component
	{
		DECLARE_COMPONENT_TYPE;

	public:
		Mesh_data_component(std::shared_ptr<Entity> entity)
			: Component(entity)
			, _meshData(nullptr)
		{}

		~Mesh_data_component() = default;

		// TODO: Will eventually reference an asset ID here, rather than actual mesh data.

		const std::shared_ptr<Mesh_data>& meshData() const { return _meshData; }
		void setMeshData(const std::shared_ptr<Mesh_data>& meshData) { _meshData = meshData; }

	private:

		std::shared_ptr<Mesh_data> _meshData;
	};
}

