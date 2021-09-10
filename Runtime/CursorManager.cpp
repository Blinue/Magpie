#include "pch.h"
#include "CursorManager.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;


bool CursorManager::Initialize() {
	App& app = App::GetInstance();

	// 限制鼠标在窗口内
	if (!ClipCursor(&App::GetInstance().GetSrcClientRect())) {
		SPDLOG_LOGGER_ERROR(logger, "ClipCursor 失败");
	}

	if (App::GetInstance().IsAdjustCursorSpeed()) {
		// 设置鼠标移动速度
		if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0)) {
			const RECT& srcClient = app.GetSrcClientRect();
			const RECT& destRect = app.GetRenderer().GetDestRect();
			float scaleX = float(destRect.right - destRect.left) / (srcClient.right - srcClient.left);
			float scaleY = float(destRect.bottom - destRect.top) / (srcClient.bottom - srcClient.top);

			long newSpeed = std::clamp(lroundf(_cursorSpeed / (scaleX + scaleY) * 2), 1L, 20L);

			if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0)) {
				SPDLOG_LOGGER_ERROR(logger, "设置光标移速失败");
			}
		} else {
			SPDLOG_LOGGER_ERROR(logger, "获取光标移速失败");
		}
	}

	static bool initialized = false;
	if (!initialized) {
		if (!MagInitialize()) {
			SPDLOG_LOGGER_ERROR(logger, "MagInitialize 失败");
		} else {
			initialized = true;
		}
	}
	
	if (!MagShowSystemCursor(FALSE)) {
		SPDLOG_LOGGER_ERROR(logger, "MagShowSystemCursor 失败");
	}

	return true;
}

CursorManager::~CursorManager() {
	ClipCursor(nullptr);

	if (App::GetInstance().IsAdjustCursorSpeed()) {
		SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);
	}

	MagShowSystemCursor(TRUE);
}
