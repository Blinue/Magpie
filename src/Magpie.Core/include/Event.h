#pragma once

namespace Magpie::Core {

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

	template <typename T>
	explicit EventRevoker(T&& revoker) noexcept : _revoker(std::forward<T>(revoker)) {}

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

struct EventToken {
	uint32_t value = 0;

	explicit operator bool() const noexcept {
		return value != 0;
	}
};

// 简易且高效的事件，不支持在回调中修改事件本身
template <typename... TArgs>
class Event {
private:
	using _FunctionType = std::function<void(TArgs...)>;

public:
	template <typename T>
	EventToken operator()(T&& handler) {
		_delegates.emplace_back(++_curToken, std::forward<T>(handler));
		return { _curToken };
	}

	void operator()(EventToken token) noexcept {
		auto it = std::find_if(_delegates.begin(), _delegates.end(),
			[token](const std::pair<uint32_t, _FunctionType>& d) { return d.first == token.value; });
		assert(it != _delegates.end());
		_delegates.erase(it);
	}

	template <typename T>
	EventRevoker operator()(winrt::auto_revoke_t, T&& handler) {
		EventToken token = operator()(std::forward<T>(handler));
		return EventRevoker([this, token]() {
			// 调用者应确保此函数在 Event 的生命周期内执行
			operator()(token);
		});
	}

	template <typename... TArgs1>
	void Invoke(TArgs1&&... args) {
		for (auto& pair : _delegates) {
			pair.second(std::forward<TArgs1>(args)...);
		}
	}

private:
	// 考虑到回调数量都很少，std::vector 的性能更好
	std::vector<std::pair<uint32_t, _FunctionType>> _delegates;
	uint32_t _curToken = 0;
};

// 对 Event 的每个操作加锁
template <typename... TArgs>
class MultithreadEvent : public Event<TArgs...> {
private:
	using _BaseType = Event<TArgs...>;

public:
	template <typename T>
	EventToken operator()(T&& handler) {
		auto lock = _lock.lock_exclusive();
		return _BaseType::operator()(std::forward<T>(handler));
	}

	void operator()(EventToken token) noexcept {
		auto lock = _lock.lock_exclusive();
		_BaseType::operator()(token);
	}

	template <typename T>
	EventRevoker operator()(winrt::auto_revoke_t, T&& handler) {
		auto lock = _lock.lock_exclusive();
		return _BaseType::operator()(winrt::auto_revoke, std::forward<T>(handler));
	}

	template <typename... TArgs1>
	void Invoke(TArgs1&&... args) {
		auto lock = _lock.lock_exclusive();
		_BaseType::Invoke(std::forward<TArgs1>(args)...);
	}

private:
	wil::srwlock _lock;
};

// 用于简化 WinRT 组件的创建，和 wil::(un)typed_event 相比支持任意委托类型以及 auto revoke
template <typename T>
struct WinRTEvent {
	winrt::event_token operator()(const T& handler) {
		return _handler.add(handler);
	}

	auto operator()(const winrt::event_token& token) noexcept {
		return _handler.remove(token);
	}

	EventRevoker operator()(winrt::auto_revoke_t, const T& handler) {
		winrt::event_token token = operator()(handler);
		return EventRevoker([this, token]() {
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

}
