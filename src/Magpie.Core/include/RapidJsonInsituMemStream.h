#include <rapidjson/stream.h>

namespace Magpie {

// radpidjson 只提供了 \0 结尾字符串的原位解析，这个类使得不以 \0 结尾的字符串也可以原位解析。
// 它基本上是 radpidjson::MemoryStream 和 radpidjson::GenericInsituStringStream 的组合。
struct RapidJsonInsituMemStream {
	using Ch = char;

	RapidJsonInsituMemStream(Ch* src, size_t size) : _src(src), _begin(src), _end(src + size) {}

	// 从流读取当前字符，不移动读取指针
	Ch Peek() const {
		return _src == _end ? '\0' : *_src;
	}

	// 从流读取当前字符，移动读取指针至下一字符
	Ch Take() {
		return _src == _end ? '\0' : *_src++;
	}

	// 获取读取指针
	size_t Tell() const {
		return _src - _begin;
	}

	// 从当前读取指针开始写入操作
	Ch* PutBegin() {
		_dest = _src;
		return _dest;
	}

	// 写入一个字符
	void Put(Ch c) {
		assert(_dest >= _begin && _dest < _end);
		*_dest++ = c;
	}

	// 完成写操作
	size_t PutEnd(Ch* begin) {
		assert (_dest);
		return _dest - begin;
	}

	// 清空缓冲区（不会被调用）
	void Flush() { assert(false); }

private:
	Ch* _src = nullptr;
	Ch* _dest = nullptr;
	const Ch* _begin = nullptr;
	const Ch* _end = nullptr;
};

}

namespace rapidjson {

// 启用“局部流拷贝”优化，rapidjson 会将流对象拷贝到本地变量来帮助编译器优化。
template<>
struct StreamTraits<::Magpie::RapidJsonInsituMemStream> {
	enum { copyOptimization = 1 };
};

}
