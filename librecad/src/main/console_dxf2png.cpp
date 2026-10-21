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
#include <QtCore>

#include "console_command_utils.h"
#include "main.h"

#include "qc_applicationwindow.h"
#include "qg_dialogfactory.h"

#include "console_dxf2png.h"
#include "lc_imageexporter.h"
#include "main.h"
#include "qc_applicationwindow.h"
#include "qg_dialogfactory.h"
#include "rs.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"

///////////////////////////////////////////////////////////////////////
/// \brief openDocAndSetGraphic opens a DXF file and prepares all its graphics content
/// for further manipulations
/// \return
//////////////////////////////////////////////////////////////////////
static std::unique_ptr<RS_Document> openDocAndSetGraphic(const QString&);

static void touchGraphic(RS_Graphic*);

static bool parsePngSizeArg(const QString& arg, QSize& size);

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

    // Raster and vector output both go through slotFileExport: the same
    // viewport and painter as the application, so the exported file shows
    // what the application shows. LC_ActionFileExportMakerCam::writeSvg
    // writes only a few entity types and silently drops the rest, text above
    // all.
    const QSize borders{5, 5};
    const bool black = false;
    const bool bw = false;
    const bool ret = slotFileExport(graphic, outputFile, spec.outputExt.toUpper(),
                                    outputSize, borders, black, bw);

    if (!ret) {
        qCritical("ERROR: failed to write '%s'", qPrintable(outputFile));
        return false;
    }

    qDebug() << "Printing" << inputFile << "to" << outputFile << "Done";
    return true;
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
    if (spec.outputExt == "svg")
        appDesc << "SVG is fitted to the output canvas, not exported at 1:1 drawing-unit scale.";
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
    QSize pngSize;
    if (!parsePngSizeArg(parser.value(pngSizeOpt), pngSize)) {
        qCritical("ERROR: invalid output size '%s'; use WxH in pixels, such as 1920x1080.",
                  qPrintable(parser.value(pngSizeOpt)));
        return EXIT_FAILURE;
    }

    QStringList skippedArgs;
    const QStringList inputFiles =
        LC_Console::collectInputFiles(args, spec.acceptedExts, &skippedArgs);
    if (inputFiles.isEmpty()) {
        qCritical("ERROR: no %s files found in arguments.",
                  qPrintable(LC_Console::extensionDescription(spec.acceptedExts)));
        return EXIT_FAILURE;
    }
    for (const QString& skipped : skippedArgs) {
        qWarning("WARNING: '%s' is not a %s file and was skipped.", qPrintable(skipped),
                 qPrintable(LC_Console::extensionDescription(spec.acceptedExts)));
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

    QStringList outputFiles;
    for (const QString& inputFile : inputFiles) {
        outputFiles.append(outFile.isEmpty()
            ? LC_Console::defaultOutputPath(inputFile, spec.outputExt, outDir)
            : outFile);
    }

    QString outputTargetsError;
    if (!LC_Console::validateOutputTargets(inputFiles, outputFiles, &outputTargetsError)) {
        qCritical("ERROR: %s", qPrintable(outputTargetsError));
        return EXIT_FAILURE;
    }

    RS_FONTLIST->init();
    RS_PATTERNLIST->init();

    int failed = 0;
    for (int i = 0; i < inputFiles.size(); ++i) {
        if (!exportOneImageFile(spec, inputFiles.at(i), outputFiles.at(i), pngSize))
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

static std::unique_ptr<RS_Document> openDocAndSetGraphic(const QString& dxfFile){
    auto doc = std::make_unique<RS_Graphic>();
    // LC_Console::importGraphic() reports on stderr. Importing through the
    // document storage opens a message box no console command can close.
    if (!LC_Console::importGraphic(*doc, dxfFile))
        return {};

    const RS_Graphic* graphic = doc->getGraphic();
    if (graphic == nullptr) {
        qCritical("ERROR: no drawing in '%s'", qPrintable(dxfFile));
        return {};
    }

    return doc;
}

static void touchGraphic(RS_Graphic* graphic){
    // If margin < 0.0, values from dxf file are used.
    constexpr double marginLeft = -1.0;
    constexpr double marginTop = -1.0;
    constexpr double marginRight = -1.0;
    constexpr double marginBottom = -1.0;

    constexpr int pagesH = 0;      // If number of pages < 1,
    constexpr int pagesV = 0;      // use value from dxf file.

    graphic->calculateBorders();


    LC_PlotSettings* ps = graphic->getPlotSettings();
    ps->setMarginsInMm(marginLeft, marginTop,
                        marginRight, marginBottom);
    ps->setPagesNum(pagesH, pagesV);

    //if (params.pageSize != RS_Vector(0.0, 0.0))
    //    graphic->setPaperSize(params.pageSize);

    graphic->fitToPage(); // fit and center
}

bool slotFileExport(RS_Graphic* graphic, const QString& name,
        const QString& format, const QSize& size, const QSize& borders, const bool black, const bool bw) {
    return LC_ImageExporter{}.exportToImage(graphic, {name, format, size, borders, black, bw});
}

/////////////////
/// \brief Parses the user input of PNG output resolution
/// \param arg - input string, empty for the default size
/// \param size - receives the output size in pixels
/// \return false if the input is not a positive WxH size
///
static bool parsePngSizeArg(const QString& arg, QSize& size) {
    size = QSize(2000, 1000); // default resolution

    if (arg.isEmpty()) {
        return true;
    }

    const QRegularExpression re("^(?<width>\\d+)[xX](?<height>\\d+)$");
    const QRegularExpressionMatch match = re.match(arg);
    if (!match.hasMatch()) {
        return false;
    }

    const int width = match.captured("width").toInt();
    const int height = match.captured("height").toInt();
    if (width <= 0 || height <= 0) {
        return false;
    }

    size = QSize(width, height);
    return true;
}
