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

#include <QSize>
#include <QString>

class RS_Graphic;

int console_dxf2png(int argc, char* argv[]);
int console_dwg2png(int argc, char* argv[]);
int console_dxf2svg(int argc, char* argv[]);
int console_dwg2svg(int argc, char* argv[]);

// Paint the graphic with RS_Painter into the named file. Raster formats are
// written as images; "SVG" is painted through a QSvgGenerator. Returns true
// only if the file was written.
bool slotFileExport(RS_Graphic* graphic,
                    const QString& name,
                    const QString& format,
                    const QSize& size,
                    const QSize& borders,
                    bool black,
                    bool bw = true);

#endif
