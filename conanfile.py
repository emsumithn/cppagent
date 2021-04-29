from conans import ConanFile, CMake, tools

class CppAgentConan(ConanFile):
    name = "mtconnect_cppagent"
    version = "1.7"
    generators = "cmake", "xcode", "visual_studio"
    url = "https://github.com/mtconnect/cppagent_dev.git"
    license = "Apache License 2.0"
    settings = "os", "compiler", "arch", "build_type", "arch_build"
    
    requires = "boost/1.75.0", "libxml2/2.9.10", "date/2.4.1", "nlohmann_json/3.9.1", "mqtt_cpp/7.0.1"
    build_policy = "missing"
    default_options = {
        "boost:shared": True,
        "boost:bzip2": False,
        "boost:lzma": False,
        "boost:numa": False,
        "boost:without_wave": True,
        "boost:without_test": True,
        "boost:without_json": True,
        "boost:without_mpi": True,
        "boost:without_stacktrace": True,
        "boost:extra_b2_flags": "-j 2 -d +1 cxxstd=17",

        "libxml2:http": False,
        "libxml2:ftp": False,
        "libxml2:iconv": False,
        "libxml2:zlib": False
        }

    def configure(self):
        if "libcxx" in self.settings.compiler.fields and self.settings.compiler.libcxx == "libstdc++":
            raise Exception("This package is only compatible with libstdc++11, add -s compiler.libcxx=libstdc++11")
        self.settings.compiler.cppstd = 17
        
        if self.settings.os == "Windows" and (not self.settings.compiler.toolset or self.settings.compiler.toolset != "v140_xp"):
            self.options["boost"].extra_b2_flags = self.options["boost"].extra_b2_flags + " define=BOOST_USE_WINAPI_VERSION=0x0600"
        elif self.settings.os == "Windows":
            self.options["boost"].extra_b2_flags = self.options["boost"].extra_b2_flags + " define=BOOST_USE_WINAPI_VERSION=0x0501"
            self.options["boost"].i18n_backend = "icu"

    def requirements(self):
        if self.settings.os != "Windows" or (not self.settings.compiler.toolset or self.settings.compiler.toolset != "v140_xp"):
            self.requires("gtest/1.10.0")

    def build(self):
        cmake = CMake(self, build_type=self.settings.get_safe("build_type", default="Release"))
        if self.settings.os == "Windows" and (self.settings.compiler.toolset and self.settings.compiler.toolset == "v140_xp"):
            cmake.definitions["AGENT_ENABLE_UNITTESTS"] = "OFF"
            
        cmake.configure()
        cmake.build()
        if self.settings.os == "Windows" and (self.settings.compiler.toolset and self.settings.compiler.toolset == "v140_xp"):
            cmake.test()

    def imports(self):
        self.copy("*.dll", "bin", "bin")
        self.copy("*.so", "bin", "bin")
        self.copy("*.dylib", "lib", "lib")

        
    
    
