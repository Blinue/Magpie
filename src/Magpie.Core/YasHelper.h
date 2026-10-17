#pragma once
#include "ByteBuffer.h"
#include "SmallVector.h"
// YAS 暂不支持 ARM64
// https://github.com/niXman/yas/pull/121
#ifdef _M_ARM64
#define _LITTLE_ENDIAN
#endif
#pragma warning(push)
// C4458: “size”的声明隐藏了类成员
// C4127: 条件表达式是常量
#pragma warning(disable: 4458 4127)
#include <yas/mem_streams.hpp>
#include <yas/binary_oarchive.hpp>
#include <yas/binary_iarchive.hpp>
#include <yas/types/std/pair.hpp>
#include <yas/types/std/string.hpp>
#include <yas/types/std/string_view.hpp>
#include <yas/types/std/vector.hpp>
#include <yas/types/std/variant.hpp>
#pragma warning(pop)

namespace yas::detail {

// 可平凡复制类型，注意不检查指针成员
template <size_t F, typename T>
struct serializer<
	type_prop::not_a_fundamental,
	ser_case::use_internal_serializer,
	F,
	T
> {
	template <typename Archive, typename = std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, T>>
	static Archive& save(Archive& ar, const T& o) noexcept {
		ar.write(&o, sizeof(T));
		return ar;
	}

	template <typename Archive, typename = std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, T>>
	static Archive& load(Archive& ar, T& o) noexcept {
		ar.read(&o, sizeof(T));
		return ar;
	}
};

// SmallVector
template <size_t F, typename T, unsigned N>
struct serializer<
	type_prop::not_a_fundamental,
	ser_case::use_internal_serializer,
	F,
	Magpie::SmallVector<T, N>
> {
	template <typename Archive>
	static Archive& save(Archive& ar, const Magpie::SmallVectorImpl<T>& vector) noexcept {
		return concepts::array::save<F>(ar, vector);
	}

	template <typename Archive>
	static Archive& load(Archive& ar, Magpie::SmallVectorImpl<T>& vector) noexcept {
		return concepts::array::load<F>(ar, vector);
	}
};

// 避免依赖 D3DCompiler_47.dll
struct SimpleBlob : winrt::implements<SimpleBlob, ID3DBlob> {
public:
	SimpleBlob(uint32_t size) : _buffer(size), _size(size) {}

	void* STDMETHODCALLTYPE GetBufferPointer() noexcept override {
		return _buffer.Data();
	}

	size_t STDMETHODCALLTYPE GetBufferSize() noexcept override {
		return _size;
	}

private:
	Magpie::ByteBuffer _buffer;
	uint32_t _size;
};

// winrt::com_ptr<ID3DBlob>
template <std::size_t F>
struct serializer<
	type_prop::not_a_fundamental,
	ser_case::use_internal_serializer,
	F,
	winrt::com_ptr<ID3DBlob>
> {
	template <typename Archive>
	static Archive& save(Archive& ar, const winrt::com_ptr<ID3DBlob>& blob) noexcept {
		uint32_t size = (uint32_t)blob->GetBufferSize();
		ar& size;

		ar.write(blob->GetBufferPointer(), size);

		return ar;
	}

	template <typename Archive>
	static Archive& load(Archive& ar, winrt::com_ptr<ID3DBlob>& blob) noexcept {
		uint32_t size = 0;
		ar& size;

		blob = winrt::make<SimpleBlob>(size);
		ar.read(blob->GetBufferPointer(), size);

		return ar;
	}
};

}

#ifdef _M_ARM64
#undef _LITTLE_ENDIAN
#endif
