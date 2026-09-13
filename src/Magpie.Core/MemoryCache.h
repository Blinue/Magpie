#pragma once
#include <parallel_hashmap/phmap.h>

namespace Magpie {

namespace impl {

template<typename Key, typename Value, bool RequireStableValueAddress>
struct CacheStorage {
protected:
	phmap::flat_hash_map<Key, Value> _data;
};

template<typename Key, typename Value>
struct CacheStorage<Key, Value, true> {
protected:
	phmap::node_hash_map<Key, Value> _data;
};

template<typename T>
class HasInUse {
	template <typename U, typename = decltype(std::declval<U>().IsInUse())> static constexpr bool get_value(int) { return true; }
	template <typename> static constexpr bool get_value(...) { return false; }

public:
	static constexpr bool value = get_value<T>(0);
};

}

// 不是线程安全的，多线程使用需自行同步
template<typename Key, typename Value, uint32_t MaxCacheCount, bool RequireStableValueAddress = false>
class LruMemoryCache : public impl::CacheStorage<Key, std::pair<Value, uint32_t>, RequireStableValueAddress> {
public:
	LruMemoryCache() = default;
	LruMemoryCache(const LruMemoryCache&) = delete;
	LruMemoryCache(LruMemoryCache&&) = default;

	void Add(const Key& key, Value&& value) noexcept {
		auto& data = this->_data;
		data[key] = { std::move(value),_nextLastAccess++ };

		// 超过限制则清理一半较旧的缓存
		if (data.size() > MaxCacheCount) {
			assert(data.size() == MaxCacheCount + 1);
			std::array<uint32_t, MaxCacheCount + 1> allLastAccess{};
			std::transform(data.begin(), data.end(), allLastAccess.begin(),
				[](const auto& pair) { return pair.second.second; });

			auto midIt = allLastAccess.begin() + allLastAccess.size() / 2;
			std::nth_element(allLastAccess.begin(), midIt, allLastAccess.end());
			uint32_t midLastAccess = *midIt;

			for (auto it = data.begin(); it != data.end();) {
				if (it->second.second >= midLastAccess) {
					++it;
					continue;
				}

				// 保留仍在使用中的缓存
				if constexpr (impl::HasInUse<Value>::value) {
					if (it->second.IsInUse()) {
						++it;
						continue;
					}
				}

				it = data.erase(it);
			}
		}
	}

	const Value* Find(const Key& key) noexcept {
		auto it = this->_data.find(key);
		if (it == this->_data.end()) {
			return nullptr;
		} else {
			it->second.second = _nextLastAccess++;
			return &it->second.first;
		}
	}

	void Clear() noexcept {
		if constexpr (impl::HasInUse<Value>::value) {
			for (auto it = this->_data.begin(); it != this->_data.end();) {
				if (it->second.IsInUse()) {
					++it;
				} else {
					it = this->_data.erase(it);
				}
			}
		} else {
			this->_data.clear();
		}
	}

private:
	uint32_t _nextLastAccess = 0;
};

}
