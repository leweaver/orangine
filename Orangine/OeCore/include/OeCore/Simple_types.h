#pragma once

namespace oe {
	typedef unsigned long long Asset_id;
	typedef const Asset_id& FAsset_id;

	// For use in buffers, where it is imperitive that it is exactly N floats big
	// and packing order is important.
	// Convert to a SSE::VectorN before performing mathamatical operations.
	struct Float2 {
		Float2() : x(0.0f), y(0.0f) { }
		Float2(float x, float y) : x(x), y(y) { }
		Float2(const SSE::Vector3& vec) : x(vec.getX()), y(vec.getY()) { }

		float x, y;
	};

	struct Float3 {
		Float3() : x(0.0f), y(0.0f), z(0.0f) { }
		Float3(float x, float y, float z) : x(x), y(y), z(z) { }
		Float3(const SSE::Vector3& vec) : x(vec.getX()), y(vec.getY()), z(vec.getZ()) { }

		float x, y, z;
	};

	struct Float4 {
		Float4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }
		Float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) { }
		Float4(const SSE::Vector4& vec) : x(vec.getX()), y(vec.getY()), z(vec.getZ()), w(vec.getW()) { }

		float x, y, z, w;
	};
}