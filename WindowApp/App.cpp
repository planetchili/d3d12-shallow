#include "App.h"
#include <d3d12.h> 
#include <wrl.h> 
#include "GraphicsError.h"

namespace chil::app
{
	using Microsoft::WRL::ComPtr;

	int Run(win::IWindow& window)
	{
		ComPtr<ID3D12Device2> device;
		D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)) >> chk;

		return 0;
	}
}