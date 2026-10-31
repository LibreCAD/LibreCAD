/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <boost/multiprecision/cpp_int.hpp>
#include "lc_offsetcurve_persistence_fixture.h"
#include "lc_offsetcurvexdata.h"
#include "rs_layer.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_undo.h"
#include "rs_variable.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {
namespace f = lc::test::offset_persistence;
using Path = std::filesystem::path;
struct Format { const char* name; RS2::FormatType app; DRW::Version codec; bool dwg = false; };
constexpr std::array<Format, 8> dxf{{
    {"R12", RS2::FormatDXFRW12, DRW::AC1009}, {"R14", RS2::FormatDXFRW14, DRW::AC1014},
    {"R2000", RS2::FormatDXFRW2000, DRW::AC1015}, {"R2004", RS2::FormatDXFRW2004, DRW::AC1018},
    {"R2007", RS2::FormatDXFRW, DRW::AC1021}, {"R2010", RS2::FormatDXFRW2010, DRW::AC1024},
    {"R2013", RS2::FormatDXFRW2013, DRW::AC1027}, {"R2018", RS2::FormatDXFRW2018, DRW::AC1032}}};
#ifdef DWGSUPPORT
constexpr std::array<Format, 6> dwg{{
    {"R2000", RS2::FormatDWG, DRW::AC1015, true}, {"R2004", RS2::FormatDWG2004, DRW::AC1018, true},
    {"R2007", RS2::FormatDWG2007, DRW::AC1021, true}, {"R2010", RS2::FormatDWG2010, DRW::AC1024, true},
    {"R2013", RS2::FormatDWG2013, DRW::AC1027, true}, {"R2018", RS2::FormatDWG2018, DRW::AC1032, true}}};
#endif
struct Setup { const bool settings = lc::test::ensureSettings(); lc::test::QuietLog quiet; };

// RS_Debug writes to the stderr FILE, not the C++ stream or admission report.
// Restore the descriptor before making assertions, including on exceptions.
class ExportDiagnostics {
#ifdef _WIN32
    static constexpr auto duplicate = &_dup;
    static constexpr auto replace = &_dup2;
    static constexpr auto close = &_close;
    static constexpr auto descriptor = &_fileno;
#else
    static constexpr auto duplicate = &dup;
    static constexpr auto replace = &dup2;
    static constexpr auto close = &::close;
    static constexpr auto descriptor = &fileno;
#endif
    FILE* file = std::tmpfile();
    int saved = -1;
    bool active = false;
    bool finished = false;
public:
    ExportDiagnostics() {
        if (!file || std::fflush(stderr) != 0) return;
        saved = duplicate(descriptor(stderr));
        active = saved >= 0 && replace(descriptor(file), descriptor(stderr)) >= 0;
    }
    ~ExportDiagnostics() {
        if (active) { std::fflush(stderr); replace(saved, descriptor(stderr)); }
        if (saved >= 0) close(saved);
        if (file) std::fclose(file);
    }
    ExportDiagnostics(const ExportDiagnostics&) = delete;
    ExportDiagnostics& operator=(const ExportDiagnostics&) = delete;
    bool ready() const { return active; }
    bool complete() const { return finished; }
    std::string finish() {
        if (!active || std::fflush(stderr) != 0 || replace(saved, descriptor(stderr)) < 0) return {};
        active = false;
        if (std::fseek(file, 0, SEEK_SET) != 0) return {};
        std::string text;
        std::array<char, 4096> buffer{};
        while (const auto count = std::fread(buffer.data(), 1, buffer.size(), file)) {
            if (text.size() + count > 1024 * 1024) return {};
            text.append(buffer.data(), count);
        }
        finished = std::ferror(file) == 0;
        return text;
    }
};
// Existing persistence snapshot/fingerprint, reused here without changing its
// original tests; the legacy foreign-graph extension follows below.
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

std::string foreignGraphState(const LC_DwgAdvancedMetadata& metadata);
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
    out << foreignGraphState(metadata);
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
        out << graphic.count() << graphic.isModified() << undo << redo << ':' << graphic.undoStateEpoch() << ';';
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

template<class... Values> void graphFields(std::ostream& out, const Values&... values) {
    const auto field = [&](const auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_enum_v<T>) out << static_cast<int>(value);
        else if constexpr (std::is_same_v<T, std::string>) out << std::quoted(value);
        else if constexpr (std::is_floating_point_v<T>) out << f::bits(value);
        else out << value;
        out << ':';
    };
    (field(values), ...); out << ';';
}
std::string foreignGraphState(const LC_DwgAdvancedMetadata& metadata) {
    std::ostringstream out;
    const auto handles = [&](const auto& values) {
        out << values.size() << ':'; for (auto value : values) graphFields(out, value); out << ';';
    };
    const auto optional = [&](const auto& value) {
        out << value.has_value() << ':'; if (value) out << *value; out << ';';
    };
    const auto tags = [&](const auto& values) {
        f::Tags copied; for (const auto& value : values) copied.push_back(std::make_shared<DRW_Variant>(value));
        out << f::tagsState(copied) << ';';
    };
    const auto common = [&](const auto& record) {
        graphFields(out, record.handle, record.parentHandle, record.xDictHandle, record.numReactors,
            unsigned(record.xDictFlag), record.hasDsData, record.replayState);
        handles(record.reactorHandles); out << f::tagsState(record.extData);
        out << record.appData.size() << ':'; for (const auto& group : record.appData) tags(group);
    };
    const auto borderState = [&](const DRW_TableStyleBorder& border) {
        graphFields(out, border.m_edgeFlags, border.m_propertyOverrideFlags, border.m_borderType,
            border.m_color, border.m_lineWeight, border.m_lineTypeHandle, border.m_visible,
            border.m_doubleLineSpacing);
    };
    const auto cellState = [&](const DRW_TableStyleCellStyle& cell) {
        graphFields(out, cell.m_type, cell.m_hasData, cell.m_propertyOverrideFlags, cell.m_mergeFlags,
            cell.m_backgroundColor, cell.m_contentLayout, cell.m_marginOverrideFlags, cell.m_verticalMargin,
            cell.m_horizontalMargin, cell.m_bottomMargin, cell.m_rightMargin, cell.m_marginHorizontalSpacing,
            cell.m_marginVerticalSpacing, cell.m_id, cell.m_styleClass, cell.m_name, cell.m_borders.size());
        const auto& content = cell.m_contentFormat;
        graphFields(out, content.m_propertyOverrideFlags, content.m_propertyFlags, content.m_valueDataType,
            content.m_valueUnitType, content.m_valueFormatString, content.m_rotation, content.m_blockScale,
            content.m_cellAlignment, content.m_contentColor, content.m_textStyleHandle, content.m_textHeight);
        for (const auto& border : cell.m_borders) borderState(border);
    };
    const auto location = [&](const auto& receipt) {
        graphFields(out, receipt.m_version, receipt.m_sourceOffset, receipt.m_sourceMapOrdinal,
            receipt.m_sourceOffsetSpace, receipt.m_hasSourceLocation, receipt.m_complete);
    };
    const auto classIdentity = [&](const auto& receipt) {
        graphFields(out, receipt.m_encodedType, receipt.m_resolvedType, receipt.m_recordName, receipt.m_className);
        optional(receipt.m_classStreamOrdinal);
    };
    const auto& frames = metadata.dwgFrameCoverageReport();
    graphFields(out, frames.m_status, frames.m_complete, frames.m_entries.size(), metadata.dwgFramePublicationCoverageComplete());
    handles(metadata.dwgFramePublicationConflicts());
    for (const auto& row : frames.m_entries)
        graphFields(out, row.m_handle, row.m_sourceOffset, row.m_sourceMapOrdinal, row.m_sourceOffsetSpace,
            row.m_disposition, row.m_reason, unsigned(row.m_publicationCount));
    out << "publications=" << metadata.dwgFramePublications().size() << ';';
    for (const auto& row : metadata.dwgFramePublications()) {
        graphFields(out, row.m_version, row.m_handle, row.m_sourceOffset, row.m_sourceMapOrdinal,
            row.m_sourceOffsetSpace, row.m_hasSourceLocation, row.m_isEntity, row.m_isCustomClass,
            row.m_commonHandleDataValidated, row.m_commonLinkEvidence, row.m_parentHandle, row.m_xDictHandle,
            row.m_numReactors, unsigned(row.m_xDictFlag), row.m_carrier);
        classIdentity(row); handles(row.m_reactorHandles); handles(row.m_controlHandles);
    }
    out << "dictionaries=" << metadata.dictionaries().size() << ';';
    for (const auto& record : metadata.dictionaries()) {
        common(record); graphFields(out, record.cloning, record.hardOwner, record.name, record.typedPayloadComplete, record.entries.size());
        out << record.countCap.has_value() << ';';
        if (record.countCap) { const auto& cap = *record.countCap;
            graphFields(out, cap.declaredCount, cap.clampedCount, cap.bitOffset, cap.remainingBytes, cap.remainingBits); }
        for (const auto& entry : record.entries) graphFields(out, entry.name, entry.handle);
    }
    out << "dictionary-vars=" << metadata.dictionaryVars().size() << ';';
    for (const auto& record : metadata.dictionaryVars()) { common(record); graphFields(out, record.schema, record.value); }
    out << "xrecords=" << metadata.xrecords().size() << ';';
    for (const auto& record : metadata.xrecords()) {
        common(record); graphFields(out, record.cloning, record.rawDataValid);
        tags(record.values); tags(record.dataEntries); handles(record.rawData);
        out << record.handleValues.size() << ';';
        for (const auto& value : record.handleValues) graphFields(out, value.first, value.second);
    }
    out << "memberships=" << metadata.dwgDictionaryMemberships().size() << ';';
    for (const auto& row : metadata.dwgDictionaryMemberships()) {
        location(row); graphFields(out, row.m_dictionaryHandle, row.m_entries.size());
        for (const auto& entry : row.m_entries) graphFields(out, entry.m_name, entry.m_handle);
    }
    out << "defaults=" << metadata.dwgDictionaryWithDefaultMemberships().size() << ';';
    for (const auto& row : metadata.dwgDictionaryWithDefaultMemberships()) {
        location(row); classIdentity(row); graphFields(out, row.m_dictionaryHandle, row.m_cloning,
            row.m_hardOwner, row.m_defaultEntryHandle, row.m_entries.size());
        for (const auto& entry : row.m_entries) graphFields(out, entry.m_name, entry.m_handle);
    }
    out << "typed=" << metadata.dwgTypedReferences().size() << ';';
    for (const auto& row : metadata.dwgTypedReferences()) {
        location(row); graphFields(out, row.m_sourceHandle, row.m_encodedType, row.m_resolvedType,
            row.m_field, row.m_referenceCode, row.m_targetHandle); optional(row.m_classStreamOrdinal);
    }
    out << "styles=" << metadata.tables().size() << ';';
    for (const auto& record : metadata.tables()) {
        graphFields(out, record.handle, record.parentHandle, record.recordName, record.replayState, record.hasDsData,
            record.m_tableFlowDirection, record.m_tableFlags, record.m_tableHorizontalCellMargin,
            record.m_tableVerticalCellMargin, record.m_tableUnknownHandle, record.titleSuppressed,
            record.headerSuppressed, record.nativeTableStyle.has_value());
        handles(record.m_tableStyleIds); handles(record.m_tableStyleNames); handles(record.m_tableTextHeights);
        handles(record.m_tableAlignments); handles(record.m_tableColors);
        if (record.nativeTableStyle) {
            const auto& style = *record.nativeTableStyle;
            graphFields(out, style.tType, style.handle, style.parentHandle, style.name, style.flags,
                style.xDictHandle, style.reactorCount(), unsigned(style.extensionDictionaryFlag()),
                style.hasDwgCommonLinkTail(), style.hasDataStorageBinaryData());
            handles(style.reactorHandles);
            f::Tags extended;
            for (const auto* tag : style.extData)
                extended.push_back(tag ? std::make_shared<DRW_Variant>(*tag) : nullptr);
            out << f::tagsState(extended) << ';';
            out << style.appData.size() << ';'; for (const auto& group : style.appData) tags(group);
            for (const auto* references : {&style.pendingAppIdResolutions, &style.pendingLayerRefResolutions}) {
                out << references->size() << ';';
                for (const auto& ref : *references)
                    graphFields(out, ref.indexInExtData, ref.handleRef, ref.rawHandleRef, ref.useRawHandleRef);
            }
            graphFields(out, style.m_name, style.m_flowDirection, style.m_flags, style.m_horizontalCellMargin,
                style.m_verticalCellMargin, style.m_titleSuppressed, style.m_headerSuppressed, style.m_unknownHandle,
                style.m_rowStyles.size(), style.m_cellStyles.size(), style.m_subrecordRanges.size());
            for (const auto& row : style.m_rowStyles) {
                graphFields(out, row.m_textStyleHandle, row.m_textHeight, row.m_textAlignment, row.m_textColor,
                    row.m_fillColor, row.m_hasBackgroundColor, row.m_valueDataType, row.m_valueUnitType,
                    row.m_valueFormatString, row.m_borders.size());
                for (const auto& border : row.m_borders)
                    borderState(border);
            }
            cellState(style.m_tableCellStyle);
            for (const auto& cell : style.m_cellStyles) cellState(cell);
            for (const auto& range : style.m_subrecordRanges)
                graphFields(out, range.m_name, range.m_startBit, range.m_bitSize, range.m_version,
                    range.m_count, range.m_parseComplete);
        }
    }
    out << "cell-style-maps=" << metadata.cellStyleMaps().size() << ';';
    for (const auto& row : metadata.cellStyleMaps()) {
        graphFields(out, row.handle, row.parentHandle, row.m_cellStyleCount, row.m_borderCount,
            row.m_contentFormatCount, row.m_namedCellStyleCount, row.m_visibleBorderCount, row.m_marginStyleCount,
            row.m_unknownRangeCount, row.m_incompleteRangeCount, row.replayState);
        handles(row.m_styleIds); handles(row.m_styleClasses); handles(row.m_styleNames);
        handles(row.m_textStyleHandles); handles(row.m_lineTypeHandles); handles(row.m_textHeights);
        handles(row.m_alignments); handles(row.m_colors);
    }
    out << "sections=" << metadata.rawDwgSections().size() << ';';
    for (const auto& section : metadata.rawDwgSections()) {
        graphFields(out, section.name, section.version, section.encoding, section.encrypted, section.maxSize, section.replayState);
        handles(section.data);
    }
    return out.str();
}

Path fixture(const std::string& name) {
#ifdef LIBRECAD_TEST_DIR
    return Path(LIBRECAD_TEST_DIR) / "offsetcurve_independent" / name;
#else
    return Path(__FILE__).parent_path() / "testdata" / "offsetcurve_independent" / name;
#endif
}
RS_Vector bezier(const RS_SplineData& data, long double t) {
    REQUIRE(data.controlPoints.size() == data.degree + 1);
    std::array<std::array<long double, 2>, 4> points{};
    for (std::size_t i = 0; i <= data.degree; ++i) points[i] = {data.controlPoints[i].x, data.controlPoints[i].y};
    for (std::size_t n = data.degree; n; --n) for (std::size_t i = 0; i < n; ++i)
        for (std::size_t a = 0; a < 2; ++a) points[i][a] = (1 - t) * points[i][a] + t * points[i + 1][a];
    return {double(points[0][0]), double(points[0][1])};
}
void close(const RS_Vector& a, const RS_Vector& b, double tolerance = 2e-10) {
    CHECK(std::abs(a.x - b.x) <= tolerance); CHECK(std::abs(a.y - b.y) <= tolerance); CHECK(a.z == 0);
}
QString savedLayer(const QString& name, const Format& format) {
    // The R12 writer uses writeUtf8Caps for symbol references.
    return format.codec == DRW::AC1009 ? name.toUpper() : name;
}
f::Tags savedForeign(const f::Tags& tags) {
    f::Tags expected;
    for (std::size_t i = 0; i < tags.size(); ++i) {
        const auto& tag = tags[i];
        const bool ours = tag && tag->code() == 1001 && tag->type() == DRW_Variant::STRING &&
            QString::fromStdString(*tag->content.s).compare(QStringLiteral("LibreCad"), Qt::CaseInsensitive) == 0;
        const bool empty = i + 1 == tags.size() || (tags[i + 1] && tags[i + 1]->code() == 1001);
        // G removes its own empty APPID framing after stripping the last
        // definition. Never remove foreign headers, payloads or typed values.
        if (!(ours && empty)) expected.push_back(tag);
    }
    return expected;
}
RS_Vector savedControl(const RS_Vector& point, const Format& format) {
    if (format.dwg) return point; // Binary doubles remain exact.
    const auto coordinate = [](double value) {
        // Match the actual ASCII writer's precision(16) serialization exactly,
        // rather than introducing a geometric epsilon or accepting arbitrary ULPs.
        std::ostringstream text; text.imbue(std::locale::classic());
        text << std::setprecision(16) << value;
        std::istringstream parsed(text.str()); parsed.imbue(std::locale::classic());
        double result = 0; parsed >> result; REQUIRE_FALSE(parsed.fail());
        return result;
    };
    return {coordinate(point.x), coordinate(point.y), coordinate(point.z)};
}
void absent(const f::Tags& tags) {
    CHECK(f::markerCount(tags) == 0);
    const auto parsed = LC_OffsetCurveXData::parseAndStrip(tags);
    REQUIRE(parsed.remaining);
    CHECK(parsed.status == LC_OffsetCurveXData::Status::NoSubList);
    CHECK(parsed.knownCandidates == 0); CHECK_FALSE(parsed.definition);
    CHECK(f::tagsState(*parsed.remaining) == f::tagsState(tags));
    for (const auto& v : tags) if (v && v->type() == DRW_Variant::STRING) {
        CHECK(*v->content.s != "safe owned tail"); CHECK(*v->content.s != "nested safe item");
    }
}
void load(RS_Graphic& g, const Path& path, const Format& format) {
    REQUIRE(lc::test::importDrawing(g, path, format.app)); g.onLoadingCompleted();
}
void prepareInput(RS_Graphic& g, const Path& input, int degree) {
    // Literal mathematical fixture, not the feature's writer or producer.
    f::put(input, f::ascii(f::raw(degree))); load(g, input, dxf.back());
    auto* native = dynamic_cast<LC_OffsetCurve*>(f::sole(g)); REQUIRE(native); REQUIRE(native->hasEditableDefinition());
}
struct Prepared {
    LC_OffsetCurve* original = nullptr; // document retains this for undo
    RS_SplineData source;
    std::vector<RS_SplineData> expected;
    f::Tags foreign;
    QString layer;
    RS_Pen pen;
};
Prepared trimDrawing(RS_Graphic& g, bool middle) {
    auto* native = dynamic_cast<LC_OffsetCurve*>(f::sole(g)); REQUIRE(native); REQUIRE(native->hasEditableDefinition());
    Prepared out; out.original = native; out.source = native->getData(); out.foreign = native->getDrwExtData();
    REQUIRE(native->getLayer(false)); out.layer = native->getLayer(false)->getName(); out.pen = native->getPen(false);
    REQUIRE(out.source.controlPoints.size() == out.source.degree + 1);
    const auto lo = bezier(out.source, middle ? .25L : .5L), hi = bezier(out.source, .75L);
    double x0 = out.source.controlPoints.front().x, x1 = x0;
    for (const auto& p : out.source.controlPoints) { x0 = std::min(x0, p.x); x1 = std::max(x1, p.x); }
    std::unique_ptr<RS_Entity> cutter;
    if (middle) {
        RS_SplineData u{1, false}; u.controlPoints = {{x0 - 1, lo.y}, {x1 + 1, lo.y}, {x1 + 1, hi.y}, {x0 - 1, hi.y}};
        u.weights.assign(4, 1); u.knotslist = {0, 0, 1, 2, 3, 3}; cutter = std::make_unique<RS_Spline>(nullptr, u);
    } else cutter = std::make_unique<RS_Line>(nullptr, RS_Vector{x0 - 1, lo.y}, RS_Vector{x1 + 1, lo.y});
    const auto pick = bezier(out.source, middle ? .5L : .25L);
    LC_GraphicViewport viewport; viewport.setDocument(&g);
    REQUIRE(g.undoableModify(&viewport, [&](LC_DocumentModificationBatch& batch) {
        const auto result = RS_Modification::trimStoredCache(g, *native, pick, *cutter, lo, false, batch);
        const int refusal = result.cacheRefusal ? static_cast<int>(*result.cacheRefusal) : -1;
        CAPTURE(refusal);
        REQUIRE(result.result); CHECK(result.nativeDemotions == 1); return result.result;
    }));
    REQUIRE(native->isDeleted()); const auto entities = f::live(g); REQUIRE(entities.size() == (middle ? 2u : 1u));
    for (std::size_t i = 0; i < entities.size(); ++i) {
        REQUIRE(entities[i]->rtti() == RS2::EntitySpline); const auto* spline = static_cast<RS_Spline*>(entities[i]);
        out.expected.push_back(spline->getData()); absent(spline->getDrwExtData());
        CHECK(f::tagsState(spline->getDrwExtData()) == f::tagsState(out.foreign));
        const long double a = middle ? (i == 0 ? 0.L : .75L) : .5L;
        const long double b = middle && i == 0 ? .25L : 1.L;
        // Independent expected retained ranges, NOT the kernel's interval labels.
        for (int k = 0; k <= 64; ++k) close(bezier(out.expected.back(), k / 64.L), bezier(out.source, a + (b - a) * k / 64.L));
        CHECK(spline->ownerLineage() == 0); CHECK(spline->sourceHandle() == 0); CHECK(spline->xDictHandle() == 0);
        CHECK(spline->reactorHandles().empty()); CHECK(spline->lineage() != native->lineage());
    }
    return out;
}
struct Capture final : lc::test::NullDrwInterface {
    std::vector<DRW_Spline> splines;
    std::vector<DRW_Polyline> polylines;
    std::size_t ellipses = 0;
    void addSpline(const DRW_Spline* p) override { REQUIRE(p); splines.push_back(*p); }
    void addPolyline(const DRW_Polyline& p) override { polylines.push_back(p); }
    void addEllipse(const DRW_Ellipse&) override { ++ellipses; }
};
Capture inspect(const Path& path, const Format& format) {
    Capture out;
#ifdef DWGSUPPORT
    if (format.dwg) {
        dwgRW reader(path.string().c_str()); REQUIRE(reader.read(&out, false)); CHECK(reader.getVersion() == format.codec); return out;
    }
#else
    REQUIRE_FALSE(format.dwg);
#endif
    dxfRW reader(path.string().c_str()); REQUIRE(reader.read(&out, false)); CHECK(reader.getSourceVersion() == format.codec); return out;
}
double distanceToSegment(RS_Vector p, RS_Vector a, RS_Vector b) {
    const long double dx = static_cast<long double>(b.x) - a.x, dy = static_cast<long double>(b.y) - a.y;
    const long double d = dx * dx + dy * dy;
    const long double t = d == 0 ? 0 : std::clamp(((static_cast<long double>(p.x) - a.x) * dx +
        (static_cast<long double>(p.y) - a.y) * dy) / d, 0.L, 1.L);
    return double(std::hypot(static_cast<long double>(p.x) - a.x - t * dx, static_cast<long double>(p.y) - a.y - t * dy));
}
void inspectPrepared(const Path& file, const Format& format, const Prepared& expected) {
    const auto records = inspect(file, format); CHECK(records.ellipses == 0);
    if (format.codec == DRW::AC1009) {
        CHECK(records.splines.empty()); REQUIRE(records.polylines.size() == expected.expected.size());
        for (std::size_t i = 0; i < records.polylines.size(); ++i) {
            const auto& poly = records.polylines[i]; REQUIRE(poly.vertlist.size() >= 2); CHECK((poly.flags & 1) == 0);
            CHECK(poly.layer == savedLayer(expected.layer, format).toStdString()); absent(poly.extData);
            CHECK(f::tagsState(poly.extData) == f::tagsState(savedForeign(expected.foreign)));
            const auto& data = expected.expected[i];
            const double extent = data.controlPoints.front().distanceTo(data.controlPoints.back());
            for (int k = 0; k <= 64; ++k) {
                const auto point = bezier(data, k / 64.L); double distance = std::numeric_limits<double>::infinity();
                for (std::size_t j = 1; j < poly.vertlist.size(); ++j) {
                    REQUIRE(poly.vertlist[j - 1]); REQUIRE(poly.vertlist[j]);
                    const auto& a = poly.vertlist[j - 1]->basePoint; const auto& b = poly.vertlist[j]->basePoint;
                    distance = std::min(distance, distanceToSegment(point, {a.x, a.y}, {b.x, b.y}));
                }
                CHECK(distance <= 2e-4 * extent + 1e-11);
            }
        }
    } else {
        CHECK(records.polylines.empty()); REQUIRE(records.splines.size() == expected.expected.size());
        for (std::size_t i = 0; i < records.splines.size(); ++i) {
            const auto& raw = records.splines[i]; const auto& data = expected.expected[i];
            CHECK(raw.degree == int(data.degree)); CHECK((raw.flags & 3) == 0); CHECK(raw.layer == expected.layer.toStdString());
            REQUIRE(raw.controllist.size() == data.controlPoints.size());
            for (std::size_t j = 0; j < raw.controllist.size(); ++j) { REQUIRE(raw.controllist[j]); close({raw.controllist[j]->x, raw.controllist[j]->y, raw.controllist[j]->z}, data.controlPoints[j]); }
            CHECK(raw.knotslist == data.knotslist); CHECK(raw.fitlist.empty());
            CHECK((raw.weightlist.empty() || raw.weightlist == data.weights)); absent(raw.extData);
            CHECK(f::tagsState(raw.extData) == f::tagsState(savedForeign(expected.foreign)));
        }
    }
}
void ordinary(RS_Graphic& g, const Prepared& expected, const Format& format) {
    const auto entities = f::live(g); REQUIRE(entities.size() == expected.expected.size());
    for (std::size_t i = 0; i < entities.size(); ++i) {
        const auto* e = entities[i]; CHECK(dynamic_cast<const LC_OffsetCurve*>(e) == nullptr);
        REQUIRE(e->getLayer(false)); CHECK(e->getLayer(false)->getName() == savedLayer(expected.layer, format)); CHECK(e->isVisible());
        CHECK(e->getPen(false).getColor() == expected.pen.getColor());
        CHECK(e->getPen(false).getLineType() == expected.pen.getLineType());
        if (format.codec >= DRW::AC1015) CHECK(e->getPen(false).getWidth() == expected.pen.getWidth());
        absent(e->getDrwExtData()); CHECK(f::tagsState(savedForeign(e->getDrwExtData())) == f::tagsState(savedForeign(expected.foreign)));
        if (format.codec == DRW::AC1009) REQUIRE(e->rtti() == RS2::EntityPolyline);
        else {
            REQUIRE(e->rtti() == RS2::EntitySpline); const auto& actual = static_cast<const RS_Spline*>(e)->getData();
            CHECK(actual.degree == expected.expected[i].degree);
            CHECK_FALSE(actual.m_closedFlag); CHECK_FALSE(actual.m_periodicFlag);
            CHECK(actual.knotslist == expected.expected[i].knotslist);
            CHECK(actual.fitPoints.empty()); CHECK(actual.savedOpenKnots.empty());
            CHECK((actual.weights.empty() || actual.weights == expected.expected[i].weights));
            REQUIRE(actual.controlPoints.size() == expected.expected[i].controlPoints.size());
            for (std::size_t j = 0; j < actual.controlPoints.size(); ++j) close(actual.controlPoints[j], expected.expected[i].controlPoints[j]);
        }
    }
}
void row(const Format& format, int degree, bool middle) {
    Setup setup; lc::test::OutputDirectory out("T-files-" + std::string(format.dwg ? "dwg-" : "dxf-") + format.name + "-" + std::to_string(degree) + (middle ? "-two" : "-one"));
    RS_Graphic source; prepareInput(source, out.file("input.dxf"), degree); const auto expected = trimDrawing(source, middle);
    const auto first = out.file(format.dwg ? "first.dwg" : "first.dxf");
    REQUIRE(lc::test::exportDrawing(source, first, format.app)); inspectPrepared(first, format, expected);
    RS_Graphic reopened; load(reopened, first, format); ordinary(reopened, expected, format);
    const auto second = out.file(format.dwg ? "second.dwg" : "second.dxf");
    REQUIRE(lc::test::exportDrawing(reopened, second, format.app)); inspectPrepared(second, format, expected);
    RS_Graphic again; load(again, second, format); ordinary(again, expected, format);
}
struct BinaryPlain final : lc::test::NullDrwInterface {
    dxfRW& writer; const std::vector<DRW_Spline>& splines;
    BinaryPlain(dxfRW& w, const std::vector<DRW_Spline>& s) : writer(w), splines(s) {}
    void writeHeader(DRW_Header& h) override { h.addStr("$DWGCODEPAGE", "ANSI_1252", 3); }
    void writeLayers() override {
        DRW_Layer layer; layer.name = "FixtureOffsets"; layer.color = 3; REQUIRE(writer.writeLayer(&layer));
    }
    void writeAppId() override {
        for (const auto* name : {"LibreCad", "FOREIGN_A", "FOREIGN_B"}) { DRW_AppId app; app.name = name; REQUIRE(writer.writeAppId(&app)); }
    }
    void writeEntities() override {
        for (const auto& spline : splines) { DRW_Spline copy = spline; REQUIRE(writer.writeSpline(&copy)); }
    }
};
void sameDefinition(const LC_EllipseOffsetDefinition& actual, const LC_EllipseOffsetDefinition& expected) {
    close(actual.basis.center, expected.basis.center, 0); close(actual.basis.majorP, expected.basis.majorP, 0);
    CHECK(actual.basis.ratio == expected.basis.ratio);
    CHECK(actual.basis.angle1 == expected.basis.angle1); CHECK(actual.basis.angle2 == expected.basis.angle2);
    CHECK(actual.basis.reversed == expected.basis.reversed); CHECK(actual.basis.isArc == expected.basis.isArc);
    CHECK(actual.distance == expected.distance); CHECK(actual.relTolerance == expected.relTolerance);
    CHECK(actual.towardCentre == expected.towardCentre); CHECK(actual.cacheReversed == expected.cacheReversed);
}
void sameTail(const LC_OffsetCurve::OwnedTail& actual, const LC_OffsetCurve::OwnedTail& expected) {
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        CAPTURE(i); CHECK(actual[i].code == expected[i].code);
        REQUIRE(actual[i].value.index() == expected[i].value.index());
        switch (actual[i].value.index()) {
        case 0: CHECK(std::get<QString>(actual[i].value) == std::get<QString>(expected[i].value)); break;
        case 1: close(std::get<RS_Vector>(actual[i].value), std::get<RS_Vector>(expected[i].value), 0); break;
        case 2: CHECK(std::get<double>(actual[i].value) == std::get<double>(expected[i].value)); break;
        case 3: CHECK(std::get<std::int16_t>(actual[i].value) == std::get<std::int16_t>(expected[i].value)); break;
        case 4: CHECK(std::get<std::int32_t>(actual[i].value) == std::get<std::int32_t>(expected[i].value)); break;
        default: FAIL("Unexpected native extension type");
        }
    }
}
struct NativeBefore {
    RS_SplineData cache;
    LC_EllipseOffsetDefinition definition;
    unsigned flags;
    LC_OffsetCurve::OwnedTail tail;
    f::Tags foreign;
    QString layer;
    RS_Pen pen;
    bool whole, closed;
    explicit NativeBefore(const LC_OffsetCurve& curve) : cache(curve.getData()), definition(curve.definition()),
        flags(curve.unknownOwnedFlags()), tail(curve.unknownOwnedTail()), foreign(curve.getDrwExtData()),
        layer(curve.getLayer(false) ? curve.getLayer(false)->getName() : QString()), pen(curve.getPen(false)),
        whole(curve.sourceIsWhole()), closed(curve.cacheFormsClosedChain()) {
        REQUIRE(curve.hasEditableDefinition()); REQUIRE(curve.getLayer(false));
        const auto& geometry = curve.definition();
        const double portable = LC_CurveOffset::kEllipseRelativeOffsetTolerance;
        REQUIRE(geometry.relTolerance >= .1 * portable);
        REQUIRE(geometry.relTolerance <= 10 * portable);
        const RS_Ellipse source{nullptr, geometry.basis};
        const double readTolerance = LC_CurveOffset::toleranceFor(source, geometry.distance, geometry.relTolerance);
        REQUIRE(std::isfinite(readTolerance)); REQUIRE(readTolerance > 0);
        // This is the actual tolerance reconstructed by the file reader, not
        // the possibly looser tolerance used for an in-memory adoption.
        const auto checked = LC_EllipseOffsetGeometry::checkCache(curve.getData(), geometry, readTolerance);
        CAPTURE(checked.reason, checked.span); REQUIRE(checked);
        REQUIRE(flags != 0); REQUIRE_FALSE(tail.empty()); f::foreign(curve);
    }
};
void restoredNative(RS_Graphic& graphic, const NativeBefore& expected, const Format* serialized = nullptr) {
    auto* curve = dynamic_cast<LC_OffsetCurve*>(f::sole(graphic)); REQUIRE(curve);
    REQUIRE(curve->hasEditableDefinition()); CHECK(curve->getParent() == &graphic);
    CHECK(curve->sourceIsWhole() == expected.whole); CHECK(curve->cacheFormsClosedChain() == expected.closed);
    sameDefinition(curve->definition(), expected.definition);
    CHECK(curve->unknownOwnedFlags() == expected.flags); sameTail(curve->unknownOwnedTail(), expected.tail);
    const auto& data = curve->getData(); CHECK(data.degree == expected.cache.degree);
    CHECK(data.m_closedFlag == expected.cache.m_closedFlag); CHECK(data.m_periodicFlag == expected.cache.m_periodicFlag);
    CHECK(data.knotslist == expected.cache.knotslist); CHECK(data.weights == expected.cache.weights);
    CHECK(data.fitPoints.empty()); CHECK(data.savedOpenKnots.empty());
    REQUIRE(data.controlPoints.size() == expected.cache.controlPoints.size());
    for (std::size_t i = 0; i < data.controlPoints.size(); ++i)
        close(data.controlPoints[i], serialized ? savedControl(expected.cache.controlPoints[i], *serialized)
                                              : expected.cache.controlPoints[i], 0);
    REQUIRE(curve->getLayer(false)); CHECK(curve->getLayer(false)->getName() == expected.layer);
    CHECK(curve->isVisible()); CHECK(curve->getPen(false).getColor() == expected.pen.getColor());
    CHECK(curve->getPen(false).getLineType() == expected.pen.getLineType());
    CHECK(curve->getPen(false).getWidth() == expected.pen.getWidth());
    CHECK(f::tagsState(curve->getDrwExtData()) == f::tagsState(expected.foreign)); f::foreign(*curve);
}
void inspectNative(const Path& path, const Format& format, const NativeBefore& expected) {
    const auto records = inspect(path, format); REQUIRE(records.splines.size() == 1);
    CHECK(records.polylines.empty()); CHECK(records.ellipses == 0);
    const auto& spline = records.splines.front(); CHECK(spline.degree == int(expected.cache.degree));
    CHECK(bool(spline.flags & 1) == expected.cache.m_closedFlag);
    CHECK(bool(spline.flags & 2) == expected.cache.m_periodicFlag);
    CHECK(spline.layer == expected.layer.toStdString()); CHECK(spline.knotslist == expected.cache.knotslist);
    CHECK((spline.weightlist.empty() || spline.weightlist == expected.cache.weights)); CHECK(spline.fitlist.empty());
    REQUIRE(spline.controllist.size() == expected.cache.controlPoints.size());
    for (std::size_t i = 0; i < spline.controllist.size(); ++i) {
        REQUIRE(spline.controllist[i]); const auto& p = *spline.controllist[i];
        close({p.x, p.y, p.z}, savedControl(expected.cache.controlPoints[i], format), 0);
    }
    REQUIRE(f::markerCount(spline.extData) == 1);
    const auto parsed = LC_OffsetCurveXData::parseAndStrip(spline.extData);
    REQUIRE(parsed.status == LC_OffsetCurveXData::Status::Schema1);
    REQUIRE(parsed.knownCandidates == 1); REQUIRE(parsed.definition); REQUIRE(parsed.remaining);
    sameDefinition(parsed.definition->geometry, expected.definition);
    CHECK(parsed.definition->reservedFlags == expected.flags); sameTail(parsed.definition->tail, expected.tail);
    CHECK(f::tagsState(*parsed.remaining) == f::tagsState(expected.foreign));
}
void plainTwice(RS_Graphic& source, const lc::test::OutputDirectory& out, const Format& format,
                const Prepared& expected, const std::string& prefix) {
    const std::string extension = format.dwg ? ".dwg" : ".dxf";
    const auto first = out.file(prefix + "-first" + extension);
    REQUIRE(lc::test::exportDrawing(source, first, format.app)); inspectPrepared(first, format, expected);
    RS_Graphic reopened; load(reopened, first, format); ordinary(reopened, expected, format);
    const auto second = out.file(prefix + "-second" + extension);
    REQUIRE(lc::test::exportDrawing(reopened, second, format.app)); inspectPrepared(second, format, expected);
    RS_Graphic again; load(again, second, format); ordinary(again, expected, format);
}
void undoRedoFiles(RS_Graphic& source, const lc::test::OutputDirectory& out, const Format& format,
                   const Prepared& trimmed, const NativeBefore& native) {
    REQUIRE(source.undo()); REQUIRE(f::sole(source) == trimmed.original); restoredNative(source, native);
    const std::string extension = format.dwg ? ".dwg" : ".dxf";
    const auto first = out.file("undo-native-first" + extension);
    REQUIRE(lc::test::exportDrawing(source, first, format.app)); inspectNative(first, format, native);
    RS_Graphic reopened; load(reopened, first, format); restoredNative(reopened, native, &format);
    const auto second = out.file("undo-native-second" + extension);
    REQUIRE(lc::test::exportDrawing(reopened, second, format.app)); inspectNative(second, format, native);
    RS_Graphic again; load(again, second, format); restoredNative(again, native, &format);
    REQUIRE(source.redo()); REQUIRE(trimmed.original->isDeleted()); ordinary(source, trimmed, format);
    plainTwice(source, out, format, trimmed, "redo-plain");
}
void openUndoRow(const Format& format, int degree) {
    CAPTURE(format.name, format.dwg, degree);
    lc::test::OutputDirectory out("T-files-undo-" + std::string(format.dwg ? "dwg-" : "dxf-") + std::to_string(degree));
    RS_Graphic source; prepareInput(source, out.file("native.dxf"), degree);
    auto* curve = dynamic_cast<LC_OffsetCurve*>(f::sole(source)); REQUIRE(curve);
    const NativeBefore before(*curve); const auto expected = trimDrawing(source, true);
    undoRedoFiles(source, out, format, expected, before);
}
LC_OffsetCurve* closedPolygon(RS_Graphic& graphic, bool reverse, std::size_t segments, int degree) {
    using Rational = boost::multiprecision::cpp_rational;
    REQUIRE((segments == 128 || segments == 256));
    REQUIRE((degree == 1 || degree == 3));
    graphic.initForNewDocument(); graphic.onLoadingCompleted();
    constexpr double radius = 1.25, worldTolerance = .001;
    const std::size_t quarter = segments / 4;
    RS_SplineData data{1, false};
    for (std::size_t i = 0; i < segments; ++i) {
        const auto offset = i % quarter;
        const double angle = std::acos(-1.) * (double(offset) / (2 * double(quarter)));
        // Exact cardinal vertices; do not use cos(pi/2) as a cutter root.
        RS_Vector p = offset == 0 ? RS_Vector{radius, 0} :
            RS_Vector{radius * std::cos(angle), radius * std::sin(angle)};
        REQUIRE(p.x > 0); REQUIRE(p.y >= 0);
        if (offset != 0) REQUIRE(p.y > 0);
        for (std::size_t q = 0; q < i / quarter; ++q) p = {-p.y, p.x};
        data.controlPoints.push_back(p);
    }
    data.controlPoints.push_back(data.controlPoints.front());
    data.knotslist.push_back(0);
    for (std::size_t i = 0; i <= segments; ++i) data.knotslist.push_back(double(i));
    data.knotslist.push_back(double(segments));
    data.weights.assign(segments + 1, 1); data.m_closedFlag = true;
    REQUIRE(data.type == RS_SplineData::SplineType::ClampedOpen);
    REQUIRE(data.knotslist.size() == segments + 3);
    REQUIRE_FALSE(data.m_periodicFlag);
    for (const auto& p : data.controlPoints) {
        REQUIRE(p.valid); REQUIRE(std::isfinite(p.x)); REQUIRE(std::isfinite(p.y)); REQUIRE(p.z == 0);
    }
    close(data.controlPoints[quarter], {0, radius}, 0);
    close(data.controlPoints[2 * quarter], {-radius, 0}, 0);
    close(data.controlPoints[3 * quarter], {0, -radius}, 0);
    REQUIRE(f::bits(data.controlPoints.front().x) == f::bits(data.controlPoints.back().x));
    REQUIRE(f::bits(data.controlPoints.front().y) == f::bits(data.controlPoints.back().y));
    // For the ideal N-gon, sagitta <= R*(pi/N)^2/2 and pi < 22/7.
    // Exact rational checks below prove this bound (plus a vertex allowance)
    // for the ACTUAL binary64 controls, independently of libm accuracy or C.
    const Rational r{radius}, piUpper = Rational(22) / 7;
    const Rational vertexAllowance = Rational(64) * Rational(std::numeric_limits<double>::epsilon()) * r;
    const Rational chordBound = r * piUpper * piUpper / (Rational(2) * segments * segments) + vertexAllowance;
    REQUIRE((chordBound < Rational(worldTolerance)));
    const Rational innerVertex = r - vertexAllowance, outerVertex = r + vertexAllowance;
    const Rational innerChord = r - chordBound;
    REQUIRE((innerChord > 0));
    for (std::size_t i = 0; i < segments; ++i) {
        CAPTURE(segments, i);
        const auto& a = data.controlPoints[i]; const auto& b = data.controlPoints[i + 1];
        const Rational ax{a.x}, ay{a.y}, bx{b.x}, by{b.y};
        const Rational normSquared = ax * ax + ay * ay;
        REQUIRE((normSquared >= innerVertex * innerVertex));
        REQUIRE((normSquared <= outerVertex * outerVertex));
        const Rational dx = bx - ax, dy = by - ay, lengthSquared = dx * dx + dy * dy;
        const Rational area = ax * by - ay * bx, dot = ax * bx + ay * by;
        REQUIRE((lengthSquared > 0)); REQUIRE((area > 0)); REQUIRE((dot > 0));
        // min radius on the entire supporting line is area/sqrt(lengthSquared).
        // Convexity bounds max radius by the endpoints. Positive area/dot and
        // the four cardinal anchors cover one full turn, not a partial arc.
        REQUIRE((area * area >= innerChord * innerChord * lengthSquared));
    }
    if (degree == 3) {
        const auto vertices = data.controlPoints;
        const Rational roundingAllowance = vertexAllowance;
        REQUIRE((chordBound + roundingAllowance < Rational(worldTolerance)));
        REQUIRE((vertexAllowance + roundingAllowance < Rational(worldTolerance)));
        data = RS_SplineData{3, false};
        data.controlPoints.reserve(3 * segments + 1);
        data.controlPoints.push_back(vertices.front());
        for (std::size_t span = 0; span < segments; ++span) {
            const auto& a = vertices[span]; const auto& b = vertices[span + 1];
            const Rational ax{a.x}, ay{a.y}, bx{b.x}, by{b.y};
            for (unsigned k = 1; k < 3; ++k) {
                const Rational exactX = ((3 - k) * ax + k * bx) / 3;
                const Rational exactY = ((3 - k) * ay + k * by) / 3;
                const RS_Vector control{exactX.convert_to<double>(), exactY.convert_to<double>()};
                const Rational dx = Rational(control.x) - exactX;
                const Rational dy = Rational(control.y) - exactY;
                CAPTURE(segments, span, k);
                REQUIRE(control.valid); REQUIRE(control.z == 0);
                REQUIRE(std::isfinite(control.x)); REQUIRE(std::isfinite(control.y));
                REQUIRE((dx * dx + dy * dy <= roundingAllowance * roundingAllowance));
                data.controlPoints.push_back(control);
            }
            data.controlPoints.push_back(b);
        }
        // The exact one-third controls elevate each polygon edge to a cubic.
        // Bernstein weights are nonnegative and sum to one, so the measured
        // control errors bound the WHOLE actual cubic's distance from that
        // edge. Together with the exact polygon annulus above, this proves
        // both radial bounds for every actual rounded cubic, not samples.
        for (std::size_t k = 0; k <= segments; ++k)
            for (unsigned repeat = 0; repeat < (k == 0 || k == segments ? 4u : 3u); ++repeat)
                data.knotslist.push_back(double(k));
        data.weights.assign(data.controlPoints.size(), 1);
        data.m_closedFlag = true;
    }
    REQUIRE(data.degree == std::size_t(degree));
    REQUIRE(data.controlPoints.size() == std::size_t(degree) * segments + 1);
    REQUIRE(data.knotslist.size() == std::size_t(degree) * segments + degree + 2);
    REQUIRE(data.weights.size() == data.controlPoints.size());
    REQUIRE(std::all_of(data.weights.begin(), data.weights.end(), [](double w) { return w == 1; }));
    close(data.controlPoints[degree * quarter], {0, radius}, 0);
    close(data.controlPoints[degree * 2 * quarter], {-radius, 0}, 0);
    close(data.controlPoints[degree * 3 * quarter], {0, -radius}, 0);
    REQUIRE(f::bits(data.controlPoints.front().x) == f::bits(data.controlPoints.back().x));
    REQUIRE(f::bits(data.controlPoints.front().y) == f::bits(data.controlPoints.back().y));
    if (reverse) std::reverse(data.controlPoints.begin(), data.controlPoints.end());
    RS_Spline cache{&graphic, data}; LC_EllipseOffsetDefinition definition;
    definition.basis = {{0, 0}, {1, 0}, 1}; definition.distance = .25;
    definition.relTolerance = .001; definition.cacheReversed = reverse;
    LC_OffsetCheckResult checked; LC_OffsetCurveRefusal refusal = LC_OffsetCurveRefusal::InvalidInput;
    auto curve = LC_OffsetCurve::adopt(cache, definition, worldTolerance, &checked, {}, &refusal);
    CAPTURE(segments, reverse, refusal, checked.reason, checked.span, checked.work.cacheSpans,
        checked.work.vertices, checked.work.distancePairs, checked.work.deepest, checked.work.orderingComparisons);
    REQUIRE(checked); REQUIRE(checked.analysis); REQUIRE(checked.analysis->closed()); REQUIRE(curve);
    REQUIRE(curve->hasEditableDefinition()); REQUIRE(curve->sourceIsWhole()); REQUIRE(curve->cacheFormsClosedChain());
    REQUIRE(curve->getData().degree == std::size_t(degree));
    REQUIRE(curve->getData().controlPoints.size() == std::size_t(degree) * segments + 1);
    REQUIRE(curve->checkedWorldTolerance() == worldTolerance);
    REQUIRE(curve->getData().type == RS_SplineData::SplineType::ClampedOpen);
    REQUIRE(curve->getData().m_closedFlag); REQUIRE_FALSE(curve->getData().m_periodicFlag);
    auto* layer = new RS_Layer("FixtureOffsets"); graphic.addLayer(layer); curve->setLayer(layer);
    curve->setPen(RS_Pen{RS_Color{0, 255, 0}, RS2::Width07, RS2::SolidLine});
    curve->setDrwExtData(f::metadata(f::Tags{}));
    REQUIRE(curve->setOwnedExtensions(0x40, {{1000, QStringLiteral("safe owned tail")},
        {1071, std::int32_t{123456}}, {1002, QStringLiteral("{")},
        {1000, QStringLiteral("nested safe item")}, {1002, QStringLiteral("}")}}));
    auto* result = curve.release(); graphic.addEntity(result); return result;
}
Prepared trimClosed(RS_Graphic& graphic, LC_OffsetCurve& curve) {
    REQUIRE(curve.hasEditableDefinition()); REQUIRE(curve.cacheFormsClosedChain());
    REQUIRE(f::sole(graphic) == &curve); REQUIRE(curve.getLayer(false));
    Prepared expected; expected.original = &curve; expected.source = curve.getData();
    expected.foreign = curve.getDrwExtData(); expected.layer = curve.getLayer(false)->getName(); expected.pen = curve.getPen(false);
    const std::size_t degree = expected.source.degree;
    REQUIRE((degree == 1 || degree == 3));
    REQUIRE(expected.source.controlPoints.size() >= degree + 1);
    REQUIRE((expected.source.controlPoints.size() - 1) % degree == 0);
    const auto sourceSpans = (expected.source.controlPoints.size() - 1) / degree;
    REQUIRE(sourceSpans % 4 == 0); REQUIRE(sourceSpans >= 4);
    const auto firstRetained = sourceSpans / 4, retainedSpans = sourceSpans / 2;
    // x=0 meets only the two stored quarter vertices. Remove the right half;
    // retain the exact left-half control range, independent of trim's output.
    RS_SplineData half{int(degree), false};
    half.controlPoints.assign(expected.source.controlPoints.begin() + degree * firstRetained,
        expected.source.controlPoints.begin() + degree * (firstRetained + retainedSpans) + 1);
    for (std::size_t k = 0; k <= retainedSpans; ++k)
        for (std::size_t i = 0; i < (k == 0 || k == retainedSpans ? degree + 1 : degree); ++i)
            half.knotslist.push_back(double(k));
    half.weights.assign(half.controlPoints.size(), 1);
    expected.expected.push_back(half);
    // The exact stored closing vertex selects the right half in either
    // direction. It is not either cutter root and needs no rounded curve evaluation.
    const auto pick = expected.source.controlPoints.front(); REQUIRE(pick.x > 0);
    RS_Line cutter{nullptr, {0, -3}, {0, 3}}; LC_GraphicViewport viewport; viewport.setDocument(&graphic);
    const auto prepared = LC_OffsetCacheTrim::prepare(expected.source, cutter, {pick});
    CAPTURE(degree, sourceSpans, prepared.refusal);
    REQUIRE(prepared.refusal == LC_OffsetCacheTrim::Refusal::None);
    REQUIRE(prepared.fragments.size() == 1);
    const auto& preparedHalf = prepared.fragments.front();
    REQUIRE(preparedHalf.data.controlPoints.size() == half.controlPoints.size());
    CHECK(preparedHalf.data.knotslist == half.knotslist);
    CHECK(preparedHalf.data.weights == half.weights);
    for (std::size_t i = 0; i < half.controlPoints.size(); ++i)
        close(preparedHalf.data.controlPoints[i], half.controlPoints[i], 0);
    CHECK(preparedHalf.start.imageX.contains(half.controlPoints.front().x));
    CHECK(preparedHalf.start.imageY.contains(half.controlPoints.front().y));
    CHECK(preparedHalf.end.imageX.contains(half.controlPoints.back().x));
    CHECK(preparedHalf.end.imageY.contains(half.controlPoints.back().y));
    REQUIRE(graphic.undoableModify(&viewport, [&](LC_DocumentModificationBatch& batch) {
        const auto result = RS_Modification::trimStoredCache(graphic, curve, pick, cutter, {0, 0}, false, batch);
        const int refusal = result.cacheRefusal ? static_cast<int>(*result.cacheRefusal) : -1;
        CAPTURE(refusal);
        REQUIRE(result.result); CHECK(result.nativeDemotions == 1); return result.result;
    }));
    REQUIRE(curve.isDeleted()); const auto entities = f::live(graphic); REQUIRE(entities.size() == 1);
    REQUIRE(entities.front()->rtti() == RS2::EntitySpline);
    const auto* fragment = static_cast<const RS_Spline*>(entities.front());
    REQUIRE(dynamic_cast<const LC_OffsetCurve*>(fragment) == nullptr); const auto& actual = fragment->getData();
    REQUIRE(actual.degree == degree); REQUIRE(actual.controlPoints.size() == half.controlPoints.size());
    CHECK(actual.knotslist == half.knotslist); CHECK(actual.weights == half.weights);
    CHECK_FALSE(actual.m_closedFlag); CHECK_FALSE(actual.m_periodicFlag);
    for (std::size_t i = 0; i < half.controlPoints.size(); ++i) close(actual.controlPoints[i], half.controlPoints[i], 0);
    close(actual.controlPoints.front(), half.controlPoints.front(), 0); close(actual.controlPoints.back(), half.controlPoints.back(), 0);
    CHECK(actual.controlPoints.front().x == 0); CHECK(actual.controlPoints.back().x == 0);
    // Probe every retained span, not only the endpoints/control hull, with the
    // existing independent de Casteljau evaluator. Traversal/order are fixed.
    for (std::size_t span = 0; span < retainedSpans; ++span) {
        RS_SplineData retained{int(degree), false}, original{int(degree), false};
        retained.controlPoints.assign(actual.controlPoints.begin() + degree * span,
            actual.controlPoints.begin() + degree * span + degree + 1);
        original.controlPoints.assign(expected.source.controlPoints.begin() + degree * (span + firstRetained),
            expected.source.controlPoints.begin() + degree * (span + firstRetained) + degree + 1);
        const int samples = degree == 1 ? 4 : 64;
        for (int k = 0; k <= samples; ++k) {
            const auto p = bezier(retained, static_cast<long double>(k) / samples);
            close(p, bezier(original, static_cast<long double>(k) / samples)); CHECK(p.x <= 0);
        }
    }
    absent(fragment->getDrwExtData()); CHECK(f::tagsState(fragment->getDrwExtData()) == f::tagsState(expected.foreign));
    CHECK(fragment->getLayer(false) == curve.getLayer(false)); CHECK(fragment->getPen(false) == curve.getPen(false));
    CHECK(fragment->isVisible()); CHECK(fragment->ownerLineage() == 0); CHECK(fragment->sourceHandle() == 0);
    CHECK(fragment->xDictHandle() == 0); CHECK(fragment->reactorHandles().empty()); CHECK(fragment->lineage() != curve.lineage());
    return expected;
}
void closedRow(const Format& format, int degree, bool reverse, std::size_t segments) {
    CAPTURE(format.name, format.dwg, degree, reverse, segments);
    REQUIRE((degree == 1 || degree == 3)); REQUIRE((segments == 128 || segments == 256));
    lc::test::OutputDirectory out("T-files-closed-degree" + std::to_string(degree) + "-" + std::to_string(segments) + "-" +
        std::string(format.dwg ? "dwg-" : "dxf-") + (reverse ? "reverse" : "forward"));
    RS_Graphic source; auto* curve = closedPolygon(source, reverse, segments, degree);
    const NativeBefore before(*curve); const auto expected = trimClosed(source, *curve);
    plainTwice(source, out, format, expected, "trimmed-plain");
    undoRedoFiles(source, out, format, expected, before);
}
}

TEST_CASE("T files remain ordinary across every DXF version and a second save", "[lc-offsetcurve][trim-files][dxf]") {
    for (const auto& format : dxf) for (int degree : {1, 3}) for (bool middle : {false, true}) row(format, degree, middle);
}

#ifdef DWGSUPPORT
TEST_CASE("T files remain ordinary across every DWG writer version and a second save", "[lc-offsetcurve][trim-files][dwg]") {
    for (const auto& format : dwg) for (int degree : {1, 3}) for (bool middle : {false, true}) row(format, degree, middle);
}
#endif

TEST_CASE("T files binary DXF codec outputs cannot recreate a trimmed definition", "[lc-offsetcurve][trim-files][binary]") {
    Setup setup;
    for (const auto& format : {dxf[1], dxf[7]}) for (int degree : {1, 3}) for (bool middle : {false, true}) {
        lc::test::OutputDirectory out("T-files-binary-" + std::string(format.name) + "-" + std::to_string(degree) + (middle ? "-two" : "-one"));
        RS_Graphic source; prepareInput(source, out.file("native.dxf"), degree); const auto expected = trimDrawing(source, middle);
        const auto ascii = out.file("ordinary-ascii.dxf"); REQUIRE(lc::test::exportDrawing(source, ascii, format.app));
        const auto records = inspect(ascii, format); REQUIRE(records.splines.size() == expected.expected.size());
        const auto binary = out.file("ordinary-binary.dxf");
        { dxfRW writer(binary.string().c_str()); BinaryPlain emitter(writer, records.splines); REQUIRE(writer.write(&emitter, format.codec, true)); }
        REQUIRE(lc::test::fileBytes(binary).compare(0, 22, std::string("AutoCAD Binary DXF\r\n\x1a\0", 22)) == 0);
        inspectPrepared(binary, format, expected); RS_Graphic reopened; load(reopened, binary, format); ordinary(reopened, expected, format);
        const auto second = out.file("second-ascii.dxf"); REQUIRE(lc::test::exportDrawing(reopened, second, format.app));
        inspectPrepared(second, format, expected); RS_Graphic again; load(again, second, format); ordinary(again, expected, format);
    }
}

TEST_CASE("T files trim independently authored binary native inputs through the application", "[lc-offsetcurve][trim-files][binary][read]") {
    Setup setup;
    for (const auto& format : {dxf[1], dxf[7]}) for (int degree : {1, 3}) {
        lc::test::OutputDirectory out("T-files-binary-read-" + std::string(format.name) + "-" + std::to_string(degree));
        RS_Graphic source; load(source, fixture("dxf-binary-" + std::string(format.name) + "-native-degree" + std::to_string(degree) + ".dxf"), format);
        const auto expected = trimDrawing(source, false); const auto output = out.file("trimmed.dxf");
        REQUIRE(lc::test::exportDrawing(source, output, format.app)); inspectPrepared(output, format, expected);
        RS_Graphic reopened; load(reopened, output, format); ordinary(reopened, expected, format);
    }
}

#ifdef DWGSUPPORT
TEST_CASE("T files DXF to DWG conversions cover all DWG targets without re-adoption", "[lc-offsetcurve][trim-files][conversion]") {
    Setup setup;
    for (const auto& format : dwg) for (int degree : {1, 3}) for (bool middle : {false, true}) {
        lc::test::OutputDirectory out("T-files-dxf-dwg-" + std::string(format.name) + "-" + std::to_string(degree) + (middle ? "-two" : "-one"));
        RS_Graphic source; prepareInput(source, out.file("native.dxf"), degree); const auto expected = trimDrawing(source, middle);
        REQUIRE(lc::test::exportDrawing(source, out.file("plain.dxf"), dxf.back().app));
        RS_Graphic converted; load(converted, out.file("plain.dxf"), dxf.back());
        REQUIRE(lc::test::exportDrawing(converted, out.file("converted.dwg"), format.app));
        inspectPrepared(out.file("converted.dwg"), format, expected); RS_Graphic reopened;
        load(reopened, out.file("converted.dwg"), format); ordinary(reopened, expected, format);
    }
}

TEST_CASE("T files DWG to every DXF version preserves plain fragments including R12", "[lc-offsetcurve][trim-files][conversion]") {
    Setup setup;
    for (const auto& format : dxf) for (int degree : {1, 3}) for (bool middle : {false, true}) {
        lc::test::OutputDirectory out("T-files-dwg-dxf-" + std::string(format.name) + "-" + std::to_string(degree) + (middle ? "-two" : "-one"));
        RS_Graphic source; prepareInput(source, out.file("native.dxf"), degree); const auto expected = trimDrawing(source, middle);
        REQUIRE(lc::test::exportDrawing(source, out.file("plain.dwg"), dwg.back().app));
        RS_Graphic converted; load(converted, out.file("plain.dwg"), dwg.back());
        REQUIRE(lc::test::exportDrawing(converted, out.file("converted.dxf"), format.app));
        inspectPrepared(out.file("converted.dxf"), format, expected); RS_Graphic reopened;
        load(reopened, out.file("converted.dxf"), format); ordinary(reopened, expected, format);
    }
}

TEST_CASE("T files alternate all modern format versions without recovering native metadata", "[lc-offsetcurve][trim-files][conversion][chain]") {
    Setup setup;
    const std::array<Format, 13> chain{{dxf[1], dwg[0], dxf[2], dwg[1], dxf[3], dwg[2], dxf[4], dwg[3], dxf[5], dwg[4], dxf[6], dwg[5], dxf[7]}};
    for (int degree : {1, 3}) for (bool middle : {false, true}) {
        lc::test::OutputDirectory out("T-files-chain-" + std::to_string(degree) + (middle ? "-two" : "-one"));
        auto source = std::make_unique<RS_Graphic>(); prepareInput(*source, out.file("native.dxf"), degree);
        auto expected = trimDrawing(*source, middle);
        for (std::size_t i = 0; i < chain.size(); ++i) {
            const auto path = out.file(std::to_string(i) + (chain[i].dwg ? ".dwg" : ".dxf"));
            REQUIRE(lc::test::exportDrawing(*source, path, chain[i].app)); inspectPrepared(path, chain[i], expected);
            // R14 has no entity lineweight. Require the defined ByLayer result
            // through all following modern conversions, not the lost width 25.
            if (chain[i].codec < DRW::AC1015) expected.pen.setWidth(RS2::WidthByLayer);
            auto next = std::make_unique<RS_Graphic>(); load(*next, path, chain[i]); ordinary(*next, expected, chain[i]); source = std::move(next);
        }
    }
}

TEST_CASE("T files legacy R13 and R14 native reads demote to DXF and unsupported foreign DWG migration refuses atomically", "[lc-offsetcurve][trim-files][legacy]") {
    Setup setup;
    const auto unsupportedForeignTarget = [](const RS_Graphic& graphic) {
        const auto& metadata = graphic.dwgAdvancedMetadata();
        DwgDataStorageWriterCapability capability;
        REQUIRE(getDwgDataStorageWriterCapability(DwgDataStorageWriterBinding::TableStyle, capability));
        REQUIRE(capability.maxVersion == DRW::AC1021);
        REQUIRE(capability.maxVersion < DRW::AC1032);
        std::size_t requiredUnsupportedStyles = 0;
        for (const auto& style : metadata.tables()) {
            if (!style.nativeTableStyle) continue;
            REQUIRE(style.recordName == style.nativeTableStyle->m_name);
            REQUIRE(style.nativeTableStyle->validateDwgPayload(DRW::AC1021));
            REQUIRE_FALSE(style.nativeTableStyle->validateDwgPayload(DRW::AC1032));
            const auto raw = std::find_if(metadata.rawObjects().begin(), metadata.rawObjects().end(),
                [&](const auto& record) { return record.handle == style.handle; });
            REQUIRE(raw != metadata.rawObjects().end()); REQUIRE_FALSE(raw->rawBytes.empty());
            REQUIRE(raw->version == metadata.sourceDwgVersion());
            REQUIRE(raw->version != DRW::AC1032); REQUIRE(raw->recordName == "TABLESTYLE");
            std::size_t requiredMemberships = 0;
            for (const auto& dictionary : metadata.dwgDictionaryMemberships())
                for (const auto& entry : dictionary.m_entries) if (entry.m_handle == style.handle) {
                    CHECK_FALSE(entry.m_name.empty()); ++requiredMemberships;
                }
            REQUIRE(requiredMemberships > 0); ++requiredUnsupportedStyles;
        }
        REQUIRE(requiredUnsupportedStyles > 0);
        return requiredUnsupportedStyles;
    };
    const auto actualForeignRefusals = [](const RS_Graphic& graphic, const std::string& diagnostics) {
        const auto& metadata = graphic.dwgAdvancedMetadata();
        const std::string prefix = "RS_FilterDXFRW: unresolved DWG DICTIONARY entry target 0x";
        std::set<std::uint32_t> targets;
        std::istringstream lines(diagnostics);
        for (std::string line; std::getline(lines, line);) {
            if (line.compare(0, prefix.size(), prefix) != 0) continue;
            std::istringstream number(line.substr(prefix.size()));
            std::uint64_t value = 0; number >> std::hex >> value;
            REQUIRE_FALSE(number.fail()); number >> std::ws; REQUIRE(number.eof());
            REQUIRE(value > 0); REQUIRE(value <= std::numeric_limits<std::uint32_t>::max());
            const auto handle = static_cast<std::uint32_t>(value);
            CAPTURE(handle, line);
            const auto raw = std::find_if(metadata.rawObjects().begin(), metadata.rawObjects().end(),
                [&](const auto& record) { return record.handle == handle; });
            REQUIRE(raw != metadata.rawObjects().end());
            REQUIRE_FALSE(raw->isEntity); REQUIRE(raw->isCustomClass);
            REQUIRE_FALSE(raw->recordName.empty()); REQUIRE_FALSE(raw->className.empty());
            REQUIRE_FALSE(raw->rawBytes.empty()); REQUIRE(raw->version == metadata.sourceDwgVersion());
            REQUIRE(raw->version != DRW::AC1032);
            std::size_t requiredMemberships = 0;
            for (const auto& membership : metadata.dwgDictionaryMemberships())
                for (const auto& entry : membership.m_entries) if (entry.m_handle == handle) {
                    REQUIRE_FALSE(entry.m_name.empty()); REQUIRE(membership.m_complete);
                    const auto owner = std::find_if(metadata.dictionaries().begin(), metadata.dictionaries().end(),
                        [&](const auto& dictionary) { return dictionary.handle == membership.m_dictionaryHandle; });
                    REQUIRE(owner != metadata.dictionaries().end());
                    REQUIRE(metadata.authorizeDwgDictionarySource(*owner).authorized());
                    ++requiredMemberships;
                }
            REQUIRE(requiredMemberships > 0); targets.insert(handle);
        }
        // This must be the exporter's actual unresolved required edge, not a
        // nearby unsupported TABLESTYLE witness or a generic false return.
        REQUIRE_FALSE(targets.empty());
        return targets;
    };
    const std::array<Format, 2> legacyRows{{
        {"R13", RS2::FormatDWG, DRW::AC1012, true}, {"R14", RS2::FormatDWG, DRW::AC1014, true}}};
    for (const auto& legacy : legacyRows) for (int degree : {1, 3}) {
        CAPTURE(legacy.name, degree);
        lc::test::OutputDirectory out("T-files-legacy-" + std::string(legacy.name) + "-" + std::to_string(degree));
        RS_Graphic source; load(source, fixture("oda-dwg-" + std::string(legacy.name) + "-native-degree" + std::to_string(degree) + ".dwg"), legacy);
        const auto& metadata = source.dwgAdvancedMetadata();
        const auto& coverage = metadata.dwgFrameCoverageReport();
        REQUIRE(coverage.m_status == DRW_DwgFrameCoverageStatus::FinalizedComplete);
        REQUIRE(coverage.m_complete); REQUIRE(metadata.dwgFramePublicationCoverageComplete());
        CHECK(metadata.dwgFramePublicationConflicts().empty());
        REQUIRE_FALSE(metadata.cellStyleMaps().empty());
        for (const auto& map : metadata.cellStyleMaps()) {
            CHECK(map.m_cellStyleCount > 0); CHECK(map.m_incompleteRangeCount == 0);
        }
        for (const auto& dictionary : metadata.dictionaries())
            CHECK(metadata.authorizeDwgDictionarySource(dictionary).authorized());
        const auto rawBefore = metadataState(metadata);
        auto* native = dynamic_cast<LC_OffsetCurve*>(f::sole(source)); REQUIRE(native);
        REQUIRE(native->hasEditableDefinition()); REQUIRE(native->getData().degree == std::size_t(degree));
        const auto& definition = native->definition();
        const double portable = LC_CurveOffset::kEllipseRelativeOffsetTolerance;
        REQUIRE(definition.relTolerance >= .1 * portable); REQUIRE(definition.relTolerance <= 10 * portable);
        const RS_Ellipse basis(nullptr, definition.basis);
        const double readTolerance = LC_CurveOffset::toleranceFor(basis, definition.distance, definition.relTolerance);
        REQUIRE(std::isfinite(readTolerance)); REQUIRE(readTolerance > 0);
        const auto checked = LC_EllipseOffsetGeometry::checkCache(native->getData(), definition, readTolerance);
        CAPTURE(checked.reason, checked.span); REQUIRE(checked);
        const auto nativeFingerprint = [&] {
            std::ostringstream state; cacheState(state, native->getData()); nativeState(state, *native);
            state << f::tagsState(native->getDrwExtData()); return state.str();
        };
        const auto nativeBefore = nativeFingerprint();
        const auto nativePen = native->getPen(false); const auto* nativeLayer = native->getLayer(false);
        const auto checkNative = [&] {
            REQUIRE(f::sole(source) == native); REQUIRE_FALSE(native->isDeleted());
            REQUIRE(native->hasEditableDefinition()); CHECK(nativeFingerprint() == nativeBefore);
            CHECK(native->getPen(false) == nativePen); CHECK(native->getLayer(false) == nativeLayer);
        };
        // A real R2018 control file distinguishes source-graph refusal from an
        // unavailable writer or unwritable target. Never remove foreign data.
        RS_Graphic control;
        control.addEntity(new RS_Line(&control, RS_LineData({0, 0}, {1, 1})));
        const auto existing = out.file("existing.dwg");
        REQUIRE(lc::test::exportDrawing(control, existing, RS2::FormatDWG2018));
        const auto existingBytes = lc::test::fileBytes(existing); REQUIRE_FALSE(existingBytes.empty());
        const auto refuse = [&](const char* state) {
            CAPTURE(state);
            const auto unsupported = unsupportedForeignTarget(source);
            const Snapshot before(source);
            const auto inventory = out.entries();
            const auto absent = out.file("absent.dwg"); REQUIRE_FALSE(std::filesystem::exists(absent));
            std::set<std::uint32_t> observedTargets;
            for (const auto& target : {absent, existing}) {
                RS_FilterDXFRW filter;
                ExportDiagnostics capture; REQUIRE(capture.ready());
                const bool exported = filter.fileExport(source, lc::test::toQString(target), RS2::FormatDWG2018);
                const auto diagnostics = capture.finish(); REQUIRE(capture.complete());
                CAPTURE(diagnostics); REQUIRE_FALSE(exported);
                INFO(filter.dwgExportAdmissionReport().toStdString());
                INFO(filter.exportReport().toStdString());
                const auto actualTargets = actualForeignRefusals(source, diagnostics);
                observedTargets.insert(actualTargets.begin(), actualTargets.end());
                CHECK(out.entries() == inventory); CHECK_FALSE(std::filesystem::exists(absent));
                CHECK(lc::test::fileBytes(existing) == existingBytes); before.check();
            }
            std::cout << "LEGACY_READ_ONLY_REFUSAL source=" << legacy.name << " degree=" << degree
                << " state=" << state << " target=DWGR2018 actualDiagnostic=unresolved-required-foreign-DICTIONARY-target"
                << " observedTargets=" << observedTargets.size() << " separateUnsupportedStyleWitnesses=" << unsupported
                << " absent+existing=atomic source+raw+history=unchanged\n";
        };
        refuse("untrimmed-native");
        const auto expected = trimDrawing(source, false);
        CHECK(metadataState(metadata) == rawBefore); refuse("trimmed-plain");
        { const Snapshot before(source); plainTwice(source, out, dxf.back(), expected, "modern"); before.check(); }
        REQUIRE(source.undo()); checkNative();
        CHECK(metadataState(metadata) == rawBefore); refuse("undo-native");
        REQUIRE(source.redo()); REQUIRE(expected.original->isDeleted()); ordinary(source, expected, dxf.back());
        CHECK(metadataState(metadata) == rawBefore); refuse("redo-plain");
        // Refusal snapshots above also need a functional membership check:
        // the final failed saves must not damage the existing undo cycle.
        REQUIRE(source.undo()); checkNative(); CHECK(metadataState(metadata) == rawBefore);
        REQUIRE(source.redo()); REQUIRE(expected.original->isDeleted()); ordinary(source, expected, dxf.back());
        CHECK(metadataState(metadata) == rawBefore);
        { const Snapshot before(source); plainTwice(source, out, dxf.back(), expected, "redo"); before.check(); }
    }
}
#endif

TEST_CASE("T files undo before save restores the native definition and redo saves only plain fragments", "[lc-offsetcurve][trim-files][undo]") {
    Setup setup;
    for (int degree : {1, 3}) openUndoRow(dxf.back(), degree);
}

TEST_CASE("T files closed C-admitted native trim stays ordinary across DXF R2018 saves and undo redo", "[lc-offsetcurve][trim-files][closed][dxf][undo]") {
    Setup setup;
    for (bool reverse : {false, true}) {
        for (int degree : {1, 3}) for (std::size_t segments : {128u, 256u})
            DYNAMIC_SECTION("degree " << degree << " segments " << segments << " reverse " << reverse)
                closedRow(dxf.back(), degree, reverse, segments);
    }
}

#ifdef DWGSUPPORT
TEST_CASE("T files closed C-admitted native trim stays ordinary across DWG R2018 saves and undo redo", "[lc-offsetcurve][trim-files][closed][dwg][undo]") {
    Setup setup;
    for (bool reverse : {false, true}) {
        for (int degree : {1, 3}) for (std::size_t segments : {128u, 256u})
            DYNAMIC_SECTION("degree " << degree << " segments " << segments << " reverse " << reverse)
                closedRow(dwg.back(), degree, reverse, segments);
    }
}

TEST_CASE("T files DWG undo before save restores both native degrees and redo stays ordinary after second reopen", "[lc-offsetcurve][trim-files][undo][dwg]") {
    Setup setup;
    for (int degree : {1, 3}) openUndoRow(dwg.back(), degree);
}
#endif
