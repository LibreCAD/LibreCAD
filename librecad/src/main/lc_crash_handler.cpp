/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD www.librecad.org
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <cstdlib>
#include <cstring>

// #include "main.h"

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#define _XOPEN_SOURCE 700

#ifdef _WIN32
#include <windows.h>
#include <DbgHelp.h>
#ifdef _MSC_VER
#pragma comment(lib, "DbgHelp.lib")
#endif
#elif defined(__linux__) || defined(__APPLE__)
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <cxxabi.h>
#include <sys/time.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <atomic>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

#include "lc_crash_handler.h"

#ifdef _WIN32
namespace {
LONG WINAPI windowsCrashHandler(EXCEPTION_POINTERS* pExceptionInfo)
{
    HANDLE hProcess = GetCurrentProcess();
    SymInitialize(hProcess, nullptr, TRUE);

    char crashFileName[MAX_PATH];
    GetModuleFileNameA(nullptr, crashFileName, MAX_PATH);
    strcat_s(crashFileName, MAX_PATH, ".crash.log");

    FILE* pFile = nullptr;
    fopen_s(&pFile, crashFileName, "w");

    if (pFile != nullptr) {
        fprintf(pFile, "=========================================\n");
        fprintf(pFile, "LibreCAD Crash Report\n");
        // fprintf(pFile, "Version: %s\n", XSTR(LC_VERSION));
        fprintf(pFile, "=========================================\n\n");

        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(pFile, "Crash Time: %04d-%02d-%02d %02d:%02d:%02d\n\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        EXCEPTION_RECORD* pExceptionRecord = pExceptionInfo->ExceptionRecord;
        fprintf(pFile, "Exception Code: 0x%08X\n", pExceptionRecord->ExceptionCode);
        fprintf(pFile, "Exception Address: 0x%p\n", pExceptionRecord->ExceptionAddress);
        fprintf(pFile, "Number of Parameters: %d\n", pExceptionRecord->NumberParameters);

        for (ULONG i = 0; i < pExceptionRecord->NumberParameters; i++) {
            fprintf(pFile, "Parameter %d: 0x%08X\n", i, pExceptionRecord->ExceptionInformation[i]);
        }
        fprintf(pFile, "\n");

        fprintf(pFile, "Call Stack:\n");
        fprintf(pFile, "-----------\n");

        CONTEXT* pContext = pExceptionInfo->ContextRecord;
        STACKFRAME64 stackFrame = {0};

#ifdef _M_IX86
        stackFrame.AddrPC.Offset = pContext->Eip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = pContext->Esp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = pContext->Ebp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
#elif _M_X64
        stackFrame.AddrPC.Offset = pContext->Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = pContext->Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = pContext->Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
#elif defined(_M_ARM64)
        stackFrame.AddrPC.Offset = pContext->Pc;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = pContext->Sp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = pContext->Fp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
#else
#error Unsupported platform
#endif

        HANDLE hThread = GetCurrentThread();
        DWORD machineType = IMAGE_FILE_MACHINE_I386;
#ifdef _M_X64
        machineType = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_ARM64)
        machineType = IMAGE_FILE_MACHINE_ARM64;
#endif

        for (int i = 0; i < 64; i++) {
            if (!StackWalk64(machineType, hProcess, hThread, &stackFrame, pContext,
                            nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
                break;
            }

            if (stackFrame.AddrPC.Offset == 0) {
                break;
            }

            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {0};
            PSYMBOL_INFO pSymbol = reinterpret_cast<PSYMBOL_INFO>(buffer);
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 displacement = 0;
            SymFromAddr(hProcess, stackFrame.AddrPC.Offset, &displacement, pSymbol);

            char moduleName[MAX_PATH] = {0};
            HMODULE hModule = nullptr;
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(stackFrame.AddrPC.Offset), &hModule);
            if (hModule != nullptr) {
                GetModuleFileNameA(hModule, moduleName, MAX_PATH);
            }

            fprintf(pFile, "%2d: 0x%p %s (%s+0x%X)\n",
                    i,
                    reinterpret_cast<void*>(stackFrame.AddrPC.Offset),
                    pSymbol->Name,
                    moduleName,
                    static_cast<DWORD>(stackFrame.AddrPC.Offset -
                    reinterpret_cast<DWORD64>(hModule)));
        }

        fprintf(pFile, "\n=========================================\n");
        fprintf(pFile, "End of Crash Report\n");
        fprintf(pFile, "=========================================\n");
        fclose(pFile);

        printf("\n\n*** LibreCAD has crashed ***\n");
        printf("Crash log saved to: %s\n\n", crashFileName);
    }

    SymCleanup(hProcess);
    return EXCEPTION_EXECUTE_HANDLER;
}
} // anonymous namespace
#elif defined(__linux__) || defined(__APPLE__)

// Async-signal-safe helpers. Anything called from posixSignalHandler() must
// be on POSIX's AS-safe list (open/write/_exit/raise/sigaction/backtrace/
// backtrace_symbols_fd) — no stdio, no malloc, no localtime, no dlopen.

namespace {

// Re-entry guard so a second crashing thread (or a re-fault inside the
// handler) doesn't garble the log or recurse forever.
std::atomic_flag g_inCrashHandler = ATOMIC_FLAG_INIT;

// Dedicated stack so a SIGSEGV from stack overflow can still run the handler.
// Sized at 64 KiB; SIGSTKSZ is too small on some platforms once we use
// backtrace().
constexpr size_t kAltStackSize = 64 * 1024;
alignas(16) unsigned char g_altStack[kAltStackSize];

const char* signalName(int sig)
{
    switch (sig) {
        case SIGHUP:  return "SIGHUP";
        case SIGINT:  return "SIGINT";
        case SIGQUIT: return "SIGQUIT";
        case SIGILL:  return "SIGILL";
        case SIGTRAP: return "SIGTRAP";
        case SIGABRT: return "SIGABRT";
        case SIGBUS:  return "SIGBUS";
        case SIGFPE:  return "SIGFPE";
        case SIGKILL: return "SIGKILL";
        case SIGUSR1: return "SIGUSR1";
        case SIGSEGV: return "SIGSEGV";
        case SIGUSR2: return "SIGUSR2";
        case SIGPIPE: return "SIGPIPE";
        case SIGALRM: return "SIGALRM";
        case SIGTERM: return "SIGTERM";
        default:      return "Unknown";
    }
}

inline void writeAll(int fd, const char* buf, size_t len)
{
    while (len > 0) {
        ssize_t n = write(fd, buf, len);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;
        }
        buf += n;
        len -= static_cast<size_t>(n);
    }
}

inline void writeStr(int fd, const char* s)
{
    size_t n = 0;
    while (s[n] != '\0') ++n;
    writeAll(fd, s, n);
}

// AS-safe unsigned -> decimal. Returns characters written.
size_t writeUnsignedDec(int fd, unsigned long long v, int minWidth = 0)
{
    char buf[32];
    int n = 0;
    if (v == 0) {
        buf[n++] = '0';
    } else {
        while (v > 0 && n < (int)sizeof(buf)) {
            buf[n++] = (char)('0' + (v % 10));
            v /= 10;
        }
    }
    while (n < minWidth && n < (int)sizeof(buf)) buf[n++] = '0';
    char out[32];
    for (int i = 0; i < n; ++i) out[i] = buf[n - 1 - i];
    writeAll(fd, out, (size_t)n);
    return (size_t)n;
}

void writeSignedDec(int fd, long long v)
{
    if (v < 0) {
        writeStr(fd, "-");
        v = -v;
    }
    writeUnsignedDec(fd, (unsigned long long)v);
}

void writeHex(int fd, unsigned long long v, int minWidth = 0)
{
    static const char digits[] = "0123456789abcdef";
    char buf[32];
    int n = 0;
    if (v == 0) {
        buf[n++] = '0';
    } else {
        while (v > 0 && n < (int)sizeof(buf)) {
            buf[n++] = digits[v & 0xF];
            v >>= 4;
        }
    }
    while (n < minWidth && n < (int)sizeof(buf)) buf[n++] = '0';
    char out[32];
    for (int i = 0; i < n; ++i) out[i] = buf[n - 1 - i];
    writeAll(fd, out, (size_t)n);
}

// Build "<exepath>.<pid>.<unixtime>.crash.log" without dynamic allocation.
// Returns false if path can't be obtained or the buffer would overflow.
bool buildCrashFileName(char* out, size_t outSize, pid_t pid, time_t now)
{
    if (outSize < 32) return false;
    size_t base = 0;

#ifdef __linux__
    ssize_t r = readlink("/proc/self/exe", out, outSize - 1);
    if (r > 0) {
        base = (size_t)r;
    } else {
        const char* fallback = "librecad";
        while (fallback[base] && base < outSize - 1) {
            out[base] = fallback[base];
            ++base;
        }
    }
#elif defined(__APPLE__)
    uint32_t size = (uint32_t)outSize;
    if (_NSGetExecutablePath(out, &size) == 0) {
        while (base < outSize && out[base] != '\0') ++base;
    } else {
        const char* fallback = "LibreCAD";
        while (fallback[base] && base < outSize - 1) {
            out[base] = fallback[base];
            ++base;
        }
    }
#endif

    auto append = [&](const char* s) {
        while (*s && base < outSize - 1) out[base++] = *s++;
    };
    auto appendU = [&](unsigned long long v) {
        char tmp[32];
        int n = 0;
        if (v == 0) tmp[n++] = '0';
        while (v > 0 && n < (int)sizeof(tmp)) { tmp[n++] = (char)('0' + (v % 10)); v /= 10; }
        while (n-- > 0 && base < outSize - 1) out[base++] = tmp[n];
    };

    append(".");
    appendU((unsigned long long)pid);
    append(".");
    appendU((unsigned long long)now);
    append(".crash.log");
    if (base >= outSize) return false;
    out[base] = '\0';
    return true;
}

void posixSignalHandler(int sig, siginfo_t* info, [[maybe_unused]] void* context)
{
    // First crasher wins; everyone else falls through to the default handler.
    if (g_inCrashHandler.test_and_set()) {
        signal(sig, SIG_DFL);
        raise(sig);
        return;
    }

    char crashFileName[PATH_MAX];
    crashFileName[0] = '\0';

    struct timeval tv;
    gettimeofday(&tv, nullptr);

    if (!buildCrashFileName(crashFileName, sizeof(crashFileName),
                            getpid(), tv.tv_sec)) {
        const char* fb = "librecad.crash.log";
        size_t i = 0;
        while (fb[i] && i < sizeof(crashFileName) - 1) {
            crashFileName[i] = fb[i];
            ++i;
        }
        crashFileName[i] = '\0';
    }

    int fd = open(crashFileName,
                  O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
                  0600);
    if (fd >= 0) {
        writeStr(fd, "=========================================\n"
                     "LibreCAD Crash Report\n"
                     "=========================================\n\n");

        writeStr(fd, "Crash Time (epoch s.us): ");
        writeUnsignedDec(fd, (unsigned long long)tv.tv_sec);
        writeStr(fd, ".");
        writeUnsignedDec(fd, (unsigned long long)tv.tv_usec, 6);
        writeStr(fd, "\nPID: ");
        writeUnsignedDec(fd, (unsigned long long)getpid());
        writeStr(fd, "\n\nSignal: ");
        writeSignedDec(fd, sig);
        writeStr(fd, " (");
        writeStr(fd, signalName(sig));
        writeStr(fd, ")\n");

        if (info != nullptr) {
            writeStr(fd, "Signal Code: ");
            writeSignedDec(fd, info->si_code);
            writeStr(fd, "\nFault Address: 0x");
            writeHex(fd, (unsigned long long)(uintptr_t)info->si_addr);
            writeStr(fd, "\n");
        }

        writeStr(fd, "\nCall Stack:\n-----------\n");

        void* callstack[64];
        int frames = backtrace(callstack, 64);
        // backtrace_symbols_fd writes directly to fd without allocating.
        backtrace_symbols_fd(callstack, frames, fd);

        writeStr(fd, "\n=========================================\n"
                     "End of Crash Report\n"
                     "=========================================\n");
        close(fd);

        writeStr(STDERR_FILENO, "\n*** LibreCAD has crashed ***\nCrash log: ");
        writeStr(STDERR_FILENO, crashFileName);
        writeStr(STDERR_FILENO, "\n");
    }

    signal(sig, SIG_DFL);
    raise(sig);
}

} // anonymous namespace
#endif

void LC_CrashHandler::install()
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(windowsCrashHandler);
#elif defined(__linux__) || defined(__APPLE__)
    // Install an alternate signal stack so a stack-overflow SIGSEGV can
    // still run the handler instead of re-faulting on the exhausted stack.
    stack_t ss{};
    ss.ss_sp = g_altStack;
    ss.ss_size = sizeof(g_altStack);
    ss.ss_flags = 0;
    sigaltstack(&ss, nullptr);

    struct sigaction sa{};
    sa.sa_sigaction = posixSignalHandler;
    sigemptyset(&sa.sa_mask);
    // SA_ONSTACK: run on g_altStack. SA_RESETHAND: if the handler itself
    // faults with the same signal, the kernel uses the default action
    // (core dump) instead of recursing. SA_NODEFER is intentionally NOT
    // set — the triggering signal stays blocked while the handler runs.
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
#endif
}
