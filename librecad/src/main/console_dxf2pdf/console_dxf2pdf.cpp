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

#include <QtCore>
#include <QCoreApplication>
#include <QApplication>

#include "rs_debug.h"
#include "rs_fontlist.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"

#include "console_command_utils.h"
#include "main.h"

#include "console_dxf2pdf.h"
#include "pdf_print_loop.h"


static RS_Vector parsePageSizeArg(QString);
static void parsePagesNumArg(QString, PdfPrintParams&);
static void parseMarginsArg(QString, PdfPrintParams&);

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

    if (args.isEmpty())
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

    params.inputFiles = LC_Console::collectInputFiles(args, spec.acceptedExts);

    if (params.inputFiles.isEmpty()) {
        qCritical("ERROR: no %s files found in arguments.",
                  qPrintable(LC_Console::extensionDescription(spec.acceptedExts)));
        return EXIT_FAILURE;
    }

    if (LC_Console::containsDwgInput(params.inputFiles) &&
        !LC_Console::dwgSupportAvailable()) {
        qCritical("ERROR: DWG input requires a build with DWGSUPPORT enabled.");
        return EXIT_FAILURE;
    }

    QString dirError;
    if (!LC_Console::ensureOutputDirectory(params.outDir, &dirError)) {
        qCritical("ERROR: %s.", qPrintable(dirError));
        return EXIT_FAILURE;
    }

    RS_FONTLIST->init();
    RS_PATTERNLIST->init();

    PdfPrintLoop *loop = new PdfPrintLoop(params, &app);

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
