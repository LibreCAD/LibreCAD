
unix {
    macx {
        INSTALLDIR = ../../

        # Homebrew Qt can retain the macOS SDK path from the machine it was
        # packaged on. If that exact SDK is not installed locally, qmake emits
        # a stale -isysroot and clang cannot find even standard C++ headers.
        LC_MACOSX_SDK_PATH = $$system(xcrun --sdk macosx --show-sdk-path 2>/dev/null)
        !isEmpty(LC_MACOSX_SDK_PATH):exists($$LC_MACOSX_SDK_PATH) {
            QMAKE_MAC_SDK = macosx
            QMAKE_MAC_SDK_PATH = $$LC_MACOSX_SDK_PATH
            QMAKE_MAC_SDK.macosx.Path = $$LC_MACOSX_SDK_PATH

            QMAKE_CFLAGS = $$replace(QMAKE_CFLAGS, "-isysroot[ ]+[^ ]+", "-isysroot $$LC_MACOSX_SDK_PATH")
            QMAKE_CXXFLAGS = $$replace(QMAKE_CXXFLAGS, "-isysroot[ ]+[^ ]+", "-isysroot $$LC_MACOSX_SDK_PATH")
            QMAKE_LFLAGS = $$replace(QMAKE_LFLAGS, "-isysroot[ ]+[^ ]+", "-isysroot $$LC_MACOSX_SDK_PATH")
        }
    } else {
        INSTALLDIR = ../../unix
    }
}

win32 {
    INSTALLDIR = ../../windows
}
