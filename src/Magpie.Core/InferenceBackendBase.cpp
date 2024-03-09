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
		OutputDebugStringA((log + "\n").c_str());
	} else if (severity == ORT_LOGGING_LEVEL_WARNING) {
		Logger::Get().Warn(log);
	} else {
		Logger::Get().Error(log);
	}
}

static bool IsTensorShapeValid(const Ort::ConstTensorTypeAndShapeInfo& tensorInfo) {
	// 输入输出维度应是 [-1,3,-1,-1]
	std::vector<int64_t> dimensions = tensorInfo.GetShape();
	return dimensions.size() == 4 && dimensions[0] == -1 &&
		dimensions[1] == 3 && dimensions[2] == -1 && dimensions[3] == -1;
}

bool InferenceBackendBase::_IsValidModel(const Ort::Session& session, bool& isFP16Data) {
	if (session.GetInputCount() != 1 || session.GetOutputCount() != 1) {
		Logger::Get().Error("不支持有多个输入/输出的模型");
		return false;
	}

	// 必须在 inputTypeInfo 的生命周期内使用 inputTensorInfo
	Ort::TypeInfo inputTypeInfo = session.GetInputTypeInfo(0);
	Ort::ConstTensorTypeAndShapeInfo inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();

	ONNXTensorElementDataType dataType = inputTensorInfo.GetElementType();
	if (dataType != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16 && dataType != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
		Logger::Get().Error("不支持 float16 和 float 之外的输入数据类型");
		return false;
	}

	isFP16Data = dataType == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16;

	Ort::TypeInfo outputInfo = session.GetOutputTypeInfo(0);
	Ort::ConstTensorTypeAndShapeInfo outputTensorInfo = outputInfo.GetTensorTypeAndShapeInfo();
	if (outputInfo.GetTensorTypeAndShapeInfo().GetElementType() != dataType) {
		Logger::Get().Error("不支持输入和输出数据类型不同的模型");
		return false;
	}

	if (!IsTensorShapeValid(inputTensorInfo)) {
		Logger::Get().Error("不支持的输入数据格式");
		return false;
	}

	if (!IsTensorShapeValid(outputTensorInfo)) {
		Logger::Get().Error("不支持的输出数据格式");
		return false;
	}
	
	return true;
}

}
