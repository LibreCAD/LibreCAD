/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/


#include "libdwgr.h"
#include <cstdio>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <exception>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>
#include "intern/drw_dbg.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgreader.h"
#include "intern/dwgreaderR1_40.h"
#include "intern/dwgreaderR11.h"
#include "intern/dwgwriter.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "intern/dwgreader15.h"
#include "intern/dwgreader18.h"
#include "intern/dwgreader21.h"
#include "intern/dwgreader24.h"
#include "intern/dwgreader27.h"
#include "intern/dwgreader32.h"
#include "intern/dwg_dxf_output_transaction.h"
#include "intern/dwgsafety.h"
#include "intern/drw_reserve.h"
#include "intern/dwgutil.h"

#define FIRSTHANDLE 48

namespace {

using Binding = DwgDataStorageWriterBinding;
using Capability = DwgDataStorageWriterCapability;
using Operation = DwgDataStorageWriterOperation;

// Keep this table next to the DWG write orchestration.  It is the single
// executable description used by the filter before it advertises a modern
// DataStorage presence bit; class ordinals remain writer-local when needed.
const Capability kDwgDataStorageWriterCapabilities[] = {
    {Binding::MLeaderStyle, Operation::WriteMLeaderStyle,
     "MLEADERSTYLE", "AcDbMLeaderStyle",
     "MLEADERSTYLE", DRW_MLeaderStyle::kDwgClassNum, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::TableStyle, Operation::WriteTableStyle,
     "TABLESTYLE", "AcDbTableStyle", "TABLESTYLE",
     DRW_TableStyle::kDwgClassNum, 0, DRW::AC1015, DRW::AC1021,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::Material, Operation::WriteMaterial,
     "MATERIAL", "AcDbMaterial", "MATERIAL",
     DRW_Material::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::LightList, Operation::WriteLightList,
     "LIGHTLIST", "AcDbLightList", "LIGHTLIST", 0, 0,
     DRW::AC1015, DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true,
     true},
    {Binding::Group, Operation::WriteGroup,
     "GROUP", "", "GROUP", 0, dwgObjType::GROUP, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, false, true},
    {Binding::Dictionary, Operation::WriteDictionary,
     "DICTIONARY", "", "DICTIONARY", 0, dwgObjType::DICTIONARY,
     DRW::AC1015, DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, false,
     true},
    {Binding::XRecord, Operation::WriteXRecord,
     "XRECORD", "", "XRECORD", 0, dwgObjType::XRECORD, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, false, true},
    {Binding::Layout, Operation::WriteLayout,
     "LAYOUT", "", "LAYOUT", 0, dwgObjType::LAYOUT, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, false, true},
    {Binding::MLineStyle, Operation::WriteMLineStyle,
     "MLINESTYLE", "", "MLINESTYLE", 0, dwgObjType::MLINESTYLE,
     DRW::AC1015, DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, false,
     true},
    {Binding::AcDbPlaceholder, Operation::WriteAcDbPlaceholder,
     "ACDBPLACEHOLDER", "", "ACDBPLACEHOLDER", 0,
     dwgObjType::ACDBPLACEHOLDER, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, false, true},
    {Binding::RasterVariables, Operation::WriteRasterVariables,
     "RASTERVARIABLES", "AcDbRasterVariables",
     "RASTERVARIABLES", DRW_RasterVariables::kDwgClassNum, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::WipeoutVariables, Operation::WriteWipeoutVariables,
     "WIPEOUTVARIABLES", "AcDbWipeoutVariables",
     "WIPEOUTVARIABLES", DRW_WipeoutVariables::kDwgClassNum, 0,
     DRW::AC1015, DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true,
     true},
    {Binding::GeoData, Operation::WriteGeoData,
     "GEODATA", "AcDbGeoData", "GEODATA",
     DRW_GeoData::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::SpatialFilter, Operation::WriteSpatialFilter,
     "SPATIAL_FILTER", "AcDbSpatialFilter",
     "SPATIAL_FILTER", DRW_SpatialFilter::kDwgClassNum, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::Scale, Operation::WriteScale,
     "SCALE", "AcDbScale", "SCALE", DRW_Scale::kDwgClassNum,
     0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::IDBuffer, Operation::WriteIDBuffer,
     "IDBUFFER", "AcDbIdBuffer", "IDBUFFER",
     DRW_IDBuffer::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::LayerIndex, Operation::WriteLayerIndex,
     "LAYER_INDEX", "AcDbLayerIndex", "LAYER_INDEX",
     DRW_LayerIndex::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::SpatialIndex, Operation::WriteSpatialIndex,
     "SPATIAL_INDEX", "AcDbSpatialIndex",
     "SPATIAL_INDEX", DRW_SpatialIndex::kDwgClassNum, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::DictionaryVar, Operation::WriteDictionaryVar,
     "DICTIONARYVAR", "AcDbDictionaryVar",
     "DICTIONARYVAR", DRW_DictionaryVar::kDwgClassNum, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::DictionaryWithDefault, Operation::WriteDictionaryWithDefault,
     "ACDBDICTIONARYWDFLT",
     "AcDbDictionaryWithDefault", "ACDBDICTIONARYWDFLT",
     DRW_DictionaryWithDefault::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::SortEntsTable, Operation::WriteSortEntsTable,
     "SORTENTSTABLE", "AcDbSortentsTable",
     "SORTENTSTABLE", DRW_SortEntsTable::kDwgClassNum, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::FieldList, Operation::WriteFieldList,
     "FIELDLIST", "AcDbFieldList", "FIELDLIST",
     DRW_FieldList::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::Field, Operation::WriteField,
     "FIELD", "AcDbField", "FIELD", DRW_Field::kDwgClassNum,
     0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::UnderlayPdf, Operation::WriteUnderlayDefinition,
     "UNDERLAYDEFINITION", "AcDbPdfDefinition",
     "PDFDEFINITION", DRW_UnderlayDefinition::kDwgClassNumPdf, 0,
     DRW::AC1015, DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true,
     true},
    {Binding::UnderlayDgn, Operation::WriteUnderlayDefinition,
     "UNDERLAYDEFINITION", "AcDbDgnDefinition",
     "DGNDEFINITION", DRW_UnderlayDefinition::kDwgClassNumDgn, 0,
     DRW::AC1015, DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true,
     true},
    {Binding::UnderlayDwf, Operation::WriteUnderlayDefinition,
     "UNDERLAYDEFINITION", "AcDbDwfDefinition",
     "DWFDEFINITION", DRW_UnderlayDefinition::kDwgClassNumDwf, 0,
     DRW::AC1015, DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true,
     true},
    {Binding::PointCloudDefinition, Operation::WritePointCloudDef,
     "POINTCLOUDDEFINITION",
     "AcDbPointCloudDef", "POINTCLOUDDEFINITION",
     DRW_PointCloudDef::kDwgClassNumDefinition, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::PointCloudDefinitionEx, Operation::WritePointCloudDef,
     "POINTCLOUDDEFINITION",
     "AcDbPointCloudDefEx", "POINTCLOUDDEFINITIONEX",
     DRW_PointCloudDef::kDwgClassNumDefinitionEx, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::PointCloudDefinitionReactor, Operation::WritePointCloudDef,
     "POINTCLOUDDEFINITION",
     "AcDbPointCloudDefReactor", "POINTCLOUDDEFREACTOR",
     DRW_PointCloudDef::kDwgClassNumReactor, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::PointCloudDefinitionReactorEx, Operation::WritePointCloudDef,
     "POINTCLOUDDEFINITION",
     "AcDbPointCloudDefReactorEx", "POINTCLOUDDEFREACTOREX",
     DRW_PointCloudDef::kDwgClassNumReactorEx, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::NavisworksModelDefinition, Operation::WriteNavisworksModelDef,
     "NAVISWORKSMODELDEF",
     "AcDbNavisworksModelDef", "NAVISWORKSMODELDEF",
     DRW_NavisworksModelDef::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::PointCloudColorMap, Operation::WritePointCloudColorMap,
     "POINTCLOUDCOLORMAP",
     "AcDbPointCloudColorMap", "POINTCLOUDCOLORMAP",
     DRW_PointCloudColorMap::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::DbColor, Operation::WriteDbColor,
     "DBCOLOR", "AcDbColor", "DBCOLOR",
     DRW_DbColor::kDwgClassNum, 0, DRW::AC1018, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::DimensionAssociation, Operation::WriteDimensionAssociation,
     "DIMASSOC", "AcDbDimAssoc", "DIMASSOC",
     DRW_DimensionAssociation::kDwgClassNum, 0, DRW::AC1021,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::EvaluationGraph, Operation::WriteEvaluationGraph,
     "ACAD_EVALUATION_GRAPH", "AcDbEvalGraph",
     "ACAD_EVALUATION_GRAPH", DRW_EvaluationGraph::kDwgClassNum, 0,
     DRW::AC1021, DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true,
     true},
    {Binding::TvDeviceProperties, Operation::WriteTvDeviceProperties,
     "TVDEVICEPROPERTIES",
     "AcDbTvDeviceProperties", "TVDEVICEPROPERTIES",
     DRW_TvDeviceProperties::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::VxControl, Operation::WriteVxControl,
     "VXCONTROL", "AcDbVxControl", "VXCONTROL",
     DRW_VxControl::kDwgClassNum, 0, DRW::AC1015, DRW::UNKNOWNV,
     DwgDataStorageWriterPhase::Objects, true, true},
    {Binding::VxTableRecord, Operation::WriteVxTableRecord,
     "VXTABLERECORD", "AcDbVxTableRecord",
     "VXTABLERECORD", DRW_VxTableRecord::kDwgClassNum, 0, DRW::AC1015,
     DRW::UNKNOWNV, DwgDataStorageWriterPhase::Objects, true, true},
};

const Capability* findDwgDataStorageWriterCapability(
    DwgDataStorageWriterBinding binding) {
    if (binding == Binding::None)
        return nullptr;
    for (const Capability& capability : kDwgDataStorageWriterCapabilities) {
        if (capability.binding == binding)
            return &capability;
    }
    return nullptr;
}

bool sameDwgDataStorageText(const char* left, const char* right) {
    return left != nullptr && right != nullptr && std::strcmp(left, right) == 0;
}

} // namespace

bool getDwgDataStorageWriterCapability(
    DwgDataStorageWriterBinding binding,
    DwgDataStorageWriterCapability& capability) {
    const Capability* found = findDwgDataStorageWriterCapability(binding);
    if (found == nullptr)
        return false;
    capability = *found;
    return true;
}

bool getDwgDataStorageWriterCapabilities(
    std::vector<DwgDataStorageWriterCapability>& capabilities) {
    capabilities.assign(std::begin(kDwgDataStorageWriterCapabilities),
                        std::end(kDwgDataStorageWriterCapabilities));
    return true;
}

DwgDataStorageWriterBinding findDwgDataStorageWriterBinding(
    const char* family, const char* className, const char* recordName) {
    if (family == nullptr || className == nullptr || recordName == nullptr)
        return Binding::None;
    for (const Capability& capability : kDwgDataStorageWriterCapabilities) {
        if (sameDwgDataStorageText(capability.family, family)
            && sameDwgDataStorageText(capability.className, className)
            && sameDwgDataStorageText(capability.recordName, recordName)) {
            return capability.binding;
        }
    }
    return Binding::None;
}

bool dwgRW::canWriteDwgDataStorageBinding(
    DwgDataStorageWriterBinding binding) const {
    DwgDataStorageWriterCapability capability;
    if (!getDwgDataStorageWriterCapability(binding, capability)
        || capability.operation
               == DwgDataStorageWriterOperation::None
        || !capability.acceptsPresenceBit
        || capability.phase != DwgDataStorageWriterPhase::Objects) {
        return false;
    }
    if (capability.minVersion != DRW::UNKNOWNV
        && version < capability.minVersion) {
        return false;
    }
    if (capability.maxVersion != DRW::UNKNOWNV
        && version > capability.maxVersion) {
        return false;
    }
    if (writer != nullptr
        && !writer->canWriteDwgDataStorageOperation(capability.operation)) {
        return false;
    }
    return true;
}

struct dwgRW::DwgEntityWriteState {
    DRW_Entity *entity {nullptr};
    std::uint32_t handle {DRW::NoHandle};
    std::uint32_t parentHandle {DRW::NoHandle};
    DRW::Space space {DRW::ModelSpace};
    std::uint8_t haveNextLinks {0};
    std::uint8_t hasDsData {0};
    std::uint8_t plotFlags {0};
    std::uint8_t ltFlags {0};
    std::uint8_t materialFlag {0};
    std::uint8_t shadowFlag {0};
    std::uint8_t hasFullVisualStyle {0};
    std::uint8_t hasFaceVisualStyle {0};
    std::uint8_t hasEdgeVisualStyle {0};
    bool hasAcDbColorH {false};
    std::uint32_t acDbColorHandle {0};
    dwgHandle lTypeH;
    dwgHandle layerH;
    std::uint32_t nextEntLink {0};
    std::uint32_t prevEntLink {0};
    bool ownerHandle {false};
    std::uint8_t xDictFlag {0};
    std::int32_t numReactors {0};
    std::uint32_t objSize {0};
    std::uint64_t dwgDataEndBit {0};
    std::int16_t oType {0};
    std::vector<std::pair<std::size_t, std::uint32_t>> eedAppIdRefs;
    std::vector<std::pair<std::size_t, std::uint32_t>> eedLayerRefs;
    std::uint16_t eedCodePage {30};

    bool hasInsertState {false};
    dwgHandle insertBlockRecordHandle;
    dwgHandle insertSeqEndHandle;
    std::vector<dwgHandle> insertAttributeHandles;

    bool hasPolylineState {false};
    dwgHandle polylineSeqEndHandle;

    bool hasVertexState {false};
    DRW_Vertex::DwgSubtype vertexSubtype {DRW_Vertex::DwgSubtype::Auto};

    bool hasMLineState {false};
    std::uint32_t mlineStyleHandle {0};

    // IMAGE writes stage a local copy and publish it to the caller only after
    // all auxiliary frames succeed. Keep a complete copy for later rollback.
    std::shared_ptr<DRW_Image> imageSnapshot;
};

struct dwgRW::DwgTableWriteState {
    DRW_TableEntry *entry {nullptr};
    std::uint32_t handle {0};
    std::uint32_t parentHandle {0};
    int flags {0};
    std::vector<std::uint32_t> reactorHandles;
    std::uint32_t xDictHandle {0};
    std::vector<DRW_TableEntry::PendingHandleRef> pendingAppIdResolutions;
    std::vector<DRW_TableEntry::PendingHandleRef> pendingLayerRefResolutions;
    std::uint8_t xDictFlag {0};
    std::int32_t numReactors {0};
    std::uint8_t hasDsData {0};
    std::int16_t oType {0};
    std::uint32_t objSize {0};
    std::uint32_t handleStreamBitSize {0};
};

struct dwgRW::DwgWriteTransaction {
    dwgWriter::CompoundWriteCheckpoint checkpoint;
    std::uint64_t token {0};
    std::size_t successfulEntityWrites {0};
    WriteSkipCounters writeSkipCounters;
    bool requiredWriteFailure {false};
    bool suppressFailureCounters {false};
    bool trackCallerMutations {false};
    std::vector<DwgEntityWriteState> entityMutations;
    std::vector<DwgTableWriteState> tableMutations;
};

/*enum sections {
    secUnknown,
    secHeader,
    secTables,
    secBlocks,
    secEntities,
    secObjects
};*/

dwgRW::dwgRW(const char* name)
    : fileName{ name }
{
    DRW_DBGSL(DRW_dbg::Level::None);
}

dwgRW::~dwgRW() = default;

dwgRW::DwgEntityWriteState dwgRW::captureEntityWriteState(
    DRW_Entity *entity) const {
    DwgEntityWriteState state;
    state.entity = entity;
    if (entity == nullptr)
        return state;

    state.handle = entity->handle;
    state.parentHandle = entity->parentHandle;
    state.space = entity->space;
    state.haveNextLinks = entity->haveNextLinks;
    state.hasDsData = entity->hasDsData;
    state.plotFlags = entity->plotFlags;
    state.ltFlags = entity->ltFlags;
    state.materialFlag = entity->materialFlag;
    state.shadowFlag = entity->shadowFlag;
    state.hasFullVisualStyle = entity->hasFullVisualStyle;
    state.hasFaceVisualStyle = entity->hasFaceVisualStyle;
    state.hasEdgeVisualStyle = entity->hasEdgeVisualStyle;
    state.hasAcDbColorH = entity->hasAcDbColorH;
    state.acDbColorHandle = entity->acDbColorHandle;
    state.lTypeH = entity->lTypeH;
    state.layerH = entity->layerH;
    state.nextEntLink = entity->nextEntLink;
    state.prevEntLink = entity->prevEntLink;
    state.ownerHandle = entity->ownerHandle;
    state.xDictFlag = entity->xDictFlag;
    state.numReactors = entity->numReactors;
    state.objSize = entity->objSize;
    state.dwgDataEndBit = entity->dwgDataEndBit;
    state.oType = entity->oType;
    state.eedAppIdRefs.reserve(entity->dwgEedAppIdWriteRefs.size());
    for (const auto& ref : entity->dwgEedAppIdWriteRefs)
        state.eedAppIdRefs.emplace_back(ref.indexInExtData, ref.handleRef);
    state.eedLayerRefs.reserve(entity->dwgEedLayerWriteRefs.size());
    for (const auto& ref : entity->dwgEedLayerWriteRefs)
        state.eedLayerRefs.emplace_back(ref.indexInExtData, ref.handleRef);
    state.eedCodePage = entity->dwgEedCodePage;

    if (auto *insert = dynamic_cast<DRW_Insert *>(entity)) {
        state.hasInsertState = true;
        state.insertBlockRecordHandle = insert->blockRecH;
        state.insertSeqEndHandle = insert->seqendH;
        state.insertAttributeHandles = insert->attribHandles;
    }
    if (auto *polyline = dynamic_cast<DRW_Polyline *>(entity)) {
        state.hasPolylineState = true;
        state.polylineSeqEndHandle = polyline->seqEndH;
    }
    if (auto *vertex = dynamic_cast<DRW_Vertex *>(entity)) {
        state.hasVertexState = true;
        state.vertexSubtype = vertex->m_dwgSubtype;
    }
    if (auto *mline = dynamic_cast<DRW_MLine *>(entity)) {
        state.hasMLineState = true;
        state.mlineStyleHandle = mline->styleHandle;
    }
    if (auto *image = dynamic_cast<DRW_Image *>(entity))
        state.imageSnapshot = std::make_shared<DRW_Image>(*image);
    return state;
}

dwgRW::DwgTableWriteState dwgRW::captureTableWriteState(
    DRW_TableEntry *entry) const {
    DwgTableWriteState state;
    state.entry = entry;
    if (entry != nullptr) {
        state.handle = entry->handle;
        state.parentHandle = entry->parentHandle;
        state.flags = entry->flags;
        state.reactorHandles = entry->reactorHandles;
        state.xDictHandle = entry->xDictHandle;
        state.pendingAppIdResolutions = entry->pendingAppIdResolutions;
        state.pendingLayerRefResolutions = entry->pendingLayerRefResolutions;
        state.xDictFlag = entry->xDictFlag;
        state.numReactors = entry->numReactors;
        state.hasDsData = entry->hasDsData;
        state.oType = entry->oType;
        state.objSize = entry->objSize;
        state.handleStreamBitSize = entry->handleStreamBitSize;
    }
    return state;
}

void dwgRW::restoreEntityWriteState(const DwgEntityWriteState& state) {
    DRW_Entity *entity = state.entity;
    if (entity == nullptr)
        return;

    if (state.imageSnapshot) {
        if (auto *image = dynamic_cast<DRW_Image *>(entity)) {
            *image = *state.imageSnapshot;
            return;
        }
    }

    entity->handle = state.handle;
    entity->parentHandle = state.parentHandle;
    entity->space = state.space;
    entity->haveNextLinks = state.haveNextLinks;
    entity->hasDsData = state.hasDsData;
    entity->plotFlags = state.plotFlags;
    entity->ltFlags = state.ltFlags;
    entity->materialFlag = state.materialFlag;
    entity->shadowFlag = state.shadowFlag;
    entity->hasFullVisualStyle = state.hasFullVisualStyle;
    entity->hasFaceVisualStyle = state.hasFaceVisualStyle;
    entity->hasEdgeVisualStyle = state.hasEdgeVisualStyle;
    entity->hasAcDbColorH = state.hasAcDbColorH;
    entity->acDbColorHandle = state.acDbColorHandle;
    entity->lTypeH = state.lTypeH;
    entity->layerH = state.layerH;
    entity->nextEntLink = state.nextEntLink;
    entity->prevEntLink = state.prevEntLink;
    entity->ownerHandle = state.ownerHandle;
    entity->xDictFlag = state.xDictFlag;
    entity->numReactors = state.numReactors;
    entity->objSize = state.objSize;
    entity->dwgDataEndBit = state.dwgDataEndBit;
    entity->oType = state.oType;
    entity->dwgEedAppIdWriteRefs.clear();
    for (const auto& ref : state.eedAppIdRefs)
        entity->dwgEedAppIdWriteRefs.push_back({ref.first, ref.second});
    entity->dwgEedLayerWriteRefs.clear();
    for (const auto& ref : state.eedLayerRefs)
        entity->dwgEedLayerWriteRefs.push_back({ref.first, ref.second});
    entity->dwgEedCodePage = state.eedCodePage;

    if (state.hasInsertState) {
        if (auto *insert = dynamic_cast<DRW_Insert *>(entity)) {
            insert->blockRecH = state.insertBlockRecordHandle;
            insert->seqendH = state.insertSeqEndHandle;
            insert->attribHandles = state.insertAttributeHandles;
        }
    }
    if (state.hasPolylineState) {
        if (auto *polyline = dynamic_cast<DRW_Polyline *>(entity))
            polyline->seqEndH = state.polylineSeqEndHandle;
    }
    if (state.hasVertexState) {
        if (auto *vertex = dynamic_cast<DRW_Vertex *>(entity))
            vertex->m_dwgSubtype = state.vertexSubtype;
    }
    if (state.hasMLineState) {
        if (auto *mline = dynamic_cast<DRW_MLine *>(entity))
            mline->styleHandle = state.mlineStyleHandle;
    }
}

void dwgRW::restoreTableWriteState(const DwgTableWriteState& state) {
    if (state.entry == nullptr)
        return;
    state.entry->handle = state.handle;
    state.entry->parentHandle = state.parentHandle;
    state.entry->flags = state.flags;
    state.entry->reactorHandles = state.reactorHandles;
    state.entry->xDictHandle = state.xDictHandle;
    state.entry->pendingAppIdResolutions = state.pendingAppIdResolutions;
    state.entry->pendingLayerRefResolutions = state.pendingLayerRefResolutions;
    state.entry->xDictFlag = state.xDictFlag;
    state.entry->numReactors = state.numReactors;
    state.entry->hasDsData = state.hasDsData;
    state.entry->oType = state.oType;
    state.entry->objSize = state.objSize;
    state.entry->handleStreamBitSize = state.handleStreamBitSize;
}

void dwgRW::restoreEntityWriteStates(
    const std::vector<DwgEntityWriteState>& states) {
    for (auto it = states.crbegin(); it != states.crend(); ++it)
        restoreEntityWriteState(*it);
}

void dwgRW::restoreTableWriteStates(
    const std::vector<DwgTableWriteState>& states) {
    for (auto it = states.crbegin(); it != states.crend(); ++it)
        restoreTableWriteState(*it);
}

void dwgRW::rememberCallerMutations(
    const std::vector<DwgEntityWriteState>& states) {
    if (states.empty() || m_dwgWriteTransactions.empty()
        || !m_dwgWriteTransactions.back()->trackCallerMutations)
        return;
    auto& mutations = m_dwgWriteTransactions.back()->entityMutations;
    mutations.insert(mutations.end(), states.cbegin(), states.cend());
}

void dwgRW::rememberCallerMutation(const DwgTableWriteState& state) {
    if (state.entry == nullptr || m_dwgWriteTransactions.empty()
        || !m_dwgWriteTransactions.back()->trackCallerMutations)
        return;
    m_dwgWriteTransactions.back()->tableMutations.push_back(state);
}

void dwgRW::forgetCallerMutations(
    const std::vector<DwgEntityWriteState>& states) {
    if (states.empty() || m_dwgWriteTransactions.empty()
        || !m_dwgWriteTransactions.back()->trackCallerMutations)
        return;
    auto& mutations = m_dwgWriteTransactions.back()->entityMutations;
    for (auto state = states.crbegin(); state != states.crend(); ++state) {
        const auto it = std::find_if(
            mutations.rbegin(), mutations.rend(),
            [&state](const DwgEntityWriteState& candidate) {
                return candidate.entity == state->entity;
            });
        if (it != mutations.rend())
            mutations.erase(std::next(it).base());
    }
}

void dwgRW::setDebug(DRW::DebugLevel lvl){
    switch (lvl){
    case DRW::DebugLevel::Debug:
        DRW_DBGSL(DRW_dbg::Level::Debug);
        break;
    case DRW::DebugLevel::None:
        DRW_DBGSL(DRW_dbg::Level::None);
    }
}

/*reads metadata and loads image preview*/
bool dwgRW::getPreview(){
    bool isOk = false;
    error = DRW::BAD_NONE;

    std::ifstream filestr;
    isOk = openFile(&filestr);
    if (!isOk)
        return false;

    isOk = reader->readMetaData();
    if (isOk) {
        isOk = reader->readPreview();
    } else
        error = DRW::BAD_READ_METADATA;

    filestr.close();
    if (reader) {
        reader.reset();
    }
    return isOk;
}

bool dwgRW::testReader(){
    bool isOk = false;

    std::ifstream filestr;
    filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
    if (!filestr.is_open() || !filestr.good() ){
        error = DRW::BAD_OPEN;
        return isOk;
    }

    dwgBuffer fileBuf(&filestr);
    if (fileBuf.size() > static_cast<std::uint64_t>(
            std::numeric_limits<int>::max())) {
        filestr.close();
        return false;
    }
    std::vector<std::uint8_t> tmpStrData;
    if (!DRW::resize(tmpStrData, static_cast<int>(fileBuf.size()))
        || !fileBuf.getBytes(tmpStrData.data(), fileBuf.size())) {
        filestr.close();
        return false;
    }
    dwgBuffer dataBuf(tmpStrData.data(), fileBuf.size());
    fileBuf.setPosition(0);
    DRW_DBG("\ndwgRW::testReader filebuf size: ");DRW_DBG(fileBuf.size());
    DRW_DBG("\ndwgRW::testReader dataBuf size: ");DRW_DBG(dataBuf.size());
    DRW_DBG("\n filebuf pos: ");DRW_DBG(fileBuf.getPosition());
    DRW_DBG("\n dataBuf pos: ");DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\n filebuf bitpos: ");DRW_DBG(fileBuf.getBitPos());
    DRW_DBG("\n dataBuf bitpos: ");DRW_DBG(dataBuf.getBitPos());
    DRW_DBG("\n filebuf first byte : ");DRW_DBGH(fileBuf.getRawChar8());
    DRW_DBG("\n dataBuf  first byte : ");DRW_DBGH(dataBuf.getRawChar8());
    fileBuf.setBitPos(4);
    dataBuf.setBitPos(4);
    DRW_DBG("\n filebuf first byte : ");DRW_DBGH(fileBuf.getRawChar8());
    DRW_DBG("\n dataBuf  first byte : ");DRW_DBGH(dataBuf.getRawChar8());
    DRW_DBG("\n filebuf pos: ");DRW_DBG(fileBuf.getPosition());
    DRW_DBG("\n dataBuf pos: ");DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\n filebuf bitpos: ");DRW_DBG(fileBuf.getBitPos());
    DRW_DBG("\n dataBuf bitpos: ");DRW_DBG(dataBuf.getBitPos());
    fileBuf.setBitPos(6);
    dataBuf.setBitPos(6);
    DRW_DBG("\n filebuf pos: ");DRW_DBG(fileBuf.getPosition());
    DRW_DBG("\n dataBuf pos: ");DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\n filebuf bitpos: ");DRW_DBG(fileBuf.getBitPos());
    DRW_DBG("\n dataBuf bitpos: ");DRW_DBG(dataBuf.getBitPos());
    DRW_DBG("\n filebuf first byte : ");DRW_DBGH(fileBuf.getRawChar8());
    DRW_DBG("\n dataBuf  first byte : ");DRW_DBGH(dataBuf.getRawChar8());
    fileBuf.setBitPos(0);
    dataBuf.setBitPos(0);
    DRW_DBG("\n filebuf first byte : ");DRW_DBGH(fileBuf.getRawChar8());
    DRW_DBG("\n dataBuf  first byte : ");DRW_DBGH(dataBuf.getRawChar8());
    DRW_DBG("\n filebuf pos: ");DRW_DBG(fileBuf.getPosition());
    DRW_DBG("\n dataBuf pos: ");DRW_DBG(dataBuf.getPosition());
    DRW_DBG("\n filebuf bitpos: ");DRW_DBG(fileBuf.getBitPos());
    DRW_DBG("\n dataBuf bitpos: ");DRW_DBG(dataBuf.getBitPos());

    filestr.close();
    DRW_DBG("\n\n");
    return isOk;
}

/*start reading dwg file header and, if can read it, continue reading all*/
bool dwgRW::read(DRW_Interface *interface_, bool ext){
    bool isOk = false;
    error = DRW::BAD_NONE;
    applyExt = ext;
    iface = interface_;
    resetReadDiagnostics();

//testReader();return false;

    std::ifstream filestr;
    isOk = openFile(&filestr);
    if (!isOk)
        return false;

    isOk = readInstalledReader();
    filestr.close();

    return isOk;
}

bool dwgRW::readBuffer(const std::uint8_t *data, std::uint64_t size,
                       DRW_Interface *interface_, bool ext) {
    error = DRW::BAD_NONE;
    applyExt = ext;
    iface = interface_;
    resetReadDiagnostics();

    if (data == nullptr || size < 6) {
        error = DRW::BAD_OPEN;
        return false;
    }

    auto buffer = std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(data), size);
    if (!openBuffer(std::move(buffer)))
        return false;

    return readInstalledReader();
}

bool dwgRW::readInstalledReader() {
    if (!reader) {
        error = DRW::BAD_OPEN;
        return false;
    }

    bool isOk = reader->readMetaData();
    if (isOk) {
        isOk = reader->readFileHeader();
        if (isOk) {
            isOk = processDwg();
        } else {
            error = DRW::BAD_READ_FILE_HEADER;
        }
    } else {
        error = DRW::BAD_READ_METADATA;
    }

    captureReaderDiagnostics();
    reader.reset();
    return isOk;
}

void dwgRW::captureReaderDiagnostics() {
    if (!reader)
        return;

    // Capture per-entity failure count + skipped custom-class breakdown before
    // destroying the reader so the public getters (post-read) can still surface
    // them.
    m_entityParseFailures = reader->m_entityParseFailures;
    m_objectParseFailures = reader->m_objectParseFailures;
    m_classesCrcMismatch = reader->m_classesCrcMismatch;
    m_r2007CrcMismatch = reader->m_r2007CrcMismatch;
    m_r2004CrcMismatch = reader->m_r2004CrcMismatch;
    // The reader is discarded immediately after capture. Move the optional
    // report so diagnostic allocation cannot turn a completed parse into an
    // exception during publication.
    m_integrityDiagnostics = std::move(reader->m_integrityDiagnostics);
    m_integrityDiagnosticsDropped = reader->m_integrityDiagnosticsDropped;
    m_dataStorageLinkFailures = reader->m_dataStorageLinkFailures;
    m_dataStorageOrphanRecords = reader->m_dataStorageOrphanRecords;
    m_skippedCustomClasses = reader->m_skippedCustomClasses;
    m_skippedUnsupportedObjects = reader->m_skippedUnsupportedObjects;
    m_decodedProxyPrimitives = reader->m_decodedProxyPrimitives;
    m_layerNameOrder = reader->m_layerNameOrder;
    m_ltypeNameOrder = reader->m_ltypeNameOrder;
    codePage = reader->getCodePage();
}

void dwgRW::resetReadDiagnostics() {
    m_entityParseFailures = 0;
    m_objectParseFailures = 0;
    m_classesCrcMismatch = 0;
    m_r2007CrcMismatch = 0;
    m_r2004CrcMismatch = 0;
    m_integrityDiagnostics.clear();
    m_integrityDiagnosticsDropped = 0;
    m_dataStorageLinkFailures = 0;
    m_dataStorageOrphanRecords = 0;
    m_skippedCustomClasses.clear();
    m_skippedUnsupportedObjects.clear();
    m_decodedProxyPrimitives = 0;
    m_layerNameOrder.clear();
    m_ltypeNameOrder.clear();
}

/**
 * Factory method which creates a reader for the specified DWG version.
 *
 * \returns nullptr if version is not supported.
*/
size_t dwgRW::getEntityParseFailures() const {
    // Prefer the dwgRW-side cache (survives reader.reset() at end of
    // read()). Fall back to live reader for the unusual case of a
    // caller querying mid-read.
    return reader ? reader->m_entityParseFailures : m_entityParseFailures;
}

size_t dwgRW::getObjectParseFailures() const {
    // Mirrors getEntityParseFailures: prefer the dwgRW-side cache (survives
    // reader.reset()), fall back to the live reader for a mid-read query.
    return reader ? reader->m_objectParseFailures : m_objectParseFailures;
}

size_t dwgRW::getClassesCrcMismatch() const {
    // Non-fatal CLASSES CRC mismatch count (warn-only). Same cache-then-live
    // pattern as the parse-failure getters.
    return reader ? reader->m_classesCrcMismatch : m_classesCrcMismatch;
}

size_t dwgRW::getR2007CrcMismatch() const {
    return reader ? reader->m_r2007CrcMismatch : m_r2007CrcMismatch;
}

size_t dwgRW::getR2004CrcMismatch() const {
    return reader ? reader->m_r2004CrcMismatch : m_r2004CrcMismatch;
}

size_t dwgRW::getDataStorageLinkFailures() const {
    return reader ? reader->m_dataStorageLinkFailures : m_dataStorageLinkFailures;
}

size_t dwgRW::getDataStorageOrphanRecords() const {
    return reader ? reader->m_dataStorageOrphanRecords : m_dataStorageOrphanRecords;
}

std::vector<DwgIntegrityDiagnostic> dwgRW::getIntegrityDiagnostics() const {
    return reader ? reader->m_integrityDiagnostics : m_integrityDiagnostics;
}

std::size_t dwgRW::getIntegrityDiagnosticsDropped() const {
    return reader ? reader->m_integrityDiagnosticsDropped
                  : m_integrityDiagnosticsDropped;
}

std::unordered_map<std::string, size_t> dwgRW::getSkippedCustomClasses() const {
    return reader ? reader->m_skippedCustomClasses : m_skippedCustomClasses;
}

std::unordered_map<std::string, size_t> dwgRW::getSkippedUnsupportedObjects() const {
    return reader ? reader->m_skippedUnsupportedObjects : m_skippedUnsupportedObjects;
}

void dwgRW::resetWriteSkipCounters() {
    m_writeSkipCounters = {};
    m_requiredWriteFailure = false;
    m_successfulEntityWrites = 0;
    m_lastSuccessfulEntityHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_lastDwgCompoundEntityWriteReceipt = {};
    m_lastWriteTransaction.reset();
    m_dwgWriteTransactions.clear();
}

std::uint32_t dwgRW::allocNextHandle() {
    return writer != nullptr ? writer->allocNextHandle() : 0;
}

bool dwgRW::getEmittedDwgTableRecordHandles(
    std::vector<std::uint32_t>& handles) const {
    handles.clear();
    return writer != nullptr
        && writer->getEmittedDwgTableRecordHandles(handles);
}

bool dwgRW::getEmittedDwgTableControlHandles(
    std::vector<std::uint32_t>& handles) const {
    handles.clear();
    return writer != nullptr
        && writer->getEmittedDwgTableControlHandles(handles);
}

bool dwgRW::registerDwgNamedObjectDictionaryEntry(
    const std::string& name, std::uint32_t childHandle) {
    const bool accepted = writer != nullptr
        && writer->registerDwgNamedObjectDictionaryEntry(name, childHandle);
    if (!accepted)
        markDwgWriteFailure();
    return accepted;
}

bool dwgRW::getEmittedDwgNamedObjectDictionaryEntries(
    std::vector<DRW::DwgNamedObjectDictionaryEntry>& entries) const {
    entries.clear();
    return writer != nullptr
        && writer->getEmittedDwgNamedObjectDictionaryEntries(entries);
}

bool dwgRW::getEmittedDwgBlockWriteResult(
    DRW::DwgBlockWriteResult& result) const {
    result = {};
    return writer != nullptr
        && writer->getEmittedDwgBlockWriteResult(result);
}

bool dwgRW::getLastDwgObjectHandleOccurrences(
    std::vector<DRW::DwgObjectHandleOccurrence>& occurrences) const {
    occurrences.clear();
    if (m_lastDwgObjectFrameValid) {
        occurrences = m_lastDwgObjectFrame.occurrences;
        return true;
    }
    return writer != nullptr
        && writer->getLastDwgObjectHandleOccurrences(occurrences);
}

bool dwgRW::getLastDwgObjectFrame(
    DRW::DwgObjectFrameReceipt& frame) const {
    frame = {};
    if (m_lastDwgObjectFrameValid) {
        frame = m_lastDwgObjectFrame;
        return true;
    }
    return writer != nullptr && writer->getLastDwgObjectFrame(frame);
}

bool dwgRW::getLastDwgCompoundEntityWriteReceipt(
    DRW::DwgCompoundEntityWriteReceipt& receipt) const {
    receipt = {};
    if (!m_lastDwgCompoundEntityWriteReceipt.valid)
        return false;
    receipt = m_lastDwgCompoundEntityWriteReceipt;
    return true;
}

bool dwgRW::setDwgObjectFrameProvenance(
    std::uint16_t classNumber, std::uint16_t writerOperation,
    std::uint64_t admissionToken) {
    if (writer == nullptr)
        return false;
    writer->setDwgObjectFrameProvenance(
        classNumber, writerOperation, admissionToken);
    return true;
}

std::uint16_t dwgRW::getDwgTypedClassNumber(
    const char* className, const char* recordName,
    std::uint16_t fallback) const {
    if (writer == nullptr || className == nullptr || recordName == nullptr)
        return fallback;
    return writer->typedDwgClassNum(className, recordName, fallback);
}

bool dwgRW::getDwgClassInstanceClass(
    std::uint32_t handle, std::uint16_t& classNumber) const {
    classNumber = 0;
    return writer != nullptr
        && writer->getDwgClassInstanceClass(handle, classNumber);
}

bool dwgRW::hasDwgClassDefinition(std::uint16_t classNumber) const {
    return writer != nullptr && writer->hasDwgClassDefinition(classNumber);
}

void dwgRW::clearDwgObjectFrameProvenance() {
    if (writer != nullptr)
        writer->clearDwgObjectFrameProvenance();
}

bool dwgRW::rollbackLastDwgObjectWrite(bool restoreCallerState) {
    if (writer == nullptr || !m_lastWriteTransaction)
        return false;

    const auto& transaction = *m_lastWriteTransaction;
    writer->rollbackCompoundWrite(transaction.checkpoint);
    if (restoreCallerState)
        restoreEntityWriteStates(transaction.entityMutations);
    forgetCallerMutations(transaction.entityMutations);
    m_lastWriteTransaction.reset();
    m_lastSuccessfulEntityHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_lastDwgCompoundEntityWriteReceipt = {};
    if (m_successfulEntityWrites != 0)
        --m_successfulEntityWrites;
    return true;
}

std::uint64_t dwgRW::beginDwgWriteTransaction(bool suppressFailureCounters,
                                              bool trackCallerMutations,
                                              bool includeAdmissionState) {
    if (writer == nullptr || m_nextDwgWriteTransaction == 0)
        return 0;

    const std::uint64_t token = m_nextDwgWriteTransaction++;
    auto transaction = std::make_unique<DwgWriteTransaction>();
    transaction->checkpoint = includeAdmissionState
        ? writer->checkpointPublicTransaction()
        : writer->checkpointCompoundWrite();
    transaction->token = token;
    transaction->successfulEntityWrites = m_successfulEntityWrites;
    transaction->writeSkipCounters = m_writeSkipCounters;
    transaction->requiredWriteFailure = m_requiredWriteFailure;
    transaction->suppressFailureCounters = suppressFailureCounters;
    transaction->trackCallerMutations = trackCallerMutations;
    m_dwgWriteTransactions.push_back(std::move(transaction));

    // An explicit scope supersedes any implicit last-write scope. The
    // transaction stack owns the rollback boundary while it is active.
    m_lastWriteTransaction.reset();
    m_lastSuccessfulEntityHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_lastDwgCompoundEntityWriteReceipt = {};
    return token;
}

bool dwgRW::commitDwgWriteTransaction(std::uint64_t token) {
    if (token == 0 || m_dwgWriteTransactions.empty()
        || m_dwgWriteTransactions.back()->token != token)
        return false;

    auto transaction = std::move(m_dwgWriteTransactions.back());
    m_dwgWriteTransactions.pop_back();
    if (!m_dwgWriteTransactions.empty()
        && m_dwgWriteTransactions.back()->trackCallerMutations) {
        auto& parent = *m_dwgWriteTransactions.back();
        parent.entityMutations.insert(parent.entityMutations.end(),
                                      transaction->entityMutations.cbegin(),
                                      transaction->entityMutations.cend());
        parent.tableMutations.insert(parent.tableMutations.end(),
                                     transaction->tableMutations.cbegin(),
                                     transaction->tableMutations.cend());
    }
    m_lastWriteTransaction.reset();
    return true;
}

bool dwgRW::rollbackDwgWriteTransaction(std::uint64_t token) {
    if (token == 0 || m_dwgWriteTransactions.empty()
        || m_dwgWriteTransactions.back()->token != token)
        return false;

    const DwgWriteTransaction& transaction = *m_dwgWriteTransactions.back();
    const std::size_t successfulEntityWrites =
        transaction.successfulEntityWrites;
    const WriteSkipCounters writeSkipCounters = transaction.writeSkipCounters;
    const bool requiredWriteFailure = transaction.requiredWriteFailure;
    const bool suppressFailureCounters = transaction.suppressFailureCounters;
    const bool trackCallerMutations = transaction.trackCallerMutations;
    const auto entityMutations = transaction.entityMutations;
    const auto tableMutations = transaction.tableMutations;
    writer->rollbackCompoundWrite(transaction.checkpoint);
    m_dwgWriteTransactions.pop_back();
    m_lastWriteTransaction.reset();
    if (suppressFailureCounters)
        m_writeSkipCounters = writeSkipCounters;
    m_requiredWriteFailure = requiredWriteFailure;
    m_successfulEntityWrites = successfulEntityWrites;
    m_lastSuccessfulEntityHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_lastDwgCompoundEntityWriteReceipt = {};
    if (trackCallerMutations) {
        restoreTableWriteStates(tableMutations);
        restoreEntityWriteStates(entityMutations);
    }
    return true;
}

bool dwgRW::recordWriteResult(WriteSkipKind kind, bool ok) {
    if (ok)
        return true;

    switch (kind) {
    case WriteSkipKind::Entity:
        ++m_writeSkipCounters.entityWrites;
        break;
    case WriteSkipKind::TableRecord:
        ++m_writeSkipCounters.tableRecordWrites;
        break;
    case WriteSkipKind::Object:
        ++m_writeSkipCounters.objectWrites;
        break;
    case WriteSkipKind::ClassRegistration:
        ++m_writeSkipCounters.classRegistrations;
        break;
    case WriteSkipKind::RawObject:
        ++m_writeSkipCounters.rawObjectWrites;
        break;
    case WriteSkipKind::RawSection:
        ++m_writeSkipCounters.rawSectionWrites;
        break;
    case WriteSkipKind::BlockDefinition:
        ++m_writeSkipCounters.blockDefinitions;
        break;
    }
    return false;
}

bool dwgRW::writeObjectTransaction(
    WriteSkipKind kind, const std::function<bool()>& write) {
    return writeObjectTransaction(kind, nullptr, write);
}

bool dwgRW::writeObjectTransaction(
    WriteSkipKind kind, DRW_TableEntry *mutationTarget,
    const std::function<bool()>& write) {
    if (writer == nullptr || !write)
        return recordWriteResult(kind, false);

    // A direct OBJECT call starts a new implicit scope. Do not leave an
    // earlier entity checkpoint armed for rollback after this object.
    m_lastWriteTransaction.reset();
    m_lastSuccessfulEntityHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_lastDwgCompoundEntityWriteReceipt = {};
    const auto checkpoint = writer->checkpointCompoundWrite();
    const std::vector<DwgTableWriteState> mutations = mutationTarget != nullptr
        ? std::vector<DwgTableWriteState>{captureTableWriteState(mutationTarget)}
        : std::vector<DwgTableWriteState>{};
    DRW::DwgObjectFrameReceipt frame;
    const bool ok = write() && writer->getLastDwgObjectFrame(frame);
    if (!ok) {
        writer->rollbackCompoundWrite(checkpoint);
        restoreTableWriteStates(mutations);
    } else if (!mutations.empty()) {
        rememberCallerMutation(mutations.front());
    }
    return recordWriteResult(kind, ok);
}

bool dwgRW::encodeEntityForWrite(DRW_Entity *ent,
                                 DRW_Entity *mutationTarget) {
    m_lastSuccessfulEntityHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_lastDwgCompoundEntityWriteReceipt = {};
    m_lastWriteTransaction.reset();
    if (writer == nullptr || ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);

    if (mutationTarget == nullptr)
        mutationTarget = ent;
    const std::vector<DwgEntityWriteState> mutations {
        captureEntityWriteState(mutationTarget)
    };
    const auto checkpoint = writer->checkpointCompoundWrite();
    if (!writer->encodeEntity(ent)) {
        // Concrete writers normally roll themselves back, but the public
        // boundary owns the no-partial-frame guarantee for every writer.
        writer->rollbackCompoundWrite(checkpoint);
        restoreEntityWriteStates(mutations);
        return recordWriteResult(WriteSkipKind::Entity, false);
    }

    if (!writer->getLastDwgObjectFrame(m_lastDwgObjectFrame)) {
        writer->rollbackCompoundWrite(checkpoint);
        restoreEntityWriteStates(mutations);
        writer->markObjectWriteFailure();
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
        m_lastDwgCompoundEntityWriteReceipt = {};
        return recordWriteResult(WriteSkipKind::Entity, false);
    }
    if (m_failAfterFirstBlockOwnedEntityForTest) {
        m_failAfterFirstBlockOwnedEntityForTest = false;
        writer->rollbackCompoundWrite(checkpoint);
        restoreEntityWriteStates(mutations);
        return recordWriteResult(WriteSkipKind::Entity, false);
    }
    auto transaction = std::make_unique<DwgWriteTransaction>();
    transaction->checkpoint = checkpoint;
    transaction->entityMutations = mutations;
    rememberCallerMutations(mutations);
    m_lastWriteTransaction = std::move(transaction);
    m_lastDwgObjectFrameValid = true;
    ++m_successfulEntityWrites;
    m_lastSuccessfulEntityHandle = ent->handle;
    return recordWriteResult(WriteSkipKind::Entity, true);
}

bool dwgRW::write(DRW_Interface *interface_, DRW::Version ver, bool bin) try {
    // The 'bin' parameter is accepted only for signature symmetry with
    // dxfRW::write — DWG is always binary on disk.
    (void)bin;
    resetWriteSkipCounters();
    if (ver != DRW::AC1015 && ver != DRW::AC1018 && ver != DRW::AC1021 &&
        ver != DRW::AC1024 && ver != DRW::AC1027 &&
        ver != DRW::AC1032) {
        error = DRW::BAD_VERSION;
        return false;
    }
    if (interface_ == nullptr) {
        error = DRW::BAD_OPEN;
        return false;
    }
    if (m_handleReservationFailed) {
        error = DRW::BAD_OPEN;
        return false;
    }
    iface = interface_;
    version = ver;
    error = DRW::BAD_NONE;

    DwgDxfOutputTransaction output(fileName, std::ios::binary);
    if (!output.open()) {
        error = DRW::BAD_OPEN;
        return false;
    }
    std::ofstream& filestr = output.stream();

    // Let the caller populate the header vars first.  Mirror of
    // dxfRW::write at libdxfrw.cpp:152-153.  The iface is allowed to
    // ignore the callback — in that case `header` keeps its default
    // (empty) state and the encoder emits per-var defaults.
    iface->writeHeader(header);
    if (m_requiredWriteFailure) {
        error = DRW::BAD_OPEN;
        return false;
    }

    if (ver == DRW::AC1032)
        writer = std::make_unique<dwgWriter32>(&filestr, &header);
    else if (ver == DRW::AC1027)
        writer = std::make_unique<dwgWriter27>(&filestr, &header);
    else if (ver == DRW::AC1024)
        writer = std::make_unique<dwgWriter24>(&filestr, &header);
    else if (ver == DRW::AC1021)
        writer = std::make_unique<dwgWriter21>(&filestr, &header);
    else if (ver == DRW::AC1018)
        writer = std::make_unique<dwgWriter18>(&filestr, &header);
    else
        writer = std::make_unique<dwgWriter15>(&filestr, &header);

    // Seed caller-reserved handles into the writer's HandleAllocator BEFORE
    // any defineBlock()/next() mint (writeBlocks runs below).  Without this a
    // block-record handle minted from 0x30 can collide with a fixed-type
    // OBJECT's preserved low handle → duplicate object-map entry →
    // writeDwgHandles() fails → BAD_OPEN aborts the whole save. (P3 #1)
    for (std::uint32_t h : m_reservedHandles)
        writer->reserveHandle(h);

    iface->writeDwgClasses();

    // Section emit order (mirror of dxfRW::write).  Per-section helpers
    // emit framing; the iface callbacks drive caller-side enumeration
    // of entities/blocks/objects into the object stream between
    // writeDwgObjects (control objects + table records) and
    // writeDwgHandles (object map).
    bool ok = !m_requiredWriteFailure
        && writer->prepareDwgClassManifest()
        && writer->writeFileHeaderStub() &&
              writer->writeDwgHeader() &&
              writer->writeDwgClasses();
    if (ok)
        writer->freezeDwgClasses();
    if (ok) {
        // Collect user-defined table records before emitting the objects
        // section.  Each iface callback calls back into dwgRW::add*() which
        // forwards to the writer's pending lists.  Order matters: LTypes
        // before Layers so ltype→handle resolution works in emitLayerRecord.
        iface->writeLTypes();
        if (m_requiredWriteFailure)
            ok = false;
        if (ok)
            iface->writeLayers();
        if (m_requiredWriteFailure)
            ok = false;
        if (ok)
            iface->writeTextstyles();
        if (m_requiredWriteFailure)
            ok = false;
        if (ok)
            iface->writeViews();
        if (m_requiredWriteFailure)
            ok = false;
        if (ok)
            iface->writeUCSs();
        if (m_requiredWriteFailure)
            ok = false;
        if (ok)
            iface->writeVports();
        if (m_requiredWriteFailure)
            ok = false;
        if (ok)
            iface->writeDimstyles();
        if (m_requiredWriteFailure)
            ok = false;
        ok = ok && writer->collectPendingTableEedAppIds();
        if (ok)
            iface->collectDwgAppIds();
        if (m_requiredWriteFailure)
            ok = false;
        if (ok)
            iface->writeAppId();
        if (m_requiredWriteFailure)
            ok = false;
        if (ok)
            ok = writer->writeDwgObjects();
    }
    if (ok) {
        // Caller-driven object-stream content. writeBlocks fires first so the
        // caller can define and populate user blocks. The object callback
        // must also run before BLOCK_RECORD objects are emitted: raw entity
        // replay records its handle in the owning block list.
        iface->writeBlocks();
        ok = !m_requiredWriteFailure;
        if (ok)
            iface->writeEntities();
        ok = ok && !m_requiredWriteFailure;
        if (ok)
            iface->writeObjects();
        ok = ok && !m_requiredWriteFailure;
        if (ok) {
            const bool filterFinalized = iface->finalizeDwgWrite();
            ok = filterFinalized && !m_requiredWriteFailure
                && writer->validateDwgClassInstanceCounts()
                && writer->validateDwgClassInstanceFrames()
                && !writer->hasEntityEedWriteFailure()
                && !writer->hasObjectEedWriteFailure()
                && writer->emitDeferredBlockControl();
        }
        if (ok) {
            const bool structureFinalized = iface->finalizeDwgWriteStructure();
            ok = structureFinalized && !m_requiredWriteFailure
                && writer->validateDwgNamedObjectDictionaryEntries();
        }
    }
    if (ok)
        ok = writer->finalizeHeaderHandseed();
    const bool handlesOk = ok && writer->writeDwgHandles();
    const bool secondHeaderOk = handlesOk && writer->writeSecondHeader();
    const bool finalOk = secondHeaderOk && writer->finalize();
    ok = finalOk;
    writer.reset();
    if (ok && !filestr.fail())
        ok = output.commit();
    else
        output.abort();
    if (!ok)
        error = DRW::BAD_OPEN;
    return ok;
}
catch (const std::exception&) {
    writer.reset();
    error = DRW::BAD_OPEN;
    return false;
}
catch (...) {
    writer.reset();
    error = DRW::BAD_OPEN;
    return false;
}

// Per-entity write API — invoked from the caller's `writeEntities`
// iface callback.  Each forwards to the writer's `encodeEntity` (a
// virtual on the base `dwgWriter`).  Returns false if the writer isn't
// ready (e.g., caller invoked outside `writeEntities`).
bool dwgRW::writePoint(DRW_Point *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeLine(DRW_Line *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::write3DLine(DRW_3DLine *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeCircle(DRW_Circle *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeArc(DRW_Arc *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeEllipse(DRW_Ellipse *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeText(DRW_Text *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeRText(DRW_RText *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeArcAlignedText(DRW_ArcAlignedText *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeLWPolyline(DRW_LWPolyline *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeRay(DRW_Ray *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeXline(DRW_Xline *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeTrace(DRW_Trace *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeSolid(DRW_Solid *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::write3dface(DRW_3Dface *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeInsert(DRW_Insert *ent) {
    m_lastSuccessfulEntityHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_lastDwgCompoundEntityWriteReceipt = {};
    m_lastWriteTransaction.reset();
    if (writer == nullptr || ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);

    std::vector<DwgEntityWriteState> mutations;
    mutations.reserve(1 + ent->attlist.size());
    mutations.push_back(captureEntityWriteState(ent));
    for (const auto& attrib : ent->attlist) {
        if (attrib != nullptr)
            mutations.push_back(captureEntityWriteState(attrib.get()));
    }
    const auto checkpoint = writer->checkpointCompoundWrite();
    const auto fail = [&]() {
        writer->rollbackCompoundWrite(checkpoint);
        restoreEntityWriteStates(mutations);
        m_lastWriteTransaction.reset();
        return recordWriteResult(WriteSkipKind::Entity, false);
    };

    const bool hasCallerAttributes = !ent->attlist.empty();
    const bool hasEmptyLegacySequence =
        !hasCallerAttributes && ent->attribHandles.size() == 2u
        && ent->attribHandles[0].ref == DRW::NoHandle
        && ent->attribHandles[1].ref == DRW::NoHandle
        && ent->seqendH.ref != DRW::NoHandle;
    if (hasEmptyLegacySequence && version >= DRW::AC1018)
        return fail();
    const bool preserveEmptyLegacy = hasEmptyLegacySequence
        && version < DRW::AC1018;
    const bool hasNoSequence =
        !hasCallerAttributes && ent->attribHandles.empty()
        && ent->seqendH.ref == DRW::NoHandle;
    if (!hasCallerAttributes && !preserveEmptyLegacy && !hasNoSequence)
        return fail();

    if (ent->handle == 0)
        ent->handle = writer->allocNextHandle();
    else
        writer->reserveHandle(ent->handle);

    if (ent->handle == DRW::NoHandle)
        return fail();
    if (preserveEmptyLegacy && ent->seqendH.ref == ent->handle)
        return fail();

    if (ent->attlist.size() > dwgSafety::MaxOwnedObjectCount)
        return fail();

    if (hasCallerAttributes) {
        ent->attribHandles.clear();
        std::vector<dwgHandle> actualAttribHandles;
        actualAttribHandles.reserve(ent->attlist.size());
        std::unordered_set<std::uint32_t> childHandles;
        childHandles.reserve(ent->attlist.size());
        for (const auto& attrib : ent->attlist) {
            if (!attrib)
                return fail();
            if (attrib->handle == 0)
                attrib->handle = writer->allocNextHandle();
            else
                writer->reserveHandle(attrib->handle);
            if (attrib->handle == ent->handle
                || !childHandles.insert(attrib->handle).second)
                return fail();
            attrib->parentHandle = ent->handle;
            attrib->space = ent->space;
            dwgHandle handle;
            handle.code = 5;
            handle.ref = attrib->handle;
            handle.ref64 = attrib->handle;
            for (std::uint32_t value = attrib->handle; value != 0; value >>= 8)
                ++handle.size;
            actualAttribHandles.push_back(handle);
        }

        if (ent->seqendH.ref == DRW::NoHandle)
            ent->seqendH.ref = writer->allocNextHandle();
        else
            writer->reserveHandle(ent->seqendH.ref);
        if (ent->seqendH.ref == ent->handle
            || std::any_of(actualAttribHandles.cbegin(), actualAttribHandles.cend(),
                           [&](const dwgHandle& handle) {
                               return handle.ref == ent->seqendH.ref;
                           }))
            return fail();

        if (version < DRW::AC1018) {
            // R13-R2000 stores only the first and last ATTRIB handles. A
            // single attribute therefore appears twice on the wire.
            ent->attribHandles.push_back(actualAttribHandles.front());
            ent->attribHandles.push_back(actualAttribHandles.back());
        } else {
            ent->attribHandles = std::move(actualAttribHandles);
        }
    } else if (!preserveEmptyLegacy) {
        ent->seqendH = {};
    }

    // BLOCK_RECORD.entMap is an ordered ownership list in R2004+.  Emit the
    // owner before its ATTRIB children so a reader can consume the sequence
    // transactionally instead of encountering orphan children first.
    if (!writer->encodeEntity(ent))
        return fail();
    DRW::DwgObjectFrameReceipt parentFrame;
    if (!writer->getLastDwgObjectFrame(parentFrame))
        return fail();

    for (const auto& attrib : ent->attlist) {
        if (!writer->encodeEntity(attrib.get()))
            return fail();
    }
    if (hasCallerAttributes || preserveEmptyLegacy) {
        DRW_SeqEnd seqEnd;
        seqEnd.handle = ent->seqendH.ref;
        seqEnd.parentHandle = ent->handle;
        seqEnd.space = ent->space;
        if (consumeCompoundSeqEndFailureForTest()
            || !writer->encodeEntity(&seqEnd))
            return fail();
    }
    ++m_successfulEntityWrites;
    m_lastSuccessfulEntityHandle = ent->handle;
    m_lastDwgObjectFrame = std::move(parentFrame);
    m_lastDwgObjectFrameValid = true;
    auto transaction = std::make_unique<DwgWriteTransaction>();
    transaction->checkpoint = checkpoint;
    transaction->entityMutations = mutations;
    rememberCallerMutations(mutations);
    m_lastWriteTransaction = std::move(transaction);
    return recordWriteResult(WriteSkipKind::Entity, true);
}

bool dwgRW::writeTable(DRW_Table *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeMText(DRW_MText *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeSpline(DRW_Spline *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeHelix(DRW_Helix *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeAttrib(DRW_Attrib *ent) {
    (void)ent;
    markDwgWriteFailure();
    return recordWriteResult(WriteSkipKind::Entity, false);
}

bool dwgRW::writeAttdef(DRW_Attdef *ent) {
    if (writer == nullptr || !writer->hasActiveBlockContent()) {
        markDwgWriteFailure();
        return recordWriteResult(WriteSkipKind::Entity, false);
    }
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeHatch(DRW_Hatch *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeMPolygon(DRW_MPolygon *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeDimension(DRW_Dimension *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeTolerance(DRW_Tolerance *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeLight(DRW_Light *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeCamera(DRW_Camera *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeGeoPositionMarker(DRW_GeoPositionMarker *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeSectionObject(DRW_SectionObject *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeMLine(DRW_MLine *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeUnderlay(DRW_Underlay *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);
    // Entity custom class must be in CLASSES before the body is encoded.
    if (!registerUnderlayEntityClass(ent->kind))
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return encodeEntityForWrite(ent);
}

bool dwgRW::registerNavisworksModelEntityClass() {
    if (writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerNavisworksModelEntityClass());
}

bool dwgRW::registerUnderlayEntityClass(DRW_Underlay::Kind kind) {
    if (writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerUnderlayEntityClass(kind));
}

bool dwgRW::writePolyline(DRW_Polyline *ent) {
    m_lastSuccessfulEntityHandle = 0;
    m_lastDwgObjectFrame = {};
    m_lastDwgObjectFrameValid = false;
    m_lastDwgCompoundEntityWriteReceipt = {};
    m_lastWriteTransaction.reset();
    if (writer == nullptr || ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);
    std::vector<DwgEntityWriteState> mutations;
    mutations.reserve(1 + ent->vertlist.size());
    mutations.push_back(captureEntityWriteState(ent));
    for (const auto& vertex : ent->vertlist) {
        if (vertex != nullptr)
            mutations.push_back(captureEntityWriteState(vertex.get()));
    }
    const auto checkpoint = writer->checkpointCompoundWrite();
    const auto fail = [&]() {
        writer->rollbackCompoundWrite(checkpoint);
        restoreEntityWriteStates(mutations);
        m_lastWriteTransaction.reset();
        m_lastDwgCompoundEntityWriteReceipt = {};
        return recordWriteResult(WriteSkipKind::Entity, false);
    };
    if (ent->vertlist.size() > dwgSafety::MaxOwnedObjectCount
        || ent->vertlist.size()
               > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        return fail();

    // Pre-allocate the polyline handle before vertex handles so entity-sweep
    // ordering can stage the parent and its owned children as one aggregate.
    if (ent->handle == 0)
        ent->handle = writer->allocNextHandle();
    else
        writer->reserveHandle(ent->handle);
    const bool isPolyface = (ent->flags & 64) != 0;
    const bool isMesh = (ent->flags & 16) != 0;
    const bool is3D = (ent->flags & 8) != 0;
    std::unordered_set<std::uint32_t> childHandles;
    childHandles.reserve(ent->vertlist.size() + 1);
    // Reserve all child handles before encoding the parent. This preserves the
    // owner-first BLOCK_RECORD.entMap order without changing caller handles.
    for (auto& v : ent->vertlist) {
        if (!v)
            return fail();
        v->parentHandle = ent->handle;
        v->space = ent->space;
        if (v->dwgSubtype() == DRW_Vertex::DwgSubtype::Auto) {
            if (isPolyface) {
                v->setDwgSubtype((v->flags & 64) != 0
                    ? DRW_Vertex::DwgSubtype::Polyface
                    : DRW_Vertex::DwgSubtype::PolyfaceFace);
            } else if (isMesh) {
                v->setDwgSubtype(DRW_Vertex::DwgSubtype::Mesh);
            } else if (is3D) {
                v->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex3D);
            } else {
                v->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);
            }
        }
        if (!ent->isDwgVertexCompatible(*v))
            return fail();
        if (v->handle == 0)
            v->handle = writer->allocNextHandle();
        else
            writer->reserveHandle(v->handle);
        if (v->handle == ent->handle
            || !childHandles.insert(v->handle).second)
            return fail();
    }
    DRW_SeqEnd seqEnd;
    seqEnd.handle = writer->allocNextHandle();
    seqEnd.parentHandle = ent->handle;
    seqEnd.space = ent->space;
    if (seqEnd.handle == ent->handle
        || !childHandles.insert(seqEnd.handle).second)
        return fail();
    ent->setDwgSeqEndHandle(seqEnd.handle);

    // The parent must be the first entry in a modern BLOCK_RECORD ownership
    // list; its handle references are already complete because all children
    // and the SEQEND were reserved above.
    DRW::DwgCompoundEntityWriteReceipt receipt;
    receipt.parentHandle = ent->handle;
    receipt.parentOwnerHandle = ent->parentHandle;
    const auto appendFrame = [&receipt](
        DRW::DwgCompoundEntityFrameRole role,
        const DRW::DwgObjectFrameReceipt& frame, std::uint32_t expectedHandle) {
        if (!frame.valid || frame.generation == 0 || expectedHandle == 0
            || frame.objectHandle != expectedHandle) {
            return false;
        }
        for (const auto& existing : receipt.frames) {
            if (existing.frame.objectHandle == expectedHandle)
                return false;
        }
        receipt.frames.push_back({role, frame});
        return true;
    };

    if (!writer->encodeEntity(ent))
        return fail();
    DRW::DwgObjectFrameReceipt parentFrame;
    if (!writer->getLastDwgObjectFrame(parentFrame)
        || !appendFrame(DRW::DwgCompoundEntityFrameRole::Parent, parentFrame,
                        ent->handle))
        return fail();
    for (auto& v : ent->vertlist) {
        if (!writer->encodeEntity(v.get()))
            return fail();
        DRW::DwgObjectFrameReceipt vertexFrame;
        if (!writer->getLastDwgObjectFrame(vertexFrame)
            || !appendFrame(DRW::DwgCompoundEntityFrameRole::Vertex,
                            vertexFrame, v->handle)) {
            return fail();
        }
    }
    if (consumeCompoundSeqEndFailureForTest()
        || !writer->encodeEntity(&seqEnd))
        return fail();
    DRW::DwgObjectFrameReceipt seqEndFrame;
    if (!writer->getLastDwgObjectFrame(seqEndFrame)
        || !appendFrame(DRW::DwgCompoundEntityFrameRole::SeqEnd,
                        seqEndFrame, seqEnd.handle)
        || receipt.frames.size() != ent->vertlist.size() + 2) {
        return fail();
    }
    receipt.valid = true;
    ++m_successfulEntityWrites;
    m_lastSuccessfulEntityHandle = ent->handle;
    m_lastDwgObjectFrame = std::move(parentFrame);
    m_lastDwgObjectFrameValid = true;
    m_lastDwgCompoundEntityWriteReceipt = std::move(receipt);
    auto transaction = std::make_unique<DwgWriteTransaction>();
    transaction->checkpoint = checkpoint;
    transaction->entityMutations = mutations;
    rememberCallerMutations(mutations);
    m_lastWriteTransaction = std::move(transaction);
    return recordWriteResult(WriteSkipKind::Entity, true);
}

bool dwgRW::writeLeader(DRW_Leader *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeMLeader(DRW_MLeader *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeViewport(DRW_Viewport *ent) {
    return encodeEntityForWrite(ent);
}

// Phase 6.1 — SHAPE passthrough (no native LibreCAD entity).
bool dwgRW::writeShape(DRW_Shape *ent) {
    return encodeEntityForWrite(ent);
}

// Phase 6.2 — OLE2FRAME passthrough (opaque payload preserved by encodeDwg).
bool dwgRW::writeOle2Frame(DRW_Ole2Frame *ent) {
    return encodeEntityForWrite(ent);
}

// Legacy OLEFRAME passthrough. The typed encoder retains the bounded opaque
// payload while the raw unsupported carrier keeps exact source bytes available
// for same-version replay when the high-level filter needs it.
bool dwgRW::writeOleFrame(DRW_OleFrame *ent) {
    return encodeEntityForWrite(ent);
}

// Table-record add* methods — forward to dwgWriter15 via dynamic_cast since
// the add*() API lives on dwgWriter15 (all concrete writers derive from it).
// Declared early so entity writers (IMAGE / IMAGEDEF) can reuse it.
static dwgWriter15 *asWriter15(std::unique_ptr<dwgWriter> &w) {
    return dynamic_cast<dwgWriter15 *>(w.get());
}

void dwgRW::setDwgWriteFailurePointForTest(
    DwgWriteFailurePoint point) {
    if (point == DwgWriteFailurePoint::AfterFirstBlockOwnedEntity) {
        m_failAfterFirstBlockOwnedEntityForTest = true;
        return;
    }
    if (auto *w = asWriter15(writer); w != nullptr)
        w->setWriteFailurePointForTest(static_cast<std::uint8_t>(point));
}

bool dwgRW::writeDwgObjectsForTest() {
    return writer != nullptr && writer->writeDwgObjects();
}

bool dwgRW::emitDeferredBlockControlForTest() {
    return writer != nullptr && writer->emitDeferredBlockControl();
}

bool dwgRW::writeMesh(DRW_Mesh *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeWipeout(DRW_Wipeout *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);
    // WIPEOUT is fixed entity type 1109; no CLASSES registration is needed.
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeImageDef(DRW_ImageDef *object) {
    auto *w = asWriter15(writer);
    if (w == nullptr || object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, object,
        [w, object] { return w->writeImageDef(*object); });
}

bool dwgRW::writeImageDefinitionReactor(DRW_ImageDefinitionReactor *object) {
    auto *w = asWriter15(writer);
    if (w == nullptr || object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, object,
        [w, object] { return w->writeImageDefinitionReactor(*object); });
}

bool dwgRW::registerPointCloudDefObjectClass(DRW_PointCloudDef *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerPointCloudDefObjectClass(object->m_kind,
                                                  object->handle));
}

bool dwgRW::writePointCloudDef(DRW_PointCloudDef *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, object,
        [w, object] { return w->writePointCloudDef(*object); });
}

bool dwgRW::registerNavisworksModelDefObjectClass(
    DRW_NavisworksModelDef *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerNavisworksModelDefObjectClass(object->handle));
}

bool dwgRW::writeNavisworksModelDef(DRW_NavisworksModelDef *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, object,
        [w, object] { return w->writeNavisworksModelDef(*object); });
}

bool dwgRW::registerPointCloudColorMapObjectClass(
    DRW_PointCloudColorMap *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerPointCloudColorMapObjectClass(object->handle));
}

bool dwgRW::writePointCloudColorMap(DRW_PointCloudColorMap *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, object,
        [w, object] { return w->writePointCloudColorMap(*object); });
}

bool dwgRW::registerImageDefReactorObjectClass(
    DRW_ImageDefinitionReactor *object) {
    if (writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    const std::uint32_t handle = object != nullptr ? object->handle : 0;
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerImageDefReactorObjectClass(handle));
}

bool dwgRW::writeImage(DRW_Image *ent, const std::string *fileName,
                       const DRW_ImageDef *definition,
                       const DRW_ImageDefinitionReactor *reactorDefinition) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);

    // Keep caller-visible handle/reference and inferred clip fields staged
    // until the complete IMAGE transaction has been accepted.
    DRW_Image image = *ent;

    dwgWriter::CompoundWriteCheckpoint compoundCheckpoint;
    bool hasCompoundWrite = false;
    const auto rollbackCompoundImage = [&]() {
        if (!hasCompoundWrite || writer == nullptr)
            return;
        writer->rollbackCompoundWrite(compoundCheckpoint);
        m_lastWriteTransaction.reset();
        m_lastSuccessfulEntityHandle = 0;
        m_lastDwgObjectFrame = {};
        m_lastDwgObjectFrameValid = false;
    };

    // When a path is provided, emit IMAGEDEF + reactor and wire handles so
    // re-read delivers both the entity frame and linkImage().
    if (fileName != nullptr && !fileName->empty()) {
        auto *w = asWriter15(writer);
        if (w == nullptr)
            return recordWriteResult(WriteSkipKind::Entity, false);

        // IMAGE is a compound write: IMAGEDEF and IMAGEDEF_REACTOR must not
        // survive if the final IMAGE frame is rejected. Keep one boundary
        // around all three frames and retain it for the public rollback API.
        compoundCheckpoint = writer->checkpointCompoundWrite();
        hasCompoundWrite = true;

        if ((definition != nullptr && definition->hasDataStorageBinaryData())
            || (reactorDefinition != nullptr
                && reactorDefinition->hasDataStorageBinaryData())) {
            return recordWriteResult(WriteSkipKind::Object, false);
        }

        DRW_ImageDef imageDef;
        if (definition != nullptr)
            imageDef = *definition;
        imageDef.handle = 0;
        imageDef.name = *fileName;
        if (definition == nullptr) {
            imageDef.u = image.sizeu;
            imageDef.v = image.sizev;
            imageDef.up = 1.0;
            imageDef.vp = 1.0;
            imageDef.loaded = 1;
            imageDef.resolution = 0;
        }
        if (imageDef.parentHandle == 0)
            imageDef.parentHandle = 0xC;  // named object dictionary default owner
        if (!w->writeImageDef(imageDef)) {
            rollbackCompoundImage();
            return recordWriteResult(WriteSkipKind::Object, false);
        }

        // Allocate the image entity handle before the reactor so the reactor
        // can name the image as its owner when the caller left it unset.
        if (image.handle == 0)
            image.handle = w->allocNextHandle();
        else
            w->reserveHandle(image.handle);

        DRW_ImageDefinitionReactor reactor;
        if (reactorDefinition != nullptr)
            reactor = *reactorDefinition;
        reactor.handle = 0;
        if (reactorDefinition == nullptr)
            reactor.m_classVersion = 2;
        reactor.parentHandle = image.handle;
        if (!w->writeImageDefinitionReactor(reactor)) {
            rollbackCompoundImage();
            return recordWriteResult(WriteSkipKind::Object, false);
        }

        image.ref = imageDef.handle;
        image.m_imageDefReactorHandle = reactor.handle;
    }

    // IMAGE uses fixed oType 101 — no custom class registration.
    if (image.m_clipBoundaryType == 0 && image.clipPath.empty()) {
        // Default full-image rectangle so encodeDwg accepts a fresh IMAGE.
        image.m_clipBoundaryType = 1;
        image.clipPath = {DRW_Coord{-0.5, -0.5, 0.0},
                          DRW_Coord{image.sizeu - 0.5, image.sizev - 0.5, 0.0}};
    }
    if (!encodeEntityForWrite(&image, ent)) {
        rollbackCompoundImage();
        return false;
    }
    if (hasCompoundWrite && m_lastWriteTransaction)
        m_lastWriteTransaction->checkpoint = compoundCheckpoint;
    else {
        rollbackCompoundImage();
        return recordWriteResult(WriteSkipKind::Entity, false);
    }
    *ent = image;
    return true;
}

bool dwgRW::writePointCloud(DRW_PointCloud *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writePointCloudEx(DRW_PointCloudEx *ent) {
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeNavisworksModel(DRW_NavisworksModel *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::Entity, false);
    if (!registerNavisworksModelEntityClass())
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return encodeEntityForWrite(ent);
}

bool dwgRW::writeSurface(DRW_Surface *ent) {
    if (ent == nullptr || ent->getDwgClassNum() == 0)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return encodeEntityForWrite(ent);
}

bool dwgRW::registerSurfaceEntityClass(DRW_Surface *ent) {
    if (writer == nullptr || ent == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerSurfaceEntityClass(ent));
}

bool dwgRW::registerDwgEntityClassInstance(std::uint16_t classNumber,
                                           std::uint32_t handle) {
    if (writer == nullptr || handle == 0)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (handle == std::numeric_limits<std::uint32_t>::max())
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    writer->reserveHandle(handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerDwgEntityClassInstance(classNumber, handle));
}

std::uint32_t dwgRW::defineBlock(const std::string& name, const DRW_Coord& basePoint,
                           int insUnits) {
    if (writer == nullptr) {
        (void)recordWriteResult(WriteSkipKind::BlockDefinition, false);
        return 0;
    }
    const std::uint32_t handle = writer->defineBlock(name, basePoint, insUnits);
    (void)recordWriteResult(WriteSkipKind::BlockDefinition, handle != 0);
    return handle;
}

bool dwgRW::beginBlockContent(std::uint32_t blockRecordHandle) {
    return writer != nullptr && writer->beginBlockContent(blockRecordHandle);
}

bool dwgRW::endBlockContent() {
    return writer != nullptr && writer->endBlockContent();
}

bool dwgRW::addLType(DRW_LType *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    const auto mutation = captureTableWriteState(ent);
    const std::uint32_t handle = w->addLType(*ent);
    if (handle != 0) {
        ent->handle = handle;
        rememberCallerMutation(mutation);
    } else {
        restoreTableWriteState(mutation);
    }
    return recordWriteResult(WriteSkipKind::TableRecord, handle != 0);
}
bool dwgRW::addLayer(DRW_Layer *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    const auto mutation = captureTableWriteState(ent);
    const std::uint32_t handle = w->addLayer(*ent);
    if (handle != 0) {
        ent->handle = handle;
        rememberCallerMutation(mutation);
    } else {
        restoreTableWriteState(mutation);
    }
    return recordWriteResult(WriteSkipKind::TableRecord, handle != 0);
}
bool dwgRW::addTextstyle(DRW_Textstyle *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    const auto mutation = captureTableWriteState(ent);
    const std::uint32_t handle = w->addTextstyle(*ent);
    if (handle != 0) {
        ent->handle = handle;
        rememberCallerMutation(mutation);
    } else {
        restoreTableWriteState(mutation);
    }
    return recordWriteResult(WriteSkipKind::TableRecord, handle != 0);
}
bool dwgRW::addUCS(DRW_UCS *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    const auto mutation = captureTableWriteState(ent);
    const bool ok = w->addUcs(*ent);
    if (ok)
        rememberCallerMutation(mutation);
    else
        restoreTableWriteState(mutation);
    return recordWriteResult(WriteSkipKind::TableRecord, ok);
}
bool dwgRW::addView(DRW_View *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    const auto mutation = captureTableWriteState(ent);
    const bool ok = w->addView(*ent);
    if (ok)
        rememberCallerMutation(mutation);
    else
        restoreTableWriteState(mutation);
    return recordWriteResult(WriteSkipKind::TableRecord, ok);
}
bool dwgRW::addVport(DRW_Vport *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    const auto mutation = captureTableWriteState(ent);
    const bool ok = w->addVport(*ent);
    if (ok)
        rememberCallerMutation(mutation);
    else
        restoreTableWriteState(mutation);
    return recordWriteResult(WriteSkipKind::TableRecord, ok);
}
bool dwgRW::addViewportEntityHeader(DRW_ViewportEntityHeader *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    const auto mutation = captureTableWriteState(ent);
    const bool ok = w->addViewportEntityHeader(*ent);
    if (ok)
        rememberCallerMutation(mutation);
    else
        restoreTableWriteState(mutation);
    return recordWriteResult(WriteSkipKind::TableRecord, ok);
}
bool dwgRW::addDimstyle(DRW_Dimstyle *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    const auto mutation = captureTableWriteState(ent);
    const std::uint32_t handle = w->addDimstyle(*ent);
    if (handle != 0) {
        ent->handle = handle;
        rememberCallerMutation(mutation);
    } else {
        restoreTableWriteState(mutation);
    }
    return recordWriteResult(WriteSkipKind::TableRecord, handle != 0);
}
bool dwgRW::addAppId(DRW_AppId *ent) {
    if (ent == nullptr)
        return recordWriteResult(WriteSkipKind::TableRecord, false);
    auto *w = asWriter15(writer);
    if (w == nullptr) return recordWriteResult(WriteSkipKind::TableRecord, false);
    const auto mutation = captureTableWriteState(ent);
    const std::uint32_t handle = w->addAppId(*ent);
    if (handle != 0) {
        ent->handle = handle;
        rememberCallerMutation(mutation);
    } else {
        restoreTableWriteState(mutation);
    }
    return recordWriteResult(WriteSkipKind::TableRecord, handle != 0);
}

bool dwgRW::writeAcDbPlaceholder(DRW_AcDbPlaceholder *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeAcDbPlaceholder(*object); });
}

bool dwgRW::writeVbaProject(DRW_VbaProject *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeVbaProject(*object); });
}

bool dwgRW::writeDbColor(DRW_DbColor *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeDbColor(*object); });
}

bool dwgRW::registerDbColorObjectClass(DRW_DbColor *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerDbColorObjectClass(object->handle));
}

bool dwgRW::registerPlotSettingsObjectClass(std::uint32_t handle) {
    if (writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (handle != 0)
        writer->reserveHandle(handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerPlotSettingsObjectClass(handle));
}

bool dwgRW::validateDwgClassInstanceCounts() const {
    return writer != nullptr && writer->validateDwgClassInstanceCounts();
}

bool dwgRW::registerDimensionAssociationObjectClass(
    DRW_DimensionAssociation *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerDimensionAssociationObjectClass(object->handle));
}

bool dwgRW::writeDimensionAssociation(DRW_DimensionAssociation *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeDimensionAssociation(*object); });
}

bool dwgRW::registerEvaluationGraphObjectClass(
    DRW_EvaluationGraph *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerEvaluationGraphObjectClass(object->handle));
}

bool dwgRW::writeEvaluationGraph(DRW_EvaluationGraph *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeEvaluationGraph(*object); });
}

bool dwgRW::writeBlockRepresentationData(
    DRW_BlockRepresentationData *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeBlockRepresentationData(*object); });
}

bool dwgRW::registerSunObjectClass(DRW_Sun *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerSunObjectClass(object->handle));
}

bool dwgRW::writeSun(DRW_Sun *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeSun(*object); });
}

bool dwgRW::registerMLeaderStyleObjectClass(DRW_MLeaderStyle *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerMLeaderStyleObjectClass(object->handle));
}

bool dwgRW::writeMLeaderStyle(DRW_MLeaderStyle *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeMLeaderStyle(*object); });
}

bool dwgRW::registerTableStyleObjectClass(DRW_TableStyle *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerTableStyleObjectClass(object->handle));
}

bool dwgRW::writeTableStyle(DRW_TableStyle *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeTableStyle(*object); });
}

bool dwgRW::registerMaterialObjectClass(DRW_Material *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerMaterialObjectClass(object->handle));
}

bool dwgRW::writeMaterial(DRW_Material *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeMaterial(*object); });
}

bool dwgRW::registerLightListObjectClass(DRW_LightList *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerLightListObjectClass(object->handle));
}

bool dwgRW::writeLightList(DRW_LightList *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeLightList(*object); });
}

bool dwgRW::registerBackgroundObjectClass(DRW_Background *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerBackgroundObjectClass(object->m_kind, object->handle));
}

bool dwgRW::writeBackground(DRW_Background *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeBackground(*object); });
}

bool dwgRW::registerSunStudyObjectClass(DRW_SunStudy *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerSunStudyObjectClass(object->handle));
}

bool dwgRW::writeSunStudy(DRW_SunStudy *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeSunStudy(*object); });
}

bool dwgRW::registerMotionPathObjectClass(DRW_MotionPath *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerMotionPathObjectClass(object->handle));
}

bool dwgRW::writeMotionPath(DRW_MotionPath *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeMotionPath(*object); });
}

bool dwgRW::registerCurvePathObjectClass(DRW_CurvePath *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerCurvePathObjectClass(object->handle));
}

bool dwgRW::writeCurvePath(DRW_CurvePath *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeCurvePath(*object); });
}

bool dwgRW::registerPointPathObjectClass(DRW_PointPath *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerPointPathObjectClass(object->handle));
}

bool dwgRW::writePointPath(DRW_PointPath *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writePointPath(*object); });
}

bool dwgRW::registerObjectPtrObjectClass(DRW_ObjectPtr *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerObjectPtrObjectClass(object->handle));
}

bool dwgRW::writeObjectPtr(DRW_ObjectPtr *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeObjectPtr(*object); });
}

bool dwgRW::registerPartialViewingIndexObjectClass(
    DRW_PartialViewingIndex *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerPartialViewingIndexObjectClass(object->handle));
}

bool dwgRW::writePartialViewingIndex(DRW_PartialViewingIndex *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writePartialViewingIndex(*object); });
}

bool dwgRW::registerRenderSettingsObjectClass(DRW_RenderSettings *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerRenderSettingsObjectClass(object->m_kind,
                                                   object->handle));
}

bool dwgRW::writeRenderSettings(DRW_RenderSettings *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeRenderSettings(*object); });
}

bool dwgRW::registerVisualStyleObjectClass(DRW_VisualStyle *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerVisualStyleObjectClass(object->handle));
}

bool dwgRW::writeVisualStyle(DRW_VisualStyle *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeVisualStyle(*object); });
}

bool dwgRW::writeDictionary(DRW_Dictionary *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeDictionary(*object); });
}

bool dwgRW::writeXRecord(DRW_XRecord *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeXRecord(*object); });
}

bool dwgRW::writePlotSettings(DRW_PlotSettings *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writePlotSettings(*object); });
}

bool dwgRW::writeLayout(DRW_Layout *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeLayout(*object); });
}

bool dwgRW::writeGroup(DRW_Group *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeGroup(*object); });
}

bool dwgRW::writeMLineStyle(DRW_MLineStyle *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeMLineStyle(*object); });
}

bool dwgRW::registerRasterVariablesObjectClass(DRW_RasterVariables *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerRasterVariablesObjectClass(object->handle));
}

bool dwgRW::writeRasterVariables(DRW_RasterVariables *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeRasterVariables(*object); });
}

bool dwgRW::registerWipeoutVariablesObjectClass(DRW_WipeoutVariables *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerWipeoutVariablesObjectClass(object->handle));
}

bool dwgRW::registerWipeoutEntityClass() {
    if (writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerWipeoutEntityClass());
}

bool dwgRW::writeWipeoutVariables(DRW_WipeoutVariables *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeWipeoutVariables(*object); });
}

bool dwgRW::registerGeoDataObjectClass(DRW_GeoData *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerGeoDataObjectClass(object->handle));
}

bool dwgRW::writeGeoData(DRW_GeoData *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeGeoData(*object); });
}

bool dwgRW::registerSpatialFilterObjectClass(DRW_SpatialFilter *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerSpatialFilterObjectClass(object->handle));
}

bool dwgRW::writeSpatialFilter(DRW_SpatialFilter *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeSpatialFilter(*object); });
}

// PR 8d.2a — five small no-storage OBJECTS families.  Same wrapper shape as
// the PR 8d.1b/c/d trio (RasterVariables/GeoData/SpatialFilter).
bool dwgRW::registerScaleObjectClass(DRW_Scale *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerScaleObjectClass(object->handle));
}

bool dwgRW::writeScale(DRW_Scale *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeScale(*object); });
}

bool dwgRW::registerIDBufferObjectClass(DRW_IDBuffer *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerIDBufferObjectClass(object->handle));
}

bool dwgRW::writeIDBuffer(DRW_IDBuffer *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeIDBuffer(*object); });
}

bool dwgRW::registerLayerIndexObjectClass(DRW_LayerIndex *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerLayerIndexObjectClass(object->handle));
}

bool dwgRW::writeLayerIndex(DRW_LayerIndex *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeLayerIndex(*object); });
}

bool dwgRW::registerSpatialIndexObjectClass(DRW_SpatialIndex *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerSpatialIndexObjectClass(object->handle));
}

bool dwgRW::writeSpatialIndex(DRW_SpatialIndex *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeSpatialIndex(*object); });
}

bool dwgRW::registerDictionaryVarObjectClass(DRW_DictionaryVar *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerDictionaryVarObjectClass(object->handle));
}

bool dwgRW::writeDictionaryVar(DRW_DictionaryVar *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeDictionaryVar(*object); });
}

// PR 8d.2b — four larger no-storage OBJECTS families.  Same wrapper shape as
// the PR 8d.2a trio.
bool dwgRW::registerDictionaryWithDefaultObjectClass(DRW_DictionaryWithDefault *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerDictionaryWithDefaultObjectClass(object->handle));
}

bool dwgRW::writeDictionaryWithDefault(DRW_DictionaryWithDefault *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeDictionaryWithDefault(*object); });
}

bool dwgRW::registerSortEntsTableObjectClass(DRW_SortEntsTable *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerSortEntsTableObjectClass(object->handle));
}

bool dwgRW::writeSortEntsTable(DRW_SortEntsTable *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeSortEntsTable(*object); });
}

bool dwgRW::registerFieldListObjectClass(DRW_FieldList *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerFieldListObjectClass(object->handle));
}

bool dwgRW::writeFieldList(DRW_FieldList *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeFieldList(*object); });
}

bool dwgRW::registerFieldObjectClass(DRW_Field *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (consumeFieldClassRegistrationFailureForTest())
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration,
                             writer->registerFieldObjectClass(object->handle));
}

bool dwgRW::writeField(DRW_Field *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, object] { return w->writeField(*object); });
}

bool dwgRW::registerUnderlayDefinitionObjectClass(
    DRW_UnderlayDefinition *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerUnderlayDefinitionObjectClass(object->kind,
                                                      object->handle));
}

bool dwgRW::writeUnderlayDefinition(DRW_UnderlayDefinition *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeUnderlayDefinition(*object); });
}

bool dwgRW::registerRawDwgObjectClass(const DRW_UnsupportedObject *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    const bool registered = writer->registerRawObjectClass(*object);
    if (registered && object->m_handle != 0)
        writer->reserveHandle(object->m_handle);
    return recordWriteResult(WriteSkipKind::ClassRegistration, registered);
}

bool dwgRW::writeRawDwgObject(DRW_UnsupportedObject *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::RawObject, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::RawObject, false);
    const bool written = w->replayRawObject(*object);
    if (!written)
        w->rollbackRawObjectClassInstance(*object);
    return recordWriteResult(WriteSkipKind::RawObject, written);
}

bool dwgRW::writeRawDwgSection(const DRW_RawDwgSection *section) {
    if (section == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::RawSection, false);
    return recordWriteResult(WriteSkipKind::RawSection,
                             writer->addRawDwgSection(*section));
}

bool dwgRW::registerSectionObjectClass(DRW_Section *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerSectionObjectClass(object->m_kind, object->handle));
}

bool dwgRW::writeSection(const DRW_Section *section) {
    if (section == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object, [w, section] { return w->writeSection(*section); });
}

bool dwgRW::registerTvDevicePropertiesObjectClass(
    DRW_TvDeviceProperties *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerTvDevicePropertiesObjectClass(object->handle));
}

bool dwgRW::writeTvDeviceProperties(DRW_TvDeviceProperties *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeTvDeviceProperties(*object); });
}

bool dwgRW::registerVxControlObjectClass(DRW_VxControl *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerVxControlObjectClass(object->handle));
}

bool dwgRW::writeVxControl(DRW_VxControl *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeVxControl(*object); });
}

bool dwgRW::registerVxTableRecordObjectClass(DRW_VxTableRecord *object) {
    if (object == nullptr || writer == nullptr)
        return recordWriteResult(WriteSkipKind::ClassRegistration, false);
    if (object->handle != 0)
        writer->reserveHandle(object->handle);
    return recordWriteResult(
        WriteSkipKind::ClassRegistration,
        writer->registerVxTableRecordObjectClass(object->handle));
}

bool dwgRW::writeVxTableRecord(DRW_VxTableRecord *object) {
    if (object == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    auto *w = asWriter15(writer);
    if (w == nullptr)
        return recordWriteResult(WriteSkipKind::Object, false);
    return writeObjectTransaction(
        WriteSkipKind::Object,
        [w, object] { return w->writeVxTableRecord(*object); });
}

std::unique_ptr<dwgReader> dwgRW::createReaderForVersion(
    DRW::Version version, std::unique_ptr<dwgBuffer> buffer, dwgRW *p)
{
    switch ( version ) {
       // unsupported (no parser exists)
       case DRW::UNKNOWNV:
       case DRW::MC00:
       case DRW::AC12:
       case DRW::AC150:
       case DRW::AC1002:
           break;          // R2.5: same family as R2.6, but no corpus fixture
                           //       to validate -> left rejected for now.
       case DRW::AC14:     // R1.40: dedicated pre-R2.0b reader (different
                           //   container: no @0x14 section pointers / no
                           //   per-record size or CRC; entity stream @0x202).
           return std::unique_ptr< dwgReader >( new dwgReaderR1_40(std::move(buffer), p) );
       case DRW::AC210:    // R2.10, R2.6, R9: same pre-R13 container as R10 (the
       case DRW::AC1003:   //   SINCE(R_2_0b)/PRE(R_10) branch). dwgReaderR11
       case DRW::AC1004:   //   handles them via `version`-gated deltas (1B LTYPE
                           //   handle, 2D LINE/POINT/3DLINE bodies, elevation-
                           //   for-all). Validated vs dwgread.
       case DRW::AC1006:   // R10: 1B LTYPE handle; bodies read 3D unless HAS_ELEVATION.
       case DRW::AC1009:   // R11: 2B LTYPE handle + 2-byte table `used` field.
           return std::unique_ptr< dwgReader >( new dwgReaderR11(std::move(buffer), p) );

       case DRW::AC1012:
       case DRW::AC1014:
       case DRW::AC1015:
           return std::unique_ptr< dwgReader >( new dwgReader15(std::move(buffer), p) );

       case DRW::AC1018:
           return std::unique_ptr< dwgReader >( new dwgReader18(std::move(buffer), p) );

       case DRW::AC1021:
           return std::unique_ptr< dwgReader >( new dwgReader21(std::move(buffer), p) );

       case DRW::AC1024:
           return std::unique_ptr< dwgReader >( new dwgReader24(std::move(buffer), p) );

       case DRW::AC1027:
           return std::unique_ptr< dwgReader >( new dwgReader27(std::move(buffer), p) );

       case DRW::AC1032:
           return std::unique_ptr< dwgReader >( new dwgReader32(std::move(buffer), p) );
           break;
    }
    return nullptr;
}

/* Open the file and stores it in filestr, install the correct reader version.
 * If fail opening file, error are set as DRW::BAD_OPEN
 * If not are DWG or are unsupported version, error are set as DRW::BAD_VERSION
 * and closes filestr.
 * Return true on succeed or false on fail
*/
bool dwgRW::openFile(std::ifstream *filestr){
    bool isOk = false;
    DRW_DBG("dwgRW::read 1\n");
    filestr->open (fileName.c_str(), std::ios_base::in | std::ios::binary);
    if (!filestr->is_open() || !filestr->good() ){
        error = DRW::BAD_OPEN;
        return isOk;
    }

    auto buffer = std::make_unique<dwgBuffer>(filestr);
    isOk = openBuffer(std::move(buffer));
    if (!isOk)
        filestr->close();

    return isOk;
}

bool dwgRW::openBuffer(std::unique_ptr<dwgBuffer> buffer) {
    bool isOk = false;
    if (!buffer || buffer->size() < 6) {
        error = DRW::BAD_VERSION;
        return false;
    }

    version = sniffVersion(buffer.get());
    reader = createReaderForVersion(version, std::move(buffer), this);

    if (!reader) {
        error = DRW::BAD_VERSION;
    } else
        isOk = true;

    return isOk;
}

DRW::Version dwgRW::sniffVersion(dwgBuffer *buffer) {
    if (buffer == nullptr || buffer->size() < 6)
        return DRW::UNKNOWNV;

    char line[7];
    for (int i = 0; i < 6; ++i)
        line[i] = static_cast<char>(buffer->getRawChar8());
    line[6]='\0';
    DRW_DBG("dwgRW::read 2\n");
    DRW_DBG("dwgRW::read line version: ");
    DRW_DBG(line);
    DRW_DBG("\n");

    // check version line against known version strings
    DRW::Version sniffedVersion = DRW::UNKNOWNV;
    for ( auto it = DRW::dwgVersionStrings.begin(); it != DRW::dwgVersionStrings.end(); ++it )
    {
        if ( std::strncmp( line, it->first, sizeof(line) ) == 0 ) {
            sniffedVersion = it->second;
            break;
        }
    }

    buffer->resetPosition();
    return sniffedVersion;
}

/********* Reader Process *********/

bool dwgRW::processDwg() {
    DRW_DBG("dwgRW::processDwg() start processing dwg\n");
    bool ret = false;
    bool ret2 = false;
    bool classReadCompleted = false;
    DRW_Header hdr;
    bool coverageFinalized = false;
    bool classCoverageFinalized = false;
    const auto finalizeCoverage = [this, &coverageFinalized](bool completed) noexcept {
        if (coverageFinalized)
            return;
        coverageFinalized = true;
        reader->finalizeDwgFrameCoverageNoThrow(*iface, completed);
    };
    const auto finalizeClassCoverage =
        [this, &classCoverageFinalized, &classReadCompleted]() noexcept {
            if (classCoverageFinalized)
                return;
            classCoverageFinalized = true;
            reader->finalizeDwgClassCoverageNoThrow(
                *iface, classReadCompleted);
        };

    try {
    reader->beginDwgClassCoverage();
    ret = reader->readDwgHeader(hdr);
    if (!ret) {
        error = DRW::BAD_READ_HEADER;
    }

    ret2 = reader->readDwgClasses();
    classReadCompleted = ret2;
    if (ret && !ret2) {
        error = DRW::BAD_READ_CLASSES;
        ret = ret2;
    }

    ret2 = reader->readDwgHandles();
    if (ret && !ret2) {
        error = DRW::BAD_READ_HANDLES;
        ret = ret2;
    }

    // readDwgTables/Blocks/Entities/Objects all depend on a valid header,
    // classes map and object handle map from the phases above -- on a
    // corrupted file where one of those already failed (ret is false),
    // running them anyway walks whatever ended up in ObjectMap (empty,
    // partial, or built from garbage offsets) and pays full CRC/decompress
    // cost per bogus "entity"/"object" before each is individually reported
    // as a parse failure. `ret &&` short-circuits the call once ret is
    // false, turning that multi-second futile parse into a fast, clear
    // error return; the already-set error code (from whichever phase failed
    // first) is preserved, matching the existing "first failure wins"
    // pattern below.
    ret2 = ret && reader->readDwgTables(hdr);
    if (ret && !ret2) {
        error = DRW::BAD_READ_TABLES;
        ret = ret2;
    }

    if (ret) {
    try {
    iface->addHeader(&hdr);
    } catch (...) {
        error = DRW::BAD_READ_HEADER;
        ret = false;
    }
    }

    if (ret) {
    try {
    for (auto it=reader->ltypemap.begin(); it!=reader->ltypemap.end(); ++it) {
        DRW_LType *lt = it->second;
        iface->addLType(const_cast<DRW_LType&>(*lt) );
    }
    for (auto it=reader->layermap.begin(); it!=reader->layermap.end(); ++it) {
        DRW_Layer *ly = it->second;
        iface->addLayer(const_cast<DRW_Layer&>(*ly));
    }

    for (auto it=reader->stylemap.begin(); it!=reader->stylemap.end(); ++it) {
        DRW_Textstyle *ly = it->second;
        iface->addTextStyle(const_cast<DRW_Textstyle&>(*ly));
    }

    for (auto it=reader->dimstylemap.begin(); it!=reader->dimstylemap.end(); ++it) {
        DRW_Dimstyle *ly = it->second;
        iface->addDimStyle(const_cast<DRW_Dimstyle&>(*ly));
    }

    for (auto it=reader->vportmap.begin(); it!=reader->vportmap.end(); ++it) {
        DRW_Vport *ly = it->second;
        iface->addVport(const_cast<DRW_Vport&>(*ly));
    }

    for (auto it=reader->appIdmap.begin(); it!=reader->appIdmap.end(); ++it) {
        DRW_AppId *ly = it->second;
        iface->addAppId(const_cast<DRW_AppId&>(*ly));
    }

    for (auto it=reader->viewmap.begin(); it!=reader->viewmap.end(); ++it) {
        DRW_View *vw = it->second;
        iface->addView(const_cast<DRW_View&>(*vw));
    }

    for (auto it=reader->ucsmap.begin(); it!=reader->ucsmap.end(); ++it) {
        DRW_UCS *u = it->second;
        iface->addUCS(const_cast<DRW_UCS&>(*u));
    }
    } catch (...) {
        error = DRW::BAD_READ_TABLES;
        ret = false;
    }
    if (ret && !reader->publishDeferredTableFramePublications(*iface)) {
        error = DRW::BAD_READ_TABLES;
        ret = false;
    }
    }

    if (ret) {
        try {
            ret2 = reader->readDwgBlocks(*iface);
        } catch (...) {
            error = DRW::BAD_READ_BLOCKS;
            ret = false;
        }
    }
    if (ret && !ret2) {
        error = DRW::BAD_READ_BLOCKS;
        ret = ret2;
    }

    if (ret) {
        try {
            ret2 = reader->readDwgEntities(*iface);
        } catch (...) {
            error = DRW::BAD_READ_ENTITIES;
            ret = false;
        }
    }
    if (ret && !ret2) {
        error = DRW::BAD_READ_ENTITIES;
        ret = ret2;
    }

    if (ret) {
        try {
            ret2 = reader->readDwgObjects(*iface);
        } catch (...) {
            error = DRW::BAD_READ_OBJECTS;
            ret = false;
        }
    }
    if (ret && !ret2) {
        error = DRW::BAD_READ_OBJECTS;
        ret = ret2;
    }

    if (ret) {
        try {
        for (const DRW_RawDwgSection& section : reader->m_rawDwgSections)
            iface->addRawDwgSection(section);
        for (const DRW_DataStorageSection& storage : reader->m_dataStorageSections)
            iface->addDataStorage(storage);
        } catch (...) {
            error = DRW::BAD_READ_OBJECTS;
            ret = false;
        }
    }

    finalizeClassCoverage();
    finalizeCoverage(ret);
    return ret;
    } catch (...) {
        if (error == DRW::BAD_NONE)
            error = DRW::BAD_READ_SECTION;
        finalizeClassCoverage();
        finalizeCoverage(false);
        return false;
    }
}
