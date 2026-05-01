/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD www.librecad.org
** Copyright (C) 2026 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "lc_crashhandler.h"

#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <mutex>

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QString>
#include <QSysInfo>
#include <QtGlobal>

#ifdef Q_OS_WIN
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <dbghelp.h>
#  include <fcntl.h>
#  include <io.h>
#  include <process.h>
#  include <sys/stat.h>
#else
#  include <csignal>
#  include <execinfo.h>
#  include <fcntl.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#define LC_CRASH_STR_(x) #x
#define LC_CRASH_STR(x) LC_CRASH_STR_(x)
#ifndef LC_VERSION
#  define LC_VERSION unknown
#endif

namespace {

constexpr int kMaxFrames = 128;
constexpr size_t kAltStackSize = 64 * 1024;
constexpr size_t kHeaderBufSize = 4096;
constexpr size_t kPathBufSize = 1024;

int g_logFd = -1;
char g_logPath[kPathBufSize] = {0};
char g_header[kHeaderBufSize] = {0};
size_t g_headerLen = 0;
std::atomic<int> g_inHandler{0};

#ifndef Q_OS_WIN
char g_altStack[kAltStackSize];
#endif

// ----- async-signal-safe writers ---------------------------------------------

void safeWriteFd(int fd, const char* buf, size_t len)
{
    if (fd < 0 || buf == nullptr || len == 0)
        return;
    while (len > 0) {
#ifdef Q_OS_WIN
        DWORD written = 0;
        HANDLE h = (fd == 2)
            ? GetStdHandle(STD_ERROR_HANDLE)
            : reinterpret_cast<HANDLE>(_get_osfhandle(fd));
        if (h == INVALID_HANDLE_VALUE || h == nullptr) return;
        if (!WriteFile(h, buf, static_cast<DWORD>(len), &written, nullptr) || written == 0) return;
        buf += written;
        len -= written;
#else
        ssize_t n = ::write(fd, buf, len);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;
        }
        if (n == 0) return;
        buf += n;
        len -= static_cast<size_t>(n);
#endif
    }
}

void safeWrite(const char* buf, size_t len)
{
    safeWriteFd(2, buf, len);
    if (g_logFd >= 0)
        safeWriteFd(g_logFd, buf, len);
}

void safeWriteStr(const char* s)
{
    if (s == nullptr) return;
    safeWrite(s, std::strlen(s));
}

size_t writeUnsignedTo(char* dst, size_t cap, unsigned long long v)
{
    char tmp[32];
    int i = 0;
    if (v == 0) tmp[i++] = '0';
    while (v > 0 && i < static_cast<int>(sizeof(tmp))) {
        tmp[i++] = static_cast<char>('0' + (v % 10));
        v /= 10;
    }
    size_t n = 0;
    while (i > 0 && n < cap) {
        dst[n++] = tmp[--i];
    }
    return n;
}

void safeWriteUnsigned(unsigned long long v)
{
    char tmp[32];
    size_t n = writeUnsignedTo(tmp, sizeof(tmp), v);
    safeWrite(tmp, n);
}

void safeWriteHex(unsigned long long v)
{
    char tmp[20];
    int i = 0;
    if (v == 0) tmp[i++] = '0';
    while (v > 0 && i < static_cast<int>(sizeof(tmp))) {
        unsigned d = static_cast<unsigned>(v & 0xFu);
        tmp[i++] = static_cast<char>((d < 10) ? ('0' + d) : ('a' + d - 10));
        v >>= 4;
    }
    char buf[24];
    size_t n = 0;
    buf[n++] = '0';
    buf[n++] = 'x';
    while (i > 0) buf[n++] = tmp[--i];
    safeWrite(buf, n);
}

// ----- log file (opened lazily inside the handler) ---------------------------

void openLogIfNeeded()
{
    if (g_logFd >= 0 || g_logPath[0] == 0) return;
#ifdef Q_OS_WIN
    g_logFd = ::_open(g_logPath,
                      _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY,
                      _S_IREAD | _S_IWRITE);
#else
    g_logFd = ::open(g_logPath, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
#endif
}

// ----- POSIX signal handler --------------------------------------------------

#ifndef Q_OS_WIN

const char* signalName(int sig)
{
    switch (sig) {
        case SIGSEGV: return "SIGSEGV (invalid memory access)";
        case SIGABRT: return "SIGABRT (abort)";
        case SIGFPE:  return "SIGFPE (arithmetic error)";
        case SIGILL:  return "SIGILL (illegal instruction)";
        case SIGBUS:  return "SIGBUS (bus error)";
        default:      return "signal";
    }
}

void signalHandler(int sig, siginfo_t* info, void* /*ucontext*/)
{
    if (g_inHandler.fetch_add(1) > 0) {
        // Re-entry: bail without further processing.
        _Exit(128 + sig);
    }

    openLogIfNeeded();

    safeWriteStr("\n=================== LibreCAD CRASH ===================\n");
    safeWrite(g_header, g_headerLen);
    safeWriteStr("Cause:    ");
    safeWriteStr(signalName(sig));
    safeWriteStr(" (signo=");
    safeWriteUnsigned(static_cast<unsigned long long>(sig));
    safeWriteStr(")\n");
    if (info != nullptr) {
        safeWriteStr("Faulting address: ");
        safeWriteHex(reinterpret_cast<unsigned long long>(info->si_addr));
        safeWriteStr("\n");
    }
    safeWriteStr("\nBacktrace:\n");

    void* frames[kMaxFrames];
    int n = backtrace(frames, kMaxFrames);
    if (n > 0) {
        backtrace_symbols_fd(frames, n, 2);
        if (g_logFd >= 0)
            backtrace_symbols_fd(frames, n, g_logFd);
    } else {
        safeWriteStr("  (no frames captured)\n");
    }
    safeWriteStr("======================================================\n");

    // SA_RESETHAND already restored SIG_DFL; re-raise so the OS can
    // produce a core dump or notify the platform crash reporter.
    raise(sig);
}

void installPosix()
{
    stack_t alt;
    std::memset(&alt, 0, sizeof(alt));
    alt.ss_sp = g_altStack;
    alt.ss_size = sizeof(g_altStack);
    alt.ss_flags = 0;
    sigaltstack(&alt, nullptr);

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = &signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_ONSTACK;

    const int sigs[] = { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS };
    for (int s : sigs) sigaction(s, &sa, nullptr);
}

#endif // !Q_OS_WIN

// ----- Windows unhandled-exception filter ------------------------------------

#ifdef Q_OS_WIN

std::once_flag g_symInitFlag;

void initSymbols()
{
    SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
    SymInitialize(GetCurrentProcess(), nullptr, TRUE);
}

const char* exceptionName(DWORD code)
{
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:      return "EXCEPTION_ACCESS_VIOLATION";
        case EXCEPTION_STACK_OVERFLOW:        return "EXCEPTION_STACK_OVERFLOW";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:    return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:    return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_ILLEGAL_INSTRUCTION:   return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case EXCEPTION_PRIV_INSTRUCTION:      return "EXCEPTION_PRIV_INSTRUCTION";
        case EXCEPTION_BREAKPOINT:            return "EXCEPTION_BREAKPOINT";
        case EXCEPTION_INT_OVERFLOW:          return "EXCEPTION_INT_OVERFLOW";
        case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        default: return "UNKNOWN_EXCEPTION";
    }
}

LONG WINAPI unhandledFilter(EXCEPTION_POINTERS* info)
{
    if (g_inHandler.fetch_add(1) > 0)
        return EXCEPTION_CONTINUE_SEARCH;

    openLogIfNeeded();
    std::call_once(g_symInitFlag, initSymbols);

    safeWriteStr("\n=================== LibreCAD CRASH ===================\n");
    safeWrite(g_header, g_headerLen);
    safeWriteStr("Cause:    ");
    safeWriteStr(exceptionName(info->ExceptionRecord->ExceptionCode));
    safeWriteStr(" (code=");
    safeWriteHex(info->ExceptionRecord->ExceptionCode);
    safeWriteStr(")\nFaulting address: ");
    safeWriteHex(reinterpret_cast<unsigned long long>(info->ExceptionRecord->ExceptionAddress));
    safeWriteStr("\n\nBacktrace:\n");

    void* frames[kMaxFrames];
    USHORT n = CaptureStackBackTrace(0, kMaxFrames, frames, nullptr);

    HANDLE proc = GetCurrentProcess();
    char symBuf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    std::memset(symBuf, 0, sizeof(symBuf));
    SYMBOL_INFO* sym = reinterpret_cast<SYMBOL_INFO*>(symBuf);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = MAX_SYM_NAME;

    IMAGEHLP_LINE64 line;
    std::memset(&line, 0, sizeof(line));
    line.SizeOfStruct = sizeof(line);

    for (USHORT i = 0; i < n; ++i) {
        DWORD64 addr = reinterpret_cast<DWORD64>(frames[i]);
        safeWriteStr("  #");
        safeWriteUnsigned(i);
        safeWriteStr("  ");
        safeWriteHex(addr);
        safeWriteStr("  ");
        DWORD64 disp = 0;
        if (SymFromAddr(proc, addr, &disp, sym))
            safeWriteStr(sym->Name);
        else
            safeWriteStr("(no symbol)");
        DWORD lineDisp = 0;
        if (SymGetLineFromAddr64(proc, addr, &lineDisp, &line)) {
            safeWriteStr("  (");
            safeWriteStr(line.FileName);
            safeWriteStr(":");
            safeWriteUnsigned(line.LineNumber);
            safeWriteStr(")");
        }
        safeWriteStr("\n");
    }
    safeWriteStr("======================================================\n");

    return EXCEPTION_CONTINUE_SEARCH;
}

void installWindows()
{
    SetUnhandledExceptionFilter(&unhandledFilter);
}

#endif // Q_OS_WIN

// ----- Common: std::terminate + Qt fatal -------------------------------------

QtMessageHandler g_prevQtHandler = nullptr;

void qtMessageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    if (type == QtFatalMsg) {
        safeWriteStr("\n[Qt FATAL] ");
        const QByteArray utf = msg.toUtf8();
        safeWrite(utf.constData(), static_cast<size_t>(utf.size()));
        if (ctx.file != nullptr) {
            safeWriteStr("\n  at ");
            safeWriteStr(ctx.file);
            safeWriteStr(":");
            safeWriteUnsigned(static_cast<unsigned long long>(ctx.line));
        }
        if (ctx.function != nullptr) {
            safeWriteStr(" in ");
            safeWriteStr(ctx.function);
        }
        safeWriteStr("\n");
        // Qt itself will abort -> SIGABRT (POSIX) or unhandled exception
        // (Windows) -> our handler dumps the stack.
    }
    if (g_prevQtHandler != nullptr)
        g_prevQtHandler(type, ctx, msg);
}

void terminateHandler()
{
    safeWriteStr("\n[std::terminate] uncaught C++ exception\n");
    try {
        if (auto p = std::current_exception()) std::rethrow_exception(p);
    } catch (const std::exception& e) {
        safeWriteStr("  what(): ");
        safeWriteStr(e.what());
        safeWriteStr("\n");
    } catch (...) {
        safeWriteStr("  (non-std exception)\n");
    }
    std::abort();  // -> SIGABRT path on POSIX, unhandled filter on Windows.
}

// ----- Pre-render header + path ----------------------------------------------

void buildHeader()
{
    char* p = g_header;
    char* end = g_header + sizeof(g_header) - 1;
    auto put = [&](const char* s) {
        if (s == nullptr) return;
        while (*s != '\0' && p < end) *p++ = *s++;
    };
    auto putUnsigned = [&](unsigned long long v) {
        char tmp[32];
        size_t n = writeUnsignedTo(tmp, sizeof(tmp), v);
        for (size_t i = 0; i < n && p < end; ++i) *p++ = tmp[i];
    };

    put("Version:  LibreCAD ");
    put(LC_CRASH_STR(LC_VERSION));
    put("\n");

    put("Qt:       ");
    put(qVersion());
    put("\n");

    const QByteArray osName = QSysInfo::prettyProductName().toLocal8Bit();
    const QByteArray kernel = QSysInfo::kernelType().toLocal8Bit();
    const QByteArray kver   = QSysInfo::kernelVersion().toLocal8Bit();
    const QByteArray arch   = QSysInfo::currentCpuArchitecture().toLocal8Bit();
    put("OS:       ");
    put(osName.constData());
    put(" / ");
    put(kernel.constData());
    put(" ");
    put(kver.constData());
    put(" (");
    put(arch.constData());
    put(")\n");

    put("PID:      ");
#ifdef Q_OS_WIN
    putUnsigned(static_cast<unsigned long long>(GetCurrentProcessId()));
#else
    putUnsigned(static_cast<unsigned long long>(::getpid()));
#endif
    put("\n");

    const QByteArray ts = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toLocal8Bit();
    put("Time:     ");
    put(ts.constData());
    put(" UTC\n");

    g_headerLen = static_cast<size_t>(p - g_header);
}

void preparePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dir.isEmpty())
        dir = QDir::tempPath();
    QDir().mkpath(dir);

    const QString stamp = QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss");
#ifdef Q_OS_WIN
    const qint64 pid = static_cast<qint64>(GetCurrentProcessId());
#else
    const qint64 pid = static_cast<qint64>(::getpid());
#endif
    const QString path = QString("%1/crash-%2-%3.log").arg(dir, stamp).arg(pid);
    const QByteArray pathLocal = QFile::encodeName(path);
    if (pathLocal.size() > 0 && pathLocal.size() < static_cast<int>(sizeof(g_logPath))) {
        std::memcpy(g_logPath, pathLocal.constData(), static_cast<size_t>(pathLocal.size()));
        g_logPath[pathLocal.size()] = 0;
        std::fprintf(stderr,
                     "LC_CrashHandler installed; on crash, log -> %s\n",
                     g_logPath);
    }
}

} // namespace

void LC_CrashHandler::install()
{
    static std::once_flag once;
    std::call_once(once, []() {
        preparePath();
        buildHeader();
        std::set_terminate(&terminateHandler);
        g_prevQtHandler = qInstallMessageHandler(&qtMessageHandler);
#ifdef Q_OS_WIN
        installWindows();
#else
        installPosix();
#endif
    });
}
