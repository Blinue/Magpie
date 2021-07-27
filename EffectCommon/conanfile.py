from conans import ConanFile, CMake

class EffectCommonConan(ConanFile):
	requires = "fmt/8.0.1", "nlohmann_json/3.9.1"
	generators = "visual_studio"
	