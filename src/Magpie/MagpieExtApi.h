#pragma once
#include <stdint.h>
#include <winerror.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* MagpieExtContext;

enum MagpieLogLevel {
	MagpieLogLevelInfo,
	MagpieLogLevelWarn,
	MagpieLogLevelError,
};

struct MagpieLogSourceLocation {
	const char* fileName;
	int line;
	const char* funcName;
};

enum MagpieColorKind {
	MagpieColorKindSDR,
	MagpieColorKindWCG,
	MagpieColorKindHDR
};

enum MagpieOptionType {
	MagpieOptionTypeBool,
	MagpieOptionTypeInt,
	MagpieOptionTypeFloat,
	MagpieOptionTypeString,
	MagpieOptionTypeSelection
};

struct MagpieBoolOptionDesc {
	uint8_t defaultValue;
};

struct MagpieIntOptionDesc {
	int32_t defaultValue;
	int32_t minValue;
	int32_t maxValue;
	int32_t step;
};

struct MagpieFloatOptionDesc {
	float defaultValue;
	float minValue;
	float maxValue;
	float step;
};

struct MagpieStringOptionDesc {
	const char* defaultValue;
};

struct MagpieSelectionOptionDesc {
	const char** items;
	uint32_t itemCount;
	uint32_t defaultValue;
};

struct MagpieOptionValue {
	const char* id;
	union {
		uint8_t boolValue;
		int32_t intValue;
		float floatValue;
		const char* stringValue;
		uint32_t selectionValue;
	};
};

struct MagpieImageEncoderOptionDesc {
	// 必需
	const char* id;
	// 必需
	const char* displayName;
	const char* description;

	MagpieOptionType type;
	union {
		MagpieBoolOptionDesc boolOption;
		MagpieIntOptionDesc intOption;
		MagpieFloatOptionDesc floatOption;
		MagpieStringOptionDesc stringOption;
		MagpieSelectionOptionDesc selectionOption;
	};
};

struct MagpieImageEncoderDesc {
	// 必需
	const char* id;
	// 必需
	const char* displayName;
	// 必需
	const char* fileExtension;
	const char* description;
	
	const MagpieImageEncoderOptionDesc* optionDescs;
	uint32_t optionCount;

	// 像素格式始终是 R16G16B16_UINT16，但不同的色彩模式下颜色编码不同。
	// SDR - 色域: sRGB    传递函数: sRGB
	// WCG - 色域: BT.2100 传递函数: 伽马 2.4
	// HDR - 色域: BT.2100 传递函数: PQ
	// 白点始终是 D65。
	HRESULT (*encode)(
		const MagpieOptionValue* optionValues,
		uint32_t optionValueCount,
		const char* fileName,
		uint32_t imageWidth,
		uint32_t imageHeight,
		const uint8_t* pixelData,
		uint64_t pixelDataSize,
		uint32_t rowPitch,
		MagpieColorKind colorKind
	);
};

struct MagpieEffectDesc {
	// 必需
	const char* id;
	// 必需
	const char* displayName;
	const char* description;

	// 0 表示可以自由缩放
	uint32_t scaleFactor;
	// 0 表示不限制
	uint32_t minFrameRate;
	uint8_t isOutputAliasingSupported;
};

struct MagpieExtensionDesc {
	// 必需
	const char* displayName;
	const char* author;
	const char* homePageUrl;
	const char* description;

	const MagpieImageEncoderDesc* imageEncoderDescs;
	uint32_t imageEncoderCount;
	const MagpieEffectDesc* effectDescs;
	uint32_t effectCount;
};

struct MagpieHostApi {
	// 记录日志。message 不得为空或空字符串。
	HRESULT (*log)(
		MagpieExtContext context,
		MagpieLogSourceLocation sourceLocation,
		MagpieLogLevel level,
		const char* message
	);
	// 获取应用显示语言。language 不得为空。函数返回时 language 指向语言字符串，该字符串在插
	// 件的生命周期内始终有效。
	HRESULT (*getLanguage)(MagpieExtContext context, const char** language);
	// 注册插件功能。
	HRESULT (*registerExtension)(MagpieExtContext context, const MagpieExtensionDesc* desc);
};

#define MAGPIE_EXT_ENTRY MagpieExtEntry

#ifdef __cplusplus
}
#endif
