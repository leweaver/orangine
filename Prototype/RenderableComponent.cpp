#include "pch.h"
#include "RenderableComponent.h"
#include "DeviceResources.h"

using namespace DirectX;

void OE::RenderableComponent::Initialize()
{
}

void OE::RenderableComponent::Update()
{
}

struct SimpleVertexCombined
{
	XMFLOAT3 Pos;
	XMFLOAT3 Col;
};

void OE::RenderableComponent::CreateRendererData(const DX::DeviceResources deviceResources)
{
	{
		// Supply the actual vertex data.
		SimpleVertexCombined verticesCombo[] =
		{
			XMFLOAT3(0.0f, 0.5f, 0.5f),
			XMFLOAT3(0.0f, 0.0f, 0.5f),
			XMFLOAT3(0.5f, -0.5f, 0.5f),
			XMFLOAT3(0.5f, 0.0f, 0.0f),
			XMFLOAT3(-0.5f, -0.5f, 0.5f),
			XMFLOAT3(0.0f, 0.5f, 0.0f),
		};

		// Fill in a buffer description.
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(SimpleVertexCombined) * 3;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = verticesCombo;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the vertex buffer.
		ID3D11Buffer*      pVertexBuffer;
		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &pVertexBuffer);
		assert(!FAILED(hr));
	}

	// ---------------
	{
		// Create indices.
		unsigned int indices[] = { 0, 1, 2 };

		// Fill in a buffer description.
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = sizeof(unsigned int) * 3;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		// Define the resource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = indices;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the buffer with the device.
		ID3D11Buffer *g_pIndexBuffer = nullptr;
		HRESULT hr = deviceResources.GetD3DDevice()->CreateBuffer(&bufferDesc, &InitData, &g_pIndexBuffer);
		assert(!FAILED(hr));

		// Set the buffer.
		deviceResources.GetD3DDeviceContext()->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	}
}
