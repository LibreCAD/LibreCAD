/******************************************************************************
**
** This file was created for the LibreCAD project, a 2D CAD program.
**
** Copyright (C) 2018 Alexander Pravdin <aledin@mail.ru>
** Copyright (C) 2022 A. Stebich (librecad@mail.lordofbikes.de)
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

#include "console_dxf2pdf.h"

#include <QApplication>
#include <QCoreApplication>
#include <QtCore>

#include "main.h"
#include "pdf_print_loop.h"
#include "rs_debug.h"
#include "rs_fontlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"

#include "console_command_utils.h"
#include "main.h"

#include "console_dxf2pdf.h"
#include "pdf_print_loop.h"


static bool parsePageSizeArg(const QString& arg, RS_Vector& pageSize);
static bool parsePagesNumArg(const QString&, PdfPrintParams&);
static bool parseMarginsArg(const QString&, PdfPrintParams&);

namespace {

struct PdfCommandSpec {
    QString commandName;
    QString inputExt;
    QString inputLabel;
    QStringList acceptedExts;
};

int runPdfCommand(int argc, char* argv[], const PdfCommandSpec& spec) {
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

    RS_Settings::init(app.organizationName(), app.applicationName());
    RS_SYSTEM->init(app.applicationName(), app.applicationVersion(), XSTR(QC_APPDIR),
                    normalizedArgv[0]);

    QCommandLineParser parser;

    QStringList appDesc;
    const QString command = context.displayCommand();
    appDesc << "";
    appDesc << spec.commandName + " " + QObject::tr("usage: ") + command +
                   QObject::tr(" [options] <%1_files>").arg(spec.inputExt);
    appDesc << "";
    appDesc << QObject::tr("Print %1 file(s) to PDF file(s).").arg(spec.inputLabel);
    if (spec.commandName == "dxf2pdf")
        appDesc << QObject::tr("DWG input is accepted for compatibility; prefer dwg2pdf for DWG files.");
    appDesc << "";
    appDesc << "Examples:";
    appDesc << "";
    appDesc << "  " + command + QObject::tr(" *.%1").arg(spec.inputExt);
    appDesc << "    " + QObject::tr("-- print all %1 files to PDF files with the same names.").arg(spec.inputExt.toUpper());
    appDesc << "";
    appDesc << "  " + command + QObject::tr(" -o some.pdf *.%1").arg(spec.inputExt);
    appDesc << "    " + QObject::tr("-- print all %1 files to 'some.pdf'.").arg(spec.inputExt.toUpper());
    parser.setApplicationDescription( appDesc.join( "\n"));

    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fitOpt(QStringList() << "a" << "fit",
        QObject::tr( "Auto fit and center drawing to page."));
    parser.addOption(fitOpt);

    QCommandLineOption centerOpt(QStringList() << "c" << "center",
        QObject::tr( "Auto center drawing on page."));
    parser.addOption(centerOpt);

    QCommandLineOption grayOpt(QStringList() << "k" << "grayscale",
        QObject::tr( "Print grayscale."));
    parser.addOption(grayOpt);

    QCommandLineOption monoOpt(QStringList() << "m" << "monochrome",
        QObject::tr( "Print monochrome (black/white)."));
    parser.addOption(monoOpt);

    QCommandLineOption pageSizeOpt(QStringList() << "p" << "paper",
        QObject::tr( "Paper size (Width x Height) in mm."), "WxH");
    parser.addOption(pageSizeOpt);

    QCommandLineOption resOpt(QStringList() << "r" << "resolution",
        QObject::tr( "Output resolution (DPI)."), "integer");
    parser.addOption(resOpt);

    QCommandLineOption scaleOpt(QStringList() << "s" << "scale",
        QObject::tr( "Output scale. E.g.: 0.01 (for 1:100 scale)."), "double");
    parser.addOption(scaleOpt);

    QCommandLineOption marginsOpt(QStringList() << "f" << "margins",
        QObject::tr( "Paper margins in mm (integer or float)."), "L,T,R,B");
    parser.addOption(marginsOpt);

    QCommandLineOption pagesNumOpt(QStringList() << "z" << "pages",
        QObject::tr( "Print on multiple pages (Horiz. x Vert.)."), "HxV");
    parser.addOption(pagesNumOpt);

    QCommandLineOption outFileOpt(QStringList() << "o" << "output" << "outfile",
        QObject::tr( "Output PDF file.", "file"), "outfile");
    parser.addOption(outFileOpt);

    QCommandLineOption outDirOpt(QStringList() << "t" << "directory",
        QObject::tr( "Target output directory."), "path");
    parser.addOption(outDirOpt);

    parser.addPositionalArgument(QObject::tr("<%1_files>").arg(spec.inputExt),
        QObject::tr("Input %1 file(s)").arg(spec.inputLabel));

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty() || (args.size() == 1 && args[0] == "dxf2pdf")) {
        parser.showHelp(EXIT_FAILURE);
    }

    PdfPrintParams params;

    params.fitToPage = parser.isSet(fitOpt);
    params.centerOnPage = parser.isSet(centerOpt);
    params.grayscale = parser.isSet(grayOpt);
    params.monochrome = parser.isSet(monoOpt);

    // An option value that cannot be read is refused rather than ignored: a
    // silently dropped value prints the drawing at the wrong size or scale.
    if (!parsePageSizeArg(parser.value(pageSizeOpt), params.pageSize)) {
        qCritical("ERROR: invalid paper size '%s'; use WxH in mm, such as 210x297.",
                  qPrintable(parser.value(pageSizeOpt)));
        return EXIT_FAILURE;
    }

    if (parser.isSet(resOpt)) {
        bool resOk = false;
        const int res = parser.value(resOpt).toInt(&resOk);
        if (!resOk || res <= 0) {
            qCritical("ERROR: invalid resolution '%s'; use a positive number of DPI.",
                      qPrintable(parser.value(resOpt)));
            return EXIT_FAILURE;
        }
        params.resolution = res;
    }

    if (parser.isSet(scaleOpt)) {
        bool scaleOk = false;
        const double scale = parser.value(scaleOpt).toDouble(&scaleOk);
        if (!scaleOk || scale <= 0.0) {
            qCritical("ERROR: invalid scale '%s'; use a positive number, such as 0.01 for 1:100.",
                      qPrintable(parser.value(scaleOpt)));
            return EXIT_FAILURE;
        }
        params.scale = scale;
    }

    if (!parseMarginsArg(parser.value(marginsOpt), params)) {
        qCritical("ERROR: invalid margins '%s'; use L,T,R,B in mm, such as 10,10,10,10.",
                  qPrintable(parser.value(marginsOpt)));
        return EXIT_FAILURE;
    }

    if (!parsePagesNumArg(parser.value(pagesNumOpt), params)) {
        qCritical("ERROR: invalid number of pages '%s'; use HxV, such as 2x1.",
                  qPrintable(parser.value(pagesNumOpt)));
        return EXIT_FAILURE;
    }

    params.outFile = parser.value(outFileOpt);
    params.outDir = parser.value(outDirOpt);

    QStringList skippedArgs;
    params.inputFiles = LC_Console::collectInputFiles(args, spec.acceptedExts, &skippedArgs);

    if (params.inputFiles.isEmpty()) {
        qCritical("ERROR: no %s files found in arguments.",
                  qPrintable(LC_Console::extensionDescription(spec.acceptedExts)));
        return EXIT_FAILURE;
    }
    for (const QString& skipped : skippedArgs) {
        qWarning("WARNING: '%s' is not a %s file and was skipped.", qPrintable(skipped),
                 qPrintable(LC_Console::extensionDescription(spec.acceptedExts)));
    }

    if (LC_Console::containsDwgInput(params.inputFiles) &&
        !LC_Console::dwgSupportAvailable()) {
        qCritical("ERROR: DWG input requires a build with DWGSUPPORT enabled.");
        return EXIT_FAILURE;
    }

    // -o names the single PDF every input is printed into, so it is allowed
    // with several inputs, but not together with an output directory.
    QString outputOptionsError;
    if (!LC_Console::validateOutputOptions(params.inputFiles.size(), params.outFile,
                                           params.outDir, true, false,
                                           &outputOptionsError)) {
        qCritical("ERROR: %s", qPrintable(outputOptionsError));
        return EXIT_FAILURE;
    }

    QString dirError;
    if (!LC_Console::ensureOutputDirectory(params.outDir, &dirError)) {
        qCritical("ERROR: %s.", qPrintable(dirError));
        return EXIT_FAILURE;
    }

    QStringList outputFiles;
    if (params.outFile.isEmpty()) {
        for (const QString& inputFile : params.inputFiles)
            outputFiles.append(LC_Console::defaultOutputPath(inputFile, "pdf", params.outDir));
    } else {
        outputFiles.append(params.outFile);
    }

    QString outputTargetsError;
    if (!LC_Console::validateOutputTargets(params.inputFiles, outputFiles, &outputTargetsError)) {
        qCritical("ERROR: %s", qPrintable(outputTargetsError));
        return EXIT_FAILURE;
    }

    RS_FONTLIST->init();
    RS_PATTERNLIST->init();

    auto *loop = new PdfPrintLoop(params, &app);

    QObject::connect(loop, &PdfPrintLoop::finished, &app,
                     [&app](int exitCode) { app.exit(exitCode); });

    QTimer::singleShot(0, loop, SLOT(run()));

    return app.exec();
}

} // namespace

int console_dxf2pdf(int argc, char* argv[])
{
    return runPdfCommand(argc, argv,
                         {QStringLiteral("dxf2pdf"),
                          QStringLiteral("dxf"),
                          QStringLiteral("DXF"),
                          LC_Console::acceptedExtensions(QStringLiteral("dxf"),
                                                         {QStringLiteral("dwg")})});
}

int console_dwg2pdf(int argc, char* argv[])
{
    return runPdfCommand(argc, argv,
                         {QStringLiteral("dwg2pdf"),
                          QStringLiteral("dwg"),
                          QStringLiteral("DWG"),
                          LC_Console::acceptedExtensions(QStringLiteral("dwg"))});
}


static bool parsePageSizeArg(const QString& arg, RS_Vector& pageSize){
    pageSize = RS_Vector(0.0, 0.0);

    if (arg.isEmpty()) {
        return true;
    }

    const QRegularExpression re("^(?<width>\\d+)[xX](?<height>\\d+)$");
    const QRegularExpressionMatch match = re.match(arg);
    if (!match.hasMatch()) {
        return false;
    }

    pageSize.x = match.captured("width").toDouble();
    pageSize.y = match.captured("height").toDouble();
    return pageSize.x > 0.0 && pageSize.y > 0.0;
}

static bool parsePagesNumArg(const QString& arg, PdfPrintParams& params){
    if (arg.isEmpty()) {
        return true;
    }

    const QRegularExpression re("^(?<horiz>\\d+)[xX](?<vert>\\d+)$");
    const QRegularExpressionMatch match = re.match(arg);
    if (!match.hasMatch()) {
        return false;
    }

    params.pagesH = match.captured("horiz").toInt();
    params.pagesV = match.captured("vert").toInt();
    return params.pagesH > 0 && params.pagesV > 0;
}

static bool parseMarginsArg(const QString& arg, PdfPrintParams& params){
    if (arg.isEmpty()) {
        return true;
    }

    const QRegularExpression re("^(?<left>\\d+(?:\\.\\d+)?),"
                          "(?<top>\\d+(?:\\.\\d+)?),"
                          "(?<right>\\d+(?:\\.\\d+)?),"
                          "(?<bottom>\\d+(?:\\.\\d+)?)$");
    const QRegularExpressionMatch match = re.match(arg);

    if (!match.hasMatch()) {
        return false;
    }

    params.margins.left = match.captured("left").toDouble();
    params.margins.top = match.captured("top").toDouble();
    params.margins.right = match.captured("right").toDouble();
    params.margins.bottom = match.captured("bottom").toDouble();
    return true;
}
