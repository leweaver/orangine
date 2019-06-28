
//--------------------------------------------------------------------------------------
// Colour Space
//--------------------------------------------------------------------------------------

float3 SRGBtoLINEAR(float3 inColor)
{
	// See: https://en.wikipedia.org/wiki/SRGB#The_reverse_transformation
	float3 edge = step(0.04045, inColor);
	return lerp(inColor / 12.92, pow((inColor + 0.055) / (1.055), 2.4), edge);
}