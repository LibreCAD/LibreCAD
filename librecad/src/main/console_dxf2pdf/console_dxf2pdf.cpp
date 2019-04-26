/******************************************************************************
**
** This file was created for the LibreCAD project, a 2D CAD program.
**
** Copyright (C) 2018 Alexander Pravdin <aledin@mail.ru>
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

#include "rs_debug.h"
#include "rs_fontlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"

#include "main.h"

#include "console_dxf2pdf.h"
#include "pdf_print_loop.h"


static RS_Vector parsePageSizeArg(QString);
static void parsePagesNumArg(QString, PdfPrintParams&);
static void parseMarginsArg(QString, PdfPrintParams&);


int console_dxf2pdf(int argc, char* argv[])
{
    //QT_REQUIRE_VERSION(argc, argv, "5.2.1");

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
    if (prgInfo.baseName() != "dxf2pdf") {
        librecad = prgInfo.filePath();
        appDesc = "\ndxf2pdf usage: " + prgInfo.filePath()
            + " dxf2pdf [options] <dxf_files>\n";
    }
    appDesc += "\nPrint a bunch of DXF files to PDF file(s).";
    appDesc += "\n\n";
    appDesc += "Examples:\n\n";
    appDesc += "  " + librecad + " dxf2pdf *.dxf";
    appDesc += "    -- print all dxf files to pdf files with the same names.\n";
    appDesc += "\n";
    appDesc += "  " + librecad + " dxf2pdf -o some.pdf *.dxf";
    appDesc += "    -- print all dxf files to 'some.pdf' file.";
    parser.setApplicationDescription(appDesc);

    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fitOpt(QStringList() << "a" << "fit",
        "Auto fit and center drawing to page.");
    parser.addOption(fitOpt);

    QCommandLineOption centerOpt(QStringList() << "c" << "center",
        "Auto center drawing on page.");
    parser.addOption(centerOpt);

    QCommandLineOption grayOpt(QStringList() << "k" << "grayscale",
        "Print grayscale.");
    parser.addOption(grayOpt);

    QCommandLineOption monoOpt(QStringList() << "m" << "monochrome",
        "Print monochrome (black/white).");
    parser.addOption(monoOpt);

    QCommandLineOption pageSizeOpt(QStringList() << "p" << "paper",
        "Paper size (Width x Height) in mm.", "WxH");
    parser.addOption(pageSizeOpt);

    QCommandLineOption resOpt(QStringList() << "r" << "resolution",
        "Output resolution (DPI).", "integer");
    parser.addOption(resOpt);

    QCommandLineOption scaleOpt(QStringList() << "s" << "scale",
        "Output scale. E.g.: 0.01 (for 1:100 scale).", "double");
    parser.addOption(scaleOpt);

    QCommandLineOption marginsOpt(QStringList() << "f" << "margins",
        "Paper margins in mm (integer or float).", "L,T,R,B");
    parser.addOption(marginsOpt);

    QCommandLineOption pagesNumOpt(QStringList() << "z" << "pages",
        "Print on multiple pages (Horiz. x Vert.).", "HxV");
    parser.addOption(pagesNumOpt);

    QCommandLineOption outFileOpt(QStringList() << "o" << "outfile",
        "Output PDF file.", "file");
    parser.addOption(outFileOpt);

    QCommandLineOption outDirOpt(QStringList() << "t" << "directory",
        "Target output directory.", "path");
    parser.addOption(outDirOpt);

    parser.addPositionalArgument("<dxf_files>", "Input DXF file(s)");

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty() || (args.size() == 1 && args[0] == "dxf2pdf"))
        parser.showHelp(EXIT_FAILURE);

    PdfPrintParams params;

    params.fitToPage = parser.isSet(fitOpt);
    params.centerOnPage = parser.isSet(centerOpt);
    params.grayscale = parser.isSet(grayOpt);
    params.monochrome = parser.isSet(monoOpt);
    params.pageSize = parsePageSizeArg(parser.value(pageSizeOpt));

    bool resOk;
    int res = parser.value(resOpt).toInt(&resOk);
    if (resOk)
        params.resolution = res;

    bool scaleOk;
    double scale = parser.value(scaleOpt).toDouble(&scaleOk);
    if (scaleOk)
        params.scale = scale;

    parseMarginsArg(parser.value(marginsOpt), params);
    parsePagesNumArg(parser.value(pagesNumOpt), params);

    params.outFile = parser.value(outFileOpt);
    params.outDir = parser.value(outDirOpt);

    for (auto arg : args) {
        QFileInfo dxfFileInfo(arg);
        if (dxfFileInfo.suffix().toLower() != "dxf")
            continue; // Skip files without .dxf extension
        params.dxfFiles.append(arg);
    }

    if (params.dxfFiles.isEmpty())
        parser.showHelp(EXIT_FAILURE);

    if (!params.outDir.isEmpty()) {
        // Create output directory
        if (!QDir().mkpath(params.outDir)) {
            qDebug() << "ERROR: Cannot create directory" << params.outDir;
            return EXIT_FAILURE;
        }
    }

    RS_FONTLIST->init();
    RS_PATTERNLIST->init();

    PdfPrintLoop *loop = new PdfPrintLoop(params, &app);

    QObject::connect(loop, SIGNAL(finished()), &app, SLOT(quit()));

    QTimer::singleShot(0, loop, SLOT(run()));

    return app.exec();
}


static RS_Vector parsePageSizeArg(QString arg)
{
    RS_Vector v(0.0, 0.0);

    if (arg.isEmpty())
        return v;

    QRegularExpression re("^(?<width>\\d+)[x|X]{1}(?<height>\\d+)$");
    QRegularExpressionMatch match = re.match(arg);

    if (match.hasMatch()) {
        QString width = match.captured("width");
        QString height = match.captured("height");
        v.x = width.toDouble();
        v.y = height.toDouble();
    } else {
        qDebug() << "WARNING: Ignoring bad page size:" << arg;
    }

    return v;
}


static void parsePagesNumArg(QString arg, PdfPrintParams& params)
{
    if (arg.isEmpty())
        return;

    QRegularExpression re("^(?<horiz>\\d+)[x|X](?<vert>\\d+)$");
    QRegularExpressionMatch match = re.match(arg);

    if (match.hasMatch()) {
        QString h = match.captured("horiz");
        QString v = match.captured("vert");
        params.pagesH = h.toInt();
        params.pagesV = v.toInt();
    } else {
        qDebug() << "WARNING: Ignoring bad number of pages:" << arg;
    }
}


static void parseMarginsArg(QString arg, PdfPrintParams& params)
{
    if (arg.isEmpty())
        return;

    QRegularExpression re("^(?<left>\\d+(?:\\.\\d+)?),"
                          "(?<top>\\d+(?:\\.\\d+)?),"
                          "(?<right>\\d+(?:\\.\\d+)?),"
                          "(?<bottom>\\d+(?:\\.\\d+)?)$");
    QRegularExpressionMatch match = re.match(arg);

    if (match.hasMatch()) {
        QString left = match.captured("left");
        QString top = match.captured("top");
        QString right = match.captured("right");
        QString bottom = match.captured("bottom");
        params.margins.left = left.toDouble();
        params.margins.top = top.toDouble();
        params.margins.right = right.toDouble();
        params.margins.bottom = bottom.toDouble();
    } else {
        qDebug() << "WARNING: Ignoring bad paper margins:" << arg;
    }
}
