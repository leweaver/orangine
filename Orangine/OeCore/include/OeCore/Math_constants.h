#pragma once

namespace oe {
	namespace Math {

		extern const float PI;
		extern const float PIMUL2;
		extern const float ONEDIVPI; // 1 over PI
		extern const float ONEDIV2PI;  // 1 over 2PI
		extern const float PIDIV2;
		extern const float PIDIV4;

		namespace Direction {
			extern const SSE::Vector3 Left;
			extern const SSE::Vector3 Right;
			extern const SSE::Vector3 Up;
			extern const SSE::Vector3 Down;
			extern const SSE::Vector3 Forward;
			extern const SSE::Vector3 Backward;
		}
	}
}