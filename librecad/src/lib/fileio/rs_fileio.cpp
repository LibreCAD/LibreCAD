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

#include <cstddef>
#include <QFileInfo>
#include <QTextStream>
#ifdef DWGSUPPORT
#include <QMessageBox>
#include <QApplication>
#endif
#include "rs_fileio.h"
#include "rs_filtercxf.h"
#include "rs_filterdxf1.h"
#include "rs_filterjww.h"
#include "rs_filterlff.h"
#include "rs_filterdxfrw.h"

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

    if (RS2::FormatUnknown != t) {
		std::unique_ptr<RS_FilterInterface>&& filter(getImportFilter(file, t));
		if (filter){
#ifdef DWGSUPPORT
            if (file.endsWith(".dwg",Qt::CaseInsensitive)){
                QMessageBox::StandardButton sel = QMessageBox::warning(qApp->activeWindow(), QObject::tr("Warning"),
                                                  QObject::tr("experimental, save your work first.\nContinue?"),
                                                  QMessageBox::Ok|QMessageBox::Cancel, QMessageBox::NoButton);
                if (sel == QMessageBox::Cancel)
                    return false;
            }
#endif
            return filter->fileImport(graphic, file, t);
        }
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FileIO::fileImport: failed to import file: %s",
                        file.toLatin1().data());
    }
    else {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_FileIO::fileImport: failed to detect file format: %s",
                        file.toLatin1().data());
    }

    return false;
}


/** \brief extension2Type convert extension to file format type
 * \param file type
 * \param verifyRead read the file to verify dxf/dxfrw type, default to false
 * \return file format type
 */
RS2::FormatType RS_FileIO::detectFormat(QString const& file, bool forRead)
{
	// look up table
	std::map<QString, RS2::FormatType> list{
		{"dxf", RS2::FormatDXFRW},
		{"cxf", RS2::FormatCXF},
		{"lff", RS2::FormatLFF}
	};
	// only read support for dwg
	if(forRead) list["dwg"]=RS2::FormatDWG;

	QString const extension = QFileInfo(file).suffix().toLower();
	RS2::FormatType type=(list.find(extension)!=
			list.end()) ? list[extension]:RS2::FormatUnknown;

	//only read dxf to verify
	if (forRead && type==RS2::FormatDXFRW) {
		type = RS2::FormatDXFRW;
		QFile f(file);

		if (!f.open(QIODevice::ReadOnly)) {
			// Error opening file:
			RS_DEBUG->print(RS_Debug::D_WARNING,
							"%s:"
							"Cannot open file: %s",
							__func__,
							file.toLatin1().data());
			type = RS2::FormatUnknown;
		} else {
			RS_DEBUG->print("%s:"
							"Successfully opened DXF file: %s",
							__func__,
							file.toLatin1().data());

			QTextStream ts(&f);
			QString line;
			int c=0;
			while (!ts.atEnd() && ++c<100) {
				line = ts.readLine();
				if (line=="$ACADVER" || line=="ENTITIES") {
					type = RS2::FormatDXFRW;
					break;
				}
				// very simple reduced DXF:
				//                if (line=="ENTITIES" && c<10) {
				//                    type = RS2::FormatDXFRW;
				//                }
			}
			f.close();
		}
	}
	return type;
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
		type=detectFormat(file, false);
    }

	std::unique_ptr<RS_FilterInterface>&& filter(getExportFilter(file, type));
	if (filter){
        return filter->fileExport(graphic, file, type);
    }
    RS_DEBUG->print("RS_FileIO::fileExport: no filter found");

    return false;
}


RS_FileIO* RS_FileIO::instance() {
	static RS_FileIO* uniqueInstance=nullptr;
	if (!uniqueInstance) {
		uniqueInstance = new RS_FileIO();
	}
	return uniqueInstance;
}

/**
 * @return Filter which can import the given file type.
 */
std::unique_ptr<RS_FilterInterface> RS_FileIO::getImportFilter(const QString &fileName,
									RS2::FormatType t) const{
	for(auto f: getFilters()){
		std::unique_ptr<RS_FilterInterface> filter(f());
		if(filter &&
				filter->canImport(fileName, t)){
			return filter;
		}
	}
	return nullptr;
}

/**
 * @return Filter which can export the given file type.
 */
std::unique_ptr<RS_FilterInterface> RS_FileIO::getExportFilter(const QString &fileName,
									RS2::FormatType t) const{
	for(auto f: getFilters()){
		std::unique_ptr<RS_FilterInterface> filter(f());
		if(filter &&
				filter->canExport(fileName, t)){
			return filter;
		}
	}
	return nullptr;
}

std::vector<std::function<RS_FilterInterface*()>> RS_FileIO::getFilters()
{
												  return {
												  RS_FilterLFF::createFilter
												  ,RS_FilterDXFRW::createFilter
												  ,RS_FilterCXF::createFilter
												  ,RS_FilterJWW::createFilter
												  ,RS_FilterDXF1::createFilter
												  };
}

// EOF
