/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2022 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DRW_RESERVE_H
#define DRW_RESERVE_H

#include <vector>
#include "drw_dbg.h"

namespace DRW
{
    /**
     * Template to protect vector<>.reserve() calls.
     * Malformed or suspicious input files can cause std::exceptions,
     * which are caught here to avoid crashes or other vulnerabilities.
     */
    template <typename T>
    bool reserve(std::vector<T> &list, const int size)
    {
        try {
            list.reserve( size);
        }
        catch (const std::exception& e) {
            DRW_DBG( "std::exception : ");
            DRW_DBG( e.what());
            DRW_DBG( " - ");
            DRW_DBG( size);
            DRW_DBG( "\n");
            return false;
        }
        catch (...) {
            DRW_DBG( "vector<>.reserve() exception : ");
            DRW_DBG( size);
            DRW_DBG( "\n");
            return false;
        }

        return true;
    }
}
#endif // DRW_RESERVE_H
