#pragma once

namespace Magpie {

template <typename T>
class EventRevokerT {
public:
	EventRevokerT() noexcept = default;

	EventRevokerT(const EventRevokerT&) noexcept = delete;
	EventRevokerT(EventRevokerT&& other) noexcept {
		_Swap(other);
	}

	EventRevokerT& operator=(const EventRevokerT&) noexcept = delete;
	EventRevokerT& operator=(EventRevokerT&& other) noexcept {
		EventRevokerT(std::move(other))._Swap(*this);
		return *this;
	}

	EventRevokerT(T* event, T::TokenType token) noexcept : _event(event), _token(token) {}

	~EventRevokerT() {
		if (_event) {
			_event->operator()(_token);
		}
	}

	void Revoke() {
		if (_event) {
			_event->operator()(_token);
			_event = nullptr;
		}
	}

private:
	void _Swap(EventRevokerT& other) noexcept {
		std::swap(_event, other._event);
		std::swap(_token, other._token);
	}

	T* _event = nullptr;
	T::TokenType _token;
};

struct EventToken {
	uint32_t value = 0;

	explicit operator bool() const noexcept {
		return value != 0;
	}
};

#ifdef _DEBUG
inline int _DEBUG_DELEGATE_COUNT = 0;
#endif

// 简易且高效的事件，不支持在回调中修改事件本身
template <typename... TArgs>
class Event {
private:
	using _FunctionType = std::function<void(TArgs...)>;

public:
	using TokenType = EventToken;
	using EventRevoker = EventRevokerT<Event>;

#ifdef _DEBUG
	~Event() {
		_DEBUG_DELEGATE_COUNT -= (int)_delegates.size();
	}
#endif

	template <typename T>
	EventToken operator()(T&& handler) {
#ifdef _DEBUG
		++_DEBUG_DELEGATE_COUNT;
#endif
		_delegates.emplace_back(++_curToken, std::forward<T>(handler));
		return { _curToken };
	}

	void operator()(EventToken token) noexcept {
#ifdef _DEBUG
		--_DEBUG_DELEGATE_COUNT;
#endif
		auto it = std::find_if(_delegates.begin(), _delegates.end(),
			[token](const std::pair<uint32_t, _FunctionType>& d) { return d.first == token.value; });
		assert(it != _delegates.end());
		_delegates.erase(it);
	}

	// 调用者应确保 EventRevoker 在 Event 的生命周期内执行撤销
	template <typename T>
	EventRevoker operator()(winrt::auto_revoke_t, T&& handler) {
		return EventRevoker(this, operator()(std::forward<T>(handler)));
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
	using EventRevoker = EventRevokerT<MultithreadEvent>;

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
		return EventRevoker(this, _BaseType::operator()(std::forward<T>(handler)));
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
	using TokenType = winrt::event_token;
	using EventRevoker = EventRevokerT<WinRTEvent>;

	winrt::event_token operator()(const T& handler) {
		return _handler.add(handler);
	}

	auto operator()(const winrt::event_token& token) noexcept {
		return _handler.remove(token);
	}

	EventRevoker operator()(winrt::auto_revoke_t, const T& handler) {
		winrt::event_token token = operator()(handler);
		return EventRevoker(this, token);
	}

	template <typename... TArgs>
	auto Invoke(TArgs&&... args) {
		return _handler(std::forward<TArgs>(args)...);
	}

private:
	winrt::event<T> _handler;
};

}
