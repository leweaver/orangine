#include "pch.h"
#include "Mesh_vertex_layout.h"

#include "Utils.h"

using namespace oe;

Mesh_vertex_layout::Mesh_vertex_layout(const std::vector<Vertex_attribute_semantic>& vertexLayout)
    : Mesh_vertex_layout(vertexLayout, {}, 0)
{   
}

Mesh_vertex_layout::Mesh_vertex_layout(
    const std::vector<Vertex_attribute_semantic>& vertexLayout,
    const std::vector<Vertex_attribute_semantic>& morphTargetLayout,
    uint8_t morphTargetCount)
    : _hash(0)
    , _vertexLayout(vertexLayout)
    , _morphTargetLayout(morphTargetLayout)
    , _morphTargetCount(morphTargetCount)
{
    _hash = std::hash<uint8_t>{}(morphTargetCount);
    for (const auto layoutEntry : vertexLayout) {
        hash_combine(_hash, layoutEntry.attribute);
        hash_combine(_hash, layoutEntry.semanticIndex);
    }
    for (const auto layoutEntry : morphTargetLayout) {
        hash_combine(_hash, layoutEntry.attribute);
        hash_combine(_hash, layoutEntry.semanticIndex);
    }
}
