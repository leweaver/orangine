#include "OeCore/Mesh_vertex_layout.h"

#include "OeCore/EngineUtils.h"

using namespace oe;

Mesh_vertex_layout::Mesh_vertex_layout(
        const std::vector<Vertex_attribute_element>& vertexLayout, Mesh_index_type meshIndexType)
    : Mesh_vertex_layout(vertexLayout, {}, 0, meshIndexType)
{}

Mesh_vertex_layout::Mesh_vertex_layout(
        const std::vector<Vertex_attribute_element>& vertexLayout,
        const std::vector<Vertex_attribute_semantic>& morphTargetLayout, uint8_t morphTargetCount,
        Mesh_index_type meshIndexType)
    : _hash(0)
    , _vertexLayout(vertexLayout)
    , _morphTargetLayout(morphTargetLayout)
    , _morphTargetCount(morphTargetCount)
    , _meshIndexType(meshIndexType)
{
  _hash = std::hash<uint8_t>{}(morphTargetCount);
  hash_combine(_hash, meshIndexType);
  
  for (const auto layoutEntry : vertexLayout) {
    hash_combine(_hash, layoutEntry.semantic.attribute);
    hash_combine(_hash, layoutEntry.semantic.semanticIndex);
    hash_combine(_hash, layoutEntry.type);
    hash_combine(_hash, layoutEntry.component);
  }
  for (const auto layoutEntry : morphTargetLayout) {
    hash_combine(_hash, layoutEntry.attribute);
    hash_combine(_hash, layoutEntry.semanticIndex);
  }
}
