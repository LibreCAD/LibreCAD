/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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

#include <map>

#include "lc_printing.h"

namespace {

// supported paper formats should be added here
const std::map<RS2::PaperFormat, QPrinter::PageSize> paperToPage = {
    {
            {RS2::A0, QPrinter::A0},
            {RS2::A1, QPrinter::A1},
            {RS2::A2, QPrinter::A2},
            {RS2::A3, QPrinter::A3},
            {RS2::A4, QPrinter::A4},

                /* Removed ISO "B" and "C" series,  C5E,  Comm10E,  DLE,  (envelope sizes) */

            {RS2::Letter, QPrinter::Letter},
            {RS2::Legal,  QPrinter::Legal},
            {RS2::Tabloid, QPrinter::Tabloid},

                //case RS2::Ansi_A, QPrinter::AnsiA},
                //case RS2::Ansi_B, QPrinter::AnsiB},
            {RS2::Ansi_C, QPrinter::AnsiC},
            {RS2::Ansi_D, QPrinter::AnsiD},
            {RS2::Ansi_E, QPrinter::AnsiE},

            {RS2::Arch_A, QPrinter::ArchA},
            {RS2::Arch_B, QPrinter::ArchB},
            {RS2::Arch_C, QPrinter::ArchC},
            {RS2::Arch_D, QPrinter::ArchD},
            {RS2::Arch_E, QPrinter::ArchE},
    }
};
}

QPrinter::PageSize LC_Printing::rsToQtPaperFormat(RS2::PaperFormat paper)
{
    return (paperToPage.count(paper) == 1) ? paperToPage.at(paper) : QPrinter::Custom;
}
