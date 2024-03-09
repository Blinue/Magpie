#include "pch.h"
#include "InferenceBackendBase.h"
#include "StrUtils.h"
#include "Logger.h"

#pragma comment(lib, "onnxruntime.lib")

namespace Magpie::Core {

void ORT_API_CALL InferenceBackendBase::_OrtLog(
	void* /*param*/,
	OrtLoggingLevel severity,
	const char* /*category*/,
	const char* /*logid*/,
	const char* /*code_location*/,
	const char* message
) {
	const char* SEVERITIES[] = {
		"verbose",
		"info",
		"warning",
		"error",
		"fatal"
	};

	std::string log = StrUtils::Concat("[", SEVERITIES[severity], "] ", message);
	if (severity == ORT_LOGGING_LEVEL_INFO) {
		Logger::Get().Info(log);
	} else if (severity == ORT_LOGGING_LEVEL_WARNING) {
		Logger::Get().Warn(log);
	} else {
		Logger::Get().Error(log);
	}

	OutputDebugStringA((log + "\n").c_str());
}

}
