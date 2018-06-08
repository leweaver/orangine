﻿#include "pch.h"

#include "Mikk_tspace_triangle_mesh_interface.h"
#include "SimpleMath.h"
#include "Mesh_utils.h"

using namespace oe;
using namespace std::literals;

const char* g_msg_index_buffer_format = "MikkTSpaceTriangleMeshInterface requires MeshData with a valid index buffer format (16/32 bit SINT/UINT).";

Mikk_tspace_triangle_mesh_interface::Mikk_tspace_triangle_mesh_interface(const std::shared_ptr<Mesh_data>& meshData,
	Vertex_attribute texCoordAttribute,
	bool generateBitangentsIfPresent)
	: m_interface()
	, _userData{
		meshData,
		meshData->vertexBufferAccessors[Vertex_attribute::Position].get(),
		meshData->vertexBufferAccessors[Vertex_attribute::Normal].get(),
		meshData->vertexBufferAccessors[texCoordAttribute].get(),
		meshData->vertexBufferAccessors[Vertex_attribute::Tangent].get(),
		meshData->vertexBufferAccessors[Vertex_attribute::Bi_Tangent].get(),
	}
{
	if (meshData->m_meshIndexType != Mesh_index_type::Triangles)
		throw std::logic_error("MikkTSpaceTriangleMeshInterface only supports MeshData with MeshIndexType TRiANGLES.");

	if (!meshData->indexBufferAccessor || (meshData->indexBufferAccessor->count % 3) != 0)
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires MeshData with a valid index buffer (count must be multiple of 3).");
	if (!(
		meshData->indexBufferAccessor->format == DXGI_FORMAT_R16_UINT || 
		meshData->indexBufferAccessor->format == DXGI_FORMAT_R32_UINT ||
		meshData->indexBufferAccessor->format == DXGI_FORMAT_R16_SINT ||
		meshData->indexBufferAccessor->format == DXGI_FORMAT_R32_SINT)) 
	{
		throw std::logic_error(g_msg_index_buffer_format);
	}

	if (!_userData.positionAccessor)
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires MeshData with a valid VA_POSITION vertex buffer.");
	if (!_userData.normalAccessor)
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires MeshData with a valid VA_NORMAL vertex buffer.");
	if (!_userData.texCoordAccessor) {
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires MeshData with a valid "s.append(
			Vertex_attribute_meta::semanticName(texCoordAttribute)) +
			" vertex buffer.");
	}
	if (!_userData.tangentAccessor) {
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires a valid "s.append(
			Vertex_attribute_meta::semanticName(Vertex_attribute::Tangent)) +
			" vertex buffer either on the given MeshData, or as a constructor argument.");
	}

	m_interface.m_getNumFaces = &Mikk_tspace_triangle_mesh_interface::getNumFaces;
	m_interface.m_getNumVerticesOfFace = &Mikk_tspace_triangle_mesh_interface::getNumVerticesOfFace;
	m_interface.m_getPosition = &Mikk_tspace_triangle_mesh_interface::getPosition;
	m_interface.m_getNormal = &Mikk_tspace_triangle_mesh_interface::getNormal;
	m_interface.m_getTexCoord = &Mikk_tspace_triangle_mesh_interface::getTexCoord;
	
	if (_userData.bitangentAccessor && generateBitangentsIfPresent)
		m_interface.m_setTSpace = &Mikk_tspace_triangle_mesh_interface::setTSpace;
	else
		m_interface.m_setTSpaceBasic = &Mikk_tspace_triangle_mesh_interface::setTSpaceBasic;
}

int Mikk_tspace_triangle_mesh_interface::getNumFaces(const SMikkTSpaceContext* pContext)
{
	auto* userData = getUserData(pContext);

	const auto vertexCount = userData->meshData->indexBufferAccessor->count / 3;
	assert(vertexCount <= INT32_MAX);
	return static_cast<int32_t>(vertexCount);
}

int Mikk_tspace_triangle_mesh_interface::getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
{
	return 3;
}

void readVector3Accessor(float* fvPosOut, const Mesh_vertex_buffer_accessor*  const vertexAccessor, const uint32_t indexValue)
{
	assert(indexValue <= vertexAccessor->count);
	assert(sizeof(DirectX::SimpleMath::Vector3) <= vertexAccessor->stride);
	const auto* pos = vertexAccessor->buffer->data + vertexAccessor->offset + indexValue * vertexAccessor->stride;
	const auto vector = reinterpret_cast<const DirectX::SimpleMath::Vector3*>(pos);

	fvPosOut[0] = vector->x;
	fvPosOut[1] = vector->y;
	fvPosOut[2] = vector->z;
}

void writeVector4Accessor(DirectX::SimpleMath::Vector4 src, Mesh_vertex_buffer_accessor* vertexAccessor, const uint32_t indexValue)
{
	assert(indexValue <= vertexAccessor->count);
	assert(sizeof(DirectX::SimpleMath::Vector4) <= vertexAccessor->stride);
	auto* pos = vertexAccessor->buffer->data + vertexAccessor->offset + indexValue * vertexAccessor->stride;
	const auto vector = reinterpret_cast<DirectX::SimpleMath::Vector4*>(pos);

	*vector = src;
}

void readVector2Accessor(float* fvPosOut, const Mesh_vertex_buffer_accessor*  const vertexAccessor, const uint32_t indexValue)
{
	assert(indexValue <= vertexAccessor->count);
	assert(sizeof(DirectX::SimpleMath::Vector2) <= vertexAccessor->stride);
	const auto* pos = vertexAccessor->buffer->data + vertexAccessor->offset + indexValue * vertexAccessor->stride;
	const auto vector = reinterpret_cast<const DirectX::SimpleMath::Vector2*>(pos);

	fvPosOut[0] = vector->x;
	fvPosOut[1] = vector->y;
}

void Mikk_tspace_triangle_mesh_interface::getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace,
	const int iVert)
{
	const auto userData = getUserData(pContext);
	const auto indexValue = getVertexIndex(userData, iFace, iVert);
	readVector3Accessor(fvPosOut, userData->positionAccessor, indexValue);
}

void Mikk_tspace_triangle_mesh_interface::getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace,
	const int iVert)
{
	const auto userData = getUserData(pContext);
	const auto indexValue = getVertexIndex(userData, iFace, iVert);
	readVector3Accessor(fvNormOut, userData->normalAccessor, indexValue);
}

void Mikk_tspace_triangle_mesh_interface::getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace,
	const int iVert)
{
	const auto userData = getUserData(pContext);
	const auto indexValue = getVertexIndex(userData, iFace, iVert);
	readVector2Accessor(fvTexcOut, userData->texCoordAccessor, indexValue);
}

void Mikk_tspace_triangle_mesh_interface::setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[],
	const float fSign, const int iFace, const int iVert)
{
	const auto userData = getUserData(pContext);
	const auto indexValue = getVertexIndex(userData, iFace, iVert);
	writeVector4Accessor(
		{ fvTangent[0], fvTangent[1], fvTangent[2], fSign }, 
		userData->tangentAccessor, 
		indexValue);
}

void Mikk_tspace_triangle_mesh_interface::setTSpace(const SMikkTSpaceContext* pContext, const float fvTangent[],
	const float fvBiTangent[], const float fMagS, const float fMagT, const tbool bIsOrientationPreserving, const int iFace,
	const int iVert)
{
	const auto userData = getUserData(pContext);
	const auto indexValue = getVertexIndex(userData, iFace, iVert);
	const auto fSign = bIsOrientationPreserving ? 1.0f : -1.0f;

	writeVector4Accessor(
		{ fvTangent[0], fvTangent[1], fvTangent[2], fSign },
		userData->tangentAccessor,
		indexValue);

	writeVector4Accessor(
		{ fvBiTangent[0], fvBiTangent[1], fvBiTangent[2], fSign },
		userData->bitangentAccessor,
		indexValue);
}

uint32_t Mikk_tspace_triangle_mesh_interface::getVertexIndex(User_data* userData, const int iFace, const int iVert)
{
	// Determine the index buffer value
	const auto indexAccessor = userData->meshData->indexBufferAccessor.get();
	const auto index = static_cast<uint32_t>(iFace * 3 + iVert);
	assert(index <= indexAccessor->count);
	const auto indexBufferPos = indexAccessor->buffer->data + indexAccessor->offset + index * indexAccessor->stride;

	return mesh_utils::convertIndexValue(indexAccessor->format, indexBufferPos);
}