#pragma once
#include "Logger.h"

namespace Magpie {

// Each instance belongs to one immediate context. Query operations must stay on
// that context's owning thread, including when it is SINGLETHREADED.
class GpuCompletion {
public:
	bool Initialize(ID3D11Device5* device) noexcept {
		HRESULT hr = device->CreateFence(0, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
		if (SUCCEEDED(hr)) {
			if (!_event.try_create(wil::EventOptions::None, nullptr)) {
				Logger::Get().Win32Error("CreateEvent 失败");
				return false;
			}
			return true;
		}

		// VMware SVGA can expose ID3D11Device5 while rejecting CreateFence.
		Logger::Get().ComWarn("CreateFence 不可用，回退到 D3D11_QUERY_EVENT", hr);
		const D3D11_QUERY_DESC desc{ .Query = D3D11_QUERY_EVENT };
		hr = device->CreateQuery(&desc, _query.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateQuery(D3D11_QUERY_EVENT) 失败", hr);
			return false;
		}
		return true;
	}

	bool UsesQuery() const noexcept {
		return bool(_query);
	}

	bool Signal(ID3D11DeviceContext4* context) noexcept {
		if (_query) {
			context->End(_query.get());
		} else {
			HRESULT hr = context->Signal(_fence.get(), ++_value);
			if (FAILED(hr)) {
				Logger::Get().ComError("Signal 失败", hr);
				return false;
			}
			hr = _fence->SetEventOnCompletion(_value, _event.get());
			if (FAILED(hr)) {
				Logger::Get().ComError("SetEventOnCompletion 失败", hr);
				return false;
			}
		}
		context->Flush();
		return true;
	}

	bool Wait(ID3D11DeviceContext4* context, DWORD timeout = 5000) noexcept {
		if (!_query) {
			if (WaitForSingleObject(_event.get(), timeout) == WAIT_OBJECT_0) {
				return true;
			}
			Logger::Get().Error("等待 GPU Fence 失败或超时");
			return false;
		}

		const ULONGLONG start = GetTickCount64();
		for (;;) {
			BOOL completed = FALSE;
			HRESULT hr = context->GetData(_query.get(), &completed, sizeof(completed),
				D3D11_ASYNC_GETDATA_DONOTFLUSH);
			if (hr == S_OK && completed) {
				return true;
			}
			if (FAILED(hr)) {
				Logger::Get().ComError("GetData(D3D11_QUERY_EVENT) 失败", hr);
				return false;
			}
			if (GetTickCount64() - start >= timeout) {
				Logger::Get().Error("等待 GPU 事件查询超时");
				return false;
			}
			Sleep(1);
		}
	}

private:
	winrt::com_ptr<ID3D11Fence> _fence;
	winrt::com_ptr<ID3D11Query> _query;
	uint64_t _value = 0;
	wil::unique_event_nothrow _event;
};

}
