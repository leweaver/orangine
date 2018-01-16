#pragma once

#include "Component.h"

#include "MeshData.h"

/*
 * CPU side mesh vertex, index or animation buffer. Used to create a new mesh. 
 */
namespace OE 
{

	class MeshDataComponent : public Component
	{
		DECLARE_COMPONENT_TYPE;

		std::shared_ptr<MeshData> m_meshData;

	public:
		MeshDataComponent();
		~MeshDataComponent();

		// TODO: Will eventually reference an asset ID here, rather than actual mesh data.

		const std::shared_ptr<MeshData> &getMeshData() const
		{
			return m_meshData;
		}

		void setMeshData(const std::shared_ptr<MeshData> &meshData)
		{
			m_meshData = meshData;
		}
	};
}

