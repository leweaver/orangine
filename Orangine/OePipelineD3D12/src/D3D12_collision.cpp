#include "OeCore/Collision.h"
#include "D3D12_vendor.h"

#include <DirectXCollision.h>

oe::BoundingSphere
oe::BoundingSphere::createFromPoints(Float3* points, int numPoints, size_t strideBytes) {
  DirectX::BoundingSphere dxBoundingSphere;
  DirectX::BoundingSphere::CreateFromPoints(
      dxBoundingSphere, numPoints, reinterpret_cast<DirectX::XMFLOAT3*>(points), strideBytes);
  return oe::BoundingSphere(LoadVector3(dxBoundingSphere.Center), dxBoundingSphere.Radius);
}

void oe::BoundingSphere::createMerged(
    BoundingSphere& out,
    const BoundingSphere& input1,
    const BoundingSphere& input2) {
  DirectX::BoundingSphere bs;
  DirectX::BoundingSphere::CreateMerged(
      bs,
      DirectX::BoundingSphere(StoreVector3(input1.center), input1.radius),
      DirectX::BoundingSphere(StoreVector3(input2.center), input2.radius));
  out.center = LoadVector3(bs.Center);
  out.radius = bs.Radius;
}

DirectX::ContainmentType oe::BoundingFrustumRH::Contains(const oe::BoundingSphere& sh) const {
  using namespace DirectX;

  XMFLOAT3 originFlt3 = StoreVector3(origin);
  XMFLOAT4 orientationFlt4 = StoreQuat(orientation);

  // Load origin and orientation of the frustum.
  XMVECTOR vOrigin = XMLoadFloat3(&originFlt3);
  XMVECTOR vOrientation = XMLoadFloat4(&orientationFlt4);

  // Create 6 planes (do it inline to encourage use of registers)
  XMVECTOR NearPlane = XMVectorSet(0.0f, 0.0f, 1.0f, -nearPlane);
  NearPlane = DirectX::Internal::XMPlaneTransform(NearPlane, vOrientation, vOrigin);
  NearPlane = XMPlaneNormalize(NearPlane);

  XMVECTOR FarPlane = XMVectorSet(0.0f, 0.0f, -1.0f, farPlane);
  FarPlane = DirectX::Internal::XMPlaneTransform(FarPlane, vOrientation, vOrigin);
  FarPlane = XMPlaneNormalize(FarPlane);

  XMVECTOR RightPlane = XMVectorSet(1.0f, 0.0f, -rightSlope, 0.0f);
  RightPlane = DirectX::Internal::XMPlaneTransform(RightPlane, vOrientation, vOrigin);
  RightPlane = XMPlaneNormalize(RightPlane);

  XMVECTOR LeftPlane = XMVectorSet(-1.0f, 0.0f, leftSlope, 0.0f);
  LeftPlane = DirectX::Internal::XMPlaneTransform(LeftPlane, vOrientation, vOrigin);
  LeftPlane = XMPlaneNormalize(LeftPlane);

  XMVECTOR TopPlane = XMVectorSet(0.0f, 1.0f, -topSlope, 0.0f);
  TopPlane = DirectX::Internal::XMPlaneTransform(TopPlane, vOrientation, vOrigin);
  TopPlane = XMPlaneNormalize(TopPlane);

  XMVECTOR BottomPlane = XMVectorSet(0.0f, -1.0f, bottomSlope, 0.0f);
  BottomPlane = DirectX::Internal::XMPlaneTransform(BottomPlane, vOrientation, vOrigin);
  BottomPlane = XMPlaneNormalize(BottomPlane);

  auto bsh = DirectX::BoundingSphere(StoreVector3(sh.center), sh.radius);

  return bsh.ContainedBy(NearPlane, FarPlane, RightPlane, LeftPlane, TopPlane, BottomPlane);
}

bool oe::BoundingOrientedBox::contains(const oe::BoundingSphere& boundingSphere) const {
  DirectX::BoundingOrientedBox bob(
      StoreVector3(center), StoreVector3(extents), StoreQuat(orientation));
  DirectX::BoundingSphere bs(StoreVector3(boundingSphere.center), boundingSphere.radius);
  return bob.Contains(bs);
}

void oe::BoundingFrustumRH::CreateFromMatrix(
    BoundingFrustumRH& Out,
    const SSE::Matrix4& projection) {
  using namespace DirectX;

  // Corners of the projection frustum in homogenous space.
  static SSE::Vector4 HomogenousPoints[6] = {
      {1.0f, 0.0f, -1.0f, 1.0f},  // right (at far plane)
      {-1.0f, 0.0f, -1.0f, 1.0f}, // left
      {0.0f, 1.0f, -1.0f, 1.0f},  // top
      {0.0f, -1.0f, -1.0f, 1.0f}, // bottom

      {0.0f, 0.0f, 0.0f, 1.0f}, // near
      {0.0f, 0.0f, 1.0f, 1.0f}  // far
  };

  const auto projectionInv = SSE::inverse(projection);

  // Compute the frustum corners in world space.
  XMVECTOR Points[6];

  for (size_t i = 0; i < 6; ++i) {
    // Transform point.
    auto point = StoreVector4(projectionInv * HomogenousPoints[i]);
    Points[i] = XMLoadFloat4(&point);
  }

  Out.origin = SSE::Vector3(0.0f);
  Out.orientation = SSE::Quat::identity();

  // Compute the slopes.
  Points[0] = XMVectorMultiply(Points[0], XMVectorReciprocal(XMVectorSplatZ(Points[0])));
  Points[1] = XMVectorMultiply(Points[1], XMVectorReciprocal(XMVectorSplatZ(Points[1])));
  Points[2] = XMVectorMultiply(Points[2], XMVectorReciprocal(XMVectorSplatZ(Points[2])));
  Points[3] = XMVectorMultiply(Points[3], XMVectorReciprocal(XMVectorSplatZ(Points[3])));

  Out.rightSlope = XMVectorGetX(Points[0]);
  Out.leftSlope = XMVectorGetX(Points[1]);
  Out.topSlope = XMVectorGetY(Points[2]);
  Out.bottomSlope = XMVectorGetY(Points[3]);

  // Compute near and far.
  Points[4] = XMVectorMultiply(Points[4], XMVectorReciprocal(XMVectorSplatW(Points[4])));
  Points[5] = XMVectorMultiply(Points[5], XMVectorReciprocal(XMVectorSplatW(Points[5])));

  Out.nearPlane = XMVectorGetZ(Points[4]);
  Out.farPlane = XMVectorGetZ(Points[5]);
}

void oe::BoundingFrustumRH::GetCorners(SSE::Vector3* Corners) const {
  using namespace DirectX;

  assert(Corners != 0);

  XMFLOAT3 originFlt3 = StoreVector3(origin);
  XMFLOAT4 orientationFlt4 = StoreQuat(orientation);

  // Load origin and orientation of the frustum.
  XMVECTOR vOrigin = XMLoadFloat3(&originFlt3);
  XMVECTOR vOrientation = XMLoadFloat4(&orientationFlt4);

  assert(DirectX::Internal::XMQuaternionIsUnit(vOrientation));

  // Build the corners of the frustum.
  XMVECTOR vRightTop = XMVectorSet(rightSlope, topSlope, 1.0f, 0.0f);
  XMVECTOR vRightBottom = XMVectorSet(rightSlope, bottomSlope, 1.0f, 0.0f);
  XMVECTOR vLeftTop = XMVectorSet(leftSlope, topSlope, 1.0f, 0.0f);
  XMVECTOR vLeftBottom = XMVectorSet(leftSlope, bottomSlope, 1.0f, 0.0f);
  XMVECTOR vNear = XMVectorReplicatePtr(&nearPlane);
  XMVECTOR vFar = XMVectorReplicatePtr(&farPlane);

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

  XMFLOAT3 corner;
  for (size_t i = 0; i < CORNER_COUNT; ++i) {
    XMVECTOR C = XMVectorAdd(XMVector3Rotate(vCorners[i], vOrientation), vOrigin);
    XMStoreFloat3(&corner, C);
    Corners[i] = LoadVector3(corner);
  }
}