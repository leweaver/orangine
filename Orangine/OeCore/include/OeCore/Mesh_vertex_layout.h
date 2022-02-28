#pragma once

#include "Renderer_types.h"

#include <vector>

namespace oe {

    // Describes a Mesh_data object; what attributes it has, what kind of morph targets etc.s
    class Mesh_vertex_layout {
    public:
      Mesh_vertex_layout(const std::vector<Vertex_attribute_element>& vertexLayout, Mesh_index_type meshIndexType);

      Mesh_vertex_layout(
              const std::vector<Vertex_attribute_element>& vertexLayout,
              const std::vector<Vertex_attribute_semantic>& morphTargetLayout, uint8_t morphTargetCount,
              Mesh_index_type meshIndexType);

      // Get a hash that represents the state of the properties
        size_t propertiesHash() const
        {
            return _hash;
        };

        const std::vector<Vertex_attribute_element>& vertexLayout() const
        {
            return _vertexLayout;
        }

        // The semantic index of a morph target should match an entry in the vertexLayout.
        // It tells us which what we are targeting.
        const std::vector<Vertex_attribute_semantic>& morphTargetLayout() const
        {
            return _morphTargetLayout;
        }

        uint8_t morphTargetCount() const {
            return _morphTargetCount;
        }

        Mesh_index_type getMeshIndexType() const {
          return _meshIndexType;
        }

    private:
        size_t _hash;
        const std::vector<Vertex_attribute_element> _vertexLayout;
        const std::vector<Vertex_attribute_semantic> _morphTargetLayout;
        const uint8_t _morphTargetCount;
        const Mesh_index_type _meshIndexType;
    };
}
