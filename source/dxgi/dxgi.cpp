#include "log.hpp"
#include "hook_manager.hpp"
#include "dxgi_device.hpp"
#include "dxgi_swapchain.hpp"

void dump_swapchain_desc(const DXGI_SWAP_CHAIN_DESC &desc)
{
	LOG(TRACE) << "> Dumping swap chain description:";
	LOG(TRACE) << "  +-----------------------------------------+-----------------------------------------+";
	LOG(TRACE) << "  | Parameter                               | Value                                   |";
	LOG(TRACE) << "  +-----------------------------------------+-----------------------------------------+";
	LOG(TRACE) << "  | Width                                   | " << std::setw(39) << desc.BufferDesc.Width << " |";
	LOG(TRACE) << "  | Height                                  | " << std::setw(39) << desc.BufferDesc.Height << " |";
	LOG(TRACE) << "  | RefreshRate                             | " << std::setw(19) << desc.BufferDesc.RefreshRate.Numerator << ' ' << std::setw(19) << desc.BufferDesc.RefreshRate.Denominator << " |";
	LOG(TRACE) << "  | Format                                  | " << std::setw(39) << desc.BufferDesc.Format << " |";
	LOG(TRACE) << "  | ScanlineOrdering                        | " << std::setw(39) << desc.BufferDesc.ScanlineOrdering << " |";
	LOG(TRACE) << "  | Scaling                                 | " << std::setw(39) << desc.BufferDesc.Scaling << " |";
	LOG(TRACE) << "  | SampleCount                             | " << std::setw(39) << desc.SampleDesc.Count << " |";
	LOG(TRACE) << "  | SampleQuality                           | " << std::setw(39) << desc.SampleDesc.Quality << " |";
	LOG(TRACE) << "  | BufferUsage                             | " << std::setw(39) << desc.BufferUsage << " |";
	LOG(TRACE) << "  | BufferCount                             | " << std::setw(39) << desc.BufferCount << " |";
	LOG(TRACE) << "  | OutputWindow                            | " << std::setw(39) << desc.OutputWindow << " |";
	LOG(TRACE) << "  | Windowed                                | " << std::setw(39) << (desc.Windowed != FALSE ? "TRUE" : "FALSE") << " |";
	LOG(TRACE) << "  | SwapEffect                              | " << std::setw(39) << desc.SwapEffect << " |";
	LOG(TRACE) << "  | Flags                                   | " << std::setw(39) << std::hex << desc.Flags << std::dec << " |";
	LOG(TRACE) << "  +-----------------------------------------+-----------------------------------------+";

	if (desc.SampleDesc.Count > 1)
	{
		LOG(WARNING) << "> Multisampling is enabled. This is not compatible with depth buffer access, which was therefore disabled.";
	}
}
void dump_swapchain_desc(const DXGI_SWAP_CHAIN_DESC1 &desc)
{
	LOG(TRACE) << "> Dumping swap chain description:";
	LOG(TRACE) << "  +-----------------------------------------+-----------------------------------------+";
	LOG(TRACE) << "  | Parameter                               | Value                                   |";
	LOG(TRACE) << "  +-----------------------------------------+-----------------------------------------+";
	LOG(TRACE) << "  | Width                                   | " << std::setw(39) << desc.Width << " |";
	LOG(TRACE) << "  | Height                                  | " << std::setw(39) << desc.Height << " |";
	LOG(TRACE) << "  | Format                                  | " << std::setw(39) << desc.Format << " |";
	LOG(TRACE) << "  | Stereo                                  | " << std::setw(39) << (desc.Stereo != FALSE ? "TRUE" : "FALSE") << " |";
	LOG(TRACE) << "  | SampleCount                             | " << std::setw(39) << desc.SampleDesc.Count << " |";
	LOG(TRACE) << "  | SampleQuality                           | " << std::setw(39) << desc.SampleDesc.Quality << " |";
	LOG(TRACE) << "  | BufferUsage                             | " << std::setw(39) << desc.BufferUsage << " |";
	LOG(TRACE) << "  | BufferCount                             | " << std::setw(39) << desc.BufferCount << " |";
	LOG(TRACE) << "  | Scaling                                 | " << std::setw(39) << desc.Scaling << " |";
	LOG(TRACE) << "  | SwapEffect                              | " << std::setw(39) << desc.SwapEffect << " |";
	LOG(TRACE) << "  | AlphaMode                               | " << std::setw(39) << desc.AlphaMode << " |";
	LOG(TRACE) << "  | Flags                                   | " << std::setw(39) << std::hex << desc.Flags << std::dec << " |";
	LOG(TRACE) << "  +-----------------------------------------+-----------------------------------------+";

	if (desc.SampleDesc.Count > 1)
	{
		LOG(WARNING) << "> Multisampling is enabled. This is not compatible with depth buffer access, which was therefore disabled.";
	}
}

// IDXGIFactory
HRESULT STDMETHODCALLTYPE IDXGIFactory_CreateSwapChain(IDXGIFactory *pFactory, IUnknown *pDevice, DXGI_SWAP_CHAIN_DESC *pDesc, IDXGISwapChain **ppSwapChain)
{
	LOG(INFO) << "Redirecting '" << "IDXGIFactory::CreateSwapChain" << "(" << pFactory << ", " << pDevice << ", " << pDesc << ", " << ppSwapChain << ")' ...";

	IUnknown *device_orig = pDevice;
	D3D10Device *device_d3d10 = nullptr;
	D3D11Device *device_d3d11 = nullptr;
	D3D12CommandQueue *commandqueue_d3d12 = nullptr;

	if (pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr)
	{
		return DXGI_ERROR_INVALID_CALL;
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&device_d3d10)))
	{
		device_orig = device_d3d10->_orig;

		device_d3d10->Release();
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&device_d3d11)))
	{
		device_orig = device_d3d11->_orig;

		device_d3d11->Release();
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&commandqueue_d3d12)))
	{
		device_orig = commandqueue_d3d12->_orig;

		commandqueue_d3d12->Release();
	}

	dump_swapchain_desc(*pDesc);

	const HRESULT hr = reshade::hooks::call(&IDXGIFactory_CreateSwapChain)(pFactory, device_orig, pDesc, ppSwapChain);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'IDXGIFactory::CreateSwapChain' failed with error code " << std::hex << hr << std::dec << "!";

		return hr;
	}

	IDXGISwapChain *const swapchain = *ppSwapChain;

	DXGI_SWAP_CHAIN_DESC desc;
	swapchain->GetDesc(&desc);

	if ((desc.BufferUsage & DXGI_USAGE_RENDER_TARGET_OUTPUT) == 0)
	{
		LOG(WARNING) << "> Skipping swap chain due to missing 'DXGI_USAGE_RENDER_TARGET_OUTPUT' flag.";
	}
	else if (device_d3d10 != nullptr)
	{
		device_d3d10->AddRef();

		const auto runtime = std::make_shared<reshade::d3d10::d3d10_runtime>(device_d3d10->_orig, swapchain);

		if (!runtime->on_init(desc))
		{
			LOG(ERROR) << "Failed to initialize Direct3D 10 runtime environment on runtime " << runtime.get() << ".";
		}

		device_d3d10->_runtimes.push_back(runtime);

		*ppSwapChain = new DXGISwapChain(device_d3d10, swapchain, runtime);
	}
	else if (device_d3d11 != nullptr)
	{
		device_d3d11->AddRef();

		const auto runtime = std::make_shared<reshade::d3d11::d3d11_runtime>(device_d3d11->_orig, swapchain);

		if (!runtime->on_init(desc))
		{
			LOG(ERROR) << "Failed to initialize Direct3D 11 runtime environment on runtime " << runtime.get() << ".";
		}

		device_d3d11->_runtimes.push_back(runtime);

		*ppSwapChain = new DXGISwapChain(device_d3d11, swapchain, runtime);
	}
	else if (commandqueue_d3d12 != nullptr)
	{
		IDXGISwapChain3 *swapchain3 = nullptr;

		if (SUCCEEDED(swapchain->QueryInterface(&swapchain3)))
		{
			commandqueue_d3d12->AddRef();

			const auto runtime = std::make_shared<reshade::d3d12::d3d12_runtime>(commandqueue_d3d12->_device->_orig, commandqueue_d3d12->_orig, swapchain3);

			if (!runtime->on_init(desc))
			{
				LOG(ERROR) << "Failed to initialize Direct3D 12 runtime environment on runtime " << runtime.get() << ".";
			}

			*ppSwapChain = new DXGISwapChain(commandqueue_d3d12, swapchain3, runtime);

			swapchain3->Release();
		}
		else
		{
			LOG(WARNING) << "> Skipping swap chain because it is missing support for the 'IDXGISwapChain3' interface.";
		}
	}
	else
	{
		LOG(WARNING) << "> Skipping swap chain because it was created without a (hooked) Direct3D device.";
	}

	LOG(TRACE) << "> Returned swap chain object: " << *ppSwapChain;

	return S_OK;
}

// IDXGIFactory2
HRESULT STDMETHODCALLTYPE IDXGIFactory2_CreateSwapChainForHwnd(IDXGIFactory2 *pFactory, IUnknown *pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1 *pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain)
{
	LOG(INFO) << "Redirecting '" << "IDXGIFactory2::CreateSwapChainForHwnd" << "(" << pFactory << ", " << pDevice << ", " << hWnd << ", " << pDesc << ", " << pFullscreenDesc << ", " << pRestrictToOutput << ", " << ppSwapChain << ")' ...";

	IUnknown *device_orig = pDevice;
	D3D10Device *device_d3d10 = nullptr;
	D3D11Device *device_d3d11 = nullptr;
	D3D12CommandQueue *commandqueue_d3d12 = nullptr;

	if (pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr)
	{
		return DXGI_ERROR_INVALID_CALL;
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&device_d3d10)))
	{
		device_orig = device_d3d10->_orig;

		device_d3d10->Release();
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&device_d3d11)))
	{
		device_orig = device_d3d11->_orig;

		device_d3d11->Release();
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&commandqueue_d3d12)))
	{
		device_orig = commandqueue_d3d12->_orig;

		commandqueue_d3d12->Release();
	}

	dump_swapchain_desc(*pDesc);

	const HRESULT hr = reshade::hooks::call(&IDXGIFactory2_CreateSwapChainForHwnd)(pFactory, device_orig, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'IDXGIFactory2::CreateSwapChainForHwnd' failed with error code " << std::hex << hr << std::dec << "!";

		return hr;
	}

	IDXGISwapChain1 *const swapchain = *ppSwapChain;

	DXGI_SWAP_CHAIN_DESC desc;
	swapchain->GetDesc(&desc);

	if ((desc.BufferUsage & DXGI_USAGE_RENDER_TARGET_OUTPUT) == 0)
	{
		LOG(WARNING) << "> Skipping swap chain due to missing 'DXGI_USAGE_RENDER_TARGET_OUTPUT' flag.";
	}
	else if (device_d3d10 != nullptr)
	{
		device_d3d10->AddRef();

		const auto runtime = std::make_shared<reshade::d3d10::d3d10_runtime>(device_d3d10->_orig, swapchain);

		if (!runtime->on_init(desc))
		{
			LOG(ERROR) << "Failed to initialize Direct3D 10 runtime environment on runtime " << runtime.get() << ".";
		}

		device_d3d10->_runtimes.push_back(runtime);

		*ppSwapChain = new DXGISwapChain(device_d3d10, swapchain, runtime);
	}
	else if (device_d3d11 != nullptr)
	{
		device_d3d11->AddRef();

		const auto runtime = std::make_shared<reshade::d3d11::d3d11_runtime>(device_d3d11->_orig, swapchain);

		if (!runtime->on_init(desc))
		{
			LOG(ERROR) << "Failed to initialize Direct3D 11 runtime environment on runtime " << runtime.get() << ".";
		}

		device_d3d11->_runtimes.push_back(runtime);

		*ppSwapChain = new DXGISwapChain(device_d3d11, swapchain, runtime);
	}
	else if (commandqueue_d3d12 != nullptr)
	{
		IDXGISwapChain3 *swapchain3 = nullptr;

		if (SUCCEEDED(swapchain->QueryInterface(&swapchain3)))
		{
			commandqueue_d3d12->AddRef();

			const auto runtime = std::make_shared<reshade::d3d12::d3d12_runtime>(commandqueue_d3d12->_device->_orig, commandqueue_d3d12->_orig, swapchain3);

			if (!runtime->on_init(desc))
			{
				LOG(ERROR) << "Failed to initialize Direct3D 12 runtime environment on runtime " << runtime.get() << ".";
			}

			*ppSwapChain = new DXGISwapChain(commandqueue_d3d12, swapchain3, runtime);

			swapchain3->Release();
		}
		else
		{
			LOG(WARNING) << "> Skipping swap chain because it is missing support for the 'IDXGISwapChain3' interface.";
		}
	}
	else
	{
		LOG(WARNING) << "> Skipping swap chain because it was created without a (hooked) Direct3D device.";
	}

	LOG(TRACE) << "> Returned swap chain object: " << *ppSwapChain;

	return S_OK;
}
HRESULT STDMETHODCALLTYPE IDXGIFactory2_CreateSwapChainForCoreWindow(IDXGIFactory2 *pFactory, IUnknown *pDevice, IUnknown *pWindow, const DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain)
{
	LOG(INFO) << "Redirecting '" << "IDXGIFactory2::CreateSwapChainForCoreWindow" << "(" << pFactory << ", " << pDevice << ", " << pWindow << ", " << pDesc << ", " << pRestrictToOutput << ", " << ppSwapChain << ")' ...";

	IUnknown *device_orig = pDevice;
	D3D10Device *device_d3d10 = nullptr;
	D3D11Device *device_d3d11 = nullptr;
	D3D12CommandQueue *commandqueue_d3d12 = nullptr;

	if (pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr)
	{
		return DXGI_ERROR_INVALID_CALL;
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&device_d3d10)))
	{
		device_orig = device_d3d10->_orig;

		device_d3d10->Release();
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&device_d3d11)))
	{
		device_orig = device_d3d11->_orig;

		device_d3d11->Release();
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&commandqueue_d3d12)))
	{
		device_orig = commandqueue_d3d12->_orig;

		commandqueue_d3d12->Release();
	}

	dump_swapchain_desc(*pDesc);

	const HRESULT hr = reshade::hooks::call(&IDXGIFactory2_CreateSwapChainForCoreWindow)(pFactory, device_orig, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'IDXGIFactory2::CreateSwapChainForCoreWindow' failed with error code " << std::hex << hr << std::dec << "!";

		return hr;
	}

	IDXGISwapChain1 *const swapchain = *ppSwapChain;

	DXGI_SWAP_CHAIN_DESC desc;
	swapchain->GetDesc(&desc);

	if ((desc.BufferUsage & DXGI_USAGE_RENDER_TARGET_OUTPUT) == 0)
	{
		LOG(WARNING) << "> Skipping swap chain due to missing 'DXGI_USAGE_RENDER_TARGET_OUTPUT' flag.";
	}
	else if (device_d3d10 != nullptr)
	{
		device_d3d10->AddRef();

		const auto runtime = std::make_shared<reshade::d3d10::d3d10_runtime>(device_d3d10->_orig, swapchain);

		if (!runtime->on_init(desc))
		{
			LOG(ERROR) << "Failed to initialize Direct3D 10 runtime environment on runtime " << runtime.get() << ".";
		}

		device_d3d10->_runtimes.push_back(runtime);

		*ppSwapChain = new DXGISwapChain(device_d3d10, swapchain, runtime);
	}
	else if (device_d3d11 != nullptr)
	{
		device_d3d11->AddRef();

		const auto runtime = std::make_shared<reshade::d3d11::d3d11_runtime>(device_d3d11->_orig, swapchain);

		if (!runtime->on_init(desc))
		{
			LOG(ERROR) << "Failed to initialize Direct3D 11 runtime environment on runtime " << runtime.get() << ".";
		}

		device_d3d11->_runtimes.push_back(runtime);

		*ppSwapChain = new DXGISwapChain(device_d3d11, swapchain, runtime);
	}
	else if (commandqueue_d3d12 != nullptr)
	{
		IDXGISwapChain3 *swapchain3 = nullptr;

		if (SUCCEEDED(swapchain->QueryInterface(&swapchain3)))
		{
			commandqueue_d3d12->AddRef();

			const auto runtime = std::make_shared<reshade::d3d12::d3d12_runtime>(commandqueue_d3d12->_device->_orig, commandqueue_d3d12->_orig, swapchain3);

			if (!runtime->on_init(desc))
			{
				LOG(ERROR) << "Failed to initialize Direct3D 12 runtime environment on runtime " << runtime.get() << ".";
			}

			*ppSwapChain = new DXGISwapChain(commandqueue_d3d12, swapchain3, runtime);

			swapchain3->Release();
		}
		else
		{
			LOG(WARNING) << "> Skipping swap chain because it is missing support for the 'IDXGISwapChain3' interface.";
		}
	}
	else
	{
		LOG(WARNING) << "> Skipping swap chain because it was created without a (hooked) Direct3D device.";
	}

	LOG(TRACE) << "> Returned swap chain object: " << *ppSwapChain;

	return S_OK;
}
HRESULT STDMETHODCALLTYPE IDXGIFactory2_CreateSwapChainForComposition(IDXGIFactory2 *pFactory, IUnknown *pDevice, const DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain)
{
	LOG(INFO) << "Redirecting '" << "IDXGIFactory2::CreateSwapChainForComposition" << "(" << pFactory << ", " << pDevice << ", " << pDesc << ", " << pRestrictToOutput << ", " << ppSwapChain << ")' ...";

	IUnknown *device_orig = pDevice;
	D3D10Device *device_d3d10 = nullptr;
	D3D11Device *device_d3d11 = nullptr;
	D3D12CommandQueue *commandqueue_d3d12 = nullptr;

	if (pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr)
	{
		return DXGI_ERROR_INVALID_CALL;
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&device_d3d10)))
	{
		device_orig = device_d3d10->_orig;

		device_d3d10->Release();
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&device_d3d11)))
	{
		device_orig = device_d3d11->_orig;

		device_d3d11->Release();
	}
	else if (SUCCEEDED(pDevice->QueryInterface(&commandqueue_d3d12)))
	{
		device_orig = commandqueue_d3d12->_orig;

		commandqueue_d3d12->Release();
	}

	dump_swapchain_desc(*pDesc);

	const HRESULT hr = reshade::hooks::call(&IDXGIFactory2_CreateSwapChainForComposition)(pFactory, device_orig, pDesc, pRestrictToOutput, ppSwapChain);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'IDXGIFactory2::CreateSwapChainForComposition' failed with error code " << std::hex << hr << std::dec << "!";

		return hr;
	}

	IDXGISwapChain1 *const swapchain = *ppSwapChain;

	DXGI_SWAP_CHAIN_DESC desc;
	swapchain->GetDesc(&desc);

	if ((desc.BufferUsage & DXGI_USAGE_RENDER_TARGET_OUTPUT) == 0)
	{
		LOG(WARNING) << "> Skipping swap chain due to missing 'DXGI_USAGE_RENDER_TARGET_OUTPUT' flag.";
	}
	else if (device_d3d10 != nullptr)
	{
		device_d3d10->AddRef();

		const auto runtime = std::make_shared<reshade::d3d10::d3d10_runtime>(device_d3d10->_orig, swapchain);

		if (!runtime->on_init(desc))
		{
			LOG(ERROR) << "Failed to initialize Direct3D 10 runtime environment on runtime " << runtime.get() << ".";
		}

		device_d3d10->_runtimes.push_back(runtime);

		*ppSwapChain = new DXGISwapChain(device_d3d10, swapchain, runtime);
	}
	else if (device_d3d11 != nullptr)
	{
		device_d3d11->AddRef();

		const auto runtime = std::make_shared<reshade::d3d11::d3d11_runtime>(device_d3d11->_orig, swapchain);

		if (!runtime->on_init(desc))
		{
			LOG(ERROR) << "Failed to initialize Direct3D 11 runtime environment on runtime " << runtime.get() << ".";
		}

		device_d3d11->_runtimes.push_back(runtime);

		*ppSwapChain = new DXGISwapChain(device_d3d11, swapchain, runtime);
	}
	else if (commandqueue_d3d12 != nullptr)
	{
		IDXGISwapChain3 *swapchain3 = nullptr;

		if (SUCCEEDED(swapchain->QueryInterface(&swapchain3)))
		{
			commandqueue_d3d12->AddRef();

			const auto runtime = std::make_shared<reshade::d3d12::d3d12_runtime>(commandqueue_d3d12->_device->_orig, commandqueue_d3d12->_orig, swapchain3);

			if (!runtime->on_init(desc))
			{
				LOG(ERROR) << "Failed to initialize Direct3D 12 runtime environment on runtime " << runtime.get() << ".";
			}

			*ppSwapChain = new DXGISwapChain(commandqueue_d3d12, swapchain3, runtime);

			swapchain3->Release();
		}
		else
		{
			LOG(WARNING) << "> Skipping swap chain because it is missing support for the 'IDXGISwapChain3' interface.";
		}
	}
	else
	{
		LOG(WARNING) << "> Skipping swap chain because it was created without a (hooked) Direct3D device.";
	}

	LOG(TRACE) << "> Returned swap chain object: " << *ppSwapChain;

	return S_OK;
}

// DXGI
HOOK_EXPORT HRESULT WINAPI DXGIDumpJournal()
{
	assert(false);

	return E_NOTIMPL;
}
HOOK_EXPORT HRESULT WINAPI DXGIReportAdapterConfiguration()
{
	assert(false);

	return E_NOTIMPL;
}

HOOK_EXPORT HRESULT WINAPI DXGID3D10CreateDevice(HMODULE hModule, IDXGIFactory *pFactory, IDXGIAdapter *pAdapter, UINT Flags, void *pUnknown, void **ppDevice)
{
	return reshade::hooks::call(&DXGID3D10CreateDevice)(hModule, pFactory, pAdapter, Flags, pUnknown, ppDevice);
}
HOOK_EXPORT HRESULT WINAPI DXGID3D10CreateLayeredDevice(void *pUnknown1, void *pUnknown2, void *pUnknown3, void *pUnknown4, void *pUnknown5)
{
	return reshade::hooks::call(&DXGID3D10CreateLayeredDevice)(pUnknown1, pUnknown2, pUnknown3, pUnknown4, pUnknown5);
}
HOOK_EXPORT SIZE_T WINAPI DXGID3D10GetLayeredDeviceSize(const void *pLayers, UINT NumLayers)
{
	return reshade::hooks::call(&DXGID3D10GetLayeredDeviceSize)(pLayers, NumLayers);
}
HOOK_EXPORT HRESULT WINAPI DXGID3D10RegisterLayers(const void *pLayers, UINT NumLayers)
{
	return reshade::hooks::call(&DXGID3D10RegisterLayers)(pLayers, NumLayers);
}

HOOK_EXPORT HRESULT WINAPI CreateDXGIFactory(REFIID riid, void **ppFactory)
{
	OLECHAR riidString[40];
	StringFromGUID2(riid, riidString, ARRAYSIZE(riidString));

	LOG(INFO) << "Redirecting '" << "CreateDXGIFactory" << "(" << riidString << ", " << ppFactory << ")' ...";

	const HRESULT hr = reshade::hooks::call(&CreateDXGIFactory)(riid, ppFactory);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'CreateDXGIFactory' failed with error code " << std::hex << hr << std::dec << "!";

		return hr;
	}

	IDXGIFactory *const factory = static_cast<IDXGIFactory *>(*ppFactory);
	IDXGIFactory2 *factory2 = nullptr;

	reshade::hooks::install(VTABLE(factory), 10, reinterpret_cast<reshade::hook::address>(&IDXGIFactory_CreateSwapChain));

	if (SUCCEEDED(factory->QueryInterface(&factory2)))
	{
		reshade::hooks::install(VTABLE(factory2), 15, reinterpret_cast<reshade::hook::address>(&IDXGIFactory2_CreateSwapChainForHwnd));
		reshade::hooks::install(VTABLE(factory2), 16, reinterpret_cast<reshade::hook::address>(&IDXGIFactory2_CreateSwapChainForCoreWindow));
		reshade::hooks::install(VTABLE(factory2), 24, reinterpret_cast<reshade::hook::address>(&IDXGIFactory2_CreateSwapChainForComposition));

		factory2->Release();
	}

	LOG(TRACE) << "> Returned factory object: " << *ppFactory;

	return S_OK;
}
HOOK_EXPORT HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void **ppFactory)
{
	OLECHAR riidString[40];
	StringFromGUID2(riid, riidString, ARRAYSIZE(riidString));

	LOG(INFO) << "Redirecting '" << "CreateDXGIFactory1" << "(" << riidString << ", " << ppFactory << ")' ...";

	const HRESULT hr = reshade::hooks::call(&CreateDXGIFactory1)(riid, ppFactory);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'CreateDXGIFactory1' failed with error code " << std::hex << hr << std::dec << "!";

		return hr;
	}

	IDXGIFactory *const factory = static_cast<IDXGIFactory *>(*ppFactory);
	IDXGIFactory2 *factory2 = nullptr;

	reshade::hooks::install(VTABLE(factory), 10, reinterpret_cast<reshade::hook::address>(&IDXGIFactory_CreateSwapChain));

	if (SUCCEEDED(factory->QueryInterface(&factory2)))
	{
		reshade::hooks::install(VTABLE(factory2), 15, reinterpret_cast<reshade::hook::address>(&IDXGIFactory2_CreateSwapChainForHwnd));
		reshade::hooks::install(VTABLE(factory2), 16, reinterpret_cast<reshade::hook::address>(&IDXGIFactory2_CreateSwapChainForCoreWindow));
		reshade::hooks::install(VTABLE(factory2), 24, reinterpret_cast<reshade::hook::address>(&IDXGIFactory2_CreateSwapChainForComposition));

		factory2->Release();
	}

	LOG(TRACE) << "> Returned factory object: " << *ppFactory;

	return S_OK;
}
HOOK_EXPORT HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID riid, void **ppFactory)
{
	OLECHAR riidString[40];
	StringFromGUID2(riid, riidString, ARRAYSIZE(riidString));

	LOG(INFO) << "Redirecting '" << "CreateDXGIFactory2" << "(" << flags << ", " << riidString << ", " << ppFactory << ")' ...";

#ifdef _DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	const HRESULT hr = reshade::hooks::call(&CreateDXGIFactory2)(flags, riid, ppFactory);

	if (FAILED(hr))
	{
		LOG(WARNING) << "> 'CreateDXGIFactory2' failed with error code " << std::hex << hr << std::dec << "!";

		return hr;
	}

	IDXGIFactory *const factory = static_cast<IDXGIFactory *>(*ppFactory);
	IDXGIFactory2 *factory2 = nullptr;

	reshade::hooks::install(VTABLE(factory), 10, reinterpret_cast<reshade::hook::address>(&IDXGIFactory_CreateSwapChain));

	if (SUCCEEDED(factory->QueryInterface(&factory2)))
	{
		reshade::hooks::install(VTABLE(factory2), 15, reinterpret_cast<reshade::hook::address>(&IDXGIFactory2_CreateSwapChainForHwnd));
		reshade::hooks::install(VTABLE(factory2), 16, reinterpret_cast<reshade::hook::address>(&IDXGIFactory2_CreateSwapChainForCoreWindow));
		reshade::hooks::install(VTABLE(factory2), 24, reinterpret_cast<reshade::hook::address>(&IDXGIFactory2_CreateSwapChainForComposition));

		factory2->Release();
	}

	LOG(TRACE) << "> Returned factory object: " << *ppFactory;

	return S_OK;
}
