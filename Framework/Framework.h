#pragma once


#ifdef _DEBUG
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif

#include <Windows.h>
#include <wrl.h>
#include <assert.h>

//STL
#include <fstream>      // std::ifstream
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <functional>
#include <iterator>
#include <thread>
#include <mutex>
#include <queue>
#include <algorithm>
#include <chrono>
#include <utility>
#include <process.h>
#include <future>

using namespace std;
using namespace Microsoft::WRL;

//Direct3D
#include <d3dcompiler.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <shellapi.h>
#pragma comment(lib,"D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include <pix3.h>


#define Check(hr) { assert(SUCCEEDED(hr)); }


#define SafeRelease(p){ if(p){ (p)->Release(); (p) = NULL; } }
#define SafeDelete(p){ if(p){ delete (p); (p) = NULL; } }
#define SafeDeleteArray(p){ if(p){ delete [] (p); (p) = NULL; } }
typedef unsigned int uint;
typedef unsigned long ulong;


//Math

#include "./Math/Math.h"
#include "./Math/Vector2.h"
#include "./Math/Vector3.h"
#include "./Math/Vector4.h"
#include "./Math/Color4.h"
#include "./Math/Quaternion.h"
#include "./Math/Matrix.h"


//Global Data
#include "GlobalDatas/GlobalData.h"

//Utillity
#include "Utility/BinaryFile.h"
#include "Utility/Path.h"
#include "Utility/String.h"

//Mainsystem
#include "Core/D3D12/d3dx12.h"
#include "Core/D3D12/D3D.h"
#include "Core/D3D12/D3D12_Helper.h"




//SubSystems
#include "Systems/Keyboard.h"
#include "Systems/Mouse.h"
#include "Systems/Time.h"
#include "Systems/Thread.h"
//Viewer
//#include "Viewer/Viewport.h"
#include "Viewer/Camera.h"

