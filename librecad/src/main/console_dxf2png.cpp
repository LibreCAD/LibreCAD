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


#include <QApplication>
#include <QCoreApplication>
#include <QImageWriter>
#include <QtCore>
#include <QtSvg>

#include "console_dxf2pdf.h"

#include "lc_printing.h"

#include "main.h"

#include "qc_applicationwindow.h"
#include "qg_dialogfactory.h"

#include "rs.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_painterqt.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_staticgraphicview.h"
#include "rs_system.h"


///////////////////////////////////////////////////////////////////////
/// \brief openDocAndSetGraphic opens a DXF file and prepares all its graphics content
/// for further manipulations
/// \return
//////////////////////////////////////////////////////////////////////
static bool openDocAndSetGraphic(RS_Document**, RS_Graphic**, QString&);

static void touchGraphic(RS_Graphic*);

static QSize parsePngSizeArg(QString);

bool slotFileExport(RS_Graphic* graphic,
                    const QString& name,
                    const QString& format,
                    QSize size,
                    QSize borders,
                    bool black,
                    bool bw=true);


/////////
/// \brief console_dxf2png is called if librecad
/// as console dxf2png tool for converting DXF to PNG.
/// \param argc
/// \param argv
/// \return
///
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
        XSTR(QC_APPDIR), prgDir.toLatin1().data());

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

    QCommandLineOption outFileOpt(QStringList() << "o" << "outfile",
        "Output PNG file.", "file");
    parser.addOption(outFileOpt);

    QCommandLineOption pngSizeOpt(QStringList() << "r" << "resolution",
        "Output PNG size (Width x Height) in pixels.", "WxH");
    parser.addOption(pngSizeOpt);

    parser.addPositionalArgument("<dxf_files>", "Input DXF file");

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty() || (args.size() == 1 && args[0] == "dxf2png"))
        parser.showHelp(EXIT_FAILURE);

    // Set PNG size from user input
    QSize pngSize = parsePngSizeArg(parser.value(pngSizeOpt)); // If nothing, use default values.

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

    QString& dxfFile = dxfFiles[0];

    QFileInfo dxfFileInfo(dxfFile);
    QString fn = dxfFileInfo.completeBaseName(); // original DXF file name
    if(fn.isEmpty())
        fn = "unnamed";

    // Set output filename from user input if present
    QString outFile = parser.value(outFileOpt);
    if (outFile.isEmpty()) {
        outFile = dxfFileInfo.path() + "/" + fn + ".png";
    } else {
        outFile = dxfFileInfo.path() + "/" + outFile;
    }

    // Open the file and process the graphics

    RS_Document *doc = nullptr;
    RS_Graphic *graphic = nullptr;

    if (!openDocAndSetGraphic(&doc, &graphic, dxfFile))
        return 1;

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
        if (format == "jpeg" || format == "tiff") {
            // Don't add the aliases
        } else {
            st = QString("%1 (%2)(*.%2)")
                    .arg(QG_DialogFactory::extToFormat(format))
                    .arg(format);
        }
        if (st.length() > 0)
            filters.push_back(st);
    }
    // revise list of filters
    filters.removeDuplicates();
    filters.sort();

    // find out extension:
    QString filter = "Portable Network Graphic (png)(*.png)";
    QString format = "";
    int i = filter.indexOf("(*.");
    if (i!=-1) {
        int i2 = filter.indexOf(QRegExp("[) ]"), i);
        format = filter.mid(i + 3, i2 - (i + 3));
        format = format.toUpper();
    }

    // append extension to file:
    if (!QFileInfo(fn).fileName().contains(".")) {
        fn.push_back("." + format.toLower());
    }

    QSize borders = QSize(5, 5);
    bool black = false;
    bool bw = false;

    bool ret = slotFileExport(graphic, outFile, format, pngSize, borders,
                black, bw);

    qDebug() << "Printing" << dxfFile << "to" << outFile << (ret ? "Done" : "Failed");
    return 0;
}


static bool openDocAndSetGraphic(RS_Document** doc, RS_Graphic** graphic,
                                 QString& dxfFile)
{
    *doc = new RS_Graphic();

    if (!(*doc)->open(dxfFile, RS2::FormatUnknown)) {
        qDebug() << "ERROR: Failed to open document" << dxfFile;
        qDebug() << "Check if file exists";
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

bool slotFileExport(RS_Graphic* graphic, const QString& name,
        const QString& format, QSize size, QSize borders, bool black, bool bw) {

    if (graphic==nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFileExport: "
                "no graphic");
        return false;
    }

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    bool ret = false;
    // set vars for normal pictures and vectors (svg)
    QPixmap* picture = new QPixmap(size);

    QSvgGenerator* vector = new QSvgGenerator();

    // set buffer var
    QPaintDevice* buffer;

    if(format.toLower() != "svg") {
        buffer = picture;
    } else {
        vector->setSize(size);
        vector->setViewBox(QRectF(QPointF(0,0),size));
        vector->setFileName(name);
        buffer = vector;
    }

    // set painter with buffer
    RS_PainterQt painter(buffer);

    if (black) {
        painter.setBackground( Qt::black);
        if (bw) {
            painter.setDrawingMode( RS2::ModeWB);
        }
    }
    else {
        painter.setBackground(Qt::white);
        if (bw) {
            painter.setDrawingMode( RS2::ModeBW);
        }
    }

    painter.eraseRect(0,0, size.width(), size.height());

    RS_StaticGraphicView gv(size.width(), size.height(), &painter, &borders);
    if (black) {
        gv.setBackground(Qt::black);
    } else {
        gv.setBackground(Qt::white);
    }
    gv.setContainer(graphic);
    gv.zoomAuto(false);
    gv.drawEntity(&painter, gv.getContainer());

    // end the picture output
    if(format.toLower() != "svg")
    {
        // RVT_PORT QImageIO iio;
        QImageWriter iio;
        QImage img = picture->toImage();
        // RVT_PORT iio.setImage(img);
        iio.setFileName(name);
        iio.setFormat(format.toLatin1());
        // RVT_PORT if (iio.write()) {
        if (iio.write(img)) {
            ret = true;
        }
//        QString error=iio.errorString();
    }
    QApplication::restoreOverrideCursor();

    // GraphicView deletes painter
    painter.end();
    // delete vars
    delete picture;
    delete vector;

    return ret;
}

/////////////////
/// \brief Parses the user input of PNG output resolution and
/// converts it to a vector value
/// \param arg - input string
/// \return
///
static QSize parsePngSizeArg(QString arg)
{
    QSize v(2000, 1000); // default resolution

    if (arg.isEmpty())
        return v;

    QRegularExpression re("^(?<width>\\d+)[x|X]{1}(?<height>\\d+)$");
    QRegularExpressionMatch match = re.match(arg);

    if (match.hasMatch()) {
        QString width = match.captured("width");
        QString height = match.captured("height");
        v.setWidth(width.toDouble());
        v.setHeight(height.toDouble());
    } else {
        qDebug() << "WARNING: Ignoring incorrect PNG resolution:" << arg;
    }

    return v;
}
