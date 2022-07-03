#pragma once
#include "CommonPCH.h"
#include "Utils.h"


struct WinRTUtils {
	class EventRevoker {
	public:
		EventRevoker() noexcept = default;

		EventRevoker(const EventRevoker&) noexcept = delete;
		EventRevoker(EventRevoker&& other) noexcept = default;

		EventRevoker& operator=(const EventRevoker& other) noexcept = delete;
		EventRevoker& operator=(EventRevoker&& other) noexcept {
			EventRevoker(std::move(other)).Swap(*this);
			return *this;
		}

		explicit EventRevoker(std::function<void()>&& revoker) noexcept : _revoker(std::forward<std::function<void()>>(revoker)) {}

		~EventRevoker() noexcept {
			if (_revoker) {
				_revoker();
			}
		}

		void Swap(EventRevoker& other) noexcept {
			std::swap(_revoker, other._revoker);
		}

	private:
		std::function<void()> _revoker{};
	};
};
