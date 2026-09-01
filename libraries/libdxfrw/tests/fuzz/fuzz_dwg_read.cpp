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
 * Phase 1 (1.6) — libFuzzer / ASan / UBSan entry point on dwgRW::read.
 *
 * dwgRW::read has no in-memory entry (libdwgr.cpp opens a std::ifstream via
 * openFile), so each iteration writes the fuzz input to a temp file, reads it
 * back through libdxfrw with a no-op DRW_Interface, and unlinks the file.
 * Only crashes / UB matter; the bool return is ignored.
 *
 * This is deliberately NOT wired into the LibreCAD CMake/qmake build (keep the
 * fuzzer toolchain out of the normal build). It depends only on libdxfrw,
 * which has no Qt dependency.
 *
 * --- libFuzzer build (CI / OSS-Fuzz / Linux clang with the fuzzer runtime) ---
 *   clang++ -std=c++17 -g -O1 \
 *     -fsanitize=fuzzer,address,undefined \
 *     -I libraries/libdxfrw/src \
 *     libraries/libdxfrw/tests/fuzz/fuzz_dwg_read.cpp \
 *     libraries/libdxfrw/src/[!.]*.cpp libraries/libdxfrw/src/intern/[!.]*.cpp \
 *     -o /tmp/fuzz_dwg_read
 *   # seed from the corpora:
 *   mkdir -p /tmp/dwg_seeds
 *   cp ~/doc/dwg/[!.]*.dwg ~/doc/dwg2/[!.]*.dwg ~/dev/dwg_samples/[!.]*.dwg /tmp/dwg_seeds/ 2>/dev/null
 *   /tmp/fuzz_dwg_read -max_total_time=120 /tmp/dwg_seeds
 *
 * --- Standalone replay driver (verifiable on toolchains WITHOUT libFuzzer,
 *     e.g. Apple clang which ships no libclang_rt.fuzzer) ---
 *   clang++ -std=c++17 -g -O1 -DFUZZ_STANDALONE_MAIN \
 *     -fsanitize=address,undefined \
 *     -I libraries/libdxfrw/src \
 *     libraries/libdxfrw/tests/fuzz/fuzz_dwg_read.cpp \
 *     libraries/libdxfrw/src/[!.]*.cpp libraries/libdxfrw/src/intern/[!.]*.cpp \
 *     -o /tmp/replay_dwg_read
 *   /tmp/replay_dwg_read ~/doc/dwg2/[!.]*.dwg     # replays each file through read()
 *
 * The standalone driver runs the exact same LLVMFuzzerTestOneInput body over
 * each file's bytes, so a clean ASan/UBSan run over the corpus there is strong
 * evidence the libFuzzer build is sound where the runtime is available.
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <unistd.h>  // mkstemp/write/close

#include "fuzz_null_interface.h"
#include "libdwgr.h"

namespace {

// Write `data` to a fresh temp file, read it through dwgRW, unlink it.
void readOnce(const uint8_t* data, size_t size) {
    char tmpl[] = "/tmp/fuzz_dwg_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0)
        return;
    if (size > 0) {
        // Best-effort write; partial writes still produce a (truncated) input.
        ssize_t w = ::write(fd, data, size);
        (void)w;
    }
    ::close(fd);

    {
        dwgRW rw(tmpl);
        FuzzNullInterface iface;
        (void)rw.read(&iface, /*ext=*/false);  // ignore result; crashes/UB matter
    }
    ::remove(tmpl);
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    readOnce(data, size);
    return 0;
}

#ifdef FUZZ_STANDALONE_MAIN
// Replay driver for toolchains without the libFuzzer runtime: feed each file
// path on argv through the same body the fuzzer exercises.
int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::FILE* f = std::fopen(argv[i], "rb");
        if (f == nullptr) {
            std::fprintf(stderr, "skip (open failed): %s\n", argv[i]);
            continue;
        }
        std::fseek(f, 0, SEEK_END);
        long n = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        std::vector<uint8_t> buf(n > 0 ? static_cast<size_t>(n) : 0);
        if (!buf.empty()) {
            size_t got = std::fread(buf.data(), 1, buf.size(), f);
            buf.resize(got);
        }
        std::fclose(f);
        readOnce(buf.data(), buf.size());
        std::fprintf(stderr, "ok: %s (%zu bytes)\n", argv[i], buf.size());
    }
    return 0;
}
#endif
