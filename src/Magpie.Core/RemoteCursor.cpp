#include "pch.h"
#include "RemoteCursor.h"
#include "Logger.h"

namespace Magpie {

RemoteCursor::RemoteCursor() noexcept {
	if (FAILED(_updateEvent.create(wil::EventOptions::None))) {
		Logger::Get().Win32Error("创建远程光标事件失败");
		return;
	}
	_thread = std::thread(&RemoteCursor::_ThreadProc, this);
}

RemoteCursor::~RemoteCursor() noexcept {
	if (!_thread.joinable()) {
		return;
	}
	{
		std::lock_guard lock(_mutex);
		_stopping = true;
	}
	_updateEvent.SetEvent();
	_thread.join();
}

void RemoteCursor::Update(DWORD threadId, CURSORINFO* cursorInfo) noexcept {
	if (!_thread.joinable()) {
		return;
	}
	{
		std::lock_guard lock(_mutex);
		_requestedThreadId = threadId;
		if (threadId && threadId == _attachedThreadId && cursorInfo &&
			!(cursorInfo->flags & CURSOR_SUPPRESSED)) {
			// ShowCursor 隐藏后 GetCursorInfo 的 hCursor 也可能为 NULL。
			// 显隐和形状都必须从附加的输入队列读取，不能只修正 flags。
			cursorInfo->hCursor = _hCursor;
			cursorInfo->flags = _isCursorShowing && _hCursor ? CURSOR_SHOWING : 0;
		}
	}
	_updateEvent.SetEvent();
}

void RemoteCursor::_ThreadProc() noexcept {
	// AttachThreadInput 要求双方都已有消息队列。
	MSG msg;
	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
	const DWORD threadId = GetCurrentThreadId();
	const HANDLE updateEvent = _updateEvent.get();
	int hideCount = 0;

	auto restore = [&]() {
		// 只抵消本线程的调用；不把游戏的显示计数强行恢复成 0。
		while (hideCount > 0) {
			ShowCursor(TRUE);
			--hideCount;
		}
		if (_attachedThreadId) {
			AttachThreadInput(threadId, _attachedThreadId, FALSE);
			_attachedThreadId = 0;
		}
	};

	while (true) {
		// 附加输入队列期间也必须处理消息，不能只等待事件。
		const DWORD waitResult = MsgWaitForMultipleObjectsEx(
			1, &updateEvent, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
		if (waitResult == WAIT_FAILED) {
			Logger::Get().Win32Error("等待远程光标事件失败");
			break;
		}
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		std::lock_guard lock(_mutex);
		if (_stopping) {
			break;
		}

		if (_requestedThreadId != _attachedThreadId) {
			restore();
			if (_requestedThreadId && AttachThreadInput(threadId, _requestedThreadId, TRUE)) {
				_attachedThreadId = _requestedThreadId;
			}
		}
		if (!_attachedThreadId) {
			continue;
		}

		// 先减再加来读取共享计数，避免探测本身短暂显示系统光标。
		ShowCursor(FALSE);
		int cursorCount = ShowCursor(TRUE);
		const bool isCursorShowing = cursorCount + hideCount >= 0;
		while (cursorCount >= 0) {
			cursorCount = ShowCursor(FALSE);
			++hideCount;
		}
		// ShowCursor 不会清空输入队列中的光标形状。
		_hCursor = GetCursor();
		_isCursorShowing = isCursorShowing;
	}

	std::lock_guard lock(_mutex);
	restore();
}

}
