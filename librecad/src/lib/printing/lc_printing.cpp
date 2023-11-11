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
const std::map<RS2::PaperFormat, QPageSize::PageSizeId> paperToPage = {
    {
            {RS2::A0, QPageSize::A0},
            {RS2::A1, QPageSize::A1},
            {RS2::A2, QPageSize::A2},
            {RS2::A3, QPageSize::A3},
            {RS2::A4, QPageSize::A4},

                /* Removed ISO "B" and "C" series,  C5E,  Comm10E,  DLE,  (envelope sizes) */

            {RS2::Letter, QPageSize::Letter},
            {RS2::Legal,  QPageSize::Legal},
            {RS2::Tabloid, QPageSize::Tabloid},

                //case RS2::Ansi_A, QPageSize::AnsiA},
                //case RS2::Ansi_B, QPageSize::AnsiB},
            {RS2::Ansi_C, QPageSize::AnsiC},
            {RS2::Ansi_D, QPageSize::AnsiD},
            {RS2::Ansi_E, QPageSize::AnsiE},

            {RS2::Arch_A, QPageSize::ArchA},
            {RS2::Arch_B, QPageSize::ArchB},
            {RS2::Arch_C, QPageSize::ArchC},
            {RS2::Arch_D, QPageSize::ArchD},
            {RS2::Arch_E, QPageSize::ArchE},
    }
};
}

QPageSize::PageSizeId LC_Printing::rsToQtPaperFormat(RS2::PaperFormat paper)
{
    return (paperToPage.count(paper) == 1) ? paperToPage.at(paper) : QPageSize::Custom;
}
