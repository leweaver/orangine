//-------------------------------------------------------------------------------------
// Much of the implementation of BoundingFrustum is taken from DirectXCollision:
//
// DirectXCollision.inl -- C++ Collision Math library
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//  
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkID=615560
//-------------------------------------------------------------------------------------

#pragma once

inline void oe::BoundingFrustumRH::CreateFromMatrix(BoundingFrustumRH& Out, DirectX::FXMMATRIX Projection)
{
	using namespace DirectX;
		
	// Corners of the projection frustum in homogenous space.
	static XMVECTORF32 HomogenousPoints[6] =
	{
		{ { {  1.0f,  0.0f, -1.0f, 1.0f } } },   // right (at far plane)
		{ { { -1.0f,  0.0f, -1.0f, 1.0f } } },   // left
		{ { {  0.0f,  1.0f, -1.0f, 1.0f } } },   // top
		{ { {  0.0f, -1.0f, -1.0f, 1.0f } } },   // bottom

		{ { {  0.0f,  0.0f,  0.0f, 1.0f } } },   // near
		{ { {  0.0f,  0.0f,  1.0f, 1.0f } } }    // far
	};

	XMVECTOR Determinant;
	XMMATRIX matInverse = XMMatrixInverse(&Determinant, Projection);

	// Compute the frustum corners in world space.
	XMVECTOR Points[6];

	for (size_t i = 0; i < 6; ++i)
	{
		// Transform point.
		Points[i] = XMVector4Transform(HomogenousPoints[i], matInverse);
	}

	Out.Origin = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Out.Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	// Compute the slopes.
	Points[0] = XMVectorMultiply(Points[0], XMVectorReciprocal(XMVectorSplatZ(Points[0])));
	Points[1] = XMVectorMultiply(Points[1], XMVectorReciprocal(XMVectorSplatZ(Points[1])));
	Points[2] = XMVectorMultiply(Points[2], XMVectorReciprocal(XMVectorSplatZ(Points[2])));
	Points[3] = XMVectorMultiply(Points[3], XMVectorReciprocal(XMVectorSplatZ(Points[3])));

	Out.RightSlope = XMVectorGetX(Points[0]);
	Out.LeftSlope = XMVectorGetX(Points[1]);
	Out.TopSlope = XMVectorGetY(Points[2]);
	Out.BottomSlope = XMVectorGetY(Points[3]);

	// Compute near and far.
	Points[4] = XMVectorMultiply(Points[4], XMVectorReciprocal(XMVectorSplatW(Points[4])));
	Points[5] = XMVectorMultiply(Points[5], XMVectorReciprocal(XMVectorSplatW(Points[5])));

	Out.Near = XMVectorGetZ(Points[4]);
	Out.Far = XMVectorGetZ(Points[5]);
}

inline void oe::BoundingFrustumRH::GetCorners(DirectX::XMFLOAT3* Corners) const
{
	using namespace DirectX;

	assert(Corners != 0);

	// Load origin and orientation of the frustum.
	XMVECTOR vOrigin = XMLoadFloat3(&Origin);
	XMVECTOR vOrientation = XMLoadFloat4(&Orientation);

	assert(DirectX::Internal::XMQuaternionIsUnit(vOrientation));

	// Build the corners of the frustum.
	XMVECTOR vRightTop = XMVectorSet(RightSlope, TopSlope, 1.0f, 0.0f);
	XMVECTOR vRightBottom = XMVectorSet(RightSlope, BottomSlope, 1.0f, 0.0f);
	XMVECTOR vLeftTop = XMVectorSet(LeftSlope, TopSlope, 1.0f, 0.0f);
	XMVECTOR vLeftBottom = XMVectorSet(LeftSlope, BottomSlope, 1.0f, 0.0f);
	XMVECTOR vNear = XMVectorReplicatePtr(&Near);
	XMVECTOR vFar = XMVectorReplicatePtr(&Far);

	// Returns 8 corners position of bounding frustum.
	//     Near    Far
	//    0----1  4----5
	//    |    |  |    |
	//    |    |  |    |
	//    3----2  7----6

	XMVECTOR vCorners[CORNER_COUNT];
	vCorners[0] = XMVectorMultiply(vLeftTop, vNear);
	vCorners[1] = XMVectorMultiply(vRightTop, vNear);
	vCorners[2] = XMVectorMultiply(vRightBottom, vNear);
	vCorners[3] = XMVectorMultiply(vLeftBottom, vNear);
	vCorners[4] = XMVectorMultiply(vLeftTop, vFar);
	vCorners[5] = XMVectorMultiply(vRightTop, vFar);
	vCorners[6] = XMVectorMultiply(vRightBottom, vFar);
	vCorners[7] = XMVectorMultiply(vLeftBottom, vFar);

	for (size_t i = 0; i < CORNER_COUNT; ++i)
	{
		XMVECTOR C = XMVectorAdd(XMVector3Rotate(vCorners[i], vOrientation), vOrigin);
		XMStoreFloat3(&Corners[i], C);
	}
}