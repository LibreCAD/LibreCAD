/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

/*
 * Fuzz / ASan / UBSan entry point on dxfRW::read.
 *
 * The public reader detects ASCII versus binary DXF from the file sentinel,
 * so each input is passed through that detection path instead of being
 * interpreted by a test-only parser. The bool result is intentionally ignored:
 * malformed files are expected to fail, while crashes and undefined behavior
 * are the findings this harness is designed to expose.
 *
 * This is deliberately not wired into the normal CMake/qmake build. It only
 * depends on libdxfrw and can be built with the same source list as the DWG
 * harness:
 *
 *   clang++ -std=c++17 -g -O1 \
 *     -fsanitize=fuzzer,address,undefined \
 *     -I libraries/libdxfrw/src \
 *     libraries/libdxfrw/tests/fuzz/fuzz_dxf_read.cpp \
 *     libraries/libdxfrw/src/[!.]*.cpp libraries/libdxfrw/src/intern/[!.]*.cpp \
 *     -o /tmp/fuzz_dxf_read
 *   /tmp/fuzz_dxf_read -max_total_time=120 /tmp/dxf_seeds
 *
 * For toolchains without libFuzzer, add -DFUZZ_STANDALONE_MAIN and replace
 * -fsanitize=fuzzer,address,undefined with -fsanitize=address,undefined; the
 * resulting driver replays each path passed on argv.
 */

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <unistd.h>  // close, mkstemp, write

#include "fuzz_null_interface.h"
#include "libdxfrw.h"

namespace {

bool writeAll(int fd, const std::uint8_t *data, std::size_t size) {
    std::size_t written = 0;
    while (written < size) {
        const ssize_t count = ::write(fd, data + written, size - written);
        if (count <= 0)
            return false;
        written += static_cast<std::size_t>(count);
    }
    return true;
}

void readOnce(const std::uint8_t *data, std::size_t size) {
    char tmpl[] = "/tmp/fuzz_dxf_XXXXXX";
    const int fd = mkstemp(tmpl);
    if (fd < 0)
        return;

    const bool written = size == 0 || writeAll(fd, data, size);
    ::close(fd);
    if (written) {
        dxfRW rw(tmpl);
        FuzzNullInterface iface;
        (void)rw.read(&iface, /*ext=*/false);
    }
    std::remove(tmpl);
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data,
                                       std::size_t size) {
    readOnce(data, size);
    return 0;
}

#ifdef FUZZ_STANDALONE_MAIN
int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        std::FILE *file = std::fopen(argv[i], "rb");
        if (file == nullptr) {
            std::fprintf(stderr, "skip (open failed): %s\n", argv[i]);
            continue;
        }
        std::fseek(file, 0, SEEK_END);
        const long length = std::ftell(file);
        std::fseek(file, 0, SEEK_SET);
        std::vector<std::uint8_t> bytes(
            length > 0 ? static_cast<std::size_t>(length) : 0);
        if (!bytes.empty()) {
            const std::size_t read =
                std::fread(bytes.data(), 1, bytes.size(), file);
            bytes.resize(read);
        }
        std::fclose(file);
        readOnce(bytes.data(), bytes.size());
        std::fprintf(stderr, "ok: %s (%zu bytes)\n", argv[i], bytes.size());
    }
    return 0;
}
#endif
