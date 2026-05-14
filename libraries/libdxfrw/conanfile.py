from conans import ConanFile, CMake


class LibdxfrwConan(ConanFile):
    name = "libdxfrw"
    version = "1.0.0"
    license = "GPL2"
    url = "https://github.com/LibreCAD/libdxfrw"
    description = "C++ library to read/write DXF files in binary and ascii form and to read DWG from r14 to v2015"
    topics = ("dxf", "dwg")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    generators = "cmake"

    def source(self):
        self.run("git clone --branch " + self.version + " https://github.com/LibreCAD/libdxfrw.git")

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure(source_folder="libdxfrw")
        return cmake

    def package_info(self):
        self.cpp_info.libs = ["dxfrw"]
