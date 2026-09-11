#include "pch.h"
#include "CursorHelper.h"
#include "ByteBuffer.h"
#include "Logger.h"
#include "SmallVector.h"
#include "Win32Helper.h"
#include <mmsystem.h> // FOURCC

namespace Magpie {

struct RTAG {
	DWORD ckID;
	DWORD ckSize;
};

static WORD GetRealIconSize(WORD size) noexcept {
	// 0 等价于 256
	return size == 0 ? (WORD)256 : size;
}

wil::unique_hcursor CursorHelper::ExtractCursorFromModule(
	HMODULE hModule,
	LPCWSTR resName,
	uint32_t preferredWidth
) noexcept {
	HRSRC hRes = FindResource(hModule, resName, RT_GROUP_CURSOR);
	if (!hRes) {
		Logger::Get().Win32Error("FindResource 失败");
		return nullptr;
	}

	HGLOBAL hResLoad = LoadResource(hModule, hRes);
	if (!hResLoad) {
		Logger::Get().Win32Error("LoadResource 失败");
		return nullptr;
	}

	// 解析光标资源
#pragma pack(push, 2)
	// 来自 https://learn.microsoft.com/en-us/windows/win32/menurc/resdir
	struct RESDIR {
		WORD Width;
		WORD Height;
		WORD Planes;
		WORD BitCount;
		DWORD BytesInRes;
		WORD IconCursorId;
	};

	// 来自 https://learn.microsoft.com/en-us/windows/win32/menurc/newheader
	struct NEWHEADER {
		WORD Reserved;
		WORD ResType;
		WORD ResCount;
		RESDIR entries[1];
	};
#pragma pack(pop)

	const NEWHEADER& header = *(const NEWHEADER*)LockResource(hResLoad);
	if (header.Reserved != 0 || header.ResType != 2) {
		Logger::Get().Error("不是光标资源");
		return nullptr;
	}

	const uint32_t resCount = header.ResCount;
	if (resCount == 0 || resCount > 256) {
		Logger::Get().Error("无可用光标资源");
		return nullptr;
	}

	struct IconInfo {
		WORD width;
		WORD bitCount;
		WORD id;
	};
	SmallVector<IconInfo, 0> iconInfos(resCount);

	for (uint32_t i = 0; i < resCount; ++i) {
		const RESDIR& entry = header.entries[i];
		// 宽度和高度的 0 等价于 256
		iconInfos[i] = IconInfo{
			GetRealIconSize(entry.Width),
			entry.BitCount,
			entry.IconCursorId
		};
	}

	// 尺寸从小到大排序；如果尺寸相同，色深从大到小排序，以便获得色深最大的光标
	std::sort(iconInfos.begin(), iconInfos.end(), [](const IconInfo& l, const IconInfo& r) {
		return l.width < r.width || (l.width == r.width && l.bitCount > r.bitCount);
	});

	// 寻找完美匹配或更大的资源
	WORD targetResId;
	{
		auto it = std::lower_bound(
			iconInfos.begin(),
			iconInfos.end(),
			preferredWidth,
			[](const IconInfo& iconInfo, uint32_t target) {
				return iconInfo.width < target;
			}
		);
		if (it == iconInfos.end()) {
			targetResId = iconInfos.back().id;
		} else {
			targetResId = it->id;
		}
	}

	hRes = FindResource(hModule, MAKEINTRESOURCE(targetResId), RT_CURSOR);
	if (!hRes) {
		Logger::Get().Win32Error("FindResource 失败");
		return nullptr;
	}

	hResLoad = LoadResource(hModule, hRes);
	if (!hResLoad) {
		Logger::Get().Win32Error("LoadResource 失败");
		return nullptr;
	}

	HICON hIcon = CreateIconFromResourceEx((PBYTE)LockResource(hResLoad),
		SizeofResource(hModule, hRes), FALSE, 0x30000, 0, 0, LR_DEFAULTCOLOR);
	if (!hIcon) {
		Logger::Get().Win32Error("CreateIconFromResourceEx 失败");
		return nullptr;
	}

	return wil::unique_hcursor(hIcon);
}

// CUR 文件结构如下，参考自 https://en.wikipedia.org/wiki/ICO_(file_format)#File_structure
// [ICONDIR]
//   [ICONDIRENTRY 1]
//   [ICONDIRENTRY 2]
//   ...
// [位图 1]
// [位图 2]
// ...
static wil::unique_hcursor LoadIcoFromFileMap(
	const uint8_t* fileData,
	const uint8_t* fileEnd,
	uint32_t preferredWidth
) noexcept {
#pragma pack(push, 2)
	struct ICONDIR {
		WORD idReserved;
		WORD idType;
		WORD idCount;
	};

	struct ICONDIRENTRY {
		BYTE bWidth;
		BYTE bHeight;
		BYTE bColorCount;
		BYTE bReserved;
		WORD xHotSpot;
		WORD yHotSpot;
		DWORD dwBytesInRes;
		DWORD dwImageOffset;
	};

	struct LOCALHEADER {
		WORD xHotSpot;
		WORD yHotSpot;
	};
#pragma pack(pop)

	if (fileData + sizeof(ICONDIR) > fileEnd) {
		Logger::Get().Error("文件无效");
		return nullptr;
	}

	uint32_t entryCount;
	{
		const ICONDIR& header = *(ICONDIR*)fileData;

		if (header.idReserved != 0 || header.idType != 2) {
			Logger::Get().Error("不是光标资源");
			return nullptr;
		}

		if (header.idCount == 0 || header.idCount > 256) {
			Logger::Get().Error("无可用光标资源");
			return nullptr;
		}

		entryCount = header.idCount;
	}

	const ICONDIRENTRY* targetEntry;
	{
		const ICONDIRENTRY* pEntries = (const ICONDIRENTRY*)(fileData + sizeof(ICONDIR));

		if ((uint8_t*)pEntries + sizeof(ICONDIRENTRY) * entryCount > fileEnd) {
			Logger::Get().Error("文件无效");
			return nullptr;
		}

		// 寻找完美匹配或更大的资源
		std::vector<const ICONDIRENTRY*> entries(entryCount);

		for (uint32_t i = 0; i < entryCount; ++i) {
			entries[i] = &pEntries[i];
		}

		// 尺寸从小到大排序；和资源不同，cur 文件不区分色深
		std::sort(entries.begin(), entries.end(), [](const ICONDIRENTRY* l, const ICONDIRENTRY* r) {
			return GetRealIconSize(l->bWidth) < GetRealIconSize(r->bWidth);
		});

		auto it = std::lower_bound(
			entries.begin(),
			entries.end(),
			preferredWidth,
			[](const ICONDIRENTRY* entry, uint32_t target) {
				return GetRealIconSize(entry->bWidth) < target;
			}
		);
		if (it == entries.end()) {
			targetEntry = entries.back();
		} else {
			targetEntry = *it;
		}
	}

	const uint8_t* pCursroData = fileData + targetEntry->dwImageOffset;

	if (pCursroData + targetEntry->dwBytesInRes > fileEnd) {
		Logger::Get().Error("文件无效");
		return nullptr;
	}

	// RT_CURSOR 结构为 LOCALHEADER 后跟位图数据
	// https://learn.microsoft.com/en-us/windows/win32/menurc/resource-file-formats#cursor-and-icon-resources
	ByteBuffer cursorData(sizeof(LOCALHEADER) + targetEntry->dwBytesInRes);
	// 设置热点
	*(LOCALHEADER*)cursorData.Data() = { targetEntry->xHotSpot, targetEntry->yHotSpot };
	// 读取位图数据
	std::memcpy(cursorData.Data() + sizeof(LOCALHEADER), pCursroData, targetEntry->dwBytesInRes);

	wil::unique_hcursor hCursor(CreateIconFromResourceEx(cursorData.Data(),
		sizeof(LOCALHEADER) + targetEntry->dwBytesInRes, FALSE, 0x30000, 0, 0, LR_DEFAULTCOLOR));
	if (!hCursor) {
		Logger::Get().Win32Error("CreateIconFromResourceEx 失败");
		return nullptr;
	}

	return hCursor;
}

static std::chrono::nanoseconds JifRateToDuration(uint32_t jifRate) noexcept {
	using namespace std::chrono;
	return nanoseconds(seconds(jifRate)) / 60;
}

// RIFF 格式参见 https://en.wikipedia.org/wiki/Resource_Interchange_File_Format
// ANI 文件结构如下，来自 https://en.wikipedia.org/wiki/ANI_(file_format)
// RIFF('ACON'
//   [LIST('INFO'
//     [INAM(<ZSTR>)]          // 标题 (可选)
//     [IART(<ZSTR>)]          // 作者 (可选)
//   )]
//   'anih'(<ANIHEADER>)       // ANI 文件头
//   ['rate'(<DWORD...>)]      // 速率表 (jiffies 数组)。如果设置了 AF_SEQUENCE 标志，则数
//                             // 量为 ANIHEADER.cSteps，否则为 ANIHEADER.cFrames。
//   ['seq '(<DWORD...>)]      // 序列表 (帧索引值数组)。当设置 AF_SEQUENCE 标志时应存在，
//                             // 数量为 ANIHEADER.cSteps。
//   LIST('fram'               // 帧数据列表，数量为 ANIHEADER.cFrames
//     'icon'(<icon_data_1>)   // 第 1 帧
//     'icon'(<icon_data_2>)   // 第 2 帧
//     ...
//   )
// )
static bool LoadAniFromFileMap(
	const uint8_t* fileData,
	const uint8_t* fileEnd,
	uint32_t preferredWidth,
	SmallVectorImpl<wil::unique_hcursor>& frames,
	SmallVectorImpl<std::pair<uint32_t, std::chrono::nanoseconds>>& frameSequence
) {
#pragma pack(push, 2)
	struct ANIHEADER {
		DWORD cbSizeof;
		DWORD cFrames;              // 帧数据列表元素数量
		DWORD cSteps;               // 序列表元素数量
		DWORD cx, cy;               // 不使用
		DWORD cBitCount, cPlanes;   // 不使用
		DWORD jifRate;              // 默认显示速率, 单位为 jiffy (1/60s)
		DWORD fl;                   // 必须设置 AF_ICON，可选 AF_SEQUENCE
	};
#pragma pack(pop)

	constexpr DWORD FOURCC_ACON = mmioFOURCC('A', 'C', 'O', 'N');
	constexpr DWORD FOURCC_anih = mmioFOURCC('a', 'n', 'i', 'h');
	constexpr DWORD FOURCC_rate = mmioFOURCC('r', 'a', 't', 'e');
	constexpr DWORD FOURCC_seq = mmioFOURCC('s', 'e', 'q', ' ');
	constexpr DWORD FOURCC_fram = mmioFOURCC('f', 'r', 'a', 'm');
	constexpr DWORD FOURCC_icon = mmioFOURCC('i', 'c', 'o', 'n');

	constexpr DWORD AF_ICON = 0x1;
	constexpr DWORD AF_SEQUENCE = 0x2;

	// 已经检查 RIFF 头
	fileData += sizeof(RTAG);

	if (fileData + sizeof(uint32_t) > fileEnd) {
		Logger::Get().Error("文件无效");
		return false;
	}
	if (*(uint32_t*)fileData != FOURCC_ACON) {
		Logger::Get().Error("文件无效");
		return false;
	}
	fileData += sizeof(uint32_t);

	ANIHEADER aniHeader{};
	uint32_t curFrameIdx = 0;

	while (fileData + sizeof(RTAG) < fileEnd) {
		RTAG tag = *(RTAG*)fileData;
		fileData += sizeof(RTAG);

		const uint8_t* chunkEnd = fileData + ((tag.ckSize + 1) & ~1);
		if (chunkEnd > fileEnd) {
			Logger::Get().Error("文件无效");
			return false;
		}

		// 不确定是不是强制的，在 Windows 的实现中，anih 块必须比 fram、rate 和 seq 块先
		// 出现。我们和系统保持一致，这可以简化代码。
		switch (tag.ckID) {
		case FOURCC_anih:
		{
			if (fileData + sizeof(ANIHEADER) > chunkEnd) {
				Logger::Get().Error("文件无效");
				return false;
			}

			aniHeader = *(ANIHEADER*)fileData;

			if (aniHeader.cbSizeof != sizeof(ANIHEADER) ||
				aniHeader.cFrames == 0 ||
				((aniHeader.fl & AF_SEQUENCE) && aniHeader.cSteps == 0) ||
				!(aniHeader.fl & AF_ICON))
			{
				Logger::Get().Error("文件无效");
				return false;
			}

			frames.resize(aniHeader.cFrames);

			// 如果只有一帧则不是动态光标
			if (aniHeader.cFrames > 1) {
				if (aniHeader.fl & AF_SEQUENCE) {
					frameSequence.resize(aniHeader.cSteps);
					// 用于检查 seq 块是否存在
					frameSequence[0].first = std::numeric_limits<uint32_t>::max();
				} else {
					frameSequence.resize(aniHeader.cFrames);
					// 逐帧播放
					for (uint32_t i = 0; i < frameSequence.size(); ++i) {
						frameSequence[i].first = i;
					}
				}

				for (auto& pair : frameSequence) {
					pair.second = JifRateToDuration(aniHeader.jifRate);
				}
			}

			break;
		}
		case FOURCC_LIST:
		{
			if (fileData + sizeof(uint32_t) > chunkEnd) {
				Logger::Get().Error("文件无效");
				return false;
			}

			// 如果不是 fram 块则跳过此 LIST 块
			if (*(uint32_t*)fileData != FOURCC_fram) {
				break;
			}

			fileData += sizeof(uint32_t);

			// 确保已解析 anih 块
			if (aniHeader.cbSizeof == 0) {
				Logger::Get().Error("文件无效");
				return false;
			}

			if (curFrameIdx == aniHeader.cFrames) {
				break;
			}

			while (fileData + sizeof(RTAG) < chunkEnd) {
				tag = *(RTAG*)fileData;
				fileData += sizeof(RTAG);

				const uint8_t* subChunkEnd = fileData + ((tag.ckSize + 1) & ~1);
				if (subChunkEnd > chunkEnd) {
					Logger::Get().Error("文件无效");
					return false;
				}

				if (tag.ckID == FOURCC_icon) {
					wil::unique_hcursor hCursor = LoadIcoFromFileMap(fileData, subChunkEnd, preferredWidth);
					if (hCursor) {
						frames[curFrameIdx++] = std::move(hCursor);
					} else {
						Logger::Get().Error("LoadIcoFromFileMap 失败");
						return false;
					}

					if (curFrameIdx == aniHeader.cFrames) {
						break;
					}
				}

				fileData = subChunkEnd;
			}

			break;
		}
		case FOURCC_rate:
		{
			// 确保已解析 anih 块
			if (aniHeader.cbSizeof == 0) {
				Logger::Get().Error("文件无效");
				return false;
			}

			// 只有一帧则忽略 rate 块
			if (frameSequence.empty()) {
				break;
			}

			if (fileData + sizeof(uint32_t) * frameSequence.size() > chunkEnd) {
				Logger::Get().Error("文件无效");
				return false;
			}

			for (auto& pair : frameSequence) {
				pair.second = JifRateToDuration(*(uint32_t*)fileData);
				fileData += sizeof(uint32_t);
			}

			break;
		}
		case FOURCC_seq:
		{
			// 确保已解析 anih 块
			if (aniHeader.cbSizeof == 0) {
				Logger::Get().Error("文件无效");
				return false;
			}

			// 无 AF_SEQUENCE 标志或只有一帧时忽略 seq 块
			if (!(aniHeader.fl & AF_SEQUENCE) || frameSequence.empty()) {
				break;
			}

			if (fileData + sizeof(uint32_t) * aniHeader.cSteps > chunkEnd) {
				Logger::Get().Error("文件无效");
				return false;
			}

			for (auto& pair : frameSequence) {
				pair.first = *(uint32_t*)fileData;
				fileData += sizeof(uint32_t);
			}
			
			break;
		}
		}

		fileData = chunkEnd;
	}

	// 确保所有帧都已提取
	if (frames.empty() || curFrameIdx != aniHeader.cFrames) {
		Logger::Get().Error("文件无效");
		return false;
	}

	// 只有一帧时 frameSequence 为空
	if (frameSequence.empty()) {
		return true;
	}

	for (const auto& pair : frameSequence) {
		// 确保持续时间不为 0
		if (pair.second.count() == 0) {
			Logger::Get().Error("文件无效");
			return false;
		}
	}

	if (aniHeader.fl & AF_SEQUENCE) {
		std::vector<bool> frameInUse(aniHeader.cFrames);

		for (const auto& pair : frameSequence) {
			// 检查序列是否合法
			if (pair.first >= aniHeader.cFrames) {
				Logger::Get().Error("文件无效");
				return false;
			}

			frameInUse[pair.first] = true;
		}

		// 删除未被使用的帧
		for (int i = aniHeader.cFrames - 1; i >= 0; --i) {
			if (frameInUse[i]) {
				continue;
			}

			frames.erase(frames.begin() + i);

			// 删除一帧后调整索引
			for (auto& pair : frameSequence) {
				if (pair.first > (uint32_t)i) {
					--pair.first;
				}
			}
		}

		// 只剩一帧则不是动态光标
		if (frames.size() == 1) {
			frameSequence.clear();
		}
	}

	return true;
}

bool CursorHelper::ExtractCursorFramesFromFile(
	const wchar_t* fileName,
	uint32_t preferredWidth,
	SmallVectorImpl<wil::unique_hcursor>& frames,
	SmallVectorImpl<std::pair<uint32_t, std::chrono::nanoseconds>>& frameSequence
) noexcept {
	assert(frames.empty() && frameSequence.empty());

	CREATEFILE2_EXTENDED_PARAMETERS extendedParams{
		.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
		.dwFileAttributes = FILE_ATTRIBUTE_NORMAL,
		.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN
	};
	wil::unique_hfile hFile(CreateFile2(
		fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
	if (!hFile) {
		Logger::Get().Win32Error("CreateFile2 失败");
		return false;
	}

	const DWORD fileSize = GetFileSize(hFile.get(), nullptr);
	// 这个检查确保可以访问 RIFF 头，也确保不会把空文件传给 CreateFileMapping
	if (fileSize < sizeof(RTAG)) {
		Logger::Get().Error("文件无效");
		return false;
	}

	wil::unique_handle hFileMap(CreateFileMapping(
		hFile.get(), nullptr, PAGE_READONLY, 0, 0, nullptr));
	if (!hFileMap) {
		Logger::Get().Win32Error("CreateFileMapping 失败");
		return false;
	}

	wil::unique_mapview_ptr<const uint8_t> fileData((const uint8_t*)MapViewOfFile(
		hFileMap.get(), FILE_MAP_READ, 0, 0, 0));
	if (!fileData) {
		Logger::Get().Win32Error("MapViewOfFile 失败");
		return false;
	}

	const uint8_t* fileEnd = fileData.get() + fileSize;

	// 存在 RIFF 头则 ani，否则为 ico
	if (((RTAG*)fileData.get())->ckID == FOURCC_RIFF) {
		if (!LoadAniFromFileMap(fileData.get(), fileEnd, preferredWidth, frames, frameSequence)) {
			Logger::Get().Error("LoadAniFromFileMap 失败");
			return false;
		}
	} else {
		wil::unique_hcursor hCursor = LoadIcoFromFileMap(fileData.get(), fileEnd, preferredWidth);
		if (hCursor) {
			frames.push_back(std::move(hCursor));
		} else {
			Logger::Get().Error("LoadIcoFromFileMap 失败");
			return false;
		}
	}

	return true;
}

void CursorHelper::TryResolveAnimatedCursor(
	HCURSOR hCursor,
	SmallVectorImpl<HCURSOR>& frames,
	SmallVectorImpl<std::pair<uint32_t, std::chrono::nanoseconds>>& frameSequence
) noexcept {
	assert(hCursor && frames.empty() && frameSequence.empty());

	using FnGetCursorFrameInfo = HCURSOR WINAPI(
		HCURSOR hcur,
		LPWSTR  lpName,
		int     iFrame,
		LPDWORD pjifRate,
		LPINT   pccur
	);

	static FnGetCursorFrameInfo* getCursorFrameInfo = [] {
		return Win32Helper::LoadFunction<FnGetCursorFrameInfo>(L"user32.dll", "GetCursorFrameInfo");
	}();

	if (!getCursorFrameInfo) {
		return;
	}

	// GetCursorFrameInfo 直接返回内部句柄，无需销毁
	DWORD jifRate;
	int stepCount;
	HCURSOR hCursorFrame = getCursorFrameInfo(hCursor, nullptr, 0, &jifRate, &stepCount);
	if (!hCursorFrame || stepCount <= 1) {
		// 失败或不是动态光标
		return;
	}

	frames.reserve(stepCount);
	frameSequence.resize(stepCount);

	frames.push_back(hCursorFrame);
	frameSequence[0] = { 0, JifRateToDuration(jifRate) };

	for (int i = 1; i < stepCount; ++i) {
		hCursorFrame = getCursorFrameInfo(hCursor, nullptr, i, &jifRate, &stepCount);
		if (!hCursorFrame) {
			// 失败时确保结果为空
			frames.clear();
			frameSequence.clear();
			return;
		}

		// 排除重复的帧，用序列表实现
		const uint32_t frameCount = (uint32_t)frames.size();
		uint32_t j = 0;
		for (; j < frameCount; ++j) {
			if (frames[j] == hCursorFrame) {
				break;
			}
		}

		if (j == frameCount) {
			frames.push_back(hCursorFrame);
		}

		frameSequence[i] = { j, JifRateToDuration(jifRate) };
	}
}

}
