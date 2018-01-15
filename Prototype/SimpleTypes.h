#pragma once

namespace OE {
	typedef unsigned long long AssetId;
	typedef const AssetId &FAssetId;

	struct Color
	{
		Color() : Color(0, 0, 0) {}
		Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}	

		uint8_t r, g, b;
	};
	struct ColorA
	{
		ColorA() : ColorA(0, 0, 0, 255) {}
		ColorA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}

		uint8_t r, g, b, a;
	};
}