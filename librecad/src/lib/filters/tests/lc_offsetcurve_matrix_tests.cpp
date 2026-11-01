/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>

#include <QByteArray>
#include <QCryptographicHash>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "lc_offsetcurve_persistence_fixture.h"
#include "rs_layer.h"
#include "rs_polyline.h"

namespace {
namespace f = lc::test::offset_persistence;
using Path = std::filesystem::path;
using Tags = f::Tags;
struct Setup {
    const bool settings = lc::test::ensureSettings();
    lc::test::QuietLog quiet;
};
struct Format {
    const char *release;
    const char *code;
    RS2::FormatType app;
    DRW::Version codec;
    bool dwg = false;
};
constexpr std::array<Format, 8> Dxf{{
    {"R12", "AC1009", RS2::FormatDXFRW12, DRW::AC1009},
    {"R14", "AC1014", RS2::FormatDXFRW14, DRW::AC1014},
    {"R2000", "AC1015", RS2::FormatDXFRW2000, DRW::AC1015},
    {"R2004", "AC1018", RS2::FormatDXFRW2004, DRW::AC1018},
    {"R2007", "AC1021", RS2::FormatDXFRW, DRW::AC1021},
    {"R2010", "AC1024", RS2::FormatDXFRW2010, DRW::AC1024},
    {"R2013", "AC1027", RS2::FormatDXFRW2013, DRW::AC1027},
    {"R2018", "AC1032", RS2::FormatDXFRW2018, DRW::AC1032}}};
#ifdef DWGSUPPORT
constexpr std::array<Format, 6> Dwg{{
    {"R2000", "AC1015", RS2::FormatDWG, DRW::AC1015, true},
    {"R2004", "AC1018", RS2::FormatDWG2004, DRW::AC1018, true},
    {"R2007", "AC1021", RS2::FormatDWG2007, DRW::AC1021, true},
    {"R2010", "AC1024", RS2::FormatDWG2010, DRW::AC1024, true},
    {"R2013", "AC1027", RS2::FormatDWG2013, DRW::AC1027, true},
    {"R2018", "AC1032", RS2::FormatDWG2018, DRW::AC1032, true}}};
// R14 is a read-only legacy row, not a new application writer enum.
constexpr Format LegacyR14{"R14", "AC1014", RS2::FormatDWG, DRW::AC1014, true};
constexpr Format LegacyR13{"R13", "AC1012", RS2::FormatDWG, DRW::AC1012, true};
constexpr std::array<Format, 8> DwgInputs{LegacyR13, LegacyR14,
    Dwg[0], Dwg[1], Dwg[2], Dwg[3], Dwg[4], Dwg[5]};
#endif
Path fixture(const std::string &name) {
#ifdef LIBRECAD_TEST_DIR
    return Path(LIBRECAD_TEST_DIR) / "offsetcurve_independent" / name;
#else
    return Path(__FILE__).parent_path() / "testdata" / "offsetcurve_independent" / name;
#endif
}
std::string boundedBytes(const Path &path) {
    std::ifstream in(path, std::ios::binary);
    REQUIRE(in.good());
    std::string result;
    std::array<char, 4096> buffer{};
    while (in.read(buffer.data(), buffer.size()) || in.gcount()) {
        REQUIRE(result.size() + static_cast<std::size_t>(in.gcount()) <= 2u*1024u*1024u);
        result.append(buffer.data(), static_cast<std::size_t>(in.gcount()));
    }
    REQUIRE(in.eof());
    return result;
}
std::string nativeName(const std::string &prefix, int degree, const char *suffix) {
    return prefix + "-native-degree" + std::to_string(degree) + suffix;
}
std::string materialize(const char *file, const Format &format, const Path &destination) {
    auto bytes = boundedBytes(fixture(file));
    const std::string marker = "@ACADVER@";
    const auto at = bytes.find(marker);
    REQUIRE(at != std::string::npos);
    REQUIRE(bytes.find(marker, at + marker.size()) == std::string::npos);
    bytes.replace(at, marker.size(), format.code);
    f::put(destination, bytes);
    return bytes;
}
void handNative(const Path &path, int degree, const Format &format = Dxf.back()) {
    REQUIRE((degree == 1 || degree == 3));
    REQUIRE(format.app != RS2::FormatDXFRW12);
    materialize(degree == 1 ? "hand-degree1.dxf.in" : "hand-degree3.dxf.in", format, path);
}
std::vector<RS_Vector> authoredControls(int degree) {
    if (degree == 1) return {{1.25, 0}, {1.2499375005208315, .012499791667708331}};
    REQUIRE(degree == 3);
    return {{1.25, 0}, {1.25, .004166675347243924},
        {1.2499791665798616, .008333324652495661},
        {1.2499375005208315, .012499791667708331}};
}
std::vector<double> authoredKnots(int degree) {
    std::vector<double> knots(static_cast<std::size_t>(degree + 1), 0.);
    knots.insert(knots.end(), static_cast<std::size_t>(degree + 1), 1.);
    return knots;
}
Tags expectedOwned(double distance = .25) {
    return {
        std::make_shared<DRW_Variant>(1001, std::string("LibreCad")),
        std::make_shared<DRW_Variant>(1000, std::string("LC_OFFSET_CURVE")),
        std::make_shared<DRW_Variant>(1002, std::string("{")),
        std::make_shared<DRW_Variant>(1070, std::int32_t{1}),
        std::make_shared<DRW_Variant>(1000, std::string("ELLIPSE")),
        std::make_shared<DRW_Variant>(1011, DRW_Coord(0, 0, 0)),
        std::make_shared<DRW_Variant>(1012, DRW_Coord(1, 0, 0)),
        std::make_shared<DRW_Variant>(1040, 1.),
        std::make_shared<DRW_Variant>(1040, 0.),
        std::make_shared<DRW_Variant>(1040, .01),
        std::make_shared<DRW_Variant>(1070, std::int32_t{0}),
        std::make_shared<DRW_Variant>(1041, distance),
        std::make_shared<DRW_Variant>(1040, .0001),
        std::make_shared<DRW_Variant>(1002, std::string("}"))};
}
Tags expectedForeign(const std::string &tag) {
    return {std::make_shared<DRW_Variant>(1001, std::string("LC2961_FIXTURE")),
        std::make_shared<DRW_Variant>(1000, tag),
        std::make_shared<DRW_Variant>(1070, std::int32_t{1}),
        std::make_shared<DRW_Variant>(1000, std::string("independent-math-not-LibreCAD-writer"))};
}
void checkForeign(const Tags &tags, const std::string &tag) {
    CHECK(f::tagsState(f::application(tags, "LC2961_FIXTURE")) == f::tagsState(expectedForeign(tag)));
}
void close(double actual, double expected) {
    REQUIRE(std::isfinite(actual));
    CHECK(std::abs(actual - expected) <= 128*std::numeric_limits<double>::epsilon()*std::max(1., std::abs(expected)));
}
RS_Vector bezier(std::vector<RS_Vector> net, double t) {
    REQUIRE((net.size() == 2 || net.size() == 4));
    for (std::size_t n = net.size(); n > 1; --n)
        for (std::size_t i = 0; i + 1 < n; ++i) net[i] = net[i]*(1 - t) + net[i + 1]*t;
    return net.front();
}
LC_OffsetCurve &native(RS_Graphic &graphic, int degree, const std::string &tag, double distance = .25) {
    auto *curve = dynamic_cast<LC_OffsetCurve*>(f::sole(graphic));
    REQUIRE(curve);
    REQUIRE(curve->hasEditableDefinition());
    CHECK(curve->rtti() == RS2::EntityOffsetCurve);
    CHECK(curve->getParent() == &graphic);
    CHECK(curve->isVisible()); REQUIRE(curve->getLayer()); CHECK(curve->getLayer()->getName() == "0");
    const auto &d = curve->definition();
    CHECK(d.basis.center == RS_Vector(0, 0)); CHECK(d.basis.majorP == RS_Vector(1, 0));
    CHECK(d.basis.ratio == 1); CHECK(d.basis.angle1 == 0); CHECK(d.basis.angle2 == .01);
    CHECK(d.basis.isArc); CHECK_FALSE(d.basis.reversed); CHECK_FALSE(d.towardCentre); CHECK_FALSE(d.cacheReversed);
    CHECK(d.distance == distance); CHECK(d.relTolerance == .0001);
    CHECK(curve->unknownOwnedFlags() == 0); CHECK(curve->unknownOwnedTail().empty());
    CHECK(f::markerCount(curve->getDrwExtData()) == 0);
    checkForeign(curve->getDrwExtData(), tag);
    const auto &data = curve->getData();
    REQUIRE((data.degree == 1 || data.degree == 3));
    if (degree) {
        CHECK(data.degree == static_cast<std::size_t>(degree));
        const auto controls = authoredControls(degree);
        REQUIRE(data.controlPoints.size() == controls.size());
        for (std::size_t i = 0; i < controls.size(); ++i) {
            close(data.controlPoints[i].x, controls[i].x); close(data.controlPoints[i].y, controls[i].y);
            CHECK(data.controlPoints[i].z == 0);
        }
        CHECK(data.knotslist == authoredKnots(degree));
    }
    CHECK(data.weights == std::vector<double>(data.controlPoints.size(), 1.));
    CHECK(data.fitPoints.empty()); CHECK_FALSE(data.m_closedFlag); CHECK_FALSE(data.m_periodicFlag);
    double a = 0, b = 0; REQUIRE(curve->getParameterDomain(a, b)); REQUIRE(a < b);
    for (unsigned i = 0; i <= 16; ++i) {
        const double t = static_cast<double>(i)/16;
        const auto p = curve->getPointAt(a + t*(b - a));
        REQUIRE(p.valid);
        CHECK(std::abs(std::hypot(p.x, p.y) - (1 + distance)) <= curve->checkedWorldTolerance());
        if (degree) {
            const auto expected = bezier(authoredControls(degree), t);
            close(p.x, expected.x); close(p.y, expected.y);
        }
    }
    return *curve;
}
struct Snapshot {
    const LC_OffsetCurve &curve;
    const RS_Graphic &graphic;
    const std::string before;
    const RS_Pen pen;
    Snapshot(const LC_OffsetCurve &curve, const RS_Graphic &graphic)
        : curve(curve), graphic(graphic), before(state()), pen(curve.getPen(false)) {}
    std::string state() const {
        std::ostringstream out;
        bool undo = false, redo = false; graphic.collectUndoState(undo, redo);
        out << graphic.count() << graphic.isModified() << undo << redo << ';';
        out << curve.getParent() << ':' << curve.getLayer() << ':' << curve.sourceHandle() << ':'
            << curve.xDictHandle() << ':' << curve.lineage() << ':' << curve.ownerLineage() << ':'
            << curve.isSelected() << curve.isDeleted() << curve.isVisible() << ';';
        for (auto handle : curve.reactorHandles()) out << handle << ';';
        const auto &d = curve.definition();
        for (double n : {d.basis.center.x, d.basis.center.y, d.basis.majorP.x, d.basis.majorP.y,
                 d.basis.ratio, d.basis.angle1, d.basis.angle2, d.distance, d.relTolerance,
                 curve.checkedWorldTolerance()}) out << f::bits(n) << ';';
        out << d.basis.reversed << d.basis.isArc << d.towardCentre << d.cacheReversed << curve.isDefined() << ';';
        const auto &data = curve.getData();
        out << data.degree << ':' << static_cast<int>(data.type) << ':' << data.m_closedFlag << data.m_periodicFlag << ';';
        for (const auto &p : data.controlPoints) out << f::bits(p.x) << ':' << f::bits(p.y) << ':' << f::bits(p.z) << ';';
        for (auto k : data.knotslist) out << f::bits(k) << ';';
        for (auto w : data.weights) out << f::bits(w) << ';';
        out << f::tagsState(curve.getDrwExtData());
        for (const auto &v : curve.getDrwExtData()) out << v.get() << ';';
        for (const auto *child : curve) out << child << ':' << child->getParent() << ';';
        return out.str();
    }
    void check() const { CHECK(state() == before); CHECK(curve.getPen(false) == pen); }
};
struct Capture final : lc::test::NullDrwInterface {
    std::vector<DRW_Spline> splines;
    std::vector<DRW_Line> lines;
    std::vector<std::string> apps;
    std::vector<DRW_PlotSettings> plots;
    std::vector<DRW_Layout> layouts;
    unsigned ellipses = 0;
    void addSpline(const DRW_Spline *spline) override { REQUIRE(spline); splines.push_back(*spline); }
    void addLine(const DRW_Line &line) override { lines.push_back(line); }
    void addAppId(const DRW_AppId &app) override { apps.push_back(app.name); }
    void addEllipse(const DRW_Ellipse&) override { ++ellipses; }
    void addPlotSettings(const DRW_PlotSettings *settings) override {
        REQUIRE(settings); plots.push_back(*settings);
    }
    void addLayout(const DRW_Layout &layout) override { layouts.push_back(layout); }
};
Capture inspect(const Path &path, const Format &format) {
    Capture result;
    if (format.dwg) {
#ifdef DWGSUPPORT
        REQUIRE(boundedBytes(path).substr(0, 6) == format.code);
        dwgRW reader(path.string().c_str()); REQUIRE(reader.read(&result, false));
        CHECK(reader.getVersion() == format.codec);
#else
        FAIL("No DWG test should be registered in a non-DWG build");
#endif
    } else {
        dxfRW reader(path.string().c_str()); REQUIRE(reader.read(&result, false));
        CHECK(reader.getSourceVersion() == format.codec);
    }
    return result;
}
void appIds(const Capture &captured, bool needsNative) {
    for (const auto *name : {"LibreCad", "LC2961_FIXTURE"}) {
        const auto count = std::count_if(captured.apps.begin(), captured.apps.end(), [=](const std::string &app) {
            return QString::fromStdString(app).compare(name, Qt::CaseInsensitive) == 0;
        });
        if (needsNative || std::string(name) == "LC2961_FIXTURE") CHECK(count == 1);
        else CHECK(count <= 1);
    }
}
void rawAscii(const Path &path, const Format &format, double distance) {
    const auto records = lc::test::dxfRecords(path);
    unsigned versions = 0;
    for (const auto &record : records)
        for (std::size_t i = 0; i + 1 < record.m_groups.size(); ++i)
            if (record.m_groups[i].m_code == 9 && record.m_groups[i].m_value == "$ACADVER") {
                CHECK(record.m_groups[i + 1].m_code == 1);
                CHECK(record.m_groups[i + 1].m_value == format.code); ++versions;
            }
    CHECK(versions == 1);
    const auto splines = lc::test::dxfRecords(path, "SPLINE"); REQUIRE(splines.size() == 1);
    std::vector<lc::test::DxfGroup> owned;
    bool inside = false;
    for (const auto &group : splines.front().m_groups) {
        if (group.m_code == 1001) inside = QString::fromStdString(group.m_value).compare("LibreCad", Qt::CaseInsensitive) == 0;
        if (inside) owned.push_back(group);
    }
    constexpr std::array<int, 18> codes{1001, 1000, 1002, 1070, 1000, 1011, 1021, 1031,
        1012, 1022, 1032, 1040, 1040, 1040, 1070, 1041, 1040, 1002};
    REQUIRE(owned.size() == codes.size());
    for (std::size_t i = 0; i < codes.size(); ++i) CHECK(owned[i].m_code == codes[i]);
    CHECK(owned[1].m_value == "LC_OFFSET_CURVE"); CHECK(owned[2].m_value == "{");
    CHECK(owned[4].m_value == "ELLIPSE"); CHECK(owned[17].m_value == "}");
    const std::array<std::pair<std::size_t, double>, 13> values{{
        {3, 1}, {5, 0}, {6, 0}, {7, 0}, {8, 1}, {9, 0}, {10, 0},
        {11, 1}, {12, 0}, {13, .01}, {14, 0}, {15, distance}, {16, .0001}}};
    for (const auto &value : values) close(std::stod(owned[value.first].m_value), value.second);
    CHECK(lc::test::dxfRecords(path, "ELLIPSE").empty());
}
void inspectSpline(const Path &path, const Format &format, const RS_SplineData &expected,
                   const std::string &tag, double distance = .25, bool binary = false) {
    const auto captured = inspect(path, format);
    REQUIRE(captured.splines.size() == 1); CHECK(captured.lines.empty()); CHECK(captured.ellipses == 0);
    appIds(captured, true);
    const auto &spline = captured.splines.front();
    if (!format.dwg && format.codec >= DRW::AC1015)
        CHECK(spline.parentHandle == DRW::DxfModelSpaceBlockRecordHandle);
    CHECK(spline.degree == static_cast<int>(expected.degree));
    CHECK(spline.ncontrol == static_cast<int>(expected.controlPoints.size()));
    CHECK(spline.nknots == static_cast<int>(expected.knotslist.size()));
    REQUIRE(spline.controllist.size() == expected.controlPoints.size());
    for (std::size_t i = 0; i < expected.controlPoints.size(); ++i) {
        REQUIRE(spline.controllist[i]);
        close(spline.controllist[i]->x, expected.controlPoints[i].x);
        close(spline.controllist[i]->y, expected.controlPoints[i].y);
        CHECK(spline.controllist[i]->z == 0);
    }
    CHECK(spline.knotslist == expected.knotslist);
    CHECK((spline.weightlist.empty() || spline.weightlist == std::vector<double>(expected.controlPoints.size(), 1.)));
    CHECK(spline.fitlist.empty()); CHECK(spline.nfit == 0);
    // R13/R14 APPID spelling is uppercased by the independent writer. APPID
    // identity is case-insensitive; the ordered payload itself remains exact.
    Tags owned;
    bool inOwned = false;
    for (const auto &tagItem : spline.extData) {
        REQUIRE(tagItem);
        if (tagItem->code() == 1001) {
            REQUIRE(tagItem->type() == DRW_Variant::STRING);
            inOwned = QString::fromStdString(*tagItem->content.s).compare("LibreCad", Qt::CaseInsensitive) == 0;
            continue;
        }
        if (inOwned) owned.push_back(tagItem);
    }
    auto expectedPayload = expectedOwned(distance);
    expectedPayload.erase(expectedPayload.begin());
    CHECK(f::tagsState(owned) == f::tagsState(expectedPayload));
    CHECK(f::markerCount(spline.extData) == 1); checkForeign(spline.extData, tag);
    if (!format.dwg && !binary) rawAscii(path, format, distance);
}
void load(RS_Graphic &graphic, const Path &path, const Format &format) {
    REQUIRE(lc::test::importDrawing(graphic, path, format.app));
    graphic.onLoadingCompleted();
}
std::string handTag(int degree) { return "hand-ascii-native-degree" + std::to_string(degree); }
void writerRow(const Format &format, int degree, bool edited = false) {
    Setup setup;
    lc::test::OutputDirectory out("offset-matrix-write-" + std::string(format.dwg ? "dwg-" : "dxf-") +
        format.release + "-d" + std::to_string(degree) + (edited ? "-edit" : ""));
    const auto input = out.file("hand-input.dxf"); handNative(input, degree);
    RS_Graphic graphic; load(graphic, input, Dxf.back());
    auto &curve = native(graphic, degree, handTag(degree));
    const double distance = edited ? .5 : .25;
    if (edited) {
        const auto changed = curve.setDistance(distance);
        REQUIRE(changed); CHECK(changed.changed); CHECK(changed.engineRuns == 1);
        native(graphic, 0, handTag(degree), distance);
    }
    const Snapshot before(curve, graphic);
    const auto target = out.file(format.dwg ? "actual.dwg" : "actual.dxf");
    REQUIRE(lc::test::exportDrawing(graphic, target, format.app)); before.check();
    inspectSpline(target, format, curve.getData(), handTag(degree), distance);
    RS_Graphic reopened; load(reopened, target, format);
    native(reopened, edited ? 0 : degree, handTag(degree), distance);
}
void line(RS_Graphic &graphic, const std::string &tag) {
    auto *entity = f::sole(graphic); REQUIRE(entity->rtti() == RS2::EntityLine);
    CHECK(entity->getStartpoint() == RS_Vector(0, 0)); CHECK(entity->getEndpoint() == RS_Vector(1, 0));
    CHECK(entity->isVisible()); CHECK(entity->getParent() == &graphic);
    CHECK(f::markerCount(entity->getDrwExtData()) == 0); checkForeign(entity->getDrwExtData(), tag);
}
void lineWriter(const Format &format) {
    Setup setup;
    lc::test::OutputDirectory out("offset-matrix-line-" + std::string(format.dwg ? "dwg-" : "dxf-") + format.release);
    // This also exercises an independent reader control for every DXF row.
    const auto inputFormat = format.dwg ? Dxf.back() : format;
    materialize("hand-line.dxf.in", inputFormat, out.file("hand-line.dxf"));
    RS_Graphic graphic; load(graphic, out.file("hand-line.dxf"), inputFormat);
    line(graphic, "hand-ascii-line-control");
    const auto target = out.file(format.dwg ? "actual-line.dwg" : "actual-line.dxf");
    REQUIRE(lc::test::exportDrawing(graphic, target, format.app));
    const auto captured = inspect(target, format);
    REQUIRE(captured.lines.size() == 1); CHECK(captured.splines.empty()); CHECK(captured.ellipses == 0);
    appIds(captured, false); checkForeign(captured.lines.front().extData, "hand-ascii-line-control");
    CHECK(captured.lines.front().basePoint.x == 0); CHECK(captured.lines.front().basePoint.y == 0);
    CHECK(captured.lines.front().secPoint.x == 1); CHECK(captured.lines.front().secPoint.y == 0);
    CHECK(f::markerCount(captured.lines.front().extData) == 0);
    RS_Graphic again; load(again, target, format); line(again, "hand-ascii-line-control");
}
template <typename Writer> struct PlotEmitter final : lc::test::NullDrwInterface {
    Writer &writer;
    DRW_PlotSettings plot;
    DRW_Layout layout;
    DRW_Dictionary plots;
    DRW_Dictionary layouts;
    explicit PlotEmitter(Writer &writer, int dpi) : writer(writer) {
        plots.handle = 0x712; layouts.handle = 0x713;
        for (auto *dictionary : {&plots, &layouts}) {
            dictionary->parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
            dictionary->cloning = 1; dictionary->hardOwner = 1;
        }
        plots.m_entries.push_back({"PageSetup", 0x710});
        layouts.m_entries.push_back({"Layout1", 0x711});
        plot.handle = 0x710; plot.parentHandle = plots.handle;
        plot.pageSetupName = "PageSetup";
        plot.shadePlotCustomDPI = dpi;
        layout.handle = 0x711; layout.parentHandle = layouts.handle;
        layout.name = "Layout1"; layout.shadePlotCustomDPI = dpi;
        layout.tabOrder = 1;
        layout.paperSpaceBlockRecordHandle.ref = std::is_same_v<Writer, dxfRW>
            ? DRW::DxfPaperSpaceBlockRecordHandle : DRW::DwgPaperSpaceBlockRecordHandle;
    }
    void writeHeader(DRW_Header &) override {
        if constexpr (std::is_same_v<Writer, dxfRW>) {
            writer.setRootDictEntries({{"ACAD_PLOTSETTINGS", "712"}, {"ACAD_LAYOUT", "713"}});
            writer.setNamedDictObjects({plots, layouts});
        }
    }
    void writeDwgClasses() override {
#ifdef DWGSUPPORT
        if constexpr (std::is_same_v<Writer, dwgRW>) {
            REQUIRE(writer.registerPlotSettingsObjectClass(plot.handle));
            REQUIRE(writer.registerDwgNamedObjectDictionaryEntry("ACAD_PLOTSETTINGS", plots.handle));
            REQUIRE(writer.registerDwgNamedObjectDictionaryEntry("ACAD_LAYOUT", layouts.handle));
        }
#endif
    }
    void writeObjects() override {
#ifdef DWGSUPPORT
        if constexpr (std::is_same_v<Writer, dwgRW>) {
            REQUIRE(writer.writeDictionary(&plots)); REQUIRE(writer.writeDictionary(&layouts));
        }
#endif
        REQUIRE(writer.writePlotSettings(&plot)); REQUIRE(writer.writeLayout(&layout));
    }
};
void plotRow(const Format &format, int dpi) {
    lc::test::OutputDirectory out("offset-matrix-plot-" + std::string(format.dwg ? "dwg-" : "dxf-") +
        format.release + "-dpi" + std::to_string(dpi));
    const auto file = out.file(format.dwg ? "plots.dwg" : "plots.dxf");
    if (format.dwg) {
#ifdef DWGSUPPORT
        dwgRW writer(file.string().c_str()); PlotEmitter<dwgRW> source(writer, dpi);
        REQUIRE(writer.write(&source, format.codec, false));
        CHECK(source.plot.shadePlotCustomDPI == dpi); CHECK(source.layout.shadePlotCustomDPI == dpi);
#else
        FAIL("DWG-only plot row in non-DWG configuration");
#endif
    } else {
        dxfRW writer(file.string().c_str()); PlotEmitter<dxfRW> source(writer, dpi);
        REQUIRE(writer.write(&source, format.codec, false));
        CHECK(source.plot.shadePlotCustomDPI == dpi); CHECK(source.layout.shadePlotCustomDPI == dpi);
    }
    lc::test::keepForInspection(file);
    const auto captured = inspect(file, format);
    const int expected = format.codec >= DRW::AC1018 ? (dpi == 0 ? 300 : dpi) : 0;
    REQUIRE(captured.plots.size() == 1); REQUIRE(captured.layouts.size() == 1);
    CHECK(captured.plots.front().shadePlotCustomDPI == expected);
    CHECK(captured.layouts.front().shadePlotCustomDPI == expected);
    if (!format.dwg) for (const auto *name : {"PLOTSETTINGS", "LAYOUT"}) {
        const auto records = lc::test::dxfRecords(file, name); REQUIRE(records.size() == 1);
        const auto values = records.front().own(78);
        if (format.codec >= DRW::AC1018) {
            REQUIRE(values.size() == 1); CHECK(std::stoi(values.front()) == expected);
        } else CHECK(values.empty());
    }
}
}

TEST_CASE("G matrix DXF plot and layout writers use a valid unspecified DPI without changing source", "[lc-offsetcurve][g-matrix][plot][dxf]") {
    Setup setup;
    for (const auto &format : Dxf) if (format.codec >= DRW::AC1015)
        for (int dpi : {0, 100, 600, 32767}) DYNAMIC_SECTION(format.release << " DPI " << dpi) plotRow(format, dpi);
}
#ifdef DWGSUPPORT
TEST_CASE("G matrix DWG plot and layout writers use a valid unspecified DPI without changing source", "[lc-offsetcurve][g-matrix][plot][dwg]") {
    Setup setup;
    for (const auto &format : Dwg) for (int dpi : {0, 100, 600, 32767})
        DYNAMIC_SECTION(format.release << " DPI " << dpi) plotRow(format, dpi);
}
#endif

TEST_CASE("G matrix independently generated binary fixture hashes are pinned", "[lc-offsetcurve][g-matrix][provenance]") {
    const auto manifest = boundedBytes(fixture("SHA256SUMS"));
    std::istringstream in(manifest);
    std::set<std::string> names;
    std::string hash, name;
    while (in >> hash >> name) {
        CAPTURE(name);
        REQUIRE(hash.size() == 64); REQUIRE(names.insert(name).second);
        REQUIRE(name.find('/') == std::string::npos);
        const auto bytes = boundedBytes(fixture(name));
        CHECK(QCryptographicHash::hash(QByteArray::fromStdString(bytes), QCryptographicHash::Sha256).toHex().toStdString() == hash);
    }
    CHECK(names.size() == 24);
    CHECK(names.count("dwg-R13-native-degree1.dwg") == 0);
    CHECK(names.count("dwg-R2007-native-degree3.dwg") == 0);
    std::istringstream hand(boundedBytes(fixture("HAND-SHA256SUMS")));
    std::set<std::string> handNames;
    while (hand >> hash >> name) {
        CAPTURE(name);
        REQUIRE(hash.size() == 64); REQUIRE(handNames.insert(name).second);
        REQUIRE(name.find('/') == std::string::npos);
        const auto bytes = boundedBytes(fixture(name));
        CHECK(QCryptographicHash::hash(QByteArray::fromStdString(bytes), QCryptographicHash::Sha256).toHex().toStdString() == hash);
    }
    CHECK(handNames.size() == 4);
    std::istringstream oda(boundedBytes(fixture("ODA-SHA256SUMS")));
    std::set<std::string> odaNames;
    while (oda >> hash >> name) {
        CAPTURE(name);
        REQUIRE(hash.size() == 64); REQUIRE(odaNames.insert(name).second);
        REQUIRE(name.find('/') == std::string::npos);
        const auto bytes = boundedBytes(fixture(name));
        CHECK(QCryptographicHash::hash(QByteArray::fromStdString(bytes), QCryptographicHash::Sha256).toHex().toStdString() == hash);
    }
    CHECK(odaNames.size() == 24);
}

TEST_CASE("G matrix independent ASCII adoption covers every editable DXF version and both degrees", "[lc-offsetcurve][g-matrix][read][dxf]") {
    Setup setup;
    for (const auto &format : Dxf) {
        if (format.app == RS2::FormatDXFRW12) continue; // Its ordinary lane is a separate real test below.
        for (int degree : {1, 3}) DYNAMIC_SECTION(format.release << " degree " << degree) {
            lc::test::OutputDirectory out("offset-matrix-ascii-read-" + std::string(format.release) + "-d" + std::to_string(degree));
            const auto path = out.file("hand-input.dxf"); handNative(path, degree, format);
            RS_Graphic graphic; load(graphic, path, format); native(graphic, degree, handTag(degree));
            const auto captured = inspect(path, format); REQUIRE(captured.splines.size() == 1);
            appIds(captured, true); CHECK(captured.ellipses == 0);
        }
    }
}

TEST_CASE("G matrix independent binary DXF R14 and R2018 inputs reach actual application adoption", "[lc-offsetcurve][g-matrix][read][dxf][binary]") {
    Setup setup;
    for (const auto &format : {Dxf[1], Dxf[7]}) for (int degree : {1, 3})
        DYNAMIC_SECTION(format.release << " degree " << degree) {
            const auto tag = nativeName("dxf-binary-" + std::string(format.release), degree, "");
            const auto path = fixture(tag + ".dxf");
            const std::string signature("AutoCAD Binary DXF\r\n\x1a\0", 22);
            REQUIRE(boundedBytes(path).compare(0, signature.size(), signature) == 0);
            RS_Graphic graphic; load(graphic, path, format); native(graphic, degree, tag);
            const auto captured = inspect(path, format); appIds(captured, true);
        }
}

TEST_CASE("G matrix binary DXF codec writes remain adoptable without a binary application option", "[lc-offsetcurve][g-matrix][write][dxf][binary]") {
    Setup setup;
    for (const auto &format : {Dxf[1], Dxf[7]}) for (int degree : {1, 3})
        DYNAMIC_SECTION(format.release << " degree " << degree) {
            const auto tag = nativeName("dxf-binary-" + std::string(format.release), degree, "");
            const auto independent = inspect(fixture(tag + ".dxf"), format);
            REQUIRE(independent.splines.size() == 1);
            lc::test::OutputDirectory out("offset-matrix-binary-write-" + std::string(format.release) + "-d" + std::to_string(degree));
            const auto path = out.file("codec-actual.dxf");
            dxfRW writer(path.string().c_str());
            struct Emit final : lc::test::NullDrwInterface {
                dxfRW &writer; DRW_Spline spline;
                Emit(dxfRW &writer, const DRW_Spline &spline) : writer(writer), spline(spline) {
                    // The imported owner belongs to the independent source document.
                    this->spline.handle = DRW::NoHandle;
                    this->spline.parentHandle = DRW::NoHandle;
                }
                void writeAppId() override {
                    for (const auto *name : {"LibreCad", "LC2961_FIXTURE"}) {
                        DRW_AppId app; app.name = name; REQUIRE(writer.writeAppId(&app));
                    }
                }
                void writeEntities() override { REQUIRE(writer.writeSpline(&spline)); }
            } source(writer, independent.splines.front());
            REQUIRE(writer.write(&source, format.codec, true)); lc::test::keepForInspection(path);
            const std::string signature("AutoCAD Binary DXF\r\n\x1a\0", 22);
            REQUIRE(boundedBytes(path).compare(0, signature.size(), signature) == 0);
            RS_Graphic graphic; load(graphic, path, format);
            const auto &curve = native(graphic, degree, tag);
            inspectSpline(path, format, curve.getData(), tag, .25, true);
        }
}

TEST_CASE("G matrix application DXF writers inspect explicit versions schema and geometry", "[lc-offsetcurve][g-matrix][write][dxf]") {
    for (const auto &format : Dxf) {
        if (format.app == RS2::FormatDXFRW12) continue;
        for (int degree : {1, 3}) DYNAMIC_SECTION(format.release << " degree " << degree) writerRow(format, degree);
    }
}

TEST_CASE("G matrix meaningful edits survive a second save in every editable DXF version", "[lc-offsetcurve][g-matrix][edit][dxf]") {
    for (const auto &format : Dxf) {
        if (format.app == RS2::FormatDXFRW12) continue;
        for (int degree : {1, 3}) DYNAMIC_SECTION(format.release << " original degree " << degree) writerRow(format, degree, true);
    }
}

TEST_CASE("G matrix R12 read and degree-one and degree-three writes remain ordinary checked polylines", "[lc-offsetcurve][g-matrix][r12]") {
    Setup setup;
    RS_Graphic authored; load(authored, fixture("hand-R12-polyline.dxf"), Dxf[0]);
    CHECK(f::sole(authored)->rtti() == RS2::EntityPolyline);
    CHECK(f::markerCount(f::sole(authored)->getDrwExtData()) == 0);
    for (int degree : {1, 3}) DYNAMIC_SECTION("original degree " << degree) {
        lc::test::OutputDirectory out("offset-matrix-r12-d" + std::to_string(degree));
        handNative(out.file("native-input.dxf"), degree);
        RS_Graphic graphic; load(graphic, out.file("native-input.dxf"), Dxf.back());
        const auto &curve = native(graphic, degree, handTag(degree)); const Snapshot before(curve, graphic);
        const auto path = out.file("actual-polyline.dxf");
        REQUIRE(lc::test::exportDrawing(graphic, path, Dxf[0].app)); before.check();
        const auto captured = inspect(path, Dxf[0]);
        CHECK(captured.splines.empty()); CHECK(captured.ellipses == 0);
        CHECK(lc::test::dxfRecords(path, "SPLINE").empty()); CHECK(lc::test::dxfRecords(path, "ELLIPSE").empty());
        const auto polylines = lc::test::dxfRecords(path, "POLYLINE"); REQUIRE(polylines.size() == 1);
        for (const auto &value : polylines.front().own(1000)) CHECK(value != "LC_OFFSET_CURVE");
        const auto vertices = lc::test::dxfRecords(path, "VERTEX"); REQUIRE(vertices.size() >= 2); REQUIRE(vertices.size() <= 32767);
        std::vector<RS_Vector> points;
        double previousAngle = -1e-12;
        for (const auto &vertex : vertices) {
            const auto x = vertex.own(10), y = vertex.own(20); REQUIRE(x.size() == 1); REQUIRE(y.size() == 1);
            const RS_Vector p(std::stod(x.front()), std::stod(y.front()));
            REQUIRE(std::isfinite(p.x)); REQUIRE(std::isfinite(p.y));
            const double angle = std::atan2(p.y, p.x);
            CHECK(angle >= previousAngle - 1e-12); CHECK(angle <= .01 + 1e-12); previousAngle = angle;
            CHECK(std::abs(std::hypot(p.x, p.y) - 1.25) < .0001);
            points.push_back(p);
        }
        const auto controls = authoredControls(degree);
        // Independent control-box diagonal for these literal monotone nets.
        const double approximationTolerance = 1e-4*std::hypot(
            1.25 - 1.2499375005208315, .012499791667708331);
        const double rounding = 64*std::numeric_limits<double>::epsilon()*1.25;
        if (degree == 3) CHECK(vertices.size() > 2);
        close(points.front().x, controls.front().x); close(points.front().y, controls.front().y);
        close(points.back().x, controls.back().x); close(points.back().y, controls.back().y);
        // Independent Bezier samples against the actual emitted segments;
        // this is an approximation regression, not a new continuous certificate.
        for (unsigned sample = 0; sample <= 64; ++sample) {
            const auto p = bezier(controls, static_cast<double>(sample)/64);
            double nearest = std::numeric_limits<double>::infinity();
            for (std::size_t i = 1; i < points.size(); ++i) {
                const auto a = points[i - 1], b = points[i];
                const double dx = b.x - a.x, dy = b.y - a.y, square = dx*dx + dy*dy;
                REQUIRE(square > 0);
                const double u = std::clamp(((p.x - a.x)*dx + (p.y - a.y)*dy)/square, 0., 1.);
                nearest = std::min(nearest, std::hypot(p.x - a.x - u*dx, p.y - a.y - u*dy));
            }
            CHECK(nearest <= approximationTolerance + rounding);
        }
        RS_Graphic reopened; load(reopened, path, Dxf[0]);
        CHECK(f::sole(reopened)->rtti() == RS2::EntityPolyline);
        CHECK(f::markerCount(f::sole(reopened)->getDrwExtData()) == 0);
    }
}

TEST_CASE("G matrix LINE controls remain unchanged through all explicit DXF writers", "[lc-offsetcurve][g-matrix][line-control][dxf]") {
    for (const auto &format : Dxf) DYNAMIC_SECTION(format.release) lineWriter(format);
}

TEST_CASE("G matrix independent binary DXF LINE controls stay ordinary", "[lc-offsetcurve][g-matrix][line-control][binary]") {
    Setup setup;
    for (const auto &format : {Dxf[1], Dxf[7]}) DYNAMIC_SECTION(format.release) {
        const auto tag = "dxf-binary-" + std::string(format.release) + "-line-control";
        RS_Graphic graphic; load(graphic, fixture(tag + ".dxf"), format); line(graphic, tag);
        const auto captured = inspect(fixture(tag + ".dxf"), format); REQUIRE(captured.lines.size() == 1);
        appIds(captured, false); CHECK(captured.splines.empty());
    }
}

TEST_CASE("G matrix binary DXF codec LINE writes provide actual external-inspection controls", "[lc-offsetcurve][g-matrix][line-control][binary][write]") {
    Setup setup;
    for (const auto &format : {Dxf[1], Dxf[7]}) DYNAMIC_SECTION(format.release) {
        const auto tag = "dxf-binary-" + std::string(format.release) + "-line-control";
        const auto independent = inspect(fixture(tag + ".dxf"), format);
        REQUIRE(independent.lines.size() == 1);
        lc::test::OutputDirectory out("offset-matrix-binary-line-write-" + std::string(format.release));
        const auto path = out.file("codec-actual-line.dxf");
        dxfRW writer(path.string().c_str());
        struct Emit final : lc::test::NullDrwInterface {
            dxfRW &writer; DRW_Line line;
            Emit(dxfRW &writer, const DRW_Line &line) : writer(writer), line(line) {
                this->line.handle = DRW::NoHandle;
                this->line.parentHandle = DRW::NoHandle;
            }
            void writeAppId() override {
                DRW_AppId app; app.name = "LC2961_FIXTURE"; REQUIRE(writer.writeAppId(&app));
            }
            void writeEntities() override { REQUIRE(writer.writeLine(&line)); }
        } source(writer, independent.lines.front());
        REQUIRE(writer.write(&source, format.codec, true)); lc::test::keepForInspection(path);
        const std::string signature("AutoCAD Binary DXF\r\n\x1a\0", 22);
        REQUIRE(boundedBytes(path).compare(0, signature.size(), signature) == 0);
        RS_Graphic graphic; load(graphic, path, format); line(graphic, tag);
        const auto captured = inspect(path, format); REQUIRE(captured.lines.size() == 1);
        if (format.codec >= DRW::AC1015)
            CHECK(captured.lines.front().parentHandle == DRW::DxfModelSpaceBlockRecordHandle);
        appIds(captured, false); CHECK(captured.splines.empty()); CHECK(captured.ellipses == 0);
        CHECK(f::markerCount(captured.lines.front().extData) == 0);
        checkForeign(captured.lines.front().extData, tag);
    }
}

#ifdef DWGSUPPORT
TEST_CASE("G matrix upstream counted Unicode NUL remains payload and cannot acquire native editability", "[lc-offsetcurve][g-matrix][read][dwg][upstream-defect]") {
    Setup setup;
    for (const auto &format : {Dwg[3], Dwg[4], Dwg[5]}) for (int degree : {1, 3})
        DYNAMIC_SECTION(format.release << " degree " << degree) {
            const auto path = fixture(nativeName("dwg-" + std::string(format.release), degree, ".dwg"));
            const auto raw = inspect(path, format); REQUIRE(raw.splines.size() == 1);
            const auto &tags = raw.splines.front().extData;
            unsigned countedMarker = 0;
            for (const auto &tag : tags) if (tag && tag->code() == 1000 && tag->type() == DRW_Variant::STRING)
                if (*tag->content.s == std::string("LC_OFFSET_CURVE") + '\0') ++countedMarker;
            REQUIRE(countedMarker == 1); CHECK(f::markerCount(tags) == 0);
            RS_Graphic graphic; load(graphic, path, format);
            const auto *curve = dynamic_cast<RS_Spline*>(f::sole(graphic)); REQUIRE(curve);
            CHECK(curve->rtti() == RS2::EntitySpline);
            CHECK(f::tagsState(curve->getDrwExtData()) == f::tagsState(tags));
            CHECK(curve->getData().degree == static_cast<std::size_t>(degree));
        }
    for (const auto &format : {Dwg[3], Dwg[4], Dwg[5]}) DYNAMIC_SECTION(format.release << " LINE") {
        const auto path = fixture("dwg-" + std::string(format.release) + "-line-control.dwg");
        const auto raw = inspect(path, format); REQUIRE(raw.lines.size() == 1);
        RS_Graphic graphic; load(graphic, path, format);
        const auto *entity = f::sole(graphic); CHECK(entity->rtti() == RS2::EntityLine);
        CHECK(entity->isVisible()); CHECK(entity->getStartpoint() == RS_Vector(0, 0));
        CHECK(entity->getEndpoint() == RS_Vector(1, 0));
        CHECK(f::tagsState(entity->getDrwExtData()) == f::tagsState(raw.lines.front().extData));
        const auto foreign = f::application(entity->getDrwExtData(), "LC2961_FIXTURE");
        REQUIRE(foreign.size() == 4); REQUIRE(foreign[1]->type() == DRW_Variant::STRING);
        CHECK(*foreign[1]->content.s == std::string("dwg-") + format.release + "-line-control" + '\0');
    }
}

TEST_CASE("G matrix upstream R14 visibility bit is preserved rather than inverted to match its writer", "[lc-offsetcurve][g-matrix][read][dwg][upstream-defect]") {
    Setup setup;
    for (int degree : {1, 3}) DYNAMIC_SECTION("degree " << degree) {
        const auto path = fixture(nativeName("dwg-R14", degree, ".dwg"));
        const auto raw = inspect(path, LegacyR14); REQUIRE(raw.splines.size() == 1);
        CHECK_FALSE(raw.splines.front().visible);
        RS_Graphic graphic; load(graphic, path, LegacyR14);
        const auto *curve = dynamic_cast<LC_OffsetCurve*>(f::sole(graphic)); REQUIRE(curve);
        CHECK(curve->hasEditableDefinition()); CHECK_FALSE(curve->isVisible());
        checkForeign(curve->getDrwExtData(), nativeName("dwg-R14", degree, ""));
        const auto controls = authoredControls(degree);
        REQUIRE(curve->getData().controlPoints.size() == controls.size());
        for (std::size_t i = 0; i < controls.size(); ++i) {
            close(curve->getData().controlPoints[i].x, controls[i].x);
            close(curve->getData().controlPoints[i].y, controls[i].y);
        }
    }
    const auto path = fixture("dwg-R14-line-control.dwg");
    const auto raw = inspect(path, LegacyR14); REQUIRE(raw.lines.size() == 1);
    CHECK_FALSE(raw.lines.front().visible);
    RS_Graphic graphic; load(graphic, path, LegacyR14);
    const auto *entity = f::sole(graphic); CHECK(entity->rtti() == RS2::EntityLine);
    CHECK_FALSE(entity->isVisible()); CHECK(entity->getStartpoint() == RS_Vector(0, 0));
    CHECK(entity->getEndpoint() == RS_Vector(1, 0));
    checkForeign(entity->getDrwExtData(), "dwg-R14-line-control");
}

TEST_CASE("G matrix independent ODA DWG fixtures reach adoption including genuine R13 R14 and R2007", "[lc-offsetcurve][g-matrix][read][dwg]") {
    Setup setup;
    for (const auto &format : DwgInputs) for (int degree : {1, 3})
        DYNAMIC_SECTION(format.release << " degree " << degree) {
            const auto tag = nativeName("dxf-ascii-R2018", degree, "");
            const auto path = fixture(nativeName("oda-dwg-" + std::string(format.release), degree, ".dwg"));
            RS_Graphic graphic; load(graphic, path, format); native(graphic, degree, tag);
            const auto captured = inspect(path, format); appIds(captured, true);
            REQUIRE(captured.splines.size() == 1); CHECK(captured.ellipses == 0);
        }
}

TEST_CASE("G matrix unmodified ACadSharp byte-coded DWG inputs independently reach adoption", "[lc-offsetcurve][g-matrix][read][dwg]") {
    Setup setup;
    for (const auto &format : {Dwg[0], Dwg[1]}) {
        for (int degree : {1, 3}) DYNAMIC_SECTION(format.release << " degree " << degree) {
            const auto tag = nativeName("dwg-" + std::string(format.release), degree, "");
            RS_Graphic graphic; load(graphic, fixture(tag + ".dwg"), format); native(graphic, degree, tag);
        }
        DYNAMIC_SECTION(format.release << " LINE") {
            const auto tag = "dwg-" + std::string(format.release) + "-line-control";
            RS_Graphic graphic; load(graphic, fixture(tag + ".dwg"), format); line(graphic, tag);
        }
    }
}

TEST_CASE("G matrix application DWG writers inspect all six explicit physical versions", "[lc-offsetcurve][g-matrix][write][dwg]") {
    for (const auto &format : Dwg) for (int degree : {1, 3})
        DYNAMIC_SECTION(format.release << " degree " << degree) writerRow(format, degree);
}

TEST_CASE("G matrix edited caches survive second save across all six DWG writers", "[lc-offsetcurve][g-matrix][edit][dwg]") {
    for (const auto &format : Dwg) for (int degree : {1, 3})
        DYNAMIC_SECTION(format.release << " original degree " << degree) writerRow(format, degree, true);
}

TEST_CASE("G matrix LINE controls qualify every DWG writer and independent available reader", "[lc-offsetcurve][g-matrix][line-control][dwg]") {
    for (const auto &format : Dwg) DYNAMIC_SECTION("write " << format.release) lineWriter(format);
    Setup setup;
    for (const auto &format : DwgInputs) DYNAMIC_SECTION("independent read " << format.release) {
        const auto path = fixture("oda-dwg-" + std::string(format.release) + "-line-control.dwg");
        const auto tag = "dxf-ascii-R2018-line-control";
        RS_Graphic graphic; load(graphic, path, format); line(graphic, tag);
        const auto captured = inspect(path, format);
        REQUIRE(captured.lines.size() == 1); CHECK(captured.splines.empty()); appIds(captured, false);
    }
}

TEST_CASE("G matrix real DXF to DWG conversions cover both degrees and every DWG target", "[lc-offsetcurve][g-matrix][conversion][dxf-to-dwg]") {
    Setup setup;
    for (const auto &format : Dwg) for (int degree : {1, 3})
        DYNAMIC_SECTION(format.release << " degree " << degree) {
            lc::test::OutputDirectory out("offset-matrix-convert-to-" + std::string(format.release) + "-d" + std::to_string(degree));
            const auto dxf = std::find_if(Dxf.begin(), Dxf.end(), [&](const Format &f) { return f.codec == format.codec; });
            REQUIRE(dxf != Dxf.end()); handNative(out.file("independent.dxf"), degree, *dxf);
            RS_Graphic graphic; load(graphic, out.file("independent.dxf"), *dxf);
            const auto &curve = native(graphic, degree, handTag(degree)); const Snapshot before(curve, graphic);
            const auto target = out.file("converted.dwg");
            REQUIRE(lc::test::exportDrawing(graphic, target, format.app)); before.check();
            inspectSpline(target, format, curve.getData(), handTag(degree));
            RS_Graphic reopened; load(reopened, target, format); native(reopened, degree, handTag(degree));
        }
}

TEST_CASE("G matrix independent DWG to DXF conversions include genuine R13 R14 and both degrees", "[lc-offsetcurve][g-matrix][conversion][dwg-to-dxf]") {
    Setup setup;
    for (const auto &source : DwgInputs) for (int degree : {1, 3})
        DYNAMIC_SECTION(source.release << " degree " << degree) {
            const auto targetFormat = std::find_if(Dxf.begin(), Dxf.end(), [&](const Format &f) {
                return f.codec == (source.codec == DRW::AC1012 ? DRW::AC1014 : source.codec);
            });
            REQUIRE(targetFormat != Dxf.end());
            const auto tag = nativeName("dxf-ascii-R2018", degree, "");
            RS_Graphic graphic; load(graphic, fixture(nativeName("oda-dwg-" + std::string(source.release), degree, ".dwg")), source);
            const auto &curve = native(graphic, degree, tag); const Snapshot before(curve, graphic);
            lc::test::OutputDirectory out("offset-matrix-convert-from-" + std::string(source.release) + "-d" + std::to_string(degree));
            const auto target = out.file("converted.dxf");
            REQUIRE(lc::test::exportDrawing(graphic, target, targetFormat->app)); before.check();
            inspectSpline(target, *targetFormat, curve.getData(), tag);
            RS_Graphic reopened; load(reopened, target, *targetFormat); native(reopened, degree, tag);
        }
}

TEST_CASE("G matrix alternating format chains preserve edits through every modern version", "[lc-offsetcurve][g-matrix][conversion][chain][edit]") {
    Setup setup;
    // Every modern DXF writer and every DWG writer is explicitly included.
    const std::array<Format, 13> chain{Dxf[1], Dwg[0], Dxf[2], Dwg[1], Dxf[3], Dwg[2],
        Dxf[4], Dwg[3], Dxf[5], Dwg[4], Dxf[6], Dwg[5], Dxf[7]};
    for (int degree : {1, 3}) DYNAMIC_SECTION("original degree " << degree) {
        lc::test::OutputDirectory out("offset-matrix-chain-d" + std::to_string(degree));
        handNative(out.file("hand-input.dxf"), degree);
        auto graphic = std::make_unique<RS_Graphic>(); load(*graphic, out.file("hand-input.dxf"), Dxf.back());
        native(*graphic, degree, handTag(degree));
        for (std::size_t step = 0; step < chain.size(); ++step) {
            CAPTURE(step, chain[step].release, chain[step].dwg);
            auto &curve = native(*graphic, step < 3 ? degree : 0, handTag(degree), step < 3 ? .25 : .5);
            if (step == 2) {
                const auto edit = curve.setDistance(.5);
                REQUIRE(edit); CHECK(edit.engineRuns == 1); CHECK(edit.changed);
            }
            const double distance = step < 2 ? .25 : .5;
            const Snapshot before(curve, *graphic);
            const auto target = out.file("step-" + std::to_string(step) + (chain[step].dwg ? ".dwg" : ".dxf"));
            REQUIRE(lc::test::exportDrawing(*graphic, target, chain[step].app)); before.check();
            inspectSpline(target, chain[step], curve.getData(), handTag(degree), distance);
            auto reopened = std::make_unique<RS_Graphic>(); load(*reopened, target, chain[step]);
            native(*reopened, step < 2 ? degree : 0, handTag(degree), distance);
            graphic = std::move(reopened);
        }
    }
}
#endif
