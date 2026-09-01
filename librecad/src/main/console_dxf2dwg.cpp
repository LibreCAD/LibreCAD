/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD www.librecad.org
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
****************************************************************************/

#include <cstdlib>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include "console_command_utils.h"
#include "console_dxf2dwg.h"
#include "main.h"
#include "rs.h"
#include "rs_debug.h"
#include "rs_filterdxfrw.h"
#include "rs_fileio.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_patternlist.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {

RS2::FormatType parseDxfVersion(const QString& ver) {
    QString v = ver.toLower().remove('r');
    if (v == "12")   return RS2::FormatDXFRW12;
    if (v == "14")   return RS2::FormatDXFRW14;
    if (v == "2000") return RS2::FormatDXFRW2000;
    if (v == "2004") return RS2::FormatDXFRW2004;
    if (v == "2007") return RS2::FormatDXFRW;
    if (v == "2018") return RS2::FormatDXFRW2018;
    return RS2::FormatUnknown;
}

bool convertFile(const QString& inputFile, const QString& outputFile, RS2::FormatType outFmt) {
    // Fix 7: check existence before hitting the parser with a cryptic error
    if (!QFileInfo::exists(inputFile)) {
        qCritical("ERROR: input file does not exist: '%s'", qPrintable(inputFile));
        return false;
    }

    RS_Graphic graphic;
    graphic.initForNewDocument();

    // Fix 3: pass callback so partial/full failures print to stderr, no dialog
    bool imported = RS_FileIO::instance()->fileImport(
        graphic, inputFile, RS2::FormatUnknown,
        [&](bool partial, const QString& errMsg) -> bool {
            if (partial) {
                qWarning("WARNING: partial import of '%s': %s",
                         qPrintable(inputFile), qPrintable(errMsg));
                return true;
            }
            qCritical("ERROR: import failed for '%s': %s",
                      qPrintable(inputFile), qPrintable(errMsg));
            return false;
        });

    if (!imported)
        return false;

    // Fix 2: finalise dimension styles before export
    graphic.onLoadingCompleted();

    if (!RS_FileIO::instance()->fileExport(graphic, outputFile, outFmt)) {
        qCritical("ERROR: failed to export '%s'", qPrintable(outputFile));
        return false;
    }

#ifdef DWGSUPPORT
    if (outFmt == RS2::FormatDWG)
        qInfo("%s -> %s OK (DWG R2000 / AC1015)", qPrintable(inputFile), qPrintable(outputFile));
    else if (outFmt == RS2::FormatDWG2004)
        qInfo("%s -> %s OK (DWG R2004 / AC1018)", qPrintable(inputFile), qPrintable(outputFile));
    else if (outFmt == RS2::FormatDWG2007)
        qInfo("%s -> %s OK (DWG R2007 / AC1021)", qPrintable(inputFile), qPrintable(outputFile));
    else if (outFmt == RS2::FormatDWG2010)
        qInfo("%s -> %s OK (DWG R2010 / AC1024)", qPrintable(inputFile), qPrintable(outputFile));
    else if (outFmt == RS2::FormatDWG2013)
        qInfo("%s -> %s OK (DWG R2013 / AC1027)", qPrintable(inputFile), qPrintable(outputFile));
    else if (outFmt == RS2::FormatDWG2018)
        qInfo("%s -> %s OK (DWG R2018 / AC1032)", qPrintable(inputFile), qPrintable(outputFile));
    else
#endif
        qInfo("%s -> %s OK", qPrintable(inputFile), qPrintable(outputFile));

    return true;
}

#ifdef DWGSUPPORT
RS2::FormatType formatForDwgVersion(DRW::Version version) {
    switch (version) {
    case DRW::AC1015: return RS2::FormatDWG;
    case DRW::AC1018: return RS2::FormatDWG2004;
    case DRW::AC1021: return RS2::FormatDWG2007;
    case DRW::AC1024: return RS2::FormatDWG2010;
    case DRW::AC1027: return RS2::FormatDWG2013;
    case DRW::AC1032: return RS2::FormatDWG2018;
    default: return RS2::FormatUnknown;
    }
}

int runDwgAdmissionReport(int argc, char** argv) {
    RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
    const LC_Console::CommandContext context =
        LC_Console::contextForCommand(argc, argv, "dwg-admission-report");
    LC_Console::NormalizedArgv normalizedArgs(argc, argv, context);
    int normalizedArgc = normalizedArgs.argc();
    char** normalizedArgv = normalizedArgs.argv();
    QCoreApplication app(normalizedArgc, normalizedArgv);
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD");
    QCoreApplication::setApplicationVersion(XSTR(LC_VERSION));
    RS_Settings::init(app.organizationName(), app.applicationName());
    RS_SYSTEM->init(app.applicationName(), app.applicationVersion(),
                    XSTR(QC_APPDIR), normalizedArgv[0]);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Inspect the DWG export-admission decision for one explicit DWG path.");
    parser.addHelpOption();
    parser.addPositionalArgument("dwg_file", "Input DWG file.");
    parser.process(app);
    const QStringList inputs = parser.positionalArguments();
    if (inputs.size() != 1) {
        qCritical("ERROR: dwg-admission-report requires exactly one DWG file.");
        return EXIT_FAILURE;
    }
    const QString input = inputs.front();
    if (QFileInfo(input).suffix().compare("dwg", Qt::CaseInsensitive) != 0
        || !QFileInfo(input).isFile()) {
        qCritical("ERROR: input must name an existing .dwg file.");
        return EXIT_FAILURE;
    }

    RS_FONTLIST->init();
    RS_PATTERNLIST->init();
    RS_Graphic graphic;
    graphic.initForNewDocument();
    if (!RS_FileIO::instance()->fileImport(
            graphic, input, RS2::FormatDWG,
            [](bool partial, const QString& error) {
                if (partial) {
                    qWarning("WARNING: partial import: %s", qPrintable(error));
                    return true;
                }
                qCritical("ERROR: import failed: %s", qPrintable(error));
                return false;
            })) {
        return EXIT_FAILURE;
    }
    graphic.onLoadingCompleted();
    const RS2::FormatType target = formatForDwgVersion(
        graphic.dwgAdvancedMetadata().sourceDwgVersion());
    if (target == RS2::FormatUnknown) {
        qCritical("ERROR: imported DWG version has no LibreCAD DWG export target.");
        return EXIT_FAILURE;
    }

    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid()) {
        qCritical("ERROR: unable to create temporary DWG report directory.");
        return EXIT_FAILURE;
    }
    RS_FilterDXFRW filter;
    const bool published = filter.fileExport(
        graphic, temporaryDirectory.filePath("admission-check.dwg"), target);
    qInfo().noquote() << filter.dwgExportAdmissionReport();
    qInfo("publish=%s", published ? "success" : "refused");
    return published ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif

int runConversion(int argc, char** argv,
                  const QString& commandName,
                  const QString& inputExt,
                  const QString& outputExt,
                  RS2::FormatType defaultOutputFmt,
                  bool allowVersionOption)
{
    RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

    // Fix 4: QCoreApplication suffices — no GUI widgets needed now that
    // RS_FileIO::fileImport uses the errorCallback path in this tool.
    const LC_Console::CommandContext context =
        LC_Console::contextForCommand(argc, argv, commandName);
    LC_Console::NormalizedArgv normalizedArgs(argc, argv, context);
    int normalizedArgc = normalizedArgs.argc();
    char** normalizedArgv = normalizedArgs.argv();

    QCoreApplication app(normalizedArgc, normalizedArgv);
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD");
    QCoreApplication::setApplicationVersion(XSTR(LC_VERSION));

    RS_Settings::init(app.organizationName(), app.applicationName());
    RS_SYSTEM->init(app.applicationName(), app.applicationVersion(), XSTR(QC_APPDIR),
                    normalizedArgv[0]);

    QCommandLineParser parser;

    const QString execName = context.displayCommand();

    QStringList appDesc;
    appDesc << "";
    appDesc << QString("Convert %1 file(s) to %2.").arg(inputExt.toUpper(), outputExt.toUpper());
    appDesc << "";
    appDesc << "Examples:";
    appDesc << "";
    appDesc << "  " + execName + " drawing." + inputExt;
    appDesc << "    -- converts drawing." + inputExt + " to drawing." + outputExt + " in the same directory.";
    appDesc << "";
    appDesc << "  " + execName + " -t /output/dir *." + inputExt;
    appDesc << "    -- converts all " + inputExt.toUpper() + " files to the specified directory.";
    parser.setApplicationDescription(appDesc.join("\n"));

    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption outFileOpt(QStringList() << "o" << "output",
        QObject::tr("Output file (single input only)."), "file");
    parser.addOption(outFileOpt);

    QCommandLineOption outDirOpt(QStringList() << "t" << "directory",
        QObject::tr("Target output directory."), "path");
    parser.addOption(outDirOpt);

    QCommandLineOption versionOpt(QStringList() << "V" << "dxf-version",
        QObject::tr("DXF output version: r12, r14, r2000, r2004, r2007 (default), r2018."), "version");
    if (allowVersionOption)
        parser.addOption(versionOpt);

#ifdef DWGSUPPORT
    QCommandLineOption dwgVersionOpt(QStringList() << "V" << "dwg-version",
        QObject::tr("DWG output version: r2000 (default), r2004, r2007, r2010, r2013, r2018."), "version");
    if (outputExt == "dwg")
        parser.addOption(dwgVersionOpt);
#endif

    parser.addPositionalArgument("<" + inputExt + "_files>",
        QObject::tr("Input %1 file(s).").arg(inputExt.toUpper()));

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty())
        parser.showHelp(EXIT_FAILURE);

    RS2::FormatType outFmt = defaultOutputFmt;
    if (allowVersionOption && parser.isSet(versionOpt)) {
        outFmt = parseDxfVersion(parser.value(versionOpt));
        if (outFmt == RS2::FormatUnknown) {
            qCritical("ERROR: unknown DXF version '%s'", qPrintable(parser.value(versionOpt)));
            return EXIT_FAILURE;
        }
    }
#ifdef DWGSUPPORT
    if (outputExt == "dwg" && parser.isSet(dwgVersionOpt)) {
        outFmt = LC_Console::dwgFormatForVersion(parser.value(dwgVersionOpt));
        if (outFmt == RS2::FormatUnknown) {
            qCritical("ERROR: unknown DWG version '%s'; use r2000, r2004, r2007, r2010, r2013, or r2018.",
                      qPrintable(parser.value(dwgVersionOpt)));
            return EXIT_FAILURE;
        }
    }
#endif

    // Fix 9: plain locals instead of ConvertParams struct
    QString outFile = parser.value(outFileOpt);
    QString outDir  = parser.value(outDirOpt);

    const QStringList inputFiles =
        LC_Console::collectInputFiles(args, LC_Console::acceptedExtensions(inputExt));

    // Fix 8: clear diagnostic instead of dumping the full help page
    if (inputFiles.isEmpty()) {
        qCritical("ERROR: no .%s files found in arguments.", qPrintable(inputExt));
        return EXIT_FAILURE;
    }

    if (inputExt == "dwg" && !LC_Console::dwgSupportAvailable()) {
        qCritical("ERROR: DWG input requires a build with DWGSUPPORT enabled.");
        return EXIT_FAILURE;
    }

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

    // Fix 5: probe for a working export filter before processing any files,
    // so a missing DWGSUPPORT build gives a clear error upfront.
    if (!RS_FileIO::instance()->getExportFilter("probe." + outputExt, outFmt)) {
        if (outputExt == "dwg")
            qCritical("ERROR: DWG export requires a build with DWGSUPPORT enabled.");
        else
            qCritical("ERROR: no export filter available for format '%s'.", qPrintable(outputExt));
        return EXIT_FAILURE;
    }

    int failed = 0;
    for (const auto& inputFile : inputFiles) {
        QString outputFile;
        if (!outFile.isEmpty()) {
            outputFile = outFile;
        } else {
            outputFile = LC_Console::defaultOutputPath(inputFile, outputExt, outDir);
        }

        if (!convertFile(inputFile, outputFile, outFmt))
            ++failed;
    }

    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace

int consoleDxf2dwg(int argc, char** argv) {
#ifdef DWGSUPPORT
    return runConversion(argc, argv,
                         "dxf2dwg", "dxf", "dwg",
                         RS2::FormatDWG,
                         false);
#else
    (void)argc; (void)argv;
    qCritical("ERROR: DWG export requires a build with DWGSUPPORT enabled.");
    return EXIT_FAILURE;
#endif
}

int consoleDwg2dxf(int argc, char** argv) {
#ifndef DWGSUPPORT
    (void)argc; (void)argv;
    qCritical("ERROR: DWG input requires a build with DWGSUPPORT enabled.");
    return EXIT_FAILURE;
#else
    return runConversion(argc, argv,
                         "dwg2dxf", "dwg", "dxf",
                         RS2::FormatDXFRW,
                         true);
#endif
}

int consoleDwgAdmissionReport(int argc, char** argv) {
#ifdef DWGSUPPORT
    return runDwgAdmissionReport(argc, argv);
#else
    (void)argc;
    (void)argv;
    qCritical("ERROR: DWG input requires a build with DWGSUPPORT enabled.");
    return EXIT_FAILURE;
#endif
}
