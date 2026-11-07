/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include "lc_fileidentity.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <limits>
#ifdef Q_OS_UNIX
#include <sys/stat.h>
#endif
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#endif

bool LC_FileIdentity::NativeState::operator==(const NativeState& other) const noexcept {
    return valid && other.valid && device == other.device && inode == other.inode && size == other.size &&
        modifiedSeconds == other.modifiedSeconds && modifiedNanos == other.modifiedNanos &&
        changedSeconds == other.changedSeconds && changedNanos == other.changedNanos;
}

bool LC_FileIdentity::NativeState::sameObject(const NativeState& other) const noexcept {
    return valid && other.valid && device == other.device && inode == other.inode;
}

namespace {
#ifdef Q_OS_UNIX
LC_FileIdentity::NativeState nativeState(const struct stat& info) noexcept {
    LC_FileIdentity::NativeState state;
    if (!S_ISREG(info.st_mode) || info.st_size < 0) return state;
    state.valid = true; state.device = quint64(info.st_dev); state.inode = quint64(info.st_ino);
    state.size = qint64(info.st_size);
#if defined(Q_OS_DARWIN)
    state.modifiedSeconds = info.st_mtimespec.tv_sec; state.modifiedNanos = info.st_mtimespec.tv_nsec;
    state.changedSeconds = info.st_ctimespec.tv_sec; state.changedNanos = info.st_ctimespec.tv_nsec;
#elif defined(Q_OS_LINUX)
    state.modifiedSeconds = info.st_mtim.tv_sec; state.modifiedNanos = info.st_mtim.tv_nsec;
    state.changedSeconds = info.st_ctim.tv_sec; state.changedNanos = info.st_ctim.tv_nsec;
#else
    state.modifiedSeconds = info.st_mtime; state.changedSeconds = info.st_ctime;
#endif
    return state;
}
#endif

#ifdef Q_OS_WIN
LC_FileIdentity::NativeState nativeState(HANDLE handle) noexcept {
    LC_FileIdentity::NativeState state;
    BY_HANDLE_FILE_INFORMATION info{};
    FILE_BASIC_INFO basic{};
    if (handle == INVALID_HANDLE_VALUE || !GetFileInformationByHandle(handle, &info) ||
        (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
        !GetFileInformationByHandleEx(handle, FileBasicInfo, &basic, static_cast<DWORD>(sizeof(basic)))) return state;
    const quint64 fileSize = (quint64(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    if (fileSize > quint64(std::numeric_limits<qint64>::max())) return state;
    state.valid = true;
    state.device = info.dwVolumeSerialNumber;
    state.inode = (quint64(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
    state.size = qint64(fileSize);
    constexpr qint64 WindowsEpochTicks = 116444736000000000LL;
    constexpr qint64 TicksPerSecond = 10000000LL;
    const auto splitTime = [=](qint64 ticks, qint64& seconds, qint64& nanos) {
        ticks -= WindowsEpochTicks;
        seconds = ticks / TicksPerSecond;
        qint64 remainder = ticks % TicksPerSecond;
        if (remainder < 0) { --seconds; remainder += TicksPerSecond; }
        nanos = remainder * 100;
    };
    splitTime(basic.LastWriteTime.QuadPart, state.modifiedSeconds, state.modifiedNanos);
    splitTime(basic.ChangeTime.QuadPart, state.changedSeconds, state.changedNanos);
    return state;
}

LC_FileIdentity::NativeState nativePathState(const QString& path) {
    const std::wstring name = path.toStdWString();
    const HANDLE handle = CreateFileW(name.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return {};
    const auto state = nativeState(handle);
    CloseHandle(handle);
    return state;
}
#endif
}

LC_FileIdentity LC_FileIdentity::capture(const QString& file) {
    QFileInfo info(file);
    LC_FileIdentity identity;
    if (!info.exists() || !info.isFile()) return identity;
    identity.sourcePath = QDir::cleanPath(info.absoluteFilePath());
    identity.canonicalPath = info.canonicalFilePath();
    identity.size = info.size();
    identity.modified = info.lastModified();
    identity.metadataChanged = info.metadataChangeTime();
    identity.birth = info.birthTime();
#ifdef Q_OS_UNIX
    struct stat native{};
    const auto encoded = QFile::encodeName(file);
    if (::stat(encoded.constData(), &native) == 0) identity.native = nativeState(native);
#endif
#ifdef Q_OS_WIN
    identity.native = nativePathState(file);
#endif
    identity.valid = !identity.canonicalPath.isEmpty() && identity.size >= 0 && identity.modified.isValid();
    return identity;
}

bool LC_FileIdentity::matches(const QString& file) const {
    const auto current = capture(file);
    return valid && current.valid && canonicalPath == current.canonicalPath && size == current.size &&
        modified == current.modified && (!metadataChanged.isValid() || metadataChanged == current.metadataChanged) &&
        (!birth.isValid() || birth == current.birth) && (!native.valid || native == current.native);
}

bool LC_FileIdentity::matchesDescriptor(int descriptor) const noexcept {
#ifdef Q_OS_UNIX
    struct stat info{};
    return descriptor >= 0 && native.valid && ::fstat(descriptor, &info) == 0 && native == nativeState(info);
#elif defined(Q_OS_WIN)
    if (descriptor < 0 || !native.valid) return false;
    const auto raw = _get_osfhandle(descriptor);
    return raw != -1 && native == nativeState(reinterpret_cast<HANDLE>(raw));
#else
    Q_UNUSED(descriptor);
    return false;
#endif
}

bool LC_FileIdentity::refersToSameObject(const QString& file) const {
    if (!valid || file.isEmpty()) return false;
    const auto current = capture(file);
    if (!current.valid) return false;
    if (native.valid && current.native.valid) return native.sameObject(current.native);
    return canonicalPath == current.canonicalPath;
}

bool LC_FileIdentity::refersToPath(const QString& file) const {
    if (!valid || sourcePath.isEmpty() || file.isEmpty()) return false;
    const QString currentPath = QDir::cleanPath(QFileInfo(file).absoluteFilePath());
#ifdef Q_OS_WIN
    return sourcePath.compare(currentPath, Qt::CaseInsensitive) == 0;
#else
    return sourcePath == currentPath;
#endif
}
