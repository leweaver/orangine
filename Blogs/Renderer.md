# Introduction
When I started developing this engine about a year ago, I had a pretty specific goal in mind: create a 3D renderer as a way to learn the DirectX 11 API (I had previously worked heavily in DirectX 9). To make sure that I didn't get lost in the weeds, for this first version I specifically chose to keep the scope down to providing only enough functionality to correctly render glTF 2.0 files with a deferred lighting pipeline. 

I haven't focused too heavily on performance optimization in this v1.0; nor anything fancy (such as cascading shadow maps).

I have used a few resources to aid in this: 
- [Introduction to 3D Game Programming with DirectX 11](https://www.amazon.com/Introduction-3D-Game-Programming-DirectX/dp/1936420228), by Frank Luna
    - This is an excellent book, however many of the references and examples are out of date since the D3D effects framework is now deprecated.
- [DirectXTK](https://github.com/Microsoft/DirectXTK)
    - Mainly for the SimpleMath library, CommonStates and DeviceResources classes.
    - I specifically didn't use or refer to their PBR implementation, as I wanted to learn how to create my own!
- [Disney BRDF Notes](http://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf)
- [glTF 2.0 PBR notes](https://github.com/KhronosGroup/glTF-WebGL-PBR/)
- [glTF 2.0 Specification](https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md)
- [IBLBaker - Envmap Irradiance Baker](http://www.derkreature.com/iblbaker/)
     - Used to precompute irradiance specular, diffuse and brdf textures.
- [CryEngine 3 Deferred Shading slides](https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3)
    - There are a lot of tips here that I haven't implemented yet, especially around normal storage in the G-Buffer.

# 

# G-Buffer Format

# 