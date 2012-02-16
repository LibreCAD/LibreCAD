/*
 * Workaround: Emulate Qt 4.4 functions.
 */

#include "emu_qt44.h"

#if QT_VERSION < 0x040400

#ifdef _WIN32
#include <shlobj.h>
#else
#error Don't know how to emulate Qt 4.4 here ...
#endif

#define PATHBUFSIZE MAX_PATH

static QString emu_qt44_location(INT csidl)
{
    TCHAR szPath[PATHBUFSIZE];
    HRESULT hRes;

    hRes = SHGetFolderPath(NULL, 
        csidl, 
        NULL, 
        0, 
        szPath);
    if (SUCCEEDED(hRes)) {
#if defined(_UNICODE) || defined(UNICODE)
        INT size = wcslen(szPath);
        return QString::fromUtf16(szPath, size);
#elif defined (_MBCS) || defined(MBCS)
        INT size = _mbslen(szPath);
        return QString::fromUtf8(szPath);
#else
        INT size = strlen(szPath);
        return QString::fromLatin1(szPath);
#endif
    } else {
        return "";
    }
}

QString emu_qt44_storageLocationDocuments(void)
{
    return emu_qt44_location(CSIDL_PERSONAL);
}

QString emu_qt44_storageLocationData(void)
{
    return emu_qt44_location(CSIDL_LOCAL_APPDATA);
}

#endif // QT_VERSION 0x040400
