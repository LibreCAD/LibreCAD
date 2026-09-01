/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011 Rallaz, rallazz@gmail.com
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2026 LibreCAD (librecad.org)
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License as published by the Free Software
** Foundation either version 2 of the License, or (at your option)
**  any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
* USA
**
**********************************************************************/

#ifndef RS_FILTERDXFRW_H
#define RS_FILTERDXFRW_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "drw_interface.h"
#include "lc_extentitydata.h"
#include "libdxfrw.h"
#ifdef DWGSUPPORT
#include "libdwgr.h"
#include <functional>
#endif
#include "rs_color.h"
#include "rs_dimension.h"
#include "rs_filterinterface.h"

class DL_WriterA;
class LC_DimStyle;
class LC_Hyperbola;
class LC_MLeader;
class LC_Parabola;
class LC_SplinePoints;
class LC_Tolerance;
class LC_Wipeout;
class RS_Arc;
class RS_Circle;
class RS_Ellipse;
class RS_Entity;
class RS_Hatch;
class RS_Image;
class RS_Insert;
class RS_Layer;
class RS_Leader;
class RS_Line;
class RS_MText;
class RS_Point;
class RS_Polyline;
class RS_Polyline;
class RS_Solid;
class RS_Spline;
class RS_Text;

struct DRW_AcisBrep;

#ifdef DWGSUPPORT
class DwgWriteFailureTestAccess;

/**
 * DWG-only identity state. Keeping these maps together makes their export
 * lifetime explicit without changing the established callback call sites.
 */
class RS_FilterDXFRW_DwgWriteIdentityRegistry {
  friend class DwgWriteFailureTestAccess;

protected:
  enum class DwgWriteSourceKind : std::uint8_t {
    Object,
    Entity,
    Block,
    FixedStructural,
    TableControl,
    TableRecord,
    RawCarrier,
    GeneratedChild
  };

  enum class DwgWriteIdentityState : std::uint8_t {
    Reserved,
    Selected,
    Committed,
    Suppressed
  };

  enum class DwgWriteIdentityLookup : std::uint8_t {
    Missing,
    Ambiguous,
    Reserved,
    Selected,
    Committed,
    Suppressed
  };

  enum class DwgWriteReferenceStatus : std::uint8_t {
    Resolved,
    LegacySpaceAlias,
    Missing,
    Ambiguous,
    Reserved,
    Selected,
    Committed,
    Suppressed,
    Unpaired
  };

  struct DwgWriteIdentityKey {
    DwgWriteSourceKind kind;
    std::uint32_t outputHandle;

    bool operator<(const DwgWriteIdentityKey &other) const {
      if (kind != other.kind)
        return static_cast<std::uint8_t>(kind) <
               static_cast<std::uint8_t>(other.kind);
      return outputHandle < other.outputHandle;
    }
  };

  struct DwgWriteSourceKey {
    DwgWriteSourceKind kind;
    std::uint32_t sourceHandle;
    std::uint32_t sourceOrdinal = 0;

    bool operator<(const DwgWriteSourceKey &other) const {
      if (kind != other.kind)
        return static_cast<std::uint8_t>(kind) <
               static_cast<std::uint8_t>(other.kind);
      if (sourceHandle != other.sourceHandle)
        return sourceHandle < other.sourceHandle;
      return sourceOrdinal < other.sourceOrdinal;
    }
  };

  struct DwgWriteIdentity {
    DwgWriteSourceKind kind;
    std::uint32_t sourceHandle;
    std::uint32_t outputHandle;
    DwgWriteIdentityState state = DwgWriteIdentityState::Reserved;
  };

  struct DwgClassFrameAdmission {
    DwgWriteSourceKind kind;
    std::uint32_t sourceHandle = 0;
    std::uint32_t sourceOrdinal = 0;
    std::uint32_t outputHandle = 0;
    std::uint16_t classNumber = 0;
    // The source key is intentionally split: a generated frame has no
    // source DWG handle and is identified by an explicit ordinal.
    enum class Lifecycle : std::uint8_t {
      SourceBacked,
      Generated
    } lifecycle = Lifecycle::SourceBacked;
    DRW::Version version = DRW::UNKNOWNV;
    std::uint32_t ownerHandle = DRW::NoHandle;
    std::uint64_t frameBitEstimate = 0;
    std::uint32_t expectedFrameCount = 1;
    bool committed = false;
  };

  enum class DwgFixedEntityRoute : std::uint8_t {
    Line,
    Point,
    Circle,
    Arc,
    PointExtrusion,
    LineExtrusion,
    Ray,
    Xline,
    Trace,
    Solid,
    Face3d
  };

  static bool isDwgFixedEntityWireType(DwgFixedEntityRoute route,
                                       std::uint16_t wireType) {
    switch (route) {
    case DwgFixedEntityRoute::Line:
    case DwgFixedEntityRoute::LineExtrusion:
      return wireType == dwgType::LINE;
    case DwgFixedEntityRoute::Point:
    case DwgFixedEntityRoute::PointExtrusion:
      return wireType == dwgType::POINT;
    case DwgFixedEntityRoute::Circle:
      return wireType == dwgType::CIRCLE;
    case DwgFixedEntityRoute::Arc:
      return wireType == dwgType::ARC;
    case DwgFixedEntityRoute::Ray:
      return wireType == dwgType::RAY;
    case DwgFixedEntityRoute::Xline:
      return wireType == dwgType::XLINE;
    case DwgFixedEntityRoute::Trace:
      return wireType == dwgType::TRACE;
    case DwgFixedEntityRoute::Solid:
      return wireType == dwgType::SOLID;
    case DwgFixedEntityRoute::Face3d:
      return wireType == dwgType::FACE3D;
    }
    return false;
  }

  struct DwgFixedEntityFrameReceipt {
    DwgFixedEntityRoute route = DwgFixedEntityRoute::Line;
    std::uint32_t sourceHandle = 0;
    std::uint32_t outputHandle = 0;
    std::uint16_t wireType = 0;
    DRW::Version version = DRW::UNKNOWNV;
    std::uint64_t frameGeneration = 0;
    std::uint32_t expectedFrameCount = 1;
    bool committed = false;
  };

  // Every source-backed route that emits exactly one entity frame is
  // declared before CLASSES.  This binds the later frame receipt to the
  // class/fixed-route admission that supplied its instance count.
  enum class DwgEntityWritePlanRoute : std::uint8_t {
    FixedSingleFrame,
    CustomSingleFrame
  };

  struct DwgEntityWritePlan {
    const RS_Entity *entity = nullptr;
    std::uint32_t sourceHandle = 0;
    std::uint32_t outputHandle = 0;
    DwgEntityWritePlanRoute route = DwgEntityWritePlanRoute::FixedSingleFrame;
    DwgFixedEntityRoute fixedRoute = DwgFixedEntityRoute::Line;
    std::uint16_t expectedFrameType = 0;
    DRW::Version version = DRW::UNKNOWNV;
    std::uint64_t frameGeneration = 0;
    bool admitted = false;
    bool committed = false;
  };

  struct DwgWriteOwnerReceipt {
    DwgWriteSourceKind producerKind;
    std::uint32_t producerSourceHandle = 0;
    std::uint32_t producerOutputHandle = 0;
    std::uint32_t sourceOwnerHandle = DRW::NoHandle;
    std::uint32_t outputOwnerHandle = 0;
    bool required = true;
  };

  struct DwgWriteReferenceReceipt {
    DwgWriteSourceKind producerKind;
    std::uint32_t producerSourceHandle = 0;
    std::uint32_t producerOutputHandle = 0;
    std::string referenceName;
    std::uint32_t referenceOrdinal = 0;
    DwgWriteSourceKind targetKind;
    std::uint32_t targetSourceHandle = 0;
    std::uint32_t targetOutputHandle = 0;
    bool required = true;
    std::uint8_t wireHandleCode = 0;
  };

  struct DwgWriteOptionalDropReceipt {
    DwgWriteSourceKind producerKind;
    std::uint32_t producerSourceHandle = 0;
    std::uint32_t producerOutputHandle = 0;
    std::string referenceName;
    std::uint32_t referenceOrdinal = 0;
    DwgWriteSourceKind targetKind;
    std::uint32_t targetSourceHandle = 0;
    DwgWriteReferenceStatus targetStatus = DwgWriteReferenceStatus::Missing;
    DRW::Version version = DRW::UNKNOWNV;
    std::string family;
    std::string reason;
  };

  struct DwgWriteIdentityCheckpoint {
    std::map<std::uint32_t, std::set<DwgWriteSourceKind>> sourceKinds;
    std::map<DwgWriteIdentityKey, DwgWriteIdentity> identitiesByOutput;
    std::map<DwgWriteSourceKey, std::set<std::uint32_t>> outputsBySource;
    std::map<std::uint32_t, DwgWriteIdentityKey> ownersByOutput;
    std::map<std::string, std::uint32_t> blockNamesByOutput;
    std::map<std::uint32_t, DRW::DwgObjectFrameReceipt>
        objectOccurrencesByOutput;
    std::map<DwgWriteSourceKey, DwgClassFrameAdmission>
        classFrameAdmissionsBySource;
    std::map<std::uint32_t, DwgWriteSourceKey> classFrameAdmissionsByOutput;
    std::map<DwgWriteSourceKey, DwgFixedEntityFrameReceipt>
        fixedEntityFrameReceiptsBySource;
    std::map<std::uint32_t, DwgWriteSourceKey> fixedEntityFrameReceiptsByOutput;
    std::map<const RS_Entity *, DwgEntityWritePlan> entityWritePlansByEntity;
    std::map<std::uint32_t, const RS_Entity *> entityWritePlansBySource;
    std::vector<DwgWriteOwnerReceipt> ownerReceipts;
    std::vector<DwgWriteReferenceReceipt> referenceReceipts;
    std::vector<DwgWriteOptionalDropReceipt> optionalDropReceipts;
    std::map<std::uint32_t, std::uint32_t> handleRemap;
    std::set<std::uint32_t> knownHandles;
    std::map<const RS_Entity *, std::uint32_t> entityHandlesByEntity;
    std::map<std::uint32_t, std::uint32_t> entityHandleRemap;
    std::set<std::uint32_t> entityHandlesEmitted;
    std::set<std::uint32_t> duplicateEntityHandles;
    std::set<std::uint32_t> lightHandlesPlanned;
    std::map<std::uint32_t, std::uint32_t> viewHandleRemap;
    std::map<std::uint32_t, std::uint32_t> ucsHandleRemap;
    std::map<std::uint32_t, std::uint32_t> vportHandleRemap;
    std::map<std::uint32_t, std::uint32_t> blockHandleRemap;
    std::set<std::uint32_t> duplicateBlockHandles;
    std::map<std::string, std::uint32_t> ltypeHandleByName;
    std::map<std::string, std::uint32_t> textStyleHandleByName;
  };

  DwgWriteIdentityCheckpoint checkpointDwgWriteIdentity() const {
    DwgWriteIdentityCheckpoint checkpoint;
    checkpoint.sourceKinds = m_dwgWriteSourceKinds;
    checkpoint.identitiesByOutput = m_dwgWriteIdentityByOutput;
    checkpoint.outputsBySource = m_dwgWriteIdentityOutputsBySource;
    checkpoint.ownersByOutput = m_dwgWriteIdentityOwnerByOutput;
    checkpoint.blockNamesByOutput = m_dwgWriteBlockHandleByName;
    checkpoint.objectOccurrencesByOutput = m_dwgWriteObjectOccurrencesByOutput;
    checkpoint.classFrameAdmissionsBySource =
        m_dwgWriteClassFrameAdmissionsBySource;
    checkpoint.classFrameAdmissionsByOutput =
        m_dwgWriteClassFrameAdmissionsByOutput;
    checkpoint.fixedEntityFrameReceiptsBySource =
        m_dwgWriteFixedEntityFrameReceiptsBySource;
    checkpoint.fixedEntityFrameReceiptsByOutput =
        m_dwgWriteFixedEntityFrameReceiptsByOutput;
    checkpoint.entityWritePlansByEntity = m_dwgWriteEntityPlansByEntity;
    checkpoint.entityWritePlansBySource = m_dwgWriteEntityPlansBySource;
    checkpoint.ownerReceipts = m_dwgWriteOwnerReceipts;
    checkpoint.referenceReceipts = m_dwgWriteReferenceReceipts;
    checkpoint.optionalDropReceipts = m_dwgWriteOptionalDropReceipts;
    checkpoint.handleRemap = m_dwgWriteHandleRemap;
    checkpoint.knownHandles = m_dwgWriteKnownHandles;
    checkpoint.entityHandlesByEntity = m_dwgWriteEntityHandleByEntity;
    checkpoint.entityHandleRemap = m_dwgWriteEntityHandleRemap;
    checkpoint.entityHandlesEmitted = m_dwgWriteEntityHandlesEmitted;
    checkpoint.duplicateEntityHandles = m_dwgWriteDuplicateEntityHandles;
    checkpoint.lightHandlesPlanned = m_dwgWriteLightHandlesPlanned;
    checkpoint.viewHandleRemap = m_dwgWriteViewHandleRemap;
    checkpoint.ucsHandleRemap = m_dwgWriteUcsHandleRemap;
    checkpoint.vportHandleRemap = m_dwgWriteVportHandleRemap;
    checkpoint.blockHandleRemap = m_dwgWriteBlockHandleRemap;
    checkpoint.duplicateBlockHandles = m_dwgWriteDuplicateBlockHandles;
    checkpoint.ltypeHandleByName = m_dwgWriteLTypeHandleByName;
    checkpoint.textStyleHandleByName = m_dwgWriteTextStyleHandleByName;
    return checkpoint;
  }

  void restoreDwgWriteIdentity(const DwgWriteIdentityCheckpoint &checkpoint) {
    m_dwgWriteSourceKinds = checkpoint.sourceKinds;
    m_dwgWriteIdentityByOutput = checkpoint.identitiesByOutput;
    m_dwgWriteIdentityOutputsBySource = checkpoint.outputsBySource;
    m_dwgWriteIdentityOwnerByOutput = checkpoint.ownersByOutput;
    m_dwgWriteBlockHandleByName = checkpoint.blockNamesByOutput;
    m_dwgWriteObjectOccurrencesByOutput = checkpoint.objectOccurrencesByOutput;
    m_dwgWriteClassFrameAdmissionsBySource =
        checkpoint.classFrameAdmissionsBySource;
    m_dwgWriteClassFrameAdmissionsByOutput =
        checkpoint.classFrameAdmissionsByOutput;
    m_dwgWriteFixedEntityFrameReceiptsBySource =
        checkpoint.fixedEntityFrameReceiptsBySource;
    m_dwgWriteFixedEntityFrameReceiptsByOutput =
        checkpoint.fixedEntityFrameReceiptsByOutput;
    m_dwgWriteEntityPlansByEntity = checkpoint.entityWritePlansByEntity;
    m_dwgWriteEntityPlansBySource = checkpoint.entityWritePlansBySource;
    m_dwgWriteOwnerReceipts = checkpoint.ownerReceipts;
    m_dwgWriteReferenceReceipts = checkpoint.referenceReceipts;
    m_dwgWriteOptionalDropReceipts = checkpoint.optionalDropReceipts;
    m_dwgWriteHandleRemap = checkpoint.handleRemap;
    m_dwgWriteKnownHandles = checkpoint.knownHandles;
    m_dwgWriteEntityHandleByEntity = checkpoint.entityHandlesByEntity;
    m_dwgWriteEntityHandleRemap = checkpoint.entityHandleRemap;
    m_dwgWriteEntityHandlesEmitted = checkpoint.entityHandlesEmitted;
    m_dwgWriteDuplicateEntityHandles = checkpoint.duplicateEntityHandles;
    m_dwgWriteLightHandlesPlanned = checkpoint.lightHandlesPlanned;
    m_dwgWriteViewHandleRemap = checkpoint.viewHandleRemap;
    m_dwgWriteUcsHandleRemap = checkpoint.ucsHandleRemap;
    m_dwgWriteVportHandleRemap = checkpoint.vportHandleRemap;
    m_dwgWriteBlockHandleRemap = checkpoint.blockHandleRemap;
    m_dwgWriteDuplicateBlockHandles = checkpoint.duplicateBlockHandles;
    m_dwgWriteLTypeHandleByName = checkpoint.ltypeHandleByName;
    m_dwgWriteTextStyleHandleByName = checkpoint.textStyleHandleByName;
  }

  void clearDwgWriteIdentity() {
    m_dwgWriteSourceKinds.clear();
    m_dwgWriteIdentityByOutput.clear();
    m_dwgWriteIdentityOutputsBySource.clear();
    m_dwgWriteIdentityOwnerByOutput.clear();
    m_dwgWriteBlockHandleByName.clear();
    m_dwgWriteObjectOccurrencesByOutput.clear();
    m_dwgWriteClassFrameAdmissionsBySource.clear();
    m_dwgWriteClassFrameAdmissionsByOutput.clear();
    m_dwgWriteFixedEntityFrameReceiptsBySource.clear();
    m_dwgWriteFixedEntityFrameReceiptsByOutput.clear();
    m_dwgWriteEntityPlansByEntity.clear();
    m_dwgWriteEntityPlansBySource.clear();
    m_dwgWriteHandleRemap.clear();
    m_dwgWriteKnownHandles.clear();
    m_dwgWriteEntityHandleByEntity.clear();
    m_dwgWriteEntityHandleRemap.clear();
    m_dwgWriteEntityHandlesEmitted.clear();
    m_dwgWriteDuplicateEntityHandles.clear();
    m_dwgWriteLightHandlesPlanned.clear();
    m_dwgWriteViewHandleRemap.clear();
    m_dwgWriteUcsHandleRemap.clear();
    m_dwgWriteVportHandleRemap.clear();
    m_dwgWriteBlockHandleRemap.clear();
    m_dwgWriteDuplicateBlockHandles.clear();
    m_dwgWriteOwnerReceipts.clear();
    m_dwgWriteReferenceReceipts.clear();
    m_dwgWriteOptionalDropReceipts.clear();
    m_dwgWriteLTypeHandleByName.clear();
    m_dwgWriteTextStyleHandleByName.clear();
  }

  bool reserveDwgWriteIdentity(DwgWriteSourceKind kind,
                               std::uint32_t sourceHandle,
                               std::uint32_t outputHandle) {
    if (outputHandle == 0)
      return false;

    const DwgWriteIdentityKey key{kind, outputHandle};
    const auto ownerIt = m_dwgWriteIdentityOwnerByOutput.find(outputHandle);
    if (ownerIt != m_dwgWriteIdentityOwnerByOutput.end() &&
        ownerIt->second < key) {
      return false;
    }
    if (ownerIt != m_dwgWriteIdentityOwnerByOutput.end() &&
        key < ownerIt->second) {
      return false;
    }

    const auto it = m_dwgWriteIdentityByOutput.find(key);
    if (it != m_dwgWriteIdentityByOutput.end())
      return it->second.sourceHandle == sourceHandle &&
             it->second.state != DwgWriteIdentityState::Suppressed;

    m_dwgWriteIdentityOwnerByOutput.emplace(outputHandle, key);
    m_dwgWriteIdentityByOutput.emplace(
        key, DwgWriteIdentity{kind, sourceHandle, outputHandle});
    m_dwgWriteIdentityOutputsBySource[{kind, sourceHandle}].insert(
        outputHandle);
    return true;
  }

  bool selectDwgWriteIdentity(DwgWriteSourceKind kind,
                              std::uint32_t sourceHandle,
                              std::uint32_t outputHandle) {
    if (!reserveDwgWriteIdentity(kind, sourceHandle, outputHandle))
      return false;
    const DwgWriteIdentityKey key{kind, outputHandle};
    auto it = m_dwgWriteIdentityByOutput.find(key);
    if (it == m_dwgWriteIdentityByOutput.end() ||
        it->second.state == DwgWriteIdentityState::Suppressed)
      return false;
    if (it->second.state == DwgWriteIdentityState::Reserved)
      it->second.state = DwgWriteIdentityState::Selected;
    return true;
  }

  bool commitDwgWriteIdentity(DwgWriteSourceKind kind,
                              std::uint32_t outputHandle) {
    const auto it = m_dwgWriteIdentityByOutput.find(
        DwgWriteIdentityKey{kind, outputHandle});
    if (it == m_dwgWriteIdentityByOutput.end() ||
        it->second.state == DwgWriteIdentityState::Reserved ||
        it->second.state == DwgWriteIdentityState::Suppressed)
      return false;
    it->second.state = DwgWriteIdentityState::Committed;
    return true;
  }

  bool registerDwgWriteFixedStructuralIdentity(std::uint32_t outputHandle) {
    if (outputHandle == 0 ||
        !reserveDwgWriteIdentity(DwgWriteSourceKind::FixedStructural, 0,
                                 outputHandle) ||
        !selectDwgWriteIdentity(DwgWriteSourceKind::FixedStructural, 0,
                                outputHandle))
      return false;
    return commitDwgWriteIdentity(DwgWriteSourceKind::FixedStructural,
                                  outputHandle);
  }

  bool rollbackDwgWriteIdentity(DwgWriteSourceKind kind,
                                std::uint32_t outputHandle) {
    const auto it = m_dwgWriteIdentityByOutput.find(
        DwgWriteIdentityKey{kind, outputHandle});
    if (it == m_dwgWriteIdentityByOutput.end())
      return false;
    if (it->second.state == DwgWriteIdentityState::Committed)
      return false;
    it->second.state = DwgWriteIdentityState::Suppressed;
    return true;
  }

  DwgWriteIdentityLookup
  lookupDwgWriteIdentity(DwgWriteSourceKind kind, std::uint32_t sourceHandle,
                         std::uint32_t *outputHandle = nullptr) const {
    if (outputHandle != nullptr)
      *outputHandle = 0;
    if (sourceHandle == 0)
      return DwgWriteIdentityLookup::Missing;

    const auto sourceIt = m_dwgWriteIdentityOutputsBySource.find(
        DwgWriteSourceKey{kind, sourceHandle});
    if (sourceIt == m_dwgWriteIdentityOutputsBySource.end())
      return DwgWriteIdentityLookup::Missing;

    const DwgWriteIdentity *identity = nullptr;
    bool suppressed = false;
    for (std::uint32_t candidate : sourceIt->second) {
      const auto it =
          m_dwgWriteIdentityByOutput.find(DwgWriteIdentityKey{kind, candidate});
      if (it == m_dwgWriteIdentityByOutput.end())
        continue;
      if (it->second.state == DwgWriteIdentityState::Suppressed) {
        suppressed = true;
        continue;
      }
      if (identity != nullptr)
        return DwgWriteIdentityLookup::Ambiguous;
      identity = &it->second;
    }
    if (identity == nullptr)
      return suppressed ? DwgWriteIdentityLookup::Suppressed
                        : DwgWriteIdentityLookup::Missing;
    if (outputHandle != nullptr)
      *outputHandle = identity->outputHandle;
    switch (identity->state) {
    case DwgWriteIdentityState::Reserved:
      return DwgWriteIdentityLookup::Reserved;
    case DwgWriteIdentityState::Selected:
      return DwgWriteIdentityLookup::Selected;
    case DwgWriteIdentityState::Committed:
      return DwgWriteIdentityLookup::Committed;
    case DwgWriteIdentityState::Suppressed:
      break;
    }
    return DwgWriteIdentityLookup::Missing;
  }

  bool isDwgWriteIdentityCommitted(DwgWriteSourceKind kind,
                                   std::uint32_t outputHandle) const {
    const auto it = m_dwgWriteIdentityByOutput.find(
        DwgWriteIdentityKey{kind, outputHandle});
    return it != m_dwgWriteIdentityByOutput.end() &&
           it->second.state == DwgWriteIdentityState::Committed;
  }

  bool isDwgWriteOutputCommitted(std::uint32_t outputHandle) const {
    const auto ownerIt = m_dwgWriteIdentityOwnerByOutput.find(outputHandle);
    return ownerIt != m_dwgWriteIdentityOwnerByOutput.end() &&
           isDwgWriteIdentityCommitted(ownerIt->second.kind, outputHandle);
  }

  bool
  recordDwgWriteObjectOccurrences(std::uint32_t outputHandle,
                                  const DRW::DwgObjectFrameReceipt &frame) {
    if (outputHandle == 0 || !frame.valid || frame.generation == 0 ||
        frame.objectHandle != outputHandle || frame.version == DRW::UNKNOWNV)
      return false;

    std::set<std::uint32_t> dataOrdinals;
    std::set<std::uint32_t> handleOrdinals;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> dataRanges;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> handleRanges;
    for (const DRW::DwgObjectHandleOccurrence &occurrence : frame.occurrences) {
      if (occurrence.objectHandle != outputHandle ||
          occurrence.token.startBit >= occurrence.token.endBit)
        return false;

      std::set<std::uint32_t> *ordinals = nullptr;
      std::vector<std::pair<std::uint64_t, std::uint64_t>> *ranges = nullptr;
      std::uint64_t segmentSize = 0;
      switch (occurrence.serializedSegment) {
      case DRW::DwgObjectSerializedSegment::Data:
        ordinals = &dataOrdinals;
        ranges = &dataRanges;
        segmentSize = frame.dataBitSize;
        break;
      case DRW::DwgObjectSerializedSegment::Handles:
        ordinals = &handleOrdinals;
        ranges = &handleRanges;
        segmentSize = frame.handleBitSize;
        break;
      default:
        return false;
      }
      if (occurrence.token.endBit > segmentSize ||
          !ordinals->insert(occurrence.segmentOrdinal).second)
        return false;
      ranges->emplace_back(occurrence.token.startBit, occurrence.token.endBit);
    }

    const auto rangesAreDisjoint = [](auto ranges) {
      std::sort(ranges.begin(), ranges.end());
      for (std::size_t index = 1; index < ranges.size(); ++index) {
        if (ranges[index - 1].second > ranges[index].first)
          return false;
      }
      return true;
    };
    if (!rangesAreDisjoint(dataRanges) || !rangesAreDisjoint(handleRanges))
      return false;

    return m_dwgWriteObjectOccurrencesByOutput.emplace(outputHandle, frame)
        .second;
  }

  bool rollbackDwgWriteObjectOccurrences(std::uint32_t outputHandle) {
    return m_dwgWriteObjectOccurrencesByOutput.erase(outputHandle) != 0;
  }

  const DRW::DwgObjectFrameReceipt *
  findDwgWriteObjectOccurrences(std::uint32_t outputHandle) const {
    const auto it = m_dwgWriteObjectOccurrencesByOutput.find(outputHandle);
    return it == m_dwgWriteObjectOccurrencesByOutput.end() ? nullptr
                                                           : &it->second;
  }

  bool admitDwgClassFrame(DwgWriteSourceKind kind, std::uint32_t sourceHandle,
                          std::uint32_t outputHandle, std::uint16_t classNumber,
                          std::uint32_t sourceOrdinal = 0) {
    return admitDwgClassFrameDescriptor(kind, sourceHandle, outputHandle,
                                        classNumber, sourceOrdinal,
                                        DRW::UNKNOWNV, DRW::NoHandle, 0);
  }

  // Record the immutable contract for one ordinary class-backed frame.
  // Legacy callers use admitDwgClassFrame() until their source row is
  // migrated; new callers should provide the target version explicitly.
  bool admitDwgClassFrameDescriptor(DwgWriteSourceKind kind,
                                    std::uint32_t sourceHandle,
                                    std::uint32_t outputHandle,
                                    std::uint16_t classNumber,
                                    std::uint32_t sourceOrdinal,
                                    DRW::Version version,
                                    std::uint32_t ownerHandle = DRW::NoHandle,
                                    std::uint64_t frameBitEstimate = 0) {
    if ((sourceHandle == 0) == (sourceOrdinal == 0) || classNumber == 0 ||
        (sourceHandle != 0 && outputHandle == 0))
      return false;

    const DwgClassFrameAdmission::Lifecycle lifecycle =
        sourceHandle == 0 ? DwgClassFrameAdmission::Lifecycle::Generated
                          : DwgClassFrameAdmission::Lifecycle::SourceBacked;
    if (frameBitEstimate >
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) *
            8u) {
      return false;
    }

    const DwgWriteSourceKey sourceKey{kind, sourceHandle, sourceOrdinal};
    const auto sourceIt =
        m_dwgWriteClassFrameAdmissionsBySource.find(sourceKey);
    if (sourceIt != m_dwgWriteClassFrameAdmissionsBySource.end()) {
      const DwgClassFrameAdmission &admission = sourceIt->second;
      return admission.outputHandle == outputHandle &&
             admission.classNumber == classNumber &&
             admission.sourceOrdinal == sourceOrdinal &&
             admission.lifecycle == lifecycle &&
             (version == DRW::UNKNOWNV || admission.version == version) &&
             (ownerHandle == DRW::NoHandle ||
              admission.ownerHandle == ownerHandle) &&
             (frameBitEstimate == 0 ||
              admission.frameBitEstimate == frameBitEstimate);
    }

    if (outputHandle != 0 &&
        m_dwgWriteClassFrameAdmissionsByOutput.find(outputHandle) !=
            m_dwgWriteClassFrameAdmissionsByOutput.end())
      return false;

    const DwgClassFrameAdmission admission{
        kind,      sourceHandle, sourceOrdinal, outputHandle,     classNumber,
        lifecycle, version,      ownerHandle,   frameBitEstimate, 1,
        false};
    m_dwgWriteClassFrameAdmissionsBySource.emplace(sourceKey, admission);
    if (outputHandle != 0)
      m_dwgWriteClassFrameAdmissionsByOutput.emplace(outputHandle, sourceKey);
    return true;
  }

  bool getDwgClassFrameDescriptor(DwgWriteSourceKind kind,
                                  std::uint32_t sourceHandle,
                                  std::uint32_t sourceOrdinal,
                                  DwgClassFrameAdmission &descriptor) const {
    const auto it = m_dwgWriteClassFrameAdmissionsBySource.find(
        DwgWriteSourceKey{kind, sourceHandle, sourceOrdinal});
    if (it == m_dwgWriteClassFrameAdmissionsBySource.end())
      return false;
    descriptor = it->second;
    return true;
  }

  bool bindDwgClassFrameOutput(DwgWriteSourceKind kind,
                               std::uint32_t sourceHandle,
                               std::uint32_t sourceOrdinal,
                               std::uint32_t outputHandle) {
    if (sourceOrdinal == 0 || outputHandle == 0)
      return false;
    const DwgWriteSourceKey sourceKey{kind, sourceHandle, sourceOrdinal};
    const auto sourceIt =
        m_dwgWriteClassFrameAdmissionsBySource.find(sourceKey);
    if (sourceIt == m_dwgWriteClassFrameAdmissionsBySource.end())
      return false;
    auto &admission = sourceIt->second;
    if (admission.outputHandle != 0)
      return admission.outputHandle == outputHandle;
    if (m_dwgWriteClassFrameAdmissionsByOutput.find(outputHandle) !=
        m_dwgWriteClassFrameAdmissionsByOutput.end())
      return false;
    admission.outputHandle = outputHandle;
    m_dwgWriteClassFrameAdmissionsByOutput.emplace(outputHandle, sourceKey);
    return true;
  }

  bool findDwgClassFrameAdmission(DwgWriteSourceKind kind,
                                  std::uint32_t sourceHandle,
                                  std::uint32_t outputHandle,
                                  std::uint16_t &classNumber, bool &committed,
                                  std::uint32_t sourceOrdinal = 0) const {
    classNumber = 0;
    committed = false;
    const auto it = m_dwgWriteClassFrameAdmissionsBySource.find(
        DwgWriteSourceKey{kind, sourceHandle, sourceOrdinal});
    if (it == m_dwgWriteClassFrameAdmissionsBySource.end() ||
        (outputHandle != 0 && it->second.outputHandle != outputHandle))
      return false;
    classNumber = it->second.classNumber;
    committed = it->second.committed;
    return true;
  }

  bool commitDwgClassFrame(DwgWriteSourceKind kind, std::uint32_t sourceHandle,
                           std::uint32_t outputHandle,
                           std::uint16_t classNumber,
                           std::uint32_t sourceOrdinal = 0) {
    const auto it = m_dwgWriteClassFrameAdmissionsBySource.find(
        DwgWriteSourceKey{kind, sourceHandle, sourceOrdinal});
    if (it == m_dwgWriteClassFrameAdmissionsBySource.end() ||
        it->second.outputHandle != outputHandle ||
        it->second.classNumber != classNumber ||
        it->second.sourceOrdinal != sourceOrdinal)
      return false;
    it->second.committed = true;
    return true;
  }

  bool validateDwgClassFrameAdmissions() const {
    if (m_dwgWriteClassFrameAdmissionsBySource.size() !=
        m_dwgWriteClassFrameAdmissionsByOutput.size())
      return false;
    for (const auto &entry : m_dwgWriteClassFrameAdmissionsBySource) {
      const DwgWriteSourceKey &sourceKey = entry.first;
      const DwgClassFrameAdmission &admission = entry.second;
      if (!admission.committed || admission.kind != sourceKey.kind ||
          admission.sourceHandle != sourceKey.sourceHandle ||
          admission.sourceOrdinal != sourceKey.sourceOrdinal ||
          admission.outputHandle == 0 || admission.classNumber == 0 ||
          admission.expectedFrameCount != 1 ||
          (admission.lifecycle ==
                   DwgClassFrameAdmission::Lifecycle::SourceBacked
               ? admission.sourceHandle == 0 || admission.sourceOrdinal != 0 ||
                     admission.outputHandle == 0
               : admission.sourceHandle != 0 || admission.sourceOrdinal == 0))
        return false;
      const auto outputIt =
          m_dwgWriteClassFrameAdmissionsByOutput.find(admission.outputHandle);
      if (outputIt == m_dwgWriteClassFrameAdmissionsByOutput.end() ||
          outputIt->second < sourceKey || sourceKey < outputIt->second)
        return false;
      if (admission.sourceHandle != 0 &&
          !isDwgWriteIdentityCommitted(admission.kind, admission.outputHandle))
        return false;
      const auto frameIt =
          m_dwgWriteObjectOccurrencesByOutput.find(admission.outputHandle);
      if (frameIt == m_dwgWriteObjectOccurrencesByOutput.end() ||
          !frameIt->second.valid ||
          frameIt->second.objectHandle != admission.outputHandle ||
          frameIt->second.classNumber != admission.classNumber)
        return false;
    }
    return true;
  }

  bool admitDwgFixedEntityFrame(DwgFixedEntityRoute route,
                                std::uint32_t sourceHandle,
                                std::uint32_t outputHandle,
                                std::uint16_t wireType, DRW::Version version) {
    if (!isDwgFixedEntityWireType(route, wireType) || sourceHandle == 0 ||
        outputHandle == 0 || wireType == 0 || version == DRW::UNKNOWNV)
      return false;

    const DwgWriteSourceKey sourceKey{DwgWriteSourceKind::Entity, sourceHandle,
                                      0};
    if (m_dwgWriteClassFrameAdmissionsBySource.find(sourceKey) !=
        m_dwgWriteClassFrameAdmissionsBySource.end())
      return false;

    const auto sourceIt =
        m_dwgWriteFixedEntityFrameReceiptsBySource.find(sourceKey);
    if (sourceIt != m_dwgWriteFixedEntityFrameReceiptsBySource.end()) {
      const auto &receipt = sourceIt->second;
      return receipt.route == route && receipt.outputHandle == outputHandle &&
             receipt.wireType == wireType && receipt.version == version;
    }

    if (m_dwgWriteFixedEntityFrameReceiptsByOutput.find(outputHandle) !=
        m_dwgWriteFixedEntityFrameReceiptsByOutput.end())
      return false;

    const DwgFixedEntityFrameReceipt receipt{
        route, sourceHandle, outputHandle, wireType, version, 0, 1, false};
    m_dwgWriteFixedEntityFrameReceiptsBySource.emplace(sourceKey, receipt);
    m_dwgWriteFixedEntityFrameReceiptsByOutput.emplace(outputHandle, sourceKey);
    return true;
  }

  bool commitDwgFixedEntityFrame(DwgFixedEntityRoute route,
                                 std::uint32_t sourceHandle,
                                 std::uint32_t outputHandle,
                                 const DRW::DwgObjectFrameReceipt &frame) {
    const DwgWriteSourceKey sourceKey{DwgWriteSourceKind::Entity, sourceHandle,
                                      0};
    const auto sourceIt =
        m_dwgWriteFixedEntityFrameReceiptsBySource.find(sourceKey);
    if (sourceIt == m_dwgWriteFixedEntityFrameReceiptsBySource.end())
      return false;
    auto &receipt = sourceIt->second;
    if (receipt.route != route || receipt.outputHandle != outputHandle ||
        receipt.committed || !frame.valid ||
        frame.objectHandle != outputHandle ||
        frame.version != receipt.version ||
        frame.classNumber != receipt.wireType || frame.generation == 0)
      return false;
    const auto outputIt =
        m_dwgWriteFixedEntityFrameReceiptsByOutput.find(outputHandle);
    if (outputIt == m_dwgWriteFixedEntityFrameReceiptsByOutput.end() ||
        outputIt->second < sourceKey || sourceKey < outputIt->second)
      return false;
    receipt.frameGeneration = frame.generation;
    receipt.committed = true;
    return true;
  }

  bool validateDwgFixedEntityFrameReceipts() const {
    if (m_dwgWriteFixedEntityFrameReceiptsBySource.size() !=
        m_dwgWriteFixedEntityFrameReceiptsByOutput.size())
      return false;
    for (const auto &entry : m_dwgWriteFixedEntityFrameReceiptsBySource) {
      const auto &sourceKey = entry.first;
      const auto &receipt = entry.second;
      if (sourceKey.kind != DwgWriteSourceKind::Entity ||
          sourceKey.sourceHandle != receipt.sourceHandle ||
          sourceKey.sourceOrdinal != 0 ||
          !isDwgFixedEntityWireType(receipt.route, receipt.wireType) ||
          receipt.outputHandle == 0 || receipt.wireType == 0 ||
          receipt.version == DRW::UNKNOWNV || receipt.frameGeneration == 0 ||
          receipt.expectedFrameCount != 1 || !receipt.committed)
        return false;
      const auto outputIt =
          m_dwgWriteFixedEntityFrameReceiptsByOutput.find(receipt.outputHandle);
      if (outputIt == m_dwgWriteFixedEntityFrameReceiptsByOutput.end() ||
          outputIt->second < sourceKey || sourceKey < outputIt->second ||
          !isDwgWriteIdentityCommitted(DwgWriteSourceKind::Entity,
                                       receipt.outputHandle))
        return false;
      const auto frameIt =
          m_dwgWriteObjectOccurrencesByOutput.find(receipt.outputHandle);
      if (frameIt == m_dwgWriteObjectOccurrencesByOutput.end() ||
          !frameIt->second.valid ||
          frameIt->second.generation != receipt.frameGeneration ||
          frameIt->second.objectHandle != receipt.outputHandle ||
          frameIt->second.version != receipt.version ||
          frameIt->second.classNumber != receipt.wireType)
        return false;
    }
    return true;
  }

  bool stageDwgEntityWritePlan(const RS_Entity *entity,
                               std::uint32_t sourceHandle,
                               std::uint32_t outputHandle,
                               DwgEntityWritePlanRoute route,
                               DwgFixedEntityRoute fixedRoute,
                               std::uint16_t expectedFrameType,
                               DRW::Version version) {
    if (entity == nullptr || sourceHandle == 0 || outputHandle == 0 ||
        expectedFrameType == 0 || version == DRW::UNKNOWNV ||
        (route == DwgEntityWritePlanRoute::CustomSingleFrame &&
         (expectedFrameType < 500 || version < DRW::AC1015))) {
      return false;
    }

    std::uint32_t selectedOutputHandle = 0;
    const DwgWriteIdentityLookup identityState = lookupDwgWriteIdentity(
        DwgWriteSourceKind::Entity, sourceHandle, &selectedOutputHandle);
    if ((identityState != DwgWriteIdentityLookup::Selected &&
         identityState != DwgWriteIdentityLookup::Committed) ||
        selectedOutputHandle != outputHandle) {
      return false;
    }

    const auto sourceIt = m_dwgWriteEntityPlansBySource.find(sourceHandle);
    if (sourceIt != m_dwgWriteEntityPlansBySource.end() &&
        sourceIt->second != entity) {
      return false;
    }
    const auto planIt = m_dwgWriteEntityPlansByEntity.find(entity);
    if (planIt != m_dwgWriteEntityPlansByEntity.end()) {
      const DwgEntityWritePlan &plan = planIt->second;
      return plan.sourceHandle == sourceHandle &&
             plan.outputHandle == outputHandle && plan.route == route &&
             plan.fixedRoute == fixedRoute &&
             plan.expectedFrameType == expectedFrameType &&
             plan.version == version;
    }

    const DwgEntityWritePlan plan{entity,       sourceHandle,
                                  outputHandle, route,
                                  fixedRoute,   expectedFrameType,
                                  version,      0,
                                  false,        false};
    m_dwgWriteEntityPlansByEntity.emplace(entity, plan);
    m_dwgWriteEntityPlansBySource.emplace(sourceHandle, entity);
    return true;
  }

  DwgEntityWritePlan *findDwgEntityWritePlan(const RS_Entity *entity) {
    const auto it = m_dwgWriteEntityPlansByEntity.find(entity);
    return it == m_dwgWriteEntityPlansByEntity.end() ? nullptr : &it->second;
  }

  const DwgEntityWritePlan *
  findDwgEntityWritePlan(const RS_Entity *entity) const {
    const auto it = m_dwgWriteEntityPlansByEntity.find(entity);
    return it == m_dwgWriteEntityPlansByEntity.end() ? nullptr : &it->second;
  }

  bool admitDwgEntityWritePlan(const RS_Entity *entity) {
    DwgEntityWritePlan *plan = findDwgEntityWritePlan(entity);
    if (plan == nullptr || plan->committed)
      return false;
    plan->admitted = true;
    return true;
  }

  bool commitDwgEntityWritePlan(const RS_Entity *entity,
                                const DRW::DwgObjectFrameReceipt &frame) {
    DwgEntityWritePlan *plan = findDwgEntityWritePlan(entity);
    if (plan == nullptr || !plan->admitted || plan->committed || !frame.valid ||
        frame.generation == 0 || frame.objectHandle != plan->outputHandle ||
        frame.classNumber != plan->expectedFrameType ||
        frame.version != plan->version) {
      return false;
    }
    const auto sourceIt =
        m_dwgWriteEntityPlansBySource.find(plan->sourceHandle);
    if (sourceIt == m_dwgWriteEntityPlansBySource.end() ||
        sourceIt->second != entity) {
      return false;
    }
    plan->frameGeneration = frame.generation;
    plan->committed = true;
    return true;
  }

  bool validateDwgEntityWritePlans() const {
    if (m_dwgWriteEntityPlansByEntity.size() !=
        m_dwgWriteEntityPlansBySource.size()) {
      return false;
    }
    for (const auto &entry : m_dwgWriteEntityPlansByEntity) {
      const RS_Entity *entity = entry.first;
      const DwgEntityWritePlan &plan = entry.second;
      if (entity == nullptr || plan.entity != entity ||
          plan.sourceHandle == 0 || plan.outputHandle == 0 ||
          plan.expectedFrameType == 0 || plan.version == DRW::UNKNOWNV ||
          !plan.admitted || !plan.committed || plan.frameGeneration == 0 ||
          (plan.route == DwgEntityWritePlanRoute::CustomSingleFrame &&
           (plan.expectedFrameType < 500 || plan.version < DRW::AC1015)) ||
          !isDwgWriteIdentityCommitted(DwgWriteSourceKind::Entity,
                                       plan.outputHandle)) {
        return false;
      }
      const auto sourceIt =
          m_dwgWriteEntityPlansBySource.find(plan.sourceHandle);
      if (sourceIt == m_dwgWriteEntityPlansBySource.end() ||
          sourceIt->second != entity) {
        return false;
      }
      const auto frameIt =
          m_dwgWriteObjectOccurrencesByOutput.find(plan.outputHandle);
      if (frameIt == m_dwgWriteObjectOccurrencesByOutput.end() ||
          !frameIt->second.valid ||
          frameIt->second.generation != plan.frameGeneration ||
          frameIt->second.objectHandle != plan.outputHandle ||
          frameIt->second.classNumber != plan.expectedFrameType ||
          frameIt->second.version != plan.version) {
        return false;
      }
      if (plan.route == DwgEntityWritePlanRoute::FixedSingleFrame) {
        const auto receiptIt =
            m_dwgWriteFixedEntityFrameReceiptsBySource.find(DwgWriteSourceKey{
                DwgWriteSourceKind::Entity, plan.sourceHandle, 0});
        if (receiptIt == m_dwgWriteFixedEntityFrameReceiptsBySource.end() ||
            receiptIt->second.route != plan.fixedRoute ||
            receiptIt->second.outputHandle != plan.outputHandle ||
            receiptIt->second.wireType != plan.expectedFrameType ||
            receiptIt->second.version != plan.version ||
            !receiptIt->second.committed) {
          return false;
        }
      } else {
        const auto admissionIt =
            m_dwgWriteClassFrameAdmissionsBySource.find(DwgWriteSourceKey{
                DwgWriteSourceKind::Entity, plan.sourceHandle, 0});
        if (admissionIt == m_dwgWriteClassFrameAdmissionsBySource.end() ||
            admissionIt->second.outputHandle != plan.outputHandle ||
            admissionIt->second.classNumber != plan.expectedFrameType ||
            admissionIt->second.version != plan.version ||
            !admissionIt->second.committed) {
          return false;
        }
      }
    }
    return true;
  }

  bool bindDwgWriteBlockName(const std::string &normalizedName,
                             std::uint32_t outputHandle) {
    if (normalizedName.empty() || outputHandle == 0)
      return false;
    const auto [it, inserted] =
        m_dwgWriteBlockHandleByName.emplace(normalizedName, outputHandle);
    return inserted || it->second == outputHandle;
  }

  DwgWriteReferenceStatus
  resolveDwgWriteBlockName(const std::string &normalizedName,
                           std::uint32_t *outputHandle = nullptr) const {
    if (outputHandle != nullptr)
      *outputHandle = 0;

    if (normalizedName == "*MODEL_SPACE") {
      if (outputHandle != nullptr)
        *outputHandle = DRW::DwgModelSpaceBlockRecordHandle;
      return DwgWriteReferenceStatus::LegacySpaceAlias;
    }
    if (normalizedName == "*PAPER_SPACE") {
      if (outputHandle != nullptr)
        *outputHandle = DRW::DwgPaperSpaceBlockRecordHandle;
      return DwgWriteReferenceStatus::LegacySpaceAlias;
    }

    const auto bindingIt = m_dwgWriteBlockHandleByName.find(normalizedName);
    if (bindingIt == m_dwgWriteBlockHandleByName.end())
      return DwgWriteReferenceStatus::Missing;
    const std::uint32_t boundHandle = bindingIt->second;
    if (outputHandle != nullptr)
      *outputHandle = boundHandle;

    const auto ownerIt = m_dwgWriteIdentityOwnerByOutput.find(boundHandle);
    if (ownerIt == m_dwgWriteIdentityOwnerByOutput.end())
      return DwgWriteReferenceStatus::Resolved;
    if (ownerIt->second.kind != DwgWriteSourceKind::Block)
      return DwgWriteReferenceStatus::Ambiguous;
    const auto identityIt = m_dwgWriteIdentityByOutput.find(ownerIt->second);
    if (identityIt == m_dwgWriteIdentityByOutput.end())
      return DwgWriteReferenceStatus::Missing;
    switch (identityIt->second.state) {
    case DwgWriteIdentityState::Reserved:
      return DwgWriteReferenceStatus::Reserved;
    case DwgWriteIdentityState::Selected:
      return DwgWriteReferenceStatus::Selected;
    case DwgWriteIdentityState::Committed:
      return DwgWriteReferenceStatus::Committed;
    case DwgWriteIdentityState::Suppressed:
      return DwgWriteReferenceStatus::Suppressed;
    }
    return DwgWriteReferenceStatus::Missing;
  }

  std::uint32_t sourceDwgWriteIdentityHandle(DwgWriteSourceKind kind,
                                             std::uint32_t outputHandle) const {
    const auto ownerIt = m_dwgWriteIdentityOwnerByOutput.find(outputHandle);
    if (ownerIt == m_dwgWriteIdentityOwnerByOutput.end() ||
        ownerIt->second.kind != kind)
      return 0;
    const auto identityIt = m_dwgWriteIdentityByOutput.find(ownerIt->second);
    return identityIt == m_dwgWriteIdentityByOutput.end()
               ? 0
               : identityIt->second.sourceHandle;
  }

  DwgWriteReferenceStatus
  resolveDwgWriteBlockOwner(std::uint32_t sourceHandle,
                            std::uint32_t *outputHandle = nullptr) const {
    if (outputHandle != nullptr)
      *outputHandle = 0;

    if (sourceHandle == DRW::NoHandle ||
        sourceHandle == DRW::DxfModelSpaceBlockRecordHandle ||
        sourceHandle == DRW::DwgModelSpaceBlockRecordHandle) {
      if (outputHandle != nullptr)
        *outputHandle = DRW::DwgModelSpaceBlockRecordHandle;
      return DwgWriteReferenceStatus::LegacySpaceAlias;
    }
    if (sourceHandle == DRW::DxfPaperSpaceBlockRecordHandle ||
        sourceHandle == DRW::DwgPaperSpaceBlockRecordHandle) {
      if (outputHandle != nullptr)
        *outputHandle = DRW::DwgPaperSpaceBlockRecordHandle;
      return DwgWriteReferenceStatus::LegacySpaceAlias;
    }
    if (m_dwgWriteDuplicateBlockHandles.count(sourceHandle) != 0)
      return DwgWriteReferenceStatus::Ambiguous;

    std::uint32_t resolvedHandle = 0;
    switch (lookupDwgWriteIdentity(DwgWriteSourceKind::Block, sourceHandle,
                                   &resolvedHandle)) {
    case DwgWriteIdentityLookup::Missing:
      return DwgWriteReferenceStatus::Missing;
    case DwgWriteIdentityLookup::Ambiguous:
      return DwgWriteReferenceStatus::Ambiguous;
    case DwgWriteIdentityLookup::Reserved:
      break;
    case DwgWriteIdentityLookup::Selected:
      if (outputHandle != nullptr)
        *outputHandle = resolvedHandle;
      return DwgWriteReferenceStatus::Selected;
    case DwgWriteIdentityLookup::Committed:
      if (outputHandle != nullptr)
        *outputHandle = resolvedHandle;
      return DwgWriteReferenceStatus::Committed;
    case DwgWriteIdentityLookup::Suppressed:
      return DwgWriteReferenceStatus::Suppressed;
    }
    if (outputHandle != nullptr)
      *outputHandle = resolvedHandle;
    return DwgWriteReferenceStatus::Reserved;
  }

  bool recordDwgWriteOwnerReceipt(const DwgWriteOwnerReceipt &receipt) {
    if (receipt.producerSourceHandle == 0 ||
        receipt.producerOutputHandle == 0 || receipt.outputOwnerHandle == 0)
      return false;

    for (const DwgWriteOwnerReceipt &existing : m_dwgWriteOwnerReceipts) {
      const bool sameReceipt =
          existing.producerKind == receipt.producerKind &&
          existing.producerSourceHandle == receipt.producerSourceHandle &&
          existing.producerOutputHandle == receipt.producerOutputHandle &&
          existing.sourceOwnerHandle == receipt.sourceOwnerHandle &&
          existing.outputOwnerHandle == receipt.outputOwnerHandle &&
          existing.required == receipt.required;
      if (sameReceipt)
        return true;

      const bool sameProducer =
          existing.producerKind == receipt.producerKind &&
          existing.producerSourceHandle == receipt.producerSourceHandle &&
          existing.producerOutputHandle == receipt.producerOutputHandle;
      if (sameProducer)
        return false;
    }
    m_dwgWriteOwnerReceipts.push_back(receipt);
    return true;
  }

  bool rollbackDwgWriteOwnerReceipt(const DwgWriteOwnerReceipt &receipt) {
    for (auto it = m_dwgWriteOwnerReceipts.begin();
         it != m_dwgWriteOwnerReceipts.end(); ++it) {
      if (it->producerKind == receipt.producerKind &&
          it->producerSourceHandle == receipt.producerSourceHandle &&
          it->producerOutputHandle == receipt.producerOutputHandle &&
          it->sourceOwnerHandle == receipt.sourceOwnerHandle &&
          it->outputOwnerHandle == receipt.outputOwnerHandle &&
          it->required == receipt.required) {
        m_dwgWriteOwnerReceipts.erase(it);
        return true;
      }
    }
    return false;
  }

  bool recordDwgWriteReferenceReceipt(const DwgWriteReferenceReceipt &receipt) {
    if ((receipt.producerSourceHandle == 0 &&
         receipt.producerKind != DwgWriteSourceKind::Entity) ||
        receipt.producerOutputHandle == 0 || receipt.referenceName.empty())
      return false;
    if (receipt.required && receipt.targetOutputHandle == 0)
      return false;

    for (const DwgWriteReferenceReceipt &existing :
         m_dwgWriteReferenceReceipts) {
      const bool sameReceipt =
          existing.producerKind == receipt.producerKind &&
          existing.producerSourceHandle == receipt.producerSourceHandle &&
          existing.producerOutputHandle == receipt.producerOutputHandle &&
          existing.referenceName == receipt.referenceName &&
          existing.referenceOrdinal == receipt.referenceOrdinal &&
          existing.targetKind == receipt.targetKind &&
          existing.targetSourceHandle == receipt.targetSourceHandle &&
          existing.targetOutputHandle == receipt.targetOutputHandle &&
          existing.required == receipt.required &&
          existing.wireHandleCode == receipt.wireHandleCode;
      if (sameReceipt)
        return true;

      const bool sameReference =
          existing.producerKind == receipt.producerKind &&
          existing.producerSourceHandle == receipt.producerSourceHandle &&
          existing.producerOutputHandle == receipt.producerOutputHandle &&
          existing.referenceName == receipt.referenceName &&
          existing.referenceOrdinal == receipt.referenceOrdinal &&
          existing.wireHandleCode == receipt.wireHandleCode;
      if (sameReference)
        return false;
    }
    m_dwgWriteReferenceReceipts.push_back(receipt);
    return true;
  }

  bool
  rollbackDwgWriteReferenceReceipt(const DwgWriteReferenceReceipt &receipt) {
    for (auto it = m_dwgWriteReferenceReceipts.begin();
         it != m_dwgWriteReferenceReceipts.end(); ++it) {
      if (it->producerKind == receipt.producerKind &&
          it->producerSourceHandle == receipt.producerSourceHandle &&
          it->producerOutputHandle == receipt.producerOutputHandle &&
          it->referenceName == receipt.referenceName &&
          it->referenceOrdinal == receipt.referenceOrdinal &&
          it->targetKind == receipt.targetKind &&
          it->targetSourceHandle == receipt.targetSourceHandle &&
          it->targetOutputHandle == receipt.targetOutputHandle &&
          it->required == receipt.required &&
          it->wireHandleCode == receipt.wireHandleCode) {
        m_dwgWriteReferenceReceipts.erase(it);
        return true;
      }
    }
    return false;
  }

  bool validateDwgWriteReferenceReceipts() const {
    for (const DwgWriteReferenceReceipt &receipt :
         m_dwgWriteReferenceReceipts) {
      if (!isDwgWriteOutputCommitted(receipt.producerOutputHandle))
        return false;
      if (!receipt.required && receipt.targetOutputHandle == 0)
        continue;
      if (receipt.targetOutputHandle == 0)
        return false;

      if (receipt.targetSourceHandle == 0) {
        const auto ownerIt =
            m_dwgWriteIdentityOwnerByOutput.find(receipt.targetOutputHandle);
        if (ownerIt == m_dwgWriteIdentityOwnerByOutput.end() ||
            ownerIt->second.kind != receipt.targetKind ||
            !isDwgWriteIdentityCommitted(ownerIt->second.kind,
                                         receipt.targetOutputHandle)) {
          return false;
        }
        continue;
      }

      std::uint32_t resolvedTarget = 0;
      if (lookupDwgWriteIdentity(receipt.targetKind, receipt.targetSourceHandle,
                                 &resolvedTarget) !=
              DwgWriteIdentityLookup::Committed ||
          resolvedTarget != receipt.targetOutputHandle) {
        return false;
      }
    }
    return true;
  }

  bool recordDwgWriteOptionalDropReceipt(
      const DwgWriteOptionalDropReceipt &receipt) {
    if ((receipt.producerSourceHandle == 0 &&
         receipt.producerKind != DwgWriteSourceKind::Entity) ||
        receipt.producerOutputHandle == 0 || receipt.referenceName.empty() ||
        receipt.targetSourceHandle == 0 || receipt.version == DRW::UNKNOWNV ||
        receipt.family.empty() || receipt.reason.empty() ||
        receipt.targetStatus == DwgWriteReferenceStatus::Resolved ||
        receipt.targetStatus == DwgWriteReferenceStatus::LegacySpaceAlias ||
        receipt.targetStatus == DwgWriteReferenceStatus::Reserved ||
        receipt.targetStatus == DwgWriteReferenceStatus::Selected ||
        receipt.targetStatus == DwgWriteReferenceStatus::Committed)
      return false;

    for (const DwgWriteOptionalDropReceipt &existing :
         m_dwgWriteOptionalDropReceipts) {
      const bool sameReceipt =
          existing.producerKind == receipt.producerKind &&
          existing.producerSourceHandle == receipt.producerSourceHandle &&
          existing.producerOutputHandle == receipt.producerOutputHandle &&
          existing.referenceName == receipt.referenceName &&
          existing.referenceOrdinal == receipt.referenceOrdinal &&
          existing.targetKind == receipt.targetKind &&
          existing.targetSourceHandle == receipt.targetSourceHandle &&
          existing.targetStatus == receipt.targetStatus &&
          existing.version == receipt.version &&
          existing.family == receipt.family &&
          existing.reason == receipt.reason;
      if (sameReceipt)
        return true;

      const bool sameReference =
          existing.producerKind == receipt.producerKind &&
          existing.producerSourceHandle == receipt.producerSourceHandle &&
          existing.producerOutputHandle == receipt.producerOutputHandle &&
          existing.referenceName == receipt.referenceName &&
          existing.referenceOrdinal == receipt.referenceOrdinal;
      if (sameReference)
        return false;
    }
    m_dwgWriteOptionalDropReceipts.push_back(receipt);
    return true;
  }

  bool recordDwgWriteOptionalDrop(
      DwgWriteSourceKind producerKind, std::uint32_t producerSourceHandle,
      std::uint32_t producerOutputHandle, const char *referenceName,
      std::uint32_t referenceOrdinal, DwgWriteSourceKind targetKind,
      std::uint32_t targetSourceHandle, DwgWriteReferenceStatus targetStatus,
      DRW::Version version, const char *family, const char *reason) {
    if (referenceName == nullptr || family == nullptr || reason == nullptr)
      return false;
    return recordDwgWriteOptionalDropReceipt(
        {producerKind, producerSourceHandle, producerOutputHandle,
         referenceName, referenceOrdinal, targetKind, targetSourceHandle,
         targetStatus, version, family, reason});
  }

  std::size_t dwgWriteOptionalReferenceDropCount() const {
    return m_dwgWriteOptionalDropReceipts.size();
  }

  bool validateDwgWriteOptionalDropReceipts() const {
    for (const DwgWriteOptionalDropReceipt &receipt :
         m_dwgWriteOptionalDropReceipts) {
      if (!isDwgWriteOutputCommitted(receipt.producerOutputHandle) ||
          receipt.targetSourceHandle == 0 || receipt.version == DRW::UNKNOWNV ||
          receipt.family.empty() || receipt.reason.empty()) {
        return false;
      }
      if (receipt.targetStatus != DwgWriteReferenceStatus::Missing &&
          receipt.targetStatus != DwgWriteReferenceStatus::Ambiguous &&
          receipt.targetStatus != DwgWriteReferenceStatus::Suppressed &&
          receipt.targetStatus != DwgWriteReferenceStatus::Unpaired) {
        return false;
      }
    }
    return true;
  }

  bool validateDwgWriteOwnerReceipts(
      const DRW::DwgBlockWriteResult *published = nullptr) const {
    for (const DwgWriteOwnerReceipt &receipt : m_dwgWriteOwnerReceipts) {
      if (!receipt.required)
        continue;
      if (!isDwgWriteOutputCommitted(receipt.producerOutputHandle) ||
          !isDwgWriteOutputCommitted(receipt.outputOwnerHandle)) {
        return false;
      }

      const bool modelAlias =
          receipt.sourceOwnerHandle == DRW::NoHandle ||
          receipt.sourceOwnerHandle == DRW::DxfModelSpaceBlockRecordHandle ||
          receipt.sourceOwnerHandle == DRW::DwgModelSpaceBlockRecordHandle;
      const bool paperAlias =
          receipt.sourceOwnerHandle == DRW::DxfPaperSpaceBlockRecordHandle ||
          receipt.sourceOwnerHandle == DRW::DwgPaperSpaceBlockRecordHandle;
      if (modelAlias) {
        if (receipt.outputOwnerHandle != DRW::DwgModelSpaceBlockRecordHandle)
          return false;
      } else if (paperAlias) {
        if (receipt.outputOwnerHandle != DRW::DwgPaperSpaceBlockRecordHandle)
          return false;
      } else {
        std::uint32_t resolvedOwner = 0;
        if (lookupDwgWriteIdentity(DwgWriteSourceKind::Block,
                                   receipt.sourceOwnerHandle, &resolvedOwner) !=
                DwgWriteIdentityLookup::Committed ||
            resolvedOwner != receipt.outputOwnerHandle) {
          return false;
        }
      }

      if (published != nullptr) {
        std::size_t ownerRecordCount = 0;
        std::size_t producerRecordCount = 0;
        std::uint32_t producerOwnerHandle = 0;
        for (const DRW::DwgBlockWriteRecord &record : published->blockRecords) {
          const auto countHandle =
              [producerHandle = receipt.producerOutputHandle](
                  const std::vector<std::uint32_t> &handles) {
                return static_cast<std::size_t>(std::count(
                    handles.cbegin(), handles.cend(), producerHandle));
              };
          const std::size_t producerCount = countHandle(record.entityHandles) +
                                            countHandle(record.insertHandles);
          if (record.blockRecordHandle == receipt.outputOwnerHandle) {
            ++ownerRecordCount;
            if (producerCount != 0)
              producerOwnerHandle = record.blockRecordHandle;
          }
          if (producerCount != 0) {
            producerRecordCount += producerCount;
            if (producerRecordCount > 1)
              return false;
            producerOwnerHandle = record.blockRecordHandle;
          }
        }
        if (ownerRecordCount != 1 || producerRecordCount != 1 ||
            producerOwnerHandle != receipt.outputOwnerHandle) {
          return false;
        }
      }
    }
    return true;
  }

  bool validateDwgWriteIdentityLedger(bool allowSelectedBlocks = false) const {
    for (const auto &entry : m_dwgWriteIdentityByOutput) {
      if (entry.second.state == DwgWriteIdentityState::Selected &&
          !(allowSelectedBlocks &&
            entry.second.kind == DwgWriteSourceKind::Block))
        return false;
    }

    for (std::uint32_t sourceHandle : m_dwgWriteEntityHandlesEmitted) {
      std::uint32_t outputHandle = 0;
      if (lookupDwgWriteIdentity(DwgWriteSourceKind::Entity, sourceHandle,
                                 &outputHandle) !=
          DwgWriteIdentityLookup::Committed) {
        return false;
      }
      const auto remapIt = m_dwgWriteEntityHandleRemap.find(sourceHandle);
      if (remapIt == m_dwgWriteEntityHandleRemap.end() ||
          remapIt->second != outputHandle) {
        return false;
      }
    }
    return true;
  }

  void noteDwgWriteSourceKind(std::uint32_t sourceHandle,
                              DwgWriteSourceKind kind) {
    if (sourceHandle != 0)
      m_dwgWriteSourceKinds[sourceHandle].insert(kind);
  }

  bool isDwgWriteSourceAmbiguous(std::uint32_t sourceHandle) const {
    const auto it = m_dwgWriteSourceKinds.find(sourceHandle);
    if (it != m_dwgWriteSourceKinds.end() && it->second.size() > 1)
      return true;
    return m_dwgWriteDuplicateEntityHandles.count(sourceHandle) != 0 ||
           m_dwgWriteDuplicateBlockHandles.count(sourceHandle) != 0;
  }

  bool isDwgWriteSourceAmbiguous(std::uint32_t sourceHandle,
                                 DwgWriteSourceKind kind) const {
    if (kind == DwgWriteSourceKind::Entity &&
        m_dwgWriteDuplicateEntityHandles.count(sourceHandle) != 0)
      return true;
    if (kind == DwgWriteSourceKind::Block &&
        m_dwgWriteDuplicateBlockHandles.count(sourceHandle) != 0)
      return true;

    const auto it = m_dwgWriteIdentityOutputsBySource.find(
        DwgWriteSourceKey{kind, sourceHandle});
    if (it == m_dwgWriteIdentityOutputsBySource.end())
      return false;
    std::size_t activeOutputs = 0;
    for (std::uint32_t outputHandle : it->second) {
      const auto identity = m_dwgWriteIdentityByOutput.find(
          DwgWriteIdentityKey{kind, outputHandle});
      if (identity != m_dwgWriteIdentityByOutput.end() &&
          identity->second.state != DwgWriteIdentityState::Suppressed &&
          ++activeOutputs > 1)
        return true;
    }
    return false;
  }

  bool hasDwgWriteKnownHandle(std::uint32_t sourceHandle) const {
    return m_dwgWriteKnownHandles.count(sourceHandle) != 0;
  }

  bool hasDwgWriteHandleRemap(std::uint32_t sourceHandle) const {
    return m_dwgWriteHandleRemap.count(sourceHandle) != 0;
  }

  std::uint32_t resolveDwgWriteHandle(std::uint32_t sourceHandle) const {
    const auto it = m_dwgWriteHandleRemap.find(sourceHandle);
    return it == m_dwgWriteHandleRemap.end() ? sourceHandle : it->second;
  }

  // Opaque bytes may only retain a reference whose target remains available
  // under its original handle. Entity emission can be required by callers
  // that run after the entity stream has been written.
  bool
  isDwgWriteOpaqueReferenceStable(std::uint32_t sourceHandle,
                                  bool requireEntityEmission = false) const {
    if (sourceHandle == DRW::NoHandle ||
        sourceHandle == DRW::DwgModelSpaceBlockRecordHandle ||
        sourceHandle == DRW::DwgPaperSpaceBlockRecordHandle ||
        sourceHandle == DRW::DxfModelSpaceBlockRecordHandle ||
        sourceHandle == DRW::DxfPaperSpaceBlockRecordHandle) {
      return true;
    }

    const bool hasEntityTarget =
        m_dwgWriteEntityHandleRemap.count(sourceHandle) != 0;
    const bool hasObjectTarget = hasDwgWriteKnownHandle(sourceHandle);
    const bool hasBlockTarget =
        m_dwgWriteBlockHandleRemap.count(sourceHandle) != 0;
    if ((hasEntityTarget && hasObjectTarget) ||
        (hasEntityTarget &&
         m_dwgWriteDuplicateEntityHandles.count(sourceHandle) != 0) ||
        (hasBlockTarget &&
         m_dwgWriteDuplicateBlockHandles.count(sourceHandle) != 0) ||
        (hasBlockTarget && (hasEntityTarget || hasObjectTarget))) {
      return false;
    }
    if (hasEntityTarget) {
      const auto it = m_dwgWriteEntityHandleRemap.find(sourceHandle);
      return (!requireEntityEmission ||
              m_dwgWriteEntityHandlesEmitted.count(sourceHandle) != 0) &&
             it != m_dwgWriteEntityHandleRemap.end() &&
             it->second == sourceHandle;
    }
    if (hasBlockTarget) {
      const auto it = m_dwgWriteBlockHandleRemap.find(sourceHandle);
      return it != m_dwgWriteBlockHandleRemap.end() &&
             it->second == sourceHandle;
    }
    if (hasObjectTarget)
      return resolveDwgWriteHandle(sourceHandle) == sourceHandle;
    return false;
  }

  // Resolve a generic DWG OBJECT reference while retaining the distinction
  // between a planned target and an identity that was later suppressed.
  // Generic OBJECT fields remain optional for now, so callers may drop a
  // non-resolved result according to their family policy.
  DwgWriteReferenceStatus
  resolveDwgWriteObjectHandle(std::uint32_t sourceHandle,
                              std::uint32_t *outputHandle = nullptr) const {
    if (outputHandle != nullptr)
      *outputHandle = 0;
    if (sourceHandle == DRW::NoHandle)
      return DwgWriteReferenceStatus::Resolved;
    if (isDwgWriteSourceAmbiguous(sourceHandle, DwgWriteSourceKind::Object))
      return DwgWriteReferenceStatus::Ambiguous;
    if (!hasDwgWriteKnownHandle(sourceHandle))
      return DwgWriteReferenceStatus::Missing;

    std::uint32_t identityOutput = 0;
    const DwgWriteIdentityLookup identityStatus = lookupDwgWriteIdentity(
        DwgWriteSourceKind::Object, sourceHandle, &identityOutput);
    if (identityStatus == DwgWriteIdentityLookup::Suppressed)
      return DwgWriteReferenceStatus::Suppressed;
    if (identityStatus == DwgWriteIdentityLookup::Ambiguous)
      return DwgWriteReferenceStatus::Ambiguous;
    if (identityStatus == DwgWriteIdentityLookup::Selected ||
        identityStatus == DwgWriteIdentityLookup::Committed) {
      if (outputHandle != nullptr)
        *outputHandle = identityOutput;
      return identityStatus == DwgWriteIdentityLookup::Selected
                 ? DwgWriteReferenceStatus::Selected
                 : DwgWriteReferenceStatus::Committed;
    }

    if (outputHandle != nullptr)
      *outputHandle = resolveDwgWriteHandle(sourceHandle);
    return DwgWriteReferenceStatus::Resolved;
  }

  DwgWriteReferenceStatus
  resolveDwgWriteEntityHandle(std::uint32_t sourceHandle,
                              std::uint32_t *outputHandle = nullptr) const {
    if (outputHandle != nullptr)
      *outputHandle = 0;
    if (sourceHandle == DRW::NoHandle)
      return DwgWriteReferenceStatus::Resolved;
    if (m_dwgWriteDuplicateEntityHandles.count(sourceHandle) != 0 ||
        isDwgWriteSourceAmbiguous(sourceHandle, DwgWriteSourceKind::Entity))
      return DwgWriteReferenceStatus::Ambiguous;

    std::uint32_t resolvedHandle = 0;
    switch (lookupDwgWriteIdentity(DwgWriteSourceKind::Entity, sourceHandle,
                                   &resolvedHandle)) {
    case DwgWriteIdentityLookup::Missing:
      return DwgWriteReferenceStatus::Missing;
    case DwgWriteIdentityLookup::Ambiguous:
      return DwgWriteReferenceStatus::Ambiguous;
    case DwgWriteIdentityLookup::Reserved:
      break;
    case DwgWriteIdentityLookup::Selected:
      if (outputHandle != nullptr)
        *outputHandle = resolvedHandle;
      return DwgWriteReferenceStatus::Selected;
    case DwgWriteIdentityLookup::Committed:
      if (outputHandle != nullptr)
        *outputHandle = resolvedHandle;
      return DwgWriteReferenceStatus::Committed;
    case DwgWriteIdentityLookup::Suppressed:
      return DwgWriteReferenceStatus::Suppressed;
    }
    if (outputHandle != nullptr)
      *outputHandle = resolvedHandle;
    return DwgWriteReferenceStatus::Reserved;
  }

  DwgWriteReferenceStatus
  resolveDwgWriteCommonHandle(std::uint32_t sourceHandle,
                              std::uint32_t *outputHandle = nullptr,
                              DwgWriteSourceKind *targetKind = nullptr) const {
    if (outputHandle != nullptr)
      *outputHandle = 0;
    if (targetKind != nullptr)
      *targetKind = DwgWriteSourceKind::Object;
    if (sourceHandle == DRW::NoHandle)
      return DwgWriteReferenceStatus::Resolved;

    // These dictionaries are fixed structural objects, not ordinary
    // source-backed OBJECTS. Resolve them before probing entity/object
    // maps so receipt validation can use their fixed identity.
    if (sourceHandle == DRW::DwgNamedObjectsDictionaryHandle ||
        sourceHandle == DRW::DwgAcadGroupDictionaryHandle) {
      if (outputHandle != nullptr)
        *outputHandle = sourceHandle;
      if (targetKind != nullptr)
        *targetKind = DwgWriteSourceKind::FixedStructural;
      return DwgWriteReferenceStatus::Committed;
    }

    // Preserved raw OBJECTS are committed under RawCarrier, not Object.
    // Prefer that exact identity when no typed Object/Entity identity exists;
    // otherwise reference receipts name a target kind that can never commit.
    std::uint32_t rawOutputHandle = 0;
    const DwgWriteIdentityLookup rawIdentityStatus =
        lookupDwgWriteIdentity(DwgWriteSourceKind::RawCarrier, sourceHandle,
                               &rawOutputHandle);
    if (rawIdentityStatus == DwgWriteIdentityLookup::Ambiguous)
      return DwgWriteReferenceStatus::Ambiguous;
    if (rawIdentityStatus == DwgWriteIdentityLookup::Reserved ||
        rawIdentityStatus == DwgWriteIdentityLookup::Selected ||
        rawIdentityStatus == DwgWriteIdentityLookup::Committed) {
      const DwgWriteIdentityLookup entityIdentityStatus =
          lookupDwgWriteIdentity(DwgWriteSourceKind::Entity, sourceHandle);
      const DwgWriteIdentityLookup objectIdentityStatus =
          lookupDwgWriteIdentity(DwgWriteSourceKind::Object, sourceHandle);
      const bool typedIdentityExists =
          entityIdentityStatus == DwgWriteIdentityLookup::Reserved ||
          entityIdentityStatus == DwgWriteIdentityLookup::Selected ||
          entityIdentityStatus == DwgWriteIdentityLookup::Committed ||
          objectIdentityStatus == DwgWriteIdentityLookup::Reserved ||
          objectIdentityStatus == DwgWriteIdentityLookup::Selected ||
          objectIdentityStatus == DwgWriteIdentityLookup::Committed;
      if (!typedIdentityExists) {
        if (outputHandle != nullptr)
          *outputHandle = rawOutputHandle;
        if (targetKind != nullptr)
          *targetKind = DwgWriteSourceKind::RawCarrier;
        switch (rawIdentityStatus) {
        case DwgWriteIdentityLookup::Reserved:
          return DwgWriteReferenceStatus::Reserved;
        case DwgWriteIdentityLookup::Selected:
          return DwgWriteReferenceStatus::Selected;
        case DwgWriteIdentityLookup::Committed:
          return DwgWriteReferenceStatus::Committed;
        default:
          break;
        }
      }
    }

    const DwgWriteReferenceStatus entityStatus =
        resolveDwgWriteEntityHandle(sourceHandle, outputHandle);
    std::uint32_t objectOutputHandle = 0;
    const DwgWriteReferenceStatus objectStatus =
        resolveDwgWriteObjectHandle(sourceHandle, &objectOutputHandle);
    const auto isLive = [](DwgWriteReferenceStatus status) {
      return status == DwgWriteReferenceStatus::Resolved ||
             status == DwgWriteReferenceStatus::LegacySpaceAlias ||
             status == DwgWriteReferenceStatus::Reserved ||
             status == DwgWriteReferenceStatus::Selected ||
             status == DwgWriteReferenceStatus::Committed;
    };
    if (entityStatus == DwgWriteReferenceStatus::Ambiguous ||
        objectStatus == DwgWriteReferenceStatus::Ambiguous)
      return DwgWriteReferenceStatus::Ambiguous;
    if (isLive(entityStatus) && isLive(objectStatus))
      return DwgWriteReferenceStatus::Ambiguous;
    if (isLive(entityStatus)) {
      if (targetKind != nullptr)
        *targetKind = DwgWriteSourceKind::Entity;
      return entityStatus;
    }
    if (isLive(objectStatus)) {
      if (outputHandle != nullptr)
        *outputHandle = objectOutputHandle;
      if (targetKind != nullptr)
        *targetKind = DwgWriteSourceKind::Object;
      return objectStatus;
    }
    if (entityStatus == DwgWriteReferenceStatus::Suppressed &&
        objectStatus != DwgWriteReferenceStatus::Suppressed &&
        targetKind != nullptr)
      *targetKind = DwgWriteSourceKind::Entity;
    if (entityStatus == DwgWriteReferenceStatus::Suppressed ||
        objectStatus == DwgWriteReferenceStatus::Suppressed)
      return DwgWriteReferenceStatus::Suppressed;
    return DwgWriteReferenceStatus::Missing;
  }

  DwgWriteReferenceStatus
  resolveDwgWriteOwnerHandle(std::uint32_t sourceHandle,
                             std::uint32_t *outputHandle = nullptr) const {
    if (outputHandle != nullptr)
      *outputHandle = 0;
    if (sourceHandle == DRW::NoHandle)
      return DwgWriteReferenceStatus::Resolved;
    if (sourceHandle == DRW::DxfModelSpaceBlockRecordHandle ||
        sourceHandle == DRW::DwgModelSpaceBlockRecordHandle) {
      if (outputHandle != nullptr)
        *outputHandle = DRW::DwgModelSpaceBlockRecordHandle;
      return DwgWriteReferenceStatus::LegacySpaceAlias;
    }
    if (sourceHandle == DRW::DxfPaperSpaceBlockRecordHandle ||
        sourceHandle == DRW::DwgPaperSpaceBlockRecordHandle) {
      if (outputHandle != nullptr)
        *outputHandle = DRW::DwgPaperSpaceBlockRecordHandle;
      return DwgWriteReferenceStatus::LegacySpaceAlias;
    }
    if (sourceHandle == DRW::DwgNamedObjectsDictionaryHandle ||
        sourceHandle == DRW::DwgAcadGroupDictionaryHandle) {
      if (outputHandle != nullptr)
        *outputHandle = sourceHandle;
      return DwgWriteReferenceStatus::LegacySpaceAlias;
    }

    std::uint32_t blockOutputHandle = 0;
    const DwgWriteReferenceStatus blockStatus =
        resolveDwgWriteBlockOwner(sourceHandle, &blockOutputHandle);
    std::uint32_t rawOutputHandle = 0;
    const DwgWriteIdentityLookup rawIdentityStatus =
        lookupDwgWriteIdentity(DwgWriteSourceKind::RawCarrier, sourceHandle,
                               &rawOutputHandle);
    const DwgWriteIdentityLookup objectIdentityStatus =
        lookupDwgWriteIdentity(DwgWriteSourceKind::Object, sourceHandle);
    std::uint32_t objectOutputHandle = 0;
    const DwgWriteReferenceStatus objectStatus =
        resolveDwgWriteObjectHandle(sourceHandle, &objectOutputHandle);
    const auto isLive = [](DwgWriteReferenceStatus status) {
      return status == DwgWriteReferenceStatus::Resolved ||
             status == DwgWriteReferenceStatus::LegacySpaceAlias ||
             status == DwgWriteReferenceStatus::Selected ||
             status == DwgWriteReferenceStatus::Committed;
    };
    if (blockStatus == DwgWriteReferenceStatus::Ambiguous ||
        objectStatus == DwgWriteReferenceStatus::Ambiguous)
      return DwgWriteReferenceStatus::Ambiguous;
    if ((rawIdentityStatus == DwgWriteIdentityLookup::Reserved ||
         rawIdentityStatus == DwgWriteIdentityLookup::Selected ||
         rawIdentityStatus == DwgWriteIdentityLookup::Committed) &&
        !isLive(blockStatus) &&
        objectIdentityStatus != DwgWriteIdentityLookup::Reserved &&
        objectIdentityStatus != DwgWriteIdentityLookup::Selected &&
        objectIdentityStatus != DwgWriteIdentityLookup::Committed) {
      if (outputHandle != nullptr)
        *outputHandle = rawOutputHandle;
      switch (rawIdentityStatus) {
      case DwgWriteIdentityLookup::Reserved:
        return DwgWriteReferenceStatus::Reserved;
      case DwgWriteIdentityLookup::Selected:
        return DwgWriteReferenceStatus::Selected;
      case DwgWriteIdentityLookup::Committed:
        return DwgWriteReferenceStatus::Committed;
      default:
        break;
      }
    }
    if (isLive(blockStatus) && isLive(objectStatus))
      return DwgWriteReferenceStatus::Ambiguous;
    if (isLive(blockStatus)) {
      if (outputHandle != nullptr)
        *outputHandle = blockOutputHandle;
      return blockStatus;
    }
    if (isLive(objectStatus)) {
      if (outputHandle != nullptr)
        *outputHandle = objectOutputHandle;
      return objectStatus;
    }
    if (blockStatus == DwgWriteReferenceStatus::Suppressed ||
        objectStatus == DwgWriteReferenceStatus::Suppressed)
      return DwgWriteReferenceStatus::Suppressed;
    return DwgWriteReferenceStatus::Missing;
  }

  DwgWriteReferenceStatus resolveDwgWriteTableRecordHandle(
      std::uint32_t sourceHandle,
      const std::map<std::uint32_t, std::uint32_t> &remap,
      std::uint32_t *outputHandle = nullptr) const {
    if (outputHandle != nullptr)
      *outputHandle = 0;
    if (sourceHandle == DRW::NoHandle)
      return DwgWriteReferenceStatus::Resolved;

    const auto remapIt = remap.find(sourceHandle);
    if (remapIt == remap.end())
      return DwgWriteReferenceStatus::Missing;
    const std::uint32_t resolvedHandle = remapIt->second;
    if (outputHandle != nullptr)
      *outputHandle = resolvedHandle;

    const auto ownerIt = m_dwgWriteIdentityOwnerByOutput.find(resolvedHandle);
    if (ownerIt == m_dwgWriteIdentityOwnerByOutput.end() ||
        ownerIt->second.kind != DwgWriteSourceKind::TableRecord)
      return DwgWriteReferenceStatus::Missing;
    const auto identityIt = m_dwgWriteIdentityByOutput.find(ownerIt->second);
    if (identityIt == m_dwgWriteIdentityByOutput.end())
      return DwgWriteReferenceStatus::Missing;
    switch (identityIt->second.state) {
    case DwgWriteIdentityState::Reserved:
      return DwgWriteReferenceStatus::Reserved;
    case DwgWriteIdentityState::Selected:
      return DwgWriteReferenceStatus::Selected;
    case DwgWriteIdentityState::Committed:
      return DwgWriteReferenceStatus::Committed;
    case DwgWriteIdentityState::Suppressed:
      return DwgWriteReferenceStatus::Suppressed;
    }
    return DwgWriteReferenceStatus::Missing;
  }

  std::map<std::uint32_t, std::set<DwgWriteSourceKind>> m_dwgWriteSourceKinds;

  // The legacy maps below remain during migration. This ledger is the
  // authoritative lifecycle for identities already migrated to the new API.
  std::map<DwgWriteIdentityKey, DwgWriteIdentity> m_dwgWriteIdentityByOutput;
  std::map<DwgWriteSourceKey, std::set<std::uint32_t>>
      m_dwgWriteIdentityOutputsBySource;
  std::map<std::uint32_t, DwgWriteIdentityKey> m_dwgWriteIdentityOwnerByOutput;

  // Structural object identities and selected output targets.
  std::map<std::uint32_t, std::uint32_t> m_dwgWriteHandleRemap;
  std::set<std::uint32_t> m_dwgWriteKnownHandles;

  // Entity source identity and commit state.
  std::map<const RS_Entity *, std::uint32_t> m_dwgWriteEntityHandleByEntity;
  std::map<std::uint32_t, std::uint32_t> m_dwgWriteEntityHandleRemap;
  std::set<std::uint32_t> m_dwgWriteEntityHandlesEmitted;
  std::set<std::uint32_t> m_dwgWriteDuplicateEntityHandles;
  // Metadata-only LIGHT identities are planned before LIGHTLIST objects are
  // emitted; their entity identities are committed through the common
  // identity ledger when the LIGHT entity is written.
  std::set<std::uint32_t> m_dwgWriteLightHandlesPlanned;

  // Deferred named-table identities.
  std::map<std::uint32_t, std::uint32_t> m_dwgWriteViewHandleRemap;
  std::map<std::uint32_t, std::uint32_t> m_dwgWriteUcsHandleRemap;
  std::map<std::uint32_t, std::uint32_t> m_dwgWriteVportHandleRemap;

  // BLOCK_RECORD identities and source ambiguity.
  std::map<std::uint32_t, std::uint32_t> m_dwgWriteBlockHandleRemap;
  std::set<std::uint32_t> m_dwgWriteDuplicateBlockHandles;
  // Normalized BLOCK_RECORD names are bound before block contents are
  // encoded, so INSERT references do not depend on block-list order.
  std::map<std::string, std::uint32_t> m_dwgWriteBlockHandleByName;
  std::map<std::uint32_t, DRW::DwgObjectFrameReceipt>
      m_dwgWriteObjectOccurrencesByOutput;
  std::map<DwgWriteSourceKey, DwgClassFrameAdmission>
      m_dwgWriteClassFrameAdmissionsBySource;
  std::map<std::uint32_t, DwgWriteSourceKey>
      m_dwgWriteClassFrameAdmissionsByOutput;
  std::map<DwgWriteSourceKey, DwgFixedEntityFrameReceipt>
      m_dwgWriteFixedEntityFrameReceiptsBySource;
  std::map<std::uint32_t, DwgWriteSourceKey>
      m_dwgWriteFixedEntityFrameReceiptsByOutput;
  std::map<const RS_Entity *, DwgEntityWritePlan> m_dwgWriteEntityPlansByEntity;
  std::map<std::uint32_t, const RS_Entity *> m_dwgWriteEntityPlansBySource;
  std::vector<DwgWriteOwnerReceipt> m_dwgWriteOwnerReceipts;
  std::vector<DwgWriteReferenceReceipt> m_dwgWriteReferenceReceipts;
  std::vector<DwgWriteOptionalDropReceipt> m_dwgWriteOptionalDropReceipts;

  // Name-based table identities used when source numeric handles are not
  // portable across DWG files.
  std::map<std::string, std::uint32_t> m_dwgWriteLTypeHandleByName;
  std::map<std::string, std::uint32_t> m_dwgWriteTextStyleHandleByName;
};
#endif

// test-only friend; defined in tests/dwg_header_app_vars_tests.cpp. Grants
// the header-var regression suite access to the private m_graphic/
// m_currentContainer so it can exercise addHeader against a real RS_Graphic.
class RsFilterDxfRwHeaderTestAccess;
// test-only friend; defined in tests/family_exposure_tests.cpp. Grants access
// to m_graphic for cross-read filter exposure / Navisworks metadata tests.
class RsFilterDxfRwExposureTestAccess;
/**
 * This format filter class can import and export DXF files.
 * It depends on the libdxfrw library.
 *
 * @author Rallaz
 */
class RS_FilterDXFRW : public RS_FilterInterface,
                       DRW_Interface
#ifdef DWGSUPPORT
    ,
                       private RS_FilterDXFRW_DwgWriteIdentityRegistry
#endif
{
  friend class RsFilterDxfRwHeaderTestAccess;
  friend class RsFilterDxfRwExposureTestAccess;
#ifdef DWGSUPPORT
  friend class DwgWriteFailureTestAccess;
#endif
public:
  enum class DwgDataStorageIdentityStatus : std::uint8_t {
    Missing,
    NumericOnly,
    LexicalOnly,
    Consistent,
    Conflict,
    OwnerMismatch
  };

  RS_FilterDXFRW();
  ~RS_FilterDXFRW() override;
  RS_FilterDXFRW(const RS_FilterDXFRW &) = delete;
  RS_FilterDXFRW &operator=(const RS_FilterDXFRW &) = delete;
  RS_FilterDXFRW(RS_FilterDXFRW &&) = delete;
  RS_FilterDXFRW &operator=(RS_FilterDXFRW &&) = delete;

  bool canImport(const QString & /*fileName*/,
                 const RS2::FormatType t) const override {
#ifdef DWGSUPPORT
    return t == RS2::FormatDXFRW || t == RS2::FormatDWG ||
           t == RS2::FormatDWG2004 || t == RS2::FormatDWG2007 ||
           t == RS2::FormatDWG2010 || t == RS2::FormatDWG2013 ||
           t == RS2::FormatDWG2018;
#else
    return (t == RS2::FormatDXFRW);
#endif
  }

  bool canExport(const QString & /*fileName*/,
                 const RS2::FormatType t) const override {
#ifdef DWGSUPPORT
    if (t == RS2::FormatDWG || t == RS2::FormatDWG2004 ||
        t == RS2::FormatDWG2007 || t == RS2::FormatDWG2010 ||
        t == RS2::FormatDWG2013 || t == RS2::FormatDWG2018)
      return true;
#endif
    return (t == RS2::FormatDXFRW2018 || t == RS2::FormatDXFRW ||
            t == RS2::FormatDXFRW2004 || t == RS2::FormatDXFRW2000 ||
            t == RS2::FormatDXFRW14 || t == RS2::FormatDXFRW12);
  }

  // Error messages
  QString lastError() const override;

#ifdef DWGSUPPORT
  // Snapshot of the current DWG admission attempt for the explicit-path
  // developer diagnostic command. It is read-only and never discovers files.
  QString dwgExportAdmissionReport() const;
#endif

  // Import:
  bool fileImport(RS_Graphic &g, const QString &file,
                  RS2::FormatType type) override;

  // Methods from DRW_CreationInterface:
  void addHeader(const DRW_Header *data) override;

  void addLType(const DRW_LType &data) override;

  void addLayer(const DRW_Layer &data) override;
  void addDimStyle(const DRW_Dimstyle &data) override;
  void addVport(const DRW_Vport &data) override;
  void addView(const DRW_View &data) override;
  void addUCS(const DRW_UCS &data) override;
  void addVisualStyle(const DRW_VisualStyle &data) override;

public:
  struct TableFallbackRenderSummary {
    size_t gridEntityCount = 0;
    size_t textEntityCount = 0;
    size_t placeholderEntityCount = 0;
    size_t unresolvedTextStyleCount = 0;
    size_t clampedDimensionCount = 0;
  };

  void addTextStyle(const DRW_Textstyle &data) override;
  void addAppId(const DRW_AppId &data) override;
  void addBlock(const DRW_Block &data) override;
  void setBlock(int handle) override;
  void endBlock() override;
  void addPoint(const DRW_Point &data) override;
  void addLine(const DRW_Line &data) override;
  void add3DLine(const DRW_3DLine &data) override;
  void addRay(const DRW_Ray &data) override;
  void addXline(const DRW_Xline &data) override;
  void addCircle(const DRW_Circle &data) override;
  void addArc(const DRW_Arc &data) override;
  void addEllipse(const DRW_Ellipse &data) override;
  void addLWPolyline(const DRW_LWPolyline &data) override;
  void addMLine(const DRW_MLine *data) override;
  void addMLineStyle(const DRW_MLineStyle &data) override;
  void addUnderlay(const DRW_Underlay *data) override;
  void linkUnderlay(const DRW_UnderlayDefinition *data) override;
  void addShape(const DRW_Shape &data) override;
  void addOle2Frame(const DRW_Ole2Frame &data) override;
  void addOleFrame(const DRW_OleFrame &data) override;
  void addText(const DRW_Text &data) override;
  void addAttDef(const DRW_Attdef &data) override;
  void addPolyline(const DRW_Polyline &data) override;
  void addSpline(const DRW_Spline *data) override;
  void addHelix(const DRW_Helix *data) override;
  void addKnot(const DRW_Entity &) override {}
  void addInsert(const DRW_Insert &data) override;
  void addTable(const DRW_Table &data) override;
  bool addTableFallback(const DRW_Table &data,
                        TableFallbackRenderSummary *summary = nullptr);
  void addTrace(const DRW_Trace &data) override;
  void addTolerance(const DRW_Tolerance &data) override;
  void addSolid(const DRW_Solid &data) override;
  void addModelerGeometry(const DRW_ModelerGeometry &data) override;
  void addMesh(const DRW_Mesh &data) override;
  void addSurface(const DRW_Surface *data) override;
  //! Convert a decoded ACIS wireframe into RS_* entities, parented to and added
  //! into `container` (projected to 2D by dropping Z, exactly like addMesh).
  //! Straight edges -> RS_Line, ellipse edges -> RS_Ellipse (line fallback when
  //! degenerate), intcurve edges -> RS_Spline through the control polygon (line
  //! fallback), isolated vertices -> RS_Point. Returns the created entities
  //! WITHOUT attributes applied, so callers can post-process them (e.g.
  //! setEntityAttributes). Static + container-scoped so it is unit-testable
  //! without a full DXF import (no m_graphic needed).
  static std::vector<RS_Entity *>
  acisWireframeToEntities(const DRW_AcisBrep &brep,
                          RS_EntityContainer *container);
  void addLight(const DRW_Light &data) override;
  void addLightList(const DRW_LightList &data) override;
  void addDataLink(const DRW_DataLink &data) override;
  void addGeoMapImage(const DRW_GeoMapImage &data) override;
  void addLayerFilter(const DRW_LayerFilter &data) override;
  void addCamera(const DRW_Camera &data) override;
  void addGeoPositionMarker(const DRW_GeoPositionMarker &data) override;
  void addTvDeviceProperties(const DRW_TvDeviceProperties &data) override;
  void addViewportEntityHeader(const DRW_ViewportEntityHeader &data) override;
  void addVxControl(const DRW_VxControl &data) override;
  void addVxTableRecord(const DRW_VxTableRecord &data) override;
  void addMText(const DRW_MText &data) override;
  /** Build an RS_MText from a DRW_MText payload, handling alignment / drawing
   *  direction / line spacing / oldMText legacy correction.  Caller takes
   *  ownership of the returned entity (parent is null). */
  RS_MText *mtextEntityFromDRW(const DRW_MText &data);
  void addDimAlign(const DRW_DimAligned *data) override;
  void addDimLinear(const DRW_DimLinear *data) override;
  void addDimRadial(const DRW_DimRadial *data) override;
  void addDimDiametric(const DRW_DimDiametric *data) override;
  void addDimAngular(const DRW_DimAngular *data) override;
  void addDimAngular3P(const DRW_DimAngular3p *data) override;
  void addDimOrdinate(const DRW_DimOrdinate *data) override;
  void addDimArc(const DRW_DimArc *data) override;
  void addLeader(const DRW_Leader *data) override;
  void addHatch(const DRW_Hatch *data) override;

  void addViewport(const DRW_Viewport & /*data*/) override {}

  void addImage(const DRW_Image *data) override;
  void linkImage(const DRW_ImageDef *data) override;
  void addWipeout(const DRW_Wipeout *data) override;
  void addMLeader(const DRW_MLeader *data) override;
  void addMLeaderStyle(const DRW_MLeaderStyle *data) override;
  void addDetailViewStyle(const DRW_DetailViewStyle &data) override;
  void addSectionViewStyle(const DRW_SectionViewStyle &data) override;
  void addBreakData(const DRW_BreakData &data) override;
  void addBreakPointRef(const DRW_BreakPointRef &data) override;
  void addGroup(const DRW_Group &data) override;
  void
  addImageDefinitionReactor(const DRW_ImageDefinitionReactor &data) override;
  void addRasterVariables(const DRW_RasterVariables &data) override;
  void addWipeoutVariables(const DRW_WipeoutVariables &data) override;
  void addSpatialFilter(const DRW_SpatialFilter &data) override;
  void addGeoData(const DRW_GeoData &data) override;
  void addTableGeometry(const DRW_TableGeometry &data) override;
  void addTableStyle(const DRW_TableStyle &data) override;
  void addTableContent(const DRW_TableContentObject &data) override;
  void addObjectContextData(const DRW_ObjectContextData &data) override;
  void addCellStyleMap(const DRW_CellStyleMap &data) override;
  void addUnsupportedObject(const DRW_UnsupportedObject &data) override;
  void addDwgFramePublication(const DRW_DwgFramePublication &data) override;
  void
  addDwgDictionaryMembership(const DRW_DwgDictionaryMembership &data) override;
  void addDwgDictionaryWithDefaultMembership(
      const DRW_DwgDictionaryWithDefaultMembership &data) override;
  void addDwgGroupMembership(const DRW_DwgGroupMembership &data) override;
  void addDwgSortEntsMembership(const DRW_DwgSortEntsMembership &data) override;
  void
  addDwgFieldListMembership(const DRW_DwgFieldListMembership &data) override;
  void
  addDwgFieldPayloadReceipt(const DRW_DwgFieldPayloadReceipt &data) override;
  void addDwgTypedReference(const DRW_DwgTypedReference &data) override;
  void addDwgBlockReachability(const DRW_DwgBlockReachability &data) override;
  void
  addDwgFrameCoverageReport(const DRW_DwgFrameCoverageReport &data) override;
  void
  addDwgClassCoverageReport(const DRW_DwgClassCoverageReport &data) override;
  void addRawDwgSection(const DRW_RawDwgSection &data) override;
  void addDataStorage(const DRW_DataStorageSection &data) override;
  void addRawDxfObject(const DRW_RawDxfObject &data) override;
  void addRawDxfEntity(const DRW_RawDxfObject &data) override;
  void addRawDxfSection(const DRW_RawDxfSection &data) override;
  void addDxfClass(const DRW_Class &data) override;
  void addAcDbPlaceholder(const DRW_AcDbPlaceholder &data) override;
  void addCsacDocumentOptions(const DRW_CsacDocumentOptions &data) override;
  void addContextDataManager(const DRW_ContextDataManager &data) override;
  void addProxyEntity(const DRW_ProxyEntity &data) override;
  void addVbaProject(const DRW_VbaProject &data) override;
  void addProxyObject(const DRW_ProxyObject &data) override;
  void addSun(const DRW_Sun &data) override;
  // Cross-read parity: metadata-only exposure (PR-4a/4c) — no document geometry
  void addPointCloud(const DRW_PointCloud *data) override;
  void addPointCloudEx(const DRW_PointCloudEx *data) override;
  void addNavisworksModel(const DRW_NavisworksModel *data) override;
  void addPointCloudDef(const DRW_PointCloudDef &data) override;
  void addNavisworksModelDef(const DRW_NavisworksModelDef &data) override;
  void addPointCloudColorMap(const DRW_PointCloudColorMap &data) override;
  void addBackground(const DRW_Background &data) override;
  void addMaterial(const DRW_Material &data) override;
  void addRenderSettings(const DRW_RenderSettings &data) override;
  void addSunStudy(const DRW_SunStudy &data) override;
  void addMotionPath(const DRW_MotionPath &data) override;
  void addCurvePath(const DRW_CurvePath &data) override;
  void addPointPath(const DRW_PointPath &data) override;
  void addObjectPtr(const DRW_ObjectPtr &data) override;
  void addPartialViewingIndex(const DRW_PartialViewingIndex &data) override;
  void addDbColor(const DRW_DbColor &data) override;
  void addDimensionAssociation(const DRW_DimensionAssociation &data) override;
  void addEvaluationGraph(const DRW_EvaluationGraph &data) override;
  void
  addBlockRepresentationData(const DRW_BlockRepresentationData &data) override;
  void addSection(const DRW_Section &data) override;
  void addSectionObject(const DRW_SectionObject &data) override;
  void addMPolygon(const DRW_MPolygon *data) override;
  void addDictionary(const DRW_Dictionary &data) override;
  void addXRecord(const DRW_XRecord &data) override;
  void addLayout(const DRW_Layout &data) override;
  // PR 8d.2a — five small no-storage OBJECTS families.
  void addScale(const DRW_Scale &data) override;
  void addIDBuffer(const DRW_IDBuffer &data) override;
  void addIndex(const DRW_Index &data) override;
  void addLayerIndex(const DRW_LayerIndex &data) override;
  void addSpatialIndex(const DRW_SpatialIndex &data) override;
  void addDictionaryVar(const DRW_DictionaryVar &data) override;
  // PR 8d.2b — four larger no-storage OBJECTS families.
  void addDictionaryWithDefault(const DRW_DictionaryWithDefault &data) override;
  void addSortEntsTable(const DRW_SortEntsTable &data) override;
  void addFieldList(const DRW_FieldList &data) override;
  void addDataTable(const DRW_DataTable &data) override;
  void addDynamicBlockObject(const DRW_DynamicBlockObject &data) override;
  void addField(const DRW_Field &data) override;
  void addAssociativeObject(const DRW_AssociativeObject &data) override;
  void addAcShHistoryObject(const DRW_AcShHistoryObject &data) override;

  void add3dFace(const DRW_3Dface &data) override;
  void addComment(const char *) override;

  void addPlotSettings(const DRW_PlotSettings *data) override;

  // Export:
  bool fileExport(RS_Graphic &g, const QString &file,
                  RS2::FormatType type) override;
#ifdef DWGSUPPORT
  dwgRW::WriteSkipCounters lastDwgWriteSkipCounters() const {
    return m_lastDwgWriteSkipCounters;
  }
  std::size_t lastDwgOptionalReferenceDropCount() const {
    return m_lastDwgOptionalReferenceDropCount;
  }

  bool recordDwgWriteObjectOccurrences(std::uint32_t outputHandle,
                                       const DRW::DwgObjectFrameReceipt &frame);
#endif

  void writeHeader(DRW_Header &data) override;
  void writeDwgClasses() override;
  void writeLType(const UTF8STRING &lTypeName, const UTF8STRING &ltDescription,
                  int ltSize, double ltLength,
                  const std::vector<double> &ltPath);
  bool writeLTypeRecord(DRW_LType &ltype);
  void writeEntities() override;
  void writeLTypes() override;
  void writeLayers() override;
  void writeViews() override;
  void writeUCSs() override;
  void writeTextstyles() override;
  bool writeTextStyleRecord(DRW_Textstyle &textStyle);
  void writeVports() override;
  void writeBlockRecords() override;
  void writeBlocks() override;
  void writeDimstyles() override;
  void prepareDRWDimStyleZerosSuppression(DRW_Dimstyle &d,
                                          const LC_DimStyle *ds);
  void prepareDRWDimStyleArrows(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleScaling(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleExtLine(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleDimLine(DRW_Dimstyle &d, const LC_DimStyle *ds);
  std::uint32_t findLineTypeHandleToWrite(const QString &name) const;
  DRW::Version dimStyleTargetVersion() const;
  void prepareDRWDimStyleText(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleLinearFormat(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleFractions(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleAngularFormat(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleRadial(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleTolerance(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleArc(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleLeader(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyleExtData(DRW_Dimstyle &d, const LC_DimStyle *ds);
  void prepareDRWDimStyle(DRW_Dimstyle &d, const LC_DimStyle *ds);

  void prepareTextStyleName(QString &sty) const;
  void writeObjects() override;
  bool finalizeDwgWrite() override;
  bool finalizeDwgWriteStructure() override;
  void collectDwgAppIds() override;
  void writeAppId() override;

  void writePoint(const RS_Point *p);
  void writeLine(const RS_Line *l);
  void writeCircle(const RS_Circle *c);
  void writeArc(const RS_Arc *a);
  void writeEllipse(const RS_Ellipse *s);
  void writeHyperbola(LC_Hyperbola *h);
  void writeParabola(LC_Parabola *p);
  void writeSolid(RS_Solid *s);
  void writeLWPolyline(RS_Polyline *l);
  void writeSpline(RS_Spline *s);
  void writeSplinePoints(LC_SplinePoints *s);
  void writeInsert(const RS_Insert *i);
  void writeMText(const RS_MText *t);
  void writeText(RS_Text *t);
  void writeHatch(RS_Hatch *h);
  void writeImage(const RS_Image *i);
  void writeWipeout(LC_Wipeout *w);
  enum class DwgEntityProductionResult : std::uint8_t {
    SingleFrame,
    SelfCommittedCompound,
    Failed
  };
  DwgEntityProductionResult writeMLeader(LC_MLeader *m);
  void writeLeader(const RS_Leader *l);
  void writeDimension(RS_Dimension *d);
  void writeTolerance(LC_Tolerance *t);
  void writePolyline(const RS_Polyline *p);

  /*	void writeEntityContainer(DL_WriterA& dw, RS_EntityContainer* con,
                  const DRW_Entity& attrib);
      void writeAtomicEntities(DL_WriterA& dw, RS_EntityContainer* c,
                  const DRW_Entity& attrib, RS2::ResolveLevel level);*/

  void setEntityAttributes(RS_Entity *entity, const DRW_Entity *attrib);
  void getEntityAttributes(DRW_Entity *ent, const RS_Entity *entity);

  static QString toDxfString(const QString &str);
  static QString toNativeString(const QString &data);

  /** Build an LC_SplinePoints from a DRW_Spline boundary edge of a hatch
   *  loop. Degrees 1, 2, and 3 all converge to a quadratic spline-points
   *  representation; cubic input is approximated by 64-point sampling.
   *  Returns nullptr on degenerate input. Exposed for unit tests. */
  static LC_SplinePoints *buildHatchSplineEdge(RS_EntityContainer *hatchLoop,
                                               const DRW_Spline *s);

  /** Snap LC_SplinePoints endpoints inside hatchLoop to neighboring
   *  line-like edge endpoints when within 10× ENDPOINT_TOLERANCE.
   *  Counters boundary-stream float drift in mixed loops. */
  static void snapSplineEdgeEndpoints(RS_EntityContainer *hatchLoop);

public:
  RS_Pen attributesToPen(const DRW_Layer *att) const;

  static RS_Color numberToColor(int num);
  static int colorToNumber(const RS_Color &col, int *rgb);

  static RS2::LineType nameToLineType(const QString &name);
  static QString lineTypeToName(RS2::LineType lineType);
  // static QString lineTypeToDescription(RS2::LineType lineType);

  /// True when raw-preserved OBJECT bytes captured at DWG version `src` can
  /// replay verbatim into target `tgt`. Opaque object frames are only safe
  /// for the exact source version; all cross-version conversions are blocked.
  static bool sameRawObjectEncodingFamily(DRW::Version src, DRW::Version tgt);

  static RS2::LineWidth numberToWidth(DRW_LW_Conv::lineWidth lw);
  static DRW_LW_Conv::lineWidth widthToNumber(RS2::LineWidth width);

  static RS2::AngleFormat numberToAngleFormat(int num);
  static int angleFormatToNumber(RS2::AngleFormat af);

  static RS2::Unit numberToUnit(int num);
  static int unitToNumber(RS2::Unit unit);

  static bool isVariableTwoDimensional(const QString &var);

  static RS_FilterInterface *createFilter() { return new RS_FilterDXFRW(); }

protected:
  enum class DimStyleOverrideParseResult : std::uint8_t {
    Absent,
    Valid,
    Malformed
  };

  void parseDimStyleExtData(const DRW_Dimstyle &s, LC_DimStyle *result);
  bool resolveBlockNameByHandle(std::uint32_t handle,
                                QString &block_name) const;
  DimStyleOverrideParseResult
  parseDimStyleOverride(const LC_ExtEntityData *data,
                        std::unique_ptr<LC_DimStyle> &result) const;
  RS_DimensionData convDimensionData(const DRW_Dimension *data);
  void fillEntityExtData(std::vector<std::shared_ptr<DRW_Variant>> &extData,
                         LC_ExtEntityData *entityData);
  LC_ExtEntityData *extractEntityExtData(
      const std::vector<std::shared_ptr<DRW_Variant>> &extData);
  bool shouldGenerateExtEntityData(const RS_Dimension *entity);
  QString toHexStr(int n);
  void addDimStyleOverrideToExtendedData(LC_ExtEntityData *extEntityData,
                                         LC_DimStyle *styleOverride);
  RS_Layer *importLayerForEntity(const QString &layName,
                                 const std::string &rawLayerName);

private:
  void prepareBlocks();
  void prepareDxfEntityHandleMap();
  void writeEntity(RS_Entity *e);
#ifdef DWGSUPPORT
  void prepareDwgEntityHandleMap();
  enum class DwgExportAdmissionFamily : std::uint8_t {
    BlockDefinition,
    Insert,
    Wipeout,
    OpaqueTable,
    PlotSettings
  };
  enum class DwgExportAdmissionPolicy : std::uint8_t {
    Native,
    Typed,
    SameVersionRaw,
    Refuse
  };
  enum class DwgExportAdmissionReason : std::uint8_t {
    None,
    InvalidBlockDefinition,
    DuplicateBlockDefinition,
    MissingBlockDefinition,
    UnsupportedWipeoutReference,
    InvalidOpaqueTableRaw,
    MissingOpaqueTableSummary,
    AmbiguousOpaqueTableSummary,
    InvalidOpaqueTableSummary,
    OpaqueTableReference,
    UnsupportedPlotSettingsDataStorage,
    UnsupportedPlotSettingsApplicationData,
    RawPlotSettingsUnavailable,
    DuplicateRawPlotSettings,
    SourceChanged
  };
  struct DwgExportAdmissionRecord {
    DwgExportAdmissionFamily family = DwgExportAdmissionFamily::Insert;
    DwgExportAdmissionPolicy policy = DwgExportAdmissionPolicy::Refuse;
    DwgExportAdmissionReason reason = DwgExportAdmissionReason::SourceChanged;
    std::uint32_t sourceHandle = 0;
    std::uint32_t relatedSourceHandle = 0;
    std::size_t sourceOrdinal = std::numeric_limits<std::size_t>::max();
    std::string normalizedName;
  };
  struct DwgBlockDefinitionPlanRecord {
    RS_Block *block = nullptr;
    std::uint32_t sourceHandle = 0;
    std::string normalizedName;
  };
  enum class DwgOpaqueTableReferenceKind : std::uint8_t {
    None,
    RawFrame,
    Owner,
    MissingSummary,
    AmbiguousSummary,
    InvalidSummary,
    TableStyle,
    Unknown,
    TextStyle,
    LineType,
    Value,
    Block,
    Field,
    Attribute,
    Geometry
  };
  enum class DwgOpaqueTableFailurePhase : std::uint8_t {
    Preflight,
    PostClasses,
    Objects,
    Finalized
  };
  enum class DwgOpaqueTableFailureKind : std::uint8_t {
    RawFrame,
    MissingSummary,
    AmbiguousSummary,
    InvalidSummary,
    OwnerGraph,
    SourceGroup,
    RootEntry,
    ChildEntry,
    Reference,
    SourceChanged,
    TargetNodReceipt,
    ChildFrameReceipt,
    RawFrameReceipt,
    TableRecordReceipt
  };
  struct DwgOpaqueTableFailure {
    DwgOpaqueTableFailurePhase phase = DwgOpaqueTableFailurePhase::Preflight;
    DwgOpaqueTableFailureKind kind = DwgOpaqueTableFailureKind::RawFrame;
    std::uint32_t sourceHandle = 0;
    std::uint32_t relatedHandle = 0;
    std::size_t ordinal = std::numeric_limits<std::size_t>::max();

    bool operator==(const DwgOpaqueTableFailure &other) const {
      return phase == other.phase && kind == other.kind &&
             sourceHandle == other.sourceHandle &&
             relatedHandle == other.relatedHandle && ordinal == other.ordinal;
    }

    bool operator!=(const DwgOpaqueTableFailure &other) const {
      return !(*this == other);
    }
  };
  enum class DwgOpaqueTableSummaryKind : std::uint8_t {
    None,
    TableStyle,
    TableContent,
    CellStyleMap
  };
  enum class DwgOpaqueTableReferenceTargetDomain : std::uint8_t {
    None,
    TableRecord
  };
  enum class DwgOpaqueTableReferenceProofPhase : std::uint8_t {
    None,
    TableRecords
  };
  enum class DwgOpaqueTableReceiptStage : std::uint8_t {
    TableRecordsEmitted,
    RawObjectsEmitted,
    Finalized
  };
  struct DwgOpaqueTableRawFingerprint {
    DRW::Version version = DRW::UNKNOWNV;
    int objectType = 0;
    std::uint32_t handle = 0;
    std::uint32_t parentHandle = 0;
    bool commonHandleDataValidated = false;
    DRW_DwgCommonLinkEvidence commonLinkEvidence =
        DRW_DwgCommonLinkEvidence::Unknown;
    std::vector<std::uint32_t> reactorHandles;
    std::uint32_t xDictHandle = 0;
    std::int32_t numReactors = 0;
    std::uint8_t xDictFlag = 0;
    std::uint32_t blockOwnerHandle = 0;
    std::uint32_t bodyBitSize = 0;
    std::uint64_t objectOffset = 0;
    std::uint32_t objectSize = 0;
    bool isEntity = false;
    bool isCustomClass = false;
    bool hasDataStorage = false;
    LC_DwgAdvancedMetadata::ReplayState replayState =
        LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed;
    bool hasClassDefinition = false;
    std::uint16_t classProxyFlag = 0;
    std::string recordName;
    std::string className;
    std::string classAppName;
    bool classWasProxy = false;
    std::uint16_t classEntityFlagRaw = 0;
    std::int32_t classDwgVersion = 0;
    std::int32_t classMaintenanceVersion = 0;
    std::int32_t classUnknown1 = 0;
    std::int32_t classUnknown2 = 0;
    std::size_t rawByteCount = 0;
    std::array<std::uint8_t, 32> rawIdentityDigest{};

    bool operator==(const DwgOpaqueTableRawFingerprint &other) const {
      return version == other.version && objectType == other.objectType &&
             handle == other.handle && parentHandle == other.parentHandle &&
             commonHandleDataValidated == other.commonHandleDataValidated &&
             commonLinkEvidence == other.commonLinkEvidence &&
             reactorHandles == other.reactorHandles &&
             xDictHandle == other.xDictHandle &&
             numReactors == other.numReactors && xDictFlag == other.xDictFlag &&
             blockOwnerHandle == other.blockOwnerHandle &&
             bodyBitSize == other.bodyBitSize &&
             objectOffset == other.objectOffset &&
             objectSize == other.objectSize && isEntity == other.isEntity &&
             isCustomClass == other.isCustomClass &&
             hasDataStorage == other.hasDataStorage &&
             replayState == other.replayState &&
             hasClassDefinition == other.hasClassDefinition &&
             classProxyFlag == other.classProxyFlag &&
             recordName == other.recordName && className == other.className &&
             classAppName == other.classAppName &&
             classWasProxy == other.classWasProxy &&
             classEntityFlagRaw == other.classEntityFlagRaw &&
             classDwgVersion == other.classDwgVersion &&
             classMaintenanceVersion == other.classMaintenanceVersion &&
             classUnknown1 == other.classUnknown1 &&
             classUnknown2 == other.classUnknown2 &&
             rawByteCount == other.rawByteCount &&
             rawIdentityDigest == other.rawIdentityDigest;
    }
  };
  struct DwgOpaqueTableSummaryFingerprint {
    DwgOpaqueTableSummaryKind kind = DwgOpaqueTableSummaryKind::None;
    std::size_t summaryIndex = std::numeric_limits<std::size_t>::max();
    std::uint32_t handle = 0;
    std::uint32_t parentHandle = 0;
    LC_DwgAdvancedMetadata::ReplayState replayState =
        LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed;
    bool fallbackRendered = false;
    bool fallbackInvalidated = false;
    std::size_t relationCount = 0;
    std::array<std::uint8_t, 32> identityDigest{};

    bool operator==(const DwgOpaqueTableSummaryFingerprint &other) const {
      return kind == other.kind && summaryIndex == other.summaryIndex &&
             handle == other.handle && parentHandle == other.parentHandle &&
             replayState == other.replayState &&
             fallbackRendered == other.fallbackRendered &&
             fallbackInvalidated == other.fallbackInvalidated &&
             relationCount == other.relationCount &&
             identityDigest == other.identityDigest;
    }
  };
  struct DwgOpaqueTableReferencePlanRecord {
    DwgOpaqueTableReferenceKind kind = DwgOpaqueTableReferenceKind::None;
    std::size_t producerRawObjectIndex =
        std::numeric_limits<std::size_t>::max();
    std::uint32_t producerSourceHandle = 0;
    std::uint32_t producerOutputHandle = 0;
    std::uint32_t targetSourceHandle = 0;
    std::size_t targetOrdinal = std::numeric_limits<std::size_t>::max();
    DwgOpaqueTableReferenceTargetDomain targetDomain =
        DwgOpaqueTableReferenceTargetDomain::None;
    std::uint32_t targetOutputHandle = 0;
    DwgOpaqueTableReferenceProofPhase proofPhase =
        DwgOpaqueTableReferenceProofPhase::None;

    bool operator==(const DwgOpaqueTableReferencePlanRecord &other) const {
      return kind == other.kind &&
             producerRawObjectIndex == other.producerRawObjectIndex &&
             producerSourceHandle == other.producerSourceHandle &&
             producerOutputHandle == other.producerOutputHandle &&
             targetSourceHandle == other.targetSourceHandle &&
             targetOrdinal == other.targetOrdinal &&
             targetDomain == other.targetDomain &&
             targetOutputHandle == other.targetOutputHandle &&
             proofPhase == other.proofPhase;
    }
  };
  struct DwgOpaqueTableOwnerGraphPlanRecord {
    std::size_t rawObjectIndex = std::numeric_limits<std::size_t>::max();
    std::uint32_t rawSourceHandle = 0;
    std::uint32_t rawOutputHandle = 0;
    std::uint32_t rawOwnerHandle = 0;
    std::size_t rootDictionaryIndex = std::numeric_limits<std::size_t>::max();
    std::size_t childDictionaryIndex = std::numeric_limits<std::size_t>::max();
    std::uint32_t rootSourceHandle = 0;
    std::uint32_t rootOutputHandle = 0;
    std::uint32_t childSourceHandle = 0;
    std::uint32_t childOutputHandle = 0;
    std::size_t rootEntryOrdinal = std::numeric_limits<std::size_t>::max();
    std::size_t childEntryOrdinal = std::numeric_limits<std::size_t>::max();
    std::string rootEntryName;
    std::string childEntryName;
    std::array<std::uint8_t, 32> rootIdentityDigest{};
    std::array<std::uint8_t, 32> childIdentityDigest{};

    bool operator==(const DwgOpaqueTableOwnerGraphPlanRecord &other) const {
      return rawObjectIndex == other.rawObjectIndex &&
             rawSourceHandle == other.rawSourceHandle &&
             rawOutputHandle == other.rawOutputHandle &&
             rawOwnerHandle == other.rawOwnerHandle &&
             rootDictionaryIndex == other.rootDictionaryIndex &&
             childDictionaryIndex == other.childDictionaryIndex &&
             rootSourceHandle == other.rootSourceHandle &&
             rootOutputHandle == other.rootOutputHandle &&
             childSourceHandle == other.childSourceHandle &&
             childOutputHandle == other.childOutputHandle &&
             rootEntryOrdinal == other.rootEntryOrdinal &&
             childEntryOrdinal == other.childEntryOrdinal &&
             rootEntryName == other.rootEntryName &&
             childEntryName == other.childEntryName &&
             rootIdentityDigest == other.rootIdentityDigest &&
             childIdentityDigest == other.childIdentityDigest;
    }

    bool operator!=(const DwgOpaqueTableOwnerGraphPlanRecord &other) const {
      return !(*this == other);
    }
  };
  struct DwgOpaqueTableInspection {
    bool isOpaqueTable = false;
    bool eligible = false;
    std::uint32_t sourceHandle = 0;
    DwgOpaqueTableRawFingerprint rawFingerprint;
    DwgOpaqueTableSummaryFingerprint summaryFingerprint;
    DwgOpaqueTableReferenceKind referenceKind =
        DwgOpaqueTableReferenceKind::None;
    std::uint32_t referenceHandle = 0;
    std::size_t referenceOrdinal = std::numeric_limits<std::size_t>::max();
    std::vector<DwgOpaqueTableReferencePlanRecord> referencePlan;
    std::optional<DwgOpaqueTableOwnerGraphPlanRecord> ownerGraph;
    std::optional<DwgOpaqueTableFailure> preflightFailure;
  };
  struct DwgOpaqueTablePlanRecord {
    std::size_t rawObjectIndex = std::numeric_limits<std::size_t>::max();
    std::size_t admissionRecordIndex = std::numeric_limits<std::size_t>::max();
    DwgOpaqueTableRawFingerprint rawFingerprint;
    DwgOpaqueTableSummaryFingerprint summaryFingerprint;
    DwgOpaqueTableReferenceKind referenceKind =
        DwgOpaqueTableReferenceKind::None;
    std::uint32_t referenceHandle = 0;
    std::size_t referenceOrdinal = std::numeric_limits<std::size_t>::max();
    std::vector<DwgOpaqueTableReferencePlanRecord> referencePlan;
    std::optional<DwgOpaqueTableOwnerGraphPlanRecord> ownerGraph;
    std::optional<DwgOpaqueTableFailure> preflightFailure;
  };
  struct DwgExportAdmissionPlan {
    DRW::Version version = DRW::UNKNOWNV;
    bool valid = true;
    bool frozen = false;
    std::size_t blockListCount = 0;
    std::size_t plotSettingsCount = 0;
    std::size_t rawObjectCount = 0;
    std::size_t dictionaryCount = 0;
    std::size_t tableCount = 0;
    std::size_t cellStyleMapCount = 0;
    std::vector<DwgExportAdmissionRecord> records;
    std::vector<DwgBlockDefinitionPlanRecord> blockDefinitions;
    std::map<const RS_Insert *, std::size_t> insertRecordIndexes;
    std::map<const LC_Wipeout *, std::size_t> wipeoutRecordIndexes;
    std::map<std::size_t, DwgOpaqueTablePlanRecord> opaqueTableRecordIndexes;
    std::map<std::size_t, std::size_t> plotSettingsRecordIndexes;
    std::map<std::uint32_t, std::size_t> rawPlotSettingsRecordIndexes;
    std::set<std::uint32_t> rawPlotSettingsHandles;
  };
  /**
   * Frozen FIELD/FIELDLIST payload graph shared by CLASSES and OBJECTS.
   * FIELDLIST references are valid only when every non-null field target is
   * represented by a candidate in the same graph.
   */
  struct DwgFieldWritePlan {
    struct FieldCandidate {
      std::size_t metadataIndex = std::numeric_limits<std::size_t>::max();
      std::uint32_t sourceHandle = 0;
      LC_DwgAdvancedMetadata::ReplayState replayState =
          LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed;
      LC_DwgAdvancedMetadata::DwgFieldSourceAuthorizationStatus
          sourceAuthorization =
              LC_DwgAdvancedMetadata::DwgFieldSourceAuthorizationStatus::
                  NotDwgSource;
      DRW_Field payload;
      std::vector<std::size_t> childFieldCandidateIndexes;
      bool classAdmitted = false;
      bool objectEmitted = false;
      std::uint32_t outputHandle = 0;
      DRW::DwgObjectFrameReceipt frame;
    };
    struct FieldListCandidate {
      std::size_t metadataIndex = std::numeric_limits<std::size_t>::max();
      std::uint32_t sourceHandle = 0;
      LC_DwgAdvancedMetadata::ReplayState replayState =
          LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed;
      LC_DwgAdvancedMetadata::DwgFieldListSourceAuthorizationStatus
          sourceAuthorization =
              LC_DwgAdvancedMetadata::DwgFieldListSourceAuthorizationStatus::
                  NotDwgSource;
      DRW_FieldList payload;
      std::vector<std::size_t> fieldCandidateIndexes;
      bool classAdmitted = false;
      bool objectEmitted = false;
      std::uint32_t outputHandle = 0;
      DRW::DwgObjectFrameReceipt frame;
    };

    DRW::Version version = DRW::UNKNOWNV;
    bool valid = true;
    bool frozen = false;
    bool classesAdmitted = false;
    std::size_t fieldRecordCount = 0;
    std::size_t fieldListRecordCount = 0;
    std::vector<FieldCandidate> fieldCandidates;
    std::map<std::uint32_t, std::size_t> fieldCandidateIndexes;
    std::vector<FieldListCandidate> fieldListCandidates;
    std::map<std::uint32_t, std::size_t> fieldListCandidateIndexes;
  };
  bool prepareDwgExportAdmissionPlan();
  bool prepareDwgFieldWritePlan();
  bool validateDwgFieldWritePlan(bool requireClassAdmission) const;
  bool validateDwgFieldWriteReceipts() const;
  bool validateDwgExportAdmissionPlan(DwgOpaqueTableFailurePhase phase) const;
  bool validateDwgOpaqueTableReferenceReceipts(
      DwgOpaqueTableReceiptStage stage, DwgOpaqueTableFailurePhase phase) const;
  static const char *
  dwgOpaqueTableFailurePhaseName(DwgOpaqueTableFailurePhase phase);
  static const char *
  dwgOpaqueTableFailureKindName(DwgOpaqueTableFailureKind kind);
  void recordDwgOpaqueTableRuntimeFailure(std::size_t rawObjectIndex,
                                          DwgOpaqueTableFailure failure) const;
  bool isDwgInsertAdmitted(const RS_Insert *insert) const;
  bool isDwgWipeoutAdmitted(const LC_Wipeout *wipeout) const;
  DwgOpaqueTableInspection
  inspectDwgOpaqueTableRawObject(std::size_t rawObjectIndex) const;
  bool isDwgOpaqueTableRawSelected(std::size_t rawObjectIndex) const;
  bool isDwgTypedPlotSettingsSelected(std::size_t index) const;
  bool isDwgRawPlotSettingsSelected(std::uint32_t sourceHandle) const;
  bool prepareDwgEntityWritePlans();
  bool admitDwgEntityWritePlans();
  enum class DwgDataStorageDecision : std::uint8_t {
    EmitTyped,
    EmitRaw,
    LegacyAdvisory,
    Suppress,
    Fail
  };
  struct DwgDataStorageCarrier {
    enum class Kind : std::uint8_t { Typed, Raw };
    enum class EmissionPhase : std::uint8_t { Objects, Section };
    enum class WriterCapability : std::uint8_t { None, Typed, Raw };

    Kind kind = Kind::Typed;
    DwgDataStorageWriterBinding binding = DwgDataStorageWriterBinding::None;
    EmissionPhase phase = EmissionPhase::Objects;
    WriterCapability capability = WriterCapability::None;
    DRW::Version capabilityMinVersion = DRW::UNKNOWNV;
    DRW::Version capabilityMaxVersion = DRW::UNKNOWNV;
    std::string sectionName;
    std::string writerFamily;
    std::string className;
    std::string recordName;
    std::uint16_t classNumber = 0;
    std::uint32_t ownerHandle = 0;
    std::uint64_t storageHandle = 0;
    std::string storageHandleKey;
    bool hasStorageHandle = false;
    bool hasStorageHandleKey = false;
    bool replayAllowed = true;
    std::size_t rawObjectIndex = std::numeric_limits<std::size_t>::max();
  };
  struct DwgDataStorageWriterDescriptor {
    const char *family = "";
    const char *className = "";
    const char *recordName = "";
    std::uint16_t classNumber = 0;
    DRW::Version minVersion = DRW::UNKNOWNV;
    DRW::Version maxVersion = DRW::UNKNOWNV;
    bool supportsDataStorage = true;
    DwgDataStorageWriterBinding binding = DwgDataStorageWriterBinding::None;
    DwgDataStorageCarrier::EmissionPhase phase =
        DwgDataStorageCarrier::EmissionPhase::Objects;
  };
  enum class DwgDataStorageLedgerState : std::uint8_t {
    Planned,
    ClassAdmitted,
    ObjectCommitted,
    SectionCommitted,
    Suppressed,
    Failed
  };
  struct DwgDataStorageIdentityResult {
    std::size_t rawSectionIndex = std::numeric_limits<std::size_t>::max();
    std::size_t recordIndex = std::numeric_limits<std::size_t>::max();
    std::uint64_t numericHandle = 0;
    std::string handleKey;
    bool hasNumericHandle = false;
    bool hasHandleKey = false;
    bool matchesOwnerBySourceValue = false;
    DwgDataStorageIdentityStatus status = DwgDataStorageIdentityStatus::Missing;
  };
  /** Immutable source identity for one source-order DataStorage row. */
  struct DwgDataStorageSourceRowKey {
    std::string sectionName;
    DRW::Version version = DRW::UNKNOWNV;
    std::size_t rawSectionIndex = std::numeric_limits<std::size_t>::max();
    std::size_t recordIndex = std::numeric_limits<std::size_t>::max();

    bool isValid() const {
      return !sectionName.empty() && version != DRW::UNKNOWNV &&
             rawSectionIndex != std::numeric_limits<std::size_t>::max() &&
             recordIndex != std::numeric_limits<std::size_t>::max();
    }

    bool operator==(const DwgDataStorageSourceRowKey &other) const {
      return sectionName == other.sectionName && version == other.version &&
             rawSectionIndex == other.rawSectionIndex &&
             recordIndex == other.recordIndex;
    }

    bool operator!=(const DwgDataStorageSourceRowKey &other) const {
      return !(*this == other);
    }

    bool operator<(const DwgDataStorageSourceRowKey &other) const {
      if (sectionName != other.sectionName)
        return sectionName < other.sectionName;
      if (version != other.version)
        return static_cast<int>(version) < static_cast<int>(other.version);
      if (rawSectionIndex != other.rawSectionIndex)
        return rawSectionIndex < other.rawSectionIndex;
      return recordIndex < other.recordIndex;
    }
  };
  struct DwgDataStorageDecisionRecord {
    DwgDataStorageDecision decision = DwgDataStorageDecision::Fail;
    std::size_t storageIndex = std::numeric_limits<std::size_t>::max();
    std::size_t rawSectionIndex = std::numeric_limits<std::size_t>::max();
    std::size_t recordIndex = std::numeric_limits<std::size_t>::max();
    std::string sectionName;
    std::string writerFamily;
    std::string className;
    std::string recordName;
    std::uint16_t classNumber = 0;
    std::uint32_t ownerHandle = 0;
    std::uint64_t storageHandle = 0;
    std::string storageHandleKey;
    DwgDataStorageWriterBinding binding = DwgDataStorageWriterBinding::None;
    bool identityMatchesOwnerBySourceValue = false;
    DwgDataStorageIdentityStatus identityStatus =
        DwgDataStorageIdentityStatus::Missing;
    DwgDataStorageCarrier::EmissionPhase selectedPhase =
        DwgDataStorageCarrier::EmissionPhase::Objects;
    DwgDataStorageCarrier::WriterCapability selectedCapability =
        DwgDataStorageCarrier::WriterCapability::None;
    std::size_t selectedCarrierIndex = std::numeric_limits<std::size_t>::max();
    std::size_t typedCarrierIndex = std::numeric_limits<std::size_t>::max();
    std::size_t rawCarrierIndex = std::numeric_limits<std::size_t>::max();
    DwgDataStorageSourceRowKey sourceRow;
    std::uint64_t admissionToken = 0;
  };
  struct DwgDataStorageCommitReceipt {
    std::uint64_t generation = 0;
    std::size_t carrierIndex = std::numeric_limits<std::size_t>::max();
    DwgDataStorageCarrier::Kind kind = DwgDataStorageCarrier::Kind::Typed;
    DwgDataStorageWriterBinding binding = DwgDataStorageWriterBinding::None;
    DwgDataStorageCarrier::EmissionPhase phase =
        DwgDataStorageCarrier::EmissionPhase::Objects;
    DwgDataStorageCarrier::WriterCapability capability =
        DwgDataStorageCarrier::WriterCapability::None;
    DwgDataStorageLedgerState state = DwgDataStorageLedgerState::Planned;
    DwgWriteSourceKind sourceKind = DwgWriteSourceKind::Object;
    std::uint32_t expectedBlockOwner = DRW::NoHandle;
    std::uint16_t classNumber = 0;
    std::uint16_t writerClassNumber = 0;
    std::uint64_t admissionToken = 0;
    DwgDataStorageSourceRowKey sourceRow;
    std::string writerFamily;
    std::string className;
    std::string recordName;
    std::uint32_t sourceHandle = 0;
    std::uint32_t outputHandle = 0;
    bool classAdmitted = false;
    bool objectCommitted = false;
    bool sectionCommitted = false;
    bool frameValid = false;
    DRW::DwgObjectFrameReceipt frame;
  };
  struct DwgDataStorageSectionReceipt {
    std::uint64_t generation = 0;
    std::size_t rawSectionIndex = std::numeric_limits<std::size_t>::max();
    std::set<std::size_t> recordIndexes;
    std::set<DwgDataStorageSourceRowKey> sourceRows;
    //! Source-order records copied without an object-frame link. This
    //! includes valid wide identities that cannot enter the UInt32
    //! object-handle graph.
    std::set<std::size_t> opaqueRecordIndexes;
  };
  struct DwgDataStorageReplayPlan {
    DRW::Version version = DRW::UNKNOWNV;
    std::uint64_t generation = 0;
    bool valid = true;
    bool frozen = false;
    std::vector<DwgDataStorageCarrier> carriers;
    std::vector<DwgDataStorageDecisionRecord> decisions;
    std::map<std::uint32_t, std::size_t> decisionByOwner;
    std::map<DwgDataStorageSourceRowKey, std::size_t> decisionBySourceRow;
    std::vector<std::size_t> decisionByCarrier;
    std::map<std::string, std::size_t> decisionByStorageIdentity;
    std::set<std::string> sectionNames;
    std::set<std::size_t> rawSectionIndexes;
    std::map<std::size_t, std::set<std::size_t>> opaqueRecordIndexes;
  };
  struct DwgDataStorageAdmissionEntry {
    std::size_t decisionIndex = std::numeric_limits<std::size_t>::max();
    std::size_t carrierIndex = std::numeric_limits<std::size_t>::max();
    std::uint32_t sourceHandle = 0;
    std::uint32_t outputHandle = 0;
    std::uint32_t expectedBlockOwner = DRW::NoHandle;
    std::uint16_t classNumber = 0;
    std::uint64_t admissionToken = 0;
    DwgDataStorageSourceRowKey sourceRow;
    DwgDataStorageCarrier::Kind kind = DwgDataStorageCarrier::Kind::Typed;
    DwgDataStorageCarrier::EmissionPhase phase =
        DwgDataStorageCarrier::EmissionPhase::Objects;
    DwgDataStorageCarrier::WriterCapability capability =
        DwgDataStorageCarrier::WriterCapability::None;
    DwgWriteSourceKind sourceKind = DwgWriteSourceKind::Object;
    DwgDataStorageWriterBinding binding = DwgDataStorageWriterBinding::None;
    bool classRequired = false;
    bool payloadValidated = false;
    std::uint32_t payloadBitEstimate = 0;
  };
  struct DwgDataStorageAdmissionSummary {
    DRW::Version version = DRW::UNKNOWNV;
    std::uint64_t generation = 0;
    bool valid = false;
    std::vector<DwgDataStorageAdmissionEntry> entries;
  };
  std::vector<DwgDataStorageCarrier> collectDwgDataStorageCarriers() const;
  bool prepareDwgDataStorageReplayPlan(DRW::Version version);
  bool validateDwgDataStoragePreflight(DRW::Version targetVersion) const;
  bool freezeDwgDataStorageReplayPlan();
  bool validateDwgDataStorageAdmissionSummary(bool requireClassAdmission) const;
  bool validateDwgDataStorageReplayReceipts(DRW::Version targetVersion) const;
  bool recordDwgDataStorageClassAdmission(std::uint32_t sourceHandle,
                                          DwgDataStorageCarrier::Kind kind);
  bool bindDwgDataStorageFrame(std::uint32_t sourceHandle,
                               DwgDataStorageCarrier::Kind kind);
  bool recordDwgDataStorageObjectCommit(
      std::uint32_t sourceHandle, DwgDataStorageCarrier::Kind kind,
      DwgWriteSourceKind sourceKind, std::uint32_t outputHandle,
      const DRW::DwgObjectFrameReceipt &frame);
  bool
  recordDwgDataStorageSectionCommit(std::size_t rawSectionIndex,
                                    const std::set<std::size_t> &recordIndexes);
  void failDwgDataStorageCommit(std::uint32_t sourceHandle);
  bool hasDwgDataStorageClassAdmission(std::uint32_t sourceHandle,
                                       DwgDataStorageCarrier::Kind kind) const;
  static DwgDataStorageIdentityResult resolveDwgDataStorageIdentity(
      std::uint32_t ownerHandle, std::uint64_t storageHandle,
      bool hasStorageHandle, const std::string &storageHandleKey,
      bool hasStorageHandleKey,
      std::size_t rawSectionIndex = std::numeric_limits<std::size_t>::max(),
      std::size_t recordIndex = std::numeric_limits<std::size_t>::max());
  static DwgDataStorageSourceRowKey makeDwgDataStorageSourceRowKey(
      const std::string &sectionName, DRW::Version version,
      std::size_t rawSectionIndex, std::size_t recordIndex);
  const DwgDataStorageDecisionRecord *
  dataStorageDecisionForOwner(std::uint32_t ownerHandle) const;
  bool isDwgDataStorageTypedSelected(std::uint32_t ownerHandle) const;
  bool isDwgDataStorageRawSelected(std::uint32_t ownerHandle) const;
  bool stageDwgTableRecordIdentity(DwgWriteSourceKind kind,
                                   std::uint32_t sourceHandle,
                                   std::uint32_t outputHandle);
  bool commitDwgTableRecordIdentities();
  bool commitDwgTableControlIdentities();
  bool commitDwgGeneratedEntityWrite(std::uint32_t outputHandle);
  bool recordDwgEntityFrame(std::uint32_t outputHandle);
  bool commitDwgPolylineCompound(const RS_Entity *anchor);
  bool commitDwgEntityWrite(const RS_Entity *entity);
  void rollbackDwgEntityWrite(const RS_Entity *entity);
  const DwgDataStorageDecisionRecord *
  dataStorageDecisionForCarrier(std::size_t carrierIndex) const;
  bool encodedDwgDataStoragePresenceBit(std::uint32_t sourceHandle,
                                        bool requested) const;
  bool allowsDwgDataStoragePresenceBit(std::uint32_t sourceHandle) const;
  bool canReplayDwgDataStorage(std::uint32_t sourceHandle) const;
  void printDwgError(int le);
  QString strVal(const DRW_Variant *var);
  QString printDwgVersion(int v);
#endif

  /** Pointer to the m_graphic we currently operate on. */
  RS_Graphic *m_graphic = nullptr;
  /** File name. Used to find out the full path of images. */
  QString m_file;
  /** Pointer to current entity container (either block or graphic) */
  RS_EntityContainer *m_currentContainer = nullptr;
  /** MLINESTYLE cache: name → style data. Populated by addMLineStyle as
   *  the OBJECTS section is decoded; consumed by addMLine to compute
   *  per-element offsets when decomposing the MLINE into N polylines. */
  std::map<QString, DRW_MLineStyle> m_mlineStyleCache;

  /** DXF export: raw-net OBJECT handles to NOT re-emit because the codec
   *  regenerates them (the source ACAD_GROUP dictionary, and any object
   *  colliding with the fixed root/group handles C/D). Computed in fileExport
   *  before write(), consumed by the rawDxfObjects re-emit in writeObjects. */
  std::set<std::uint32_t> m_dxfSuppressedObjectHandles;

  /** DXF export (DWG->DXF): SOURCE handles of the named parent dictionaries
   *  emitted via setNamedDictObjects (F4-followup). Computed in fileExport,
   *  consumed by the data-only OBJECT emitters in writeObjects: a data-only
   *  record whose 330 parent is in this set (or 0, or a raw-net handle) keeps
   *  it; otherwise the parent is zeroed so writeObjectOwner emits C and the
   *  object is never a dangling-owner reference. */
  std::set<std::uint32_t> m_dxfEmittedNamedDictHandles;

  /** UNDERLAYDEFINITION cache: handle → definition (filename, sheet, kind).
   *  Populated by linkUnderlay (OBJECTS section, after entities are
   *  parsed). Consumed at export time + by future UI surfaces that
   *  want the filename for a given underlay. Cleared per import. */
  std::map<std::uint32_t, DRW_UnderlayDefinition> m_underlayDefMap;

  /** Raw unsupported DWG payloads kept during import for diagnostics and
   *  future round-trip/semantic decoders. */
  std::vector<DRW_UnsupportedObject> m_unsupportedDwgObjects;

  /** Recursion guard for embedXref. Holds the absolute paths of files
   *  currently being loaded (the host file plus any in-progress XREF
   *  embeds). Refuses re-entry on circular references. */
  std::set<QString> m_xrefStack;
  /** Names of XREF blocks recorded by addBlock() for the current import.
   *  Used after fileImport finishes to warn the user about XREF blocks
   *  that no modelspace INSERT references (orphan XREFs that AutoCAD
   *  typically renders through a paper-space layout viewport, which
   *  LibreCAD doesn't support). */
  std::set<QString> m_xrefBlockNames;
  /** Resolve an XREF path stored in DRW_Block::xrefPath. Tries:
   *  (1) the stored path as absolute, (2) host-dir + stored path,
   *  (3) host-dir + basename, (4) host-dir + basename with case-
   *  insensitive + space-to-underscore matching, (5) same as (4)
   *  but accepting .dxf in place of .dwg. Returns absolute path on
   *  success, empty QString on failure. */
  QString resolveXrefPath(const QString &xrefPath) const;
  /** Embed external file's modelspace contents into @p block as a
   *  read-only XREF resolution. Layers are namespaced
   *  `<blockName>|<layerName>`. Recursion guarded by m_xrefStack.
   *  Returns true on success (block populated), false on failure
   *  (block left as it was). */
  bool embedXref(RS_Block *block, const QString &xrefPath,
                 const QString &blockName);

  /** DWG file format version recognized at openFile() time. Set even
   *  on the BAD_VERSION error path so the user-facing error message
   *  can name which version was found and which range is supported.
   *  UNKNOWNV when the magic header isn't recognized at all. */
  DRW::Version m_dwgVersion = DRW::UNKNOWNV;

  /** Scan @p container for RS_Polyline entities carrying LibreCAD_MLINE
   *  XDATA and reconstruct DRW_MLine entities by grouping siblings.
   *  Consumed polylines are written into @p consumed so the normal
   *  entity-write loop skips them. Polylines without metadata, or
   *  groups missing siblings, are left for plain LWPOLYLINE export. */
  void reconstructMLines(RS_EntityContainer *container,
                         std::set<RS_Entity *> &consumed);

  /** Scan @p container for RS_Polyline fallback geometry carrying
   *  LibreCAD_POLYLINE_MESH / LibreCAD_POLYLINE_PFACE XDATA and
   *  reconstruct native old-style POLYLINE entities when the complete
   *  decomposed group is still present and matches the preserved source
   *  topology. Consumed polylines are written into @p consumed so the
   *  normal entity-write loop skips them. */
  void reconstructPolylineSidecars(RS_EntityContainer *container,
                                   std::set<RS_Entity *> &consumed);

  /** Scan @p container for RS_Polyline entities carrying
   *  LibreCAD_UNDERLAY XDATA and reconstruct DRW_Underlay entities.
   *  Each polyline maps 1:1 to an underlay (no group). Consumed
   *  polylines are written into @p consumed so the normal
   *  entity-write loop skips them. Polylines without metadata fall
   *  through as plain LWPOLYLINEs. */
  void reconstructUnderlays(RS_EntityContainer *container,
                            std::set<RS_Entity *> &consumed);

  /** Scan @p container for RS entities carrying a F2 type-fidelity
   *  sidecar (LibreCAD_RAY / _XLINE / _TRACE / _3DFACE XDATA), rebuild the
   *  native DRW type with full geometry (incl. Z) via the existing
   *  writeRay/writeXline/writeTrace/write3dface, and record the consumed
   *  RS entity so the normal entity-write loop skips it. Entities without
   *  the marker fall through to their lossy default write. */
  void reconstructTypedConversions(RS_EntityContainer *container,
                                   std::set<RS_Entity *> &consumed);

  /** Reconstruct sidecars that each produce one native entity frame.
   *  Unlike mesh/polyface and MLINE reconstruction, these routes do not
   *  consume sibling fallback geometry and are safe in user blocks. */
  bool reconstructSingleFrameSidecars(RS_EntityContainer *container,
                                      std::set<RS_Entity *> &consumed);
  /** File m_codePage. Used to find the text coder. */
  QString m_codePage;
  /** File version. */
  QString m_versionStr;
  int m_version = 0;
  /** Library File version. */
#define LIBDXFRW_VERSION(version, release, patch)                              \
  (((version) << 16) | ((release) << 8) | (patch))
  bool m_isLibDxfRw{false};
  uint m_libDxfRwVersion = 0;
  /** dimension style. */
  QString m_dimStyle;
  /** text style. */
  QString m_textStyle;
  /** Temporary list to handle unnamed blocks to write R12 dxf. */
  QHash<RS_Entity *, QString> m_noNameBlock;
  /** Planned DXF INSERT handles grouped by their referenced block name. */
  std::map<QString, std::vector<std::uint32_t>> m_dxfBlockInsertHandles;
  QHash<QString, QString> m_fontList;
  bool m_oldMText = false;
  dxfRW *m_dxfW{nullptr};
  dxfRW *m_dxfR{nullptr};
#ifdef DWGSUPPORT
  dwgRW *m_dwgW{nullptr};
  DwgExportAdmissionPlan m_dwgExportAdmissionPlan;
  DwgFieldWritePlan m_dwgFieldWritePlan;
  mutable std::map<std::size_t, DwgOpaqueTableFailure>
      m_dwgOpaqueTableRuntimeFailures;
  DwgDataStorageReplayPlan m_dwgDataStorageReplayPlan;
  DwgDataStorageAdmissionSummary m_dwgDataStorageAdmissionSummary;
  std::uint64_t m_nextDwgDataStorageReplayGeneration{1};
  std::uint64_t m_nextDwgDataStorageAdmissionToken{1};
  std::map<std::size_t, DwgDataStorageSectionReceipt>
      m_dwgDataStorageSectionReceipts;
  std::map<std::uint32_t, DwgDataStorageCarrier::Kind>
      m_dwgDataStorageCommittedKinds;
  std::map<std::uint32_t, DwgDataStorageCommitReceipt>
      m_dwgDataStorageCommitReceipts;
  std::set<std::uint32_t> m_dwgWriteDataStorageKnownHandles;
  dwgRW::WriteSkipCounters m_lastDwgWriteSkipCounters;
  std::size_t m_lastDwgOptionalReferenceDropCount{0};
  // Test-only boundary between class admission and later section callbacks.
  std::function<void()> m_dwgPostClassesTestHook;
  // Test-only boundary after OBJECTS emission and before final receipts.
  std::function<void()> m_dwgPostObjectsTestHook;
#endif
  // DRW_Interface write callbacks are void. Preserve callback failures until
  // fileExport can return them to the caller instead of reporting success.
  bool m_writeFailed{false};
  /** Latch a typed DXF writer failure across the void callback boundary. */
  bool noteDxfWrite(bool success) {
    if (!success)
      m_writeFailed = true;
    return success;
  }
  /** If saved version are 2004 or above can save color in RGB value. */
  bool m_exactColor = false;
  /** hash of block containers and handleBlock numbers to read dwg files */
  QHash<int, RS_EntityContainer *> m_blockHash;
  /** Per-import layer cache keyed by NFC-normalized name. */
  QHash<QString, RS_Layer *> m_importLayerCache;
  /** Fast-path mirror of m_importLayerCache keyed by the RAW (pre-
   *  normalization) name, checked first in importLayerForEntity() so a
   *  repeated layer name (the common case: few distinct layers, many
   *  entities) skips the NFC-normalization pass entirely on a hit. Falls
   *  back to m_importLayerCache (which does normalize) on a miss, so this
   *  is purely additive -- never the only source of truth for a name. */
  QHash<QString, RS_Layer *> m_importLayerRawCache;
  /** Pointer to entity container to store possible orphan entities like paper
   * space */
  RS_EntityContainer *m_dummyContainer = nullptr;
  void applyParsedDimStyleExtData(const LC_DimStyle *dimStyle,
                                  const QString &appName,
                                  const std::vector<DRW_Variant> &vector);
  LC_DimStyle *createDimStyle(const DRW_Dimstyle &s);
  void
  addPolylineSegment(RS_Polyline &polyline, const RS_Vector &previousPosition,
                     const RS_Vector &currentPosition, double segmentBulge,
                     double nextBulge,
                     const std::vector<std::shared_ptr<DRW_Variant>> &extData);
  /**
   * Handle degree-2 SPLINE with exactly 3 control points (rational quadratic
   * conic).
   * @return true if a conic entity (hyperbola or parabola) was created and
   * handled
   */
  bool handleQuadraticConicSpline(const DRW_Spline *data);
};

#endif
