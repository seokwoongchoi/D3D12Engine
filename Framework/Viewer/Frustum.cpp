#include "Framework.h"
#include "Frustum.h"

Frustum::Frustum()
{
	
}

Frustum::~Frustum()
{
}

void Frustum::Update()
{
	//camera->GetMatrix();
	 GlobalData::GetVP(&VP);
	
	

	//left
	planes[0].a = VP._14 + VP._11;
	planes[0].b = VP._24 + VP._21;
	planes[0].c = VP._34 + VP._31;
	planes[0].d = VP._44 + VP._41;
	//right
	planes[1].a = VP._14 - VP._11;
	planes[1].b = VP._24 - VP._21;
	planes[1].c = VP._34 - VP._31;
	planes[1].d = VP._44 - VP._41;

	//bottom
	planes[2].a = VP._14 + VP._12;
	planes[2].b = VP._24 + VP._22;
	planes[2].c = VP._34 + VP._32;
	planes[2].d = VP._44 + VP._42;
	//top
	planes[3].a = VP._14 - VP._12;
	planes[3].b = VP._24 - VP._22;
	planes[3].c = VP._34 - VP._32;
	planes[3].d = VP._44 - VP._42;

	//near
	planes[4].a = VP._13;
	planes[4].b = VP._23;
	planes[4].c = VP._33;
	planes[4].d = VP._43;
	//far
	planes[5].a = VP._14 - VP._13;
	planes[5].b = VP._24 - VP._23;
	planes[5].c = VP._34 - VP._33;
	planes[5].d = VP._44 - VP._43;
	
	//for (uint i = 0; i < 4; i++)
	//{
	//	//left
	//	planes[0][i] = W.m[i][3] + W.m[i][0];
	//	//right
	//	planes[1][i] = W.m[i][3] + W.m[i][0];
	//	//bottom
	//	planes[2][i] = W.m[i][3] + W.m[i][1];
	//	//top
	//	planes[3][i] = W.m[i][3] + W.m[i][1];
	//	//near
	//	planes[4][i] = W.m[i][2];
	//	//far
	//	planes[5][i] = W.m[i][3] + W.m[i][2];
	//}
	
	

	for (int i = 0; i < 6; i++)
		D3DXPlaneNormalize(&planes[i], &planes[i]);


}

void Frustum::Render(Plane * plane)
{
	memcpy(plane, this->planes, sizeof(Plane) * 6);
}

bool Frustum::ContainPoint(Vector3 & position)
{
	for (int i = 0; i < 6; i++)
	{
		if (D3DXPlaneDotCoord(&planes[i], &position) < 0.0f)
		{
			return false;
		}
	}

	return true;
}

bool Frustum::ContainRect(float xCenter, float yCenter, float zCenter, float xSize, float ySize, float zSize)
{
	for (int i = 0; i < 6; i++)
	{
		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter - xSize), (yCenter - ySize), (zCenter - zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter + xSize), (yCenter - ySize), (zCenter - zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter - xSize), (yCenter + ySize), (zCenter - zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter - xSize), (yCenter - ySize), (zCenter + zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter + xSize), (yCenter + ySize), (zCenter - zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter + xSize), (yCenter - ySize), (zCenter + zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter - xSize), (yCenter + ySize), (zCenter + zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter + xSize), (yCenter + ySize), (zCenter + zSize))) >= 0.0f)
			continue;

		return false;
	}

	return true;
}

bool Frustum::ContainRect(const Vector3 & center, const Vector3 & size)
{
	for (int i = 0; i < 6; i++)
	{
		if (D3DXPlaneDotCoord(&planes[i], &Vector3((center.x - size.x), (center.y - size.y), (center.z - size.z))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((center.x + size.x), (center.y - size.y), (center.z - size.z))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((center.x - size.x), (center.y + size.y), (center.z - size.z))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((center.x - size.x), (center.y - size.y), (center.z + size.z))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((center.x + size.x), (center.y + size.y), (center.z - size.z))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((center.x + size.x), (center.y - size.y), (center.z + size.z))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((center.x - size.x), (center.y + size.y), (center.z + size.z))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((center.x + size.x), (center.y + size.y), (center.z + size.z))) >= 0.0f)
			continue;

		return false;
	}

	return true;
}

//bool Frustum::ContainRect(Vector3 center, Vector3 size)
//{
//	return ContainRect(center.x, center.y, center.z, size.x, size.y, size.z);
//}

bool Frustum::ContainCube(Vector3 & center, float radius)
{


	for (int i = 0; i < 6; i++)
	{

		check.x = center.x - radius;
		check.y = center.y - radius;
		check.z = center.z - radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x + radius;
		check.y = center.y - radius;
		check.z = center.z - radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x - radius;
		check.y = center.y + radius;
		check.z = center.z - radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x + radius;
		check.y = center.y + radius;
		check.z = center.z - radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x - radius;
		check.y = center.y - radius;
		check.z = center.z + radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x + radius;
		check.y = center.y - radius;
		check.z = center.z + radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x - radius;
		check.y = center.y + radius;
		check.z = center.z + radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x + radius;
		check.y = center.y + radius;
		check.z = center.z + radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;


		return false;
	}
	return true;


}
