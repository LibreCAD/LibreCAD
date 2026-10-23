/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "lc_drwnullinterface.h"
#include "lc_filtertestsupport.h"
#include "lc_offsetcurve.h"
#include "libdxfrw.h"
#include "rs_ellipse.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_spline.h"
#include "intern/drw_textcodec.h"
#ifdef DWGSUPPORT
#include "libdwgr.h"
#include "intern/dwgbufferw.h"
#endif

namespace {
using Tags = std::vector<std::shared_ptr<DRW_Variant>>;
using Path = std::filesystem::path;
std::uint64_t bits(double value) {
    std::uint64_t result;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
void samePoint(const RS_Vector &a, const RS_Vector &b) {
    CHECK(a.valid == b.valid);
    CHECK(bits(a.x) == bits(b.x)); CHECK(bits(a.y) == bits(b.y)); CHECK(bits(a.z) == bits(b.z));
}
void sameTags(const Tags &a, const Tags &b) {
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CAPTURE(i); REQUIRE(a[i]); REQUIRE(b[i]);
        CHECK(a[i]->code() == b[i]->code()); REQUIRE(a[i]->type() == b[i]->type());
        switch (a[i]->type()) {
        case DRW_Variant::STRING: REQUIRE(a[i]->content.s); CHECK(*a[i]->content.s == *b[i]->content.s); break;
        case DRW_Variant::INTEGER: CHECK(a[i]->i_val() == b[i]->i_val()); break;
        case DRW_Variant::INTEGER64: CHECK(a[i]->i64_val() == b[i]->i64_val()); break;
        case DRW_Variant::DOUBLE: CHECK(bits(a[i]->d_val()) == bits(b[i]->d_val())); break;
        case DRW_Variant::COORD:
            REQUIRE(a[i]->coord()); REQUIRE(b[i]->coord());
            CHECK(bits(a[i]->coord()->x) == bits(b[i]->coord()->x));
            CHECK(bits(a[i]->coord()->y) == bits(b[i]->coord()->y));
            CHECK(bits(a[i]->coord()->z) == bits(b[i]->coord()->z)); break;
        case DRW_Variant::BINARY: CHECK(*a[i]->binary() == *b[i]->binary()); break;
        case DRW_Variant::INVALID: break;
        }
    }
}
Tags deepTags(const Tags &tags) {
    Tags result;
    for (const auto &tag : tags) { REQUIRE(tag); result.push_back(std::make_shared<DRW_Variant>(*tag)); }
    return result;
}
void writeBytes(const Path &file, const std::string &bytes) {
    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    stream.close(); REQUIRE(stream.good());
}

struct Drawing {
    const bool settings = lc::test::ensureSettings();
    lc::test::QuietLog quiet;
    RS_Graphic graphic;
    Drawing() { graphic.initForNewDocument(); graphic.onLoadingCompleted(); }
};
struct Carrier {
    unsigned degree;
    double end;
    std::vector<RS_Vector> controls;
    std::vector<double> knots;
};
// Independent literal approximations to the radius-two offset of a unit circle.
// No production serializer, offset engine, or native constructor generates them.
Carrier carrier(unsigned degree) {
    if (degree == 1)
        return {1, .0001, {{2, 0}, {1.99999999, .0001999999996666667}}, {0, 0, 1, 1}};
    REQUIRE(degree == 3);
    return {3, .1, {{2, 0}, {2, .06668055902865638},
        {1.996665278587803, .13331939931707593},
        {1.9900083305560516, .1996668332936563}}, {0, 0, 0, 0, 1, 1, 1, 1}};
}
std::string schema(const Carrier &c, int flags = 16, int axisCode = 1012, double distance = 1) {
    std::ostringstream out; out << std::setprecision(17);
    out << "1000\nLC_OFFSET_CURVE\n1002\n{\n1070\n1\n1000\nELLIPSE\n"
        << "1011\n0\n1021\n0\n1031\n0\n" << axisCode << "\n1\n"
        << axisCode + 10 << "\n0\n" << axisCode + 20 << "\n0\n"
        << "1040\n1\n1040\n0\n1040\n" << c.end << "\n1070\n" << flags
        << "\n1041\n" << distance << "\n1040\n0.0001\n1000\nowned\n1002\n}\n";
    return out.str();
}
std::string rawDxf(Carrier c, const std::string &version = "AC1032",
                   const std::string &owned = {}, const std::string &foreign = "foreign",
                   const std::string &ownedText = "owned", bool tilted = false,
                   bool rational = false) {
    std::ostringstream out; out << std::setprecision(17);
    out << "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\n" << version
        << "\n9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n"
        << "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nAPPID\n5\n40\n100\nAcDbSymbolTable\n70\n2\n"
        << "0\nAPPID\n5\n41\n100\nAcDbSymbolTableRecord\n100\nAcDbRegAppTableRecord\n2\nlIbReCaD\n70\n0\n"
        << "0\nAPPID\n5\n42\n100\nAcDbSymbolTableRecord\n100\nAcDbRegAppTableRecord\n2\nFOREIGN\n70\n0\n"
        << "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nSPLINE\n5\n100\n"
        << "100\nAcDbEntity\n8\n0\n62\n3\n100\nAcDbSpline\n70\n" << (rational ? 12 : 8)
        << "\n71\n" << c.degree << "\n72\n" << c.knots.size() << "\n73\n" << c.controls.size()
        << "\n74\n0\n42\n1e-10\n43\n1e-10\n";
    for (const auto knot : c.knots) out << "40\n" << knot << "\n";
    for (const auto &point : c.controls) out << "10\n" << point.x << "\n20\n" << point.y << "\n30\n" << point.z << "\n";
    if (rational) for (std::size_t i = 0; i < c.controls.size(); ++i) out << "41\n" << (i == 1 ? .5 : 1) << "\n";
    if (tilted) out << "210\n1\n220\n0\n230\n1\n";
    std::string definition = owned.empty() ? schema(c) : owned;
    const auto at = definition.find("1000\nowned\n");
    if (at != std::string::npos) definition.replace(at, 11, "1000\n" + ownedText + "\n");
    out << "1001\nlIbReCaD\n" << definition
        << "1001\nFOREIGN\n1000\n" << foreign << "\n1003\n0\n1004\nE9FE\n1005\n0\n1071\n-2147483648\n"
        << "0\nENDSEC\n0\nEOF\n";
    return out.str();
}
LC_OffsetCurve &onlyNative(RS_Graphic &graphic) {
    REQUIRE(graphic.count() == 1);
    auto *entity = graphic.entityAt(0); REQUIRE(entity);
    REQUIRE(entity->rtti() == RS2::EntityOffsetCurve);
    auto *native = dynamic_cast<LC_OffsetCurve *>(entity); REQUIRE(native);
    REQUIRE(native->hasEditableDefinition()); return *native;
}
void load(RS_Graphic &graphic, const Path &file, RS2::FormatType format = RS2::FormatDXFRW2018) {
    REQUIRE(lc::test::importDrawing(graphic, file, format)); graphic.onLoadingCompleted();
}
std::size_t markers(const Tags &tags) {
    return static_cast<std::size_t>(std::count_if(tags.begin(), tags.end(), [](const auto &tag) {
        return tag && tag->code() == 1000 && tag->type() == DRW_Variant::STRING &&
            tag->content.s && *tag->content.s == "LC_OFFSET_CURVE";
    }));
}
std::string firstText(const Tags &tags, const std::string &appid) {
    bool inGroup = false;
    for (const auto &tag : tags) {
        REQUIRE(tag);
        if (tag->code() == 1001) inGroup = *tag->content.s == appid;
        else if (inGroup && tag->code() == 1000) return *tag->content.s;
    }
    return {};
}
struct NativeState {
    const LC_OffsetCurve &entity;
    RS_SplineData data;
    LC_EllipseOffsetDefinition definition;
    LC_OffsetCurve::OwnedTail tail;
    std::vector<RS_Vector> grips;
    Tags aliases, tags;
    RS_EntityContainer *parent;
    RS_Layer *layer;
    RS_Pen pen;
    unsigned flags, ownedFlags;
    quint32 handle, dictionary;
    std::vector<quint32> reactors;
    unsigned long long identity, lineage, owner;
    std::vector<const RS_Entity *> children;
    NativeState(const LC_OffsetCurve &e) : entity(e), data(e.getData()), definition(e.definition()),
        tail(e.unknownOwnedTail()), grips(e.cachedDefinitionGrips()), aliases(e.getDrwExtData()), tags(deepTags(aliases)),
        parent(e.getParent()), layer(e.getLayer()), pen(e.getPen(false)), flags(e.getFlags()), ownedFlags(e.unknownOwnedFlags()),
        handle(e.sourceHandle()), dictionary(e.xDictHandle()), reactors(e.reactorHandles()), identity(e.getId()),
        lineage(e.lineage()), owner(e.ownerLineage()) { for (const auto *child : e) children.push_back(child); }
    void check() const {
        CHECK(entity.isConsistent()); CHECK(entity.hasEditableDefinition());
        CHECK(RS_Spline::geometryFingerprint(entity.getData()) == RS_Spline::geometryFingerprint(data));
        CHECK(entity.getParent() == parent); CHECK(entity.getLayer() == layer); CHECK(entity.getPen(false) == pen);
        CHECK(entity.getFlags() == flags); CHECK(entity.unknownOwnedFlags() == ownedFlags);
        CHECK(entity.sourceHandle() == handle); CHECK(entity.xDictHandle() == dictionary); CHECK(entity.reactorHandles() == reactors);
        CHECK(entity.getId() == identity); CHECK(entity.lineage() == lineage); CHECK(entity.ownerLineage() == owner);
        const auto &d = entity.definition(); samePoint(d.basis.center, definition.basis.center); samePoint(d.basis.majorP, definition.basis.majorP);
        CHECK(bits(d.basis.ratio) == bits(definition.basis.ratio)); CHECK(bits(d.basis.angle1) == bits(definition.basis.angle1));
        CHECK(bits(d.basis.angle2) == bits(definition.basis.angle2)); CHECK(d.basis.reversed == definition.basis.reversed);
        CHECK(d.towardCentre == definition.towardCentre); CHECK(d.cacheReversed == definition.cacheReversed);
        CHECK(bits(d.distance) == bits(definition.distance)); CHECK(bits(d.relTolerance) == bits(definition.relTolerance));
        REQUIRE(entity.unknownOwnedTail().size() == tail.size());
        for (std::size_t i = 0; i < tail.size(); ++i) {
            CHECK(entity.unknownOwnedTail()[i].code == tail[i].code);
            REQUIRE(entity.unknownOwnedTail()[i].value.index() == tail[i].value.index());
            const auto &actual = entity.unknownOwnedTail()[i].value;
            switch (tail[i].value.index()) {
            case 0: CHECK(std::get<QString>(actual) == std::get<QString>(tail[i].value)); break;
            case 1: samePoint(std::get<RS_Vector>(actual), std::get<RS_Vector>(tail[i].value)); break;
            case 2: CHECK(bits(std::get<double>(actual)) == bits(std::get<double>(tail[i].value))); break;
            case 3: CHECK(std::get<std::int16_t>(actual) == std::get<std::int16_t>(tail[i].value)); break;
            case 4: CHECK(std::get<std::int32_t>(actual) == std::get<std::int32_t>(tail[i].value)); break;
            }
        }
        REQUIRE(entity.cachedDefinitionGrips().size() == grips.size());
        for (std::size_t i = 0; i < grips.size(); ++i) samePoint(entity.cachedDefinitionGrips()[i], grips[i]);
        sameTags(entity.getDrwExtData(), tags); CHECK(entity.getDrwExtData() == aliases);
        REQUIRE(entity.count() == children.size());
        for (std::size_t i = 0; i < children.size(); ++i) CHECK(entity.entityAt(static_cast<int>(i)) == children[i]);
    }
};
void checkWrittenSchema(const Path &file, unsigned degree) {
    const auto splines = lc::test::dxfRecords(file, "SPLINE"); REQUIRE(splines.size() == 1);
    const auto &record = splines.front();
    REQUIRE(record.own(71).size() == 1); CHECK(std::stoul(record.own(71).front()) == degree);
    const auto &groups = record.m_groups;
    const auto marker = std::find_if(groups.begin(), groups.end(), [](const auto &g) {
        return g.m_code == 1000 && g.m_value == "LC_OFFSET_CURVE";
    });
    REQUIRE(marker != groups.end());
    CHECK(std::count_if(groups.begin(), groups.end(), [](const auto &g) {
        return g.m_code == 1000 && g.m_value == "LC_OFFSET_CURVE";
    }) == 1);
    const std::array<int, 22> codes{{1000,1002,1070,1000,1011,1021,1031,1012,1022,1032,
        1040,1040,1040,1070,1041,1040,1000,1002,1001,1000,1003,1004}};
    // The owned list ends before the next APPID; foreign ordering is checked
    // independently, not by asking the production parser to validate its writer.
    const auto index = static_cast<std::size_t>(marker - groups.begin());
    REQUIRE(groups.size() >= index + 18);
    for (std::size_t i = 0; i < 18; ++i) CHECK(groups[index + i].m_code == codes[i]);
    CHECK(groups[index + 2].m_value == "1"); CHECK(groups[index + 3].m_value == "ELLIPSE");
    CHECK(std::stod(groups[index + 7].m_value) == 1); CHECK(std::stoi(groups[index + 13].m_value) == 16);
    CHECK(groups[index + 16].m_value == "owned"); CHECK(groups[index + 17].m_value == "}");
    CHECK(lc::test::dxfRecords(file, "ELLIPSE").empty());
    std::size_t registrations = 0;
    for (const auto &app : lc::test::dxfRecords(file, "APPID")) {
        REQUIRE(app.own(2).size() == 1); auto name = app.own(2).front();
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        if (name == "LIBRECAD") ++registrations;
    }
    CHECK(registrations == 1);
}

struct CodecFixture : lc::test::NullDrwInterface {
    dxfRW *dxf = nullptr;
#ifdef DWGSUPPORT
    dwgRW *dwg = nullptr;
#endif
    std::unique_ptr<DRW_Spline> spline;
    std::string page = "ANSI_1252";
    bool ok = true;
    explicit CodecFixture(unsigned degree) : spline(std::make_unique<DRW_Spline>()) {
        const auto c = carrier(degree); spline->degree = static_cast<std::int32_t>(degree); spline->flags = 8;
        for (const auto &p : c.controls) spline->controllist.push_back(std::make_shared<DRW_Coord>(p.x, p.y, p.z));
        spline->knotslist = c.knots; spline->ncontrol = static_cast<std::int32_t>(c.controls.size());
        spline->nknots = static_cast<std::int32_t>(c.knots.size()); spline->nfit = 0;
        // Literal typed schema, independent of LC_OffsetCurveXData::forExport.
        spline->extData = {
            std::make_shared<DRW_Variant>(1001, std::string("LibreCad")),
            std::make_shared<DRW_Variant>(1000, std::string("LC_OFFSET_CURVE")),
            std::make_shared<DRW_Variant>(1002, std::string("{")),
            std::make_shared<DRW_Variant>(1070, std::int32_t{1}),
            std::make_shared<DRW_Variant>(1000, std::string("ELLIPSE")),
            std::make_shared<DRW_Variant>(1011, DRW_Coord{0,0,0}),
            std::make_shared<DRW_Variant>(1012, DRW_Coord{1,0,0}),
            std::make_shared<DRW_Variant>(1040, 1.0), std::make_shared<DRW_Variant>(1040, 0.0),
            std::make_shared<DRW_Variant>(1040, c.end), std::make_shared<DRW_Variant>(1070, std::int32_t{16}),
            std::make_shared<DRW_Variant>(1041, 1.0), std::make_shared<DRW_Variant>(1040, 1e-4),
            std::make_shared<DRW_Variant>(1000, std::string("owned")),
            std::make_shared<DRW_Variant>(1002, std::string("}")),
            std::make_shared<DRW_Variant>(1001, std::string("FOREIGN")),
            std::make_shared<DRW_Variant>(1000, std::string("foreign"))};
    }
    void writeHeader(DRW_Header &header) override { header.addStr("$DWGCODEPAGE", page, 3); }
    void writeAppId() override {
        for (const auto *name : {"LibreCad", "FOREIGN"}) {
            DRW_AppId app; app.name = name;
#ifdef DWGSUPPORT
            if (dwg) { ok = dwg->addAppId(&app) && ok; continue; }
#endif
            ok = dxf->writeAppId(&app) && ok;
        }
    }
    void writeEntities() override {
#ifdef DWGSUPPORT
        if (dwg) { ok = dwg->writeSpline(spline.get()) && ok; return; }
#endif
        ok = dxf->writeSpline(spline.get()) && ok;
    }
};
}

TEST_CASE("offset adapter adopts original independent degree one and cubic DXF carriers", "[offset-adapter][import]") {
    lc::test::OutputDirectory out("offset_adapter_raw");
    for (unsigned degree : {1u, 3u}) {
        CAPTURE(degree); const auto c = carrier(degree); const auto file = out.file(std::to_string(degree) + ".dxf");
        writeBytes(file, rawDxf(c)); Drawing drawing; load(drawing.graphic, file);
        auto &native = onlyNative(drawing.graphic); CHECK(native.getData().degree == degree);
        REQUIRE(native.getData().controlPoints.size() == c.controls.size());
        for (std::size_t i = 0; i < c.controls.size(); ++i) samePoint(native.getData().controlPoints[i], c.controls[i]);
        CHECK(native.getData().knotslist == c.knots); CHECK(native.getData().weights == std::vector<double>(c.controls.size(), 1));
        CHECK(native.getData().type == RS_SplineData::SplineType::ClampedOpen);
        CHECK(native.getParent() == &drawing.graphic); REQUIRE(native.getLayer()); CHECK(native.getLayer()->getName() == "0");
        CHECK(native.sourceHandle() == 0x100); CHECK(native.getPen(false).getColor() == RS_Color(0, 255, 0));
        CHECK(markers(native.getDrwExtData()) == 0); CHECK(firstText(native.getDrwExtData(), "FOREIGN") == "foreign");
        CHECK(native.unknownOwnedFlags() == 16); REQUIRE(native.unknownOwnedTail().size() == 1);
        CHECK(std::get<QString>(native.unknownOwnedTail()[0].value) == QStringLiteral("owned"));
    }
}

TEST_CASE("offset adapter raw Z normal weights and knots never gain editability through repair", "[offset-adapter][import]") {
    lc::test::OutputDirectory out("offset_adapter_raw_refusal");
    for (const unsigned defect : {0u, 1u, 2u, 3u}) {
        CAPTURE(defect); auto c = carrier(3);
        if (defect == 0) c.controls[1].z = 1e-12;
        if (defect == 3) c.knots[3] = .25;
        const auto file = out.file(std::to_string(defect) + ".dxf");
        writeBytes(file, rawDxf(c, "AC1032", {}, "foreign", "owned", defect == 1, defect == 2));
        Drawing drawing; load(drawing.graphic, file);
        REQUIRE(drawing.graphic.count() == 1); const auto *entity = drawing.graphic.entityAt(0); REQUIRE(entity);
        CHECK(entity->rtti() != RS2::EntityOffsetCurve); CHECK(markers(entity->getDrwExtData()) == 0);
        CHECK(firstText(entity->getDrwExtData(), "FOREIGN") == "foreign"); CHECK(entity->getParent() == &drawing.graphic);
    }
}

TEST_CASE("offset adapter never invents a source codec from a missing or unrecognized version", "[offset-adapter][import]") {
    lc::test::OutputDirectory out("offset_adapter_unknown_version");
    for (const bool preseed : {false, true}) for (const bool missing : {false, true}) {
        CAPTURE(preseed, missing);
        auto bytes = rawDxf(carrier(3), missing ? "AC1032" : "unrecognized1032");
        if (missing) {
            const std::string header = "9\n$ACADVER\n1\nAC1032\n";
            const auto at = bytes.find(header);
            REQUIRE(at != std::string::npos);
            bytes.erase(at, header.size());
        }
        const auto file = out.file(missing ? "missing.dxf" : "unknown.dxf");
        writeBytes(file, bytes);
        Drawing drawing;
        if (preseed) drawing.graphic.addVariable("$ACADVER", "AC1032", 1);
        if (!missing) {
            CHECK_FALSE(lc::test::importDrawing(drawing.graphic, file, RS2::FormatDXFRW2018));
            CHECK(drawing.graphic.count() == 0);
            continue;
        }
        load(drawing.graphic, file);
        REQUIRE(drawing.graphic.count() == 1);
        const auto* entity = drawing.graphic.entityAt(0);
        REQUIRE(entity);
        CHECK(entity->rtti() != RS2::EntityOffsetCurve);
        CHECK(markers(entity->getDrwExtData()) == 0);
        CHECK(firstText(entity->getDrwExtData(), "FOREIGN") == "foreign");
    }
}

TEST_CASE("offset adapter missing source codepage cannot inherit the destination codec", "[offset-adapter][import]") {
    lc::test::OutputDirectory out("offset_adapter_source_codepage");
    const auto c = carrier(3);
    auto owned = schema(c);
    const std::string placeholder = "1000\nowned\n";
    const auto at = owned.find(placeholder);
    REQUIRE(at != std::string::npos);
    std::string tail;
    // 16*(253 payload bytes + 3 typed framing bytes) is the exact tail cap.
    for (unsigned i = 0; i < 16; ++i) tail += "1000\n" + std::string(253, '\xE9') + "\n";
    owned.replace(at, placeholder.size(), tail);
    auto bytes = rawDxf(c, "AC1014", owned);
    const std::string page = "9\n$DWGCODEPAGE\n3\nANSI_1252\n";
    const auto pageAt = bytes.find(page);
    REQUIRE(pageAt != std::string::npos);
    bytes.erase(pageAt, page.size());
    const auto file = out.file("default-codepage.dxf");
    writeBytes(file, bytes);
    Drawing drawing;
    drawing.graphic.addVariable("$DWGCODEPAGE", "ANSI_1251", 3);
    load(drawing.graphic, file, RS2::FormatDXFRW14);
    const auto& native = onlyNative(drawing.graphic);
    REQUIRE(native.unknownOwnedTail().size() == 16);
    for (const auto& item : native.unknownOwnedTail()) {
        REQUIRE(item.code == 1000);
        CHECK(std::get<QString>(item.value) == QString(253, QChar(0xE9)));
    }
}

TEST_CASE("offset adapter strips invalid and duplicate known schemas on ordinary fallback", "[offset-adapter][import]") {
    lc::test::OutputDirectory out("offset_adapter_schema_refusal"); const auto c = carrier(3);
    const std::array<std::string, 4> payloads{{schema(c, 24), schema(c, 16, 1011), schema(c) + schema(c), schema(c, 16, 1012, .5)}};
    for (std::size_t i = 0; i < payloads.size(); ++i) {
        CAPTURE(i); const auto file = out.file(std::to_string(i) + ".dxf"); writeBytes(file, rawDxf(c, "AC1032", payloads[i]));
        Drawing drawing; load(drawing.graphic, file); REQUIRE(drawing.graphic.count() == 1);
        const auto *entity = drawing.graphic.entityAt(0); REQUIRE(entity);
        CHECK(entity->rtti() != RS2::EntityOffsetCurve); CHECK(markers(entity->getDrwExtData()) == 0);
        CHECK(firstText(entity->getDrwExtData(), "FOREIGN") == "foreign");
    }
}

TEST_CASE("offset adapter retains unknown schemas while stripping known data before fallback", "[offset-adapter][import]") {
    const std::string unknown = "1000\nLC_OFFSET_CURVE\n1002\n{\n1070\n2\n1000\nfuture\n1002\n}\n";
    lc::test::OutputDirectory out("offset_adapter_unknown"); const auto file = out.file("input.dxf");
    writeBytes(file, rawDxf(carrier(3), "AC1032", unknown + schema(carrier(3), 24)));
    Drawing drawing; load(drawing.graphic, file); REQUIRE(drawing.graphic.count() == 1);
    auto *entity = drawing.graphic.entityAt(0); REQUIRE(entity); CHECK(entity->rtti() != RS2::EntityOffsetCurve);
    CHECK(markers(entity->getDrwExtData()) == 1);
    const auto saved = out.file("saved.dxf"); REQUIRE(lc::test::exportDrawing(drawing.graphic, saved, RS2::FormatDXFRW2018));
    Drawing reopened; load(reopened.graphic, saved); REQUIRE(reopened.graphic.count() == 1);
    CHECK(reopened.graphic.entityAt(0)->rtti() != RS2::EntityOffsetCurve);
    CHECK(markers(reopened.graphic.entityAt(0)->getDrwExtData()) == 1);
}

TEST_CASE("offset adapter source DXF text decoding uses actual codepage without altering handles or binary", "[offset-adapter][codec-prerequisite]") {
    lc::test::OutputDirectory out("offset_adapter_source_text");
    const std::string utf8 = "caf\xC3\xA9";
    for (const auto &version : {std::string("AC1014"), std::string("AC1032")}) {
        CAPTURE(version); const auto raw = version == "AC1014" ? std::string("caf\xE9") : utf8;
        const auto file = out.file(version + ".dxf"); writeBytes(file, rawDxf(carrier(3), version, {}, raw, raw));
        Drawing drawing; load(drawing.graphic, file); auto &native = onlyNative(drawing.graphic);
        REQUIRE(native.unknownOwnedTail().size() == 1); CHECK(std::get<QString>(native.unknownOwnedTail()[0].value).toUtf8().toStdString() == utf8);
        CHECK(firstText(native.getDrwExtData(), "FOREIGN") == utf8);
        bool binary = false, handle = false;
        for (const auto &tag : native.getDrwExtData()) {
            if (tag->code() == 1004) { REQUIRE(tag->type() == DRW_Variant::STRING); CHECK(*tag->content.s == "E9FE"); binary = true; }
            if (tag->code() == 1005) { REQUIRE(tag->type() == DRW_Variant::STRING); CHECK(*tag->content.s == "0"); handle = true; }
        }
        CHECK(binary); CHECK(handle);
    }
    const auto escaped = out.file("escaped.dxf");
    writeBytes(escaped, rawDxf(carrier(3), "AC1014", {}, "\\U+4E2D", "\\U+4E2D"));
    Drawing drawing; load(drawing.graphic, escaped); auto &native = onlyNative(drawing.graphic);
    CHECK(firstText(native.getDrwExtData(), "FOREIGN") == "\xE4\xB8\xAD");
    CHECK(std::get<QString>(native.unknownOwnedTail()[0].value).toUtf8().toStdString() == "\xE4\xB8\xAD");
}

TEST_CASE("offset adapter DXF raw text bound is enforced before conversion", "[offset-adapter][codec-prerequisite]") {
    lc::test::OutputDirectory out("offset_adapter_source_text_cap");
    const auto accepted = out.file("255.dxf"), refused = out.file("256.dxf");
    writeBytes(accepted, rawDxf(carrier(3), "AC1014", {}, std::string(255, 'x')));
    writeBytes(refused, rawDxf(carrier(3), "AC1014", {}, std::string(256, 'x')));
    Drawing a; load(a.graphic, accepted); REQUIRE(onlyNative(a.graphic).hasEditableDefinition());
    Drawing b; CHECK_FALSE(lc::test::importDrawing(b.graphic, refused, RS2::FormatDXFRW14));
}

TEST_CASE("offset adapter modern DXF output contains independently expected schema and one APPID", "[offset-adapter][export]") {
    const std::array<RS2::FormatType, 7> formats{{RS2::FormatDXFRW14, RS2::FormatDXFRW2000, RS2::FormatDXFRW2004,
        RS2::FormatDXFRW, RS2::FormatDXFRW2010, RS2::FormatDXFRW2013, RS2::FormatDXFRW2018}};
    lc::test::OutputDirectory out("offset_adapter_dxf_outputs");
    for (const unsigned degree : {1u, 3u}) {
        const auto input = out.file("source" + std::to_string(degree) + ".dxf"); writeBytes(input, rawDxf(carrier(degree)));
        Drawing drawing; load(drawing.graphic, input); auto &native = onlyNative(drawing.graphic); const NativeState state(native);
        for (std::size_t i = 0; i < formats.size(); ++i) {
            CAPTURE(degree, i); const auto file = out.file(std::to_string(degree) + "_" + std::to_string(i) + ".dxf");
            REQUIRE(lc::test::exportDrawing(drawing.graphic, file, formats[i])); state.check(); checkWrittenSchema(file, degree);
            Drawing reopened; load(reopened.graphic, file, formats[i]);
            CHECK(onlyNative(reopened.graphic).getData().degree == degree);
        }
    }
}

TEST_CASE("offset adapter R12 output is geometry only and never re-adopts", "[offset-adapter][r12]") {
    lc::test::OutputDirectory out("offset_adapter_r12");
    for (unsigned degree : {1u, 3u}) {
        CAPTURE(degree); const auto input = out.file(std::to_string(degree) + "_source.dxf"); writeBytes(input, rawDxf(carrier(degree)));
        Drawing drawing; load(drawing.graphic, input); auto &native = onlyNative(drawing.graphic); const NativeState state(native);
        const auto file = out.file(std::to_string(degree) + ".dxf"); REQUIRE(lc::test::exportDrawing(drawing.graphic, file, RS2::FormatDXFRW12));
        state.check(); CHECK(lc::test::dxfRecords(file, "SPLINE").empty()); REQUIRE(lc::test::dxfRecords(file, "POLYLINE").size() == 1);
        CHECK(lc::test::fileBytes(file).find("LC_OFFSET_CURVE") == std::string::npos);
        Drawing reopened; load(reopened.graphic, file, RS2::FormatDXFRW12); REQUIRE(reopened.graphic.count() == 1);
        CHECK(reopened.graphic.entityAt(0)->rtti() != RS2::EntityOffsetCurve);
        const auto vertices = lc::test::dxfRecords(file, "VERTEX"); REQUIRE(vertices.size() >= 2);
        const double tolerance = 1e-4 * carrier(degree).controls.front().distanceTo(carrier(degree).controls.back());
        std::vector<RS_Vector> points;
        for (const auto &vertex : vertices) {
            REQUIRE(vertex.own(10).size() == 1); REQUIRE(vertex.own(20).size() == 1);
            const auto x = std::stod(vertex.own(10).front()), y = std::stod(vertex.own(20).front());
            CHECK(std::abs(std::hypot(x, y) - 2) <= tolerance);
            points.emplace_back(x, y);
        }
        // R12 is an approximation written with the existing decimal codec,
        // not an exact binary cache carrier. Keep endpoint roundoff local.
        const auto endpoint = [](const RS_Vector &actual, const RS_Vector &expected) {
            const double roundoff = 4 * std::numeric_limits<double>::epsilon() *
                std::max({1.0, std::abs(expected.x), std::abs(expected.y)});
            CHECK(actual.distanceTo(expected) <= roundoff);
        };
        endpoint(points.front(), carrier(degree).controls.front());
        endpoint(points.back(), carrier(degree).controls.back());
        for (std::size_t i = 1; i < points.size(); ++i) {
            const auto delta = points[i] - points[i - 1]; const auto &start = points[i - 1];
            const double length2 = delta.x*delta.x + delta.y*delta.y; REQUIRE(length2 > 0);
            const double u = std::clamp(-(start.x*delta.x + start.y*delta.y)/length2, 0.0, 1.0);
            const auto closest = start + delta*u;
            CHECK(std::abs(std::hypot(closest.x, closest.y) - 2) <= tolerance + 1e-9);
        }
    }
}

TEST_CASE("offset adapter actual edit clears owned extensions before second save", "[offset-adapter][edit]") {
    lc::test::OutputDirectory out("offset_adapter_edit"); const auto input = out.file("source.dxf"); writeBytes(input, rawDxf(carrier(3)));
    Drawing drawing; load(drawing.graphic, input); auto &native = onlyNative(drawing.graphic);
    const auto edited = native.setDistance(1.25); REQUIRE(edited); CHECK(edited.engineRuns == 1);
    CHECK(native.unknownOwnedFlags() == 0); CHECK(native.unknownOwnedTail().empty());
    const auto file = out.file("edited.dxf"); REQUIRE(lc::test::exportDrawing(drawing.graphic, file, RS2::FormatDXFRW2018));
    Drawing reopened; load(reopened.graphic, file); auto &again = onlyNative(reopened.graphic);
    CHECK(again.definition().distance == 1.25); CHECK(again.unknownOwnedTail().empty()); CHECK(again.unknownOwnedFlags() == 0);
    CHECK(lc::test::dxfRecords(file, "ELLIPSE").empty());
}

TEST_CASE("offset adapter refused metadata preserves target source and undo state with same-filter retry", "[offset-adapter][refusal]") {
    lc::test::OutputDirectory out("offset_adapter_refusal"); const auto input = out.file("source.dxf"); writeBytes(input, rawDxf(carrier(3)));
    Drawing drawing; load(drawing.graphic, input); auto &native = onlyNative(drawing.graphic);
    auto unsafe = native.getDrwExtData(); unsafe.push_back(std::make_shared<DRW_Variant>(1002, std::string("{")));
    native.setDrwExtData(unsafe); const NativeState state(native);
    bool beforeUndo = false, beforeRedo = false; drawing.graphic.collectUndoState(beforeUndo, beforeRedo);
    for (const bool existing : {false, true}) {
        const auto file = out.file(existing ? "existing.dxf" : "absent.dxf");
        if (existing) writeBytes(file, "original-target-bytes");
        const auto entries = out.entries(); RS_FilterDXFRW filter;
        CHECK_FALSE(filter.fileExport(drawing.graphic, lc::test::toQString(file), RS2::FormatDXFRW2018)); state.check();
        CHECK(out.entries() == entries); CHECK(std::filesystem::exists(file) == existing);
        if (existing) CHECK(lc::test::fileBytes(file) == "original-target-bytes");
        bool undo = false, redo = false; drawing.graphic.collectUndoState(undo, redo); CHECK(undo == beforeUndo); CHECK(redo == beforeRedo);
        Drawing good; good.graphic.addEntity(new RS_Line(&good.graphic, RS_Vector{0,0}, RS_Vector{1,1}));
        REQUIRE(filter.fileExport(good.graphic, lc::test::toQString(out.file(existing ? "retry2.dxf" : "retry1.dxf")), RS2::FormatDXFRW2018));
    }
}

TEST_CASE("offset adapter ASCII XDATA preserves controls and literal carets in owned and foreign text", "[offset-adapter][codec-prerequisite]") {
    lc::test::OutputDirectory out("offset_adapter_carets");
    const auto input = out.file("source.dxf"); writeBytes(input, rawDxf(carrier(3)));
    for (const std::string text : {std::string("\t"), std::string("^I"), std::string("literal ^ value"), std::string("^^\t^")}) {
        CAPTURE(text);
        Drawing drawing; load(drawing.graphic, input); auto &native = onlyNative(drawing.graphic);
        auto tags = native.getDrwExtData();
        tags.push_back(std::make_shared<DRW_Variant>(1000, text));
        native.setDrwExtData(tags);
        REQUIRE(native.setOwnedExtensions(0x10, {{1000, QString::fromUtf8(text)}}));
        const NativeState state(native);
        for (const auto format : {RS2::FormatDXFRW2000, RS2::FormatDXFRW2018}) {
            const auto file = out.file(format == RS2::FormatDXFRW2000 ? "legacy.dxf" : "modern.dxf");
            REQUIRE(lc::test::exportDrawing(drawing.graphic, file, format));
            Drawing reopened; load(reopened.graphic, file, format); const auto &copy = onlyNative(reopened.graphic);
            REQUIRE(copy.unknownOwnedTail().size() == 1);
            CHECK(std::get<QString>(copy.unknownOwnedTail()[0].value).toUtf8().toStdString() == text);
            REQUIRE(copy.getDrwExtData().back());
            REQUIRE(copy.getDrwExtData().back()->type() == DRW_Variant::STRING);
            CHECK(*copy.getDrwExtData().back()->content.s == text);
            state.check();
        }
    }
}

TEST_CASE("offset adapter refuses nonlossless charset escape spellings instead of rewriting foreign strings", "[offset-adapter][refusal]") {
    lc::test::OutputDirectory out("offset_adapter_nonlossless"); const auto input = out.file("source.dxf"); writeBytes(input, rawDxf(carrier(3)));
    for (const std::string text : {std::string("\\U+4E2D"), std::string("\\M+182A0")}) {
        CAPTURE(text); Drawing drawing; load(drawing.graphic, input); auto &native = onlyNative(drawing.graphic);
        auto tags = native.getDrwExtData();
        tags.push_back(std::make_shared<DRW_Variant>(1000, text)); native.setDrwExtData(tags);
        const NativeState state(native); const auto file = out.file(text.find("\\U+") == 0 ? "unicode-escape.dxf" : "mif-escape.dxf");
        CHECK_FALSE(lc::test::exportDrawing(drawing.graphic, file, RS2::FormatDXFRW2018));
        CHECK_FALSE(std::filesystem::exists(file)); state.check();
    }
}

TEST_CASE("offset adapter refuses a nonportable in memory native definition without modifying the target", "[offset-adapter][refusal]") {
    lc::test::OutputDirectory out("offset_adapter_nonportable");
    const auto input = out.file("source.dxf"); writeBytes(input, rawDxf(carrier(1)));
    Drawing drawing; load(drawing.graphic, input); auto &native = onlyNative(drawing.graphic);
    auto definition = native.definition(); definition.relTolerance = .01;
    const RS_Line cache{&drawing.graphic, native.getStartpoint(), native.getEndpoint()};
    auto nonportable = LC_OffsetCurve::adopt(cache, definition, .01);
    REQUIRE(nonportable); REQUIRE(nonportable->hasEditableDefinition());
    nonportable->setLayer(drawing.graphic.getActiveLayer());
    auto* source = nonportable.get(); drawing.graphic.addEntity(nonportable.release());
    const NativeState state(*source);
    for (const bool existing : {false, true}) {
        const auto file = out.file(existing ? "existing.dxf" : "absent.dxf");
        if (existing) writeBytes(file, "existing-target");
        const auto entries = out.entries();
        CHECK_FALSE(lc::test::exportDrawing(drawing.graphic, file, RS2::FormatDXFRW2018));
        CHECK(out.entries() == entries); CHECK(std::filesystem::exists(file) == existing);
        if (existing) CHECK(lc::test::fileBytes(file) == "existing-target");
        state.check();
    }
}

TEST_CASE("offset adapter never sanitizes invalid native controls while exporting plain geometry", "[offset-adapter][refusal]") {
    lc::test::OutputDirectory out("offset_adapter_native_control_refusal");
    const auto input = out.file("source.dxf");
    writeBytes(input, rawDxf(carrier(3)));
    std::vector<RS2::FormatType> formats{RS2::FormatDXFRW12, RS2::FormatDXFRW2000, RS2::FormatDXFRW2018};
#ifdef DWGSUPPORT
    formats.push_back(RS2::FormatDWG);
    formats.push_back(RS2::FormatDWG2018);
#endif
    for (unsigned defect = 0; defect < 4; ++defect) {
        Drawing drawing;
        load(drawing.graphic, input);
        auto& native = onlyNative(drawing.graphic);
        auto& point = native.getData().controlPoints[1];
        if (defect == 0) point.z = std::numeric_limits<double>::quiet_NaN();
        if (defect == 1) point.valid = false;
        if (defect == 2) point.z = std::numeric_limits<double>::infinity();
        if (defect == 3) point.z = 1;
        const auto original = point;
        const auto fingerprint = RS_Spline::geometryFingerprint(native.getData());
        const auto tags = deepTags(native.getDrwExtData());
        const auto aliases = native.getDrwExtData();
        const auto id = native.getId();
        const auto flags = native.getFlags();
        const auto children = native.count();
        const auto pen = native.getPen(false);
        const auto* layer = native.getLayer();
        for (std::size_t f = 0; f < formats.size(); ++f) for (const bool existing : {false, true}) {
            CAPTURE(defect, f, existing);
            const auto file = out.file(std::to_string(defect) + "_" + std::to_string(f) + (existing ? "_old" : "_new"));
            if (existing) writeBytes(file, "unchanged-target");
            const auto entries = out.entries();
            CHECK_FALSE(lc::test::exportDrawing(drawing.graphic, file, formats[f]));
            CHECK(out.entries() == entries);
            CHECK(std::filesystem::exists(file) == existing);
            if (existing) CHECK(lc::test::fileBytes(file) == "unchanged-target");
            CHECK(RS_Spline::geometryFingerprint(native.getData()) == fingerprint);
            samePoint(native.getData().controlPoints[1], original);
            sameTags(native.getDrwExtData(), tags);
            CHECK(native.getDrwExtData() == aliases);
            CHECK(native.getId() == id);
            CHECK(native.getFlags() == flags);
            CHECK(native.count() == children);
            CHECK(native.getPen(false) == pen);
            CHECK(native.getLayer() == layer);
            CHECK(native.getParent() == &drawing.graphic);
            CHECK(drawing.graphic.entityAt(0) == &native);
        }
    }
}

TEST_CASE("offset adapter R12 refuses a genuine approximation vertex-cap exhaustion", "[offset-adapter][r12][refusal]") {
    lc::test::OutputDirectory out("offset_adapter_r12_cap"); Drawing drawing;
    RS_SplineData data{1, false}; data.type = RS_SplineData::SplineType::ClampedOpen;
    for (unsigned i = 0; i <= 32768; ++i) data.controlPoints.emplace_back(i % 2, (i / 2) % 2);
    data.knotslist.push_back(0);
    for (unsigned i = 0; i <= 32768; ++i) data.knotslist.push_back(i);
    data.knotslist.push_back(32768); data.weights.assign(data.controlPoints.size(), 1);
    auto *spline = new RS_Spline(&drawing.graphic, data); REQUIRE(spline->validate()); drawing.graphic.addEntity(spline);
    const auto fingerprint = RS_Spline::geometryFingerprint(spline->getData());
    const auto file = out.file("guard.dxf"); writeBytes(file, "guard"); const auto entries = out.entries();
    CHECK_FALSE(lc::test::exportDrawing(drawing.graphic, file, RS2::FormatDXFRW12));
    CHECK(lc::test::fileBytes(file) == "guard"); CHECK(out.entries() == entries);
    CHECK(RS_Spline::geometryFingerprint(spline->getData()) == fingerprint);
}

TEST_CASE("offset adapter binary DXF codec fixtures reach actual application adoption", "[offset-adapter][binary]") {
    lc::test::OutputDirectory out("offset_adapter_binary");
    for (const auto version : {DRW::AC1014, DRW::AC1032}) for (unsigned degree : {1u, 3u}) {
        CAPTURE(static_cast<int>(version), degree); CodecFixture fixture(degree);
        const auto file = out.file(std::to_string(static_cast<int>(version)) + "_" + std::to_string(degree) + ".dxf");
        dxfRW writer(file.string().c_str()); fixture.dxf = &writer;
        REQUIRE(writer.write(&fixture, version, true)); REQUIRE(fixture.ok);
        Drawing drawing; load(drawing.graphic, file); CHECK(onlyNative(drawing.graphic).getData().degree == degree);
    }
}

#ifdef DWGSUPPORT
TEST_CASE("offset adapter byte text-area encoding follows the actual selected DWG file codepage", "[offset-adapter][codec-prerequisite]") {
    DRW_TextCodec codec; codec.setVersion(DRW::AC1018, false); codec.setByteCodePage("ANSI_1251");
    dwgBufferW buffer; buffer.setDecoder(&codec);
    CHECK(buffer.encodeTextAreaString("\xD0\xB0", false) == std::string("\xE0"));
    CHECK(buffer.encodeTextAreaString("\xD0\xB0", true) == std::string("\x30\x04", 2));
    DRW_TextCodec standalone; standalone.setVersion(DRW::AC1018, false); standalone.setCodePage("ANSI_1252", false);
    dwgBufferW fallback; fallback.setDecoder(&standalone);
    CHECK(fallback.encodeTextAreaString("caf\xC3\xA9", false) == std::string("caf\xE9"));
}

TEST_CASE("offset adapter independent DWG EED fixtures adopt both degrees through the application", "[offset-adapter][dwg-import]") {
    lc::test::OutputDirectory out("offset_adapter_dwg_fixture");
    for (const auto version : {DRW::AC1015, DRW::AC1032}) for (unsigned degree : {1u, 3u}) {
        CAPTURE(static_cast<int>(version), degree); CodecFixture fixture(degree); fixture.page = "ANSI_1251";
        *fixture.spline->extData.back()->content.s = "\xD0\xB0";
        const auto file = out.file(std::to_string(static_cast<int>(version)) + "_" + std::to_string(degree) + ".dwg");
        dwgRW writer(file.string().c_str()); fixture.dwg = &writer;
        REQUIRE(writer.write(&fixture, version, false)); REQUIRE(fixture.ok);
        Drawing drawing; load(drawing.graphic, file, RS2::FormatDWG); auto &native = onlyNative(drawing.graphic);
        CHECK(native.getData().degree == degree); CHECK(firstText(native.getDrwExtData(), "FOREIGN") == "\xD0\xB0");
    }
}

TEST_CASE("offset adapter DWG outputs preserve EED across actual cross-format second saves", "[offset-adapter][dwg-export]") {
    const std::array<RS2::FormatType, 6> formats{{RS2::FormatDWG, RS2::FormatDWG2004, RS2::FormatDWG2007,
        RS2::FormatDWG2010, RS2::FormatDWG2013, RS2::FormatDWG2018}};
    lc::test::OutputDirectory out("offset_adapter_dwg_outputs");
    for (unsigned degree : {1u, 3u}) {
        const auto input = out.file(std::to_string(degree) + "_source.dxf"); writeBytes(input, rawDxf(carrier(degree)));
        Drawing drawing; load(drawing.graphic, input); auto &native = onlyNative(drawing.graphic); const NativeState state(native);
        for (std::size_t i = 0; i < formats.size(); ++i) {
            CAPTURE(degree, i); const auto file = out.file(std::to_string(degree) + "_" + std::to_string(i) + ".dwg");
            REQUIRE(lc::test::exportDrawing(drawing.graphic, file, formats[i])); state.check();
            Drawing reopened; load(reopened.graphic, file, formats[i]); auto &again = onlyNative(reopened.graphic);
            CHECK(again.getData().degree == degree); CHECK(again.unknownOwnedFlags() == 16);
            CHECK(firstText(again.getDrwExtData(), "FOREIGN") == "foreign");
            const auto dxf = out.file(std::to_string(degree) + "_" + std::to_string(i) + "_second.dxf");
            REQUIRE(lc::test::exportDrawing(reopened.graphic, dxf, RS2::FormatDXFRW2018)); checkWrittenSchema(dxf, degree);
        }
    }
}
#endif
