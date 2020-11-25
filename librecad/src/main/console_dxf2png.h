/******************************************************************************
**
** This file was created for the LibreCAD project, a 2D CAD program.
**
** Copyright (C) 2020 Nikita Letov <letovnn@gmail.com>
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
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
******************************************************************************/
#ifndef CONSOLE_DXF2PNG_H
#define CONSOLE_DXF2PNG_H

struct PngPrintParams {
        QStringList dxfFiles;
        QString outDir;
        QString outFile;
        int resolution = 1200;
        bool centerOnPage;
        bool fitToPage;
        bool monochrome;
        bool grayscale;
        double scale = 0.0;  // If scale <= 0.0, use value from dxf file.
        RS_Vector pageSize;  // If zeros, use value from dxf file.
        struct {
            double left = -1.0;
            double top = -1.0;
            double right = -1.0;
            double bottom = -1.0;
        } margins;           // If margin < 0.0, use value from dxf file.
        int pagesH = 0;      // If number of pages < 1,
        int pagesV = 0;      // use value from dxf file.
};


int console_dxf2png(int argc, char** argv);



#endif // CONSOLE_DXF2PNG_H
