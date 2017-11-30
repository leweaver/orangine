#include "pch.h"
#include "RenderableComponent.h"
#include "DeviceResources.h"

using namespace DirectX;
using namespace OE;

void RenderableComponent::Initialize()
{
}

void RenderableComponent::Update()
{
}

struct SimpleVertexCombined
{
	XMFLOAT3 Pos;
	XMFLOAT3 Col;
};

RendererData* RenderableComponent::CreateRendererData(const DX::DeviceResources deviceResources)
{
	m_rendererData = std::make_unique<RendererData>();
	{
		// Supply the actual vertex data.
		const float size = 1.0f;
		const unsigned int NUM_VERTICES = 8;
		SimpleVertexCombined verticesCombo[] =
		{/*
			XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f),
			XMFLOAT3(1.0f, -0.0f, 0.0f), XMFLOAT3(0.5f, 0.0f, 0.0f),
			XMFLOAT3(0.0f, 1.0f, 0.0f),  XMFLOAT3(0.0f, 0.5f, 0.0f)
			*/				
			XMFLOAT3( size,  size,  size), XMFLOAT3(1.0f, 1.0f, 1.0f),
			XMFLOAT3(-size,  size,  size), XMFLOAT3(0.0f, 1.0f, 1.0f),
			XMFLOAT3( size, -size,  size), XMFLOAT3(1.0f, 0.0f, 1.0f),
			XMFLOAT3(-size, -size,  size), XMFLOAT3(0.0f, 0.0f, 1.0f),

			XMFLOAT3( size,  size, -size), XMFLOAT3(1.0f, 1.0f, 0.0f),
			XMFLOAT3(-size,  size, -size), XMFLOAT3(0.0f, 1.0f, 0.0f),
			XMFLOAT3( size, -size, -size), XMFLOAT3(1.0f, 0.0f, 0.0f),
			XMFLOAT3(-size, -size, -size), XMFLOAT3(0.0f, 0.0f, 0.0f),
		};

		// Fill in a buffer description.
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(SimpleVertexCombined) * NUM_VERTICES;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = verticesCombo;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the vertex buffer.
		m_rendererData->m_vertexBuffers.push_back(std::make_unique<VertexBufferDesc>());
		VertexBufferDesc &vertexBufferDesc = *m_rendererData->m_vertexBuffers.rbegin()->get();

		// Get a reference
		vertexBufferDesc.m_stride = sizeof(SimpleVertexCombined);
		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &vertexBufferDesc.m_buffer);
		assert(!FAILED(hr));
	}

	// ---------------
	{
		// Create indices.
		//unsigned int indices[] = { 0, 1, 2 };
		const int NUM_INDICES = 36;
		unsigned int indices[NUM_INDICES];

		int pos = 0;
		// -X side
		indices[pos++] = 5; indices[pos++] = 7; indices[pos++] = 3;
		indices[pos++] = 5; indices[pos++] = 3; indices[pos++] = 1;

		// +X side
		indices[pos++] = 6; indices[pos++] = 4; indices[pos++] = 0;
		indices[pos++] = 6; indices[pos++] = 0; indices[pos++] = 2;

		// -Y side
		indices[pos++] = 7; indices[pos++] = 6; indices[pos++] = 2;
		indices[pos++] = 7; indices[pos++] = 2; indices[pos++] = 3;

		// +Y side
		indices[pos++] = 4; indices[pos++] = 5; indices[pos++] = 1;
		indices[pos++] = 4; indices[pos++] = 1; indices[pos++] = 0;

		// -Z side
		indices[pos++] = 5; indices[pos++] = 4; indices[pos++] = 6;
		indices[pos++] = 5; indices[pos++] = 6; indices[pos++] = 7;

		// +Z side
		indices[pos++] = 3; indices[pos++] = 2; indices[pos++] = 0;
		indices[pos++] = 3; indices[pos++] = 0; indices[pos++] = 1;


		// Fill in a buffer description.
		m_rendererData->m_indexCount = NUM_INDICES;
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(unsigned int) * NUM_INDICES;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		// Define the resource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = indices;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the buffer with the device.
		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &m_rendererData->m_indexBuffer);
		assert(!FAILED(hr));
	}

	return m_rendererData.get();
}
