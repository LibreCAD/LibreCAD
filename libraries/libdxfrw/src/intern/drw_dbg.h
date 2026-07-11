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
#define DRW_DBG(a) DRW_dbg::getInstance()->printIfEnabled(a)
#define DRW_DBGH(a) DRW_dbg::getInstance()->printHIfEnabled(a)
#define DRW_DBGB(a) DRW_dbg::getInstance()->printBIfEnabled(a)
#define DRW_DBGHL(a, b, c) DRW_dbg::getInstance()->printHLIfEnabled(a, b ,c)
#define DRW_DBGPT(a, b, c) DRW_dbg::getInstance()->printPTIfEnabled(a, b, c)

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
    Level level{Level::None};
    DRW::DebugPrinter silentDebug;
    std::unique_ptr< DRW::DebugPrinter > debugPrinter;
    DRW::DebugPrinter* currentPrinter{nullptr};
};


#endif // DRW_DBG_H
