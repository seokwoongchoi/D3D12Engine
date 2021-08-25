#pragma once


#define terrain_gridpoints					512
#define terrain_numpatches_1d				64
#define terrain_geometry_scale				1.0f
#define terrain_maxheight					30.0f 
#define terrain_minheight					-20.0f 
#define terrain_fractalfactor				0.68f;
#define terrain_fractalinitialvalue			100.0f
#define terrain_smoothfactor1				0.99f
#define terrain_smoothfactor2				0.10f
#define terrain_rockfactor					0.95f
#define terrain_smoothsteps					40

#define terrain_height_underwater_start		-100.0f
#define terrain_height_underwater_end		-8.0f
#define terrain_height_sand_start			-30.0f
#define terrain_height_sand_end				1.7f
#define terrain_height_grass_start			1.7f
#define terrain_height_grass_end			30.0f
#define terrain_height_rocks_start			-2.0f
#define terrain_height_trees_start			4.0f
#define terrain_height_trees_end			30.0f
#define terrain_slope_grass_start			0.96f
#define terrain_slope_rocks_start			0.85f

#define terrain_far_range terrain_gridpoints*terrain_geometry_scale


#define water_normalmap_resource_buffer_size_xy			2048
#define terrain_layerdef_map_texture_size				1024
#define terrain_depth_shadow_map_texture_size			512

#define main_buffer_size_multiplier			1.0f
#define reflection_buffer_size_multiplier   1.0f
#define refraction_buffer_size_multiplier   1.0f
namespace Island11Enums
{
	namespace RenderPass {
		enum Value { Terrain = 0,  Count };
	}
	
	
}
struct TerrainConstantBuffer
{
	
	Matrix view;
	Matrix projection;
	
};


struct VertexIsland
{
	Vector2 origin;
	Vector2 size;
};
class Island11
{
public:
	explicit Island11(ID3D12Device* device);
	~Island11();
	Island11(const Island11&) = delete;
	Island11& operator=(const Island11&) = delete;

public:
	void Test(uint threadindex);
	uint data = 0;
	/*inline ID3D12PipelineState* GetTerrainPSO()
	{
		m_pipelineState[Island11Enums::RenderPass::Terrain].Get();
	}*/
	ComPtr<ID3D12RootSignature> m_rootSignatures[Island11Enums::RenderPass::Count];
	ComPtr<ID3D12PipelineState> m_pipelineStates[Island11Enums::RenderPass::Count];
//private:
	void CreateRootSignatures(ID3D12Device* pDevice);
	void CreatePipelineStates(ID3D12Device* pDevice);
	void CreateVertexBuffer(ID3D12Device* pDevice);
};

//float bilinear_interpolation(float fx, float fy, float a, float b, float c, float d);

