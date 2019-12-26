#pragma once

#include "Renderer_enum.h"
#include "Material_context.h"

#include <memory>
#include <array>
#include <map>
#include <vectormath/vectormath.hpp>

namespace oe 
{
	struct D3D_buffer
	{
		D3D_buffer(ID3D11Buffer* buffer, size_t size);
		D3D_buffer();
		~D3D_buffer();

		ID3D11Buffer* const d3dBuffer;
        const size_t size;
	};

	struct D3D_buffer_accessor
	{
		D3D_buffer_accessor(std::shared_ptr<D3D_buffer> buffer, uint32_t stride, uint32_t offset);
		~D3D_buffer_accessor();

		std::shared_ptr<D3D_buffer> buffer;
		uint32_t stride;
		uint32_t offset;
	};

    struct Renderer_animation_data {
		static constexpr size_t morphWeightsSize = 8;
        std::array<float, morphWeightsSize> morphWeights;

        uint32_t numBoneTransforms = 0;
        std::array<SSE::Matrix4, g_max_bone_transforms> boneTransformConstants;
    };

	struct Renderer_data
	{
		Renderer_data();
		~Renderer_data();

		void release();

        unsigned int vertexCount;
		std::map<Vertex_attribute_semantic, std::unique_ptr<D3D_buffer_accessor>> vertexBuffers;
        
		std::unique_ptr<D3D_buffer_accessor> indexBufferAccessor;
		DXGI_FORMAT indexFormat;
		unsigned int indexCount;

		D3D11_PRIMITIVE_TOPOLOGY topology;

		bool failedRendering;
	};

    // NOTE: When adding new items to this, make sure to also add them to the hashing function
    struct Renderer_features_enabled {
        bool vertexMorph = true;
        bool skinnedAnimation = true;

        Debug_display_mode debugDisplayMode = Debug_display_mode::None;
        bool shadowsEnabled = true;
        bool irradianceMappingEnabled = true;
        bool enableShaderOptimization = false;
        size_t hash() const;
    };
}
