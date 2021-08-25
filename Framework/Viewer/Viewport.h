#pragma once
class Viewport
{
public:
	Viewport(float width, float height, float x = 0, float y = 0, float minDepth = 0, float maxDepth = 1);
	~Viewport();
		
	void Set(float width, float height, float x = 0, float y = 0, float minDepth = 0, float maxDepth = 1);

	float GetWidth() { return width; }
	float GetHeight() { return height; }

	void GetRay(OUT Vector3* position, OUT Vector3* direction, IN const Matrix& w, IN const Matrix& v, IN const Matrix& p);
	void Unprojection(OUT Vector3* position, const Vector3& source, const Matrix& W, const Matrix& V, const Matrix& P);
	void Projection(OUT Vector3* position, const Vector3& source, const Matrix& W, const Matrix& V, const Matrix& P);
private:
	float x, y;
	float width, height;
	float minDepth, maxDepth;

	D3D12_VIEWPORT viewport;

	Matrix invView;
	Matrix invWorld;
	Vector3 cameraPosition;
};