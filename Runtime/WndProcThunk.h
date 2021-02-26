// 向 WndProc 传递 this 指针的 thunk
// 参考自 https://www.codeproject.com/Articles/1121696/Cplusplus-WinAPI-Wrapper-Object-using-thunks-x-and
//
// 注意 32 位和 64 位使用的方法不同
// 参考如下代码：
// #if defined(_M_IX86)
// static LRESULT CALLBACK WndProc(DWORD_PTR This, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
// #elif defined(_M_AMD64)
// static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, DWORD_PTR This)
// #endif

#pragma once
#include <memoryapi.h>

class WndProcThunk {
public:
    WndProcThunk() {
        _asmCode = (_StdCallAsm*)VirtualAlloc(NULL, sizeof(_StdCallAsm), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    }

    ~WndProcThunk() {
        if (_asmCode) {
            VirtualFree(_asmCode, 0, MEM_RELEASE);
        }
    }

    template<typename T>
    void* Init(void* pThis, T fn) {
#if defined(_M_IX86)
        _asmCode->push1 = 0x34ff;
        _asmCode->push2 = 0xc724;
        _asmCode->mov1 = 0x2444;
        _asmCode->mov2 = 0x04;
        _asmCode->pThis = (UINT_PTR)pThis;
        _asmCode->jmp = 0xE9;
        _asmCode->relproc = (UINT_PTR)fn - ((UINT32)_asmCode + sizeof(_StdCallAsm));
#elif defined(_M_AMD64)
        _asmCode->RaxMov = 0xb848;
        _asmCode->RaxImm = (UINT_PTR)pThis;
        _asmCode->RspMov = 0x24448948;
        _asmCode->RspMov1 = 0x9028;
        _asmCode->Rax2Mov = 0xb848;
        _asmCode->ProcImm = (UINT_PTR)mfn;
        _asmCode->RaxJmp = 0xe0ff;
#endif
        FlushInstructionCache(GetCurrentProcess(), _asmCode, sizeof(_StdCallAsm));

        return _asmCode;
    }

private:
#if defined(_M_IX86)
#pragma pack(push,1)
    struct _StdCallAsm {
        UINT16 push1;
        UINT16 push2;
        UINT16 mov1;
        UINT8 mov2;
        UINT_PTR pThis;
        UINT8 jmp;
        UINT_PTR relproc;
    };
#pragma pack(pop)
#elif defined(_M_AMD64)
#pragma pack(push,2)
    struct _StdCallAsm {
        UINT16 RaxMov;
        UINT_PTR RaxImm;
        UINT32 RspMov;
        UINT16 RspMov1;
        UINT16 Rax2Mov;
        UINT_PTR ProcImm;
        UINT16 RaxJmp;
    };
#pragma pack(pop)
#else
    // vs 似乎不支持 static_assert 中使用中文
    static_assert(false, "Unsupported processor architecture!");
#endif

    _StdCallAsm* _asmCode;
};

