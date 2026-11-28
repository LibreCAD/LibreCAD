/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <limits>
#include <type_traits>

#include "lc_offsetcurve_persistence_fixture.h"
#include "lc_copyutils.h"
#include "lc_dwgadvancedmetadata.h"
#include "rs_block.h"
#include "rs_clipboard.h"
#include "rs_hatch.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_selection.h"
#include "rs_undo.h"
#include "rs_variable.h"

namespace {
namespace f = lc::test::offset_persistence;

struct Setup {
    const bool ready{lc::test::ensureSettings()};
    lc::test::QuietLog quiet;
};

void vectorState(std::ostream& out, const RS_Vector& p) {
    out << p.valid << ':' << f::bits(p.x) << ',' << f::bits(p.y) << ',' << f::bits(p.z) << ';';
}
void cacheState(std::ostream& out, const RS_SplineData& data) {
    out << data.degree << ':' << static_cast<int>(data.type) << ':' << data.m_closedFlag << data.m_periodicFlag << ';';
    out << static_cast<int>(data.savedOpenType) << ':';
    for (double knot : data.savedOpenKnots) out << f::bits(knot) << ';';
    for (const auto& p : data.controlPoints) vectorState(out, p);
    out << '|'; for (const auto& p : data.fitPoints) vectorState(out, p);
    out << '|'; for (double knot : data.knotslist) out << f::bits(knot) << ';';
    out << '|'; for (double weight : data.weights) out << f::bits(weight) << ';';
}
void nativeState(std::ostream& out, const LC_OffsetCurve& curve) {
    const auto& d = curve.definition();
    vectorState(out, d.basis.center); vectorState(out, d.basis.majorP);
    for (double value : {d.basis.ratio, d.basis.angle1, d.basis.angle2, d.distance, d.relTolerance}) out << f::bits(value) << ';';
    out << d.basis.reversed << d.basis.isArc << d.towardCentre << d.cacheReversed
        << curve.isDefined() << curve.isConsistent() << ';';
    out << curve.unknownOwnedFlags() << ':' << f::bits(curve.checkedWorldTolerance()) << ';';
    for (const auto& item : curve.unknownOwnedTail()) {
        out << item.code << ':' << item.value.index() << ':';
        std::visit([&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, QString>) { for (const auto unit : value) out << unit.unicode() << ','; }
            else if constexpr (std::is_same_v<T, RS_Vector>) vectorState(out, value);
            else if constexpr (std::is_same_v<T, double>) out << f::bits(value);
            else out << value;
        }, item.value);
        out << ';';
    }
    for (const auto& grip : curve.cachedDefinitionGrips()) vectorState(out, grip);
}

std::string metadataState(const LC_DwgAdvancedMetadata& metadata) {
    std::ostringstream out;
    out << static_cast<int>(metadata.sourceDwgVersion()) << ';' << metadata.appIdTableEntries().size() << ';';
    for (const auto& [name, value] : metadata.appIdTableEntries()) {
        out << std::quoted(name) << ':' << std::quoted(value.name) << ':' << value.tType << ':' << value.handle << ':' << value.parentHandle << ':'
            << value.flags << ':' << value.reactorCount() << ':' << unsigned(value.extensionDictionaryFlag())
            << ':' << value.hasDwgCommonLinkTail() << ':' << value.hasDataStorageBinaryData() << ':' << value.xDictHandle;
        for (auto handle : value.reactorHandles) out << ',' << handle;
        f::Tags tags;
        for (const auto* tag : value.extData) tags.push_back(tag ? std::make_shared<DRW_Variant>(*tag) : nullptr);
        out << ':' << f::tagsState(tags);
        for (const auto& group : value.appData) {
            tags.clear();
            for (const auto& tag : group) tags.push_back(std::make_shared<DRW_Variant>(tag));
            out << ':' << f::tagsState(tags);
        }
        for (const auto* refs : {&value.pendingAppIdResolutions, &value.pendingLayerRefResolutions}) {
            out << "refs=" << refs->size() << ';';
            for (const auto& ref : *refs)
                out << ':' << ref.indexInExtData << ',' << ref.handleRef << ',' << ref.rawHandleRef << ',' << ref.useRawHandleRef;
        }
        out << ';';
    }
    const auto rawDxf = [&out](const auto& objects) {
        for (const auto& object : objects) {
            out << std::quoted(object.name) << ':' << object.handle << ':' << object.parentHandle << ':'
                << static_cast<int>(object.m_version) << ':' << object.hasRawValues << ':';
            f::Tags tags;
            for (const auto& group : object.groups) tags.push_back(std::make_shared<DRW_Variant>(group));
            out << f::tagsState(tags);
            for (const auto& text : object.rawValues) out << std::quoted(text);
            out << ';';
        }
    };
    out << "objects=" << metadata.rawDxfObjects().size() << ';'; rawDxf(metadata.rawDxfObjects());
    out << "entities=" << metadata.rawDxfEntities().size() << ';'; rawDxf(metadata.rawDxfEntities());
    out << "raw=" << metadata.rawObjects().size() << ';';
    for (const auto& object : metadata.rawObjects()) {
        out << object.version << ':' << object.objectType << ':' << object.handle << ':' << object.parentHandle
            << ':' << object.commonHandleDataValidated << ':' << static_cast<int>(object.commonLinkEvidence)
            << ':' << object.xDictHandle << ':' << object.numReactors << ':' << unsigned(object.xDictFlag)
            << ':' << object.blockOwnerHandle << ':' << object.bodyBitSize << ':' << object.objectOffset
            << ':' << object.objectSize << ':' << object.isEntity << ':' << object.isCustomClass
            << ':' << static_cast<int>(object.family) << ':' << std::quoted(object.recordName)
            << ':' << std::quoted(object.className) << ':' << object.typedPayloadValidated
            << ':' << static_cast<int>(object.replayState) << ':' << object.hasDataStorage
            << ':' << object.hasClassDefinition << ':' << object.classProxyFlag << ':' << std::quoted(object.classAppName)
            << ':' << object.classWasProxy << ':' << object.classEntityFlagRaw << ':' << object.classDwgVersion
            << ':' << object.classMaintenanceVersion << ':' << object.classUnknown1 << ':' << object.classUnknown2;
        for (auto handle : object.reactorHandles) out << ',' << handle;
        out << '|'; for (auto byte : object.rawBytes) out << unsigned(byte) << ',';
        out << ';';
    }
    return out.str();
}

void variableState(std::ostream& out, const QHash<QString, RS_Variable>& variables) {
    auto keys = variables.keys();
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys) {
        const auto& value = variables[key];
        out << std::quoted(key.toStdString()) << ':' << value.getCode() << ':' << value.getType() << ':';
        switch (value.getType()) {
        case RS2::VariableString: out << std::quoted(value.getString().toStdString()); break;
        case RS2::VariableDouble: out << f::bits(value.getDouble()); break;
        case RS2::VariableInt: out << value.getInt(); break;
        case RS2::VariableVector: vectorState(out, value.getVector()); break;
        default: break;
        }
        out << ';';
    }
}

struct Snapshot {
    RS_Graphic& graphic;
    std::string before;
    std::vector<RS_Pen> pens;
    RS_Pen activePen;
    RS_Layer* activeLayer = nullptr;
    QHash<QString, RS_Variable> variables, customProperties;
    std::string advanced;
    explicit Snapshot(RS_Graphic& graphic) : graphic(graphic), before(state()) {
        for (auto* e : graphic) { REQUIRE(e); pens.push_back(e->getPen(false)); }
        activePen = graphic.getActivePen(); activeLayer = graphic.getActiveLayer();
        for (auto it = graphic.getVariableDict().cbegin(); it != graphic.getVariableDict().cend(); ++it)
            variables.insert(it.key(), it.value());
        for (auto it = graphic.getCustomProperties().cbegin(); it != graphic.getCustomProperties().cend(); ++it)
            customProperties.insert(it.key(), it.value());
        advanced = metadataState(graphic.dwgAdvancedMetadata());
    }
    std::string state() const {
        std::ostringstream out;
        bool undo = false, redo = false; graphic.collectUndoState(undo, redo);
        out << graphic.count() << graphic.isModified() << undo << redo << ';';
        out << static_cast<const RS_Undo&>(graphic);
        out << static_cast<int>(graphic.getFormatType()) << ':' << std::quoted(graphic.getFilename().toStdString()) << ';';
        out << graphic.getVariableDictObjectRef()->isModified() << ';';
        variableState(out, graphic.getVariableDict());
        variableState(out, graphic.getCustomProperties());
        for (auto* entity : graphic) {
            REQUIRE(entity);
            out << entity << ':' << entity->getParent() << ':' << entity->getLayer() << ':' << entity->rtti()
                << ':' << entity->getId() << ':' << entity->getFlags() << ':';
            out << entity->isDeleted() << entity->isSelected() << entity->isVisible() << ':';
            out << entity->sourceHandle() << ':' << entity->xDictHandle() << ':' << entity->lineage() << ':' << entity->ownerLineage() << ':';
            for (auto handle : entity->reactorHandles()) out << handle << ',';
            out << f::tagsState(entity->getDrwExtData()) << ':';
            for (const auto& tag : entity->getDrwExtData()) out << tag.get() << ',';
            vectorState(out, entity->getStartpoint());
            vectorState(out, entity->getEndpoint());
            if (const auto* spline = dynamic_cast<const RS_Spline*>(entity)) {
                cacheState(out, spline->getData());
                REQUIRE(spline->count() <= 65536);
                for (const auto* child : *spline) {
                    REQUIRE(child); out << child << ':' << child->getParent() << ':';
                    vectorState(out, child->getStartpoint()); vectorState(out, child->getEndpoint());
                }
            }
            if (const auto* curve = dynamic_cast<const LC_OffsetCurve*>(entity)) nativeState(out, *curve);
            out << '\n';
        }
        return out.str();
    }
    void check() const {
        CHECK(state() == before);
        CHECK(graphic.getVariableDict() == variables);
        CHECK(graphic.getCustomProperties() == customProperties);
        CHECK(metadataState(graphic.dwgAdvancedMetadata()) == advanced);
        CHECK(graphic.getActivePen() == activePen); CHECK(graphic.getActiveLayer() == activeLayer);
        std::size_t index = 0;
        for (const auto* e : graphic) { REQUIRE(index < pens.size()); CHECK(e->getPen(false) == pens[index++]); }
        CHECK(index == pens.size());
    }
};

void load(RS_Graphic& graphic, const std::filesystem::path& path, const DRW_Spline& raw, bool helix = false) {
    f::put(path, f::ascii(raw, helix));
    REQUIRE(lc::test::importDrawing(graphic, path, RS2::FormatDXFRW2018));
}

void ordinary(const std::string& name, const DRW_Spline& raw, bool keepMarker = false, bool helix = false) {
    Setup setup;
    lc::test::OutputDirectory directory("offset-persistence-" + name);
    RS_Graphic graphic;
    load(graphic, directory.file("input.dxf"), raw, helix);
    f::fallback(graphic, keepMarker);
    if (keepMarker) CHECK(f::tagsState(f::sole(graphic)->getDrwExtData()) == f::tagsState(raw.extData));
    Snapshot before(graphic);
    const auto output = directory.file("ordinary.dxf");
    REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW2018));
    before.check();
    const auto captured = f::readCodec(output);
    CHECK(captured.ellipses == 0);
    if (keepMarker) {
        REQUIRE(captured.splines.size() == 1);
        CHECK(f::tagsState(captured.splines.front().extData) == f::tagsState(raw.extData));
    }
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    f::fallback(reopened, keepMarker);
    if (keepMarker) CHECK(f::tagsState(f::sole(reopened)->getDrwExtData()) == f::tagsState(raw.extData));
}

void checkPayload(const DRW_Spline& spline, unsigned expectedFlags = 0x40, bool tail = true) {
    std::string app;
    for (const auto& tag : spline.extData)
        if (tag && tag->code() == 1001 && tag->type() == DRW_Variant::STRING &&
            QString::fromStdString(*tag->content.s).compare("LibreCad", Qt::CaseInsensitive) == 0) {
            app = *tag->content.s; break;
        }
    REQUIRE_FALSE(app.empty());
    auto tags = f::application(spline.extData, app);
    REQUIRE_FALSE(tags.empty());
    auto start = std::find_if(tags.begin(), tags.end(), [](const auto& tag) {
        return tag->code() == 1000 && tag->type() == DRW_Variant::STRING && *tag->content.s == "LC_OFFSET_CURVE";
    });
    REQUIRE(start != tags.end());
    const auto expected = f::owned(expectedFlags, tail);
    REQUIRE(static_cast<std::size_t>(tags.end() - start) >= expected.size());
    const f::Tags actual(start, start + static_cast<std::ptrdiff_t>(expected.size()));
    CHECK(f::tagsState(actual) == f::tagsState(expected));
    CHECK(f::markerCount(spline.extData) == 1);
    CHECK(f::tagsState(f::application(spline.extData, "FOREIGN_A")) == f::tagsState(f::foreignBefore()));
    CHECK(f::tagsState(f::application(spline.extData, "FOREIGN_B")) == f::tagsState(f::foreignAfter()));
}
}

TEST_CASE("G hand-authored degree-one ASCII cache becomes editable without a source ellipse", "[lc-offsetcurve][g-fixtures][dxf]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-linear");
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(1));
    auto* curve = f::native(graphic, 1);
    CHECK(curve->getLayer()->getName() == "FixtureOffsets");
    CHECK(curve->getPen(false).getWidth() == RS2::Width07);
    CHECK(curve->unknownOwnedFlags() == 0x40);
    REQUIRE(curve->unknownOwnedTail().size() == 5);
    CHECK(std::get<QString>(curve->unknownOwnedTail().front().value) == "safe owned tail");
    f::foreign(*curve);
}

TEST_CASE("G hand-authored degree-three polynomial carrier is admitted bit-exactly", "[lc-offsetcurve][g-fixtures][dxf]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-cubic");
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(3));
    f::foreign(*f::native(graphic, 3));
}

TEST_CASE("G explicit complete unit weights and zero normal sentinel remain eligible", "[lc-offsetcurve][g-fixtures][dxf]") {
    Setup setup; const int degree = GENERATE(1, 3);
    lc::test::OutputDirectory directory("offset-persistence-units");
    auto raw = f::raw(degree); raw.normalVec = {0, 0, 0}; raw.weightlist.assign(raw.controllist.size(), 1.);
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), raw);
    f::native(graphic, degree);
}

TEST_CASE("G valid orientation physical-side and reserved flags survive actual file round trips", "[lc-offsetcurve][g-fixtures][flags]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-persistence-positive-flags");
    struct Lane { DRW::Version version; RS2::FormatType format; bool binary; bool dwg; };
    std::vector<Lane> lanes{{DRW::AC1032, RS2::FormatDXFRW2018, false, false},
        {DRW::AC1014, RS2::FormatDXFRW14, true, false},
        {DRW::AC1032, RS2::FormatDXFRW2018, true, false}};
#ifdef DWGSUPPORT
    lanes.push_back({DRW::AC1015, RS2::FormatDWG, false, true});
    lanes.push_back({DRW::AC1032, RS2::FormatDWG2018, false, true});
#endif
    for (const int degree : {1, 3}) for (unsigned profile = 0; profile < 8; ++profile) {
        const bool reversed = (profile & 1) != 0, inward = (profile & 2) != 0;
        const bool cacheReversed = (profile & 4) != 0;
        const unsigned flags = 0x7ff0 | profile | ((inward != reversed) ? 8u : 0u);
        auto payload = f::owned(flags);
        if (reversed) std::swap(payload[7], payload[8]);
        const double distance = inward ? .25 : 1.;
        payload[10] = std::make_shared<DRW_Variant>(1041, distance);
        auto raw = f::raw(degree, f::metadata(payload));
        if (inward) for (auto& p : raw.controllist) {
            p->x = 8 + (p->x - 8)*.375;
            p->y = 16 + (p->y - 16)*.375;
        }
        if (reversed != cacheReversed) std::reverse(raw.controllist.begin(), raw.controllist.end());
        const auto check = [&](const LC_OffsetCurve& curve) {
            REQUIRE(curve.hasEditableDefinition());
            CHECK(curve.definition().basis.reversed == reversed);
            CHECK(curve.definition().towardCentre == inward);
            CHECK(curve.definition().cacheReversed == cacheReversed);
            CHECK(curve.definition().basis.angle1 == (reversed ? .015625 : 0));
            CHECK(curve.definition().basis.angle2 == (reversed ? 0 : .015625));
            CHECK(curve.definition().distance == distance);
            CHECK(curve.unknownOwnedFlags() == 0x7ff0);
            REQUIRE(curve.getData().controlPoints.size() == raw.controllist.size());
            for (std::size_t i = 0; i < raw.controllist.size(); ++i) {
                CHECK(f::bits(curve.getData().controlPoints[i].x) == f::bits(raw.controllist[i]->x));
                CHECK(f::bits(curve.getData().controlPoints[i].y) == f::bits(raw.controllist[i]->y));
            }
        };
        for (std::size_t laneIndex = 0; laneIndex < lanes.size(); ++laneIndex) {
            CAPTURE(degree, profile, laneIndex);
            const auto& lane = lanes[laneIndex];
            const auto stem = std::to_string(degree) + "_" + std::to_string(profile) + "_" + std::to_string(laneIndex);
            const auto input = directory.file(stem + (lane.dwg ? ".dwg" : ".dxf"));
#ifdef DWGSUPPORT
            if (lane.dwg) f::dwgFile(input, raw, lane.version);
            else
#endif
            if (lane.binary) f::binaryDxf(input, raw, lane.version);
            else f::put(input, f::ascii(raw));
            const auto decoded = f::readCodec(input, lane.dwg); REQUIRE(decoded.splines.size() == 1);
            CHECK(f::tagsState(decoded.splines.front().extData) == f::tagsState(raw.extData));
            RS_Graphic graphic;
            REQUIRE(lc::test::importDrawing(graphic, input, lane.format));
            auto* curve = dynamic_cast<LC_OffsetCurve*>(f::sole(graphic)); REQUIRE(curve); check(*curve);
            f::foreign(*curve); Snapshot before(graphic);
            const auto output = directory.file(stem + (lane.dwg ? "_copy.dwg" : "_copy.dxf"));
            REQUIRE(lc::test::exportDrawing(graphic, output, lane.format)); before.check();
            const auto wire = f::readCodec(output, lane.dwg); REQUIRE(wire.splines.size() == 1);
            auto expected = f::application(f::metadata({}), "LibreCad"); f::append(expected, payload);
            CHECK(f::tagsState(f::application(wire.splines.front().extData, "LibreCad")) == f::tagsState(expected));
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, output, lane.format));
            const auto* copy = dynamic_cast<LC_OffsetCurve*>(f::sole(reopened)); REQUIRE(copy); check(*copy);
            f::foreign(*copy); before.check();
        }
    }
}

TEST_CASE("G malformed unknown subtree stays raw and cannot publish an unsafe save", "[lc-offsetcurve][g-fixtures][unknown-schema][save-refusal]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-persistence-malformed-unknown");
    auto unknown = f::owned(0, false, 2);
    unknown.pop_back();
    f::append(unknown, f::owned());
    const auto raw = f::raw(3, f::metadata(unknown));
    for (const bool binary : {false, true}) {
        CAPTURE(binary);
        const auto input = directory.file(binary ? "binary.dxf" : "ascii.dxf");
        if (binary) f::binaryDxf(input, raw, DRW::AC1032);
        else f::put(input, f::ascii(raw));
        const auto sourceBytes = lc::test::fileBytes(input);
        const auto decoded = f::readCodec(input); REQUIRE(decoded.splines.size() == 1);
        CHECK(f::tagsState(decoded.splines.front().extData) == f::tagsState(raw.extData));
        RS_Graphic graphic;
        REQUIRE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW2018));
        auto* entity = f::sole(graphic); REQUIRE(entity);
        CHECK(entity->rtti() == RS2::EntitySpline); CHECK(entity->isVisible());
        CHECK(entity->getLength() > 0); CHECK(f::markerCount(entity->getDrwExtData()) == 2);
        CHECK(f::tagsState(entity->getDrwExtData()) == f::tagsState(raw.extData));
        f::foreign(*entity); Snapshot before(graphic);
        std::vector<RS2::FormatType> formats{RS2::FormatDXFRW12, RS2::FormatDXFRW2018};
#ifdef DWGSUPPORT
        formats.push_back(RS2::FormatDWG2018);
#endif
        for (std::size_t i = 0; i < formats.size(); ++i) for (const bool existing : {false, true}) {
            CAPTURE(i, existing);
            const auto target = directory.file(std::to_string(binary) + "_" + std::to_string(i) + (existing ? "_old" : "_new"));
            if (existing) f::put(target, "unknown metadata target sentinel");
            const auto entries = directory.entries();
            CHECK_FALSE(lc::test::exportDrawing(graphic, target, formats[i]));
            CHECK(directory.entries() == entries); before.check();
            if (existing) CHECK(lc::test::fileBytes(target) == "unknown metadata target sentinel");
            else CHECK_FALSE(std::filesystem::exists(target));
            CHECK(lc::test::fileBytes(input) == sourceBytes);
        }
    }
}

TEST_CASE("G no-edit save preserves typed schema tail flags attributes and shared source tags", "[lc-offsetcurve][g-fixtures][dxf]") {
    Setup setup; const int degree = GENERATE(1, 3);
    lc::test::OutputDirectory directory("offset-persistence-no-edit");
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(degree));
    auto* curve = f::native(graphic, degree); Snapshot before(graphic);
    const auto output = directory.file("saved.dxf");
    REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW2018));
    before.check();
    const auto captured = f::readCodec(output);
    REQUIRE(captured.splines.size() == 1); CHECK(captured.ellipses == 0);
    checkPayload(captured.splines.front());
    CHECK(captured.splines.front().layer == "FixtureOffsets");
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    auto* again = f::native(reopened, degree);
    CHECK(again->unknownOwnedFlags() == curve->unknownOwnedFlags());
    CHECK(again->unknownOwnedTail().size() == curve->unknownOwnedTail().size());
}

TEST_CASE("G meaningful real offset edit clears only owned extras before second save", "[lc-offsetcurve][g-fixtures][dxf][edit]") {
    Setup setup; const int degree = GENERATE(1, 3);
    lc::test::OutputDirectory directory("offset-persistence-edited");
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(degree));
    auto* curve = f::native(graphic, degree);
    const auto result = curve->setDistance(1.25);
    REQUIRE(result); CHECK(result.engineRuns == 1);
    CHECK(curve->unknownOwnedFlags() == 0); CHECK(curve->unknownOwnedTail().empty()); f::foreign(*curve);
    Snapshot before(graphic);
    const auto output = directory.file("edited.dxf");
    REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW2018)); before.check();
    const auto captured = f::readCodec(output); REQUIRE(captured.splines.size() == 1);
    const auto& tags = captured.splines.front().extData;
    CHECK(f::markerCount(tags) == 1);
    CHECK(f::tagsState(tags).find("safe owned tail") == std::string::npos);
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    auto* again = dynamic_cast<LC_OffsetCurve*>(f::sole(reopened));
    REQUIRE(again); REQUIRE(again->hasEditableDefinition()); CHECK(again->definition().distance == 1.25);
    CHECK(again->unknownOwnedFlags() == 0); CHECK(again->unknownOwnedTail().empty()); f::foreign(*again);
}

TEST_CASE("G no-op and bounded refused edit retain imported owned extras", "[lc-offsetcurve][g-fixtures][dxf][edit]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-refused-edit");
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(3));
    auto* curve = f::native(graphic, 3); Snapshot before(graphic);
    const auto noop = curve->setDistance(curve->definition().distance);
    REQUIRE(noop); CHECK(noop.engineRuns == 0); before.check();
    LC_OffsetRunLimits limits; limits.maxSamples = 1;
    const auto refused = curve->setDistance(1.25, limits);
    CHECK_FALSE(refused); before.check();
    REQUIRE(lc::test::exportDrawing(graphic, directory.file("still-original.dxf"), RS2::FormatDXFRW2018)); before.check();
    auto captured = f::readCodec(directory.file("still-original.dxf")); REQUIRE(captured.splines.size() == 1);
    checkPayload(captured.splines.front());
}

TEST_CASE("G duplicate known schema candidates never choose a convenient valid definition", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto payload = f::owned(); f::append(payload, f::owned());
    ordinary("duplicate", f::raw(3, f::metadata(payload)));
}
TEST_CASE("G owned fields in the wrong order become visible ordinary geometry", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto payload = f::owned(); std::swap(payload[6], payload[10]);
    ordinary("order", f::raw(1, f::metadata(payload)));
}
TEST_CASE("G wrong typed distance code is not accepted as schema one", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto payload = f::owned(); payload[10] = std::make_shared<DRW_Variant>(1040, 1.);
    ordinary("type", f::raw(3, f::metadata(payload)));
}
TEST_CASE("G conflicting physical-side flag cannot promote an ordinary cache", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    ordinary("flag-conflict", f::raw(1, f::metadata(f::owned(0x48))));
}
TEST_CASE("G invalid schema ranges preserve only foreign and unrelated data", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    const int field = GENERATE(6, 9, 10, 11);
    auto payload = f::owned();
    if (field == 9) payload[field] = std::make_shared<DRW_Variant>(1070, -1);
    else payload[field] = std::make_shared<DRW_Variant>(field == 10 ? 1041 : 1040, 0.);
    ordinary("range", f::raw(3, f::metadata(payload)));
}
TEST_CASE("G obsolete axis-endpoint schema is not reinterpreted as a displacement", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto payload = f::owned(); payload[5] = std::make_shared<DRW_Variant>(1011, DRW_Coord(9, 16, 0));
    ordinary("old-axis", f::raw(1, f::metadata(payload)));
}
TEST_CASE("G raw nonzero control Z refuses native admission before projection", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto raw = f::raw(3); raw.controllist[1] = std::make_shared<DRW_Coord>(*raw.controllist[1]); raw.controllist[1]->z = .25;
    ordinary("raw-z", raw);
}
TEST_CASE("G tilted explicit normal refuses native admission without hiding geometry", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto raw = f::raw(1); raw.normalVec = {1, 0, 1}; ordinary("normal", raw);
}
TEST_CASE("G nonunit rational weights remain ordinary and never certify repaired controls", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto raw = f::raw(3); raw.flags |= 4; raw.weightlist = {1, 2, 2, 1}; ordinary("weighted", raw);
}
TEST_CASE("G partial weight list is ordinary fallback not implied all-unit authority", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto raw = f::raw(3); raw.weightlist = {1}; ordinary("partial-weights", raw);
}
TEST_CASE("G degree-two converter fallback cannot inherit editable definition", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    ordinary("degree-two", f::raw(2));
}
TEST_CASE("G high-degree approximation remains visible but noneditable", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    ordinary("degree-four", f::raw(4));
}
TEST_CASE("G fit data alongside controls cannot become an editable cache", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto raw = f::raw(1); raw.fitlist = {std::make_shared<DRW_Coord>(10, 16, 0), std::make_shared<DRW_Coord>(9.999755859375, 16.03125, 0)};
    raw.nfit = 2; ordinary("fit-mode", raw);
}
TEST_CASE("G HELIX wire identity is never promoted despite valid-looking schema", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    ordinary("helix", f::raw(3), false, true);
}
TEST_CASE("G missing wire control is a real codec refusal not a fabricated null callback", "[lc-offsetcurve][g-fixtures][dxf][codec-refusal]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-missing-control");
    auto raw = f::raw(1); raw.controllist.back().reset();
    const auto input = directory.file("missing.dxf"); f::put(input, f::ascii(raw));
    RS_Graphic graphic; CHECK_FALSE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW2018));
    for (auto* e : graphic) if (e) CHECK(e->rtti() != RS2::EntityOffsetCurve);
}
TEST_CASE("G missing declared knot is rejected by actual file validation", "[lc-offsetcurve][g-fixtures][dxf][codec-refusal]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-missing-knot");
    auto raw = f::raw(3); raw.knotslist.pop_back();
    const auto input = directory.file("missing.dxf"); f::put(input, f::ascii(raw));
    RS_Graphic graphic; CHECK_FALSE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW2018));
    for (auto* e : graphic) if (e) CHECK(e->rtti() != RS2::EntityOffsetCurve);
}
TEST_CASE("G noncanonical knots are ordinary fallback rather than a normalized proof", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto raw = f::raw(3); raw.knotslist[3] = .125; ordinary("noncanonical-knots", raw);
}
TEST_CASE("G unknown schema remains raw ordinary metadata across no-edit save", "[lc-offsetcurve][g-fixtures][dxf][unknown-schema]") {
    ordinary("schema-two", f::raw(1, f::metadata(f::owned(0x40, true, 2))), true);
}
TEST_CASE("G missing known close strips to its APPID boundary without consuming foreign data", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto payload = f::owned(); payload.pop_back(); ordinary("missing-close", f::raw(3, f::metadata(payload)));
}
TEST_CASE("G unsupported owned item is dropped while the complete safe tail survives", "[lc-offsetcurve][g-fixtures][dxf][tail]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-tail-drop");
    auto payload = f::owned(); payload.insert(payload.end() - 1, std::make_shared<DRW_Variant>(1005, std::string("AB")));
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(1, f::metadata(payload)));
    auto* curve = f::native(graphic, 1); CHECK(curve->unknownOwnedFlags() == 0x40);
    const auto& tail = curve->unknownOwnedTail();
    REQUIRE(tail.size() == 5);
    CHECK(tail[0].code == 1000); CHECK(std::get<QString>(tail[0].value) == "safe owned tail");
    CHECK(tail[1].code == 1071); CHECK(std::get<std::int32_t>(tail[1].value) == 123456);
    CHECK(tail[2].code == 1002); CHECK(std::get<QString>(tail[2].value) == "{");
    CHECK(tail[3].code == 1000); CHECK(std::get<QString>(tail[3].value) == "nested safe item");
    CHECK(tail[4].code == 1002); CHECK(std::get<QString>(tail[4].value) == "}");
    f::foreign(*curve);
    Snapshot before(graphic);
    const auto output = directory.file("saved.dxf");
    REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW2018));
    before.check();
    const auto wire = f::readCodec(output); REQUIRE(wire.splines.size() == 1);
    checkPayload(wire.splines.front());
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    CHECK(f::native(reopened, 1)->unknownOwnedTail().size() == 5);
}

TEST_CASE("G binary R14 and R2018 reader fixtures reach actual native admission", "[lc-offsetcurve][g-fixtures][binary-dxf]") {
    Setup setup; const int degree = GENERATE(1, 3); const bool r14 = GENERATE(false, true);
    lc::test::OutputDirectory directory("offset-persistence-binary-positive");
    const auto input = directory.file("literal-binary.dxf");
    const auto raw = f::raw(degree); const auto originalTags = f::tagsState(raw.extData);
    f::binaryDxf(input, raw, r14 ? DRW::AC1014 : DRW::AC1032);
    CHECK(f::tagsState(raw.extData) == originalTags);
    const auto captured = f::readCodec(input); REQUIRE(captured.splines.size() == 1);
    checkPayload(captured.splines.front());
    REQUIRE(captured.splines.front().controllist.size() == raw.controllist.size());
    for (std::size_t i = 0; i < raw.controllist.size(); ++i) {
        CHECK(f::bits(captured.splines.front().controllist[i]->x) == f::bits(raw.controllist[i]->x));
        CHECK(f::bits(captured.splines.front().controllist[i]->y) == f::bits(raw.controllist[i]->y));
    }
    RS_Graphic graphic;
    REQUIRE(lc::test::importDrawing(graphic, input, r14 ? RS2::FormatDXFRW14 : RS2::FormatDXFRW2018));
    f::foreign(*f::native(graphic, degree));
}

TEST_CASE("G binary hostile raw Z and duplicate schema cannot acquire editability", "[lc-offsetcurve][g-fixtures][binary-dxf][hostile]") {
    Setup setup; const bool duplicate = GENERATE(false, true); const int degree = GENERATE(1, 3);
    lc::test::OutputDirectory directory("offset-persistence-binary-hostile");
    auto raw = f::raw(degree);
    if (duplicate) { auto payload = f::owned(); f::append(payload, f::owned()); raw.extData = f::metadata(payload); }
    else raw.controllist.front()->z = .125;
    const auto input = directory.file("hostile-binary.dxf"); f::binaryDxf(input, raw, DRW::AC1032);
    const auto wire = f::readCodec(input); REQUIRE(wire.splines.size() == 1);
    if (duplicate) CHECK(f::markerCount(wire.splines.front().extData) == 2);
    else CHECK(wire.splines.front().controllist.front()->z == .125);
    RS_Graphic graphic; REQUIRE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW2018));
    f::fallback(graphic);
}

#ifdef DWGSUPPORT
TEST_CASE("G legacy DWG source codepage admits and preserves the exact EED tail budget", "[lc-offsetcurve][g-fixtures][dwg][eed][codepage][boundary]") {
    Setup setup;
    lc::test::OutputDirectory directory("offset-persistence-legacy-codepage");
    std::string text;
    for (unsigned i = 0; i < 252; ++i) text += "\xD1\x8F";
    auto payload = f::owned(0x40, false);
    f::Tags tail;
    for (unsigned i = 0; i < 16; ++i) tail.push_back(std::make_shared<DRW_Variant>(1000, text));
    payload.insert(payload.end() - 1, tail.begin(), tail.end());
    for (const auto version : {DRW::AC1015, DRW::AC1018}) {
        CAPTURE(static_cast<int>(version));
        const auto input = directory.file(std::to_string(static_cast<int>(version)) + ".dwg");
        f::dwgFile(input, f::raw(1, f::metadata(payload)), version, "ANSI_1251");
        f::Capture captured;
        dwgRW reader(input.string().c_str());
        REQUIRE(reader.read(&captured, false));
        REQUIRE(reader.getCodePage() == "ANSI_1251");
        RS_Graphic graphic;
        REQUIRE(lc::test::importDrawing(graphic, input, RS2::FormatDWG));
        auto* curve = f::native(graphic, 1);
        REQUIRE(curve->unknownOwnedTail().size() == 16);
        for (const auto& item : curve->unknownOwnedTail())
            CHECK(std::get<QString>(item.value) == QString(252, QChar(0x044F)));
        CHECK(graphic.getVariableString("$DWGCODEPAGE", "") == "ANSI_1251");
        Snapshot before(graphic);
        for (const auto format : {RS2::FormatDWG, RS2::FormatDXFRW2000}) {
            const auto output = directory.file(std::to_string(static_cast<int>(version)) +
                (format == RS2::FormatDWG ? "_copy.dwg" : "_copy.dxf"));
            REQUIRE(lc::test::exportDrawing(graphic, output, format));
            before.check();
            RS_Graphic reopened;
            REQUIRE(lc::test::importDrawing(reopened, output, format));
            const auto* copy = f::native(reopened, 1);
            REQUIRE(copy->unknownOwnedTail().size() == 16);
            for (const auto& item : copy->unknownOwnedTail())
                CHECK(std::get<QString>(item.value) == QString(252, QChar(0x044F)));
        }
    }
}

TEST_CASE("G actual R2000 and R2018 DWG EED fixtures admit both exact carrier degrees", "[lc-offsetcurve][g-fixtures][dwg][eed]") {
    Setup setup; const int degree = GENERATE(1, 3); const bool legacy = GENERATE(false, true);
    lc::test::OutputDirectory directory("offset-persistence-eed-positive");
    const auto input = directory.file("literal.dwg"); const auto raw = f::raw(degree);
    const auto originalTags = f::tagsState(raw.extData);
    f::dwgFile(input, raw, legacy ? DRW::AC1015 : DRW::AC1032);
    CHECK(f::tagsState(raw.extData) == originalTags);
    const auto wire = f::readCodec(input, true); REQUIRE(wire.splines.size() == 1);
    checkPayload(wire.splines.front());
    RS_Graphic graphic;
    REQUIRE(lc::test::importDrawing(graphic, input, legacy ? RS2::FormatDWG : RS2::FormatDWG2018));
    auto* curve = f::native(graphic, degree); f::foreign(*curve); Snapshot before(graphic);
    const auto output = directory.file("application-saved.dwg");
    REQUIRE(lc::test::exportDrawing(graphic, output, legacy ? RS2::FormatDWG : RS2::FormatDWG2018)); before.check();
    const auto saved = f::readCodec(output, true); REQUIRE(saved.splines.size() == 1); checkPayload(saved.splines.front());
    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, output, legacy ? RS2::FormatDWG : RS2::FormatDWG2018));
    f::native(reopened, degree);
}

TEST_CASE("G actual hostile DWG EED duplicate or weighted carriers stay ordinary", "[lc-offsetcurve][g-fixtures][dwg][eed][hostile]") {
    Setup setup; const bool weighted = GENERATE(false, true); const int degree = GENERATE(1, 3);
    lc::test::OutputDirectory directory("offset-persistence-eed-hostile");
    auto raw = f::raw(degree);
    if (weighted) { raw.flags |= 4; raw.weightlist.assign(raw.controllist.size(), 1.); raw.weightlist[1] = 2.; }
    else { auto payload = f::owned(); f::append(payload, f::owned()); raw.extData = f::metadata(payload); }
    const auto input = directory.file("hostile.dwg"); f::dwgFile(input, raw, DRW::AC1032);
    const auto wire = f::readCodec(input, true); REQUIRE(wire.splines.size() == 1);
    if (weighted) { REQUIRE(wire.splines.front().weightlist.size() > 1); CHECK(wire.splines.front().weightlist[1] == 2.); }
    else CHECK(f::markerCount(wire.splines.front().extData) == 2);
    RS_Graphic graphic; REQUIRE(lc::test::importDrawing(graphic, input, RS2::FormatDWG2018)); f::fallback(graphic);
    Snapshot before(graphic);
    const auto output = directory.file("ordinary.dwg");
    REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDWG2018)); before.check();
    const auto saved = f::readCodec(output, true); REQUIRE(saved.splines.size() == 1);
    CHECK(f::markerCount(saved.splines.front().extData) == 0);
}
#endif

TEST_CASE("G mixed-case owned APPID retains first spelling without a duplicate registration", "[lc-offsetcurve][g-fixtures][dxf][appid]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-appid-case");
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(1, f::metadata(f::owned(), "lIbReCaD")));
    f::native(graphic, 1);
    const auto output = directory.file("saved.dxf"); REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW2018));
    const auto wire = f::readCodec(output); REQUIRE(wire.splines.size() == 1); checkPayload(wire.splines.front());
    unsigned count = 0;
    for (const auto& record : wire.apps)
        if (QString::fromStdString(record.name).compare("LibreCad", Qt::CaseInsensitive) == 0) ++count;
    CHECK(count == 1);
    CHECK(std::count_if(wire.apps.begin(), wire.apps.end(), [](const auto& record) {
        return record.name == " LibreCad ";
    }) == 1);
    bool firstSpelling = false;
    for (const auto& tag : wire.splines.front().extData)
        if (tag->code() == 1001 && tag->type() == DRW_Variant::STRING && *tag->content.s == "lIbReCaD") firstSpelling = true;
    CHECK(firstSpelling);
}

TEST_CASE("G whitespace APPID is foreign not an owned namespace", "[lc-offsetcurve][g-fixtures][dxf][appid]") {
    ordinary("appid-whitespace", f::raw(3, f::metadata(f::owned(), " LibreCad ")), true);
}

TEST_CASE("G one valid schema can coexist with opaque unknown schema without nested promotion", "[lc-offsetcurve][g-fixtures][dxf][unknown-schema]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-unknown-coexist");
    auto unknown = f::owned(0, false, 2);
    auto nested = f::owned(0, false); unknown.insert(unknown.end() - 1, nested.begin(), nested.end());
    auto payload = unknown;
    f::append(payload, f::owned());
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(3, f::metadata(payload)));
    auto* curve = f::native(graphic, 3); CHECK(f::markerCount(curve->getDrwExtData()) == 2); f::foreign(*curve);
    const auto remaining = f::metadata(unknown);
    CHECK(f::tagsState(curve->getDrwExtData()) == f::tagsState(remaining));
    const auto expectedWire = [&remaining](const f::Tags& current) {
        auto expected = remaining;
        const auto where = std::find_if(expected.begin(), expected.end(), [](const auto& item) {
            return item->code() == 1001 && *item->content.s == "FOREIGN_B";
        });
        REQUIRE(where != expected.end());
        expected.insert(where, current.begin(), current.end());
        return expected;
    };
    Snapshot before(graphic);
    const auto output = directory.file("saved.dxf"); REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW2018)); before.check();
    const auto wire = f::readCodec(output); REQUIRE(wire.splines.size() == 1); CHECK(f::markerCount(wire.splines.front().extData) == 3);
    CHECK(f::tagsState(wire.splines.front().extData) == f::tagsState(expectedWire(f::owned())));
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    auto* copy = f::native(reopened, 3);
    CHECK(f::tagsState(copy->getDrwExtData()) == f::tagsState(remaining));
    const auto edited = copy->setDistance(1.25);
    REQUIRE(edited); CHECK(edited.changed); CHECK(edited.engineRuns == 1);
    CHECK(f::tagsState(copy->getDrwExtData()) == f::tagsState(remaining));
    Snapshot editedState(reopened);
    const auto editedFile = directory.file("edited.dxf");
    REQUIRE(lc::test::exportDrawing(reopened, editedFile, RS2::FormatDXFRW2018));
    editedState.check();
    auto current = f::owned(0, false);
    current[10] = std::make_shared<DRW_Variant>(1041, 1.25);
    const auto editedWire = f::readCodec(editedFile); REQUIRE(editedWire.splines.size() == 1);
    CHECK(f::tagsState(editedWire.splines.front().extData) == f::tagsState(expectedWire(current)));
    RS_Graphic finalCopy;
    REQUIRE(lc::test::importDrawing(finalCopy, editedFile, RS2::FormatDXFRW2018));
    auto* finalNative = dynamic_cast<LC_OffsetCurve*>(f::sole(finalCopy));
    REQUIRE(finalNative); REQUIRE(finalNative->hasEditableDefinition());
    CHECK(finalNative->getDegree() == copy->getDegree());
    CHECK(finalNative->definition().distance == 1.25);
    CHECK(f::tagsState(finalNative->getDrwExtData()) == f::tagsState(remaining));
}

TEST_CASE("G closure repair dependency cannot borrow a native certificate", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    auto raw = f::raw(3); raw.flags |= 1; ordinary("closure-repair", raw);
}

TEST_CASE("G nonfinite raw normal is rejected by the codec not reported as adapter fallback", "[lc-offsetcurve][g-fixtures][dxf][codec-refusal]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-nonfinite-normal");
    auto raw = f::raw(1); raw.normalVec.x = std::numeric_limits<double>::infinity();
    const auto input = directory.file("normal.dxf"); f::put(input, f::ascii(raw));
    RS_Graphic graphic; CHECK_FALSE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW2018));
    for (auto* e : graphic) if (e) CHECK(e->rtti() != RS2::EntityOffsetCurve);
}

TEST_CASE("G clone persists its own admitted definition without mutating the source", "[lc-offsetcurve][g-fixtures][dxf][clone]") {
    Setup setup; const int degree = GENERATE(1, 3);
    lc::test::OutputDirectory directory("offset-persistence-clone");
    RS_Graphic source; load(source, directory.file("input.dxf"), f::raw(degree));
    auto* curve = f::native(source, degree); Snapshot before(source);
    RS_Graphic destination; destination.initForNewDocument();
    std::unique_ptr<RS_Entity> copy(curve->clone());
    REQUIRE(copy); CHECK(copy.get() != curve); copy->reparent(&destination); copy->setLayer(destination.getActiveLayer());
    destination.addEntity(copy.release()); f::native(destination, degree); before.check();
    const auto output = directory.file("cloned.dxf"); REQUIRE(lc::test::exportDrawing(destination, output, RS2::FormatDXFRW2018));
    before.check();
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    auto* again = f::native(reopened, degree); CHECK(again->unknownOwnedFlags() == 0x40); f::foreign(*again);
}

TEST_CASE("G clipboard identity paste keeps native while rounded similarity paste is plain on disk", "[lc-offsetcurve][g-fixtures][dxf][clipboard]") {
    Setup setup; const int degree = GENERATE(1, 3); const bool rounded = GENERATE(false, true);
    lc::test::OutputDirectory directory("offset-persistence-paste");
    RS_Graphic source; load(source, directory.file("input.dxf"), f::raw(degree));
    auto* curve = f::native(source, degree); curve->setSelectionFlag(true); Snapshot before(source);
    struct ClipboardScope { ClipboardScope() { RS_CLIPBOARD->clear(); } ~ClipboardScope() { RS_CLIPBOARD->clear(); } } clipboard;
    const RS_Vector reference = rounded ? RS_Vector(.1, .2) : RS_Vector(0, 0);
    QList<RS_Entity*> selected{curve};
    CHECK(LC_CopyUtils::copy(reference, selected, &source) == selected); before.check();
    lc::test::EditableDrawing destination;
    destination.m_graphic.initForNewDocument(); destination.m_graphic.setUnit(source.getUnit());
    const LC_CopyUtils::RS_PasteData paste = rounded
        ? LC_CopyUtils::RS_PasteData({4, 8}, 1.5, .3) : LC_CopyUtils::RS_PasteData({0, 0});
    destination.modify([&](LC_DocumentModificationBatch& batch) {
        LC_CopyUtils::paste(paste, &destination.m_graphic, batch);
    });
    auto* pasted = f::sole(destination.m_graphic);
    CHECK(pasted->getParent() == &destination.m_graphic);
    if (rounded) CHECK(pasted->rtti() == RS2::EntitySpline);
    else f::native(destination.m_graphic, degree);
    auto expected = curve->toPlainSpline(); expected->move(-reference);
    expected->scale({0, 0}, {paste.factor, paste.factor}); expected->rotate({0, 0}, paste.angle); expected->move(paste.insertionPoint);
    auto* actual = dynamic_cast<RS_Spline*>(pasted); REQUIRE(actual);
    const auto& a = actual->getData().controlPoints; const auto& b = expected->getData().controlPoints;
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) { CHECK(a[i].x == b[i].x); CHECK(a[i].y == b[i].y); }
    before.check();
    const auto output = directory.file("pasted.dxf");
    REQUIRE(lc::test::exportDrawing(destination.m_graphic, output, RS2::FormatDXFRW2018)); before.check();
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    if (rounded) { CHECK(f::sole(reopened)->rtti() == RS2::EntitySpline); CHECK(f::markerCount(f::sole(reopened)->getDrwExtData()) == 0); }
    else f::native(reopened, degree);
    REQUIRE(destination.m_graphic.undo()); CHECK(f::live(destination.m_graphic).empty());
    REQUIRE(destination.m_graphic.redo()); CHECK(f::live(destination.m_graphic).size() == 1);
}

TEST_CASE("G block definition persists native but uniform and nonuniform INSERT expansions stay plain", "[lc-offsetcurve][g-fixtures][dxf][block][insert]") {
    Setup setup; const int degree = GENERATE(1, 3); const bool uniform = GENERATE(false, true);
    lc::test::OutputDirectory directory("offset-persistence-block");
    RS_Graphic source; load(source, directory.file("input.dxf"), f::raw(degree));
    auto* original = f::native(source, degree); Snapshot sourceBefore(source);
    RS_Graphic destination; destination.initForNewDocument();
    auto* block = new RS_Block(&destination, RS_BlockData("OffsetBlock", {0, 0}, false));
    REQUIRE(destination.addBlock(block));
    auto* child = original->clone(); child->reparent(block); child->setLayer(destination.getActiveLayer()); block->addEntity(child);
    auto* definition = f::native(*block, degree);
    std::ostringstream originalDefinition; nativeState(originalDefinition, *definition); cacheState(originalDefinition, definition->getData());
    const RS_Vector scale{2, uniform ? 2. : 3.};
    auto* insert = new RS_Insert(&destination, RS_InsertData("OffsetBlock", {4, 8}, scale, 0, 1, 1, {0, 0}));
    insert->setLayer(destination.getActiveLayer()); destination.addEntity(insert); insert->update();
    auto checkExpansion = [&](RS_Insert& instance) {
        auto* expanded = dynamic_cast<RS_Spline*>(f::sole(instance)); REQUIRE(expanded);
        CHECK(expanded->rtti() == RS2::EntitySpline); CHECK(expanded->getParent() == &instance);
        const auto expected = f::controls(degree); REQUIRE(expanded->getData().controlPoints.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            CHECK(expanded->getData().controlPoints[i].x == expected[i].x*scale.x + 4);
            CHECK(expanded->getData().controlPoints[i].y == expected[i].y*scale.y + 8);
        }
    };
    checkExpansion(*insert); sourceBefore.check();
    const auto output = directory.file("block.dxf"); REQUIRE(lc::test::exportDrawing(destination, output, RS2::FormatDXFRW2018));
    sourceBefore.check();
    std::ostringstream afterDefinition; nativeState(afterDefinition, *definition); cacheState(afterDefinition, definition->getData());
    CHECK(afterDefinition.str() == originalDefinition.str());
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    auto* loadedInsert = dynamic_cast<RS_Insert*>(f::sole(reopened)); REQUIRE(loadedInsert);
    auto* loadedBlock = reopened.findBlock("OffsetBlock"); REQUIRE(loadedBlock);
    f::foreign(*f::native(*loadedBlock, degree)); checkExpansion(*loadedInsert);
    const auto wire = f::readCodec(output); CHECK(wire.ellipses == 0);
}

TEST_CASE("G hatch serializes native boundary as an ordinary spline edge without owned schema", "[lc-offsetcurve][g-fixtures][dxf][hatch]") {
    Setup setup; const int degree = GENERATE(1, 3);
    lc::test::OutputDirectory directory("offset-persistence-hatch");
    RS_Graphic source; load(source, directory.file("input.dxf"), f::raw(degree));
    auto* original = f::native(source, degree); Snapshot before(source);
    RS_Graphic destination; destination.initForNewDocument();
    auto hatch = std::make_unique<RS_Hatch>(&destination, RS_HatchData(true, 1., 0., "SOLID"));
    auto loop = std::make_unique<RS_EntityContainer>(hatch.get());
    auto* edge = original->clone(); edge->reparent(loop.get()); edge->setLayer(destination.getActiveLayer()); loop->addEntity(edge);
    const auto end = original->getData().controlPoints.back(), start = original->getData().controlPoints.front();
    loop->addEntity(new RS_Line(loop.get(), end, RS_Vector(9, 18)));
    loop->addEntity(new RS_Line(loop.get(), RS_Vector(9, 18), start));
    hatch->addEntity(loop.release()); REQUIRE(hatch->validate()); hatch->setLayer(destination.getActiveLayer());
    destination.addEntity(hatch.release()); before.check();
    const auto output = directory.file("hatch.dxf"); REQUIRE(lc::test::exportDrawing(destination, output, RS2::FormatDXFRW2018)); before.check();
    const auto wire = f::readCodec(output); REQUIRE(wire.hatches.size() == 1); CHECK(wire.splines.empty()); CHECK(wire.ellipses == 0);
    unsigned splineEdges = 0;
    for (const auto& boundary : wire.hatches.front().looplist) {
        REQUIRE(boundary);
        for (const auto& segment : boundary->objlist) {
            REQUIRE(segment); CHECK(f::markerCount(segment->extData) == 0);
            if (segment->eType == DRW::SPLINE) { ++splineEdges; CHECK(segment->extData.empty()); }
        }
    }
    CHECK(splineEdges == 1);
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    CHECK(f::sole(reopened)->rtti() == RS2::EntityHatch);
}

namespace {
enum class SaveDefect { NullTag, ForeignBrace, InvalidText, TooFewControls, NonfiniteControl, HugeDegree, TargetTail, R12Knots };
void failedSave(const std::string& name, SaveDefect defect) {
    Setup setup;
    for (bool existing : {false, true}) {
        CAPTURE(name, existing);
        lc::test::OutputDirectory directory("offset-persistence-refused-" + name);
        lc::test::EditableDrawing drawing;
        auto& graphic = drawing.m_graphic;
        load(graphic, directory.file("input.dxf"), f::raw(3));
        auto* curve = f::native(graphic, 3);
        graphic.addVariable("$USERI1", 73, 70);
        graphic.addVariable("$USERR1", 2.5, 40);
        graphic.addVariable("$INSBASE", RS_Vector(5, 7, 0), 10);
        graphic.addVariable("$PROJECTNAME", "failure-state sentinel", 1);
        graphic.addCustomProperty("Offset fixture", "preserved custom metadata");
        DRW_AppId app; app.name = "PERSIST_META_KEEP"; app.handle = 0x720;
        graphic.dwgAdvancedMetadata().addAppIdTableEntry(app);
        DRW_RawDxfObject raw;
        raw.name = "XRECORD"; raw.handle = 0x721; raw.m_version = DRW::AC1032;
        raw.groups = {DRW_Variant(5, std::string("721")),
            DRW_Variant(100, std::string("AcDbXrecord")), DRW_Variant(280, std::int32_t{1}),
            DRW_Variant(1, std::string("preserved raw object"))};
        graphic.dwgAdvancedMetadata().addRawDxfObject(raw);
        std::array<RS_Line*, 2> history;
        for (unsigned i = 0; i < history.size(); ++i) {
            auto* line = new RS_Line(&graphic, RS_Vector(20, i), RS_Vector(21, i));
            line->setLayer(curve->getLayer());
            drawing.modify([&](LC_DocumentModificationBatch& batch) { batch.add(line); });
            history[i] = line;
        }
        REQUIRE(graphic.undo());
        REQUIRE_FALSE(history[0]->isDeleted()); REQUIRE(history[1]->isDeleted());
        const std::array<unsigned long long, 2> historyIds{history[0]->getId(), history[1]->getId()};
        std::ostringstream originalHistory;
        originalHistory << static_cast<const RS_Undo&>(graphic);
        const auto format = defect == SaveDefect::R12Knots ? RS2::FormatDXFRW12 : RS2::FormatDXFRW2018;
        Snapshot validSource(graphic);
        REQUIRE(lc::test::exportDrawing(graphic, directory.file("control.dxf"), format));
        validSource.check();
        const auto inputBytes = lc::test::fileBytes(directory.file("input.dxf"));
        const auto controlBytes = lc::test::fileBytes(directory.file("control.dxf"));
        auto tags = curve->getDrwExtData();
        switch (defect) {
        case SaveDefect::NullTag: tags.push_back(nullptr); curve->setDrwExtData(tags); break;
        case SaveDefect::ForeignBrace:
            tags.push_back(std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_UNBALANCED")));
            tags.push_back(std::make_shared<DRW_Variant>(1002, std::string("{"))); curve->setDrwExtData(tags); break;
        case SaveDefect::InvalidText:
            tags.push_back(std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_INVALID_UTF8")));
            tags.push_back(std::make_shared<DRW_Variant>(1000, std::string("\xc0\xaf", 2))); curve->setDrwExtData(tags); break;
        case SaveDefect::TooFewControls: curve->getData().controlPoints.resize(1); break;
        case SaveDefect::NonfiniteControl: curve->getData().controlPoints.front().x = std::numeric_limits<double>::quiet_NaN(); break;
        case SaveDefect::HugeDegree: curve->getData().degree = std::numeric_limits<std::size_t>::max(); break;
        case SaveDefect::TargetTail:
            REQUIRE(curve->setOwnedExtensions(0x40, {{1000, QString(300, QChar('x'))}})); break;
        case SaveDefect::R12Knots: curve->getData().knotslist.clear(); break;
        }
        Snapshot before(graphic);
        const auto target = directory.file("target.dxf");
        const std::string sentinel("preexisting target\0not replaced", 30);
        if (existing) f::put(target, sentinel);
        const auto entries = directory.entries();
        RS_FilterDXFRW filter;
        CHECK_FALSE(filter.fileExport(graphic, lc::test::toQString(target), format));
        before.check(); CHECK(directory.entries() == entries);
        if (existing) CHECK(lc::test::fileBytes(target) == sentinel);
        else CHECK_FALSE(std::filesystem::exists(target));
        CHECK(lc::test::fileBytes(directory.file("input.dxf")) == inputBytes);
        CHECK(lc::test::fileBytes(directory.file("control.dxf")) == controlBytes);
        RS_Graphic retry; retry.initForNewDocument(); retry.addEntity(new RS_Line(&retry, RS_Vector(0, 0), RS_Vector(1, 1)));
        REQUIRE(filter.fileExport(retry, lc::test::toQString(directory.file("retry.dxf")), RS2::FormatDXFRW2018));
        before.check();
        CHECK(lc::test::fileBytes(directory.file("input.dxf")) == inputBytes);
        CHECK(lc::test::fileBytes(directory.file("control.dxf")) == controlBytes);
        // Exercise the retained history only after read-only state checks;
        // real undo/redo may rebuild a native cache's display children.
        REQUIRE(graphic.redo()); CHECK_FALSE(history[1]->isDeleted());
        REQUIRE(graphic.undo()); CHECK(history[1]->isDeleted());
        REQUIRE(graphic.undo()); CHECK(history[0]->isDeleted());
        REQUIRE(graphic.redo()); CHECK_FALSE(history[0]->isDeleted()); CHECK(history[1]->isDeleted());
        CHECK(history[0]->getId() == historyIds[0]); CHECK(history[1]->getId() == historyIds[1]);
        std::ostringstream finalHistory;
        finalHistory << static_cast<const RS_Undo&>(graphic);
        CHECK(finalHistory.str() == originalHistory.str());
    }
}
}

TEST_CASE("G null shared metadata makes save fail without target publication", "[lc-offsetcurve][g-fixtures][dxf][save-refusal]") {
    failedSave("null-tag", SaveDefect::NullTag);
}
TEST_CASE("G unsafe foreign brace merge never overwrites an existing or absent target", "[lc-offsetcurve][g-fixtures][dxf][save-refusal]") {
    failedSave("foreign-brace", SaveDefect::ForeignBrace);
}
TEST_CASE("G invalid shared UTF8 is a real export refusal with unchanged source bytes", "[lc-offsetcurve][g-fixtures][dxf][save-refusal]") {
    failedSave("invalid-text", SaveDefect::InvalidText);
}
TEST_CASE("G too few controls cannot become a successful silently empty save", "[lc-offsetcurve][g-fixtures][dxf][save-refusal]") {
    failedSave("few-controls", SaveDefect::TooFewControls);
}
TEST_CASE("G nonfinite controls fail publication while preserving the original shared graph", "[lc-offsetcurve][g-fixtures][dxf][save-refusal]") {
    failedSave("nan-control", SaveDefect::NonfiniteControl);
}
TEST_CASE("G oversized degree is refused without allocating an oversized fixture", "[lc-offsetcurve][g-fixtures][dxf][save-refusal]") {
    failedSave("huge-degree", SaveDefect::HugeDegree);
}
TEST_CASE("G target-unrepresentable native tail cannot be silently omitted from modern DXF", "[lc-offsetcurve][g-fixtures][dxf][save-refusal]") {
    failedSave("target-tail", SaveDefect::TargetTail);
}
TEST_CASE("G invalid stored knots refuse R12 before approximation", "[lc-offsetcurve][g-fixtures][dxf][save-refusal][r12]") {
    failedSave("r12-knots", SaveDefect::R12Knots);
}

TEST_CASE("G R12 emits independently checked plain polyline geometry and preserves the native source", "[lc-offsetcurve][g-fixtures][dxf][r12]") {
    Setup setup; const int degree = GENERATE(1, 3);
    lc::test::OutputDirectory directory("offset-persistence-r12");
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(degree));
    f::native(graphic, degree); Snapshot before(graphic);
    const auto output = directory.file("r12.dxf"); REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW12)); before.check();
    CHECK(lc::test::dxfRecords(output, "SPLINE").empty()); CHECK(lc::test::dxfRecords(output, "ELLIPSE").empty());
    const auto records = lc::test::dxfRecords(output, "POLYLINE"); REQUIRE(records.size() == 1);
    for (const auto& value : records.front().own(1000)) CHECK(value != "LC_OFFSET_CURVE");
    const auto vertices = lc::test::dxfRecords(output, "VERTEX"); REQUIRE(vertices.size() >= 2);
    const auto expected = f::controls(1); const double dx = expected[1].x - expected[0].x, dy = expected[1].y - expected[0].y;
    for (const auto& vertex : vertices) {
        REQUIRE(vertex.own(10).size() == 1); REQUIRE(vertex.own(20).size() == 1);
        const double x = std::stod(vertex.own(10).front()), y = std::stod(vertex.own(20).front());
        CHECK(std::isfinite(x)); CHECK(std::isfinite(y));
        CHECK(std::abs((x - expected[0].x)*dy - (y - expected[0].y)*dx) <= 1e-12);
        CHECK(y >= expected[0].y - 1e-12); CHECK(y <= expected[1].y + 1e-12);
    }
    CHECK(std::stod(vertices.front().own(10).front()) == Catch::Approx(expected.front().x).margin(1e-12));
    CHECK(std::stod(vertices.back().own(20).front()) == Catch::Approx(expected.back().y).margin(1e-12));
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW12));
    CHECK(f::sole(reopened)->rtti() != RS2::EntityOffsetCurve);
}

TEST_CASE("G exact ASCII tail budget admits 4096 bytes and drops 4097 bytes whole", "[lc-offsetcurve][g-fixtures][dxf][tail][boundary]") {
    Setup setup; const bool over = GENERATE(false, true);
    lc::test::OutputDirectory directory("offset-persistence-tail-boundary");
    auto payload = f::owned(0x40, false);
    f::Tags tail;
    for (unsigned i = 0; i < 15; ++i) tail.push_back(std::make_shared<DRW_Variant>(1000, std::string(255, 'x')));
    tail.push_back(std::make_shared<DRW_Variant>(1000, std::string(over ? 224 : 223, 'y')));
    payload.insert(payload.end() - 1, tail.begin(), tail.end());
    // Typed ASCII tail charge: 15*(3+255) + 3+223 == 4096, no braces truncated.
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), f::raw(3, f::metadata(payload)));
    auto* curve = f::native(graphic, 3); f::foreign(*curve);
    CHECK(curve->unknownOwnedTail().size() == (over ? 0u : 16u));
    if (!over) {
        REQUIRE(std::holds_alternative<QString>(curve->unknownOwnedTail().back().value));
        CHECK(std::get<QString>(curve->unknownOwnedTail().back().value).size() == 223);
    }
    Snapshot before(graphic); const auto output = directory.file("saved.dxf");
    REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW2018)); before.check();
    const auto wire = f::readCodec(output); REQUIRE(wire.splines.size() == 1);
    auto actual = f::application(wire.splines.front().extData, "LibreCad");
    const auto textItems = static_cast<unsigned>(std::count_if(actual.begin(), actual.end(), [](const auto& tag) {
        return tag->code() == 1000 && tag->type() == DRW_Variant::STRING &&
            tag->content.s->size() == 255 && tag->content.s->front() == 'x';
    }));
    CHECK(textItems == (over ? 0u : 15u));
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018));
    CHECK(f::native(reopened, 3)->unknownOwnedTail().size() == (over ? 0u : 16u));
}

TEST_CASE("G schema centre and axis raw Z are rejected without discarding carrier geometry", "[lc-offsetcurve][g-fixtures][dxf][hostile]") {
    const int field = GENERATE(4, 5);
    auto payload = f::owned();
    const auto p = *payload[field]->coord();
    payload[field] = std::make_shared<DRW_Variant>(field == 4 ? 1011 : 1012, DRW_Coord(p.x, p.y, .125));
    ordinary("definition-z", f::raw(1, f::metadata(payload)));
}

TEST_CASE("G import retains invisible carrier attributes without inserting a source object", "[lc-offsetcurve][g-fixtures][dxf][attributes]") {
    Setup setup; lc::test::OutputDirectory directory("offset-persistence-hidden");
    auto raw = f::raw(1); raw.visible = false;
    RS_Graphic graphic; load(graphic, directory.file("input.dxf"), raw);
    auto* curve = dynamic_cast<LC_OffsetCurve*>(f::sole(graphic)); REQUIRE(curve);
    REQUIRE(curve->hasEditableDefinition()); CHECK_FALSE(curve->isVisible());
    CHECK(curve->getLayer()->getName() == "FixtureOffsets"); f::foreign(*curve);
    Snapshot before(graphic);
    const auto output = directory.file("hidden.dxf"); REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW2018)); before.check();
    const auto wire = f::readCodec(output); REQUIRE(wire.splines.size() == 1);
    CHECK_FALSE(wire.splines.front().visible); CHECK(wire.ellipses == 0);
}

TEST_CASE("G plain undefined and inconsistent carriers strip stale known schema before export", "[lc-offsetcurve][g-fixtures][dxf][stale]") {
    Setup setup; const unsigned kind = GENERATE(0u, 1u, 2u);
    lc::test::OutputDirectory directory("offset-persistence-stale");
    RS_Graphic source; load(source, directory.file("input.dxf"), f::raw(3));
    auto* original = f::native(source, 3); Snapshot sourceBefore(source);
    std::unique_ptr<RS_Spline> carrier;
    if (kind == 0) carrier = original->toPlainSpline();
    else carrier.reset(static_cast<RS_Spline*>(original->clone()));
    if (kind == 1) carrier->scale({0, 0}, {2, 3});
    if (kind == 2) carrier->getData().controlPoints.front().x += .125;
    if (const auto* native = dynamic_cast<LC_OffsetCurve*>(carrier.get())) REQUIRE_FALSE(native->hasEditableDefinition());
    carrier->setDrwExtData(f::metadata());
    RS_Graphic graphic; graphic.initForNewDocument(); carrier->reparent(&graphic); carrier->setLayer(graphic.getActiveLayer());
    graphic.addEntity(carrier.release()); Snapshot before(graphic);
    const auto output = directory.file("ordinary.dxf"); REQUIRE(lc::test::exportDrawing(graphic, output, RS2::FormatDXFRW2018));
    before.check(); sourceBefore.check();
    const auto wire = f::readCodec(output); REQUIRE(wire.splines.size() == 1); CHECK(f::markerCount(wire.splines.front().extData) == 0);
    RS_Graphic reopened; REQUIRE(lc::test::importDrawing(reopened, output, RS2::FormatDXFRW2018)); f::fallback(reopened);
}
