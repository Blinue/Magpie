#pragma once

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

		void Revoke() {
			if (_revoker) {
				_revoker();
				_revoker = {};
			}
		}

	private:
		std::function<void()> _revoker;
	};

	// 和 wil::(un)typed_event 相比支持任意委托类型以及 auto revoke
	template <typename T>
	struct Event {
		winrt::event_token operator()(const T& handler) {
			return _handler.add(handler);
		}

		auto operator()(const winrt::event_token& token) noexcept {
			return _handler.remove(token);
		}

		EventRevoker operator()(winrt::auto_revoke_t, const T& handler) {
			winrt::event_token token = operator()(handler);
			return WinRTUtils::EventRevoker([this, token]() {
				// 调用者应确保此函数在 Event 的生命周期内执行
				operator()(token);
			});
		}

		template <typename... TArgs>
		auto Invoke(TArgs&&... args) {
			return _handler(std::forward<TArgs>(args)...);
		}

	private:
		winrt::event<T> _handler;
	};
};
