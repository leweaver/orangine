#include "pch.h"

#include "../Engine/Utils.h"

#include <SimpleMath.h>

using namespace DirectX;
using namespace std::string_literals;
using namespace oe;

class Utils_tests : public ::testing::Test {
};

TEST_F(Utils_tests, isPowerOfTwo)
{
	static_assert(is_power_of_two(1 << 0));
	static_assert(is_power_of_two(1 << 1));
	static_assert(is_power_of_two(1 << 2));
	static_assert(is_power_of_two(1 << 3));
	static_assert(is_power_of_two(1 << 4));
	//...
	static_assert(is_power_of_two(1 << 31));

	// 2 ^ n can never be zero.
	static_assert(!is_power_of_two(0));

	static_assert(!is_power_of_two(3));
	static_assert(!is_power_of_two(5));
	static_assert(!is_power_of_two(6));
	static_assert(!is_power_of_two(7));
	static_assert(!is_power_of_two(9));
	//...
	static_assert(!is_power_of_two(UINT32_MAX));
}