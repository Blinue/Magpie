#pragma once
#include "pch.h"


// 简单的事件发生器
template<typename T>
class EventEmitter {
public:
	virtual ~EventEmitter() {}

	EventEmitter& on(std::function<T> func) {
		_funcs.push_back(func);
		return *this;
	}

	EventEmitter& off(std::function<T> func) {
		auto it = std::find(_funcs.begin(), _funcs.end(), func);
		if (it != _funcs.end()) {
			_funcs.erase(it);
		}

		return *this;
	}

	EventEmitter& clear() {
		_funcs.clear();
		return *this;
	}
protected:
	void emit() {
		for (const auto& func : _funcs) {
			func();
		}
	}

private:
	std::vector<std::function<T>> _funcs;
};
