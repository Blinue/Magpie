#include "pch.h"
#include "MagRuntime.h"
#if __has_include("MagRuntime.g.cpp")
#include "MagRuntime.g.cpp"
#endif
#include "MagApp.h"
#include "Logger.h"
#include <dispatcherqueue.h>


namespace winrt::Magpie::Runtime::implementation {

MagRuntime::MagRuntime(uint64_t pLogger) {
	Logger::Get().Initialize(*(Logger*)pLogger);
}

void MagRuntime::Run(uint64_t hwndSrc, MagSettings const& settings) {
	if (_running) {
		return;
	}

	_running = true;
	if (_magWindThread.joinable()) {
		_magWindThread.join();
	}
	_magWindThread = std::thread([=, this]() {
		winrt::init_apartment(winrt::apartment_type::multi_threaded);

		DispatcherQueueOptions options{};
		options.dwSize = sizeof(options);
		options.threadType = DQTYPE_THREAD_CURRENT;
		options.apartmentType = DQTAT_COM_NONE;

		HRESULT hr = CreateDispatcherQueueController(options, (ABI::Windows::System::IDispatcherQueueController**)put_abi(_dqc));
		if (FAILED(hr)) {
			_running = false;
			return;
		}

		MagApp& app = MagApp::Get();
		app.Run((HWND)hwndSrc, settings);

		_running = false;
		_dqc = nullptr;
	});
}

void MagRuntime::Stop() {
	if (!_running || !_dqc) {
		return;
	}

	_dqc.DispatcherQueue().TryEnqueue([]() {
		MagApp::Get().Stop();
	});

	if (_magWindThread.joinable()) {
		_magWindThread.join();
	}
}

}
