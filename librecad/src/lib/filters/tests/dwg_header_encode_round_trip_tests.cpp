/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

/**
 * Phase 3a self-consistency tests for DRW_Header::encodeDwg.
 *
 * Pattern: encode a DRW_Header via encodeDwg, wrap the bytes as a
 * dwgBuffer, and parseDwg them back into a fresh DRW_Header.  The
 * encode/decode order must agree var-for-var; any bit-stream desync
 * shifts every subsequent value and fails an assertion downstream.
 *
 * Test 1: default-constructed header round-trips with the expected
 *   per-var defaults (caught: encoder uses wrong default values).
 *
 * Test 2: explicitly-set vars + control-handle members round-trip
 *   (caught: encoder reads from vars map but writes wrong type, or
 *   skips a member-stored handle).
 */

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_header.h"
#include "drw_interface.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "libdwgr.h"

/// Friend accessor for the protected encodeDwg / parseDwg and the
/// private control-handle members.  Declared as a friend by drw_header.h
/// via the SETHDRFRIENDS macro.
class DrwHeaderEncodeTestAccess {
public:
    static bool encode(DRW_Header& h, DRW::Version v,
                       dwgBufferW* buf, dwgBufferW* hBuf) {
        return h.encodeDwg(v, buf, hBuf);
    }
    static bool parse(DRW_Header& h, DRW::Version v,
                      dwgBuffer* buf, dwgBuffer* hBuf) {
        return h.parseDwg(v, buf, hBuf, /*mv=*/0);
    }
    static duint32& linetypeCtrl(DRW_Header& h)    { return h.linetypeCtrl; }
    static duint32& layerCtrl(DRW_Header& h)       { return h.layerCtrl; }
    static duint32& styleCtrl(DRW_Header& h)       { return h.styleCtrl; }
    static duint32& dimstyleCtrl(DRW_Header& h)    { return h.dimstyleCtrl; }
    static duint32& appidCtrl(DRW_Header& h)       { return h.appidCtrl; }
    static duint32& blockCtrl(DRW_Header& h)       { return h.blockCtrl; }
    static duint32& viewCtrl(DRW_Header& h)        { return h.viewCtrl; }
    static duint32& ucsCtrl(DRW_Header& h)         { return h.ucsCtrl; }
    static duint32& vportCtrl(DRW_Header& h)       { return h.vportCtrl; }
    static duint32& vpEntHeaderCtrl(DRW_Header& h) { return h.vpEntHeaderCtrl; }
    static duint32& handSeed(DRW_Header& h)        { return h.handSeed; }
};
using HA = DrwHeaderEncodeTestAccess;

namespace {

/// Encode `h` into a stream that begins with the 4-byte RL section-size
/// (matching what parseDwg expects to consume first), with the size
/// back-patched once the body is known.  Returns the accumulated bytes.
std::vector<duint8> encodeWithSizePrefix(DRW_Header& h,
                                         DRW::Version v = DRW::AC1015) {
    dwgBufferW w;
    // Reserve 4 bytes for the size_RL placeholder that parseDwg reads first.
    w.putRawLong32(0);
    REQUIRE(DrwHeaderEncodeTestAccess::encode(h, v, &w, &w));
    w.alignToByte();
    // Patch the size field: body bytes after the 4-byte size prefix.
    duint32 bodySize = static_cast<duint32>(w.size()) - 4;
    w.patchRawLong32(0, bodySize);
    return w.data();
}

/// Helper: extract a double var.
double dbl(const DRW_Header& h, const char* name) {
    auto it = h.vars.find(name);
    REQUIRE(it != h.vars.end());
    REQUIRE(it->second != nullptr);
    return it->second->d_val();
}

/// Helper: extract an int var.
dint32 i32(const DRW_Header& h, const char* name) {
    auto it = h.vars.find(name);
    REQUIRE(it != h.vars.end());
    REQUIRE(it->second != nullptr);
    return it->second->i_val();
}

} // namespace

TEST_CASE("DRW_Header::encodeDwg round-trips defaults",
          "[dwg-write][header-encode]") {
    DRW_Header src;  // default-constructed: vars map is empty

    auto bytes = encodeWithSizePrefix(src);
    REQUIRE(bytes.size() > 16);  // sanity: encoder emitted something

    // Decode into a fresh header.
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Header dst;
    REQUIRE(DrwHeaderEncodeTestAccess::parse(dst, DRW::AC1015, &r, &r));

    // -- Spot-check defaults from the encoder's defaults table ----------------
    // BD doubles
    REQUIRE(dbl(dst, "LTSCALE")  == 1.0);
    REQUIRE(dbl(dst, "TEXTSIZE") == 2.5);
    REQUIRE(dbl(dst, "DIMSCALE") == 1.0);
    REQUIRE(dbl(dst, "DIMASZ")   == 0.18);
    REQUIRE(dbl(dst, "DIMEXO")   == 0.0625);
    REQUIRE(dbl(dst, "DIMTXT")   == 0.18);
    REQUIRE(dbl(dst, "DIMCEN")   == 0.09);
    REQUIRE(dbl(dst, "DIMALTF")  == 25.4);
    REQUIRE(dbl(dst, "DIMGAP")   == 0.09);

    // BS shorts with non-zero defaults
    REQUIRE(i32(dst, "LUNITS")     == 2);
    REQUIRE(i32(dst, "LUPREC")     == 4);
    REQUIRE(i32(dst, "SPLINESEGS") == 8);
    REQUIRE(i32(dst, "MAXACTVP")   == 64);
    REQUIRE(i32(dst, "ISOLINES")   == 4);
    REQUIRE(i32(dst, "DIMLUNIT")   == 2);
    REQUIRE(i32(dst, "DIMATFIT")   == 3);
    REQUIRE(i32(dst, "DIMDSEP")    == static_cast<dint32>('.'));
    REQUIRE(i32(dst, "DIMTOLJ")    == 1);
    REQUIRE(i32(dst, "DIMALTD")    == 2);
    REQUIRE(i32(dst, "TSTACKALIGN") == 1);
    REQUIRE(i32(dst, "TSTACKSIZE") == 70);

    // DIMLWD/DIMLWE default to -2 (signed).  parseDwg reads BS as duint16
    // and stores it as dint32 without sign extension, so -2 round-trips as
    // the 16-bit bit pattern 0xFFFE == 65534.
    REQUIRE(i32(dst, "DIMLWD") == 0xFFFE);
    REQUIRE(i32(dst, "DIMLWE") == 0xFFFE);

    // Control-handle members: a fresh default header has all 0 (null).
    REQUIRE(HA::linetypeCtrl(dst)    == 0u);
    REQUIRE(HA::layerCtrl(dst)       == 0u);
    REQUIRE(HA::styleCtrl(dst)       == 0u);
    REQUIRE(HA::dimstyleCtrl(dst)    == 0u);
    REQUIRE(HA::appidCtrl(dst)       == 0u);
    REQUIRE(HA::blockCtrl(dst)       == 0u);
    REQUIRE(HA::viewCtrl(dst)        == 0u);
    REQUIRE(HA::ucsCtrl(dst)         == 0u);
    REQUIRE(HA::vportCtrl(dst)       == 0u);
    REQUIRE(HA::vpEntHeaderCtrl(dst) == 0u);
    REQUIRE(HA::handSeed(dst)        == 0u);
}

TEST_CASE("DRW_Header::encodeDwg round-trips populated vars and control handles",
          "[dwg-write][header-encode]") {
    DRW_Header src;

    // Override a representative spread of vars (BD, BS, BL) to non-default
    // values so the test catches encoder bugs that hardcode defaults.
    src.addDouble("LTSCALE", 4.25, 40);
    src.addDouble("TEXTSIZE", 1.5, 40);
    src.addDouble("DIMSCALE", 0.5, 40);
    src.addDouble("DIMGAP",   0.125, 40);
    src.addInt("LUNITS", 4, 70);
    src.addInt("LUPREC", 6, 70);
    src.addInt("SPLINESEGS", 16, 70);
    src.addInt("MAXACTVP", 32, 70);
    src.addInt("DIMDSEP", static_cast<int>(','), 70);
    src.addInt("DIMLWD", 30, 70);   // 0.30mm
    src.addInt("DIMLWE", 50, 70);   // 0.50mm

    // Control-handle members (member-stored, NOT in vars map).
    HA::linetypeCtrl(src)    = 0x05;
    HA::layerCtrl(src)       = 0x02;
    HA::styleCtrl(src)       = 0x03;
    HA::dimstyleCtrl(src)    = 0x0A;
    HA::appidCtrl(src)       = 0x09;
    HA::blockCtrl(src)       = 0x01;
    HA::viewCtrl(src)        = 0x06;
    HA::ucsCtrl(src)         = 0x07;
    HA::vportCtrl(src)       = 0x08;
    HA::vpEntHeaderCtrl(src) = 0x0B;
    HA::handSeed(src)        = 0x66;  // high-water-mark — preserved on round-trip

    auto bytes = encodeWithSizePrefix(src);

    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Header dst;
    REQUIRE(DrwHeaderEncodeTestAccess::parse(dst, DRW::AC1015, &r, &r));

    REQUIRE(dbl(dst, "LTSCALE")  == 4.25);
    REQUIRE(dbl(dst, "TEXTSIZE") == 1.5);
    REQUIRE(dbl(dst, "DIMSCALE") == 0.5);
    REQUIRE(dbl(dst, "DIMGAP")   == 0.125);
    REQUIRE(i32(dst, "LUNITS")     == 4);
    REQUIRE(i32(dst, "LUPREC")     == 6);
    REQUIRE(i32(dst, "SPLINESEGS") == 16);
    REQUIRE(i32(dst, "MAXACTVP")   == 32);
    REQUIRE(i32(dst, "DIMDSEP")    == static_cast<dint32>(','));
    REQUIRE(i32(dst, "DIMLWD") == 30);
    REQUIRE(i32(dst, "DIMLWE") == 50);

    REQUIRE(HA::linetypeCtrl(dst)    == 0x05u);
    REQUIRE(HA::layerCtrl(dst)       == 0x02u);
    REQUIRE(HA::styleCtrl(dst)       == 0x03u);
    REQUIRE(HA::dimstyleCtrl(dst)    == 0x0Au);
    REQUIRE(HA::appidCtrl(dst)       == 0x09u);
    REQUIRE(HA::blockCtrl(dst)       == 0x01u);
    REQUIRE(HA::viewCtrl(dst)        == 0x06u);
    REQUIRE(HA::ucsCtrl(dst)         == 0x07u);
    REQUIRE(HA::vportCtrl(dst)       == 0x08u);
    REQUIRE(HA::vpEntHeaderCtrl(dst) == 0x0Bu);
    REQUIRE(HA::handSeed(dst)        == 0x66u);
}

TEST_CASE("DRW_Header::encodeDwg round-trips coord vars and the CEPSNTYPE=3 branch",
          "[dwg-write][header-encode]") {
    DRW_Header src;

    // Coord vars exercise the 3BD path; these were unexercised by the
    // earlier round-trip tests (which only set scalar vars).
    src.addCoord("EXTMIN", DRW_Coord(-100.5, -200.25, 0.0), 10);
    src.addCoord("EXTMAX", DRW_Coord(500.0, 300.75, 50.0), 10);
    src.addCoord("INSBASE", DRW_Coord(1.0, 2.0, 3.0), 10);

    // CEPSNTYPE == 3 triggers the optional CPSNID handle emit; the path
    // is currently `hBbuf->putHandle(makeHardPtr(0))` (null), and we
    // verify the parse picks the null up without desyncing the stream.
    src.addInt("CEPSNTYPE", 3, 70);
    // Set a non-default INSUNITS so the surrounding stream is exercised
    // with a real value rather than the encoder default.
    src.addInt("INSUNITS", 4, 70);

    auto bytes = encodeWithSizePrefix(src);

    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Header dst;
    REQUIRE(DrwHeaderEncodeTestAccess::parse(dst, DRW::AC1015, &r, &r));

    auto coord = [&](const char* k) -> DRW_Coord {
        auto it = dst.vars.find(k);
        REQUIRE(it != dst.vars.end());
        REQUIRE(it->second != nullptr);
        REQUIRE(it->second->coord() != nullptr);
        return *it->second->coord();
    };

    auto extmin = coord("EXTMIN");
    REQUIRE(extmin.x == -100.5);
    REQUIRE(extmin.y == -200.25);
    REQUIRE(extmin.z == 0.0);

    auto extmax = coord("EXTMAX");
    REQUIRE(extmax.x == 500.0);
    REQUIRE(extmax.y == 300.75);
    REQUIRE(extmax.z == 50.0);

    auto insbase = coord("INSBASE");
    REQUIRE(insbase.x == 1.0);
    REQUIRE(insbase.y == 2.0);
    REQUIRE(insbase.z == 3.0);

    REQUIRE(i32(dst, "CEPSNTYPE") == 3);
    REQUIRE(i32(dst, "INSUNITS")  == 4);
}

namespace {

/// Capturing iface for the fixture-replay test: copies the parsed
/// DRW_Header for inspection after dwgRW::read returns.
class HeaderCaptureIface : public DRW_Interface {
public:
    DRW_Header m_captured;
    bool m_gotHeader {false};

    void addHeader(const DRW_Header* h) override {
        if (h != nullptr) { m_captured = *h; m_gotHeader = true; }
    }

    // Required iface no-ops.
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addLine(const DRW_Line&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addArc(const DRW_Arc&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addLWPolyline(const DRW_LWPolyline&) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText&) override {}
    void addText(const DRW_Text&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}
    void writeHeader(DRW_Header&) override {}
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
};

std::vector<duint8> slurpFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    in.seekg(0, std::ios::end);
    auto sz = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<duint8> buf(static_cast<size_t>(sz));
    if (sz > 0)
        in.read(reinterpret_cast<char*>(buf.data()), sz);
    return buf;
}

duint32 leU32(const std::vector<duint8>& b, size_t off) {
    return static_cast<duint32>(b[off])
         | (static_cast<duint32>(b[off + 1]) << 8)
         | (static_cast<duint32>(b[off + 2]) << 16)
         | (static_cast<duint32>(b[off + 3]) << 24);
}

} // namespace

// Fixture-replay test.  Hidden by `[.]` tag — runs only when explicitly
// invoked with that tag or with the test's exact name.  Requires the
// developer-local R2000 fixture at ~/doc/dwg2/Patterns-art-block.dwg.
//
// Goal: round-trip a header parsed from a real AutoCAD file through
// encoder→parser and verify selected vars survive.  This is a stronger
// encoder check than the synthetic default-only round-trip because it
// uses real values for every var the parser populates.
//
// Note on byte-exact equality: a true byte-compare against the original
// section payload is NOT achievable yet.  parseDwg discards the 4 leading
// "Unknown BDs" without storing them, and HANDSEED in the file points
// at the doc's real high-water-mark handle (encoder writes null).  Both
// are encoder-replay limitations to revisit in a later phase; for now
// the test reports the section-size delta and the longest common prefix
// as INFO so divergences are visible without failing the test.
TEST_CASE("DRW_Header::encodeDwg replays a real fixture header round-trip",
          "[.dwg_header_fixture_replay]") {
    const char* home = std::getenv("HOME");
    if (home == nullptr) { SUCCEED("HOME not set"); return; }
    const std::string path = std::string(home) + "/doc/dwg2/Patterns-art-block.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("~/doc/dwg2/Patterns-art-block.dwg not found"); return;
    }

    // Slurp the file for the section locator + original HEADER body.
    auto fileBytes = slurpFile(path);
    REQUIRE(fileBytes.size() > 100);
    REQUIRE(std::memcmp(fileBytes.data(), "AC1015", 6) == 0);

    duint32 numSections = leU32(fileBytes, 0x15);
    REQUIRE(numSections >= 3);

    duint32 headerAddr = leU32(fileBytes, 0x19 + 0 * 9 + 1);
    duint32 headerSecSize = leU32(fileBytes, 0x19 + 0 * 9 + 5);
    REQUIRE(headerAddr > 0);
    REQUIRE(headerSecSize >= 38);

    duint32 bodySizeOnDisk = leU32(fileBytes, headerAddr + 16);
    REQUIRE(bodySizeOnDisk + 16 + 4 + 16 + 2 == headerSecSize);
    const duint8* originalBody = fileBytes.data() + headerAddr + 16 + 4;

    // Read the fixture and capture the parsed header.
    HeaderCaptureIface cap;
    dwgR reader(path.c_str());
    bool ok = reader.read(&cap, /*ext=*/false);
    REQUIRE(cap.m_gotHeader);
    (void)ok;

    // Snapshot the captured vars we plan to assert against; the encoder
    // doesn't mutate `cap.m_captured`, but reading via the friend
    // accessor against the same instance keeps the test code explicit.
    auto snapshotDouble = [&](const char* k) -> double { return dbl(cap.m_captured, k); };
    auto snapshotInt    = [&](const char* k) -> dint32 { return i32(cap.m_captured, k); };

    const double origLTSCALE  = snapshotDouble("LTSCALE");
    const double origTEXTSIZE = snapshotDouble("TEXTSIZE");
    const double origDIMSCALE = snapshotDouble("DIMSCALE");
    const double origDIMASZ   = snapshotDouble("DIMASZ");
    const dint32 origLUNITS   = snapshotInt("LUNITS");
    const dint32 origLUPREC   = snapshotInt("LUPREC");
    const dint32 origMAXACTVP = snapshotInt("MAXACTVP");
    const dint32 origDIMDSEP  = snapshotInt("DIMDSEP");
    const duint32 origBlockCtrl    = HA::blockCtrl(cap.m_captured);
    const duint32 origLayerCtrl    = HA::layerCtrl(cap.m_captured);
    const duint32 origLinetypeCtrl = HA::linetypeCtrl(cap.m_captured);

    // Re-encode the captured header.
    dwgBufferW w;
    w.putRawLong32(0);
    REQUIRE(DrwHeaderEncodeTestAccess::encode(cap.m_captured, DRW::AC1015, &w, &w));
    w.alignToByte();
    duint32 reBodySize = static_cast<duint32>(w.size()) - 4;
    w.patchRawLong32(0, reBodySize);

    // Informational: how close are we to the on-disk body?
    duint32 commonPrefix = 0;
    {
        const duint8* reBody = w.data().data() + 4;
        duint32 minSize = std::min(bodySizeOnDisk, reBodySize);
        for (duint32 i = 0; i < minSize; ++i) {
            if (originalBody[i] != reBody[i]) break;
            ++commonPrefix;
        }
    }
    INFO("original HEADER body bytes:   " << bodySizeOnDisk);
    INFO("re-encoded HEADER body bytes: " << reBodySize);
    INFO("longest common prefix bytes:  " << commonPrefix);

    // Re-parse the re-encoded body and confirm the round-trip preserves
    // the vars we snapshotted.  This is the substantive check: it doesn't
    // depend on AutoCAD's specific compression choices but still catches
    // encoder bugs that would scramble downstream vars.
    dwgBuffer r(w.data().data(), w.size());
    DRW_Header rt;
    REQUIRE(DrwHeaderEncodeTestAccess::parse(rt, DRW::AC1015, &r, &r));

    REQUIRE(dbl(rt, "LTSCALE")  == origLTSCALE);
    REQUIRE(dbl(rt, "TEXTSIZE") == origTEXTSIZE);
    REQUIRE(dbl(rt, "DIMSCALE") == origDIMSCALE);
    REQUIRE(dbl(rt, "DIMASZ")   == origDIMASZ);
    REQUIRE(i32(rt, "LUNITS")   == origLUNITS);
    REQUIRE(i32(rt, "LUPREC")   == origLUPREC);
    REQUIRE(i32(rt, "MAXACTVP") == origMAXACTVP);
    REQUIRE(i32(rt, "DIMDSEP")  == origDIMDSEP);
    REQUIRE(HA::blockCtrl(rt)    == origBlockCtrl);
    REQUIRE(HA::layerCtrl(rt)    == origLayerCtrl);
    REQUIRE(HA::linetypeCtrl(rt) == origLinetypeCtrl);

    // Section-size sanity: the encoder should land within a reasonable
    // band of the original — gross size divergence indicates a missing
    // var or wrong primitive width.  Encoder may produce a smaller body
    // (more aggressive compression of zero/default values) so the lower
    // bound is looser than the upper.
    REQUIRE(reBodySize >= bodySizeOnDisk / 2);
    REQUIRE(reBodySize <= bodySizeOnDisk * 3 / 2);
}

// R2004 (AC1018) omits vpEntHeaderCtrl from the control-handle block.
// Verify that encoding with AC1018 drops that handle so the parser sees
// the correct offset for all subsequent handles (linetypeCtrl, layerCtrl, …).
TEST_CASE("DRW_Header::encodeDwg R2004 omits vpEntHeaderCtrl",
          "[dwg-write][header-encode]") {
    DRW_Header src;

    HA::blockCtrl(src)       = 0x01;
    HA::layerCtrl(src)       = 0x02;
    HA::styleCtrl(src)       = 0x03;
    HA::linetypeCtrl(src)    = 0x05;
    HA::viewCtrl(src)        = 0x06;
    HA::ucsCtrl(src)         = 0x07;
    HA::vportCtrl(src)       = 0x08;
    HA::appidCtrl(src)       = 0x09;
    HA::dimstyleCtrl(src)    = 0x0A;
    HA::vpEntHeaderCtrl(src) = 0x0B;  // R2000-only; must NOT survive AC1018 round-trip
    HA::handSeed(src)        = 0x66;

    auto bytes = encodeWithSizePrefix(src, DRW::AC1018);

    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Header dst;
    REQUIRE(DrwHeaderEncodeTestAccess::parse(dst, DRW::AC1018, &r, &r));

    // All 9 R2004 control handles must round-trip correctly.
    REQUIRE(HA::blockCtrl(dst)    == 0x01u);
    REQUIRE(HA::layerCtrl(dst)    == 0x02u);
    REQUIRE(HA::styleCtrl(dst)    == 0x03u);
    REQUIRE(HA::linetypeCtrl(dst) == 0x05u);
    REQUIRE(HA::viewCtrl(dst)     == 0x06u);
    REQUIRE(HA::ucsCtrl(dst)      == 0x07u);
    REQUIRE(HA::vportCtrl(dst)    == 0x08u);
    REQUIRE(HA::appidCtrl(dst)    == 0x09u);
    REQUIRE(HA::dimstyleCtrl(dst) == 0x0Au);
    REQUIRE(HA::handSeed(dst)     == 0x66u);
    // vpEntHeaderCtrl is not written or read for AC1018 — dst must stay zero.
    REQUIRE(HA::vpEntHeaderCtrl(dst) == 0u);
}
