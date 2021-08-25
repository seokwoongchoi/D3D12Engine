#pragma once
enum class ReadMeshType:uint
{
	Default,
	StaticMesh,
	SkeletalMesh,
};

enum class ReadParticleType :uint
{
	Default,
	Spark,
	Blood,
	Smoke
};
struct shadowDesc
{
	Vector3 Position;
	Vector4 Indices;
	Vector4 Weights;
};
struct GlobalViewData
{
	Matrix view;
	Matrix proj;
	
	Vector3 pos;
	Vector3 dir;
	

	Vector3 lookAt;
};
class GlobalData
{
public:
	inline static const void SetWorldViewData(GlobalViewData* data)
	{
		memcpy(&viewData, data, sizeof(GlobalViewData));
		
	}
	inline static const void GetView( Matrix* const view) {	memcpy(view, &GlobalData::viewData.view, sizeof(Matrix));}
	inline static const void GetProj(Matrix* const proj) { memcpy(proj, &GlobalData::viewData.proj, sizeof(Matrix)); }
	inline static const void GetVP(Matrix* const vp) {memcpy(vp,&(GlobalData::viewData.view*GlobalData::viewData.proj),sizeof(Matrix));	}
	
	inline static const Vector3& Position() { return GlobalData::viewData.pos; }
	inline static const Vector3& Forward() { return GlobalData::viewData.dir; }
	inline static const Vector3& LookAt() { return GlobalData::viewData.lookAt; }
	

	inline static const Vector3& LightDirection() { return GlobalData::LightDir; }
//	inline static const Vector3& LightColor() 
//	{
//	
//auto LightCol= Vector3::Lerp(&GlobalData::yellow, &GlobalData::white, GlobalData::factor);
//
//		return GlobalData::LightCol;
//	}

	inline static const void LightDirection(const Vector3&LightDir) { GlobalData::LightDir=LightDir; }
	inline static const void LightColor(const float&factor) { GlobalData::factor =  factor; }

	static GlobalViewData viewData;


	static Vector3 yellow;
	static Vector3 white;
	static Vector3 LightDir;
	static Vector3 LightCol;
	static float factor;
};

