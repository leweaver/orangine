#pragma once

#include "Material.h"
#include "Texture.h"

#include <memory>
#include <array>
#include "SimpleMath.h"
#include "Material_base.h"

namespace oe {
	struct PBR_material_vs_constant_buffer : Vertex_constant_buffer_base
	{
		DirectX::XMMATRIX worldView; // for debugging only
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
        DirectX::XMFLOAT4 morphWeights[2];
	};

	struct PBR_material_ps_constant_buffer : Pixel_constant_buffer_base
	{
		DirectX::XMMATRIX world;
		DirectX::XMFLOAT4 baseColor;
		DirectX::XMFLOAT4 metallicRoughness; // metallic, roughness
		DirectX::XMFLOAT4 emissive; // emissive color (RGB)
		DirectX::XMFLOAT4 eyePosition;
	};

	class PBR_material : public Material_base<
		PBR_material_vs_constant_buffer,
		PBR_material_ps_constant_buffer>
	{
		using Base_type = Material_base<
			PBR_material_vs_constant_buffer,
			PBR_material_ps_constant_buffer>;

		enum Texture_type
		{
			BaseColor,
			MetallicRoughness,
			Normal,
			Occlusion,
			Emissive,

			NumTextureTypes
		};

	public:

		static constexpr uint32_t max_lights = 8;

		PBR_material();

		PBR_material(const PBR_material& other) = delete;
		PBR_material(PBR_material&& other) = delete;
		void operator=(const PBR_material& other) = delete;
		void operator=(PBR_material&& other) = delete;

		virtual ~PBR_material() = default;

		/*
		 * The RGBA components of the base color of the material.The fourth component(A) is the alpha coverage of the material.
		 * The alphaMode property specifies how alpha is interpreted. These values are linear.
		 * If a baseColorTexture is specified, this value is multiplied with the texel values.
		 */
		DirectX::SimpleMath::Color baseColor() const
		{
			return _baseColor;
		}
		void setBaseColor(const DirectX::SimpleMath::Color& color)
		{
			_baseColor = color;
		}

		/*
		 * The base color texture. This texture contains RGB(A) components in sRGB color space.
		 * The first three components (RGB) specify the base color of the material. If the fourth component (A) is present, it represents the alpha coverage of the material.
		 * Otherwise, an alpha of 1.0 is assumed. The alphaMode property specifies how alpha is interpreted.
		 * The stored texels must not be premultiplied.
		 */
		const std::shared_ptr<Texture>& baseColorTexture() const
		{
			return _textures[BaseColor];
		}

		void setBaseColorTexture(const std::shared_ptr<Texture>& baseColorTexture)
		{
			if (_textures[BaseColor] != baseColorTexture) {
				_textures[BaseColor] = baseColorTexture;
				markRequiresRecompile();
			}
		}

		/*
		 * The metalness of the material. A value of 1.0 means the material is a metal.
		 * A value of 0.0 means the material is a dielectric.
		 * Values in between are for blending between metals and dielectrics such as dirty metallic surfaces. This value is linear.
		 * If a metallicRoughnessTexture is specified, this value is multiplied with the metallic texel values.
		 */
		float metallicFactor() const
		{
			return _metallic;
		}

		void setMetallicFactor(float metallic)
		{
			_metallic = metallic;
		}

		/*
		 * The roughness of the material. A value of 1.0 means the material is completely rough.
		 * A value of 0.0 means the material is completely smooth. This value is linear.
		 * If a metallicRoughnessTexture is specified, this value is multiplied with the roughness texel values.
		 */
		float roughnessFactor() const
		{
			return _roughness;
		}

		void setRoughnessFactor(float roughness)
		{
			_roughness = roughness;
		}

		const DirectX::SimpleMath::Color& emissiveFactor() const
		{
			return _emissive;
		}

		void setEmissiveFactor(const DirectX::SimpleMath::Color& emissive)
		{
			_emissive = emissive;
		}

		float alphaCutoff() const
		{
			return _alphaCutoff;
		}

		void setAlphaCutoff(float alphaCutoff)
		{
			_alphaCutoff = alphaCutoff;
		}

		/*
		 * The metallic-roughness texture.
		 * The metalness values are sampled from the B channel. The roughness values are sampled from the G channel.
		 * These values are linear.
		 * If other channels are present (R or A), they are ignored for metallic-roughness calculations.
		 */
		const std::shared_ptr<Texture>& metallicRoughnessTexture() const
		{
			return _textures[MetallicRoughness];
		}

		void setMetallicRoughnessTexture(const std::shared_ptr<Texture>& metallicRoughnessTexture)
		{
			if (_textures[MetallicRoughness] != metallicRoughnessTexture) {
				_textures[MetallicRoughness] = metallicRoughnessTexture;
				markRequiresRecompile();
			}
		}

		const std::shared_ptr<Texture>& normalTexture() const
		{
			return _textures[Normal];
		}

		void setNormalTexture(const std::shared_ptr<Texture>& normalTexture)
		{
			if (_textures[Normal] != normalTexture) {
				_textures[Normal] = normalTexture;
				markRequiresRecompile();
			}
		}

		const std::shared_ptr<Texture>& occlusionTexture() const
		{
			return _textures[Occlusion];
		}

		void setOcclusionTexture(const std::shared_ptr<Texture>& occlusionTexture)
		{
			if (_textures[Occlusion] != occlusionTexture) {
				_textures[Occlusion] = occlusionTexture;
				markRequiresRecompile();
			}
		}

		const std::shared_ptr<Texture>& emissiveTexture() const
		{
			return _textures[Emissive];
		}

		void setEmissiveTexture(const std::shared_ptr<Texture>& emissiveTexture)
		{
			if (_textures[Emissive] != emissiveTexture) {
				_textures[Emissive] = emissiveTexture;
				markRequiresRecompile();
			}
		}

		Material_light_mode lightMode() override { return Material_light_mode::Lit; }
		const std::string& materialType() const override;

        nlohmann::json serialize(bool compilerPropertiesOnly) const override;

        std::set<std::string> configFlags(Render_pass_blend_mode blendMode, const Mesh_vertex_layout& meshVertexLayout) const override;
        std::vector<Vertex_attribute_element> vertexInputs(const std::set<std::string>& flags) const override;
        Shader_resources shaderResources(const Render_light_data& renderLightData) const override;
	    Shader_compile_settings vertexShaderSettings(const std::set<std::string>& flags) const override;
        Shader_compile_settings pixelShaderSettings(const std::set<std::string>& flags) const override;

        static void decodeMorphTargetConfig(const std::set<std::string>& flags,
            uint8_t& targetCount,
            int8_t& positionPosition,
            int8_t& normalPosition,
            int8_t& tangentPosition);

	protected:

        void applyVertexLayoutShaderCompileSettings(Shader_compile_settings&) const;

		void updateVSConstantBufferValues(PBR_material_vs_constant_buffer& constants,
			const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix,
            const Renderer_animation_data& rendererAnimationData) const override;

		void updatePSConstantBufferValues(PBR_material_ps_constant_buffer& constants,
			const DirectX::SimpleMath::Matrix& worldMatrix,
			const DirectX::SimpleMath::Matrix& viewMatrix,
			const DirectX::SimpleMath::Matrix& projMatrix) const override;

        bool requiresTexCoord0() const
        {
            return
                _textures[BaseColor] ||
                _textures[MetallicRoughness] ||
                _textures[Emissive] ||
                _textures[Occlusion] ||
                _textures[Normal];
        }

        bool requiresTangents() const
        {
            return _textures[Normal] != nullptr;
        }

        static int getMorphPositionAttributeIndexOffset();
        static int getMorphNormalAttributeIndexOffset();
        int getMorphTangentAttributeIndexOffset() const;

	private:

		DirectX::SimpleMath::Color _baseColor;
		float _metallic;
		float _roughness;
		DirectX::SimpleMath::Color _emissive;
		float _alphaCutoff;

		std::array<std::shared_ptr<Texture>, NumTextureTypes> _textures;

		// Compiled state
		uint32_t _boundTextureCount;
		std::array<ID3D11SamplerState*, NumTextureTypes> _samplerStates{};
		std::array<ID3D11ShaderResourceView*, NumTextureTypes> _shaderResourceViews{};
	};
}
