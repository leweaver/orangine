#include "pch.h"

#include "MikkTSpaceTriangleMeshInterface.h"
#include <utility>
#include "SimpleMath.h"

using namespace OE;
using namespace std::literals;

const char * MSG_INDEX_BUFFER_FORMAT = "MikkTSpaceTriangleMeshInterface requires MeshData with a valid index buffer format (16/32 bit SINT/UINT).";

MikkTSpaceTriangleMeshInterface::MikkTSpaceTriangleMeshInterface(std::shared_ptr<MeshData> meshData,
	VertexAttribute texCoordAttribute,
	bool generateBitangentsIfPresent)
	: m_userData {
		meshData,
		meshData->m_vertexBufferAccessors[VertexAttribute::VA_POSITION].get(),
		meshData->m_vertexBufferAccessors[VertexAttribute::VA_NORMAL].get(),
		meshData->m_vertexBufferAccessors[texCoordAttribute].get(),
		meshData->m_vertexBufferAccessors[VertexAttribute::VA_TANGENT].get(),
		meshData->m_vertexBufferAccessors[VertexAttribute::VA_BITANGENT].get(),
	}
	, m_interface()
{
	if (meshData->m_meshIndexType != MeshIndexType::TRIANGLES)
		throw std::logic_error("MikkTSpaceTriangleMeshInterface only supports MeshData with MeshIndexType TRiANGLES.");

	if (!meshData->m_indexBufferAccessor || (meshData->m_indexBufferAccessor->m_count % 3) != 0)
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires MeshData with a valid index buffer (count must be multiple of 3).");
	if (!(
		meshData->m_indexBufferAccessor->m_format == DXGI_FORMAT_R16_UINT || 
		meshData->m_indexBufferAccessor->m_format == DXGI_FORMAT_R32_UINT ||
		meshData->m_indexBufferAccessor->m_format == DXGI_FORMAT_R16_SINT ||
		meshData->m_indexBufferAccessor->m_format == DXGI_FORMAT_R32_SINT)) 
	{
		throw std::logic_error(MSG_INDEX_BUFFER_FORMAT);
	}

	if (!m_userData.positionAccessor)
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires MeshData with a valid VA_POSITION vertex buffer.");
	if (!m_userData.normalAccessor)
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires MeshData with a valid VA_NORMAL vertex buffer.");
	if (!m_userData.texCoordAccessor) {
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires MeshData with a valid "s + 
			VertexAttributeMeta::semanticName(texCoordAttribute) +
			" vertex buffer.");
	}
	if (!m_userData.tangentAccessor) {
		throw std::logic_error("MikkTSpaceTriangleMeshInterface requires a valid "s +
			VertexAttributeMeta::semanticName(VertexAttribute::VA_TANGENT) +
			" vertex buffer either on the given MeshData, or as a constructor argument.");
	}

	m_interface.m_getNumFaces = &MikkTSpaceTriangleMeshInterface::getNumFaces;
	m_interface.m_getNumVerticesOfFace = &MikkTSpaceTriangleMeshInterface::getNumVerticesOfFace;
	m_interface.m_getPosition = &MikkTSpaceTriangleMeshInterface::getPosition;
	m_interface.m_getNormal = &MikkTSpaceTriangleMeshInterface::getNormal;
	m_interface.m_getTexCoord = &MikkTSpaceTriangleMeshInterface::getTexCoord;
	
	if (m_userData.bitangentAccessor)
		m_interface.m_setTSpace = &MikkTSpaceTriangleMeshInterface::setTSpace;
	else
		m_interface.m_setTSpaceBasic = &MikkTSpaceTriangleMeshInterface::setTSpaceBasic;
}

int MikkTSpaceTriangleMeshInterface::getNumFaces(const SMikkTSpaceContext *pContext)
{
	UserData *userData = getUserData(pContext);

	const uint32_t vertexCount = userData->meshData->m_indexBufferAccessor->m_count / 3;
	assert(vertexCount <= INT32_MAX);
	return static_cast<int32_t>(vertexCount);
}

int MikkTSpaceTriangleMeshInterface::getNumVerticesOfFace(const SMikkTSpaceContext *pContext, const int iFace)
{
	return 3;
}

void readVector3Accessor(float *fvPosOut, const MeshVertexBufferAccessor * const vertexAccessor, const uint32_t indexValue)
{
	assert(indexValue <= vertexAccessor->m_count);
	assert(sizeof(DirectX::SimpleMath::Vector3) <= vertexAccessor->m_stride);
	const uint8_t *pos = vertexAccessor->m_buffer->m_data + vertexAccessor->m_offset + indexValue * vertexAccessor->m_stride;
	const auto vector = reinterpret_cast<const DirectX::SimpleMath::Vector3*>(pos);

	fvPosOut[0] = vector->x;
	fvPosOut[1] = vector->y;
	fvPosOut[2] = vector->z;
}

void writeVector4Accessor(DirectX::SimpleMath::Vector4 src, MeshVertexBufferAccessor *vertexAccessor, const uint32_t indexValue)
{
	assert(indexValue <= vertexAccessor->m_count);
	assert(sizeof(DirectX::SimpleMath::Vector4) <= vertexAccessor->m_stride);
	uint8_t *pos = vertexAccessor->m_buffer->m_data + vertexAccessor->m_offset + indexValue * vertexAccessor->m_stride;
	const auto vector = reinterpret_cast<DirectX::SimpleMath::Vector4*>(pos);

	*vector = src;
}

void readVector2Accessor(float *fvPosOut, const MeshVertexBufferAccessor * const vertexAccessor, const uint32_t indexValue)
{
	assert(indexValue <= vertexAccessor->m_count);
	assert(sizeof(DirectX::SimpleMath::Vector2) <= vertexAccessor->m_stride);
	const uint8_t *pos = vertexAccessor->m_buffer->m_data + vertexAccessor->m_offset + indexValue * vertexAccessor->m_stride;
	const auto vector = reinterpret_cast<const DirectX::SimpleMath::Vector2*>(pos);

	fvPosOut[0] = vector->x;
	fvPosOut[1] = vector->y;
}

void MikkTSpaceTriangleMeshInterface::getPosition(const SMikkTSpaceContext *pContext, float fvPosOut[], const int iFace,
	const int iVert)
{
	const auto userData = getUserData(pContext);
	const uint32_t indexValue = getVertexIndex(userData, iFace, iVert);
	readVector3Accessor(fvPosOut, userData->positionAccessor, indexValue);
}

void MikkTSpaceTriangleMeshInterface::getNormal(const SMikkTSpaceContext *pContext, float fvNormOut[], const int iFace,
	const int iVert)
{
	const auto userData = getUserData(pContext);
	const uint32_t indexValue = getVertexIndex(userData, iFace, iVert);
	readVector3Accessor(fvNormOut, userData->normalAccessor, indexValue);
}

void MikkTSpaceTriangleMeshInterface::getTexCoord(const SMikkTSpaceContext *pContext, float fvTexcOut[], const int iFace,
	const int iVert)
{
	const auto userData = getUserData(pContext);
	const uint32_t indexValue = getVertexIndex(userData, iFace, iVert);
	readVector2Accessor(fvTexcOut, userData->texCoordAccessor, indexValue);
}

void MikkTSpaceTriangleMeshInterface::setTSpaceBasic(const SMikkTSpaceContext *pContext, const float fvTangent[],
	const float fSign, const int iFace, const int iVert)
{
	const auto userData = getUserData(pContext);
	const uint32_t indexValue = getVertexIndex(userData, iFace, iVert);
	writeVector4Accessor(
		{ fvTangent[0], fvTangent[1], fvTangent[2], fSign }, 
		userData->tangentAccessor, 
		indexValue);
}

void MikkTSpaceTriangleMeshInterface::setTSpace(const SMikkTSpaceContext *pContext, const float fvTangent[],
	const float fvBiTangent[], const float fMagS, const float fMagT, const tbool bIsOrientationPreserving, const int iFace,
	const int iVert)
{
	const auto userData = getUserData(pContext);
	const uint32_t indexValue = getVertexIndex(userData, iFace, iVert);
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

uint32_t MikkTSpaceTriangleMeshInterface::getVertexIndex(UserData *userData, const int iFace, const int iVert)
{
	// Determine the index buffer value
	const auto indexAccessor = userData->meshData->m_indexBufferAccessor.get();
	const auto index = static_cast<uint32_t>(iFace * 3 + iVert);
	assert(index <= indexAccessor->m_count);
	uint8_t *indexBufferPos = indexAccessor->m_buffer->m_data + indexAccessor->m_offset + index * indexAccessor->m_stride;

	switch (indexAccessor->m_format)
	{
	case DXGI_FORMAT_R16_UINT:
		return static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(indexBufferPos));
	case DXGI_FORMAT_R32_UINT:
		return static_cast<uint32_t>(*reinterpret_cast<uint32_t*>(indexBufferPos));
	case DXGI_FORMAT_R16_SINT:
	{
		auto *signedvalue = reinterpret_cast<int16_t*>(indexBufferPos);
		assert(*signedvalue >= 0);
		return static_cast<uint32_t>(*signedvalue);
	}
	case DXGI_FORMAT_R32_SINT:
	{
		auto *signedvalue = reinterpret_cast<int32_t*>(indexBufferPos);
		assert(*signedvalue >= 0);
		return static_cast<uint32_t>(*signedvalue);
	}
	default:
		throw std::runtime_error(MSG_INDEX_BUFFER_FORMAT);
	}
}