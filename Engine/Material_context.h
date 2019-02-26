#pragma once

#include "Renderer_enum.h"

#include <wrl/client.h>
#include <set>
#include "Mesh_data.h"

namespace oe {
    /**
     * Material context is a where a compiled Material resources are stored for a 
     * specific mesh. This is necessary as multiple renderables (with differing layouts)
     * can point to a single Material instance.
     * 
     * Don't modify anything in here directly, unless you are Material_manager::bind
     */
    class Material_context {
    public:

        struct Compiled_material {
            
            // State that the compiled state relied on
            size_t materialHash;
            size_t meshHash;
            Render_pass_blend_mode blendMode;
            std::vector<Vertex_attribute_semantic> vsInputs;
            std::set<std::string> flags;

            // D3D Shaders
            Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
            Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
            Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
        };

        Material_context() = default;

        ~Material_context()
        {
            resetShaderResourceViews();
            resetSamplerStates();
        }

        void resetShaderResourceViews()
        {
            for (auto srv : shaderResourceViews) {
                if (srv) srv->Release();
            }
            shaderResourceViews.resize(0);
        }
        void resetSamplerStates()
        {
            for (auto ss : samplerStates) {
                if (ss) ss->Release();
            }
            samplerStates.resize(0);
        }

        std::shared_ptr<Compiled_material> compiledMaterial;

        // These are not stored as ComPtr, so that the array can be passed directly to d3d.
        // When adding and removing from here, remember to AddRef and Release.
        std::vector<ID3D11ShaderResourceView*> shaderResourceViews;
        std::vector<ID3D11SamplerState*> samplerStates;
    };
}
