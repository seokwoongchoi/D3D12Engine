#pragma once


struct D3DDesc
{
	wstring AppName;
	HINSTANCE Instance;
	HWND Handle;
	uint Width;
	uint Height;
	bool bVsync;
	bool bFullScreen;

};


class D3D
{

public:
	 static const D3DDesc& GetDesc()	{return d3dDesc;}
	 static void SetDesc(const D3DDesc& desc){	d3dDesc = desc;	}
	 static const uint& Width() { return d3dDesc.Width; }
	 static const uint& Height() { return d3dDesc.Height; }

	 static void Width(const uint& width) { d3dDesc.Width= width; }
	 static void Height(const uint& height) {  d3dDesc.Height=height; }
	
public:
	

	
	
	static D3DDesc d3dDesc;
	

	
};


