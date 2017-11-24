#pragma once

#include <DirectXMath.h>

namespace OE
{
	namespace Math
	{
		const DirectX::XMVECTOR VEC_ZERO = DirectX::XMVectorZero();
		const DirectX::XMVECTOR VEC_ONE = DirectX::XMVectorSplatOne();

		//Right Handed Coord System
		const DirectX::XMVECTOR VEC_UP = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		const DirectX::XMVECTOR VEC_DOWN = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
		const DirectX::XMVECTOR VEC_LEFT = DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);
		const DirectX::XMVECTOR VEC_RIGHT = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
		const DirectX::XMVECTOR VEC_FORWARD = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
		const DirectX::XMVECTOR VEC_BACKWARD = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		const DirectX::XMVECTOR QUAT_IDENTITY = DirectX::XMQuaternionIdentity();
	}
}