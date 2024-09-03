/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 Dongxu Li (dongxuli2011@gmail.com)

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

#ifndef LC_PRINTING_H
#define LC_PRINTING_H

#include <QPageSize>

#include "rs.h"

class RS_Graphic;
class QC_MDIWindow;

namespace LC_Printing
{
    enum class PrinterType { Printer, PDF };
    QPageSize::PageSizeId rsToQtPaperFormat(RS2::PaperFormat f);

    /**
     * @brief Print - the implementation of drawing printing
     * @param mdiWindow - the mdiWindow to print
     * @param printerType - whether printing to a printer or a PDF file
     */
    void Print(QC_MDIWindow &mdiWindow, PrinterType printerType);
}

#endif // LC_PRINTING_H
