from conans import ConanFile

class Magpie(ConanFile):
	settings = {
		"os": "Windows",
		"compiler": {"Visual Studio": {"version": ["17"]}},
		"build_type": ["Debug", "Release"],
		"arch": ["x86_64", "armv8"]
	}
	requires = [
		"fmt/9.1.0",
		"spdlog/1.11.0",
		"muparser/2.3.4",
		"yas/7.1.0",
		"rapidjson/cci.20220822",
		"zstd/1.5.5",
		"imgui/1.89.4",
		"parallel-hashmap/1.37",
		"kuba-zip/0.2.6"
	]
	generators = "visual_studio"
	default_options = {
		"fmt:header_only": True,
		"spdlog:header_only": True,
		"spdlog:no_exceptions": True
	}

	def imports(self):
		self.copy("imgui_impl_dx11.*", dst="../../../src/Magpie.Core", src="./res/bindings")
