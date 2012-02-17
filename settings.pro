win32 {
    include(settings_windows.pro)
}
unix {
    macx {
        include(settings_macx.pro)
    } else {
        include(settings_linux.pro)
    }
}
