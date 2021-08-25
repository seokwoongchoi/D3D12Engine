#include "Framework.h"
#include "SSAO.h"

SSAO::SSAO(ID3D12Device* device)
{
	CreateRootSignatures(device);
	CreatePipelineStates(device);
}

SSAO::~SSAO()
{
}

void SSAO::Compute(ID3D12GraphicsCommandList* commnadList, const D3D12_GPU_DESCRIPTOR_HANDLE& DepthSRV, const D3D12_GPU_DESCRIPTOR_HANDLE& NormalsSRV)
{
}

void SSAO::Resize(const uint& width, const uint& height)
{
}

void SSAO::DownscaleDepth(ID3D12GraphicsCommandList* commnadList, const D3D12_GPU_DESCRIPTOR_HANDLE& MiniDepth_UAV, const D3D12_GPU_DESCRIPTOR_HANDLE& DepthSRV, const D3D12_GPU_DESCRIPTOR_HANDLE& NormalsSRV)
{
}

void SSAO::ComputeSSAO(ID3D12GraphicsCommandList* commnadList, const D3D12_GPU_DESCRIPTOR_HANDLE& SSAO_UAV, const D3D12_GPU_DESCRIPTOR_HANDLE& MiniDepth_SRV)
{
}

void SSAO::CreateRootSignatures(ID3D12Device* pDevice)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Create a root signature for the downScale pass.
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		
		CD3DX12_ROOT_PARAMETER1 rootParameters[3]; // Performance tip: Order root parameters from most frequently accessed to least frequently accessed.
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0]); // 2 frequently changed diffuse + normal textures - starting in register t0.
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1]);
		rootParameters[2].InitAsConstants(32, 0);
		

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr);// ,
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | // Performance tip: Limit root signature access when possible.
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		Check(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		Check(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignatures[CSPass::DownScale])));
		
	}

	// Create a root signature for the SSAO pass.
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[3]; // Performance tip: Order root parameters from most frequently accessed to least frequently accessed.
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0]); // 2 frequently changed diffuse + normal textures - starting in register t0.
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1]);
		rootParameters[2].InitAsConstants(32, 0);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr);// ,
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | // Performance tip: Limit root signature access when possible.
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			//D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		Check(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		Check(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignatures[CSPass::SSAO])));

	}
}

void SSAO::CreatePipelineStates(ID3D12Device* pDevice)
{
	// Create the scene and shadow render pass pipeline state.
	{
		ComPtr<ID3DBlob> cs;

		cs = CompileShader(L"../_Shaders/PostEffects/SSAO.hlsl", nullptr, "DepthDownscale", "cs_5_0");

		D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
		computePsoDesc.pRootSignature = rootSignatures[CSPass::DownScale].Get();
		computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

		Check(pDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&pipelineStates[CSPass::DownScale])));
		NAME_D3D12_OBJECT(pipelineStates[CSPass::DownScale]);

	
	}

	// Create the scene and shadow render pass pipeline state.
	{
		ComPtr<ID3DBlob> cs;

		cs = CompileShader(L"../_Shaders/PostEffects/SSAO.hlsl", nullptr, "SSAOCompute", "cs_5_0");

		D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
		computePsoDesc.pRootSignature = rootSignatures[CSPass::SSAO].Get();
		computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

		Check(pDevice->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&pipelineStates[CSPass::SSAO])));
		NAME_D3D12_OBJECT(pipelineStates[CSPass::SSAO]);


	}
}
