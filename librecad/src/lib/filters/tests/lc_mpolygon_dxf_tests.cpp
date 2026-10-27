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
****************************************************************************/

// H09-1 regression tests. MPOLYGON code 70 is the version and 71 is the solid
// flag; libdxfrw used to hand both to DRW_Hatch which stored 70 as `solid`.
// Every ezdxf-written pattern MPOLYGON (71 = 0, 70 = version) therefore read
// as a solid fill. The writer must now use ODA's tag order (70 version first,
// 71 solid, loop records with 73 before 72, 76 trailer gates after-loop
// fields, no 75/97/330) so independent readers accept the round trip.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "drw_entities.h"
#include "libdxfrw.h"

namespace {

// Stub addMPolygon capture: all other DRW_Interface overrides are no-ops.
class CaptureIface : public DRW_Interface {
public:
    void addHeader(const DRW_Header *) override {}
    void addLType(const DRW_LType &) override {}
    void addLayer(const DRW_Layer &) override {}
    void addDimStyle(const DRW_Dimstyle &) override {}
    void addVport(const DRW_Vport &) override {}
    void addTextStyle(const DRW_Textstyle &) override {}
    void addAppId(const DRW_AppId &) override {}
    void addBlock(const DRW_Block &) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point &) override {}
    void addLine(const DRW_Line &) override {}
    void addRay(const DRW_Ray &) override {}
    void addXline(const DRW_Xline &) override {}
    void addArc(const DRW_Arc &) override {}
    void addCircle(const DRW_Circle &) override {}
    void addEllipse(const DRW_Ellipse &) override {}
    void addLWPolyline(const DRW_LWPolyline &) override {}
    void addPolyline(const DRW_Polyline &) override {}
    void addSpline(const DRW_Spline *) override {}
    void addKnot(const DRW_Entity &) override {}
    void addInsert(const DRW_Insert &) override {}
    void addTrace(const DRW_Trace &) override {}
    void add3dFace(const DRW_3Dface &) override {}
    void addSolid(const DRW_Solid &) override {}
    void addMText(const DRW_MText &) override {}
    void addText(const DRW_Text &) override {}
    void addDimAlign(const DRW_DimAligned *) override {}
    void addDimLinear(const DRW_DimLinear *) override {}
    void addDimRadial(const DRW_DimRadial *) override {}
    void addDimDiametric(const DRW_DimDiametric *) override {}
    void addDimAngular(const DRW_DimAngular *) override {}
    void addDimAngular3P(const DRW_DimAngular3p *) override {}
    void addDimArc(const DRW_DimArc *) override {}
    void addDimOrdinate(const DRW_DimOrdinate *) override {}
    void addLeader(const DRW_Leader *) override {}
    void addHatch(const DRW_Hatch *) override { ++m_hatchCount; }
    void addMPolygon(const DRW_MPolygon *mp) override {
        if (mp) {
            m_mpolygon = *mp;
            // Deep-copy the loops because the base class owns them.
            m_mpolygon.looplist.clear();
            for (const auto& l : mp->looplist) {
                if (!l) continue;
                m_mpolygon.looplist.push_back(l);
            }
        }
        ++m_count;
    }
    void addViewport(const DRW_Viewport &) override {}
    void addImage(const DRW_Image *) override {}
    void addWipeout(const DRW_Wipeout *) override {}
    void addMLeader(const DRW_MLeader *) override {}
    void addMLeaderStyle(const DRW_MLeaderStyle *) override {}
    void linkImage(const DRW_ImageDef *) override {}
    void addComment(const char *) override {}
    void addPlotSettings(const DRW_PlotSettings *) override {}
    void writeHeader(DRW_Header &) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}

    int m_count{0};
    int m_hatchCount{0};
    DRW_MPolygon m_mpolygon;
};

bool readDxfString(const std::string& dxf, CaptureIface& cap, const char* name) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(path);
    {
        std::ofstream out(path);
        out << dxf;
    }
    dxfRW reader(path.string().c_str());
    const bool ok = reader.read(&cap, /*ext=*/true);
    std::filesystem::remove(path);
    return ok;
}

// An ezdxf-written pattern MPOLYGON. Transcribed from
// evidence/fixalgo/dxf_semantics/oda/in/m1_mpoly_pattern.dxf: ANSI31 at
// scale 4, rectangle 100x60, fill is pattern (71 = 0), one definition line.
const char* kPatternMPolygon =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nMPOLYGON\n5\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbMPolygon\n"
    "70\n1\n10\n0.0\n20\n0.0\n30\n0.0\n210\n0.0\n220\n0.0\n230\n1.0\n"
    "2\nANSI31\n71\n0\n"
    "91\n1\n"
    "92\n3\n73\n1\n72\n0\n93\n4\n"
    "10\n0.0\n20\n0.0\n"
    "10\n100.0\n20\n0.0\n"
    "10\n100.0\n20\n60.0\n"
    "10\n0.0\n20\n60.0\n"
    "76\n1\n52\n0.0\n41\n4.0\n77\n0\n78\n1\n"
    "53\n45.0\n43\n0.0\n44\n0.0\n45\n-8.9802561212\n46\n8.9802561212\n79\n0\n"
    "11\n0.0\n21\n0.0\n99\n0\n"
    "0\nENDSEC\n0\nEOF\n";

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF MPOLYGON: 70 is the version, 71 is the solid flag",
          "[lc-hatchfix][mpolygon]") {
    CaptureIface cap;
    REQUIRE(readDxfString(kPatternMPolygon, cap, "lc_mpoly_pattern.dxf"));
    REQUIRE(cap.m_count == 1);
    const DRW_MPolygon& mp = cap.m_mpolygon;
    CHECK(mp.mpVersion == 1);
    CHECK(mp.solid == 0);           // the broken reader gave 1 (solid)
    CHECK(mp.name == "ANSI31");
    CHECK(mp.hpattern == 1);        // code 76
    CHECK(mp.scale == 4.0);
    CHECK(mp.patternLines.size() == 1u);
    // Group 53 reads as radians since libdxfrw 0.6.4: 45 deg -> pi/4.
    CHECK(mp.patternLines.at(0).angle == Catch::Approx(0.7853981633974483).margin(1e-12));
    REQUIRE(mp.looplist.size() == 1u);
    const auto& loop = mp.looplist.at(0);
    CHECK((loop->type & 2) == 2);
    auto* pl = dynamic_cast<DRW_LWPolyline*>(loop->objlist.at(0).get());
    REQUIRE(pl != nullptr);
    CHECK(pl->vertlist.size() == 4u);
    CHECK((pl->flags & 1) == 1);    // closed, from code 73 before 72
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF MPOLYGON: an omitted 71 reads as a pattern (solid = 0)",
          "[lc-hatchfix][mpolygon]") {
    // DRW_Hatch() defaults solid = 1. If code 71 is missing, the MPOLYGON must
    // still read as a pattern.
    const char* dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nMPOLYGON\n5\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbMPolygon\n"
        "70\n1\n10\n0.0\n20\n0.0\n30\n0.0\n210\n0.0\n220\n0.0\n230\n1.0\n"
        "2\nANSI31\n"
        "91\n1\n"
        "92\n3\n73\n1\n72\n0\n93\n3\n"
        "10\n0.0\n20\n0.0\n"
        "10\n1.0\n20\n0.0\n"
        "10\n0.0\n20\n1.0\n"
        "76\n1\n52\n0.0\n41\n1.0\n77\n0\n78\n0\n"
        "11\n0.0\n21\n0.0\n99\n0\n"
        "0\nENDSEC\n0\nEOF\n";
    CaptureIface cap;
    REQUIRE(readDxfString(dxf, cap, "lc_mpoly_no71.dxf"));
    REQUIRE(cap.m_count == 1);
    CHECK(cap.m_mpolygon.solid == 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF MPOLYGON writer: version 70 precedes solid 71 and emits ODA tag order",
          "[lc-hatchfix][mpolygon]") {
    const auto path = std::filesystem::temp_directory_path() / "lc_mpoly_write_order.dxf";
    std::filesystem::remove(path);
    {
        dxfRW writer(path.string().c_str());
        class Emit : public CaptureIface {
        public:
            void writeEntities() override {
                DRW_MPolygon mp;
                mp.layer = "0";
                mp.mpVersion = 1;
                mp.solid = 0;
                mp.name = "ANSI31";
                mp.hpattern = 1;
                mp.scale = 4.0;
                mp.angle = 0.0;
                mp.doubleflag = 0;
                DRW_Hatch::PatternLine line;
                line.angle = 45.0;
                line.baseX = 0.0;
                line.baseY = 0.0;
                line.offsetX = -8.9802561212;
                line.offsetY = 8.9802561212;
                mp.patternLines.push_back(line);
                auto loop = std::make_shared<DRW_HatchLoop>(1);
                auto pl = std::make_shared<DRW_LWPolyline>();
                pl->flags = 1;
                for (double y : {0.0, 10.0, 10.0, 0.0}) {
                    auto v = std::make_shared<DRW_Vertex2D>();
                    v->x = (y == 0.0) ? 0.0 : 10.0;
                    v->y = y;
                    v->bulge = 0.0;
                    pl->vertlist.push_back(v);
                }
                // Fix x coordinates: 0,10,10,0
                pl->vertlist[0]->x = 0.0; pl->vertlist[0]->y = 0.0;
                pl->vertlist[1]->x = 10.0; pl->vertlist[1]->y = 0.0;
                pl->vertlist[2]->x = 10.0; pl->vertlist[2]->y = 10.0;
                pl->vertlist[3]->x = 0.0; pl->vertlist[3]->y = 10.0;
                loop->objlist.push_back(pl);
                loop->type = 3;
                mp.looplist.push_back(loop);
                m_writer->writeMPolygon(&mp);
            }
            dxfRW* m_writer{nullptr};
        } emit;
        emit.m_writer = &writer;
        REQUIRE(writer.write(&emit, DRW::AC1021, /*bin=*/false));
    }

    std::ifstream in(path);
    std::string body((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    std::filesystem::remove(path);
    body.erase(std::remove(body.begin(), body.end(), '\r'), body.end());

    const std::size_t mpAt = body.find("\n  0\nMPOLYGON\n");
    REQUIRE(mpAt != std::string::npos);
    const std::size_t ac = body.find("\n100\nAcDbMPolygon\n", mpAt);
    REQUIRE(ac != std::string::npos);
    const std::size_t p70 = body.find("\n 70\n", ac);
    const std::size_t p71 = body.find("\n 71\n", ac);
    const std::size_t p92 = body.find("\n 92\n", ac);
    const std::size_t p73 = body.find("\n 73\n", p92);
    const std::size_t p72 = body.find("\n 72\n", p92);
    const std::size_t p76 = body.find("\n 76\n", ac);
    REQUIRE(p70 != std::string::npos);
    REQUIRE(p71 != std::string::npos);
    REQUIRE(p92 != std::string::npos);
    REQUIRE(p73 != std::string::npos);
    REQUIRE(p72 != std::string::npos);
    REQUIRE(p76 != std::string::npos);
    CHECK(p70 < p71);               // 70 version before 71 solid
    CHECK(p73 < p72);               // 73 closed before 72 bulge, inside the loop
    CHECK(p71 < p92);               // solid before loop count 91/92
    CHECK(p92 < p76);               // 76 trailer last

    // ODA writes no 75 or 97 on MPOLYGON.
    CHECK(body.find("\n 75\n", mpAt) == std::string::npos);
    // 97 would appear only after a loop; ensure it is not written there.
    const std::size_t next0 = body.find("\n  0\n", ac);
    CHECK(body.find("\n 97\n", ac) >= next0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF MPOLYGON writer refuses a non-default annotatedBoundary",
          "[lc-hatchfix][mpolygon]") {
    const auto path = std::filesystem::temp_directory_path() / "lc_mpoly_annot.dxf";
    std::filesystem::remove(path);
    dxfRW writer(path.string().c_str());
    class Emit : public CaptureIface {
    public:
        void writeEntities() override {
            DRW_MPolygon mp;
            mp.layer = "0";
            mp.mpVersion = 1;
            mp.solid = 1;
            mp.name = "SOLID";
            mp.annotatedBoundary = 42;   // the writer must reject this
            auto loop = std::make_shared<DRW_HatchLoop>(3);
            auto pl = std::make_shared<DRW_LWPolyline>();
            pl->flags = 1;
            for (int i = 0; i < 3; ++i) {
                auto v = std::make_shared<DRW_Vertex2D>();
                v->x = i * 1.0; v->y = (i % 2) * 1.0; v->bulge = 0;
                pl->vertlist.push_back(v);
            }
            loop->objlist.push_back(pl);
            mp.looplist.push_back(loop);
            m_ok = m_writer->writeMPolygon(&mp);
        }
        dxfRW* m_writer{nullptr};
        bool m_ok{true};
    } emit;
    emit.m_writer = &writer;
    (void)writer.write(&emit, DRW::AC1021, /*bin=*/false);
    CHECK_FALSE(emit.m_ok);
    std::filesystem::remove(path);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF MPOLYGON writer flattens an ARC-edge loop into bulges",
          "[lc-hatchfix][mpolygon]") {
    const auto path = std::filesystem::temp_directory_path() / "lc_mpoly_arc.dxf";
    std::filesystem::remove(path);
    {
        dxfRW writer(path.string().c_str());
        class Emit : public CaptureIface {
        public:
            void writeEntities() override {
                DRW_MPolygon mp;
                mp.layer = "0";
                mp.mpVersion = 1;
                mp.solid = 1;
                mp.name = "SOLID";
                // Two edges: a line from (0,0) to (10,0) and a semicircle
                // back to (0,0), ccw, radius 5, center (5,0).
                auto loop = std::make_shared<DRW_HatchLoop>(0);
                auto ln = std::make_shared<DRW_Line>();
                ln->basePoint.x = 0.0; ln->basePoint.y = 0.0;
                ln->secPoint.x = 10.0; ln->secPoint.y = 0.0;
                loop->objlist.push_back(ln);
                auto arc = std::make_shared<DRW_Arc>();
                arc->basePoint.x = 5.0; arc->basePoint.y = 0.0;
                arc->radious = 5.0;
                arc->staangle = 0.0;
                arc->endangle = M_PI;
                arc->isccw = 1;
                loop->objlist.push_back(arc);
                mp.looplist.push_back(loop);
                m_ok = m_writer->writeMPolygon(&mp);
            }
            dxfRW* m_writer{nullptr};
            bool m_ok{false};
        } emit;
        emit.m_writer = &writer;
        REQUIRE(writer.write(&emit, DRW::AC1021, /*bin=*/false));
        REQUIRE(emit.m_ok);
    }
    // Read it back: boundary is a polyline with a nonzero bulge on the arc leg.
    CaptureIface cap;
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&cap, /*ext=*/true));
    std::filesystem::remove(path);
    REQUIRE(cap.m_count == 1);
    REQUIRE(cap.m_mpolygon.looplist.size() == 1u);
    auto* pl = dynamic_cast<DRW_LWPolyline*>(
        cap.m_mpolygon.looplist.at(0)->objlist.at(0).get());
    REQUIRE(pl != nullptr);
    CHECK(pl->vertlist.size() == 2u);
    // The arc bulge for a 180-degree ccw sweep is tan(pi/4) = 1.
    bool sawBulge = false;
    for (const auto& v : pl->vertlist) {
        if (v && std::fabs(v->bulge - 1.0) < 1e-9) sawBulge = true;
    }
    CHECK(sawBulge);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF MPOLYGON writer refuses a SPLINE-edge loop",
          "[lc-hatchfix][mpolygon]") {
    const auto path = std::filesystem::temp_directory_path() / "lc_mpoly_spline.dxf";
    std::filesystem::remove(path);
    dxfRW writer(path.string().c_str());
    class Emit : public CaptureIface {
    public:
        void writeEntities() override {
            DRW_MPolygon mp;
            mp.layer = "0";
            mp.mpVersion = 1;
            mp.solid = 1;
            mp.name = "SOLID";
            auto loop = std::make_shared<DRW_HatchLoop>(0);
            auto sp = std::make_shared<DRW_Spline>();
            sp->degree = 2;
            loop->objlist.push_back(sp);
            mp.looplist.push_back(loop);
            m_ok = m_writer->writeMPolygon(&mp);
        }
        dxfRW* m_writer{nullptr};
        bool m_ok{true};
    } emit;
    emit.m_writer = &writer;
    (void)writer.write(&emit, DRW::AC1021, /*bin=*/false);
    CHECK_FALSE(emit.m_ok);
    std::filesystem::remove(path);
}

} // namespace
