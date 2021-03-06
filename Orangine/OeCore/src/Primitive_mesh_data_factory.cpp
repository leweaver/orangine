#include "OeCore/Primitive_mesh_data_factory.h"
#include "OeCore/Mesh_data.h"
#include "OeCore/Mikk_tspace_triangle_mesh_interface.h"
#include "OeCore/Collision.h"
#include "OeCore/Color.h"
#include "OeCore/Math_constants.h"

#include <array>
#include <cstddef>

#include "D3D12/D3D12_vendor.h"
#include <GeometricPrimitive.h>

using namespace oe;
using namespace DirectX;

void addIndicesAccessor(Mesh_data& meshData, std::vector<uint16_t>&& indices)
{
	// Indices
	const auto srcSize = sizeof(std::remove_reference_t<decltype(indices)>::value_type) * indices.size();
	assert(srcSize < static_cast<unsigned long long>(INT32_MAX));

	auto meshBuffer = std::make_shared<Mesh_buffer>(static_cast<int>(srcSize));
	memcpy_s(meshBuffer->data, meshBuffer->dataSize, indices.data(), srcSize);

	meshData.indexBufferAccessor = std::make_unique<Mesh_index_buffer_accessor>(meshBuffer, 
        Element_component::Unsigned_short,
		static_cast<uint32_t>(indices.size()), 
        static_cast<uint32_t>(sizeof(uint16_t)), 
        0);
}

template <class TVertex_type, int TSemantic_index = 0>
void addPositionBufferAccessor(std::shared_ptr<Mesh_data> meshData, std::shared_ptr<Mesh_buffer> meshBuffer, const std::vector<TVertex_type>& vertices) {
    constexpr auto vertexSize = sizeof(TVertex_type);
    meshData->vertexBufferAccessors[{Vertex_attribute::Position, TSemantic_index}] = std::make_unique<Mesh_vertex_buffer_accessor>(
        meshBuffer,
        Vertex_attribute_element{ {Vertex_attribute::Position, TSemantic_index}, Element_type::Vector3, Element_component::Float },
        static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(vertexSize),
        static_cast<uint32_t>(offsetof(TVertex_type, position))
        );
}

template <class TVertex_type, int TSemantic_index = 0>
void addColorBufferAccessor(std::shared_ptr<Mesh_data> meshData, std::shared_ptr<Mesh_buffer> meshBuffer, const std::vector<TVertex_type>& vertices) {
    constexpr auto vertexSize = sizeof(TVertex_type);
    meshData->vertexBufferAccessors[{Vertex_attribute::Color, TSemantic_index}] = std::make_unique<Mesh_vertex_buffer_accessor>(
        meshBuffer,
        Vertex_attribute_element{ {Vertex_attribute::Color, TSemantic_index}, Element_type::Vector4, Element_component::Float },
        static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(vertexSize),
        static_cast<uint32_t>(offsetof(TVertex_type, color))
        );
}

template <class TVertex_type, int TSemantic_index = 0>
void addNormalBufferAccessor(std::shared_ptr<Mesh_data> meshData, std::shared_ptr<Mesh_buffer> meshBuffer, const std::vector<TVertex_type>& vertices) {
    constexpr auto vertexSize = sizeof(TVertex_type);
    meshData->vertexBufferAccessors[{Vertex_attribute::Normal, TSemantic_index}] = std::make_unique<Mesh_vertex_buffer_accessor>(
        meshBuffer,
        Vertex_attribute_element{ {Vertex_attribute::Normal, TSemantic_index}, Element_type::Vector3, Element_component::Float },
        static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(vertexSize),
        static_cast<uint32_t>(offsetof(TVertex_type, normal))
        );
}

template <class TVertex_type, int TSemantic_index = 0>
void addTextureBufferAccessor(std::shared_ptr<Mesh_data> meshData, std::shared_ptr<Mesh_buffer> meshBuffer, const std::vector<TVertex_type>& vertices) {
    constexpr auto vertexSize = sizeof(TVertex_type);
    meshData->vertexBufferAccessors[{Vertex_attribute::Tex_coord, TSemantic_index}] = std::make_unique<Mesh_vertex_buffer_accessor>(
        meshBuffer,
        Vertex_attribute_element{ {Vertex_attribute::Tex_coord, TSemantic_index}, Element_type::Vector2, Element_component::Float },
        static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(vertexSize),
        static_cast<uint32_t>(offsetof(TVertex_type, textureCoordinate))
        );
}

template <class TVertex_type, int TSemantic_index = 0>
void addTangentBufferAccessor(std::shared_ptr<Mesh_data> meshData, const std::vector<TVertex_type>& vertices) {
    auto tangentsBuffer = std::make_shared<Mesh_buffer>(static_cast<int>(sizeof(XMFLOAT4) * vertices.size()));
    meshData->vertexBufferAccessors[{Vertex_attribute::Tangent, TSemantic_index}] = std::make_unique<Mesh_vertex_buffer_accessor>(
        tangentsBuffer,
        Vertex_attribute_element{ {Vertex_attribute::Tangent, TSemantic_index}, Element_type::Vector4, Element_component::Float },
        static_cast<uint32_t>(vertices.size()), static_cast<uint32_t>(sizeof(XMFLOAT4)),
        0
        );

    Mikk_tspace_triangle_mesh_interface generator(meshData);
    SMikkTSpaceContext context;

    context.m_pUserData = generator.userData();
    context.m_pInterface = generator.getInterface();
    if (!genTangSpaceDefault(&context)) {
        OE_THROW(std::exception("Failed to generate tangents"));
    }
}

template <class TVertex_type>
std::shared_ptr<Mesh_data> createMeshData(std::vector<TVertex_type>&& vertices, std::vector<uint16_t>&& indices)
{
	// Unsupported vertex type; add a specialization below

	// ReSharper disable once CppStaticAssertFailure
	static_assert(false);

	return nullptr;
}

template <>
std::shared_ptr<Mesh_data> createMeshData(std::vector<DirectX::VertexPosition>&& vertices, std::vector<uint16_t>&& indices)
{
	auto meshData = std::make_shared<Mesh_data>(Mesh_vertex_layout({
        {Vertex_attribute::Position, 0}
        }));
    addIndicesAccessor(*meshData.get(), move(indices));

    using TVertex_type = DirectX::VertexPosition;

    const auto vertexSize = sizeof(TVertex_type);
    const auto srcBufferSize = vertexSize * vertices.size();
    assert(srcBufferSize < static_cast<unsigned long long>(INT32_MAX));

    auto posNormalTexBuffer = std::make_shared<Mesh_buffer>(static_cast<int>(srcBufferSize));
    memcpy_s(posNormalTexBuffer->data, posNormalTexBuffer->dataSize, vertices.data(), srcBufferSize);

    // Position
    addPositionBufferAccessor(meshData, posNormalTexBuffer, vertices);

    return meshData;
}

template <>
std::shared_ptr<Mesh_data> createMeshData(std::vector<DirectX::VertexPositionColor>&& vertices, std::vector<uint16_t>&& indices)
{
    auto meshData = std::make_shared<Mesh_data>(Mesh_vertex_layout({
        {Vertex_attribute::Position, 0},
        {Vertex_attribute::Color, 0}
        }));
    addIndicesAccessor(*meshData.get(), move(indices));

    using TVertex_type = DirectX::VertexPositionColor;

    const auto vertexSize = sizeof(TVertex_type);
    const auto srcBufferSize = vertexSize * vertices.size();
    assert(srcBufferSize < static_cast<unsigned long long>(INT32_MAX));

    auto meshBuffer = std::make_shared<Mesh_buffer>(static_cast<int>(srcBufferSize));
    memcpy_s(meshBuffer->data, meshBuffer->dataSize, vertices.data(), srcBufferSize);

    // Position
    addPositionBufferAccessor(meshData, meshBuffer, vertices);

    // Color
    addColorBufferAccessor(meshData, meshBuffer, vertices);

    return meshData;
}

template <>
std::shared_ptr<Mesh_data> createMeshData(std::vector<DirectX::VertexPositionNormalTexture>&& vertices, std::vector<uint16_t>&& indices)
{
    auto meshData = std::make_shared<Mesh_data>(Mesh_vertex_layout({
        {Vertex_attribute::Position, 0},
        {Vertex_attribute::Normal, 0},
        {Vertex_attribute::Tex_coord, 0}
        }));
    addIndicesAccessor(*meshData.get(), move(indices));

	using TVertex_type = DirectX::VertexPositionNormalTexture;

	const auto vertexSize = sizeof(TVertex_type);
	const auto srcBufferSize = vertexSize * vertices.size();
	assert(srcBufferSize < static_cast<unsigned long long>(INT32_MAX));

	auto posNormalTexBuffer = std::make_shared<Mesh_buffer>(static_cast<int>(srcBufferSize));
	memcpy_s(posNormalTexBuffer->data, posNormalTexBuffer->dataSize, vertices.data(), srcBufferSize);

	// Position
    addPositionBufferAccessor(meshData, posNormalTexBuffer, vertices);

	// Normal
    addNormalBufferAccessor(meshData, posNormalTexBuffer, vertices);

	// Texcoord
    addTextureBufferAccessor(meshData, posNormalTexBuffer, vertices);

	// Tangents
    addTangentBufferAccessor(meshData, vertices);

	return meshData;
}

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createQuad(float width, float height)
{
	return createQuad(width, height, SSE::Vector3(-width * 0.5f, -height * 0.5f, 0.0f));
}

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createQuad(float width, float height, const SSE::Vector3& positionOffset)
{
	const auto 
		top    = positionOffset.getY() + height,
		right  = positionOffset.getX() + width,
		bottom = positionOffset.getY() + 0.0f,
		left   = positionOffset.getX() + 0.0f;
	std::vector<DirectX::GeometricPrimitive::VertexType> vertices;
	float z = positionOffset.getZ();
	XMFLOAT3 backward = { math::backward.getX(), math::backward.getY(), math::backward.getZ() };
	vertices.push_back(DirectX::GeometricPrimitive::VertexType({ left,  top,    z }, backward, { 0.0f, 0.0f }));
	vertices.push_back(DirectX::GeometricPrimitive::VertexType({ right, top,    z }, backward, { 1.0f, 0.0f }));
	vertices.push_back(DirectX::GeometricPrimitive::VertexType({ left,  bottom, z }, backward, { 0.0f, 1.0f }));
	vertices.push_back(DirectX::GeometricPrimitive::VertexType({ right, bottom, z }, backward, { 1.0f, 1.0f }));

	std::vector<uint16_t> indices = {
		0, 2, 1,
		1, 2, 3
	};

	auto md = createMeshData(move(vertices), move(indices));
	generateTangents(md);
	return md;
}

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createCone(float diameter, float height, size_t tessellation)
{
    std::vector<DirectX::GeometricPrimitive::VertexType> vertices;
    std::vector<uint16_t> indices;

    // FIXME: This doesn't seem right, as we are trying to use a RH coordinate system:
    DirectX::GeometricPrimitive::CreateCone(vertices, indices, diameter, height, tessellation, false);

    auto md = createMeshData(move(vertices), move(indices));
    generateTangents(md);
    return md;
}

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createSphere(float radius, size_t tessellation, bool invertNormals)
{
	std::vector<DirectX::GeometricPrimitive::VertexType> vertices;
	std::vector<uint16_t> indices;

	// FIXME: This doesn't seem right, as we are trying to use a RH coordinate system:
	DirectX::GeometricPrimitive::CreateSphere(vertices, indices, radius * 2.0f, tessellation, false, invertNormals);

	auto md = createMeshData(move(vertices), move(indices));
	generateTangents(md);
	return md;
}

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createBox(const SSE::Vector3& size)
{
	std::vector<DirectX::GeometricPrimitive::VertexType> vertices;
	std::vector<uint16_t> indices;

	// FIXME: This doesn't seem right, as we are trying to use a RH coordinate system:
	DirectX::GeometricPrimitive::CreateBox(vertices, indices, { size.getX(), size.getY(), size.getZ() }, false);

	auto md = createMeshData(move(vertices), move(indices));
	generateTangents(md);
	return md;
}

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createTeapot(size_t tessellation)
{
	std::vector<DirectX::GeometricPrimitive::VertexType> vertices;
	std::vector<uint16_t> indices;

	// FIXME: This doesn't seem right, as we are trying to use a RH coordinate system:
	DirectX::GeometricPrimitive::CreateTeapot(vertices, indices, 1, tessellation, false);

	auto md = createMeshData(move(vertices), move(indices));
	generateTangents(md);
	return md;
}

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createFrustumLines(const BoundingFrustumRH& frustum)
{
	constexpr auto numVertices = 8;
	// Returns 8 corners position of bounding frustum.
	//     Near    Far
	//    0----1  4----5
	//    |    |  |    |
	//    |    |  |    |
	//    3----2  7----6
	auto corners = std::array<SSE::Vector3, numVertices>();
	frustum.GetCorners(corners.data());

	std::vector<uint16_t> indices = {
		0,1, 1,2, 2,3, 3,0, // Near Plane Edges
		0,4, 1,5, 2,6, 3,7, // Far Plane Edges
		4,5, 5,6, 6,7, 7,4  // Sides
	};

	auto vertices = std::vector<DirectX::VertexPosition>(8);
	for (size_t i = 0; i < vertices.size(); ++i) {
		vertices[i].position = StoreVector3(corners[i]);
	}

	auto md = createMeshData(move(vertices), move(indices));
	md->m_meshIndexType = Mesh_index_type::Lines;
	return md;
}

std::shared_ptr<Mesh_data> Primitive_mesh_data_factory::createAxisWidgetLines() {
    /*
        y
        |    -z
        |  /
        | /
        |/______ x
        o
    */
    std::vector<DirectX::VertexPositionColor> vertices = {
        { { 0, 0, 0 }, StoreVector4(oe::Colors::Red) },
        { { 1, 0, 0 }, StoreVector4(oe::Colors::Red) },
        { { 0, 0, 0 }, StoreVector4(oe::Colors::Green) },
        { { 0, 1, 0 }, StoreVector4(oe::Colors::Green) },
        { { 0, 0, 0 }, StoreVector4(oe::Colors::Blue) },
        { { 0, 0,-1 }, StoreVector4(oe::Colors::Blue) }
    }; 
    std::vector<uint16_t> indices = { 0,1, 2,3, 4,5 };

    auto md = createMeshData(move(vertices), move(indices));
    md->m_meshIndexType = Mesh_index_type::Lines;
    return md;
}

void Primitive_mesh_data_factory::generateNormals(
	const Mesh_index_buffer_accessor &indexBufferAccessor, 
	const Mesh_vertex_buffer_accessor &positionBufferAccessor,
	Mesh_vertex_buffer_accessor &normalBufferAccessor)
{
	if (indexBufferAccessor.count % 3 != 0)
		OE_THROW(std::logic_error("Expected index buffer to have a count that is a multiple of 3."));

    if (positionBufferAccessor.attributeElement.semantic != Vertex_attribute_semantic{ Vertex_attribute::Position, 0 })
        OE_THROW(std::runtime_error("Given vertex buffer accessor must have type VA_POSITION"));

    if (normalBufferAccessor.attributeElement.semantic != Vertex_attribute_semantic{Vertex_attribute::Normal, 0})
		OE_THROW(std::runtime_error("Given normal buffer accessor must have type VA_NORMAL"));

	if (normalBufferAccessor.count != positionBufferAccessor.count)
		OE_THROW(std::runtime_error("Given position and normal buffer accessors must have the same count"));
	
	const auto indexBufferStart = indexBufferAccessor.buffer->data + indexBufferAccessor.offset;
	const auto positionBufferStart = positionBufferAccessor.buffer->data + positionBufferAccessor.offset;
	const auto normalBufferStart = normalBufferAccessor.buffer->data + normalBufferAccessor.offset;

	assert(normalBufferAccessor.stride >= sizeof(XMFLOAT3));
	const auto normalBufferEnd = normalBufferStart + normalBufferAccessor.count * normalBufferAccessor.stride + sizeof(XMFLOAT3);
		
	// Iterate over each triangle, creating a flat normal.
	if (indexBufferAccessor.component == Element_component::Unsigned_short)
	{		
		assert(sizeof(uint16_t) <= indexBufferAccessor.stride);
		const uint16_t numVertices = positionBufferAccessor.count;

		for (uint32_t idx = 0; idx < indexBufferAccessor.count;)
		{
			const auto& i0 = *reinterpret_cast<const uint16_t*>(indexBufferStart + idx * indexBufferAccessor.stride);
			const auto& i1 = *reinterpret_cast<const uint16_t*>(indexBufferStart + ++idx * indexBufferAccessor.stride);
			const auto& i2 = *reinterpret_cast<const uint16_t*>(indexBufferStart + ++idx * indexBufferAccessor.stride);
			++idx;

			if (i0 > numVertices || i1 > numVertices || i2 > numVertices) {
				OE_THROW(std::logic_error("IndexBuffer index near " + std::to_string(idx - 3) +
					" out of range of given vertexBuffer (numVertices=" + std::to_string(numVertices) + ")"));
			}

			const auto& p0 = LoadVector3(*reinterpret_cast<const XMFLOAT3*>(positionBufferStart + i0 * positionBufferAccessor.stride));
			const auto& p1 = LoadVector3(*reinterpret_cast<const XMFLOAT3*>(positionBufferStart + i1 * positionBufferAccessor.stride));
			const auto& p2 = LoadVector3(*reinterpret_cast<const XMFLOAT3*>(positionBufferStart + i2 * positionBufferAccessor.stride));

			auto normalBufferPos = normalBufferStart + i0 * normalBufferAccessor.stride;
			assert(normalBufferPos >= normalBufferStart && normalBufferPos < normalBufferEnd);
			auto n0 = LoadVector3(*reinterpret_cast<XMFLOAT3*>(normalBufferPos));
			normalBufferPos = normalBufferStart + i1 * normalBufferAccessor.stride;
			assert(normalBufferPos >= normalBufferStart && normalBufferPos < normalBufferEnd);
			auto n1 = LoadVector3(*reinterpret_cast<XMFLOAT3*>(normalBufferPos));
			normalBufferPos = normalBufferStart + i2 * normalBufferAccessor.stride;
			assert(normalBufferPos >= normalBufferStart && normalBufferPos < normalBufferEnd);
			auto n2 = LoadVector3(*reinterpret_cast<XMFLOAT3*>(normalBufferPos));
			
			// Create two vectors from which we calculate the normal
			const auto v0 = p1 - p0;
			const auto v1 = p2 - p0;
			
			// Calculate flat normal. (Counter clockwise winding order)
			n0 = SSE::normalize(SSE::cross(v0, v1));
			n1 = n2 = n0;

			/*
			 * Test values:
			 * p0 = 0, 0, 0
			 * p1 = 1, 0, 0
			 * p2 = 0, 0, -1
			 * v0 = 1, 0, 0
			 * v1 = 0, 0, -1
			 * n  = 0, 1, 0
			 */
		}
	}
}

void Primitive_mesh_data_factory::generateTangents(std::shared_ptr<Mesh_data> meshData)
{
	Mikk_tspace_triangle_mesh_interface generator(meshData);
	SMikkTSpaceContext context;

	context.m_pUserData = generator.userData();
	context.m_pInterface = generator.getInterface();
	if (!genTangSpaceDefault(&context)) {
		OE_THROW(std::exception("Failed to generate tangents"));
	}
}
