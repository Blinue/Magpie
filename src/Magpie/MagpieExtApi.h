#pragma once
#include <stdint.h>
#include <winerror.h>

#ifdef __cplusplus
extern "C" {
#endif

enum MagpieOptionType {
	MagpieOptionTypeBool,
	MagpieOptionTypeInt,
	MagpieOptionTypeFloat,
	MagpieOptionTypeString
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

struct MagpieOptionValue {
	const char* id;
	union {
		uint8_t boolValue;
		int32_t intValue;
		float floatValue;
		const char* stringValue;
	};
};

struct MagpieImageEncoderOptionDesc {
	const char* id;
	const char* displayName;
	const char* description;

	MagpieOptionType type;
	union {
		MagpieBoolOptionDesc boolOption;
		MagpieIntOptionDesc intOption;
		MagpieFloatOptionDesc floatOption;
		MagpieStringOptionDesc stringOption;
	};
};

struct MagpieImageEncoderDesc {
	const char* id;
	const char* displayName;
	const char* description;
	const char* fileExtension;

	const MagpieImageEncoderOptionDesc* optionDescs;
	uint32_t optionCount;

	HRESULT (*encode)(
		const MagpieOptionValue* optionValues,
		uint32_t optionValueCount,
		const char* fileName,
		uint32_t imageWidth,
		uint32_t imageHeight,
		const uint8_t* pixelData,
		uint64_t pixelDataSize,
		uint32_t rowPitch
	);
};

struct MagpieEffectDesc {
	const char* id;
	const char* displayName;
	const char* description;

	// 0 表示可以自由缩放
	uint32_t scaleFactor;
	// 0 表示不限制
	uint32_t minFrameRate;
	uint8_t isOutputAliasingSupported;
};

struct MagpieExtensionDesc {
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
	HRESULT (*getLanguage)(char* language, uint32_t bufferSize);
	HRESULT (*log)(const char* message);
	HRESULT (*registerExtension)(const MagpieExtensionDesc* desc);
};

#define MAGPIE_EXT_ENTRY MagpieExtEntry

#ifdef __cplusplus
}
#endif
