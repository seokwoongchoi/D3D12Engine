#include "Framework.h"
#include "Island11_HLSL.h"

using namespace Island11Enums;
Island11::Island11(ID3D12Device* device)
{
	CreateRootSignatures(device);
	CreatePipelineStates(device);
}

Island11::~Island11()
{
}

void Island11::Test(uint threadindex)
{
	//cout << to_string(threadindex) << endl;
	data = threadindex;
}

void Island11::CreateRootSignatures(ID3D12Device* device)
{

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Create a root signature for the shadow pass.
	{
		CD3DX12_ROOT_PARAMETER1 rootParameters[1]; // Performance tip: Order root parameters from most frequently accessed to least frequently accessed.
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_DOMAIN); // 1 frequently changed constant buffer.

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS | // Performance tip: Limit root signature access when possible.
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);
			

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		Check(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		Check(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignatures[RenderPass::Terrain])));
		NAME_D3D12_OBJECT(m_rootSignatures[RenderPass::Terrain]);
	}

	
}

void Island11::CreatePipelineStates(ID3D12Device* device)
{

	// Create the scene and shadow render pass pipeline state.
	{

		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> hullShader;
		ComPtr<ID3DBlob> domainShader;
		ComPtr<ID3DBlob> pixelShader;
		vertexShader = CompileShader(L"../_Shaders/Environment/Island11.hlsl", nullptr, "PassThroughVS", "vs_5_0");
		hullShader = CompileShader(L"../_Shaders/Environment/Island11.hlsl", nullptr, "PatchHS", "hs_5_0");
		domainShader = CompileShader(L"../_Shaders/Environment/Island11.hlsl", nullptr, "HeightFieldPatchDS", "ds_5_0");
		pixelShader = CompileShader(L"../_Shaders/Environment/Island11.hlsl", nullptr, "HeightFieldPatchPacking", "ps_5_0");

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "PATCH_PARAMETERS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		
		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilDesc.StencilEnable = TRUE;
		depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		const D3D12_DEPTH_STENCILOP_DESC stencilMarkOp = { D3D12_STENCIL_OP_REPLACE, D3D12_STENCIL_OP_REPLACE, D3D12_STENCIL_OP_REPLACE, D3D12_COMPARISON_FUNC_ALWAYS };
		depthStencilDesc.FrontFace = stencilMarkOp;
		depthStencilDesc.BackFace = stencilMarkOp;
		// Describe and create the PSO for rendering the scene.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.HS = CD3DX12_SHADER_BYTECODE(hullShader.Get());
		psoDesc.DS = CD3DX12_SHADER_BYTECODE(domainShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignatures[RenderPass::Terrain].Get();
	
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		psoDesc.NumRenderTargets = 3;
		

		psoDesc.RTVFormats[0] = (DXGI_FORMAT_R10G10B10A2_UNORM);
		psoDesc.RTVFormats[1] = (DXGI_FORMAT_R10G10B10A2_UNORM);
		psoDesc.RTVFormats[2] = (DXGI_FORMAT_R16G16_UNORM);



		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;
		Check(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStates[RenderPass::Terrain])));
		
		NAME_D3D12_OBJECT(m_pipelineStates[RenderPass::Terrain]);
	}





}

void Island11::CreateVertexBuffer(ID3D12Device * pDevice)
{
	float* patches_rawdata = new float[terrain_numpatches_1d*terrain_numpatches_1d * 4];
	// creating terrain vertex buffer
	for (int i = 0; i < terrain_numpatches_1d; i++)
		for (int j = 0; j < terrain_numpatches_1d; j++)
		{
			patches_rawdata[(i + j * terrain_numpatches_1d) * 4 + 0] = i * terrain_geometry_scale*terrain_gridpoints / terrain_numpatches_1d;
			patches_rawdata[(i + j * terrain_numpatches_1d) * 4 + 1] = j * terrain_geometry_scale*terrain_gridpoints / terrain_numpatches_1d;
			patches_rawdata[(i + j * terrain_numpatches_1d) * 4 + 2] = terrain_geometry_scale * terrain_gridpoints / terrain_numpatches_1d;
			patches_rawdata[(i + j * terrain_numpatches_1d) * 4 + 3] = terrain_geometry_scale * terrain_gridpoints / terrain_numpatches_1d;
		}

	D3D11_BUFFER_DESC buf_desc;
	memset(&buf_desc, 0, sizeof(buf_desc));

	buf_desc.ByteWidth = terrain_numpatches_1d * terrain_numpatches_1d * 4 * sizeof(float);
	buf_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buf_desc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA subresource_data;
	ZeroMemory(&subresource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	subresource_data.pSysMem = patches_rawdata;
	subresource_data.SysMemPitch = 0;
	subresource_data.SysMemSlicePitch = 0;

	Check(device->CreateBuffer(&buf_desc, &subresource_data, &heightfield_vertexbuffer));
	SafeDeleteArray(patches_rawdata);
}
