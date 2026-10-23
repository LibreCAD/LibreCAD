/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_OFFSETCURVE_PERSISTENCE_FIXTURE_H
#define LC_OFFSETCURVE_PERSISTENCE_FIXTURE_H

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <locale>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"
#include "lc_drwnullinterface.h"
#include "lc_filtertestsupport.h"
#include "lc_offsetcurve.h"
#include "libdxfrw.h"
#include "rs_line.h"
#ifdef DWGSUPPORT
#include "libdwgr.h"
#endif

namespace lc::test::offset_persistence {
using Tags = std::vector<std::shared_ptr<DRW_Variant>>;

// Hand-authored GPL fixtures, independent of the schema merger and offset producer.
// Circle centre (8,16), radius 1, outward distance 1, sweep [0,1/64].
// The exact dyadic chord has sagitta < 0.000062 and endpoint error < 0.000002.
// The collinear cubic has the same image. Neither claims an exact circular arc.
inline std::vector<DRW_Coord> controls(int degree) {
    if (degree == 1) return {{10, 16, 0}, {9.999755859375, 16.03125, 0}};
    if (degree == 3) return {{10, 16, 0}, {9.99993896484375, 16.0078125, 0},
                           {9.99981689453125, 16.0234375, 0}, {9.999755859375, 16.03125, 0}};
    std::vector<DRW_Coord> result;
    for (int i = 0; i <= degree; ++i) {
        const double f = static_cast<double>(i) / degree;
        result.emplace_back(10 - f/4096, 16 + f/32, 0);
    }
    return result;
}

inline Tags owned(unsigned flags = 0x40, bool tail = true, int schema = 1) {
    Tags result{
        std::make_shared<DRW_Variant>(1000, std::string("LC_OFFSET_CURVE")),
        std::make_shared<DRW_Variant>(1002, std::string("{")),
        std::make_shared<DRW_Variant>(1070, schema),
        std::make_shared<DRW_Variant>(1000, std::string("ELLIPSE")),
        std::make_shared<DRW_Variant>(1011, DRW_Coord(8, 16, 0)),
        std::make_shared<DRW_Variant>(1012, DRW_Coord(1, 0, 0)),
        std::make_shared<DRW_Variant>(1040, 1.),
        std::make_shared<DRW_Variant>(1040, 0.),
        std::make_shared<DRW_Variant>(1040, .015625),
        std::make_shared<DRW_Variant>(1070, static_cast<std::int32_t>(flags)),
        std::make_shared<DRW_Variant>(1041, 1.),
        std::make_shared<DRW_Variant>(1040, .0001)};
    if (tail) {
        result.push_back(std::make_shared<DRW_Variant>(1000, std::string("safe owned tail")));
        result.push_back(std::make_shared<DRW_Variant>(1071, 123456));
        result.push_back(std::make_shared<DRW_Variant>(1002, std::string("{")));
        result.push_back(std::make_shared<DRW_Variant>(1000, std::string("nested safe item")));
        result.push_back(std::make_shared<DRW_Variant>(1002, std::string("}")));
    }
    result.push_back(std::make_shared<DRW_Variant>(1002, std::string("}")));
    return result;
}

inline Tags foreignBefore() {
    return {std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_A")),
            std::make_shared<DRW_Variant>(1000, std::string("foreign before")),
            std::make_shared<DRW_Variant>(1040, .375)};
}
inline Tags foreignAfter() {
    return {std::make_shared<DRW_Variant>(1001, std::string("FOREIGN_B")),
            std::make_shared<DRW_Variant>(1000, std::string("foreign after")),
            std::make_shared<DRW_Variant>(1070, 17)};
}
inline void append(Tags& target, const Tags& source) { target.insert(target.end(), source.begin(), source.end()); }
inline Tags metadata(const Tags& payload = owned(), const std::string& app = "LibreCad") {
    auto result = foreignBefore();
    result.push_back(std::make_shared<DRW_Variant>(1001, app));
    result.push_back(std::make_shared<DRW_Variant>(1000, std::string("unrelated leading")));
    append(result, payload);
    result.push_back(std::make_shared<DRW_Variant>(1000, std::string("unrelated trailing")));
    append(result, foreignAfter());
    return result;
}

inline DRW_Spline raw(int degree = 1, const Tags& data = metadata()) {
    DRW_Spline spline;
    spline.degree = degree; spline.flags = 8; spline.layer = "FixtureOffsets";
    spline.color = 3; spline.lWeight = DRW_LW_Conv::width07;
    spline.normalVec = {0, 0, 1}; spline.m_scenario = 1;
    for (const auto& p : controls(degree)) spline.controllist.push_back(std::make_shared<DRW_Coord>(p));
    spline.knotslist.assign(static_cast<std::size_t>(degree + 1), 0.);
    spline.knotslist.insert(spline.knotslist.end(), static_cast<std::size_t>(degree + 1), 1.);
    spline.ncontrol = static_cast<std::int32_t>(spline.controllist.size());
    spline.nknots = static_cast<std::int32_t>(spline.knotslist.size());
    spline.nfit = 0; spline.extData = data;
    return spline;
}

inline void group(std::ostream& out, int code, const std::string& value) { out << code << '\n' << value << '\n'; }
template <typename T> inline void number(std::ostream& out, int code, T value) { out << code << '\n' << value << '\n'; }
inline void point(std::ostream& out, int code, const DRW_Coord& p) {
    number(out, code, p.x); number(out, code + 10, p.y); number(out, code + 20, p.z);
}
inline void xdata(std::ostream& out, const Tags& tags) {
    for (const auto& tag : tags) {
        REQUIRE(tag);
        switch (tag->type()) {
        case DRW_Variant::STRING: group(out, tag->code(), *tag->content.s); break;
        case DRW_Variant::INTEGER: number(out, tag->code(), tag->i_val()); break;
        case DRW_Variant::DOUBLE: number(out, tag->code(), tag->d_val()); break;
        case DRW_Variant::COORD: {
            const auto& p = *tag->coord();
            number(out, tag->code(), p.x); number(out, tag->code() + 10, p.y); number(out, tag->code() + 20, p.z);
            break;
        }
        default: FAIL("Fixture's ASCII emitter supports only its literal typed payloads");
        }
    }
}

inline std::string ascii(const DRW_Spline& spline, bool helix = false) {
    std::ostringstream out;
    out.imbue(std::locale::classic()); out << std::setprecision(17);
    out << "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n"
           "9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n0\nSECTION\n2\nTABLES\n"
           "0\nTABLE\n2\nLAYER\n5\n40\n100\nAcDbSymbolTable\n70\n2\n"
           "0\nLAYER\n5\n41\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
           "0\nLAYER\n5\n42\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n2\nFixtureOffsets\n70\n0\n62\n3\n6\nCONTINUOUS\n0\nENDTAB\n"
           "0\nTABLE\n2\nAPPID\n5\n50\n100\nAcDbSymbolTable\n70\n4\n";
    unsigned appHandle = 51;
    for (const auto* app : {"LibreCad", "FOREIGN_A", "FOREIGN_B", " LibreCad "})
        out << "0\nAPPID\n5\n" << appHandle++ << "\n100\nAcDbSymbolTableRecord\n100\nAcDbRegAppTableRecord\n2\n" << app << "\n70\n0\n";
    out << "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n";
    group(out, 0, helix ? "HELIX" : "SPLINE");
    out << "5\nA1\n100\nAcDbEntity\n8\n" << spline.layer << "\n62\n3\n370\n25\n";
    number(out, 60, spline.visible ? 0 : 1);
    out << "100\nAcDbSpline\n";
    number(out, 70, spline.flags); number(out, 71, spline.degree);
    number(out, 72, spline.nknots); number(out, 73, spline.ncontrol); number(out, 74, spline.nfit);
    number(out, 42, spline.tolknot); number(out, 43, spline.tolcontrol); number(out, 44, spline.tolfit);
    point(out, 210, spline.normalVec);
    for (double knot : spline.knotslist) number(out, 40, knot);
    for (double weight : spline.weightlist) number(out, 41, weight);
    for (const auto& p : spline.controllist) if (p) point(out, 10, *p);
    for (const auto& p : spline.fitlist) if (p) point(out, 11, *p);
    if (helix) out << "100\nAcDbHelix\n90\n1\n91\n0\n10\n8\n20\n16\n30\n0\n"
                       "11\n10\n21\n16\n31\n0\n12\n0\n22\n0\n32\n1\n40\n2\n41\n1\n42\n0\n290\n0\n280\n0\n";
    xdata(out, spline.extData);
    out << "0\nENDSEC\n0\nEOF\n";
    return out.str();
}
inline void put(const std::filesystem::path& file, const std::string& bytes) {
    std::ofstream out(file, std::ios::binary);
    REQUIRE(out.good()); out.write(bytes.data(), static_cast<std::streamsize>(bytes.size())); out.close();
    REQUIRE(out.good());
}

inline std::uint64_t bits(double value) {
    std::uint64_t result; static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result)); return result;
}
inline std::string tagsState(const Tags& tags) {
    std::ostringstream out;
    for (const auto& tag : tags) {
        if (!tag) { out << "null;"; continue; }
        out << tag->code() << ':' << static_cast<int>(tag->type()) << ':';
        switch (tag->type()) {
        case DRW_Variant::STRING: out << tag->content.s->size() << ':' << *tag->content.s; break;
        case DRW_Variant::INTEGER: out << tag->i_val(); break;
        case DRW_Variant::INTEGER64: out << tag->i64_val(); break;
        case DRW_Variant::DOUBLE: out << bits(tag->d_val()); break;
        case DRW_Variant::COORD: out << bits(tag->coord()->x) << ',' << bits(tag->coord()->y) << ',' << bits(tag->coord()->z); break;
        case DRW_Variant::BINARY: for (auto byte : *tag->binary()) out << static_cast<unsigned>(byte) << ','; break;
        case DRW_Variant::INVALID: break;
        }
        out << ';';
    }
    return out.str();
}
inline Tags application(const Tags& tags, const std::string& name) {
    Tags result; bool inside = false;
    for (const auto& tag : tags) {
        REQUIRE(tag);
        if (tag->code() == 1001) inside = tag->type() == DRW_Variant::STRING && *tag->content.s == name;
        if (inside) result.push_back(tag);
    }
    return result;
}
inline unsigned markerCount(const Tags& tags) {
    return static_cast<unsigned>(std::count_if(tags.begin(), tags.end(), [](const auto& tag) {
        return tag && tag->code() == 1000 && tag->type() == DRW_Variant::STRING && *tag->content.s == "LC_OFFSET_CURVE";
    }));
}
inline void foreign(const RS_Entity& entity) {
    CHECK(tagsState(application(entity.getDrwExtData(), "FOREIGN_A")) == tagsState(foreignBefore()));
    CHECK(tagsState(application(entity.getDrwExtData(), "FOREIGN_B")) == tagsState(foreignAfter()));
}

struct Capture final : NullDrwInterface {
    std::vector<DRW_AppId> apps;
    std::vector<DRW_Spline> splines;
    std::vector<DRW_Helix> helices;
    std::vector<DRW_Hatch> hatches;
    unsigned ellipses = 0;
    void addAppId(const DRW_AppId& value) override { apps.push_back(value); }
    void addSpline(const DRW_Spline* value) override { REQUIRE(value); splines.push_back(*value); }
    void addHelix(const DRW_Helix* value) override { REQUIRE(value); helices.push_back(*value); }
    void addHatch(const DRW_Hatch* value) override { REQUIRE(value); hatches.push_back(*value); }
    void addEllipse(const DRW_Ellipse&) override { ++ellipses; }
};
inline Capture readCodec(const std::filesystem::path& file, bool dwg = false) {
    Capture result;
#ifdef DWGSUPPORT
    if (dwg) { dwgRW reader(file.string().c_str()); REQUIRE(reader.read(&result, false)); return result; }
#else
    REQUIRE_FALSE(dwg);
#endif
    dxfRW reader(file.string().c_str()); REQUIRE(reader.read(&result, false)); return result;
}

// Real file emission from literal raw records, not the application G writer.
template <typename Writer> struct Emit final : NullDrwInterface {
    Writer& writer;
    DRW_Spline spline;
    std::string codePage;
    bool wrote = false;
    explicit Emit(Writer& writer, const DRW_Spline& spline, std::string page = "ANSI_1252")
        : writer(writer), spline(spline), codePage(std::move(page)) {}
    void writeHeader(DRW_Header& header) override { header.addStr("$DWGCODEPAGE", codePage, 3); }
    void writeLayers() override {
        DRW_Layer layer; layer.name = "FixtureOffsets"; layer.color = 3;
        if constexpr (std::is_same_v<Writer, dxfRW>) REQUIRE(writer.writeLayer(&layer));
        else REQUIRE(writer.addLayer(&layer));
    }
    void writeAppId() override {
        for (const auto* name : {"LibreCad", "FOREIGN_A", "FOREIGN_B"}) {
            DRW_AppId app; app.name = name;
            if constexpr (std::is_same_v<Writer, dxfRW>) REQUIRE(writer.writeAppId(&app));
            else REQUIRE(writer.addAppId(&app));
        }
    }
    void writeEntities() override { wrote = writer.writeSpline(&spline); REQUIRE(wrote); }
};
inline void binaryDxf(const std::filesystem::path& file, const DRW_Spline& spline, DRW::Version version) {
    dxfRW writer(file.string().c_str()); Emit<dxfRW> source(writer, spline);
    REQUIRE(writer.write(&source, version, true)); REQUIRE(source.wrote);
    const std::string signature("AutoCAD Binary DXF\r\n\x1a\0", 22);
    REQUIRE(fileBytes(file).compare(0, signature.size(), signature) == 0);
}
#ifdef DWGSUPPORT
inline void dwgFile(const std::filesystem::path& file, const DRW_Spline& spline, DRW::Version version,
                    std::string page = "ANSI_1252") {
    dwgRW writer(file.string().c_str()); Emit<dwgRW> source(writer, spline, std::move(page));
    REQUIRE(writer.write(&source, version, false)); REQUIRE(source.wrote);
    REQUIRE(fileBytes(file).substr(0, 2) == "AC");
}
#endif

inline std::vector<RS_Entity*> live(RS_EntityContainer& container) {
    std::vector<RS_Entity*> result;
    for (auto* entity : container) if (entity && !entity->isDeleted()) result.push_back(entity);
    return result;
}
inline RS_Entity* sole(RS_EntityContainer& container) {
    const auto entities = live(container); REQUIRE(entities.size() == 1); return entities.front();
}
inline LC_OffsetCurve* native(RS_EntityContainer& container, int degree) {
    auto* entity = sole(container);
    auto* curve = dynamic_cast<LC_OffsetCurve*>(entity);
    REQUIRE(curve); REQUIRE(curve->hasEditableDefinition());
    CHECK(curve->rtti() == RS2::EntityOffsetCurve); CHECK(curve->getParent() == &container);
    CHECK(curve->getData().degree == static_cast<std::size_t>(degree));
    CHECK(curve->definition().basis.center == RS_Vector(8, 16));
    CHECK(curve->definition().basis.majorP == RS_Vector(1, 0));
    CHECK(curve->definition().distance == 1.); CHECK(curve->definition().relTolerance == .0001);
    CHECK_FALSE(curve->definition().towardCentre);
    const auto expected = controls(degree);
    const auto& data = curve->getData(); REQUIRE(data.controlPoints.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CHECK(bits(data.controlPoints[i].x) == bits(expected[i].x));
        CHECK(bits(data.controlPoints[i].y) == bits(expected[i].y));
        CHECK(data.controlPoints[i].z == 0);
    }
    const auto expectedKnots = raw(degree).knotslist;
    CHECK(data.knotslist == expectedKnots);
    CHECK(data.weights == std::vector<double>(expected.size(), 1.));
    CHECK(curve->count() > 0); CHECK(curve->getLength() > 0);
    return curve;
}
inline void fallback(RS_Graphic& graphic, bool keepMarker = false) {
    auto* entity = sole(graphic);
    CHECK(entity->rtti() != RS2::EntityOffsetCurve);
    CHECK(entity->isVisible()); CHECK(entity->getLength() > 0);
    CHECK(markerCount(entity->getDrwExtData()) == (keepMarker ? 1u : 0u));
    foreign(*entity);
}
}
#endif
