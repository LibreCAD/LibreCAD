/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QXmlStreamReader>

#include "drw_base.h"
#include "lc_filtertestsupport.h"
#include "lc_makercamsvg.h"
#include "lc_offsetcurve.h"
#include "lc_xmlwriterqxmlstreamwriter.h"
#include "rs_block.h"
#include "rs_filterjww.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"

// Opt in only when a build actually supplies the historical dxflib dependency,
// RS_FilterDXF translation unit, and removed FormatDXFOLD/FormatDXFOLD12 enums.
// Header discovery alone would not establish a linkable real exporter.
#ifdef LC_TEST_LEGACY_DXF_EXPORT
#include "rs_filterdxf.h"
#endif

namespace {
const QString svgNamespace = QStringLiteral("http://www.w3.org/2000/svg");
const QString lcNamespace = QStringLiteral("https://librecad.org");
const QString xlinkNamespace = QStringLiteral("http://www.w3.org/1999/xlink");

std::uint64_t bits(const double value) {
    std::uint64_t result;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

void exactPoint(const RS_Vector& a, const RS_Vector& b) {
    CHECK(a.valid == b.valid); CHECK(bits(a.x) == bits(b.x)); CHECK(bits(a.y) == bits(b.y)); CHECK(bits(a.z) == bits(b.z));
}

struct Drawing {
    const bool settingsReady{lc::test::ensureSettings()};
    lc::test::QuietLog quiet;
    RS_Graphic graphic;
    RS_Layer* layer = nullptr;
    Drawing() {
        graphic.initForNewDocument(); graphic.onLoadingCompleted(); graphic.setUnit(RS2::Millimeter);
        layer = new RS_Layer("Offsets"); layer->lock(true); graphic.addLayer(layer);
    }
    std::pair<bool, bool> undoState() const {
        bool undo = false, redo = false; graphic.collectUndoState(undo, redo); return {undo, redo};
    }
};

void decorate(RS_Entity& entity, RS_Layer* layer) {
    entity.setLayer(layer); entity.setPen(RS_Pen{RS_Color{29, 53, 79}, RS2::Width11, RS2::DashLine});
    entity.setSourceHandle(1101); entity.setXDictHandle(1102); entity.setReactorHandles({1103});
    entity.setUserDefVar("export-dispatch", "foreign attribute");
    entity.setDrwExtData({std::make_shared<DRW_Variant>(1001, std::string("LibreCad")),
        std::make_shared<DRW_Variant>(1000, std::string("LC_OFFSET_CURVE")),
        std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_EXPORT_DATA")),
        std::make_shared<DRW_Variant>(1000, std::string("foreign export value"))});
}

std::unique_ptr<LC_OffsetCurve> admitted(RS_EntityContainer* parent, const bool linear, const bool reversed = false) {
    LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1, linear ? .25 : 0, linear ? .2501 : 0, false};
    definition.distance = 1; definition.relTolerance = .01;
    std::unique_ptr<RS_Entity> cache;
    if (linear) {
        const auto frame = LC_EllipseOffsetGeometry::ReferenceFrame::create(definition.basis, definition.distance); REQUIRE(frame);
        LC_CurveJet start, end;
        REQUIRE(frame->offsetJet(0, start)); REQUIRE(frame->offsetJet(frame->parameterEnd(), end));
        cache = std::make_unique<RS_Line>(parent, start.point * frame->scale(), end.point * frame->scale());
    } else {
        RS_SplineData data{3, false};
        std::array<RS_Vector, 4> net{{{2, 0}, {2, 9.0/8}, {9.0/8, 2}, {0, 2}}};
        for (unsigned quadrant = 0; quadrant < 4; ++quadrant) {
            for (unsigned i = quadrant ? 1 : 0; i < 4; ++i) data.controlPoints.push_back(net[i]);
            for (auto& point : net) point = {-point.y, point.x};
        }
        for (unsigned i = 0; i <= 4; ++i)
            for (unsigned j = 0; j < (i == 0 || i == 4 ? 4u : 3u); ++j) data.knotslist.push_back(i);
        data.weights.assign(data.controlPoints.size(), 1); data.m_closedFlag = true;
        cache = std::make_unique<RS_Spline>(parent, data);
    }
    LC_OffsetCheckResult checked;
    auto result = LC_OffsetCurve::adopt(*cache, definition, .02, &checked);
    CAPTURE(checked.reason, checked.span); REQUIRE(checked); REQUIRE(result); REQUIRE(result->hasEditableDefinition());
    if (reversed) { result->revertDirection(); REQUIRE(result->hasEditableDefinition()); }
    REQUIRE(result->setOwnedExtensions(0x10, {{1000, QStringLiteral("owned export sentinel")}}));
    return result;
}

LC_OffsetCurve* addNative(Drawing& drawing, const bool linear = false, const bool reversed = false) {
    auto owned = admitted(&drawing.graphic, linear, reversed); decorate(*owned, drawing.layer);
    auto* result = owned.release(); drawing.graphic.addEntity(result); return result;
}

RS_Line* addFrame(Drawing& drawing) {
    auto* result = new RS_Line(&drawing.graphic, RS_Vector{20, 10}, RS_Vector{30, 20});
    result->setLayer(drawing.graphic.getActiveLayer()); drawing.graphic.addEntity(result); return result;
}

struct NativeState {
    const LC_OffsetCurve& source;
    RS_SplineData data;
    LC_EllipseOffsetDefinition definition;
    std::vector<RS_Vector> grips;
    LC_OffsetCurve::OwnedTail tail;
    RS_EntityContainer* parent;
    RS_Layer* layer;
    RS_Pen pen;
    unsigned flags, ownedFlags;
    unsigned long long id, lineage, owner;
    quint32 handle, dictionary;
    std::vector<quint32> reactors;
    std::vector<std::pair<int, std::string>> xdata;
    QString variable;
    const RS_Vector *controlsStorage, *gripsStorage;
    bool defined, consistent;
    double tolerance;
    std::vector<const RS_Entity*> children;
    std::vector<RS_Vector> childEnds;
    explicit NativeState(const LC_OffsetCurve& entity) : source(entity), data(entity.getData()), definition(entity.definition()),
        grips(entity.cachedDefinitionGrips()), tail(entity.unknownOwnedTail()), parent(entity.getParent()), layer(entity.getLayer()),
        pen(entity.getPen(false)), flags(entity.getFlags()), ownedFlags(entity.unknownOwnedFlags()), id(entity.getId()),
        lineage(entity.lineage()), owner(entity.ownerLineage()), handle(entity.sourceHandle()), dictionary(entity.xDictHandle()),
        reactors(entity.reactorHandles()), variable(entity.getUserDefVar("export-dispatch")),
        controlsStorage(entity.getData().controlPoints.data()), gripsStorage(entity.cachedDefinitionGrips().data()),
        defined(entity.isDefined()), consistent(entity.isConsistent()), tolerance(entity.checkedWorldTolerance()) {
        for (const auto& tag : entity.getDrwExtData()) {
            REQUIRE(tag); REQUIRE(tag->type() == DRW_Variant::STRING); xdata.emplace_back(tag->code(), *tag->content.s);
        }
        for (const auto* child : entity) {
            REQUIRE(child); children.push_back(child); childEnds.push_back(child->getStartpoint()); childEnds.push_back(child->getEndpoint());
        }
    }
    void check() const {
        CHECK(RS_Spline::geometryFingerprint(source.getData()) == RS_Spline::geometryFingerprint(data));
        CHECK(source.getData().controlPoints.data() == controlsStorage); CHECK(source.cachedDefinitionGrips().data() == gripsStorage);
        CHECK(source.isDefined() == defined); CHECK(source.isConsistent() == consistent);
        CHECK(source.getParent() == parent); CHECK(source.getLayer() == layer); CHECK(source.getPen(false) == pen); CHECK(source.getFlags() == flags);
        CHECK(source.getId() == id); CHECK(source.lineage() == lineage); CHECK(source.ownerLineage() == owner);
        CHECK(source.sourceHandle() == handle); CHECK(source.xDictHandle() == dictionary); CHECK(source.reactorHandles() == reactors);
        CHECK(source.getUserDefVar("export-dispatch") == variable); CHECK(bits(source.checkedWorldTolerance()) == bits(tolerance));
        const auto& actual = source.definition(); exactPoint(actual.basis.center, definition.basis.center); exactPoint(actual.basis.majorP, definition.basis.majorP);
        CHECK(bits(actual.basis.ratio) == bits(definition.basis.ratio)); CHECK(bits(actual.basis.angle1) == bits(definition.basis.angle1));
        CHECK(bits(actual.basis.angle2) == bits(definition.basis.angle2)); CHECK(actual.basis.reversed == definition.basis.reversed);
        CHECK(bits(actual.distance) == bits(definition.distance)); CHECK(bits(actual.relTolerance) == bits(definition.relTolerance));
        CHECK(actual.towardCentre == definition.towardCentre); CHECK(actual.cacheReversed == definition.cacheReversed);
        REQUIRE(source.cachedDefinitionGrips().size() == grips.size());
        for (std::size_t i = 0; i < grips.size(); ++i) exactPoint(source.cachedDefinitionGrips()[i], grips[i]);
        CHECK(source.unknownOwnedFlags() == ownedFlags); REQUIRE(source.unknownOwnedTail().size() == tail.size());
        for (std::size_t i = 0; i < tail.size(); ++i) {
            CHECK(source.unknownOwnedTail()[i].code == tail[i].code); REQUIRE(tail[i].value.index() == 0);
            REQUIRE(source.unknownOwnedTail()[i].value.index() == 0);
            CHECK(std::get<QString>(source.unknownOwnedTail()[i].value) == std::get<QString>(tail[i].value));
        }
        REQUIRE(source.getDrwExtData().size() == xdata.size());
        for (std::size_t i = 0; i < xdata.size(); ++i) {
            const auto& tag = source.getDrwExtData()[i]; REQUIRE(tag); REQUIRE(tag->type() == DRW_Variant::STRING);
            CHECK(tag->code() == xdata[i].first); CHECK(*tag->content.s == xdata[i].second);
        }
        REQUIRE(source.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) {
            CHECK(source.entityAt(static_cast<int>(i)) == children[i]); CHECK(children[i]->getParent() == &source);
            exactPoint(children[i]->getStartpoint(), childEnds[2*i]); exactPoint(children[i]->getEndpoint(), childEnds[2*i + 1]);
        }
    }
};

struct Element {
    QString layer;
    std::map<QString, QString> attributes;
    QString value(const QString& name, const QString& uri = {}) const {
        const auto found = attributes.find(uri + QLatin1Char('|') + name);
        return found == attributes.end() ? QString{} : found->second;
    }
};
struct Svg {
    std::string bytes;
    std::vector<Element> paths, lines, groups, uses;
};

Svg svg(RS_Graphic& graphic, const bool invisible = true, const bool inlineBlocks = false) {
    LC_MakerCamSVG writer{std::make_unique<LC_XMLWriterQXmlStreamWriter>(), invisible, true, inlineBlocks};
    REQUIRE(writer.generate(&graphic));
    Svg result; result.bytes = writer.resultAsString();
    CHECK(result.bytes.find("LC_OFFSET_CURVE") == std::string::npos);
    CHECK(result.bytes.find("owned export sentinel") == std::string::npos);
    QXmlStreamReader reader{QString::fromStdString(result.bytes)};
    std::vector<QString> layers;
    unsigned roots = 0;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            const auto layer = reader.attributes().value(lcNamespace, QStringLiteral("layername"));
            layers.push_back(layer.isEmpty() ? (layers.empty() ? QString{} : layers.back()) : layer.toString());
            Element element; element.layer = layers.back();
            for (const auto& attribute : reader.attributes())
                element.attributes.emplace(attribute.namespaceUri().toString() + QLatin1Char('|') + attribute.name().toString(), attribute.value().toString());
            if (reader.namespaceUri() == svgNamespace) {
                if (reader.name() == QStringLiteral("svg")) ++roots;
                else if (reader.name() == QStringLiteral("path")) result.paths.push_back(element);
                else if (reader.name() == QStringLiteral("line")) result.lines.push_back(element);
                else if (reader.name() == QStringLiteral("g")) result.groups.push_back(element);
                else if (reader.name() == QStringLiteral("use")) result.uses.push_back(element);
            }
        } else if (reader.isEndElement()) { REQUIRE_FALSE(layers.empty()); layers.pop_back(); }
    }
    REQUIRE_FALSE(reader.hasError()); REQUIRE(roots == 1); REQUIRE(layers.empty());
    return result;
}

struct Command { QChar letter; std::vector<RS_Vector> points; };
std::vector<Command> commands(const Element& path, const RS_Vector& minimum, const RS_Vector& maximum) {
    std::vector<Command> result;
    for (const auto& token : path.value(QStringLiteral("d")).split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        QString coordinates = token;
        if (token.front().isLetter()) { result.push_back({token.front(), {}}); coordinates = token.mid(1); }
        if (coordinates.isEmpty()) continue;
        REQUIRE_FALSE(result.empty());
        const auto xy = coordinates.split(QLatin1Char(',')); REQUIRE(xy.size() == 2);
        bool validX = false, validY = false;
        const auto x = xy[0].toDouble(&validX), y = xy[1].toDouble(&validY);
        REQUIRE(validX); REQUIRE(validY); REQUIRE(std::isfinite(x)); REQUIRE(std::isfinite(y));
        result.back().points.emplace_back(x + minimum.x, maximum.y - y);
    }
    REQUIRE_FALSE(result.empty());
    return result;
}

void pathMatches(const Element& path, const RS_SplineData& data, const RS_Vector& minimum,
                 const RS_Vector& maximum, const RS_Vector& offset = {0, 0}) {
    const auto actual = commands(path, minimum, maximum);
    REQUIRE((data.degree == 1 || data.degree == 3));
    const auto spans = (data.controlPoints.size() - 1) / data.degree;
    REQUIRE(actual.size() == 1 + spans); CHECK(actual[0].letter == QLatin1Char('M')); REQUIRE(actual[0].points.size() == 1);
    CHECK(actual[0].points[0].distanceTo(data.controlPoints[0] + offset) < 2e-8);
    for (std::size_t i = 0; i < spans; ++i) {
        CAPTURE(i); const auto& command = actual[i + 1];
        CHECK(command.letter == (data.degree == 1 ? QLatin1Char('L') : QLatin1Char('C')));
        REQUIRE(command.points.size() == data.degree);
        for (std::size_t j = 0; j < data.degree; ++j)
            CHECK(command.points[j].distanceTo(data.controlPoints[data.degree*i + j + 1] + offset) < 2e-8);
    }
}

Element layerGroup(const Svg& document, const QString& name) {
    for (const auto& group : document.groups) if (group.value(QStringLiteral("layername"), lcNamespace) == name) return group;
    FAIL("Expected exported layer group");
    return {};
}

QByteArray bytes(const QString& path) {
    QFile file{path}; REQUIRE(file.open(QIODevice::ReadOnly)); return file.readAll();
}
} // namespace

TEST_CASE("native MakerCAM SVG exports stored line and cubic cache commands in both directions",
          "[lc-offsetcurve][types][export-dispatch]") {
    for (const bool linear : {false, true}) for (const bool reversed : {false, true}) {
        CAPTURE(linear, reversed); Drawing drawing; auto* curve = addNative(drawing, linear, reversed); addFrame(drawing);
        const NativeState before{*curve}; const auto undo = drawing.undoState(); const auto count = drawing.graphic.count();
        const auto actual = svg(drawing.graphic); REQUIRE(actual.paths.size() == 1); REQUIRE(actual.lines.size() == 1);
        CHECK(actual.paths[0].layer == QStringLiteral("Offsets"));
        pathMatches(actual.paths[0], curve->getData(), drawing.graphic.getMin(), drawing.graphic.getMax());
        before.check(); CHECK(drawing.undoState() == undo); CHECK(drawing.graphic.count() == count);
    }
}

TEST_CASE("native MakerCAM SVG matches ordinary cache geometry and existing layer attribute policy",
          "[lc-offsetcurve][types][export-dispatch]") {
    Drawing drawing, plain; auto* curve = addNative(drawing); addFrame(drawing); const NativeState before{*curve};
    auto cache = curve->toPlainSpline(); cache->reparent(&plain.graphic); cache->setLayer(plain.layer); plain.graphic.addEntity(cache.release()); addFrame(plain);
    const auto actual = svg(drawing.graphic), expected = svg(plain.graphic);
    REQUIRE(actual.paths.size() == 1); REQUIRE(expected.paths.size() == 1); REQUIRE(actual.lines.size() == 1); REQUIRE(expected.lines.size() == 1);
    CHECK(actual.paths[0].attributes == expected.paths[0].attributes); CHECK(actual.lines[0].attributes == expected.lines[0].attributes);
    const auto& layer = layerGroup(actual, QStringLiteral("Offsets"));
    CHECK(layer.attributes == layerGroup(expected, QStringLiteral("Offsets")).attributes);
    CHECK(layer.value(QStringLiteral("is_locked"), lcNamespace) == QStringLiteral("true"));
    CHECK(layer.value(QStringLiteral("fill")) == QStringLiteral("none")); CHECK(layer.value(QStringLiteral("stroke")) == QStringLiteral("black"));
    before.check();
}

TEST_CASE("native MakerCAM SVG uses transformed undefined cache rather than the old offset definition",
          "[lc-offsetcurve][types][export-dispatch]") {
    Drawing drawing; auto* curve = addNative(drawing); curve->scale({0, 0}, {2, 3}); REQUIRE_FALSE(curve->isDefined());
    const NativeState before{*curve}; const auto actual = svg(drawing.graphic); REQUIRE(actual.paths.size() == 1);
    pathMatches(actual.paths[0], curve->getData(), drawing.graphic.getMin(), drawing.graphic.getMax()); before.check();
}

TEST_CASE("native MakerCAM SVG exports a finite inconsistent cache without restamping its definition",
          "[lc-offsetcurve][types][export-dispatch]") {
    Drawing drawing; auto* curve = addNative(drawing); curve->getData().controlPoints[1].x += .125; curve->update(); REQUIRE_FALSE(curve->isConsistent());
    const NativeState before{*curve}; const auto actual = svg(drawing.graphic); REQUIRE(actual.paths.size() == 1);
    pathMatches(actual.paths[0], curve->getData(), drawing.graphic.getMin(), drawing.graphic.getMax()); before.check();
}

TEST_CASE("native MakerCAM SVG keeps deletion and frozen-layer dispatch identical to ordinary entities",
          "[lc-offsetcurve][types][export-dispatch]") {
    Drawing drawing; auto* curve = addNative(drawing); addFrame(drawing); curve->setFlag(RS2::FlagDeleted);
    { const NativeState before{*curve}; const auto actual = svg(drawing.graphic); CHECK(actual.paths.empty()); CHECK(actual.lines.size() == 1); before.check(); }
    curve->delFlag(RS2::FlagDeleted); drawing.layer->freeze(true);
    const NativeState before{*curve};
    CHECK(svg(drawing.graphic, false).paths.empty());
    const auto included = svg(drawing.graphic, true); REQUIRE(included.paths.size() == 1);
    CHECK(layerGroup(included, QStringLiteral("Offsets")).value(QStringLiteral("style")) == QStringLiteral("display: none;")); before.check();
}

TEST_CASE("native MakerCAM SVG writes ordinary geometry inside inline and referenced blocks",
          "[lc-offsetcurve][types][export-dispatch]") {
    Drawing drawing;
    auto* block = new RS_Block(&drawing.graphic, RS_BlockData("Offset block", {0, 0}, false)); drawing.graphic.addBlock(block);
    auto owned = admitted(block, false); decorate(*owned, drawing.layer); auto* curve = owned.release(); block->addEntity(curve);
    auto* insert = new RS_Insert(&drawing.graphic, RS_InsertData("Offset block", {4, 8}, {1, 1}, 0, 1, 1, {0, 0}));
    insert->setLayer(drawing.graphic.getActiveLayer()); drawing.graphic.addEntity(insert); insert->update(); REQUIRE(insert->count() == 1);
    const NativeState before{*curve};
    for (const bool inlineBlocks : {false, true}) {
        const auto actual = svg(drawing.graphic, true, inlineBlocks); REQUIRE(actual.paths.size() == 1);
        pathMatches(actual.paths[0], curve->getData(), drawing.graphic.getMin(), drawing.graphic.getMax(), inlineBlocks ? RS_Vector{4, 8} : RS_Vector{0, 0});
        CHECK(actual.uses.size() == (inlineBlocks ? 0u : 1u));
        if (!inlineBlocks) {
            CHECK(actual.uses[0].value(QStringLiteral("href"), xlinkNamespace) == QStringLiteral("#") + QString::number(block->getId()));
            CHECK(actual.uses[0].value(QStringLiteral("x")) == QStringLiteral("4"));
            CHECK(actual.uses[0].value(QStringLiteral("y")) == QStringLiteral("-8"));
        }
        before.check();
    }
}

TEST_CASE("native JWW export refuses the actual unavailable writer and preserves absent or existing targets",
          "[lc-offsetcurve][types][export-dispatch]") {
    Drawing drawing; auto* curve = addNative(drawing); addFrame(drawing); const NativeState before{*curve}; const auto undo = drawing.undoState();
    const QTemporaryDir directory; REQUIRE(directory.isValid());
    const QByteArray original = QByteArrayLiteral("original JWW target\0with binary suffix");
    for (const auto format : {RS2::FormatJWW, RS2::FormatJWC}) for (const bool exists : {false, true}) {
        const auto file = directory.filePath(QString::number(static_cast<int>(format)) + (exists ? QStringLiteral("-old.jww") : QStringLiteral("-absent.jww")));
        if (exists) { QFile target{file}; REQUIRE(target.open(QIODevice::WriteOnly)); REQUIRE(target.write(original) == original.size()); }
        const auto names = QDir(directory.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
        RS_FilterJWW filter; CHECK_FALSE(filter.fileExport(drawing.graphic, file, format));
        if (exists) CHECK(bytes(file) == original); else CHECK_FALSE(QFileInfo::exists(file));
        CHECK(QDir(directory.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot) == names);
        before.check(); CHECK(drawing.graphic.count() == 2); CHECK(drawing.undoState() == undo);
    }
}

TEST_CASE("ordinary line JWW export remains an honest unsupported-writer refusal",
          "[lc-offsetcurve][types][export-dispatch]") {
    Drawing drawing; auto* line = addFrame(drawing); const auto start = line->getStartpoint(), end = line->getEndpoint();
    const auto flags = line->getFlags(); const auto undo = drawing.undoState(); const QTemporaryDir directory; REQUIRE(directory.isValid());
    const auto file = directory.filePath("line.jww"); RS_FilterJWW filter;
    CHECK_FALSE(filter.fileExport(drawing.graphic, file, RS2::FormatJWW)); CHECK_FALSE(QFileInfo::exists(file));
    CHECK(line->getParent() == &drawing.graphic); CHECK(line->getFlags() == flags); exactPoint(line->getStartpoint(), start); exactPoint(line->getEndpoint(), end);
    CHECK(drawing.graphic.count() == 1); CHECK(drawing.undoState() == undo);
}

TEST_CASE("legacy DXF native dispatch writes visible cache primitives when the real legacy exporter is supplied",
          "[lc-offsetcurve][types][export-dispatch]") {
#ifdef LC_TEST_LEGACY_DXF_EXPORT
    for (const auto format : {RS2::FormatDXFOLD, RS2::FormatDXFOLD12}) for (const bool linear : {false, true}) {
        Drawing drawing; auto* curve = addNative(drawing, linear); const NativeState before{*curve};
        auto* sibling = addFrame(drawing); decorate(*sibling, drawing.layer);
        const QTemporaryDir directory; REQUIRE(directory.isValid()); const auto file = directory.filePath("cache.dxf");
        RS_FilterDXF filter; REQUIRE(filter.fileExport(drawing.graphic, file, format));
        const auto path = std::filesystem::path(file.toStdString()); const auto lines = lc::test::dxfRecords(path, "LINE");
        REQUIRE(lines.size() == curve->count() + 1); CHECK(lc::test::dxfRecords(path, "SPLINE").empty());
        CHECK(lc::test::fileBytes(path).find("LC_OFFSET_CURVE") == std::string::npos);
        const auto coordinate = [](const lc::test::DxfRecord& record, const int code, const double expected) {
            const auto values = record.own(code); REQUIRE(values.size() == 1);
            bool parsed = false; const auto actual = QString::fromStdString(values[0]).toDouble(&parsed);
            REQUIRE(parsed); REQUIRE(std::isfinite(actual));
            CHECK(std::abs(actual - expected) <= 1e-9 * std::max(1.0, std::abs(expected)));
        };
        std::size_t i = 0;
        for (const auto* child : *curve) {
            const auto* line = dynamic_cast<const RS_Line*>(child); REQUIRE(line);
            const auto& record = lines[i++]; const auto a = line->getStartpoint(), b = line->getEndpoint();
            coordinate(record, 10, a.x); coordinate(record, 20, a.y); coordinate(record, 11, b.x); coordinate(record, 21, b.y);
            CHECK(record.own(8) == std::vector<std::string>{"Offsets"});
            for (const auto code : {6, 62, 370}) CHECK(record.own(code) == lines.back().own(code));
        }
        bool versionSeen = false;
        for (const auto& record : lc::test::dxfRecords(path, "SECTION"))
            for (std::size_t j = 1; j < record.m_groups.size(); ++j)
                if (record.m_groups[j - 1].m_code == 9 && record.m_groups[j - 1].m_value == "$ACADVER") {
                    CHECK(record.m_groups[j].m_value == (format == RS2::FormatDXFOLD12 ? "AC1009" : "AC1015")); versionSeen = true;
                }
        CHECK(versionSeen); before.check();
    }
#else
    SKIP("Legacy RS_FilterDXF is not registered; dl_dxf.h and FormatDXFOLD enums are absent. No surrogate writer is used.");
#endif
}
