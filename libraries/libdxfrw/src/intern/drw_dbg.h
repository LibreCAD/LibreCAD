/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DRW_DBG_H
#define DRW_DBG_H

#include <string>
#include <iostream>
#include <memory>
#include <utility>
#include "../drw_base.h"
//#include <iomanip>

#define DRW_DBGSL(a) DRW_dbg::getInstance()->setLevel(a)
#define DRW_DBGGL DRW_dbg::getInstance()->getLevel()
// These route through static dbg* wrappers (below). The macro ARGUMENT is
// always evaluated (some call sites rely on side effects, e.g.
// DRW_DBG(buf->getRawShort16()) advances the buffer), exactly as the original
// getInstance()->printIfEnabled(a) did. The win is that the wrapper checks the
// fast inline s_enabled flag BEFORE calling getInstance() and printing, so the
// disabled path (the norm) no longer pays for getInstance()/isDebugEnabled()
// per call -- previously the two hottest functions in a large DWG read.
#define DRW_DBG(a) DRW_dbg::dbg(a)
#define DRW_DBGH(a) DRW_dbg::dbgH(a)
#define DRW_DBGB(a) DRW_dbg::dbgB(a)
#define DRW_DBGHL(a, b, c) DRW_dbg::dbgHL(a, b, c)
#define DRW_DBGPT(a, b, c) DRW_dbg::dbgPT(a, b, c)

class DRW_dbg {
public:
    enum class Level {
        None,
        Debug
    };
    void setLevel(Level lvl);
    /**
     * Sets a custom debug printer to use when non-silent output
     * is required.
     */
    void setCustomDebugPrinter(std::unique_ptr<DRW::DebugPrinter> printer);
    Level getLevel();
    bool isDebugEnabled() const;
    static DRW_dbg *getInstance();

    // Static wrappers behind the DRW_DBG* macros. The argument is evaluated by
    // the caller (side effects such as DRW_DBG(buf->getRawShort16()) advancing
    // the buffer are preserved), then s_enabled is checked INLINE before
    // getInstance()/print -- so on the common disabled path there is no
    // getInstance() call and no virtual isDebugEnabled(). Debug is off by
    // default; these were the two hottest functions in a large DWG read.
    template<typename T> static void dbg(T&& v) {
        if (s_enabled) getInstance()->print(std::forward<T>(v));
    }
    template<typename T> static void dbgH(T&& v) {
        if (s_enabled) getInstance()->printH(std::forward<T>(v));
    }
    template<typename T> static void dbgB(T&& v) {
        if (s_enabled) getInstance()->printB(std::forward<T>(v));
    }
    template<typename C, typename S, typename H>
    static void dbgHL(C&& c, S&& s, H&& h) {
        if (s_enabled) getInstance()->printHL(std::forward<C>(c),
                                              std::forward<S>(s),
                                              std::forward<H>(h));
    }
    template<typename X, typename Y, typename Z>
    static void dbgPT(X&& x, Y&& y, Z&& z) {
        if (s_enabled) getInstance()->printPT(std::forward<X>(x),
                                              std::forward<Y>(y),
                                              std::forward<Z>(z));
    }
    void print(const std::string &s);
    void print(signed char i);
    void print(unsigned char i);
    void print(int i);
    void print(unsigned int i);
    void print(long int i);
    void print(long long int i);
    void print(long unsigned int i);
    void print(long long unsigned int i);
    void print(double d);
    void printH(long long int i);
    void printB(int i);
    void printHL(int c, int s, int h);
    void printPT(double x, double y, double z);

    template<typename T>
    void printIfEnabled(T&& value) {
        if (isDebugEnabled())
            print(std::forward<T>(value));
    }

    template<typename T>
    void printHIfEnabled(T&& value) {
        if (isDebugEnabled())
            printH(std::forward<T>(value));
    }

    template<typename T>
    void printBIfEnabled(T&& value) {
        if (isDebugEnabled())
            printB(std::forward<T>(value));
    }

    template<typename C, typename S, typename H>
    void printHLIfEnabled(C&& c, S&& s, H&& h) {
        if (isDebugEnabled())
            printHL(std::forward<C>(c), std::forward<S>(s), std::forward<H>(h));
    }

    template<typename X, typename Y, typename Z>
    void printPTIfEnabled(X&& x, Y&& y, Z&& z) {
        if (isDebugEnabled())
            printPT(std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z));
    }

private:
    DRW_dbg();
    static DRW_dbg *instance;
    static bool s_enabled;   // mirror of (level == Debug); updated in setLevel
    Level level{Level::None};
    DRW::DebugPrinter silentDebug;
    std::unique_ptr< DRW::DebugPrinter > debugPrinter;
    DRW::DebugPrinter* currentPrinter{nullptr};
};


#endif // DRW_DBG_H
