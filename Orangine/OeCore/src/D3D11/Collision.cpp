#include "pch.h"

#include "OeCore/Collision.h"
#include "DirectX_utils.h"
#include <DirectXCollision.h>

oe::BoundingSphere oe::BoundingSphere::createFromPoints(Float3* points, int numPoints, size_t strideBytes) {
    DirectX::BoundingSphere dxBoundingSphere;
    DirectX::BoundingSphere::CreateFromPoints(dxBoundingSphere, numPoints, reinterpret_cast<DirectX::XMFLOAT3*>(points), strideBytes);
    return oe::BoundingSphere(LoadVector3(dxBoundingSphere.Center), dxBoundingSphere.Radius);
}

void oe::BoundingSphere::createMerged(BoundingSphere& out, const BoundingSphere& input1, const BoundingSphere& input2) {
    DirectX::BoundingSphere bs;
    DirectX::BoundingSphere::CreateMerged(bs,
        DirectX::BoundingSphere(StoreVector3(input1.center), input1.radius),
        DirectX::BoundingSphere(StoreVector3(input2.center), input2.radius)
    );
    out.center = LoadVector3(bs.Center);
    out.radius = bs.Radius;
}

DirectX::ContainmentType oe::BoundingFrustumRH::Contains(const oe::BoundingSphere& sh) const
{
    using namespace DirectX;

    // Load origin and orientation of the frustum.
    XMVECTOR vOrigin = XMLoadFloat3(&Origin);
    XMVECTOR vOrientation = XMLoadFloat4(&Orientation);

    // Create 6 planes (do it inline to encourage use of registers)
    XMVECTOR NearPlane = XMVectorSet(0.0f, 0.0f, 1.0f, -Near);
    NearPlane = DirectX::Internal::XMPlaneTransform(NearPlane, vOrientation, vOrigin);
    NearPlane = XMPlaneNormalize(NearPlane);

    XMVECTOR FarPlane = XMVectorSet(0.0f, 0.0f, -1.0f, Far);
    FarPlane = DirectX::Internal::XMPlaneTransform(FarPlane, vOrientation, vOrigin);
    FarPlane = XMPlaneNormalize(FarPlane);

    XMVECTOR RightPlane = XMVectorSet(1.0f, 0.0f, -RightSlope, 0.0f);
    RightPlane = DirectX::Internal::XMPlaneTransform(RightPlane, vOrientation, vOrigin);
    RightPlane = XMPlaneNormalize(RightPlane);

    XMVECTOR LeftPlane = XMVectorSet(-1.0f, 0.0f, LeftSlope, 0.0f);
    LeftPlane = DirectX::Internal::XMPlaneTransform(LeftPlane, vOrientation, vOrigin);
    LeftPlane = XMPlaneNormalize(LeftPlane);

    XMVECTOR TopPlane = XMVectorSet(0.0f, 1.0f, -TopSlope, 0.0f);
    TopPlane = DirectX::Internal::XMPlaneTransform(TopPlane, vOrientation, vOrigin);
    TopPlane = XMPlaneNormalize(TopPlane);

    XMVECTOR BottomPlane = XMVectorSet(0.0f, -1.0f, BottomSlope, 0.0f);
    BottomPlane = DirectX::Internal::XMPlaneTransform(BottomPlane, vOrientation, vOrigin);
    BottomPlane = XMPlaneNormalize(BottomPlane);

    auto bsh = DirectX::BoundingSphere(StoreVector3(sh.center), sh.radius);

    return bsh.ContainedBy(NearPlane, FarPlane, RightPlane, LeftPlane, TopPlane, BottomPlane);
}