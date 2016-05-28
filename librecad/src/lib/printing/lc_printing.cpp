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
    switch (f)
    {
        default:
        case RS2::Custom:
            return QPrinter::Custom;
        case RS2::Letter:
            return QPrinter::Letter;
        case RS2::Legal:
            return QPrinter::Legal;
        case RS2::Executive:
            return QPrinter::Executive;
        case RS2::A0:
            return QPrinter::A0;
        case RS2::A1:
            return QPrinter::A1;
        case RS2::A2:
            return QPrinter::A2;
        case RS2::A3:
            return QPrinter::A3;
        case RS2::A4:
            return QPrinter::A4;
        case RS2::A5:
            return QPrinter::A5;
        case RS2::A6:
            return QPrinter::A6;
        case RS2::A7:
            return QPrinter::A7;
        case RS2::A8:
            return QPrinter::A8;
        case RS2::A9:
            return QPrinter::A9;
        case RS2::B0:
            return QPrinter::B0;
        case RS2::B1:
            return QPrinter::B1;
        case RS2::B2:
            return QPrinter::B2;
        case RS2::B3:
            return QPrinter::B3;
        case RS2::B4:
            return QPrinter::B4;
        case RS2::B5:
            return QPrinter::B5;
        case RS2::B6:
            return QPrinter::B6;
        case RS2::B7:
            return QPrinter::B7;
        case RS2::B8:
            return QPrinter::B8;
        case RS2::B9:
            return QPrinter::B9;
        case RS2::B10:
            return QPrinter::B10;
        case RS2::C5E:
            return QPrinter::C5E;
        case RS2::Comm10E:
            return QPrinter::Comm10E;
        case RS2::DLE:
            return QPrinter::DLE;
        case RS2::Folio:
            return QPrinter::Folio;
        case RS2::Ledger:
            return QPrinter::Ledger;
        case RS2::Tabloid:
            return QPrinter::Tabloid;
        #if QT_VERSION >= 0x050400
         case RS2::Arch_A:
             return QPrinter::ArchA;
         case RS2::Arch_B:
             return QPrinter::ArchB;
         case RS2::Arch_C:
             return QPrinter::ArchC;
         case RS2::Arch_D:
             return QPrinter::ArchD;
         case RS2::Arch_E:
             return QPrinter::ArchE;
        #endif
        case RS2::NPageSize:
            return QPrinter::NPageSize;
    }
}
