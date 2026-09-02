/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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
 * Per-DRW_*-OBJECT round-trip tests. Mirrors
 * dwg_entity_encode_round_trip_tests.cpp for the OBJECTS section
 * (DICTIONARY / XRECORD / LAYOUT / GROUP / SCALE / ...).
 *
 * Pattern: build a `dwgBufferW` with the same byte stream the real encoder
 * will eventually produce, parse it back via the friend-accessor, and
 * assert the resulting struct's fields match the source.  When the real
 * encoder lands (Phase B of the OBJECTS support plan), the in-test helper
 * here gets replaced by `DrwObjectEncodeTestAccess::encode`.
 *
 * Legacy object tests include AC1014/AC1015 because object-size anchoring is
 * already present in R13/R14, while string fields stay inline before R2007.
 * AC1018/AC1024 require a separate string buffer and are covered when the
 * encoder lands.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using Catch::Approx;

#include "drw_base.h"
#include "drw_classes.h"
#include "drw_entities.h"
#include "drw_objects.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgreader.h"
#include "intern/dwgsafety.h"
#include "intern/dwgwriter18.h"

class DwgWriter18AppInfoTestAccess : public dwgWriter18 {
public:
    using dwgWriter18::buildAppInfoContent;
};

TEST_CASE("dwgBufferW records and rolls back handle occurrences",
          "[dwg-write][buffer][handles]") {
    dwgBufferW writer;
    writer.putBit(1);

    dwgHandle first;
    first.code = DRW::DwgSoftPointer;
    first.ref = 0x1234u;
    first.ref64 = first.ref;
    writer.putHandle(first);

    REQUIRE(writer.handleOccurrences().size() == 1u);
    CHECK(writer.handleOccurrences().front().code == DRW::DwgSoftPointer);
    CHECK(writer.handleOccurrences().front().reference == 0x1234u);
    CHECK(writer.handleOccurrences().front().startBit == 1u);
    CHECK(writer.handleOccurrences().front().endBit
          > writer.handleOccurrences().front().startBit);

    const std::size_t firstSize = writer.size();
    writer.putFixedHandle(DRW::DwgHardPointer, 5, 0x100000001ULL);
    REQUIRE(writer.handleOccurrences().size() == 2u);
    CHECK(writer.handleOccurrences().back().reference == 0x100000001ULL);

    writer.truncate(firstSize);
    REQUIRE(writer.handleOccurrences().size() == 1u);
    CHECK(writer.handleOccurrences().front().reference == 0x1234u);

    writer.reset();
    CHECK(writer.handleOccurrences().empty());
}

// Friend accessor for the protected parseDwg / encodeDwg + private
// post-parse fields. Declared as a friend by drw_objects.h via SETOBJFRIENDS.
class DrwObjectEncodeTestAccess {
public:
    static bool readCommonHandles(
        DRW_TableEntry& entry, DRW::Version version, dwgBuffer* buffer,
        std::uint32_t* parentHandle,
        std::vector<std::uint32_t>* reactorHandles,
        std::uint32_t* xDictHandle) {
        return entry.readDwgObjectCommonHandles(
            version, buffer, parentHandle, reactorHandles, xDictHandle);
    }

    static bool parse(DRW_TableEntry& e, DRW::Version v, dwgBuffer* buf) {
        std::uint32_t bs = 0;
        return e.parseDwg(v, buf, bs);
    }
    static bool parse(DRW_TableEntry& e, DRW::Version v, dwgBuffer* buf,
                      std::uint32_t bs) {
        return e.parseDwg(v, buf, bs);
    }
    static bool encodeDimensionAssociation(
        const DRW_DimensionAssociation& e, DRW::Version v, dwgBufferW* buf,
        dwgBufferW* strBuf = nullptr, dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeEvaluationGraph(
        const DRW_EvaluationGraph& e, DRW::Version v, dwgBufferW* buf,
        dwgBufferW* strBuf = nullptr, dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseBlockRecord(DRW_Block_Record& e, DRW::Version v,
                                 dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseBlockRecord(DRW_Block_Record& e, DRW::Version v,
                                 dwgBuffer* buf, std::uint32_t bs) {
        return e.parseDwg(v, buf, bs);
    }
    static const std::vector<std::uint32_t>& blockEntityHandles(
        const DRW_Block_Record& e) {
        return e.entMap;
    }
    static std::uint32_t blockHandle(const DRW_Block_Record& e) {
        return e.block;
    }
    static std::uint32_t endBlockHandle(const DRW_Block_Record& e) {
        return e.endBlock;
    }
    static bool parseTextstyle(DRW_Textstyle& e, DRW::Version v,
                               dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseLtype(DRW_LType& e, DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeLtype(const DRW_LType& e, DRW::Version v,
                            dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                            dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseLayer(DRW_Layer& e, DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeLayer(const DRW_Layer& e, DRW::Version v,
                            dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                            dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseAppId(DRW_AppId& e, DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeAppId(const DRW_AppId& e, DRW::Version v,
                            dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                            dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseDimstyle(DRW_Dimstyle& e, DRW::Version v,
                              dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseDimstyle(DRW_Dimstyle& e, DRW::Version v,
                              dwgBuffer* buf, std::uint32_t bs) {
        return e.parseDwg(v, buf, bs);
    }
    static bool encodeDimstyle(const DRW_Dimstyle& e, DRW::Version v,
                               dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                               dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeTextstyle(const DRW_Textstyle& e, DRW::Version v,
                                dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                                dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseViewportEntityHeader(DRW_ViewportEntityHeader& e,
                                          DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeViewportEntityHeader(
        const DRW_ViewportEntityHeader& e, DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool parseVxControl(DRW_VxControl& e, DRW::Version v,
                               dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeVxControl(const DRW_VxControl& e, DRW::Version v,
                                dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                                dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseAssociative(DRW_AssociativeObject& e, DRW::Version v,
                                 dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseDynamicBlock(DRW_DynamicBlockObject& e, DRW::Version v,
                                  dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseBlockRepresentationData(
        DRW_BlockRepresentationData& e, DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseVxTableRecord(DRW_VxTableRecord& e, DRW::Version v,
                                   dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeVxTableRecord(const DRW_VxTableRecord& e,
                                    DRW::Version v, dwgBufferW* buf,
                                    dwgBufferW* strBuf = nullptr,
                                    dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseTvDeviceProperties(DRW_TvDeviceProperties& e,
                                        DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeTvDeviceProperties(
        const DRW_TvDeviceProperties& e, DRW::Version v, dwgBufferW* buf,
        dwgBufferW* strBuf = nullptr, dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseCsacDocumentOptions(DRW_CsacDocumentOptions& e,
                                         DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseContextDataManager(DRW_ContextDataManager& e,
                                        DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseVbaProject(DRW_VbaProject& e, DRW::Version v,
                                dwgBuffer* buf, std::uint32_t bs = 0) {
        return e.parseDwg(v, buf, bs);
    }
    static bool parseObjectContextData(DRW_ObjectContextData& e,
                                       DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseBackground(DRW_Background& e, DRW::Version v,
                                dwgBuffer* buf, std::uint32_t bs = 0) {
        return e.parseDwg(v, buf, bs);
    }
    static bool encodeBackground(const DRW_Background& e, DRW::Version v,
                                 dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                                 dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseSunStudy(DRW_SunStudy& e, DRW::Version v,
                              dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeSunStudy(const DRW_SunStudy& e, DRW::Version v,
                               dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                               dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseMotionPath(DRW_MotionPath& e, DRW::Version v,
                                dwgBuffer* buf, std::uint32_t bs = 0) {
        return e.parseDwg(v, buf, bs);
    }
    static bool encodeMotionPath(const DRW_MotionPath& e, DRW::Version v,
                                 dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                                 dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseCurvePath(DRW_CurvePath& e, DRW::Version v,
                               dwgBuffer* buf, std::uint32_t bs = 0) {
        return e.parseDwg(v, buf, bs);
    }
    static bool encodeCurvePath(const DRW_CurvePath& e, DRW::Version v,
                                dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                                dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parsePointPath(DRW_PointPath& e, DRW::Version v,
                               dwgBuffer* buf, std::uint32_t bs = 0) {
        return e.parseDwg(v, buf, bs);
    }
    static bool encodePointPath(const DRW_PointPath& e, DRW::Version v,
                                dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                                dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseObjectPtr(DRW_ObjectPtr& e, DRW::Version v,
                               dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeObjectPtr(const DRW_ObjectPtr& e, DRW::Version v,
                                dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                                dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parsePartialViewingIndex(DRW_PartialViewingIndex& e,
                                         DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parsePartialViewingIndex(DRW_PartialViewingIndex& e,
                                         DRW::Version v, dwgBuffer* buf,
                                         std::uint32_t bs) {
        return e.parseDwg(v, buf, bs);
    }
    static bool encodePartialViewingIndex(
        const DRW_PartialViewingIndex& e, DRW::Version v, dwgBufferW* buf,
        dwgBufferW* strBuf = nullptr, dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseRenderSettings(DRW_RenderSettings& e, DRW::Version v,
                                    dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool encodeRenderSettings(const DRW_RenderSettings& e,
                                     DRW::Version v, dwgBufferW* buf,
                                     dwgBufferW* strBuf = nullptr,
                                     dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseVisualStyle(DRW_VisualStyle& e, DRW::Version v,
                                 dwgBuffer* buf, std::uint32_t bs = 0) {
        return e.parseDwg(v, buf, bs);
    }
    static bool encodeVisualStyle(const DRW_VisualStyle& e, DRW::Version v,
                                  dwgBufferW* buf,
                                  dwgBufferW* strBuf = nullptr,
                                  dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool parseSection(DRW_Section& e, DRW::Version v, dwgBuffer* buf) {
        return e.parseDwg(v, buf, 0);
    }
    static bool parseSection(DRW_Section& e, DRW::Version v, dwgBuffer* buf,
                             std::uint32_t bs) {
        return e.parseDwg(v, buf, bs);
    }
    static bool encodeSection(const DRW_Section& e, DRW::Version v,
                              dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                              dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeDictionary(const DRW_Dictionary& e, DRW::Version v,
                                  dwgBufferW* buf,
                                  dwgBufferW* strBuf = nullptr,
                                  dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeScale(const DRW_Scale& e, DRW::Version v,
                             dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeScaleModern(const DRW_Scale& e, DRW::Version v,
                                  dwgBufferW* buf, dwgBufferW* strBuf) {
        return e.encodeDwg(v, buf, strBuf);
    }
    static bool encodeVport(const DRW_Vport& e, DRW::Version v,
                             dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeVport(const DRW_Vport& e, DRW::Version v,
                            dwgBufferW* buf, dwgBufferW* strBuf,
                            dwgBufferW* handleBuf) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeGroup(const DRW_Group& e, DRW::Version v,
                             dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeXRecord(const DRW_XRecord& e, DRW::Version v,
                               dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodePlotSettings(const DRW_PlotSettings& e, DRW::Version v,
                                   dwgBufferW* buf,
                                   dwgBufferW* strBuf = nullptr,
                                   dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeLayout(const DRW_Layout& e, DRW::Version v,
                              dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeLayout(const DRW_Layout& e, DRW::Version v,
                             dwgBufferW* buf, dwgBufferW* strBuf,
                             dwgBufferW* handleBuf) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeUcs(const DRW_UCS& e, DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeView(const DRW_View& e, DRW::Version v,
                           dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeView(const DRW_View& e, DRW::Version v,
                           dwgBufferW* buf, dwgBufferW* strBuf,
                           dwgBufferW* handleBuf) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeMLineStyle(const DRW_MLineStyle& e, DRW::Version v,
                                 dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeRasterVariables(const DRW_RasterVariables& e,
                                       DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeWipeoutVariables(const DRW_WipeoutVariables& e,
                                        DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeSortEntsTable(const DRW_SortEntsTable& e,
                                     DRW::Version v, dwgBufferW* buf,
                                     dwgBufferW* strBuf = nullptr,
                                     dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeSpatialFilter(const DRW_SpatialFilter& e,
                                     DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeGeoData(const DRW_GeoData& e, DRW::Version v,
                               dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeMLeaderStyle(const DRW_MLeaderStyle& e, DRW::Version v,
                                   dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeUnderlayDefinition(const DRW_UnderlayDefinition& e,
                                         DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeUnderlayDefinitionModern(const DRW_UnderlayDefinition& e,
                                               DRW::Version v, dwgBufferW* buf,
                                               dwgBufferW* strBuf,
                                               dwgBufferW* handleBuf) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeAcDbPlaceholder(const DRW_AcDbPlaceholder& e,
                                      DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeVbaProject(const DRW_VbaProject& e, DRW::Version v,
                                 dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeSun(const DRW_Sun& e, DRW::Version v,
                          dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeDictionaryVar(const DRW_DictionaryVar& e, DRW::Version v,
                                     dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeMaterial(const DRW_Material& e, DRW::Version v,
                               dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                               dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeDictionaryWDflt(const DRW_DictionaryWithDefault& e,
                                       DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeIDBuffer(const DRW_IDBuffer& e, DRW::Version v,
                                dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeLayerIndex(const DRW_LayerIndex& e, DRW::Version v,
                                  dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeImageDef(const DRW_ImageDef& e, DRW::Version v,
                               dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                               dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeDbColor(const DRW_DbColor& e, DRW::Version v,
                              dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                              dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeImageDefinitionReactor(
        const DRW_ImageDefinitionReactor& e, DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodePointCloudDef(const DRW_PointCloudDef& e, DRW::Version v,
                                    dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeNavisworksModelDef(const DRW_NavisworksModelDef& e,
                                         DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodePointCloudColorMap(const DRW_PointCloudColorMap& e,
                                         DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeSpatialIndex(const DRW_SpatialIndex& e, DRW::Version v,
                                    dwgBufferW* buf,
                                    dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, nullptr, handleBuf);
    }
    static bool encodeField(const DRW_Field& e, DRW::Version v,
                             dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                             dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static bool encodeFieldList(const DRW_FieldList& e, DRW::Version v,
                                 dwgBufferW* buf, dwgBufferW* strBuf = nullptr,
                                 dwgBufferW* handleBuf = nullptr) {
        return e.encodeDwg(v, buf, strBuf, handleBuf);
    }
    static std::int16_t oType(const DRW_TableEntry& e) { return e.oType; }
    static void setNumReactors(DRW_TableEntry& e, std::int32_t n) { e.numReactors = n; }
    static void setXDictFlag(DRW_TableEntry& e, std::uint8_t f) { e.xDictFlag = f; }
    static std::uint8_t xDictFlag(const DRW_TableEntry& e) { return e.xDictFlag; }
    static std::int32_t numReactors(const DRW_TableEntry& e) { return e.numReactors; }
    // Phase 3A.0 — DIMSTYLE struct->vars sync (syncStructToVars is public, but
    // the wrapper keeps the test call site uniform with the access pattern).
    static void syncDimstyle(DRW_Dimstyle& d) { d.syncStructToVars(); }
};

class TableEntryProbe final : public DRW_TableEntry {
protected:
    bool parseDwg(DRW::Version version, dwgBuffer* buffer,
                  std::uint32_t bs = 0) override {
        return DRW_TableEntry::parseDwg(version, buffer, nullptr, bs);
    }
};

namespace {

class DispatchCapture final : public DRW_Interface {
public:
    int objectContextCallbacks = 0;
    int underlayDefinitionCallbacks = 0;
    int rawObjectCallbacks = 0;

    void addObjectContextData(const DRW_ObjectContextData&) override {
        ++objectContextCallbacks;
    }
    void linkUnderlay(const DRW_UnderlayDefinition*) override {
        ++underlayDefinitionCallbacks;
    }
    void addUnsupportedObject(const DRW_UnsupportedObject&) override {
        ++rawObjectCallbacks;
    }

    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addLine(const DRW_Line&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
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
    void addDimArc(const DRW_DimArc*) override {}
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

class DispatchReader final : public dwgReader {
public:
    using dwgReader::readDwgEntity;

    DispatchReader(std::uint8_t* data, std::size_t size)
        : dwgReader(std::make_unique<dwgBuffer>(data, size), nullptr) {
        version = DRW::AC1018;
    }

    bool parseObject(dwgBuffer* buffer, objHandle& object,
                     DRW_Interface& interface_) {
        return readDwgObject(buffer, object, interface_);
    }

    void addClass(std::uint32_t number, const DRW_Class& data) {
        classesmap[number] = new DRW_Class(data);
    }

protected:
    bool readMetaData() override { return false; }
    bool readFileHeader() override { return false; }
    bool readDwgHeader(DRW_Header&) override { return false; }
    bool readDwgClasses() override { return false; }
    bool readDwgHandles() override { return false; }
    bool readDwgTables(DRW_Header&) override { return false; }
    bool readDwgBlocks(DRW_Interface&) override { return false; }
    bool readDwgEntities(DRW_Interface&) override { return false; }
    bool readDwgObjects(DRW_Interface&) override { return false; }
};

dwgHandle nullHandle();
dwgHandle hardPtr(std::uint32_t ref);

std::vector<std::uint8_t> makeObjectFrame(
    const std::vector<std::uint8_t>& body) {
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(body.size()));
    frame.putBytes(body.data(), body.size());
    auto bytes = frame.data();
    dwgBuffer crcBuffer(bytes.data(), bytes.size());
    const std::uint16_t crc = crcBuffer.crc8(
        0xC0C1, 0, static_cast<std::int32_t>(bytes.size()));
    bytes.push_back(static_cast<std::uint8_t>(crc & 0xFF));
    bytes.push_back(static_cast<std::uint8_t>(crc >> 8));
    return bytes;
}

std::vector<std::uint8_t> makeObjectContextFrame(bool includeHandles) {
    constexpr DRW::Version version = DRW::AC1018;
    constexpr std::uint16_t classNumber = 501;
    dwgBufferW body;
    body.putObjType(version, classNumber);
    const std::size_t objectSizeBit = body.bitCount();
    body.putRawLong32(0); // object-size-in-bits placeholder
    body.putHandle(hardPtr(0x55));
    body.putBitShort(0);  // empty EED
    body.putBitLong(0);   // no reactors
    body.putBit(includeHandles ? 0 : 1); // xdictionary flag
    body.putBitShort(1);  // class version
    body.putBit(0);       // default flag
    body.patchRawLong32AtBit(objectSizeBit, body.bitCount());
    if (includeHandles) {
        body.putHandle(hardPtr(0x44));
        body.putHandle(nullHandle());
    }
    return makeObjectFrame(body.data());
}

std::vector<std::uint8_t> snapshot(const dwgBufferW& w) { return w.data(); }

std::size_t objectSizeBitOffset(const dwgBufferW& data) {
    if (data.data().empty())
        return 2;
    const std::uint8_t bsCode = (data.data()[0] >> 6) & 0x03;
    return bsCode == 0x01 ? 10 : (bsCode == 0x00 ? 18 : 2);
}

// Build a null handle (code 0, size 0).  Parser will read ref=0 back.
dwgHandle nullHandle() {
    dwgHandle h;
    h.code = 0;
    h.size = 0;
    h.ref  = 0;
    return h;
}

// Build a hard-pointer (code 4) handle.  Mirrors makeHardPtr in
// dwgwriter15.cpp (file-local there; inlined here).
dwgHandle hardPtr(std::uint32_t ref) {
    dwgHandle h;
    h.code = (ref == 0) ? 0 : 4;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        std::uint32_t t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

dwgHandle handleWithCode(std::uint8_t code, std::uint32_t ref) {
    dwgHandle h = hardPtr(ref);
    h.code = ref == 0 ? 0 : code;
    return h;
}

// Emit the common DRW_TableEntry preamble used by every OBJECTS-section
// record. Mirrors dwgwriter15.cpp::emitRecordPreamble for AC1015 (R2000),
// which is what DRW_TableEntry::parseDwg consumes in version < AC1024
// without the string-buffer split.
//
// `numReactors` and `xDictFlag` configure the corresponding preamble fields;
// the caller is responsible for emitting matching parent/reactor/xdic
// handles at the head of the post-body handle stream.
void emitObjectPreamble(dwgBufferW& body, DRW::Version version,
                        std::uint16_t oType, std::uint32_t handle,
                        std::int32_t numReactors = 0, std::uint8_t xDictFlag = 0) {
    body.putObjType(version, oType);
    if (version > DRW::AC1014 && version < DRW::AC1024) {
        // RL 32-bit obj-size-in-bits placeholder; parser reads but does not
        // need it when the inline legacy handle stream follows the body.
        body.putRawLong32(0);
    }
    body.putHandle(hardPtr(handle));
    body.putBitShort(0);            // EED size = 0
    body.putBitLong(numReactors);
    if (version > DRW::AC1015) {
        body.putBit(xDictFlag);     // xDictFlag (R2004+)
    }
}

void emitLegacyTableStyleRow(dwgBufferW& body, DRW::Version version,
                             int colorIndex) {
    body.putBitDouble(1.0);
    body.putBitShort(2);
    body.putCmColor(version, static_cast<std::uint16_t>(colorIndex));
    body.putCmColor(version, static_cast<std::uint16_t>(colorIndex + 1));
    body.putBit(1);
    for (int i = 0; i < 6; ++i) {
        body.putBitShort(i + 1);
        body.putBit(1);
        body.putCmColor(version, static_cast<std::uint16_t>(colorIndex + i + 2));
    }
}

// R13/R14 table entries carry the legacy object-size RL after the common
// preamble.  Keep this helper separate from the R2000+ preamble above so a
// legacy dictionary test exercises the same cursor sequence as the reader.
void emitLegacyObjectPreamble(dwgBufferW& body, DRW::Version version,
                               std::uint16_t oType, std::uint32_t handle,
                               std::int32_t numReactors = 0) {
    body.putObjType(version, oType);
    body.putHandle(hardPtr(handle));
    body.putBitShort(0);       // EED size = 0
    body.putRawLong32(0);      // legacy object size in bits
    body.putBitLong(numReactors); // numReactors
}

// Emit the common handle-stream prefix: parentHandle + numReactors reactor
// handles + xdic (when xDictFlag != 1).  Must precede any type-specific
// handles in the post-body handle stream so the parser's
// readCommonObjectHandles call lands its consumption in the right slot.
void emitCommonHandlePrefix(dwgBufferW& body, std::uint32_t parentHandle,
                            const std::vector<std::uint32_t>& reactorHandles,
                            std::uint8_t xDictFlag) {
    body.putHandle(hardPtr(parentHandle));
    for (std::uint32_t ref : reactorHandles) {
        body.putHandle(hardPtr(ref));
    }
    if (xDictFlag != 1) {
        body.putHandle(nullHandle());      // xdic
    }
}

// Object handle streams keep the owner offset-resolved, but reactor and
// xdictionary references are read as absolute handles.  Codes A/C make that
// distinction observable in a regression fixture.
void emitObjectHandlePrefix(dwgBufferW& body, std::uint32_t parentHandle,
                            const std::vector<std::uint32_t>& reactorHandles,
                            std::uint32_t xDictHandle,
                            std::uint8_t xDictFlag) {
    body.putHandle(hardPtr(parentHandle));
    for (std::uint32_t ref : reactorHandles)
        body.putHandle(handleWithCode(0xA, ref));
    if (xDictFlag != 1)
        body.putHandle(handleWithCode(0xC, xDictHandle));
}

TEST_CASE("DWG BLOCKREPRESENTATIONDATA decodes fixed body and object handles",
          "[dwg-read][object-encode][block-representation]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/1120, /*handle=*/0x500,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    body.putBitShort(7); // value70 / flag
    emitObjectHandlePrefix(body, 0x20, {0x30}, 0x40, /*xDictFlag=*/0);
    body.putHandle(handleWithCode(3, 0x50)); // block reference

    const auto bytes = body.data();
    dwgBuffer reader(const_cast<std::uint8_t *>(bytes.data()), bytes.size());
    DRW_BlockRepresentationData parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseBlockRepresentationData(
        parsed, version, &reader));
    CHECK(parsed.handle == 0x500u);
    CHECK(parsed.parentHandle == 0x20u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x30u);
    CHECK(parsed.xDictHandle == 0x40u);
    CHECK(parsed.m_flag == 7u);
    CHECK(parsed.m_blockHandle == 0x50u);

    auto truncatedBytes = bytes;
    truncatedBytes.pop_back();
    dwgBuffer truncatedReader(
        const_cast<std::uint8_t *>(truncatedBytes.data()),
        truncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseBlockRepresentationData(
        parsed, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_flag == 0u);
    CHECK(parsed.m_blockHandle == 0u);
}

std::vector<std::uint8_t> emitAc1018ViewStyleObject(
    bool section, double firstNumericField = 0.0) {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW data;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0,
                       /*handle=*/0x650, /*numReactors=*/1,
                       /*xDictFlag=*/0);

    const auto putColor = [&data, version]() {
        data.putCmColor(version, 256);
    };
    const auto putEmptyText = [&data, version]() {
        data.putVariableText(version, "");
    };

    data.putBitShort(0); // AcDbModelDocViewStyle class version
    putEmptyText();      // description
    data.putBit(0);      // modified for recompute
    data.putBitShort(0); // style class version
    data.putBitLong(0);  // flags

    if (!section) {
        putColor();
        data.putBitDouble(firstNumericField);
        putEmptyText();
        data.putBitDouble(0.0);
        data.putRawChar8(0);
        putColor();
        data.putBitDouble(0.0);
        data.putBitLong(0);
        putColor();
        putColor();
        data.putBitDouble(0.0);
        data.putBitLong(0);
        data.putBitDouble(0.0);
        data.putBitLong(0);
        putEmptyText();
        data.putBitLong(0);
        putColor();
        data.putBitLong(0);
        putColor();
        data.putRawChar8(0);
    } else {
        putColor();
        data.putBitDouble(firstNumericField);
        putColor();
        data.putBitDouble(0.0);
        putEmptyText();
        data.putBitDouble(0.0);
        data.putBitLong(0);
        putColor();
        data.putBitLong(0);
        putColor();
        data.putBitDouble(0.0);
        data.putBitDouble(0.0);
        putColor();
        data.putBitDouble(0.0);
        data.putBitLong(0);
        data.putBitDouble(0.0);
        data.putBitLong(0);
        putEmptyText();
        putColor();
        putColor();
        putEmptyText();
        data.putBitDouble(0.0);
        data.putBitLong(0);
        data.putBit(0);
        data.putBit(0);
        data.putBitLong(0);
        data.putBitDouble(0.0);
        data.putBitLong(0);
        data.putBitDouble(0.0);
        data.putBitLong(0); // no hatch angles
    }

    data.alignToByte();
    data.patchRawLong32AtBit(objectSizeBitOffset(data), data.bitCount());
    emitObjectHandlePrefix(handles, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{0x43}, /*xDictHandle=*/0x44,
                           /*xDictFlag=*/0);
    for (int i = 0; i < 6; ++i)
        handles.putHandle(nullHandle());
    handles.alignToByte();

    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1015VisualStyleObject(
    const DRW_VisualStyle& source) {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW data;
    dwgBufferW writerHandles;
    emitObjectPreamble(data, version, DRW_VisualStyle::kDwgClassNum,
                       source.handle, source.reactorCount(),
                       source.extensionDictionaryFlag());
    if (!DrwObjectEncodeTestAccess::encodeVisualStyle(
            source, version, &data, nullptr, &writerHandles))
        return {};

    data.alignToByte();
    data.patchRawLong32AtBit(objectSizeBitOffset(data), data.bitCount());
    dwgBufferW objectHandles;
    emitObjectHandlePrefix(objectHandles,
                           static_cast<std::uint32_t>(source.parentHandle),
                           source.reactorHandles,
                           static_cast<std::uint32_t>(source.xDictHandle),
                           source.extensionDictionaryFlag());
    objectHandles.alignToByte();
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), objectHandles.data().begin(),
                 objectHandles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1018DbColorObject() {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW data;
    emitObjectPreamble(data, version, DRW_DbColor::kDwgType,
                       /*handle=*/0x660, /*numReactors=*/1,
                       /*xDictFlag=*/0);
    data.putCmColor(version, 7);
    data.alignToByte();
    data.patchRawLong32AtBit(objectSizeBitOffset(data), data.bitCount());
    dwgBufferW handles;
    emitObjectHandlePrefix(handles, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{0x43}, /*xDictHandle=*/0x44,
                           /*xDictFlag=*/0);
    handles.alignToByte();
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

// Assemble the R2007 object frame used by dwgWriter21::finishObject.  The
// parser uses the data-section bit size to locate both the detached string
// footer and the following handle stream, so testing only encodeDwg() would
// miss the version-specific framing contract.
std::vector<std::uint8_t> emitAc1021VportObject(const DRW_Vport& source) {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0x41 /* VPORT */,
                       source.handle, /*numReactors=*/1,
                       /*xDictFlag=*/0);
    if (!DrwObjectEncodeTestAccess::encodeVport(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    if (stringBytes != 0) {
        for (int i = 0; i < 7; ++i)
            data.putBit(0);
        const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
        if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
            return {};
        data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
        data.putBit(1);
    }
    data.alignToByte();
    handles.alignToByte();

    if (data.data().empty())
        return {};
    const std::uint8_t bsCode = (data.data()[0] >> 6) & 0x03;
    const std::size_t rlBitOffset =
        (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
    data.patchRawLong32AtBit(rlBitOffset,
                             static_cast<std::uint32_t>(data.size() * 8));

    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1021ViewObject(const DRW_View& source) {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0x3D /* VIEW */,
                       source.handle,
                       static_cast<std::int32_t>(source.reactorHandles.size()),
                       source.extensionDictionaryFlag());
    if (!DrwObjectEncodeTestAccess::encodeView(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    if (stringBytes != 0) {
        for (int i = 0; i < 7; ++i)
            data.putBit(0);
        const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
        if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
            return {};
        data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
        data.putBit(1);
    }
    data.alignToByte();
    handles.alignToByte();

    if (data.data().empty())
        return {};
    const std::uint8_t bsCode = (data.data()[0] >> 6) & 0x03;
    const std::size_t rlBitOffset =
        (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
    data.patchRawLong32AtBit(rlBitOffset,
                             static_cast<std::uint32_t>(data.size() * 8));

    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1021TextStyleObject(
    const DRW_Textstyle& source) {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0x35 /* STYLE */,
                       source.handle,
                       static_cast<std::int32_t>(source.reactorHandles.size()),
                       source.extensionDictionaryFlag());
    if (!DrwObjectEncodeTestAccess::encodeTextstyle(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return {};
    data.putRawShort16(static_cast<std::uint16_t>(
        stringBytes == 0 ? 0 : stringBitSize));
    data.putBit(stringBytes == 0 ? 0 : 1);
    data.alignToByte();
    handles.alignToByte();

    if (data.data().empty())
        return {};
    data.patchRawLong32AtBit(
        objectSizeBitOffset(data), static_cast<std::uint32_t>(data.size() * 8));
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1021AppIdObject(const DRW_AppId& source) {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0x0D /* APPID */,
                       source.handle, /*numReactors=*/0, /*xDictFlag=*/0);
    if (!DrwObjectEncodeTestAccess::encodeAppId(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return {};
    data.putRawShort16(static_cast<std::uint16_t>(
        stringBytes == 0 ? 0 : stringBitSize));
    data.putBit(stringBytes == 0 ? 0 : 1);
    data.alignToByte();
    handles.alignToByte();

    if (data.data().empty())
        return {};
    data.patchRawLong32AtBit(
        objectSizeBitOffset(data), static_cast<std::uint32_t>(data.size() * 8));
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1021LtypeObject(const DRW_LType& source) {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0x39 /* LTYPE */,
                       source.handle,
                       static_cast<std::int32_t>(source.reactorHandles.size()),
                       source.extensionDictionaryFlag());
    if (!DrwObjectEncodeTestAccess::encodeLtype(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    if (stringBytes != 0) {
        for (int i = 0; i < 7; ++i)
            data.putBit(0);
        const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
        if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
            return {};
        data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
        data.putBit(1);
    }
    data.alignToByte();
    handles.alignToByte();
    if (data.data().empty())
        return {};
    data.patchRawLong32AtBit(
        objectSizeBitOffset(data), static_cast<std::uint32_t>(data.size() * 8));

    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1027LayerObject(
    const DRW_Layer& source, std::uint32_t& handleBits) {
    const DRW::Version version = DRW::AC1027;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0x33 /* LAYER */,
                       source.handle,
                       static_cast<std::int32_t>(source.reactorHandles.size()),
                       source.extensionDictionaryFlag());
    data.putBit(0); // has_ds_data (R2013+ common object data)
    if (!DrwObjectEncodeTestAccess::encodeLayer(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return {};
    data.putRawShort16(static_cast<std::uint16_t>(
        stringBytes == 0 ? 0 : stringBitSize));
    data.putBit(stringBytes == 0 ? 0 : 1);
    data.alignToByte();
    handles.alignToByte();
    handleBits = static_cast<std::uint32_t>(handles.data().size() * 8u);

    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1015DimstyleObject(
    const DRW_Dimstyle& source) {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW data;
    emitObjectPreamble(data, version, /*oType=*/0x45 /* DIMSTYLE */,
                       source.handle, /*numReactors=*/0, /*xDictFlag=*/0);
    if (!DrwObjectEncodeTestAccess::encodeDimstyle(
            source, version, &data))
        return {};
    return snapshot(data);
}

std::vector<std::uint8_t> emitR2007DimstyleObject(
    const DRW_Dimstyle& source, DRW::Version version,
    std::uint32_t *handleBits = nullptr) {
    if (version < DRW::AC1021 || version >= DRW::AC1027)
        return {};
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0x45 /* DIMSTYLE */,
                       source.handle, /*numReactors=*/0, /*xDictFlag=*/0);
    if (!DrwObjectEncodeTestAccess::encodeDimstyle(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return {};
    data.putRawShort16(static_cast<std::uint16_t>(
        stringBytes == 0 ? 0 : stringBitSize));
    data.putBit(stringBytes == 0 ? 0 : 1);
    data.alignToByte();
    handles.alignToByte();

    if (data.data().empty())
        return {};
    if (version < DRW::AC1024) {
        data.patchRawLong32AtBit(
            objectSizeBitOffset(data),
            static_cast<std::uint32_t>(data.size() * 8));
    }
    if (handleBits != nullptr)
        *handleBits = static_cast<std::uint32_t>(handles.data().size() * 8);
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1021ImageDefObject(
    const DRW_ImageDef& source) {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0,
                       source.handle, /*numReactors=*/0, /*xDictFlag=*/0);
    if (!DrwObjectEncodeTestAccess::encodeImageDef(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return {};
    data.putRawShort16(static_cast<std::uint16_t>(
        stringBytes == 0 ? 0 : stringBitSize));
    data.putBit(stringBytes == 0 ? 0 : 1);
    data.alignToByte();
    handles.alignToByte();

    if (data.data().empty())
        return {};
    data.patchRawLong32AtBit(
        objectSizeBitOffset(data), static_cast<std::uint32_t>(data.size() * 8));
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1024FieldObject(
    const DRW_Field& source, std::uint32_t& handleBits) {
    const DRW::Version version = DRW::AC1024;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0,
                       source.handle, /*numReactors=*/0, /*xDictFlag=*/1);
    if (!DrwObjectEncodeTestAccess::encodeField(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return {};
    data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    data.putBit(1);
    data.alignToByte();
    handles.alignToByte();

    handleBits = static_cast<std::uint32_t>(handles.data().size() * 8u);
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitVbaProjectObject(DRW::Version version,
                                               const std::vector<std::uint8_t>& payload,
                                               std::uint32_t& handleBits,
                                               std::uint32_t declaredSize = 0) {
    dwgBufferW body;
    dwgBufferW handles;
    emitObjectPreamble(body, version, DRW_VbaProject::kDwgType,
                       /*handle=*/0xA10, /*numReactors=*/0,
                       /*xDictFlag=*/0);
    if (version > DRW::AC1024)
        body.putBit(0); // has_ds_data
    if (declaredSize == 0)
        declaredSize = static_cast<std::uint32_t>(payload.size());
    body.putBitLong(static_cast<std::int32_t>(declaredSize));
    if (!payload.empty())
        body.putBytes(payload.data(), payload.size());

    if (version > DRW::AC1018) {
        body.putBit(0); // no detached strings
        body.putRawShort16(0);
        body.putBit(0);
        body.alignToByte();

        if (version < DRW::AC1024) {
            const std::uint8_t bsCode = (body.data()[0] >> 6) & 0x03;
            const std::size_t rlBitOffset =
                (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
            body.patchRawLong32AtBit(
                rlBitOffset, static_cast<std::uint32_t>(body.bitCount()));
        }

        handles.putHandle(hardPtr(0xA11));
        handles.putHandle(nullHandle());
        handles.alignToByte();
        handleBits = static_cast<std::uint32_t>(handles.size() * 8u);
        std::vector<std::uint8_t> result = body.data();
        result.insert(result.end(), handles.data().begin(), handles.data().end());
        return result;
    }

    const std::uint8_t bsCode = (body.data()[0] >> 6) & 0x03;
    const std::size_t rlBitOffset =
        (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
    body.patchRawLong32AtBit(rlBitOffset,
                             static_cast<std::uint32_t>(body.bitCount()));
    emitCommonHandlePrefix(body, /*parentHandle=*/0xA11, {}, 0);
    handleBits = 0;
    return body.data();
}

std::vector<std::uint8_t> emitAc1021VxObject(
    std::uint16_t objectType, std::uint32_t handle, std::uint32_t classVersion,
    std::uint32_t flags, const std::string& name,
    const std::vector<std::uint32_t>& recordHandles,
    const std::vector<std::uint8_t>& rawData = {}) {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, objectType, handle,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    data.putBitLong(static_cast<std::int32_t>(classVersion));
    data.putBitLong(static_cast<std::int32_t>(flags));
    if (objectType == DRW_VxControl::kDwgClassNum)
        data.putBitLong(static_cast<std::int32_t>(recordHandles.size()));
    if (!rawData.empty())
        data.putBytes(rawData.data(), rawData.size());
    if (!name.empty())
        strings.putVariableText(version, name);
    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return {};
    data.putRawShort16(static_cast<std::uint16_t>(
        stringBytes == 0 ? 0 : stringBitSize));
    data.putBit(stringBytes == 0 ? 0 : 1);
    data.alignToByte();

    handles.putHandle(hardPtr(0x810));       // owner (offset-resolved)
    handles.putHandle(hardPtr(0x811));       // reactor (absolute)
    handles.putHandle(hardPtr(0x812));       // xdictionary (absolute)
    for (std::uint32_t ref : recordHandles)
        handles.putHandle(hardPtr(ref));
    handles.alignToByte();

    if (data.data().empty())
        return {};
    const std::uint8_t bsCode = (data.data()[0] >> 6) & 0x03;
    const std::size_t rlBitOffset =
        (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
    data.patchRawLong32AtBit(rlBitOffset,
                             static_cast<std::uint32_t>(data.size() * 8));
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1021TvDeviceProperties(
    std::int32_t flags, std::int32_t maxRegenThreads,
    std::int32_t useLutPalette, std::uint64_t alternateHighlight,
    std::uint64_t alternateHighlightColor,
    std::uint64_t geometryShaderUsage, std::int32_t blendingMode,
    double antialiasingLevel, double valueBd2) {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW handles;
    emitObjectPreamble(data, version, DRW_TvDeviceProperties::kDwgClassNum,
                       /*handle=*/0x930, /*numReactors=*/1,
                       /*xDictFlag=*/0);
    DRW_TvDeviceProperties properties;
    properties.parentHandle = 0x910;
    properties.reactorHandles = {0x911};
    properties.xDictHandle = 0x912;
    properties.setDwgCommonObjectState(1, 0, false);
    properties.flags = flags;
    properties.maxRegenThreads = maxRegenThreads;
    properties.useLutPalette = useLutPalette;
    properties.alternateHighlight = alternateHighlight;
    properties.alternateHighlightColor = alternateHighlightColor;
    properties.geometryShaderUsage = geometryShaderUsage;
    properties.blendingMode = blendingMode;
    properties.antialiasingLevel = antialiasingLevel;
    properties.valueBd2 = valueBd2;
    if (!properties.encodeDwg(version, &data, nullptr, &handles))
        return {};
    data.alignToByte();
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    data.putRawShort16(0); // no detached strings
    data.putBit(0);
    data.alignToByte();

    handles.alignToByte();

    if (data.data().empty())
        return {};
    const std::uint8_t bsCode = (data.data()[0] >> 6) & 0x03;
    const std::size_t rlBitOffset =
        (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
    data.patchRawLong32AtBit(rlBitOffset,
                             static_cast<std::uint32_t>(data.size() * 8));
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1021CsacDocumentOptions(
    std::uint32_t classVersion, std::uint32_t flags) {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW handles;
    emitObjectPreamble(data, version, DRW_CsacDocumentOptions::kDwgClassNum,
                       /*handle=*/0x950, /*numReactors=*/1,
                       /*xDictFlag=*/0);
    data.putBitLong(static_cast<std::int32_t>(classVersion));
    data.putBitLong(static_cast<std::int32_t>(flags));
    data.alignToByte();
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    data.putRawShort16(0); // no detached strings
    data.putBit(0);
    data.alignToByte();

    handles.putHandle(hardPtr(0x940));
    handles.putHandle(hardPtr(0x941));
    handles.putHandle(hardPtr(0x942));
    handles.alignToByte();

    if (data.data().empty())
        return {};
    const std::uint8_t bsCode = (data.data()[0] >> 6) & 0x03;
    const std::size_t rlBitOffset =
        (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
    data.patchRawLong32AtBit(rlBitOffset,
                             static_cast<std::uint32_t>(data.size() * 8));
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1021SectionObject(
    DRW_Section::Kind kind, const std::vector<std::int32_t>& bodyValues,
    const std::vector<std::uint32_t>& typeSpecificHandles) {
    const DRW::Version version = DRW::AC1021;
    const std::uint16_t objectType = kind == DRW_Section::Manager
        ? DRW_Section::kDwgClassNumManager
        : DRW_Section::kDwgClassNumSettings;
    dwgBufferW data;
    dwgBufferW handles;
    emitObjectPreamble(data, version, objectType, /*handle=*/0xA00,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    if (kind == DRW_Section::Manager) {
        data.putBit(1); // is_live
        data.putBitShort(static_cast<std::uint16_t>(bodyValues.size()));
    } else {
        data.putBitLong(bodyValues.empty() ? 0 : bodyValues[0]); // curr_type
        data.putBitLong(bodyValues.size() < 2 ? 0 : bodyValues[1]); // num_types
    }
    data.alignToByte();
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    data.putRawShort16(0); // no detached strings
    data.putBit(0);
    data.alignToByte();

    handles.putHandle(hardPtr(0xA10)); // owner
    handles.putHandle(hardPtr(0xA11)); // reactor
    handles.putHandle(hardPtr(0xA12)); // xdictionary
    for (std::uint32_t ref : typeSpecificHandles)
        handles.putHandle(hardPtr(ref));
    handles.alignToByte();

    if (data.data().empty())
        return {};
    const std::uint8_t bsCode = (data.data()[0] >> 6) & 0x03;
    const std::size_t rlBitOffset =
        (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
    data.patchRawLong32AtBit(rlBitOffset,
                             static_cast<std::uint32_t>(data.size() * 8));
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1027SectionObject(
    bool hasDsData, std::uint32_t* handleBitSize = nullptr,
    const std::vector<std::uint8_t>& rawTail = {}) {
    const DRW::Version version = DRW::AC1027;
    dwgBufferW data;
    dwgBufferW handles;
    emitObjectPreamble(data, version, DRW_Section::kDwgClassNumSettings,
                       /*handle=*/0xB00, /*numReactors=*/0,
                       /*xDictFlag=*/0);
    data.putBit(hasDsData ? 1 : 0); // AcDb:AcDsPrototype_1b flag
    data.putBitLong(2);             // curr_type
    data.putBitLong(0);             // num_types
    for (std::uint8_t byte : rawTail) {
        for (int bit = 7; bit >= 0; --bit)
            data.putBit((byte >> bit) & 1u);
    }
    data.alignToByte();
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    data.putRawShort16(0); // no detached strings
    data.putBit(0);
    data.alignToByte();

    handles.putHandle(hardPtr(0xB10)); // owner
    handles.putHandle(nullHandle());   // xdictionary
    handles.alignToByte();
    if (handleBitSize != nullptr)
        *handleBitSize = static_cast<std::uint32_t>(handles.size() * 8);

    if (data.data().empty())
        return {};
    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

std::vector<std::uint8_t> emitAc1027VisualStyleObject(
    const DRW_VisualStyle& source, std::uint32_t* handleBitSize = nullptr) {
    const DRW::Version version = DRW::AC1027;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, DRW_VisualStyle::kDwgClassNum,
                       source.handle, source.reactorCount(),
                       source.extensionDictionaryFlag());
    data.putBit(source.hasDataStorageBinaryData() ? 1 : 0);
    if (!DrwObjectEncodeTestAccess::encodeVisualStyle(
            source, version, &data, &strings, &handles))
        return {};

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    if (stringBytes != 0)
        data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return {};
    data.putRawShort16(static_cast<std::uint16_t>(
        stringBytes == 0 ? 0 : stringBitSize));
    data.putBit(stringBytes == 0 ? 0 : 1);
    data.alignToByte();
    handles.alignToByte();
    if (handleBitSize != nullptr)
        *handleBitSize = static_cast<std::uint32_t>(handles.size() * 8);

    std::vector<std::uint8_t> bytes = data.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    return bytes;
}

// Emit a LAYOUT body for the test.  Inverts DRW_Layout::parseDwg
// (drw_objects.cpp).  AC1015/AC1018 — pre-R2007 strings stay inline; LAYOUT's
// R2004+ shade-plot block and viewport handles are exercised through the
// version branch.
//
// `parentHandle` / `reactorHandles` / `xDictFlag` configure the common
// handle-stream prefix emitted immediately before the type-specific
// handles. They must match the values fed to emitObjectPreamble's
// numReactors / xDictFlag for the parser's readCommonObjectHandles call
// to consume them correctly.
void emitLayoutBody(dwgBufferW& body, DRW::Version version,
                    const DRW_Layout& src,
                    std::uint32_t parentHandle = 0,
                    const std::vector<std::uint32_t>& reactorHandles = {},
                    std::uint8_t xDictFlag = 0,
                    std::uint32_t paperSpaceBlockRecordRef = 0,
                    std::uint32_t lastActiveViewportRef = 0,
                    std::uint32_t baseUcsRef = 0,
                    std::uint32_t namedUcsRef = 0,
                    bool emitViewportHandles = true) {
    body.putVariableText(version, src.pageSetupName);
    body.putVariableText(version, src.printerConfig);
    body.putBitShort(src.plotLayoutFlags);
    body.putBitDouble(src.marginLeft);
    body.putBitDouble(src.marginBottom);
    body.putBitDouble(src.marginRight);
    body.putBitDouble(src.marginTop);
    body.putBitDouble(src.paperWidth);
    body.putBitDouble(src.paperHeight);
    body.putVariableText(version, src.paperSize);
    body.putBitDouble(src.plotOriginX);
    body.putBitDouble(src.plotOriginY);
    body.putBitShort(src.paperUnits);
    body.putBitShort(src.plotRotation);
    body.putBitShort(src.plotType);
    body.putBitDouble(src.windowMinX);
    body.putBitDouble(src.windowMinY);
    body.putBitDouble(src.windowMaxX);
    body.putBitDouble(src.windowMaxY);
    if (version < DRW::AC1018) {
        body.putVariableText(version, src.plotViewName);
    }
    body.putBitDouble(src.realWorldUnits);
    body.putBitDouble(src.drawingUnits);
    body.putVariableText(version, src.currentStyleSheet);
    body.putBitShort(src.scaleType);
    body.putBitDouble(src.scaleFactor);
    body.putBitDouble(src.paperImageOriginX);
    body.putBitDouble(src.paperImageOriginY);
    if (version >= DRW::AC1018) {
        body.putBitShort(src.shadePlotMode);
        body.putBitShort(src.shadePlotResLevel);
        body.putBitShort(src.shadePlotCustomDPI);
    }
    body.putVariableText(version, src.name);
    body.putBitLong(src.tabOrder);
    body.putBitShort(src.layoutFlags);
    body.put3BitDouble(src.ucsOrigin);
    body.putRawDouble(src.limMinX);
    body.putRawDouble(src.limMinY);
    body.putRawDouble(src.limMaxX);
    body.putRawDouble(src.limMaxY);
    body.put3BitDouble(src.insPoint);
    body.put3BitDouble(src.ucsXAxis);
    body.put3BitDouble(src.ucsYAxis);
    body.putBitDouble(src.elevation);
    body.putBitShort(src.orthoViewType);
    body.put3BitDouble(src.extMin);
    body.put3BitDouble(src.extMax);
    if (version >= DRW::AC1018) {
        // BitLong per ODA §20.4.84 / libreDWG FIELD_BL (num_viewports) -- must
        // match DRW_Layout::parseDwg's getBitLong() or the round-trip desyncs.
        body.putBitLong(src.viewportCount);
    }
    // Object common handle prefix (parentHandle + reactors + xdic) per ODA
    // §20.4.84 + base-object spec. Reactors and xdic use raw A/C codes so a
    // table-entry offset decode would produce visibly wrong references.
    emitObjectHandlePrefix(body, parentHandle, reactorHandles, 0,
                           xDictFlag);

    // Type-specific handle stream tail (inline for AC1015/AC1018).
    if (version >= DRW::AC1018) {
        body.putHandle(nullHandle());          // plotViewHandle
    }
    body.putHandle(hardPtr(paperSpaceBlockRecordRef));
    body.putHandle(hardPtr(lastActiveViewportRef));
    body.putHandle(hardPtr(baseUcsRef));
    body.putHandle(hardPtr(namedUcsRef));
    if (version >= DRW::AC1018) {
        if (emitViewportHandles) {
            for (std::int32_t i = 0; i < src.viewportCount; ++i) {
                body.putHandle(nullHandle());
            }
        }
    }
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::parseDwg captures PlotSettings prefix + layout-specific fields",
          "[dwg-read][object-encode][layout]") {
    DRW_Layout src;
    src.pageSetupName    = "MyPageSetup";
    src.printerConfig    = "HP LaserJet";
    src.plotLayoutFlags  = 5;
    src.marginLeft       = 12.7;
    src.marginBottom     = 6.35;
    src.marginRight      = 12.7;
    src.marginTop        = 6.35;
    src.paperWidth       = 297.0;
    src.paperHeight      = 210.0;
    src.paperSize        = "A4";
    src.plotOriginX      = 5.0;
    src.plotOriginY      = 10.0;
    src.paperUnits       = 0;
    src.plotRotation     = 1;
    src.plotType         = 2;
    src.windowMinX       = -50.0;
    src.windowMinY       = -25.0;
    src.windowMaxX       = 250.0;
    src.windowMaxY       = 175.0;
    src.plotViewName     = "MyView";   // ignored on AC1018+
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 100.0;
    src.currentStyleSheet = "monochrome.ctb";
    src.scaleType        = 16;
    src.scaleFactor      = 0.01;
    src.paperImageOriginX = 1.5;
    src.paperImageOriginY = 2.5;
    src.name        = "Layout1";
    src.tabOrder    = 3;
    src.layoutFlags = 0;
    src.ucsOrigin   = DRW_Coord{1.0, 2.0, 3.0};
    src.limMinX     = -10.0;
    src.limMinY     = -20.0;
    src.limMaxX     = 100.0;
    src.limMaxY     = 200.0;
    src.insPoint    = DRW_Coord{0.5, 1.5, 2.5};
    src.ucsXAxis    = DRW_Coord{1.0, 0.0, 0.0};
    src.ucsYAxis    = DRW_Coord{0.0, 1.0, 0.0};
    src.elevation   = 7.5;
    src.orthoViewType = 4;
    src.extMin      = DRW_Coord{-15.0, -25.0, 0.0};
    src.extMax      = DRW_Coord{120.0, 220.0, 0.0};
    src.viewportCount = 0;

    // R2000: no R2004+ shade-plot block, no R2007+ visualStyle handle.
    DRW::Version ver = DRW::AC1015;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/82, /*handle=*/0x100);
    emitLayoutBody(w, ver, src);

    auto bytes = snapshot(w);
    REQUIRE(bytes.size() > 0);

    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.pageSetupName == "MyPageSetup");
    REQUIRE(dst.printerConfig == "HP LaserJet");
    REQUIRE(dst.plotLayoutFlags == 5);
    REQUIRE(dst.marginLeft   == Approx(12.7));
    REQUIRE(dst.marginBottom == Approx(6.35));
    REQUIRE(dst.marginRight  == Approx(12.7));
    REQUIRE(dst.marginTop    == Approx(6.35));
    REQUIRE(dst.paperWidth   == Approx(297.0));
    REQUIRE(dst.paperHeight  == Approx(210.0));
    REQUIRE(dst.paperSize    == "A4");
    REQUIRE(dst.plotOriginX  == Approx(5.0));
    REQUIRE(dst.plotOriginY  == Approx(10.0));
    REQUIRE(dst.paperUnits   == 0);
    REQUIRE(dst.plotRotation == 1);
    REQUIRE(dst.plotType     == 2);
    REQUIRE(dst.windowMinX   == Approx(-50.0));
    REQUIRE(dst.windowMaxY   == Approx(175.0));
    REQUIRE(dst.plotViewName == "MyView");
    REQUIRE(dst.realWorldUnits   == Approx(1.0));
    REQUIRE(dst.drawingUnits     == Approx(100.0));
    REQUIRE(dst.currentStyleSheet == "monochrome.ctb");
    REQUIRE(dst.scaleType   == 16);
    REQUIRE(dst.scaleFactor == Approx(0.01));
    REQUIRE(dst.paperImageOriginX == Approx(1.5));
    REQUIRE(dst.paperImageOriginY == Approx(2.5));

    REQUIRE(dst.name        == "Layout1");
    REQUIRE(dst.tabOrder    == 3);
    REQUIRE(dst.layoutFlags == 0);
    REQUIRE(dst.ucsOrigin.x == Approx(1.0));
    REQUIRE(dst.ucsOrigin.y == Approx(2.0));
    REQUIRE(dst.ucsOrigin.z == Approx(3.0));
    REQUIRE(dst.limMinX     == Approx(-10.0));
    REQUIRE(dst.limMaxX     == Approx(100.0));
    REQUIRE(dst.insPoint.x  == Approx(0.5));
    REQUIRE(dst.ucsXAxis.x  == Approx(1.0));
    REQUIRE(dst.ucsYAxis.y  == Approx(1.0));
    REQUIRE(dst.elevation   == Approx(7.5));
    REQUIRE(dst.orthoViewType == 4);
    REQUIRE(dst.extMin.x    == Approx(-15.0));
    REQUIRE(dst.extMax.y    == Approx(220.0));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::parseDwg AC1018 captures shade-plot block + viewport handles",
          "[dwg-read][object-encode][layout]") {
    DRW_Layout src;
    src.pageSetupName    = "PrintSetup";
    src.printerConfig    = "Generic PostScript";
    src.plotLayoutFlags  = 0;
    src.paperWidth       = 420.0;
    src.paperHeight      = 297.0;
    src.paperSize        = "A3";
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 1.0;
    src.currentStyleSheet = "";
    src.scaleType        = 0;
    src.scaleFactor      = 1.0;
    src.shadePlotMode      = 3;
    src.shadePlotResLevel  = 5;
    src.shadePlotCustomDPI = 300;
    src.name = "Sheet1";
    src.tabOrder = 1;
    src.layoutFlags = 1;
    src.viewportCount = 2;

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, 82, 0x101);
    emitLayoutBody(w, ver, src);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.pageSetupName == "PrintSetup");
    REQUIRE(dst.paperSize     == "A3");
    REQUIRE(dst.shadePlotMode      == 3);
    REQUIRE(dst.shadePlotResLevel  == 5);
    REQUIRE(dst.shadePlotCustomDPI == 300);
    REQUIRE(dst.name == "Sheet1");
    REQUIRE(dst.tabOrder == 1);
    REQUIRE(dst.viewportCount == 2);
    REQUIRE(dst.viewportHandles.size() == 2u);
}

TEST_CASE("DRW_Layout rejects an impossible viewport count before allocation",
          "[dwg-read][object-encode][layout][safety]") {
    DRW_Layout source;
    source.viewportCount = DRW_Layout::kMaxViewportCount;

    const DRW::Version version = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/82, /*handle=*/0x115,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    emitLayoutBody(encoded, version, source, /*parentHandle=*/0,
                   /*reactorHandles=*/{}, /*xDictFlag=*/1,
                   /*paperSpaceBlockRecordRef=*/0,
                   /*lastActiveViewportRef=*/0, /*baseUcsRef=*/0,
                   /*namedUcsRef=*/0, /*emitViewportHandles=*/false);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Layout parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.viewportHandles.empty());
}

TEST_CASE("DRW_Layout rejects non-finite DWG values transactionally",
          "[dwg-read][object-encode][layout][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/82, /*handle=*/0x116);
    encoded.putVariableText(version, "PageSetup");
    encoded.putVariableText(version, "Printer");
    encoded.putBitShort(0); // plot layout flags
    encoded.putBitDouble(nan); // margin left

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Layout parsed;
    parsed.name = "stale layout";
    parsed.marginLeft = 9.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.marginLeft == Approx(0.0));
    CHECK(parsed.parentHandle == 0u);
}

// Regression test for the handle-stream alignment bug in DRW_Layout::parseDwg:
// for AC1015/AC1018 the parser previously skipped readCommonObjectHandles,
// so it consumed parentHandle + reactor + xdic bytes as the first
// type-specific handles. With non-zero parentHandle + reactor count the
// downstream type-specific assertions catch the misalignment.
//
// Pre-fix: dst.parentHandle stays 0 (never read) and dst.paperSpaceBlockRecordHandle.ref
// reads 0x42 (the misaligned parentHandle value) instead of the expected 0x70.
// Post-fix: both come out correctly.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::parseDwg consumes parentHandle + reactors before type-specific handles",
          "[dwg-read][object-encode][layout]") {
    DRW_Layout src;
    src.pageSetupName    = "Setup";
    src.printerConfig    = "Printer";
    src.paperWidth       = 297.0;
    src.paperHeight      = 210.0;
    src.paperSize        = "A4";
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 1.0;
    src.scaleType        = 0;
    src.scaleFactor      = 1.0;
    src.shadePlotMode      = 0;
    src.shadePlotResLevel  = 0;
    src.shadePlotCustomDPI = 0;
    src.name = "Layout1";
    src.tabOrder = 1;
    src.layoutFlags = 0;
    src.viewportCount = 0;

    const std::uint32_t kParent = 0x42;
    const std::vector<std::uint32_t> kReactors = {0x50, 0x51};
    const std::uint32_t kPaperSpaceRef = 0x70;
    const std::uint32_t kLastActiveRef = 0x71;
    const std::uint32_t kBaseUcsRef    = 0x72;
    const std::uint32_t kNamedUcsRef   = 0x73;

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, 82, 0x110,
                       static_cast<std::int32_t>(kReactors.size()),
                       /*xDictFlag=*/0);
    emitLayoutBody(w, ver, src, kParent, kReactors, /*xDictFlag=*/0,
                   kPaperSpaceRef, kLastActiveRef, kBaseUcsRef, kNamedUcsRef);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    // The fix guarantees parent is read into parentHandle, and type-specific
    // handles land in the correct slots.
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle)        == kParent);
    REQUIRE(dst.reactorHandles == kReactors);
    REQUIRE(dst.xDictHandle == 0u);
    REQUIRE(dst.paperSpaceBlockRecordHandle.ref           == kPaperSpaceRef);
    REQUIRE(dst.lastActiveViewportHandle.ref              == kLastActiveRef);
    REQUIRE(dst.baseUcsHandle.ref                         == kBaseUcsRef);
    REQUIRE(dst.namedUcsHandle.ref                        == kNamedUcsRef);

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(dst, ver,
                                                  &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(dst.name.empty());
    CHECK(dst.viewportHandles.empty());
}

// IDBUFFER parser test (ODA §20.4.79).  Common preamble + RC + BL + handle
// stream tail = parentHandle + xdic + N object-handles.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_IDBuffer::parseDwg captures object-id handle list",
          "[dwg-read][object-encode][idbuffer]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x200);  // custom-class
    w.putRawChar8(0);                                            // classVersion
    w.putBitLong(3);                                             // numIds
    const std::uint32_t bodyEndBit = w.bitCount();

    emitCommonHandlePrefix(w, /*parentHandle=*/0x10, /*reactors=*/{}, /*xDictFlag=*/0);
    w.putHandle(hardPtr(0x100));
    w.putHandle(hardPtr(0x101));
    w.putHandle(hardPtr(0x102));
    w.patchRawLong32AtBit(objectSizeBitOffset(w), bodyEndBit);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_IDBuffer dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x10u);
    REQUIRE(dst.classVersion == 0);
    REQUIRE(dst.objIds.size() == 3u);
    REQUIRE(dst.objIds[0] == 0x100u);
    REQUIRE(dst.objIds[1] == 0x101u);
    REQUIRE(dst.objIds[2] == 0x102u);
}

// LAYER_INDEX parser test (ODA §20.4.83).  timestamp1, timestamp2,
// numentries, then repeat (BL indexLong + TV name).  Handle stream:
// parentHandle + xdic + N entry handles.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_LayerIndex::parseDwg captures per-layer entry handles",
          "[dwg-read][object-encode][layerindex]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x300);
    w.putBitLong(0x12345);          // timestamp1
    w.putBitLong(0x6789a);          // timestamp2
    w.putBitLong(2);                // numEntries

    w.putBitLong(1);
    w.putVariableText(ver, "Layer0");
    w.putBitLong(2);
    w.putVariableText(ver, "Layer1");

    emitCommonHandlePrefix(w, 0x20, {}, 0);
    w.putHandle(hardPtr(0x200));    // entry 0 -> idbuffer handle
    w.putHandle(hardPtr(0x201));    // entry 1 -> idbuffer handle

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_LayerIndex dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x20u);
    REQUIRE(dst.timestamp1 == 0x12345u);
    REQUIRE(dst.timestamp2 == 0x6789au);
    REQUIRE(dst.entries.size() == 2u);
    REQUIRE(dst.entries[0].indexLong == 1);
    REQUIRE(dst.entries[0].name == "Layer0");
    REQUIRE(dst.entries[0].entryHandle == 0x200u);
    REQUIRE(dst.entries[1].indexLong == 2);
    REQUIRE(dst.entries[1].name == "Layer1");
    REQUIRE(dst.entries[1].entryHandle == 0x201u);
}

// 8b-1 (gap wipeoutvariables-object-not-dispatched): WIPEOUTVARIABLES carries
// the drawing-wide display-frame flag (DXF 70). It is now parsed + dispatched
// (addWipeoutVariables) instead of only landing in the raw-replay skip set.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_WipeoutVariables::parseDwg captures display_frame flag",
          "[dwg-read][object-encode][wipeout]") {
    REQUIRE(DRW_WipeoutVariables{}.m_displayFrame == 0);  // default

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x600);
    w.putBitShort(1);   // display_frame = 1
    emitCommonHandlePrefix(w, 0, {}, 0);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_WipeoutVariables dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));
    REQUIRE(dst.m_displayFrame == 1);
}

// WIPEOUTVARIABLES encoder round-trip. Body: one display-frame BS plus the
// common handle prefix, with no type-specific handle tail.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_WipeoutVariables::encodeDwg round-trips display frame",
          "[dwg-write][object-encode][wipeoutvariables]") {
    DRW_WipeoutVariables src;
    src.handle = 0x601;
    src.parentHandle = 0xC;          // Named-objects dictionary
    src.m_displayFrame = 40000;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0 /* custom-class */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeWipeoutVariables(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_WipeoutVariables dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_displayFrame == 40000);
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0xCu);
    REQUIRE(r.isGood());
}

TEST_CASE("DRW_WipeoutVariables rejects a truncated common handle stream",
          "[dwg-read][object-encode][wipeoutvariables][safety]") {
    DRW_WipeoutVariables source;
    source.handle = 0x602u;
    source.parentHandle = 0xCu;
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, DRW::AC1018, /*oType=*/0, source.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeWipeoutVariables(
        source, DRW::AC1018, &encoded));

    auto bytes = snapshot(encoded);
    REQUIRE(bytes.size() > 1);
    bytes.pop_back();

    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_WipeoutVariables parsed;
    parsed.m_displayFrame = 9;
    parsed.parentHandle = 0xBEEFu;
    parsed.reactorHandles.push_back(0xCAFEu);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_displayFrame == 0u);
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
}

TEST_CASE("DRW_WipeoutVariables bounds common handle metadata",
          "[dwg-write][object-encode][wipeoutvariables][safety]") {
    DRW_WipeoutVariables source;
    dwgBufferW body;

    DrwObjectEncodeTestAccess::setNumReactors(
        source, static_cast<std::int32_t>(dwgSafety::MaxReactorCount + 1));
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeWipeoutVariables(
        source, DRW::AC1018, &body));
    CHECK(body.data().empty());

    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 2);
    body.reset();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeWipeoutVariables(
        source, DRW::AC1018, &body));
    CHECK(body.data().empty());
}

TEST_CASE("DRW_WipeoutVariables preserves unsigned display frame",
          "[dwg-write][object-encode][wipeoutvariables][safety]") {
    DRW_WipeoutVariables source;
    source.m_displayFrame = 40000;

    dwgBufferW body;
    REQUIRE(DrwObjectEncodeTestAccess::encodeWipeoutVariables(
        source, DRW::AC1018, &body));
    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    CHECK(reader.getBitShort() == 40000);
}

TEST_CASE("DRW_AcDbPlaceholder validates its common handle stream",
          "[dwg-read][object-encode][placeholder][safety]") {
    const DRW::Version ver = DRW::AC1018;

    dwgBufferW valid;
    emitObjectPreamble(valid, ver, /*oType=*/0, /*handle=*/0x610,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    emitCommonHandlePrefix(valid, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{0x43}, /*xDictFlag=*/0);
    auto validBytes = snapshot(valid);
    dwgBuffer validReader(const_cast<std::uint8_t *>(validBytes.data()),
                          validBytes.size());
    DRW_AcDbPlaceholder parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x42u);
    REQUIRE(parsed.reactorHandles.size() == 1);
    CHECK(parsed.reactorHandles.front() == 0x43u);
    CHECK(parsed.xDictHandle == 0u);

    SECTION("modern handle stream cannot consume the whole frame") {
        const DRW::Version modern = DRW::AC1024;
        dwgBufferW malformed;
        emitObjectPreamble(malformed, modern, /*oType=*/0,
                           /*handle=*/0x612, /*numReactors=*/0,
                           /*xDictFlag=*/1);
        auto bytes = snapshot(malformed);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_AcDbPlaceholder rejected;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
            rejected, modern, &reader,
            static_cast<std::uint32_t>(bytes.size() * 8u)));
        CHECK_FALSE(reader.isGood());
        CHECK(rejected.handle == 0u);
    }

    dwgBufferW truncated;
    emitObjectPreamble(truncated, ver, /*oType=*/0, /*handle=*/0x611,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    auto truncatedBytes = snapshot(truncated);
    dwgBuffer truncatedReader(
        const_cast<std::uint8_t *>(truncatedBytes.data()),
        truncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("dwgBuffer copies preserve their cursor position",
          "[dwg-read][object-encode][buffer][safety]") {
    std::vector<std::uint8_t> bytes = {0x11, 0x22, 0x33};
    dwgBuffer source(bytes.data(), bytes.size());
    REQUIRE(source.getRawChar8() == 0x11u);

    dwgBuffer fork = source;
    CHECK(fork.getRawChar8() == 0x22u);
    CHECK(source.getRawChar8() == 0x22u);
}

TEST_CASE("DRW_VisualStyle rejects a null DWG buffer",
          "[dwg-read][object-encode][visualstyle][safety]") {
    DRW_VisualStyle style;
    style.handle = 0xB80;
    style.parentHandle = 0x42;
    style.reactorHandles = {0x43};
    style.xDictHandle = 0x44;
    style.desc = "stale";
    style.m_bodyDecoded = true;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        style, DRW::AC1021, nullptr));
    CHECK(style.handle == 0u);
    CHECK(style.parentHandle == 0u);
    CHECK(style.reactorHandles.empty());
    CHECK(style.xDictHandle == 0u);
    CHECK(style.desc.empty());
    CHECK_FALSE(style.m_bodyDecoded);
}

TEST_CASE("DRW_VisualStyle legacy DWG body round-trips with common handles",
          "[dwg-read][dwg-write][object-encode][visualstyle]") {
    const DRW::Version version = DRW::AC1015;
    DRW_VisualStyle source;
    source.handle = 0xB80;
    source.parentHandle = 0x42;
    source.reactorHandles = {0x44};
    source.xDictHandle = 0x43;
    source.desc = "Legacy visual style";
    source.type = 7;
    source.m_body.faceLightingModel = 2;
    source.m_body.faceOpacity = 0.75;
    source.m_body.faceMonoColor = 5;
    source.m_body.edgeModel = 3;
    source.m_body.edgeColor = 6;
    source.m_body.edgeWidth = 4;
    source.m_body.edgeDoHidePrecision = true;
    source.m_body.displaySettings = 9;
    source.m_body.displayBrightness = 2.0;
    source.m_body.displayShadowType = 1;
    source.m_body.internalOnly = true;
    source.setDwgCommonObjectState(1, 0, false);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, DRW_VisualStyle::kDwgClassNum,
                       source.handle, 1, 0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeVisualStyle(
        source, version, &encoded));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_VisualStyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseVisualStyle(
        parsed, version, &reader));
    CHECK(parsed.desc == source.desc);
    CHECK(parsed.type == source.type);
    CHECK(parsed.m_body.faceLightingModel == 2);
    CHECK(parsed.m_body.faceOpacity == Approx(0.75));
    CHECK(parsed.m_body.faceMonoColor == 5);
    CHECK(parsed.m_body.edgeColor == 6);
    CHECK(parsed.m_body.edgeDoHidePrecision);
    CHECK(parsed.m_body.displayShadowType == 1);
    CHECK(parsed.m_body.internalOnly);
    CHECK(parsed.parentHandle == 0x42);
    CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x44});
    CHECK(parsed.xDictHandle == 0x43u);

    REQUIRE(bytes.size() > 1);
    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    DRW_VisualStyle truncated;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseVisualStyle(
        truncated, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncated.handle == 0u);
    CHECK(truncated.parentHandle == 0u);
    CHECK(truncated.reactorHandles.empty());
    CHECK(truncated.xDictHandle == 0u);
    CHECK_FALSE(truncated.m_bodyDecoded);
}

TEST_CASE("DRW_VisualStyle AC1027 body preserves strings and DS flag",
          "[dwg-read][dwg-write][object-encode][visualstyle][ac1027]") {
    DRW_VisualStyle source;
    source.handle = 0xB81;
    source.parentHandle = 0x52;
    source.reactorHandles = {0x54};
    source.xDictHandle = 0x53;
    source.desc = "R2013 visual style";
    source.type = 11;
    source.m_body.extLightingModel = 3;
    source.m_body.faceLightingModel = 4;
    source.m_body.faceMonoColor = 7;
    source.m_body.displayBrightness = 1.25;
    source.m_body.hasR2013bExpansion = true;
    source.m_body.bProp1c = true;
    source.m_body.blProp25 = 25;
    source.m_body.cProp29 = 8;
    source.m_body.strokes = "stroke data";
    source.m_body.bProp37 = true;
    source.setDwgCommonObjectState(1, 0, true);

    std::uint32_t handleBitSize = 0;
    auto bytes = emitAc1027VisualStyleObject(source, &handleBitSize);
    REQUIRE_FALSE(bytes.empty());
    REQUIRE(handleBitSize != 0);

    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1027, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(const_cast<std::uint8_t *>(bytes.data()), bytes.size(),
                     &decoder);
    DRW_VisualStyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseVisualStyle(
        parsed, DRW::AC1027, &reader, handleBitSize));
    CHECK(parsed.hasDataStorageBinaryData());
    CHECK(parsed.desc == source.desc);
    CHECK(parsed.type == source.type);
    CHECK(parsed.m_body.extLightingModel == 3);
    CHECK(parsed.m_body.faceLightingModel == 4);
    CHECK(parsed.m_body.bProp1c);
    CHECK(parsed.m_body.blProp25 == 25);
    CHECK(parsed.m_body.cProp29 == 8);
    CHECK(parsed.m_body.strokes == "stroke data");
    CHECK(parsed.m_body.bProp37);
    CHECK(parsed.parentHandle == 0x52);
    CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x54});
    CHECK(parsed.xDictHandle == 0x53u);
}

TEST_CASE("DRW_VisualStyle rejects a body truncated before its handle stream",
          "[dwg-read][object-encode][visualstyle][safety]") {
    DRW_VisualStyle source;
    source.handle = 0xB83;
    source.parentHandle = 0x62;
    source.desc = "truncated visual style";
    source.setDwgCommonObjectState(1, 0, false);

    std::uint32_t handleBitSize = 0;
    auto bytes = emitAc1027VisualStyleObject(source, &handleBitSize);
    REQUIRE(handleBitSize != 0);
    const std::uint32_t shortenedHandleBitSize = handleBitSize + 64u;
    REQUIRE(static_cast<std::uint64_t>(bytes.size()) * 8u
            > shortenedHandleBitSize);

    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1027, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_VisualStyle parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseVisualStyle(
        parsed, DRW::AC1027, &reader, shortenedHandleBitSize));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.handle == 0u);
    CHECK_FALSE(parsed.m_bodyDecoded);
}

TEST_CASE("DRW_VisualStyle rejects lossy field narrowing",
          "[dwg-write][object-encode][visualstyle][safety]") {
    DRW_VisualStyle source;
    source.m_body.edgeWidth = -1;
    dwgBufferW widthBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVisualStyle(
        source, DRW::AC1015, &widthBody));
    CHECK(widthBody.data().empty());

    source.m_body.edgeWidth = 0;
    source.m_body.edgeHaloGap = std::numeric_limits<std::uint8_t>::max() + 1;
    dwgBufferW byteBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVisualStyle(
        source, DRW::AC1015, &byteBody));
    CHECK(byteBody.data().empty());

    source.m_body.edgeHaloGap = 0;
    source.m_body.extLightingModel = -1;
    dwgBufferW modernBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVisualStyle(
        source, DRW::AC1024, &modernBody));
    CHECK(modernBody.data().empty());

    source.m_body.extLightingModel = 0;
    source.m_body.cProp29 = std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW colorBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVisualStyle(
        source, DRW::AC1027, &colorBody));
    CHECK(colorBody.data().empty());

    source.m_body.cProp29 = 0;
    source.m_body.faceOpacity = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW nonFiniteBody;
    nonFiniteBody.putRawChar8(0xA5);
    const auto beforeNonFinite = nonFiniteBody.data();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVisualStyle(
        source, DRW::AC1015, &nonFiniteBody));
    CHECK(nonFiniteBody.data() == beforeNonFinite);
}

TEST_CASE("DRW_VisualStyle rejects non-finite body doubles",
          "[dwg-read][object-encode][visualstyle][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, DRW_VisualStyle::kDwgClassNum,
                       /*handle=*/0xB84);
    encoded.putVariableText(version, "");
    encoded.putBitLong(0); // style type
    encoded.putBitLong(0); // face lighting model
    encoded.putBitLong(0); // face lighting quality
    encoded.putBitLong(0); // face color mode
    encoded.putBitDouble(nan); // face opacity
    encoded.alignToByte();
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded),
                                encoded.bitCount());

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_VisualStyle parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseVisualStyle(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.handle == 0u);
    CHECK_FALSE(parsed.m_bodyDecoded);
}

TEST_CASE("DRW_VisualStyle reads absolute object reactors and xdictionary",
          "[dwg-read][object-encode][visualstyle][object-handles]") {
    DRW_VisualStyle source;
    source.handle = 0xB82;
    source.parentHandle = 0x42;
    source.reactorHandles = {0x43};
    source.xDictHandle = 0x44;
    source.desc = "Object handles";
    source.type = 3;
    source.setDwgCommonObjectState(1, 0, false);

    auto bytes = emitAc1015VisualStyleObject(source);
    REQUIRE_FALSE(bytes.empty());
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_VisualStyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseVisualStyle(
        parsed, DRW::AC1015, &reader));
    CHECK(parsed.parentHandle == 0x42u);
    REQUIRE(parsed.reactorHandles.size() == 1);
    CHECK(parsed.reactorHandles.front() == 0x43u);
    CHECK(parsed.xDictHandle == 0x44u);
}

TEST_CASE("DRW_GeoData rejects invalid versions and mesh-count overflow",
          "[dwg-read][dwg-write][object-encode][geodata][safety]") {
    DRW_GeoData source;
    source.m_version = 0;

    dwgBufferW encoded;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeGeoData(
        source, DRW::AC1027, &encoded));

    source.m_version = 2;
    source.m_designPoint = DRW_Coord{
        std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0};
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeGeoData(
        source, DRW::AC1027, &encoded));
    source.m_designPoint = DRW_Coord{};

    source.m_points.resize(
        static_cast<std::size_t>(DRW_GeoData::kMaxMeshItems) + 1u);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeGeoData(
        source, DRW::AC1027, &encoded));

    source.m_points.clear();
    source.m_faces.resize(
        static_cast<std::size_t>(DRW_GeoData::kMaxMeshItems) + 1u);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeGeoData(
        source, DRW::AC1027, &encoded));
}

TEST_CASE("DWG object encoders reject invalid common state transactionally",
          "[dwg-write][object-encode][common-handles][safety]") {
    const auto checkInvalidXDict = [](auto& object, DRW::Version version,
                                      auto encode) {
        DrwObjectEncodeTestAccess::setXDictFlag(object, 2);
        dwgBufferW rejected;
        CHECK_FALSE(encode(object, version, &rejected));
        CHECK(rejected.data().empty());
    };

    DRW_MLineStyle mlineStyle;
    checkInvalidXDict(mlineStyle, DRW::AC1015,
                      DrwObjectEncodeTestAccess::encodeMLineStyle);

    DRW_SpatialFilter spatialFilter;
    checkInvalidXDict(spatialFilter, DRW::AC1018,
                      DrwObjectEncodeTestAccess::encodeSpatialFilter);

    DRW_GeoData geoData;
    geoData.m_version = 2;
    checkInvalidXDict(geoData, DRW::AC1018,
                      DrwObjectEncodeTestAccess::encodeGeoData);

    DRW_MLeaderStyle mleaderStyle;
    checkInvalidXDict(mleaderStyle, DRW::AC1021,
                      DrwObjectEncodeTestAccess::encodeMLeaderStyle);

    DRW_UnderlayDefinition underlay;
    checkInvalidXDict(underlay, DRW::AC1018,
                      DrwObjectEncodeTestAccess::encodeUnderlayDefinition);

    DRW_ImageDefinitionReactor imageReactor;
    checkInvalidXDict(imageReactor, DRW::AC1015,
                      DrwObjectEncodeTestAccess::encodeImageDefinitionReactor);

    DRW_AcDbPlaceholder placeholder;
    checkInvalidXDict(placeholder, DRW::AC1018,
                      DrwObjectEncodeTestAccess::encodeAcDbPlaceholder);

    DRW_Sun sun;
    checkInvalidXDict(sun, DRW::AC1021,
                      DrwObjectEncodeTestAccess::encodeSun);

    DRW_VbaProject vba;
    DrwObjectEncodeTestAccess::setNumReactors(vba, -1);
    dwgBufferW rejectedVba;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVbaProject(
        vba, DRW::AC1015, &rejectedVba));
    CHECK(rejectedVba.data().empty());
}

TEST_CASE("DWG object encoders propagate poisoned common buffers",
          "[dwg-write][object-encode][common-handles][safety]") {
    const auto checkPoisoned = [](auto& object, DRW::Version version,
                                  auto encode) {
        dwgBufferW poisoned;
        poisoned.putBitLongLong(0xFF00000000000000ULL);
        CHECK_FALSE(encode(object, version, &poisoned));
    };

    DRW_MLineStyle mlineStyle;
    checkPoisoned(mlineStyle, DRW::AC1015,
                  DrwObjectEncodeTestAccess::encodeMLineStyle);

    DRW_SpatialFilter spatialFilter;
    checkPoisoned(spatialFilter, DRW::AC1018,
                  DrwObjectEncodeTestAccess::encodeSpatialFilter);

    DRW_GeoData geoData;
    geoData.m_version = 2;
    checkPoisoned(geoData, DRW::AC1018,
                  DrwObjectEncodeTestAccess::encodeGeoData);

    DRW_MLeaderStyle mleaderStyle;
    checkPoisoned(mleaderStyle, DRW::AC1021,
                  DrwObjectEncodeTestAccess::encodeMLeaderStyle);

    DRW_UnderlayDefinition underlay;
    checkPoisoned(underlay, DRW::AC1018,
                  DrwObjectEncodeTestAccess::encodeUnderlayDefinition);

    DRW_ImageDefinitionReactor imageReactor;
    checkPoisoned(imageReactor, DRW::AC1015,
                  DrwObjectEncodeTestAccess::encodeImageDefinitionReactor);

    DRW_AcDbPlaceholder placeholder;
    checkPoisoned(placeholder, DRW::AC1018,
                  DrwObjectEncodeTestAccess::encodeAcDbPlaceholder);

    DRW_VbaProject vba;
    checkPoisoned(vba, DRW::AC1015,
                  DrwObjectEncodeTestAccess::encodeVbaProject);

    DRW_Sun sun;
    checkPoisoned(sun, DRW::AC1021,
                  DrwObjectEncodeTestAccess::encodeSun);
}

TEST_CASE("DWG object encoders propagate detached buffer failures",
          "[dwg-write][object-encode][buffer-state][safety]") {
    const auto checkPoisoned = [](auto encode, bool usesStringStream) {
        dwgBufferW body;
        body.putBitLongLong(0xFF00000000000000ULL);
        CHECK_FALSE(encode(&body, nullptr, nullptr));

        if (usesStringStream) {
            dwgBufferW validBody;
            dwgBufferW strings;
            strings.putBitLongLong(0xFF00000000000000ULL);
            dwgBufferW handles;
            CHECK_FALSE(encode(&validBody, &strings, &handles));
        }

        dwgBufferW validStrings;
        dwgBufferW validBody;
        dwgBufferW poisonedHandles;
        poisonedHandles.putBitLongLong(0xFF00000000000000ULL);
        CHECK_FALSE(encode(&validBody, &validStrings, &poisonedHandles));
    };

    DRW_Section section;
    section.m_kind = DRW_Section::Manager;
    checkPoisoned([&section](dwgBufferW* body, dwgBufferW* strings,
                             dwgBufferW* handles) {
        return DrwObjectEncodeTestAccess::encodeSection(
            section, DRW::AC1021, body, strings, handles);
    }, true);

    DRW_RenderSettings renderSettings;
    renderSettings.m_kind = DRW_RenderSettings::Settings;
    checkPoisoned([&renderSettings](dwgBufferW* body, dwgBufferW* strings,
                                    dwgBufferW* handles) {
        return DrwObjectEncodeTestAccess::encodeRenderSettings(
            renderSettings, DRW::AC1021, body, strings, handles);
    }, true);

    DRW_PointPath pointPath;
    checkPoisoned([&pointPath](dwgBufferW* body, dwgBufferW* strings,
                               dwgBufferW* handles) {
        return DrwObjectEncodeTestAccess::encodePointPath(
            pointPath, DRW::AC1021, body, strings, handles);
    }, false);

    DRW_VisualStyle visualStyle;
    checkPoisoned([&visualStyle](dwgBufferW* body, dwgBufferW* strings,
                                 dwgBufferW* handles) {
        return DrwObjectEncodeTestAccess::encodeVisualStyle(
            visualStyle, DRW::AC1027, body, strings, handles);
    }, true);
}

TEST_CASE("DRW_GeoData rejects non-finite DWG body values transactionally",
          "[dwg-read][object-encode][geodata][safety]") {
    const DRW::Version version = DRW::AC1018;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    const auto makeBytes = [&](bool badDesignPoint, bool badMeshPoint,
                               std::int32_t pointCountOverride = -1) {
        dwgBufferW body;
        emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0xC02,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitLong(2);                 // class version
        body.putHandle(hardPtr(0x1F));      // host block
        body.putBitShort(2);                // coordinates type
        body.put3BitDouble(badDesignPoint
                               ? DRW_Coord{nan, 0.0, 0.0}
                               : DRW_Coord{0.0, 0.0, 0.0});
        body.put3BitDouble(DRW_Coord{500000.0, 4000000.0, 0.0});
        body.putBitDouble(1.0);              // horizontal unit scale
        body.putBitLong(6);                 // horizontal units
        body.putBitDouble(1.0);              // vertical unit scale
        body.putBitLong(6);                 // vertical units
        body.put3BitDouble(DRW_Coord{0.0, 0.0, 1.0});
        body.put2RawDouble(DRW_Coord{0.0, 1.0, 0.0});
        body.putBitLong(1);                 // scale estimation method
        body.putBitDouble(1.0);              // user scale factor
        body.putBit(0);                     // sea-level correction
        body.putBitDouble(0.0);              // sea-level elevation
        body.putBitDouble(6378137.0);       // projection radius
        body.putVariableText(version, "EPSG:3857");
        body.putVariableText(version, "<rss/>");
        body.putVariableText(version, "from");
        body.putVariableText(version, "to");
        body.putVariableText(version, "coverage");
        body.putBitLong(pointCountOverride >= 0
                            ? pointCountOverride : (badMeshPoint ? 1 : 0));
        if (badMeshPoint) {
            body.putRawDouble(nan);
            body.putRawDouble(0.0);
            body.putRawDouble(1.0);
            body.putRawDouble(1.0);
        }
        if (pointCountOverride >= 0)
            body.patchRawLong32AtBit(objectSizeBitOffset(body), body.bitCount());
        body.putBitLong(0);                 // mesh faces
        emitCommonHandlePrefix(body, 0x1A, {}, /*xDictFlag=*/1);
        return snapshot(body);
    };

    for (auto bytes : {makeBytes(true, false), makeBytes(false, true)}) {
        dwgBuffer reader(const_cast<std::uint8_t *>(bytes.data()), bytes.size());
        DRW_GeoData parsed;
        parsed.m_version = 99;
        parsed.m_designPoint = DRW_Coord{9.0, 9.0, 9.0};
        parsed.parentHandle = 0x98;

        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.m_version == 0);
        CHECK(parsed.m_designPoint.x == 0.0);
        CHECK(parsed.parentHandle == 0);
        CHECK(parsed.m_points.empty());
    }

    auto bytes = makeBytes(false, false, DRW_GeoData::kMaxMeshItems);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_GeoData parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_points.empty());
}

TEST_CASE("DRW view styles reject truncated frames transactionally",
          "[dwg-read][object-encode][viewstyle][safety]") {
    const auto checkTruncated = [](DRW_TableEntry& style) {
        style.parentHandle = 0x42;
        style.reactorHandles.push_back(0x43u);
        style.xDictHandle = 0x44u;

        dwgBufferW truncated;
        emitObjectPreamble(truncated, DRW::AC1018, /*oType=*/0,
                           /*handle=*/0x650, /*numReactors=*/1,
                           /*xDictFlag=*/0);
        emitCommonHandlePrefix(truncated, 0x42u, {0x43u}, /*xDictFlag=*/0);
        auto bytes = snapshot(truncated);
        dwgBuffer reader(bytes.data(), bytes.size());
        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
            style, DRW::AC1018, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(style.parentHandle == 0);
        CHECK(style.reactorHandles.empty());
        CHECK(style.xDictHandle == 0u);
    };

    DRW_DetailViewStyle detailStyle;
    checkTruncated(detailStyle);
    DRW_SectionViewStyle sectionStyle;
    checkTruncated(sectionStyle);
}

TEST_CASE("DRW view styles use object handle semantics in AC1018",
          "[dwg-read][object-encode][viewstyle][object-handles]") {
    const auto checkHandles = [](bool section) {
        auto bytes = emitAc1018ViewStyleObject(section);
        REQUIRE_FALSE(bytes.empty());
        dwgBuffer reader(bytes.data(), bytes.size());
        if (section) {
            DRW_SectionViewStyle style;
            REQUIRE(DrwObjectEncodeTestAccess::parse(
                style, DRW::AC1018, &reader));
            CHECK(style.parentHandle == 0x42u);
            REQUIRE(style.reactorHandles.size() == 1);
            CHECK(style.reactorHandles.front() == 0x43u);
            CHECK(style.xDictHandle == 0x44u);
        } else {
            DRW_DetailViewStyle style;
            REQUIRE(DrwObjectEncodeTestAccess::parse(
                style, DRW::AC1018, &reader));
            CHECK(style.parentHandle == 0x42u);
            REQUIRE(style.reactorHandles.size() == 1);
            CHECK(style.reactorHandles.front() == 0x43u);
            CHECK(style.xDictHandle == 0x44u);
        }
    };

    checkHandles(false);
    checkHandles(true);
}

TEST_CASE("DRW_DetailViewStyle rejects a body ending before style fields",
          "[dwg-read][object-encode][viewstyle]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x651);
    encoded.putBitShort(0);       // model-document class version
    encoded.putVariableText(ver, "");
    encoded.putBit(0);            // modified for recompute
    const std::uint32_t bodyEndBit = encoded.bitCount();
    encoded.putBitShort(0);       // class version outside the body
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DetailViewStyle parsed;
    parsed.m_classVersion = 7;
    parsed.m_identifierHeight = 12.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_classVersion == 0u);
    CHECK(parsed.m_identifierHeight == 0.0);
}

TEST_CASE("DRW_SectionViewStyle rejects a body ending before style fields",
          "[dwg-read][object-encode][viewstyle]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x652);
    encoded.putBitShort(0);       // model-document class version
    encoded.putVariableText(ver, "");
    encoded.putBit(0);            // modified for recompute
    const std::uint32_t bodyEndBit = encoded.bitCount();
    encoded.putBitShort(0);       // class version outside the body
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SectionViewStyle parsed;
    parsed.m_classVersion = 7;
    parsed.m_identifierHeight = 12.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_classVersion == 0u);
    CHECK(parsed.m_identifierHeight == 0.0);
}

TEST_CASE("DRW view styles reject non-finite numeric fields",
          "[dwg-read][object-encode][viewstyle][safety]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const auto check = [nan](bool section) {
        auto bytes = emitAc1018ViewStyleObject(section, nan);
        REQUIRE_FALSE(bytes.empty());
        dwgBuffer reader(bytes.data(), bytes.size());
        if (section) {
            DRW_SectionViewStyle parsed;
            parsed.m_identifierHeight = 42.0;
            CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
                parsed, DRW::AC1018, &reader));
            CHECK_FALSE(reader.isGood());
            CHECK(parsed.m_identifierHeight == 0.0);
        } else {
            DRW_DetailViewStyle parsed;
            parsed.m_identifierHeight = 42.0;
            CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
                parsed, DRW::AC1018, &reader));
            CHECK_FALSE(reader.isGood());
            CHECK(parsed.m_identifierHeight == 0.0);
        }
    };
    check(false);
    check(true);
}

TEST_CASE("DRW_EvaluationGraph rejects malformed input transactionally",
          "[dwg-read][object-encode][evalgraph][safety]") {
    DRW_EvaluationGraph graph;
    graph.m_value96 = 99;
    graph.m_nodes.push_back({});

    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        graph, DRW::AC1018, nullptr));
    CHECK(graph.m_value96 == 0);
    CHECK(graph.m_value97 == 0);
    CHECK(graph.m_nodes.empty());
    CHECK(graph.m_edges.empty());

    dwgBufferW truncated;
    emitObjectPreamble(truncated, DRW::AC1018, /*oType=*/0,
                       /*handle=*/0x620, /*numReactors=*/0,
                       /*xDictFlag=*/1);
    truncated.putBitLong(1); // value96; value97 is intentionally absent
    auto bytes = snapshot(truncated);
    dwgBuffer reader(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        graph, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(graph.m_value96 == 0);
    CHECK(graph.m_nodes.empty());
    CHECK(graph.m_edges.empty());
}

TEST_CASE("DRW_EvaluationGraph rejects a count outside its body frame",
          "[dwg-read][object-encode][evalgraph][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0, /*handle=*/0x625,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(0); // value96
    encoded.putBitLong(0); // value97
    encoded.putBitLong(0); // no nodes
    const auto bodyEnd = static_cast<std::uint32_t>(encoded.bitCount());
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x624, {}, 1);
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEnd);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_EvaluationGraph graph;
    REQUIRE_FALSE(DrwObjectEncodeTestAccess::parse(graph, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(graph.m_nodes.empty());
    CHECK(graph.m_edges.empty());
    CHECK(graph.parentHandle == 0u);
}

TEST_CASE("DRW_EvaluationGraph rejects counts above its native limit",
          "[dwg-read][object-encode][evalgraph][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0, /*handle=*/0x626,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(0); // value96
    encoded.putBitLong(0); // value97
    encoded.putBitLong(static_cast<std::int32_t>(
        DRW_EvaluationGraph::kMaxEntries + 1));
    const auto bodyEnd = static_cast<std::uint32_t>(encoded.bitCount());
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x624, {}, 1);
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEnd);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_EvaluationGraph graph;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(graph, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(graph.m_nodes.empty());
    CHECK(graph.m_edges.empty());
    CHECK(graph.parentHandle == 0u);
}

TEST_CASE("DRW_DbColor rejects malformed input transactionally",
          "[dwg-read][object-encode][dbcolor][safety]") {
    DRW_DbColor color;
    color.rgb = 0x102030;
    color.name = "stale";
    color.bookName = "stale-book";
    color.parentHandle = 0x42;

    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        color, DRW::AC1018, nullptr));
    CHECK(color.rgb == -1);
    CHECK(color.name.empty());
    CHECK(color.bookName.empty());
    CHECK(color.parentHandle == 0);

    dwgBufferW truncated;
    emitObjectPreamble(truncated, DRW::AC1018, /*oType=*/1004,
                       /*handle=*/0x630, /*numReactors=*/0,
                       /*xDictFlag=*/1);
    truncated.putBitShort(7); // CMC index; RGB and flags are intentionally absent
    auto bytes = snapshot(truncated);
    dwgBuffer reader(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        color, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(color.rgb == -1);
    CHECK(color.name.empty());
    CHECK(color.bookName.empty());
    CHECK(color.parentHandle == 0);
}

TEST_CASE("DRW_DbColor reads absolute object reactors and xdictionary",
          "[dwg-read][object-encode][dbcolor][object-handles]") {
    auto bytes = emitAc1018DbColorObject();
    REQUIRE_FALSE(bytes.empty());
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DbColor parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1018, &reader));
    CHECK(parsed.rgb == -1);
    CHECK(parsed.colorIndex == 7u);
    CHECK(parsed.parentHandle == 0x42u);
    REQUIRE(parsed.reactorHandles.size() == 1);
    CHECK(parsed.reactorHandles.front() == 0x43u);
    CHECK(parsed.xDictHandle == 0x44u);
}

TEST_CASE("DRW_DbColor writer preserves common object handles",
          "[dwg-write][object-encode][dbcolor]") {
    const DRW::Version ver = DRW::AC1018;
    DRW_DbColor source;
    source.handle = 0x661u;
    source.parentHandle = 0x42u;
    source.reactorHandles = {0x43u};
    source.xDictHandle = 0x44u;
    source.colorIndex = 7u;
    DrwObjectEncodeTestAccess::setNumReactors(source, 1);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 0);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_DbColor::kDwgType, source.handle,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeDbColor(source, ver, &encoded));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DbColor parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.colorIndex == 7u);
    CHECK(parsed.parentHandle == 0x42u);
    REQUIRE(parsed.reactorHandles == std::vector<std::uint32_t>{0x43u});
    CHECK(parsed.xDictHandle == 0x44u);
    CHECK(reader.isGood());
}

TEST_CASE("DRW_DimensionAssociation consumes extended reference records transactionally",
          "[dwg-read][object-encode][dimassoc][safety]") {
    const DRW::Version ver = DRW::AC1021;

    dwgBufferW body;
    emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x640,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    body.putBitLong(0x01); // one osnap point reference
    body.putBit(1);       // trans-space
    body.putRawChar8(2);  // rotated dimension type
    body.putRawChar8(6);  // intersection osnap type
    body.putBitLong(2);   // two xrefs; native replay must remain raw-only
    body.putBitLong(1);   // main subentity type
    body.putBitLong(2);   // main graphics marker
    body.putBitLong(1);   // one xref path
    body.putBitDouble(1.5);
    body.put3BitDouble({1.0, 2.0, 3.0});
    body.putBitLong(1);   // one intersection object
    body.putBitLong(3);   // intersection subentity type
    body.putBitLong(4);   // intersection graphics marker
    body.putBitLong(1);   // one intersection xref path
    body.putBit(1);       // has last point reference

    dwgBufferW strings;
    strings.putVariableText(ver, "AcDbOsnapPointRef");
    strings.putVariableText(ver, ""); // xref path
    strings.putVariableText(ver, ""); // intersection xref path
    body.alignToByte();
    body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u));
    body.putBit(1); // string stream is present

    dwgBufferW handles;
    emitCommonHandlePrefix(handles, /*parentHandle=*/0x42, {}, 1);
    handles.putHandle(hardPtr(0x650)); // dimension handle
    handles.putHandle(hardPtr(0x651)); // first xref
    handles.putHandle(hardPtr(0x652)); // second xref
    handles.putHandle(hardPtr(0x653)); // intersection object

    body.alignToByte();
    body.patchRawLong32AtBit(2,
                             static_cast<std::uint32_t>(body.size() * 8u));
    body.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DimensionAssociation association;
    REQUIRE(DrwObjectEncodeTestAccess::parse(association, ver, &reader));
    CHECK(static_cast<std::uint32_t>(association.parentHandle) == 0x42u);
    CHECK(association.m_dimensionHandle == 0x650u);
    CHECK(association.m_associativityFlags == 0x01u);
    CHECK(association.m_isTransSpace);
    REQUIRE(association.m_osnapRefs.size() == 1);
    CHECK(association.m_osnapRefs.front().m_objectOsnapType == 6);
    CHECK(association.m_osnapRefs.front().m_objectHandle == 0x651u);
    CHECK(association.m_hasUnrepresentableDetail);

    dwgBufferW rejectedBody;
    dwgBufferW rejectedStrings;
    dwgBufferW rejectedHandles;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDimensionAssociation(
        association, ver, &rejectedBody, &rejectedStrings, &rejectedHandles));

    association.m_hasUnrepresentableDetail = false;
    association.m_osnapRefs.front().m_objectOsnapType = 0;
    dwgBufferW representableBody;
    dwgBufferW representableStrings;
    dwgBufferW representableHandles;
    CHECK(DrwObjectEncodeTestAccess::encodeDimensionAssociation(
        association, ver, &representableBody, &representableStrings,
        &representableHandles));

    association.m_dimensionHandle = 0x777;
    association.m_osnapRefs.push_back({"stale", 0, 0x778});
    dwgBuffer truncated(bytes.data(), bytes.size() - 1);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        association, ver, &truncated));
    CHECK_FALSE(truncated.isGood());
    CHECK(association.m_dimensionHandle == 0);
    CHECK(association.m_osnapRefs.empty());
    CHECK(association.parentHandle == 0);
}

TEST_CASE("associative DWG writers report buffer and count failures",
          "[dwg-write][object-encode][associative][safety]") {
    DRW_DimensionAssociation association;
    association.m_associativityFlags = 1;
    association.m_osnapRefs = {{std::string(0xFFFFu, 'x'), 0, 0}};
    dwgBufferW associationBody;
    dwgBufferW associationStrings;
    dwgBufferW associationHandles;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDimensionAssociation(
        association, DRW::AC1021, &associationBody, &associationStrings,
        &associationHandles));
    CHECK_FALSE(associationStrings.isGood());

    DRW_EvaluationGraph graph;
    graph.m_nodes.resize(DRW_EvaluationGraph::kMaxEntries + 1);
    dwgBufferW graphBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeEvaluationGraph(
        graph, DRW::AC1021, &graphBody));
    CHECK(graphBody.data().empty());
}

TEST_CASE("DRW_DimensionAssociation rejects non-finite osnap geometry",
          "[dwg-read][object-encode][dimassoc][safety]") {
    const DRW::Version ver = DRW::AC1021;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    const auto makeRecord = [&](bool badDistance) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x641,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitLong(0x01); // one osnap point reference
        body.putBit(0);        // model space
        body.putRawChar8(0);   // rotated dimension type
        body.putRawChar8(0);   // osnap type
        body.putBitLong(0);    // no xrefs
        body.putBitDouble(badDistance ? nan : 0.0);
        body.put3BitDouble(badDistance
                               ? DRW_Coord{0.0, 0.0, 0.0}
                               : DRW_Coord{nan, 0.0, 0.0});
        body.putBit(0);        // no last-point reference

        dwgBufferW strings;
        strings.putVariableText(ver, "AcDbOsnapPointRef");
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream is present

        dwgBufferW handles;
        emitCommonHandlePrefix(handles, 0x42, {}, /*xDictFlag=*/1);
        handles.putHandle(hardPtr(0x650)); // dimension handle
        body.alignToByte();
        body.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(body.size() * 8u));
        body.putBytes(handles.data().data(), handles.data().size());
        return snapshot(body);
    };

    for (auto bytes : {makeRecord(true), makeRecord(false)}) {
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_DimensionAssociation association;
        association.m_dimensionHandle = 0x777;
        association.m_osnapRefs.push_back({"stale", 0, 0x778});
        association.parentHandle = 0x98;

        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
            association, ver, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(association.m_dimensionHandle == 0);
        CHECK(association.m_osnapRefs.empty());
        CHECK(association.parentHandle == 0);
    }
}

TEST_CASE("DRW_EvaluationGraph encodes and parses an AC1021 object frame",
          "[dwg-read][dwg-write][object-encode][evalgraph]") {
    const DRW::Version ver = DRW::AC1021;
    DRW_EvaluationGraph source;
    source.handle = 0x670u;
    source.parentHandle = 0x42u;
    source.reactorHandles = {0x43u};
    source.xDictHandle = 0x44u;
    source.setDwgCommonObjectState(1, 0, false);
    source.m_value96 = 11;
    source.m_value97 = 22;
    source.m_nodes.push_back({3, 4, 5, 0x51u, 6, 7, 8, 9});
    source.m_edges.push_back({10, 11, 12, 13, 14, 15, 16, 17, 18, 19});

    dwgBufferW body;
    dwgBufferW handles;
    emitObjectPreamble(body, ver, /*oType=*/0, source.handle,
                       source.reactorCount(), source.extensionDictionaryFlag());
    REQUIRE(DrwObjectEncodeTestAccess::encodeEvaluationGraph(
        source, ver, &body, nullptr, &handles));

    body.alignToByte();
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(0); // no detached string stream
    body.putBit(0);
    body.alignToByte();
    handles.alignToByte();
    body.patchRawLong32AtBit(
        objectSizeBitOffset(body), static_cast<std::uint32_t>(body.size() * 8u));
    body.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_EvaluationGraph parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_value96 == 11);
    CHECK(parsed.m_value97 == 22);
    REQUIRE(parsed.m_nodes.size() == 1u);
    CHECK(parsed.m_nodes.front().m_expressionHandle == 0x51u);
    REQUIRE(parsed.m_edges.size() == 1u);
    CHECK(parsed.m_edges.front().m_value92e == 19);
    CHECK(parsed.parentHandle == 0x42u);
    CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x43u});
    CHECK(parsed.xDictHandle == 0x44u);
    CHECK(reader.isGood());
}

TEST_CASE("DRW_Sun validates modern fields and handle framing transactionally",
          "[dwg-read][object-encode][sun][safety]") {
    const DRW::Version ver = DRW::AC1021;

    dwgBufferW body;
    emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x660,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    DRW_Sun source;
    source.parentHandle = 0x42;
    source.m_classVersion = 2;
    source.m_isOn = true;
    source.m_color = 4;
    source.m_intensity = 2.5;
    source.m_hasShadow = true;
    source.m_julianDay = 2460000;
    source.m_milliseconds = 43200000;
    source.m_isDaylightSavings = true;
    source.m_shadowType = 1;
    source.m_shadowMapSize = 512;
    source.m_shadowSoftness = 6;
    dwgBufferW handles;
    REQUIRE(source.encodeDwg(ver, &body, &handles));
    body.alignToByte();
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(0); // no detached color-name strings
    body.putBit(0);        // string stream is absent
    body.alignToByte();
    body.patchRawLong32AtBit(2,
                             static_cast<std::uint32_t>(body.size() * 8u));
    body.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Sun parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.parentHandle == 0x42);
    CHECK(parsed.m_classVersion == 2u);
    CHECK(parsed.m_color == 4u);
    CHECK(parsed.m_shadowMapSize == 512u);

    source.m_classVersion = 11;
    CHECK_FALSE(source.encodeDwg(ver, &body, &handles));

    source.m_classVersion = 2;
    source.m_intensity = std::numeric_limits<double>::quiet_NaN();
    CHECK_FALSE(source.encodeDwg(ver, &body, &handles));

    parsed.m_classVersion = 99;
    parsed.m_color = 123;
    dwgBuffer truncated(bytes.data(), bytes.size() - 1);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &truncated));
    CHECK(parsed.m_classVersion == 0u);
    CHECK(parsed.m_color == 0u);
    CHECK(parsed.parentHandle == 0);
    CHECK_FALSE(truncated.isGood());
}

TEST_CASE("DRW_Sun reads the legacy inline body before its handles",
          "[dwg-read][object-encode][sun][ac1015]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW body;
    emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x661,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    body.putBitLong(1);       // class version
    body.putBit(1);            // enabled
    body.putCmColor(ver, 3);  // ACI color
    body.putBitDouble(0.85);
    body.putBit(1);            // shadows enabled
    body.putBitLong(2460828);
    body.putBitLong(43200000);
    body.putBit(0);            // daylight savings
    body.putBitLong(1);        // shadow type
    body.putBitShort(1024);
    body.putRawChar8(16);
    emitCommonHandlePrefix(body, /*parentHandle=*/0x42, {}, /*xDictFlag=*/0);

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Sun parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_classVersion == 1u);
    CHECK(parsed.m_isOn);
    CHECK(parsed.m_color == 3u);
    CHECK(parsed.m_intensity == Approx(0.85));
    CHECK(parsed.parentHandle == 0x42u);
    CHECK(parsed.xDictHandle == 0u);
    CHECK(reader.isGood());
}

TEST_CASE("DRW_Sun rejects non-finite body values before handles",
          "[dwg-read][object-encode][sun][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0,
                       /*handle=*/0x662);
    encoded.putBitLong(1);       // class version
    encoded.putBit(1);           // enabled
    encoded.putCmColor(version, 3);
    encoded.putBitDouble(nan);   // intensity

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Sun parsed;
    parsed.m_classVersion = 9;
    parsed.m_intensity = 4.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_classVersion == 0u);
    CHECK(parsed.m_intensity == Approx(0.0));
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_SpatialIndex rejects a truncated modern common handle stream",
          "[dwg-read][object-encode][spatialindex][safety]") {
    const DRW::Version ver = DRW::AC1021;

    dwgBufferW valid;
    emitObjectPreamble(valid, ver, /*oType=*/0, /*handle=*/0x620,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    DRW_SpatialIndex source;
    source.timestamp1 = 123u;
    source.timestamp2 = 456u;
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);
    dwgBufferW handles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeSpatialIndex(
        source, ver, &valid, &handles));
    valid.putBit(0); // empty R2007+ string-stream presence flag
    valid.alignToByte();
    valid.patchRawLong32AtBit(2,
                              static_cast<std::uint32_t>(valid.size() * 8u));
    valid.putBytes(handles.data().data(), handles.data().size());
    auto validBytes = snapshot(valid);
    dwgBuffer validReader(const_cast<std::uint8_t*>(validBytes.data()),
                          validBytes.size());
    DRW_SpatialIndex parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.timestamp1 == 123u);
    CHECK(parsed.timestamp2 == 456u);

    dwgBufferW truncated;
    emitObjectPreamble(truncated, ver, /*oType=*/0, /*handle=*/0x621,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    truncated.putBitLong(123);
    truncated.putBitLong(456);
    truncated.putBit(0);
    truncated.alignToByte();
    truncated.patchRawLong32AtBit(2,
                                  static_cast<std::uint32_t>(truncated.size() * 8u));
    auto truncatedBytes = snapshot(truncated);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK(parsed.timestamp1 == 0u);
    CHECK(parsed.timestamp2 == 0u);
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
    CHECK_FALSE(truncatedReader.isGood());
}

TEST_CASE("DRW_TableGeometry publishes cells transactionally",
          "[dwg-read][object-encode][tablegeometry][safety]") {
    const DRW::Version ver = DRW::AC1021;

    auto appendFooterAndHandles = [](dwgBufferW& body,
                                     std::uint32_t parentHandle,
                                     const std::vector<std::uint32_t>& cellHandles,
                                     const std::vector<std::uint32_t>& reactors = {},
                                     std::uint8_t xDictFlag = 1,
                                     std::uint32_t xDictHandle = 0) {
        body.putBit(0); // empty R2007+ string-stream presence flag
        body.alignToByte();
        body.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(body.size() * 8u));
        dwgBufferW handles;
        handles.putHandle(hardPtr(parentHandle));
        for (std::uint32_t reactor : reactors)
            handles.putHandle(handleWithCode(0xA, reactor));
        if (xDictFlag != 1)
            handles.putHandle(handleWithCode(0xC, xDictHandle));
        for (std::uint32_t cellHandle : cellHandles)
            handles.putHandle(hardPtr(cellHandle));
        body.putBytes(handles.data().data(), handles.data().size());
    };

    dwgBufferW valid;
    emitObjectPreamble(valid, ver, /*oType=*/0, /*handle=*/0x650,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    valid.putBitLong(1); // rows
    valid.putBitLong(1); // columns
    valid.putBitLong(1); // cells
    valid.putBitLong(7); // cell flags
    valid.putBitDouble(1.5);
    valid.putBitDouble(2.5);
    valid.putBitLong(1); // content geometry count
    valid.put3BitDouble(DRW_Coord(1.0, 2.0, 3.0));
    valid.put3BitDouble(DRW_Coord(4.0, 5.0, 6.0));
    valid.putBitDouble(7.0);
    valid.putBitDouble(8.0);
    valid.putBitDouble(9.0);
    valid.putBitDouble(10.0);
    valid.putBitLong(11);
    appendFooterAndHandles(valid, 0x45, {0x48}, {0x46}, 0, 0x47);

    auto validBytes = snapshot(valid);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_TableGeometry parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.m_rowCount == 1);
    CHECK(parsed.m_columnCount == 1);
    CHECK(parsed.parentHandle == 0x45u);
    CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x46u});
    CHECK(parsed.xDictHandle == 0x47u);
    REQUIRE(parsed.m_cells.size() == 1u);
    CHECK(parsed.m_cells.front().m_flags == 7);
    CHECK(parsed.m_cells.front().m_unknownHandle == 0x48u);
    REQUIRE(parsed.m_cells.front().m_contents.size() == 1u);
    CHECK(parsed.m_cells.front().m_contents.front().m_unknown == 11);

    dwgBufferW truncated;
    emitObjectPreamble(truncated, ver, /*oType=*/0, /*handle=*/0x651,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    truncated.putBitLong(1); // rows
    truncated.putBitLong(2); // columns
    truncated.putBitLong(2); // cells
    truncated.putBitLong(7);
    truncated.putBitDouble(1.5);
    truncated.putBitDouble(2.5);
    truncated.putBitLong(0); // first cell has no content geometry
    truncated.putBitLong(8);
    truncated.putBitDouble(3.5);
    truncated.putBitDouble(4.5);
    truncated.putBitLong(1); // second cell is missing its content body
    appendFooterAndHandles(truncated, 0x47, {0x4A, 0x4B}, {0x48}, 0, 0x49);

    auto truncatedBytes = snapshot(truncated);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    parsed.parentHandle = 0x99;
    parsed.reactorHandles.push_back(0x98);
    parsed.xDictHandle = 0x97;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_cells.empty());
    CHECK(parsed.m_rowCount == 0);
    CHECK(parsed.m_columnCount == 0);
    CHECK(parsed.m_cellCount == 0);
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("DRW_TableGeometry rejects inconsistent declared counts",
          "[dwg-read][object-encode][tablegeometry][safety]") {
    const DRW::Version ver = DRW::AC1021;
    dwgBufferW invalid;
    emitObjectPreamble(invalid, ver, /*oType=*/0, /*handle=*/0x652,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    invalid.putBitLong(0); // rows
    invalid.putBitLong(0); // columns
    invalid.putBitLong(1); // impossible nonzero cell count
    invalid.putBit(0); // empty R2007+ string-stream presence flag
    invalid.alignToByte();
    invalid.patchRawLong32AtBit(
        2, static_cast<std::uint32_t>(invalid.size() * 8u));
    dwgBufferW handles;
    emitCommonHandlePrefix(handles, 0x45, {}, /*xDictFlag=*/1);
    invalid.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(invalid);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_TableGeometry parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_cells.empty());
    CHECK(parsed.m_rowCount == 0);
    CHECK(parsed.m_columnCount == 0);
    CHECK(parsed.m_cellCount == 0);
    CHECK(parsed.parentHandle == 0);
}

TEST_CASE("DRW_TableGeometry rejects a content count outside its body frame",
          "[dwg-read][object-encode][tablegeometry][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0, /*handle=*/0x654,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(1); // rows
    encoded.putBitLong(1); // columns
    encoded.putBitLong(1); // cells
    encoded.putBitLong(0); // cell flags
    encoded.putBitDouble(1.0);
    encoded.putBitDouble(1.0);
    encoded.putBit(0); // empty R2007+ string-stream footer
    encoded.alignToByte();
    encoded.patchRawLong32AtBit(
        objectSizeBitOffset(encoded),
        static_cast<std::uint32_t>(encoded.size() * 8u));
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x45, {}, 1);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_TableGeometry parsed;
    REQUIRE_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_cells.empty());
    CHECK(parsed.m_rowCount == 0);
    CHECK(parsed.m_columnCount == 0);
    CHECK(parsed.m_cellCount == 0);
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_TableGeometry accepts minimum-width compressed content geometry",
          "[dwg-read][object-encode][tablegeometry]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0, /*handle=*/0x655,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(1); // rows
    encoded.putBitLong(1); // columns
    encoded.putBitLong(1); // cells
    encoded.putBitLong(0); // cell flags
    encoded.putBitDouble(0.0);
    encoded.putBitDouble(0.0);
    encoded.putBitLong(3); // content geometry count
    for (int i = 0; i < 3; ++i) {
        encoded.put3BitDouble(DRW_Coord(0.0, 0.0, 0.0));
        encoded.put3BitDouble(DRW_Coord(0.0, 0.0, 0.0));
        encoded.putBitDouble(0.0);
        encoded.putBitDouble(0.0);
        encoded.putBitDouble(0.0);
        encoded.putBitDouble(0.0);
        encoded.putBitLong(0);
    }
    encoded.putBit(0); // empty R2007+ string-stream presence flag
    encoded.alignToByte();
    encoded.patchRawLong32AtBit(
        objectSizeBitOffset(encoded),
        static_cast<std::uint32_t>(encoded.size() * 8u));
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x45, {}, 1);
    encoded.putHandle(hardPtr(0x48));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_TableGeometry parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    REQUIRE(parsed.m_cells.size() == 1u);
    CHECK(parsed.m_cells.front().m_contents.size() == 3u);
    CHECK(parsed.m_cells.front().m_contents.front().m_topLeft.x == 0.0);
    CHECK(parsed.m_cells.front().m_unknownHandle == 0x48u);
}

TEST_CASE("DRW_TableGeometry rejects non-finite geometry values transactionally",
          "[dwg-read][object-encode][tablegeometry][safety]") {
    const DRW::Version ver = DRW::AC1021;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    const auto makeRecord = [&](bool badCellSize) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x653,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitLong(1); // rows
        body.putBitLong(1); // columns
        body.putBitLong(1); // cells
        body.putBitLong(7); // cell flags
        body.putBitDouble(badCellSize ? nan : 1.0);
        body.putBitDouble(2.0);
        body.putBitLong(1); // content geometry count
        body.put3BitDouble(badCellSize
                               ? DRW_Coord{1.0, 2.0, 3.0}
                               : DRW_Coord{nan, 2.0, 3.0});
        body.put3BitDouble(DRW_Coord{4.0, 5.0, 6.0});
        body.putBitDouble(7.0);
        body.putBitDouble(8.0);
        body.putBitDouble(9.0);
        body.putBitDouble(10.0);
        body.putBitLong(11);
        body.putBit(0); // empty R2007+ string-stream presence flag
        body.alignToByte();
        body.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(body.size() * 8u));
        emitCommonHandlePrefix(body, 0x45, {}, /*xDictFlag=*/1);
        body.putHandle(hardPtr(0x48));
        return snapshot(body);
    };

    for (auto bytes : {makeRecord(true), makeRecord(false)}) {
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_TableGeometry parsed;
        parsed.m_rowCount = 99;
        parsed.m_cells.push_back({});
        parsed.parentHandle = 0x98;

        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.m_rowCount == 0);
        CHECK(parsed.m_columnCount == 0);
        CHECK(parsed.m_cellCount == 0);
        CHECK(parsed.m_cells.empty());
        CHECK(parsed.parentHandle == 0);
    }
}

TEST_CASE("DRW_TableContentObject publishes complete state transactionally",
          "[dwg-read][object-encode][tablecontent][safety]") {
    const DRW::Version ver = DRW::AC1021;

    auto makeRecord = [ver](bool truncateStyleHandle,
                            bool omitBodyCounts = false) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x660,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        if (!omitBodyCounts) {
            body.putBitLong(0); // columns
            body.putBitLong(0); // rows
            body.putBitLong(0); // field references
            body.putBitLong(0); // cell-style type
            body.putBitShort(0); // cell-style data flags
            body.putBitLong(0); // merged ranges
        }

        dwgBufferW strings;
        strings.putVariableText(ver, std::string{}); // name
        strings.putVariableText(ver, std::string{}); // description
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        body.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(body.size() * 8u));

        dwgBufferW handles;
        handles.putHandle(hardPtr(0x45)); // parent
        handles.putHandle(hardPtr(0x46)); // reactor
        handles.putHandle(hardPtr(0x47)); // xdictionary
        handles.putHandle(hardPtr(0x48)); // table style
        auto bytes = snapshot(body);
        const auto handleBytes = handles.data();
        bytes.insert(bytes.end(), handleBytes.begin(), handleBytes.end());
        if (truncateStyleHandle)
            bytes.pop_back();
        return bytes;
    };

    DRW_TableContentObject nullInput;
    nullInput.parentHandle = 0x99;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(nullInput, ver, nullptr));
    CHECK_FALSE(nullInput.m_parseComplete);
    CHECK(nullInput.parentHandle == 0);

    auto validBytes = makeRecord(false);
    DRW_TextCodec codec;
    codec.setVersion(ver, false);
    dwgBuffer validReader(validBytes.data(), validBytes.size(), &codec);
    DRW_TableContentObject parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.m_parseComplete);
    CHECK(parsed.parentHandle == 0x45);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x46u);
    CHECK(parsed.xDictHandle == 0x47u);
    CHECK(parsed.m_content.m_tableStyleHandle == 0x48u);

    auto missingBodyBytes = makeRecord(false, true);
    dwgBuffer missingBodyReader(missingBodyBytes.data(),
                               missingBodyBytes.size(), &codec);
    parsed.parentHandle = 0x99;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &missingBodyReader));
    CHECK_FALSE(missingBodyReader.isGood());
    CHECK_FALSE(parsed.m_parseComplete);
    CHECK(parsed.m_content.m_columns.empty());
    CHECK(parsed.m_content.m_rows.empty());
    CHECK(parsed.parentHandle == 0);

    auto truncatedBytes = makeRecord(true);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size(),
                              &codec);
    parsed.parentHandle = 0x99;
    parsed.reactorHandles.push_back(0x98);
    parsed.xDictHandle = 0x97;
    parsed.m_content.m_columns.resize(1);
    parsed.m_parseComplete = true;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK_FALSE(parsed.m_parseComplete);
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
    CHECK(parsed.m_content.m_columns.empty());
    CHECK(parsed.m_content.m_tableStyleHandle == 0u);
}

TEST_CASE("DRW_CellStyleMap rejects partial and oversized records",
          "[dwg-read][object-encode][cellstylemap][safety]") {
    const DRW::Version ver = DRW::AC1021;

    auto makeRecord = [ver](std::int32_t cellStyleCount,
                            bool truncateHandle) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x670,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        body.putBitLong(cellStyleCount);
        if (cellStyleCount == 1) {
            body.putBitLong(0); // cell-style type
            body.putBitShort(0); // cell-style data flags
            body.putBitLong(7); // style id
            body.putBitLong(8); // style class
        }

        dwgBufferW strings;
        strings.putVariableText(ver, std::string{}); // style name
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        body.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(body.size() * 8u));

        dwgBufferW handles;
        handles.putHandle(hardPtr(0x45)); // parent
        handles.putHandle(hardPtr(0x46)); // reactor
        handles.putHandle(hardPtr(0x47)); // xdictionary
        auto bytes = snapshot(body);
        const auto handleBytes = handles.data();
        bytes.insert(bytes.end(), handleBytes.begin(), handleBytes.end());
        if (truncateHandle)
            bytes.pop_back();
        return bytes;
    };

    DRW_CellStyleMap nullInput;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(nullInput, ver, nullptr));
    CHECK(nullInput.m_cellStyles.empty());

    auto validBytes = makeRecord(1, false);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_CellStyleMap parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    REQUIRE(parsed.m_cellStyles.size() == 1u);
    CHECK(parsed.m_cellStyles.front().m_id == 7);
    CHECK(parsed.m_cellStyles.front().m_styleClass == 8);
    CHECK(parsed.parentHandle == 0x45);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x46u);
    CHECK(parsed.xDictHandle == 0x47u);

    auto truncatedBytes = makeRecord(1, true);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    parsed.parentHandle = 0x99;
    parsed.reactorHandles.push_back(0x98);
    parsed.xDictHandle = 0x97;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_cellStyles.empty());
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);

    auto oversizedBytes = makeRecord(100001, false);
    dwgBuffer oversizedReader(oversizedBytes.data(), oversizedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &oversizedReader));
    CHECK(parsed.m_cellStyles.empty());
    CHECK(parsed.parentHandle == 0);
}

TEST_CASE("DRW_CellStyleMap reads a modern bounded handle stream",
          "[dwg-read][object-encode][cellstylemap][ac1024]") {
    const DRW::Version version = DRW::AC1024;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x690,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    body.putBitLong(1); // one cell style
    body.putBitLong(0); // cell-style type
    body.putBitShort(0); // no cell-style data
    body.putBitLong(7); // style id
    body.putBitLong(8); // style class

    dwgBufferW strings;
    strings.putVariableText(version, "ModernCellStyle");
    body.alignToByte();
    body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u));
    body.putBit(1); // string stream present
    body.alignToByte();

    dwgBufferW handles;
    handles.putHandle(hardPtr(0x45)); // parent
    handles.putHandle(hardPtr(0x46)); // reactor
    handles.putHandle(hardPtr(0x47)); // xdictionary
    handles.alignToByte();
    const std::uint32_t handleBits = static_cast<std::uint32_t>(
        handles.data().size() * 8u);
    auto bytes = snapshot(body);
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());

    DRW_TextCodec decoder;
    decoder.setVersion(version, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_CellStyleMap parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader,
                                              handleBits));
    REQUIRE(parsed.m_cellStyles.size() == 1u);
    CHECK(parsed.m_cellStyles.front().m_id == 7);
    CHECK(parsed.m_cellStyles.front().m_styleClass == 8);
    CHECK(parsed.m_cellStyles.front().m_name == "ModernCellStyle");
    CHECK(parsed.parentHandle == 0x45u);
    CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x46u});
    CHECK(parsed.xDictHandle == 0x47u);

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size(), &decoder);
    DRW_CellStyleMap rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &truncatedReader, handleBits));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(rejected.m_cellStyles.empty());
    CHECK(rejected.parentHandle == 0u);
    CHECK(rejected.reactorHandles.empty());
    CHECK(rejected.xDictHandle == 0u);
}

TEST_CASE("DRW_TableStyle rejects partial modern records",
          "[dwg-read][object-encode][tablestyle][safety]") {
    const DRW::Version ver = DRW::AC1024;
    constexpr std::uint32_t fullHandleBits = 64;

    auto makeRecord = [ver](std::int32_t cellStyleCount,
                            bool truncateHandle) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x680,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        body.putRawChar8(1); // class version
        body.putBitLong(0); // unknown BL 1
        body.putBitLong(0); // unknown BL 2
        body.putBitLong(0); // cell-style type
        body.putBitShort(0); // cell-style data flags
        body.putBitLong(7); // cell-style id
        body.putBitLong(8); // cell-style class
        body.putBitLong(cellStyleCount);

        dwgBufferW strings;
        strings.putVariableText(ver, std::string{}); // table style name
        strings.putVariableText(ver, std::string{}); // cell style name
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();

        dwgBufferW handles;
        handles.putHandle(hardPtr(0x45)); // parent
        handles.putHandle(hardPtr(0x46)); // reactor
        handles.putHandle(hardPtr(0x47)); // xdictionary
        handles.putHandle(hardPtr(0x48)); // table-style cell-style handle
        auto bytes = snapshot(body);
        const auto handleBytes = handles.data();
        bytes.insert(bytes.end(), handleBytes.begin(), handleBytes.end());
        if (truncateHandle)
            bytes.pop_back();
        return bytes;
    };

    DRW_TableStyle nullInput;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        nullInput, ver, nullptr, fullHandleBits));
    CHECK(nullInput.m_cellStyles.empty());

    auto validBytes = makeRecord(0, false);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_TableStyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &validReader, fullHandleBits));
    CHECK(parsed.m_name.empty());
    CHECK(parsed.m_tableCellStyle.m_id == 7);
    CHECK(parsed.m_tableCellStyle.m_styleClass == 8);
    CHECK(parsed.m_unknownHandle == 0x48u);
    CHECK(parsed.parentHandle == 0x45);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x46u);
    CHECK(parsed.xDictHandle == 0x47u);

    auto truncatedBytes = makeRecord(0, true);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    parsed.parentHandle = 0x99;
    parsed.reactorHandles.push_back(0x98);
    parsed.xDictHandle = 0x97;
    parsed.m_unknownHandle = 0x96;
    parsed.m_tableCellStyle.m_id = 0x95;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader, fullHandleBits - 8));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_cellStyles.empty());
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
    CHECK(parsed.m_unknownHandle == 0u);
    CHECK(parsed.m_tableCellStyle.m_id == 0);

    auto oversizedBytes = makeRecord(100001, false);
    dwgBuffer oversizedReader(oversizedBytes.data(), oversizedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &oversizedReader, fullHandleBits));
    CHECK(parsed.m_cellStyles.empty());
    CHECK(parsed.parentHandle == 0);
}

TEST_CASE("DRW_DynamicBlockObject rejects a truncated common handle stream",
          "[dwg-read][object-encode][dynblock][safety]") {
    const DRW::Version ver = DRW::AC1015;

    dwgBufferW valid;
    emitObjectPreamble(valid, ver, /*oType=*/0, /*handle=*/0x660,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    const std::uint32_t objectSize = valid.bitCount();
    valid.putHandle(hardPtr(0x42));
    valid.putHandle(handleWithCode(0xA, 0x43));
    valid.putHandle(handleWithCode(0xC, 0x44));
    valid.patchRawLong32AtBit(2, objectSize);

    auto validBytes = snapshot(valid);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_DynamicBlockObject parsed("BLOCKPROPERTIESTABLE");
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.parentHandle == 0x42);

    parsed.m_evalExprParsed = true;
    parsed.m_bodyFullyDecoded = true;
    parsed.m_stateNames.push_back("stale");
    parsed.reactorHandles.push_back(0x99u);
    parsed.xDictHandle = 0x9Au;
    auto truncatedBytes = validBytes;
    REQUIRE(truncatedBytes.size() > 5);
    truncatedBytes.resize(truncatedBytes.size() - 5);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK_FALSE(parsed.m_evalExprParsed);
    CHECK_FALSE(parsed.m_bodyFullyDecoded);
    CHECK(parsed.m_stateNames.empty());
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0);
}

TEST_CASE("DRW_DynamicBlockObject retains eval expression handle values",
          "[dwg-read][object-encode][dynblock][safety]") {
    const DRW::Version ver = DRW::AC1015;

    dwgBufferW valid;
    emitObjectPreamble(valid, ver, /*oType=*/0, /*handle=*/0x661,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    valid.putBitLong(-1);  // evalexpr parentid
    valid.putBitLong(33);  // evalexpr major
    valid.putBitLong(29);  // evalexpr minor
    valid.putBitShort(91); // evalexpr value_code: handle
    valid.putBitLong(7);   // evalexpr nodeid
    const std::uint32_t objectSize = valid.bitCount();
    valid.putHandle(hardPtr(0x42));
    valid.putHandle(handleWithCode(0xA, 0x43));
    valid.putHandle(handleWithCode(0xC, 0x44));
    valid.putHandle(hardPtr(0x99)); // evalexpr.value.handle91
    valid.patchRawLong32AtBit(2, objectSize);

    auto validBytes = snapshot(valid);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_DynamicBlockObject parsed("DYNAMICBLOCKPROXYNODE");
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.m_evalExprParsed);
    CHECK(parsed.m_valueCode == 91);
    CHECK(parsed.m_valueHandle == 0x99u);
    REQUIRE(parsed.reactorHandles.size() == 1);
    CHECK(parsed.reactorHandles.front() == 0x43u);
    CHECK(parsed.xDictHandle == 0x44u);

    auto truncatedBytes = validBytes;
    REQUIRE(truncatedBytes.size() > 1);
    truncatedBytes.pop_back();
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK_FALSE(parsed.m_evalExprParsed);
    CHECK(parsed.m_valueHandle == 0u);
}

TEST_CASE("DRW_DynamicBlockObject decodes purge preventer fields transactionally",
          "[dwg-read][object-encode][dynblock][safety]") {
    const DRW::Version ver = DRW::AC1015;

    const auto makeRecord = [ver](std::uint8_t blockCode) {
        dwgBufferW record;
        emitObjectPreamble(record, ver, /*oType=*/0, /*handle=*/0x663,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        record.putBitShort(1); // AcDbDynamicBlockPurgePreventer flag
        const std::uint32_t objectSize = record.bitCount();
        record.putHandle(hardPtr(0x42));
        record.putHandle(handleWithCode(0xA, 0x43));
        record.putHandle(handleWithCode(0xC, 0x44));
        record.putHandle(handleWithCode(blockCode, 0x99)); // BLOCK handle
        record.patchRawLong32AtBit(2, objectSize);
        return snapshot(record);
    };

    auto validBytes = makeRecord(0x5);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_DynamicBlockObject parsed("ACDB_DYNAMICBLOCKPURGEPREVENTER_VERSION");
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.m_kind == DRW_DynamicBlockObject::Kind::PurgePreventer);
    CHECK(parsed.m_purgeFlag == 1);
    CHECK(parsed.m_purgeBlockHandle == 0x99u);
    CHECK(parsed.m_bodyFullyDecoded);
    CHECK(parsed.parentHandle == 0x42u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x43u);
    CHECK(parsed.xDictHandle == 0x44u);

    auto wrongCodeBytes = makeRecord(0x4);
    dwgBuffer wrongCodeReader(wrongCodeBytes.data(), wrongCodeBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &wrongCodeReader));
    CHECK_FALSE(wrongCodeReader.isGood());
    CHECK(parsed.m_purgeFlag == 0);
    CHECK(parsed.m_purgeBlockHandle == 0u);
    CHECK_FALSE(parsed.m_bodyFullyDecoded);
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);

    auto truncatedBytes = validBytes;
    REQUIRE(truncatedBytes.size() > 1u);
    truncatedBytes.pop_back();
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    parsed.m_purgeFlag = 7;
    parsed.m_purgeBlockHandle = 0x98u;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_purgeFlag == 0);
    CHECK(parsed.m_purgeBlockHandle == 0u);
    CHECK_FALSE(parsed.m_bodyFullyDecoded);
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("BLOCKPARAMDEPENDENCYBODY fixed fields are transactional",
          "[dwg-read][object-encode][dynblock][safety]") {
    const DRW::Version version = DRW::AC1021;

    auto makeRecord = [version](bool complete) {
        dwgBufferW body;
        emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x662,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitShort(1); // AcDbAssocDependencyBody adb_version
        body.putBitShort(1); // AcDbImpAssocDimDependencyBodyBase version
        if (complete)
            body.putBitShort(0); // AcDbBlockParameterDependencyBody version
        else
            body.put2Bits(0); // truncated BS: selector without its RS payload

        dwgBufferW strings;
        strings.putVariableText(version, "Radius=1.0000");
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        body.patchRawLong32AtBit(
            objectSizeBitOffset(body), static_cast<std::uint32_t>(
                body.bitCount()));

        dwgBufferW handles;
        emitObjectHandlePrefix(handles, /*parentHandle=*/0x42,
                               /*reactorHandles=*/{}, /*xDictHandle=*/0,
                               /*xDictFlag=*/1);
        auto bytes = snapshot(body);
        bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
        return bytes;
    };

    const auto validBytes = makeRecord(true);
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBuffer validReader(const_cast<std::uint8_t *>(validBytes.data()),
                          validBytes.size(), &codec);
    DRW_DynamicBlockObject parsed("BLOCKPARAMDEPENDENCYBODY");
    REQUIRE(DrwObjectEncodeTestAccess::parseDynamicBlock(
        parsed, version, &validReader));
    CHECK(parsed.m_adbVersion == 1);
    CHECK(parsed.m_dimensionBaseVersion == 1);
    CHECK(parsed.m_dependencyName == "Radius=1.0000");
    CHECK(parsed.m_classVersion == 0);
    CHECK(parsed.m_bodyFullyDecoded);
    CHECK(parsed.parentHandle == 0x42u);

    const auto truncatedBytes = makeRecord(false);
    dwgBuffer truncatedReader(
        const_cast<std::uint8_t *>(truncatedBytes.data()),
        truncatedBytes.size(), &codec);
    parsed.m_adbVersion = 9;
    parsed.m_dependencyName = "stale";
    parsed.m_bodyFullyDecoded = true;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDynamicBlock(
        parsed, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_adbVersion == 0);
    CHECK(parsed.m_dimensionBaseVersion == 0);
    CHECK(parsed.m_dependencyName.empty());
    CHECK(parsed.m_classVersion == 0);
    CHECK_FALSE(parsed.m_bodyFullyDecoded);
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("dynamic detached text cannot consume its string footer",
          "[dwg-read][object-encode][dynblock][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x667,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    body.putBitShort(1); // AcDbAssocDependencyBody version
    body.putBitShort(1); // AcDbImpAssocDimDependencyBodyBase version
    body.putBitShort(0); // BLOCKPARAMDEPENDENCYBODY class version

    // Declare two UTF-16 code units but provide only one. The footer describes
    // the actual stream, so accepting this requires reading beyond stringEnd.
    dwgBufferW strings;
    strings.putBitShort(2);
    strings.putRawChar8('x');
    strings.putRawChar8(0);
    body.alignToByte();
    body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u));
    body.putBit(1); // string stream is present
    body.alignToByte();
    body.patchRawLong32AtBit(
        objectSizeBitOffset(body), static_cast<std::uint32_t>(body.bitCount()));

    dwgBufferW handles;
    emitCommonHandlePrefix(handles, /*parentHandle=*/0x42, {}, 1);
    auto bytes = snapshot(body);
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());

    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DynamicBlockObject parsed("BLOCKPARAMDEPENDENCYBODY");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDynamicBlock(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_adbVersion == 0);
    CHECK(parsed.m_dimensionBaseVersion == 0);
    CHECK(parsed.m_dependencyName.empty());
    CHECK(parsed.m_classVersion == 0);
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("BLOCKVISIBILITYPARAMETER rejects counts beyond its body",
          "[dwg-read][object-encode][dynblock][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x663,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    body.putBitLong(0); // evalexpr parentid
    body.putBitLong(1); // evalexpr major
    body.putBitLong(2); // evalexpr minor
    body.putSBitShort(-9999); // evalexpr value_code: no value
    body.putBitLong(3); // evalexpr nodeid
    body.putVariableText(version, "Visibility"); // element name
    body.putBitLong(25); // element major
    body.putBitLong(104); // element minor
    body.putBitLong(0); // element eed1071
    body.putBit(0); // show_properties
    body.putBit(0); // chain_actions
    body.put3BitDouble(DRW_Coord{}); // default point
    body.putBitLong(0); // property-info 1 connection count
    body.putBitLong(0); // property-info 2 connection count
    body.putBitLong(2); // property-info count
    body.putBit(1); // is_initialized
    body.putVariableText(version, "Visibility");
    body.putVariableText(version, "Visibility description");
    body.putBit(0); // unknown bool
    body.putBitLong(0); // block count
    body.putBitLong(100001); // state count exceeds the remaining body
    body.patchRawLong32AtBit(
        objectSizeBitOffset(body), static_cast<std::uint32_t>(body.bitCount()));
    body.putHandle(hardPtr(0x42));

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DynamicBlockObject parsed("BLOCKVISIBILITYPARAMETER");
    parsed.m_stateCount = 7;
    parsed.m_stateNames.push_back("stale");
    parsed.parentHandle = 0x99;

    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDynamicBlock(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_stateCount == 0);
    CHECK(parsed.m_stateNames.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("two-point dynamic parameters reject an oversized property group",
          "[dwg-read][object-encode][dynblock][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x664,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    body.putBitLong(0); // evalexpr parentid
    body.putBitLong(1); // evalexpr major
    body.putBitLong(2); // evalexpr minor
    body.putSBitShort(-9999); // evalexpr value_code: no value
    body.putBitLong(3); // evalexpr nodeid
    body.putVariableText(version, "Linear"); // element name
    body.putBitLong(25); // element major
    body.putBitLong(104); // element minor
    body.putBitLong(0); // element eed1071
    body.putBit(0); // show_properties
    body.putBit(0); // chain_actions
    body.put3BitDouble(DRW_Coord{}); // first point
    body.put3BitDouble(DRW_Coord{}); // second point
    body.putBitShort(1000); // group count exceeds the remaining body
    body.patchRawLong32AtBit(
        objectSizeBitOffset(body), static_cast<std::uint32_t>(body.bitCount()));
    body.putHandle(hardPtr(0x42));

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DynamicBlockObject parsed("BLOCKLINEARPARAMETER");
    parsed.m_firstPoint = DRW_Coord{9.0, 9.0, 9.0};
    parsed.m_twoPointPropertyCodes[0].push_back(7);
    parsed.parentHandle = 0x99;

    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDynamicBlock(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_firstPoint.x == 0.0);
    CHECK(parsed.m_twoPointPropertyCodes[0].empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("dynamic value sets reject values beyond the body",
          "[dwg-read][object-encode][dynblock][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x665,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    body.putBitLong(0); // evalexpr parentid
    body.putBitLong(1); // evalexpr major
    body.putBitLong(2); // evalexpr minor
    body.putSBitShort(-9999); // evalexpr value_code: no value
    body.putBitLong(3); // evalexpr nodeid
    body.putVariableText(version, "Rotation"); // element name
    body.putBitLong(25); // element major
    body.putBitLong(104); // element minor
    body.putBitLong(0); // element eed1071
    body.putBit(0); // show_properties
    body.putBit(0); // chain_actions
    body.put3BitDouble(DRW_Coord{}); // first point
    body.put3BitDouble(DRW_Coord{}); // second point
    for (int i = 0; i < 4; ++i)
        body.putBitShort(0); // empty property group
    for (int i = 0; i < 4; ++i)
        body.putBitLong(0); // property state
    body.putBitShort(0); // parameter base location
    body.put3BitDouble(DRW_Coord{}); // base angle point
    body.putVariableText(version, "Angle");
    body.putVariableText(version, "Angle description");
    body.putBitDouble(0.0); // angle
    body.putBitLong(0); // value-set flags
    body.putBitDouble(0.0); // minimum
    body.putBitDouble(1.0); // maximum
    body.putBitDouble(0.1); // increment
    body.putBitShort(10000); // no room for the declared values
    body.patchRawLong32AtBit(
        objectSizeBitOffset(body), static_cast<std::uint32_t>(body.bitCount()));
    body.putHandle(hardPtr(0x42));

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DynamicBlockObject parsed("BLOCKROTATIONPARAMETER");
    parsed.m_rotationValues.push_back(9.0);
    parsed.parentHandle = 0x99;

    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDynamicBlock(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_rotationValues.empty());
    CHECK(parsed.m_rotationDeclaredValueCount == 0);
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("flip dynamic parameter suffix rejects a truncated field",
          "[dwg-read][object-encode][dynblock][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x666,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    body.putBitLong(0); // evalexpr parentid
    body.putBitLong(1); // evalexpr major
    body.putBitLong(2); // evalexpr minor
    body.putSBitShort(-9999); // evalexpr value_code: no value
    body.putBitLong(3); // evalexpr nodeid
    body.putVariableText(version, "Flip"); // element name
    body.putBitLong(25); // element major
    body.putBitLong(104); // element minor
    body.putBitLong(0); // element eed1071
    body.putBit(0); // show_properties
    body.putBit(0); // chain_actions
    body.put3BitDouble(DRW_Coord{}); // first point
    body.put3BitDouble(DRW_Coord{}); // second point
    for (int i = 0; i < 4; ++i)
        body.putBitShort(0); // empty property groups
    for (int i = 0; i < 4; ++i)
        body.putBitLong(0); // property states
    body.putBitShort(0); // parameter base location
    body.putVariableText(version, "Caption");
    body.putVariableText(version, "Description");
    body.putVariableText(version, "Base");
    body.putVariableText(version, "Flipped");
    body.put3BitDouble(DRW_Coord{}); // caption location
    body.putVariableText(version, "Tooltip");
    body.patchRawLong32AtBit(
        objectSizeBitOffset(body), static_cast<std::uint32_t>(body.bitCount()));
    body.putHandle(hardPtr(0x42));

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DynamicBlockObject parsed("BLOCKFLIPPARAMETER");
    parsed.m_flipCaption = "stale";
    parsed.parentHandle = 0x99;

    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDynamicBlock(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_flipCaption.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_BreakData and DRW_BreakPointRef reject truncated common handles",
          "[dwg-read][object-encode][break][safety]") {
    const DRW::Version ver = DRW::AC1021;
    const DRW::Version inlineVer = DRW::AC1018;

    dwgBufferW breakData;
    emitObjectPreamble(breakData, ver, /*oType=*/0, /*handle=*/0x630,
                       /*numReactors=*/1, /*xDictFlag=*/1);
    breakData.putBitLong(0);       // point reference count
    breakData.putBit(0);           // empty R2007+ string-stream presence flag
    breakData.alignToByte();
    breakData.patchRawLong32AtBit(2,
                                  static_cast<std::uint32_t>(breakData.size() * 8u));
    dwgBufferW breakDataHandles;
    emitObjectHandlePrefix(breakDataHandles, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{0x43}, /*xDictHandle=*/0,
                           /*xDictFlag=*/1);
    breakDataHandles.putHandle(nullHandle()); // dimension reference
    breakData.putBytes(breakDataHandles.data().data(),
                       breakDataHandles.data().size());
    auto breakDataBytes = snapshot(breakData);
    dwgBuffer breakDataReader(breakDataBytes.data(), breakDataBytes.size());
    DRW_BreakData parsedBreakData;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsedBreakData, ver, &breakDataReader));
    CHECK(parsedBreakData.parentHandle == 0x42);
    REQUIRE(parsedBreakData.reactorHandles.size() == 1);
    CHECK(parsedBreakData.reactorHandles.front() == 0x43u);
    CHECK(parsedBreakData.m_pointRefHandles.empty());
    CHECK(parsedBreakData.xDictHandle == 0u);

    SECTION("point-reference count cannot consume the detached string stream") {
        dwgBufferW malformedBody;
        emitObjectPreamble(malformedBody, ver, /*oType=*/0,
                           /*handle=*/0x636, /*numReactors=*/0,
                           /*xDictFlag=*/1);
        dwgBufferW detachedStrings;
        detachedStrings.putVariableText(ver, "");
        malformedBody.alignToByte();
        malformedBody.putBytes(detachedStrings.data().data(),
                               detachedStrings.data().size());
        for (int i = 0; i < 7; ++i)
            malformedBody.putBit(0);
        malformedBody.putRawShort16(static_cast<std::uint16_t>(
            detachedStrings.data().size() * 8u + 7u));
        malformedBody.putBit(1); // detached strings are present
        malformedBody.alignToByte();
        malformedBody.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(malformedBody.size() * 8u));
        dwgBufferW malformedHandles;
        emitCommonHandlePrefix(malformedHandles, /*parentHandle=*/0x42,
                               {}, /*xDictFlag=*/1);
        malformedHandles.putHandle(nullHandle()); // dimension reference
        malformedBody.putBytes(malformedHandles.data().data(),
                               malformedHandles.data().size());

        auto malformedBytes = snapshot(malformedBody);
        dwgBuffer malformedReader(malformedBytes.data(),
                                  malformedBytes.size());
        parsedBreakData.parentHandle = 0x99;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
            parsedBreakData, ver, &malformedReader));
        CHECK_FALSE(malformedReader.isGood());
        CHECK(parsedBreakData.parentHandle == 0u);
        CHECK(parsedBreakData.m_pointRefHandles.empty());
    }

    dwgBufferW breakDataTruncated;
    emitObjectPreamble(breakDataTruncated, ver, /*oType=*/0, /*handle=*/0x631,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    breakDataTruncated.putBitLong(0);
    breakDataTruncated.putBit(0);
    breakDataTruncated.alignToByte();
    breakDataTruncated.patchRawLong32AtBit(
        2, static_cast<std::uint32_t>(breakDataTruncated.size() * 8u));
    auto breakDataTruncatedBytes = snapshot(breakDataTruncated);
    dwgBuffer breakDataTruncatedReader(
        breakDataTruncatedBytes.data(), breakDataTruncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsedBreakData, ver, &breakDataTruncatedReader));
    CHECK_FALSE(breakDataTruncatedReader.isGood());
    CHECK(parsedBreakData.parentHandle == 0);
    CHECK(parsedBreakData.reactorHandles.empty());
    CHECK(parsedBreakData.xDictHandle == 0u);

    SECTION("point-reference count cannot outrun the handle stream") {
        dwgBufferW malformedCount;
        emitObjectPreamble(malformedCount, ver, /*oType=*/0,
                           /*handle=*/0x635, /*numReactors=*/0,
                           /*xDictFlag=*/1);
        malformedCount.putBitLong(2);
        malformedCount.putBit(0);
        malformedCount.alignToByte();
        malformedCount.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(malformedCount.size() * 8u));
        auto malformedCountBytes = snapshot(malformedCount);
        dwgBuffer malformedCountReader(
            malformedCountBytes.data(), malformedCountBytes.size());
        parsedBreakData.parentHandle = 0x99;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
            parsedBreakData, ver, &malformedCountReader));
        CHECK_FALSE(malformedCountReader.isGood());
        CHECK(parsedBreakData.parentHandle == 0u);
        CHECK(parsedBreakData.m_pointRefHandles.empty());
    }

    dwgBufferW breakPointRef;
    emitObjectPreamble(breakPointRef, inlineVer, /*oType=*/0, /*handle=*/0x632,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    emitObjectHandlePrefix(breakPointRef, /*parentHandle=*/0x43,
                           /*reactorHandles=*/{0x44}, /*xDictHandle=*/0x45,
                           /*xDictFlag=*/0);
    auto breakPointRefBytes = snapshot(breakPointRef);
    dwgBuffer breakPointRefReader(
        breakPointRefBytes.data(), breakPointRefBytes.size());
    DRW_BreakPointRef parsedBreakPointRef;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsedBreakPointRef, inlineVer, &breakPointRefReader));
    CHECK(static_cast<std::uint32_t>(parsedBreakPointRef.parentHandle) == 0x43u);
    REQUIRE(parsedBreakPointRef.reactorHandles.size() == 1);
    CHECK(parsedBreakPointRef.reactorHandles.front() == 0x44u);
    CHECK(parsedBreakPointRef.xDictHandle == 0x45u);

    SECTION("modern common header cannot end after the declared body") {
        const DRW::Version modern = DRW::AC1024;
        dwgBufferW malformed;
        emitObjectPreamble(malformed, modern, /*oType=*/0,
                           /*handle=*/0x634, /*numReactors=*/0,
                           /*xDictFlag=*/1);
        auto malformedBytes = snapshot(malformed);
        REQUIRE(malformedBytes.size() > 1);
        dwgBuffer reader(malformedBytes.data(), malformedBytes.size());
        DRW_BreakPointRef rejected;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
            rejected, modern, &reader,
            static_cast<std::uint32_t>(malformedBytes.size() * 8u - 1u)));
        CHECK_FALSE(reader.isGood());
        CHECK(rejected.parentHandle == 0u);
    }

    dwgBufferW breakPointRefTruncated;
    emitObjectPreamble(breakPointRefTruncated, inlineVer, /*oType=*/0, /*handle=*/0x633,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    auto breakPointRefTruncatedBytes = snapshot(breakPointRefTruncated);
    dwgBuffer breakPointRefTruncatedReader(
        breakPointRefTruncatedBytes.data(), breakPointRefTruncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsedBreakPointRef, inlineVer, &breakPointRefTruncatedReader));
    CHECK_FALSE(breakPointRefTruncatedReader.isGood());
    CHECK(parsedBreakPointRef.parentHandle == 0);
    CHECK(parsedBreakPointRef.reactorHandles.empty());
    CHECK(parsedBreakPointRef.xDictHandle == 0u);
}

TEST_CASE("modern associative and ACSh shells reject truncated common handles",
          "[dwg-read][object-encode][associative][safety]") {
    const DRW::Version ver = DRW::AC1021;

    auto makeModernShell = [&](std::uint32_t objectHandle,
                               std::uint32_t parentHandle,
                               bool includeCommonHandles) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0, objectHandle,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        body.putBit(0); // empty R2007+ string-stream presence flag
        body.alignToByte();
        body.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(body.size() * 8u));
        if (includeCommonHandles) {
            dwgBufferW handles;
            handles.putHandle(hardPtr(parentHandle));
            handles.putHandle(handleWithCode(0xA, parentHandle + 1));
            handles.putHandle(handleWithCode(0xC, parentHandle + 2));
            body.putBytes(handles.data().data(), handles.data().size());
        }
        return snapshot(body);
    };

    auto associativeBytes = makeModernShell(0x640, 0x42, true);
    dwgBuffer associativeReader(associativeBytes.data(), associativeBytes.size());
    DRW_AssociativeObject parsedAssociative("ACDBASSOCUNKNOWN");
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsedAssociative, ver, &associativeReader));
    CHECK(static_cast<std::uint32_t>(parsedAssociative.parentHandle) == 0x42u);
    REQUIRE(parsedAssociative.reactorHandles.size() == 1);
    CHECK(parsedAssociative.reactorHandles.front() == 0x43u);
    CHECK(parsedAssociative.xDictHandle == 0x44u);

    parsedAssociative.m_classVersion = 7;
    parsedAssociative.m_prefixStatuses.emplace_back();
    parsedAssociative.reactorHandles.push_back(0x99u);
    parsedAssociative.xDictHandle = 0x9Au;
    auto associativeTruncatedBytes = makeModernShell(0x641, 0, false);
    dwgBuffer associativeTruncatedReader(
        associativeTruncatedBytes.data(), associativeTruncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsedAssociative, ver, &associativeTruncatedReader));
    CHECK_FALSE(associativeTruncatedReader.isGood());
    CHECK(parsedAssociative.m_classVersion == 0);
    CHECK(parsedAssociative.m_prefixStatuses.empty());
    CHECK(parsedAssociative.parentHandle == 0);
    CHECK(parsedAssociative.reactorHandles.empty());
    CHECK(parsedAssociative.xDictHandle == 0);

    auto acshBytes = makeModernShell(0x642, 0x43, true);
    dwgBuffer acshReader(acshBytes.data(), acshBytes.size());
    DRW_AcShHistoryObject parsedAcSh("ACSH_UNKNOWN_CLASS");
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsedAcSh, ver, &acshReader));
    CHECK(static_cast<std::uint32_t>(parsedAcSh.parentHandle) == 0x43u);
    REQUIRE(parsedAcSh.reactorHandles.size() == 1);
    CHECK(parsedAcSh.reactorHandles.front() == 0x44u);
    CHECK(parsedAcSh.xDictHandle == 0x45u);

    parsedAcSh.m_major = 7;
    parsedAcSh.m_prefixStatuses.emplace_back();
    parsedAcSh.reactorHandles.push_back(0x9Bu);
    parsedAcSh.xDictHandle = 0x9Cu;
    auto acshTruncatedBytes = makeModernShell(0x643, 0, false);
    dwgBuffer acshTruncatedReader(
        acshTruncatedBytes.data(), acshTruncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsedAcSh, ver, &acshTruncatedReader));
    CHECK_FALSE(acshTruncatedReader.isGood());
    CHECK(parsedAcSh.m_major == 0);
    CHECK(parsedAcSh.m_prefixStatuses.empty());
    CHECK(parsedAcSh.parentHandle == 0);
    CHECK(parsedAcSh.reactorHandles.empty());
    CHECK(parsedAcSh.xDictHandle == 0);
}

TEST_CASE("action-parameter shells honor the declared body boundary",
          "[dwg-read][object-encode][associative][safety]") {
    const DRW::Version version = DRW::AC1021;

    auto makeVertex = [&](bool complete) {
        dwgBufferW body;
        emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x660,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitShort(1);             // is_r2013
        body.putBitLong(2);              // asdap_class_version
        body.putBitLong(3);              // vertex class_version
        if (complete)
            body.put3BitDouble(DRW_Coord{1.0, 2.0, 3.0});
        body.putBit(0);                  // empty R2007+ string stream
        body.alignToByte();
        body.patchRawLong32AtBit(
            objectSizeBitOffset(body), static_cast<std::uint32_t>(
                body.bitCount()));
        dwgBufferW handles;
        emitObjectHandlePrefix(handles, /*parentHandle=*/0x42,
                               /*reactorHandles=*/{}, /*xDictHandle=*/0,
                               /*xDictFlag=*/1);
        handles.putHandle(hardPtr(0x77)); // single-dependency handle
        handles.alignToByte();
        body.putBytes(handles.data().data(), handles.data().size());
        return snapshot(body);
    };

    const auto vertexBytes = makeVertex(true);
    dwgBuffer vertexReader(const_cast<std::uint8_t *>(vertexBytes.data()),
                           vertexBytes.size());
    DRW_AssociativeObject vertex("ACDBASSOCVERTEXACTIONPARAM");
    REQUIRE(DrwObjectEncodeTestAccess::parseAssociative(
        vertex, version, &vertexReader));
    CHECK(vertex.m_actionParamPrefixParsed);
    CHECK(vertex.m_singleDependencyActionParamParsed);
    CHECK(vertex.m_dependencyHandle == 0x77u);
    CHECK(vertex.m_classVersion == 3u);
    CHECK(vertex.m_point.x == 1.0);
    CHECK(vertex.m_point.y == 2.0);
    CHECK(vertex.m_point.z == 3.0);

    const auto truncatedVertexBytes = makeVertex(false);
    dwgBuffer truncatedVertexReader(
        const_cast<std::uint8_t *>(truncatedVertexBytes.data()),
        truncatedVertexBytes.size());
    DRW_AssociativeObject truncatedVertex("ACDBASSOCVERTEXACTIONPARAM");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAssociative(
        truncatedVertex, version, &truncatedVertexReader));
    CHECK_FALSE(truncatedVertexReader.isGood());
    CHECK(truncatedVertex.m_dependencyHandle == 0u);
    CHECK(truncatedVertex.m_point.x == 0.0);

    auto makeOsnap = [&](bool complete) {
        dwgBufferW body;
        emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x661,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitShort(1);             // is_r2013
        body.putBitShort(2);             // compound class_version
        body.putBitShort(0);             // bs1
        body.putBitLong(0);              // num_params
        body.putBit(0);                  // has_child_param
        if (complete) {
            body.putBitShort(7);         // status
            body.putRawChar8(4);          // osnap_mode
            body.putBitDouble(0.75);      // param
        }
        body.putBit(0);                  // empty R2007+ string stream
        body.alignToByte();
        body.patchRawLong32AtBit(
            objectSizeBitOffset(body), static_cast<std::uint32_t>(
                body.bitCount()));
        dwgBufferW handles;
        emitObjectHandlePrefix(handles, /*parentHandle=*/0x43,
                               /*reactorHandles=*/{}, /*xDictHandle=*/0,
                               /*xDictFlag=*/1);
        handles.alignToByte();
        body.putBytes(handles.data().data(), handles.data().size());
        return snapshot(body);
    };

    const auto osnapBytes = makeOsnap(true);
    dwgBuffer osnapReader(const_cast<std::uint8_t *>(osnapBytes.data()),
                          osnapBytes.size());
    DRW_AssociativeObject osnap("ACDBASSOCOSNAPPOINTREFACTIONPARAM");
    REQUIRE(DrwObjectEncodeTestAccess::parseAssociative(
        osnap, version, &osnapReader));
    CHECK(osnap.m_actionParamPrefixParsed);
    CHECK(osnap.m_compoundActionParamParsed);
    CHECK(osnap.m_status == 7);
    CHECK(osnap.m_osnapMode == 4u);
    CHECK(osnap.m_parameter == 0.75);

    const auto truncatedOsnapBytes = makeOsnap(false);
    dwgBuffer truncatedOsnapReader(
        const_cast<std::uint8_t *>(truncatedOsnapBytes.data()),
        truncatedOsnapBytes.size());
    DRW_AssociativeObject truncatedOsnap(
        "ACDBASSOCOSNAPPOINTREFACTIONPARAM");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAssociative(
        truncatedOsnap, version, &truncatedOsnapReader));
    CHECK_FALSE(truncatedOsnapReader.isGood());
    CHECK(truncatedOsnap.m_parameter == 0.0);
}

TEST_CASE("associative dependency prefixes honor the declared body boundary",
          "[dwg-read][object-encode][associative][safety]") {
    const DRW::Version version = DRW::AC1021;

    auto makeDependency = [&](bool complete) {
        dwgBufferW body;
        emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x662,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitShort(2); // class_version
        body.putBitLong(0x12); // status
        body.putBit(1); // is_read_dependency
        body.putBit(0); // is_write_dependency
        body.putBit(1); // is_attached_to_object
        body.putBit(0); // is_delegating_to_owning_action
        body.putBitLong(-1); // order
        body.putBit(1); // has_name
        if (complete)
            body.putBitLong(7); // dependency body id

        dwgBufferW strings;
        strings.putVariableText(version, "dependency");
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        body.patchRawLong32AtBit(
            objectSizeBitOffset(body), static_cast<std::uint32_t>(
                body.size() * 8u));

        dwgBufferW handles;
        emitObjectHandlePrefix(handles, /*parentHandle=*/0x42,
                               /*reactorHandles=*/{}, /*xDictHandle=*/0,
                               /*xDictFlag=*/1);
        handles.putHandle(handleWithCode(3, 0x77)); // dependency-on
        handles.putHandle(handleWithCode(4, 0x78)); // read dependency
        handles.putHandle(handleWithCode(3, 0x79)); // node
        handles.putHandle(handleWithCode(4, 0x7A)); // dependency body
        handles.alignToByte();
        auto bytes = snapshot(body);
        bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
        return bytes;
    };

    const auto validBytes = makeDependency(true);
    dwgBuffer validReader(const_cast<std::uint8_t *>(validBytes.data()),
                          validBytes.size());
    DRW_AssociativeObject valid("ACDBASSOCDEPENDENCY");
    REQUIRE(DrwObjectEncodeTestAccess::parseAssociative(
        valid, version, &validReader));
    CHECK(valid.m_classVersion == 2u);
    CHECK(valid.m_status == 0x12);
    CHECK(valid.m_dependencyHandle == 0x77u);
    CHECK(valid.m_readDependencyHandle == 0x78u);
    CHECK(valid.m_writeDependencyHandle == 0x79u);
    CHECK(valid.parentHandle == 0x42u);

    const auto truncatedBytes = makeDependency(false);
    dwgBuffer truncatedReader(
        const_cast<std::uint8_t *>(truncatedBytes.data()),
        truncatedBytes.size());
    DRW_AssociativeObject truncated("ACDBASSOCDEPENDENCY");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAssociative(
        truncated, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncated.m_classVersion == 0u);
    CHECK(truncated.m_status == 0);
    CHECK(truncated.m_dependencyHandle == 0u);
    CHECK(truncated.parentHandle == 0u);
}

TEST_CASE("geometry dependency extensions honor the declared body boundary",
          "[dwg-read][object-encode][associative][safety]") {
    const DRW::Version version = DRW::AC1021;

    auto makeGeometryDependency = [&](bool complete) {
        dwgBufferW body;
        emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x663,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitShort(2); // dependency class_version
        body.putBitLong(0x13); // dependency status
        body.putBit(1); // is_read_dependency
        body.putBit(1); // is_write_dependency
        body.putBit(0); // is_attached_to_object
        body.putBit(0); // is_delegating_to_owning_action
        body.putBitLong(0); // order
        body.putBit(0); // no dependency name
        body.putBitLong(7); // dependency body id
        if (complete) {
            body.putBitShort(0); // geometry dependency class_version
            body.putBit(1); // enabled
            body.putBit(1); // dependent_on_compound_object
        }

        dwgBufferW strings;
        strings.putVariableText(version, "Line");
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        body.patchRawLong32AtBit(
            objectSizeBitOffset(body), static_cast<std::uint32_t>(
                body.size() * 8u));

        dwgBufferW handles;
        emitObjectHandlePrefix(handles, /*parentHandle=*/0x43,
                               /*reactorHandles=*/{}, /*xDictHandle=*/0,
                               /*xDictFlag=*/1);
        handles.putHandle(handleWithCode(3, 0x80)); // dependency-on
        handles.putHandle(handleWithCode(4, 0x81)); // read dependency
        handles.putHandle(handleWithCode(3, 0x82)); // node
        handles.putHandle(handleWithCode(4, 0x83)); // dependency body
        handles.alignToByte();
        auto bytes = snapshot(body);
        bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
        return bytes;
    };

    const auto validBytes = makeGeometryDependency(true);
    dwgBuffer validReader(const_cast<std::uint8_t *>(validBytes.data()),
                          validBytes.size());
    DRW_AssociativeObject valid("ACDBASSOCGEOMDEPENDENCY");
    REQUIRE(DrwObjectEncodeTestAccess::parseAssociative(
        valid, version, &validReader));
    CHECK(valid.m_classVersion == 2u);
    CHECK(valid.m_status == 0x13);
    CHECK(valid.m_dependencyHandle == 0x80u);
    CHECK(valid.m_readDependencyHandle == 0x81u);
    CHECK(valid.parentHandle == 0x43u);

    const auto truncatedBytes = makeGeometryDependency(false);
    dwgBuffer truncatedReader(
        const_cast<std::uint8_t *>(truncatedBytes.data()),
        truncatedBytes.size());
    DRW_AssociativeObject truncated("ACDBASSOCGEOMDEPENDENCY");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAssociative(
        truncated, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncated.m_classVersion == 0u);
    CHECK(truncated.m_dependencyHandle == 0u);
    CHECK(truncated.parentHandle == 0u);
}

TEST_CASE("associative network tails are transactional",
          "[dwg-read][object-encode][associative][safety]") {
    const DRW::Version version = DRW::AC1021;

    auto makeNetwork = [&](bool complete) {
        dwgBufferW body;
        emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x664,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitShort(1); // AcDbAssocAction class_version
        body.putBitLong(0);  // geometry_status
        body.putBitLong(0);  // action_index
        body.putBitLong(0);  // max_assoc_dep_index
        body.putBitLong(0);  // num_deps
        body.putBitShort(0); // network_version
        body.putBitLong(7);  // network_action_index
        body.putBitLong(1);  // num_actions
        body.putBit(1);      // action is owned
        body.putBitLong(1);  // num_owned_actions
        dwgBufferW strings;
        strings.putVariableText(version, "");
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        body.patchRawLong32AtBit(
            objectSizeBitOffset(body),
            static_cast<std::uint32_t>(body.bitCount()));

        dwgBufferW handles;
        emitCommonHandlePrefix(handles, /*parentHandle=*/0x42,
                               /*reactorHandles=*/{}, /*xDictFlag=*/1);
        handles.putHandle(hardPtr(0x80)); // owning network
        handles.putHandle(hardPtr(0x81)); // action body
        if (complete)
            handles.putHandle(hardPtr(0x82)); // network action
        if (complete)
            handles.putHandle(hardPtr(0x83)); // owned action
        handles.alignToByte();
        auto bytes = snapshot(body);
        bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
        return bytes;
    };

    const auto validBytes = makeNetwork(true);
    dwgBuffer validReader(const_cast<std::uint8_t *>(validBytes.data()),
                          validBytes.size());
    DRW_AssociativeObject valid("ACDBASSOCNETWORK");
    REQUIRE(DrwObjectEncodeTestAccess::parseAssociative(
        valid, version, &validReader));
    CHECK(valid.m_actionIndex == 7);
    REQUIRE(valid.m_actions.size() == 1u);
    CHECK(valid.m_actions.front().m_isOwned);
    CHECK(valid.m_actions.front().m_handle == 0x82u);
    CHECK(valid.m_ownedActions == std::vector<std::uint32_t>{0x83u});
    CHECK(valid.parentHandle == 0x42u);

    const auto truncatedBytes = makeNetwork(false);
    dwgBuffer truncatedReader(
        const_cast<std::uint8_t *>(truncatedBytes.data()),
        truncatedBytes.size());
    DRW_AssociativeObject truncated("ACDBASSOCNETWORK");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAssociative(
        truncated, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncated.m_actions.empty());
    CHECK(truncated.m_ownedActions.empty());
    CHECK(truncated.m_actionIndex == 0);
    CHECK(truncated.parentHandle == 0u);
}

TEST_CASE("associative detached text cannot consume its string footer",
          "[dwg-read][object-encode][associative][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x666,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    body.putBitShort(2); // dependency class version
    body.putBitLong(0x12); // dependency status
    body.putBit(1); // is_read_dependency
    body.putBit(0); // is_write_dependency
    body.putBit(1); // is_attached_to_object
    body.putBit(0); // is_delegating_to_owning_action
    body.putBitLong(0); // order
    body.putBit(1); // dependency name is present in the string stream
    body.putBitLong(7); // dependency body id
    body.putBitShort(2); // geometry dependency class version
    body.putBit(1); // enabled
    body.putBit(0); // dependent_on_compound_object

    // Declare two UTF-16 code units but provide only one. The footer still
    // describes the actual detached stream, so a reader using objSize as the
    // text limit would continue into the footer and accept the malformed name.
    dwgBufferW strings;
    strings.putBitShort(2);
    strings.putRawChar8('x');
    strings.putRawChar8(0);
    body.alignToByte();
    body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u));
    body.putBit(1); // string stream is present
    body.alignToByte();
    body.patchRawLong32AtBit(
        objectSizeBitOffset(body), static_cast<std::uint32_t>(body.bitCount()));

    dwgBufferW handles;
    emitCommonHandlePrefix(handles, /*parentHandle=*/0x42, {}, 1);
    handles.putHandle(hardPtr(0x70)); // dependency-on
    handles.putHandle(hardPtr(0x71)); // read dependency
    handles.putHandle(hardPtr(0x72)); // node
    handles.putHandle(hardPtr(0x73)); // dependency body
    handles.alignToByte();
    body.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_AssociativeObject parsed("ACDBASSOCGEOMDEPENDENCY");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAssociative(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_classVersion == 0u);
    CHECK(parsed.m_status == 0);
    CHECK(parsed.m_dependencyHandle == 0u);
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("aligned dimension action bodies reject truncated handle tails",
          "[dwg-read][object-encode][associative][safety]") {
    const DRW::Version version = DRW::AC1021;

    auto makeRecord = [&](bool complete) {
        dwgBufferW body;
        emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x665,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitShort(2); // action-body class version
        body.putBitLong(0);  // dependency body version
        body.putBitLong(0);  // reserved action-body value

        dwgBufferW strings;
        strings.putVariableText(version, "");
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        body.patchRawLong32AtBit(
            objectSizeBitOffset(body),
            static_cast<std::uint32_t>(body.bitCount()));

        dwgBufferW handles;
        emitCommonHandlePrefix(handles, /*parentHandle=*/0x42,
                               /*reactorHandles=*/{}, /*xDictFlag=*/1);
        handles.putHandle(hardPtr(0x80)); // dependency
        handles.putHandle(hardPtr(0x81)); // r-node
        if (complete)
            handles.putHandle(hardPtr(0x82)); // d-node
        handles.alignToByte();
        body.putBytes(handles.data().data(), handles.data().size());
        return snapshot(body);
    };

    const auto validBytes = makeRecord(true);
    dwgBuffer validReader(const_cast<std::uint8_t *>(validBytes.data()),
                          validBytes.size());
    DRW_AssociativeObject valid("ACDBASSOCALIGNEDDIMACTIONBODY");
    REQUIRE(DrwObjectEncodeTestAccess::parseAssociative(
        valid, version, &validReader));
    CHECK(valid.m_classVersion == 2u);
    CHECK(valid.m_dependencyHandle == 0x80u);
    CHECK(valid.m_rNodeHandle == 0x81u);
    CHECK(valid.m_dNodeHandle == 0x82u);
    CHECK(valid.parentHandle == 0x42u);

    const auto truncatedBytes = makeRecord(false);
    dwgBuffer truncatedReader(
        const_cast<std::uint8_t *>(truncatedBytes.data()),
        truncatedBytes.size());
    DRW_AssociativeObject truncated("ACDBASSOCALIGNEDDIMACTIONBODY");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAssociative(
        truncated, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncated.m_classVersion == 0u);
    CHECK(truncated.m_dependencyHandle == 0u);
    CHECK(truncated.m_rNodeHandle == 0u);
    CHECK(truncated.m_dNodeHandle == 0u);
    CHECK(truncated.parentHandle == 0u);
}

TEST_CASE("fixed PERSUBENTMGR decodes scalar vectors before common handles",
          "[dwg-read][object-encode][associative][persistence]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x650,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    body.putBitLong(2);  // class_version
    body.putBitLong(0);  // unknown_0
    body.putBitLong(2);  // unknown_2
    body.putBitLong(3);  // numassocsteps
    body.putBitLong(1);  // numassocsubents
    body.putBitLong(2);  // num_steps
    body.putBitLong(11);
    body.putBitLong(22);
    body.putBitLong(1);  // num_subents
    body.putBitLong(99);
    body.alignToByte();
    for (int i = 0; i < 7; ++i)
        body.putBit(0);  // empty R2007+ string stream
    body.putRawShort16(0);
    body.putBit(0);
    body.alignToByte();
    dwgBufferW handles;
    emitObjectHandlePrefix(handles, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{0x43}, /*xDictHandle=*/0x44,
                           /*xDictFlag=*/0);
    handles.alignToByte();
    body.patchRawLong32AtBit(
        objectSizeBitOffset(body), static_cast<std::uint32_t>(body.size() * 8));
    body.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_AssociativeObject parsed("ACDBPERSSUBENTMANAGER");
    REQUIRE(DrwObjectEncodeTestAccess::parseAssociative(
        parsed, version, &reader));
    CHECK(parsed.m_classVersion == 2u);
    CHECK(parsed.m_persistentUnknown0 == 0u);
    CHECK(parsed.m_persistentUnknown2 == 2u);
    CHECK(parsed.m_persistentAssocStepCount == 3u);
    CHECK(parsed.m_persistentAssocSubentityCount == 1u);
    CHECK(parsed.m_persistentSteps == std::vector<std::uint32_t>{11u, 22u});
    CHECK(parsed.m_persistentSubentityIds
          == std::vector<std::uint32_t>{99u});
    CHECK(parsed.m_persistentSubentityHandles.empty());
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x42u);
    REQUIRE(parsed.reactorHandles.size() == 1);
    CHECK(parsed.reactorHandles.front() == 0x43u);
    CHECK(parsed.xDictHandle == 0x44u);
}

TEST_CASE("fixed PERSUBENTMGR rejects a truncated scalar vector",
          "[dwg-read][object-encode][associative][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x651,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    body.putBitLong(2);  // class_version
    body.putBitLong(0);  // unknown_0
    body.putBitLong(2);  // unknown_2
    body.putBitLong(3);  // numassocsteps
    body.putBitLong(1);  // numassocsubents
    body.putBitLong(1);  // num_steps
    // Omit the step value and terminate the data section immediately.
    body.alignToByte();
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(0);
    body.putBit(0);
    body.alignToByte();
    body.patchRawLong32AtBit(
        objectSizeBitOffset(body), static_cast<std::uint32_t>(body.size() * 8));

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_AssociativeObject parsed("ACDBPERSSUBENTMANAGER");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAssociative(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_classVersion == 0u);
    CHECK(parsed.m_persistentSteps.empty());
}

TEST_CASE("ACSH history body honors the modern object boundary",
          "[dwg-read][object-encode][acsh][safety]") {
    const DRW::Version ver = DRW::AC1021;

    auto makeHistory = [&](bool complete) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x650,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitLong(2); // major
        body.putBitLong(3); // minor
        if (complete) {
            body.putBitLong(4); // h_nodeid
            body.putBit(1);     // show_history
            body.putBit(0);     // record_history
        }
        body.alignToByte();
        body.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(body.size() * 8u));

        dwgBufferW handles;
        emitCommonHandlePrefix(handles, /*parentHandle=*/0x44,
                               {}, /*xDictFlag=*/1);
        handles.putHandle(hardPtr(0x55)); // AcDbShHistory owner
        body.putBytes(handles.data().data(), handles.data().size());
        return snapshot(body);
    };

    auto historyBytes = makeHistory(true);
    dwgBuffer historyReader(historyBytes.data(), historyBytes.size());
    DRW_AcShHistoryObject parsed("ACSH_HISTORY_CLASS");
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &historyReader));
    CHECK(parsed.m_major == 2);
    CHECK(parsed.m_minor == 3);
    CHECK(parsed.m_historyNodeId == 4);
    CHECK(parsed.m_showHistory);
    CHECK_FALSE(parsed.m_recordHistory);
    CHECK(parsed.m_ownerHandle == 0x55u);
    CHECK(parsed.parentHandle == 0x44);

    parsed.m_major = 99;
    parsed.m_ownerHandle = 0x66;
    auto truncatedBytes = makeHistory(false);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_major == 0);
    CHECK(parsed.m_ownerHandle == 0);
    CHECK(parsed.parentHandle == 0);
}

TEST_CASE("ACSH history body cannot consume the detached string stream",
          "[dwg-read][object-encode][acsh][safety]") {
    const DRW::Version ver = DRW::AC1021;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x653,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(2); // major
    encoded.putBitLong(3); // minor

    dwgBufferW strings;
    strings.putVariableText(ver, "detached");
    encoded.alignToByte();
    encoded.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        encoded.putBit(0);
    encoded.putRawShort16(static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u));
    encoded.putBit(1); // detached strings are present
    encoded.alignToByte();
    encoded.patchRawLong32AtBit(
        2, static_cast<std::uint32_t>(encoded.size() * 8u));

    dwgBufferW handles;
    emitCommonHandlePrefix(handles, /*parentHandle=*/0x44,
                           {}, /*xDictFlag=*/1);
    handles.putHandle(hardPtr(0x55)); // AcDbShHistory owner
    encoded.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_AcShHistoryObject parsed("ACSH_HISTORY_CLASS");
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_major == 0u);
    CHECK(parsed.m_minor == 0u);
    CHECK(parsed.m_historyNodeId == 0u);
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.m_ownerHandle == 0u);
}

TEST_CASE("DRW_UnderlayDefinition captures common handle references",
          "[dwg-read][object-encode][underlaydefinition]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0xA30,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "reference.pdf");
    encoded.putVariableText(ver, "Sheet 1");
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(hardPtr(0xA1));
    encoded.putHandle(hardPtr(0xB1));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_UnderlayDefinition parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.filename == "reference.pdf");
    CHECK(parsed.sheetName == "Sheet 1");
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x1Fu);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0xA1u);
    CHECK(parsed.xDictHandle == 0xB1u);

    parsed.kind = DRW_UnderlayDefinition::DGN;
    parsed.filename = "stale.dgn";
    parsed.sheetName = "stale sheet";
    parsed.parentHandle = 0x77;
    auto truncated = snapshot(encoded);
    truncated.resize(4);
    dwgBuffer truncatedReader(truncated.data(), truncated.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK(parsed.kind == DRW_UnderlayDefinition::DGN);
    CHECK(parsed.filename.empty());
    CHECK(parsed.sheetName.empty());
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
    CHECK_FALSE(truncatedReader.isGood());
}

TEST_CASE("DRW_UnderlayDefinition reads AC1024 detached text fields",
          "[dwg-read][dwg-write][object-encode][underlaydefinition]") {
    const DRW::Version ver = DRW::AC1024;
    DRW_UnderlayDefinition source;
    source.kind = DRW_UnderlayDefinition::DGN;
    source.filename = "reference.dgn";
    source.sheetName = "Sheet 2";
    source.handle = 0xA33u;
    source.parentHandle = 0x42u;
    source.reactorHandles = {0x44u};
    source.xDictHandle = 0x43u;
    source.setDwgCommonObjectState(1, 0, false);

    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, ver, DRW_UnderlayDefinition::kDwgClassNumDgn,
                       source.handle, source.reactorCount(),
                       source.extensionDictionaryFlag());
    REQUIRE(DrwObjectEncodeTestAccess::encodeUnderlayDefinitionModern(
        source, ver, &data, &strings, &handles));

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    REQUIRE(stringBytes > 0u);
    data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    REQUIRE(stringBitSize <= std::numeric_limits<std::uint16_t>::max());
    data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    data.putBit(1); // detached strings are present
    data.alignToByte();

    handles.alignToByte();
    const std::uint32_t handleBits =
        static_cast<std::uint32_t>(handles.data().size() * 8u);
    auto bytes = snapshot(data);
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());

    DRW_TextCodec decoder;
    decoder.setVersion(ver, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_UnderlayDefinition parsed;
    parsed.kind = source.kind;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader,
                                             handleBits));
    CHECK(parsed.filename == source.filename);
    CHECK(parsed.sheetName == source.sheetName);
    CHECK(parsed.parentHandle == source.parentHandle);
    CHECK(parsed.reactorHandles == source.reactorHandles);
    CHECK(parsed.xDictHandle == source.xDictHandle);
}

TEST_CASE("DWG OBJECTS readers reject typed bodies past the declared size",
          "[dwg-read][object-encode][safety]") {
    const DRW::Version version = DRW::AC1015;
    const auto reject = [version](dwgBufferW& encoded,
                                  DRW_TableEntry& object) {
        REQUIRE(encoded.bitCount() > 1);
        encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded),
                                    static_cast<std::uint32_t>(
                                        encoded.bitCount() - 1));
        auto bytes = snapshot(encoded);
        dwgBuffer reader(bytes.data(), bytes.size());
        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(object, version,
                                                      &reader));
        CHECK_FALSE(reader.isGood());
    };

    SECTION("underlay definition") {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, version, /*oType=*/0,
                           /*handle=*/0xA31);
        encoded.putVariableText(version, "reference.pdf");
        encoded.putVariableText(version, "Sheet 1");
        DRW_UnderlayDefinition object;
        reject(encoded, object);
    }

    SECTION("group") {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, version, /*oType=*/0,
                           /*handle=*/0xA32);
        encoded.putVariableText(version, "group");
        encoded.putBitShort(0);
        encoded.putBitShort(1);
        encoded.putBitLong(0);
        DRW_Group object;
        reject(encoded, object);
    }

    SECTION("layer filter") {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, version, /*oType=*/0,
                           /*handle=*/0xA33);
        encoded.putBitLong(0);
        DRW_LayerFilter object;
        reject(encoded, object);
    }

    SECTION("image definition reactor") {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, version, /*oType=*/0,
                           /*handle=*/0xA34);
        encoded.putBitLong(0);
        DRW_ImageDefinitionReactor object;
        reject(encoded, object);
    }

    SECTION("AcDbPlaceholder") {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, version, /*oType=*/0,
                           /*handle=*/0xA35);
        DRW_AcDbPlaceholder object;
        reject(encoded, object);
    }
}

// P4-13 (gap object-imagedef-uv-size-dropped): DRW_ImageDef::parseDwg read
// the image pixel size (DXF 10/20) via get2RawDouble and discarded it; it now
// assigns u/v (distinct from the up/vp pixel-scale fields).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_ImageDef::parseDwg captures u/v pixel size",
          "[dwg-read][object-encode][imagedef]") {
    DRW::Version ver = DRW::AC1015;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x500);
    w.putBitLong(0);                 // imgVersion (class version)
    w.putRawDouble(1024.0);          // size.x → u
    w.putRawDouble(768.0);           // size.y → v
    w.putVariableText(ver, "img");   // name
    w.putBit(1);                     // loaded
    w.putRawChar8(2);                // resolution
    w.putRawDouble(0.25);            // up (pixel scale U)
    w.putRawDouble(0.5);             // vp (pixel scale V)

    // Handle stream: owner, reactors, and xdictionary.
    emitCommonHandlePrefix(w, /*parentHandle=*/0x10, /*reactors=*/{}, /*xDictFlag=*/0);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_ImageDef dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.u == 1024.0);        // was discarded before the fix
    REQUIRE(dst.v == 768.0);
    REQUIRE(dst.up == 0.25);         // pixel scale unaffected
    REQUIRE(dst.vp == 0.5);
    REQUIRE(dst.name == "img");
}

TEST_CASE("DRW_ImageDef class version is bounded and round-trips",
          "[dwg-read][dwg-write][object-encode][imagedef]") {
    DRW::Version ver = DRW::AC1015;
    DRW_ImageDef src;
    src.handle = 0x501;
    src.parentHandle = 0x10;
    src.imgVersion = 4;
    src.u = 1024.0;
    src.v = 768.0;
    src.name = "img.png";
    src.loaded = 1;
    src.resolution = 5;
    src.up = 0.25;
    src.vp = 0.5;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 0);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, 0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeImageDef(src, ver, &encoded));
    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ImageDef parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.imgVersion == 4);
    CHECK(parsed.u == 1024.0);
    CHECK(parsed.v == 768.0);
    CHECK(parsed.resolution == 5);

    DRW_ImageDef invalid = src;
    invalid.imgVersion = DRW_ImageDef::kMaxClassVersion + 1;
    dwgBufferW invalidWriter;
    emitObjectPreamble(invalidWriter, ver, 0, invalid.handle,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeImageDef(invalid, ver,
                                                          &invalidWriter));

    invalid = src;
    invalid.u = std::numeric_limits<double>::quiet_NaN();
    invalidWriter.reset();
    invalidWriter.putRawChar8(0x5A);
    const auto invalidBytes = snapshot(invalidWriter);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeImageDef(invalid, ver,
                                                          &invalidWriter));
    CHECK(invalidWriter.data() == invalidBytes);

    dwgBufferW malformed;
    emitObjectPreamble(malformed, ver, 0, 0x502,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    malformed.putBitLong(DRW_ImageDef::kMaxClassVersion + 1);
    auto malformedBytes = snapshot(malformed);
    dwgBuffer malformedReader(malformedBytes.data(), malformedBytes.size());
    DRW_ImageDef rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(rejected, ver,
                                                 &malformedReader));
}

TEST_CASE("DRW_ImageDef AC1021 framing preserves detached path",
          "[dwg-read][object-encode][imagedef][ac1021]") {
    DRW_ImageDef source;
    source.handle = 0x510;
    source.parentHandle = 0x511;
    source.imgVersion = 4;
    source.u = 1920.0;
    source.v = 1080.0;
    source.name = "image.png";
    source.loaded = 1;
    source.resolution = 5;
    source.up = 0.25;
    source.vp = 0.5;
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 0);

    auto bytes = emitAc1021ImageDefObject(source);
    REQUIRE_FALSE(bytes.empty());
    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1021, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_ImageDef parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1021, &reader));
    CHECK(parsed.handle == source.handle);
    CHECK(parsed.parentHandle == source.parentHandle);
    CHECK(parsed.imgVersion == source.imgVersion);
    CHECK(parsed.u == Approx(source.u));
    CHECK(parsed.v == Approx(source.v));
    CHECK(parsed.name == source.name);
    CHECK(parsed.loaded == source.loaded);
    CHECK(parsed.resolution == source.resolution);
    CHECK(parsed.up == Approx(source.up));
    CHECK(parsed.vp == Approx(source.vp));

    auto truncated = bytes;
    REQUIRE_FALSE(truncated.empty());
    truncated.pop_back(); // truncate the required xdictionary handle
    dwgBuffer truncatedReader(truncated.data(), truncated.size(), &decoder);
    parsed.name = "stale-name";
    parsed.parentHandle = 0x999;
    parsed.u = 9.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1021, &truncatedReader));
    CHECK(parsed.name.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.u == 0.0);
}

TEST_CASE("DRW_ImageDef rejects a body ending before image size",
          "[dwg-read][object-encode][imagedef][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0,
                       /*handle=*/0x520);
    encoded.putBitLong(0);         // image class version
    const std::uint32_t classVersionEndBit = encoded.bitCount();
    encoded.putRawDouble(1024.0);  // image width
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded),
                                classVersionEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ImageDef rejected;
    rejected.name = "stale-name";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
}

TEST_CASE("DRW_ImageDef rejects non-finite DWG image sizes transactionally",
          "[dwg-read][object-encode][imagedef][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0,
                       /*handle=*/0x521);
    encoded.putBitLong(0); // image class version
    encoded.putRawDouble(nan); // image width

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ImageDef rejected;
    rejected.name = "stale-image";
    rejected.u = 9.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
    CHECK(rejected.u == Approx(0.0));
    CHECK(rejected.parentHandle == 0u);
}

TEST_CASE("DRW_ImageDefinitionReactor class version is bounded",
          "[dwg-read][dwg-write][object-encode][imagedef]") {
    DRW::Version ver = DRW::AC1015;
    DRW_ImageDefinitionReactor src;
    src.handle = 0x503;
    src.parentHandle = 0x70;
    src.reactorHandles = {0x71u};
    src.xDictHandle = 0x72u;
    src.m_classVersion = 2;
    src.setDwgCommonObjectState(1, 0, false);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_ImageDefinitionReactor::kDwgClassNum,
                       src.handle, /*numReactors=*/1, /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeImageDefinitionReactor(src, ver,
                                                                     &encoded));
    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ImageDefinitionReactor parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_classVersion == 2);
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x70u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0x71u);
    CHECK(parsed.xDictHandle == 0x72u);

    auto truncated = bytes;
    truncated.pop_back();
    parsed.m_classVersion = 9;
    parsed.parentHandle = 0x99;
    parsed.reactorHandles.push_back(0x98);
    parsed.xDictHandle = 0x97;
    dwgBuffer truncatedReader(truncated.data(), truncated.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver,
                                                 &truncatedReader));
    CHECK(parsed.m_classVersion == 0);
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);

    dwgBufferW invalidEncoded;
    emitObjectPreamble(invalidEncoded, ver,
                       DRW_ImageDefinitionReactor::kDwgClassNum,
                       /*handle=*/0x505, /*numReactors=*/1, /*xDictFlag=*/0);
    invalidEncoded.putBitLong(
        DRW_ImageDefinitionReactor::kMaxClassVersion + 1);
    emitCommonHandlePrefix(invalidEncoded, /*parentHandle=*/0x70,
                           /*reactorHandles=*/{0x71}, /*xDictFlag=*/0);
    auto invalidBytes = snapshot(invalidEncoded);
    dwgBuffer invalidReader(invalidBytes.data(), invalidBytes.size());
    DRW_ImageDefinitionReactor invalidParsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        invalidParsed, ver, &invalidReader));
    CHECK_FALSE(invalidReader.isGood());
    CHECK(invalidParsed.m_classVersion == 0);

    DRW_ImageDefinitionReactor invalid = src;
    invalid.m_classVersion = DRW_ImageDefinitionReactor::kMaxClassVersion + 1;
    dwgBufferW invalidWriter;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeImageDefinitionReactor(
        invalid, ver, &invalidWriter));
}

TEST_CASE("DRW_ImageDefinitionReactor rejects a truncated common handle stream",
          "[dwg-read][object-encode][imagedef]") {
    DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_ImageDefinitionReactor::kDwgClassNum,
                       /*handle=*/0x504, /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(2);
    encoded.putHandle(hardPtr(0x70));
    encoded.putHandle(hardPtr(0x71));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ImageDefinitionReactor parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
}

TEST_CASE("DRW_LayerFilter uses object common handle semantics",
          "[dwg-read][object-encode][layerfilter]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x510,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(2);
    encoded.putVariableText(ver, "LayerA");
    encoded.putVariableText(ver, "LayerB");
    encoded.putHandle(hardPtr(0x20));
    encoded.putHandle(handleWithCode(0x0A, 0x21));
    encoded.putHandle(handleWithCode(0x0A, 0x22));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LayerFilter parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_names == std::vector<UTF8STRING>{"LayerA", "LayerB"});
    CHECK(parsed.parentHandle == 0x20u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x21u);
    CHECK(parsed.xDictHandle == 0x22u);

    auto truncatedBytes = bytes;
    REQUIRE(truncatedBytes.size() > 8);
    truncatedBytes.resize(truncatedBytes.size() - 8);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    DRW_LayerFilter truncated;
    truncated.m_names.push_back("stale");
    truncated.parentHandle = 0xBEEFu;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        truncated, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncated.m_names.empty());
    CHECK(truncated.parentHandle == 0u);

    dwgBufferW physicallyTruncated;
    emitObjectPreamble(physicallyTruncated, ver, /*oType=*/0,
                       /*handle=*/0x511, /*numReactors=*/0,
                       /*xDictFlag=*/1);
    physicallyTruncated.putBitLong(static_cast<std::int32_t>(
        DRW_LayerFilter::kMaxNameCount));
    emitCommonHandlePrefix(physicallyTruncated, /*parentHandle=*/0,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);
    auto physicallyTruncatedBytes = snapshot(physicallyTruncated);
    dwgBuffer physicallyTruncatedReader(physicallyTruncatedBytes.data(),
                                         physicallyTruncatedBytes.size());
    DRW_LayerFilter physicallyTruncatedParsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        physicallyTruncatedParsed, ver, &physicallyTruncatedReader));
    CHECK_FALSE(physicallyTruncatedReader.isGood());
    CHECK(physicallyTruncatedParsed.m_names.empty());
}

TEST_CASE("DRW_LightList rejects a truncated common handle stream",
          "[dwg-read][object-encode][lightlist][safety]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x520,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(0); // class version
    encoded.putBitLong(0); // light count
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x20,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LightList parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_classVersion == 0u);
    CHECK(parsed.m_lights.empty());

    REQUIRE(bytes.size() > 8);
    bytes.resize(bytes.size() - 8);
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    parsed.m_classVersion = 7;
    parsed.m_lights.push_back({0x42u, "stale"});
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_classVersion == 0u);
    CHECK(parsed.m_lights.empty());
}

TEST_CASE("DRW_LightList keeps child entries inside the object body",
          "[dwg-read][safety][lightlist]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x521,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(0); // class version
    encoded.putBitLong(1); // one light entry
    const std::uint32_t bodyBitSize = encoded.bitCount();
    encoded.putHandle(hardPtr(0x70)); // light handle is outside the body
    encoded.putVariableText(ver, "Lamp");
    encoded.putHandle(hardPtr(0x20)); // common owner handle
    encoded.patchRawLong32AtBit(2, bodyBitSize);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LightList parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_lights.empty());
    CHECK(parsed.m_classVersion == 0u);
}

TEST_CASE("DRW_LightList reads a bounded child entry",
          "[dwg-read][object-encode][lightlist]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x522,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(7); // class version
    encoded.putBitLong(1); // one light entry
    encoded.putHandle(hardPtr(0x70));
    encoded.putVariableText(ver, "Lamp");
    const std::uint32_t bodyEnd = encoded.bitCount();
    encoded.putHandle(hardPtr(0x20)); // common owner handle
    encoded.patchRawLong32AtBit(2, bodyEnd);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LightList parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_classVersion == 7u);
    REQUIRE(parsed.m_lights.size() == 1u);
    CHECK(parsed.m_lights[0].m_handle == 0x70u);
    CHECK(parsed.m_lights[0].m_name == "Lamp");
    CHECK(parsed.parentHandle == 0x20u);
}

TEST_CASE("DRW_DataLink uses object common handle semantics",
          "[dwg-read][object-encode][datalink]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x511,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "Adapter");
    encoded.putVariableText(ver, "Description");
    encoded.putVariableText(ver, "Tooltip");
    encoded.putVariableText(ver, "Connection");
    encoded.putBitLong(1);
    encoded.putBitLong(2);
    encoded.putBitLong(3);
    for (int i = 0; i < 7; ++i)
        encoded.putBitShort(static_cast<std::int16_t>(i + 1));
    encoded.putBitShort(8);
    encoded.putBitLong(9);
    encoded.putVariableText(ver, "Updated");
    encoded.putBitLong(0);
    encoded.putHandle(hardPtr(0x30));
    encoded.putHandle(hardPtr(0x20));
    encoded.putHandle(handleWithCode(0x0A, 0x21));
    encoded.putHandle(handleWithCode(0x0A, 0x22));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DataLink parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_dataAdapter == "Adapter");
    CHECK(parsed.m_hardOwnerHandle == 0x30u);
    CHECK(parsed.parentHandle == 0x20u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x21u);
    CHECK(parsed.xDictHandle == 0x22u);

    auto truncatedBytes = bytes;
    REQUIRE(truncatedBytes.size() > 8);
    truncatedBytes.resize(truncatedBytes.size() - 8);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    parsed.m_dataAdapter = "stale";
    parsed.m_customDataCount = 7;
    parsed.parentHandle = 0xBEEFu;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_dataAdapter.empty());
    CHECK(parsed.m_customDataCount == 0);
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_DataLink rejects custom data outside the body",
          "[dwg-read][object-encode][datalink][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0x513,
                       /*numReactors=*/0);
    for (int i = 0; i < 4; ++i)
        body.putVariableText(version, "");
    for (int i = 0; i < 3; ++i)
        body.putBitLong(0);
    for (int i = 0; i < 7; ++i)
        body.putBitShort(0);
    body.putBitShort(0);
    body.putBitLong(DRW_DataLink::kMaxCustomDataCount);
    body.patchRawLong32AtBit(objectSizeBitOffset(body), body.bitCount());
    body.putHandle(nullHandle()); // hard owner, outside the declared body
    emitCommonHandlePrefix(body, /*parentHandle=*/0x20,
                           /*reactorHandles=*/{}, /*xDictFlag=*/0);

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DataLink parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_customData.empty());
}

TEST_CASE("DRW_GeoMapImage uses object common handle semantics",
          "[dwg-read][object-encode][geomapimage]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x512,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(2);
    encoded.put3BitDouble(DRW_Coord{1.0, 2.0, 3.0});
    encoded.put2RawDouble(DRW_Coord{4.0, 5.0, 0.0});
    encoded.putBitShort(6);
    encoded.putBit(1);
    encoded.putRawChar8(7);
    encoded.putRawChar8(8);
    encoded.putRawChar8(9);
    encoded.putHandle(hardPtr(0x20));
    encoded.putHandle(handleWithCode(0x0A, 0x21));
    encoded.putHandle(handleWithCode(0x0A, 0x22));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_GeoMapImage parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_classVersion == 2);
    CHECK(parsed.m_displayProps == 6u);
    CHECK(parsed.m_clipping);
    CHECK(parsed.parentHandle == 0x20u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x21u);
    CHECK(parsed.xDictHandle == 0x22u);

    auto truncatedBytes = bytes;
    REQUIRE(truncatedBytes.size() > 8);
    truncatedBytes.resize(truncatedBytes.size() - 8);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    parsed.m_classVersion = 7;
    parsed.m_displayProps = 99;
    parsed.parentHandle = 0xBEEFu;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_classVersion == 0);
    CHECK(parsed.m_displayProps == 0u);
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_GeoMapImage rejects non-finite geometry transactionally",
          "[dwg-read][object-encode][geomapimage][safety]") {
    const DRW::Version ver = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    const auto makeRecord = [&](bool badInsertionPoint) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x513,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitLong(2);
        body.put3BitDouble(badInsertionPoint
                               ? DRW_Coord{nan, 2.0, 3.0}
                               : DRW_Coord{1.0, 2.0, 3.0});
        body.put2RawDouble(badInsertionPoint
                               ? DRW_Coord{4.0, nan, 0.0}
                               : DRW_Coord{4.0, 5.0, 0.0});
        body.putBitShort(6);
        body.putBit(1);
        body.putRawChar8(7);
        body.putRawChar8(8);
        body.putRawChar8(9);
        body.putHandle(hardPtr(0x20));
        return snapshot(body);
    };

    for (auto bytes : {makeRecord(true), makeRecord(false)}) {
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_GeoMapImage parsed;
        parsed.m_classVersion = 7;
        parsed.m_displayProps = 99;
        parsed.parentHandle = 0x98;

        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.m_classVersion == 0);
        CHECK(parsed.m_displayProps == 0u);
        CHECK(parsed.parentHandle == 0);
    }
}

// 2a.0 (gap entity-reactors-xdict-dropped-roundtrip): DRW_TableEntry gained
// reactorHandles/xDictHandle. The hand-written copy ctor must copy them (a
// missing init-list entry would silently drop reactors on copy) and reset()
// must clear them.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_TableEntry copy ctor + reset handle reactor/xdict members",
          "[dwg-read][object-encode][phase2a]") {
    DRW_Block_Record src;   // any concrete DRW_TableEntry
    src.reactorHandles = {0xA0, 0xA1};
    src.xDictHandle = 0xB0;

    DRW_Block_Record copy(src);   // exercises the hand-written copy ctor
    REQUIRE(copy.reactorHandles.size() == 2u);
    REQUIRE(copy.reactorHandles[0] == 0xA0u);
    REQUIRE(copy.reactorHandles[1] == 0xA1u);
    REQUIRE(copy.xDictHandle == 0xB0u);

    copy.reset();
    REQUIRE(copy.reactorHandles.empty());
    REQUIRE(copy.xDictHandle == 0u);
    // The source must be untouched by the copy.
    REQUIRE(src.reactorHandles.size() == 2u);
    REQUIRE(src.xDictHandle == 0xB0u);
}

TEST_CASE("DRW_TableEntry extData copy and move retain ownership safely",
          "[dwg-read][object-encode][ownership]") {
    TableEntryProbe source;
    source.extData.push_back(nullptr);
    REQUIRE(source.addExtData(std::make_unique<DRW_Variant>(
        1000, std::string{"payload"})));

    TableEntryProbe copy(source);
    REQUIRE(copy.extData.size() == 2u);
    REQUIRE(copy.extData[0] == nullptr);
    REQUIRE(copy.extData[1] != source.extData[1]);
    REQUIRE(*copy.extData[1]->content.s == "payload");

    TableEntryProbe assigned;
    REQUIRE(assigned.addExtData(std::make_unique<DRW_Variant>(1071, 7)));
    assigned = source;
    REQUIRE(assigned.extData.size() == 2u);
    REQUIRE(assigned.extData[0] == nullptr);
    REQUIRE(assigned.extData[1] != source.extData[1]);

    TableEntryProbe moved(std::move(copy));
    REQUIRE(moved.extData.size() == 2u);
    REQUIRE(moved.extData[0] == nullptr);
    REQUIRE(moved.extData[1] != nullptr);
    REQUIRE(copy.extData.empty());
}

TEST_CASE("DRW_Block_Record preserves xref handle and bounds owned count",
          "[dwg-read][safety][blockrecord]") {
    const DRW::Version version = DRW::AC1018;

    dwgBufferW valid;
    emitObjectPreamble(valid, version, /*oType=*/49, /*handle=*/0x410,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    valid.putVariableText(version, "Block");
    valid.putBit(0);             // is_xref_ref
    valid.putBitShort(0);        // xref index
    valid.putBit(0);             // is_xref_dep
    valid.putBit(0);             // anonymous
    valid.putBit(0);             // has attributes
    valid.putBit(0);             // is xref
    valid.putBit(0);             // xref overlay
    valid.putBit(0);             // xref loaded
    valid.putBitLong(0);         // num_owned
    valid.put3BitDouble(DRW_Coord(0.0, 0.0, 0.0));
    valid.putVariableText(version, ""); // xref path
    valid.putRawChar8(2);        // first opaque insert-count marker
    valid.putRawChar8(1);        // second opaque insert-count marker
    valid.putRawChar8(0);        // num inserts terminator
    valid.putVariableText(version, ""); // description
    valid.putBitLong(3);         // preview size
    valid.putRawChar8(0x42);
    valid.putRawChar8(0x4D);
    valid.putRawChar8(0x50);
    const std::uint32_t bodyEndBit = valid.bitCount();

    valid.putHandle(hardPtr(0x30));  // BLOCK_CONTROL owner
    valid.putHandle(nullHandle());   // extension dictionary
    valid.putHandle(hardPtr(0xAB));  // COMMON_TABLE_FLAGS xref handle
    valid.putHandle(hardPtr(0x411)); // BLOCK entity
    valid.putHandle(hardPtr(0x412)); // ENDBLK entity
    valid.putHandle(hardPtr(0x413)); // first INSERT soft pointer
    valid.putHandle(hardPtr(0x414)); // second INSERT soft pointer
    valid.putHandle(nullHandle());   // LAYOUT
    valid.patchRawLong32AtBit(objectSizeBitOffset(valid), bodyEndBit);

    auto validBytes = snapshot(valid);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_Block_Record parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseBlockRecord(parsed, version,
                                                         &validReader));
    CHECK(parsed.xrefBlockHandle.ref == 0xABu);
    CHECK(parsed.insertCount == 2u);
    REQUIRE(parsed.insertHandles.size() == 2u);
    CHECK(parsed.insertHandles[0] == 0x413u);
    CHECK(parsed.insertHandles[1] == 0x414u);
    REQUIRE(parsed.previewData.size() == 3u);
    CHECK(parsed.previewData[0] == 0x42u);
    CHECK(parsed.previewData[1] == 0x4Du);
    CHECK(parsed.previewData[2] == 0x50u);
    CHECK(parsed.layoutHandle == 0u);

    auto truncatedBytes = validBytes;
    REQUIRE(!truncatedBytes.empty());
    truncatedBytes.pop_back(); // remove the required layout handle
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    DRW_Block_Record truncated = parsed;
    truncated.basePoint = DRW_Coord(9.0, 8.0, 7.0);
    truncated.xrefPath = "stale-xref";
    truncated.description = "stale-description";
    truncated.previewData = {0xAA};
    truncated.canExplode = false;
    truncated.layoutHandle = 0x999u;
    truncated.xrefBlockHandle = hardPtr(0x998u);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseBlockRecord(
        truncated, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncated.basePoint.x == 0.0);
    CHECK(truncated.basePoint.y == 0.0);
    CHECK(truncated.basePoint.z == 0.0);
    CHECK(truncated.xrefPath.empty());
    CHECK(truncated.description.empty());
    CHECK(truncated.previewData.empty());
    CHECK(truncated.canExplode);
    CHECK(truncated.layoutHandle == 0u);
    CHECK(truncated.xrefBlockHandle.ref == 0u);

    dwgBufferW invalid;
    emitObjectPreamble(invalid, version, /*oType=*/49, /*handle=*/0x420,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    invalid.putVariableText(version, "Block");
    invalid.putBit(0);
    invalid.putBitShort(0);
    invalid.putBit(0);
    invalid.putBit(0);
    invalid.putBit(0);
    invalid.putBit(0);
    invalid.putBit(0);
    invalid.putBit(0);
    invalid.putBitLong(0xF00000);

    auto invalidBytes = snapshot(invalid);
    dwgBuffer invalidReader(invalidBytes.data(), invalidBytes.size());
    DRW_Block_Record rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseBlockRecord(
        rejected, version, &invalidReader));
    CHECK_FALSE(invalidReader.isGood());
}

TEST_CASE("DRW_Block_Record reads an AC1024 detached body and ownership list",
          "[dwg-read][object-encode][blockrecord][ac1024]") {
    const DRW::Version version = DRW::AC1024;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/49, /*handle=*/0x510,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    data.putBitShort(0);        // is_xref_resolved
    data.putBit(0);             // anonymous
    data.putBit(0);             // has attributes
    data.putBit(0);             // xref
    data.putBit(0);             // xref overlay
    data.putBit(0);             // xref loaded
    data.putBitLong(1);         // num_owned
    data.put3BitDouble(DRW_Coord(1.0, 2.0, 3.0));
    strings.putVariableText(version, "BlockName");
    data.putRawChar8(0);        // insert vector terminator
    strings.putVariableText(version, "xref.dwg");
    strings.putVariableText(version, "description");
    data.putBitLong(1);         // preview size
    data.putRawChar8(0x42);
    data.putBitShort(4);        // insertion units
    data.putBit(1);             // can explode
    data.putRawChar8(0);        // block scaling

    data.alignToByte();
    strings.alignToByte();
    REQUIRE(!strings.data().empty());
    data.putBytes(strings.data().data(), strings.data().size());
    for (int bit = 0; bit < 7; ++bit)
        data.putBit(0);
    const std::uint32_t stringBitSize =
        static_cast<std::uint32_t>(strings.data().size() * 8u + 7u);
    REQUIRE(stringBitSize <= 0x7FFFu);
    data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    data.putBit(1);             // detached strings are present
    data.alignToByte();

    handles.putHandle(hardPtr(0x30));   // BLOCK_CONTROL owner
    handles.putHandle(nullHandle());    // extension dictionary
    handles.putHandle(hardPtr(0xAB));   // xref block handle
    handles.putHandle(hardPtr(0x511));  // BLOCK entity
    handles.putHandle(hardPtr(0x512));  // owned entity
    handles.putHandle(hardPtr(0x513));  // ENDBLK entity
    handles.putHandle(nullHandle());    // LAYOUT
    handles.alignToByte();

    auto bytes = snapshot(data);
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    DRW_TextCodec decoder;
    decoder.setVersion(version, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size());
    reader.decoder = &decoder;
    DRW_Block_Record parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseBlockRecord(
        parsed, version, &reader,
        static_cast<std::uint32_t>(handles.data().size() * 8u)));
    CHECK(parsed.name == "BlockName");
    CHECK(parsed.xrefPath == "xref.dwg");
    CHECK(parsed.description == "description");
    CHECK(parsed.basePoint.x == Catch::Approx(1.0));
    CHECK(parsed.basePoint.y == Catch::Approx(2.0));
    CHECK(parsed.basePoint.z == Catch::Approx(3.0));
    const auto& entityHandles =
        DrwObjectEncodeTestAccess::blockEntityHandles(parsed);
    REQUIRE(entityHandles.size() == 1u);
    CHECK(entityHandles[0] == 0x512u);
    CHECK(DrwObjectEncodeTestAccess::blockHandle(parsed) == 0x511u);
    CHECK(DrwObjectEncodeTestAccess::endBlockHandle(parsed) == 0x513u);
    CHECK(parsed.xrefBlockHandle.ref == 0xABu);
    CHECK(parsed.layoutHandle == 0u);
    REQUIRE(parsed.previewData.size() == 1u);
    CHECK(parsed.previewData[0] == 0x42u);
    CHECK(parsed.insUnits == 4);
    CHECK(parsed.canExplode);
}

TEST_CASE("DRW_Textstyle stages body and table handles",
          "[dwg-read][object-encode][textstyle]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x35,
                       /*handle=*/0x430, /*numReactors=*/1);
    encoded.putVariableText(version, "Styled");
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.putBit(1);             // vertical text
    encoded.putBit(0);             // shape file
    encoded.putBitDouble(2.5);     // fixed height
    encoded.putBitDouble(0.8);     // width factor
    encoded.putBitDouble(0.25);    // oblique angle
    encoded.putRawChar8(6);        // generation flags
    encoded.putBitDouble(3.5);     // last height
    encoded.putVariableText(version, "font.shx");
    encoded.putVariableText(version, "bigfont.shx");
    encoded.putHandle(hardPtr(0x440));       // STYLE_CONTROL owner
    encoded.putHandle(handleWithCode(0xA, 0x441)); // reactor
    encoded.putHandle(handleWithCode(0xC, 0x442)); // xdictionary
    encoded.putHandle(handleWithCode(0xC, 0x443)); // xref block

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Textstyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseTextstyle(
        parsed, version, &reader));
    CHECK(parsed.name == "Styled");
    CHECK(parsed.flags == 4);
    CHECK(parsed.height == Approx(2.5));
    CHECK(parsed.width == Approx(0.8));
    CHECK(parsed.oblique == Approx(0.25));
    CHECK(parsed.genFlag == 6);
    CHECK(parsed.lastHeight == Approx(3.5));
    CHECK(parsed.font == "font.shx");
    CHECK(parsed.bigFont == "bigfont.shx");
    CHECK(parsed.parentHandle == 0x440u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x441u);
    CHECK(parsed.xDictHandle == 0x442u);
    CHECK(parsed.xrefBlockHandle.ref == 0x443u);

    bytes.pop_back(); // truncate the required xref-block handle
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    parsed.name = "stale-name";
    parsed.font = "stale-font";
    parsed.bigFont = "stale-bigfont";
    parsed.reactorHandles = {0x444};
    parsed.xDictHandle = 0x445;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseTextstyle(
        parsed, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.font == "txt");
    CHECK(parsed.bigFont.empty());
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("DRW_Textstyle rejects a body ending before xref flags",
          "[dwg-read][object-encode][textstyle][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x35,
                       /*handle=*/0x451);
    encoded.putVariableText(version, "ShortStyle");
    const std::uint32_t nameEndBit = encoded.bitCount();
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), nameEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Textstyle rejected;
    rejected.name = "stale-name";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseTextstyle(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
}

TEST_CASE("DRW_Textstyle AC1021 framing preserves detached strings",
          "[dwg-read][object-encode][textstyle][ac1021]") {
    DRW_Textstyle source;
    source.handle = 0x450;
    source.name = "R2007-STYLE";
    source.flags = 1 | 4;
    source.height = 1.25;
    source.width = 0.75;
    source.oblique = 0.125;
    source.genFlag = 2;
    source.lastHeight = 2.5;
    source.font = "unicode.shx";
    source.bigFont = "unicode-big.shx";
    source.reactorHandles = {0x451u};
    source.xDictHandle = 0x452u;
    source.xrefBlockHandle = hardPtr(0x453u);

    auto bytes = emitAc1021TextStyleObject(source);
    REQUIRE_FALSE(bytes.empty());
    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1021, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_Textstyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseTextstyle(
        parsed, DRW::AC1021, &reader));
    CHECK(parsed.name == source.name);
    CHECK(parsed.flags == (source.flags | 0x40));
    CHECK(parsed.height == Approx(source.height));
    CHECK(parsed.width == Approx(source.width));
    CHECK(parsed.oblique == Approx(source.oblique));
    CHECK(parsed.genFlag == source.genFlag);
    CHECK(parsed.lastHeight == Approx(source.lastHeight));
    CHECK(parsed.font == source.font);
    CHECK(parsed.bigFont == source.bigFont);
    CHECK(parsed.parentHandle == 0x03u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x451u);
    CHECK(parsed.xDictHandle == 0x452u);
    CHECK(parsed.xrefBlockHandle.ref == 0x453u);

    auto truncated = bytes;
    REQUIRE(truncated.size() > 2);
    truncated.resize(truncated.size() - 2);
    dwgBuffer truncatedReader(truncated.data(), truncated.size(), &decoder);
    parsed.name = "stale-name";
    parsed.bigFont = "stale-bigfont";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseTextstyle(
        parsed, DRW::AC1021, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.bigFont.empty());
}

TEST_CASE("DRW_Textstyle rejects generation flags outside DWG RC",
          "[dwg-write][object-encode][textstyle][safety]") {
    DRW_Textstyle source;
    source.genFlag = 0x100;

    dwgBufferW body;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeTextstyle(
        source, DRW::AC1018, &body));
    CHECK(snapshot(body).empty());

    source.genFlag = -1;
    dwgBufferW negativeFlagsBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeTextstyle(
        source, DRW::AC1018, &negativeFlagsBody));
    CHECK(snapshot(negativeFlagsBody).empty());

    source.genFlag = 1;
    dwgBufferW reservedGenerationFlagBody;
    reservedGenerationFlagBody.putRawChar8(0x6C);
    const auto reservedGenerationFlagBytes = snapshot(reservedGenerationFlagBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeTextstyle(
        source, DRW::AC1018, &reservedGenerationFlagBody));
    CHECK(reservedGenerationFlagBody.data() == reservedGenerationFlagBytes);

    source.genFlag = 0;
    source.height = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW nonFiniteHeightBody;
    nonFiniteHeightBody.putRawChar8(0xA5);
    const auto nonFiniteHeightBytes = snapshot(nonFiniteHeightBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeTextstyle(
        source, DRW::AC1018, &nonFiniteHeightBody));
    CHECK(nonFiniteHeightBody.data() == nonFiniteHeightBytes);

    source.height = 0.0;
    source.lastHeight = std::numeric_limits<double>::infinity();
    dwgBufferW nonFiniteLastHeightBody;
    nonFiniteLastHeightBody.putRawChar8(0x5A);
    const auto nonFiniteLastHeightBytes = snapshot(nonFiniteLastHeightBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeTextstyle(
        source, DRW::AC1018, &nonFiniteLastHeightBody));
    CHECK(nonFiniteLastHeightBody.data() == nonFiniteLastHeightBytes);
}

TEST_CASE("DRW_Textstyle rejects non-finite DWG values",
          "[dwg-read][object-encode][textstyle][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x35,
                       /*handle=*/0x431u);
    encoded.putVariableText(version, "NonFiniteStyle");
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.putBit(0);             // vertical text
    encoded.putBit(0);             // shape file
    encoded.putBitDouble(nan);
    encoded.putBitDouble(0.0);
    encoded.putBitDouble(0.0);
    encoded.putRawChar8(0);
    encoded.putBitDouble(0.0);
    encoded.putVariableText(version, "font.shx");
    encoded.putVariableText(version, "");

    const std::uint32_t bodyEndBit = encoded.bitCount();
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);
    encoded.putHandle(hardPtr(0x03u)); // STYLE_CONTROL owner
    encoded.putHandle(nullHandle());   // xdictionary
    encoded.putHandle(nullHandle());   // xref block

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Textstyle rejected;
    rejected.name = "stale-style";
    rejected.height = 1.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseTextstyle(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
    CHECK(rejected.height == 0.0);
}

TEST_CASE("DRW_LType rejects non-finite emitted lengths",
          "[dwg-write][object-encode][ltype][safety]") {
    DRW_LType source;
    source.length = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW lengthBody;
    lengthBody.putRawChar8(0xA5);
    const auto lengthBytes = snapshot(lengthBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeLtype(
        source, DRW::AC1018, &lengthBody));
    CHECK(lengthBody.data() == lengthBytes);

    source.length = 0.0;
    source.size = 1;
    source.path = {std::numeric_limits<double>::infinity()};
    dwgBufferW dashBody;
    dashBody.putRawChar8(0x5A);
    const auto dashBytes = snapshot(dashBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeLtype(
        source, DRW::AC1018, &dashBody));
    CHECK(dashBody.data() == dashBytes);
}

TEST_CASE("DRW_AppId stages body and table handles",
          "[dwg-read][object-encode][appid]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x0D,
                       /*handle=*/0x460, /*numReactors=*/1);
    encoded.putVariableText(version, "ACAD");
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.putRawChar8(0);        // unknown code 71
    encoded.putHandle(hardPtr(0x20));
    encoded.putHandle(handleWithCode(0x0A, 0x21));
    encoded.putHandle(handleWithCode(0x0C, 0x22));
    encoded.putHandle(handleWithCode(0x0C, 0x23));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_AppId parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseAppId(parsed, version, &reader));
    CHECK(parsed.name == "ACAD");
    CHECK(parsed.flags == 0);
    CHECK(parsed.unknown71 == 0u);
    CHECK(parsed.parentHandle == 0x20u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x21u);
    CHECK(parsed.xDictHandle == 0x22u);

    bytes.pop_back(); // truncate the required xref handle
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    parsed.name = "stale-name";
    parsed.parentHandle = 0x99;
    parsed.reactorHandles = {0x98};
    parsed.xDictHandle = 0x97;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAppId(
        parsed, version, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("DRW_AppId AC1021 framing preserves detached strings",
          "[dwg-read][object-encode][appid][ac1021]") {
    DRW_AppId source;
    source.handle = 0x470;
    source.name = "R2007-APPID";
    source.flags = 0;
    source.unknown71 = 0xA5;

    auto bytes = emitAc1021AppIdObject(source);
    REQUIRE_FALSE(bytes.empty());
    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1021, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_AppId parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseAppId(
        parsed, DRW::AC1021, &reader));
    CHECK(parsed.name == source.name);
    CHECK(parsed.flags == (source.flags | 0x40));
    CHECK(parsed.unknown71 == source.unknown71);
    CHECK(parsed.parentHandle == 0x09u);
    CHECK(parsed.xDictHandle == 0u);

    auto truncated = bytes;
    REQUIRE(truncated.size() > 2);
    truncated.resize(truncated.size() - 2);
    dwgBuffer truncatedReader(truncated.data(), truncated.size(), &decoder);
    parsed.name = "stale-name";
    parsed.parentHandle = 0x99;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAppId(
        parsed, DRW::AC1021, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_AppId rejects a body ending before xref flags",
          "[dwg-read][object-encode][appid][safety]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0x0D, /*handle=*/0x471,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "ACAD");
    const std::uint32_t nameEndBit = encoded.bitCount();
    encoded.putBit(0);       // xref referenced
    encoded.putBitShort(0);  // xref index
    encoded.putBit(0);       // xref dependent
    encoded.putRawChar8(0);  // unknown code 71
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x20,
                           /*reactorHandles=*/{}, /*xDictFlag=*/0);
    encoded.putHandle(hardPtr(0x21)); // xref handle
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), nameEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_AppId parsed;
    parsed.name = "stale-name";
    parsed.flags = 99;
    parsed.parentHandle = 0x99;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseAppId(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.flags == 0);
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
}

TEST_CASE("DRW_AppId encodes every supplied reactor handle",
          "[dwg-write][object-encode][appid][safety]") {
    const DRW::Version version = DRW::AC1015;
    DRW_AppId source;
    source.name = "REACT-APP";
    source.reactorHandles = {0x21u, 0x22u};
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x0D,
                       /*handle=*/0x460u, /*numReactors=*/2,
                       /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeAppId(
        source, version, &encoded));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_AppId parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseAppId(
        parsed, version, &reader));
    REQUIRE(parsed.reactorHandles.size() == 2u);
    CHECK(parsed.reactorHandles[0] == 0x21u);
    CHECK(parsed.reactorHandles[1] == 0x22u);
}

TEST_CASE("DRW_AppId rejects invalid table flags transactionally",
          "[dwg-write][object-encode][appid][safety]") {
    DRW_AppId source;
    source.flags = 1;
    dwgBufferW encoded;
    encoded.putRawChar8(0xA5);
    const auto before = snapshot(encoded);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeAppId(
        source, DRW::AC1015, &encoded));
    CHECK(snapshot(encoded) == before);
}

TEST_CASE("legacy table records accept an empty bounded body",
          "[dwg-read][object-encode][table][legacy]") {
    const DRW::Version version = DRW::AC1014;
    const auto makeRecord = [&](std::uint16_t objectType,
                                std::uint32_t handle) {
        dwgBufferW encoded;
        encoded.putObjType(version, objectType);
        encoded.putHandle(hardPtr(handle));
        encoded.putBitShort(0); // EED size = 0
        const std::size_t objectSizeBit = encoded.bitCount();
        encoded.putRawLong32(0); // empty R13/R14 object-data range
        encoded.putBitLong(0);   // no reactors
        encoded.putVariableText(version, "EmptyBody");
        const std::uint32_t objectSize = encoded.bitCount();
        emitCommonHandlePrefix(encoded, /*parentHandle=*/0x20,
                               /*reactorHandles=*/{}, /*xDictFlag=*/0);
        encoded.putHandle(nullHandle()); // table xref block
        const std::uint8_t objectSizeBytes[] = {
            static_cast<std::uint8_t>(objectSize & 0xFFu),
            static_cast<std::uint8_t>((objectSize >> 8) & 0xFFu),
            static_cast<std::uint8_t>((objectSize >> 16) & 0xFFu),
            static_cast<std::uint8_t>((objectSize >> 24) & 0xFFu)};
        REQUIRE(encoded.patchRawBytesAtBit(objectSizeBit, objectSizeBytes,
                                            sizeof(objectSizeBytes)));
        return snapshot(encoded);
    };

    auto layerBytes = makeRecord(/*objectType=*/0x34, /*handle=*/0x4C0);
    dwgBuffer layerReader(layerBytes.data(), layerBytes.size());
    DRW_Layer layer;
    REQUIRE(DrwObjectEncodeTestAccess::parseLayer(
        layer, version, &layerReader));
    CHECK(layer.name == "EmptyBody");
    CHECK(layer.flags == 0);
    CHECK(layer.color == 0);
    CHECK(layer.plotF);
    CHECK(layer.parentHandle == 0x20u);
    CHECK(layer.xDictHandle == 0u);

    auto ltypeBytes = makeRecord(/*objectType=*/0x39, /*handle=*/0x4C1);
    dwgBuffer ltypeReader(ltypeBytes.data(), ltypeBytes.size());
    DRW_LType ltype;
    REQUIRE(DrwObjectEncodeTestAccess::parseLtype(
        ltype, version, &ltypeReader));
    CHECK(ltype.name == "EmptyBody");
    CHECK(ltype.size == 0);
    CHECK(ltype.path.empty());
    CHECK(ltype.parentHandle == 0x20u);
    CHECK(ltype.xDictHandle == 0u);

    auto textstyleBytes = makeRecord(/*objectType=*/0x35, /*handle=*/0x4C2);
    dwgBuffer textstyleReader(textstyleBytes.data(), textstyleBytes.size());
    DRW_Textstyle textstyle;
    REQUIRE(DrwObjectEncodeTestAccess::parseTextstyle(
        textstyle, version, &textstyleReader));
    CHECK(textstyle.name == "EmptyBody");
    CHECK(textstyle.flags == 0);
    CHECK(textstyle.parentHandle == 0x20u);
    CHECK(textstyle.xDictHandle == 0u);

    auto appIdBytes = makeRecord(/*objectType=*/0x43, /*handle=*/0x4C3);
    dwgBuffer appIdReader(appIdBytes.data(), appIdBytes.size());
    DRW_AppId appId;
    REQUIRE(DrwObjectEncodeTestAccess::parseAppId(
        appId, version, &appIdReader));
    CHECK(appId.name == "EmptyBody");
    CHECK(appId.flags == 0);
    CHECK(appId.parentHandle == 0x20u);
    CHECK(appId.xDictHandle == 0u);
}

TEST_CASE("DRW_Dimstyle stages body and reference handles",
          "[dwg-read][object-encode][dimstyle]") {
    DRW_Dimstyle source;
    source.handle = 0x480;
    source.name = "R2000-DIMSTYLE";
    source.dimpost = "<> mm";
    source.dimapost = "[<>]";
    source.dimscale = 2.0;
    source.dimtxt = 0.75;
    source.dimtad = 1;
    source.dimdec = 3;
    source.dimlwd = -3;
    source.dimlwe = 4;
    source.dimtxstyH = hardPtr(0x481);
    source.dimldrblkH = hardPtr(0x482);
    source.dimblkH = hardPtr(0x483);
    source.dimblk1H = hardPtr(0x484);
    source.dimblk2H = hardPtr(0x485);

    auto bytes = emitAc1015DimstyleObject(source);
    REQUIRE_FALSE(bytes.empty());
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Dimstyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseDimstyle(
        parsed, DRW::AC1015, &reader));
    CHECK(parsed.name == source.name);
    CHECK(parsed.dimpost == source.dimpost);
    CHECK(parsed.dimapost == source.dimapost);
    CHECK(parsed.dimscale == Approx(source.dimscale));
    CHECK(parsed.dimtxt == Approx(source.dimtxt));
    CHECK(parsed.dimtad == source.dimtad);
    CHECK(parsed.dimdec == source.dimdec);
    CHECK(parsed.dimlwd == source.dimlwd);
    CHECK(parsed.dimlwe == source.dimlwe);
    CHECK(parsed.parentHandle == 0x0Au);
    CHECK(parsed.dimtxstyH.ref == 0x481u);
    CHECK(parsed.dimldrblkH.ref == 0x482u);
    CHECK(parsed.dimblkH.ref == 0x483u);
    CHECK(parsed.dimblk1H.ref == 0x484u);
    CHECK(parsed.dimblk2H.ref == 0x485u);

    bytes.pop_back(); // truncate the final DIMBLK2 handle
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    parsed.name = "stale-name";
    parsed.dimpost = "stale-post";
    parsed.dimtxstyH = hardPtr(0x999);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDimstyle(
        parsed, DRW::AC1015, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.dimpost.empty());
    CHECK(parsed.dimtxstyH.ref == 0u);
}

TEST_CASE("DRW_Dimstyle decodes the R13/R14 typed body",
          "[dwg-read][object-encode][dimstyle][legacy]") {
    for (const DRW::Version version : {DRW::AC1012, DRW::AC1014}) {
        dwgBufferW encoded;
        encoded.putObjType(version, /*oType=*/0x45);
        encoded.putHandle(hardPtr(0x4B0));
        encoded.putBitShort(0); // EED size = 0
        const std::size_t objectSizeBit = encoded.bitCount();
        encoded.putRawLong32(0); // R13/R14 object-data end in bits
        encoded.putBitLong(1);   // one reactor
        encoded.putVariableText(version, "LegacyDimstyle");

        encoded.putBit(1);       // xref referenced
        encoded.putBitShort(7);  // xref index
        encoded.putBit(1);       // xref dependent
        encoded.putBit(1);       // DIMTOL
        encoded.putBit(0);       // DIMLIM
        encoded.putBit(1);       // DIMTIH
        encoded.putBit(0);       // DIMTOH
        encoded.putBit(1);       // DIMSE1
        encoded.putBit(0);       // DIMSE2
        encoded.putBit(1);       // DIMALT
        encoded.putBit(0);       // DIMTOFL
        encoded.putBit(1);       // DIMSAH
        encoded.putBit(0);       // DIMTIX
        encoded.putBit(1);       // DIMSOXD
        encoded.putRawChar8(5);  // DIMALTD
        encoded.putRawChar8(6);  // DIMZIN
        encoded.putBit(1);       // DIMSD1
        encoded.putBit(0);       // DIMSD2
        encoded.putRawChar8(2);  // DIMTOLJ
        encoded.putRawChar8(4);  // DIMJUST
        encoded.putRawChar8(3);  // DIMFIT
        encoded.putBit(1);       // DIMUPT
        encoded.putRawChar8(8);  // DIMTZIN
        encoded.putRawChar8(9);  // DIMALTZ
        encoded.putRawChar8(10); // DIMALTTZ
        encoded.putRawChar8(2);  // DIMTAD
        encoded.putBitShort(1);  // DIMUNIT
        encoded.putBitShort(2);  // DIMAUNIT
        encoded.putBitShort(3);  // DIMDEC
        encoded.putBitShort(4);  // DIMTDEC
        encoded.putBitShort(5);  // DIMALTU
        encoded.putBitShort(6);  // DIMALTTD

        encoded.putBitDouble(2.0);   // DIMSCALE
        encoded.putBitDouble(3.0);   // DIMASZ
        encoded.putBitDouble(4.0);   // DIMEXO
        encoded.putBitDouble(5.0);   // DIMDLI
        encoded.putBitDouble(6.0);   // DIMEXE
        encoded.putBitDouble(7.0);   // DIMRND
        encoded.putBitDouble(8.0);   // DIMDLE
        encoded.putBitDouble(9.0);   // DIMTP
        encoded.putBitDouble(10.0);  // DIMTM
        encoded.putBitDouble(11.0);  // DIMTXT
        encoded.putBitDouble(12.0);  // DIMCEN
        encoded.putBitDouble(13.0);  // DIMTSZ
        encoded.putBitDouble(14.0);  // DIMALTF
        encoded.putBitDouble(15.0);  // DIMLFAC
        encoded.putBitDouble(16.0);  // DIMTVP
        encoded.putBitDouble(17.0);  // DIMTFAC
        encoded.putBitDouble(18.0);  // DIMGAP
        encoded.putVariableText(version, "POST");
        encoded.putVariableText(version, "ALTPOST");
        encoded.putVariableText(version, "DIMBLK");
        encoded.putVariableText(version, "DIMBLK1");
        encoded.putVariableText(version, "DIMBLK2");
        encoded.putCmColor(version, 1);
        encoded.putCmColor(version, 2);
        encoded.putCmColor(version, 3);
        encoded.putBit(0); // final table-entry flag

        const std::uint32_t objectSize = encoded.bitCount();
        emitCommonHandlePrefix(encoded, /*parentHandle=*/0x20,
                               /*reactorHandles=*/{0x30},
                               /*xDictFlag=*/0);
        encoded.putHandle(nullHandle());  // xref block
        encoded.putHandle(hardPtr(0x40)); // DIMTXSTY
        const std::uint8_t objectSizeBytes[] = {
            static_cast<std::uint8_t>(objectSize & 0xFFu),
            static_cast<std::uint8_t>((objectSize >> 8) & 0xFFu),
            static_cast<std::uint8_t>((objectSize >> 16) & 0xFFu),
            static_cast<std::uint8_t>((objectSize >> 24) & 0xFFu)};
        REQUIRE(encoded.patchRawBytesAtBit(objectSizeBit, objectSizeBytes,
                                            sizeof(objectSizeBytes)));

        auto bytes = snapshot(encoded);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_Dimstyle parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseDimstyle(
            parsed, version, &reader));
        CHECK(parsed.name == "LegacyDimstyle");
        CHECK(parsed.flags == 0x50);
        CHECK(parsed.dimtol == 1);
        CHECK(parsed.dimlim == 0);
        CHECK(parsed.dimtih == 1);
        CHECK(parsed.dimtoh == 0);
        CHECK(parsed.dimse1 == 1);
        CHECK(parsed.dimse2 == 0);
        CHECK(parsed.dimalt == 1);
        CHECK(parsed.dimtofl == 0);
        CHECK(parsed.dimsah == 1);
        CHECK(parsed.dimtix == 0);
        CHECK(parsed.dimsoxd == 1);
        CHECK(parsed.dimaltd == 5);
        CHECK(parsed.dimzin == 6);
        CHECK(parsed.dimsd1 == 1);
        CHECK(parsed.dimsd2 == 0);
        CHECK(parsed.dimtolj == 2);
        CHECK(parsed.dimjust == 4);
        CHECK(parsed.dimfit == 3);
        CHECK(parsed.dimupt == 1);
        CHECK(parsed.dimtzin == 8);
        CHECK(parsed.dimaltz == 9);
        CHECK(parsed.dimaltttz == 10);
        CHECK(parsed.dimtad == 2);
        CHECK(parsed.dimunit == 1);
        CHECK(parsed.dimaunit == 2);
        CHECK(parsed.dimdec == 3);
        CHECK(parsed.dimtdec == 4);
        CHECK(parsed.dimaltu == 5);
        CHECK(parsed.dimalttd == 6);
        CHECK(parsed.dimscale == Approx(2.0));
        CHECK(parsed.dimtxt == Approx(11.0));
        CHECK(parsed.dimgap == Approx(18.0));
        CHECK(parsed.dimpost == "POST");
        CHECK(parsed.dimapost == "ALTPOST");
        CHECK(parsed.dimblk == "DIMBLK");
        CHECK(parsed.dimblk1 == "DIMBLK1");
        CHECK(parsed.dimblk2 == "DIMBLK2");
        CHECK(parsed.dimclrd == 1);
        CHECK(parsed.dimclre == 2);
        CHECK(parsed.dimclrt == 3);
        CHECK(parsed.parentHandle == 0x20u);
        REQUIRE(parsed.reactorHandles.size() == 1);
        CHECK(parsed.reactorHandles.front() == 0x30u);
        CHECK(parsed.xDictHandle == 0u);
        CHECK(parsed.dimtxstyH.ref == 0x40u);
    }
}

TEST_CASE("DRW_Dimstyle AC1021 framing preserves detached strings",
          "[dwg-read][object-encode][dimstyle][ac1021]") {
    DRW_Dimstyle source;
    source.handle = 0x490;
    source.name = "R2007-DIMSTYLE";
    source.dimpost = "<>";
    source.dimapost = "alt <>";
    source.dimscale = 1.5;
    source.dimjogang = 0.25;
    source.dimfxlon = 1;
    source.dimtxstyH = hardPtr(0x491);
    source.dimldrblkH = hardPtr(0x492);
    source.dimblkH = hardPtr(0x493);
    source.dimblk1H = hardPtr(0x494);
    source.dimblk2H = hardPtr(0x495);
    source.dimltypeH = hardPtr(0x496);
    source.dimltex1H = hardPtr(0x497);
    source.dimltex2H = hardPtr(0x498);

    auto bytes = emitR2007DimstyleObject(source, DRW::AC1021);
    REQUIRE_FALSE(bytes.empty());
    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1021, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_Dimstyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseDimstyle(
        parsed, DRW::AC1021, &reader));
    CHECK(parsed.name == source.name);
    CHECK(parsed.dimpost == source.dimpost);
    CHECK(parsed.dimapost == source.dimapost);
    CHECK(parsed.dimscale == Approx(source.dimscale));
    CHECK(parsed.dimjogang == Approx(source.dimjogang));
    CHECK(parsed.dimfxlon == source.dimfxlon);
    CHECK(parsed.flags == 0x40);
    CHECK(parsed.parentHandle == 0x0Au);
    CHECK(parsed.dimtxstyH.ref == 0x491u);
    CHECK(parsed.dimblk2H.ref == 0x495u);
    CHECK(parsed.dimltypeH.ref == 0x496u);
    CHECK(parsed.dimltex1H.ref == 0x497u);
    CHECK(parsed.dimltex2H.ref == 0x498u);

    auto truncated = bytes;
    REQUIRE(truncated.size() > 2);
    truncated.resize(truncated.size() - 2);
    dwgBuffer truncatedReader(truncated.data(), truncated.size(), &decoder);
    parsed.name = "stale-name";
    parsed.dimpost = "stale-post";
    parsed.dimltex2H = hardPtr(0x999);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDimstyle(
        parsed, DRW::AC1021, &truncatedReader));
    CHECK(parsed.name.empty());
    CHECK(parsed.dimpost.empty());
    CHECK(parsed.dimltex2H.ref == 0u);
}

TEST_CASE("DRW_Dimstyle AC1024 preserves text direction",
          "[dwg-read][object-encode][dimstyle][ac1024]") {
    DRW_Dimstyle source;
    source.handle = 0x499;
    source.name = "R2010-DIMSTYLE";
    source.dimtxtdirection = 1;

    std::uint32_t handleBits = 0;
    auto bytes = emitR2007DimstyleObject(source, DRW::AC1024, &handleBits);
    REQUIRE_FALSE(bytes.empty());
    REQUIRE(handleBits != 0);
    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1024, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_Dimstyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseDimstyle(
        parsed, DRW::AC1024, &reader, handleBits));
    CHECK(parsed.dimtxtdirection == 1);
    const DRW_Variant *direction = parsed.get("$DIMTXTDIRECTION");
    REQUIRE(direction != nullptr);
    CHECK(direction->code() == 295);
    CHECK(direction->i_val() == 1);

    bytes = emitR2007DimstyleObject(source, DRW::AC1021);
    REQUIRE_FALSE(bytes.empty());
    decoder.setVersion(DRW::AC1021, false);
    dwgBuffer r2007Reader(bytes.data(), bytes.size(), &decoder);
    REQUIRE(DrwObjectEncodeTestAccess::parseDimstyle(
        parsed, DRW::AC1021, &r2007Reader));
    CHECK(parsed.dimtxtdirection == 0);
}

TEST_CASE("DRW_Dimstyle rejects a body ending before xref flags",
          "[dwg-read][object-encode][dimstyle][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x45,
                       /*handle=*/0x4A0);
    encoded.putVariableText(version, "ShortDimstyle");
    const std::uint32_t nameEndBit = encoded.bitCount();
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), nameEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Dimstyle rejected;
    rejected.name = "stale-name";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDimstyle(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
}

TEST_CASE("DRW_Dimstyle rejects lossy numeric field narrowing",
          "[dwg-write][object-encode][dimstyle][safety]") {
    DRW_Dimstyle source;
    source.dimtfill = -1;
    dwgBufferW fillBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDimstyle(
        source, DRW::AC1021, &fillBody));
    CHECK(fillBody.data().empty());

    source.dimtfill = 0;
    source.dimclrd = std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW colorBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDimstyle(
        source, DRW::AC1015, &colorBody));
    CHECK(colorBody.data().empty());

    source.dimclrd = 0;
    source.dimlwd = std::numeric_limits<std::int16_t>::max() + 1;
    dwgBufferW weightBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDimstyle(
        source, DRW::AC1015, &weightBody));
    CHECK(weightBody.data().empty());

    source.dimlwd = -2;
    source.dimscale = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW nonfiniteBody;
    nonfiniteBody.putRawChar8(0xA5);
    const auto nonfiniteBefore = snapshot(nonfiniteBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDimstyle(
        source, DRW::AC1015, &nonfiniteBody));
    CHECK(snapshot(nonfiniteBody) == nonfiniteBefore);

    source.dimscale = 1.0;
    source.dimaltmzf = std::numeric_limits<double>::infinity();
    dwgBufferW r2010Body;
    r2010Body.putRawChar8(0x5A);
    const auto r2010Before = snapshot(r2010Body);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDimstyle(
        source, DRW::AC1024, &r2010Body));
    CHECK(snapshot(r2010Body) == r2010Before);
}

TEST_CASE("DRW_Dimstyle rejects non-finite DWG values transactionally",
          "[dwg-read][object-encode][dimstyle][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x45,
                       /*handle=*/0x4A1);
    encoded.putVariableText(version, "Dimstyle");
    encoded.putBit(0); // xref referenced
    encoded.putBitShort(0); // xref index
    encoded.putBit(0); // xref dependent
    encoded.putVariableText(version, ""); // DIMPOST
    encoded.putVariableText(version, ""); // DIMAPOST
    encoded.putBitDouble(nan); // DIMSCALE

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Dimstyle parsed;
    parsed.name = "stale dimstyle";
    parsed.dimscale = 9.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseDimstyle(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.dimscale == Approx(1.0));
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_Block_Record rejects oversized preview data",
          "[dwg-read][safety][blockrecord]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/49, /*handle=*/0x430,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    encoded.putVariableText(version, "Block");
    encoded.putBit(0);
    encoded.putBitShort(0);
    encoded.putBit(0);
    encoded.putBit(0);
    encoded.putBit(0);
    encoded.putBit(0);
    encoded.putBit(0);
    encoded.putBit(0);
    encoded.putBitLong(0);         // num_owned
    encoded.put3BitDouble(DRW_Coord(0.0, 0.0, 0.0));
    encoded.putVariableText(version, "");
    encoded.putRawChar8(0);
    encoded.putVariableText(version, "");
    encoded.putBitLong(0xA00001);  // beyond libreDWG's preview_size bound

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Block_Record rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseBlockRecord(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
}

TEST_CASE("DRW_Block_Record requires a bounded insert terminator",
          "[dwg-read][safety][blockrecord]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/49, /*handle=*/0x431,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putVariableText(version, "Block");
    encoded.putBit(0);             // xref reference
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.putBit(0);             // anonymous
    encoded.putBit(0);             // has attributes
    encoded.putBit(0);             // xref
    encoded.putBit(0);             // xref overlay
    encoded.putBit(0);             // xref loaded
    encoded.putBitLong(0);          // owned object count
    encoded.put3BitDouble(DRW_Coord(0.0, 0.0, 0.0));
    encoded.putVariableText(version, "");
    encoded.putRawChar8(1);         // missing zero insert terminator

    const std::uint32_t bodyBitSize = encoded.bitCount();
    encoded.putHandle(hardPtr(0x30)); // block control
    encoded.putHandle(hardPtr(0x31)); // xref block
    encoded.putHandle(hardPtr(0x32)); // block entity
    encoded.putHandle(hardPtr(0x33)); // end block
    encoded.putHandle(nullHandle());  // layout
    encoded.patchRawLong32AtBit(2, bodyBitSize);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Block_Record parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseBlockRecord(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.previewData.empty());
    CHECK(parsed.layoutHandle == 0u);
}

// 2a.6 (gap object-blockrecord-layout-explode-dropped): DRW_Block_Record now
// retains description (DXF 4), canExplode (280), blockScaling (281) and the
// owning LAYOUT handle (340), which parseDwg previously read-and-discarded.
// The full BLOCK_HEADER body is version-dependent and handle-heavy; the
// parse-path correctness is covered by the corpus golden check (read-only,
// member-assignment-only change). This guards the struct defaults + reset.
// NOTE: blockScaling/canExplode are populated only for R2007+ (>AC1018).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Block_Record exposes layout/explode/scaling/description members",
          "[dwg-read][object-encode][blockrecord]") {
    DRW_Block_Record br;
    // Documented defaults (used by the filter when a DWG omits them).
    REQUIRE(br.canExplode == true);
    REQUIRE(br.blockScaling == 0);
    REQUIRE(br.layoutHandle == 0u);
    REQUIRE(br.description.empty());

    // Mutate, then reset() must restore the documented defaults so a reused
    // record cannot leak a prior block's layout association.
    br.description = "stamped";
    br.canExplode = false;
    br.blockScaling = 3;
    br.layoutHandle = 0xABCD;
    br.reset();
    REQUIRE(br.canExplode == true);
    REQUIRE(br.blockScaling == 0);
    REQUIRE(br.layoutHandle == 0u);
    REQUIRE(br.description.empty());
}

// 0B.4a (gap object-mlinestyle-ltindex-inverted-version-gate-read):
// PRE-R2018 MLINESTYLE stores a signed inline lt index (BSd) per element;
// the version gate was inverted (read the BS only for R2018+), desyncing
// every element by one BS for the common R2000-R2013 case.  This test
// builds a 2-element AC1015 MLINESTYLE and asserts (a) both element
// offsets/colors stay aligned (no per-element BS drift), (b) the signed
// inline index is now CONSUMED and STORED in linetypeIndex (incl. a
// negative sentinel proving the signed read), and (c) buff stays good.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_MLineStyle::parseDwg captures per-element signed lt index (pre-R2018)",
          "[dwg-read][object-encode][mlinestyle]") {
    DRW::Version ver = DRW::AC1015;  // pre-R2018: inline BSd index present
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x400,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    w.putVariableText(ver, "MyStyle");    // name
    w.putVariableText(ver, "desc");       // description
    w.putBitShort(0);                     // flags
    w.putCmColor(ver, 256);               // fill color (CMC)
    w.putBitDouble(1.0471975512);         // startAngle
    w.putBitDouble(2.0943951024);         // endAngle
    w.putRawChar8(2);                     // numLines
    // element 0
    w.putBitDouble(0.5);                  // offset
    w.putCmColor(ver, 1);                 // color
    w.putSBitShort(0);                    // linetypeIndex (BSd) = 0
    // element 1 — uses a negative sentinel to prove the signed read.
    w.putBitDouble(-0.5);                 // offset
    w.putCmColor(ver, 2);                 // color
    w.putSBitShort(-2);                   // linetypeIndex (BSd) = -2
    w.putHandle(hardPtr(0x20));
    w.putHandle(handleWithCode(0x0A, 0x21));
    w.putHandle(handleWithCode(0x0A, 0x22));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_MLineStyle dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.name == "MyStyle");        // read before the element loop
    REQUIRE(dst.elements.size() == 2u);
    REQUIRE(dst.elements[0].offset == Approx(0.5));   // no BS drift
    REQUIRE(dst.elements[0].color == 1);
    REQUIRE(dst.elements[0].linetypeIndex == 0);      // inline index consumed
    REQUIRE(dst.elements[1].offset == Approx(-0.5));  // 2nd element aligned
    REQUIRE(dst.elements[1].color == 2);
    REQUIRE(dst.elements[1].linetypeIndex == -2);     // signed read preserved
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x20u);
    REQUIRE(dst.reactorHandles == std::vector<std::uint32_t>{0x21u});
    REQUIRE(dst.xDictHandle == 0x22u);
    REQUIRE(r.isGood());                  // stream stayed aligned
}

TEST_CASE("DRW_Index uses object common handle semantics",
          "[dwg-read][object-encode][index]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x410,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(0xAAAA);
    encoded.putBitLong(0xBBBB);
    const std::uint32_t bodyEndBit = encoded.bitCount();
    encoded.putHandle(hardPtr(0x20));
    encoded.putHandle(handleWithCode(0x0A, 0x21));
    encoded.putHandle(handleWithCode(0x0A, 0x22));
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Index parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.timestamp1 == 0xAAAAu);
    CHECK(parsed.timestamp2 == 0xBBBBu);
    CHECK(parsed.parentHandle == 0x20u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x21u);
    CHECK(parsed.xDictHandle == 0x22u);

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.timestamp1 == 0u);
    CHECK(parsed.timestamp2 == 0u);
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("DRW_MLineStyle rejects a truncated common handle stream",
          "[dwg-read][object-encode][mlinestyle]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/73, /*handle=*/0x402,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "Style");
    encoded.putVariableText(ver, "Description");
    encoded.putBitShort(0);
    encoded.putCmColor(ver, 256);
    encoded.putBitDouble(0.0);
    encoded.putBitDouble(1.0);
    encoded.putRawChar8(0);
    encoded.putHandle(hardPtr(0x20)); // parent only; reactor is truncated

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_MLineStyle parsed;
    parsed.name = "stale style";
    parsed.description = "stale description";
    parsed.elements.push_back({});
    parsed.parentHandle = 0xDEADu;
    parsed.reactorHandles.push_back(0xCAFEu);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.description.empty());
    CHECK(parsed.elements.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
}

TEST_CASE("DRW_MLineStyle rejects a body ending before flags",
          "[dwg-read][object-encode][mlinestyle]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/73, /*handle=*/0x403);
    encoded.putVariableText(ver, "Style");
    encoded.putVariableText(ver, "Description");
    const std::uint32_t descriptionEndBit = encoded.bitCount();
    encoded.putBitShort(0); // outside the declared object body
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded),
                                 descriptionEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_MLineStyle parsed;
    parsed.name = "stale style";
    parsed.description = "stale description";
    parsed.flags = 9;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.description.empty());
    CHECK(parsed.flags == 0);
}

// SPATIAL_INDEX parser test (ODA §20.4.95).  Body beyond timestamps is
// opaque, so we exercise only the timestamp capture.  Handle stream is
// gated to AC1024+ in the parser; under AC1018 the handles are not read.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_MLineStyle::encodeDwg round-trips elements (pre-R2018)",
          "[dwg-write][object-encode][mlinestyle]") {
    DRW::Version ver = DRW::AC1015;
    DRW_MLineStyle src;
    src.handle = 0x401;
    src.name = "MyStyle";
    src.description = "desc";
    src.flags = 40000;
    src.fillColor = 4;
    src.startAngle = 1.0471975512;
    src.endAngle = 2.0943951024;
    DRW_MLineElement a;
    a.offset = 0.5;
    a.color = 1;
    a.linetypeIndex = 0;
    DRW_MLineElement b;
    b.offset = -0.5;
    b.color = 2;
    b.linetypeIndex = -2;
    src.elements.push_back(a);
    src.elements.push_back(b);

    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/73, src.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeMLineStyle(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_MLineStyle dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.name == "MyStyle");
    REQUIRE(dst.description == "desc");
    REQUIRE(dst.flags == 40000);
    REQUIRE(dst.fillColor == 4);
    REQUIRE(dst.elements.size() == 2u);
    REQUIRE(dst.elements[0].offset == Approx(0.5));
    REQUIRE(dst.elements[0].color == 1);
    REQUIRE(dst.elements[0].linetypeIndex == 0);
    REQUIRE(dst.elements[1].offset == Approx(-0.5));
    REQUIRE(dst.elements[1].color == 2);
    REQUIRE(dst.elements[1].linetypeIndex == -2);
    REQUIRE(r.isGood());
}

TEST_CASE("DRW_MLineStyle rejects lossy element count and linetype narrowing",
          "[dwg-write][object-encode][mlinestyle][safety]") {
    DRW_MLineStyle tooManyElements;
    tooManyElements.elements.resize(256);
    dwgBufferW countBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLineStyle(
        tooManyElements, DRW::AC1018, &countBody));
    CHECK(countBody.data().empty());

    DRW_MLineStyle invalidLinetypeIndex;
    DRW_MLineElement element;
    element.linetypeIndex = std::numeric_limits<std::int16_t>::max() + 1;
    invalidLinetypeIndex.elements.push_back(element);
    dwgBufferW linetypeBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLineStyle(
        invalidLinetypeIndex, DRW::AC1018, &linetypeBody));
    CHECK(linetypeBody.data().empty());

    DRW_MLineStyle invalidFlags;
    invalidFlags.flags = -1;
    dwgBufferW negativeFlagsBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLineStyle(
        invalidFlags, DRW::AC1018, &negativeFlagsBody));
    CHECK(negativeFlagsBody.data().empty());

    invalidFlags.flags = std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW oversizedFlagsBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLineStyle(
        invalidFlags, DRW::AC1018, &oversizedFlagsBody));
    CHECK(oversizedFlagsBody.data().empty());

    DRW_MLineStyle invalidGeometry;
    invalidGeometry.startAngle = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW nonFiniteBody;
    nonFiniteBody.putRawChar8(0xA5);
    const auto nonFiniteBytes = snapshot(nonFiniteBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLineStyle(
        invalidGeometry, DRW::AC1018, &nonFiniteBody));
    CHECK(nonFiniteBody.data() == nonFiniteBytes);

    invalidGeometry.startAngle = 0.0;
    DRW_MLineElement invalidOffset;
    invalidOffset.offset = std::numeric_limits<double>::infinity();
    invalidGeometry.elements.push_back(invalidOffset);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLineStyle(
        invalidGeometry, DRW::AC1018, &nonFiniteBody));
    CHECK(nonFiniteBody.data() == nonFiniteBytes);
}

TEST_CASE("DRW_MLineStyle rejects non-finite DWG values transactionally",
          "[dwg-read][object-encode][mlinestyle][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/73, /*handle=*/0x402);
    encoded.putVariableText(version, "Style");
    encoded.putVariableText(version, "Description");
    encoded.putBitShort(0); // flags
    encoded.putCmColor(version, 256); // fill color
    encoded.putBitDouble(nan); // start angle

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_MLineStyle parsed;
    parsed.name = "stale style";
    parsed.startAngle = 9.0;
    parsed.elements.push_back(DRW_MLineElement{});
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.startAngle == Approx(0.0));
    CHECK(parsed.elements.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_MLeaderStyle rejects truncated body without stale state",
          "[dwg-read][object-encode][mleaderstyle]") {
    const DRW::Version ver = DRW::AC1021;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x420,
                       /*numReactors=*/1, /*xDictFlag=*/0);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_MLeaderStyle parsed;
    parsed.styleVersion = 9;
    parsed.description = "stale description";
    parsed.textDefault = "stale text";
    parsed.parentHandle = 0xDEADu;
    parsed.reactorHandles.push_back(0xCAFEu);
    parsed.leaderLineTypeHandle.ref = 0xBEEFu;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.styleVersion == 2u);
    CHECK(parsed.description.empty());
    CHECK(parsed.textDefault.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.leaderLineTypeHandle.ref == 0u);
}

TEST_CASE("DRW_MLeaderStyle rejects a body ending before content fields",
          "[dwg-read][object-encode][mleaderstyle]") {
    const DRW::Version ver = DRW::AC1021;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x421);
    encoded.putBitShort(2); // content type
    const std::uint32_t bodyEndBit = encoded.bitCount();
    encoded.putBitShort(0); // outside the declared object body
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_MLeaderStyle parsed;
    parsed.contentType = 9;
    parsed.description = "stale description";
    parsed.parentHandle = 0xDEADu;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.contentType == 2);
    CHECK(parsed.description.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_MLeaderStyle rejects CMC indices outside unsigned DWG BS",
          "[dwg-write][object-encode][mleaderstyle][safety]") {
    DRW_MLeaderStyle source;
    source.leaderColor = -1;
    dwgBufferW leaderBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLeaderStyle(
        source, DRW::AC1021, &leaderBody));
    CHECK(leaderBody.data().empty());

    source.leaderColor = 0;
    source.textColor = std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW textBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLeaderStyle(
        source, DRW::AC1021, &textBody));
    CHECK(textBody.data().empty());

    source = DRW_MLeaderStyle();
    source.contentType = 40000;
    dwgBufferW wideBody;
    REQUIRE(DrwObjectEncodeTestAccess::encodeMLeaderStyle(
        source, DRW::AC1021, &wideBody));
    dwgBuffer wideReader(wideBody.data().data(), wideBody.data().size());
    CHECK(wideReader.getBitShort() == 40000);

    source.firstSegmentAngle = std::numeric_limits<double>::quiet_NaN();
    const auto wideBytes = snapshot(wideBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLeaderStyle(
        source, DRW::AC1021, &wideBody));
    CHECK(wideBody.data() == wideBytes);

    source.firstSegmentAngle = 0.0;
    source.blockScale.z = std::numeric_limits<double>::infinity();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMLeaderStyle(
        source, DRW::AC1021, &wideBody));
    CHECK(wideBody.data() == wideBytes);
}

TEST_CASE("DRW_MLeaderStyle rejects non-finite DWG values",
          "[dwg-read][object-encode][mleaderstyle][safety]") {
    const DRW::Version version = DRW::AC1021;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0,
                       /*handle=*/0x430u, /*numReactors=*/0,
                       /*xDictFlag=*/1);

    encoded.putBitShort(2); // content type
    encoded.putBitShort(0); // MLEADER draw order
    encoded.putBitShort(0); // leader draw order
    encoded.putBitLong(0);  // max leader points
    encoded.putBitDouble(nan);
    encoded.putBitDouble(0.0);
    encoded.putBitShort(1); // leader type
    encoded.putCmColor(version, 0);
    encoded.putBitLong(0);  // leader line weight
    encoded.putBit(1);      // landing enabled
    encoded.putBitDouble(0.0);
    encoded.putBit(1);      // auto include landing
    encoded.putBitDouble(0.0);
    encoded.putVariableText(version, "");
    encoded.putBitDouble(0.0);
    encoded.putVariableText(version, "");
    encoded.putBitShort(0); // left attachment
    encoded.putBitShort(0); // right attachment
    encoded.putBitShort(0); // text angle type
    encoded.putBitShort(0); // text alignment type
    encoded.putCmColor(version, 0);
    encoded.putBitDouble(0.0);
    encoded.putBit(0);      // text frame enabled
    encoded.putBit(0);      // always align text left
    encoded.putBitDouble(0.0);
    encoded.putCmColor(version, 0);
    encoded.put3BitDouble(DRW_Coord{1.0, 1.0, 1.0});
    encoded.putBit(0);      // block scale enabled
    encoded.putBitDouble(0.0);
    encoded.putBit(0);      // block rotation enabled
    encoded.putBitShort(0); // block connection type
    encoded.putBitDouble(1.0);
    encoded.putBit(0);      // property changed
    encoded.putBit(0);      // annotative
    encoded.putBitDouble(0.0);

    const std::uint32_t bodyEndBit = encoded.bitCount();
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0, {}, /*xDictFlag=*/1);
    for (int i = 0; i < 4; ++i)
        encoded.putHandle(nullHandle());

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_MLeaderStyle rejected;
    rejected.description = "stale description";
    rejected.firstSegmentAngle = 1.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.description.empty());
    CHECK(rejected.firstSegmentAngle == 0.0);
}

TEST_CASE("DRW_LType stages dash data and common handles",
          "[dwg-read][object-encode][ltype]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x430,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "Dashed");
    encoded.putBit(0);                 // referenced
    encoded.putBitShort(0);            // xref index
    encoded.putBit(0);                 // xref dependent
    encoded.putVariableText(ver, "three dashes");
    encoded.putBitDouble(6.0);
    encoded.putRawChar8('A');
    encoded.putRawChar8(3);
    for (double dash : {1.0, -2.0, 3.0}) {
        encoded.putBitDouble(dash);
        encoded.putBitShort(0);
        encoded.putRawDouble(0.0);
        encoded.putRawDouble(0.0);
        encoded.putBitDouble(0.0);
        encoded.putBitDouble(0.0);
        encoded.putBitShort(0);
    }
    std::vector<std::uint8_t> stringArea(256, 0);
    encoded.putBytes(stringArea.data(), stringArea.size());
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x40,
                           /*reactorHandles=*/{0x41}, /*xDictFlag=*/0);
    encoded.putHandle(handleWithCode(5, 0x44)); // external-reference block
    encoded.putHandle(handleWithCode(5, 0x51)); // dash 0 style
    encoded.putHandle(handleWithCode(5, 0x52)); // dash 1 style
    encoded.putHandle(handleWithCode(5, 0x53)); // dash 2 style

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LType parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.name == "Dashed");
    CHECK(parsed.desc == "three dashes");
    CHECK(parsed.size == 3);
    REQUIRE(parsed.path == std::vector<double>{1.0, -2.0, 3.0});
    CHECK(parsed.parentHandle == 0x40u);
    REQUIRE(parsed.reactorHandles == std::vector<std::uint32_t>{0x41u});
    CHECK(parsed.xrefBlockHandle.ref == 0x44u);
    REQUIRE(parsed.shapeHandles == std::vector<std::uint32_t>{0x51u, 0x52u,
                                                               0x53u});

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver,
                                                   &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.path.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
}

TEST_CASE("DRW_LType preserves complex segments and legacy text area",
          "[dwg-read][dwg-write][object-encode][ltype]") {
    const DRW::Version version = DRW::AC1015;
    DRW_LType source;
    source.handle = 0x445u;
    source.name = "ComplexLegacy";
    source.desc = "complex legacy";
    source.size = 3;
    source.path = {1.0, -0.5, 0.25};
    source.length = 1.75;
    source.segments = {
        DRW_LTypeSegment{1.0, 0, {}, 0.0, 0.0, 1.0, 0.0, 0, {}},
        DRW_LTypeSegment{-.5, 17, hardPtr(0x561u), 1.5, -2.0, 2.0,
                          M_PI / 3.0, 4, {}},
        DRW_LTypeSegment{.25, 0, hardPtr(0x562u), -.5, .75, .5,
                         -M_PI / 4.0, 2, "TXT"}};

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x39,
                       source.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeLtype(
        source, version, &encoded));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LType parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseLtype(parsed, version, &reader));
    REQUIRE(parsed.segments.size() == 3u);
    CHECK(parsed.alignment == 'A');
    CHECK(parsed.segments[1].complexShapeCode == 17);
    CHECK(parsed.segments[1].shapeFlags == 4);
    CHECK(parsed.segments[1].styleHandle.ref == 0x561u);
    CHECK(parsed.segments[1].xOffset == Approx(1.5));
    CHECK(parsed.segments[1].yOffset == Approx(-2.0));
    CHECK(parsed.segments[1].scale == Approx(2.0));
    CHECK(parsed.segments[1].rotation == Approx(M_PI / 3.0));
    CHECK(parsed.segments[2].shapeFlags == 2);
    CHECK(parsed.segments[2].styleHandle.ref == 0x562u);
    CHECK(parsed.segments[2].text == "TXT");
}

TEST_CASE("DRW_LType preserves complex text in the R2007 string area",
          "[dwg-read][dwg-write][object-encode][ltype][ac1021]") {
    DRW_LType source;
    source.handle = 0x446u;
    source.name = "ComplexModern";
    source.desc = "modern ltype";
    source.size = 1;
    source.path = {1.0};
    source.length = 1.0;
    source.segments = {DRW_LTypeSegment{1.0, 0, hardPtr(0x563u),
                                        0.0, 0.0, 1.0, 0.0, 2, "TéXT"}};

    const auto bytes = emitAc1021LtypeObject(source);
    REQUIRE_FALSE(bytes.empty());
    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1021, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(const_cast<std::uint8_t*>(bytes.data()), bytes.size(),
                     &decoder);
    DRW_LType parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseLtype(
        parsed, DRW::AC1021, &reader));
    REQUIRE(parsed.segments.size() == 1u);
    CHECK(parsed.segments.front().text == "TéXT");
    CHECK(parsed.segments.front().styleHandle.ref == 0x563u);
}

TEST_CASE("DRW_LType encodes the complete table handle tail",
          "[dwg-write][object-encode][ltype]") {
    const DRW::Version version = DRW::AC1015;
    DRW_LType source;
    source.handle = 0x440u;
    source.name = "Encoded";
    source.desc = "encoded linetype";
    source.size = 2;
    source.length = 3.0;
    source.path = {1.0, -2.0};
    source.shapeHandles = {0x551u, 0x552u};
    source.xrefBlockHandle = handleWithCode(5, 0x553u);
    source.reactorHandles = {0x554u};
    source.setDwgCommonObjectState(1, 0, false);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0,
                       source.handle, /*numReactors=*/1,
                       /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeLtype(source, version, &encoded));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LType parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseLtype(parsed, version, &reader));
    CHECK(parsed.xrefBlockHandle.ref == 0x553u);
    REQUIRE(parsed.shapeHandles == std::vector<std::uint32_t>{0x551u, 0x552u});
    REQUIRE(parsed.reactorHandles == std::vector<std::uint32_t>{0x554u});
}

TEST_CASE("DRW_LType rejects a body ending before dash length",
          "[dwg-read][object-encode][ltype][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0,
                       /*handle=*/0x435);
    encoded.putVariableText(version, "ShortLType");
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.putVariableText(version, "description");
    const std::uint32_t descriptionEndBit = encoded.bitCount();
    encoded.putBitDouble(1.0);     // length
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded),
                                descriptionEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LType rejected;
    rejected.name = "stale-name";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
}

TEST_CASE("DRW_LType rejects non-finite DWG values",
          "[dwg-read][object-encode][ltype][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0,
                       /*handle=*/0x432u);
    encoded.putVariableText(version, "NonFiniteLType");
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.putVariableText(version, "description");
    encoded.putBitDouble(nan);
    encoded.putRawChar8('A');
    encoded.putRawChar8(0);        // no dash entries
    std::vector<std::uint8_t> stringArea(256, 0);
    encoded.putBytes(stringArea.data(), stringArea.size());

    const std::uint32_t bodyEndBit = encoded.bitCount();
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);
    encoded.putHandle(nullHandle()); // LTYPE control owner
    encoded.putHandle(nullHandle()); // xdictionary
    encoded.putHandle(nullHandle()); // xref block

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LType rejected;
    rejected.name = "stale-ltype";
    rejected.length = 1.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseLtype(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
    CHECK(rejected.length == 0.0);
    CHECK(rejected.path.empty());
}

TEST_CASE("DRW_Layer stages color and handle references",
          "[dwg-read][object-encode][layer]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0x33, /*handle=*/0x440);
    encoded.putVariableText(ver, "LayerA");
    encoded.putBit(0);                 // referenced
    encoded.putBitShort(0);            // xref index
    encoded.putBit(0);                 // xref dependent
    encoded.putSBitShort(0x10);        // plotted, default lineweight
    encoded.putCmColor(ver, 3);
    encoded.putHandle(handleWithCode(4, 0x40)); // layer control soft pointer
    encoded.putHandle(nullHandle());   // extension dictionary
    encoded.putHandle(handleWithCode(5, 0x41)); // external-reference block
    encoded.putHandle(handleWithCode(5, 0x42)); // plot style
    encoded.putHandle(handleWithCode(5, 0x55)); // linetype

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Layer parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.name == "LayerA");
    CHECK(parsed.color == 3);
    CHECK(parsed.plotF);
    CHECK(parsed.parentHandle == 0x40u);
    CHECK(parsed.xDictHandle == 0u);
    CHECK(parsed.xrefBlockHandle.ref == 0x41u);
    CHECK(parsed.plotStyleHandle.ref == 0x42u);
    CHECK(parsed.lTypeH.ref == 0x55u);

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver,
                                                   &truncatedReader));
    CHECK(parsed.name.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.lTypeH.ref == 0u);
    CHECK(parsed.handlePlotS.empty());
}

TEST_CASE("DRW_Layer preserves AC1027 handle tail",
          "[dwg-read][dwg-write][object-encode][layer]") {
    DRW_Layer source;
    source.handle = 0x448;
    source.name = "ModernLayer";
    source.color = 7;
    source.lineType = "CONTINUOUS";
    source.plotF = true;
    source.xrefBlockHandle = handleWithCode(5, 0x449);
    source.plotStyleHandle = handleWithCode(5, 0x44A);
    source.materialHandle = handleWithCode(5, 0x44B);
    source.lTypeH = handleWithCode(5, 0x44C);
    source.unknownHandle = handleWithCode(5, 0x44D);
    source.reactorHandles = {0x44E};
    DrwObjectEncodeTestAccess::setNumReactors(source, 1);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    std::uint32_t handleBits = 0;
    const auto bytes = emitAc1027LayerObject(source, handleBits);
    REQUIRE_FALSE(bytes.empty());
    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1027, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(const_cast<std::uint8_t *>(bytes.data()), bytes.size(),
                     &decoder);
    DRW_Layer parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1027, &reader, handleBits));
    CHECK(parsed.name == source.name);
    CHECK(parsed.parentHandle == 0x02u);
    REQUIRE(parsed.reactorHandles.size() == 1);
    CHECK(parsed.reactorHandles.front() == 0x44Eu);
    CHECK(parsed.xrefBlockHandle.ref == 0x449u);
    CHECK(parsed.plotStyleHandle.ref == 0x44Au);
    CHECK(parsed.materialHandle.ref == 0x44Bu);
    CHECK(parsed.lTypeH.ref == 0x44Cu);
    CHECK(parsed.unknownHandle.ref == 0x44Du);
}

TEST_CASE("DRW_Layer preserves off state and special color indices",
          "[dwg-write][object-encode][layer]") {
    for (const DRW::Version version : {DRW::AC1012, DRW::AC1015,
                                       DRW::AC1018}) {
        DRW_Layer source;
        source.name = "OffLayer";
        source.color = -3;
        source.plotF = true;

        dwgBufferW encoded;
        if (version < DRW::AC1015) {
            emitLegacyObjectPreamble(encoded, version, /*oType=*/0x33,
                                     /*handle=*/0x446);
        } else {
            emitObjectPreamble(encoded, version, /*oType=*/0x33,
                               /*handle=*/0x446);
        }
        REQUIRE(DrwObjectEncodeTestAccess::encodeLayer(
            source, version, &encoded));

        auto bytes = snapshot(encoded);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_Layer parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseLayer(
            parsed, version, &reader));
        CHECK(parsed.color == -3);
        CHECK(parsed.name == source.name);
        CHECK(parsed.plotF);
    }

    DRW_Layer byBlock;
    byBlock.color = DRW::ColorByBlock;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, DRW::AC1018, /*oType=*/0x33,
                       /*handle=*/0x447);
    REQUIRE(DrwObjectEncodeTestAccess::encodeLayer(
        byBlock, DRW::AC1018, &encoded));
    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Layer parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseLayer(
        parsed, DRW::AC1018, &reader));
    CHECK(parsed.color == DRW::ColorByBlock);
}

TEST_CASE("DRW_Layer rejects unrepresentable color indices",
          "[dwg-write][object-encode][layer][safety]") {
    for (const int color : {-257, 257, std::numeric_limits<int>::min()}) {
        DRW_Layer source;
        source.color = color;
        dwgBufferW seeded;
        seeded.putRawChar8(0xA5);
        const auto before = snapshot(seeded);
        CHECK_FALSE(DrwObjectEncodeTestAccess::encodeLayer(
            source, DRW::AC1018, &seeded));
        CHECK(snapshot(seeded) == before);
    }
}

TEST_CASE("DRW_Layer rejects a body ending before xref flags",
          "[dwg-read][object-encode][layer][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x33,
                       /*handle=*/0x445);
    encoded.putVariableText(version, "ShortLayer");
    const std::uint32_t nameEndBit = encoded.bitCount();
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), nameEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Layer rejected;
    rejected.name = "stale-name";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
}

TEST_CASE("DRW_SpatialIndex::parseDwg captures timestamps",
          "[dwg-read][object-encode][spatialindex]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x400);
    w.putBitLong(0xAAAA);
    w.putBitLong(0xBBBB);
    w.patchRawLong32AtBit(objectSizeBitOffset(w), w.bitCount());
    emitCommonHandlePrefix(w, /*parentHandle=*/0, {}, /*xDictFlag=*/0);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_SpatialIndex dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.timestamp1 == 0xAAAAu);
    REQUIRE(dst.timestamp2 == 0xBBBBu);
}

// DICTIONARY encoder round-trip (ODA §20.4.44).  Encodes into a buffer via
// DRW_Dictionary::encodeDwg, parses it back, asserts every field.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Dictionary::encodeDwg round-trips entries + cloning",
          "[dwg-write][object-encode][dictionary]") {
    DRW_Dictionary src;
    src.handle       = 0x500;
    src.parentHandle = 0xC;          // Named-objects dictionary
    src.cloning      = 1;
    src.hardOwner    = 1;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic
    src.m_entries.push_back({"ACAD_LAYOUT",       0x1A});
    src.m_entries.push_back({"ACAD_MLINESTYLE",   0x17});
    src.m_entries.push_back({"ACAD_PLOTSETTINGS", 0x19});

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/42 /* DICTIONARY */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionary(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Dictionary dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.cloning    == 1);
    REQUIRE(dst.hardOwner  == 1);
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0xCu);
    REQUIRE(dst.m_entries.size() == 3u);
    REQUIRE(dst.m_entries[0].m_name   == "ACAD_LAYOUT");
    REQUIRE(dst.m_entries[0].m_handle == 0x1Au);
    REQUIRE(dst.m_entries[1].m_name   == "ACAD_MLINESTYLE");
    REQUIRE(dst.m_entries[1].m_handle == 0x17u);
    REQUIRE(dst.m_entries[2].m_name   == "ACAD_PLOTSETTINGS");
    REQUIRE(dst.m_entries[2].m_handle == 0x19u);

    dwgBufferW body;
    dwgBufferW handles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionary(
        src, ver, &body, nullptr, &handles));
    auto handleBytes = snapshot(handles);
    dwgBuffer hardOwnerReader(handleBytes.data(), handleBytes.size());
    CHECK(hardOwnerReader.getHandle().code == DRW::DwgSoftPointer);
    CHECK(hardOwnerReader.getHandle().code == DRW::DwgHardOwnership);
    CHECK(hardOwnerReader.getHandle().code == DRW::DwgHardOwnership);
    CHECK(hardOwnerReader.getHandle().code == DRW::DwgHardOwnership);

    src.hardOwner = 0;
    dwgBufferW softOwnerBody;
    dwgBufferW softOwnerHandles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionary(
        src, ver, &softOwnerBody, nullptr, &softOwnerHandles));
    handleBytes = snapshot(softOwnerHandles);
    dwgBuffer softOwnerReader(handleBytes.data(), handleBytes.size());
    CHECK(softOwnerReader.getHandle().code == DRW::DwgSoftPointer);
    CHECK(softOwnerReader.getHandle().code == DRW::DwgSoftOwnership);
    CHECK(softOwnerReader.getHandle().code == DRW::DwgSoftOwnership);
    CHECK(softOwnerReader.getHandle().code == DRW::DwgSoftOwnership);
}

TEST_CASE("DRW_Dictionary rejects invalid entry pairs and truncated frames",
          "[dwg][safety][object-encode][dictionary]") {
    DRW_Dictionary src;
    src.handle       = 0x501;
    src.parentHandle = 0xC;
    src.cloning      = 1;
    src.hardOwner    = 1;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);
    src.m_entries.push_back({"VALID", 0x2A});
    src.m_entries.push_back({"", 0x2B});
    src.m_entries.push_back({"NULL_HANDLE", 0});

    DRW::Version ver = DRW::AC1018;
    dwgBufferW invalid;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDictionary(src, ver, &invalid));
    CHECK(snapshot(invalid).empty());

    src.m_entries = {{"VALID", 0x2A}};
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/42 /* DICTIONARY */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionary(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Dictionary dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_entries.size() == 1u);
    CHECK(dst.m_entries[0].m_name == "VALID");
    CHECK(dst.m_entries[0].m_handle == 0x2Au);

    auto truncatedBytes = bytes;
    REQUIRE(truncatedBytes.size() > 8);
    truncatedBytes.resize(truncatedBytes.size() - 8);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    DRW_Dictionary truncated;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        truncated, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncated.m_entries.empty());
}

TEST_CASE("DRW_Dictionary rejects R14 data beyond its object boundary",
          "[dwg-read][object-encode][dictionary][framing]") {
    const DRW::Version version = DRW::AC1014;
    dwgBufferW encoded;
    encoded.putObjType(version, /*oType=*/42);
    encoded.putHandle(hardPtr(0x504));
    encoded.putBitShort(0); // EED size = 0
    const std::size_t objectSizeBit = encoded.bitCount();
    encoded.putRawLong32(0); // R13/R14 object-data end in bits
    encoded.putBitLong(0);   // no reactors
    encoded.putBitLong(1);   // one dictionary item
    encoded.putRawChar8(0);  // hard-owner flag
    const std::uint32_t objectSize = encoded.bitCount();

    // This name is deliberately outside the declared object-data range. A
    // parser that ignores the R14 boundary can consume it and only fail later
    // while trying to reinterpret the wrong cursor as the handle stream.
    encoded.putVariableText(version, "OUT-OF-BODY");
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x44,
                           /*reactorHandles=*/{}, /*xDictFlag=*/0);
    encoded.putHandle(hardPtr(0x505));
    const std::uint8_t objectSizeBytes[] = {
        static_cast<std::uint8_t>(objectSize & 0xFFu),
        static_cast<std::uint8_t>((objectSize >> 8) & 0xFFu),
        static_cast<std::uint8_t>((objectSize >> 16) & 0xFFu),
        static_cast<std::uint8_t>((objectSize >> 24) & 0xFFu)};
    REQUIRE(encoded.patchRawBytesAtBit(objectSizeBit, objectSizeBytes,
                                       sizeof(objectSizeBytes)));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Dictionary rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.handle == 0u);
    CHECK(rejected.m_entries.empty());
}

TEST_CASE("DRW_Dictionary rejects a body that crosses its AC1027 boundary",
          "[dwg-read][object-encode][dictionary][safety]") {
    const DRW::Version version = DRW::AC1027;
    DRW_Dictionary source;
    source.handle = 0x503;
    source.parentHandle = 0xC;
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/42, source.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    const std::uint32_t bodyBoundary = encoded.bitCount();
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionary(
        source, version, &encoded));
    auto bytes = snapshot(encoded);
    REQUIRE(bytes.size() * 8u > bodyBoundary);

    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Dictionary rejected;
    const std::uint32_t handleBits = static_cast<std::uint32_t>(
        bytes.size() * 8u - bodyBoundary);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &reader, handleBits));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.handle == 0u);
    CHECK(rejected.m_entries.empty());
}

TEST_CASE("DRW_Dictionary count diagnostics do not affect encoding",
          "[dwg-write][object-encode][dictionary][count-cap]") {
    DRW_Dictionary source;
    source.handle = 0x502;
    source.parentHandle = 0xC;
    source.cloning = 1;
    source.hardOwner = 1;
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);
    source.m_entries.push_back({"VALID", 0x2A});

    const auto encode = [](const DRW_Dictionary &dictionary) {
        dwgBufferW buffer;
        emitObjectPreamble(buffer, DRW::AC1018, /*oType=*/42,
                           dictionary.handle, /*numReactors=*/0,
                           /*xDictFlag=*/1);
        REQUIRE(DrwObjectEncodeTestAccess::encodeDictionary(
            dictionary, DRW::AC1018, &buffer));
        return snapshot(buffer);
    };

    const auto normalBytes = encode(source);
    DRW_Dictionary decodedPrefix = source;
    decodedPrefix.countCap = DRW_DictionaryCountCap{
        3u, 1u, 256u, 5u, 40u};
    CHECK(encode(decodedPrefix) == normalBytes);
}

// SCALE encoder round-trip (ODA §20.4.92). The encoder emits body fields;
// the common handle stream is appended by this test harness.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Scale::encodeDwg round-trips paper/drawing units + unit flag",
          "[dwg-write][object-encode][scale]") {
    DRW_Scale src;
    src.handle       = 0x600;
    src.flag         = 0;
    src.name         = "1:50";
    src.paperUnits   = 1.0;
    src.drawingUnits = 50.0;
    src.isUnitScale  = false;

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeScale(src, ver, &w));
    const std::uint32_t objectSize = w.bitCount();
    emitCommonHandlePrefix(w, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{}, /*xDictFlag=*/0);
    w.patchRawLong32AtBit(2, objectSize);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Scale dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.flag         == 0);
    REQUIRE(dst.name         == "1:50");
    REQUIRE(dst.paperUnits   == Approx(1.0));
    REQUIRE(dst.drawingUnits == Approx(50.0));
    REQUIRE(dst.isUnitScale  == false);
    REQUIRE(dst.scaleFactor() == Approx(50.0));
    CHECK(dst.parentHandle == 0x42);
    CHECK(dst.xDictHandle == 0);
}

TEST_CASE("DRW_Scale rejects a truncated common handle stream",
          "[dwg-read][object-encode][scale][safety]") {
    const DRW::Version ver = DRW::AC1015;

    dwgBufferW valid;
    emitObjectPreamble(valid, ver, /*oType=*/0, /*handle=*/0x601);
    valid.putBitShort(0);
    valid.putVariableText(ver, "1:2");
    valid.putBitDouble(1.0);
    valid.putBitDouble(2.0);
    valid.putBit(0);
    const std::uint32_t objectSize = valid.bitCount();
    emitCommonHandlePrefix(valid, /*parentHandle=*/0x43,
                           /*reactorHandles=*/{}, /*xDictFlag=*/0);
    valid.patchRawLong32AtBit(2, objectSize);

    auto validBytes = snapshot(valid);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_Scale parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.parentHandle == 0x43);

    auto truncatedBytes = validBytes;
    REQUIRE(truncatedBytes.size() > 1);
    truncatedBytes.pop_back();
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.paperUnits == Approx(1.0));
    CHECK(parsed.drawingUnits == Approx(1.0));
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_Scale rejects non-finite units before writing",
          "[dwg-write][object-encode][scale][safety]") {
    DRW_Scale source;
    source.paperUnits = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW data;
    data.putRawChar8(0xA5);
    const auto original = snapshot(data);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeScale(
        source, DRW::AC1015, &data));
    CHECK(data.data() == original);

    source.paperUnits = 1.0;
    source.drawingUnits = std::numeric_limits<double>::infinity();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeScale(
        source, DRW::AC1015, &data));
    CHECK(data.data() == original);
}

TEST_CASE("DRW_Scale rejects non-finite DWG units transactionally",
          "[dwg-read][object-encode][scale][safety]") {
    const DRW::Version ver = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();

    const auto parseMalformed = [&](double paperUnits,
                                    double drawingUnits) {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x604);
        encoded.putBitShort(0);
        encoded.putVariableText(ver, "1:2");
        encoded.putBitDouble(paperUnits);
        encoded.putBitDouble(drawingUnits);
        encoded.putBit(0);
        const std::uint32_t objectSize = encoded.bitCount();
        emitCommonHandlePrefix(encoded, /*parentHandle=*/0x44,
                               /*reactorHandles=*/{}, /*xDictFlag=*/0);
        encoded.patchRawLong32AtBit(2, objectSize);

        auto bytes = snapshot(encoded);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_Scale parsed;
        parsed.name = "stale scale";
        parsed.paperUnits = 9.0;
        parsed.drawingUnits = 8.0;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.name.empty());
        CHECK(parsed.paperUnits == Approx(1.0));
        CHECK(parsed.drawingUnits == Approx(1.0));
        CHECK(parsed.parentHandle == 0u);
    };

    parseMalformed(nan, 2.0);
    parseMalformed(1.0, infinity);
}

TEST_CASE("DRW_Scale honors the R14 object-data handle boundary",
          "[dwg-read][object-encode][scale][framing]") {
    const DRW::Version ver = DRW::AC1014;

    dwgBufferW valid;
    valid.putObjType(ver, /*oType=*/0);
    valid.putHandle(hardPtr(0x602));
    valid.putBitShort(0); // EED size = 0
    const std::size_t objectSizeBit = valid.bitCount();
    valid.putRawLong32(0); // R13/R14 object-data size in bits
    valid.putBitLong(0);   // no reactors
    valid.putBitShort(0);
    valid.putVariableText(ver, "1:2");
    valid.putBitDouble(1.0);
    valid.putBitDouble(2.0);
    valid.putBit(0);
    valid.putBit(1); // body padding intentionally remains before handles
    const std::uint32_t objectSize = valid.bitCount();
    emitCommonHandlePrefix(valid, /*parentHandle=*/0x44,
                           /*reactorHandles=*/{}, /*xDictFlag=*/0);
    const std::uint8_t objectSizeBytes[] = {
        static_cast<std::uint8_t>(objectSize & 0xFFu),
        static_cast<std::uint8_t>((objectSize >> 8) & 0xFFu),
        static_cast<std::uint8_t>((objectSize >> 16) & 0xFFu),
        static_cast<std::uint8_t>((objectSize >> 24) & 0xFFu)};
    REQUIRE(valid.patchRawBytesAtBit(objectSizeBit, objectSizeBytes,
                                     sizeof(objectSizeBytes)));

    auto bytes = snapshot(valid);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Scale parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.name == "1:2");
    CHECK(parsed.paperUnits == Approx(1.0));
    CHECK(parsed.drawingUnits == Approx(2.0));
    CHECK(parsed.parentHandle == 0x44u);
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("DRW_Scale enforces the AC1024 body boundary",
          "[dwg-read][dwg-write][object-encode][scale][safety]") {
    const DRW::Version ver = DRW::AC1024;
    DRW_Scale source;
    source.handle = 0x603u;
    source.flag = 0;
    source.name = "1:25";
    source.paperUnits = 1.0;
    source.drawingUnits = 25.0;
    source.isUnitScale = false;
    source.parentHandle = 0x42u;
    source.setDwgCommonObjectState(0, 0, false);

    dwgBufferW data;
    dwgBufferW strings;
    emitObjectPreamble(data, ver, DRW_Scale::kDwgClassNum, source.handle,
                       source.reactorCount(), source.extensionDictionaryFlag());
    REQUIRE(DrwObjectEncodeTestAccess::encodeScaleModern(
        source, ver, &data, &strings));
    const std::uint32_t bodyFieldEndBit = data.bitCount();

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    REQUIRE(stringBytes > 0u);
    data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    REQUIRE(stringBitSize <= std::numeric_limits<std::uint16_t>::max());
    data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    data.putBit(1); // detached strings are present
    data.alignToByte();
    const std::uint32_t bodyEndBit = data.bitCount();

    dwgBufferW handles;
    emitCommonHandlePrefix(handles, source.parentHandle, {}, 0);
    handles.alignToByte();
    const std::uint32_t handleBits =
        static_cast<std::uint32_t>(handles.data().size() * 8u);
    auto bytes = snapshot(data);
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());

    DRW_TextCodec decoder;
    decoder.setVersion(ver, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer validReader(bytes.data(), bytes.size(), &decoder);
    DRW_Scale valid;
    REQUIRE(DrwObjectEncodeTestAccess::parse(valid, ver, &validReader,
                                             handleBits));
    CHECK(valid.name == source.name);
    CHECK(valid.paperUnits == Approx(source.paperUnits));
    CHECK(valid.drawingUnits == Approx(source.drawingUnits));
    CHECK(valid.parentHandle == source.parentHandle);

    REQUIRE(bodyEndBit >= bodyFieldEndBit);
    REQUIRE(handleBits <= std::numeric_limits<std::uint32_t>::max()
            - (bodyEndBit - bodyFieldEndBit) - 1u);
    const std::uint32_t malformedHandleBits =
        handleBits + (bodyEndBit - bodyFieldEndBit) + 1u;
    dwgBuffer malformedReader(bytes.data(), bytes.size(), &decoder);
    DRW_Scale malformed;
    malformed.name = "stale scale";
    malformed.paperUnits = 9.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        malformed, ver, &malformedReader, malformedHandleBits));
    CHECK_FALSE(malformedReader.isGood());
    CHECK(malformed.name.empty());
    CHECK(malformed.paperUnits == Approx(1.0));
    CHECK(malformed.drawingUnits == Approx(1.0));
    CHECK(malformed.parentHandle == 0u);
}

TEST_CASE("DRW_ObjectContextData validates its framed handle stream",
          "[dwg-read][object-encode][objectcontext][safety]") {
    const DRW::Version ver = DRW::AC1015;

    dwgBufferW valid;
    emitObjectPreamble(valid, ver, /*oType=*/0, /*handle=*/0x701,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    valid.putBitShort(3); // class version
    valid.putBit(1);      // default flag
    valid.putBitShort(0); // horizontal mode
    valid.putBitDouble(0.0);
    valid.putRawDouble(0.0);
    valid.putRawDouble(0.0);
    valid.putRawDouble(0.0);
    valid.putRawDouble(0.0);
    const std::uint32_t objectSize = valid.bitCount();
    valid.putHandle(hardPtr(0x44));
    valid.putHandle(handleWithCode(0xA, 0x45));
    valid.putHandle(handleWithCode(0xC, 0x46));
    valid.putHandle(hardPtr(0x99)); // annotation scale handle
    valid.patchRawLong32AtBit(2, objectSize);

    auto validBytes = snapshot(valid);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_ObjectContextData parsed(
        "TEXTOBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::Text);
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.parentHandle == 0x44);
    CHECK(parsed.m_annotatedHandle == 0x44);
    CHECK(parsed.m_scaleHandle == 0x99);
    REQUIRE(parsed.reactorHandles.size() == 1);
    CHECK(parsed.reactorHandles.front() == 0x45u);
    CHECK(parsed.xDictHandle == 0x46u);

    parsed.m_classVersion = 99;
    parsed.m_leaderPoints.push_back({1.0, 2.0, 3.0});
    parsed.m_scaleHandle = 0xAA;
    parsed.m_annotatedHandle = 0xBB;
    parsed.reactorHandles.push_back(0xCCu);
    auto truncatedBytes = validBytes;
    REQUIRE(truncatedBytes.size() > 3);
    truncatedBytes.resize(truncatedBytes.size() - 3);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK(parsed.m_classVersion == 0);
    CHECK(parsed.m_leaderPoints.empty());
    CHECK(parsed.m_scaleHandle == 0u);
    CHECK(parsed.m_annotatedHandle == 0u);
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
    CHECK_FALSE(truncatedReader.isGood());
}

TEST_CASE("DRW_ObjectContextData rejects a truncated variable-width body field",
          "[dwg-read][object-encode][objectcontext][safety]") {
    const DRW::Version ver = DRW::AC1015;

    dwgBufferW body;
    emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x702);
    body.putBitShort(3); // class version
    body.putBit(1);      // default flag
    body.putBitShort(0); // horizontal mode
    const std::uint32_t rotationStart = body.bitCount();
    body.putBitDouble(42.0); // selector + payload; payload is excluded below
    emitCommonHandlePrefix(body, /*parentHandle=*/0x44, {}, 0);
    body.patchRawLong32AtBit(2, rotationStart + 2);

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ObjectContextData parsed(
        "TEXTOBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::Text);
    parsed.m_classVersion = 99;
    parsed.m_scaleHandle = 0xAA;

    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_classVersion == 0);
    CHECK(parsed.m_scaleHandle == 0u);
    CHECK(parsed.m_insertionPoint.x == Approx(0.0));
}

TEST_CASE("DRW_ObjectContextData rejects non-finite numeric fields",
          "[dwg-read][object-encode][objectcontext][safety]") {
    const DRW::Version ver = DRW::AC1015;

    dwgBufferW body;
    emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x705);
    body.putBitShort(3); // class version
    body.putBit(1);      // default flag
    body.putBitShort(0); // horizontal mode
    body.putBitDouble(std::numeric_limits<double>::quiet_NaN());
    const std::uint32_t objectSize = body.bitCount();
    emitCommonHandlePrefix(body, /*parentHandle=*/0, {}, 0);
    body.patchRawLong32AtBit(2, objectSize);

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ObjectContextData parsed(
        "TEXTOBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::Text);
    parsed.m_classVersion = 99;
    parsed.m_rotation = 42.0;

    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_classVersion == 0);
    CHECK(parsed.m_rotation == Approx(0.0));
}

TEST_CASE("DRW_ObjectContextData rejects a truncated optional handle",
          "[dwg-read][object-encode][objectcontext][safety]") {
    const DRW::Version ver = DRW::AC1015;

    dwgBufferW body;
    emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x703,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    body.putBitShort(3); // class version
    body.putBit(1);      // default flag
    body.putBitShort(0); // horizontal mode
    body.putBitDouble(0.0);
    body.putRawDouble(0.0);
    body.putRawDouble(0.0);
    body.putRawDouble(0.0);
    body.putRawDouble(0.0);
    const std::uint32_t objectSize = body.bitCount();
    body.putHandle(hardPtr(0x44));
    body.putHandle(handleWithCode(0xA, 0x45));
    body.putHandle(handleWithCode(0xC, 0x46));
    body.putRawChar8(0x41); // one-byte handle header, missing its payload
    body.patchRawLong32AtBit(2, objectSize);

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ObjectContextData parsed(
        "TEXTOBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::Text);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseObjectContextData(
        parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_scaleHandle == 0u);
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_ObjectContextData decodes MText column data",
          "[dwg-read][object-encode][objectcontext][mtext]") {
    const DRW::Version ver = DRW::AC1018;

    dwgBufferW body;
    body.putObjType(ver, /*oType=*/0);
    const std::size_t objectSizeBit = body.bitCount();
    body.putRawLong32(0); // object-size-in-bits placeholder
    body.putHandle(hardPtr(0x702));
    body.putBitShort(0);  // empty EED
    body.putBitLong(0);   // no reactors
    body.putBit(0);       // no xdictionary
    body.putBitShort(4); // class version
    body.putBit(0);      // default flag
    body.putBitLong(5);  // attachment
    body.put3BitDouble({1.0, 0.0, 0.0}); // x-axis direction
    body.put3BitDouble({10.0, 20.0, 0.0}); // insertion point
    body.putBitDouble(80.0); // rectangle width
    body.putBitDouble(30.0); // rectangle height
    body.putBitDouble(75.0); // extents width
    body.putBitDouble(25.0); // extents height
    body.putBitLong(2);      // height-based columns
    body.putBitLong(2);      // column height count
    body.putBitDouble(40.0); // column width
    body.putBitDouble(5.0);  // column gutter
    body.putBit(0);          // automatic height
    body.putBit(1);          // flow reversed
    body.putBitDouble(12.5);
    body.putBitDouble(25.0);
    const std::uint32_t objectSize = body.bitCount();
    emitCommonHandlePrefix(body, /*parentHandle=*/0x44, {}, 0);
    body.putHandle(hardPtr(0x99)); // annotation scale handle
    body.patchRawLong32AtBit(objectSizeBit, objectSize);

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ObjectContextData parsed(
        "MTEXTOBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::MText);
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_classVersion == 4);
    CHECK(parsed.m_attachment == 5);
    CHECK(parsed.m_insertionPoint.x == Approx(10.0));
    CHECK(parsed.m_insertionPoint.y == Approx(20.0));
    CHECK(parsed.m_columnType == 2);
    CHECK(parsed.m_columnHeightCount == 2);
    CHECK(parsed.m_fragmentCount == 0);
    CHECK(parsed.m_columnWidth == Approx(40.0));
    CHECK(parsed.m_columnGutter == Approx(5.0));
    CHECK_FALSE(parsed.m_columnAutoHeight);
    CHECK(parsed.m_columnFlowReversed);
    REQUIRE(parsed.m_columnHeights.size() == 2);
    CHECK(parsed.m_columnHeights[0] == Approx(12.5));
    CHECK(parsed.m_columnHeights[1] == Approx(25.0));
    CHECK(parsed.parentHandle == 0x44);
    CHECK(parsed.m_scaleHandle == 0x99);
}

TEST_CASE("DRW_ObjectContextData bounds MText column counts",
          "[dwg-read][object-encode][objectcontext][mtext][safety]") {
    const DRW::Version ver = DRW::AC1018;

    dwgBufferW body;
    body.putObjType(ver, /*oType=*/0);
    const std::size_t objectSizeBit = body.bitCount();
    body.putRawLong32(0); // object-size-in-bits placeholder
    body.putHandle(hardPtr(0x703));
    body.putBitShort(0);  // empty EED
    body.putBitLong(0);   // no reactors
    body.putBit(0);       // no xdictionary
    body.putBitShort(4);  // class version
    body.putBit(0);       // default flag
    body.putBitLong(5);   // attachment
    body.put3BitDouble({1.0, 0.0, 0.0});
    body.put3BitDouble({10.0, 20.0, 0.0});
    body.putBitDouble(80.0);
    body.putBitDouble(30.0);
    body.putBitDouble(75.0);
    body.putBitDouble(25.0);
    body.putBitLong(2); // height-based columns
    body.putBitLong(std::numeric_limits<std::int32_t>::max());
    body.putBitDouble(40.0);
    body.putBitDouble(5.0);
    body.putBit(1); // automatic height: no height list follows
    body.putBit(0);
    const std::uint32_t objectSize = body.bitCount();
    emitCommonHandlePrefix(body, /*parentHandle=*/0x44, {}, 0);
    body.putHandle(hardPtr(0x9A));
    body.patchRawLong32AtBit(objectSizeBit, objectSize);

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ObjectContextData parsed(
        "MTEXTOBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::MText);
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_columnHeightCount < std::numeric_limits<std::int32_t>::max());
    CHECK(parsed.m_columnHeights.empty());
    CHECK(parsed.m_columnAutoHeight);
    CHECK(parsed.m_scaleHandle == 0x9A);
}

TEST_CASE("DRW_ObjectContextData decodes MText attribute embedded scale",
          "[dwg-read][object-encode][objectcontext][mtext][scale]") {
    const DRW::Version ver = DRW::AC1018;

    dwgBufferW body;
    body.putObjType(ver, /*oType=*/0);
    const std::size_t objectSizeBit = body.bitCount();
    body.putRawLong32(0); // object-size-in-bits placeholder
    body.putHandle(hardPtr(0x704));
    body.putBitShort(0);  // empty EED
    body.putBitLong(0);   // no reactors
    body.putBit(0);       // no xdictionary
    body.putBitShort(6);  // class version
    body.putBit(1);       // default flag
    body.putBitShort(2);  // horizontal mode
    body.putBitDouble(0.25);
    body.putRawDouble(1.0);
    body.putRawDouble(2.0);
    body.putRawDouble(3.0);
    body.putRawDouble(4.0);
    body.putBit(1);       // embedded scale enabled
    body.putBitShort(0);  // embedded SCALE flag
    body.putVariableText(ver, "1:10");
    body.putBitDouble(1.0);
    body.putBitDouble(10.0);
    body.putBit(1);       // unit scale
    const std::uint32_t objectSize = body.bitCount();
    emitCommonHandlePrefix(body, /*parentHandle=*/0x44, {}, 0);
    body.putHandle(hardPtr(0x9B)); // annotation scale handle
    body.patchRawLong32AtBit(objectSizeBit, objectSize);

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ObjectContextData parsed(
        "MTEXTATTRIBUTEOBJECTCONTEXTDATA",
        DRW_ObjectContextData::Kind::MTextAttribute);
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_classVersion == 6);
    CHECK(parsed.m_defaultFlag);
    CHECK(parsed.m_horizontalMode == 2);
    CHECK(parsed.m_rotation == Approx(0.25));
    CHECK(parsed.m_insertionPoint.x == Approx(1.0));
    CHECK(parsed.m_alignmentPoint.y == Approx(4.0));
    CHECK(parsed.m_embeddedScaleEnabled);
    CHECK(parsed.m_embeddedScaleFlag == 0);
    CHECK(parsed.m_embeddedScaleName == "1:10");
    CHECK(parsed.m_embeddedScalePaperUnits == Approx(1.0));
    CHECK(parsed.m_embeddedScaleDrawingUnits == Approx(10.0));
    CHECK(parsed.m_embeddedScaleUnitScale);
    CHECK(parsed.m_scaleHandle == 0x9B);
}

TEST_CASE("DWG object dispatch rejects a malformed known context",
          "[dwg-read][objectcontext][dispatch][safety]") {
    DRW_Class contextClass;
    contextClass.recName = "TEXTOBJECTCONTEXTDATA";
    contextClass.className = "AcDbTextObjectContextData";
    contextClass.entityFlag = 0;

    auto malformed = makeObjectContextFrame(/*includeHandles=*/false);
    DispatchReader malformedReader(malformed.data(), malformed.size());
    malformedReader.addClass(501, contextClass);
    dwgBuffer malformedBuffer(malformed.data(), malformed.size());
    objHandle malformedObject(501, 0x55, 0);
    DispatchCapture malformedCapture;
    CHECK_FALSE(malformedReader.parseObject(
        &malformedBuffer, malformedObject, malformedCapture));
    CHECK(malformedCapture.objectContextCallbacks == 0);
    CHECK(malformedCapture.rawObjectCallbacks == 0);

    auto valid = makeObjectContextFrame(/*includeHandles=*/true);
    DispatchReader validReader(valid.data(), valid.size());
    validReader.addClass(501, contextClass);
    dwgBuffer validBuffer(valid.data(), valid.size());
    objHandle validObject(501, 0x55, 0);
    DispatchCapture validCapture;
    REQUIRE(validReader.parseObject(&validBuffer, validObject, validCapture));
    CHECK(validCapture.objectContextCallbacks == 1);
    CHECK(validCapture.rawObjectCallbacks == 1);
}

TEST_CASE("DWG entity dispatch rejects duplicate deferred handles",
          "[dwg-read][ownership][dispatch][safety]") {
    auto body = emitAc1018DbColorObject();
    REQUIRE(!body.empty());
    auto frame = makeObjectFrame(body);

    DispatchReader reader(frame.data(), frame.size());
    dwgBuffer buffer(frame.data(), frame.size());
    objHandle object(DRW_DbColor::kDwgType, 0x660, 0);
    DispatchCapture capture;

    REQUIRE(reader.readDwgEntity(&buffer, object, capture));
    CHECK(reader.objObjectMap.size() == 1);
    CHECK_FALSE(reader.readDwgEntity(&buffer, object, capture));
    CHECK(reader.objObjectMap.size() == 1);
    CHECK(capture.rawObjectCallbacks == 0);
}

TEST_CASE("DWG underlay definitions retain typed and raw dispatch",
          "[dwg][safety][dwg-read][ownership][dispatch][underlaydefinition]") {
    constexpr DRW::Version version = DRW::AC1018;
    dwgBufferW body;
    emitObjectPreamble(body, version,
                       DRW_UnderlayDefinition::kDwgClassNumPdf, 0xA30);
    body.putVariableText(version, "reference.pdf");
    body.putVariableText(version, "Sheet 1");
    emitCommonHandlePrefix(body, 0x42, {}, 0);
    const auto frame = makeObjectFrame(body.data());

    DRW_Class pdfClass;
    pdfClass.recName = "PDFDEFINITION";
    pdfClass.className = "AcDbPdfDefinition";
    pdfClass.entityFlag = 0;

    DispatchReader reader(const_cast<std::uint8_t*>(frame.data()), frame.size());
    reader.addClass(DRW_UnderlayDefinition::kDwgClassNumPdf, pdfClass);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    objHandle object(DRW_UnderlayDefinition::kDwgClassNumPdf, 0xA30, 0);
    DispatchCapture capture;

    REQUIRE(reader.parseObject(&buffer, object, capture));
    CHECK(capture.underlayDefinitionCallbacks == 1);
    CHECK(capture.rawObjectCallbacks == 1);
}

TEST_CASE("DRW_ObjectContextData decodes FCF and MLeader shells",
          "[dwg-read][object-encode][objectcontext][fcf][mleader]") {
    const DRW::Version ver = DRW::AC1015;

    dwgBufferW fcf;
    emitObjectPreamble(fcf, ver, /*oType=*/0, /*handle=*/0x710);
    fcf.putBitShort(5);
    fcf.putBit(1);
    fcf.put3BitDouble({1.0, 2.0, 3.0});
    fcf.put3BitDouble({0.0, 1.0, 0.0});
    const std::uint32_t fcfObjectSize = fcf.bitCount();
    emitCommonHandlePrefix(fcf, /*parentHandle=*/0x44, {}, 0);
    fcf.putHandle(hardPtr(0x99));
    fcf.patchRawLong32AtBit(2, fcfObjectSize);

    auto fcfBytes = snapshot(fcf);
    dwgBuffer fcfReader(fcfBytes.data(), fcfBytes.size());
    DRW_ObjectContextData fcfParsed(
        "FCFOBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::Fcf);
    REQUIRE(DrwObjectEncodeTestAccess::parseObjectContextData(
        fcfParsed, ver, &fcfReader));
    CHECK(fcfParsed.m_classVersion == 5);
    CHECK(fcfParsed.m_defaultFlag);
    CHECK(fcfParsed.m_fcfLocation.x == 1.0);
    CHECK(fcfParsed.m_fcfLocation.y == 2.0);
    CHECK(fcfParsed.m_fcfLocation.z == 3.0);
    CHECK(fcfParsed.m_fcfHorizontalDirection.x == 0.0);
    CHECK(fcfParsed.m_fcfHorizontalDirection.y == 1.0);
    CHECK(fcfParsed.m_fcfHorizontalDirection.z == 0.0);
    CHECK(fcfParsed.parentHandle == 0x44);
    CHECK(fcfParsed.m_scaleHandle == 0x99u);

    dwgBufferW mleader;
    emitObjectPreamble(mleader, ver, /*oType=*/0, /*handle=*/0x711);
    mleader.putBitShort(7);
    mleader.putBit(0);
    const std::uint32_t mleaderObjectSize = mleader.bitCount();
    emitCommonHandlePrefix(mleader, /*parentHandle=*/0x45, {}, 0);
    mleader.putHandle(hardPtr(0x9A));
    mleader.patchRawLong32AtBit(2, mleaderObjectSize);

    auto mleaderBytes = snapshot(mleader);
    dwgBuffer mleaderReader(mleaderBytes.data(), mleaderBytes.size());
    DRW_ObjectContextData mleaderParsed(
        "MLEADEROBJECTCONTEXTDATA", DRW_ObjectContextData::Kind::MLeader);
    REQUIRE(DrwObjectEncodeTestAccess::parseObjectContextData(
        mleaderParsed, ver, &mleaderReader));
    CHECK(mleaderParsed.m_classVersion == 7);
    CHECK_FALSE(mleaderParsed.m_defaultFlag);
    CHECK(mleaderParsed.parentHandle == 0x45);
    CHECK(mleaderParsed.m_scaleHandle == 0x9Au);
}

// VPORT encoder round-trip (P4-06): the per-viewport UCS block (origin / X /
// Y axis / elevation / ortho-type / ucsPerVP) must survive encode->parse
// instead of being written back as a hardcoded identity. AC1015 keeps the
// name string + handle stream inline.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Vport::encodeDwg round-trips per-viewport UCS geometry",
          "[dwg-write][object-encode][vport]") {
    DRW_Vport src;
    src.handle      = 0x500;
    src.name        = "MYVP";
    // Non-identity UCS values to prove they are stored, not hardcoded.
    src.ucsOrigin   = DRW_Coord(3.0, 4.0, 5.0);
    src.ucsXAxis    = DRW_Coord(0.0, 1.0, 0.0);
    src.ucsYAxis    = DRW_Coord(-1.0, 0.0, 0.0);
    src.ucsElevation = 2.0;
    src.ucsOrthoType = 1;
    src.ucsPerVP    = true;
    src.height      = 4.0;
    src.ratio       = 1.5;
    src.viewMode    = 1 | 16;
    src.renderMode = 3;
    src.useDefaultLighting = false;
    src.defaultLightingType = 2;
    src.brightness = 0.25;
    src.contrast = 0.75;
    src.ambientColor = 7;
    src.gridBehavior = 5;
    src.gridMajorLines = 9;
    src.reactorHandles = {0x501};
    src.xDictHandle = 0x502;
    src.xrefBlockHandle = hardPtr(0x503);
    src.backgroundHandle = 0x504;
    src.visualStyleHandle = 0x505;
    src.m_sunHandle = 0x506;
    src.namedUcsHandle = 0x507;
    src.baseUcsHandle = 0x508;
    DrwObjectEncodeTestAccess::setNumReactors(src, 1);

    DRW::Version ver = DRW::AC1015;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0x41 /* VPORT */, src.handle,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeVport(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Vport dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.name == "MYVP");
    REQUIRE(dst.height == Approx(4.0));
    REQUIRE(dst.ratio == Approx(1.5));
    REQUIRE(dst.ucsOrigin.x == Approx(3.0));
    REQUIRE(dst.ucsOrigin.y == Approx(4.0));
    REQUIRE(dst.ucsOrigin.z == Approx(5.0));
    REQUIRE(dst.ucsXAxis.x  == Approx(0.0));
    REQUIRE(dst.ucsXAxis.y  == Approx(1.0));
    REQUIRE(dst.ucsXAxis.z  == Approx(0.0));
    REQUIRE(dst.ucsYAxis.x  == Approx(-1.0));
    REQUIRE(dst.ucsYAxis.y  == Approx(0.0));
    REQUIRE(dst.ucsYAxis.z  == Approx(0.0));
    REQUIRE(dst.ucsElevation == Approx(2.0));
    REQUIRE(dst.ucsOrthoType == 1);
    REQUIRE(dst.ucsPerVP     == true);
    REQUIRE(dst.viewMode      == (1 | 16));
    REQUIRE(dst.renderMode == 3);
    REQUIRE(dst.reactorHandles == std::vector<std::uint32_t>{0x501});
    REQUIRE(dst.xDictHandle == 0x502);
    REQUIRE(dst.xrefBlockHandle.ref == 0x503);
    REQUIRE(dst.namedUcsHandle == 0x507);
    REQUIRE(dst.baseUcsHandle == 0x508);

    auto truncatedBytes = snapshot(w);
    REQUIRE(truncatedBytes.size() > 2);
    truncatedBytes.resize(truncatedBytes.size() - 2);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    dst.name = "STALE";
    dst.reactorHandles = {0x601};
    dst.xDictHandle = 0x602;
    dst.xrefBlockHandle = hardPtr(0x603);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        dst, ver, &truncatedReader));
    CHECK(dst.name.empty());
    CHECK(dst.reactorHandles.empty());
    CHECK(dst.xDictHandle == 0u);
    CHECK(dst.xrefBlockHandle.ref == 0u);
}

TEST_CASE("DRW_Vport AC1021 framing preserves detached strings and handles",
          "[dwg-write][object-encode][vport][ac1021]") {
    DRW_Vport source;
    source.handle = 0x700;
    source.name = "R2007-VPORT";
    source.height = 16.0;
    source.ratio = 1.25;
    source.viewMode = 1 | 16;
    source.renderMode = 3;
    source.useDefaultLighting = false;
    source.defaultLightingType = 2;
    source.brightness = 0.25;
    source.contrast = 0.75;
    source.ambientColor = 7;
    source.ambientColorRgb = 0x123456;
    source.ambientColorName = "AmbientName";
    source.gridBehavior = 5;
    source.gridMajorLines = 9;
    source.ucsOrigin = DRW_Coord(3.0, 4.0, 5.0);
    source.ucsXAxis = DRW_Coord(0.0, 1.0, 0.0);
    source.ucsYAxis = DRW_Coord(-1.0, 0.0, 0.0);
    source.ucsElevation = 2.0;
    source.ucsOrthoType = 1;
    source.ucsPerVP = true;
    source.reactorHandles = {0x701};
    source.xDictHandle = 0x702;
    source.xrefBlockHandle = hardPtr(0x703);
    source.backgroundHandle = 0x704;
    source.visualStyleHandle = 0x705;
    source.m_sunHandle = 0x706;
    source.namedUcsHandle = 0x707;
    source.baseUcsHandle = 0x708;
    DrwObjectEncodeTestAccess::setNumReactors(source, 1);

    auto bytes = emitAc1021VportObject(source);
    REQUIRE_FALSE(bytes.empty());
    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1021, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_Vport parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1021, &reader));
    REQUIRE(parsed.name == source.name);
    REQUIRE(parsed.height == Approx(source.height));
    REQUIRE(parsed.ratio == Approx(source.ratio));
    REQUIRE(parsed.viewMode == source.viewMode);
    REQUIRE(parsed.renderMode == source.renderMode);
    REQUIRE(parsed.useDefaultLighting == source.useDefaultLighting);
    REQUIRE(parsed.defaultLightingType == source.defaultLightingType);
    REQUIRE(parsed.brightness == Approx(source.brightness));
    REQUIRE(parsed.contrast == Approx(source.contrast));
    REQUIRE(parsed.ambientColor == 256u);
    REQUIRE(parsed.ambientColorRgb == source.ambientColorRgb);
    REQUIRE(parsed.ambientColorName == source.ambientColorName);
    REQUIRE(parsed.gridBehavior == source.gridBehavior);
    REQUIRE(parsed.gridMajorLines == source.gridMajorLines);
    REQUIRE(parsed.ucsOrigin.x == Approx(source.ucsOrigin.x));
    REQUIRE(parsed.ucsXAxis.y == Approx(source.ucsXAxis.y));
    REQUIRE(parsed.ucsYAxis.x == Approx(source.ucsYAxis.x));
    REQUIRE(parsed.ucsElevation == Approx(source.ucsElevation));
    REQUIRE(parsed.ucsOrthoType == source.ucsOrthoType);
    REQUIRE(parsed.ucsPerVP == source.ucsPerVP);
    REQUIRE(parsed.reactorHandles == std::vector<std::uint32_t>{0x701});
    REQUIRE(parsed.xDictHandle == 0x702);
    REQUIRE(parsed.xrefBlockHandle.ref == 0x703);
    REQUIRE(parsed.backgroundHandle == 0x704);
    REQUIRE(parsed.visualStyleHandle == 0x705);
    REQUIRE(parsed.m_sunHandle == 0x706);
    REQUIRE(parsed.namedUcsHandle == 0x707);
    REQUIRE(parsed.baseUcsHandle == 0x708);

    auto truncated = bytes;
    REQUIRE(truncated.size() > 2);
    truncated.resize(truncated.size() - 2);
    dwgBuffer truncatedReader(truncated.data(), truncated.size(), &decoder);
    parsed.name = "STALE";
    parsed.reactorHandles = {0x709};
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1021, &truncatedReader));
    CHECK(parsed.name.empty());
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("DRW_Vport uses the version-specific DWG aspect-ratio layout",
          "[dwg-write][object-encode][vport][aspect-ratio]") {
    for (const DRW::Version version : {DRW::AC1009, DRW::AC1012}) {
        DRW_Vport source;
        source.handle = 0x740;
        source.name = "RATIO-VPORT";
        source.height = 2.0;
        source.ratio = 1.5;

        dwgBufferW body;
        emitLegacyObjectPreamble(body, version, /*oType=*/0x41,
                                 source.handle);
        REQUIRE(DrwObjectEncodeTestAccess::encodeVport(
            source, version, &body));

        auto bytes = snapshot(body);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_Vport parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parse(
            parsed, version, &reader));
        CHECK(parsed.height == Approx(source.height));
        CHECK(parsed.ratio == Approx(source.ratio));
    }
}

TEST_CASE("DRW_Vport rejects a body ending before xref flags",
          "[dwg-read][object-encode][vport][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x41,
                       /*handle=*/0x750);
    encoded.putVariableText(version, "ShortVport");
    const std::uint32_t nameEndBit = encoded.bitCount();
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), nameEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Vport rejected;
    rejected.name = "stale-name";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
}

TEST_CASE("DRW_Vport rejects lossy numeric field narrowing",
          "[dwg-write][object-encode][vport][safety]") {
    DRW_Vport source;
    source.circleZoom = -1;
    dwgBufferW circleBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVport(
        source, DRW::AC1015, &circleBody));
    CHECK(circleBody.data().empty());

    source.circleZoom = 100;
    source.renderMode = std::numeric_limits<std::uint8_t>::max() + 1;
    dwgBufferW renderBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVport(
        source, DRW::AC1015, &renderBody));
    CHECK(renderBody.data().empty());

    source.renderMode = 0;
    source.ambientColor = std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW colorBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVport(
        source, DRW::AC1021, &colorBody));
    CHECK(colorBody.data().empty());

    source.ambientColor = 250;
    source.center.x = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW coordinateBody;
    coordinateBody.putRawChar8(0xA5);
    const auto coordinateBefore = snapshot(coordinateBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVport(
        source, DRW::AC1015, &coordinateBody));
    CHECK(snapshot(coordinateBody) == coordinateBefore);

    source.center.x = 0.0;
    source.brightness = std::numeric_limits<double>::infinity();
    dwgBufferW lightingBody;
    lightingBody.putRawChar8(0x5A);
    const auto lightingBefore = snapshot(lightingBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVport(
        source, DRW::AC1021, &lightingBody));
    CHECK(snapshot(lightingBody) == lightingBefore);
}

TEST_CASE("DRW_Vport rejects non-finite DWG values transactionally",
          "[dwg-read][object-encode][vport][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x41,
                       /*handle=*/0x751);
    encoded.putVariableText(version, "VPORT");
    encoded.putBit(0); // xref referenced
    encoded.putBitShort(0); // xref index
    encoded.putBit(0); // xref dependent
    encoded.putBitDouble(nan); // height

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Vport parsed;
    parsed.name = "stale vport";
    parsed.height = 9.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.height == Approx(5.13732));
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_ViewportEntityHeader preserves legacy flags and handles",
          "[dwg-read][object-encode][vport][viewport-entity-header]") {
    for (const DRW::Version version : {DRW::AC1012, DRW::AC1014,
                                       DRW::AC1015, DRW::AC1018}) {
        dwgBufferW body;
        const std::uint32_t objectHandle =
            version == DRW::AC1012 ? 0x700u
            : version == DRW::AC1014 ? 0x780u
            : version == DRW::AC1015 ? 0x800u : 0x900u;
        if (version < DRW::AC1015) {
            emitLegacyObjectPreamble(
                body, version, DRW_ViewportEntityHeader::kDwgType,
                objectHandle, /*numReactors=*/1);
        } else {
            emitObjectPreamble(body, version,
                               DRW_ViewportEntityHeader::kDwgType,
                               objectHandle, /*numReactors=*/1,
                               /*xDictFlag=*/0);
        }
        body.putVariableText(version, "VPH");
        body.putBit(1);             // xref referenced
        body.putBitShort(5);        // xref index
        body.putBit(1);             // xref dependent
        body.putBit(1);             // is on
        body.putHandle(hardPtr(0x820)); // control handle
        body.putHandle(hardPtr(0x821)); // reactor
        body.putHandle(hardPtr(0x822)); // xdictionary
        body.putHandle(hardPtr(0x823)); // xref block
        body.putHandle(hardPtr(0x824)); // viewport entity
        body.putHandle(hardPtr(objectHandle)); // previous self-reference

        auto bytes = snapshot(body);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_ViewportEntityHeader parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseViewportEntityHeader(
            parsed, version, &reader));
        CHECK(parsed.handle == objectHandle);
        CHECK(parsed.name == "VPH");
        CHECK(parsed.flags == (0x50 | 0x02));
        CHECK(parsed.isOn);
        CHECK(parsed.parentHandle == 0x820);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x821});
        CHECK(parsed.xDictHandle == 0x822);
        CHECK(parsed.xrefBlockHandle == 0x823);
        CHECK(parsed.viewportEntityHandle == 0x824);
        CHECK(parsed.previousViewportEntityHeaderHandle == 0);

        auto truncated = bytes;
        REQUIRE(truncated.size() > 1);
        truncated.pop_back();
        dwgBuffer truncatedReader(truncated.data(), truncated.size());
        parsed.name = "STALE";
        parsed.reactorHandles = {0x901};
        CHECK_FALSE(DrwObjectEncodeTestAccess::parseViewportEntityHeader(
            parsed, version, &truncatedReader));
        CHECK_FALSE(truncatedReader.isGood());
        CHECK(parsed.name.empty());
        CHECK(parsed.reactorHandles.empty());
        CHECK(parsed.viewportEntityHandle == 0u);
    }
}

TEST_CASE("DRW_View rejects a body ending before xref flags",
          "[dwg-read][object-encode][view][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x3D,
                       /*handle=*/0x760);
    encoded.putVariableText(version, "ShortView");
    const std::uint32_t nameEndBit = encoded.bitCount();
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), nameEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_View rejected;
    rejected.name = "stale-name";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
}

TEST_CASE("DRW_View rejects non-finite DWG values transactionally",
          "[dwg-read][object-encode][view][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x3D,
                       /*handle=*/0x761);
    encoded.putVariableText(version, "VIEW");
    encoded.putBit(0); // xref referenced
    encoded.putBitShort(0); // xref index
    encoded.putBit(0); // xref dependent
    encoded.putBitDouble(nan); // size Y

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_View parsed;
    parsed.name = "stale view";
    parsed.size.y = 9.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.name.empty());
    CHECK(parsed.size.y == Approx(0.0));
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_View rejects lossy numeric field narrowing",
          "[dwg-write][object-encode][view][safety]") {
    DRW_View source;
    source.renderMode = std::numeric_limits<std::uint8_t>::max() + 1;
    dwgBufferW renderBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeView(
        source, DRW::AC1015, &renderBody));
    CHECK(renderBody.data().empty());

    source.renderMode = 0;
    source.hasUCS = true;
    source.ucsOrthoType = -1;
    dwgBufferW ucsBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeView(
        source, DRW::AC1015, &ucsBody));
    CHECK(ucsBody.data().empty());

    source.hasUCS = false;
    source.ucsOrthoType = 0;
    source.m_ambientColor = std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW colorBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeView(
        source, DRW::AC1021, &colorBody));
    CHECK(colorBody.data().empty());

    source.m_ambientColor = 250;
    source.center.x = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW coordinateBody;
    coordinateBody.putRawChar8(0xA5);
    const auto coordinateBefore = snapshot(coordinateBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeView(
        source, DRW::AC1015, &coordinateBody));
    CHECK(snapshot(coordinateBody) == coordinateBefore);

    source.center.x = 0.0;
    source.m_brightness = std::numeric_limits<double>::infinity();
    dwgBufferW lightingBody;
    lightingBody.putRawChar8(0x5A);
    const auto lightingBefore = snapshot(lightingBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeView(
        source, DRW::AC1021, &lightingBody));
    CHECK(snapshot(lightingBody) == lightingBefore);
}

TEST_CASE("DRW_ViewportEntityHeader encodes legacy and R2004 handles",
          "[dwg-write][object-encode][vport][viewport-entity-header]") {
    for (const DRW::Version version : {DRW::AC1015, DRW::AC1018}) {
        DRW_ViewportEntityHeader source;
        source.handle = 0xA00u;
        source.parentHandle = 0x820;
        source.name = "VPH-WRITE";
        source.flags = 0x50;
        source.isOn = true;
        source.xrefBlockHandle = 0x823;
        source.viewportEntityHandle = 0x824;
        source.previousViewportEntityHeaderHandle = 0;
        source.reactorHandles = {0x821};
        source.xDictHandle = 0x822;
        // AC1015 must still emit the legacy xdictionary handle even if the
        // in-memory source record carries the newer no-xdict flag.
        source.setDwgCommonObjectState(1,
                                       version == DRW::AC1015 ? 1 : 0,
                                       false);

        dwgBufferW body;
        emitObjectPreamble(body, version,
                           DRW_ViewportEntityHeader::kDwgType,
                           source.handle, /*numReactors=*/1,
                           /*xDictFlag=*/0);
        REQUIRE(DrwObjectEncodeTestAccess::encodeViewportEntityHeader(
            source, version, &body));

        auto bytes = snapshot(body);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_ViewportEntityHeader parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseViewportEntityHeader(
            parsed, version, &reader));
        CHECK(parsed.handle == source.handle);
        CHECK(parsed.parentHandle == source.parentHandle);
        CHECK(parsed.name == source.name);
        CHECK(parsed.flags == 0x52);
        CHECK(parsed.isOn);
        CHECK(parsed.reactorHandles == source.reactorHandles);
        CHECK(parsed.xDictHandle == source.xDictHandle);
        CHECK(parsed.xrefBlockHandle == source.xrefBlockHandle);
        CHECK(parsed.viewportEntityHandle == source.viewportEntityHandle);
        CHECK(parsed.previousViewportEntityHeaderHandle == 0u);

        // Semantic round-tripping does not validate the relationship nibble.
        // Decode the same handle stream and assert the specification-defined
        // codes for VPEHEADER's type-specific references.
        dwgBuffer wireReader(bytes.data(), bytes.size());
        REQUIRE(wireReader.getObjType(version)
                == DRW_ViewportEntityHeader::kDwgType);
        if (version > DRW::AC1014 && version < DRW::AC1024)
            CHECK(wireReader.getRawLong32() == 0u);
        CHECK(wireReader.getHandle().ref == source.handle);
        CHECK(wireReader.getBitShort() == 0);
        CHECK(wireReader.getBitLong() == 1);
        if (version > DRW::AC1015)
            CHECK(wireReader.getBit() == 0u);
        CHECK(wireReader.getVariableText(version) == source.name);
        CHECK(wireReader.getBit() == 1u); // xref referenced
        CHECK(wireReader.getBitShort() == 0);
        CHECK(wireReader.getBit() == 1u); // xref dependent
        CHECK(wireReader.getBit() == 1u); // enabled
        CHECK(wireReader.getHandle().code == DRW::DwgSoftPointer);
        CHECK(wireReader.getHandle().code == DRW::DwgSoftPointer);
        CHECK(wireReader.getHandle().code == DRW::DwgHardOwnership);
        CHECK(wireReader.getHandle().code == DRW::DwgHardPointer);
        CHECK(wireReader.getHandle().code == DRW::DwgSoftOwnership);
        CHECK(wireReader.getHandle().code == DRW::DwgHardPointer);
    }
}

TEST_CASE("DRW_Vx records follow object and table handle semantics",
          "[dwg-read][object-encode][vport][vx]") {
    {
        dwgBufferW body;
        emitObjectPreamble(body, DRW::AC1015, DRW_VxControl::kDwgClassNum,
                           0x830, /*numReactors=*/1, /*xDictFlag=*/0);
        body.putHandle(hardPtr(0x830));
        body.putHandle(hardPtr(0x831));
        body.putHandle(hardPtr(0x832));
        auto bytes = snapshot(body);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_VxControl parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseVxControl(
            parsed, DRW::AC1015, &reader));
        CHECK(parsed.handle == 0x830);
        CHECK(parsed.classVersion == 0u);
        CHECK(parsed.flags == 0u);
        CHECK(parsed.parentHandle == 0x830);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x831});
        CHECK(parsed.xDictHandle == 0x832);
        CHECK(parsed.recordHandles.empty());
    }

    {
        dwgBufferW body;
        emitObjectPreamble(body, DRW::AC1015,
                           DRW_VxTableRecord::kDwgClassNum, 0x840,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        body.putHandle(hardPtr(0x840));
        body.putHandle(hardPtr(0x841));
        body.putHandle(hardPtr(0x842));
        auto bytes = snapshot(body);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_VxTableRecord parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseVxTableRecord(
            parsed, DRW::AC1015, &reader));
        CHECK(parsed.handle == 0x840);
        CHECK(parsed.classVersion == 0u);
        CHECK(parsed.flags == 0u);
        CHECK(parsed.name.empty());
        CHECK(parsed.parentHandle == 0x840);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x841});
        CHECK(parsed.xDictHandle == 0x842);
    }

    {
        const std::vector<std::uint8_t> rawData = {0xA5, 0x3C};
        auto bytes = emitAc1021VxObject(
            DRW_VxControl::kDwgClassNum, 0x850, 7, 3, {}, {0x851, 0x852},
            rawData);
        REQUIRE_FALSE(bytes.empty());
        DRW_TextCodec decoder;
        decoder.setVersion(DRW::AC1021, false);
        decoder.setCodePage("UTF-16", false);
        dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
        DRW_VxControl parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseVxControl(
            parsed, DRW::AC1021, &reader));
        CHECK(parsed.handle == 0x850);
        CHECK(parsed.classVersion == 7u);
        CHECK(parsed.flags == 3u);
        CHECK(parsed.parentHandle == 0x810);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x811});
        CHECK(parsed.xDictHandle == 0x812);
        CHECK(parsed.recordHandles == std::vector<std::uint32_t>{0x851, 0x852});
        REQUIRE(parsed.hasDwgRawData());
        CHECK(parsed.dwgRawDataBitSize() >= rawData.size() * 8u);
        REQUIRE(parsed.dwgRawData().size() >= rawData.size());
        CHECK(parsed.dwgRawData()[0] == rawData[0]);
        CHECK(parsed.dwgRawData()[1] == rawData[1]);

        dwgBufferW sameVersionBody;
        dwgBufferW sameVersionHandles;
        emitObjectPreamble(sameVersionBody, DRW::AC1021,
                           DRW_VxControl::kDwgClassNum, parsed.handle,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        REQUIRE(DrwObjectEncodeTestAccess::encodeVxControl(
            parsed, DRW::AC1021, &sameVersionBody, nullptr,
            &sameVersionHandles));
        CHECK(sameVersionBody.bitCount() > rawData.size() * 8u);

        dwgBufferW crossVersionBody;
        CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVxControl(
            parsed, DRW::AC1024, &crossVersionBody));
        CHECK(crossVersionBody.data().empty());

        bytes.pop_back();
        dwgBuffer truncatedReader(bytes.data(), bytes.size(), &decoder);
        CHECK_FALSE(DrwObjectEncodeTestAccess::parseVxControl(
            parsed, DRW::AC1021, &truncatedReader));
        CHECK_FALSE(truncatedReader.isGood());
    }

    {
        const std::vector<std::uint8_t> rawData = {0xF0, 0x0D};
        auto bytes = emitAc1021VxObject(
            DRW_VxTableRecord::kDwgClassNum, 0x860, 9, 5, "VX-AC1021", {},
            rawData);
        REQUIRE_FALSE(bytes.empty());
        DRW_TextCodec decoder;
        decoder.setVersion(DRW::AC1021, false);
        decoder.setCodePage("UTF-16", false);
        dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
        DRW_VxTableRecord parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseVxTableRecord(
            parsed, DRW::AC1021, &reader));
        CHECK(parsed.handle == 0x860);
        CHECK(parsed.classVersion == 9u);
        CHECK(parsed.flags == 5u);
        CHECK(parsed.name == "VX-AC1021");
        CHECK(parsed.parentHandle == 0x810);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x811});
        CHECK(parsed.xDictHandle == 0x812);
        REQUIRE(parsed.hasDwgRawData());
        CHECK(parsed.dwgRawDataBitSize() >= rawData.size() * 8u);
        REQUIRE(parsed.dwgRawData().size() >= rawData.size());
        CHECK(parsed.dwgRawData()[0] == rawData[0]);
        CHECK(parsed.dwgRawData()[1] == rawData[1]);

        auto truncated = bytes;
        REQUIRE(truncated.size() > 2);
        truncated.resize(truncated.size() - 2);
        dwgBuffer truncatedReader(truncated.data(), truncated.size(), &decoder);
        parsed.name = "STALE";
        CHECK_FALSE(DrwObjectEncodeTestAccess::parseVxTableRecord(
            parsed, DRW::AC1021, &truncatedReader));
        CHECK_FALSE(truncatedReader.isGood());
        CHECK(parsed.name.empty());
    }
}

TEST_CASE("DRW_Vx writers bound handle lists and report buffer failures",
          "[dwg-write][object-encode][vport][vx][safety]") {
    DRW_VxControl control;
    control.reactorHandles.resize(dwgSafety::MaxReactorCount + 1);
    dwgBufferW controlBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVxControl(
        control, DRW::AC1021, &controlBody));
    CHECK(controlBody.data().empty());

    DRW_VxTableRecord record;
    record.reactorHandles.resize(dwgSafety::MaxReactorCount + 1);
    dwgBufferW recordBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeVxTableRecord(
        record, DRW::AC1021, &recordBody));
    CHECK(recordBody.data().empty());

    DRW_VxControl validControl;
    validControl.classVersion = 7;
    validControl.flags = 3;
    validControl.parentHandle = 0x810;
    validControl.reactorHandles = {0x811};
    validControl.xDictHandle = 0x812;
    validControl.recordHandles = {0x851, 0x852};
    DrwObjectEncodeTestAccess::setNumReactors(validControl, 1);
    dwgBufferW validBody;
    dwgBufferW validHandles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeVxControl(
        validControl, DRW::AC1021, &validBody, nullptr, &validHandles));
    CHECK(validBody.isGood());
    CHECK(validHandles.isGood());
    CHECK_FALSE(validBody.data().empty());
    CHECK_FALSE(validHandles.data().empty());

    // Check the physical relationship codes, not only the parsed references.
    dwgBuffer handleReader(validHandles.data().data(),
                           validHandles.data().size());
    CHECK(handleReader.getHandle().code == DRW::DwgSoftPointer);
    CHECK(handleReader.getHandle().code == DRW::DwgSoftPointer);
    CHECK(handleReader.getHandle().code == DRW::DwgHardOwnership);
    CHECK(handleReader.getHandle().code == DRW::DwgSoftOwnership);
    CHECK(handleReader.getHandle().code == DRW::DwgSoftOwnership);
    CHECK(handleReader.isGood());
}

TEST_CASE("DRW_VxControl rejects an impossible record count before allocation",
          "[dwg-read][object-encode][vport][vx][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW handles;
    emitObjectPreamble(data, version, DRW_VxControl::kDwgClassNum,
                       /*handle=*/0x870u, /*numReactors=*/0,
                       /*xDictFlag=*/1);
    data.putBitLong(0); // class version
    data.putBitLong(0); // flags
    data.putBitLong(static_cast<std::int32_t>(
        dwgSafety::MaxReactorCount));
    data.alignToByte();
    data.patchRawLong32AtBit(objectSizeBitOffset(data),
                             static_cast<std::uint32_t>(data.bitCount()));
    handles.putHandle(nullHandle()); // common parent handle
    handles.alignToByte();

    auto bytes = snapshot(data);
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
    DRW_TextCodec decoder;
    decoder.setVersion(version, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_VxControl parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseVxControl(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.recordHandles.empty());
}

TEST_CASE("DRW_TvDeviceProperties preserves fields and bounded handles",
          "[dwg-read][object-encode][tv-device-properties]") {
    {
        dwgBufferW body;
        emitObjectPreamble(body, DRW::AC1015,
                           DRW_TvDeviceProperties::kDwgClassNum, 0x900,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        body.putBitLong(0x1234);
        body.putBitShort(9);
        body.putBitLong(1);
        body.putBitLongLong(0x112233445566ULL);
        body.putBitLongLong(0x223344556677ULL);
        body.putBitLongLong(0x334455667788ULL);
        body.putBitLong(4);
        body.putBitDouble(1.25);
        body.putBitDouble(2.5);
        body.putHandle(hardPtr(0x910));
        body.putHandle(hardPtr(0x911));
        body.putHandle(hardPtr(0x912));

        auto bytes = snapshot(body);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_TvDeviceProperties parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseTvDeviceProperties(
            parsed, DRW::AC1015, &reader));
        CHECK(parsed.handle == 0x900u);
        CHECK(parsed.flags == 0x1234);
        CHECK(parsed.maxRegenThreads == 9);
        CHECK(parsed.useLutPalette == 1);
        CHECK(parsed.alternateHighlight == 0x112233445566ULL);
        CHECK(parsed.alternateHighlightColor == 0x223344556677ULL);
        CHECK(parsed.geometryShaderUsage == 0x334455667788ULL);
        CHECK(parsed.blendingMode == 4);
        CHECK(parsed.antialiasingLevel == Approx(1.25));
        CHECK(parsed.valueBd2 == Approx(2.5));
        CHECK(parsed.parentHandle == 0x910);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x911});
        CHECK(parsed.xDictHandle == 0x912u);
    }

    {
        auto bytes = emitAc1021TvDeviceProperties(
            0x5678, 11, 0, 0x445566778899ULL, 0x5566778899AAULL,
            0x66778899AABBULL, 7, 3.5, 4.75);
        REQUIRE_FALSE(bytes.empty());
        DRW_TextCodec decoder;
        decoder.setVersion(DRW::AC1021, false);
        decoder.setCodePage("UTF-16", false);
        dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
        DRW_TvDeviceProperties parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseTvDeviceProperties(
            parsed, DRW::AC1021, &reader));
        CHECK(parsed.handle == 0x930u);
        CHECK(parsed.flags == 0x5678);
        CHECK(parsed.maxRegenThreads == 11);
        CHECK(parsed.alternateHighlight == 0x445566778899ULL);
        CHECK(parsed.alternateHighlightColor == 0x5566778899AAULL);
        CHECK(parsed.geometryShaderUsage == 0x66778899AABBULL);
        CHECK(parsed.blendingMode == 7);
        CHECK(parsed.antialiasingLevel == Approx(3.5));
        CHECK(parsed.valueBd2 == Approx(4.75));
        CHECK(parsed.parentHandle == 0x910);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x911});
        CHECK(parsed.xDictHandle == 0x912u);

        auto truncated = bytes;
        REQUIRE(truncated.size() > 1);
        truncated.pop_back();
        dwgBuffer truncatedReader(truncated.data(), truncated.size(),
                                  &decoder);
        parsed.flags = 99;
        parsed.reactorHandles = {0x999};
        CHECK_FALSE(DrwObjectEncodeTestAccess::parseTvDeviceProperties(
            parsed, DRW::AC1021, &truncatedReader));
        CHECK(parsed.flags == 0);
        CHECK(parsed.reactorHandles.empty());
        CHECK(parsed.parentHandle == 0);
    }
}

TEST_CASE("DRW_TvDeviceProperties rejects a body ending before BLL fields",
          "[dwg-read][object-encode][tv-device-properties][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    emitObjectPreamble(data, version, DRW_TvDeviceProperties::kDwgClassNum,
                       0x931);
    data.putBitLong(0x1234); // flags only; the next field is outside the body
    data.patchRawLong32AtBit(objectSizeBitOffset(data),
                             static_cast<std::uint32_t>(data.bitCount()));

    auto bytes = snapshot(data);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_TvDeviceProperties parsed;
    parsed.flags = 99;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseTvDeviceProperties(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.flags == 0);
    CHECK(parsed.alternateHighlight == 0);
}

TEST_CASE("DRW_TvDeviceProperties rejects bit-short overflow",
          "[dwg-write][object-encode][tv-device-properties][safety]") {
    DRW_TvDeviceProperties properties;
    properties.maxRegenThreads = 65536;
    dwgBufferW body;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeTvDeviceProperties(
        properties, DRW::AC1015, &body));
    CHECK(body.data().empty());

    properties.maxRegenThreads = 0;
    properties.antialiasingLevel = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW nonFiniteBody;
    nonFiniteBody.putRawChar8(0xA5);
    const auto beforeNonFinite = nonFiniteBody.data();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeTvDeviceProperties(
        properties, DRW::AC1015, &nonFiniteBody));
    CHECK(nonFiniteBody.data() == beforeNonFinite);

    dwgBufferW malformed;
    emitObjectPreamble(malformed, DRW::AC1015,
                       DRW_TvDeviceProperties::kDwgClassNum, 0x932);
    malformed.putBitLong(0); // flags
    malformed.putBitShort(0); // max regen threads
    malformed.putBitLong(0); // use LUT palette
    malformed.putBitLongLong(0); // alternate highlight
    malformed.putBitLongLong(0); // alternate highlight color
    malformed.putBitLongLong(0); // geometry shader usage
    malformed.putBitLong(0); // blending mode
    malformed.putBitDouble(std::numeric_limits<double>::quiet_NaN());
    auto malformedBytes = snapshot(malformed);
    dwgBuffer malformedReader(malformedBytes.data(), malformedBytes.size());
    DRW_TvDeviceProperties rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseTvDeviceProperties(
        rejected, DRW::AC1015, &malformedReader));
    CHECK_FALSE(malformedReader.isGood());
    CHECK(rejected.flags == 0);
    CHECK(rejected.parentHandle == 0);
}

TEST_CASE("DRW_CsacDocumentOptions preserves bounded fields and handles",
          "[dwg-read][object-encode][csac-document-options]") {
    {
        dwgBufferW body;
        emitObjectPreamble(body, DRW::AC1015,
                           DRW_CsacDocumentOptions::kDwgClassNum, 0x950,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        body.putHandle(hardPtr(0x940));
        body.putHandle(hardPtr(0x941));
        body.putHandle(hardPtr(0x942));

        auto bytes = snapshot(body);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_CsacDocumentOptions parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseCsacDocumentOptions(
            parsed, DRW::AC1015, &reader));
        CHECK(parsed.classVersion == 0u);
        CHECK(parsed.flags == 0u);
        CHECK(parsed.handle == 0x950u);
        CHECK(parsed.parentHandle == 0x940);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x941});
        CHECK(parsed.xDictHandle == 0x942u);
    }

    {
        auto bytes = emitAc1021CsacDocumentOptions(0xF1234567u,
                                                    0x80000001u);
        REQUIRE_FALSE(bytes.empty());
        DRW_TextCodec decoder;
        decoder.setVersion(DRW::AC1021, false);
        decoder.setCodePage("UTF-16", false);
        dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
        DRW_CsacDocumentOptions parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parseCsacDocumentOptions(
            parsed, DRW::AC1021, &reader));
        CHECK(parsed.handle == 0x950u);
        CHECK(parsed.classVersion == 0xF1234567u);
        CHECK(parsed.flags == 0x80000001u);
        CHECK(parsed.parentHandle == 0x940);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x941});
        CHECK(parsed.xDictHandle == 0x942u);

        auto truncated = bytes;
        REQUIRE(truncated.size() > 1);
        truncated.pop_back();
        dwgBuffer truncatedReader(truncated.data(), truncated.size(),
                                  &decoder);
        parsed.flags = 99;
        parsed.reactorHandles = {0x999};
        CHECK_FALSE(DrwObjectEncodeTestAccess::parseCsacDocumentOptions(
            parsed, DRW::AC1021, &truncatedReader));
        CHECK_FALSE(truncatedReader.isGood());
        CHECK(parsed.classVersion == 0u);
        CHECK(parsed.flags == 0u);
        CHECK(parsed.reactorHandles.empty());
        CHECK(parsed.parentHandle == 0);
    }
    {
        dwgBufferW data;
        dwgBufferW handles;
        emitObjectPreamble(data, DRW::AC1021,
                           DRW_CsacDocumentOptions::kDwgClassNum,
                           /*handle=*/0x951, /*numReactors=*/0,
                           /*xDictFlag=*/1);
        data.putBit(0); // One incomplete bit remains before the object boundary.
        data.patchRawLong32AtBit(objectSizeBitOffset(data),
                                 static_cast<std::uint32_t>(data.bitCount()));
        emitCommonHandlePrefix(handles, /*parentHandle=*/0,
                               /*reactorHandles=*/{}, /*xDictFlag=*/1);
        auto bytes = data.data();
        bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_CsacDocumentOptions parsed;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parseCsacDocumentOptions(
            parsed, DRW::AC1021, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.classVersion == 0u);
        CHECK(parsed.flags == 0u);
        CHECK(parsed.parentHandle == 0u);
    }
}

TEST_CASE("DRW_ContextDataManager parses sub-manager text and handles",
          "[dwg-read][object-encode][context-data-manager]") {
    dwgBufferW body;
    emitObjectPreamble(body, DRW::AC1015,
                       DRW_ContextDataManager::kDwgClassNum, 0xA50,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    body.putBitLong(2); // sub-manager count
    body.putBitLong(1);
    body.putVariableText(DRW::AC1015, "Model");
    body.putBitLong(2);
    body.putVariableText(DRW::AC1015, "Paper");
    body.putVariableText(DRW::AC1015, "Viewport");
    emitCommonHandlePrefix(body, 0xA40, {0xA41}, /*xDictFlag=*/0);
    body.putHandle(hardPtr(0xA42)); // object context
    body.putHandle(hardPtr(0xA43)); // first sub-manager
    body.putHandle(hardPtr(0xA44)); // first item
    body.putHandle(hardPtr(0xA45)); // second sub-manager
    body.putHandle(hardPtr(0xA46)); // second item
    body.putHandle(hardPtr(0xA47)); // third item

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ContextDataManager parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseContextDataManager(
        parsed, DRW::AC1015, &reader));
    CHECK(parsed.handle == 0xA50u);
    CHECK(parsed.parentHandle == 0xA40);
    CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0xA41});
    CHECK(parsed.xDictHandle == 0x0u);
    CHECK(parsed.objectContextHandle == 0xA42u);
    CHECK(parsed.subManagerCount == 2u);
    REQUIRE(parsed.subManagers.size() == 2);
    CHECK(parsed.subManagers[0].handle == 0xA43u);
    CHECK(parsed.subManagers[0].entryCount == 1u);
    REQUIRE(parsed.subManagers[0].entries.size() == 1);
    CHECK(parsed.subManagers[0].entries[0].text == "Model");
    CHECK(parsed.subManagers[0].entries[0].itemHandle == 0xA44u);
    CHECK(parsed.subManagers[1].handle == 0xA45u);
    CHECK(parsed.subManagers[1].entryCount == 2u);
    REQUIRE(parsed.subManagers[1].entries.size() == 2);
    CHECK(parsed.subManagers[1].entries[0].text == "Paper");
    CHECK(parsed.subManagers[1].entries[0].itemHandle == 0xA46u);
    CHECK(parsed.subManagers[1].entries[1].text == "Viewport");
    CHECK(parsed.subManagers[1].entries[1].itemHandle == 0xA47u);

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    parsed.objectContextHandle = 99;
    parsed.subManagers.clear();
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseContextDataManager(
        parsed, DRW::AC1015, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.objectContextHandle == 0u);
    CHECK(parsed.subManagers.empty());
}

TEST_CASE("DRW_ContextDataManager accepts compact body counts",
          "[dwg-read][object-encode][context-data-manager][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version,
                       DRW_ContextDataManager::kDwgClassNum, 0xA51,
                       /*numReactors=*/0);
    encoded.putBitLong(0); // a zero BL is encoded in its two-bit form
    const auto bodyEnd = static_cast<std::uint32_t>(encoded.bitCount());
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0xA40, {}, 0);
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEnd);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ContextDataManager parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseContextDataManager(
        parsed, version, &reader));
    CHECK(parsed.handle == 0xA51u);
    CHECK(parsed.subManagerCount == 0u);
    CHECK(parsed.subManagers.empty());
    CHECK(parsed.parentHandle == 0xA40u);
}

TEST_CASE("DRW_Section preserves manager and settings handles",
          "[dwg-read][object-encode][section]") {
    {
        dwgBufferW body;
        emitObjectPreamble(body, DRW::AC1015,
                           DRW_Section::kDwgClassNumManager, 0xA00,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        body.putBit(1); // is_live
        body.putBitShort(2);
        body.putHandle(hardPtr(0xA10)); // owner
        body.putHandle(hardPtr(0xA11)); // reactor
        body.putHandle(hardPtr(0xA12)); // xdictionary
        body.putHandle(hardPtr(0xA13));
        body.putHandle(hardPtr(0xA14));

        auto bytes = snapshot(body);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_Section parsed;
        parsed.m_kind = DRW_Section::Manager;
        REQUIRE(DrwObjectEncodeTestAccess::parseSection(
            parsed, DRW::AC1015, &reader));
        CHECK(parsed.handle == 0xA00u);
        CHECK(parsed.m_isLive);
        CHECK(parsed.m_sectionCount == 2);
        CHECK(parsed.m_sectionHandles ==
              std::vector<std::uint32_t>{0xA13, 0xA14});
        CHECK(parsed.parentHandle == 0xA10);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0xA11});
        CHECK(parsed.xDictHandle == 0xA12u);
    }

    {
        auto bytes = emitAc1021SectionObject(
            DRW_Section::Manager, {0, 0}, {0xA13, 0xA14});
        REQUIRE_FALSE(bytes.empty());
        DRW_TextCodec decoder;
        decoder.setVersion(DRW::AC1021, false);
        decoder.setCodePage("UTF-16", false);
        dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
        DRW_Section parsed;
        parsed.m_kind = DRW_Section::Manager;
        REQUIRE(DrwObjectEncodeTestAccess::parseSection(
            parsed, DRW::AC1021, &reader));
        CHECK(parsed.handle == 0xA00u);
        CHECK(parsed.m_isLive);
        CHECK(parsed.m_sectionCount == 2);
        CHECK(parsed.m_sectionHandles ==
              std::vector<std::uint32_t>{0xA13, 0xA14});
        CHECK(parsed.parentHandle == 0xA10);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0xA11});
        CHECK(parsed.xDictHandle == 0xA12u);
    }

    {
        auto bytes = emitAc1021SectionObject(
            DRW_Section::Settings, {2, 0}, {});
        REQUIRE_FALSE(bytes.empty());
        DRW_TextCodec decoder;
        decoder.setVersion(DRW::AC1021, false);
        decoder.setCodePage("UTF-16", false);
        dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
        DRW_Section parsed;
        parsed.m_kind = DRW_Section::Settings;
        REQUIRE(DrwObjectEncodeTestAccess::parseSection(
            parsed, DRW::AC1021, &reader));
        CHECK(parsed.handle == 0xA00u);
        CHECK(parsed.m_classVersion == 0);
        CHECK(parsed.m_sectionType == 2);
        CHECK(parsed.m_generationOptions == 0);
        CHECK(parsed.m_currentType == 2);
        CHECK(parsed.m_typeCount == 0);
        CHECK(parsed.m_sourceHandle == 0u);
        CHECK(parsed.m_destinationBlockHandle == 0u);
        CHECK(parsed.m_destinationFileHandle == 0u);
        CHECK(parsed.parentHandle == 0xA10);
        CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0xA11});
        CHECK(parsed.xDictHandle == 0xA12u);

        auto truncated = bytes;
        REQUIRE(truncated.size() > 1);
        truncated.pop_back();
        dwgBuffer truncatedReader(truncated.data(), truncated.size(),
                                  &decoder);
        parsed.m_classVersion = 99;
        parsed.m_sourceHandle = 0x999;
        parsed.parentHandle = 0x998;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parseSection(
            parsed, DRW::AC1021, &truncatedReader));
        CHECK_FALSE(truncatedReader.isGood());
        CHECK(parsed.m_classVersion == 0);
        CHECK(parsed.m_sourceHandle == 0u);
        CHECK(parsed.parentHandle == 0);
    }

    {
        auto bytes = emitAc1021SectionObject(
            DRW_Section::Settings, {2, DRW_Section::kMaxSectionTypeCount + 1}, {});
        REQUIRE_FALSE(bytes.empty());
        DRW_TextCodec decoder;
        decoder.setVersion(DRW::AC1021, false);
        decoder.setCodePage("UTF-16", false);
        dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
        DRW_Section parsed;
        parsed.m_kind = DRW_Section::Settings;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parseSection(
            parsed, DRW::AC1021, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.m_currentType == 0);
        CHECK(parsed.m_typeCount == 0);
        CHECK(parsed.parentHandle == 0);
    }

    for (const bool hasDsData : {false, true}) {
        std::uint32_t handleBitSize = 0;
        auto bytes = emitAc1027SectionObject(hasDsData, &handleBitSize);
        REQUIRE_FALSE(bytes.empty());
        DRW_TextCodec decoder;
        decoder.setVersion(DRW::AC1027, false);
        decoder.setCodePage("UTF-16", false);
        dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
        DRW_Section parsed;
        parsed.m_kind = DRW_Section::Settings;
        REQUIRE(DrwObjectEncodeTestAccess::parseSection(
            parsed, DRW::AC1027, &reader, handleBitSize));
        CHECK(parsed.hasDataStorageBinaryData() == hasDsData);
        CHECK(parsed.m_sectionType == 2);
        CHECK(parsed.parentHandle == 0xB10);
    }

    {
        std::uint32_t handleBitSize = 0;
        const std::vector<std::uint8_t> rawTail{0xA5};
        auto bytes = emitAc1027SectionObject(false, &handleBitSize, rawTail);
        REQUIRE_FALSE(bytes.empty());
        DRW_TextCodec decoder;
        decoder.setVersion(DRW::AC1027, false);
        decoder.setCodePage("UTF-16", false);
        dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
        DRW_Section parsed;
        parsed.m_kind = DRW_Section::Settings;
        REQUIRE(DrwObjectEncodeTestAccess::parseSection(
            parsed, DRW::AC1027, &reader, handleBitSize));
        REQUIRE(parsed.hasDwgRawData());
        CHECK(parsed.dwgRawDataVersion() == DRW::AC1027);
        CHECK(parsed.dwgRawDataBitSize() >= rawTail.size() * 8u);
        CHECK((parsed.dwgRawData()[0] & 0xFFu) == rawTail[0]);

        dwgBufferW encodedBody;
        dwgBufferW encodedStrings;
        dwgBufferW encodedHandles;
        REQUIRE(DrwObjectEncodeTestAccess::encodeSection(
            parsed, DRW::AC1027, &encodedBody, &encodedStrings,
            &encodedHandles));
        CHECK(encodedBody.bitCount() >= rawTail.size() * 8u);
        CHECK(encodedStrings.data().empty());
        CHECK_FALSE(encodedHandles.data().empty());

        dwgBuffer encodedReader(encodedBody.data().data(),
                                encodedBody.data().size());
        CHECK(encodedReader.getBitLong() == 2);
        CHECK(encodedReader.getBitLong() == 0);
        std::uint8_t encodedRaw = 0;
        for (int bit = 7; bit >= 0; --bit)
            encodedRaw |= static_cast<std::uint8_t>(
                encodedReader.getBit() << bit);
        CHECK(encodedRaw == rawTail[0]);

        dwgBufferW wrongVersionBody;
        CHECK_FALSE(DrwObjectEncodeTestAccess::encodeSection(
            parsed, DRW::AC1024, &wrongVersionBody));
        CHECK(wrongVersionBody.data().empty());
    }
}

TEST_CASE("DRW_Section rejects impossible nested counts before allocation",
          "[dwg-read][object-encode][section][safety]") {
    const DRW::Version version = DRW::AC1015;

    const auto checkRejected = [version](const std::vector<std::uint8_t>& bytes,
                                         DRW_Section::Kind kind) {
        dwgBuffer reader(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        DRW_Section parsed;
        parsed.m_kind = kind;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parseSection(
            parsed, version, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.m_sectionHandles.empty());
        CHECK(parsed.m_types.empty());
    };

    dwgBufferW manager;
    emitObjectPreamble(manager, version, DRW_Section::kDwgClassNumManager,
                       0xA03, /*numReactors=*/0);
    manager.putBit(1);
    manager.putBitShort(std::numeric_limits<std::uint16_t>::max());
    emitCommonHandlePrefix(manager, /*parentHandle=*/0xA10, {},
                           /*xDictFlag=*/0);
    checkRejected(snapshot(manager), DRW_Section::Manager);

    dwgBufferW geometrySettings;
    emitObjectPreamble(geometrySettings, version,
                       DRW_Section::kDwgClassNumSettings, 0xA04,
                       /*numReactors=*/0);
    geometrySettings.putBitLong(0); // current type
    geometrySettings.putBitLong(1); // one type
    geometrySettings.putBitLong(0); // type
    geometrySettings.putBitLong(0); // generation
    geometrySettings.putBitLong(0); // no sources
    geometrySettings.putVariableText(version, "");
    geometrySettings.putBitLong(DRW_Section::kMaxSectionGeometryCount);
    emitCommonHandlePrefix(geometrySettings, /*parentHandle=*/0xA10, {},
                           /*xDictFlag=*/0);
    checkRejected(snapshot(geometrySettings), DRW_Section::Settings);

    dwgBufferW sourceHandles;
    emitObjectPreamble(sourceHandles, version,
                       DRW_Section::kDwgClassNumSettings, 0xA05,
                       /*numReactors=*/0);
    sourceHandles.putBitLong(0); // current type
    sourceHandles.putBitLong(1); // one type
    sourceHandles.putBitLong(0); // type
    sourceHandles.putBitLong(0); // generation
    sourceHandles.putBitLong(DRW_Section::kMaxSectionSourceCount);
    sourceHandles.putVariableText(version, "");
    sourceHandles.putBitLong(0); // no geometry settings
    emitCommonHandlePrefix(sourceHandles, /*parentHandle=*/0xA10, {},
                           /*xDictFlag=*/0);
    checkRejected(snapshot(sourceHandles), DRW_Section::Settings);
}

TEST_CASE("DRW_View::encodeDwg preserves conditional and common handles",
          "[dwg-write][object-encode][view]") {
    const DRW::Version ver = DRW::AC1015;

    auto roundTrip = [&](bool associatedUcs) {
        DRW_View source;
        source.handle = 0x500;
        source.name = associatedUcs ? "WITH-UCS" : "WITHOUT-UCS";
        source.hasUCS = associatedUcs;
        source.reactorHandles = {0x82};
        source.xDictHandle = 0x81;
        source.xrefBlockHandle = hardPtr(0x90);
        source.baseUCS_ID = associatedUcs ? 0x91 : 0;
        source.namedUCS_ID = associatedUcs ? 0x92 : 0;
        if (associatedUcs) {
            source.ucsOrigin = DRW_Coord(3.0, 4.0, 5.0);
            source.ucsXAxis = DRW_Coord(0.0, 1.0, 0.0);
            source.ucsYAxis = DRW_Coord(-1.0, 0.0, 0.0);
            source.ucsElevation = 2.0;
            source.ucsOrthoType = 1;
        }
        DrwObjectEncodeTestAccess::setNumReactors(source, 1);
        DrwObjectEncodeTestAccess::setXDictFlag(source, 0);

        dwgBufferW encoded;
        emitObjectPreamble(encoded, ver, /*oType=*/0x3D /* VIEW */,
                           source.handle, /*numReactors=*/1,
                           /*xDictFlag=*/0);
        REQUIRE(DrwObjectEncodeTestAccess::encodeView(source, ver, &encoded));

        auto bytes = snapshot(encoded);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_View parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
        REQUIRE(parsed.name == source.name);
        REQUIRE(parsed.parentHandle == 0x06);
        REQUIRE(parsed.reactorHandles == std::vector<std::uint32_t>{0x82});
        REQUIRE(parsed.xDictHandle == 0x81);
        REQUIRE(parsed.xrefBlockHandle.ref == 0x90);
        REQUIRE(parsed.hasUCS == associatedUcs);
        if (associatedUcs) {
            REQUIRE(parsed.baseUCS_ID == 0x91);
            REQUIRE(parsed.namedUCS_ID == 0x92);
            REQUIRE(parsed.ucsOrigin.x == Approx(3.0));
            REQUIRE(parsed.ucsYAxis.x == Approx(-1.0));
        } else {
            REQUIRE(parsed.baseUCS_ID == 0);
            REQUIRE(parsed.namedUCS_ID == 0);
        }
    };

    roundTrip(false);
    roundTrip(true);
}

TEST_CASE("DRW_View AC1021 framing preserves lighting CMC and handles",
          "[dwg-write][object-encode][view]") {
    DRW_View source;
    source.handle = 0x510;
    source.name = "MODERN-VIEW";
    source.reactorHandles = {0x82};
    source.xDictHandle = 0x81;
    source.xrefBlockHandle = hardPtr(0x90);
    source.hasUCS = true;
    source.ucsOrthoType = 2;
    source.baseUCS_ID = 0x25;
    source.namedUCS_ID = 0x26;
    source.m_useDefaultLights = false;
    source.m_defaultLightingType = 2;
    source.m_brightness = 0.25;
    source.m_contrast = 0.75;
    source.m_ambientColor = 7;
    source.m_ambientColorRgb = 0x123456;
    source.m_ambientColorName = "Book$AmbientName";
    source.m_backgroundHandle = 0x21;
    source.m_visualStyleHandle = 0x22;
    source.m_sunHandle = 0x23;
    source.m_liveSectionHandle = 0x24;
    DrwObjectEncodeTestAccess::setNumReactors(source, 1);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 0);

    const auto bytes = emitAc1021ViewObject(source);
    REQUIRE_FALSE(bytes.empty());
    DRW_TextCodec decoder;
    decoder.setVersion(DRW::AC1021, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(const_cast<std::uint8_t *>(bytes.data()), bytes.size(),
                     &decoder);
    DRW_View parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, DRW::AC1021, &reader));
    CHECK(parsed.name == source.name);
    CHECK(parsed.parentHandle == 0x06u);
    CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0x82u});
    CHECK(parsed.xDictHandle == 0x81u);
    CHECK(parsed.xrefBlockHandle.ref == 0x90u);
    CHECK(parsed.hasUCS);
    CHECK(parsed.ucsOrthoType == 2);
    CHECK(parsed.baseUCS_ID == 0x25u);
    CHECK(parsed.namedUCS_ID == 0x26u);
    CHECK_FALSE(parsed.m_useDefaultLights);
    CHECK(parsed.m_defaultLightingType == 2);
    CHECK(parsed.m_brightness == Approx(0.25));
    CHECK(parsed.m_contrast == Approx(0.75));
    CHECK(parsed.m_ambientColor == 256u);
    CHECK(parsed.m_ambientColorRgb == 0x123456);
    CHECK(parsed.m_ambientColorName == "Book$AmbientName");
    CHECK(parsed.m_backgroundHandle == 0x21u);
    CHECK(parsed.m_visualStyleHandle == 0x22u);
    CHECK(parsed.m_sunHandle == 0x23u);
    CHECK(parsed.m_liveSectionHandle == 0x24u);
}

TEST_CASE("DRW_View::parseDwg rolls back a truncated handle stream",
          "[dwg-read][object-encode][view][safety]") {
    DRW_View source;
    source.handle = 0x500;
    source.name = "TRUNCATED-VIEW";
    source.reactorHandles = {0x82};
    source.xDictHandle = 0x81;
    source.xrefBlockHandle = hardPtr(0x90);
    DrwObjectEncodeTestAccess::setNumReactors(source, 1);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 0);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, DRW::AC1015, /*oType=*/0x3D /* VIEW */,
                       source.handle, /*numReactors=*/1,
                       /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeView(source, DRW::AC1015,
                                                  &encoded));
    auto bytes = snapshot(encoded);
    REQUIRE(bytes.size() > 2);
    bytes.resize(bytes.size() - 2);

    DRW_View parsed;
    parsed.name = "old-state";
    parsed.baseUCS_ID = 0xAA;
    dwgBuffer reader(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, DRW::AC1015,
                                                  &reader));
    CHECK(parsed.name.empty());
    CHECK(parsed.baseUCS_ID == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xrefBlockHandle.ref == 0);
}

TEST_CASE("DRW_View::parseDwg keeps table handles raw",
          "[dwg-read][object-encode][view][safety]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW body;
    emitObjectPreamble(body, ver, /*oType=*/0x3D, /*handle=*/0x500u,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    body.putVariableText(ver, "RAW-HANDLES");
    body.putBit(0);          // xref referenced
    body.putBitShort(0);     // xref index
    body.putBit(0);          // xref dependent
    body.putBitDouble(100.0); // height
    body.putBitDouble(200.0); // width
    body.put2RawDouble(DRW_Coord(10.0, 20.0));
    body.put3BitDouble(DRW_Coord(0.0, 0.0, 0.0));
    body.put3BitDouble(DRW_Coord(0.0, 0.0, 1.0));
    body.putBitDouble(0.0);  // twist
    body.putBitDouble(50.0); // lens length
    body.putBitDouble(0.0);  // front clip
    body.putBitDouble(0.0);  // back clip
    body.putBit(0);          // view mode bit 0
    body.putBit(0);          // view mode bit 1
    body.putBit(0);          // view mode bit 2
    body.putBit(1);          // inverted view mode bit 4
    body.putRawChar8(2);     // render mode
    body.putBit(0);          // paper space
    body.putBit(0);          // no associated UCS
    // The control handle is relative; all following table references are raw.
    body.putHandle(hardPtr(0x06u));
    body.putHandle(handleWithCode(0xAu, 0x82u));
    body.putHandle(handleWithCode(0xCu, 0x81u));
    body.putHandle(hardPtr(0x90u));

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_View parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.parentHandle == 0x06u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x82u);
    CHECK(parsed.xDictHandle == 0x81u);
    CHECK(parsed.xrefBlockHandle.ref == 0x90u);
}

// GROUP encoder round-trip (ODA §20.4.72).  Description TV + unnamed flag +
// selectable flag + handle count + entity handle list.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Group::encodeDwg round-trips description + entity handles",
          "[dwg-write][object-encode][group]") {
    DRW_Group src;
    src.handle        = 0x700;
    src.parentHandle  = 0xC;
    src.m_description = "MyGroup";
    src.m_isUnnamed   = false;
    src.m_selectable  = true;
    src.m_entityHandles = {0x800, 0x801, 0x802};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);  // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/72 /* GROUP */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeGroup(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Group dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_description == "MyGroup");
    REQUIRE(dst.m_isUnnamed   == false);
    REQUIRE(dst.m_selectable  == true);
    REQUIRE(dst.m_entityHandles.size() == 3u);
    REQUIRE(dst.m_entityHandles[0] == 0x800u);
    REQUIRE(dst.m_entityHandles[1] == 0x801u);
    REQUIRE(dst.m_entityHandles[2] == 0x802u);
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0xCu);
    // NOTE: members are now emitted as HARD pointers (code 4) per
    // ODA §20.4.72 rather than soft owners (code 3).  getOffsetHandle ignores
    // the reference code for codes <= 5, so this round-trip recovers identical
    // refs either way — it is the regression net proving the code-4 change did
    // not disturb the handle-stream structure.  The on-wire code value itself is
    // not asserted here: the handle stream is bit-packed (not byte-aligned), so a
    // raw byte-scan is unreliable, and only external consumers (AutoCAD) read it.
}

TEST_CASE("DRW_Group captures common handle references",
          "[dwg-read][object-encode][group]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/72, /*handle=*/0xA60,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "Group");
    encoded.putBitShort(1);
    encoded.putBitShort(0);
    encoded.putBitLong(2);
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(hardPtr(0xA1));
    encoded.putHandle(hardPtr(0xB1));
    encoded.putHandle(hardPtr(0xC1));
    encoded.putHandle(hardPtr(0xC2));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Group parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_description == "Group");
    CHECK(parsed.m_isUnnamed);
    CHECK_FALSE(parsed.m_selectable);
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x1Fu);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0xA1u);
    CHECK(parsed.xDictHandle == 0xB1u);
    REQUIRE(parsed.m_entityHandles.size() == 2u);
    CHECK(parsed.m_entityHandles[0] == 0xC1u);
    CHECK(parsed.m_entityHandles[1] == 0xC2u);

    parsed.m_description = "stale";
    parsed.m_entityHandles.push_back(0xDDu);
    auto truncated = bytes;
    truncated.pop_back();
    dwgBuffer truncatedReader(truncated.data(), truncated.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK(parsed.m_description.empty());
    CHECK(parsed.m_entityHandles.empty());
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
    dwgBufferW oversized;
    emitObjectPreamble(oversized, ver, /*oType=*/72, /*handle=*/0xA63,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    oversized.putVariableText(ver, "Group");
    oversized.putBitShort(0);
    oversized.putBitShort(1);
    oversized.putBitLong(100001);
    auto oversizedBytes = snapshot(oversized);
    dwgBuffer oversizedReader(oversizedBytes.data(), oversizedBytes.size());
    DRW_Group oversizedParsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        oversizedParsed, ver, &oversizedReader));
    CHECK_FALSE(oversizedReader.isGood());

    dwgBufferW physicallyTruncated;
    emitObjectPreamble(physicallyTruncated, ver, /*oType=*/1305,
                       /*handle=*/0xB23, /*numReactors=*/0,
                       /*xDictFlag=*/1);
    physicallyTruncated.putRawChar8(0);
    physicallyTruncated.putBitLong(static_cast<std::int32_t>(
        DRW_IDBuffer::kMaxObjectIds));
    emitCommonHandlePrefix(physicallyTruncated, /*parentHandle=*/0,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);
    auto physicallyTruncatedBytes = snapshot(physicallyTruncated);
    dwgBuffer physicallyTruncatedReader(physicallyTruncatedBytes.data(),
                                         physicallyTruncatedBytes.size());
    DRW_IDBuffer physicallyTruncatedParsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        physicallyTruncatedParsed, ver, &physicallyTruncatedReader));
    CHECK_FALSE(physicallyTruncatedReader.isGood());
    CHECK(physicallyTruncatedParsed.objIds.empty());
}

TEST_CASE("DRW_Group rejects oversized entity handle lists before encoding",
          "[dwg-write][object-encode][group][safety]") {
    DRW_Group group;
    group.m_entityHandles.resize(DRW_Group::kMaxEntityHandles + 1);

    dwgBufferW encoded;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeGroup(
        group, DRW::AC1027, &encoded));
    CHECK(encoded.data().empty());
}

TEST_CASE("DRW_Group rejects a truncated common handle stream",
          "[dwg-read][object-encode][group]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/72, /*handle=*/0xA61,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "Group");
    encoded.putBitShort(0);
    encoded.putBitShort(1);
    encoded.putBitLong(0);
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(hardPtr(0xA1));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Group parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
}

TEST_CASE("DRW_Group rejects a truncated member handle",
          "[dwg-read][object-encode][group]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/72, /*handle=*/0xA62,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putVariableText(ver, "Group");
    encoded.putBitShort(0);
    encoded.putBitShort(1);
    encoded.putBitLong(1);
    encoded.putHandle(hardPtr(0x1F));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Group parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_entityHandles.empty());
}

TEST_CASE("Point-cloud object frames roll back on truncation",
          "[dwg-read][object-encode][pointcloud][safety]") {
    const DRW::Version ver = DRW::AC1018;

    DRW_PointCloudDef definition;
    definition.m_kind = DRW_PointCloudDef::Definition;
    definition.parentHandle = 0x41;
    definition.m_classVersion = 2;
    definition.m_sourceFilename = "scan.rcp";
    definition.m_isLoaded = true;
    definition.m_pointCount = 42;
    definition.m_extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
    definition.m_extentsMax = DRW_Coord{4.0, 5.0, 6.0};
    DrwObjectEncodeTestAccess::setNumReactors(definition, 1);
    DrwObjectEncodeTestAccess::setXDictFlag(definition, 0);
    definition.reactorHandles = {0x42};
    definition.xDictHandle = 0x43;
    dwgBufferW definitionWriter;
    emitObjectPreamble(definitionWriter, ver, 0, 0x701, 1, 0);
    REQUIRE(DrwObjectEncodeTestAccess::encodePointCloudDef(
        definition, ver, &definitionWriter));
    auto definitionBytes = snapshot(definitionWriter);

    DRW_PointCloudDef parsedDefinition;
    parsedDefinition.m_kind = definition.m_kind;
    dwgBuffer definitionReader(definitionBytes.data(), definitionBytes.size());
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsedDefinition, ver, &definitionReader));
    CHECK(parsedDefinition.parentHandle == 0x41);
    CHECK(parsedDefinition.m_sourceFilename == "scan.rcp");
    CHECK(parsedDefinition.m_pointCount == 42u);
    CHECK(parsedDefinition.m_extentsMax.z == Approx(6.0));

    auto definitionTruncated = definitionBytes;
    definitionTruncated.pop_back();
    parsedDefinition.m_sourceFilename = "stale.rcp";
    parsedDefinition.m_pointCount = 99;
    parsedDefinition.parentHandle = 0x99;
    parsedDefinition.reactorHandles.push_back(0x98);
    parsedDefinition.xDictHandle = 0x97;
    dwgBuffer definitionFailureReader(definitionTruncated.data(),
                                      definitionTruncated.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsedDefinition, ver, &definitionFailureReader));
    CHECK(parsedDefinition.m_kind == DRW_PointCloudDef::Definition);
    CHECK(parsedDefinition.m_sourceFilename.empty());
    CHECK(parsedDefinition.m_pointCount == 0u);
    CHECK(parsedDefinition.parentHandle == 0);
    CHECK(parsedDefinition.reactorHandles.empty());
    CHECK(parsedDefinition.xDictHandle == 0u);
    CHECK_FALSE(definitionFailureReader.isGood());

    DRW_NavisworksModelDef navisworks;
    navisworks.parentHandle = 0x51;
    navisworks.m_flags = 5;
    navisworks.m_path = "coordination/model.nwd";
    navisworks.m_status = true;
    navisworks.m_minExtent = DRW_Coord{-1.0, -2.0, -3.0};
    navisworks.m_maxExtent = DRW_Coord{4.0, 5.0, 6.0};
    navisworks.m_hostDrawingVisibility = false;
    DrwObjectEncodeTestAccess::setNumReactors(navisworks, 1);
    DrwObjectEncodeTestAccess::setXDictFlag(navisworks, 0);
    navisworks.reactorHandles = {0x52};
    navisworks.xDictHandle = 0x53;
    dwgBufferW navisworksWriter;
    emitObjectPreamble(navisworksWriter, ver, 0, 0x702, 1, 0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeNavisworksModelDef(
        navisworks, ver, &navisworksWriter));
    auto navisworksBytes = snapshot(navisworksWriter);

    DRW_NavisworksModelDef parsedNavisworks;
    dwgBuffer navisworksReader(navisworksBytes.data(), navisworksBytes.size());
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsedNavisworks, ver, &navisworksReader));
    CHECK(parsedNavisworks.parentHandle == 0x51);
    CHECK(parsedNavisworks.m_path == navisworks.m_path);
    CHECK(parsedNavisworks.m_maxExtent.y == Approx(5.0));
    auto navisworksTruncated = navisworksBytes;
    navisworksTruncated.pop_back();
    parsedNavisworks.m_path = "stale.nwd";
    parsedNavisworks.m_flags = 99;
    parsedNavisworks.parentHandle = 0x99;
    dwgBuffer navisworksFailureReader(navisworksTruncated.data(),
                                      navisworksTruncated.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsedNavisworks, ver, &navisworksFailureReader));
    CHECK_FALSE(navisworksFailureReader.isGood());
    CHECK(parsedNavisworks.m_path.empty());
    CHECK(parsedNavisworks.m_flags == 0);
    CHECK(parsedNavisworks.parentHandle == 0);
    CHECK(parsedNavisworks.reactorHandles.empty());
    CHECK(parsedNavisworks.xDictHandle == 0u);

    DRW_PointCloudColorMap colorMap;
    colorMap.parentHandle = 0x61;
    colorMap.m_classVersion = 40000;
    colorMap.m_defaultIntensityColorScheme = "Intensity";
    colorMap.m_defaultElevationColorScheme = "Elevation";
    colorMap.m_defaultClassificationColorScheme = "Classification";
    colorMap.m_colorRamps.push_back({40001, 2, {"red", "blue"}});
    colorMap.m_colorRampCount = 1;
    colorMap.m_classificationColorRamps.push_back({3, 1, {"green"}});
    colorMap.m_classificationColorRampCount = 1;
    DrwObjectEncodeTestAccess::setNumReactors(colorMap, 1);
    DrwObjectEncodeTestAccess::setXDictFlag(colorMap, 0);
    colorMap.reactorHandles = {0x62};
    colorMap.xDictHandle = 0x63;
    dwgBufferW colorMapWriter;
    emitObjectPreamble(colorMapWriter, ver, 0, 0x703, 1, 0);
    REQUIRE(DrwObjectEncodeTestAccess::encodePointCloudColorMap(
        colorMap, ver, &colorMapWriter));
    auto colorMapBytes = snapshot(colorMapWriter);

    DRW_PointCloudColorMap parsedColorMap;
    dwgBuffer colorMapReader(colorMapBytes.data(), colorMapBytes.size());
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsedColorMap, ver, &colorMapReader));
    CHECK(parsedColorMap.parentHandle == 0x61);
    CHECK(parsedColorMap.m_classVersion == 40000);
    CHECK(parsedColorMap.m_colorRamps.size() == 1u);
    CHECK(parsedColorMap.m_colorRamps[0].m_classVersion == 40001);
    CHECK(parsedColorMap.m_colorRamps[0].m_colorSchemes[1] == "blue");
    auto colorMapTruncated = colorMapBytes;
    colorMapTruncated.pop_back();
    parsedColorMap.m_defaultIntensityColorScheme = "stale";
    parsedColorMap.m_colorRamps.push_back({});
    parsedColorMap.parentHandle = 0x99;
    dwgBuffer colorMapFailureReader(colorMapTruncated.data(),
                                    colorMapTruncated.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsedColorMap, ver, &colorMapFailureReader));
    CHECK_FALSE(colorMapFailureReader.isGood());
    CHECK(parsedColorMap.m_defaultIntensityColorScheme.empty());
    CHECK(parsedColorMap.m_colorRamps.empty());
    CHECK(parsedColorMap.m_classificationColorRamps.empty());
    CHECK(parsedColorMap.parentHandle == 0);
    CHECK(parsedColorMap.reactorHandles.empty());
    CHECK(parsedColorMap.xDictHandle == 0u);
}

TEST_CASE("DRW_NavisworksModelDef rejects non-finite extents",
          "[dwg-write][object-encode][navisworks][safety]") {
    DRW_NavisworksModelDef source;
    source.m_minExtent.x = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW minExtentBody;
    minExtentBody.putRawChar8(0xA5);
    const auto minExtentBytes = snapshot(minExtentBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeNavisworksModelDef(
        source, DRW::AC1018, &minExtentBody));
    CHECK(minExtentBody.data() == minExtentBytes);

    source.m_minExtent.x = 0.0;
    source.m_maxExtent.z = std::numeric_limits<double>::infinity();
    dwgBufferW maxExtentBody;
    maxExtentBody.putRawChar8(0x5A);
    const auto maxExtentBytes = snapshot(maxExtentBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeNavisworksModelDef(
        source, DRW::AC1018, &maxExtentBody));
    CHECK(maxExtentBody.data() == maxExtentBytes);
}

TEST_CASE("DRW_PointCloudDef rejects invalid definition fields",
          "[dwg-write][object-encode][pointcloud][safety]") {
    DRW_PointCloudDef source;
    source.m_kind = DRW_PointCloudDef::Definition;
    source.m_extentsMin.x = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW encoded;
    encoded.putRawChar8(0xA5);
    const auto before = snapshot(encoded);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePointCloudDef(
        source, DRW::AC1018, &encoded));
    CHECK(snapshot(encoded) == before);

    source.m_extentsMin.x = 0.0;
    source.m_extentsMax.z = std::numeric_limits<double>::infinity();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePointCloudDef(
        source, DRW::AC1018, &encoded));
    CHECK(snapshot(encoded) == before);

    source.m_extentsMax.z = 0.0;
    source.m_kind = static_cast<DRW_PointCloudDef::Kind>(99);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePointCloudDef(
        source, DRW::AC1018, &encoded));
    CHECK(snapshot(encoded) == before);
}

TEST_CASE("DRW_PointCloudColorMap rejects class versions outside DWG BS",
          "[dwg-write][object-encode][pointcloud][safety]") {
    DRW_PointCloudColorMap source;
    source.m_classVersion = DRW_PointCloudColorMap::kMaxClassVersion + 1;
    dwgBufferW mapBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePointCloudColorMap(
        source, DRW::AC1018, &mapBody));
    CHECK(mapBody.data().empty());

    source.m_classVersion = 0;
    source.m_colorRamps.push_back({
        DRW_PointCloudColorMap::kMaxClassVersion + 1, 0, {}});
    source.m_colorRampCount = 1;
    dwgBufferW rampBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePointCloudColorMap(
        source, DRW::AC1018, &rampBody));
    CHECK(rampBody.data().empty());
}

TEST_CASE("Point-cloud color-map rejects counts beyond the body budget",
          "[dwg-read][object-encode][pointcloud][safety]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0x704,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitShort(1); // class version
    encoded.putVariableText(ver, "");
    encoded.putVariableText(ver, "");
    encoded.putVariableText(ver, "");
    encoded.putBitLong(4096); // no body remains for the declared ramps

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_PointCloudColorMap parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_colorRamps.empty());
    CHECK(parsed.m_classificationColorRamps.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_DataTable captures common handle references",
          "[dwg-read][object-encode][datatable]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/520, /*handle=*/0xA70,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitShort(2);
    encoded.putBitLong(0);
    encoded.putBitLong(0);
    encoded.putVariableText(ver, "");
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(handleWithCode(0x0A, 0xA1));
    encoded.putHandle(handleWithCode(0x0A, 0xB1));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DataTable parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.flags == 2);
    CHECK(parsed.columnCount == 0);
    CHECK(parsed.rowCount == 0);
    CHECK(parsed.tableName.empty());
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x1Fu);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0xA1u);
    CHECK(parsed.xDictHandle == 0xB1u);
}

TEST_CASE("DRW_DataTable rejects a truncated common handle stream",
          "[dwg-read][object-encode][datatable]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/520, /*handle=*/0xA71,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitShort(0);
    encoded.putBitLong(0);
    encoded.putBitLong(0);
    encoded.putVariableText(ver, "");
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(hardPtr(0xA1));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DataTable parsed;
    parsed.flags = 99;
    parsed.tableName = "stale";
    parsed.parentHandle = 0x42;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.flags == 0);
    CHECK(parsed.tableName.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.columns.empty());
}

TEST_CASE("DRW_DataTable soft-fails impossible dimensions before nested allocation",
          "[dwg-read][object-encode][datatable][safety]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/520, /*handle=*/0xA72,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitShort(0);
    encoded.putBitLong(static_cast<std::int32_t>(DRW_Field::kMaxItems));
    encoded.putBitLong(static_cast<std::int32_t>(DRW_Field::kMaxItems));
    encoded.putVariableText(ver, "");
    encoded.putHandle(nullHandle()); // common parent handle

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DataTable parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(reader.isGood());
    CHECK(parsed.columnCount == static_cast<std::int32_t>(DRW_Field::kMaxItems));
    CHECK(parsed.rowCount == static_cast<std::int32_t>(DRW_Field::kMaxItems));
    CHECK(parsed.columns.empty());

    dwgBufferW malformed;
    emitObjectPreamble(malformed, ver, /*oType=*/520, /*handle=*/0xA74,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    malformed.putBitShort(0); // flags
    malformed.putBitLong(1); // one column
    malformed.putBitLong(1); // one row
    malformed.putVariableText(ver, "table");
    malformed.putBitLong(0); // column type
    malformed.putVariableText(ver, "column");
    malformed.putBitLong(0); // row integer value
    malformed.putBitDouble(std::numeric_limits<double>::quiet_NaN());
    auto malformedBytes = snapshot(malformed);
    dwgBuffer malformedReader(malformedBytes.data(), malformedBytes.size());
    DRW_DataTable rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, ver, &malformedReader));
    CHECK_FALSE(malformedReader.isGood());
    CHECK(rejected.columnCount == 0);
    CHECK(rejected.columns.empty());
}

TEST_CASE("DRW_DataTable keeps detached strings out of the body stream",
          "[dwg-read][object-encode][datatable][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/520, /*handle=*/0xA73,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    data.putBitShort(0); // flags
    data.putBitLong(1);  // one column, but omit its body record
    data.putBitLong(0);  // no rows
    strings.putVariableText(version, "table");
    strings.putVariableText(version, "column");

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    REQUIRE(stringBitSize <= std::numeric_limits<std::uint16_t>::max());
    data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    data.putBit(1); // detached strings are present
    data.alignToByte();
    data.patchRawLong32AtBit(
        objectSizeBitOffset(data), static_cast<std::uint32_t>(data.size() * 8u));

    emitObjectHandlePrefix(handles, /*parentHandle=*/0,
                           /*reactorHandles=*/{}, /*xDictHandle=*/0,
                           /*xDictFlag=*/1);
    handles.alignToByte();
    data.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(data);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_DataTable parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, version, &reader,
        static_cast<std::uint32_t>(handles.size() * 8u)));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.tableName.empty());
    CHECK(parsed.columnCount == 0);
    CHECK(parsed.columns.empty());
}

TEST_CASE("DRW_Group rejects an impossible member count before allocation",
          "[dwg-read][object-encode][group][safety]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0xA82,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putVariableText(ver, "group");
    encoded.putBitShort(0); // unnamed
    encoded.putBitShort(1); // selectable
    encoded.putBitLong(static_cast<std::int32_t>(DRW_Group::kMaxEntityHandles));
    encoded.putHandle(nullHandle()); // common parent handle

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Group parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_entityHandles.empty());
}

TEST_CASE("DRW_NavisworksModelDef reads bounded model metadata",
          "[dwg-read][object-encode][navisworks][safety]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/1150, /*handle=*/0xB50,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitShort(5);
    encoded.putVariableText(ver, "coordination/model.nwd");
    encoded.putBit(1);
    encoded.put3BitDouble(DRW_Coord{-1.0, -2.0, -3.0});
    encoded.put3BitDouble(DRW_Coord{4.0, 5.0, 6.0});
    encoded.putBit(0);
    const std::uint32_t bodyEndBit = encoded.bitCount();
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_NavisworksModelDef parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_flags == 5);
    CHECK(parsed.m_path == "coordination/model.nwd");
    CHECK(parsed.m_status);
    CHECK_FALSE(parsed.m_hostDrawingVisibility);
    CHECK(parsed.m_minExtent.x == Approx(-1.0));
    CHECK(parsed.m_maxExtent.z == Approx(6.0));
    CHECK(parsed.parentHandle == 0x42u);

    dwgBufferW truncated;
    emitObjectPreamble(truncated, ver, /*oType=*/1150, /*handle=*/0xB51,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    truncated.putBitShort(5);
    truncated.putVariableText(ver, "coordination/model.nwd");
    truncated.putBit(1);
    truncated.put3BitDouble(DRW_Coord{-1.0, -2.0, -3.0});
    const std::uint32_t truncatedBodyEnd = truncated.bitCount();
    emitCommonHandlePrefix(truncated, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);
    truncated.patchRawLong32AtBit(objectSizeBitOffset(truncated),
                                   truncatedBodyEnd);
    auto truncatedBytes = snapshot(truncated);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    parsed.m_path = "stale";
    parsed.parentHandle = 0x99;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_path.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_TableStyle reads the legacy body before its handle stream",
          "[dwg-read][object-encode][tablestyle]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/602, /*handle=*/0xB00,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "LegacyStyle");
    encoded.putBitShort(1);          // flow direction
    encoded.putBitShort(2);          // flags
    encoded.putBitDouble(1.5);       // horizontal margin
    encoded.putBitDouble(2.5);       // vertical margin
    encoded.putBit(0);               // title suppressed
    encoded.putBit(1);               // header suppressed
    emitLegacyTableStyleRow(encoded, ver, 10);
    emitLegacyTableStyleRow(encoded, ver, 20);
    emitLegacyTableStyleRow(encoded, ver, 30);

    encoded.putHandle(hardPtr(0x42));
    encoded.putHandle(handleWithCode(0x0A, 0x50));
    encoded.putHandle(handleWithCode(0x0A, 0x63));
    encoded.putHandle(hardPtr(0x60));
    encoded.putHandle(hardPtr(0x61));
    encoded.putHandle(hardPtr(0x62));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_TableStyle parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_name == "LegacyStyle");
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x42u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0x50u);
    CHECK(parsed.xDictHandle == 0x63u);
    REQUIRE(parsed.m_rowStyles.size() == 3u);
    CHECK(parsed.m_rowStyles[0].m_textStyleHandle == 0x60u);
    CHECK(parsed.m_rowStyles[1].m_textStyleHandle == 0x61u);
    CHECK(parsed.m_rowStyles[2].m_textStyleHandle == 0x62u);

    dwgBufferW malformed;
    emitObjectPreamble(malformed, ver, /*oType=*/602, /*handle=*/0xB01,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    malformed.putVariableText(ver, "MalformedStyle");
    malformed.putBitShort(1); // flow direction
    malformed.putBitShort(2); // flags
    malformed.putBitDouble(std::numeric_limits<double>::quiet_NaN());
    auto malformedBytes = snapshot(malformed);
    dwgBuffer malformedReader(malformedBytes.data(), malformedBytes.size());
    DRW_TableStyle rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, ver, &malformedReader));
    CHECK_FALSE(malformedReader.isGood());
    CHECK(rejected.m_name.empty());
    CHECK(rejected.m_rowStyles.empty());
}

TEST_CASE("DRW_CellStyleMap captures common handles after its body",
          "[dwg-read][object-encode][cellstylemap]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/603, /*handle=*/0xB10,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(0);           // num_cells

    encoded.putHandle(hardPtr(0x70));
    encoded.putHandle(handleWithCode(0x0A, 0x71));
    encoded.putHandle(handleWithCode(0x0A, 0x72));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_CellStyleMap parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_cellStyles.empty());
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x70u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0x71u);
    CHECK(parsed.xDictHandle == 0x72u);
}

TEST_CASE("DRW_CellStyleMap rejects a count outside its body frame",
          "[dwg-read][object-encode][cellstylemap][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/603, /*handle=*/0xB12,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    const auto bodyEnd = static_cast<std::uint32_t>(encoded.bitCount());
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x70, {}, 1);
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEnd);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_CellStyleMap parsed;
    REQUIRE_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_cellStyles.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_CellStyleMap rejects a truncated common handle stream",
          "[dwg-read][object-encode][cellstylemap]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/603, /*handle=*/0xB11,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(0);           // num_cells

    encoded.putHandle(hardPtr(0x70)); // parent only; reactor is truncated

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_CellStyleMap parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_cellStyles.empty());
}

TEST_CASE("DRW_IDBuffer captures common and object handles",
          "[dwg-read][object-encode][idbuffer]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/1305, /*handle=*/0xB20,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putRawChar8(0);
    encoded.putBitLong(2);
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{0x50}, /*xDictFlag=*/0);
    encoded.putHandle(hardPtr(0x60));
    encoded.putHandle(hardPtr(0x61));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_IDBuffer parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.classVersion == 0);
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x42u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0x50u);
    CHECK(parsed.xDictHandle == 0u);
    REQUIRE(parsed.objIds.size() == 2u);
    CHECK(parsed.objIds[0] == 0x60u);
    CHECK(parsed.objIds[1] == 0x61u);
}

TEST_CASE("DRW_IDBuffer rejects a truncated object handle stream",
          "[dwg-read][object-encode][idbuffer]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/1305, /*handle=*/0xB21,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putRawChar8(0);
    encoded.putBitLong(1);
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_IDBuffer parsed;
    parsed.classVersion = 7;
    parsed.objIds.push_back(0xDEADu);
    parsed.parentHandle = 0xBEEFu;
    parsed.reactorHandles.push_back(0xCAFEu);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.classVersion == 0);
    CHECK(parsed.objIds.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());

    dwgBufferW oversized;
    emitObjectPreamble(oversized, ver, /*oType=*/1305, /*handle=*/0xB22,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    oversized.putRawChar8(0);
    oversized.putBitLong(100001);
    auto oversizedBytes = snapshot(oversized);
    dwgBuffer oversizedReader(oversizedBytes.data(), oversizedBytes.size());
    DRW_IDBuffer oversizedParsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        oversizedParsed, ver, &oversizedReader));
    CHECK_FALSE(oversizedReader.isGood());
}

TEST_CASE("DRW_LayerIndex captures entries and their handles",
          "[dwg-read][object-encode][layerindex]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/1009, /*handle=*/0xB30,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(123);
    encoded.putBitLong(456);
    encoded.putBitLong(2);
    encoded.putBitLong(7);
    encoded.putVariableText(ver, "A");
    encoded.putBitLong(8);
    encoded.putVariableText(ver, "B");
    const std::uint32_t bodyEndBit = encoded.bitCount();
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{0x50}, /*xDictFlag=*/0);
    encoded.putHandle(hardPtr(0x60));
    encoded.putHandle(hardPtr(0x61));
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LayerIndex parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.timestamp1 == 123u);
    CHECK(parsed.timestamp2 == 456u);
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x42u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0x50u);
    REQUIRE(parsed.entries.size() == 2u);
    CHECK(parsed.entries[0].indexLong == 7);
    CHECK(parsed.entries[0].name == "A");
    CHECK(parsed.entries[0].entryHandle == 0x60u);
    CHECK(parsed.entries[1].indexLong == 8);
    CHECK(parsed.entries[1].name == "B");
    CHECK(parsed.entries[1].entryHandle == 0x61u);
}

TEST_CASE("DRW_LayerIndex rejects a truncated entry handle stream",
          "[dwg-read][object-encode][layerindex]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/1009, /*handle=*/0xB31,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(123);
    encoded.putBitLong(456);
    encoded.putBitLong(1);
    encoded.putBitLong(7);
    encoded.putVariableText(ver, "A");
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_LayerIndex parsed;
    parsed.timestamp1 = 900u;
    parsed.timestamp2 = 901u;
    parsed.entries.push_back({99, "stale", 0xDEADu});
    parsed.parentHandle = 0xBEEFu;
    parsed.reactorHandles.push_back(0xCAFEu);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.timestamp1 == 0u);
    CHECK(parsed.timestamp2 == 0u);
    CHECK(parsed.entries.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());

    dwgBufferW oversized;
    emitObjectPreamble(oversized, ver, /*oType=*/1009, /*handle=*/0xB32,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    oversized.putBitLong(0);
    oversized.putBitLong(0);
    oversized.putBitLong(100001);
    auto oversizedBytes = snapshot(oversized);
    dwgBuffer oversizedReader(oversizedBytes.data(), oversizedBytes.size());
    DRW_LayerIndex oversizedParsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        oversizedParsed, ver, &oversizedReader));
    CHECK_FALSE(oversizedReader.isGood());
}

TEST_CASE("DRW_Material captures common handle references",
          "[dwg-read][object-encode][material]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/504, /*handle=*/0xB40,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "Material");
    encoded.putVariableText(ver, "Description");
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{0x50}, /*xDictFlag=*/0);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Material parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.m_name == "Material");
    CHECK(parsed.m_description == "Description");
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x42u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0x50u);
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("DRW_Material rejects a truncated common handle stream",
          "[dwg-read][object-encode][material]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/504, /*handle=*/0xB41,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putVariableText(ver, "Material");
    encoded.putVariableText(ver, "Description");
    encoded.putHandle(hardPtr(0x42));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Material parsed;
    parsed.m_name = "stale material";
    parsed.m_description = "stale description";
    parsed.parentHandle = 0xDEADu;
    parsed.reactorHandles.push_back(0xCAFEu);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_name.empty());
    CHECK(parsed.m_description.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
}

TEST_CASE("DRW_Material rejects a truncated text field",
          "[dwg-read][object-encode][material][safety]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/504, /*handle=*/0xB42,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.put2Bits(0);       // variable-text length uses a 16-bit count
    encoded.putRawShort16(0xFFFF); // declared payload exceeds the frame

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Material parsed;
    parsed.m_name = "stale material";
    parsed.m_description = "stale description";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_name.empty());
    CHECK(parsed.m_description.empty());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
}

TEST_CASE("DRW_Material reads AC1024 detached text fields",
          "[dwg-read][dwg-write][object-encode][material]") {
    const DRW::Version ver = DRW::AC1024;
    DRW_Material source;
    source.handle = 0xB43u;
    source.parentHandle = 0x42u;
    source.reactorHandles = {0x44u};
    source.xDictHandle = 0x43u;
    source.m_name = "Material";
    source.m_description = "AC1024 description";
    source.setDwgCommonObjectState(1, 0, false);
    REQUIRE(source.setDwgRawData({0xA5u, 0x3Cu}, 16, ver));

    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, ver, DRW_Material::kDwgClassNum, source.handle,
                       source.reactorCount(), source.extensionDictionaryFlag());
    REQUIRE(DrwObjectEncodeTestAccess::encodeMaterial(
        source, ver, &data, &strings, &handles));

    data.alignToByte();
    strings.alignToByte();
    const std::uint32_t stringBytes =
        static_cast<std::uint32_t>(strings.data().size());
    REQUIRE(stringBytes > 0u);
    data.putBytes(strings.data().data(), stringBytes);
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize = stringBytes * 8u + 7u;
    REQUIRE(stringBitSize <= std::numeric_limits<std::uint16_t>::max());
    data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    data.putBit(1); // detached strings are present
    data.alignToByte();

    handles.alignToByte();
    const std::uint32_t handleBits =
        static_cast<std::uint32_t>(handles.data().size() * 8u);
    auto bytes = snapshot(data);
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());

    DRW_TextCodec decoder;
    decoder.setVersion(ver, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_Material parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader, handleBits));
    CHECK(parsed.m_name == source.m_name);
    CHECK(parsed.m_description == source.m_description);
    CHECK(parsed.parentHandle == source.parentHandle);
    CHECK(parsed.reactorHandles == source.reactorHandles);
    CHECK(parsed.xDictHandle == source.xDictHandle);
    REQUIRE(parsed.hasDwgRawData());
    CHECK(parsed.dwgRawDataBitSize() >= 16u);
    REQUIRE(parsed.dwgRawData().size() >= 2u);
    CHECK(parsed.dwgRawData()[0] == 0xA5u);
    CHECK(parsed.dwgRawData()[1] == 0x3Cu);

    dwgBufferW replayData;
    dwgBufferW replayStrings;
    dwgBufferW replayHandles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeMaterial(
        parsed, ver, &replayData, &replayStrings, &replayHandles));
    CHECK(replayData.bitCount() > 0u);

    dwgBufferW crossVersion;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMaterial(
        parsed, DRW::AC1027, &crossVersion));
    CHECK(crossVersion.data().empty());
}

TEST_CASE("DRW_Background DWG variants round-trip their typed fields",
          "[dwg-write][object-encode][background]") {
    const DRW::Version ver = DRW::AC1018;
    const auto roundTrip = [ver](const DRW_Background& source) {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, ver, /*oType=*/0, source.handle,
                           source.reactorCount(), source.extensionDictionaryFlag());
        REQUIRE(DrwObjectEncodeTestAccess::encodeBackground(
            source, ver, &encoded));
        auto bytes = snapshot(encoded);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_Background parsed;
        parsed.m_kind = source.m_kind;
        REQUIRE(DrwObjectEncodeTestAccess::parseBackground(
            parsed, ver, &reader));
        REQUIRE(reader.isGood());
        return parsed;
    };

    DRW_Background solid;
    solid.m_kind = DRW_Background::Solid;
    solid.handle = 0xB50;
    solid.parentHandle = 0x42;
    solid.reactorHandles = {0x44};
    solid.xDictHandle = 0x43;
    solid.setDwgCommonObjectState(1, 0, false);
    solid.m_classVersion = 1;
    solid.m_solidColor = 0x336699;
    const DRW_Background parsedSolid = roundTrip(solid);
    CHECK(parsedSolid.m_solidColor == 0x336699);
    CHECK(parsedSolid.parentHandle == 0x42);
    REQUIRE(parsedSolid.reactorHandles.size() == 1u);
    CHECK(parsedSolid.reactorHandles.front() == 0x44u);
    CHECK(parsedSolid.xDictHandle == 0x43u);

    DRW_Background gradient = solid;
    gradient.m_kind = DRW_Background::Gradient;
    gradient.handle = 0xB51;
    gradient.m_colorTop = 0x112233;
    gradient.m_colorMiddle = 0x445566;
    gradient.m_colorBottom = 0x778899;
    gradient.m_horizon = 0.25;
    gradient.m_height = 0.75;
    gradient.m_rotation = 45.0;
    const DRW_Background parsedGradient = roundTrip(gradient);
    CHECK(parsedGradient.m_colorTop == 0x112233);
    CHECK(parsedGradient.m_colorMiddle == 0x445566);
    CHECK(parsedGradient.m_colorBottom == 0x778899);
    CHECK(parsedGradient.m_rotation == Catch::Approx(45.0));

    DRW_Background ground = solid;
    ground.m_kind = DRW_Background::GroundPlane;
    ground.handle = 0xB52;
    ground.m_colorSkyZenith = 0x010203;
    ground.m_colorSkyHorizon = 0x040506;
    ground.m_colorUndergroundHorizon = 0x070809;
    ground.m_colorUndergroundAzimuth = 0x0A0B0C;
    ground.m_colorNear = 0x0D0E0F;
    ground.m_colorFar = 0x101112;
    const DRW_Background parsedGround = roundTrip(ground);
    CHECK(parsedGround.m_colorSkyZenith == 0x010203);
    CHECK(parsedGround.m_colorFar == 0x101112);

    DRW_Background image = solid;
    image.m_kind = DRW_Background::Image;
    image.handle = 0xB53;
    image.m_fileName = "sky.hdr";
    image.m_fitToScreen = true;
    image.m_maintainAspect = true;
    image.m_useTiling = false;
    image.m_offset = DRW_Coord{0.1, 0.2, 0.0};
    image.m_scale = DRW_Coord{1.5, 2.5, 0.0};
    const DRW_Background parsedImage = roundTrip(image);
    CHECK(parsedImage.m_fileName == "sky.hdr");
    CHECK(parsedImage.m_fitToScreen);
    CHECK(parsedImage.m_offset.x == Catch::Approx(0.1));
    CHECK(parsedImage.m_scale.y == Catch::Approx(2.5));

    DRW_Background ibl = solid;
    ibl.m_kind = DRW_Background::Ibl;
    ibl.handle = 0xB54;
    ibl.m_enabled = true;
    ibl.m_iblName = "Studio IBL";
    ibl.m_rotation = 15.0;
    ibl.m_displayImage = false;
    ibl.m_secondaryBackgroundHandle = 0x66;
    const DRW_Background parsedIbl = roundTrip(ibl);
    CHECK(parsedIbl.m_enabled);
    CHECK(parsedIbl.m_iblName == "Studio IBL");
    CHECK(parsedIbl.m_secondaryBackgroundHandle == 0x66);

    DRW_Background skylight = solid;
    skylight.m_kind = DRW_Background::Skylight;
    skylight.handle = 0xB55;
    skylight.m_sunHandle = 0x77;
    const DRW_Background parsedSkylight = roundTrip(skylight);
    CHECK(parsedSkylight.m_sunHandle == 0x77);
}

TEST_CASE("DRW_Background rejects a truncated common handle stream",
          "[dwg-read][object-encode][background][safety]") {
    DRW_Background source;
    source.m_kind = DRW_Background::Solid;
    source.handle = 0xB56;
    source.m_solidColor = 0x123456;
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, DRW::AC1018, /*oType=*/0, source.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeBackground(
        source, DRW::AC1018, &encoded));
    auto bytes = snapshot(encoded);
    REQUIRE(bytes.size() > 1);
    bytes.pop_back();

    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Background parsed;
    parsed.m_kind = DRW_Background::Solid;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseBackground(
        parsed, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
}

TEST_CASE("DRW_Background enforces the AC1024 body boundary",
          "[dwg-read][object-encode][background][safety]") {
    const DRW::Version version = DRW::AC1024;
    DRW_Background source;
    source.m_kind = DRW_Background::Solid;
    source.handle = 0xB57u;
    source.parentHandle = 0x42u;
    source.m_classVersion = 1;
    source.m_solidColor = 0x336699;
    source.setDwgCommonObjectState(0, 1, false);

    dwgBufferW data;
    emitObjectPreamble(data, version, DRW_Background::kDwgClassNumSolid,
                       source.handle, 0, source.extensionDictionaryFlag());
    dwgBufferW strings;
    dwgBufferW handles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeBackground(
        source, version, &data, &strings, &handles));
    const std::uint32_t bodyFieldEndBit = data.bitCount();
    data.alignToByte();
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    data.putRawShort16(0); // empty detached string stream
    data.putBit(0);        // no detached strings
    data.alignToByte();
    const std::uint32_t bodyEndBit = data.bitCount();
    handles.alignToByte();
    data.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(data);
    const std::uint32_t handleBits =
        static_cast<std::uint32_t>(handles.data().size() * 8u);
    dwgBuffer validReader(bytes.data(), bytes.size());
    DRW_Background valid;
    valid.m_kind = DRW_Background::Solid;
    REQUIRE(DrwObjectEncodeTestAccess::parseBackground(
        valid, version, &validReader, handleBits));
    CHECK(valid.m_classVersion == source.m_classVersion);
    CHECK(valid.m_solidColor == source.m_solidColor);

    REQUIRE(bodyEndBit >= bodyFieldEndBit);
    REQUIRE(handleBits <= std::numeric_limits<std::uint32_t>::max()
            - (bodyEndBit - bodyFieldEndBit) - 1u);
    const std::uint32_t malformedHandleBits =
        handleBits + (bodyEndBit - bodyFieldEndBit) + 1u;
    dwgBuffer malformedReader(bytes.data(), bytes.size());
    DRW_Background malformed;
    malformed.m_kind = DRW_Background::Solid;
    malformed.m_solidColor = 0xFFFFFF;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseBackground(
        malformed, version, &malformedReader, malformedHandleBits));
    CHECK_FALSE(malformedReader.isGood());
    CHECK(malformed.m_classVersion == 0);
    CHECK(malformed.m_solidColor == 0);
}

TEST_CASE("DRW_Background rejects invalid variant fields",
          "[dwg-write][object-encode][background][safety]") {
    DRW_Background source;
    source.m_kind = DRW_Background::Gradient;
    source.m_horizon = std::numeric_limits<double>::quiet_NaN();

    dwgBufferW encoded;
    encoded.putRawLong32(0xA5A5A5A5u);
    const auto before = snapshot(encoded);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeBackground(
        source, DRW::AC1018, &encoded));
    CHECK(snapshot(encoded) == before);

    source.m_horizon = 0.0;
    source.m_kind = DRW_Background::Image;
    source.m_offset.x = std::numeric_limits<double>::infinity();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeBackground(
        source, DRW::AC1018, &encoded));
    CHECK(snapshot(encoded) == before);

    source.m_offset.x = 0.0;
    source.m_kind = static_cast<DRW_Background::Kind>(99);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeBackground(
        source, DRW::AC1018, &encoded));
    CHECK(snapshot(encoded) == before);

    dwgBufferW malformed;
    emitObjectPreamble(malformed, DRW::AC1018, /*oType=*/0,
                       /*handle=*/0xB58);
    malformed.putBitLong(1); // class version
    malformed.putBitLong(0); // top color
    malformed.putBitLong(0); // middle color
    malformed.putBitLong(0); // bottom color
    malformed.putBitDouble(std::numeric_limits<double>::quiet_NaN());
    auto malformedBytes = snapshot(malformed);
    dwgBuffer malformedReader(malformedBytes.data(), malformedBytes.size());
    DRW_Background rejected;
    rejected.m_kind = DRW_Background::Gradient;
    rejected.m_horizon = 42.0;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseBackground(
        rejected, DRW::AC1018, &malformedReader));
    CHECK_FALSE(malformedReader.isGood());
    CHECK(rejected.m_horizon == 0.0);
}

TEST_CASE("DRW_SunStudy DWG round-trips dates, hours, and references",
          "[dwg-write][object-encode][sunstudy]") {
    const DRW::Version ver = DRW::AC1015;
    DRW_SunStudy source;
    source.handle = 0xB60;
    source.parentHandle = 0x42;
    source.m_classVersion = 1;
    source.m_setupName = "Study A";
    source.m_description = "Summer path";
    source.m_outputType = 0;
    source.m_useSubset = true;
    source.m_sheetSetName = "Set";
    source.m_sheetSubsetName = "Subset";
    source.m_selectDatesFromCalendar = true;
    source.m_dates = {{2460828, 3600000}, {2460829, 7200000}};
    source.m_selectRangeOfDates = true;
    source.m_startTime = 28800000;
    source.m_endTime = 64800000;
    source.m_interval = 3600000;
    source.m_hours = {true, false, true};
    source.m_shadePlotType = 2;
    source.m_viewportCount = 4;
    source.m_rowCount = 2;
    source.m_columnCount = 2;
    source.m_spacing = 1.25;
    source.m_lockViewports = true;
    source.m_labelViewports = false;
    source.m_pageSetupWizardHandle = 0x61;
    source.m_viewHandle = 0x62;
    source.m_visualStyleHandle = 0x63;
    source.m_textStyleHandle = 0x64;

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_SunStudy::kDwgType, source.handle,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeSunStudy(source, ver, &encoded));
    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SunStudy parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseSunStudy(parsed, ver, &reader));
    REQUIRE(reader.isGood());
    CHECK(parsed.m_classVersion == 1);
    CHECK(parsed.m_setupName == "Study A");
    CHECK(parsed.m_useSubset);
    REQUIRE(parsed.m_dates.size() == 2u);
    CHECK(parsed.m_dates[1].m_milliseconds == 7200000);
    CHECK(parsed.m_hours == std::vector<bool>({true, false, true}));
    CHECK(parsed.m_pageSetupWizardHandle == 0x61u);
    CHECK(parsed.m_textStyleHandle == 0x64u);
    CHECK(parsed.parentHandle == 0x42);
}

TEST_CASE("DRW_SunStudy rejects unbounded vector counts",
          "[dwg-read][object-encode][sunstudy][safety]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_SunStudy::kDwgType, 0xB61);
    encoded.putBitLong(1); // class version
    encoded.putVariableText(ver, "Study");
    encoded.putVariableText(ver, "Description");
    encoded.putBitLong(0); // output type
    encoded.putBit(0);     // use subset
    encoded.putVariableText(ver, "");
    encoded.putVariableText(ver, "");
    encoded.putBit(0); // select dates from calendar
    encoded.putBitLong(DRW_SunStudy::kMaxDateCount + 1);
    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SunStudy parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseSunStudy(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_dates.empty());
}

TEST_CASE("DRW_SunStudy rejects a body that ends before fixed fields",
          "[dwg-read][object-encode][sunstudy][safety]") {
    const DRW::Version ver = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_SunStudy::kDwgType, 0xB62);
    const std::uint32_t bodyStartBit = encoded.bitCount();

    DRW_SunStudy source;
    source.m_setupName = "Study";
    source.m_description = "Description";
    REQUIRE(DrwObjectEncodeTestAccess::encodeSunStudy(
        source, ver, &encoded));
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyStartBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SunStudy parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseSunStudy(
        parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_setupName.empty());
    CHECK(parsed.m_dates.empty());
}

TEST_CASE("DRW_SunStudy rejects non-finite spacing",
          "[dwg-write][object-encode][sunstudy][safety]") {
    DRW_SunStudy source;
    source.m_spacing = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW body;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeSunStudy(
        source, DRW::AC1015, &body));
    CHECK(body.data().empty());
}

TEST_CASE("DRW_SunStudy preserves DWG BL scalar values",
          "[dwg-write][object-encode][sunstudy]") {
    const DRW::Version ver = DRW::AC1015;
    DRW_SunStudy source;
    source.handle = 0xB63;
    source.m_classVersion = 1;
    source.m_outputType = 70000;
    source.m_shadePlotType = 70001;
    source.m_viewportCount = 70002;
    source.m_rowCount = 70003;
    source.m_columnCount = 70004;

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_SunStudy::kDwgType, source.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeSunStudy(source, ver, &encoded));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SunStudy parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseSunStudy(parsed, ver, &reader));
    REQUIRE(reader.isGood());
    CHECK(parsed.m_outputType == 70000);
    CHECK(parsed.m_shadePlotType == 70001);
    CHECK(parsed.m_viewportCount == 70002);
    CHECK(parsed.m_rowCount == 70003);
    CHECK(parsed.m_columnCount == 70004);

    parsed.m_setupName = "stale";
    parsed.m_dates.push_back({1, 2});
    parsed.m_hours.push_back(true);
    parsed.reset();
    CHECK(parsed.m_setupName.empty());
    CHECK(parsed.m_dates.empty());
    CHECK(parsed.m_hours.empty());
    CHECK(parsed.m_outputType == 0);
}

TEST_CASE("DRW_MotionPath DWG round-trips paths and common handles",
          "[dwg-write][object-encode][motionpath]") {
    const DRW::Version ver = DRW::AC1015;
    DRW_MotionPath source;
    source.handle = 0xB62;
    source.parentHandle = 0x42;
    source.reactorHandles = {0x44};
    source.xDictHandle = 0x43;
    source.setDwgCommonObjectState(1, 0, false);
    source.m_classVersion = 2;
    source.m_cameraPathHandle = 0x61;
    source.m_targetPathHandle = 0x62;
    source.m_viewTableHandle = 0x63;
    source.m_frames = 120;
    source.m_frameRate = 24;
    source.m_cornerDeceleration = true;

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_MotionPath::kDwgType, source.handle,
                       source.reactorCount(), source.extensionDictionaryFlag());
    REQUIRE(DrwObjectEncodeTestAccess::encodeMotionPath(source, ver, &encoded));
    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_MotionPath parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseMotionPath(parsed, ver, &reader));
    REQUIRE(reader.isGood());
    CHECK(parsed.m_classVersion == 2);
    CHECK(parsed.m_cameraPathHandle == 0x61u);
    CHECK(parsed.m_targetPathHandle == 0x62u);
    CHECK(parsed.m_viewTableHandle == 0x63u);
    CHECK(parsed.m_frames == 120);
    CHECK(parsed.m_frameRate == 24);
    CHECK(parsed.m_cornerDeceleration);
    CHECK(parsed.parentHandle == 0x42);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x44u);
    CHECK(parsed.xDictHandle == 0x43u);

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseMotionPath(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
}

TEST_CASE("DRW_MotionPath rejects bit-short overflow",
          "[dwg-write][object-encode][motionpath][safety]") {
    DRW_MotionPath source;
    source.m_frames = 65536;
    dwgBufferW encoded;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeMotionPath(
        source, DRW::AC1015, &encoded));
}

TEST_CASE("DRW_CurvePath and DRW_PointPath DWG round-trip fixed bodies",
          "[dwg-write][object-encode][path]" ) {
    const DRW::Version ver = DRW::AC1015;
    DRW_CurvePath curve;
    curve.handle = 0xB63;
    curve.parentHandle = 0x42;
    curve.reactorHandles = {0x44};
    curve.xDictHandle = 0x43;
    curve.setDwgCommonObjectState(1, 0, false);
    curve.m_classVersion = 3;
    curve.m_entityHandle = 0x71;

    dwgBufferW curveEncoded;
    emitObjectPreamble(curveEncoded, ver, DRW_CurvePath::kDwgType,
                       curve.handle, curve.reactorCount(),
                       curve.extensionDictionaryFlag());
    REQUIRE(DrwObjectEncodeTestAccess::encodeCurvePath(
        curve, ver, &curveEncoded));
    auto curveBytes = snapshot(curveEncoded);
    dwgBuffer curveReader(curveBytes.data(), curveBytes.size());
    DRW_CurvePath parsedCurve;
    REQUIRE(DrwObjectEncodeTestAccess::parseCurvePath(
        parsedCurve, ver, &curveReader));
    CHECK(parsedCurve.m_classVersion == 3);
    CHECK(parsedCurve.m_entityHandle == 0x71u);
    CHECK(parsedCurve.parentHandle == 0x42);
    REQUIRE(parsedCurve.reactorHandles.size() == 1u);
    CHECK(parsedCurve.reactorHandles.front() == 0x44u);
    CHECK(parsedCurve.xDictHandle == 0x43u);

    curveBytes.pop_back();
    dwgBuffer truncatedCurveReader(curveBytes.data(), curveBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseCurvePath(
        parsedCurve, ver, &truncatedCurveReader));
    CHECK_FALSE(truncatedCurveReader.isGood());

    DRW_PointPath point;
    point.handle = 0xB64;
    point.parentHandle = 0x43;
    point.reactorHandles = {0x45};
    point.xDictHandle = 0x46;
    point.setDwgCommonObjectState(1, 0, false);
    point.m_classVersion = 4;
    point.m_point = {1.25, -2.5, 3.75};

    dwgBufferW pointEncoded;
    emitObjectPreamble(pointEncoded, ver, DRW_PointPath::kDwgType,
                       point.handle, point.reactorCount(),
                       point.extensionDictionaryFlag());
    REQUIRE(DrwObjectEncodeTestAccess::encodePointPath(
        point, ver, &pointEncoded));
    auto pointBytes = snapshot(pointEncoded);
    dwgBuffer pointReader(pointBytes.data(), pointBytes.size());
    DRW_PointPath parsedPoint;
    REQUIRE(DrwObjectEncodeTestAccess::parsePointPath(
        parsedPoint, ver, &pointReader));
    CHECK(parsedPoint.m_classVersion == 4);
    CHECK(parsedPoint.m_point.x == 1.25);
    CHECK(parsedPoint.m_point.y == -2.5);
    CHECK(parsedPoint.m_point.z == 3.75);
    CHECK(parsedPoint.parentHandle == 0x43);
    REQUIRE(parsedPoint.reactorHandles.size() == 1u);
    CHECK(parsedPoint.reactorHandles.front() == 0x45u);
    CHECK(parsedPoint.xDictHandle == 0x46u);

    pointBytes.pop_back();
    dwgBuffer truncatedPointReader(pointBytes.data(), pointBytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parsePointPath(
        parsedPoint, ver, &truncatedPointReader));
    CHECK_FALSE(truncatedPointReader.isGood());
}

TEST_CASE("DRW_CurvePath enforces the AC1024 object body boundary",
          "[dwg-read][object-encode][curvepath][safety]") {
    const DRW::Version version = DRW::AC1024;
    DRW_CurvePath source;
    source.handle = 0xB70u;
    source.parentHandle = 0x42u;
    source.m_classVersion = 3;
    source.m_entityHandle = 0x71u;
    source.setDwgCommonObjectState(1, 0, false);

    dwgBufferW data;
    emitObjectPreamble(data, version, DRW_CurvePath::kDwgType,
                       source.handle, 0, source.extensionDictionaryFlag());
    dwgBufferW handles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeCurvePath(
        source, version, &data, nullptr, &handles));
    const std::uint32_t bodyFieldEndBit = data.bitCount();
    data.alignToByte();
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    data.putRawShort16(0); // empty detached string stream
    data.putBit(0);        // no detached strings
    data.alignToByte();
    const std::uint32_t bodyEndBit = data.bitCount();
    handles.alignToByte();
    data.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(data);
    const std::uint32_t handleBits =
        static_cast<std::uint32_t>(handles.data().size() * 8u);
    dwgBuffer validReader(bytes.data(), bytes.size());
    DRW_CurvePath valid;
    REQUIRE(DrwObjectEncodeTestAccess::parseCurvePath(
        valid, version, &validReader, handleBits));
    CHECK(valid.m_classVersion == source.m_classVersion);
    CHECK(valid.m_entityHandle == source.m_entityHandle);

    REQUIRE(bodyEndBit >= bodyFieldEndBit);
    REQUIRE(handleBits <= std::numeric_limits<std::uint32_t>::max()
            - (bodyEndBit - bodyFieldEndBit) - 1u);
    const std::uint32_t malformedHandleBits =
        handleBits + (bodyEndBit - bodyFieldEndBit) + 1u;
    dwgBuffer malformedReader(bytes.data(), bytes.size());
    DRW_CurvePath malformed;
    malformed.m_classVersion = 99;
    malformed.m_entityHandle = 0xDEADu;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseCurvePath(
        malformed, version, &malformedReader, malformedHandleBits));
    CHECK_FALSE(malformedReader.isGood());
    CHECK(malformed.m_classVersion == 0);
    CHECK(malformed.m_entityHandle == 0u);
}

TEST_CASE("DRW_PointPath rejects bit-short overflow",
          "[dwg-write][object-encode][pointpath][safety]") {
    DRW_PointPath source;
    source.m_classVersion = 65536;
    dwgBufferW encoded;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePointPath(
        source, DRW::AC1015, &encoded));
}

TEST_CASE("DRW_PointPath rejects non-finite coordinates",
          "[dwg-write][object-encode][pointpath][safety]") {
    DRW_PointPath source;
    source.m_point.y = std::numeric_limits<double>::infinity();

    dwgBufferW encoded;
    encoded.putRawChar8(0x5A);
    const auto before = snapshot(encoded);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePointPath(
        source, DRW::AC1015, &encoded));
    CHECK(snapshot(encoded) == before);
}

TEST_CASE("DRW_ObjectPtr DWG round-trips common handles",
          "[dwg-write][object-encode][object-ptr]") {
    const DRW::Version ver = DRW::AC1015;
    DRW_ObjectPtr source;
    source.handle = 0xB65;
    source.parentHandle = 0x42;
    source.reactorHandles = {0x44};
    source.xDictHandle = 0x43;
    source.setDwgCommonObjectState(1, 0, false);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_ObjectPtr::kDwgType,
                       source.handle, source.reactorCount(),
                       source.extensionDictionaryFlag());
    REQUIRE(DrwObjectEncodeTestAccess::encodeObjectPtr(
        source, ver, &encoded));
    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_ObjectPtr parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parseObjectPtr(parsed, ver, &reader));
    REQUIRE(reader.isGood());
    CHECK(parsed.parentHandle == 0x42);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x44u);
    CHECK(parsed.xDictHandle == 0x43u);

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    parsed.parentHandle = 0x99u;
    parsed.reactorHandles = {0x98u};
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseObjectPtr(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
}

TEST_CASE("DRW_PartialViewingIndex DWG round-trips entries and handles",
          "[dwg-write][object-encode][partial-viewing-index]") {
    const DRW::Version ver = DRW::AC1015;
    DRW_PartialViewingIndex source;
    source.handle = 0xB66;
    source.parentHandle = 0x42;
    source.reactorHandles = {0x44};
    source.xDictHandle = 0x43;
    source.m_hasEntries = true;
    source.m_entries = {
        {{0.0, 1.0, 2.0}, {3.0, 4.0, 5.0}, 0x91},
        {{-1.0, -2.0, -3.0}, {6.0, 7.0, 8.0}, 0x92},
    };
    source.setDwgCommonObjectState(1, 0, false);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, DRW_PartialViewingIndex::kDwgType,
                       source.handle, source.reactorCount(),
                       source.extensionDictionaryFlag());
    REQUIRE(DrwObjectEncodeTestAccess::encodePartialViewingIndex(
        source, ver, &encoded));
    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_PartialViewingIndex parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parsePartialViewingIndex(
        parsed, ver, &reader));
    REQUIRE(reader.isGood());
    CHECK(parsed.m_entryCount == 2u);
    CHECK(parsed.m_hasEntries);
    REQUIRE(parsed.m_entries.size() == 2u);
    CHECK(parsed.m_entries[0].extentsMin.x == 0.0);
    CHECK(parsed.m_entries[0].extentsMax.z == 5.0);
    CHECK(parsed.m_entries[0].objectHandle == 0x91u);
    CHECK(parsed.m_entries[1].extentsMin.y == -2.0);
    CHECK(parsed.m_entries[1].objectHandle == 0x92u);
    CHECK(parsed.parentHandle == 0x42);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x44u);
    CHECK(parsed.xDictHandle == 0x43u);

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    parsed.m_entryCount = 99u;
    parsed.m_entries.push_back({{}, {}, 0x98u});
    CHECK_FALSE(DrwObjectEncodeTestAccess::parsePartialViewingIndex(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_entryCount == 0u);
    CHECK(parsed.m_entries.empty());
}

TEST_CASE("DRW_PartialViewingIndex enforces the declared object body",
          "[dwg-read][object-encode][partial-viewing-index][safety]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0xB67,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(1);
    encoded.putBit(1);
    encoded.put3BitDouble(DRW_Coord{0.0, 1.0, 2.0});
    encoded.put3BitDouble(DRW_Coord{3.0, 4.0, 5.0});
    encoded.putHandle(hardPtr(0x91));
    const std::uint32_t bodyEndBit = encoded.bitCount();
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_PartialViewingIndex parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parsePartialViewingIndex(
        parsed, ver, &reader));
    REQUIRE(parsed.m_entries.size() == 1u);
    CHECK(parsed.m_entries.front().objectHandle == 0x91u);
    CHECK(parsed.parentHandle == 0x42u);

    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded),
                                bodyEndBit - 1u);
    bytes = snapshot(encoded);
    dwgBuffer truncatedBodyReader(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parsePartialViewingIndex(
        parsed, ver, &truncatedBodyReader));
    CHECK_FALSE(truncatedBodyReader.isGood());
    CHECK(parsed.m_entryCount == 0u);
    CHECK(parsed.m_entries.empty());
}

TEST_CASE("DRW_PartialViewingIndex does not read detached strings as entries",
          "[dwg-read][object-encode][partial-viewing-index][ac1021][safety]") {
    const DRW::Version version = DRW::AC1021;
    dwgBufferW data;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(data, version, /*oType=*/0,
                       /*handle=*/0xB68, /*numReactors=*/0,
                       /*xDictFlag=*/1);

    // Declare two entries but encode only one.  The detached stream is large
    // enough to make an unbounded parser appear to find a second entry.
    data.putBitLong(2);
    data.putBit(1);
    data.put3BitDouble(DRW_Coord{0.0, 1.0, 2.0});
    data.put3BitDouble(DRW_Coord{3.0, 4.0, 5.0});
    data.putHandle(hardPtr(0x91));

    data.alignToByte();
    for (int i = 0; i < 64; ++i)
        strings.putRawChar8(static_cast<std::uint8_t>(0x80u + i));
    strings.alignToByte();
    data.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        data.putBit(0);
    const std::uint32_t stringBitSize =
        static_cast<std::uint32_t>(strings.size() * 8u + 7u);
    REQUIRE(stringBitSize <= std::numeric_limits<std::uint16_t>::max());
    data.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    data.putBit(1);
    data.alignToByte();

    // AC1021 stores the object-size RL after the object type.
    data.patchRawLong32AtBit(objectSizeBitOffset(data),
                             static_cast<std::uint32_t>(data.size() * 8u));
    handles.putHandle(hardPtr(0x42));
    handles.alignToByte();
    data.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(data);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_PartialViewingIndex parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parsePartialViewingIndex(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_entryCount == 0u);
    CHECK(parsed.m_entries.empty());
}

TEST_CASE("DRW_PartialViewingIndex rejects non-finite entry extents",
          "[dwg-write][object-encode][partial-viewing-index][safety]") {
    DRW_PartialViewingIndex source;
    source.m_hasEntries = true;
    source.m_entries = {{{std::numeric_limits<double>::quiet_NaN(), 1.0, 2.0},
                         {3.0, 4.0, 5.0}, 0x91}};

    dwgBufferW encoded;
    encoded.putRawChar8(0xA5);
    const auto before = snapshot(encoded);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePartialViewingIndex(
        source, DRW::AC1018, &encoded));
    CHECK(snapshot(encoded) == before);

    source.m_entries.front().extentsMin.x = 0.0;
    source.m_hasEntries = false;
    dwgBufferW validEncoded;
    emitObjectPreamble(validEncoded, DRW::AC1018,
                       DRW_PartialViewingIndex::kDwgType, source.handle,
                       source.reactorCount(), source.extensionDictionaryFlag());
    CHECK(DrwObjectEncodeTestAccess::encodePartialViewingIndex(
        source, DRW::AC1018, &validEncoded));
    auto bytes = snapshot(validEncoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_PartialViewingIndex parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parsePartialViewingIndex(
        parsed, DRW::AC1018, &reader));
    CHECK(parsed.m_hasEntries);
    REQUIRE(parsed.m_entries.size() == 1u);
}

TEST_CASE("DRW_PartialViewingIndex rejects non-finite DWG extents",
          "[dwg-read][object-encode][partial-viewing-index][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0, /*handle=*/0xB69,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(1);
    encoded.putBit(1);
    encoded.put3BitDouble(
        DRW_Coord{std::numeric_limits<double>::quiet_NaN(), 1.0, 2.0});
    encoded.put3BitDouble(DRW_Coord{3.0, 4.0, 5.0});
    encoded.putHandle(hardPtr(0x91));
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x42,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_PartialViewingIndex parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parsePartialViewingIndex(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_entryCount == 0u);
    CHECK(parsed.m_entries.empty());
}

TEST_CASE("DWG metadata writers reject invalid common reactor counts",
          "[dwg-write][object-encode][metadata][safety]") {
    dwgBufferW encoded;
    DRW_ObjectPtr objectPtr;
    DrwObjectEncodeTestAccess::setNumReactors(objectPtr, -1);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeObjectPtr(
        objectPtr, DRW::AC1015, &encoded));
    CHECK(encoded.data().empty());

    DrwObjectEncodeTestAccess::setNumReactors(
        objectPtr, static_cast<std::int32_t>(dwgSafety::MaxReactorCount + 1));
    encoded.reset();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeObjectPtr(
        objectPtr, DRW::AC1015, &encoded));
    CHECK(encoded.data().empty());

    DRW_PartialViewingIndex partial;
    DrwObjectEncodeTestAccess::setNumReactors(
        partial, static_cast<std::int32_t>(dwgSafety::MaxReactorCount + 1));
    encoded.reset();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePartialViewingIndex(
        partial, DRW::AC1015, &encoded));
    CHECK(encoded.data().empty());
}

TEST_CASE("DRW_RenderSettings rejects lossy values before writing",
          "[dwg-write][object-encode][render-settings][safety]") {
    DRW_RenderSettings entry;
    entry.m_kind = DRW_RenderSettings::Entry;
    entry.m_shorts = {-1};
    dwgBufferW negativeShort;
    negativeShort.putRawChar8(0xA5);
    const auto negativeShortBytes = snapshot(negativeShort);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        entry, DRW::AC1015, &negativeShort));
    CHECK(negativeShort.data() == negativeShortBytes);

    entry.m_shorts = {65536};
    dwgBufferW oversizedShort;
    oversizedShort.putRawChar8(0x5A);
    const auto oversizedShortBytes = snapshot(oversizedShort);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        entry, DRW::AC1015, &oversizedShort));
    CHECK(oversizedShort.data() == oversizedShortBytes);

    DRW_RenderSettings environment;
    environment.m_kind = DRW_RenderSettings::Environment;
    environment.m_bytes = {256};
    dwgBufferW invalidByte;
    invalidByte.putRawChar8(0x3C);
    const auto invalidByteBytes = snapshot(invalidByte);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        environment, DRW::AC1015, &invalidByte));
    CHECK(invalidByte.data() == invalidByteBytes);

    DRW_RenderSettings mental;
    mental.m_kind = DRW_RenderSettings::MentalRay;
    mental.m_doubles = {std::numeric_limits<double>::quiet_NaN()};
    dwgBufferW nonFinite;
    nonFinite.putRawChar8(0xC3);
    const auto nonFiniteBytes = snapshot(nonFinite);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        mental, DRW::AC1015, &nonFinite));
    CHECK(nonFinite.data() == nonFiniteBytes);

    DRW_RenderSettings unknown;
    unknown.m_kind = static_cast<DRW_RenderSettings::Kind>(99);
    dwgBufferW invalidKind;
    invalidKind.putRawChar8(0x96);
    const auto invalidKindBytes = snapshot(invalidKind);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        unknown, DRW::AC1015, &invalidKind));
    CHECK(invalidKind.data() == invalidKindBytes);
}

TEST_CASE("DRW_RenderSettings rejects non-finite DWG values transactionally",
          "[dwg-read][object-encode][render-settings][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version,
                       DRW_RenderSettings::kDwgClassNumEntry,
                       /*handle=*/0xB6A);
    encoded.putBitLong(0); // class version
    encoded.putVariableText(version, "render.png");
    encoded.putVariableText(version, "");
    encoded.putVariableText(version, "");
    encoded.putBitLong(0);
    encoded.putBitLong(0);
    for (int i = 0; i < 6; ++i)
        encoded.putBitShort(0);
    encoded.putBitDouble(nan);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_RenderSettings parsed;
    parsed.m_kind = DRW_RenderSettings::Entry;
    parsed.m_strings = {"stale"};
    parsed.m_doubles = {9.0};
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_strings.empty());
    CHECK(parsed.m_doubles.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_RenderEntry DWG round-trips positional fields",
          "[dwg-write][object-encode][render-entry]") {
    const DRW::Version ver = DRW::AC1015;
    DRW_RenderSettings source;
    source.m_kind = DRW_RenderSettings::Entry;
    source.handle = 0xB70;
    source.parentHandle = 0x42;
    source.xDictHandle = 0x30;
    source.m_classVersion = 1;
    source.m_longs = {1, 1920, 1080, 4096, 3, 2, 1000, 4};
    source.m_strings = {"render.png", "Draft", "Top"};
    source.m_shorts = {2026, 5, 27, 34, 56, 789};
    source.m_doubles = {12.5};

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver,
                       DRW_RenderSettings::kDwgClassNumEntry,
                       source.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        source, ver, &encoded));
    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_RenderSettings parsed;
    parsed.m_kind = DRW_RenderSettings::Entry;
    REQUIRE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsed, ver, &reader));
    REQUIRE(reader.isGood());
    CHECK(parsed.m_classVersion == 1);
    CHECK(parsed.m_strings[0] == "render.png");
    CHECK(parsed.m_longs[1] == 1920);
    CHECK(parsed.m_shorts[5] == 789);
    CHECK(parsed.m_doubles[0] == 12.5);
    CHECK(parsed.parentHandle == 0x42);
    CHECK(parsed.xDictHandle == 0x30);

    bytes.pop_back();
    dwgBuffer truncatedReader(bytes.data(), bytes.size());
    parsed.m_strings = {"stale"};
    parsed.parentHandle = 0x99;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_strings.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_RenderEntry stages AC1018 handles in the legacy object stream",
          "[dwg-write][object-encode][render-entry]") {
    const DRW::Version ver = DRW::AC1018;
    DRW_RenderSettings source;
    source.m_kind = DRW_RenderSettings::Entry;
    source.handle = 0xB74;
    source.parentHandle = 0x42;
    source.reactorHandles = {0x44};
    source.xDictHandle = 0x30;
    source.setDwgCommonObjectState(1, 0, false);
    source.m_classVersion = 1;
    source.m_longs = {1, 1920, 1080, 4096, 3, 2, 1000, 4};
    source.m_strings = {"render.png", "Draft", "Top"};
    source.m_shorts = {2026, 5, 27, 34, 56, 789};
    source.m_doubles = {12.5};

    dwgBufferW encoded;
    dwgBufferW detachedHandles;
    emitObjectPreamble(encoded, ver,
                       DRW_RenderSettings::kDwgClassNumEntry,
                       source.handle, 1, 0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        source, ver, &encoded, nullptr, &detachedHandles));
    const auto detached = snapshot(detachedHandles);
    REQUIRE_FALSE(detached.empty());

    // AC1015/AC1018 append the temporary handle stream after the data body.
    // The object-size field points at that boundary so the reader can seek
    // there after decoding the data fields.
    const std::uint32_t dataBitCount = encoded.bitCount();
    REQUIRE_FALSE(encoded.data().empty());
    const std::uint8_t bsCode = (encoded.data()[0] >> 6) & 0x03;
    const std::size_t rlBitOffset =
        (bsCode == 0x01) ? 10 : (bsCode == 0x00) ? 18 : 2;
    encoded.patchRawLong32AtBit(rlBitOffset, dataBitCount);
    encoded.putBytes(detached.data(), detached.size());
    encoded.alignToByte();

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_RenderSettings parsed;
    parsed.m_kind = DRW_RenderSettings::Entry;
    REQUIRE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsed, ver, &reader));
    CHECK(parsed.parentHandle == 0x42);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x44u);
    CHECK(parsed.xDictHandle == 0x30);
}

TEST_CASE("DRW_RenderEnvironment and RenderGlobal round-trip",
          "[dwg-write][object-encode][render-settings]") {
    const DRW::Version ver = DRW::AC1015;
    DRW_RenderSettings environment;
    environment.m_kind = DRW_RenderSettings::Environment;
    environment.handle = 0xB71;
    environment.m_classVersion = 1;
    environment.m_longs = {1};
    environment.m_bools = {true, false, true};
    environment.m_bytes = {10, 20, 30};
    environment.m_doubles = {0.1, 0.9, 5.0, 100.0};
    environment.m_strings = {"env.hdr"};

    dwgBufferW environmentBytes;
    emitObjectPreamble(environmentBytes, ver,
                       DRW_RenderSettings::kDwgClassNumEnvironment,
                       environment.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        environment, ver, &environmentBytes));
    auto environmentSnapshot = snapshot(environmentBytes);
    dwgBuffer environmentReader(environmentSnapshot.data(),
                                 environmentSnapshot.size());
    DRW_RenderSettings parsedEnvironment;
    parsedEnvironment.m_kind = DRW_RenderSettings::Environment;
    REQUIRE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsedEnvironment, ver, &environmentReader));
    CHECK(parsedEnvironment.m_fogEnabled);
    CHECK(parsedEnvironment.m_fogColorG == 20);
    CHECK(parsedEnvironment.m_environmentImageEnabled);
    CHECK(parsedEnvironment.m_name == "env.hdr");

    DRW_RenderSettings global;
    global.m_kind = DRW_RenderSettings::Global;
    global.handle = 0xB72;
    global.m_classVersion = 2;
    global.m_longs = {2, 1, 0, 1280, 720};
    global.m_bools = {true, false, true};
    global.m_strings = {"output.png"};

    dwgBufferW globalBytes;
    emitObjectPreamble(globalBytes, ver,
                       DRW_RenderSettings::kDwgClassNumGlobal,
                       global.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        global, ver, &globalBytes));
    auto globalSnapshot = snapshot(globalBytes);
    dwgBuffer globalReader(globalSnapshot.data(), globalSnapshot.size());
    DRW_RenderSettings parsedGlobal;
    parsedGlobal.m_kind = DRW_RenderSettings::Global;
    REQUIRE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsedGlobal, ver, &globalReader));
    CHECK(parsedGlobal.m_classVersion == 2);
    CHECK(parsedGlobal.m_procedure == 1);
    CHECK(parsedGlobal.m_longs[4] == 720);
    CHECK(parsedGlobal.m_bools[2]);
    CHECK(parsedGlobal.m_name == "output.png");
}

TEST_CASE("DRW_RenderSettings derived families round-trip their DWG bodies",
          "[dwg-write][object-encode][render-settings]") {
    const DRW::Version ver = DRW::AC1015;

    DRW_RenderSettings settings;
    settings.m_kind = DRW_RenderSettings::Settings;
    settings.handle = 0xB73;
    settings.m_classVersion = 2;
    settings.m_longs = {2, 7};
    settings.m_strings = {"Draft", "env.hdr", "base settings"};
    settings.m_bools = {true, false, true, false};

    dwgBufferW settingsBytes;
    emitObjectPreamble(settingsBytes, ver,
                       DRW_RenderSettings::kDwgClassNumSettings,
                       settings.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        settings, ver, &settingsBytes));
    auto settingsSnapshot = snapshot(settingsBytes);
    dwgBuffer settingsReader(settingsSnapshot.data(), settingsSnapshot.size());
    DRW_RenderSettings parsedSettings;
    parsedSettings.m_kind = DRW_RenderSettings::Settings;
    REQUIRE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsedSettings, ver, &settingsReader));
    CHECK(parsedSettings.m_classVersion == 2);
    CHECK(parsedSettings.m_name == "Draft");
    CHECK(parsedSettings.m_description == "base settings");
    CHECK(parsedSettings.m_displayIndex == 7);
    CHECK(parsedSettings.m_backfacesEnabled);

    DRW_RenderSettings rapid;
    rapid.m_kind = DRW_RenderSettings::RapidRT;
    rapid.handle = 0xB74;
    rapid.m_classVersion = 3;
    rapid.m_longs = {3, 4, 2, 1, 60, 1, 2, 3};
    rapid.m_strings = {"Rapid", "", ""};
    rapid.m_bools = {false, true, false, true};
    rapid.m_doubles = {1.25, 2.5};
    rapid.m_hasPredefined = true;

    dwgBufferW rapidBytes;
    emitObjectPreamble(rapidBytes, ver,
                       DRW_RenderSettings::kDwgClassNumRapidRT,
                       rapid.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        rapid, ver, &rapidBytes));
    auto rapidSnapshot = snapshot(rapidBytes);
    dwgBuffer rapidReader(rapidSnapshot.data(), rapidSnapshot.size());
    DRW_RenderSettings parsedRapid;
    parsedRapid.m_kind = DRW_RenderSettings::RapidRT;
    REQUIRE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsedRapid, ver, &rapidReader));
    CHECK(parsedRapid.m_classVersion == 3);
    CHECK(parsedRapid.m_longs[2] == 2);
    CHECK(parsedRapid.m_longs[5] == 1);
    CHECK(parsedRapid.m_doubles[1] == 2.5);
    REQUIRE(parsedRapid.m_hasPredefinedPresent);
    CHECK(parsedRapid.m_hasPredefined);

    DRW_RenderSettings mental;
    mental.m_kind = DRW_RenderSettings::MentalRay;
    mental.handle = 0xB75;
    mental.m_classVersion = 4;
    mental.m_longs = {4, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    mental.m_strings = {"Mental", "", "", "mi description"};
    mental.m_bools = {false, false, true, false, true, true, false, true,
                      false, true, false, true, false, true};
    mental.m_shorts = {1, 2, 3, 4, 5, 6, 7};
    mental.m_doubles = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    dwgBufferW mentalBytes;
    emitObjectPreamble(mentalBytes, ver,
                       DRW_RenderSettings::kDwgClassNumMentalRay,
                       mental.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        mental, ver, &mentalBytes));
    auto mentalSnapshot = snapshot(mentalBytes);
    dwgBuffer mentalReader(mentalSnapshot.data(), mentalSnapshot.size());
    DRW_RenderSettings parsedMental;
    parsedMental.m_kind = DRW_RenderSettings::MentalRay;
    REQUIRE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsedMental, ver, &mentalReader));
    CHECK(parsedMental.m_classVersion == 4);
    CHECK(parsedMental.m_strings[3] == "mi description");
    CHECK(parsedMental.m_longs[12] == 12);
    CHECK(parsedMental.m_doubles[11] == 12.0);
    CHECK_FALSE(parsedMental.m_hasPredefinedPresent);
}

TEST_CASE("DRW_RenderSettings bounds AC1021 detached strings",
          "[dwg-read][object-encode][render-settings][safety]") {
    const DRW::Version version = DRW::AC1021;
    DRW_RenderSettings source;
    source.m_kind = DRW_RenderSettings::Settings;
    source.handle = 0xB76;
    source.m_classVersion = 2;
    source.m_longs = {2, 7};
    source.m_strings = {"Modern", "env.hdr", "description"};
    source.m_bools = {true, false, true, false};

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(body, version,
                       DRW_RenderSettings::kDwgClassNumSettings,
                       source.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        source, version, &body, &strings, &handles));
    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u));
    body.putBit(1);
    body.alignToByte();
    handles.alignToByte();
    body.patchRawLong32AtBit(objectSizeBitOffset(body),
                             static_cast<std::uint32_t>(body.bitCount()));
    std::vector<std::uint8_t> bytes = body.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());

    DRW_TextCodec decoder;
    decoder.setVersion(version, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_RenderSettings parsed;
    parsed.m_kind = DRW_RenderSettings::Settings;
    REQUIRE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsed, version, &reader));
    CHECK(parsed.m_name == "Modern");
    CHECK(parsed.m_description == "description");
    CHECK(parsed.m_displayIndex == 7);
    CHECK(parsed.m_backfacesEnabled);
}

TEST_CASE("DRW_RenderSettings preserves modern body tails",
          "[dwg-read][dwg-write][object-encode][render-settings]") {
    const DRW::Version version = DRW::AC1021;
    DRW_RenderSettings source;
    source.m_kind = DRW_RenderSettings::Settings;
    source.handle = 0xB77;
    source.m_classVersion = 2;
    source.m_longs = {2, 7};
    source.m_strings = {"Modern", "env.hdr", "description"};
    source.m_bools = {true, false, true, false};
    source.parentHandle = 0x42;
    REQUIRE(source.setDwgRawData({0xA5u}, 8, version));

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    emitObjectPreamble(body, version,
                       DRW_RenderSettings::kDwgClassNumSettings,
                       source.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        source, version, &body, &strings, &handles));
    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u));
    body.putBit(1);
    body.alignToByte();
    handles.alignToByte();
    body.patchRawLong32AtBit(objectSizeBitOffset(body),
                             static_cast<std::uint32_t>(body.bitCount()));
    std::vector<std::uint8_t> bytes = body.data();
    bytes.insert(bytes.end(), handles.data().begin(), handles.data().end());

    DRW_TextCodec decoder;
    decoder.setVersion(version, false);
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(bytes.data(), bytes.size(), &decoder);
    DRW_RenderSettings parsed;
    parsed.m_kind = DRW_RenderSettings::Settings;
    REQUIRE(DrwObjectEncodeTestAccess::parseRenderSettings(
        parsed, version, &reader));
    REQUIRE(parsed.hasDwgRawData());
    CHECK(parsed.dwgRawDataVersion() == version);
    CHECK(parsed.dwgRawDataBitSize() >= 8u);
    CHECK(parsed.dwgRawData().front() == 0xA5u);

    dwgBufferW encodedBody;
    dwgBufferW encodedStrings;
    dwgBufferW encodedHandles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        parsed, version, &encodedBody, &encodedStrings, &encodedHandles));
    CHECK(encodedBody.bitCount() > 0u);
    CHECK_FALSE(encodedHandles.data().empty());

    dwgBufferW wrongVersionBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeRenderSettings(
        parsed, DRW::AC1024, &wrongVersionBody));
    CHECK(wrongVersionBody.data().empty());
}

// XRECORD encoder round-trip (ODA §20.4.105 — typed-pair payload).  Covers
// every type bucket the parser recognises: string (TV), point (3RD), double
// (RD), int16 (RS), int32 (RL), int64 (RLL), bool (RC), byte (RC), binary
// (RC+bytes), and data-block handle (RS code + RLL).  The post-body handle
// stream carries common prefix (parent + reactors + xdic) followed by a
// pair of soft-pointer handle refs the parser stores with code 0.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_XRecord::encodeDwg round-trips every value type bucket",
          "[dwg-write][object-encode][xrecord]") {
    DRW_XRecord src;
    src.handle       = 0x800;
    src.parentHandle = 0xC;
    src.m_cloning    = 1;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    // Data-block values, one per type bucket.  Codes hand-picked from the
    // xRecordCodeIs* predicates to cover every branch.
    src.m_values.emplace_back(1,   UTF8STRING("hello"));               // string
    src.m_values.emplace_back(10,  DRW_Coord{1.0, 2.0, 3.0});           // 3D point
    src.m_values.emplace_back(40,  3.14159);                            // double
    src.m_values.emplace_back(70,  static_cast<std::int32_t>(-5));            // int16
    src.m_values.emplace_back(90,  static_cast<std::int32_t>(0x12345678));    // int32
    src.m_values.emplace_back(160, static_cast<std::int64_t>(0x1122334455667788LL)); // int64
    src.m_values.emplace_back(290, static_cast<std::int32_t>(1));             // bool
    src.m_values.emplace_back(280, static_cast<std::int32_t>(7));             // byte
    src.m_values.emplace_back(310, std::vector<std::uint8_t>{0xDE, 0xAD, 0xBE, 0xEF}); // binary

    // Data-block handle — code 330 is in xRecordCodeIsHandle range; parser
    // stores the low 32 bits.
    src.m_handleValues.emplace_back(330, 0x1234u);

    // Handle-stream refs (code 0 marks handle-stream origin per parser).
    src.m_handleValues.emplace_back(0, 0xA1u);
    src.m_handleValues.emplace_back(0, 0xA2u);

    // AC1018 (R2004) — strings still inline (R2007 is the split point) and
    // the preamble's xDictFlag bit is honored, matching what the encoder
    // emits at the head of the handle stream.
    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/79 /* XRECORD */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeXRecord(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_XRecord dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_cloning == 1);
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0xCu);
    REQUIRE(dst.m_values.size() == 9u);

    // String
    REQUIRE(dst.m_values[0].code() == 1);
    REQUIRE(dst.m_values[0].type() == DRW_Variant::STRING);
    REQUIRE(std::string(dst.m_values[0].c_str()) == "hello");

    // 3D point
    REQUIRE(dst.m_values[1].code() == 10);
    REQUIRE(dst.m_values[1].type() == DRW_Variant::COORD);
    DRW_Coord* p = dst.m_values[1].coord();
    REQUIRE(p != nullptr);
    REQUIRE(p->x == Approx(1.0));
    REQUIRE(p->y == Approx(2.0));
    REQUIRE(p->z == Approx(3.0));

    // Double
    REQUIRE(dst.m_values[2].code() == 40);
    REQUIRE(dst.m_values[2].type() == DRW_Variant::DOUBLE);
    REQUIRE(dst.m_values[2].d_val() == Approx(3.14159));

    // Int16 (stored as INTEGER)
    REQUIRE(dst.m_values[3].code() == 70);
    REQUIRE(dst.m_values[3].type() == DRW_Variant::INTEGER);
    // Parser reads RS as unsigned then casts via static_cast<int>; -5 (0xFFFB)
    // round-trips back as 0xFFFB / 65531 in unsigned representation.
    REQUIRE(static_cast<std::uint16_t>(dst.m_values[3].i_val() & 0xFFFF) == 0xFFFBu);

    // Int32
    REQUIRE(dst.m_values[4].code() == 90);
    REQUIRE(dst.m_values[4].type() == DRW_Variant::INTEGER);
    REQUIRE(static_cast<std::uint32_t>(dst.m_values[4].i_val()) == 0x12345678u);

    // Int64 (160-169)
    REQUIRE(dst.m_values[5].code() == 160);
    REQUIRE(dst.m_values[5].type() == DRW_Variant::INTEGER64);
    REQUIRE(dst.m_values[5].i64_val() == 0x1122334455667788LL);

    // Bool
    REQUIRE(dst.m_values[6].code() == 290);
    REQUIRE(dst.m_values[6].type() == DRW_Variant::INTEGER);
    REQUIRE(dst.m_values[6].i_val() == 1);

    // Byte
    REQUIRE(dst.m_values[7].code() == 280);
    REQUIRE(dst.m_values[7].type() == DRW_Variant::INTEGER);
    REQUIRE(dst.m_values[7].i_val() == 7);

    // Binary
    REQUIRE(dst.m_values[8].code() == 310);
    REQUIRE(dst.m_values[8].type() == DRW_Variant::BINARY);
    const std::vector<std::uint8_t>* raw = dst.m_values[8].binary();
    REQUIRE(raw != nullptr);
    REQUIRE(raw->size() == 4u);
    REQUIRE((*raw)[0] == 0xDE);
    REQUIRE((*raw)[3] == 0xEF);

    // Handles: 1 data-block (code 330) + 2 handle-stream (code 0) = 3.
    REQUIRE(dst.m_handleValues.size() == 3u);
    REQUIRE(dst.m_handleValues[0].first  == 330);
    REQUIRE(dst.m_handleValues[0].second == 0x1234u);
    REQUIRE(dst.m_handleValues[1].first  == 0);
    REQUIRE(dst.m_handleValues[1].second == 0xA1u);
    REQUIRE(dst.m_handleValues[2].first  == 0);
    REQUIRE(dst.m_handleValues[2].second == 0xA2u);
}

TEST_CASE("DRW_XRecord rejects cloning values outside unsigned DWG BS",
          "[dwg-write][object-encode][xrecord][safety]") {
    DRW_XRecord source;
    source.m_cloning = std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW body;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeXRecord(
        source, DRW::AC1018, &body));
    CHECK(body.data().empty());

    source.m_cloning = -1;
    dwgBufferW negativeBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeXRecord(
        source, DRW::AC1018, &negativeBody));
    CHECK(negativeBody.data().empty());

    source.m_values.clear();
    source.m_values.emplace_back(40,
                                 std::numeric_limits<double>::quiet_NaN());
    dwgBufferW nonFiniteBody;
    nonFiniteBody.putRawChar8(0xA5);
    const auto beforeNonFinite = nonFiniteBody.data();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeXRecord(
        source, DRW::AC1018, &nonFiniteBody));
    CHECK(nonFiniteBody.data() == beforeNonFinite);
}

TEST_CASE("DRW_XRecord emits trailing object ids as soft pointers",
          "[dwg-write][object-encode][xrecord][handles]") {
    const DRW::Version version = DRW::AC1018;
    DRW_XRecord source;
    source.handle = 0xA60u;
    source.parentHandle = 0xCu;
    source.m_handleValues = {{0, 0xA1u}, {0, 0xA2u}};
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/79, source.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeXRecord(
        source, version, &encoded));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    REQUIRE(reader.getObjType(version) == 79);
    REQUIRE(reader.getRawLong32() == 0u);
    REQUIRE(reader.getHandle().ref == source.handle);
    REQUIRE(reader.getBitShort() == 0u); // EED size
    REQUIRE(reader.getBitLong() == 0);    // reactor count
    REQUIRE(reader.getBit() == 1u);       // no xdictionary
    REQUIRE(reader.getBitLong() == 0);    // XRECORD data bytes
    REQUIRE(reader.getBitShort() == 0u);  // cloning flags

    const dwgHandle parent = reader.getHandle();
    REQUIRE(parent.code == DRW::DwgSoftPointer);
    REQUIRE(parent.ref == source.parentHandle);
    const dwgHandle firstObjectId = reader.getHandle();
    const dwgHandle secondObjectId = reader.getHandle();
    CHECK(firstObjectId.code == DRW::DwgSoftPointer);
    CHECK(secondObjectId.code == DRW::DwgSoftPointer);
    CHECK(firstObjectId.ref == 0xA1u);
    CHECK(secondObjectId.ref == 0xA2u);
    CHECK(reader.isGood());
}

TEST_CASE("DRW_XRecord rejects non-finite raw values",
          "[dwg-read][object-encode][xrecord][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/79, /*handle=*/0xA54,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    dwgBufferW data;
    data.putRawShort16(40); // real value
    data.putRawDouble(std::numeric_limits<double>::quiet_NaN());
    encoded.putBitLong(static_cast<std::int32_t>(data.size()));
    encoded.putBytes(data.data().data(), data.size());
    encoded.putBitShort(0); // cloning
    encoded.putHandle(nullHandle()); // common parent handle

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_XRecord parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_values.empty());
    CHECK(parsed.m_handleValues.empty());
}

TEST_CASE("DRW_XRecord captures common handle references",
          "[dwg-read][object-encode][xrecord]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/79, /*handle=*/0xA50,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(0);
    encoded.putBitShort(1);
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(hardPtr(0xA1));
    encoded.putHandle(hardPtr(0xB1));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_XRecord parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x1Fu);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0xA1u);
    CHECK(parsed.xDictHandle == 0xB1u);
    CHECK(parsed.m_values.empty());
    CHECK(parsed.m_handleValues.empty());
}

TEST_CASE("DRW_XRecord cannot read its byte count from detached strings",
          "[dwg-read][object-encode][xrecord][safety]") {
    const DRW::Version ver = DRW::AC1021;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/79, /*handle=*/0xA56,
                       /*numReactors=*/0, /*xDictFlag=*/1);

    dwgBufferW strings;
    strings.putVariableText(ver, "");
    encoded.alignToByte();
    encoded.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        encoded.putBit(0);
    encoded.putRawShort16(static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u));
    encoded.putBit(1); // detached strings are present
    encoded.alignToByte();
    encoded.patchRawLong32AtBit(
        objectSizeBitOffset(encoded),
        static_cast<std::uint32_t>(encoded.size() * 8u));

    dwgBufferW handles;
    emitCommonHandlePrefix(handles, /*parentHandle=*/0x1F,
                           {}, /*xDictFlag=*/1);
    encoded.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_XRecord parsed;
    parsed.m_values.emplace_back(1, UTF8STRING("stale"));
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_values.empty());
    CHECK(parsed.m_handleValues.empty());
    CHECK(parsed.parentHandle == 0u);
}

TEST_CASE("DRW_XRecord rejects a truncated common handle stream",
          "[dwg-read][object-encode][xrecord]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/79, /*handle=*/0xA51,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(0);
    encoded.putBitShort(1);
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(hardPtr(0xA1));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_XRecord parsed;
    parsed.m_values.emplace_back(1, UTF8STRING("stale"));
    parsed.m_handleValues.emplace_back(0, 0x99u);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_values.empty());
    CHECK(parsed.m_handleValues.empty());
}

TEST_CASE("DWG common handles publish as one transaction",
          "[dwg-read][object-encode][handles][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/79, /*handle=*/0xA54,
                       /*numReactors=*/2, /*xDictFlag=*/0);
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(hardPtr(0xA1));
    // The second reactor and xdictionary handle are deliberately absent.

    const auto bytes = snapshot(encoded);
    dwgBuffer reader(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
    TableEntryProbe entry;
    REQUIRE(DrwObjectEncodeTestAccess::parse(entry, version, &reader));

    const std::uint64_t before = reader.getPosition();
    std::uint32_t parentHandle = 0xBEEFu;
    std::vector<std::uint32_t> reactors{0xCAFEu};
    std::uint32_t xDictHandle = 0xDEADu;

    CHECK_FALSE(DrwObjectEncodeTestAccess::readCommonHandles(
        entry, version, &reader, &parentHandle, &reactors, &xDictHandle));
    CHECK(reader.isGood());
    CHECK(reader.getPosition() == before);
    CHECK(parentHandle == 0xBEEFu);
    REQUIRE(reactors.size() == 1u);
    CHECK(reactors.front() == 0xCAFEu);
    CHECK(xDictHandle == 0xDEADu);
}

TEST_CASE("DRW_XRecord rejects an oversized data block",
          "[dwg-read][object-encode][xrecord]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/79, /*handle=*/0xA52,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(4096);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_XRecord parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
}

TEST_CASE("DRW_XRecord rejects values outside the declared data block",
          "[dwg-read][object-encode][xrecord]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/79, /*handle=*/0xA53,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(1); // one declared data byte
    encoded.putRawShort16(90); // a complete int32 value follows the boundary
    encoded.putRawLong32(0x0BADF00D);
    encoded.putBitShort(0); // cloning
    encoded.putHandle(hardPtr(0x1F)); // owner handle

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_XRecord parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_values.empty());
    CHECK(parsed.m_handleValues.empty());
}

// B-5: a binary XRECORD value larger than 255 bytes. A DWG binary chunk uses a
// single RC length, so the logical value must be emitted as consecutive pairs
// rather than truncated at the one-byte limit.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_XRecord::encodeDwg preserves an oversized binary value",
          "[dwg-write][object-encode][xrecord]") {
    DRW_XRecord src;
    src.handle       = 0x801;
    src.parentHandle = 0xC;
    src.m_cloning    = 1;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);

    std::vector<std::uint8_t> big(300);
    for (std::size_t i = 0; i < big.size(); ++i)
        big[i] = static_cast<std::uint8_t>(i & 0xFF);
    src.m_values.emplace_back(310, big);                                  // >255-byte binary
    src.m_values.emplace_back(90, static_cast<std::int32_t>(0x0BADF00D)); // FOLLOWING value

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/79, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeXRecord(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_XRecord dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_values.size() == 3u);
    REQUIRE(dst.m_values[0].code() == 310);
    REQUIRE(dst.m_values[0].type() == DRW_Variant::BINARY);
    REQUIRE(dst.m_values[1].code() == 310);
    REQUIRE(dst.m_values[1].type() == DRW_Variant::BINARY);
    const std::vector<std::uint8_t>* first = dst.m_values[0].binary();
    const std::vector<std::uint8_t>* second = dst.m_values[1].binary();
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first->size() == 255u);
    REQUIRE(second->size() == 45u);
    for (std::size_t i = 0; i < big.size(); ++i) {
        const auto* chunk = i < first->size() ? first : second;
        const std::size_t offset = i < first->size() ? i : i - first->size();
        REQUIRE((*chunk)[offset] == big[i]);
    }
    // The following value survives intact, proving the repeated binary pair
    // did not desynchronise the typed-value stream.
    REQUIRE(dst.m_values[2].code() == 90);
    REQUIRE(dst.m_values[2].type() == DRW_Variant::INTEGER);
    REQUIRE(dst.m_values[2].i_val() == 0x0BADF00D);
}

TEST_CASE("DRW_XRecord::encodeDwg rejects oversized text values",
          "[dwg-write][object-encode][xrecord][safety]") {
    const UTF8STRING tooLong(0x10000u, 'x');
    for (DRW::Version ver : {DRW::AC1018, DRW::AC1021}) {
        DRW_XRecord src;
        src.handle = 0x803;
        src.parentHandle = 0xC;
        src.m_values.emplace_back(1, tooLong);
        DrwObjectEncodeTestAccess::setNumReactors(src, 0);
        DrwObjectEncodeTestAccess::setXDictFlag(src, 1);

        dwgBufferW encoded;
        emitObjectPreamble(encoded, ver, /*oType=*/79, src.handle,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        CHECK_FALSE(DrwObjectEncodeTestAccess::encodeXRecord(
            src, ver, &encoded));
    }
}

TEST_CASE("DRW_XRecord preserves ordered data handles and 64-bit payloads",
          "[dwg-write][object-encode][xrecord]") {
    DRW_XRecord src;
    src.handle = 0x802;
    src.parentHandle = 0xC;
    src.m_cloning = 1;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);

    const std::uint64_t wideHandle = 0x123456789ABCDEF0ULL;
    src.m_dataEntries.emplace_back(90, static_cast<std::int32_t>(7));
    src.m_dataEntries.emplace_back(330, wideHandle);
    src.m_dataEntries.emplace_back(1, UTF8STRING("after-handle"));

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/79, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeXRecord(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_XRecord dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_dataEntries.size() == 3u);
    REQUIRE(dst.m_dataEntries[0].code() == 90);
    REQUIRE(dst.m_dataEntries[0].i_val() == 7);
    REQUIRE(dst.m_dataEntries[1].code() == 330);
    REQUIRE(dst.m_dataEntries[1].type() == DRW_Variant::INTEGER64);
    REQUIRE(static_cast<std::uint64_t>(dst.m_dataEntries[1].i64_val()) == wideHandle);
    REQUIRE(dst.m_dataEntries[2].code() == 1);
    REQUIRE(std::string(dst.m_dataEntries[2].c_str()) == "after-handle");

    REQUIRE(dst.m_values.size() == 2u);
    REQUIRE(dst.m_values[0].code() == 90);
    REQUIRE(dst.m_values[1].code() == 1);
    REQUIRE(dst.m_handleValues.size() == 1u);
    REQUIRE(dst.m_handleValues[0].first == 330);
    REQUIRE(dst.m_handleValues[0].second == 0x9ABCDEF0u);
}

TEST_CASE("DRW_XRecord replays a bounded unknown data block",
          "[dwg-read][dwg-write][object-encode][xrecord]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/79, /*handle=*/0xA54,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(4);
    encoded.putRawShort16(104); // outside the typed XRECORD classifier
    encoded.putRawShort16(0xBEEF);
    encoded.putBitShort(1);
    encoded.putHandle(hardPtr(0x1F));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_XRecord parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    REQUIRE(parsed.m_rawDataValid);
    REQUIRE(parsed.m_rawData.size() == 4u);
    REQUIRE(parsed.m_values.empty());
    REQUIRE(parsed.m_dataEntries.empty());

    dwgBufferW replay;
    emitObjectPreamble(replay, ver, /*oType=*/79, parsed.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeXRecord(parsed, ver, &replay));
    CHECK(snapshot(replay) == bytes);
}

// LAYOUT encoder round-trip (ODA §20.4.84).  Exercises the PlotSettings prefix +
// layout-specific body + common handle prefix + type-specific handle tail at
// AC1018 (R2004), where the shade-plot block + viewport-count branch is active
// and strings are still inline (so we can avoid the separate strBuf path).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::encodeDwg round-trips PlotSettings + layout body + handles",
          "[dwg-write][object-encode][layout]") {
    DRW_Layout src;
    src.handle           = 0x1A;
    src.parentHandle     = 0x1A2;            // ACAD_LAYOUT dictionary
    src.pageSetupName    = "PrintSetup";
    src.printerConfig    = "Adobe PDF";
    src.plotLayoutFlags  = 9;
    src.marginLeft       = 7.5;
    src.marginBottom     = 20.0;
    src.marginRight      = 7.5;
    src.marginTop        = 20.0;
    src.paperWidth       = 297.0;
    src.paperHeight      = 420.0;
    src.paperSize        = "ISO A3 (297.00 x 420.00 MM)";
    src.plotOriginX      = 0.0;
    src.plotOriginY      = 0.0;
    src.paperUnits       = 0;
    src.plotRotation     = 1;
    src.plotType         = 5;
    src.windowMinX       = -10.0;
    src.windowMinY       = -10.0;
    src.windowMaxX       = 287.0;
    src.windowMaxY       = 410.0;
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 25.4;             // 1 inch = 25.4 mm
    src.currentStyleSheet = "acad.ctb";
    src.scaleType        = 16;
    src.scaleFactor      = 25.4;
    src.paperImageOriginX = 1.0;
    src.paperImageOriginY = 2.0;
    src.shadePlotMode      = 1;              // R2004+
    src.shadePlotResLevel  = 4;
    src.shadePlotCustomDPI = 600;
    src.name        = "Layout1";
    src.tabOrder    = 2;
    src.layoutFlags = 4;
    src.ucsOrigin   = DRW_Coord{1.0, 2.0, 3.0};
    src.limMinX     = -5.0;
    src.limMinY     = -7.5;
    src.limMaxX     = 200.0;
    src.limMaxY     = 300.0;
    src.insPoint    = DRW_Coord{10.0, 20.0, 0.0};
    src.ucsXAxis    = DRW_Coord{1.0, 0.0, 0.0};
    src.ucsYAxis    = DRW_Coord{0.0, 1.0, 0.0};
    src.elevation   = 0.25;
    src.orthoViewType = 6;
    src.extMin      = DRW_Coord{-1.0, -2.0, 0.0};
    src.extMax      = DRW_Coord{500.0, 600.0, 0.0};
    src.viewportCount = 2;
    src.viewportHandles = {0x80, 0x81};
    // Type-specific handles use their specification-defined relationship
    // codes; leave code=0/size=0 and set only the reference values.
    src.plotViewHandle.ref              = 0;
    src.paperSpaceBlockRecordHandle.ref = 0x70;
    src.lastActiveViewportHandle.ref    = 0x71;
    src.baseUcsHandle.ref               = 0x72;
    src.namedUcsHandle.ref              = 0x73;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);  // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/82 /* LAYOUT */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeLayout(src, ver, &w));

    auto bytes = snapshot(w);
    REQUIRE(bytes.size() > 0);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    // PlotSettings prefix
    REQUIRE(dst.pageSetupName    == "PrintSetup");
    REQUIRE(dst.printerConfig    == "Adobe PDF");
    REQUIRE(dst.plotLayoutFlags  == 9);
    REQUIRE(dst.marginLeft       == Approx(7.5));
    REQUIRE(dst.marginBottom     == Approx(20.0));
    REQUIRE(dst.marginRight      == Approx(7.5));
    REQUIRE(dst.marginTop        == Approx(20.0));
    REQUIRE(dst.paperWidth       == Approx(297.0));
    REQUIRE(dst.paperHeight      == Approx(420.0));
    REQUIRE(dst.paperSize        == "ISO A3 (297.00 x 420.00 MM)");
    REQUIRE(dst.plotOriginX      == Approx(0.0));
    REQUIRE(dst.plotOriginY      == Approx(0.0));
    REQUIRE(dst.paperUnits       == 0);
    REQUIRE(dst.plotRotation     == 1);
    REQUIRE(dst.plotType         == 5);
    REQUIRE(dst.windowMinX       == Approx(-10.0));
    REQUIRE(dst.windowMaxY       == Approx(410.0));
    REQUIRE(dst.realWorldUnits   == Approx(1.0));
    REQUIRE(dst.drawingUnits     == Approx(25.4));
    REQUIRE(dst.currentStyleSheet == "acad.ctb");
    REQUIRE(dst.scaleType        == 16);
    REQUIRE(dst.scaleFactor      == Approx(25.4));
    REQUIRE(dst.paperImageOriginX == Approx(1.0));
    REQUIRE(dst.paperImageOriginY == Approx(2.0));
    REQUIRE(dst.shadePlotMode      == 1);
    REQUIRE(dst.shadePlotResLevel  == 4);
    REQUIRE(dst.shadePlotCustomDPI == 600);

    // Layout-specific
    REQUIRE(dst.name        == "Layout1");
    REQUIRE(dst.tabOrder    == 2);
    REQUIRE(dst.layoutFlags == 4);
    REQUIRE(dst.ucsOrigin.x == Approx(1.0));
    REQUIRE(dst.ucsOrigin.y == Approx(2.0));
    REQUIRE(dst.ucsOrigin.z == Approx(3.0));
    REQUIRE(dst.limMinX     == Approx(-5.0));
    REQUIRE(dst.limMaxX     == Approx(200.0));
    REQUIRE(dst.insPoint.x  == Approx(10.0));
    REQUIRE(dst.insPoint.y  == Approx(20.0));
    REQUIRE(dst.elevation   == Approx(0.25));
    REQUIRE(dst.orthoViewType == 6);
    REQUIRE(dst.extMin.x    == Approx(-1.0));
    REQUIRE(dst.extMax.y    == Approx(600.0));
    REQUIRE(dst.viewportCount == 2);

    // Handle prefix + type-specific tail.
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle)        == 0x1A2u);
    REQUIRE(dst.paperSpaceBlockRecordHandle.ref           == 0x70u);
    REQUIRE(dst.lastActiveViewportHandle.ref              == 0x71u);
    REQUIRE(dst.baseUcsHandle.ref                         == 0x72u);
    REQUIRE(dst.namedUcsHandle.ref                        == 0x73u);
    REQUIRE(dst.viewportHandles.size() == 2u);
    REQUIRE(dst.viewportHandles[0] == 0x80u);
    REQUIRE(dst.viewportHandles[1] == 0x81u);
}

// LAYOUT encoder round-trip with non-zero reactors + xdic — verifies that the
// common handle prefix is emitted FIRST in the right order so the parser's
// readCommonObjectHandles consumes parentHandle + reactors + xdic before any
// type-specific handle.  Locks in the alignment contract introduced by PR 2.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::encodeDwg emits common handle prefix before type-specific handles",
          "[dwg-write][object-encode][layout]") {
    DRW_Layout src;
    src.handle           = 0x1B;
    src.parentHandle     = 0x55;
    src.pageSetupName    = "Setup";
    src.printerConfig    = "Printer";
    src.paperWidth       = 297.0;
    src.paperHeight      = 210.0;
    src.paperSize        = "A4";
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 1.0;
    src.currentStyleSheet = "";
    src.scaleType        = 0;
    src.scaleFactor      = 1.0;
    src.shadePlotMode      = 0;
    src.shadePlotResLevel  = 0;
    src.shadePlotCustomDPI = 0;
    src.name        = "Layout1";
    src.tabOrder    = 1;
    src.layoutFlags = 0;
    src.viewportCount = 0;
    src.plotViewHandle.ref              = 0;
    src.paperSpaceBlockRecordHandle.ref = 0x90;
    src.lastActiveViewportHandle.ref    = 0x91;
    src.baseUcsHandle.ref               = 0x92;
    src.namedUcsHandle.ref              = 0x93;
    DrwObjectEncodeTestAccess::setNumReactors(src, 2);     // 2 reactors emitted as nulls
    DrwObjectEncodeTestAccess::setXDictFlag(src, 0);       // xdic present (null)

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/82, src.handle,
                       /*numReactors=*/2, /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeLayout(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    // parentHandle must be 0x55, not 0 (and not consumed by paperSpaceBlockRecord).
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x55u);
    // Type-specific handles land in the expected slots — proves the encoder
    // emitted (parent + 2 reactors + xdic) BEFORE the type-specific block.
    REQUIRE(dst.paperSpaceBlockRecordHandle.ref == 0x90u);
    REQUIRE(dst.lastActiveViewportHandle.ref    == 0x91u);
    REQUIRE(dst.baseUcsHandle.ref               == 0x92u);
    REQUIRE(dst.namedUcsHandle.ref              == 0x93u);
}

TEST_CASE("DRW_Layout AC1021 preserves shade-plot handle order transactionally",
          "[dwg-read][dwg-write][object-encode][layout][safety]") {
    const DRW::Version ver = DRW::AC1021;

    DRW_Layout source;
    source.handle = 0x1C0;
    source.parentHandle = 0x42;
    source.pageSetupName = "Setup";
    source.printerConfig = "Printer";
    source.paperSize = "A4";
    source.currentStyleSheet = "acad.ctb";
    source.name = "Sheet-A";
    source.viewportCount = 1;
    source.viewportHandles = {0x80};
    source.plotViewHandle.ref = 0x61;
    source.shadePlotHandle.ref = 0x62;
    source.paperSpaceBlockRecordHandle.ref = 0x70;
    source.lastActiveViewportHandle.ref = 0x71;
    source.baseUcsHandle.ref = 0x72;
    source.namedUcsHandle.ref = 0x73;
    DrwObjectEncodeTestAccess::setNumReactors(source, 1);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW body;
    emitObjectPreamble(body, ver, 82, source.handle, 1, 1);
    dwgBufferW strings;
    dwgBufferW handles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeLayout(
        source, ver, &body, &strings, &handles));

    // Verify the raw handle codes before the detached stream is appended to
    // the body. This catches accidental offset encoding of type-specific
    // fields even when the referenced values happen to round-trip unchanged.
    auto handleBytes = handles.data();
    dwgBuffer rawHandles(handleBytes.data(), handleBytes.size());
    const dwgHandle rawParent = rawHandles.getHandle();
    const dwgHandle rawReactor = rawHandles.getHandle();
    const dwgHandle rawPlotView = rawHandles.getHandle();
    const dwgHandle rawShadePlot = rawHandles.getHandle();
    const dwgHandle rawPaperSpace = rawHandles.getHandle();
    const dwgHandle rawActiveViewport = rawHandles.getHandle();
    const dwgHandle rawBaseUcs = rawHandles.getHandle();
    const dwgHandle rawNamedUcs = rawHandles.getHandle();
    REQUIRE(rawHandles.isGood());
    CHECK(rawParent.code == 4);
    CHECK(rawReactor.code == 0);
    CHECK(rawPlotView.code == 5);
    CHECK(rawShadePlot.code == 4);
    CHECK(rawPaperSpace.code == 4);
    CHECK(rawActiveViewport.code == 4);
    CHECK(rawBaseUcs.code == 5);
    CHECK(rawNamedUcs.code == 5);

    body.alignToByte();
    body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    body.putRawShort16(static_cast<std::uint16_t>(
        strings.data().size() * 8u + 7u));
    body.putBit(1); // string stream present
    body.alignToByte();
    body.patchRawLong32AtBit(
        10, static_cast<std::uint32_t>(body.size() * 8u));
    body.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Layout parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x42u);
    CHECK(parsed.plotViewHandle.ref == 0x61u);
    CHECK(parsed.shadePlotHandle.ref == 0x62u);
    CHECK(parsed.paperSpaceBlockRecordHandle.ref == 0x70u);
    CHECK(parsed.lastActiveViewportHandle.ref == 0x71u);
    CHECK(parsed.baseUcsHandle.ref == 0x72u);
    CHECK(parsed.namedUcsHandle.ref == 0x73u);
    REQUIRE(parsed.viewportHandles.size() == 1u);
    CHECK(parsed.viewportHandles.front() == 0x80u);

    DRW_Layout invalid = source;
    invalid.viewportCount = DRW_Layout::kMaxViewportCount + 1;
    invalid.viewportHandles.clear();
    dwgBufferW invalidBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeLayout(
        invalid, ver, &invalidBody, nullptr, nullptr));

    parsed.parentHandle = 0x99;
    parsed.shadePlotHandle.ref = 0x98;
    parsed.viewportHandles.push_back(0x97);
    REQUIRE(bytes.size() > 1);
    bytes.pop_back();
    dwgBuffer truncated(bytes.data(), bytes.size());
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &truncated));
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.shadePlotHandle.ref == 0u);
    CHECK(parsed.viewportHandles.empty());
}

// RASTERVARIABLES encoder round-trip (ODA §20.4.91).  Tiny body: 4 ints +
// common handle prefix.  No type-specific handle tail.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_RasterVariables::encodeDwg round-trips classVersion + frame + quality + units",
          "[dwg-write][object-encode][rastervariables]") {
    DRW_RasterVariables src;
    src.handle       = 0x900;
    src.parentHandle = 0xC;          // Named-objects dictionary
    src.m_classVersion = 0;
    src.m_imageFrame   = 40000;
    src.m_imageQuality = 40001;
    src.m_units        = 40002;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0 /* custom-class */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRasterVariables(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_RasterVariables dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_classVersion == 0);
    REQUIRE(dst.m_imageFrame   == 40000);
    REQUIRE(dst.m_imageQuality == 40001);
    REQUIRE(dst.m_units        == 40002);
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0xCu);
}

TEST_CASE("DRW_RasterVariables rejects values outside DWG field widths",
          "[dwg-write][object-encode][rastervariables][safety]") {
    DRW_RasterVariables source;
    source.m_classVersion = 11;
    source.m_imageFrame = 1;
    source.m_imageQuality = 1;
    source.m_units = 2;

    dwgBufferW classVersionBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeRasterVariables(
        source, DRW::AC1018, &classVersionBody));
    CHECK(snapshot(classVersionBody).empty());

    source.m_classVersion = 0;
    source.m_imageFrame = std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW bitShortBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeRasterVariables(
        source, DRW::AC1018, &bitShortBody));
    CHECK(snapshot(bitShortBody).empty());
}

TEST_CASE("DRW_RasterVariables rejects a truncated common handle stream",
          "[dwg-read][object-encode][rastervariables][safety]") {
    DRW_RasterVariables source;
    source.handle = 0x901u;
    source.parentHandle = 0xCu;
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, DRW::AC1018, /*oType=*/0, source.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRasterVariables(
        source, DRW::AC1018, &encoded));

    auto bytes = snapshot(encoded);
    REQUIRE(bytes.size() > 1);
    bytes.pop_back();

    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_RasterVariables parsed;
    parsed.m_classVersion = 7;
    parsed.m_imageFrame = 8;
    parsed.parentHandle = 0xBEEFu;
    parsed.reactorHandles.push_back(0xCAFEu);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_classVersion == 0);
    CHECK(parsed.m_imageFrame == 0);
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
}

// SORTENTSTABLE encoder round-trip (ODA §20.4.93).  Body: numEntries +
// inline sort handles.  Handle stream: common prefix + block owner + N
// entity handles.  N parallel arrays — sortHandles[i] corresponds to
// entityHandles[i] post-sort.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_SortEntsTable::encodeDwg round-trips sort + entity handles + block owner",
          "[dwg-write][object-encode][sortentstable]") {
    DRW_SortEntsTable src;
    src.handle           = 0xA00;
    src.parentHandle     = 0x1F;            // BLOCK_RECORD owner
    src.m_blockOwnerHandle = 0x1F;
    src.m_sortHandles    = {0x101, DRW::NoHandle, 0x103};
    src.m_entityHandles  = {0x201, 0x202, 0x203};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeSortEntsTable(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_SortEntsTable dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x1Fu);
    REQUIRE(dst.m_blockOwnerHandle == 0x1Fu);
    REQUIRE(dst.m_sortHandles.size() == 3u);
    REQUIRE(dst.m_sortHandles[0] == 0x101u);
    REQUIRE(dst.m_sortHandles[1] == DRW::NoHandle);
    REQUIRE(dst.m_sortHandles[2] == 0x103u);
    REQUIRE(dst.m_entityHandles.size() == 3u);
    REQUIRE(dst.m_entityHandles[0] == 0x201u);
    REQUIRE(dst.m_entityHandles[1] == 0x202u);
    REQUIRE(dst.m_entityHandles[2] == 0x203u);
}

// SORTENTSTABLE stores one exact pair per entry. A mismatched in-memory model
// cannot be serialized without manufacturing a row, so reject it before
// emitting any payload.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_SortEntsTable::encodeDwg rejects unequal sort/entity handle counts",
          "[dwg-write][object-encode][sortentstable]") {
    DRW_SortEntsTable src;
    src.handle             = 0xA10;
    src.parentHandle       = 0x1F;
    src.m_blockOwnerHandle = 0x1F;
    src.m_sortHandles      = {0x101, 0x102};                 // 2 sort handles
    src.m_entityHandles    = {0x201, 0x202, 0x203, 0x204};   // 4 entity handles
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);

    dwgBufferW w;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeSortEntsTable(
        src, DRW::AC1018, &w));
    CHECK(w.data().empty());
}

TEST_CASE("DRW_SortEntsTable rejects oversized handle lists before encoding",
          "[dwg-write][object-encode][sortentstable][safety]") {
    DRW_SortEntsTable table;
    table.m_sortHandles.resize(DRW_SortEntsTable::kMaxEntries + 1);

    dwgBufferW encoded;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeSortEntsTable(
        table, DRW::AC1027, &encoded));
    CHECK(encoded.data().empty());
}

TEST_CASE("DRW_SortEntsTable rejects a negative entry count before allocation",
          "[dwg-read][object-encode][sortentstable][safety]") {
    dwgBufferW encoded;
    emitObjectPreamble(encoded, DRW::AC1018, /*oType=*/0,
                       /*handle=*/0xA21u);
    encoded.putBitLong(-1);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SortEntsTable parsed;
    parsed.m_sortHandles.push_back(0xDEADu);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_sortHandles.empty());
    CHECK(parsed.m_entityHandles.empty());
}

TEST_CASE("DRW_SortEntsTable rejects an impossible entry count before allocation",
          "[dwg-read][object-encode][sortentstable][safety]") {
    dwgBufferW encoded;
    emitObjectPreamble(encoded, DRW::AC1018, /*oType=*/0,
                       /*handle=*/0xA22u, /*numReactors=*/0,
                       /*xDictFlag=*/1);
    encoded.putBitLong(static_cast<std::int32_t>(
        DRW_SortEntsTable::kMaxEntries));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SortEntsTable parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_sortHandles.empty());
    CHECK(parsed.m_entityHandles.empty());
}

TEST_CASE("DRW_SortEntsTable rejects a truncated common handle stream",
          "[dwg-read][object-encode][sortentstable][safety]") {
    DRW_SortEntsTable source;
    source.handle = 0xA20u;
    source.parentHandle = 0x1Fu;
    source.m_blockOwnerHandle = 0x1Fu;
    source.m_sortHandles = {0x101u};
    source.m_entityHandles = {0x201u};
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, DRW::AC1018, /*oType=*/0, source.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeSortEntsTable(
        source, DRW::AC1018, &encoded));

    auto bytes = snapshot(encoded);
    REQUIRE(bytes.size() > 1);
    bytes.pop_back();

    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SortEntsTable parsed;
    parsed.m_sortHandles.push_back(0xDEADu);
    parsed.m_entityHandles.push_back(0xBEEFu);
    parsed.m_blockOwnerHandle = 0xCAFEu;
    parsed.parentHandle = 0xABCDu;
    parsed.reactorHandles.push_back(0x1234u);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_sortHandles.empty());
    CHECK(parsed.m_entityHandles.empty());
    CHECK(parsed.m_blockOwnerHandle == 0u);
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.reactorHandles.empty());
}

TEST_CASE("DRW_SortEntsTable validates the AC1024 body boundary",
          "[dwg-read][object-encode][sortentstable][safety]") {
    const DRW::Version version = DRW::AC1024;
    DRW_SortEntsTable source;
    source.handle = 0xA30u;
    source.parentHandle = 0x1Fu;
    source.m_blockOwnerHandle = 0x1Fu;
    source.m_sortHandles = {0x101u, 0x102u};
    source.m_entityHandles = {0x201u, 0x202u};
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW data;
    data.putObjType(version, 0); // custom SORTENTSTABLE class
    data.putHandle(hardPtr(source.handle));
    data.putBitShort(0);         // no EED
    data.putBitLong(0);           // no reactors
    data.putBit(1);               // no extension dictionary
    dwgBufferW handles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeSortEntsTable(
        source, version, &data, nullptr, &handles));
    data.alignToByte();
    handles.alignToByte();
    data.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(data);
    const std::uint32_t handleBits =
        static_cast<std::uint32_t>(handles.data().size() * 8u);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SortEntsTable parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsed, version, &reader, handleBits));
    CHECK(parsed.m_sortHandles == source.m_sortHandles);
    CHECK(parsed.m_entityHandles == source.m_entityHandles);
    CHECK(parsed.m_blockOwnerHandle == source.m_blockOwnerHandle);

    // Move the declared data/handle boundary one byte into the body. The
    // first common handle then starts before the body ends and must be rejected
    // instead of being accepted as a valid ownership graph.
    REQUIRE(handleBits <= std::numeric_limits<std::uint32_t>::max() - 8u);
    dwgBuffer malformed(bytes.data(), bytes.size());
    DRW_SortEntsTable rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &malformed, handleBits + 8u));
    CHECK_FALSE(malformed.isGood());
    CHECK(rejected.m_sortHandles.empty());
    CHECK(rejected.m_entityHandles.empty());
}

// SPATIAL_FILTER encoder round-trip (ODA §20.4.94).  Boundary points + normal
// + origin + clip flags + optional clip distances + 4x3 transform matrices.
// Handle stream is just the common prefix (no type-specific handles).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_SpatialFilter::encodeDwg round-trips clip boundary + transforms",
          "[dwg-write][object-encode][spatialfilter]") {
    DRW_SpatialFilter src;
    src.handle       = 0xB00;
    src.parentHandle = 0x44;                // INSERT/IMAGE owner
    src.m_boundaryPoints = {
        DRW_Coord{0.0, 0.0, 0.0},
        DRW_Coord{10.0, 0.0, 0.0},
        DRW_Coord{10.0, 10.0, 0.0},
        DRW_Coord{0.0, 10.0, 0.0}
    };
    src.m_normal = DRW_Coord{0.0, 0.0, 1.0};
    src.m_origin = DRW_Coord{0.0, 0.0, 0.0};
    src.m_displayBoundary = true;
    src.m_clipFrontPlane  = true;
    src.m_frontDistance   = 5.0;
    src.m_clipBackPlane   = false;
    src.m_backDistance    = 0.0;
    src.m_inverseInsertTransform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0
    };
    src.m_insertTransform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0
    };
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeSpatialFilter(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_SpatialFilter dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x44u);
    REQUIRE(dst.m_boundaryPoints.size() == 4u);
    REQUIRE(dst.m_boundaryPoints[0].x == Approx(0.0));
    REQUIRE(dst.m_boundaryPoints[1].x == Approx(10.0));
    REQUIRE(dst.m_boundaryPoints[2].y == Approx(10.0));
    REQUIRE(dst.m_boundaryPoints[3].y == Approx(10.0));
    REQUIRE(dst.m_normal.z   == Approx(1.0));
    REQUIRE(dst.m_origin.x   == Approx(0.0));
    REQUIRE(dst.m_displayBoundary == true);
    REQUIRE(dst.m_clipFrontPlane  == true);
    REQUIRE(dst.m_frontDistance   == Approx(5.0));
    REQUIRE(dst.m_clipBackPlane   == false);
    REQUIRE(dst.m_inverseInsertTransform.size() == 12u);
    REQUIRE(dst.m_inverseInsertTransform[0]  == Approx(1.0));
    REQUIRE(dst.m_inverseInsertTransform[5]  == Approx(1.0));
    REQUIRE(dst.m_inverseInsertTransform[10] == Approx(1.0));
    REQUIRE(dst.m_insertTransform.size() == 12u);
    REQUIRE(dst.m_insertTransform[5] == Approx(1.0));
}

TEST_CASE("DRW_SpatialFilter rejects an unrepresentable boundary count",
          "[dwg-write][object-encode][spatialfilter][safety]") {
    DRW_SpatialFilter filter;
    filter.m_boundaryPoints.resize(DRW_SpatialFilter::kMaxBoundaryPoints + 1);

    dwgBufferW encoded;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeSpatialFilter(
        filter, DRW::AC1027, &encoded));
    CHECK(encoded.data().empty());
}

TEST_CASE("DRW_SpatialFilter rejects a boundary count outside the body",
          "[dwg-read][object-encode][spatialfilter][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW body;
    emitObjectPreamble(body, version, /*oType=*/0, /*handle=*/0xB03,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    body.putBitShort(static_cast<std::uint16_t>(
        DRW_SpatialFilter::kMaxBoundaryPoints));
    body.patchRawLong32AtBit(objectSizeBitOffset(body), body.bitCount());
    body.putBitShort(0); // keep physical bytes available after the body
    emitCommonHandlePrefix(body, /*parentHandle=*/0x44,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SpatialFilter parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_boundaryPoints.empty());
}

TEST_CASE("DRW_SpatialFilter rejects non-finite geometry transactionally",
          "[dwg-read][dwg-write][object-encode][spatialfilter][safety]") {
    const DRW::Version ver = DRW::AC1018;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    DRW_SpatialFilter invalid;
    invalid.m_normal = DRW_Coord{nan, 0.0, 1.0};
    dwgBufferW rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeSpatialFilter(
        invalid, ver, &rejected));
    CHECK(rejected.data().empty());

    const auto makeRecord = [&](bool badNormal) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0xB02,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putBitShort(0); // boundary point count
        body.put3BitDouble(badNormal
                               ? DRW_Coord{nan, 0.0, 1.0}
                               : DRW_Coord{0.0, 0.0, 1.0});
        body.put3BitDouble(DRW_Coord{0.0, 0.0, 0.0});
        body.putBitShort(0); // display boundary
        body.putBitShort(0); // front clip
        body.putBitShort(0); // back clip
        for (int i = 0; i < 24; ++i)
            body.putBitDouble(i == 5 && !badNormal ? nan : 0.0);
        emitCommonHandlePrefix(body, 0x44, {}, /*xDictFlag=*/1);
        return snapshot(body);
    };

    for (auto bytes : {makeRecord(true), makeRecord(false)}) {
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_SpatialFilter parsed;
        parsed.m_normal = DRW_Coord{9.0, 9.0, 9.0};
        parsed.m_boundaryPoints.push_back(DRW_Coord{8.0, 8.0, 0.0});
        parsed.parentHandle = 0x98;

        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.m_boundaryPoints.empty());
        CHECK(parsed.m_normal.x == Approx(0.0));
        CHECK(parsed.m_normal.y == Approx(0.0));
        CHECK(parsed.m_normal.z == Approx(1.0));
        CHECK(parsed.parentHandle == 0);
    }
}

TEST_CASE("DRW_SpatialFilter rejects a truncated common handle stream",
          "[dwg-read][object-encode][spatialfilter][safety]") {
    const DRW::Version ver = DRW::AC1018;

    dwgBufferW valid;
    emitObjectPreamble(valid, ver, /*oType=*/0, /*handle=*/0xB01,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    valid.putBitShort(0); // boundary point count
    valid.put3BitDouble(DRW_Coord{0.0, 0.0, 1.0});
    valid.put3BitDouble(DRW_Coord{0.0, 0.0, 0.0});
    valid.putBitShort(0); // display boundary
    valid.putBitShort(0); // front clip
    valid.putBitShort(0); // back clip
    for (int i = 0; i < 24; ++i)
        valid.putBitDouble(i % 5 == 0 ? 1.0 : 0.0);
    emitCommonHandlePrefix(valid, /*parentHandle=*/0x44,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);

    auto validBytes = snapshot(valid);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_SpatialFilter parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.parentHandle == 0x44);

    auto truncatedBytes = validBytes;
    REQUIRE(truncatedBytes.size() > 1);
    truncatedBytes.pop_back();
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    parsed.m_boundaryPoints.push_back(DRW_Coord{99.0, 99.0, 0.0});
    parsed.m_normal = DRW_Coord{9.0, 9.0, 9.0};
    parsed.parentHandle = 0x99;
    parsed.reactorHandles.push_back(0x98);
    parsed.xDictHandle = 0x97;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_boundaryPoints.empty());
    CHECK(parsed.m_normal.x == Approx(0.0));
    CHECK(parsed.m_normal.y == Approx(0.0));
    CHECK(parsed.m_normal.z == Approx(1.0));
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
}

TEST_CASE("DRW_SpatialFilter rejects a body boundary inside transform data",
          "[dwg-read][object-encode][spatialfilter][safety]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0xB04,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitShort(0); // no boundary points
    encoded.put3BitDouble(DRW_Coord{0.0, 0.0, 1.0});
    encoded.put3BitDouble(DRW_Coord{0.0, 0.0, 0.0});
    encoded.putBitShort(0); // display boundary
    encoded.putBitShort(0); // front clip
    encoded.putBitShort(0); // back clip
    encoded.putBitDouble(1.0); // only one matrix value is in the body
    const std::uint32_t bodyEndBit = encoded.bitCount();
    for (int i = 1; i < 24; ++i)
        encoded.putBitDouble(0.0);
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x44,
                           /*reactorHandles=*/{}, /*xDictFlag=*/1);
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), bodyEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SpatialFilter parsed;
    parsed.m_normal = DRW_Coord{9.0, 9.0, 9.0};
    parsed.m_insertTransform.push_back(7.0);
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_boundaryPoints.empty());
    CHECK(parsed.m_insertTransform.empty());
    CHECK(parsed.m_normal.z == Approx(1.0));
    CHECK(parsed.parentHandle == 0u);
}

// GEODATA encoder round-trip (ODA §20.4.78).  Test the m_version=2/3 path
// (R2010+ schema) since that's what current AutoCAD releases write.
// Body: version + coordinatesType + version-specific fields + 3 observation
// TVs + N mesh points + N mesh faces; the host-block handle and common handles
// follow the body in the DWG handle stream.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_GeoData::encodeDwg round-trips version=2 metadata + mesh",
          "[dwg-write][object-encode][geodata]") {
    DRW_GeoData src;
    src.handle       = 0xC00;
    src.parentHandle = 0x1A;
    src.m_version    = 2;
    src.m_hostBlockHandle = 0x1F;
    src.m_coordinatesType = 1;
    src.m_designPoint    = DRW_Coord{0.0, 0.0, 0.0};
    src.m_referencePoint = DRW_Coord{500000.0, 4000000.0, 0.0};
    src.m_horizontalUnitScale = 0.3048;       // foot -> meter
    src.m_horizontalUnits     = 6;            // 6 = meters
    src.m_verticalUnitScale   = 1.0;
    src.m_verticalUnits       = 6;
    src.m_upDirection         = DRW_Coord{0.0, 0.0, 1.0};
    src.m_northDirection      = DRW_Coord{0.0, 1.0, 0.0};
    src.m_scaleEstimationMethod = 1;
    src.m_userSpecifiedScaleFactor = 1.0;
    src.m_enableSeaLevelCorrection = false;
    src.m_seaLevelElevation = 0.0;
    src.m_coordinateProjectionRadius = 6378137.0;
    src.m_coordinateSystemDefinition = "EPSG:3857";
    src.m_geoRssTag = "<rss/>";
    src.m_observationFromTag    = "from";
    src.m_observationToTag      = "to";
    src.m_observationCoverageTag = "coverage";
    src.m_points.push_back({DRW_Coord{0.0, 0.0, 0.0}, DRW_Coord{1.0, 1.0, 0.0}});
    src.m_points.push_back({DRW_Coord{1.0, 0.0, 0.0}, DRW_Coord{2.0, 1.0, 0.0}});
    src.m_faces.push_back({1, 2, 3});
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeGeoData(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_GeoData dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x1Au);
    REQUIRE(dst.m_version == 2);
    REQUIRE(dst.m_hostBlockHandle == 0x1Fu);
    REQUIRE(dst.m_coordinatesType == 1);
    REQUIRE(dst.m_designPoint.x    == Approx(0.0));
    REQUIRE(dst.m_referencePoint.x == Approx(500000.0));
    REQUIRE(dst.m_referencePoint.y == Approx(4000000.0));
    REQUIRE(dst.m_horizontalUnitScale == Approx(0.3048));
    REQUIRE(dst.m_horizontalUnits     == 6);
    REQUIRE(dst.m_verticalUnitScale   == Approx(1.0));
    REQUIRE(dst.m_verticalUnits       == 6);
    REQUIRE(dst.m_upDirection.z       == Approx(1.0));
    REQUIRE(dst.m_northDirection.y    == Approx(1.0));
    REQUIRE(dst.m_scaleEstimationMethod == 1);
    REQUIRE(dst.m_userSpecifiedScaleFactor == Approx(1.0));
    REQUIRE(dst.m_enableSeaLevelCorrection == false);
    REQUIRE(dst.m_coordinateProjectionRadius == Approx(6378137.0));
    REQUIRE(dst.m_coordinateSystemDefinition == "EPSG:3857");
    REQUIRE(dst.m_geoRssTag == "<rss/>");
    REQUIRE(dst.m_observationFromTag == "from");
    REQUIRE(dst.m_observationToTag   == "to");
    REQUIRE(dst.m_observationCoverageTag == "coverage");
    REQUIRE(dst.m_points.size() == 2u);
    REQUIRE(dst.m_points[0].m_source.x      == Approx(0.0));
    REQUIRE(dst.m_points[0].m_destination.y == Approx(1.0));
    REQUIRE(dst.m_points[1].m_source.x      == Approx(1.0));
    REQUIRE(dst.m_points[1].m_destination.x == Approx(2.0));
    REQUIRE(dst.m_faces.size() == 1u);
    REQUIRE(dst.m_faces[0].m_index1 == 1);
    REQUIRE(dst.m_faces[0].m_index2 == 2);
    REQUIRE(dst.m_faces[0].m_index3 == 3);
}

TEST_CASE("DRW_GeoData rejects a truncated handle stream transactionally",
          "[dwg-read][object-encode][geodata][safety]") {
    const DRW::Version ver = DRW::AC1018;
    DRW_GeoData source;
    source.handle = 0xC01;
    source.parentHandle = 0x1A;
    source.m_version = 2;
    source.m_hostBlockHandle = 0x1F;
    source.m_coordinatesType = 2;
    source.m_coordinateSystemDefinition = "EPSG:3857";
    source.m_observationFromTag = "from";
    source.m_observationToTag = "to";
    source.m_observationCoverageTag = "coverage";
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, source.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeGeoData(source, ver, &encoded));

    auto validBytes = snapshot(encoded);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_GeoData parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.parentHandle == 0x1A);
    CHECK(parsed.m_hostBlockHandle == 0x1Fu);

    REQUIRE(validBytes.size() > 1);
    validBytes.pop_back();
    dwgBuffer truncatedReader(validBytes.data(), validBytes.size());
    parsed.m_version = 99;
    parsed.m_hostBlockHandle = 0x99;
    parsed.m_points.push_back({DRW_Coord{9.0, 9.0, 0.0},
                               DRW_Coord{8.0, 8.0, 0.0}});
    parsed.parentHandle = 0x98;
    parsed.reactorHandles.push_back(0x97);
    parsed.xDictHandle = 0x96;

    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(parsed.m_version == 0);
    CHECK(parsed.m_hostBlockHandle == 0u);
    CHECK(parsed.m_points.empty());
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.xDictHandle == 0u);
}

// DICTIONARYVAR encoder round-trip (ODA §20.4.46).  Body: schema RC + value TV.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_DictionaryVar::encodeDwg round-trips schema + value",
          "[dwg-write][object-encode][dictionaryvar]") {
    DRW_DictionaryVar src;
    src.handle       = 0xD00;
    src.parentHandle = 0x40;
    src.m_schema     = 0;
    src.m_value      = "Standard";
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionaryVar(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_DictionaryVar dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_schema == 0);
    REQUIRE(dst.m_value  == "Standard");
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x40u);

    auto truncatedBytes = bytes;
    REQUIRE(truncatedBytes.size() > 8);
    truncatedBytes.resize(truncatedBytes.size() - 8);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    DRW_DictionaryVar truncated;
    truncated.m_schema = 7;
    truncated.m_value = "stale";
    truncated.parentHandle = 0xBEEFu;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        truncated, ver, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncated.m_schema == 0);
    CHECK(truncated.m_value.empty());
    CHECK(truncated.parentHandle == 0u);
}

TEST_CASE("DRW_DictionaryVar rejects a schema outside DWG RC",
          "[dwg-write][object-encode][dictionaryvar][safety]") {
    DRW_DictionaryVar source;
    source.m_schema = 0x100;

    dwgBufferW body;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDictionaryVar(
        source, DRW::AC1018, &body));
    CHECK(snapshot(body).empty());

    source.m_schema = -1;
    dwgBufferW negativeSchemaBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDictionaryVar(
        source, DRW::AC1018, &negativeSchemaBody));
    CHECK(snapshot(negativeSchemaBody).empty());
}

// DICTIONARYWDFLT encoder round-trip (ODA §20.4.45).  Inherits DICTIONARY's
// body + handle stream and appends a default-entry handle.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_DictionaryWithDefault::encodeDwg round-trips dictionary + default entry",
          "[dwg-write][object-encode][dictionarywdflt]") {
    DRW_DictionaryWithDefault src;
    src.handle       = 0xE00;
    src.parentHandle = 0xC;
    src.cloning      = 1;
    src.hardOwner    = 1;
    src.m_entries.push_back({"ENTRY_A", 0x71});
    src.m_entries.push_back({"ENTRY_B", 0x72});
    src.m_defaultEntryHandle = 0x71;       // default points at ENTRY_A
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionaryWDflt(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_DictionaryWithDefault dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.cloning      == 1);
    REQUIRE(dst.hardOwner    == 1);
    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0xCu);
    REQUIRE(dst.m_entries.size() == 2u);
    REQUIRE(dst.m_entries[0].m_name == "ENTRY_A");
    REQUIRE(dst.m_entries[0].m_handle == 0x71u);
    REQUIRE(dst.m_entries[1].m_name == "ENTRY_B");
    REQUIRE(dst.m_entries[1].m_handle == 0x72u);
    REQUIRE(dst.m_defaultEntryHandle == 0x71u);
}

TEST_CASE("DRW_DictionaryWithDefault preserves the R13/R14 cloning field",
          "[dwg][r13][r14][dictionary][safety]") {
    const DRW::Version versions[] = {DRW::AC1012, DRW::AC1014};
    for (const DRW::Version ver : versions) {
        DRW_DictionaryWithDefault src;
        src.handle       = 0xE10;
        src.parentHandle = 0xC;
        src.cloning      = 1;
        src.hardOwner    = 0;
        src.m_entries.push_back({"Normal", 0xF});
        src.m_defaultEntryHandle = 0xF;
        DrwObjectEncodeTestAccess::setNumReactors(src, 0);
        // R13/R14 do not carry xDictFlag in the object body; the common
        // handle stream therefore includes the legacy xdictionary slot.
        DrwObjectEncodeTestAccess::setXDictFlag(src, 0);

        dwgBufferW w;
        emitLegacyObjectPreamble(w, ver, /*oType=*/0, src.handle);
        REQUIRE(DrwObjectEncodeTestAccess::encodeDictionaryWDflt(src, ver, &w));

        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DictionaryWithDefault dst;
        REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

        CHECK(dst.cloning == 1);
        CHECK(dst.hardOwner == 0);
        REQUIRE(dst.m_entries.size() == 1u);
        CHECK(dst.m_entries[0].m_name == "Normal");
        CHECK(dst.m_entries[0].m_handle == 0xFu);
        CHECK(dst.m_defaultEntryHandle == 0xFu);
    }
}

TEST_CASE("DRW_DictionaryWithDefault reset clears common DWG state",
          "[dwg][dictionary][safety][reset]") {
    DRW_DictionaryWithDefault dictionary;
    dictionary.handle = 0xE10;
    dictionary.parentHandle = 0xC;
    dictionary.xDictHandle = 0xD;
    dictionary.reactorHandles.push_back(0xE);
    dictionary.m_entries.push_back({"Stale", 0xF});
    dictionary.m_defaultEntryHandle = 0xF;
    dictionary.cloning = 1;
    dictionary.hardOwner = 1;
    DrwObjectEncodeTestAccess::setNumReactors(dictionary, 2);
    DrwObjectEncodeTestAccess::setXDictFlag(dictionary, 1);

    dictionary.reset();

    CHECK(dictionary.handle == 0u);
    CHECK(dictionary.parentHandle == 0);
    CHECK(dictionary.xDictHandle == 0u);
    CHECK(dictionary.reactorHandles.empty());
    CHECK(DrwObjectEncodeTestAccess::numReactors(dictionary) == 0);
    CHECK(DrwObjectEncodeTestAccess::xDictFlag(dictionary) == 0);
    CHECK(dictionary.m_entries.empty());
    CHECK(dictionary.m_defaultEntryHandle == 0u);
    CHECK(dictionary.cloning == 0);
    CHECK(dictionary.hardOwner == 0);
}

TEST_CASE("DRW_Dictionary rejects cloning values outside unsigned DWG BS",
          "[dwg-write][object-encode][dictionary][safety]") {
    DRW_Dictionary source;
    source.cloning = std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW body;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDictionary(
        source, DRW::AC1018, &body));
    CHECK(body.data().empty());

    source.cloning = -1;
    dwgBufferW negativeBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDictionary(
        source, DRW::AC1018, &negativeBody));
    CHECK(negativeBody.data().empty());

    source.cloning = 0;
    source.hardOwner = 2;
    dwgBufferW invalidHardOwner;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDictionary(
        source, DRW::AC1018, &invalidHardOwner));
    CHECK(invalidHardOwner.data().empty());
}

TEST_CASE("DRW_DictionaryWithDefault requires a default object reference",
          "[dwg-write][object-encode][dictionarywdflt][safety]") {
    DRW_DictionaryWithDefault dictionary;
    dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    dictionary.m_entries = {{"ENTRY", 0x71u}};

    dwgBufferW body;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeDictionaryWDflt(
        dictionary, DRW::AC1018, &body));
    CHECK(snapshot(body).empty());
}

// IDBUFFER encoder round-trip (ODA §20.4.79).  Body: class_version RC +
// numIds BL.  Handle stream: common prefix + N object handles.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_IDBuffer::encodeDwg round-trips object id list",
          "[dwg-write][object-encode][idbuffer]") {
    DRW_IDBuffer src;
    src.handle       = 0xF00;
    src.parentHandle = 0x10;
    src.classVersion = 0;
    src.objIds       = {0x101, 0x102, 0x103, 0x104};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeIDBuffer(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_IDBuffer dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x10u);
    REQUIRE(dst.classVersion == 0);
    REQUIRE(dst.objIds.size() == 4u);
    REQUIRE(dst.objIds[0] == 0x101u);
    REQUIRE(dst.objIds[1] == 0x102u);
    REQUIRE(dst.objIds[2] == 0x103u);
    REQUIRE(dst.objIds[3] == 0x104u);
}

TEST_CASE("DRW_IDBuffer rejects oversized object-id lists before encoding",
          "[dwg-write][object-encode][idbuffer][safety]") {
    DRW_IDBuffer buffer;
    buffer.objIds.resize(DRW_IDBuffer::kMaxObjectIds + 1);

    dwgBufferW encoded;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeIDBuffer(
        buffer, DRW::AC1027, &encoded));
    CHECK(encoded.data().empty());
}

// LAYER_INDEX encoder round-trip (ODA §20.4.83).  Body: timestamps + per-
// layer index + name.  Handle stream: common prefix + per-layer entry handle.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_LayerIndex::encodeDwg round-trips timestamps + per-layer entries",
          "[dwg-write][object-encode][layerindex]") {
    DRW_LayerIndex src;
    src.handle       = 0x1000;
    src.parentHandle = 0x10;
    src.timestamp1   = 0x99887766u;
    src.timestamp2   = 0x11223344u;
    src.entries.push_back({1, "0",       0x301u});
    src.entries.push_back({2, "Layer1",  0x302u});
    src.entries.push_back({3, "Layer2",  0x303u});
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeLayerIndex(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_LayerIndex dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x10u);
    REQUIRE(dst.timestamp1 == 0x99887766u);
    REQUIRE(dst.timestamp2 == 0x11223344u);
    REQUIRE(dst.entries.size() == 3u);
    REQUIRE(dst.entries[0].indexLong == 1);
    REQUIRE(dst.entries[0].name == "0");
    REQUIRE(dst.entries[0].entryHandle == 0x301u);
    REQUIRE(dst.entries[1].indexLong == 2);
    REQUIRE(dst.entries[1].name == "Layer1");
    REQUIRE(dst.entries[1].entryHandle == 0x302u);
    REQUIRE(dst.entries[2].indexLong == 3);
    REQUIRE(dst.entries[2].name == "Layer2");
    REQUIRE(dst.entries[2].entryHandle == 0x303u);
}

TEST_CASE("DRW_LayerIndex rejects oversized entry lists before encoding",
          "[dwg-write][object-encode][layerindex][safety]") {
    DRW_LayerIndex index;
    index.entries.resize(DRW_LayerIndex::kMaxEntries + 1);

    dwgBufferW encoded;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeLayerIndex(
        index, DRW::AC1027, &encoded));
    CHECK(encoded.data().empty());
}

TEST_CASE("DWG object carriers reject invalid common state transactionally",
          "[dwg-write][object-encode][common-state][safety]") {
    DRW_RasterVariables raster;
    DRW_SortEntsTable sortents;
    DRW_DictionaryVar dictionaryVar;
    DRW_IDBuffer idBuffer;
    DRW_LayerIndex layerIndex;

    auto rejectInvalidState = [](auto& object, auto encode) {
        DrwObjectEncodeTestAccess::setNumReactors(object, -1);
        dwgBufferW negativeReactors;
        CHECK_FALSE(encode(object, &negativeReactors));
        CHECK(negativeReactors.data().empty());

        DrwObjectEncodeTestAccess::setNumReactors(object, 0);
        DrwObjectEncodeTestAccess::setXDictFlag(object, 2);
        dwgBufferW invalidXDict;
        CHECK_FALSE(encode(object, &invalidXDict));
        CHECK(invalidXDict.data().empty());
    };

    rejectInvalidState(raster, [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeRasterVariables(
            object, DRW::AC1027, buffer);
    });
    rejectInvalidState(sortents, [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeSortEntsTable(
            object, DRW::AC1027, buffer);
    });
    rejectInvalidState(dictionaryVar,
                       [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeDictionaryVar(
            object, DRW::AC1027, buffer);
    });
    rejectInvalidState(idBuffer, [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeIDBuffer(
            object, DRW::AC1027, buffer);
    });
    rejectInvalidState(layerIndex, [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeLayerIndex(
            object, DRW::AC1027, buffer);
    });

    DRW_XRecord xrecord;
    rejectInvalidState(xrecord, [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeXRecord(
            object, DRW::AC1027, buffer);
    });
    DRW_Layout layout;
    rejectInvalidState(layout, [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeLayout(
            object, DRW::AC1027, buffer);
    });
    DRW_Dictionary dictionary;
    rejectInvalidState(dictionary,
                       [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeDictionary(
            object, DRW::AC1027, buffer);
    });
    DRW_Field field;
    rejectInvalidState(field, [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeField(
            object, DRW::AC1027, buffer);
    });
    DRW_FieldList fieldList;
    rejectInvalidState(fieldList,
                       [](const auto& object, dwgBufferW* buffer) {
        return DrwObjectEncodeTestAccess::encodeFieldList(
            object, DRW::AC1027, buffer);
    });
}

// SPATIAL_INDEX encoder round-trip (ODA §20.4.95).  The object-size field
// delimits the opaque body tail from the common handle stream.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_SpatialIndex::encodeDwg round-trips timestamps at AC1018",
          "[dwg-write][object-encode][spatialindex]") {
    DRW_SpatialIndex src;
    src.handle     = 0x1100;
    src.parentHandle = 0x42;
    src.timestamp1 = 0xDEADBEEFu;
    src.timestamp2 = 0xCAFEBABEu;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeSpatialIndex(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_SpatialIndex dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.timestamp1 == 0xDEADBEEFu);
    REQUIRE(dst.timestamp2 == 0xCAFEBABEu);
    REQUIRE(dst.parentHandle == 0x42u);
}

TEST_CASE("DRW_SpatialIndex replays a bounded opaque body tail",
          "[dwg-read][dwg-write][object-encode][spatialindex]") {
    const DRW::Version ver = DRW::AC1018;
    dwgBufferW source;
    emitObjectPreamble(source, ver, /*oType=*/0, /*handle=*/0x1101,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    source.putBitLong(123);
    source.putBitLong(456);
    source.putBit(1);
    source.putBit(0);
    source.putBit(1);
    source.putBit(1);
    const std::uint32_t objectBodyEnd = source.bitCount();
    source.patchRawLong32AtBit(2, objectBodyEnd);
    source.putHandle(hardPtr(0x42));

    auto bytes = snapshot(source);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_SpatialIndex parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    REQUIRE(parsed.parentHandle == 0x42u);

    dwgBufferW replay;
    REQUIRE(DrwObjectEncodeTestAccess::encodeSpatialIndex(
        parsed, ver, &replay));

    dwgBufferW expected;
    expected.putBitLong(123);
    expected.putBitLong(456);
    expected.putBit(1);
    expected.putBit(0);
    expected.putBit(1);
    expected.putBit(1);
    expected.putHandle(hardPtr(0x42));
    CHECK(replay.data() == expected.data());
}

TEST_CASE("DRW_SpatialIndex bounds common handle metadata",
          "[dwg-write][object-encode][spatialindex][safety]") {
    DRW_SpatialIndex source;
    source.reactorHandles.resize(dwgSafety::MaxReactorCount + 1);
    dwgBufferW body;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeSpatialIndex(
        source, DRW::AC1021, &body));
    CHECK(body.data().empty());

    source.reactorHandles.clear();
    DrwObjectEncodeTestAccess::setXDictFlag(source, 2);
    body.reset();
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeSpatialIndex(
        source, DRW::AC1021, &body));
    CHECK(body.data().empty());
}

// FIELD encoder round-trip (ODA §20.4.76) — exercises dataType=2 (double)
// value branch, the simplest CadValue path that round-trips cleanly at
// AC1018 (pre-R2007 omits formatFlags + unitType + format/value strings).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Field::encodeDwg round-trips evaluator + double value + child handles",
          "[dwg-write][object-encode][field]") {
    DRW_Field src;
    src.handle       = 0x1200;
    src.parentHandle = 0x44;
    src.m_evaluatorId           = "AcVariable";
    src.m_fieldCode             = "%<\\AcVar Area>%";
    src.m_formatString          = "%lf";
    src.m_evaluationOptionFlags = 3;
    src.m_filingOptionFlags     = 1;
    src.m_fieldStateFlags       = 0;
    src.m_evaluationStatusFlags = 0;
    src.m_evaluationErrorCode   = 0;
    src.m_evaluationErrorMessage = "";
    src.m_value.m_dataType = 2;
    src.m_value.m_value.addDouble(140, 42.5);
    src.m_valueString       = "42.5";
    src.m_valueStringLength = 4;
    src.m_childHandles  = {0x401u, 0x402u};
    src.m_objectHandles = {0x501u};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeField(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Field dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x44u);
    REQUIRE(dst.m_evaluatorId == "AcVariable");
    REQUIRE(dst.m_fieldCode   == "%<\\AcVar Area>%");
    REQUIRE(dst.m_formatString == "%lf");
    REQUIRE(dst.m_evaluationOptionFlags == 3);
    REQUIRE(dst.m_filingOptionFlags     == 1);
    REQUIRE(dst.m_value.m_dataType == 2);
    REQUIRE(dst.m_value.m_value.type() == DRW_Variant::DOUBLE);
    REQUIRE(dst.m_value.m_value.d_val() == Approx(42.5));
    REQUIRE(dst.m_valueString == "42.5");
    REQUIRE(dst.m_valueStringLength == 4);
    REQUIRE(dst.m_childHandles.size()  == 2u);
    REQUIRE(dst.m_childHandles[0]  == 0x401u);
    REQUIRE(dst.m_childHandles[1]  == 0x402u);
    REQUIRE(dst.m_objectHandles.size() == 1u);
    REQUIRE(dst.m_objectHandles[0] == 0x501u);

    auto truncated = bytes;
    REQUIRE(!truncated.empty());
    truncated.pop_back();
    dwgBuffer truncatedReader(truncated.data(), truncated.size());
    dst.m_evaluatorId = "stale-evaluator";
    dst.m_childHandles = {0x999u};
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(dst, ver,
                                                  &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(dst.m_evaluatorId.empty());
    CHECK(dst.m_childHandles.empty());
}

TEST_CASE("DRW_Field validates the AC1024 body boundary",
          "[dwg-read][object-encode][field][safety]") {
    DRW_Field source;
    source.handle = 0xA60u;
    source.parentHandle = 0x44u;
    source.m_evaluatorId = "AcVariable";
    source.m_fieldCode = "%<\\AcVar Area>%";
    source.m_evaluationOptionFlags = 1;
    source.m_value.m_dataType = 2;
    source.m_value.m_value.addDouble(140, 42.5);
    source.m_valueString = "42.5";
    source.m_valueStringLength = 4;
    source.m_childHandles = {0x401u};
    source.m_objectHandles = {0x501u};
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    std::uint32_t handleBits = 0;
    auto bytes = emitAc1024FieldObject(source, handleBits);
    REQUIRE(!bytes.empty());
    REQUIRE(handleBits != 0);

    DRW_TextCodec decoder;
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(const_cast<std::uint8_t *>(bytes.data()), bytes.size(),
                     &decoder);
    DRW_Field parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1024, &reader, handleBits));
    CHECK(parsed.parentHandle == source.parentHandle);
    CHECK(parsed.m_evaluatorId == source.m_evaluatorId);
    CHECK(parsed.m_fieldCode == source.m_fieldCode);
    CHECK(parsed.m_childHandles == source.m_childHandles);
    CHECK(parsed.m_objectHandles == source.m_objectHandles);
    CHECK(parsed.hasCompleteDwgPayload());

    REQUIRE(handleBits <= std::numeric_limits<std::uint32_t>::max() - 8u);
    dwgBuffer malformed(bytes.data(), bytes.size(), &decoder);
    DRW_Field rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, DRW::AC1024, &malformed, handleBits + 8u));
    CHECK_FALSE(malformed.isGood());
    CHECK(rejected.m_childHandles.empty());
    CHECK(rejected.m_objectHandles.empty());
    CHECK_FALSE(rejected.hasCompleteDwgPayload());
}

TEST_CASE("DRW_Field marks unsupported bounded payloads incomplete",
          "[dwg-read][object-encode][field][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0, /*handle=*/0xA62u,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putVariableText(version, "AcVariable");
    encoded.putVariableText(version, "%<\\AcVar Area>%");
    encoded.putBitLong(0); // child fields
    encoded.putBitLong(0); // field objects
    encoded.putVariableText(version, "%lf");
    for (int i = 0; i < 5; ++i)
        encoded.putBitLong(0);
    encoded.putVariableText(version, "");
    encoded.putBitLong(128); // unsupported FIELD value-buffer type
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x44u, {},
                           /*xDictFlag=*/1);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Field parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK(parsed.parentHandle == 0x44u);
    CHECK(parsed.m_evaluatorId == "AcVariable");
    CHECK_FALSE(parsed.hasCompleteDwgPayload());
}

TEST_CASE("DRW_Field rejects oversized counted payloads",
          "[dwg-read][object-encode][field][safety]") {
    const DRW::Version version = DRW::AC1018;
    const auto checkRejectedField = [version](const dwgBufferW& encoded) {
        auto bytes = snapshot(encoded);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_Field parsed;
        parsed.m_evaluatorId = "stale";
        parsed.m_childHandles = {0x99u};
        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.m_evaluatorId.empty());
        CHECK(parsed.m_childHandles.empty());
        CHECK(parsed.m_childValues.empty());
        CHECK_FALSE(parsed.hasCompleteDwgPayload());
    };

    SECTION("child handle count") {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, version, /*oType=*/0, /*handle=*/0xA63u,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        encoded.putVariableText(version, "AcVariable");
        encoded.putVariableText(version, "%<\\AcVar Area>%");
        encoded.putBitLong(
            static_cast<std::int32_t>(DRW_Field::kMaxItems + 1u));
        encoded.putBitLong(0);
        emitCommonHandlePrefix(encoded, /*parentHandle=*/0x44u, {},
                               /*xDictFlag=*/1);
        checkRejectedField(encoded);
    }

    SECTION("keyed child value count") {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, version, /*oType=*/0, /*handle=*/0xA64u,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        encoded.putVariableText(version, "AcVariable");
        encoded.putVariableText(version, "%<\\AcVar Area>%");
        encoded.putBitLong(0); // child fields
        encoded.putBitLong(0); // field objects
        encoded.putVariableText(version, "%lf");
        for (int index = 0; index < 5; ++index)
            encoded.putBitLong(0);
        encoded.putVariableText(version, "");
        encoded.putBitLong(0); // CadValue type
        encoded.putBitLong(0); // CadValue value
        encoded.putVariableText(version, "");
        encoded.putBitLong(0); // value string length
        encoded.putBitLong(
            static_cast<std::int32_t>(DRW_Field::kMaxItems + 1u));
        emitCommonHandlePrefix(encoded, /*parentHandle=*/0x44u, {},
                               /*xDictFlag=*/1);
        checkRejectedField(encoded);
    }
}

TEST_CASE("DRW_Field follows AC1024 value stream ordering",
          "[dwg-read][dwg-write][object-encode][field]") {
    DRW_Field source;
    source.handle = 0xA61u;
    source.parentHandle = 0x44u;
    source.m_evaluatorId = "AcVariable";
    source.m_fieldCode = "%<\\AcVar Text>%";
    source.m_value.m_formatFlags = 0;
    source.m_value.m_dataType = 4;
    source.m_value.m_value.addString(1, "value from string stream");
    source.m_value.m_unitType = 12;
    source.m_value.m_formatString = "format";
    source.m_value.m_valueString = "must not be emitted";
    source.m_valueString = "formatted field value";
    source.m_valueStringLength = 21;
    source.m_childValues.push_back({
        "object-id",
        DRW_CadValue(),
    });
    source.m_childValues.back().m_value.m_formatFlags = 0;
    source.m_childValues.back().m_value.m_dataType = 64;
    source.m_childValues.back().m_value.m_handle = 0x77u;
    source.m_childValues.back().m_value.m_unitType = 12;
    source.m_childHandles = {0x401u};
    source.m_objectHandles = {0x501u};
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    std::uint32_t handleBits = 0;
    const auto bytes = emitAc1024FieldObject(source, handleBits);
    REQUIRE(!bytes.empty());

    DRW_TextCodec decoder;
    decoder.setCodePage("UTF-16", false);
    dwgBuffer reader(const_cast<std::uint8_t *>(bytes.data()), bytes.size(),
                     &decoder);
    DRW_Field parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1024, &reader, handleBits));

    REQUIRE(parsed.m_value.m_dataType == 4);
    REQUIRE(parsed.m_value.m_value.type() == DRW_Variant::STRING);
    CHECK(parsed.m_value.m_value.c_str() == std::string("value from string stream"));
    CHECK(parsed.m_value.m_unitType == 12);
    CHECK(parsed.m_value.m_formatString == "format");
    CHECK(parsed.m_value.m_valueString.empty());
    REQUIRE(parsed.m_childValues.size() == 1u);
    CHECK(parsed.m_childValues.front().m_value.m_dataType == 64);
    CHECK(parsed.m_childValues.front().m_value.m_handle == 0x77u);
    CHECK(parsed.m_childHandles == source.m_childHandles);
    CHECK(parsed.m_objectHandles == source.m_objectHandles);
}

// FIELDLIST encoder round-trip (ODA §20.4.67).  Body: numFields BL + unknown
// bit B.  Handle stream: common prefix + N field handles.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_FieldList::encodeDwg round-trips field handles",
          "[dwg-write][object-encode][fieldlist]") {
    DRW_FieldList src;
    src.handle       = 0x1300;
    src.parentHandle = 0x44;
    src.m_unknown    = 0;
    src.m_fieldHandles = {0x1200u, DRW::NoHandle, 0x1200u, 0x1202u};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeFieldList(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_FieldList dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<std::uint32_t>(dst.parentHandle) == 0x44u);
    REQUIRE(dst.m_unknown == 0);
    REQUIRE(dst.m_fieldHandles.size() == 4u);
    REQUIRE(dst.m_fieldHandles[0] == 0x1200u);
    REQUIRE(dst.m_fieldHandles[1] == DRW::NoHandle);
    REQUIRE(dst.m_fieldHandles[2] == 0x1200u);
    REQUIRE(dst.m_fieldHandles[3] == 0x1202u);
}

TEST_CASE("DRW_FieldList::encodeDwg rejects invalid versions and common state",
          "[dwg-write][object-encode][fieldlist][safety]") {
    DRW_FieldList source;
    source.handle = 0x1300u;
    source.parentHandle = 0x44u;
    source.m_fieldHandles = {0x1200u};
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW encoded;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeFieldList(
        source, DRW::AC1014, &encoded));
    CHECK(encoded.data().empty());

    source.m_unknown = 2;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeFieldList(
        source, DRW::AC1015, &encoded));
    CHECK(encoded.data().empty());

    source.m_unknown = 1;
    DrwObjectEncodeTestAccess::setNumReactors(source, 1);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeFieldList(
        source, DRW::AC1015, &encoded));
    CHECK(encoded.data().empty());
}

TEST_CASE("DRW_FieldList validates the AC1024 body boundary",
          "[dwg-read][object-encode][fieldlist][safety]") {
    const DRW::Version version = DRW::AC1024;
    DRW_FieldList source;
    source.handle = 0xA50u;
    source.parentHandle = 0x1Fu;
    source.m_unknown = 1;
    source.m_fieldHandles = {0x201u, 0x202u};
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW data;
    data.putObjType(version, 0); // custom FIELDLIST class
    data.putHandle(hardPtr(source.handle));
    data.putBitShort(0);         // no EED
    data.putBitLong(0);          // no reactors
    data.putBit(1);              // no extension dictionary
    dwgBufferW handles;
    REQUIRE(DrwObjectEncodeTestAccess::encodeFieldList(
        source, version, &data, nullptr, &handles));
    data.alignToByte();
    handles.alignToByte();
    data.putBytes(handles.data().data(), handles.data().size());

    auto bytes = snapshot(data);
    const std::uint32_t handleBits =
        static_cast<std::uint32_t>(handles.data().size() * 8u);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_FieldList parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(
        parsed, version, &reader, handleBits));
    CHECK(parsed.m_unknown == source.m_unknown);
    CHECK(parsed.m_fieldHandles == source.m_fieldHandles);
    CHECK(parsed.parentHandle == source.parentHandle);

    // Move the declared data/handle boundary one byte into the body. The
    // common handle stream must be rejected instead of being read from a
    // position that no longer describes the object body.
    REQUIRE(handleBits <= std::numeric_limits<std::uint32_t>::max() - 8u);
    dwgBuffer malformed(bytes.data(), bytes.size());
    DRW_FieldList rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &malformed, handleBits + 8u));
    CHECK_FALSE(malformed.isGood());
    CHECK(rejected.m_fieldHandles.empty());
}

TEST_CASE("DRW_FieldList rejects an oversized member count",
          "[dwg-read][object-encode][fieldlist][safety]") {
    const DRW::Version version = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0, /*handle=*/0xA65u,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    encoded.putBitLong(
        static_cast<std::int32_t>(DRW_Field::kMaxItems + 1u));
    encoded.putBit(0);
    emitCommonHandlePrefix(encoded, /*parentHandle=*/0x44u, {},
                           /*xDictFlag=*/1);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_FieldList parsed;
    parsed.m_fieldHandles = {0x99u};
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_fieldHandles.empty());
    CHECK_FALSE(parsed.hasCompleteDwgEntries());
}

TEST_CASE("DRW_Field writers reject oversized counts and inconsistent payloads",
          "[dwg-write][object-encode][field][safety]") {
    DRW::Version version = DRW::AC1027;

    DRW_Field field;
    field.m_childHandles.resize(DRW_Field::kMaxItems + 1);
    dwgBufferW encodedField;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeField(
        field, version, &encodedField));
    CHECK(encodedField.data().empty());

    DRW_FieldList fieldList;
    fieldList.m_fieldHandles.resize(DRW_Field::kMaxItems + 1);
    dwgBufferW encodedList;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeFieldList(
        fieldList, version, &encodedList));
    CHECK(encodedList.data().empty());

    DRW_Field malformedValue;
    malformedValue.m_value.m_dataType = 32;
    malformedValue.m_value.m_dataSize = 24;
    malformedValue.m_value.m_value.addCoord(11, DRW_Coord{1.0, 2.0, 3.0});
    malformedValue.m_value.m_rawData.push_back(0xFF);
    dwgBufferW encodedValue;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeField(
        malformedValue, version, &encodedValue));
    CHECK(encodedValue.data().empty());

    DRW_Field nullChild;
    nullChild.m_value.m_dataType = 1;
    nullChild.m_value.m_value.addInt(91, 0);
    nullChild.m_childHandles = {DRW::NoHandle};
    dwgBufferW encodedNullChild;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeField(
        nullChild, version, &encodedNullChild));
    CHECK(encodedNullChild.data().empty());

    DRW_Field nullObject;
    nullObject.m_value.m_dataType = 1;
    nullObject.m_value.m_value.addInt(91, 0);
    nullObject.m_objectHandles = {DRW::NoHandle};
    dwgBufferW encodedNullObject;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeField(
        nullObject, version, &encodedNullObject));
    CHECK(encodedNullObject.data().empty());
}

TEST_CASE("DRW_Field rejects non-finite CAD value payloads",
          "[dwg-read][dwg-write][object-encode][field][safety]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();

    DRW_Field nonFiniteValue;
    nonFiniteValue.m_value.m_dataType = 2;
    nonFiniteValue.m_value.m_value.addDouble(140, nan);
    dwgBufferW encodedValue;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeField(
        nonFiniteValue, DRW::AC1018, &encodedValue));
    CHECK(encodedValue.data().empty());

    DRW_Field nonFiniteChild;
    nonFiniteChild.m_value.m_dataType = 2;
    nonFiniteChild.m_value.m_value.addDouble(140, 1.0);
    nonFiniteChild.m_childValues.push_back({});
    nonFiniteChild.m_childValues.back().m_value.m_dataType = 2;
    nonFiniteChild.m_childValues.back().m_value.m_value.addDouble(140, infinity);
    dwgBufferW encodedChild;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeField(
        nonFiniteChild, DRW::AC1018, &encodedChild));
    CHECK(encodedChild.data().empty());

    DRW_Field nonFinitePoint;
    nonFinitePoint.m_value.m_dataType = 32;
    nonFinitePoint.m_value.m_dataSize = 24;
    nonFinitePoint.m_value.m_value.addCoord(11, DRW_Coord{1.0, 2.0, infinity});
    dwgBufferW encodedPoint;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeField(
        nonFinitePoint, DRW::AC1027, &encodedPoint));
    CHECK(encodedPoint.data().empty());

    dwgBufferW malformed;
    emitObjectPreamble(malformed, DRW::AC1018, /*oType=*/0,
                       /*handle=*/0xA70u, /*numReactors=*/0,
                       /*xDictFlag=*/1);
    malformed.putVariableText(DRW::AC1018, "AcVariable");
    malformed.putVariableText(DRW::AC1018, "field");
    malformed.putBitLong(0); // no child handles
    malformed.putBitLong(0); // no object handles
    malformed.putVariableText(DRW::AC1018, "");
    for (int i = 0; i < 5; ++i)
        malformed.putBitLong(0);
    malformed.putVariableText(DRW::AC1018, "");
    malformed.putBitLong(2); // double CAD value
    malformed.putBitDouble(nan);

    auto bytes = snapshot(malformed);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_Field rejected;
    rejected.m_evaluatorId = "stale-evaluator";
    rejected.m_childHandles = {0x999u};
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.m_evaluatorId.empty());
    CHECK(rejected.m_childHandles.empty());
}

TEST_CASE("DRW_FieldList captures common handle references",
          "[dwg-read][object-encode][fieldlist]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0xA40,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(2);
    encoded.putBit(1);
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(hardPtr(0xA1));
    encoded.putHandle(hardPtr(0xB1));
    encoded.putHandle(hardPtr(0xC1));
    encoded.putHandle(hardPtr(0xC2));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_FieldList parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x1Fu);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles[0] == 0xA1u);
    CHECK(parsed.xDictHandle == 0xB1u);
    REQUIRE(parsed.m_fieldHandles.size() == 2u);
    CHECK(parsed.m_fieldHandles[0] == 0xC1u);
    CHECK(parsed.m_fieldHandles[1] == 0xC2u);
}

TEST_CASE("DRW_FieldList rejects a truncated common handle stream",
          "[dwg-read][object-encode][fieldlist]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, ver, /*oType=*/0, /*handle=*/0xA41,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    encoded.putBitLong(0);
    encoded.putBit(0);
    encoded.putHandle(hardPtr(0x1F));
    encoded.putHandle(hardPtr(0xA1));

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_FieldList parsed;
    parsed.parentHandle = 0x99u;
    parsed.m_fieldHandles = {0x98u};
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.parentHandle == 0u);
    CHECK(parsed.m_fieldHandles.empty());
}

TEST_CASE("DRW_FieldList rejects an impossible field count before allocation",
          "[dwg-read][object-encode][fieldlist][safety]") {
    dwgBufferW encoded;
    emitObjectPreamble(encoded, DRW::AC1018, /*oType=*/0,
                       /*handle=*/0xA42u, /*numReactors=*/0,
                       /*xDictFlag=*/1);
    encoded.putBitLong(static_cast<std::int32_t>(DRW_Field::kMaxItems));
    encoded.putBit(0);
    encoded.putHandle(hardPtr(0)); // common parent handle

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_FieldList parsed;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_fieldHandles.empty());
}

// Phase 3A.0 — DRW_Dimstyle::syncStructToVars populates every $DIM key the
// LibreCAD createDimStyle consumer reads, from the parsed struct fields.
TEST_CASE("DRW_Dimstyle::syncStructToVars populates vars from struct fields",
          "[dwg-object-encode][dimstyle][data-loss]") {
    DRW_Dimstyle d;
    d.dimscale = 2.0;
    d.dimasz = 0.25;
    d.dimclrd = 1;
    d.dimlwd = 13;
    d.dimtxsty = "MyText";
    d.dimtad = 2;
    d.dimzin = 8;
    d.dimdec = 3;
    d.dimlunit = 4;

    DrwObjectEncodeTestAccess::syncDimstyle(d);

    REQUIRE(d.get("$DIMSCALE") != nullptr);
    CHECK(d.get("$DIMSCALE")->d_val() == Approx(2.0));
    CHECK(d.get("$DIMASZ")->d_val() == Approx(0.25));
    CHECK(d.get("$DIMCLRD")->i_val() == 1);
    CHECK(d.get("$DIMLWD")->i_val() == 13);
    REQUIRE(d.get("$DIMTXSTY") != nullptr);
    CHECK(std::string(d.get("$DIMTXSTY")->c_str()) == "MyText");
    CHECK(d.get("$DIMTAD")->i_val() == 2);
    CHECK(d.get("$DIMZIN")->i_val() == 8);
    CHECK(d.get("$DIMDEC")->i_val() == 3);
    CHECK(d.get("$DIMLUNIT")->i_val() == 4);
}

// Phase 3A.0 — sync is idempotent: the if(!get(key)) guard never clobbers a
// value already present in the vars map (DWG override / DXF-105 path).
TEST_CASE("DRW_Dimstyle::syncStructToVars is idempotent and never clobbers",
          "[dwg-object-encode][dimstyle][data-loss]") {
    DRW_Dimstyle d;
    d.dimscale = 2.0;
    // Pre-populate an override value; sync must not overwrite it.
    d.add("$DIMSCALE", 40, 9.0);

    DrwObjectEncodeTestAccess::syncDimstyle(d);
    REQUIRE(d.get("$DIMSCALE") != nullptr);
    CHECK(d.get("$DIMSCALE")->d_val() == Approx(9.0));

    // A second sync leaves the populated keys unchanged.
    const DRW_Variant* before = d.get("$DIMASZ");
    DrwObjectEncodeTestAccess::syncDimstyle(d);
    CHECK(d.get("$DIMASZ") == before);
    CHECK(d.get("$DIMSCALE")->d_val() == Approx(9.0));
}

// Phase 3A.0 — Rule-of-Five deep copy: copying a vars-populated DRW_Dimstyle
// and destroying both copies is clean (no double-free); mutating the copy
// does not affect the original.
TEST_CASE("DRW_Dimstyle deep-copy is independent and leak/double-free clean",
          "[dwg-object-encode][dimstyle][data-loss]") {
    DRW_Dimstyle original;
    original.dimscale = 3.5;
    original.dimtxt = 0.25;
    DrwObjectEncodeTestAccess::syncDimstyle(original);
    REQUIRE(original.get("$DIMSCALE")->d_val() == Approx(3.5));

    {
        DRW_Dimstyle copy(original);  // copy ctor — deep copy vars.
        REQUIRE(copy.get("$DIMSCALE") != nullptr);
        CHECK(copy.get("$DIMSCALE")->d_val() == Approx(3.5));
        // Pointers are distinct (deep copy, not shared).
        CHECK(copy.get("$DIMSCALE") != original.get("$DIMSCALE"));
        // Mutating the copy does not affect the original.
        copy.add("$DIMSCALE", 40, 7.0);
        CHECK(copy.get("$DIMSCALE")->d_val() == Approx(7.0));
        CHECK(original.get("$DIMSCALE")->d_val() == Approx(3.5));

        DRW_Dimstyle assigned;
        assigned = original;  // copy assignment.
        CHECK(assigned.get("$DIMSCALE")->d_val() == Approx(3.5));
        CHECK(assigned.get("$DIMSCALE") != original.get("$DIMSCALE"));

        DRW_Dimstyle moved(std::move(copy));  // move ctor steals vars.
        CHECK(moved.get("$DIMSCALE")->d_val() == Approx(7.0));
    }  // copy/assigned/moved destroyed here — must not double-free.

    // Original still intact after the inner scope destroyed its copies.
    CHECK(original.get("$DIMSCALE")->d_val() == Approx(3.5));
}

// Phase 3A.1 — new R2007/R2010 numeric/string members + handle refs
// default-construct/reset() to their documented defaults.
TEST_CASE("DRW_Dimstyle default-constructs R2007/R2010 members with defaults",
          "[dwg-object-encode][dimstyle][data-loss]") {
    DRW_Dimstyle d;
    CHECK(d.dimjogang == Approx(0.0));
    CHECK(d.dimtfill == 0);
    CHECK(d.dimtfillclr == 0);
    CHECK(d.dimarcsym == 0);
    CHECK(d.dimtxtdirection == 0);
    CHECK(d.dimaltmzf == Approx(1.0));
    CHECK(d.dimmzf == Approx(1.0));
    CHECK(d.dimaltmzs.empty());
    CHECK(d.dimmzs.empty());
    CHECK(d.dimtxstyH.ref == 0u);
    CHECK(d.dimldrblkH.ref == 0u);
    CHECK(d.dimblkH.ref == 0u);
    CHECK(d.dimblk1H.ref == 0u);
    CHECK(d.dimblk2H.ref == 0u);
    CHECK(d.dimltypeH.ref == 0u);
    CHECK(d.dimltex1H.ref == 0u);
    CHECK(d.dimltex2H.ref == 0u);

    // reset() restores the same defaults after mutation.
    d.dimjogang = 1.25;
    d.dimaltmzf = 5.0;
    d.dimtxstyH.ref = 0x99u;
    d.reset();
    CHECK(d.dimjogang == Approx(0.0));
    CHECK(d.dimaltmzf == Approx(1.0));
    CHECK(d.dimtxstyH.ref == 0u);
}

// Phase 4 (P4-04) — DRW_UCS::parseDwg reads origin/axes/elevation/orthoType +
// base/named handles. Synthetic AC1015 (R2000) UCS record per dwg.spec UCS
// binary field order; parsed back via the friend accessor.
TEST_CASE("DRW_UCS::parseDwg reads geometry, elevation, orthoType, handles",
          "[dwg-object-encode][ucs][data-loss]") {
    const DRW::Version ver = DRW::AC1015;
    constexpr std::uint16_t ucsType = 0x3F;  // UCS table record.

    dwgBufferW body;
    emitObjectPreamble(body, ver, ucsType, /*handle=*/0x30u,
                       /*numReactors=*/1, /*xDictFlag=*/0);
    body.putVariableText(ver, std::string("MyUCS"));
    body.putBit(0);          // flags bit 7 (64)
    body.putBitShort(0);     // xrefindex (version < AC1021)
    body.putBit(0);          // flags bit 5 (16)
    body.put3BitDouble(DRW_Coord(10.0, 20.0, 0.0));   // ucsorg
    body.put3BitDouble(DRW_Coord(0.0, 1.0, 0.0));     // ucsxdir
    body.put3BitDouble(DRW_Coord(-1.0, 0.0, 0.0));    // ucsydir
    body.putBitDouble(5.0);  // ucs_elevation (BD, FIRST in binary order)
    body.putBitShort(1);     // UCSORTHOVIEW (BS)
    body.putBitShort(0);     // num_orthopts = 0
    // Handle stream: relative control, raw reactor/xdic/xref, then raw
    // base_ucs and named_ucs. The coded raw refs catch accidental use of the
    // object-relative decoder for table-entry handles.
    body.putHandle(hardPtr(0x3Eu));
    body.putHandle(handleWithCode(0xAu, 0x3Fu));
    body.putHandle(handleWithCode(0xCu, 0x55u));
    body.putHandle(hardPtr(0x102u));  // xref block
    body.putHandle(hardPtr(0x100u));  // base_ucs
    body.putHandle(hardPtr(0x101u));  // named_ucs

    std::vector<std::uint8_t> bytes = snapshot(body);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_UCS dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    CHECK(dst.name == "MyUCS");
    CHECK(dst.origin.x == Approx(10.0));
    CHECK(dst.origin.y == Approx(20.0));
    CHECK(dst.xAxisDirection.x == Approx(0.0));
    CHECK(dst.xAxisDirection.y == Approx(1.0));
    CHECK(dst.yAxisDirection.x == Approx(-1.0));
    CHECK(dst.elevation == Approx(5.0));
    CHECK(dst.orthoType == 1);
    REQUIRE(dst.reactorHandles.size() == 1u);
    CHECK(dst.reactorHandles.front() == 0x3Fu);
    CHECK(dst.xDictHandle == 0x55u);
    CHECK(dst.xrefBlockHandle.ref == 0x102u);
    CHECK(dst.baseUcsHandle.ref == 0x100u);
    CHECK(dst.namedUcsHandle.ref == 0x101u);
}

TEST_CASE("DRW_UCS preserves orthographic options and rolls back truncation",
          "[dwg-object-encode][ucs][safety]") {
    const DRW::Version ver = DRW::AC1018;

    auto makeRecord = [ver](bool truncate) {
        dwgBufferW body;
        emitObjectPreamble(body, ver, /*oType=*/0x3Fu, /*handle=*/0x30u,
                           /*numReactors=*/0, /*xDictFlag=*/1);
        body.putVariableText(ver, "Options");
        body.putBit(0);
        body.putBitShort(0);
        body.putBit(0);
        body.put3BitDouble(DRW_Coord(1.0, 2.0, 3.0));
        body.put3BitDouble(DRW_Coord(1.0, 0.0, 0.0));
        body.put3BitDouble(DRW_Coord(0.0, 1.0, 0.0));
        body.putBitDouble(4.0);
        body.putBitShort(2);
        body.putBitShort(2);
        body.putBitShort(1);
        body.put3BitDouble(DRW_Coord(10.0, 11.0, 12.0));
        body.putBitShort(3);
        body.put3BitDouble(DRW_Coord(20.0, 21.0, 22.0));
        emitCommonHandlePrefix(body, /*parentHandle=*/0x40u, {}, 1);
        body.putHandle(nullHandle());
        body.putHandle(hardPtr(0x100u));
        body.putHandle(hardPtr(0x101u));
        auto bytes = snapshot(body);
        if (truncate)
            bytes.pop_back();
        return bytes;
    };

    auto valid = makeRecord(false);
    dwgBuffer reader(valid.data(), valid.size());
    DRW_UCS parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    CHECK(parsed.orthographicOptions.size() == 2u);
    CHECK(parsed.orthographicOptions[0].type == 1);
    CHECK(parsed.orthographicOptions[0].point.x == Approx(10.0));
    CHECK(parsed.orthographicOptions[1].type == 3);
    CHECK(parsed.orthographicOptions[1].point.z == Approx(22.0));
    CHECK(parsed.orthoOrigin.x == Approx(10.0));
    CHECK(parsed.orthoType == 2);

    auto truncated = makeRecord(true);
    dwgBuffer truncatedReader(truncated.data(), truncated.size());
    parsed.parentHandle = 0x99;
    parsed.orthographicOptions.push_back({9, DRW_Coord(9.0, 9.0, 9.0)});
    parsed.baseUcsHandle.ref = 0x98u;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, ver,
                                                   &truncatedReader));
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.orthographicOptions.empty());
    CHECK(parsed.baseUcsHandle.ref == 0u);
}

TEST_CASE("DRW_UCS::encodeDwg round-trips options and typed handles",
          "[dwg-object-encode][ucs][data-loss]") {
    const DRW::Version ver = DRW::AC1018;
    DRW_UCS source;
    source.name = "EncodedUCS";
    source.origin = DRW_Coord(1.0, 2.0, 3.0);
    source.xAxisDirection = DRW_Coord(1.0, 0.0, 0.0);
    source.yAxisDirection = DRW_Coord(0.0, 1.0, 0.0);
    source.elevation = 4.0;
    source.orthoType = 2;
    source.orthographicOptions = {
        {1, DRW_Coord(10.0, 11.0, 12.0)},
        {3, DRW_Coord(20.0, 21.0, 22.0)},
    };
    source.xrefBlockHandle = hardPtr(0x102u);
    source.baseUcsHandle = hardPtr(0x100u);
    source.namedUcsHandle = hardPtr(0x101u);
    source.reactorHandles = {0x103u};
    DrwObjectEncodeTestAccess::setNumReactors(source, 1);
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW body;
    emitObjectPreamble(body, ver, /*oType=*/0x3Fu, /*handle=*/0x30u,
                       /*numReactors=*/1, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeUcs(source, ver, &body));

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_UCS parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &reader));
    REQUIRE(parsed.orthographicOptions.size() == 2u);
    CHECK(parsed.orthographicOptions[0].type == 1);
    CHECK(parsed.orthographicOptions[1].point.y == Approx(21.0));
    CHECK(parsed.xrefBlockHandle.ref == 0x102u);
    CHECK(parsed.baseUcsHandle.ref == 0x100u);
    CHECK(parsed.namedUcsHandle.ref == 0x101u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x103u);
}

TEST_CASE("DRW_UCS encodes every supplied reactor handle",
          "[dwg-write][object-encode][ucs][safety]") {
    DRW_UCS source;
    source.reactorHandles = {0x103u, 0x104u};
    DrwObjectEncodeTestAccess::setNumReactors(source, 0);

    dwgBufferW body;
    emitObjectPreamble(body, DRW::AC1018, /*oType=*/0x3F,
                       /*handle=*/0x30u, /*numReactors=*/2,
                       /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeUcs(
        source, DRW::AC1018, &body));

    auto bytes = snapshot(body);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_UCS parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, DRW::AC1018, &reader));
    REQUIRE(parsed.reactorHandles.size() == 2u);
    CHECK(parsed.reactorHandles[0] == 0x103u);
    CHECK(parsed.reactorHandles[1] == 0x104u);
}

TEST_CASE("DRW_UCS rejects a body ending before xref flags",
          "[dwg-read][object-encode][ucs][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW encoded;
    emitObjectPreamble(encoded, version, /*oType=*/0x3F,
                       /*handle=*/0x750);
    encoded.putVariableText(version, "ShortUCS");
    const std::uint32_t nameEndBit = encoded.bitCount();
    encoded.putBit(0);             // xref referenced
    encoded.putBitShort(0);        // xref index
    encoded.putBit(0);             // xref dependent
    encoded.patchRawLong32AtBit(objectSizeBitOffset(encoded), nameEndBit);

    auto bytes = snapshot(encoded);
    dwgBuffer reader(bytes.data(), bytes.size());
    DRW_UCS rejected;
    rejected.name = "stale-name";
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &reader));
    CHECK_FALSE(reader.isGood());
    CHECK(rejected.name.empty());
}

TEST_CASE("DRW_UCS rejects lossy orthographic field narrowing",
          "[dwg-write][object-encode][ucs][safety]") {
    DRW_UCS source;
    source.orthoType = -1;
    dwgBufferW typeBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeUcs(
        source, DRW::AC1015, &typeBody));
    CHECK(typeBody.data().empty());

    source.orthoType = 0;
    source.orthographicOptions = {
        {-1, DRW_Coord(1.0, 2.0, 3.0)},
    };
    dwgBufferW optionBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeUcs(
        source, DRW::AC1015, &optionBody));
    CHECK(optionBody.data().empty());

    source.orthographicOptions.clear();
    source.origin.x = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW coordinateBody;
    coordinateBody.putRawChar8(0xA5);
    const auto coordinateBefore = snapshot(coordinateBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeUcs(
        source, DRW::AC1012, &coordinateBody));
    CHECK(snapshot(coordinateBody) == coordinateBefore);

    source.origin.x = 0.0;
    source.orthographicOptions = {
        {1, DRW_Coord(1.0, std::numeric_limits<double>::infinity(), 3.0)},
    };
    dwgBufferW optionPointBody;
    optionPointBody.putRawChar8(0x5A);
    const auto optionPointBefore = snapshot(optionPointBody);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeUcs(
        source, DRW::AC1015, &optionPointBody));
    CHECK(snapshot(optionPointBody) == optionPointBefore);
}

// Phase 4 (P4-01) — DRW_PlotSettings now carries the full plot field set and
// default-constructs with the documented scale defaults.
TEST_CASE("DRW_PlotSettings default-constructs full plot field set",
          "[dwg-object-encode][plotsettings][data-loss]") {
    DRW_PlotSettings ps;
    // Scale defaults are 1.0; everything else zero/empty.
    CHECK(ps.realWorldUnits == Approx(1.0));
    CHECK(ps.drawingUnits == Approx(1.0));
    CHECK(ps.scaleFactor == Approx(1.0));
    CHECK(ps.marginLeft == Approx(0.0));
    CHECK(ps.marginTop == Approx(0.0));
    CHECK(ps.paperWidth == Approx(0.0));
    CHECK(ps.paperHeight == Approx(0.0));
    CHECK(ps.plotLayoutFlags == 0);
    CHECK(ps.paperUnits == 0);
    CHECK(ps.plotType == 0);
    CHECK(ps.scaleType == 0);
    CHECK(ps.shadePlotMode == 0);
    CHECK(ps.plotViewHandle.ref == 0u);
    CHECK(ps.shadePlotHandle.ref == 0u);
    CHECK(ps.pageSetupName.empty());
    CHECK(ps.paperSize.empty());
    CHECK(ps.currentStyleSheet.empty());
}

// Phase 4 (P4-02) — DRW_PlotSettings::parseDwg reads the full plot prefix
// (margins/paper/window/scale). Synthetic AC1015 body per the LAYOUT plot
// prefix wire layout, parsed back via the friend accessor.
TEST_CASE("DRW_PlotSettings::parseDwg reads margins/paper/window/scale",
          "[dwg-object-encode][plotsettings][data-loss]") {
    const DRW::Version ver = DRW::AC1015;
    constexpr std::uint16_t psType = 0x1F4;  // custom-class number (>=500).

    dwgBufferW body;
    emitObjectPreamble(body, ver, psType, /*handle=*/0x40u,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    body.putVariableText(ver, std::string("MyPrinter"));   // pageSetupName (1)
    body.putVariableText(ver, std::string("PaperCfg"));    // printerConfig (2)
    body.putBitShort(688);          // plotLayoutFlags (70)
    body.putBitDouble(7.5);         // marginLeft (40)
    body.putBitDouble(7.6);         // marginBottom (41)
    body.putBitDouble(7.7);         // marginRight (42)
    body.putBitDouble(7.8);         // marginTop (43)
    body.putBitDouble(420.0);       // paperWidth (44)
    body.putBitDouble(297.0);       // paperHeight (45)
    body.putVariableText(ver, std::string("ISO_A3"));      // paperSize (4)
    body.putBitDouble(1.0);         // plotOriginX (46)
    body.putBitDouble(2.0);         // plotOriginY (47)
    body.putBitShort(1);            // paperUnits (72)
    body.putBitShort(2);            // plotRotation (73)
    body.putBitShort(3);            // plotType (74)
    body.putBitDouble(10.0);        // windowMinX (48)
    body.putBitDouble(11.0);        // windowMinY (49)
    body.putBitDouble(110.0);       // windowMaxX (140)
    body.putBitDouble(111.0);       // windowMaxY (141)
    body.putVariableText(ver, std::string("PlotView1"));   // plotViewName (<AC1018)
    body.putBitDouble(25.4);        // realWorldUnits (142)
    body.putBitDouble(1.0);         // drawingUnits (143)
    body.putVariableText(ver, std::string("acad.ctb"));    // currentStyleSheet (7)
    body.putBitShort(4);            // scaleType (75)
    body.putBitDouble(0.5);         // scaleFactor (147)
    body.putBitDouble(3.0);         // paperImageOriginX (148)
    body.putBitDouble(4.0);         // paperImageOriginY (149)
    // AC1015 < AC1018 — no shadeplot fields.
    emitCommonHandlePrefix(body, /*parentHandle=*/0x3Du, {}, /*xDictFlag=*/0);

    std::vector<std::uint8_t> bytes = snapshot(body);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_PlotSettings dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    CHECK(dst.pageSetupName == "MyPrinter");
    CHECK(dst.printerConfig == "PaperCfg");
    CHECK(dst.plotLayoutFlags == 688);
    CHECK(dst.marginLeft == Approx(7.5));
    CHECK(dst.marginTop == Approx(7.8));
    CHECK(dst.paperWidth == Approx(420.0));
    CHECK(dst.paperHeight == Approx(297.0));
    CHECK(dst.paperSize == "ISO_A3");
    CHECK(dst.paperUnits == 1);
    CHECK(dst.plotType == 3);
    CHECK(dst.windowMinX == Approx(10.0));
    CHECK(dst.windowMaxY == Approx(111.0));
    CHECK(dst.plotViewName == "PlotView1");
    CHECK(dst.realWorldUnits == Approx(25.4));
    CHECK(dst.currentStyleSheet == "acad.ctb");
    CHECK(dst.scaleType == 4);
    CHECK(dst.scaleFactor == Approx(0.5));
    CHECK(static_cast<std::uint32_t>(dst.parentHandle) == 0x3Du);
}

TEST_CASE("DRW_PlotSettings::encodeDwg round-trips inline plot data",
          "[dwg-write][object-encode][plotsettings]") {
    for (const DRW::Version version : {DRW::AC1015, DRW::AC1018}) {
        DRW_PlotSettings source;
        source.handle = 0x40;
        source.parentHandle = 0x3D;
        source.reactorHandles = {0x41};
        source.xDictHandle = 0x42;
        source.plotViewHandle = hardPtr(0x43);
        source.pageSetupName = "PageSetup";
        source.printerConfig = "Printer";
        source.plotLayoutFlags = 688;
        source.marginLeft = 7.5;
        source.marginBottom = 7.6;
        source.marginRight = 7.7;
        source.marginTop = 7.8;
        source.paperWidth = 420.0;
        source.paperHeight = 297.0;
        source.paperSize = "ISO_A3";
        source.plotOriginX = 1.0;
        source.plotOriginY = 2.0;
        source.paperUnits = 1;
        source.plotRotation = 2;
        source.plotType = 3;
        source.windowMinX = 10.0;
        source.windowMinY = 11.0;
        source.windowMaxX = 110.0;
        source.windowMaxY = 111.0;
        source.plotViewName = "PlotView1";
        source.realWorldUnits = 25.4;
        source.drawingUnits = 1.0;
        source.currentStyleSheet = "acad.ctb";
        source.scaleType = 4;
        source.scaleFactor = 0.5;
        source.paperImageOriginX = 3.0;
        source.paperImageOriginY = 4.0;
        source.shadePlotMode = 5;
        source.shadePlotResLevel = 6;
        source.shadePlotCustomDPI = 300;
        DrwObjectEncodeTestAccess::setNumReactors(source, 1);
        DrwObjectEncodeTestAccess::setXDictFlag(source,
                                                 version == DRW::AC1018 ? 1 : 0);

        dwgBufferW encoded;
        emitObjectPreamble(encoded, version, /*oType=*/0x1F4,
                           source.handle, source.reactorCount(),
                           source.extensionDictionaryFlag());
        REQUIRE(DrwObjectEncodeTestAccess::encodePlotSettings(
            source, version, &encoded));

        const auto bytes = snapshot(encoded);
        dwgBuffer reader(const_cast<std::uint8_t *>(bytes.data()), bytes.size());
        DRW_PlotSettings parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader));

        CHECK(parsed.handle == source.handle);
        CHECK(static_cast<std::uint32_t>(parsed.parentHandle)
              == static_cast<std::uint32_t>(source.parentHandle));
        REQUIRE(parsed.reactorHandles.size() == 1u);
        CHECK(parsed.reactorHandles.front() == 0x41u);
        CHECK(parsed.xDictHandle == (version == DRW::AC1018 ? 0x0u : 0x42u));
        CHECK(parsed.pageSetupName == source.pageSetupName);
        CHECK(parsed.printerConfig == source.printerConfig);
        CHECK(parsed.paperSize == source.paperSize);
        CHECK(parsed.plotViewName
              == (version == DRW::AC1015 ? source.plotViewName : ""));
        CHECK(parsed.marginTop == Approx(source.marginTop));
        CHECK(parsed.paperWidth == Approx(source.paperWidth));
        CHECK(parsed.windowMaxY == Approx(source.windowMaxY));
        CHECK(parsed.scaleFactor == Approx(source.scaleFactor));
        CHECK(parsed.plotViewHandle.ref
              == (version == DRW::AC1018 ? 0x43u : 0u));
        CHECK(parsed.shadePlotMode
              == (version == DRW::AC1018 ? source.shadePlotMode : 0));
    }
}

TEST_CASE("Plot settings writers reject values outside unsigned DWG BS",
          "[dwg-write][object-encode][plotsettings][layout][safety]") {
    DRW_PlotSettings plotSettings;
    plotSettings.plotLayoutFlags =
        std::numeric_limits<std::uint16_t>::max() + 1;
    dwgBufferW plotBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePlotSettings(
        plotSettings, DRW::AC1015, &plotBody));
    CHECK(plotBody.data().empty());

    plotSettings = DRW_PlotSettings();
    plotSettings.marginLeft = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW nonFinitePlot;
    nonFinitePlot.putRawChar8(0xA5);
    const auto nonFinitePlotBytes = snapshot(nonFinitePlot);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodePlotSettings(
        plotSettings, DRW::AC1015, &nonFinitePlot));
    CHECK(nonFinitePlot.data() == nonFinitePlotBytes);

    DRW_Layout layout;
    layout.layoutFlags = -1;
    dwgBufferW layoutBody;
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeLayout(
        layout, DRW::AC1018, &layoutBody));
    CHECK(layoutBody.data().empty());

    layout = DRW_Layout();
    layout.ucsOrigin.x = std::numeric_limits<double>::quiet_NaN();
    dwgBufferW invalidLayoutCoordinate;
    invalidLayoutCoordinate.putRawChar8(0x5A);
    const auto invalidLayoutBytes = snapshot(invalidLayoutCoordinate);
    CHECK_FALSE(DrwObjectEncodeTestAccess::encodeLayout(
        layout, DRW::AC1018, &invalidLayoutCoordinate));
    CHECK(invalidLayoutCoordinate.data() == invalidLayoutBytes);
}

TEST_CASE("DRW_PlotSettings rejects non-finite DWG values transactionally",
          "[dwg-read][object-encode][plotsettings][safety]") {
    const DRW::Version version = DRW::AC1015;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();

    const auto parseMalformed = [&](double marginLeft) {
        dwgBufferW encoded;
        emitObjectPreamble(encoded, version, /*oType=*/0x1F4,
                           /*handle=*/0x41);
        encoded.putVariableText(version, "PageSetup");
        encoded.putVariableText(version, "Printer");
        encoded.putBitShort(0); // plot layout flags
        encoded.putBitDouble(marginLeft);

        auto bytes = snapshot(encoded);
        dwgBuffer reader(bytes.data(), bytes.size());
        DRW_PlotSettings parsed;
        parsed.pageSetupName = "stale setup";
        parsed.marginLeft = 9.0;
        CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
            parsed, version, &reader));
        CHECK_FALSE(reader.isGood());
        CHECK(parsed.pageSetupName.empty());
        CHECK(parsed.marginLeft == Approx(0.0));
        CHECK(parsed.parentHandle == 0u);
    };

    parseMalformed(nan);
    parseMalformed(infinity);
}

TEST_CASE("DRW_PlotSettings rejects an AC1018 body overrun",
          "[dwg-read][object-encode][plotsettings][safety]") {
    const DRW::Version version = DRW::AC1018;
    DRW_PlotSettings source;
    source.handle = 0x40;
    source.parentHandle = 0x3D;
    DrwObjectEncodeTestAccess::setXDictFlag(source, 1);

    dwgBufferW data;
    emitObjectPreamble(data, version, /*oType=*/0x1F4, source.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    dwgBufferW handles;
    REQUIRE(DrwObjectEncodeTestAccess::encodePlotSettings(
        source, version, &data, nullptr, &handles));
    data.alignToByte();
    handles.alignToByte();
    const std::uint32_t bodyEnd = static_cast<std::uint32_t>(data.bitCount());
    data.patchRawLong32AtBit(objectSizeBitOffset(data), bodyEnd);
    data.putBytes(handles.data().data(), handles.data().size());

    const auto validBytes = snapshot(data);
    dwgBuffer validReader(const_cast<std::uint8_t *>(validBytes.data()),
                          validBytes.size());
    DRW_PlotSettings valid;
    REQUIRE(DrwObjectEncodeTestAccess::parse(valid, version, &validReader));

    dwgBufferW malformed;
    malformed.putBytes(validBytes.data(), validBytes.size());
    malformed.patchRawLong32AtBit(objectSizeBitOffset(malformed), bodyEnd - 1);
    const auto malformedBytes = snapshot(malformed);
    dwgBuffer malformedReader(
        const_cast<std::uint8_t *>(malformedBytes.data()),
        malformedBytes.size());
    DRW_PlotSettings rejected;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        rejected, version, &malformedReader));
    CHECK_FALSE(malformedReader.isGood());
    CHECK(rejected.parentHandle == 0);
    CHECK(rejected.plotViewHandle.ref == 0u);
}

TEST_CASE("DRW_PlotSettings AC1021 preserves deferred plot handles transactionally",
          "[dwg-read][object-encode][plotsettings][safety]") {
    const DRW::Version ver = DRW::AC1021;

    auto makeRecord = [ver](bool truncateHandle) {
        dwgBufferW body;
        // Use a zero OT so the AC1021 raw object-size field starts at bit 2.
        emitObjectPreamble(body, ver, /*oType=*/0, /*handle=*/0x40,
                           /*numReactors=*/1, /*xDictFlag=*/0);
        body.putBitShort(688);          // plot flags
        body.putBitDouble(7.5);         // left margin
        body.putBitDouble(7.6);         // bottom margin
        body.putBitDouble(7.7);         // right margin
        body.putBitDouble(7.8);         // top margin
        body.putBitDouble(420.0);        // paper width
        body.putBitDouble(297.0);        // paper height
        body.putBitDouble(1.0);          // plot origin x
        body.putBitDouble(2.0);          // plot origin y
        body.putBitShort(1);             // paper units
        body.putBitShort(2);             // rotation
        body.putBitShort(3);             // plot type
        body.putBitDouble(10.0);         // window min x
        body.putBitDouble(11.0);         // window min y
        body.putBitDouble(110.0);        // window max x
        body.putBitDouble(111.0);        // window max y
        body.putBitDouble(25.4);         // real-world units
        body.putBitDouble(1.0);           // drawing units
        body.putBitShort(4);              // standard scale type
        body.putBitDouble(0.5);           // scale factor
        body.putBitDouble(3.0);           // paper image origin x
        body.putBitDouble(4.0);           // paper image origin y
        body.putBitShort(5);              // shade plot mode
        body.putBitShort(6);              // shade plot resolution
        body.putBitShort(300);            // shade plot DPI

        dwgBufferW strings;
        // Empty TU values still exercise all four detached-string advances;
        // this direct fixture intentionally has no text codec attached.
        strings.putVariableText(ver, ""); // page setup
        strings.putVariableText(ver, ""); // printer config
        strings.putVariableText(ver, ""); // paper size
        strings.putVariableText(ver, ""); // current style sheet
        body.alignToByte();
        body.putBytes(strings.data().data(), strings.data().size());
        for (int i = 0; i < 7; ++i)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(
            strings.data().size() * 8u + 7u));
        body.putBit(1); // string stream present
        body.alignToByte();
        body.patchRawLong32AtBit(
            2, static_cast<std::uint32_t>(body.size() * 8u));

        dwgBufferW handles;
        handles.putHandle(hardPtr(0x50)); // parent
        handles.putHandle(handleWithCode(0xA, 0x51)); // reactor (raw)
        handles.putHandle(handleWithCode(0xC, 0x54)); // xdictionary (raw)
        handles.putHandle(hardPtr(0x52)); // plot view
        handles.putHandle(hardPtr(0x53)); // shade plot
        auto bytes = snapshot(body);
        const auto handleBytes = handles.data();
        bytes.insert(bytes.end(), handleBytes.begin(), handleBytes.end());
        if (truncateHandle)
            bytes.pop_back();
        return bytes;
    };

    auto validBytes = makeRecord(false);
    dwgBuffer validReader(validBytes.data(), validBytes.size());
    DRW_PlotSettings parsed;
    REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, ver, &validReader));
    CHECK(parsed.pageSetupName.empty());
    CHECK(parsed.printerConfig.empty());
    CHECK(parsed.paperSize.empty());
    CHECK(parsed.currentStyleSheet.empty());
    CHECK(parsed.plotLayoutFlags == 688);
    CHECK(parsed.marginLeft == Approx(7.5));
    CHECK(parsed.paperWidth == Approx(420.0));
    CHECK(parsed.shadePlotMode == 5);
    CHECK(parsed.shadePlotResLevel == 6);
    CHECK(parsed.shadePlotCustomDPI == 300);
    CHECK(static_cast<std::uint32_t>(parsed.parentHandle) == 0x50u);
    REQUIRE(parsed.reactorHandles.size() == 1u);
    CHECK(parsed.reactorHandles.front() == 0x51u);
    CHECK(parsed.xDictHandle == 0x54u);
    CHECK(parsed.plotViewHandle.ref == 0x52u);
    CHECK(parsed.shadePlotHandle.ref == 0x53u);

    auto truncatedBytes = makeRecord(true);
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    parsed.parentHandle = 0x99;
    parsed.reactorHandles.push_back(0x98);
    parsed.plotViewHandle.ref = 0x97;
    parsed.shadePlotHandle.ref = 0x96;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(
        parsed, ver, &truncatedReader));
    CHECK(parsed.parentHandle == 0);
    CHECK(parsed.reactorHandles.empty());
    CHECK(parsed.plotViewHandle.ref == 0u);
    CHECK(parsed.shadePlotHandle.ref == 0u);
}

TEST_CASE("DRW_VbaProject reads bounded fixed OBJECTS payload",
          "[dwg-read][object-encode][vba][safety]") {
    const std::vector<std::uint8_t> expected = {0x56u, 0x42u, 0x41u, 0x01u};

    for (const DRW::Version version : {DRW::AC1015, DRW::AC1021,
                                       DRW::AC1027}) {
        std::uint32_t handleBits = 0;
        const auto bytes = emitVbaProjectObject(version, expected, handleBits);
        dwgBuffer reader(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        DRW_VbaProject parsed;
        REQUIRE(DrwObjectEncodeTestAccess::parse(parsed, version, &reader,
                                                 handleBits));
        CHECK(parsed.handle == 0xA10u);
        CHECK(parsed.parentHandle == 0xA11);
        CHECK(parsed.m_dataSize == expected.size());
        CHECK(parsed.m_data == expected);
    }
}

TEST_CASE("DRW_VbaProject rejects an oversized declared payload",
          "[dwg-read][object-encode][vba][safety]") {
    std::uint32_t handleBits = 0;
    const auto bytes = emitVbaProjectObject(
        DRW::AC1027, {0x01u, 0x02u}, handleBits, /*declaredSize=*/0x100u);
    dwgBuffer reader(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
    DRW_VbaProject parsed;
    parsed.m_data = {0x99u};
    parsed.m_dataSize = 1;
    CHECK_FALSE(DrwObjectEncodeTestAccess::parse(parsed, DRW::AC1027,
                                                 &reader, handleBits));
    CHECK_FALSE(reader.isGood());
    CHECK(parsed.m_data.empty());
    CHECK(parsed.m_dataSize == 0u);
}

TEST_CASE("DRW_VbaProject rejects an inconsistent payload size",
          "[dwg-write][object-encode][vba][safety]") {
    DRW_VbaProject project;
    project.m_data = {0x56u, 0x42u};
    project.m_dataSize = 1u;

    dwgBufferW body;
    CHECK_FALSE(project.encodeDwg(DRW::AC1027, &body));
    CHECK(body.data().empty());
}

TEST_CASE("DWG AppInfo writer follows R18 and R21 layouts",
          "[dwg-write][appinfo][safety]") {
    const auto read16 = [](const std::vector<std::uint8_t>& bytes,
                           std::size_t offset) {
        return static_cast<std::uint16_t>(bytes[offset])
            | static_cast<std::uint16_t>(bytes[offset + 1]) << 8;
    };
    const auto read32 = [](const std::vector<std::uint8_t>& bytes,
                           std::size_t offset) {
        return static_cast<std::uint32_t>(bytes[offset])
            | static_cast<std::uint32_t>(bytes[offset + 1]) << 8
            | static_cast<std::uint32_t>(bytes[offset + 2]) << 16
            | static_cast<std::uint32_t>(bytes[offset + 3]) << 24;
    };
    const auto expectByteString = [&](const std::vector<std::uint8_t>& bytes,
                                      std::size_t& offset,
                                      const std::string& expected) {
        REQUIRE(offset + 2 <= bytes.size());
        const std::size_t length = read16(bytes, offset);
        offset += 2;
        REQUIRE(length == expected.size() + 1);
        REQUIRE(offset + length <= bytes.size());
        CHECK(std::string(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                          bytes.begin() + static_cast<std::ptrdiff_t>(offset + expected.size()))
              == expected);
        CHECK(bytes[offset + expected.size()] == 0u);
        offset += length;
    };
    const auto expectUtf16String = [&](const std::vector<std::uint8_t>& bytes,
                                       std::size_t& offset,
                                       const std::string& expected) {
        REQUIRE(offset + 2 <= bytes.size());
        const std::size_t length = read16(bytes, offset);
        offset += 2;
        REQUIRE(length == expected.size() + 1);
        REQUIRE(offset + length * 2 <= bytes.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            CHECK(bytes[offset + i * 2] ==
                  static_cast<std::uint8_t>(expected[i]));
            CHECK(bytes[offset + i * 2 + 1] == 0u);
        }
        CHECK(bytes[offset + expected.size() * 2] == 0u);
        CHECK(bytes[offset + expected.size() * 2 + 1] == 0u);
        offset += length * 2;
    };
    const auto expectChecksum = [&](const std::vector<std::uint8_t>& bytes,
                                    std::size_t& offset) {
        REQUIRE(offset + 16 <= bytes.size());
        for (std::size_t i = 0; i < 16; ++i)
            CHECK(bytes[offset + i] == 0u);
        offset += 16;
    };

    const auto legacy =
        DwgWriter18AppInfoTestAccess::buildAppInfoContent(DRW::AC1018);
    std::size_t offset = 0;
    expectByteString(legacy, offset, "AppInfoDataList");
    REQUIRE(offset + 4 <= legacy.size());
    CHECK(read32(legacy, offset) == 2u);
    offset += 4;
    expectByteString(legacy, offset, "4001");
    expectByteString(legacy, offset,
                     "<ProductInformation name=\"LibreCAD\"/>");
    expectByteString(legacy, offset, "LibreCAD");
    CHECK(offset == legacy.size());

    const auto modern =
        DwgWriter18AppInfoTestAccess::buildAppInfoContent(DRW::AC1021);
    offset = 0;
    REQUIRE(offset + 4 <= modern.size());
    CHECK(read32(modern, offset) == 2u);
    offset += 4;
    expectUtf16String(modern, offset, "AppInfoDataList");
    REQUIRE(offset + 4 <= modern.size());
    CHECK(read32(modern, offset) == 3u);
    offset += 4;
    expectChecksum(modern, offset);
    expectUtf16String(modern, offset, "LibreCAD");
    expectChecksum(modern, offset);
    expectUtf16String(modern, offset, "");
    expectChecksum(modern, offset);
    expectUtf16String(modern, offset, "");
    expectByteString(modern, offset, "LibreCAD");
    CHECK(offset == modern.size());
}

TEST_CASE("DRW_Class parsing is transactional on a truncated record",
          "[dwg-read][classes][safety]") {
    const DRW::Version version = DRW::AC1015;
    dwgBufferW writer;
    writer.putBitShort(500);
    writer.putBitShort(1);
    writer.putVariableText(version, "APP");
    writer.putVariableText(version, "AcDbLine");
    writer.putVariableText(version, "LINE");
    writer.putBit(1);
    writer.putBitShort(0x1F2);

    const auto validBytes = snapshot(writer);
    dwgBuffer validReader(const_cast<std::uint8_t *>(validBytes.data()),
                          validBytes.size());
    DRW_Class parsed;
    REQUIRE(parsed.parseDwg(version, &validReader, &validReader));
    CHECK(parsed.classNum == 500);
    CHECK(parsed.appName == "APP");
    CHECK(parsed.className == "AcDbLine");
    CHECK(parsed.recName == "LINE");
    CHECK(parsed.entityFlag == 1);

    auto truncatedBytes = validBytes;
    REQUIRE(!truncatedBytes.empty());
    truncatedBytes.pop_back();
    dwgBuffer truncatedReader(truncatedBytes.data(), truncatedBytes.size());
    DRW_Class stale;
    stale.classNum = 77;
    stale.recName = "stale";
    stale.entityFlag = 1;
    CHECK_FALSE(stale.parseDwg(version, &truncatedReader, &truncatedReader));
    CHECK_FALSE(truncatedReader.isGood());
    CHECK(truncatedReader.getPosition() == 0);
    CHECK(stale.classNum == 77);
    CHECK(stale.recName == "stale");
    CHECK(stale.entityFlag == 1);
}
