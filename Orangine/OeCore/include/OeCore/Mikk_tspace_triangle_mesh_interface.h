#pragma once

#include "Mesh_data.h"

#include <OeThirdParty/mikktspace.h>

#include <memory>

namespace oe
{
	class Mikk_tspace_triangle_mesh_interface
	{
		struct User_data
		{
			std::shared_ptr<Mesh_data> meshData;
			const Mesh_vertex_buffer_accessor *positionAccessor;
			const Mesh_vertex_buffer_accessor *normalAccessor;
			const Mesh_vertex_buffer_accessor *texCoordAccessor;
			Mesh_vertex_buffer_accessor *tangentAccessor;
			Mesh_vertex_buffer_accessor *bitangentAccessor;
		};

		SMikkTSpaceInterface m_interface;

	public:

		/*
		 * meshData must contain vertex accessors for VA_POSITION, VA_NORMAL, and a VA_TEXCOORD_?
		 * as specified by texCoordAttribute (default VA_TEXCOORD_0).
		 * 
		 * generated tangents will be placed into tangentBufferAccessor is not null.
		 * 
		 * If a bitangent accessor is provided, bitangents will also be populated unless
		 * generateBitangentsIfPresent == false.
		 */
		explicit Mikk_tspace_triangle_mesh_interface(const std::shared_ptr<Mesh_data>& meshData,
            Vertex_attribute_semantic texCoordAttribute = { Vertex_attribute::Tex_Coord, 0 },
			bool generateBitangentsIfPresent = true);
		
		SMikkTSpaceInterface *getInterface() { return &m_interface; }
		void* userData() { return &_userData; }

	private:
		// API Methods
		static int getNumFaces(const SMikkTSpaceContext *pContext);
		static int getNumVerticesOfFace(const SMikkTSpaceContext * pContext, const int iFace);
		static void getPosition(const SMikkTSpaceContext * pContext, float fvPosOut[], const int iFace, const int iVert);
		static void getNormal(const SMikkTSpaceContext * pContext, float fvNormOut[], const int iFace, const int iVert);
		static void getTexCoord(const SMikkTSpaceContext * pContext, float fvTexcOut[], const int iFace, const int iVert);
		static void setTSpaceBasic(const SMikkTSpaceContext * pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert);
		static void setTSpace(const SMikkTSpaceContext * pContext, const float fvTangent[], const float fvBiTangent[], const float fMagS, const float fMagT, 
			const tbool bIsOrientationPreserving, const int iFace, const int iVert);


		// Helper Methods
		static uint32_t getVertexIndex(User_data *userData, const int iFace, const int iVert);
		static User_data* getUserData(const SMikkTSpaceContext *pContext)
		{
			return reinterpret_cast<User_data*>(pContext->m_pUserData);
		}

		User_data _userData;
	};
}
