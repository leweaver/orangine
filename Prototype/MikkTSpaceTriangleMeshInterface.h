#pragma once

#include <mikktspace.h>

#include <memory>
#include "MeshData.h"

namespace OE
{
	class MikkTSpaceTriangleMeshInterface
	{
		struct UserData
		{
			std::shared_ptr<MeshData> meshData;
			const MeshVertexBufferAccessor *positionAccessor;
			const MeshVertexBufferAccessor *normalAccessor;
			const MeshVertexBufferAccessor *texCoordAccessor;
			MeshVertexBufferAccessor *tangentAccessor;
			MeshVertexBufferAccessor *bitangentAccessor;
		} m_userData;

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
		MikkTSpaceTriangleMeshInterface(std::shared_ptr<MeshData> meshData,
			VertexAttribute texCoordAttribute = VertexAttribute::VA_TEXCOORD_0,
			bool generateBitangentsIfPresent = true);
		
		SMikkTSpaceInterface *getInterface()
		{
			return &m_interface;
		}

		void *getUserData()
		{
			return &m_userData;
		}

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
		static uint32_t getVertexIndex(UserData *userData, const int iFace, const int iVert);
		static UserData *getUserData(const SMikkTSpaceContext *pContext)
		{
			return reinterpret_cast<UserData*>(pContext->m_pUserData);
		}
	};
}
