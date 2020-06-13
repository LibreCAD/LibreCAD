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

#include "lc_printing.h"

QPrinter::PageSize LC_Printing::rsToQtPaperFormat(RS2::PaperFormat f)
{
    switch (f) {
    case RS2::A0: return QPrinter::A0;
    case RS2::A1: return QPrinter::A1;
    case RS2::A2: return QPrinter::A2;
    case RS2::A3: return QPrinter::A3;
    case RS2::A4: return QPrinter::A4;

    /* Removed ISO "B" and "C" series, C5E, Comm10E, DLE, (envelope sizes) */

    case RS2::Letter: return QPrinter::Letter;
    case RS2::Legal:  return QPrinter::Legal;
    case RS2::Tabloid: return QPrinter::Tabloid;

    //case RS2::Ansi_A: return QPrinter::AnsiA;
    //case RS2::Ansi_B: return QPrinter::AnsiB;
    case RS2::Ansi_C: return QPrinter::AnsiC;
    case RS2::Ansi_D: return QPrinter::AnsiD;
    case RS2::Ansi_E: return QPrinter::AnsiE;

    case RS2::Arch_A: return QPrinter::ArchA;
    case RS2::Arch_B: return QPrinter::ArchB;
    case RS2::Arch_C: return QPrinter::ArchC;
    case RS2::Arch_D: return QPrinter::ArchD;
    case RS2::Arch_E: return QPrinter::ArchE;

    default:
        break;
    }

    return QPrinter::Custom;
}
