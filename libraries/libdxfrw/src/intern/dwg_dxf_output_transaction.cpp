/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * ********************************************************************************
 */

#include "dwg_dxf_output_transaction.h"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <memory>
#include <streambuf>
#include <string>

#if defined(_WIN32)
#  include <fcntl.h>
#  include <io.h>
#  include <sys/stat.h>
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

namespace {
struct FileIdentity {
#if defined(_WIN32)
    DWORD volume = 0, high = 0, low = 0;
#else
    dev_t device = 0;
    ino_t inode = 0;
#endif
};

bool descriptorIdentity(int descriptor, FileIdentity& identity) noexcept {
#if defined(_WIN32)
    const auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(descriptor));
    BY_HANDLE_FILE_INFORMATION info{};
    if (handle == INVALID_HANDLE_VALUE || !GetFileInformationByHandle(handle, &info)
        || (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) return false;
    identity = {info.dwVolumeSerialNumber, info.nFileIndexHigh, info.nFileIndexLow};
#else
    struct stat info{};
    if (::fstat(descriptor, &info) != 0 || !S_ISREG(info.st_mode)) return false;
    identity = {info.st_dev, info.st_ino};
#endif
    return true;
}

bool samePathIdentity(const std::filesystem::path& path, const FileIdentity& identity) noexcept {
#if defined(_WIN32)
    const HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    BY_HANDLE_FILE_INFORMATION info{};
    const bool valid = GetFileInformationByHandle(handle, &info) != 0;
    CloseHandle(handle);
    return valid && !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        && info.dwVolumeSerialNumber == identity.volume && info.nFileIndexHigh == identity.high
        && info.nFileIndexLow == identity.low;
#else
    struct stat info{};
    return ::lstat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode)
        && info.st_dev == identity.device && info.st_ino == identity.inode;
#endif
}

class FileBuffer final : public std::streambuf {
public:
    explicit FileBuffer(std::FILE* file) : m_file(file) {}
    ~FileBuffer() override { close(); }

    bool close() noexcept {
        if (!m_file) return true;
        auto* file = m_file;
        m_file = nullptr;
        return std::fclose(file) == 0;
    }

#if !defined(_WIN32)
    int retainDescriptor() const noexcept {
        if (!m_file) return -1;
#  if defined(F_DUPFD_CLOEXEC)
        return ::fcntl(::fileno(m_file), F_DUPFD_CLOEXEC, 0);
#  else
        const int descriptor = ::dup(::fileno(m_file));
        if (descriptor < 0) return -1;
        if (::fcntl(descriptor, F_SETFD, FD_CLOEXEC) == 0) return descriptor;
        ::close(descriptor);
        return -1;
#  endif
    }
#endif

protected:
    std::streamsize xsputn(const char* data, std::streamsize count) override {
        if (!m_file || count < 0 || static_cast<std::uintmax_t>(count) > std::numeric_limits<std::size_t>::max()) return 0;
        return static_cast<std::streamsize>(std::fwrite(data, 1, static_cast<std::size_t>(count), m_file));
    }

    int_type overflow(int_type character) override {
        if (traits_type::eq_int_type(character, traits_type::eof())) return traits_type::not_eof(character);
        return m_file && std::fputc(traits_type::to_char_type(character), m_file) != EOF
            ? character : traits_type::eof();
    }

    int sync() override { return m_file && std::fflush(m_file) == 0 ? 0 : -1; }

    pos_type seekoff(off_type offset, std::ios_base::seekdir direction, std::ios_base::openmode mode) override {
        if (!m_file || !(mode & std::ios_base::out)) return pos_type(off_type(-1));
        int origin = SEEK_SET;
        if (direction == std::ios_base::cur) origin = SEEK_CUR;
        else if (direction == std::ios_base::end) origin = SEEK_END;
        else if (direction != std::ios_base::beg) return pos_type(off_type(-1));
#if defined(_WIN32)
        if (_fseeki64(m_file, offset, origin) != 0) return pos_type(off_type(-1));
        const auto position = _ftelli64(m_file);
#else
        if (offset < std::numeric_limits<off_t>::min() || offset > std::numeric_limits<off_t>::max()
            || ::fseeko(m_file, static_cast<off_t>(offset), origin) != 0) return pos_type(off_type(-1));
        const auto position = ::ftello(m_file);
#endif
        return position < 0 ? pos_type(off_type(-1)) : pos_type(position);
    }

    pos_type seekpos(pos_type position, std::ios_base::openmode mode) override {
        return seekoff(off_type(position), std::ios_base::beg, mode);
    }

private:
    std::FILE* m_file;
};
}

struct DwgDxfOutputTransaction::Stage {
    Stage(std::FILE* file, FileIdentity fileIdentity) : buffer(file), identity(fileIdentity) {}
    ~Stage() {
#if !defined(_WIN32)
        if (retainedDescriptor >= 0) ::close(retainedDescriptor);
#endif
    }
    bool retainIdentity() noexcept {
#if !defined(_WIN32)
        retainedDescriptor = buffer.retainDescriptor();
        return retainedDescriptor >= 0;
#else
        return true;
#endif
    }
    FileBuffer buffer;
    FileIdentity identity;
#if !defined(_WIN32)
    int retainedDescriptor = -1;
#endif
};

DwgDxfOutputTransaction::DwgDxfOutputTransaction(
    const std::string& target, std::ios::openmode mode)
    : m_target(std::filesystem::path(target)), m_mode(mode) {}

DwgDxfOutputTransaction::~DwgDxfOutputTransaction() {
    if (!m_committed)
        abort();
}

bool DwgDxfOutputTransaction::createExclusiveTemporary() {
    if (m_target.empty())
        return false;

    const std::filesystem::path directory =
        m_target.parent_path().empty() ? std::filesystem::path(".")
                                       : m_target.parent_path();
    const std::string name = m_target.filename().string();
    if (name.empty())
        return false;

    const auto stamp = std::chrono::steady_clock::now()
                           .time_since_epoch().count();
    for (std::uint32_t attempt = 0; attempt != 128; ++attempt) {
        m_temporary = directory /
            (name + ".libdxfrw-" + std::to_string(stamp) + "-"
             + std::to_string(attempt));
#if defined(_WIN32)
        const int descriptor = _wopen(
            m_temporary.c_str(), _O_CREAT | _O_EXCL | _O_WRONLY | _O_NOINHERIT
                | ((m_mode & std::ios::binary) ? _O_BINARY : _O_TEXT),
            _S_IREAD | _S_IWRITE);
#else
        int flags = O_CREAT | O_EXCL | O_WRONLY;
#  if defined(O_CLOEXEC)
        flags |= O_CLOEXEC;
#  endif
        const int descriptor = ::open(
            m_temporary.c_str(), flags, 0600);
#endif
        if (descriptor < 0)
            continue;
        FileIdentity identity;
        const bool identified = descriptorIdentity(descriptor, identity);
#if defined(_WIN32)
        std::FILE* file = identified ? _fdopen(descriptor, (m_mode & std::ios::binary) ? "wb" : "w") : nullptr;
#else
        std::FILE* file = identified ? ::fdopen(descriptor, (m_mode & std::ios::binary) ? "wb" : "w") : nullptr;
#endif
        if (!file) {
#if defined(_WIN32)
            _close(descriptor);
#else
            ::close(descriptor);
#endif
            if (identified && samePathIdentity(m_temporary, identity)) {
                std::error_code ignored;
                std::filesystem::remove(m_temporary, ignored);
            }
            m_temporary.clear();
            return false;
        }
        std::unique_ptr<std::FILE, decltype(&std::fclose)> owner(file, &std::fclose);
        try {
            m_stage = std::make_unique<Stage>(file, identity);
        } catch (...) {
            owner.reset();
            if (samePathIdentity(m_temporary, identity)) {
                std::error_code ignored;
                std::filesystem::remove(m_temporary, ignored);
            }
            m_temporary.clear();
            throw;
        }
        owner.release();
        return true;
    }
    m_temporary.clear();
    return false;
}

bool DwgDxfOutputTransaction::open() {
    if (m_committed || m_finalizing || m_open || m_stage)
        return false;
    try {
        if (!createExclusiveTemporary()) return false;
        if (m_testAfterCreate) m_testAfterCreate(m_temporary);
        m_stream.rdbuf(&m_stage->buffer);
        m_stream.clear();
        m_open = true;
        return true;
    } catch (...) {
        abort();
        return false;
    }
}

bool DwgDxfOutputTransaction::stageMatchesPath() const noexcept {
    return m_stage && !m_temporary.empty() && samePathIdentity(m_temporary, m_stage->identity);
}

bool DwgDxfOutputTransaction::publish() {
#if defined(_WIN32)
    return MoveFileExW(m_temporary.c_str(), m_target.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)
        != 0;
#else
    std::error_code error;
    std::filesystem::rename(m_temporary, m_target, error);
    return !error;
#endif
}

bool DwgDxfOutputTransaction::finishWrite() {
    if (m_committed || m_finalizing || !m_open)
        return false;
    m_finalizing = true;
    m_stream.flush();
    if (!m_stream.good()) {
        abort();
        return false;
    }
    if (!m_stage->retainIdentity()) {
        abort();
        return false;
    }
    m_stream.rdbuf(nullptr);
    m_open = false;
    if (!m_stage->buffer.close() || !stageMatchesPath()) {
        abort();
        return false;
    }
    return true;
}

bool DwgDxfOutputTransaction::finishPublish() {
    if (!stageMatchesPath() || !publish()) {
        abort();
        return false;
    }
    m_committed = true;
    m_temporary.clear();
    m_stage.reset();
    return true;
}

bool DwgDxfOutputTransaction::commit() {
    return finishWrite() && finishPublish();
}

bool DwgDxfOutputTransaction::commitVerified(
    StagedFileVerifier verifier, void* context) {
    if (m_committed || m_finalizing)
        return false;
    if (!verifier) {
        abort();
        return false;
    }
    if (!finishWrite())
        return false;
    bool verified = false;
    try {
        verified = verifier(m_temporary, context);
    } catch (...) {
        // A failed semantic/reread check must never publish a partial save.
    }
    if (!verified) {
        abort();
        return false;
    }
    return finishPublish();
}

void DwgDxfOutputTransaction::abort() noexcept {
    try {
        m_stream.exceptions(std::ios::goodbit);
        m_stream.rdbuf(nullptr);
    } catch (...) {}
    m_open = false;
    if (m_stage) m_stage->buffer.close();
    if (stageMatchesPath()) {
        std::error_code ignored;
        std::filesystem::remove(m_temporary, ignored);
    }
    m_stage.reset();
    m_temporary.clear();
}
