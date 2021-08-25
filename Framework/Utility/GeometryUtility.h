#pragma once

template <typename T> class Geometry;

class GeometryUtility
{
public:
	static void CreateQuad(Geometry<struct VertexTexture>& geometry);
    static void CreateQuad(Geometry<struct VertexTextureNormal>& geometry);

    static void CreateScreenQuad
    (
        Geometry<struct VertexTexture>& geometry,
        const uint& width,
        const uint& height
    );

    static void CreateCube(Geometry<struct VertexTexture>& geometry);
	static void CreateSphere
	(
		Geometry<struct VertexTexture>& geometry,
		const float& radius = 1.0f,
		const int& slices = 15,
		const int& stacks = 15
	);

    static void CreateSphere
    (
        Geometry<struct VertexTextureNormalTangentBlend>& geometry,
        const float& radius = 1.0f,
        const int& slices = 15,
        const int& stacks = 15
    );
};