/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// A DXF with handles declares its active viewport: a VPORT record named
// "*Active" that has a handle of the file. The DWG writer writes an "*Active"
// record of its own and accepts another one in the drawing only when a reader
// delivered it. Only the DWG reader said so, and the writer took one such
// record: no drawing opened from a DXF with handles, LibreCAD's own included,
// and no drawing with tiled viewports could be saved as DWG.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

#include "lc_dwgadvancedmetadata.h"
#include "lc_dimstyle.h"
#include "lc_dimstyleslist.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"

namespace {

void ensureSettings() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication* app = QCoreApplication::instance()
                                       ? QCoreApplication::instance()
                                       : new QCoreApplication(argc, argv);
    static bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)ready;
}

std::string tempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() /
            ("lc_dxf_reopen_dwg_export_" + name))
        .string();
}

struct Format {
    RS2::FormatType m_type;
    const char* m_name;
};

const Format kDxfFormats[] = {
    {RS2::FormatDXFRW12, "dxf-r12"},       {RS2::FormatDXFRW14, "dxf-r14"},
    {RS2::FormatDXFRW2000, "dxf-2000"},    {RS2::FormatDXFRW2004, "dxf-2004"},
    {RS2::FormatDXFRW, "dxf-2007"},        {RS2::FormatDXFRW2010, "dxf-2010"},
    {RS2::FormatDXFRW2013, "dxf-2013"},    {RS2::FormatDXFRW2018, "dxf-2018"}};

const Format kDwgFormats[] = {
    {RS2::FormatDWG, "dwg-2000"},          {RS2::FormatDWG2004, "dwg-2004"},
    {RS2::FormatDWG2010, "dwg-2010"},      {RS2::FormatDWG2013, "dwg-2013"},
    {RS2::FormatDWG2018, "dwg-2018"}};

bool exportTo(RS_Graphic& graphic, const std::string& path, RS2::FormatType type) {
    RS_FilterDXFRW filter;
    return filter.fileExport(graphic, QString::fromStdString(path), type);
}

bool importFrom(RS_Graphic& graphic, const std::string& path, RS2::FormatType type) {
    RS_FilterDXFRW filter;
    return filter.fileImport(graphic, QString::fromStdString(path), type);
}

std::size_t countActiveVports(const RS_Graphic& graphic) {
    const auto& vports = graphic.dwgAdvancedMetadata().vports();
    return static_cast<std::size_t>(
        std::count_if(vports.cbegin(), vports.cend(), [](const auto& record) {
            std::string name = record.name;
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });
            return name == "*ACTIVE";
        }));
}

// The one line the drawings of this file hold, read back from a saved file.
void checkTheLine(RS_Graphic& graphic) {
    const RS_Line* line = nullptr;
    std::size_t entities = 0;
    for (RS_Entity* e : graphic) {
        ++entities;
        if (e != nullptr && e->rtti() == RS2::EntityLine)
            line = static_cast<const RS_Line*>(e);
    }
    CHECK(entities == 1);
    REQUIRE(line != nullptr);
    CHECK(line->getStartpoint().distanceTo(RS_Vector(0., 0.)) < 1e-9);
    CHECK(line->getEndpoint().distanceTo(RS_Vector(10., 5.)) < 1e-9);
}

// A VPORT record as a program that writes handles declares it.
std::string viewportRecord(const std::string& handle, const std::string& name) {
    return "0\nVPORT\n5\n" + handle +
           "\n330\n8\n100\nAcDbSymbolTableRecord\n"
           "100\nAcDbViewportTableRecord\n2\n" + name +
           "\n70\n0\n"
           "10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n12\n5.0\n22\n2.5\n40\n10.0\n41\n2.0\n";
}

// The line of this file in a drawing that has the VPORT records given.
std::string dxfWithViewports(const std::string& records) {
    return
        "0\nSECTION\n2\nHEADER\n"
        "9\n$ACADVER\n1\nAC1015\n"
        "9\n$HANDSEED\n5\nFFFF\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nVPORT\n5\n8\n330\n0\n100\nAcDbSymbolTable\n70\n2\n" +
        records +
        "0\nENDTAB\n"
        "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
        "0\nLAYER\n5\n10\n330\n2\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
        "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "0\nENDTAB\n"
        "0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n5\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
        "10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n5.0\n31\n0.0\n"
        "0\nENDSEC\n0\nEOF\n";
}

} // namespace

TEST_CASE("DXF import retains the default DIMSTYLE as a string for DWG export",
          "[dxf][dwg][filter][dimstyle-default]") {
    ensureSettings();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const QString input = QDir(directory.path()).filePath("input.dxf");
    const std::string output = QDir(directory.path()).filePath("output.dwg").toStdString();
    {
        QFile file(input);
        REQUIRE(file.open(QIODevice::WriteOnly));
        const std::string dxf = dxfWithViewports(viewportRecord("29", "*Active"));
        const auto size = static_cast<qint64>(dxf.size());
        REQUIRE(file.write(dxf.data(), size) == size);
        REQUIRE(file.flush());
    }
    RS_Graphic drawing;
    drawing.initForNewDocument();
    REQUIRE(importFrom(drawing, input.toStdString(), RS2::FormatDXFRW));
    const auto& variables = drawing.getVariableDict();
    const auto style = variables.constFind("$DIMSTYLE");
    REQUIRE(style != variables.cend());
    CHECK(style->getType() == RS2::VariableString);
    CHECK(style->getCode() == 2);
    CHECK(style->getString() == QStringLiteral("Standard"));
    REQUIRE(exportTo(drawing, output, RS2::FormatDWG));
    RS_Graphic reopened;
    reopened.initForNewDocument();
    REQUIRE(importFrom(reopened, output, RS2::FormatDWG));
    checkTheLine(reopened);
    const std::string roundTrip = QDir(directory.path()).filePath("roundtrip.dxf").toStdString();
    REQUIRE(exportTo(reopened, roundTrip, RS2::FormatDXFRW2000));
    std::ifstream stream(roundTrip);
    REQUIRE(stream.good());
    const std::string text{std::istreambuf_iterator<char>(stream), {}};
    std::istringstream pairs(text);
    std::string code, value;
    int toleranceGroups = 0;
    while (std::getline(pairs, code) && std::getline(pairs, value)) {
        if (QString::fromStdString(code).trimmed() == QStringLiteral("1001") &&
            QString::fromStdString(value).trimmed() == QStringLiteral("ACAD_DSTYLE_DIMTALN"))
            ++toleranceGroups;
    }
    CHECK(toleranceGroups == 1);
    RS_Graphic dxfReopened;
    dxfReopened.initForNewDocument();
    REQUIRE(importFrom(dxfReopened, roundTrip, RS2::FormatDXFRW));
    checkTheLine(dxfReopened);


}

TEST_CASE("DIMSTYLE source XDATA is emitted once and conflicts refuse safely",
          "[dxf][dwg][filter][dimstyle-xdata-roundtrip]") {
    ensureSettings();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const QString input = QDir(directory.path()).filePath("source.dxf");
    const std::string dwg = QDir(directory.path()).filePath("source.dwg").toStdString();
    const QString output = QDir(directory.path()).filePath("output.dxf");
    {
        QFile file(input);
        REQUIRE(file.open(QIODevice::WriteOnly));
        const QByteArray dxf("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n"
                             "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nLINE\n8\n0\n"
                             "10\n1.25\n20\n2.5\n30\n0.0\n11\n12.5\n21\n7.5\n31\n0.0\n"
                             "0\nENDSEC\n0\nEOF\n");
        REQUIRE(file.write(dxf) == dxf.size());
        REQUIRE(file.flush());
    }
    RS_Graphic drawing;
    drawing.initForNewDocument();
    REQUIRE(importFrom(drawing, input.toStdString(), RS2::FormatDXFRW));
    drawing.onLoadingCompleted();
    REQUIRE(exportTo(drawing, dwg, RS2::FormatDWG));
    RS_Graphic reopened;
    reopened.initForNewDocument();
    REQUIRE(importFrom(reopened, dwg, RS2::FormatDWG));
    reopened.onLoadingCompleted();
#ifdef LIBDXFRW_HAS_AC1015_PLOTSETTINGS_RAW_VERIFY
    const auto& sourceMetadata = reopened.dwgAdvancedMetadata();
    REQUIRE(sourceMetadata.plotSettings().size() == 1u);
    const auto& sourcePlot = sourceMetadata.plotSettings().front();
    const auto rawPlot = std::find_if(sourceMetadata.rawObjects().begin(),
                                     sourceMetadata.rawObjects().end(),
                                     [&sourcePlot](const auto& raw) {
        return raw.handle == sourcePlot.handle && raw.recordName == "PLOTSETTINGS";
    });
    REQUIRE(rawPlot != sourceMetadata.rawObjects().end());
    REQUIRE(rawPlot->commonHandleDataValidated);
    REQUIRE(rawPlot->parentHandle == sourcePlot.parentHandle);
    const QString reexportedDwg = QDir(directory.path()).filePath("reexported.dwg");
    RS_FilterDXFRW writer;
    REQUIRE(writer.fileExport(reopened, reexportedDwg, RS2::FormatDWG));
    RS_Graphic sameVersionReadback;
    sameVersionReadback.initForNewDocument();
    REQUIRE(importFrom(sameVersionReadback, reexportedDwg.toStdString(), RS2::FormatDWG));
    const auto& readbackPlot = sameVersionReadback.dwgAdvancedMetadata().plotSettings();
    REQUIRE(readbackPlot.size() == 1u);
    CHECK(readbackPlot.front().handle == sourcePlot.handle);
    CHECK(readbackPlot.front().parentHandle == sourcePlot.parentHandle);
    CHECK(readbackPlot.front().pageSetupName == sourcePlot.pageSetupName);
    const auto& readbackDictionaries = sameVersionReadback.dwgAdvancedMetadata().dictionaries();
    const auto dict = std::find_if(readbackDictionaries.begin(), readbackDictionaries.end(),
                                   [&readbackPlot](const auto& item) {
        return item.handle == readbackPlot.front().parentHandle;
    });
    REQUIRE(dict != readbackDictionaries.end());
    REQUIRE(dict->entries.size() == 1u);
    CHECK(dict->entries.front().handle == readbackPlot.front().handle);
    CHECK(dict->entries.front().name == readbackPlot.front().pageSetupName);
    const auto nod = std::find_if(readbackDictionaries.begin(), readbackDictionaries.end(),
                                  [](const auto& item) {
        return item.handle == DRW::DwgNamedObjectsDictionaryHandle;
    });
    REQUIRE(nod != readbackDictionaries.end());
    const auto plotEntry = std::find_if(nod->entries.begin(), nod->entries.end(),
                                        [&readbackPlot](const auto& entry) {
        return entry.name == "ACAD_PLOTSETTINGS" &&
               entry.handle == readbackPlot.front().parentHandle;
    });
    REQUIRE(plotEntry != nod->entries.end());

    const QString guardedDwg = QDir(directory.path()).filePath("guarded.dwg");
    QFile guarded(guardedDwg);
    REQUIRE(guarded.open(QIODevice::WriteOnly));
    REQUIRE(guarded.write("sentinel") == 8);
    guarded.close();
    auto* activePlot = reopened.getPlotSettings();
    REQUIRE(activePlot != nullptr);
    const double left = activePlot->getMarginLeftMm();
    const double top = activePlot->getMarginTopMm();
    const double right = activePlot->getMarginRightMm();
    const double bottom = activePlot->getMarginBottomMm();
    activePlot->setMarginsInMm(left + 1.0, top + 2.0, right + 3.0, bottom + 4.0);
    REQUIRE(writer.fileExport(reopened, guardedDwg, RS2::FormatDWG));
    RS_Graphic editedPlotReadback;
    editedPlotReadback.initForNewDocument();
    REQUIRE(importFrom(editedPlotReadback, guardedDwg.toStdString(), RS2::FormatDWG));
    const auto& editedPlots = editedPlotReadback.dwgAdvancedMetadata().plotSettings();
    REQUIRE(editedPlots.size() == 1u);
    CHECK(editedPlots.front().marginLeft == left + 1.0);
    CHECK(editedPlots.front().marginTop == top + 2.0);
    CHECK(editedPlots.front().marginRight == right + 3.0);
    CHECK(editedPlots.front().marginBottom == bottom + 4.0);
    CHECK(editedPlots.front().handle == sourcePlot.handle);
    CHECK(editedPlots.front().parentHandle == sourcePlot.parentHandle);
    CHECK(editedPlots.front().pageSetupName == sourcePlot.pageSetupName);
    const auto& editedDictionaries = editedPlotReadback.dwgAdvancedMetadata().dictionaries();
    const auto editedNod = std::find_if(editedDictionaries.begin(), editedDictionaries.end(),
                                        [](const auto& item) {
        return item.handle == DRW::DwgNamedObjectsDictionaryHandle;
    });
    REQUIRE(editedNod != editedDictionaries.end());
    REQUIRE(std::count_if(editedNod->entries.begin(), editedNod->entries.end(),
                          [&editedPlots](const auto& entry) {
        return entry.name == "ACAD_PLOTSETTINGS" &&
               entry.handle == editedPlots.front().parentHandle;
    }) == 1);
    const QString singleMarginDwg = QDir(directory.path()).filePath("single-margin.dwg");
    for (int which = 0; which < 4; ++which) {
        const double changedLeft = left + (which == 0 ? 1.0 : 0.0);
        const double changedTop = top + (which == 1 ? 1.0 : 0.0);
        const double changedRight = right + (which == 2 ? 1.0 : 0.0);
        const double changedBottom = bottom + (which == 3 ? 1.0 : 0.0);
        activePlot->setMarginsInMm(changedLeft, changedTop, changedRight, changedBottom);
        REQUIRE(writer.fileExport(reopened, singleMarginDwg, RS2::FormatDWG));
        RS_Graphic singleMarginReadback;
        singleMarginReadback.initForNewDocument();
        REQUIRE(importFrom(singleMarginReadback, singleMarginDwg.toStdString(), RS2::FormatDWG));
        const auto& singlePlot = singleMarginReadback.dwgAdvancedMetadata().plotSettings();
        REQUIRE(singlePlot.size() == 1u);
        CHECK(singlePlot.front().marginLeft == changedLeft);
        CHECK(singlePlot.front().marginTop == changedTop);
        CHECK(singlePlot.front().marginRight == changedRight);
        CHECK(singlePlot.front().marginBottom == changedBottom);
    }
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    const QByteArray editedPlotBytes = guarded.readAll();
    guarded.close();
    activePlot->setMarginsInMm(left, top, std::numeric_limits<double>::infinity(), bottom);
    CHECK_FALSE(writer.fileExport(reopened, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    CHECK(guarded.readAll() == editedPlotBytes);
    guarded.close();
    activePlot->setMarginsInMm(left, top, right, bottom);
    const int oldShadeMode = activePlot->getShadePlotMode();
    activePlot->setShadePlotMode(oldShadeMode == 0 ? 1 : 0);
    CHECK_FALSE(writer.fileExport(reopened, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    CHECK(guarded.readAll() == editedPlotBytes);
    guarded.close();
    activePlot->setShadePlotMode(oldShadeMode);
    REQUIRE(writer.fileExport(reopened, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    const QByteArray validDwg = guarded.readAll();
    guarded.close();

    RS_Graphic tampered;
    tampered.initForNewDocument();
    REQUIRE(importFrom(tampered, dwg, RS2::FormatDWG));
    tampered.onLoadingCompleted();
    auto& sourceDictionaries = const_cast<std::vector<LC_DwgAdvancedMetadata::DictionaryRecord>&>(
        tampered.dwgAdvancedMetadata().dictionaries());
    auto sourceNod = std::find_if(sourceDictionaries.begin(), sourceDictionaries.end(),
                                  [](const auto& item) {
        return item.handle == DRW::DwgNamedObjectsDictionaryHandle;
    });
    REQUIRE(sourceNod != sourceDictionaries.end());
    auto sourceNodPlot = std::find_if(sourceNod->entries.begin(), sourceNod->entries.end(),
                                      [](const auto& entry) {
        return entry.name == "ACAD_PLOTSETTINGS";
    });
    REQUIRE(sourceNodPlot != sourceNod->entries.end());
    const auto originalNodTarget = sourceNodPlot->handle;
    sourceNodPlot->handle += 1u;
    CHECK_FALSE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    CHECK(guarded.readAll() == validDwg);
    guarded.close();
    sourceNodPlot->handle = originalNodTarget;
    REQUIRE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    sourceNod->entries.push_back({"UNMODELED_ROOT", 0x123u});
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    const QByteArray beforeExtraRoot = guarded.readAll();
    guarded.close();
    CHECK_FALSE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    CHECK(guarded.readAll() == beforeExtraRoot);
    guarded.close();
    sourceNod->entries.pop_back();
    REQUIRE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    sourceNodPlot = std::find_if(sourceNod->entries.begin(), sourceNod->entries.end(),
                                 [](const auto& entry) {
        return entry.name == "ACAD_PLOTSETTINGS";
    });
    REQUIRE(sourceNodPlot != sourceNod->entries.end());
    sourceNodPlot->name = "acad_plotsettings";
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    const QByteArray beforeChangedName = guarded.readAll();
    guarded.close();
    CHECK_FALSE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    CHECK(guarded.readAll() == beforeChangedName);
    guarded.close();
    sourceNodPlot->name = "ACAD_PLOTSETTINGS";
    REQUIRE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));

    // Deliberately corrupt the otherwise read-only source receipt to exercise
    // the wrong-owner guard; the underlying metadata object is mutable.
    auto& tamperedRaw = const_cast<std::vector<LC_DwgAdvancedMetadata::RawObjectRecord>&>(
        tampered.dwgAdvancedMetadata().rawObjects());
    auto badRaw = std::find_if(tamperedRaw.begin(), tamperedRaw.end(),
                               [&sourcePlot](const auto& raw) {
        return raw.handle == sourcePlot.handle && raw.recordName == "PLOTSETTINGS";
    });
    REQUIRE(badRaw != tamperedRaw.end());
    auto* tamperedActive = tampered.getPlotSettings();
    REQUIRE(tamperedActive != nullptr);
    const auto oldClassAppName = badRaw->classAppName;
    badRaw->classAppName = "OTHER_APP";
    tamperedActive->setMarginsInMm(left + 1.0, top, right, bottom);
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    const QByteArray beforeChangedClass = guarded.readAll();
    guarded.close();
    CHECK_FALSE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    CHECK(guarded.readAll() == beforeChangedClass);
    guarded.close();
    badRaw->classAppName = oldClassAppName;
    REQUIRE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    tamperedActive->setMarginsInMm(left, top, right, bottom);
    REQUIRE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    const auto originalOwner = badRaw->parentHandle;
    badRaw->parentHandle = originalOwner + 1u;
    CHECK_FALSE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    CHECK(guarded.readAll() == validDwg);
    guarded.close();
    badRaw->parentHandle = originalOwner;
    REQUIRE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));

    // Tamper with the retained owner handle role while leaving the metadata
    // receipt unchanged; a stale validated flag must not admit altered bytes.
    REQUIRE(badRaw->rawBytes.size() >= 4u);
    const std::size_t ownerRoleByte = badRaw->rawBytes.size() - 4u;
    const auto originalByte = badRaw->rawBytes[ownerRoleByte];
    badRaw->rawBytes[ownerRoleByte] ^= 1u;
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    const QByteArray beforeByteTamper = guarded.readAll();
    guarded.close();
    CHECK_FALSE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    CHECK(guarded.readAll() == beforeByteTamper);
    guarded.close();
    badRaw->rawBytes[ownerRoleByte] = originalByte;
    REQUIRE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));

    DRW_UnsupportedObject duplicate;
    duplicate.m_version = badRaw->version;
    duplicate.m_objectType = badRaw->objectType;
    duplicate.m_handle = badRaw->handle;
    duplicate.m_parentHandle = badRaw->parentHandle;
    duplicate.setCommonLinkEvidence(badRaw->commonLinkEvidence);
    duplicate.m_reactorHandles = badRaw->reactorHandles;
    duplicate.m_xDictHandle = badRaw->xDictHandle;
    duplicate.m_numReactors = badRaw->numReactors;
    duplicate.m_xDictFlag = badRaw->xDictFlag;
    duplicate.m_bodyBitSize = badRaw->bodyBitSize;
    duplicate.m_objectSize = badRaw->objectSize;
    duplicate.m_isCustomClass = badRaw->isCustomClass;
    duplicate.m_hasClassDefinition = badRaw->hasClassDefinition;
    duplicate.m_recordName = badRaw->recordName;
    duplicate.m_className = badRaw->className;
    duplicate.m_rawBytes = badRaw->rawBytes;
    tampered.dwgAdvancedMetadata().addUnsupportedObject(duplicate);
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    const QByteArray beforeDuplicate = guarded.readAll();
    guarded.close();
    CHECK_FALSE(writer.fileExport(tampered, guardedDwg, RS2::FormatDWG));
    REQUIRE(guarded.open(QIODevice::ReadOnly));
    CHECK(guarded.readAll() == beforeDuplicate);
    guarded.close();
    CHECK(writer.fileExport(reopened, guardedDwg, RS2::FormatDWG));
#else
    RS_FilterDXFRW writer;
#endif
    REQUIRE(writer.fileExport(reopened, output, RS2::FormatDXFRW2000));
    QFile written(output);
    REQUIRE(written.open(QIODevice::ReadOnly));
    const QByteArray originalBytes = written.readAll();
    written.close();
    std::istringstream pairs(originalBytes.toStdString());
    std::string code, value;
    int count = 0;
    while (std::getline(pairs, code) && std::getline(pairs, value)) {
        if (QString::fromStdString(code).trimmed() == QStringLiteral("1001") &&
            QString::fromStdString(value).trimmed() == QStringLiteral("ACAD_DSTYLE_DIMTALN"))
            ++count;
    }
    CHECK(count == 1);
    RS_Graphic readback;
    readback.initForNewDocument();
    REQUIRE(importFrom(readback, output.toStdString(), RS2::FormatDXFRW));

    auto& metadata = reopened.dwgAdvancedMetadata();
    const auto* source = metadata.findDimStyleTableEntryByName("Standard");
    REQUIRE(source != nullptr);
    DRW_Dimstyle original(*source);
    DRW_Dimstyle conflicting(original);
    bool changed = false;
    std::size_t toleranceIndex = conflicting.extData.size();
    for (std::size_t i = 0; i + 2 < conflicting.extData.size(); ++i) {
        const auto* app = conflicting.extData[i];
        auto* adjustment = conflicting.extData[i + 2];
        if (app && app->code() == 1001 && app->type() == DRW_Variant::STRING &&
            app->content.s && *app->content.s == "ACAD_DSTYLE_DIMTALN" &&
            adjustment && adjustment->code() == 1070 &&
            adjustment->type() == DRW_Variant::INTEGER) {
            auto extra = std::make_unique<DRW_Variant>(1000, "unmodeled-value");
            conflicting.extData.insert(conflicting.extData.begin() + i + 3, extra.get());
            extra.release();
            toleranceIndex = i;
            changed = true;
            break;
        }
    }
    REQUIRE(changed);
    metadata.addDimStyleTableEntry(conflicting);
    CHECK_FALSE(writer.fileExport(reopened, output, RS2::FormatDXFRW2000));
    REQUIRE(written.open(QIODevice::ReadOnly));
    CHECK(written.readAll() == originalBytes);
    written.close();
    REQUIRE(original.addExtData(std::make_unique<DRW_Variant>(1001, "TEST_KEEP")));
    REQUIRE(original.addExtData(std::make_unique<DRW_Variant>(1000, "source-payload")));
    metadata.addDimStyleTableEntry(original);
    REQUIRE(writer.fileExport(reopened, output, RS2::FormatDXFRW2000));

    auto* style = reopened.getDimStyleList()->findByName(QStringLiteral("Standard"));
    REQUIRE(style != nullptr);
    auto* tolerance = style->latteralTolerance();
    const int editedValue = tolerance->adjustment() == 0 ? 1 : 0;
    tolerance->setAdjustmentRaw(editedValue);
    REQUIRE(tolerance->adjustment() == editedValue);
    REQUIRE(writer.fileExport(reopened, output, RS2::FormatDXFRW2000));
    REQUIRE(written.open(QIODevice::ReadOnly));
    std::istringstream editedPairs(written.readAll().toStdString());
    written.close();
    bool foundEditedGroup = false;
    int editedGroups = 0;
    while (std::getline(editedPairs, code) && std::getline(editedPairs, value)) {
        if (QString::fromStdString(code).trimmed() != QStringLiteral("1001") ||
            QString::fromStdString(value).trimmed() != QStringLiteral("ACAD_DSTYLE_DIMTALN"))
            continue;
        ++editedGroups;
        std::string c1, v1, c2, v2;
        REQUIRE(std::getline(editedPairs, c1));
        REQUIRE(std::getline(editedPairs, v1));
        REQUIRE(std::getline(editedPairs, c2));
        REQUIRE(std::getline(editedPairs, v2));
        foundEditedGroup = QString::fromStdString(c1).trimmed() == QStringLiteral("1070") &&
            QString::fromStdString(v1).trimmed() == QStringLiteral("392") &&
            QString::fromStdString(c2).trimmed() == QStringLiteral("1070") &&
            QString::fromStdString(v2).trimmed().toInt() == editedValue;
    }
    CHECK(editedGroups == 1);
    CHECK(foundEditedGroup);
    RS_Graphic editedReadback;
    editedReadback.initForNewDocument();
    REQUIRE(importFrom(editedReadback, output.toStdString(), RS2::FormatDXFRW));
    editedReadback.onLoadingCompleted();
    auto* editedStyle = editedReadback.getDimStyleList()->findByName(QStringLiteral("Standard"));
    REQUIRE(editedStyle != nullptr);
    CHECK(editedStyle->latteralTolerance()->adjustment() == editedValue);
    const auto* editedSource = editedReadback.dwgAdvancedMetadata()
                                   .findDimStyleTableEntryByName("Standard");
    REQUIRE(editedSource != nullptr);
    bool retainedUnknown = false;
    for (std::size_t i = 0; i + 1 < editedSource->extData.size(); ++i) {
        const auto* app = editedSource->extData[i];
        const auto* payload = editedSource->extData[i + 1];
        if (app && payload && app->code() == 1001 && payload->code() == 1000 &&
            app->type() == DRW_Variant::STRING && payload->type() == DRW_Variant::STRING &&
            app->content.s && payload->content.s && *app->content.s == "TEST_KEEP" &&
            *payload->content.s == "source-payload")
            retainedUnknown = true;
    }
    CHECK(retainedUnknown);
    const auto* unchangedSource = metadata.findDimStyleTableEntryByName("Standard");
    REQUIRE(unchangedSource != nullptr);
    REQUIRE(toleranceIndex + 2 < unchangedSource->extData.size());
    CHECK(unchangedSource->extData[toleranceIndex + 2]->i_val() != editedValue);

}

TEST_CASE("a drawing saved as DXF and opened again is saved as DWG",
          "[dxf][dwg][filter][vport]") {
    ensureSettings();
    const Format dxf = GENERATE(from_range(std::begin(kDxfFormats), std::end(kDxfFormats)));
    const Format dwg = GENERATE(from_range(std::begin(kDwgFormats), std::end(kDwgFormats)));
    CAPTURE(dxf.m_name, dwg.m_name);

    const std::string saved = tempPath(std::string(dxf.m_name) + ".dxf");
    const std::string out = tempPath(std::string(dxf.m_name) + "_" + dwg.m_name + ".dwg");
    {
        RS_Graphic drawing;
        drawing.initForNewDocument();
        drawing.addEntity(new RS_Line(&drawing, {RS_Vector(0., 0.), RS_Vector(10., 5.)}));
        REQUIRE(exportTo(drawing, saved, dxf.m_type));
        // The drawing that was never a file has no viewport record to trip over.
        REQUIRE(exportTo(drawing, out, dwg.m_type));
    }

    RS_Graphic reopened;
    reopened.initForNewDocument();
    REQUIRE(importFrom(reopened, saved, dxf.m_type));
    std::filesystem::remove(out);
    CHECK(exportTo(reopened, out, dwg.m_type));

    RS_Graphic fromDwg;
    fromDwg.initForNewDocument();
    REQUIRE(importFrom(fromDwg, out, RS2::FormatDWG));
    checkTheLine(fromDwg);
    CHECK(countActiveVports(fromDwg) == 1);

    std::filesystem::remove(saved);
    std::filesystem::remove(out);
}

TEST_CASE("a DXF that declares its viewports is saved as DWG",
          "[dxf][dwg][filter][vport]") {
    ensureSettings();
    struct Viewports {
        const char* m_label;
        std::string m_records;
        std::size_t m_read;    // VPORT records the DXF reader delivers
        std::size_t m_written; // VPORT records of the DWG
    };
    // AutoCAD writes one "*Active" record for each tiled viewport of the model
    // space; LibreCAD has one view, and writes one.
    const Viewports viewports = GENERATE(
        Viewports{"the active viewport", viewportRecord("29", "*Active"), 1, 1},
        Viewports{"two tiled viewports",
                  viewportRecord("29", "*Active") + viewportRecord("2A", "*Active"), 2, 1},
        Viewports{"the active viewport and a named one",
                  viewportRecord("29", "*Active") + viewportRecord("2A", "LEFT"), 2, 2});
    const Format dwg = GENERATE(from_range(std::begin(kDwgFormats), std::end(kDwgFormats)));
    CAPTURE(viewports.m_label, dwg.m_name);

    const std::string src = tempPath("declared.dxf");
    const std::string out = tempPath(std::string("declared_") + dwg.m_name + ".dwg");
    std::ofstream(src, std::ios::binary) << dxfWithViewports(viewports.m_records);

    RS_Graphic drawing;
    drawing.initForNewDocument();
    REQUIRE(importFrom(drawing, src, RS2::FormatDXFRW));
    // The records the writer has to tell from one it must refuse.
    REQUIRE(drawing.dwgAdvancedMetadata().vports().size() == viewports.m_read);
    REQUIRE(drawing.dwgAdvancedMetadata().vports().front().handle == 0x29u);
    std::filesystem::remove(out);
    CHECK(exportTo(drawing, out, dwg.m_type));

    RS_Graphic fromDwg;
    fromDwg.initForNewDocument();
    REQUIRE(importFrom(fromDwg, out, RS2::FormatDWG));
    checkTheLine(fromDwg);
    CHECK(countActiveVports(fromDwg) == 1);
    CHECK(fromDwg.dwgAdvancedMetadata().vports().size() == viewports.m_written);

    // What was read from the DWG is saved as DWG, as before.
    CHECK(exportTo(fromDwg, out, dwg.m_type));

    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST_CASE("ownerless plot records replay unchanged and reject edits safely",
          "[dxf][dwg][filter][plotsettings][regression]") {
    ensureSettings();
    const Format dwg = GENERATE(from_range(std::begin(kDwgFormats), std::end(kDwgFormats)));
    CAPTURE(dwg.m_name);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const std::string source = QDir(directory.path()).filePath("source.dxf").toStdString();
    const char *witnessDirectory = std::getenv("LIBRECAD_TEST_OWNERLESS_DWG_DIR");
    const std::string initial = witnessDirectory && *witnessDirectory
        ? QDir(QString::fromLocal8Bit(witnessDirectory))
              .filePath(QString("lc-viewport-before-%1.dwg").arg(QString::fromLatin1(dwg.m_name))).toStdString()
        : QDir(directory.path()).filePath("initial.dwg").toStdString();
    const QString destination = QDir(directory.path()).filePath("destination.dwg");
    std::ofstream(source, std::ios::binary) << dxfWithViewports(viewportRecord("29", "*Active"));
    RS_Graphic drawing;
    drawing.initForNewDocument();
    REQUIRE(importFrom(drawing, source, RS2::FormatDXFRW));
    if (!witnessDirectory || !*witnessDirectory)
        REQUIRE(exportTo(drawing, initial, dwg.m_type));
    RS_Graphic reopened;
    reopened.initForNewDocument();
    REQUIRE(importFrom(reopened, initial, RS2::FormatDWG));
    auto &metadata = reopened.dwgAdvancedMetadata();
    auto &raws = const_cast<std::vector<LC_DwgAdvancedMetadata::RawObjectRecord>&>(metadata.rawObjects());
    auto raw = std::find_if(raws.begin(), raws.end(), [](const auto &record) {
        return record.recordName == "PLOTSETTINGS";
    });
    REQUIRE(raw != raws.end());
    if (raw->parentHandle != 0u)
        SKIP("The selected codec generated a rooted plot record; supply an ownerless witness to exercise this path.");
    const auto originalBytes = raw->rawBytes;
    const auto originalHandle = raw->handle;
    auto &plots = const_cast<std::vector<DRW_PlotSettings>&>(metadata.plotSettings());
    REQUIRE(plots.size() == 1u);
    auto &dictionaries = const_cast<std::vector<LC_DwgAdvancedMetadata::DictionaryRecord>&>(
        metadata.dictionaries());
    auto nod = std::find_if(dictionaries.begin(), dictionaries.end(), [](const auto &row) {
        return row.handle == DRW::DwgNamedObjectsDictionaryHandle;
    });
    REQUIRE(nod != dictionaries.end());
    auto *active = reopened.getPlotSettings();
    REQUIRE(active != nullptr);
    RS_FilterDXFRW writer;
    const QByteArray sentinel("ownerless plot destination sentinel\n");
    const auto refusesAndPreserves = [&]() {
        QFile file(destination);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(sentinel) != sentinel.size())
            return false;
        file.close();
        if (writer.fileExport(reopened, destination, dwg.m_type) || !file.open(QIODevice::ReadOnly))
            return false;
        return file.readAll() == sentinel;
    };
    const auto retries = [&]() { return writer.fileExport(reopened, destination, dwg.m_type); };
    raw->rawBytes.front() ^= 1u;
    CHECK(refusesAndPreserves());
    raw->rawBytes.front() ^= 1u;
    REQUIRE(retries());
    const double oldOrigin = plots.front().plotOriginX;
    plots.front().plotOriginX += 1.0;
    CHECK(refusesAndPreserves());
    plots.front().plotOriginX = oldOrigin;
    REQUIRE(retries());
    const auto oldPlotViewHandle = plots.front().plotViewHandle;
    plots.front().plotViewHandle.ref64 = std::uint64_t{1} << 32u;
    CHECK(refusesAndPreserves());
    plots.front().plotViewHandle = oldPlotViewHandle;
    REQUIRE(retries());
    plots.front().plotViewHandle.code = 6u;
    CHECK(refusesAndPreserves());
    plots.front().plotViewHandle = oldPlotViewHandle;
    REQUIRE(retries());
#ifdef LIBDXFRW_HAS_AC1015_PLOTSETTINGS_RAW_VERIFY
    plots.front().plotViewHandle.rawCode = 6u;
    CHECK(refusesAndPreserves());
    plots.front().plotViewHandle = oldPlotViewHandle;
    REQUIRE(retries());
    plots.front().dwgEedAppIdReferences.push_back({0u, 0x12u});
    CHECK(refusesAndPreserves());
    plots.front().dwgEedAppIdReferences.clear();
    REQUIRE(retries());
#endif
    const double oldMargin = active->getMarginLeftMm();
    active->setMarginsInMm(oldMargin + 1.0, active->getMarginTopMm(),
                           active->getMarginRightMm(), active->getMarginBottomMm());
    CHECK(refusesAndPreserves());
    active->setMarginsInMm(oldMargin, active->getMarginTopMm(),
                           active->getMarginRightMm(), active->getMarginBottomMm());
    REQUIRE(retries());
    nod->entries.push_back({"ACAD_PLOTSETTINGS", originalHandle});
    CHECK(refusesAndPreserves());
    nod->entries.pop_back();
    REQUIRE(retries());
    RS_Graphic result;
    result.initForNewDocument();
    REQUIRE(importFrom(result, destination.toStdString(), RS2::FormatDWG));
    const auto &outputRaw = result.dwgAdvancedMetadata().rawObjects();
    const auto output = std::find_if(outputRaw.begin(), outputRaw.end(), [originalHandle](const auto &record) {
        return record.handle == originalHandle && record.recordName == "PLOTSETTINGS";
    });
    REQUIRE(output != outputRaw.end());
    CHECK(output->rawBytes == originalBytes);
    CHECK(output->parentHandle == 0u);
    // Removing a duplicate later must not turn an ambiguous import into a
    // trusted single-source profile.
    metadata.addPlotSettings(plots.front());
    plots.pop_back();
    CHECK(refusesAndPreserves());
}
