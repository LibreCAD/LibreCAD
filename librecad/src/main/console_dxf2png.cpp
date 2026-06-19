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

#include <cstdlib>
#include <memory>

#include <QApplication>
#include <QCoreApplication>
#include <QImageWriter>
#include <QtCore>
#include <QtSvg>

#include "console_command_utils.h"
#include "main.h"

#include "qc_applicationwindow.h"
#include "qg_dialogfactory.h"

#include "lc_actionfileexportmakercam.h"
#include "lc_documentsstorage.h"
#include "lc_graphicviewport.h"
#include "rs.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_painter.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "lc_printviewportrenderer.h"


///////////////////////////////////////////////////////////////////////
/// \brief openDocAndSetGraphic opens a DXF file and prepares all its graphics content
/// for further manipulations
/// \return
//////////////////////////////////////////////////////////////////////
static std::unique_ptr<RS_Document> openDocAndSetGraphic(QString);

static void touchGraphic(RS_Graphic*);

static QSize parsePngSizeArg(QString);

bool slotFileExport(RS_Graphic* graphic,
                    const QString& name,
                    const QString& format,
                    QSize size,
                    QSize borders,
                    bool black,
                    bool bw=true);

namespace {

struct ImageCommandSpec {
    QString commandName;
    QString inputExt;
    QString inputLabel;
    QString outputExt;
    QString outputLabel;
    QStringList acceptedExts;
};

bool exportOneImageFile(const ImageCommandSpec& spec, const QString& inputFile,
                        const QString& outputFile, const QSize& outputSize) {
    std::unique_ptr<RS_Document> doc = openDocAndSetGraphic(inputFile);
    if (doc == nullptr || doc->getGraphic() == nullptr)
        return false;

    RS_Graphic *graphic = doc->getGraphic();

    LC_LOG << "Printing" << inputFile << "to" << outputFile << ">>>>";

    touchGraphic(graphic);

    bool ret = false;
    if (spec.outputExt.compare(QStringLiteral("svg"), Qt::CaseInsensitive) == 0) {
        ret = LC_ActionFileExportMakerCam::writeSvg(outputFile, *graphic);
    } else {
        QSize borders = QSize(5, 5);
        bool black = false;
        bool bw = false;
        ret = slotFileExport(graphic, outputFile, spec.outputExt.toUpper(), outputSize,
                             borders, black, bw);
    }

    qDebug() << "Printing" << inputFile << "to" << outputFile
             << (ret ? "Done" : "Failed");
    return ret;
}

int runImageCommand(int argc, char* argv[], const ImageCommandSpec& spec) {
    RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

    const LC_Console::CommandContext context =
        LC_Console::contextForCommand(argc, argv, spec.commandName);
    LC_Console::NormalizedArgv normalizedArgs(argc, argv, context);
    int normalizedArgc = normalizedArgs.argc();
    char** normalizedArgv = normalizedArgs.argv();

    QApplication app(normalizedArgc, normalizedArgv);
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD");
    QCoreApplication::setApplicationVersion(XSTR(LC_VERSION));

    QFileInfo prgInfo(QFile::decodeName(normalizedArgv[0]));
    const QString prgDir(prgInfo.absolutePath());
    const QByteArray prgDirBytes = prgDir.toLatin1();
    RS_Settings::init(app.organizationName(), app.applicationName());
    RS_SYSTEM->init(app.applicationName(), app.applicationVersion(),
                    XSTR(QC_APPDIR), prgDirBytes.constData());

    QCommandLineParser parser;

    QStringList appDesc;
    const QString command = context.displayCommand();
    appDesc << "";
    appDesc << spec.commandName + " usage: " + command +
                   QString(" [options] <%1_files>").arg(spec.inputExt);
    appDesc << "";
    appDesc << QString("Print %1 file(s) to %2 file(s).")
                   .arg(spec.inputLabel, spec.outputLabel);
    if (spec.inputExt == "dxf")
        appDesc << QString("DWG input is accepted for compatibility; prefer dwg2%1 for DWG files.")
                       .arg(spec.outputExt);
    appDesc << "";
    appDesc << "Examples:";
    appDesc << "";
    appDesc << "  " + command + QString(" *.%1").arg(spec.inputExt);
    appDesc << "    -- print all input files to " + spec.outputLabel +
                   " files with the same names.";
    parser.setApplicationDescription(appDesc.join("\n"));

    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption outFileOpt(QStringList() << "o" << "output" << "outfile",
        "Output file (single input only).", "file");
    parser.addOption(outFileOpt);

    QCommandLineOption pngSizeOpt(QStringList() << "r" << "resolution",
        "Output size (Width x Height) in pixels.", "WxH");
    parser.addOption(pngSizeOpt);

    QCommandLineOption outDirOpt(QStringList() << "t" << "directory",
        "Target output directory.", "path");
    parser.addOption(outDirOpt);

    parser.addPositionalArgument("<" + spec.inputExt + "_files>",
        "Input " + spec.inputLabel + " file(s)");

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty())
        parser.showHelp(EXIT_FAILURE);

    // Set PNG size from user input
    QSize pngSize = parsePngSizeArg(parser.value(pngSizeOpt)); // If nothing, use default values.

    const QStringList inputFiles = LC_Console::collectInputFiles(args, spec.acceptedExts);
    if (inputFiles.isEmpty()) {
        qCritical("ERROR: no %s files found in arguments.",
                  qPrintable(LC_Console::extensionDescription(spec.acceptedExts)));
        return EXIT_FAILURE;
    }

    if (LC_Console::containsDwgInput(inputFiles) &&
        !LC_Console::dwgSupportAvailable()) {
        qCritical("ERROR: DWG input requires a build with DWGSUPPORT enabled.");
        return EXIT_FAILURE;
    }

    const QString outFile = parser.value(outFileOpt);
    const QString outDir = parser.value(outDirOpt);
    QString outputOptionsError;
    if (!LC_Console::validateOutputOptions(inputFiles.size(), outFile, outDir,
                                           false, false,
                                           &outputOptionsError)) {
        qCritical("ERROR: %s", qPrintable(outputOptionsError));
        return EXIT_FAILURE;
    }

    QString dirError;
    if (!LC_Console::ensureOutputDirectory(outDir, &dirError)) {
        qCritical("ERROR: %s.", qPrintable(dirError));
        return EXIT_FAILURE;
    }

    RS_FONTLIST->init();
    RS_PATTERNLIST->init();

    int failed = 0;
    for (const QString& inputFile : inputFiles) {
        const QString outputFile = outFile.isEmpty()
            ? LC_Console::defaultOutputPath(inputFile, spec.outputExt, outDir)
            : outFile;
        if (!exportOneImageFile(spec, inputFile, outputFile, pngSize))
            ++failed;
    }

    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace

int console_dxf2png(int argc, char* argv[])
{
    return runImageCommand(argc, argv,
        {QStringLiteral("dxf2png"), QStringLiteral("dxf"), QStringLiteral("DXF"),
         QStringLiteral("png"), QStringLiteral("PNG"),
         LC_Console::acceptedExtensions(QStringLiteral("dxf"), {QStringLiteral("dwg")})});
}

int console_dwg2png(int argc, char* argv[])
{
    return runImageCommand(argc, argv,
        {QStringLiteral("dwg2png"), QStringLiteral("dwg"), QStringLiteral("DWG"),
         QStringLiteral("png"), QStringLiteral("PNG"),
         LC_Console::acceptedExtensions(QStringLiteral("dwg"))});
}

int console_dxf2svg(int argc, char* argv[])
{
    return runImageCommand(argc, argv,
        {QStringLiteral("dxf2svg"), QStringLiteral("dxf"), QStringLiteral("DXF"),
         QStringLiteral("svg"), QStringLiteral("SVG"),
         LC_Console::acceptedExtensions(QStringLiteral("dxf"), {QStringLiteral("dwg")})});
}

int console_dwg2svg(int argc, char* argv[])
{
    return runImageCommand(argc, argv,
        {QStringLiteral("dwg2svg"), QStringLiteral("dwg"), QStringLiteral("DWG"),
         QStringLiteral("svg"), QStringLiteral("SVG"),
         LC_Console::acceptedExtensions(QStringLiteral("dwg"))});
}

static std::unique_ptr<RS_Document> openDocAndSetGraphic(QString dxfFile){
    auto doc = std::make_unique<RS_Graphic>();
    LC_DocumentsStorage storage;
    if (!storage.loadDocument(doc.get(), dxfFile, RS2::FormatUnknown)) {
    // if (!doc->open(dxfFile, RS2::FormatUnknown)) {
        qDebug() << "ERROR: Failed to open document" << dxfFile;
        qDebug() << "Check if file exists";
        return {};
    }

    RS_Graphic* graphic = doc->getGraphic();
    if (graphic == nullptr) {
        qDebug() << "ERROR: No graphic in" << dxfFile;
        return {};
    }

    return doc;
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
    RS_Painter painter(buffer);

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

    LC_GraphicViewport viewport;
    viewport.setSize(size.width(), size.height());
    viewport.setBorders(borders.width(), borders.height(), borders.width(), borders.height());

    viewport.setContainer(graphic);
    viewport.loadSettings();
    viewport.zoomAuto(false);


    LC_PrintViewportRenderer renderer(&viewport, &painter);
    renderer.loadSettings();

    if (black) {
        renderer.setBackground(Qt::black);
    } else {
        renderer.setBackground(Qt::white);
    }

    renderer.render();

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
