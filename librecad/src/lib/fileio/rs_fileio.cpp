/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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

#include <QFileInfo>
#include <QTextStream>
#include <memory>
#include <cstddef>
#include "rs_fileio.h"


  RS_FileIO::RS_FileIO()
  {
      filters.clear();
  }


/**
 * Calls the import method of the filter responsible for the format
 * of the given file.
 *
 * @param graphic The container to which we will add
 *        entities. Usually that's an RS_Graphic entity but
 *        it can also be a polyline, text, ...
 * @param file Path and name of the file to import.
 */
bool RS_FileIO::fileImport(RS_Graphic& graphic, const QString& file,
        RS2::FormatType type) {

    RS_DEBUG->print("Trying to import file '%s'...", file.toLatin1().data());

    RS2::FormatType t;
    if (type == RS2::FormatUnknown) {
        t = detectFormat(file);
    }
    else {
        t = type;
    }

    std::unique_ptr<RS_FilterInterface> filter(getImportFilter(file, t));
    if (filter.get() != nullptr){
        return filter->fileImport(graphic, file, t);
    }
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_FileIO::fileImport: failed to import file: %s",
                    file.toLatin1().data());

    return false;
}



/**
 * Calls the export method of the object responsible for the format
 * of the given file.
 *
 * @param file Path and name of the file to import.
 */
bool RS_FileIO::fileExport(RS_Graphic& graphic, const QString& file,
        RS2::FormatType type) {

    RS_DEBUG->print("RS_FileIO::fileExport");
    //RS_DEBUG->print("Trying to export file '%s'...", file.latin1());

    if (type==RS2::FormatUnknown) {
        QString extension;
        extension = QFileInfo(file).suffix().toLower();

        if (extension=="dxf") {
            type = RS2::FormatDXF;
        }
        else if (extension=="cxf") {
            type = RS2::FormatCXF;
        }
    }

    std::unique_ptr<RS_FilterInterface> filter(getExportFilter(file, type));
    if (filter.get() != nullptr){
        return filter->fileExport(graphic, file, type);
    }
    RS_DEBUG->print("RS_FileIO::fileExport: no filter found");

    return false;
}


/**
 * Detects and returns the file format of the given file.
 */
RS2::FormatType RS_FileIO::detectFormat(const QString& file) {
    RS2::FormatType type = RS2::FormatUnknown;
    QFileInfo fi(file);

    QString ext = fi.suffix().toLower();
    if (ext=="lff") {
        type = RS2::FormatLFF;
    } else if (ext=="cxf") {
        type = RS2::FormatCXF;
    } else if (ext=="dxf") {
        type = RS2::FormatDXF1;
        QFile f(file);

        if (!f.open(QIODevice::ReadOnly)) {
            // Error opening file:
            RS_DEBUG->print(RS_Debug::D_WARNING,
                                "RS_FileIO::detectFormat: Cannot open file: %s", file.toLatin1().data());
            type = RS2::FormatUnknown;
        } else {
            RS_DEBUG->print("RS_FileIO::detectFormat: "
                "Successfully opened DXF file: %s",
                file.toLatin1().data());

            QTextStream ts(&f);
            QString line;
            int c=0;
            while (!ts.atEnd() && ++c<100) {
                line = ts.readLine();
                if (line=="$ACADVER") {
#ifdef USE_DXFRW
                    type = RS2::FormatDXFRW;
#else
                    type = RS2::FormatDXF;
#endif
                }
                // very simple reduced DXF:
                if (line=="ENTITIES" && c<10) {
#ifdef USE_DXFRW
                    type = RS2::FormatDXFRW;
#else
                    type = RS2::FormatDXF;
#endif
                }
            }
            f.close();
        }
    }

    return type;
}


// EOF
