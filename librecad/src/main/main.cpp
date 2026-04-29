#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#define _XOPEN_SOURCE 700

/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2018 Simon Wells <simonrwells@gmail.com>
** Copyright (C) 2020 Nikita Letov <letovnn@gmail.com>
** Copyright (C) 2015-2016 ravas (github.com/r-a-v-a-s)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
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
#include <clocale>
#include <cstdio>
#include <cstddef>
#include <ctime>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <DbgHelp.h>
#include <imagehlp.h>
#pragma comment(lib, "DbgHelp.lib")
#elif defined(__linux__) || defined(__APPLE__)
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <cxxabi.h>
#include <sys/time.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <dlfcn.h>
#endif
#endif

#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QSplashScreen>

#include <QDir>
#include <QPushButton>
#include <QTimer>
#include <QToolBar>

#include "console_dxf2pdf.h"
#include "console_dxf2png.h"
#include "lc_application.h"
#include "main.h"

#include "lc_iconcolorsoptions.h"
#include "qc_applicationwindow.h"
#include "qg_dlginitial.h"
#include "rs_debug.h"
#include "rs_fontlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"

// default version: if not supplied by during building
#ifndef LC_VERSION
#define LC_VERSION "2.2.2-alpha"
#endif

// fixme - sand - files - complete refactoring
namespace
{
// To a plain text string to the compiled binary
const std::string g_lcVersion{"LC_VISION=" XSTR(LC_VERSION)};

// update splash for alpha/beta names)
    void updateSplash(const std::unique_ptr<QSplashScreen>& splash);
}

#ifdef _WIN32
LONG WINAPI CrashHandler(EXCEPTION_POINTERS* pExceptionInfo)
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
#else
#error Unsupported platform
#endif
        
        HANDLE hThread = GetCurrentThread();
        DWORD machineType = IMAGE_FILE_MACHINE_I386;
#ifdef _M_X64
        machineType = IMAGE_FILE_MACHINE_AMD64;
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
#elif defined(__linux__) || defined(__APPLE__)
static void signalHandler(int sig, siginfo_t* info, [[maybe_unused]]void* context)
{
    const char* signalNames[] = {
        "Unknown", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP",
        "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV",
        "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM"
    };
    
    char crashFileName[PATH_MAX];
#ifdef __linux__
    ssize_t len = readlink("/proc/self/exe", crashFileName, PATH_MAX - 1);
    if (len == -1) {
        snprintf(crashFileName, PATH_MAX, "librecad.crash.log");
    } else {
        crashFileName[len] = '\0';
        strncat(crashFileName, ".crash.log", PATH_MAX - len - 1);
    }
#elif __APPLE__
    uint32_t size = PATH_MAX;
    if (_NSGetExecutablePath(crashFileName, &size) != 0) {
        snprintf(crashFileName, PATH_MAX, "LibreCAD.crash.log");
    } else {
        strncat(crashFileName, ".crash.log", PATH_MAX - strlen(crashFileName) - 1);
    }
#endif
    
    FILE* pFile = fopen(crashFileName, "w");
    if (pFile != nullptr) {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        struct tm* tm = localtime(&tv.tv_sec);
        
        fprintf(pFile, "=========================================\n");
        fprintf(pFile, "LibreCAD Crash Report\n");
        fprintf(pFile, "=========================================\n\n");
        
        fprintf(pFile, "Crash Time: %04d-%02d-%02d %02d:%02d:%02d.%03d\n\n",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000));
        
        fprintf(pFile, "Signal: %d (%s)\n", sig, 
                sig < (int)(sizeof(signalNames)/sizeof(signalNames[0])) ? signalNames[sig] : signalNames[0]);
        
        if (info != nullptr) {
            fprintf(pFile, "Signal Code: %d\n", info->si_code);
            fprintf(pFile, "Fault Address: 0x%p\n", info->si_addr);
        }
        
        fprintf(pFile, "\nCall Stack:\n");
        fprintf(pFile, "-----------\n");
        
#ifdef __APPLE__
        void* callstack[64];
        int frames = 0;
        char** symbols = nullptr;
        
        // Dynamically load backtrace functions on macOS
        typedef int (*BacktraceFunc)(void**, int);
        typedef char** (*BacktraceSymbolsFunc)(void* const*, int);
        
        void* dlHandle = dlopen(nullptr, RTLD_LAZY);
        BacktraceFunc backtrace_func = dlHandle ? (BacktraceFunc)dlsym(dlHandle, "backtrace") : nullptr;
        BacktraceSymbolsFunc backtrace_symbols_func = dlHandle ? (BacktraceSymbolsFunc)dlsym(dlHandle, "backtrace_symbols") : nullptr;
        
        if (backtrace_func != nullptr && backtrace_symbols_func != nullptr) {
            frames = (int)backtrace_func(callstack, 64);
            symbols = backtrace_symbols_func(callstack, frames);
        }
        
        for (int i = 0; i < frames; i++) {
            char funcName[256] = "??";
            char moduleName[256] = "??";
            
            if (symbols != nullptr) {
                const char* symbol = symbols[i];
                char* addr_end = strchr((char*)symbol, ' ');
                char* func_start = addr_end ? strchr(addr_end + 1, '(') : nullptr;
                
                if (addr_end) *addr_end = '\0';
                strncpy(moduleName, symbol, sizeof(moduleName) - 1);
                
                if (func_start) {
                    char* func_end = strchr(func_start + 1, '+');
                    if (func_end) *func_end = '\0';
                    strncpy(funcName, func_start + 1, sizeof(funcName) - 1);
                    
                    int status = 0;
                    char* demangled = abi::__cxa_demangle(funcName, nullptr, nullptr, &status);
                    if (status == 0 && demangled != nullptr) {
                        strncpy(funcName, demangled, sizeof(funcName) - 1);
                        free(demangled);
                    }
                }
            }
            
            fprintf(pFile, "%2d: 0x%p %s (%s)\n", i, callstack[i], funcName, moduleName);
        }
        
        if (symbols != nullptr) {
            free(symbols);
        }
        
        if (dlHandle != nullptr) {
            dlclose(dlHandle);
        }
#else
        void* callstack[64];
        int frames = (int)backtrace(callstack, 64);
        char** symbols = backtrace_symbols(callstack, frames);
        
        for (int i = 0; i < frames; i++) {
            char funcName[256] = "??";
            char moduleName[256] = "??";
            
            if (symbols != nullptr) {
                char* begin = nullptr;
                char* end = nullptr;
                
                for (char* p = symbols[i]; *p; p++) {
                    if (*p == '(') begin = p;
                    if (*p == '+') end = p;
                }
                
                if (begin && end) {
                    *begin = '\0';
                    *end = '\0';
                    strncpy(moduleName, symbols[i], sizeof(moduleName) - 1);
                    
                    begin++;
                    size_t len = end - begin;
                    if (len > 0 && len < sizeof(funcName)) {
                        strncpy(funcName, begin, len);
                        funcName[len] = '\0';
                        
                        int status = 0;
                        char* demangled = abi::__cxa_demangle(funcName, nullptr, nullptr, &status);
                        if (status == 0 && demangled != nullptr) {
                            strncpy(funcName, demangled, sizeof(funcName) - 1);
                            free(demangled);
                        }
                    }
                } else {
                    strncpy(funcName, symbols[i], sizeof(funcName) - 1);
                }
            }
            
            fprintf(pFile, "%2d: 0x%p %s (%s)\n", i, callstack[i], funcName, moduleName);
        }
        
        if (symbols != nullptr) {
            free(symbols);
        }
#endif
        
        fprintf(pFile, "\n=========================================\n");
        fprintf(pFile, "End of Crash Report\n");
        fprintf(pFile, "=========================================\n");
        fclose(pFile);
        
        printf("\n\n*** LibreCAD has crashed ***\n");
        printf("Crash log saved to: %s\n\n", crashFileName);
    }
    
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

void InstallCrashHandler()
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(CrashHandler);
#elif defined(__linux__) || defined(__APPLE__)
    struct sigaction sa;
    sa.sa_sigaction = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
#endif
}

void showFirstLoadSetupDialog(bool first_load) {
    LC_GROUP_GUARD("Defaults");
    {
        QString unit = LC_GET_STR("Unit", "Invalid");
        // show initial config dialog:
        if (first_load){
            RS_DEBUG->print("main: show initial config dialog..");
            QG_DlgInitial di(nullptr);
            QPixmap pxm(":/images/intro_librecad.png");
            di.setPixmap(pxm);
            if (di.exec()) {
                unit = LC_GET_STR("Unit", "None");
            }
            RS_DEBUG->print("main: show initial config dialog: OK");
        }
    }
}

int showHelpMessage() {
    qDebug()<<"Usage: librecad [command] <options> <dxf file>";
    qDebug()<<"";
    qDebug()<<"Commands:";
    qDebug()<<"";
    qDebug()<<"  dxf2pdf\tRun librecad as console dxf2pdf tool. Use -h for help.";
    qDebug()<<"  dxf2png\tRun librecad as console dxf2png tool. Use -h for help.";
    qDebug()<<"  dxf2svg\tRun librecad as console dxf2svg tool. Use -h for help.";
    qDebug()<<"";
    qDebug()<<"Options:";
    qDebug()<<"";
    qDebug()<<"  -h, --help\tdisplay this message";
    qDebug()<<"  -d, --debug <level>";
    qDebug()<<"";
    RS_DEBUG->print( RS_Debug::D_NOTHING, "possible debug levels:");
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Nothing", RS_Debug::D_NOTHING);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Critical", RS_Debug::D_CRITICAL);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Error", RS_Debug::D_ERROR);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Warning", RS_Debug::D_WARNING);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Notice", RS_Debug::D_NOTICE);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Informational", RS_Debug::D_INFORMATIONAL);
    RS_DEBUG->print( RS_Debug::D_NOTHING, "    %d Debugging", RS_Debug::D_DEBUGGING);
    exit(0);
}

void showDebugSetupHelpMessage() {
    RS_DEBUG->print(RS_Debug::D_NOTHING, "possible debug levels:");
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Nothing", RS_Debug::D_NOTHING);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Critical", RS_Debug::D_CRITICAL);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Error", RS_Debug::D_ERROR);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Warning", RS_Debug::D_WARNING);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Notice", RS_Debug::D_NOTICE);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Informational", RS_Debug::D_INFORMATIONAL);
    RS_DEBUG->print(RS_Debug::D_NOTHING, "    %d Debugging", RS_Debug::D_DEBUGGING);
}

void initFontList() {
    RS_DEBUG->print("main: init fontlist..");
    RS_FONTLIST->init();
    RS_DEBUG->print("main: init fontlist: OK");
}

void initPatternList() {
    RS_DEBUG->print("main: init patternlist..");
    RS_PATTERNLIST->init();
    RS_DEBUG->print("main: init patternlist: OK");
}

void loadTranslations() {
    RS_DEBUG->print("main: loading translation..");

    LC_GROUP("Appearance");
    QString lang = LC_GET_STR("Language", "en");
    QString langCmd = LC_GET_STR("LanguageCmd", "en");
    LC_GROUP_END();

    RS_SYSTEM->loadTranslation(lang, langCmd);
    RS_DEBUG->print("main: loading translation: OK");
}

void initSystem(char** argv, LC_Application& app) {
    RS_DEBUG->print("param 0: %s", argv[0]);

    QFileInfo prgInfo( QFile::decodeName(argv[0]) );
    QString prgDir(prgInfo.absolutePath());

    RS_SYSTEM->init(app.applicationName(), app.applicationVersion(), XSTR(QC_APPDIR), prgDir);
}

void loadFilesOnStartup(QSplashScreen *splash, QC_ApplicationWindow& appWin, [[maybe_unused]]LC_Application& app, QStringList fileList) {
    RS_DEBUG->print("main: loading files..");
#ifdef __APPLE__
    // get the file list from LC_Application
    fileList << app.fileList();
#endif

    // reopen files that we open during last close of application
    // we'll reopen them if no explicit files to open are provided in command line

    appWin.openFilesOnStartup(fileList, splash);
    RS_DEBUG->print("main: loading files: OK");
}

void loadIconsStylingOptions() {
    LC_IconColorsOptions iconColorsOptions;
    iconColorsOptions.loadSettings();
    iconColorsOptions.applyOptions();
}

int execApplication(LC_Application& app) {
    RS_DEBUG->print("main: entering Qt event loop");
    QCoreApplication::processEvents();

    int return_code = app.exec();

    RS_DEBUG->print("main: exited Qt event loop");

    // Destroy the singleton
    QC_ApplicationWindow::getAppWindow().reset();
    return return_code;
}

//
bool setupDebugLevel(char level) {
    switch(level){
        case '?' : {
            showDebugSetupHelpMessage();
            return false;
        }
        case '0' + RS_Debug::D_NOTHING : {
            RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
            break;
        }
        case '0' + RS_Debug::D_CRITICAL : {
            RS_DEBUG->setLevel(RS_Debug::D_CRITICAL);
            break;
        }
        case '0' + RS_Debug::D_ERROR : {
            RS_DEBUG->setLevel(RS_Debug::D_ERROR);
            break;
        }
        case '0' + RS_Debug::D_WARNING : {
            RS_DEBUG->setLevel(RS_Debug::D_WARNING);
            break;
        }
        case '0' + RS_Debug::D_NOTICE : {
            RS_DEBUG->setLevel(RS_Debug::D_NOTICE);
            break;
        }
        case '0' + RS_Debug::D_INFORMATIONAL : {
            RS_DEBUG->setLevel(RS_Debug::D_INFORMATIONAL);
            break;
        }
        case '0' + RS_Debug::D_DEBUGGING : {
            RS_DEBUG->setLevel(RS_Debug::D_DEBUGGING);
            break;
        }
        default : {
            RS_DEBUG->setLevel(RS_Debug::D_DEBUGGING);
            break;
        }
    }
    return true;
}


#define QUICK_TEST_

/**
 * Main. Creates Application window.
 */
 // fixme - sand - refactor and split to several specialized functions
#ifndef BUILD_TESTS

int main(int argc, char** argv) {

#ifdef QUICK_TEST

#    else

    InstallCrashHandler();

    // Create compilater's error: this QT macros may be in .pro file only.
    //QT_REQUIRE_VERSION(argc, argv, "6.4");
    
    // May be this code must be on begin of main.cpp?
    #if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
        #error "This programm requires Qt6 ver.6.4.0 or higher."
    #endif
    
    // Check first two arguments in order to decide if we want to run librecad
    // as console dxf2pdf or dxf2png tools. On Linux we can create a link to
    // librecad executable and  name it dxf2pdf. So, we can run either:
    //
    //     librecad dxf2pdf [options] ...
    //
    // or just:
    //
    //     dxf2pdf [options] ...
    //
    for (int i = 0; i < qMin(argc, 2); i++) {
        QString arg(argv[i]);
        if (i == 0) {
            arg = QFileInfo(QFile::decodeName(argv[i])).baseName();
        }
        if (arg.compare("dxf2pdf") == 0) {
            return console_dxf2pdf(argc, argv);
        }
        if (arg.compare("dxf2png") == 0 || arg == "dxf2svg") {
            return console_dxf2png(argc, argv);
        }
    }

    RS_DEBUG->setLevel(RS_Debug::D_WARNING);

    LC_Application app(argc, argv);
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD");
    QCoreApplication::setApplicationVersion(XSTR(LC_VERSION));

    // fixme - sand - NEED TO CHECK WHERE lc_svgicons.so is located under linux and mac!!! That's tested for Windows
    auto appDir = app.applicationDirPath();
    auto inconEnginesDir = appDir + "/iconengines";
    app.addLibraryPath(inconEnginesDir);

    RS_Settings::init(app.organizationName(), app.applicationName());

    QGuiApplication::setDesktopFileName("librecad");

    loadIconsStylingOptions();

    bool first_load = LC_GET_ONE_BOOL("Startup", "FirstLoad", true);

    bool allowOptions=true;
    QList<int> argClean;

    for (int i=0; i<argc; i++)   {
        QString argstr(argv[i]);
        if(allowOptions&&QString::compare("--", argstr)==0){
            allowOptions=false;
            continue;
        }
        const QString help0("-h"), help1("--help");
        if (allowOptions && (help0.compare(argstr, Qt::CaseInsensitive)==0 ||
                             help1.compare(argstr, Qt::CaseInsensitive)==0 )) {
            return showHelpMessage();
        }
        const QString lpDebugSwitch0("-d"),lpDebugSwitch1("--debug") ;

        if (allowOptions&& (argstr.startsWith(lpDebugSwitch0, Qt::CaseInsensitive) ||
                             argstr.startsWith(lpDebugSwitch1, Qt::CaseInsensitive) )){
            argClean<<i;

            // to control the level of debugging output use --debug with level 0-6, e.g. --debug3
            // for a list of debug levels use --debug?
            // if no level follows, the debugging level is set
            argstr.remove(QRegularExpression("^"+lpDebugSwitch0));
            argstr.remove(QRegularExpression("^"+lpDebugSwitch1));
            char level = '3';
            if(argstr.size()==0){
                if(i+1<argc){
                    if(QRegularExpression(R"(\d*)").match(argv[i+1]).hasMatch()){
                        ++i;
                        qDebug()<<"reading "<<argv[i]<<" as debugging level";
                        level=argv[i][0];
                        argClean<<i;
                    }
                    else {
                        level = '3';
                    }
                }
                else {
                    level = '3'; //default to D_WARNING
                }
            }
            else {
                level = argstr.toStdString()[0];
            }

            if (!setupDebugLevel(level)) {
                return 0;
            }
        }
    }
    initSystem(argv, app);
    showFirstLoadSetupDialog(first_load);

    std::unique_ptr<QSplashScreen> splash;
    bool show_splash = LC_GET_ONE_BOOL("Startup","ShowSplash", true);

    if (show_splash){
        splash = std::make_unique<QSplashScreen>();
        updateSplash(splash);
        app.processEvents();
        RS_DEBUG->print("main: splashscreen: OK");
    }

    initFontList();
    initPatternList();
    loadTranslations();

    RS_DEBUG->print("main: creating main window..");
    QC_ApplicationWindow& appWin = *QC_ApplicationWindow::getAppWindow();
    auto& appWindow = QC_ApplicationWindow::getAppWindow();
    if (appWindow != nullptr) {
        appWindow->fireIconsRefresh();
    }
#ifdef __APPLE__
    app.installEventFilter(&appWin);
#endif
    RS_DEBUG->print("main: setting caption");
    appWin.setWindowTitle(app.applicationName());

    RS_DEBUG->print("main: show main window");

    QSettings settings; // fixme - direct invocation of settings
    settings.beginGroup("Defaults");
    if( !settings.contains("UseQtFileOpenDialog")) {
#ifdef __linux__
        // on Linux don't use native file dialog
        // because of case insensitive filters (issue #791)
        settings.setValue("UseQtFileOpenDialog", QVariant(1));
#else
        settings.setValue("UseQtFileOpenDialog", QVariant(0));
#endif
    }
    settings.endGroup();

    bool maximize = LC_GET_ONE_BOOL("Startup","Maximize", false);

    if (maximize || first_load) {
        appWin.showMaximized();
    }
    else {
        appWin.show();
    }

    RS_DEBUG->print("main: set focus");
    appWin.setFocus();
    RS_DEBUG->print("main: creating main window: OK");

    if (splash != nullptr){
        RS_DEBUG->print("main: updating splash");
        splash->raise();
        splash->showMessage(QObject::tr("Loading..."), Qt::AlignRight|Qt::AlignBottom, Qt::black);
        RS_DEBUG->print("main: processing events");
        qApp->processEvents();
        RS_DEBUG->print("main: updating splash: OK");
    }

    // Set LC_NUMERIC so that entering numeric values uses . as the decimal separator
    setlocale(LC_NUMERIC, "C");

    // parse command line arguments that might not need a launched program:
    // fixme - sand - add support of skipping of loading via cmdline flag
    QStringList fileList = handleArgs(argc, argv, argClean);
    loadFilesOnStartup(splash.get(), appWin, app, fileList);

    appWin.initCompleted();

    if (splash != nullptr) {
        splash->finish(&appWin);
        splash.release();
    }

    LC_GROUP("Startup");
    {
        // fixme - sand - files - add support of command line flag to suppress version check (may be useful for automation)!
        bool checkForNewVersion = LC_GET_BOOL("CheckForNewVersions", true);
        if (checkForNewVersion) {
            appWin.checkForNewVersion();
        }

        if (first_load){
            LC_SET("FirstLoad", false);
        }
    }
    LC_GROUP_END();

    return execApplication(app);
#    endif
}
#endif // BUILD_TESTS

/**
 * Handles command line arguments that might not require a GUI.
 *
 * @return list of files to load on startup.
 */
QStringList handleArgs(int argc, char** argv, const QList<int>& argClean){
    RS_DEBUG->print("main: handling args..");
    QStringList ret;

    bool doexit = false;
    for (int i = 1; i < argc; i++) {
        if (argClean.indexOf(i) >= 0) {
            continue;
        }
        auto localFileName = argv[i];
        if (!QString(localFileName).startsWith("-")) {
            auto decodedName = QFile::decodeName(localFileName);
            QFileInfo fileInfo(decodedName);
            auto absolutePath = fileInfo.absoluteFilePath();
            QString fname = QDir::toNativeSeparators(absolutePath);
            ret.append(fname);
        }
        else if (QString(localFileName) == "--exit") {
            doexit = true;
        }
    }
    if (doexit) {
        exit(0);
    }
    RS_DEBUG->print("main: handling args: OK");
    return ret;
}

QString LCReleaseLabel(){
    QString version{XSTR(LC_VERSION)};
    const std::map<QString, QString> labelMap = {
        {"rc", QObject::tr("Release Candidate")},
        {"beta", QObject::tr("BETA")},
        {"alpha", QObject::tr("ALPHA")}
    };
    for (const auto& [key, value]: labelMap) {
        if (version.contains(key, Qt::CaseInsensitive)) {
            return value;
        }
    }

    // Issue #2371: default version to alpha
    return QObject::tr("ALPHA");
}

namespace {

// Update Splash image to show "ALPHA", "BETA", and "Release Candidate"
QPixmap getSplashImage(const std::unique_ptr<QSplashScreen>& splash, const QString& label);
// Update Splash Screen
    void updateSplash(const std::unique_ptr<QSplashScreen>& splash)
    {
        if (splash == nullptr)
            return;

    QString label = LCReleaseLabel();
        if (label.isEmpty())
            return;

        QPixmap splashImage = getSplashImage(splash, label);
        splash->setPixmap(splashImage);
        splash->setAttribute(Qt::WA_DeleteOnClose);
        splash->show();
        splash->showMessage(QObject::tr("Loading.."),
                            Qt::AlignRight|Qt::AlignBottom, Qt::black);
    }

// Update Splash image to show "ALPHA", "BETA", and "Release Candidate"
    QPixmap getSplashImage(const std::unique_ptr<QSplashScreen>& splash, const QString& label)
    {
        if (splash == nullptr)
            return {};

        QPixmap pixmapSplash(":/images/splash_librecad.png");
        QPainter painter(&pixmapSplash);
        const double factorX = pixmapSplash.width()/542.;
        const double factorY = pixmapSplash.height()/337.;
        painter.setPen(QColor(255, 0, 0, 128));
        QRectF labelRect{QPointF{280.*factorX, 130.*factorY}, QPointF{480.*factorX, 170.*factorY}};
        QFont font;
        font.setPixelSize(int(labelRect.height()) - 2);
        painter.setFont(font);
        painter.drawText(labelRect,Qt::AlignRight, label);
        return pixmapSplash;
    }
}