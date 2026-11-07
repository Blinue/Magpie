#pragma once
#include "SmallVector.h"

namespace Magpie {

// CreateStreamOnHGlobal 和 SHCreateMemStream 都很低效，因此自己实现内存 IStream
struct MemStream : winrt::implements<MemStream, IStream,
	winrt::non_agile, winrt::no_weak_ref, winrt::no_module_lock> {
public:
	MemStream(uint32_t capacity) {
		_buffer.reserve(capacity);
	}

	const uint8_t* GetData() const noexcept {
		return _buffer.data();
	}

	uint32_t GetSize() const noexcept {
		return (uint32_t)_buffer.size();
	}

	HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead) noexcept override {
		const uint32_t bufferSize = (uint32_t)_buffer.size();

		if (_curPos >= bufferSize) {
			if (pcbRead) {
				*pcbRead = 0;
			}
		} else {
			if (cb > 0) {
				cb = std::min(cb, ULONG(bufferSize - _curPos));
				std::memcpy(pv, _buffer.data() + _curPos, cb);
				_curPos += cb;
			}
			
			if (pcbRead) {
				*pcbRead = cb;
			}
		}

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten) noexcept override {
		if (cb > 0) {
			const uint32_t newPos = _curPos + (uint32_t)cb;
			_buffer.resize(std::max((uint32_t)_buffer.size(), newPos));

			std::memcpy(_buffer.data() + _curPos, pv, cb);
			_curPos = newPos;
		}
		
		if (pcbWritten) {
			*pcbWritten = cb;
		}

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Seek(
		LARGE_INTEGER dlibMove,
		DWORD dwOrigin,
		ULARGE_INTEGER* plibNewPosition
	) noexcept override {
		int newPos;

		switch (dwOrigin) {
		case STREAM_SEEK_SET:
			newPos = (int)dlibMove.LowPart;
			break;
		case STREAM_SEEK_CUR:
			newPos = (int)_curPos + (int)dlibMove.LowPart;
			break;
		case STREAM_SEEK_END:
			newPos = (int)_buffer.size() + (int)dlibMove.LowPart;
			break;
		default:
			return E_INVALIDARG;
		}

		if (newPos < 0) {
			return E_INVALIDARG;
		}

		// 支持定位到尾部之后，实际写入时再更新 _buffer 尺寸
		_curPos = (uint32_t)newPos;

		if (plibNewPosition) {
			plibNewPosition->LowPart = _curPos;
			plibNewPosition->HighPart = 0;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize) noexcept override {
		_buffer.resize((uint32_t)libNewSize.LowPart);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE CopyTo(
		IStream* pstm,
		ULARGE_INTEGER cb,
		ULARGE_INTEGER* pcbRead,
		ULARGE_INTEGER* pcbWritten
	) noexcept override {
		uint32_t cbRead = 0;
		ULONG cbWritten = 0;
		HRESULT hr = S_OK;

		if (_curPos < (uint32_t)_buffer.size()) {
			cbRead = (uint32_t)_buffer.size() - _curPos;

			if (cb.HighPart == 0 && cb.LowPart < cbRead) {
				cbRead = cb.LowPart;
			}

			if (cbRead > 0) {
				hr = pstm->Write(_buffer.data() + _curPos, cbRead, &cbWritten);
				_curPos += cbRead;
			}
		}
		
		if (pcbRead) {
			pcbRead->LowPart = cbRead;
			pcbRead->HighPart = 0;
		}
		if (pcbWritten) {
			pcbWritten->LowPart = cbWritten;
			pcbWritten->HighPart = 0;
		}

		return hr;
	}

	HRESULT STDMETHODCALLTYPE Commit(DWORD) noexcept override {
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Revert() noexcept override {
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) noexcept override {
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) noexcept override {
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Stat(STATSTG* pstatstg, DWORD) noexcept override {
		*pstatstg = {
			.type = STGTY_STREAM,
			.cbSize = { .LowPart = (DWORD)_buffer.size() },
			.grfMode = STGM_READWRITE
		};
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Clone(IStream** ppstm) noexcept override {
		*ppstm = NULL;
		return E_NOTIMPL;
	}

private:
	Magpie::SmallVector<uint8_t, 0> _buffer;
	uint32_t _curPos = 0;
};

}
