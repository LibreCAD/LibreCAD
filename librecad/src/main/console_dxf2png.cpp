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


#include <QtCore>
#include <QCoreApplication>
#include <QApplication>

#include <QImageWriter>

#include "rs.h"
#include "rs_graphic.h"
#include "rs_painterqt.h"
#include "lc_printing.h"
#include "rs_staticgraphicview.h"

#include "rs_debug.h"
#include "rs_fontlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"

#include "rs_document.h"

#include "qg_dialogfactory.h"

#include "main.h"

#include "console_dxf2pdf.h"

static bool openDocAndSetGraphic(RS_Document**, RS_Graphic**, QString&);
static void touchGraphic(RS_Graphic*);


int console_dxf2png(int argc, char* argv[])
{
    RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD");
    QCoreApplication::setApplicationVersion(XSTR(LC_VERSION));

    QFileInfo prgInfo(QFile::decodeName(argv[0]));
    QString prgDir(prgInfo.absolutePath());
    RS_SETTINGS->init(app.organizationName(), app.applicationName());
    RS_SYSTEM->init(app.applicationName(), app.applicationVersion(),
        XSTR(QC_APPDIR), prgDir);

    QCommandLineParser parser;

    QString appDesc;
    QString librecad;
    if (prgInfo.baseName() != "dxf2png") {
        librecad = prgInfo.filePath();
        appDesc = "\ndxf2png usage: " + prgInfo.filePath()
            + " dxf2png [options] <dxf_files>\n";
    }
    appDesc += "\nPrint a DXF file to a PNG file.";
    appDesc += "\n\n";
    appDesc += "Examples:\n\n";
    appDesc += "  " + librecad + " dxf2png *.dxf";
    appDesc += "    -- print a dxf file to a png file with the same name.\n";
    parser.setApplicationDescription(appDesc);

    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("<dxf_files>", "Input DXF file");

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty() || (args.size() == 1 && args[0] == "dxf2png"))
        parser.showHelp(EXIT_FAILURE);

    QStringList dxfFiles;

    for (auto arg : args) {
        QFileInfo dxfFileInfo(arg);
        if (dxfFileInfo.suffix().toLower() != "dxf")
            continue; // Skip files without .dxf extension
        dxfFiles.append(arg);
    }

    if (dxfFiles.isEmpty())
        parser.showHelp(EXIT_FAILURE);

    // Output setup

    bool ret = false;
    QString& dxfFile = dxfFiles[0];

    QFileInfo dxfFileInfo(dxfFile);
    QString fn = dxfFileInfo.completeBaseName();

    QString outFile = dxfFileInfo.path()
            + "/" + fn + ".png";

    if(fn==nullptr)
        fn = "unnamed";

    // Open the file and process the graphics

    RS_Document *doc;
    RS_Graphic *graphic;

    if (!openDocAndSetGraphic(&doc, &graphic, dxfFile))
        app.exec();

    qDebug() << "Printing" << dxfFile << "to" << outFile << ">>>>";

    touchGraphic(graphic);



    // Start of the actual conversion

    RS_DEBUG->print("QC_ApplicationWindow::slotFileExport()");

    // read default settings:
    RS_SETTINGS->beginGroup("/Export");
    QString defDir = dxfFileInfo.path();

    RS_SETTINGS->endGroup();

    QStringList filters;
    QList<QByteArray> supportedImageFormats = QImageWriter::supportedImageFormats();
    supportedImageFormats.push_back("svg"); // add svg

    for (QString format: supportedImageFormats) {
        format = format.toLower();
        QString st;
        if (format=="jpeg" || format=="tiff") {
            // Don't add the aliases
        } else {
            st = QString("%1 (%2)(*.%2)")
                    .arg(QG_DialogFactory::extToFormat(format))
                    .arg(format);
        }
        if (st.length()>0)
            filters.push_back(st);
    }

    // revise list of filters
    filters.removeDuplicates();
    filters.sort();

    // find out extension:
    QString filter = "Portable Network Graphic (png)(*.png)";
    QString format = "";




    return app.exec();
}

static bool openDocAndSetGraphic(RS_Document** doc, RS_Graphic** graphic,
    QString& dxfFile)
{
    *doc = new RS_Graphic();

    if (!(*doc)->open(dxfFile, RS2::FormatUnknown)) {
        qDebug() << "ERROR: Failed to open document" << dxfFile;
        delete *doc;
        return false;
    }

    *graphic = (*doc)->getGraphic();
    if (*graphic == nullptr) {
        qDebug() << "ERROR: No graphic in" << dxfFile;
        delete *doc;
        return false;
    }

    return true;
}

static void touchGraphic(RS_Graphic* graphic)
{
    // If margin < 0.0, values from dxf file are used.
    double marginLeft = -1.0;
    double marginTop = -1.0;
    double marginRight = -1.0;
    double marginBottom = -1.0;

    int pagesH = 0;      // If number of pages < 1,
    int pagesV = 0;      // use value from dxf file.

    graphic->calculateBorders();
    graphic->setMargins(marginLeft, marginTop,
                        marginRight, marginBottom);
    graphic->setPagesNum(pagesH, pagesV);

    //if (params.pageSize != RS_Vector(0.0, 0.0))
    //    graphic->setPaperSize(params.pageSize);

    graphic->fitToPage(); // fit and center
}


