/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 or later.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation,
** Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
****************************************************************************/

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "drw_interface.h"
#include "drw_classes.h"
#include "drw_objects.h"
#include "intern/dwgutil.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgreader.h"
#include "intern/dwgreader15.h"
#include "intern/dwgreader18.h"
#include "intern/dwgreader27.h"
#include "intern/dwgreader32.h"
#include "intern/dwgreaderR11.h"
#include "intern/dwgreader21.h"
#include "intern/dwgobjectframe.h"
#include "intern/rscodec.h"
#include "intern/dwgsafety.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter24.h"
#include "libdwgr.h"
#include "lc_dwgadvancedmetadata.h"

class DwgBlockOwnershipTestAccess {
public:
    static void setHandles(DRW_Block_Record& record, std::uint32_t block,
                           std::uint32_t endBlock,
                           std::vector<std::uint32_t> entityHandles) {
        record.block = block;
        record.endBlock = endBlock;
        record.entMap = std::move(entityHandles);
    }

    static void setHandle(DRW_Block_Record& record, std::uint32_t handle) {
        record.handle = handle;
    }

    static void setLegacyEntityChain(DRW_Block_Record& record,
                                     std::uint32_t first,
                                     std::uint32_t last) {
        record.firstEH = first;
        record.lastEH = last;
    }

};

class DwgPolylineTraceTestAccess {
public:
    enum class Phase {
        CommonObject,
        CommonEed,
        CommonGraphics,
        CommonState,
        CommonAppearance,
        ParentBody,
        CommonHandles,
        OwnedHandles,
        SeqEnd,
        Complete
    };

    struct Result {
        Phase phase {Phase::CommonObject};
        std::int16_t objectType {0};
        std::uint64_t bodyEndBit {0};
        std::uint64_t handleEndBit {0};
        std::int32_t ownedObjectCount {0};
        std::uint16_t firstEedDataSize {0};
        std::uint8_t firstEedItemCode {0};
        bool stringStreamBoundsValid {false};
        bool commonDataWithStringStream {false};
        bool commonDataWithoutStringStream {false};
    };

    static Result trace(DRW_Polyline& polyline, DRW::Version version,
                        dwgBuffer& source, std::uint32_t handleBitSize) {
        Result result;
        if (source.size() > std::numeric_limits<std::uint64_t>::max() / 8
            || handleBitSize > source.size() * 8) {
            return result;
        }

        const std::uint32_t objectDataBitSize = static_cast<std::uint32_t>(
            source.size() * 8 - handleBitSize);
        std::uint64_t ignoredStringStart = 0;
        std::uint64_t ignoredStringEnd = 0;
        result.stringStreamBoundsValid =
            source.getR2007StringStreamBounds(objectDataBitSize,
                                               ignoredStringStart,
                                               ignoredStringEnd);

        dwgBuffer common = source.forkIndependent();
        const auto withinCommonData = [&common, &ignoredStringStart]() {
            return common.isGood()
                && currentBit(common) <= ignoredStringStart;
        };
        result.objectType = static_cast<std::int16_t>(common.getObjType(version));
        common.getHandle();
        if (!result.stringStreamBoundsValid || !withinCommonData())
            return result;

        dwgBuffer eedProbe = common.forkIndependent();
        result.firstEedDataSize = eedProbe.getBitShort();
        if (result.firstEedDataSize != 0 && eedProbe.isGood()) {
            eedProbe.getHandle();
            result.firstEedItemCode = eedProbe.getRawChar8();
        }
        std::vector<DwgEedChunk> eed;
        if (!readDwgEed(version, common, eed, ignoredStringStart)
            || !withinCommonData()) {
            result.phase = Phase::CommonEed;
            return result;
        }

        if (common.getBit() != 0) {
            const std::uint64_t graphicsSize = common.getBitLongLong();
            if (!withinCommonData()
                || graphicsSize > common.numRemainingBytes()
                || graphicsSize
                       > static_cast<std::uint64_t>(
                           std::numeric_limits<std::int32_t>::max() / 8)
                || !common.moveBitPos(
                    static_cast<std::int32_t>(graphicsSize * 8))) {
                result.phase = Phase::CommonGraphics;
                return result;
            }
        }

        common.get2Bits();
        common.getBitLong();
        common.getBit();
        common.getBit();
        if (!withinCommonData()) {
            result.phase = Phase::CommonState;
            return result;
        }

        common.getEnColor(version);
        common.getBitDouble();
        common.get2Bits();
        common.get2Bits();
        common.get2Bits();
        common.getRawChar8();
        common.getBit();
        common.getBit();
        common.getBit();
        common.getBitShort();
        common.getRawChar8();
        if (!withinCommonData()) {
            result.phase = Phase::CommonAppearance;
            return result;
        }

        polyline.resetDwgState();
        dwgBuffer body = source.forkIndependent();
        dwgBuffer strings = body.forkIndependent();
        result.commonDataWithStringStream =
            polyline.DRW_Entity::parseDwg(
                version, &body, &strings, handleBitSize) && body.isGood();
        if (!result.commonDataWithStringStream) {
            polyline.resetDwgState();
            body = source.forkIndependent();
            result.commonDataWithoutStringStream =
                polyline.DRW_Entity::parseDwg(
                    version, &body, nullptr, handleBitSize) && body.isGood();
        }
        if (!result.commonDataWithStringStream
            && !result.commonDataWithoutStringStream)
            return result;

        result.objectType = polyline.oType;
        result.bodyEndBit = polyline.dwgDataEndBit;
        if (polyline.oType != 0x0F)
            return result;

        const auto withinBody = [&body, &result]() {
            return body.isGood() && currentBit(body) <= result.bodyEndBit;
        };
        body.getBitShort();
        body.getBitShort();
        body.getBitDouble();
        body.getBitDouble();
        body.getThickness(version > DRW::AC1014);
        body.getBitDouble();
        body.getExtrusion(version > DRW::AC1014);
        if (!withinBody()) {
            result.phase = Phase::ParentBody;
            return result;
        }

        result.ownedObjectCount = body.getBitLong();
        if (!withinBody()
            || !dwgSafety::validOwnedObjectCount(
                result.ownedObjectCount, body.numRemainingBytes())) {
            result.phase = Phase::ParentBody;
            return result;
        }

        if (polyline.objSize > std::numeric_limits<std::uint64_t>::max()
                                      - handleBitSize) {
            result.phase = Phase::CommonHandles;
            return result;
        }
        result.handleEndBit = version > DRW::AC1021
            ? static_cast<std::uint64_t>(polyline.objSize) + handleBitSize
            : source.size() * 8;
        if (result.handleEndBit > source.size() * 8) {
            result.phase = Phase::CommonHandles;
            return result;
        }

        dwgBuffer handles = body.forkIndependent();
        if (!polyline.parseDwgEntHandle(version, &handles, true,
                                        result.handleEndBit)
            || !handles.isGood() || currentBit(handles) > result.handleEndBit) {
            return result;
        }

        for (std::int32_t i = 0; i < result.ownedObjectCount; ++i) {
            handles.getOffsetHandle(polyline.handle);
            if (!handles.isGood() || currentBit(handles) > result.handleEndBit) {
                result.phase = Phase::OwnedHandles;
                return result;
            }
        }
        handles.getOffsetHandle(polyline.handle);
        if (!handles.isGood() || currentBit(handles) > result.handleEndBit) {
            result.phase = Phase::SeqEnd;
            return result;
        }

        result.phase = Phase::Complete;
        return result;
    }

private:
    static std::uint64_t currentBit(const dwgBuffer& buffer) {
        return buffer.getPosition() * 8 + buffer.getBitPos();
    }
};

namespace {

std::filesystem::path findTs1Fixture() {
    const char *root = std::getenv("LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR");
    if (root == nullptr || *root == '\0')
        return {};

    const std::filesystem::path base(root);
    const std::array<std::filesystem::path, 3> candidates = {
        base / "ts1_2000_fields.dwg",
        base / "2000" / "TS1.dwg",
        base / "test" / "test-data" / "2000" / "TS1.dwg",
    };
    for (const auto &candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate))
            return candidate;
    }
    return {};
}

std::filesystem::path localFixture(const char *relative) {
    return std::filesystem::path(LIBRECAD_TEST_DIR) / relative;
}

bool hasLocalFixture(const char *relative) {
    return std::filesystem::is_regular_file(localFixture(relative));
}

class DwgHandleReaderProbe final : public dwgReader15 {
public:
    using dwgReader::DwgFrameMapLease;
    using dwgReader::DwgStagedFrame;
    using dwgReader::DwgSourceFrameLease;
    using dwgReader::readDwgHandles;
    using dwgReader::takeDwgSourceFrame;
    using dwgReader::detachDwgSourceFrame;
    using dwgReader::stageDetachedDwgSourceFrame;
    using dwgReader::restoreDwgSourceFrame;
    using dwgReader::deferDetachedDwgSourceFrame;
    using dwgReader::discardDetachedDwgSourceFrame;
    using dwgReader::deferDwgSourceFrame;
    using dwgReader::discardDwgSourceFrame;
    using dwgReader::addIntegrityDiagnostic;
    using dwgReader::finalizeDwgFrameCoverageNoThrow;
    using dwgReader::markDwgFrameOutcome;
    using dwgReader::publishDeferredRawObjects;
    using dwgReader::publishDwgFramePublication;
    using dwgReader::quarantineDwgFrame;
    using dwgReader::recordObjectFrameFailure;
    using dwgReader::suppressDwgFramePublication;
    using dwgReader::stageCurrentEntityFrame;
    using dwgReader::restoreStagedFrame;
    using dwgReader::abandonStagedFrame;

    explicit DwgHandleReaderProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader15(std::move(buffer), nullptr) {}
};

class DwgR2018FixtureReaderProbe final : public dwgReader32 {
public:
    enum class ReadStage : std::uint8_t {
        Metadata,
        FileHeader,
        Header,
        Classes,
        Handles,
        Tables,
        TablePublications,
        Blocks,
        Complete
    };

    explicit DwgR2018FixtureReaderProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader32(std::move(buffer), nullptr) {}

    bool readMetaData() override {
        version = DRW::AC1032;
        decoder.setVersion(version, false);
        if (!fileBuf->setPosition(11))
            return false;
        maintenanceVersion = fileBuf->getRawChar8();
        fileBuf->getRawChar8();
        previewImagePos = fileBuf->getRawLong32();
        fileBuf->getRawChar8();
        appMaintenanceVersion = fileBuf->getRawChar8();
        const std::uint16_t codePage = fileBuf->getRawShort16();
        if (const char* name = dwgCodePageName(codePage))
            decoder.setByteCodePage(name);
        return fileBuf->isGood();
    }

    bool tracePolylineForTest(
        std::uint32_t handle, DwgPolylineTraceTestAccess::Result& result,
        DwgObjectFrame& frame) {
        const auto object = ObjectMap.find(handle);
        const auto section = sections.find(secEnum::OBJECTS);
        if (object == ObjectMap.end() || section == sections.end()
            || section->second.size == 0)
            return false;

        std::unique_ptr<std::uint8_t[]> objectData;
        std::uint64_t objectDataSize = 0;
        if (!parseDataPage(section->second, objectData, objectDataSize)
            || objectData == nullptr || section->second.size > objectDataSize)
            return false;

        dwgBuffer objects(objectData.get(), section->second.size, &decoder);
        if (!frame.readAt(objects, version, object->second.loc))
            return false;
        dwgBuffer body(frame.body().data(), frame.body().size(), &decoder);
        DRW_Polyline polyline;
        result = DwgPolylineTraceTestAccess::trace(
            polyline, version, body, frame.bodyBitSize());
        return true;
    }

    std::size_t objectMapSizeForTest() const { return ObjectMap.size(); }

    bool readThroughBlocksForTest(DRW_Interface& intfa) {
        DRW_Header header;
        m_readStage = ReadStage::Metadata;
        if (!readMetaData())
            return false;
        m_readStage = ReadStage::FileHeader;
        if (!readFileHeader())
            return false;
        m_readStage = ReadStage::Header;
        if (!readDwgHeader(header))
            return false;
        m_readStage = ReadStage::Classes;
        if (!readDwgClasses())
            return false;
        m_readStage = ReadStage::Handles;
        if (!readDwgHandles())
            return false;
        m_readStage = ReadStage::Tables;
        if (!readDwgTables(header))
            return false;
        m_readStage = ReadStage::TablePublications;
        if (!publishDeferredTableFramePublications(intfa))
            return false;
        m_readStage = ReadStage::Blocks;
        if (!readDwgBlocks(intfa))
            return false;
        m_readStage = ReadStage::Complete;
        return true;
    }

    const std::vector<DwgEntityFailureDiagnostic>&
    entityFailureDiagnosticsForTest() const {
        return m_entityFailureDiagnostics;
    }

    ReadStage readStageForTest() const { return m_readStage; }

    std::size_t entityParseFailuresForTest() const {
        return m_entityParseFailures;
    }

private:
    ReadStage m_readStage {ReadStage::Metadata};
};

class DwgLegacyReaderProbe final : public dwgReaderR11 {
public:
    explicit DwgLegacyReaderProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReaderR11(std::move(buffer), nullptr) {}
};

class DwgAc1015FixtureReaderProbe final : public dwgReader15 {
public:
    using dwgReader::readDwgEntity;
    using dwgReader::readDwgObject;
    using dwgReader::walkBlockRecordEntities;

    explicit DwgAc1015FixtureReaderProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader15(std::move(buffer), nullptr) {}

    bool readMetaData() override {
        version = DRW::AC1015;
        decoder.setVersion(version, false);
        if (!fileBuf->setPosition(13))
            return false;
        previewImagePos = fileBuf->getRawLong32();
        fileBuf->getRawShort16();
        fileBuf->getRawShort16();
        return fileBuf->isGood();
    }

    [[nodiscard]] std::size_t stagedPendingInsertCountForTest() const {
        return m_pendingInsertStates.size();
    }

    [[nodiscard]] std::size_t stagedOrphanAttribCountForTest() const {
        return m_orphanAttribStates.size();
    }

    [[nodiscard]] std::size_t stagedSeqEndCountForTest() const {
        return m_stagedSeqEnds.size();
    }

};

class DwgEntityReaderProbe final : public dwgReader15 {
public:
    using dwgReader::DwgBlockJournalOutput;
    using dwgReader::DwgBlockScopeTransaction;
    using dwgReader::DwgFrameMapLease;
    using dwgReader::abandonDeferredCompoundState;
    using dwgReader::readDwgHandles;
    using dwgReader::readDwgBlocks;
    using dwgReader::readDwgEntities;
    using dwgReader::readDwgEntity;
    using dwgReader::readDwgObject;
    using dwgReader::readDwgObjects;
    using dwgReader::requiresLegacyCompoundHandling;
    using dwgReader::hasPendingCompoundStateForBlock;
    using dwgReader::validateDeferredCompoundState;
    using dwgReader::walkBlockRecordEntities;

    explicit DwgEntityReaderProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader15(std::move(buffer), nullptr) {}

    void setVersionForTest(DRW::Version value) { version = value; }

    void addAppIdForTest(std::uint32_t handle, const std::string& name) {
        auto appId = std::make_unique<DRW_AppId>();
        appId->name = name;
        appIdmap.emplace(handle, appId.release());
    }

    void addLayerForTest(std::uint32_t handle, const std::string& name) {
        auto layer = std::make_unique<DRW_Layer>();
        layer->name = name;
        layermap.emplace(handle, layer.release());
    }

    void setCodePageForTest(const std::string& value) {
        decoder.setVersion(version, false);
        setCodePage(value);
    }

    bool hasDeferredClassificationForTest(
        std::uint32_t handle, std::int16_t encodedType,
        std::int16_t resolvedType) const {
        const auto it = m_deferredFrameClassifications.find(handle);
        return it != m_deferredFrameClassifications.end()
            && it->second.encodedType == encodedType
            && it->second.resolvedType == resolvedType
            && it->second.route == DwgFrameClassification::Route::Object;
    }

    void forgeDeferredResolvedTypeForTest(
        std::uint32_t handle, std::int16_t resolvedType) {
        m_deferredFrameClassifications.at(handle).resolvedType = resolvedType;
    }

    bool hasFramePhaseSnapshotForTest(
        std::uint32_t handle, std::int16_t encodedType,
        std::int16_t resolvedType, bool terminalSuccess) const {
        return std::any_of(
            m_dwgFramePhaseSnapshots.cbegin(),
            m_dwgFramePhaseSnapshots.cend(),
            [=](const DwgFramePhaseSnapshot& snapshot) {
                const bool expectedTerminal = terminalSuccess;
                const DwgFrameMapLease::Origin expectedOrigin =
                    expectedTerminal
                        ? DwgFrameMapLease::Origin::DeferredObjectMap
                        : DwgFrameMapLease::Origin::ObjectMap;
                const DwgFramePhaseSnapshot::Destination expectedDestination =
                    expectedTerminal
                        ? DwgFramePhaseSnapshot::Destination::Published
                        : DwgFramePhaseSnapshot::Destination::DeferredObjectMap;
                const DRW_DwgFrameDisposition expectedDisposition =
                    expectedTerminal
                        ? DRW_DwgFrameDisposition::Published
                        : DRW_DwgFrameDisposition::Deferred;
                return snapshot.source.handle == handle
                    && snapshot.classification.encodedType == encodedType
                    && snapshot.classification.resolvedType == resolvedType
                    && snapshot.origin == expectedOrigin
                    && snapshot.destination == expectedDestination
                    && snapshot.disposition == expectedDisposition;
            });
    }

    bool hasFailedDeferredObjectSnapshotForTest(
        std::uint32_t handle, std::int16_t encodedType,
        std::int16_t resolvedType) const {
        return std::any_of(
            m_dwgFramePhaseSnapshots.cbegin(),
            m_dwgFramePhaseSnapshots.cend(),
            [=](const DwgFramePhaseSnapshot& snapshot) {
                return snapshot.source.handle == handle
                    && snapshot.classification.encodedType == encodedType
                    && snapshot.classification.resolvedType == resolvedType
                    && snapshot.origin
                        == DwgFrameMapLease::Origin::DeferredObjectMap
                    && snapshot.destination
                        == DwgFramePhaseSnapshot::Destination::Failed
                    && snapshot.disposition
                        == DRW_DwgFrameDisposition::Failed;
            });
    }

    void setExpectedBlockOwnerForTest(std::uint32_t value) {
        expectedBlockEntityOwner = value;
    }

    void setOwnerlessSpaceWalkForTest(bool value) {
        ownerlessSpaceWalk = value;
    }

    bool parsedHandleMismatchForTest() const {
        return parsedEntityHandleMismatch;
    }

    void addCustomEntityClass(std::uint16_t classNumber,
                              const char* recordName = "CUSTOM_ENTITY") {
        auto* customClass = new DRW_Class();
        customClass->recName = recordName;
        customClass->className = "AcDbCustomEntity";
        customClass->appName = "TEST_APP";
        customClass->proxyFlag = 0x1234;
        customClass->wasaProxyFlag = 1;
        customClass->entityFlag = 1;
        customClass->entityFlagRaw = 0x1F2;
        customClass->dwgVersion = 1027;
        customClass->maintenanceVersion = 329;
        customClass->unknown1 = 17;
        customClass->unknown2 = 23;
        customClass->classNum = classNumber;
        customClass->dwgType = 0;
        classesmap[classNumber] = customClass;
    }

    void addCustomObjectClass(std::uint16_t classNumber,
                              const char* recordName = "CUSTOM_OBJECT") {
        auto* customClass = new DRW_Class();
        customClass->recName = recordName;
        customClass->className = "AcDbCustomObject";
        customClass->appName = "TEST_APP";
        customClass->proxyFlag = 0x1234;
        customClass->wasaProxyFlag = 1;
        customClass->entityFlag = 0;
        customClass->entityFlagRaw = 0x1F3;
        customClass->dwgVersion = 1027;
        customClass->maintenanceVersion = 329;
        customClass->unknown1 = 17;
        customClass->unknown2 = 23;
        customClass->classNum = classNumber;
        customClass->dwgType = 0;
        classesmap[classNumber] = customClass;
    }

    void addFieldListObjectClass(std::uint16_t classNumber) {
        addCustomObjectClass(classNumber, "FIELDLIST");
        classesmap.at(classNumber)->className = "AcDbFieldList";
    }

    void addFieldObjectClass(std::uint16_t classNumber) {
        addCustomObjectClass(classNumber, "FIELD");
        classesmap.at(classNumber)->className = "AcDbField";
    }

    void setClassStreamOrdinalForTest(std::uint16_t classNumber,
                                      std::uint64_t ordinal) {
        m_dwgClassNumberOrdinals[classNumber] = ordinal;
    }

    void addGeoPositionMarkerClass(std::uint16_t classNumber) {
        auto* customClass = new DRW_Class();
        customClass->recName = "GEOPOSITIONMARKER";
        customClass->className = "AcDbGeoPositionMarker";
        customClass->entityFlag = 1;
        customClass->classNum = classNumber;
        customClass->dwgType = DRW_GeoPositionMarker::kDwgType;
        classesmap[classNumber] = customClass;
    }

    int customEntityDwgType(std::uint16_t classNumber) const {
        return classesmap.at(classNumber)->dwgType;
    }

    bool stagePendingInsertForTest(
        const DRW_Insert& insert,
        const DRW_DwgFramePublication& publication) {
        auto sourceIt = ObjectMap.find(insert.handle);
        if (sourceIt == ObjectMap.end()
            || m_pendingInsertStates.find(insert.handle)
                   != m_pendingInsertStates.end()) {
            return false;
        }
        DwgFrameMapLease lease;
        if (!detachDwgSourceFrame(ObjectMap, sourceIt, lease)
            || !stageDetachedDwgSourceFrame(lease)) {
            return false;
        }
        PendingInsertState pending;
        pending.entity = insert;
        pending.frame.lease.emplace(std::move(lease));
        if (!stageCurrentEntityFrame(pending.frame, publication)) {
            (void)restoreStagedFrame(pending.frame);
            return false;
        }
        try {
            m_pendingInsertStates.reserve(m_pendingInsertStates.size() + 1u);
        } catch (...) {
            (void)restoreStagedFrame(pending.frame);
            return false;
        }
        return m_pendingInsertStates.emplace(
            insert.handle, std::move(pending)).second;
    }

    bool stageReceiptlessPendingInsertForTest(const DRW_Insert& insert) {
        auto sourceIt = ObjectMap.find(insert.handle);
        if (sourceIt == ObjectMap.end()
            || m_pendingInsertStates.find(insert.handle)
                   != m_pendingInsertStates.end()) {
            return false;
        }
        DwgFrameMapLease lease;
        if (!detachDwgSourceFrame(ObjectMap, sourceIt, lease)
            || !stageDetachedDwgSourceFrame(lease)) {
            return false;
        }
        PendingInsertState pending;
        pending.entity = insert;
        pending.frame.lease.emplace(std::move(lease));
        try {
            m_pendingInsertStates.reserve(m_pendingInsertStates.size() + 1u);
        } catch (...) {
            (void)restoreStagedFrame(pending.frame);
            return false;
        }
        return m_pendingInsertStates.emplace(
            insert.handle, std::move(pending)).second;
    }

    [[nodiscard]] std::size_t stagedPendingInsertCountForTest() const {
        return m_pendingInsertStates.size();
    }

    [[nodiscard]] std::size_t stagedOrphanAttribCountForTest() const {
        return m_orphanAttribStates.size();
    }

    [[nodiscard]] std::size_t stagedSeqEndCountForTest() const {
        return m_stagedSeqEnds.size();
    }

    [[nodiscard]] std::size_t consumedPolylineChildCountForTest() const {
        return m_consumedCompoundChildHandles.size();
    }

    [[nodiscard]] std::size_t consumedSeqEndCountForTest() const {
        return m_consumedSeqEndHandles.size();
    }

    [[nodiscard]] std::size_t stagedPendingPolylineCountForTest() const {
        return m_pendingPolylineStates.size();
    }

    [[nodiscard]] std::size_t stagedOrphanPolylineVertexCountForTest() const {
        return m_orphanPolylineVertexStates.size();
    }

    [[nodiscard]] std::size_t stagedPolylineVertexCountForTest(
        std::uint32_t handle) const {
        const auto it = m_pendingPolylineStates.find(handle);
        return it == m_pendingPolylineStates.end() ? 0u
                                                    : it->second.vertices.size();
    }

    [[nodiscard]] std::uint32_t stagedPendingPolylineHandleForTest() const {
        return m_pendingPolylineStates.empty()
            ? DRW::NoHandle : m_pendingPolylineStates.begin()->first;
    }

    DRW_DwgFramePublication publicationForTest(std::uint32_t handle) const {
        const objHandle& object = ObjectMap.at(handle);
        DRW_DwgFramePublication publication;
        publication.m_handle = object.handle;
        publication.m_sourceOffset = object.loc;
        publication.m_sourceMapOrdinal = object.sourceOrdinal;
        publication.m_sourceOffsetSpace = object.sourceOffsetSpace;
        publication.m_hasSourceLocation = true;
        return publication;
    }

    bool publishFrameForTest(DRW_Interface& intfa,
                             DRW_DwgFramePublication publication) {
        return publishDwgFramePublication(intfa, std::move(publication));
    }

    bool stageInsertForTest(DRW_Insert insert,
                            const DRW_DwgFramePublication& publication,
                            DRW_Interface& intfa, bool& committed) {
        return withActiveFrameForTest(insert.handle, [&]() {
            const DwgMappedEntityOutcome outcome = stagePendingInsert(
                std::move(insert), publication, intfa);
            committed = outcome == DwgMappedEntityOutcome::CommittedCompound;
            return outcome != DwgMappedEntityOutcome::Rejected;
        });
    }

    bool stageAttributeForTest(std::shared_ptr<DRW_Attrib> attribute,
                               const DRW_DwgFramePublication& publication,
                               DRW_Interface& intfa, bool& committed) {
        if (attribute == nullptr)
            return false;
        const std::uint32_t handle = attribute->handle;
        return withActiveFrameForTest(handle, [&]() {
            const DwgMappedEntityOutcome outcome = stagePendingAttribute(
                std::move(attribute), publication, intfa);
            committed = outcome == DwgMappedEntityOutcome::CommittedCompound;
            return outcome != DwgMappedEntityOutcome::Rejected;
        });
    }

    bool stageSeqEndForTest(std::uint32_t handle, std::uint32_t owner,
                            const DRW_DwgFramePublication& publication,
                            DRW_Interface& intfa, bool& committed) {
        return withActiveFrameForTest(handle, [&]() {
            const DwgMappedEntityOutcome outcome = stagePendingSeqEnd(
                handle, owner, publication, intfa);
            committed = outcome == DwgMappedEntityOutcome::CommittedCompound;
            return outcome != DwgMappedEntityOutcome::Rejected;
        });
    }

    void failBeforeSeqEndTerminalizerMarkerForTest() {
        setTerminalizerFailurePointForTest(
            DwgTerminalizerFailurePoint::BeforeSeqEndMarker);
    }

    void failBeforeInsertOwnerTerminalizerMarkerForTest() {
        setTerminalizerFailurePointForTest(
            DwgTerminalizerFailurePoint::BeforeInsertOwnerMarker);
    }

    void failBeforeOrphanOwnerTerminalizerMarkerForTest() {
        setTerminalizerFailurePointForTest(
            DwgTerminalizerFailurePoint::BeforeOrphanOwnerMarker);
    }

    bool failPolylinePreparationForTest(std::uint8_t index) {
        switch (index) {
        case 0:
            setPolylinePrepareFailurePointForTest(
                DwgPolylinePrepareFailurePoint::BeforeReservation);
            return true;
        case 1:
            setPolylinePrepareFailurePointForTest(
                DwgPolylinePrepareFailurePoint::BeforeChildMarker);
            return true;
        case 2:
            setPolylinePrepareFailurePointForTest(
                DwgPolylinePrepareFailurePoint::BeforeSeqEndMarker);
            return true;
        default:
            return false;
        }
    }

    bool failPolylineStagingForTest(std::uint8_t index) {
        switch (index) {
        case 0:
            setPolylineStageFailurePointForTest(
                DwgPolylineStageFailurePoint::BeforeParentStateReserve);
            return true;
        case 1:
            setPolylineStageFailurePointForTest(
                DwgPolylineStageFailurePoint::BeforeParentStateInsert);
            return true;
        case 2:
            setPolylineStageFailurePointForTest(
                DwgPolylineStageFailurePoint::BeforeFrameStaging);
            return true;
        case 3:
            setPolylineStageFailurePointForTest(
                DwgPolylineStageFailurePoint::BeforeOrphanAdoption);
            return true;
        default:
            return false;
        }
    }

    void failBlockJournalReservationForTest(
        std::size_t occurrence = 1u) {
        setBlockJournalReservationFailurePointForTest(
            DwgBlockJournalReservationFailurePoint::BeforeReserve,
            occurrence);
    }

    void failBlockJournalAdoptionForTest(
        std::size_t occurrence = 1u) {
        setBlockJournalAdoptFailurePointForTest(
            DwgBlockJournalAdoptFailurePoint::BeforeTransfer,
            occurrence);
    }

    bool abandonStagedCompoundStateForTest() {
        return abandonStagedCompoundState();
    }

    bool journalMappedEntityForTest(
        dwgBuffer* buffer, std::uint32_t handle, DRW_Interface& intfa,
        DwgBlockScopeTransaction& transaction,
        DwgBlockJournalOutput& output, DwgFrameMapLease& lease) {
        const auto sourceIt = ObjectMap.find(handle);
        if (sourceIt == ObjectMap.end())
            return false;
        DwgFrameClassification classification;
        if (!classifyDwgSourceFrame(buffer, sourceIt->second, classification)
            || !detachDwgSourceFrame(ObjectMap, sourceIt, lease)) {
            return false;
        }
        lease.classification.emplace(std::move(classification));
        bool frameFailure = false;
        return readMappedDwgEntity(
            buffer, lease, intfa, &frameFailure,
            DwgIntegrityAddressSpace::DecodedBuffer, &output,
            DwgMappedEntityCompletion::Journal, &transaction) && !frameFailure;
    }

private:
    template <typename Fn>
    bool withActiveFrameForTest(std::uint32_t handle, Fn&& operation) {
        const auto sourceIt = ObjectMap.find(handle);
        if (sourceIt == ObjectMap.end() || m_activeEntityFrameLease != nullptr)
            return false;

        DwgFrameMapLease lease;
        if (!detachDwgSourceFrame(ObjectMap, sourceIt, lease))
            return false;
        m_activeEntityFrameLease = &lease;
        bool result = false;
        try {
            result = operation();
        } catch (...) {
            result = false;
        }
        m_activeEntityFrameLease = nullptr;
        if (lease.isDetached())
            (void)restoreDwgSourceFrame(lease);
        return result;
    }
};

class DwgTableStateReaderProbe final : public dwgReader15 {
public:
    using dwgReader15::readDwgClasses;
    using dwgReader15::readDwgHandles;
    using dwgReader15::readDwgHeader;
    using dwgReader15::readFileHeader;
    using dwgReader::readDwgTables;

    explicit DwgTableStateReaderProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader15(std::move(buffer), nullptr) {}

    void setVersionForTest(DRW::Version value) {
        version = value;
        decoder.setVersion(value, false);
    }

    void removeObjectForTest(std::uint32_t handle) {
        ObjectMap.erase(handle);
    }

    bool firstLinetypeRecordForTest(std::uint32_t& handle,
                                    std::uint64_t& offset) {
        for (const auto& entry : ObjectMap) {
            DwgObjectFrame frame;
            if (!frame.readAt(*fileBuf, version, entry.second.loc))
                continue;
            dwgBuffer buffer(frame.body().data(), frame.body().size(), &decoder);
            if (buffer.getObjType(version) != DRW::DwgLTypeControlObjectType)
                continue;
            buffer.resetPosition();
            DRW_ObjControl control;
            if (!control.parseDwg(version, &buffer, frame.bodyBitSize())
                || !buffer.isGood() || control.handlesList.empty())
                continue;

            const auto recordIt = ObjectMap.find(control.handlesList.front());
            if (recordIt == ObjectMap.end())
                continue;
            handle = recordIt->first;
            offset = recordIt->second.loc;
            return true;
        }
        return false;
    }

    bool replaceLinetypeControlTypeForTest(
        std::vector<std::uint8_t>& bytes, std::uint16_t replacementType) {
        for (const auto& entry : ObjectMap) {
            DwgObjectFrame frame;
            if (!frame.readAt(*fileBuf, version, entry.second.loc))
                continue;
            dwgBuffer typeBuffer(
                frame.body().data(), frame.body().size(), &decoder);
            if (typeBuffer.getObjType(version)
                    != DRW::DwgLTypeControlObjectType
                || !typeBuffer.isGood()) {
                continue;
            }

            dwgBuffer prefix(bytes.data(), bytes.size(), &decoder);
            if (!prefix.setPosition(entry.second.loc)
                || prefix.getModularShort() < 0 || !prefix.isGood()) {
                return false;
            }
            const std::size_t bodyOffset = prefix.getPosition();
            dwgBufferW encoded;
            encoded.putObjType(version, replacementType);
            const std::uint32_t bitCount = encoded.bitCount();
            if (!encoded.isGood()
                || bodyOffset > bytes.size()
                || (bitCount + 7u) / 8u > bytes.size() - bodyOffset) {
                return false;
            }
            for (std::uint32_t bit = 0; bit < bitCount; ++bit) {
                const std::uint8_t mask = static_cast<std::uint8_t>(
                    1u << (7u - bit % 8u));
                std::uint8_t& destination = bytes[bodyOffset + bit / 8u];
                destination = static_cast<std::uint8_t>(
                    destination & static_cast<std::uint8_t>(~mask));
                if ((encoded.data()[bit / 8u] & mask) != 0)
                    destination = static_cast<std::uint8_t>(destination | mask);
            }
            return true;
        }
        return false;
    }

    void seedTableStateForTest() {
        ltypemap.emplace(0x100u, new DRW_LType());
        layermap.emplace(0x101u, new DRW_Layer());
        m_ltypeNameOrder.emplace_back("stale linetype");
        m_layerNameOrder.emplace_back("stale layer");
        m_deferredRawObjects.emplace_back();
    }

    bool tableStateEmptyForTest() const {
        return ltypemap.empty() && layermap.empty() && stylemap.empty()
            && dimstylemap.empty() && vportmap.empty() && blockRecordmap.empty()
            && appIdmap.empty() && viewmap.empty() && ucsmap.empty()
            && m_ltypeNameOrder.empty() && m_layerNameOrder.empty()
            && m_deferredRawObjects.empty()
            && m_deferredTableFramePublications.empty();
    }
};

class DwgSequenceWriterProbe final : public DRW_SeqEnd {
public:
    using DRW_SeqEnd::encodeDwg;
    using DRW_SeqEnd::parseDwg;
};

class DwgAttribWriterProbe final : public DRW_Attrib {
public:
    using DRW_Attrib::encodeDwg;
};

class DwgAttdefWriterProbe final : public DRW_Attdef {
public:
    using DRW_Attdef::encodeDwg;
};

class DwgPolylineWriterProbe final : public DRW_Polyline {
public:
    using DRW_Polyline::encodeDwg;
    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;

    void setObjectType(std::int16_t value) { oType = value; }
};

class DwgInsertWriterProbe final : public DRW_Insert {
public:
    using DRW_Insert::encodeDwg;
    using DRW_Insert::encodeDwgCommon;
    using DRW_Insert::encodeDwgEntHandle;

    void setObjectType(std::int16_t value) { oType = value; }
};

class DwgTableReaderProbe final : public DRW_Table {
public:
    using DRW_Table::parseDwg;
};

class DwgShapeReaderProbe final : public DRW_Shape {
public:
    using DRW_Shape::parseDwg;
};

class DwgShapeWriterProbe final : public DRW_Shape {
public:
    using DRW_Shape::encodeDwg;
};

class DwgBlockReaderProbe final : public DRW_Block {
public:
    using DRW_Block::parseDwg;
};

class DwgOle2FrameReaderProbe final : public DRW_Ole2Frame {
public:
    using DRW_Ole2Frame::parseDwg;
};

class DwgOle2FrameWriterProbe final : public DRW_Ole2Frame {
public:
    using DRW_Ole2Frame::encodeDwg;
};

class DwgOleFrameReaderProbe final : public DRW_OleFrame {
public:
    using DRW_OleFrame::parseDwg;
};

class DwgOleFrameWriterProbe final : public DRW_OleFrame {
public:
    using DRW_OleFrame::encodeDwg;
};

class DwgTableEntryProbe final : public DRW_TableEntry {
public:
    bool parseCommon(DRW::Version version, dwgBuffer* buffer,
                     std::uint32_t bs = 0) {
        return DRW_TableEntry::parseDwg(version, buffer, nullptr, bs);
    }

protected:
    bool parseDwg(DRW::Version version, dwgBuffer* buffer,
                  std::uint32_t bs = 0) override {
        return parseCommon(version, buffer, bs);
    }
};

class DwgMLineWriterProbe final : public DRW_MLine {
public:
    using DRW_MLine::encodeDwg;
    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;

    void setObjectType(std::int16_t value) { oType = value; }
};

class DwgViewportReaderProbe final : public DRW_Viewport {
public:
    using DRW_Viewport::encodeDwg;
    using DRW_Viewport::parseDwg;
};

class DwgViewportWriterProbe final : public DRW_Viewport {
public:
    using DRW_Viewport::encodeDwg;
    using DRW_Entity::encodeDwgCommon;

    void setObjectType(std::int16_t value) { oType = value; }
};

class DwgMTextProbe final : public DRW_MText {
public:
    using DRW_MText::encodeDwg;
    using DRW_MText::parseDwg;
};

class DwgRTextProbe final : public DRW_RText {
public:
    using DRW_RText::encodeDwg;
    using DRW_RText::parseDwg;
};

class DwgTextProbe final : public DRW_Text {
public:
    using DRW_Text::encodeDwg;
    using DRW_Text::parseDwg;
};

class DwgAttribProbe final : public DRW_Attrib {
public:
    using DRW_Attrib::parseDwg;
};

class DwgAttdefProbe final : public DRW_Attdef {
public:
    using DRW_Attdef::parseDwg;
};

class DwgDimensionProbe final : public DRW_DimAligned {
public:
    using DRW_DimAligned::encodeDwg;
    using DRW_DimAligned::parseDwg;
};

class DwgRadialDimensionProbe final : public DRW_DimRadial {
public:
    using DRW_DimRadial::encodeDwg;
    using DRW_DimRadial::parseDwg;
};

class DwgLargeRadialDimensionProbe final : public DRW_DimLargeRadial {
public:
    using DRW_DimLargeRadial::encodeDwg;
    using DRW_DimLargeRadial::parseDwg;
};

class DwgDiametricDimensionProbe final : public DRW_DimDiametric {
public:
    using DRW_DimDiametric::encodeDwg;
    using DRW_DimDiametric::parseDwg;
};

class DwgAngularDimensionProbe final : public DRW_DimAngular {
public:
    using DRW_DimAngular::encodeDwg;
    using DRW_DimAngular::parseDwg;
};

class DwgAngular3pDimensionProbe final : public DRW_DimAngular3p {
public:
    using DRW_DimAngular3p::encodeDwg;
    using DRW_DimAngular3p::parseDwg;
};

class DwgOrdinateDimensionProbe final : public DRW_DimOrdinate {
public:
    using DRW_DimOrdinate::encodeDwg;
    using DRW_DimOrdinate::parseDwg;
};

class DwgArcDimensionProbe final : public DRW_DimArc {
public:
    using DRW_DimArc::encodeDwg;
    using DRW_DimArc::parseDwg;
};

class DwgMLeaderProbe final : public DRW_MLeader {
public:
    using DRW_MLeader::encodeDwg;
    using DRW_MLeader::encodeDwgCommon;
    using DRW_MLeader::parseDwg;

    void setObjectType(std::uint16_t value) { oType = value; }
};

class DwgPointCloudProbe final : public DRW_PointCloud {
public:
    using DRW_PointCloud::encodeDwg;
    using DRW_PointCloud::parseDwg;
};

class DwgPointCloudExProbe final : public DRW_PointCloudEx {
public:
    using DRW_PointCloudEx::encodeDwg;
    using DRW_PointCloudEx::parseDwg;
};

class DwgImageProbe final : public DRW_Image {
public:
    using DRW_Image::encodeDwg;
    using DRW_Image::parseDwg;
};

class DwgWipeoutProbe final : public DRW_Wipeout {
public:
    using DRW_Wipeout::parseDwg;
};

class DwgUnderlayProbe final : public DRW_Underlay {
public:
    using DRW_Underlay::encodeDwg;
    using DRW_Underlay::parseDwg;
};

class DwgNavisworksModelProbe final : public DRW_NavisworksModel {
public:
    using DRW_NavisworksModel::encodeDwg;
    using DRW_NavisworksModel::parseDwg;
};

class DwgLightProbe final : public DRW_Light {
public:
    using DRW_Light::encodeDwg;
    using DRW_Light::parseDwg;
};

class DwgCameraProbe final : public DRW_Camera {
public:
    using DRW_Camera::encodeDwg;
    using DRW_Camera::parseDwg;
};

class DwgSectionObjectProbe final : public DRW_SectionObject {
public:
    using DRW_SectionObject::encodeDwg;
    using DRW_SectionObject::parseDwg;
};

class DwgToleranceProbe final : public DRW_Tolerance {
public:
    using DRW_Tolerance::encodeDwg;
    using DRW_Tolerance::parseDwg;
};

class Dwg3DLineProbe final : public DRW_3DLine {
public:
    using DRW_3DLine::encodeDwg;
    using DRW_3DLine::parseDwg;
};

class DwgTraceWriterProbe final : public DRW_Trace {
public:
    using DRW_Trace::encodeDwg;
};

class Dwg3DFaceWriterProbe final : public DRW_3Dface {
public:
    using DRW_3Dface::encodeDwg;
};

class DwgArcAlignedTextProbe final : public DRW_ArcAlignedText {
public:
    using DRW_ArcAlignedText::encodeDwg;
    using DRW_ArcAlignedText::parseDwg;
};

class DwgSplineProbe final : public DRW_Spline {
public:
    using DRW_Spline::parseDwg;
    using DRW_Spline::encodeDwg;
    using DRW_Spline::encodeDwgCommon;

    void setObjectType(std::uint16_t value) { oType = value; }
};

class DwgHelixProbe final : public DRW_Helix {
public:
    using DRW_Helix::encodeDwg;
    using DRW_Helix::parseDwg;
};

class DwgPlaneSurfaceProbe final : public DRW_PlaneSurface {
public:
    using DRW_Surface::encodeDwg;
    using DRW_Surface::parseDwg;
};

class DwgModelerGeometryProbe final : public DRW_ModelerGeometry {
public:
    DwgModelerGeometryProbe()
        : DRW_ModelerGeometry(DRW::E3DSOLID) {}

    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;
    using DRW_ModelerGeometry::parseDwg;

    void setObjectType(std::uint16_t value) { oType = value; }
};

class DwgMeshProbe final : public DRW_Mesh {
public:
    using DRW_Mesh::encodeDwg;
    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;
    using DRW_Mesh::parseDwg;

    void setObjectType(std::uint16_t value) { oType = value; }
};

class DwgProxyObjectProbe final : public DRW_ProxyObject {
public:
    using DRW_ProxyObject::parseDwg;
};

class DwgLwPolylineProbe final : public DRW_LWPolyline {
public:
    using DRW_LWPolyline::encodeDwg;
    using DRW_LWPolyline::parseDwg;
    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;

    void setObjectType(std::uint16_t value) { oType = value; }
};

class DwgMLineProbe final : public DRW_MLine {
public:
    using DRW_MLine::parseDwg;
};

class DwgInsertProbe final : public DRW_Insert {
public:
    using DRW_Insert::parseDwg;
};

class DwgPolylineProbe final : public DRW_Polyline {
public:
    using DRW_Polyline::parseDwg;
};

class DwgVertexProbe final : public DRW_Vertex {
public:
    using DRW_Vertex::parseDwg;
};

class DwgLeaderProbe final : public DRW_Leader {
public:
    using DRW_Leader::encodeDwg;
    using DRW_Leader::parseDwg;
    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;

    void setObjectType(std::uint16_t value) { oType = value; }
};

class DwgHatchProbe final : public DRW_Hatch {
public:
    using DRW_Hatch::encodeDwgCommon;
    using DRW_Hatch::parseDwg;

    void setObjectType(std::uint16_t value) { oType = value; }
};

class DwgMPolygonProbe final : public DRW_MPolygon {
public:
    using DRW_MPolygon::encodeDwgCommon;
    using DRW_MPolygon::parseDwg;

    void setObjectType(std::uint16_t value) { oType = value; }
};

class DwgLineWriterProbe final : public DRW_Line {
public:
    using DRW_Line::encodeDwg;
    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;

    void setObjectType(std::int16_t value) { oType = value; }
};

class DwgProxyEntityWriterProbe final : public DRW_ProxyEntity {
public:
    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;

    void setObjectType(std::int16_t value) { oType = value; }
};

class DwgBlockWriterProbe final : public DRW_Block {
public:
    using DRW_Block::encodeDwg;

    void setLayerHandle(std::uint32_t value) { layerH.ref = value; }
};

class DwgGroupWriterProbe final : public DRW_Group {
public:
    using DRW_Group::encodeDwg;
};

class DwgUcsWriterProbe final : public DRW_UCS {
public:
    using DRW_UCS::encodeDwg;
};

class DwgLightListWriterProbe final : public DRW_LightList {
public:
    using DRW_LightList::encodeDwg;
};

class DwgGeoPositionMarkerWriterProbe final : public DRW_GeoPositionMarker {
public:
    using DRW_GeoPositionMarker::encodeDwg;
    using DRW_GeoPositionMarker::parseDwg;
};

class DwgLineReaderProbe final : public DRW_Line {
public:
    using DRW_Line::parseDwg;
};

class DwgPointReaderProbe final : public DRW_Point {
public:
    using DRW_Point::parseDwg;
};

class DwgCircleReaderProbe final : public DRW_Circle {
public:
    using DRW_Circle::parseDwg;
};

class DwgArcReaderProbe final : public DRW_Arc {
public:
    using DRW_Arc::parseDwg;
};

class DwgEllipseReaderProbe final : public DRW_Ellipse {
public:
    using DRW_Ellipse::parseDwg;
};

class DwgTraceReaderProbe final : public DRW_Trace {
public:
    using DRW_Trace::parseDwg;
};

class Dwg3DFaceReaderProbe final : public DRW_3Dface {
public:
    using DRW_3Dface::parseDwg;
};

class DwgVertexWriterProbe final : public DRW_Vertex {
public:
    using DRW_Vertex::encodeDwg;
    using DRW_Entity::encodeDwgCommon;

    void setObjectType(std::int16_t value) { oType = value; }

    void setEedReferences(std::uint32_t appId, std::uint32_t layer) {
        extData.emplace_back(std::make_shared<DRW_Variant>(
            1001, "WRITER_PLACEHOLDER_APP"));
        extData.emplace_back(std::make_shared<DRW_Variant>(
            1003, "WRITER_PLACEHOLDER_LAYER", true));
        dwgEedAppIdWriteRefs.push_back({0u, appId});
        dwgEedLayerWriteRefs.push_back({1u, layer});
    }
};

class DwgDictionaryReaderProbe final : public DRW_Dictionary {
public:
    using DRW_Dictionary::parseDwg;
};

class DwgDictionaryWithDefaultReaderProbe final
    : public DRW_DictionaryWithDefault {
public:
    using DRW_DictionaryWithDefault::parseDwg;
};

class DwgDictionaryVarReaderProbe final : public DRW_DictionaryVar {
public:
    using DRW_DictionaryVar::parseDwg;
};

class DwgDictionaryWithDefaultWriterProbe final
    : public DRW_DictionaryWithDefault {
public:
    using DRW_DictionaryWithDefault::encodeDwg;
};

class DwgFieldWriterProbe final : public DRW_Field {
public:
    using DRW_Field::encodeDwg;
};

class DwgObjectControlReaderProbe final : public DRW_ObjControl {
public:
    using DRW_ObjControl::parseDwg;
};

class DwgDataPageReaderProbe final : public dwgReader21 {
public:
    using dwgReader21::captureRawDwgDataSections;
    using dwgReader21::PageMapFailure;
    using dwgReader21::parseDataPage;
    using dwgReader21::parseSectionPageMap;

    explicit DwgDataPageReaderProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader21(std::move(buffer), nullptr) {}

    void setVersionForTest(DRW::Version value) { version = value; }

    std::size_t crcMismatchCount() const { return m_r2007CrcMismatch; }

    void setEmptyPrototypeSectionForTest() {
        auto& section = sections[secEnum::PROTOTYPE];
        section.Id = 1;
        section.size = 0;
    }

    void setMalformedObjectsSectionForTest() {
        auto& section = sections[secEnum::OBJECTS];
        section.Id = 1;
        section.size = 1;
        section.maxSize = 1;
        section.pages.emplace(1, dwgPageInfo(1, 0, 0));
    }

    std::size_t rawSectionCountForTest() const {
        return m_rawDwgSections.size();
    }

    std::size_t dataStorageSectionCountForTest() const {
        return m_dataStorageSections.size();
    }
};

class DwgDataPageReader18Probe final : public dwgReader18 {
public:
    using dwgReader18::captureRawDwgDataSections;
    using dwgReader18::parseDataPage;
    using dwgReader18::parseSectionPageMap;

    explicit DwgDataPageReader18Probe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader18(std::move(buffer), nullptr) {}

    void setVersionForTest(DRW::Version value) { version = value; }

    void setEmptyPrototypeSectionForTest() {
        auto& section = sections[secEnum::PROTOTYPE];
        section.Id = 1;
        section.size = 0;
    }

    void setDecodedStateForTest(const std::vector<std::uint8_t>& data) {
        objData = std::make_unique<std::uint8_t[]>(data.size());
        std::copy(data.begin(), data.end(), objData.get());
        uncompSize = data.size();
    }

    std::vector<std::uint8_t> decodedStateForTest() const {
        if (objData == nullptr)
            return {};
        return {objData.get(), objData.get() + uncompSize};
    }

    void setMalformedObjectsSectionForTest() {
        auto& section = sections[secEnum::OBJECTS];
        section.Id = 1;
        section.size = 1;
        section.maxSize = 1;
        section.pages.emplace(1, dwgPageInfo(1, 0, 0));
    }

    std::size_t sectionCountForTest() const { return sections.size(); }

    std::size_t rawSectionCountForTest() const {
        return m_rawDwgSections.size();
    }

    std::size_t dataStorageSectionCountForTest() const {
        return m_dataStorageSections.size();
    }
};

class DwgSectionLookupReader15Probe final : public dwgReader15 {
public:
    explicit DwgSectionLookupReader15Probe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader15(std::move(buffer), nullptr) {}

    std::size_t sectionCountForTest() const { return sections.size(); }

    void setHeaderSectionForTest(std::uint64_t size) {
        auto& section = sections[secEnum::HEADER];
        section.Id = 0;
        section.address = 0;
        section.size = size;
    }
};

class DwgDataStorageResetReader18Probe final : public dwgReader18 {
public:
    using dwgReader18::readFileHeader;

    explicit DwgDataStorageResetReader18Probe(
        std::unique_ptr<dwgBuffer> buffer)
        : dwgReader18(std::move(buffer), nullptr) {}

    void seedLinkedRecordForTest() {
        m_dataStorageLinkedRecords.emplace(3u, 7u);
    }

    std::size_t linkedRecordCountForTest() const {
        return m_dataStorageLinkedRecords.size();
    }

    void seedSectionForTest() {
        sections.emplace(0x7fff, dwgSectionInfo{});
    }

    std::size_t sectionCountForTest() const { return sections.size(); }
};

class DwgDataStorageResetReader21Probe final : public dwgReader21 {
public:
    using dwgReader21::readFileHeader;

    explicit DwgDataStorageResetReader21Probe(
        std::unique_ptr<dwgBuffer> buffer)
        : dwgReader21(std::move(buffer), nullptr) {}

    void seedLinkedRecordForTest() {
        m_dataStorageLinkedRecords.emplace(5u, 11u);
    }

    std::size_t linkedRecordCountForTest() const {
        return m_dataStorageLinkedRecords.size();
    }

    void seedSectionForTest() {
        sections.emplace(0x7fff, dwgSectionInfo{});
    }

    std::size_t sectionCountForTest() const { return sections.size(); }
};

class DwgClassesReader18Probe final : public dwgReader18 {
public:
    using dwgReader18::readDwgClasses;

    explicit DwgClassesReader18Probe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader18(std::move(buffer), nullptr) {}

    void setVersionForTest(DRW::Version value) { version = value; }

    void setDecodedStateForTest(const std::vector<std::uint8_t>& data) {
        objData = std::make_unique<std::uint8_t[]>(data.size());
        std::copy(data.begin(), data.end(), objData.get());
        uncompSize = data.size();
    }

    std::vector<std::uint8_t> decodedStateForTest() const {
        if (objData == nullptr)
            return {};
        return {objData.get(), objData.get() + uncompSize};
    }

    void setClassesPage(std::uint64_t logicalSize) {
        auto& section = sections[secEnum::CLASSES];
        section.Id = 1;
        section.size = logicalSize;
        section.maxSize = fileBuf->size() - 32;
        section.compressed = 1;
        section.pageCount = 1;
        section.pages.emplace(1, dwgPageInfo(1, 0, fileBuf->size()));
    }

    const DRW_DwgClassCoverageReport& classCoverageReportForTest() const {
        return m_dwgClassCoverageReport;
    }
};

class DwgClassesReader21Probe final : public dwgReader21 {
public:
    using dwgReader21::readDwgClasses;

    explicit DwgClassesReader21Probe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader21(std::move(buffer), nullptr) {}

    void setVersionForTest(DRW::Version value) { version = value; }

    void setClassesPage(std::uint64_t logicalSize) {
        auto& section = sections[secEnum::CLASSES];
        section.Id = 1;
        section.size = logicalSize;
        section.maxSize = fileBuf->size() - 32;
        section.compressed = 1;
        section.pageCount = 1;
        section.pages.emplace(1, dwgPageInfo(1, 0, fileBuf->size()));
    }

    std::size_t classCountForTest() const { return classesmap.size(); }

    const DRW_Class* classForTest(std::uint32_t classNumber) const {
        const auto it = classesmap.find(classNumber);
        return it == classesmap.end() ? nullptr : it->second;
    }

    const DRW_DwgClassCoverageReport& classCoverageReportForTest() const {
        return m_dwgClassCoverageReport;
    }
};

class DwgClassesReader15Probe final : public dwgReader15 {
public:
    using dwgReader15::readFileHeader;
    using dwgReader15::readDwgClasses;

    explicit DwgClassesReader15Probe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader15(std::move(buffer), nullptr) {}

    void setVersionForTest(DRW::Version value) { version = value; }

    void setClassesPage(std::uint64_t logicalSize) {
        auto& section = sections[secEnum::CLASSES];
        section.Id = 1;
        section.address = 0;
        section.size = logicalSize;
    }

    void seedSectionForTest() {
        sections.emplace(0x7fff, dwgSectionInfo{});
    }

    std::size_t sectionCountForTest() const { return sections.size(); }

    std::size_t classCountForTest() const { return classesmap.size(); }

    const DRW_DwgClassCoverageReport& classCoverageReportForTest() const {
        return m_dwgClassCoverageReport;
    }

    void finalizeClassCoverageForTest(
        DRW_Interface& interface, bool classReadCompleted) {
        finalizeDwgClassCoverageNoThrow(interface, classReadCompleted);
    }
};

class DwgReader18ObjectBoundsProbe final : public dwgReader18 {
public:
    explicit DwgReader18ObjectBoundsProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader18(std::move(buffer), nullptr) {}

    void setVersionForTest(DRW::Version value) { version = value; }

    void setObjectDataForTest(const std::vector<std::uint8_t>& data,
                              std::uint64_t logicalSize) {
        objData = std::make_unique<std::uint8_t[]>(data.size());
        std::copy(data.begin(), data.end(), objData.get());
        uncompSize = data.size();
        auto& section = sections[secEnum::OBJECTS];
        section.Id = 1;
        section.size = logicalSize;
    }
};

class DwgReader27ObjectBoundsProbe final : public dwgReader27 {
public:
    explicit DwgReader27ObjectBoundsProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader27(std::move(buffer), nullptr) {}

    void setVersionForTest(DRW::Version value) { version = value; }

    void setObjectDataForTest(const std::vector<std::uint8_t>& data,
                              std::uint64_t logicalSize) {
        objData = std::make_unique<std::uint8_t[]>(data.size());
        std::copy(data.begin(), data.end(), objData.get());
        uncompSize = data.size();
        auto& section = sections[secEnum::OBJECTS];
        section.Id = 1;
        section.size = logicalSize;
    }
};

class DwgSystemPageReaderProbe final : public dwgReader21 {
public:
    using dwgReader21::parseSysPage;

    explicit DwgSystemPageReaderProbe(std::unique_ptr<dwgBuffer> buffer)
        : dwgReader21(std::move(buffer), nullptr) {}

    std::size_t crcMismatchCount() const { return m_r2007CrcMismatch; }
};

class DwgClassPhaseWriterProbe final : public dwgWriter15 {
public:
    DwgClassPhaseWriterProbe(std::ofstream* stream, DRW_Header* header)
        : dwgWriter15(stream, header) {}

    using dwgWriter15::beginObject;
    using dwgWriter15::emitBlockRecord;
    using dwgWriter15::emitControlObject;
    using dwgWriter15::finishObject;
    using dwgWriter::registerRawObjectClass;

    void setVersionForTest(DRW::Version value) { m_version = value; }

    std::uint32_t lastObjectOffsetForTest() const {
        return m_objectMap.empty() ? 0 : m_objectMap.back().second;
    }

    std::size_t classDefinitionCountForTest() const {
        return m_dwgClassDefinitions.size();
    }

    std::uint32_t classInstanceEmissionCountForTest(
        std::uint16_t classNumber) const {
        const auto it = m_dwgClassInstanceEmissionCounts.find(classNumber);
        return it == m_dwgClassInstanceEmissionCounts.end() ? 0u : it->second;
    }

    std::size_t objectMapSizeForTest() const { return m_objectMap.size(); }
    bool frameWriteFailedForTest() const { return m_frameWriteError; }

    void poisonOutputForTest() { m_buf.putModularShort(-1); }
    std::size_t bufferSizeForTest() const { return m_buf.size(); }
    void addObjectMapEntryForTest(std::uint32_t handle,
                                  std::uint32_t offset) {
        m_objectMap.emplace_back(handle, offset);
    }

    const DwgClassDefinition* classDefinitionForTest(
        std::uint16_t classNumber) const {
        const auto it = std::find_if(
            m_dwgClassDefinitions.begin(), m_dwgClassDefinitions.end(),
            [classNumber](const DwgClassDefinition& definition) {
                return definition.m_classNum == classNumber;
            });
        return it == m_dwgClassDefinitions.end() ? nullptr : &*it;
    }
};

TEST_CASE("DWG public checkpoint admission cost probe",
          "[.dwg_checkpoint_benchmark][dwg][safety][benchmark]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1027);

    constexpr std::size_t recordCount = 128;
    for (std::size_t index = 0; index < recordCount; ++index) {
        const std::string suffix = std::to_string(index);

        DRW_LType ltype;
        ltype.name = "CKPT_LTYPE_" + suffix;
        REQUIRE(writer.addLType(ltype) != 0);

        DRW_Layer layer;
        layer.name = "CKPT_LAYER_" + suffix;
        REQUIRE(writer.addLayer(layer) != 0);

        DRW_Textstyle style;
        style.name = "CKPT_STYLE_" + suffix;
        REQUIRE(writer.addTextstyle(style) != 0);

        DRW_UCS ucs;
        ucs.name = "CKPT_UCS_" + suffix;
        REQUIRE(writer.addUcs(ucs));

        DRW_View view;
        view.name = "CKPT_VIEW_" + suffix;
        REQUIRE(writer.addView(view));

        DRW_Vport vport;
        vport.name = "CKPT_VPORT_" + suffix;
        REQUIRE(writer.addVport(vport));

        DRW_Dimstyle dimstyle;
        dimstyle.name = "CKPT_DIMSTYLE_" + suffix;
        REQUIRE(writer.addDimstyle(dimstyle) != 0);

        DRW_AppId appId;
        appId.name = "CKPT_APPID_" + suffix;
        REQUIRE(writer.addAppId(appId) != 0);
    }

    const auto compact = writer.checkpointCompoundWrite();
    const auto full = writer.checkpointPublicTransaction();
    CHECK_FALSE(compact.hasAdmissionState);
    CHECK(full.hasAdmissionState);
    CHECK(full.pendingLTypes.size() == recordCount);
    CHECK(full.pendingLayers.size() == recordCount);
    CHECK(full.pendingStyles.size() == recordCount);
    CHECK(full.pendingUcs.size() == recordCount);
    CHECK(full.pendingViews.size() == recordCount);
    CHECK(full.pendingVports.size() == recordCount);
    CHECK(full.pendingDimstyles.size() == recordCount);
    CHECK(full.pendingAppIds.size() == recordCount);

    const std::size_t pendingCapacity = full.pendingLTypes.capacity()
        + full.pendingLayers.capacity() + full.pendingStyles.capacity()
        + full.pendingUcs.capacity() + full.pendingViews.capacity()
        + full.pendingVports.capacity() + full.pendingDimstyles.capacity()
        + full.pendingAppIds.capacity();
    const std::size_t writingMapSize = full.writingContext.ltypeMap.size()
        + full.writingContext.layerMap.size()
        + full.writingContext.styleMap.size()
        + full.writingContext.ucsMap.size()
        + full.writingContext.viewMap.size()
        + full.writingContext.vportMap.size()
        + full.writingContext.dimstyleMap.size()
        + full.writingContext.appidMap.size();
    const std::size_t compactEntryCount = compact.classDefinitions.size()
        + compact.rawClassInstanceHandles.size()
        + compact.classInstanceEmissionCounts.size();

    const auto beforeRollback = writer.checkpointPublicTransaction();
    DRW_LType extra;
    extra.name = "CKPT_ROLLBACK_EXTRA";
    REQUIRE(writer.addLType(extra) != 0);
    writer.rollbackCompoundWrite(beforeRollback);
    const auto afterRollback = writer.checkpointPublicTransaction();
    CHECK(afterRollback.pendingLTypes.size()
          == beforeRollback.pendingLTypes.size());
    CHECK(afterRollback.writingContext.ltypeMap.size()
          == beforeRollback.writingContext.ltypeMap.size());

    constexpr std::size_t iterations = 8;
    std::size_t observedEntries = 0;
    const auto compactStart = std::chrono::steady_clock::now();
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        const auto snapshot = writer.checkpointCompoundWrite();
        observedEntries += snapshot.classDefinitions.size()
            + snapshot.rawClassInstanceHandles.size()
            + snapshot.classInstanceEmissionCounts.size();
    }
    const auto compactElapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - compactStart);

    const auto fullStart = std::chrono::steady_clock::now();
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        const auto snapshot = writer.checkpointPublicTransaction();
        observedEntries += snapshot.pendingLTypes.size()
            + snapshot.pendingLayers.size() + snapshot.pendingStyles.size()
            + snapshot.pendingUcs.size() + snapshot.pendingViews.size()
            + snapshot.pendingVports.size()
            + snapshot.pendingDimstyles.size()
            + snapshot.pendingAppIds.size();
    }
    const auto fullElapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - fullStart);
    WARN("DWG public checkpoint: records=" << recordCount
         << ", pending-capacity=" << pendingCapacity
         << " (ltype=" << full.pendingLTypes.capacity()
         << ", layer=" << full.pendingLayers.capacity()
         << ", style=" << full.pendingStyles.capacity()
         << ", ucs=" << full.pendingUcs.capacity()
         << ", view=" << full.pendingViews.capacity()
         << ", vport=" << full.pendingVports.capacity()
         << ", dimstyle=" << full.pendingDimstyles.capacity()
         << ", appid=" << full.pendingAppIds.capacity() << ")"
         << ", writing-map-size=" << writingMapSize
         << ", iterations=" << iterations
         << ", compact-us=" << compactElapsed.count()
         << ", full-us=" << fullElapsed.count());
    CHECK(observedEntries == iterations
          * (compactEntryCount + recordCount * 8));
}

class DwgClassPhaseWriter24Probe final : public dwgWriter24 {
public:
    DwgClassPhaseWriter24Probe(std::ofstream* stream, DRW_Header* header)
        : dwgWriter24(stream, header) {}

    using dwgWriter24::beginObject;
    using dwgWriter24::finishObject;
    using dwgWriter::registerRawObjectClass;

    std::uint32_t lastObjectOffsetForTest() const {
        return m_objectMap.empty() ? 0 : m_objectMap.back().second;
    }

    void addObjectMapEntryForTest(std::uint32_t handle,
                                  std::uint32_t offset) {
        m_sectionOffsets[recno::CLASSES] = 100;
        m_sectionSizes[recno::CLASSES] = 100;
        m_objectMap.emplace_back(handle, offset);
    }

    void addObjectMapEntryWithoutSectionsForTest(std::uint32_t handle,
                                                 std::uint32_t offset) {
        m_objectMap.emplace_back(handle, offset);
    }

    std::size_t bufferSizeForTest() const { return m_buf.size(); }
    std::size_t objectMapSizeForTest() const { return m_objectMap.size(); }
    bool objectWriteFailedForTest() const {
        return m_writeError || dwgWriter15::objectWriteFailed();
    }
};

class DwgStringFooterWriterProbe final : public dwgWriter24 {
public:
    using dwgWriter24::appendR2007StringStream;
};

class DwgReadProbe : public DRW_Interface {
public:
    std::size_t blockCount = 0;
    std::vector<DRW_Insert> inserts;
    std::vector<DRW_Leader> leaders;
    std::vector<DRW_LWPolyline> lwPolylines;
    std::vector<DRW_ModelerGeometry> modelerGeometry;
    std::vector<DRW_UnsupportedObject> unsupportedObjects;
    std::size_t polylineCount = 0;
    std::vector<std::uint32_t> polylineHandles;
    std::size_t mlineCount = 0;
    std::size_t mtextCount = 0;
    std::size_t viewportCount = 0;
    std::size_t geoPositionMarkerCount = 0;
    std::size_t dxfClassCount = 0;
    std::vector<DRW_DwgFramePublication> framePublications;
    std::vector<DRW_DwgClassCoverageReport> classCoverageReports;

    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}

    void addBlock(const DRW_Block&) override { ++blockCount; }
    void setBlock(int) override {}
    void endBlock() override {}

    void addPoint(const DRW_Point&) override {}
    void addLine(const DRW_Line&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addLWPolyline(const DRW_LWPolyline& polyline) override {
        lwPolylines.push_back(polyline);
    }
    void addPolyline(const DRW_Polyline& polyline) override {
        ++polylineCount;
        polylineHandles.push_back(polyline.handle);
    }
    void addMLine(const DRW_MLine*) override { ++mlineCount; }
    void addViewport(const DRW_Viewport&) override { ++viewportCount; }
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert& insert) override { inserts.push_back(insert); }
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addModelerGeometry(const DRW_ModelerGeometry& geometry) override {
        modelerGeometry.push_back(geometry);
    }
    void addMText(const DRW_MText&) override { ++mtextCount; }
    void addText(const DRW_Text&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addDimArc(const DRW_DimArc*) override {}
    void addLeader(const DRW_Leader* leader) override {
        if (leader != nullptr)
            leaders.push_back(*leader);
    }
    void addHatch(const DRW_Hatch*) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addGeoPositionMarker(const DRW_GeoPositionMarker&) override {
        ++geoPositionMarkerCount;
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& object) override {
        unsupportedObjects.push_back(object);
    }
    void addDxfClass(const DRW_Class&) override { ++dxfClassCount; }
    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        framePublications.push_back(publication);
    }
    void addDwgClassCoverageReport(
        const DRW_DwgClassCoverageReport& report) override {
        classCoverageReports.push_back(report);
    }

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

class DwgDictionaryWithDefaultReceiptProbe final : public DwgReadProbe {
public:
    std::vector<DRW_DictionaryWithDefault> dictionaries;
    std::vector<DRW_DwgTypedReference> typedReferences;

    void addDictionaryWithDefault(
        const DRW_DictionaryWithDefault& dictionary) override {
        dictionaries.push_back(dictionary);
    }

    void addDwgTypedReference(
        const DRW_DwgTypedReference& reference) override {
        typedReferences.push_back(reference);
    }
};

class DwgFieldListReceiptProbe final : public DwgReadProbe {
public:
    std::vector<DRW_FieldList> fieldLists;
    std::vector<DRW_DwgFieldListMembership> memberships;
    std::vector<std::string> callbackOrder;

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        if (publication.m_recordName == "FIELDLIST")
            callbackOrder.emplace_back("frame");
        DwgReadProbe::addDwgFramePublication(publication);
    }

    void addDwgFieldListMembership(
        const DRW_DwgFieldListMembership& membership) override {
        callbackOrder.emplace_back("membership");
        memberships.push_back(membership);
    }

    void addFieldList(const DRW_FieldList& fieldList) override {
        callbackOrder.emplace_back("list");
        fieldLists.push_back(fieldList);
    }

    void addUnsupportedObject(const DRW_UnsupportedObject& object) override {
        if (object.m_recordName == "FIELDLIST")
            callbackOrder.emplace_back("raw");
        DwgReadProbe::addUnsupportedObject(object);
    }
};

class DwgFieldReceiptProbe final : public DwgReadProbe {
public:
    std::vector<DRW_Field> fields;
    std::vector<DRW_DwgFieldPayloadReceipt> receipts;

    void addField(const DRW_Field& field) override {
        fields.push_back(field);
    }

    void addDwgFieldPayloadReceipt(
        const DRW_DwgFieldPayloadReceipt& receipt) override {
        receipts.push_back(receipt);
    }
};

class DwgPhysicalFieldFixtureProbe final : public DwgReadProbe {
public:
    std::vector<DRW_Field> fields;
    std::vector<DRW_FieldList> fieldLists;
    std::vector<DRW_DwgFieldPayloadReceipt> fieldReceipts;
    std::vector<DRW_DwgFieldListMembership> fieldListMemberships;

    void addField(const DRW_Field& field) override {
        fields.push_back(field);
    }

    void addFieldList(const DRW_FieldList& fieldList) override {
        fieldLists.push_back(fieldList);
    }

    void addDwgFieldPayloadReceipt(
        const DRW_DwgFieldPayloadReceipt& receipt) override {
        fieldReceipts.push_back(receipt);
    }

    void addDwgFieldListMembership(
        const DRW_DwgFieldListMembership& membership) override {
        fieldListMemberships.push_back(membership);
    }
};

class DwgGroupReceiptProbe final : public DwgReadProbe {
public:
    std::vector<DRW_Group> groups;
    std::vector<DRW_DwgGroupMembership> memberships;

    void addGroup(const DRW_Group& group) override {
        groups.push_back(group);
    }

    void addDwgGroupMembership(
        const DRW_DwgGroupMembership& membership) override {
        memberships.push_back(membership);
    }
};

class DwgJournalLineProbe final : public DwgReadProbe {
public:
    std::vector<DRW_Line> lines;

    void addLine(const DRW_Line& line) override {
        lines.push_back(line);
    }
};

class DwgThrowingFrameReceiptProbe final : public DwgReadProbe {
public:
    void addDwgFramePublication(
        const DRW_DwgFramePublication&) override {
        throw 1;
    }
};

class DwgNthThrowingFrameReceiptProbe final : public DwgReadProbe {
public:
    std::size_t throwOn = 0;
    std::size_t attempts = 0;
    std::vector<DRW_DwgFramePublication> publications;

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        ++attempts;
        if (attempts == throwOn)
            throw 1;
        publications.push_back(publication);
    }
};

class DwgInsertReceiptProbe final : public DwgReadProbe {
public:
    std::vector<DRW_DwgFramePublication> publications;

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        publications.push_back(publication);
    }
};

class DwgBlockInsertOrderProbe final : public DwgReadProbe {
public:
    std::vector<std::string> callbacks;
    std::vector<DRW_Block> blocks;
    std::vector<DRW_DwgFramePublication> publications;

    void addBlock(const DRW_Block& block) override {
        callbacks.emplace_back("block");
        blocks.push_back(block);
    }

    void endBlock() override { callbacks.emplace_back("endBlock"); }

    void addInsert(const DRW_Insert& insert) override {
        callbacks.emplace_back("insert");
        DwgReadProbe::addInsert(insert);
    }

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        publications.push_back(publication);
    }
};

class DwgBlockJournalProbe : public DwgReadProbe {
public:
    std::vector<std::string> callbacks;
    std::vector<DRW_Block> blocks;
    std::vector<DRW_Circle> circles;
    std::vector<DRW_Line> lines;
    std::vector<DRW_ProxyEntity> proxyEntities;
    std::vector<DRW_UnsupportedObject> rawObjects;
    std::vector<DRW_DwgFramePublication> publications;
    std::vector<DRW_DwgBlockReachability> reachabilities;

    void addBlock(const DRW_Block& block) override {
        callbacks.emplace_back("block");
        blocks.push_back(block);
    }

    void endBlock() override { callbacks.emplace_back("endBlock"); }

    void addLine(const DRW_Line& line) override {
        callbacks.emplace_back("line");
        lines.push_back(line);
    }

    void addCircle(const DRW_Circle& circle) override {
        callbacks.emplace_back("circle");
        circles.push_back(circle);
    }

    void addProxyEntity(const DRW_ProxyEntity& entity) override {
        callbacks.emplace_back("proxy");
        proxyEntities.push_back(entity);
    }

    void addUnsupportedObject(const DRW_UnsupportedObject& object) override {
        callbacks.emplace_back("raw");
        rawObjects.push_back(object);
    }

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        publications.push_back(publication);
    }

    void addDwgBlockReachability(
        const DRW_DwgBlockReachability& reachability) override {
        callbacks.emplace_back("reachability");
        reachabilities.push_back(reachability);
    }
};

class DwgThrowingBlockJournalProbe final : public DwgBlockJournalProbe {
public:
    bool throwProxyCarrier = false;
    bool throwRawCarrier = false;
    bool throwReachability = false;

    void addProxyEntity(const DRW_ProxyEntity& entity) override {
        DwgBlockJournalProbe::addProxyEntity(entity);
        if (throwProxyCarrier)
            throw 1;
    }

    void addUnsupportedObject(const DRW_UnsupportedObject& object) override {
        DwgBlockJournalProbe::addUnsupportedObject(object);
        if (throwRawCarrier)
            throw 1;
    }

    void addDwgBlockReachability(
        const DRW_DwgBlockReachability& reachability) override {
        if (throwReachability)
            throw 1;
        DwgBlockJournalProbe::addDwgBlockReachability(reachability);
    }
};

class DwgThrowingInsertReceiptProbe final : public DwgReadProbe {
public:
    std::vector<DRW_DwgFramePublication> publications;

    void addInsert(const DRW_Insert&) override {
        throw 1;
    }

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        publications.push_back(publication);
    }
};

class DwgThrowingPolylineProbe final : public DwgReadProbe {
public:
    void addPolyline(const DRW_Polyline&) override { throw 1; }
};

class DwgThrowingJournalPolylineProbe final : public DwgReadProbe {
public:
    bool throwPolyline = false;
    bool throwReceipt = false;

    void addPolyline(const DRW_Polyline& polyline) override {
        if (throwPolyline)
            throw 1;
        DwgReadProbe::addPolyline(polyline);
    }

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        if (throwReceipt)
            throw 1;
        DwgReadProbe::addDwgFramePublication(publication);
    }
};

class DwgThrowingCoverageProbe final : public DwgReadProbe {
public:
    void addDwgFrameCoverageReport(
        const DRW_DwgFrameCoverageReport&) override {
        throw 1;
    }
};

class DwgThrowingClassCoverageProbe final : public DwgReadProbe {
public:
    std::size_t attempts = 0;

    void addDwgClassCoverageReport(
        const DRW_DwgClassCoverageReport&) override {
        ++attempts;
        throw 1;
    }
};

class DwgThrowingLineProbe final : public DwgReadProbe {
public:
    void addLine(const DRW_Line&) override { throw 1; }
};

class DwgThrowingRawObjectProbe final : public DwgReadProbe {
public:
    void addUnsupportedObject(const DRW_UnsupportedObject&) override {
        throw 1;
    }
};

class DwgThrowingLayerProbe final : public DwgReadProbe {
public:
    std::vector<DRW_DwgFrameCoverageReport> coverageReports;
    std::vector<DRW_DwgFramePublication> publications;

    void addLayer(const DRW_Layer&) override { throw 1; }

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        publications.push_back(publication);
    }

    void addDwgFrameCoverageReport(
        const DRW_DwgFrameCoverageReport& report) override {
        coverageReports.push_back(report);
    }
};

class DwgTableReceiptProbe final : public DwgReadProbe {
public:
    std::vector<DRW_DwgFramePublication> publications;
    std::vector<DRW_DwgFrameCoverageReport> coverageReports;
    bool layerDelivered = false;
    bool layerReceiptAfterCallback = false;

    void addLayer(const DRW_Layer&) override { layerDelivered = true; }

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        if (publication.m_recordName == "layer")
            layerReceiptAfterCallback = layerDelivered;
        publications.push_back(publication);
    }

    void addDwgFrameCoverageReport(
        const DRW_DwgFrameCoverageReport& report) override {
        coverageReports.push_back(report);
    }
};

class DwgCompoundWriteProbe final : public DwgReadProbe {
public:
    dwgRW* writer = nullptr;
    std::vector<DRW_Polyline> polylines;
    std::vector<DRW_DwgFramePublication> publications;

    void writeEntities() override {
        if (writer == nullptr)
            return;
        DRW_Polyline polyline;
        polyline.flags = 1;
        polyline.addVertex(DRW_Vertex(0.0, 0.0, 0.0, 0.0));
        polyline.addVertex(DRW_Vertex(10.0, 0.0, 0.0, 0.0));
        REQUIRE(writer->writePolyline(&polyline));
    }

    void addPolyline(const DRW_Polyline& polyline) override {
        DwgReadProbe::addPolyline(polyline);
        polylines.push_back(polyline);
    }

    void addDwgFramePublication(
        const DRW_DwgFramePublication& publication) override {
        publications.push_back(publication);
    }
};

class DwgIncompatiblePolylineWriteProbe final : public DwgReadProbe {
public:
    dwgRW* writer = nullptr;
    bool writeAttempted = false;
    bool writeRejected = false;

    void writeEntities() override {
        if (writer == nullptr)
            return;
        DRW_Polyline polyline;
        auto vertex = std::make_shared<DRW_Vertex>(
            0.0, 0.0, 0.0, 0.0);
        vertex->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex3D);
        polyline.appendVertex(vertex);
        writeAttempted = true;
        writeRejected = !writer->writePolyline(&polyline);
    }
};

class DwgInvalidAttributeWriteProbe final : public DwgReadProbe {
public:
    enum class Kind {
        Attrib,
        Attdef
    };

    explicit DwgInvalidAttributeWriteProbe(Kind kind) : m_kind(kind) {}

    dwgRW* writer = nullptr;
    bool attempted = false;
    bool accepted = false;

    void writeEntities() override {
        if (writer == nullptr)
            return;
        attempted = true;
        if (m_kind == Kind::Attrib) {
            DRW_Attrib attrib;
            attrib.tag = "ORPHAN";
            accepted = writer->writeAttrib(&attrib);
            return;
        }
        DRW_Attdef attdef;
        attdef.tag = "OUT_OF_BLOCK";
        accepted = writer->writeAttdef(&attdef);
    }

private:
    Kind m_kind;
};

class DwgWideEedWriteProbe final : public DwgReadProbe {
public:
    enum class Mode {
        Unchanged,
        Edited
    };

    explicit DwgWideEedWriteProbe(Mode mode = Mode::Unchanged) : m_mode(mode) {}

    dwgRW* writer = nullptr;
    bool writeSucceeded = false;
    std::vector<DRW_Line> lines;

    void writeEntities() override {
        if (writer == nullptr)
            return;
        DRW_Line line;
        line.basePoint = DRW_Coord{0.0, 0.0, 0.0};
        line.secPoint = DRW_Coord{1.0, 1.0, 0.0};
        line.extData.emplace_back(std::make_shared<DRW_Variant>(1001, "ACAD"));
        auto layerRef = std::make_shared<DRW_Variant>(
            1003, "102030405060708", true);
        layerRef->setDwgRawLayerReference(0x0102030405060708ULL,
                                          DRW::AC1032);
        if (m_mode == Mode::Edited)
            layerRef->setLayerRefName("UNRESOLVED_LAYER");
        line.extData.emplace_back(std::move(layerRef));
        writeSucceeded = writer->writeLine(&line);
    }

    void addLine(const DRW_Line& line) override {
        lines.push_back(line);
    }

private:
    Mode m_mode;
};

class DwgInsertCompoundWriteProbe final : public DwgReadProbe {
public:
    dwgRW* writer = nullptr;
    bool writeMInsert = false;
    bool writeEmptyLegacySequence = false;
    bool writeResult = false;
    std::vector<DRW_Insert> inserts;

    void writeEntities() override {
        if (writer == nullptr)
            return;
        DRW_Insert insert;
        insert.colcount = writeMInsert ? 2 : 1;
        const std::uint32_t blockRecord =
            writer->defineBlock("ATTRIB-BLOCK", DRW_Coord{});
        REQUIRE(blockRecord != 0);
        insert.name = "ATTRIB-BLOCK";
        insert.blockRecH.ref = blockRecord;
        if (writeEmptyLegacySequence) {
            dwgHandle firstBoundary;
            dwgHandle lastBoundary;
            insert.attribHandles = {firstBoundary, lastBoundary};
            insert.seqendH.ref = writer->allocNextHandle();
            REQUIRE(insert.seqendH.ref != DRW::NoHandle);
        } else {
            auto attribute = std::make_shared<DRW_Attrib>();
            attribute->tag = "TAG";
            attribute->text = "VALUE";
            attribute->height = 1.0;
            insert.attlist.push_back(std::move(attribute));
        }
        writeResult = writer->writeInsert(&insert);
    }

    void addInsert(const DRW_Insert& insert) override { inserts.push_back(insert); }
};

TEST_CASE("DWG modern compound ownership lists publish parent before children",
          "[dwg][safety][compound][writer]") {
    const auto path = std::filesystem::temp_directory_path()
        / "libdxfrw_modern_compound_owner_first.dwg";
    std::filesystem::remove(path);

    DwgCompoundWriteProbe writeInterface;
    {
        dwgRW writer(path.string().c_str());
        writeInterface.writer = &writer;
        REQUIRE(writer.write(&writeInterface, DRW::AC1021, /*bin=*/false));
    }

    DwgCompoundWriteProbe readInterface;
    {
        dwgRW reader(path.string().c_str());
        REQUIRE(reader.read(&readInterface, /*ext=*/false));
        CHECK(reader.getError() == DRW::BAD_NONE);
        CHECK(reader.getEntityParseFailures() == 0);
    }
    REQUIRE(readInterface.polylines.size() == 1u);
    CHECK(readInterface.polylines.front().vertlist.size() == 2u);
    std::filesystem::remove(path);
}

TEST_CASE("DWG writer rejects incompatible POLYLINE VERTEX subtypes",
          "[dwg][safety][compound][writer]") {
    const auto path = std::filesystem::temp_directory_path()
        / "libdxfrw_incompatible_polyline_vertex.dwg";
    std::filesystem::remove(path);

    DwgIncompatiblePolylineWriteProbe interface;
    {
        dwgRW writer(path.string().c_str());
        interface.writer = &writer;
        REQUIRE(writer.write(&interface, DRW::AC1027, /*bin=*/false));
    }

    CHECK(interface.writeAttempted);
    CHECK(interface.writeRejected);
    std::filesystem::remove(path);
}

TEST_CASE("DWG writer rejects standalone attribute entities",
          "[dwg][safety][writer][ownership]") {
    for (const DwgInvalidAttributeWriteProbe::Kind kind : {
             DwgInvalidAttributeWriteProbe::Kind::Attrib,
             DwgInvalidAttributeWriteProbe::Kind::Attdef}) {
        const auto path = std::filesystem::temp_directory_path()
            / (kind == DwgInvalidAttributeWriteProbe::Kind::Attrib
                   ? "libdxfrw_orphan_attrib.dwg"
                   : "libdxfrw_out_of_block_attdef.dwg");
        std::filesystem::remove(path);

        DwgInvalidAttributeWriteProbe interface(kind);
        {
            dwgRW writer(path.string().c_str());
            interface.writer = &writer;
            CHECK_FALSE(writer.write(&interface, DRW::AC1027, /*bin=*/false));
        }

        CHECK(interface.attempted);
        CHECK_FALSE(interface.accepted);
        CHECK_FALSE(std::filesystem::exists(path));
        std::filesystem::remove(path);
    }
}

TEST_CASE("DWG writer preserves unchanged wide EED layer references",
          "[dwg][safety][writer][eed]") {
    const auto path = std::filesystem::temp_directory_path()
        / "libdxfrw_wide_eed_layer_reference.dwg";
    std::filesystem::remove(path);

    DwgWideEedWriteProbe writeInterface;
    {
        dwgRW writer(path.string().c_str());
        writeInterface.writer = &writer;
        REQUIRE(writer.write(&writeInterface, DRW::AC1032, /*bin=*/false));
    }
    REQUIRE(writeInterface.writeSucceeded);

    DwgWideEedWriteProbe readInterface;
    {
        dwgRW reader(path.string().c_str());
        REQUIRE(reader.read(&readInterface, /*ext=*/false));
        CHECK(reader.getError() == DRW::BAD_NONE);
    }
    REQUIRE(readInterface.lines.size() == 1u);
    REQUIRE(readInterface.lines.front().extData.size() == 2u);
    const auto& layerRef = readInterface.lines.front().extData[1];
    REQUIRE(layerRef != nullptr);
    CHECK(layerRef->code() == 1003);
    CHECK(std::string(layerRef->c_str()) == "102030405060708");
    CHECK(layerRef->canReplayDwgRawLayerReference(DRW::AC1032));
    CHECK(layerRef->rawDwgLayerReference() == 0x0102030405060708ULL);
    std::filesystem::remove(path);

    for (const auto mode : {DwgWideEedWriteProbe::Mode::Unchanged,
                            DwgWideEedWriteProbe::Mode::Edited}) {
        const auto rejectedPath = std::filesystem::temp_directory_path()
            / (mode == DwgWideEedWriteProbe::Mode::Unchanged
                   ? "libdxfrw_cross_version_wide_eed.dwg"
                   : "libdxfrw_edited_wide_eed.dwg");
        std::filesystem::remove(rejectedPath);
        DwgWideEedWriteProbe rejectedInterface(mode);
        {
        dwgRW writer(rejectedPath.string().c_str());
            rejectedInterface.writer = &writer;
            const DRW::Version target = mode == DwgWideEedWriteProbe::Mode::Unchanged
                ? DRW::AC1027 : DRW::AC1032;
            CHECK_FALSE(writer.write(&rejectedInterface, target, /*bin=*/false));
        }
        CHECK_FALSE(rejectedInterface.writeSucceeded);
        CHECK_FALSE(std::filesystem::exists(rejectedPath));
    }
}

TEST_CASE("DWG modern INSERT ownership lists publish parent before ATTRIB",
          "[dwg][safety][compound][writer]") {
    const auto path = std::filesystem::temp_directory_path()
        / "libdxfrw_modern_insert_owner_first.dwg";
    std::filesystem::remove(path);

    DwgInsertCompoundWriteProbe writeInterface;
    {
        dwgRW writer(path.string().c_str());
        writeInterface.writer = &writer;
        REQUIRE(writer.write(&writeInterface, DRW::AC1021, /*bin=*/false));
    }

    DwgInsertCompoundWriteProbe readInterface;
    {
        dwgRW reader(path.string().c_str());
        REQUIRE(reader.read(&readInterface, /*ext=*/false));
        CHECK(reader.getError() == DRW::BAD_NONE);
        CHECK(reader.getEntityParseFailures() == 0);
    }
    REQUIRE(readInterface.inserts.size() == 1u);
    REQUIRE(readInterface.inserts.front().attlist.size() == 1u);
    CHECK(readInterface.inserts.front().attlist.front()->tag == "TAG");
    CHECK(readInterface.inserts.front().attlist.front()->text == "VALUE");
    std::filesystem::remove(path);
}

TEST_CASE("DWG modern INSERT and MINSERT round trips preserve ownership",
          "[dwg][safety][compound][writer]") {
    for (const DRW::Version version : {DRW::AC1018, DRW::AC1021}) {
        for (const bool minsert : {false, true}) {
            CAPTURE(static_cast<int>(version), minsert);
            const auto path = std::filesystem::temp_directory_path()
                / (std::string("libdxfrw_modern_insert_")
                   + std::to_string(static_cast<int>(version))
                   + (minsert ? "_minsert.dwg" : "_insert.dwg"));
            std::filesystem::remove(path);

            DwgInsertCompoundWriteProbe writeInterface;
            writeInterface.writeMInsert = minsert;
            {
                dwgRW writer(path.string().c_str());
                writeInterface.writer = &writer;
                REQUIRE(writer.write(&writeInterface, version,
                                     /*bin=*/false));
            }
            CHECK(writeInterface.writeResult);

            DwgInsertCompoundWriteProbe readInterface;
            {
                dwgRW reader(path.string().c_str());
                REQUIRE(reader.read(&readInterface, /*ext=*/false));
                CHECK(reader.getError() == DRW::BAD_NONE);
                CHECK(reader.getEntityParseFailures() == 0);
            }
            REQUIRE(readInterface.inserts.size() == 1u);
            const DRW_Insert& insert = readInterface.inserts.front();
            CHECK(insert.isMInsert() == minsert);
            REQUIRE(insert.attlist.size() == 1u);
            REQUIRE(insert.seqendH.ref != DRW::NoHandle);
            CHECK(insert.attlist.front()->handle == insert.attribHandles.front().ref);
            if (minsert) {
                CHECK(insert.colcount == 2);
                CHECK(insert.rowcount == 1);
            }
            const auto seqEndPublication = std::find_if(
                readInterface.framePublications.cbegin(),
                readInterface.framePublications.cend(),
                [&insert](const DRW_DwgFramePublication& publication) {
                    return publication.m_isEntity
                        && publication.m_handle == insert.seqendH.ref
                        && publication.m_resolvedType == dwgType::SEQEND;
                });
            REQUIRE(seqEndPublication !=
                    readInterface.framePublications.cend());
            CHECK(seqEndPublication->m_parentHandle == insert.handle);
            std::filesystem::remove(path);
        }
    }
}

TEST_CASE("DWG AC1015 attributed INSERT and MINSERT round trip",
          "[dwg][safety][compound][writer]") {
    for (const bool minsert : {false, true}) {
        const auto path = std::filesystem::temp_directory_path()
            / (minsert ? "libdxfrw_legacy_minsert_owner_first.dwg"
                       : "libdxfrw_legacy_insert_owner_first.dwg");
        std::filesystem::remove(path);

        DwgInsertCompoundWriteProbe writeInterface;
        writeInterface.writeMInsert = minsert;
        {
            dwgRW writer(path.string().c_str());
            writeInterface.writer = &writer;
            REQUIRE(writer.write(&writeInterface, DRW::AC1015, /*bin=*/false));
        }

        DwgInsertCompoundWriteProbe readInterface;
        {
            dwgRW reader(path.string().c_str());
            REQUIRE(reader.read(&readInterface, /*ext=*/false));
            CHECK(reader.getError() == DRW::BAD_NONE);
            CHECK(reader.getEntityParseFailures() == 0);
        }
        REQUIRE(readInterface.inserts.size() == 1u);
        CHECK(readInterface.inserts.front().isMInsert() == minsert);
        REQUIRE(readInterface.inserts.front().attlist.size() == 1u);
        CHECK(readInterface.inserts.front().attlist.front()->tag == "TAG");
        CHECK(readInterface.inserts.front().attlist.front()->text == "VALUE");
        std::filesystem::remove(path);
    }
}

TEST_CASE("DWG writer preserves an explicit empty legacy INSERT sequence",
          "[dwg][safety][compound][writer]") {
    const auto path = std::filesystem::temp_directory_path()
        / "libdxfrw_legacy_empty_insert_write.dwg";
    std::filesystem::remove(path);

    DwgInsertCompoundWriteProbe writeInterface;
    writeInterface.writeEmptyLegacySequence = true;
    {
        dwgRW writer(path.string().c_str());
        writeInterface.writer = &writer;
        REQUIRE(writer.write(&writeInterface, DRW::AC1015, /*bin=*/false));
    }
    CHECK(writeInterface.writeResult);

    DwgInsertCompoundWriteProbe readInterface;
    {
        dwgRW reader(path.string().c_str());
        REQUIRE(reader.read(&readInterface, /*ext=*/false));
        CHECK(reader.getError() == DRW::BAD_NONE);
        CHECK(reader.getEntityParseFailures() == 0);
    }
    REQUIRE(readInterface.inserts.size() == 1u);
    REQUIRE(readInterface.inserts.front().attribHandles.size() == 2u);
    CHECK(readInterface.inserts.front().attribHandles[0].ref ==
          DRW::NoHandle);
    CHECK(readInterface.inserts.front().attribHandles[1].ref ==
          DRW::NoHandle);
    CHECK(readInterface.inserts.front().attlist.empty());
    CHECK(readInterface.inserts.front().seqendH.ref != DRW::NoHandle);
    std::filesystem::remove(path);
}

TEST_CASE("DWG writer rejects a legacy empty INSERT sequence in modern output",
          "[dwg][safety][compound][writer]") {
    const auto path = std::filesystem::temp_directory_path()
        / "libdxfrw_modern_empty_insert_write.dwg";
    std::filesystem::remove(path);

    DwgInsertCompoundWriteProbe writeInterface;
    writeInterface.writeEmptyLegacySequence = true;
    {
        dwgRW writer(path.string().c_str());
        writeInterface.writer = &writer;
        REQUIRE(writer.write(&writeInterface, DRW::AC1018, /*bin=*/false));
    }
    CHECK_FALSE(writeInterface.writeResult);
    std::filesystem::remove(path);
}

class DwgLongTextProbe final : public DwgReadProbe {
public:
    dwgRW* writer = nullptr;
    std::vector<std::string> texts;
    bool writeSucceeded = false;

    void writeEntities() override {
        if (writer == nullptr)
            return;
        DRW_Text text;
        text.basePoint = DRW_Coord{1.0, 2.0, 0.0};
        text.secPoint = text.basePoint;
        text.height = 1.0;
        text.text = std::string(2048, 'A');
        writeSucceeded = writer->writeText(&text);
    }

    void addText(const DRW_Text& text) override {
        texts.emplace_back(text.text);
    }
};

TEST_CASE("DWG writer rollback invalidates its last frame receipt",
          "[dwg][safety][writer][transaction]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1015);

    DRW_Line first;
    first.handle = 0x320;
    first.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    first.secPoint = DRW_Coord{10.0, 10.0, 0.0};
    REQUIRE(writer.encodeEntity(&first));

    DRW::DwgObjectFrameReceipt receipt;
    REQUIRE(writer.getLastDwgObjectFrame(receipt));
    CHECK(receipt.objectHandle == first.handle);

    const auto checkpoint = writer.checkpointCompoundWrite();
    writer.rollbackCompoundWrite(checkpoint);

    receipt = {};
    CHECK_FALSE(writer.getLastDwgObjectFrame(receipt));
    CHECK(receipt.objectHandle == 0);
}

TEST_CASE("DWG writer rollback restores class-instance accounting",
          "[dwg][safety][writer][transaction]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);

    const auto checkpoint = writer.checkpointCompoundWrite();
    REQUIRE(writer.noteDwgClassInstanceEmitted(500));
    CHECK(writer.classInstanceEmissionCountForTest(500) == 1u);

    writer.rollbackCompoundWrite(checkpoint);
    CHECK(writer.classInstanceEmissionCountForTest(500) == 0u);
}

TEST_CASE("DWG writer rejects classes discovered after CLASSES emission",
          "[dwg][safety][writer]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);

    DRW_UnsupportedObject first;
    first.m_objectType = 900;
    first.m_handle = 0x900;
    first.m_isCustomClass = true;
    first.m_className = "AcDbLateClass";
    first.m_recordName = "LATECLASS";
    REQUIRE(writer.registerRawObjectClass(first));

    writer.freezeDwgClasses();
    CHECK(writer.registerRawObjectClass(first));

    DRW_UnsupportedObject newInstance = first;
    newInstance.m_handle = 0x901;
    CHECK_FALSE(writer.registerRawObjectClass(newInstance));
    CHECK_FALSE(writer.registerRawObjectClass(newInstance));

    DRW_UnsupportedObject newClass = first;
    newClass.m_objectType = 901;
    newClass.m_className = "AcDbAnotherLateClass";
    newClass.m_recordName = "ANOTHERLATECLASS";
    CHECK_FALSE(writer.registerRawObjectClass(newClass));
}

TEST_CASE("DWG raw class replay keeps source CLASSES metadata",
          "[dwg][safety][writer]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);

    DRW_UnsupportedObject raw;
    raw.m_objectType = 901;
    raw.m_handle = 0x901;
    raw.m_isEntity = true;
    raw.m_isCustomClass = true;
    raw.m_recordName = "TEST_ENTITY";
    raw.m_className = "AcDbEntity";
    raw.m_hasClassDefinition = true;
    raw.m_classProxyFlag = 0x1234;
    raw.m_classAppName = "TEST_APP";
    raw.m_classWasProxy = true;
    raw.m_classEntityFlagRaw = 0x1F2;
    raw.m_classDwgVersion = 1027;
    raw.m_classMaintenanceVersion = 329;
    raw.m_classUnknown1 = 17;
    raw.m_classUnknown2 = 23;

    REQUIRE(writer.registerRawObjectClass(raw));
    const DwgClassDefinition* definition =
        writer.classDefinitionForTest(raw.m_objectType);
    REQUIRE(definition != nullptr);
    CHECK(definition->m_proxyFlag == raw.m_classProxyFlag);
    CHECK(definition->m_appName == raw.m_classAppName);
    CHECK(definition->m_wasProxy == raw.m_classWasProxy);
    CHECK(definition->m_entityFlagRaw == raw.m_classEntityFlagRaw);
    CHECK(definition->m_dwgVersion == raw.m_classDwgVersion);
    CHECK(definition->m_maintenanceVersion == raw.m_classMaintenanceVersion);
    CHECK(definition->m_unknown1 == raw.m_classUnknown1);
    CHECK(definition->m_unknown2 == raw.m_classUnknown2);
    CHECK(definition->m_instanceCount == 1);
}

TEST_CASE("DWG raw replay rolls back failed class and frame publication",
          "[dwg][safety][writer]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1021);

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1021;
    raw.m_objectType = 900;
    raw.m_handle = 0x902;
    raw.m_objectSize = 1;
    raw.m_isCustomClass = true;
    raw.m_className = "AcDbFailedRawReplay";
    raw.m_recordName = "FAILED_RAW_REPLAY";
    raw.m_rawBytes = {0};

    const std::size_t classCount = writer.classDefinitionCountForTest();
    CHECK_FALSE(writer.replayRawObject(raw));
    CHECK(writer.classDefinitionCountForTest() == classCount);
    CHECK_FALSE(writer.hasDwgClassDefinition(900));
    CHECK(writer.buffer().empty());
    CHECK(writer.lastObjectOffsetForTest() == 0);
}

void appendBigEndianShort(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    bytes.push_back(static_cast<std::uint8_t>(value));
}

void appendHandleMapPage(std::vector<std::uint8_t>& output,
                         const std::vector<std::uint8_t>& pageData,
                         bool corruptCrc = false) {
    const auto pageSize = static_cast<std::uint16_t>(pageData.size() + 2);
    std::vector<std::uint8_t> page;
    appendBigEndianShort(page, pageSize);
    page.insert(page.end(), pageData.begin(), pageData.end());
    dwgBuffer pageBuffer(page.data(), page.size());
    std::uint16_t crc = pageBuffer.crc8(0xC0C1, 0,
                                        static_cast<std::int32_t>(page.size()));
    if (corruptCrc)
        ++crc;
    output.insert(output.end(), page.begin(), page.end());
    appendBigEndianShort(output, crc);
}

std::vector<std::uint8_t> makeHandleMap(bool corruptTerminatorCrc) {
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0});
    appendHandleMapPage(bytes, {}, corruptTerminatorCrc);
    return bytes;
}

void appendLegacyClass(dwgBufferW& classData, std::uint16_t classNumber) {
    classData.putBitShort(classNumber);
    classData.putBitShort(0x401);
    classData.putVariableText(DRW::AC1015, "ACAD");
    classData.putVariableText(DRW::AC1015, "AcDbEntity");
    classData.putVariableText(DRW::AC1015, "TEST_ENTITY");
    classData.putBit(0);
    classData.putBitShort(0x1F3);
}

std::vector<std::uint8_t> makeLegacyClassesSectionWithNumbers(
    bool corruptEndSentinel, const std::vector<std::uint16_t>& classNumbers) {
    dwgBufferW classData;
    for (const std::uint16_t classNumber : classNumbers)
        appendLegacyClass(classData, classNumber);
    classData.alignToByte();

    dwgBufferW section;
    section.putBytes(dwgSentinels::CLASSES_BEGIN, 16);
    section.putRawLong32(static_cast<std::uint32_t>(classData.size()));
    section.putBytes(classData.data().data(), classData.size());
    dwgBuffer sectionReader(section.data().data(), section.size());
    section.putRawShort16(sectionReader.crc8(
        0xC0C1, 16, static_cast<std::int32_t>(section.size())));
    section.putBytes(dwgSentinels::CLASSES_END, 16);
    if (corruptEndSentinel)
        section.data().back() ^= 0x01;
    return section.data();
}

std::vector<std::uint8_t> makeLegacyClassesSection(
    bool corruptEndSentinel, bool includeClass = true) {
    return makeLegacyClassesSectionWithNumbers(
        corruptEndSentinel, includeClass
            ? std::vector<std::uint16_t>{500}
            : std::vector<std::uint16_t>{});
}

std::vector<std::uint8_t> makeAc18ClassesSection(bool includeClass = true) {
    dwgBufferW section;
    section.putBytes(dwgSentinels::CLASSES_BEGIN, 16);
    const auto sizeOffset = section.size();
    section.putRawLong32(0);
    section.putBitShort(includeClass ? 500 : 499);
    section.putRawChar8(0);
    section.putRawChar8(0);
    section.putBit(0);
    if (includeClass) {
        section.putBitShort(500);
        section.putBitShort(0x1234);
        section.putVariableText(DRW::AC1018, "TEST_APP");
        section.putVariableText(DRW::AC1018, "AcDbEntity");
        section.putVariableText(DRW::AC1018, "TEST_ENTITY");
        section.putBit(1);
        section.putBitShort(0x1F2);
        section.putBitLong(7);
        section.putBitLong(1018);
        section.putBitLong(329);
        section.putBitLong(17);
        section.putBitLong(23);
    }
    section.alignToByte();
    const auto payloadSize = static_cast<std::uint32_t>(
        section.size() - (sizeOffset + sizeof(std::uint32_t)));
    section.patchRawLong32(sizeOffset, payloadSize);
    const auto crc = section.crc16(0xC0C1, 16, section.size());
    section.putRawShort16(crc);
    section.putBytes(std::vector<std::uint8_t>(8, 0).data(), 8);
    section.putBytes(dwgSentinels::CLASSES_END, 16);
    return section.data();
}

std::vector<std::uint8_t> makeAc21ClassesSection(bool includeClass = true) {
    dwgBufferW section;
    section.putBytes(dwgSentinels::CLASSES_BEGIN, 16);
    const auto sizeOffset = section.size();
    section.putRawLong32(0);
    const auto bitSizeOffset = section.size();
    section.putRawLong32(0);

    dwgBufferW strings;
    const std::uint16_t maxClass = includeClass ? 500 : 499;
    section.putBitShort(maxClass);
    section.putRawChar8(0);
    section.putRawChar8(0);
    section.putBit(0);
    if (includeClass) {
        section.putBitShort(500);
        section.putBitShort(0x1234);
        strings.putVariableText(DRW::AC1021, "TEST_APP");
        strings.putVariableText(DRW::AC1021, "AcDbEntity");
        strings.putVariableText(DRW::AC1021, "TEST_ENTITY");
        section.putBit(1);
        section.putBitShort(0x1F2);
        section.putBitLong(7);
        section.putBitLong(1027);
        section.putBitLong(329);
        section.putBitLong(17);
        section.putBitLong(23);
    }

    section.alignToByte();
    strings.alignToByte();
    section.putBytes(strings.data().data(), strings.size());
    const auto stringBitSize = static_cast<std::uint64_t>(strings.size()) * 8;
    section.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    section.putBit(1);

    const auto endBit = static_cast<std::uint64_t>(section.bitCount());
    const auto dataStartBit = static_cast<std::uint64_t>(bitSizeOffset + 4) * 8;
    const auto bitSize = static_cast<std::uint32_t>(32 + endBit - dataStartBit);
    section.patchRawLong32(bitSizeOffset, bitSize);
    section.alignToByte();

    const auto payloadSize = static_cast<std::uint32_t>(
        section.size() - (sizeOffset + 4));
    section.patchRawLong32(sizeOffset, payloadSize);
    const auto crc = section.crc16(0xC0C1, 16, section.size());
    section.putRawShort16(crc);
    section.putBytes(std::vector<std::uint8_t>(8, 0).data(), 8);
    section.putBytes(dwgSentinels::CLASSES_END, 16);
    return section.data();
}

std::vector<std::uint8_t> makeEed(const std::vector<std::uint8_t>& items) {
    dwgBufferW writer;
    writer.putBitShort(static_cast<std::uint16_t>(items.size()));
    dwgHandle appHandle;
    appHandle.code = 0;
    appHandle.ref = 0x2A;
    writer.putHandle(appHandle);
    writer.putBytes(items.data(), items.size());
    writer.putBitShort(0);
    return writer.data();
}

std::vector<std::uint8_t> makeSeqEndFrame(std::uint32_t handle,
                                          std::uint32_t owner) {
    DwgSequenceWriterProbe sequenceEnd;
    sequenceEnd.handle = handle;
    sequenceEnd.parentHandle = owner;

    dwgBufferW body;
    if (!sequenceEnd.encodeDwg(DRW::AC1015, &body, 0, nullptr, nullptr))
        return {};

    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(body.data().size()));
    frame.putBytes(body.data().data(), body.data().size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

void finalizeEncodedEntityBody(dwgBufferW& body) {
    if (body.data().empty())
        return;

    const std::uint8_t bsCode =
        static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
    const std::size_t sizeBitOffset =
        bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
    body.patchRawLong32AtBit(sizeBitOffset, body.bitCount());
    body.alignToByte();
}

std::vector<std::uint8_t> makeEntityFrame(dwgBufferW& body) {
    finalizeEncodedEntityBody(body);
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(body.data().size()));
    frame.putBytes(body.data().data(), body.data().size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

std::vector<std::uint8_t> makeBlockFrame(std::uint32_t handle,
                                         std::uint32_t owner,
                                         bool isEnd,
                                         const UTF8STRING& name = "*TEST_BLOCK") {
    DwgBlockWriterProbe block;
    block.handle = handle;
    block.parentHandle = owner;
    block.setLayerHandle(0x12);
    block.color = DRW::ColorByLayer;
    block.name = isEnd ? UTF8STRING() : name;
    block.setIsEnd(isEnd);

    dwgBufferW body;
    if (!block.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    return makeEntityFrame(body);
}

std::vector<std::uint8_t> makeLineFrame(std::uint32_t handle,
                                        std::uint32_t owner) {
    DwgLineWriterProbe line;
    line.handle = handle;
    line.parentHandle = owner;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);

    dwgBufferW body;
    if (!line.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    return makeEntityFrame(body);
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeEncodedBlockBody(bool isEnd) {
    DwgBlockWriterProbe block;
    block.handle = 0x2F3u;
    block.parentHandle = 0x2F4u;
    block.setLayerHandle(0x12u);
    block.color = DRW::ColorByLayer;
    block.name = isEnd ? UTF8STRING() : UTF8STRING("*TEST_BLOCK");
    block.setIsEnd(isEnd);

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!block.encodeDwg(DRW::AC1032, &body, 0, &strings, &handles))
        return {};

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::vector<std::uint8_t> makeMalformedBlockBody() {
    DwgBlockWriterProbe block;
    block.handle = 0x240u;
    block.parentHandle = 0x241u;
    block.setLayerHandle(0x242u);
    block.name = "*TRUNCATED_BLOCK";

    dwgBufferW body;
    if (!block.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr)
        || body.data().size() < 2)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeR2007EntityFrame(
    dwgBufferW& body, dwgBufferW& handleStream) {
    if (body.data().empty())
        return {};
    const std::uint8_t bsCode =
        static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
    const std::size_t sizeBitOffset =
        bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
    body.alignToByte();
    body.patchRawLong32AtBit(
        sizeBitOffset, static_cast<std::uint32_t>(body.size() * 8));
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(
        body.data().size() + handleStream.data().size()));
    frame.putBytes(body.data().data(), body.data().size());
    frame.putBytes(handleStream.data().data(), handleStream.data().size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

std::vector<std::uint8_t> makeR2013EntityFrame(
    dwgBufferW& body, dwgBufferW& handleStream) {
    if (body.data().empty())
        return {};
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(
        body.data().size() + handleStream.data().size()));
    frame.putUModularChar(static_cast<std::uint64_t>(
        handleStream.data().size()) * 8u);
    frame.putBytes(body.data().data(), body.data().size());
    frame.putBytes(handleStream.data().data(), handleStream.data().size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

std::vector<std::uint8_t> makeModernEntityFrame(
    DRW::Version version, dwgBufferW& body, dwgBufferW& handleStream) {
    if (version == DRW::AC1018)
        return makeEntityFrame(body);
    if (version == DRW::AC1021)
        return makeR2007EntityFrame(body, handleStream);
    if (version == DRW::AC1024 || version == DRW::AC1027) {
        body.alignToByte();
        return makeR2013EntityFrame(body, handleStream);
    }
    return {};
}

std::vector<std::uint8_t> makeMalformedModernPolylineFrame(
    std::int32_t ownedObjectCount) {
    DwgPolylineWriterProbe polyline;
    polyline.handle = 0x220u;
    polyline.setObjectType(dwgType::POLYLINE_2D);

    dwgBufferW body;
    if (!polyline.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.putBitShort(0); // flags
    body.putBitShort(0); // curve type
    body.putBitDouble(0.0); // default start width
    body.putBitDouble(0.0); // default end width
    body.putThickness(0.0, true);
    body.putBitDouble(0.0); // elevation
    body.putExtrusion(DRW_Coord(0.0, 0.0, 1.0), true);
    body.putBitLong(ownedObjectCount);
    if (!polyline.encodeDwgEntHandle(DRW::AC1018, &body))
        return {};
    return makeEntityFrame(body);
}

std::vector<std::uint8_t> makeMalformedInsertBody(
    std::int32_t ownedAttributeCount) {
    DwgInsertWriterProbe insert;
    insert.handle = 0x250u;
    insert.setObjectType(dwgType::INSERT);

    dwgBufferW body;
    if (!insert.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.put3BitDouble(DRW_Coord()); // insertion point
    body.put2Bits(3); // unit scales
    body.putBitDouble(0.0); // rotation
    body.putExtrusion(DRW_Coord(0.0, 0.0, 1.0), false);
    body.putBit(1); // has attributes
    body.putBitLong(ownedAttributeCount);
    if (!insert.encodeDwgEntHandle(DRW::AC1018, &body))
        return {};
    dwgHandle blockRecord;
    blockRecord.code = 5;
    blockRecord.ref = 0;
    blockRecord.ref64 = 0;
    blockRecord.size = 0;
    body.putHandle(blockRecord); // block record
    finalizeEncodedEntityBody(body);
    return body.data();
}

std::vector<std::uint8_t> makeMalformedInsertFrame(
    std::int32_t ownedAttributeCount) {
    const auto body = makeMalformedInsertBody(ownedAttributeCount);
    if (body.empty())
        return {};
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(body.size()));
    frame.putBytes(body.data(), body.size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

std::vector<std::uint8_t> makeInsertWithChildrenFrame(
    std::uint32_t handle, std::uint32_t owner,
    std::uint32_t attribHandle, std::uint32_t seqendHandle) {
    DwgInsertWriterProbe insert;
    insert.handle = handle;
    insert.parentHandle = owner;
    insert.setObjectType(dwgType::INSERT);
    dwgHandle attrib;
    attrib.ref = attribHandle;
    insert.attribHandles.push_back(attrib);
    insert.seqendH.ref = seqendHandle;

    dwgBufferW body;
    if (!insert.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    return makeEntityFrame(body);
}

std::vector<std::uint8_t> makeInsertFrame(
    std::uint32_t handle, std::uint32_t owner) {
    DwgInsertWriterProbe insert;
    insert.handle = handle;
    insert.parentHandle = owner;
    insert.setObjectType(dwgType::INSERT);

    dwgBufferW body;
    if (!insert.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    return makeEntityFrame(body);
}

std::vector<std::uint8_t> makeMalformedTableBody(
    std::int32_t ownedAttributeCount) {
    DwgInsertWriterProbe tableHeader;
    tableHeader.handle = 0x260u;
    tableHeader.setObjectType(dwgType::INSERT);

    dwgBufferW body;
    if (!tableHeader.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.put3BitDouble(DRW_Coord()); // insertion point
    body.put2Bits(3); // unit scales
    body.putBitDouble(0.0); // rotation
    body.putExtrusion(DRW_Coord(0.0, 0.0, 1.0), false);
    body.putBit(1); // has attributes
    body.putBitLong(ownedAttributeCount);
    return body.data();
}

std::vector<std::uint8_t> makeMalformedMLineFrame(
    std::uint16_t vertexCount, std::uint16_t parameterCount) {
    DwgMLineWriterProbe mline;
    mline.handle = 0x230u;
    mline.numLines = 1;
    mline.setObjectType(47);

    dwgBufferW body;
    if (!mline.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.putBitDouble(1.0);
    body.putRawChar8(mline.justification);
    body.put3BitDouble(mline.basePoint);
    body.putExtrusion(mline.extPoint, false);
    body.putBitShort(static_cast<std::uint16_t>(mline.openClosed));
    body.putRawChar8(mline.numLines);
    body.putBitShort(vertexCount);
    if (vertexCount == 1 && parameterCount != 0) {
        body.put3BitDouble(DRW_Coord{});
        body.put3BitDouble(DRW_Coord{1.0, 0.0, 0.0});
        body.put3BitDouble(DRW_Coord{0.0, 1.0, 0.0});
        body.putBitShort(parameterCount);
        body.putBitShort(0);
    }
    if (!mline.encodeDwgEntHandle(DRW::AC1018, &body))
        return {};
    return makeEntityFrame(body);
}

std::vector<std::uint8_t> makeTruncatedMLineBody() {
    DwgMLineWriterProbe mline;
    mline.handle = 0x2F9u;
    mline.numLines = 1;
    mline.numVerts = 1;
    mline.styleHandle = 0x2FAu;
    DRW_MLineVertex vertex;
    vertex.position = DRW_Coord{1.0, 2.0, 0.0};
    vertex.vertexDir = DRW_Coord{1.0, 0.0, 0.0};
    vertex.miterDir = DRW_Coord{0.0, 1.0, 0.0};
    vertex.segParms.resize(1);
    vertex.areaFillParms.resize(1);
    mline.vertlist.push_back(std::move(vertex));

    dwgBufferW body;
    if (!mline.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedUnderlayBody() {
    DwgUnderlayProbe underlay;
    underlay.handle = 0x2FCu;
    underlay.kind = DRW_Underlay::DGN;
    underlay.clipBoundary.emplace_back(1.0, 2.0, 0.0);
    underlay.definitionHandle = 0x2FDu;

    dwgBufferW body;
    if (!underlay.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedImageBody() {
    DwgImageProbe image;
    image.handle = 0x2FFu;
    image.ref = 0x300u;
    image.m_imageDefReactorHandle = 0x301u;

    dwgBufferW body;
    if (!image.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeMalformedViewportFrame(
    std::int32_t frozenLayerCount) {
    DwgViewportWriterProbe viewport;
    viewport.handle = 0x240u;
    viewport.setObjectType(dwgType::VIEWPORT);

    dwgBufferW body;
    if (!viewport.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    for (int i = 0; i < 17; ++i)
        body.putBitDouble(0.0);
    for (int i = 0; i < 8; ++i)
        body.putRawDouble(0.0);
    body.putBitShort(0); // circle zoom
    body.putBitLong(frozenLayerCount);
    return makeEntityFrame(body);
}

std::vector<std::uint8_t> makeTruncatedViewportBody() {
    DwgViewportWriterProbe viewport;
    viewport.handle = 0x306u;
    viewport.setObjectType(dwgType::VIEWPORT);
    viewport.styleSheet = "STYLE";
    viewport.vpHeaderHandle = 0x307u;
    viewport.clipBoundaryHandle = 0x308u;
    viewport.namedUcsHandle = 0x309u;
    viewport.baseUcsHandle = 0x30Au;

    dwgBufferW body;
    if (!viewport.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedAlignedDimensionBody() {
    DwgDimensionProbe dimension;
    dimension.handle = 0x30Du;
    dimension.setDimPoint(DRW_Coord{3.0, 4.0, 0.0});
    dimension.setDef1Point(DRW_Coord{0.0, 0.0, 0.0});
    dimension.setDef2Point(DRW_Coord{1.0, 1.0, 0.0});
    dimension.setText("<>");

    dwgBufferW body;
    if (!dimension.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedRadialDimensionBody() {
    DwgRadialDimensionProbe dimension;
    dimension.handle = 0x30Eu;
    dimension.setCenterPoint(DRW_Coord{0.0, 0.0, 0.0});
    dimension.setDiameterPoint(DRW_Coord{1.0, 0.0, 0.0});
    dimension.setLeaderLength(1.0);

    dwgBufferW body;
    if (!dimension.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedLargeRadialDimensionBody() {
    DwgLargeRadialDimensionProbe dimension;
    dimension.handle = 0x30Fu;
    dimension.setCenterPoint(DRW_Coord{0.0, 0.0, 0.0});
    dimension.setChordPoint(DRW_Coord{1.0, 0.0, 0.0});
    dimension.jogPoint = DRW_Coord{2.0, 0.0, 0.0};
    dimension.overrideCenterPoint = DRW_Coord{0.0, 1.0, 0.0};

    dwgBufferW body;
    if (!dimension.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedDiametricDimensionBody() {
    DwgDiametricDimensionProbe dimension;
    dimension.handle = 0x310u;
    dimension.setDiameter1Point(DRW_Coord{0.0, 0.0, 0.0});
    dimension.setDiameter2Point(DRW_Coord{1.0, 0.0, 0.0});
    dimension.setLeaderLength(1.0);

    dwgBufferW body;
    if (!dimension.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedAngularDimensionBody() {
    DwgAngularDimensionProbe dimension;
    dimension.handle = 0x311u;
    dimension.setDimPoint(DRW_Coord{0.0, 1.0, 0.0});
    dimension.setFirstLine1(DRW_Coord{0.0, 0.0, 0.0});
    dimension.setFirstLine2(DRW_Coord{1.0, 0.0, 0.0});
    dimension.setSecondLine1(DRW_Coord{1.0, 1.0, 0.0});
    dimension.setSecondLine2(DRW_Coord{0.0, 1.0, 0.0});

    dwgBufferW body;
    if (!dimension.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedAngular3pDimensionBody() {
    DwgAngular3pDimensionProbe dimension;
    dimension.handle = 0x312u;
    dimension.setDimPoint(DRW_Coord{0.0, 1.0, 0.0});
    dimension.setFirstLine(DRW_Coord{0.0, 0.0, 0.0});
    dimension.setSecondLine(DRW_Coord{1.0, 0.0, 0.0});
    dimension.SetVertexPoint(DRW_Coord{0.0, 1.0, 0.0});

    dwgBufferW body;
    if (!dimension.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedOrdinateDimensionBody() {
    DwgOrdinateDimensionProbe dimension;
    dimension.handle = 0x313u;
    dimension.setOriginPoint(DRW_Coord{0.0, 0.0, 0.0});
    dimension.setFirstLine(DRW_Coord{1.0, 0.0, 0.0});
    dimension.setSecondLine(DRW_Coord{1.0, 1.0, 0.0});

    dwgBufferW body;
    if (!dimension.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedArcDimensionBody() {
    DwgArcDimensionProbe dimension;
    dimension.handle = 0x314u;
    dimension.setArcDefPoint(DRW_Coord{0.0, 1.0, 0.0});
    dimension.setExtLine1(DRW_Coord{0.0, 0.0, 0.0});
    dimension.setExtLine2(DRW_Coord{1.0, 0.0, 0.0});
    dimension.setArcCenter(DRW_Coord{0.0, 0.0, 0.0});
    dimension.setLeaderPt1(DRW_Coord{1.0, 1.0, 0.0});
    dimension.leaderPt2 = DRW_Coord{1.0, 1.0, 0.0};

    dwgBufferW body;
    if (!dimension.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedNavisworksModelBody() {
    DwgNavisworksModelProbe model;
    model.handle = 0x315u;
    model.definitionHandle = 0x316u;
    model.unitFactor = 1.0;

    dwgBufferW body;
    if (!model.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeMalformedSplineFrame(
    std::int32_t knotCount, std::int32_t controlCount) {
    DwgSplineProbe spline;
    spline.handle = 0x250u;
    spline.setObjectType(dwgType::SPLINE);

    dwgBufferW body;
    if (!spline.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.putBitLong(1); // control-point scenario
    body.putBitLong(1); // degree
    body.putBit(0); // rational
    body.putBit(0); // closed
    body.putBit(0); // periodic
    body.putBitDouble(0.0); // knot tolerance
    body.putBitDouble(0.0); // control-point tolerance
    body.putBitLong(knotCount);
    if (knotCount == 1)
        body.putBitDouble(0.0);
    body.putBitLong(controlCount);
    body.putBit(0); // no weights
    finalizeEncodedEntityBody(body);
    return body.data();
}

std::vector<std::uint8_t> makeMalformedFitSplineBody(
    std::int32_t fitCount) {
    DwgSplineProbe spline;
    spline.handle = 0x251u;
    spline.setObjectType(dwgType::SPLINE);

    dwgBufferW body;
    if (!spline.encodeDwgCommon(DRW::AC1027, &body))
        return {};
    body.putBitLong(2); // fit-point scenario
    body.putBitLong(9); // method-fit-points and explicit knot parameter
    body.putBitLong(0); // chord knot parameter
    body.putBitLong(3); // degree
    body.putBitDouble(0.0); // fit tolerance
    body.put3BitDouble(DRW_Coord()); // beginning tangent
    body.put3BitDouble(DRW_Coord()); // ending tangent
    body.putBitLong(fitCount);
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedSplineBody() {
    DwgSplineProbe spline;
    spline.handle = 0x304u;
    spline.degree = 1;
    spline.m_scenario = 1;
    spline.knotslist = {0.0, 0.0, 1.0, 1.0};
    spline.controllist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    spline.controllist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));

    dwgBufferW body;
    if (!spline.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedSplinePayloadBody() {
    DwgSplineProbe spline;
    spline.handle = 0x306u;
    spline.setObjectType(dwgType::SPLINE);

    dwgBufferW body;
    if (!spline.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.putBitLong(1); // control-point scenario
    body.putBitLong(1); // degree
    body.putBit(0); // rational
    body.putBit(0); // closed
    body.putBit(0); // periodic
    body.putBitDouble(0.0); // knot tolerance
    body.putBitDouble(0.0); // control-point tolerance
    body.putBitLong(4); // knots
    body.putBitLong(2); // control points
    body.putBit(0); // no weights
    for (int i = 0; i < 4; ++i)
        body.putBitDouble(0.0);
    body.put3BitDouble(DRW_Coord()); // only one of two control points
    finalizeEncodedEntityBody(body);
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedHelixBody() {
    DwgHelixProbe helix;
    helix.handle = 0x305u;
    helix.degree = 1;
    helix.m_scenario = 1;
    helix.knotslist = {0.0, 0.0, 1.0, 1.0};
    helix.controllist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    helix.controllist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));
    helix.radius = 2.0;
    helix.turns = 3.0;
    helix.turnHeight = 4.0;

    dwgBufferW body;
    if (!helix.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeMalformedHatchCountBody(int field) {
    DwgHatchProbe hatch;
    hatch.handle = 0x252u;
    hatch.setObjectType(dwgType::HATCH);

    dwgBufferW body;
    if (!hatch.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.putBitLong(0); // gradient enabled
    body.putBitLong(0); // gradient reserved
    body.putBitDouble(0.0); // gradient angle
    body.putBitDouble(0.0); // gradient shift
    body.putBitLong(0); // single-color gradient
    body.putBitDouble(0.0); // gradient tint
    body.putBitLong(field == 0 ? 10000 : 0); // gradient colors
    if (field == 0) {
        finalizeEncodedEntityBody(body);
        return body.data();
    }
    body.putVariableText(DRW::AC1018, ""); // gradient name
    body.putBitDouble(0.0); // elevation
    body.put3BitDouble(DRW_Coord{0.0, 0.0, 1.0}); // extrusion
    body.putVariableText(DRW::AC1018, "PATTERN"); // pattern name
    body.putBit(field == 1 ? 0 : 1); // solid
    body.putBit(0); // not associative
    body.putBitLong(0); // no boundary paths
    body.putBitShort(0); // hatch style
    body.putBitShort(0); // pattern type
    if (field == 1) {
        body.putBitDouble(0.0); // angle
        body.putBitDouble(1.0); // scale
        body.putBit(0); // not double-hatched
        body.putBitShort(10000); // pattern definition lines
    } else {
        body.putBitLong(10000); // seed points
    }
    finalizeEncodedEntityBody(body);
    return body.data();
}

std::vector<std::uint8_t> makeMalformedMPolygonGradientBody() {
    DwgMPolygonProbe polygon;
    polygon.handle = 0x253u;
    polygon.setObjectType(DRW::MPOLYGON);

    dwgBufferW body;
    if (!polygon.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.putBitShort(0); // leading style
    body.putBitLong(0); // gradient enabled
    body.putBitLong(0); // gradient reserved
    body.putBitDouble(0.0); // gradient angle
    body.putBitDouble(0.0); // gradient shift
    body.putBitLong(0); // single-color gradient
    body.putBitDouble(0.0); // gradient tint
    body.putBitLong(10000); // gradient colors
    finalizeEncodedEntityBody(body);
    return body.data();
}

std::vector<std::uint8_t> makeMalformedLwPolylineBody(
    DRW::Version version, int field) {
    DwgLwPolylineProbe polyline;
    polyline.handle = 0x254u;
    polyline.setObjectType(dwgType::LWPOLYLINE);

    dwgBufferW body;
    if (!polyline.encodeDwgCommon(version, &body))
        return {};
    const std::int32_t flags = field == 1 ? 16
        : field == 2 ? 1024 : field == 3 ? 32 : 0;
    body.putBitShort(flags);
    body.putBitLong(field == 0 ? 1000000 : 1); // vertices
    if (field != 0)
        body.putBitLong(1000000); // bulges, vertex IDs, or widths
    body.putRawDouble(0.0); // first vertex X
    body.putRawDouble(0.0); // first vertex Y
    finalizeEncodedEntityBody(body);
    return body.data();
}

std::vector<std::uint8_t> makeMalformedLeaderBody(DRW::Version version) {
    DwgLeaderProbe leader;
    leader.handle = 0x255u;
    leader.setObjectType(dwgType::LEADER);

    dwgBufferW body;
    if (!leader.encodeDwgCommon(version, &body))
        return {};
    body.putBit(0); // unknown bit
    body.putBitShort(0); // annotation type
    body.putBitShort(0); // path type
    body.putBitLong(5000); // points; body cannot hold this many 3BD values
    finalizeEncodedEntityBody(body);
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedLeaderBody() {
    DwgLeaderProbe leader;
    leader.handle = 0x2F3u;
    leader.vertexlist.push_back(std::make_shared<DRW_Coord>(1.0, 2.0, 0.0));

    dwgBufferW body;
    if (!leader.encodeDwg(DRW::AC1015, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeZeroVertexLwPolylineStreamBody() {
    DwgLwPolylineProbe polyline;
    polyline.handle = 0x256u;
    polyline.setObjectType(dwgType::LWPOLYLINE);

    dwgBufferW body;
    if (!polyline.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.putBitShort(16); // bulge stream present
    body.putBitLong(0); // no vertices
    body.putBitLong(1); // one independent bulge value
    body.putBitDouble(0.25);
    if (!polyline.encodeDwgEntHandle(DRW::AC1018, &body))
        return {};
    finalizeEncodedEntityBody(body);
    return body.data();
}

std::vector<std::uint8_t> makeEncodedMTextFrame(
    DRW::Version version, std::int32_t columnCount,
    bool malformedString = false) {
    DwgMTextProbe mtext;
    mtext.handle = 0x270u;
    mtext.text.clear();
    if (version >= DRW::AC1032) {
        mtext.m_r2018IsNotAnnotative = true;
        mtext.m_r2018ColumnType = 2;
        mtext.m_r2018ColumnCount = columnCount;
        mtext.m_r2018ColumnAutoHeight = true;
    }

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!mtext.encodeDwg(version, &body, 0, &strings, &handles))
        return {};
    if (malformedString) {
        strings.reset();
        strings.putBitShort(std::numeric_limits<std::uint16_t>::max());
    }

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    handles.alignToByte();
    const std::size_t dataBytes = body.size();
    if (!handles.data().empty())
        body.putBytes(handles.data().data(), handles.data().size());
    const auto handleBitSize = static_cast<std::uint32_t>(
        (body.size() - dataBytes) * 8u);

    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(body.size()));
    frame.putUModularChar(handleBitSize);
    frame.putBytes(body.data().data(), body.data().size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

std::vector<std::uint8_t> makeMalformedMTextFrame(
    std::int32_t columnCount, bool malformedString = false) {
    return makeEncodedMTextFrame(DRW::AC1032, columnCount, malformedString);
}

std::vector<std::uint8_t> makeMalformedMLeaderFrame(
    std::uint16_t classVersion) {
    DwgMLeaderProbe mleader;
    mleader.handle = 0x280u;
    mleader.classVersion = classVersion;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!mleader.encodeDwg(DRW::AC1027, &body, 0, &strings, &handles))
        return {};

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    handles.alignToByte();
    const std::size_t dataBytes = body.size();
    if (!handles.data().empty())
        body.putBytes(handles.data().data(), handles.data().size());
    const auto handleBitSize = static_cast<std::uint32_t>(
        (body.size() - dataBytes) * 8u);

    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(body.size()));
    frame.putUModularChar(handleBitSize);
    frame.putBytes(body.data().data(), body.data().size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

std::vector<std::uint8_t> makeTruncatedMLeaderBody() {
    DwgMLeaderProbe mleader;
    mleader.handle = 0x2F6u;

    dwgBufferW body;
    if (!mleader.encodeDwg(DRW::AC1027, &body, 0, nullptr, nullptr))
        return {};
    finalizeEncodedEntityBody(body);
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedPointCloudBody() {
    DwgPointCloudProbe pointCloud;
    pointCloud.handle = 0x2F7u;
    pointCloud.definitionHandle = 0x2F8u;
    pointCloud.reactorHandle = 0x2F9u;
    pointCloud.showIntensity = true;
    pointCloud.showClipping = false;

    dwgBufferW body;
    dwgBufferW strings;
    if (!pointCloud.encodeDwg(DRW::AC1027, &body, 0, &strings, nullptr))
        return {};

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedPointCloudExBody() {
    DwgPointCloudExProbe pointCloud;
    pointCloud.handle = 0x2FAu;
    pointCloud.definitionHandle = 0x2FBu;
    pointCloud.reactorHandle = 0x2FCu;
    pointCloud.showIntensity = true;
    pointCloud.showCropping = false;

    dwgBufferW body;
    dwgBufferW strings;
    if (!pointCloud.encodeDwg(DRW::AC1027, &body, 0, &strings, nullptr))
        return {};

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return body.data();
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedPlaneSurfaceBody() {
    DwgPlaneSurfaceProbe surface;
    surface.setDwgClassNum(DRW_PlaneSurface::kDwgClassNum);
    surface.handle = 0x2FDu;

    dwgBufferW body;
    dwgBufferW handles;
    if (!surface.encodeDwg(DRW::AC1027, &body, 0, nullptr, &handles))
        return {};
    body.alignToByte();
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits < 8 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return {body.data(), handleBits - 8};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedMeshBody() {
    DwgMeshProbe mesh;
    mesh.handle = 0x300u;
    mesh.vertices = {DRW_Coord{0.0, 0.0, 0.0},
                     DRW_Coord{1.0, 0.0, 0.0},
                     DRW_Coord{0.0, 1.0, 0.0}};
    mesh.faces = {{0, 1, 2}};
    mesh.edges = {{0, 1}, {1, 2}, {2, 0}};
    mesh.creases = {0.0, 0.0, 0.0};

    dwgBufferW body;
    dwgBufferW handles;
    if (!mesh.encodeDwg(DRW::AC1027, &body, 0, nullptr, &handles))
        return {};
    body.alignToByte();
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits < 8 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    if (body.data().size() <= 1)
        return {};
    body.data().pop_back();
    return {body.data(), handleBits - 8};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t> makeShapeBody() {
    DwgShapeWriterProbe shape;
    shape.handle = 0x301u;
    shape.m_insertionPoint = DRW_Coord{3.0, 4.0, 5.0};
    shape.m_scale = 2.0;
    shape.m_rotation = 0.25;
    shape.m_widthFactor = 1.25;
    shape.m_oblique = 0.1;
    shape.m_thickness = 0.5;
    shape.m_shapeIndex = 17;
    shape.m_extrusion = DRW_Coord{0.0, 0.0, 1.0};
    shape.m_shapeFileHandle = 0x302u;

    dwgBufferW body;
    dwgBufferW handles;
    if (!shape.encodeDwg(DRW::AC1027, &body, 0, nullptr, &handles))
        return {};
    body.alignToByte();
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedShapeBody() {
    auto encoded = makeShapeBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeOle2FrameBody() {
    DwgOle2FrameWriterProbe frame;
    frame.handle = 0x303u;
    frame.m_flags = 2;
    frame.m_mode = 1;
    frame.m_payloadPresent = true;
    frame.m_payloadBytes = {0x01, 0x02, 0x03, 0x04};
    frame.m_declaredPayloadLength =
        static_cast<std::uint32_t>(frame.m_payloadBytes.size());

    dwgBufferW body;
    dwgBufferW handles;
    if (!frame.encodeDwg(DRW::AC1027, &body, 0, nullptr, &handles))
        return {};
    body.alignToByte();
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedOle2FrameBody() {
    auto encoded = makeOle2FrameBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeOleFrameBody() {
    DwgOleFrameWriterProbe frame;
    frame.handle = 0x304u;
    frame.m_flags = 2;
    frame.m_mode = 1;
    frame.m_payloadPresent = true;
    frame.m_payloadBytes = {0x05, 0x06, 0x07, 0x08};
    frame.m_declaredPayloadLength =
        static_cast<std::uint32_t>(frame.m_payloadBytes.size());

    dwgBufferW body;
    dwgBufferW handles;
    if (!frame.encodeDwg(DRW::AC1027, &body, 0, nullptr, &handles))
        return {};
    body.alignToByte();
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedOleFrameBody() {
    auto encoded = makeOleFrameBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t> makeCameraBody() {
    DwgCameraProbe camera;
    camera.handle = 0x305u;
    camera.m_viewHandle = 0x306u;

    dwgBufferW body;
    dwgBufferW handles;
    if (!camera.encodeDwg(DRW::AC1027, &body, 0, nullptr, &handles))
        return {};
    body.alignToByte();
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedCameraBody() {
    auto encoded = makeCameraBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t> makeLightBody() {
    DwgLightProbe light;
    light.handle = 0x307u;
    light.m_classVersion = 1;
    light.m_name = "TEST_LIGHT";
    light.m_type = 2;
    light.m_status = true;
    light.m_color = 256;
    light.m_plotGlyph = true;
    light.m_intensity = 2.5;
    light.m_position = DRW_Coord{1.0, 2.0, 3.0};
    light.m_target = DRW_Coord{4.0, 5.0, 6.0};
    light.m_attenuationType = 1;
    light.m_useAttenuationLimits = true;
    light.m_attenuationStartLimit = 0.5;
    light.m_attenuationEndLimit = 25.0;
    light.m_hotspotAngle = 0.25;
    light.m_falloffAngle = 0.75;
    light.m_castShadows = true;
    light.m_shadowType = 1;
    light.m_shadowMapSize = 512;
    light.m_shadowMapSoftness = 3;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!light.encodeDwg(DRW::AC1027, &body, 0, &strings, &handles))
        return {};

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedLightBody() {
    auto encoded = makeLightBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeGeoPositionMarkerBody() {
    DwgGeoPositionMarkerWriterProbe marker;
    marker.handle = 0x308u;
    marker.m_classVersion = 2;
    marker.m_position = DRW_Coord{10.0, 20.0, 30.0};
    marker.m_radius = 4.0;
    marker.m_notes = "TEST_MARKER";
    marker.m_landingGap = 0.5;
    marker.m_mtextVisible = true;
    marker.m_textAlignment = 2;
    marker.m_enableFrameText = true;
    marker.mtext = std::make_unique<DRW_MText>();
    marker.mtext->text = "EMBEDDED_MARKER";
    marker.mtext->basePoint = marker.m_position;
    marker.mtext->secPoint = DRW_Coord{11.0, 20.0, 30.0};
    marker.mtext->height = 1.0;
    marker.mtext->widthscale = 1.0;
    marker.mtext->interlin = 1.0;
    marker.mtext->color = 256;
    marker.mtext->visible = true;
    marker.mtext->parentHandle = DRW::NoHandle;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!marker.encodeDwg(DRW::AC1032, &body, 0, &strings, &handles))
        return {};

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedGeoPositionMarkerBody() {
    auto encoded = makeGeoPositionMarkerBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeSectionObjectBody() {
    DwgSectionObjectProbe section;
    section.handle = 0x309u;
    section.m_state = 1;
    section.m_flags = 5;
    section.m_name = "TEST_SECTION";
    section.m_vertDir = DRW_Coord{0.0, 0.0, 1.0};
    section.m_topHeight = 10.0;
    section.m_bottomHeight = -2.0;
    section.m_indicatorAlpha = 70;
    section.m_indicatorColor = 7;
    section.m_verts = {DRW_Coord{1.0, 2.0, 0.0},
                       DRW_Coord{3.0, 4.0, 0.0}};
    section.m_blVerts = {DRW_Coord{5.0, 6.0, 0.0}};
    section.m_sectionSettingsHandle = 0x30Au;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!section.encodeDwg(DRW::AC1027, &body, 0, &strings, &handles))
        return {};

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedSectionObjectBody() {
    auto encoded = makeSectionObjectBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeToleranceBody() {
    DwgToleranceProbe tolerance;
    tolerance.handle = 0x30Bu;
    tolerance.insertionPoint = DRW_Coord{1.0, 2.0, 3.0};
    tolerance.xAxisDirectionVector = DRW_Coord{0.0, 1.0, 0.0};
    tolerance.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    tolerance.text = "TOLERANCE_TEXT";
    tolerance.dimStyleH.ref = 0x30Cu;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!tolerance.encodeDwg(DRW::AC1027, &body, 0, &strings, &handles))
        return {};

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedToleranceBody() {
    auto encoded = makeToleranceBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
make3DLineBody() {
    Dwg3DLineProbe line;
    line.handle = 0x30Du;
    line.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    line.secPoint = DRW_Coord{4.0, 5.0, 6.0};
    line.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    line.thickness = 0.25;

    dwgBufferW body;
    dwgBufferW handles;
    if (!line.encodeDwg(DRW::AC1027, &body, 0, nullptr, &handles))
        return {};
    body.alignToByte();
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncated3DLineBody() {
    auto encoded = make3DLineBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTraceBody() {
    DwgTraceWriterProbe trace;
    trace.handle = 0x30Eu;
    trace.basePoint = DRW_Coord{1.0, 2.0, 0.5};
    trace.secPoint = DRW_Coord{4.0, 5.0, 0.5};
    trace.thirdPoint = DRW_Coord{7.0, 8.0, 0.5};
    trace.fourPoint = DRW_Coord{10.0, 11.0, 0.5};
    trace.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    trace.thickness = 0.25;

    dwgBufferW body;
    dwgBufferW handles;
    if (!trace.encodeDwg(DRW::AC1027, &body, 0, nullptr, &handles))
        return {};
    body.alignToByte();
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncatedTraceBody() {
    auto encoded = makeTraceBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
make3DFaceBody() {
    Dwg3DFaceWriterProbe face;
    face.handle = 0x30Fu;
    face.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    face.secPoint = DRW_Coord{4.0, 5.0, 3.0};
    face.thirdPoint = DRW_Coord{7.0, 8.0, 3.0};
    face.fourPoint = DRW_Coord{10.0, 11.0, 3.0};
    face.invisibleflag = DRW_3Dface::AllEdges;

    dwgBufferW body;
    dwgBufferW handles;
    if (!face.encodeDwg(DRW::AC1027, &body, 0, nullptr, &handles))
        return {};
    body.alignToByte();
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeTruncated3DFaceBody() {
    auto encoded = make3DFaceBody();
    if (encoded.first.size() <= 1 || encoded.second < 8)
        return {};
    encoded.first.pop_back();
    encoded.second -= 8;
    return encoded;
}

std::vector<std::uint8_t> makeObjectFrame(
    const std::vector<std::uint8_t>& body) {
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(body.size()));
    frame.putBytes(body.data(), body.size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

std::vector<std::uint8_t> makeModernObjectFrame(
    const std::vector<std::uint8_t>& body) {
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(body.size()));
    frame.putUModularChar(0);
    frame.putBytes(body.data(), body.size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

std::vector<std::uint8_t> makeUnknownCustomEntityFrame(
    std::uint16_t classNumber, std::uint32_t handle, std::uint32_t owner,
    const std::string& proxyGraphics = {}) {
    DwgLineWriterProbe line;
    line.handle = handle;
    line.parentHandle = owner;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.proxyGraphics = proxyGraphics;
    line.setObjectType(static_cast<std::int16_t>(classNumber));

    dwgBufferW body;
    if (!line.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    if (!line.encodeDwgEntHandle(DRW::AC1018, &body))
        return {};
    return makeEntityFrame(body);
}

std::vector<std::uint8_t> makeModernUnknownCustomEntityFrame(
    DRW::Version version, std::uint16_t classNumber, std::uint32_t handle,
    std::uint32_t owner, const std::string& proxyGraphics) {
    if (version != DRW::AC1024 && version != DRW::AC1027)
        return {};

    DwgLineWriterProbe line;
    line.handle = handle;
    line.parentHandle = owner;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.proxyGraphics = proxyGraphics;
    line.setObjectType(static_cast<std::int16_t>(classNumber));

    dwgBufferW body;
    dwgBufferW handles;
    if (!line.encodeDwgCommon(version, &body))
        return {};
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    if (!line.encodeDwgEntHandle(version, &body, &handles))
        return {};
    return makeModernEntityFrame(version, body, handles);
}

std::string makeProxyCircleGraphics(std::size_t circleCount = 1u) {
    dwgBufferW graphics;
    graphics.putRawLong32(0);  // Proxy graphic header.
    graphics.putRawLong32(0);
    for (std::size_t index = 0; index < circleCount; ++index) {
        dwgBufferW payload;
        payload.putRawDouble(static_cast<double>(index));
        payload.putRawDouble(0.0);
        payload.putRawDouble(0.0);
        payload.putRawDouble(1.0);
        payload.putRawDouble(0.0);
        payload.putRawDouble(0.0);
        payload.putRawDouble(1.0);
        graphics.putRawLong32(static_cast<std::uint32_t>(
            payload.data().size() + 8u));
        graphics.putRawLong32(2);  // CIRCLE
        graphics.putBytes(payload.data().data(), payload.data().size());
    }
    if (!graphics.isGood())
        return {};
    return std::string(
        reinterpret_cast<const char*>(graphics.data().data()),
        graphics.data().size());
}

std::vector<std::uint8_t> makeProxyEntityFrame(
    std::uint32_t handle, std::uint32_t owner,
    const std::string& proxyGraphics) {
    DwgProxyEntityWriterProbe entity;
    entity.handle = handle;
    entity.parentHandle = owner;
    entity.proxyGraphics = proxyGraphics;
    entity.setObjectType(DRW_ProxyEntity::kDwgType);

    dwgBufferW body;
    if (!entity.encodeDwgCommon(DRW::AC1018, &body))
        return {};
    body.putBitLong(500);  // proxy carrier id
    body.putVariableText(DRW::AC1018, "ProxyEntity");
    body.putBitLong(0);    // drawing format
    body.putBit(0);        // source was not DXF
    const std::uint8_t bsCode =
        static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
    const std::size_t sizeBitOffset =
        bsCode == 0x01 ? 10u : bsCode == 0x00 ? 18u : 2u;
    body.patchRawLong32AtBit(sizeBitOffset, body.bitCount());
    if (!entity.encodeDwgEntHandle(DRW::AC1018, &body))
        return {};
    body.alignToByte();
    return makeObjectFrame(body.data());
}

std::vector<std::uint8_t> makeGeoPositionMarkerFrame(
    std::uint16_t classNumber) {
    DwgGeoPositionMarkerWriterProbe marker;
    marker.handle = 0x89;
    marker.color = 256;
    marker.ltypeScale = 1.0;
    marker.m_classVersion = 0;
    marker.m_position = DRW_Coord(100.0, 200.0, 5.0);
    marker.m_radius = 3.5;
    marker.m_notes = "Survey note";
    marker.m_landingGap = 1.25;
    marker.m_mtextVisible = true;
    marker.m_textAlignment = 2;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!marker.encodeDwg(DRW::AC1027, &body, 0, &strings, &handles))
        return {};

    body.alignToByte();
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    handles.alignToByte();
    const std::size_t dataBytes = body.size();
    if (!handles.data().empty())
        body.putBytes(handles.data().data(), handles.data().size());
    const auto handleBitSize = static_cast<std::uint32_t>(
        (body.size() - dataBytes) * 8u);

    // The entity encoder writes the fixed type. A real CLASSES-dispatched
    // record carries its file-local class ordinal in the OT field instead.
    dwgBuffer encoded(body.data().data(), body.data().size());
    if (encoded.getObjType(DRW::AC1027) != DRW_GeoPositionMarker::kDwgType)
        return {};
    dwgBufferW remapped;
    remapped.putObjType(DRW::AC1027, classNumber);
    for (std::size_t bit = 18; bit < body.bitCount(); ++bit)
        remapped.putBit(encoded.getBit());
    remapped.alignToByte();

    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(remapped.size()));
    frame.putUModularChar(handleBitSize);
    frame.putBytes(remapped.data().data(), remapped.data().size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

dwgHandle makeObjectHandle(std::uint32_t ref) {
    dwgHandle handle;
    handle.code = 0;
    handle.ref = ref;
    handle.ref64 = ref;
    for (std::uint32_t value = ref; value != 0; value >>= 8)
        ++handle.size;
    return handle;
}

void putDictionaryObjectPreamble(dwgBufferW& body, DRW::Version version,
                                 std::uint16_t objectType,
                                 std::uint32_t handle) {
    body.putObjType(version, objectType);
    if (version > DRW::AC1014 && version < DRW::AC1024)
        body.putRawLong32(0);
    body.putHandle(makeObjectHandle(handle));
    body.putBitShort(0);  // no EED
    if (version < DRW::AC1015)
        body.putRawLong32(0);  // legacy object size
    body.putBitLong(0);  // no reactors
    if (version > DRW::AC1015)
        body.putBit(0);  // xDictFlag: xdictionary handle follows
    if (version > DRW::AC1024)
        body.putBit(0);  // no AC1027 data-storage payload
}

std::vector<std::uint8_t> makeMalformedDictionaryBody() {
    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1015, 42, 0x7A0u);
    body.putBitLong(100001);  // above the parser's bounded object count
    body.putBitShort(0);      // cloning
    body.putRawChar8(0);      // hard owner
    return body.data();
}

std::vector<std::uint8_t> makeImpossibleDictionaryCountBody() {
    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1015, 42, 0x7A0u);
    body.putBitLong(50000);  // far beyond the remaining two-bit text budget
    body.putBitShort(0);     // cloning
    body.putRawChar8(0);     // hard owner
    return body.data();
}

std::vector<std::uint8_t> makeCappedDictionaryCountBody() {
    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1015, 42, 0x7A2u);
    body.putBitLong(2);     // both names are present; one item handle is not
    body.putBitShort(0);    // cloning
    body.putRawChar8(0);    // hard owner
    body.putVariableText(DRW::AC1015, "VALID");
    body.putVariableText(DRW::AC1015, "MISSING");
    const std::uint32_t objectSize = body.bitCount();
    body.patchRawLong32AtBit(10, objectSize);
    body.putHandle(makeObjectHandle(0));      // parent
    body.putHandle(makeObjectHandle(0));      // xdictionary
    body.putHandle(makeObjectHandle(0x903u)); // one complete item
    return body.data();
}

std::vector<std::uint8_t> makeTruncatedDictionaryBody() {
    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1015, 42, 0x7A0u);
    body.putBitLong(1);
    body.putBitShort(0);  // cloning
    body.putRawChar8(0);  // hard owner
    body.putVariableText(DRW::AC1015, "entry");
    // The common owner/xdictionary handles are intentionally absent.
    return body.data();
}

std::vector<std::uint8_t> makeOutOfBoundsDictionaryNameBody() {
    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1015, 42, 0x7A0u);
    body.putBitLong(2);
    body.putBitShort(0);      // cloning
    body.putRawChar8(0);      // hard owner
    body.putVariableText(DRW::AC1015, "in-bounds");

    // The object-size boundary ends the dictionary body after one name. The
    // second name is deliberately placed in the handle stream; a parser that
    // ignores the boundary can consume it and appear to recover valid edges.
    body.patchRawLong32AtBit(10, body.bitCount());
    body.putVariableText(DRW::AC1015, "out-of-bounds");
    body.putHandle(makeObjectHandle(0));  // parent
    body.putHandle(makeObjectHandle(0));  // xdictionary
    body.putHandle(makeObjectHandle(0x901u));
    body.putHandle(makeObjectHandle(0x902u));
    return body.data();
}

std::vector<std::uint8_t> makeOutOfBoundsDictionaryVarBody() {
    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1015,
                                DRW_DictionaryVar::kDwgClassNum, 0x7A1u);
    body.putRawChar8(0);  // schema

    // AC1015 object type 512 uses the full BS form, so the object-size RL is
    // at bit 18. The value is outside the declared body range.
    body.patchRawLong32AtBit(18, body.bitCount());
    body.putVariableText(DRW::AC1015, "out-of-bounds");
    body.putHandle(makeObjectHandle(0));  // parent
    body.putHandle(makeObjectHandle(0));  // xdictionary
    return body.data();
}

std::vector<std::uint8_t> makeR14DictionaryWithDefaultBody() {
    DwgDictionaryWithDefaultWriterProbe dictionary;
    dictionary.handle = 0x860u;
    dictionary.parentHandle = 0x0Cu;
    dictionary.cloning = 1;
    dictionary.hardOwner = 1;
    dictionary.m_defaultEntryHandle = 0x1202u;

    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1014,
                                DRW_DictionaryWithDefault::kDwgClassNum,
                                dictionary.handle);
    if (!dictionary.encodeDwg(DRW::AC1014, &body, nullptr, nullptr))
        return {};
    return body.data();
}

std::vector<std::uint8_t> makeLargeBitShortControlBody() {
    constexpr std::uint16_t ltypeControlType = 0x38;
    constexpr std::uint32_t numEntries = 256;
    constexpr std::uint32_t ltypePhantomEntries = 2;

    dwgBufferW body;
    body.putObjType(DRW::AC1015, ltypeControlType);
    body.putRawLong32(0);  // R2000 object-size field
    body.putHandle(makeObjectHandle(0x500u));
    body.putBitShort(0);   // no EED
    body.putBitLong(0);    // no reactors
    body.putBitShort(static_cast<std::uint16_t>(numEntries));
    body.putHandle(makeObjectHandle(0));  // null control handle
    body.putHandle(makeObjectHandle(0));  // null extension-dictionary handle
    for (std::uint32_t i = 0; i < numEntries + ltypePhantomEntries; ++i)
        body.putHandle(makeObjectHandle(0x600u + i));
    return body.data();
}

std::vector<std::uint8_t> makeImpossibleBitLongControlBody() {
    constexpr std::uint16_t layerControlType = 0x32;

    dwgBufferW body;
    body.putObjType(DRW::AC1015, layerControlType);
    body.putRawLong32(0);  // R2000 object-size field
    body.putHandle(makeObjectHandle(0x510u));
    body.putBitShort(0);   // no EED
    body.putBitLong(0);    // no reactors
    body.putBitLong(100001);
    body.putHandle(makeObjectHandle(0));  // null control handle
    body.putHandle(makeObjectHandle(0));  // null extension-dictionary handle
    return body.data();
}

std::vector<std::uint8_t> makeDuplicateBitShortControlBody() {
    constexpr std::uint16_t ltypeControlType = 0x38;

    dwgBufferW body;
    body.putObjType(DRW::AC1015, ltypeControlType);
    body.putRawLong32(0);  // R2000 object-size field
    body.putHandle(makeObjectHandle(0x520u));
    body.putBitShort(0);   // no EED
    body.putBitLong(0);    // no reactors
    body.putBitShort(2);   // two linetype entries
    body.putHandle(makeObjectHandle(0));  // null control handle
    body.putHandle(makeObjectHandle(0));  // null extension-dictionary handle
    body.putHandle(makeObjectHandle(0x620u));
    body.putHandle(makeObjectHandle(0x620u));
    body.putHandle(makeObjectHandle(0x621u)); // phantom entry
    body.putHandle(makeObjectHandle(0x622u)); // phantom entry
    return body.data();
}

std::vector<std::uint8_t> makeNegativeClassInstanceBody() {
    dwgBufferW body;
    body.putBitShort(500);
    body.putBitShort(0x401);
    body.putVariableText(DRW::AC1018, "ACAD");
    body.putVariableText(DRW::AC1018, "AcDbEntity");
    body.putVariableText(DRW::AC1018, "TEST_ENTITY");
    body.putBit(0);
    body.putBitShort(0x1F2);
    body.putBitLong(-1); // DXF 91 / num_instances cannot be negative.
    body.putBitLong(0);
    body.putBitLong(0);
    body.putBitLong(0);
    body.putBitLong(0);
    return body.data();
}

std::vector<std::uint8_t> makeMalformedAttribFrame() {
    dwgBufferW body;
    body.putObjType(DRW::AC1015, dwgType::ATTRIB);
    body.alignToByte();
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(body.data().size()));
    frame.putBytes(body.data().data(), body.data().size());
    const std::uint16_t crc = frame.crc16(
        0xC0C1, 0, frame.data().size());
    frame.putRawShort16(crc);
    return frame.data();
}

std::vector<std::uint8_t> makeAttribFrame(std::uint32_t handle,
                                          std::uint32_t owner) {
    DwgAttribWriterProbe attrib;
    attrib.handle = handle;
    attrib.parentHandle = owner;
    attrib.text = "value";
    attrib.tag = "TAG";
    attrib.height = 1.0;

    dwgBufferW body;
    if (!attrib.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
        return {};
    return makeEntityFrame(body);
}

struct LegacyInsertFrameFixture {
    std::vector<std::uint8_t> bytes;
    std::uint32_t firstAttribOffset = 0;
    std::uint32_t secondAttribOffset = 0;
    std::uint32_t lastAttribOffset = 0;
    std::uint32_t seqEndOffset = 0;
};

LegacyInsertFrameFixture makeLegacyInsertFrameFixture(
    bool wrongLastAttribOwner = false) {
    constexpr std::uint32_t insertHandle = 0x720;
    constexpr std::uint32_t firstAttribHandle = 0x721;
    constexpr std::uint32_t secondAttribHandle = 0x722;
    constexpr std::uint32_t lastAttribHandle = 0x723;
    constexpr std::uint32_t seqEndHandle = 0x724;

    DwgInsertWriterProbe insert;
    insert.handle = insertHandle;
    insert.setObjectType(dwgType::INSERT);
    dwgHandle first;
    first.ref = firstAttribHandle;
    dwgHandle last;
    last.ref = lastAttribHandle;
    insert.attribHandles = {first, last};
    insert.seqendH.ref = seqEndHandle;

    const auto makeAttribute = [=](std::uint32_t handle) {
        DwgAttribWriterProbe attribute;
        attribute.handle = handle;
        attribute.parentHandle = wrongLastAttribOwner && handle == lastAttribHandle
            ? insertHandle + 1u : insertHandle;
        attribute.text = "value";
        attribute.tag = "TAG";
        attribute.height = 1.0;

        dwgBufferW body;
        if (!attribute.encodeDwg(DRW::AC1015, &body, 0, nullptr, nullptr))
            return std::vector<std::uint8_t>{};
        return makeEntityFrame(body);
    };

    dwgBufferW insertBody;
    if (!insert.encodeDwg(DRW::AC1015, &insertBody, 0, nullptr, nullptr))
        return {};
    const auto insertFrame = makeEntityFrame(insertBody);
    const auto firstAttribFrame = makeAttribute(firstAttribHandle);
    const auto secondAttribFrame = makeAttribute(secondAttribHandle);
    const auto lastAttribFrame = makeAttribute(lastAttribHandle);
    const auto seqEndFrame = makeSeqEndFrame(seqEndHandle, insertHandle);
    if (insertFrame.empty() || firstAttribFrame.empty()
        || secondAttribFrame.empty() || lastAttribFrame.empty()
        || seqEndFrame.empty()) {
        return {};
    }

    LegacyInsertFrameFixture fixture;
    fixture.firstAttribOffset = static_cast<std::uint32_t>(insertFrame.size());
    fixture.secondAttribOffset = fixture.firstAttribOffset
        + static_cast<std::uint32_t>(firstAttribFrame.size());
    fixture.lastAttribOffset = fixture.secondAttribOffset
        + static_cast<std::uint32_t>(secondAttribFrame.size());
    fixture.seqEndOffset = fixture.lastAttribOffset
        + static_cast<std::uint32_t>(lastAttribFrame.size());
    fixture.bytes.reserve(insertFrame.size() + firstAttribFrame.size()
                          + secondAttribFrame.size() + lastAttribFrame.size()
                          + seqEndFrame.size());
    fixture.bytes.insert(fixture.bytes.end(), insertFrame.cbegin(),
                         insertFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), firstAttribFrame.cbegin(),
                         firstAttribFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), secondAttribFrame.cbegin(),
                         secondAttribFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), lastAttribFrame.cbegin(),
                         lastAttribFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), seqEndFrame.cbegin(),
                         seqEndFrame.cend());
    return fixture;
}

struct LegacyEmptyInsertFrameFixture {
    std::vector<std::uint8_t> bytes;
    std::uint32_t seqEndOffset = 0;
};

LegacyEmptyInsertFrameFixture makeLegacyEmptyInsertFrameFixture(
    bool asymmetricBoundaries = false) {
    constexpr std::uint32_t insertHandle = 0x730;
    constexpr std::uint32_t attributeHandle = 0x731;
    constexpr std::uint32_t seqEndHandle = 0x732;

    DwgInsertWriterProbe insert;
    insert.handle = insertHandle;
    insert.setObjectType(dwgType::INSERT);
    insert.blockRecH.ref = 0x740u;
    dwgHandle first;
    dwgHandle last;
    if (asymmetricBoundaries) {
        first.ref = attributeHandle;
        last.ref = attributeHandle;
    }
    insert.attribHandles = {first, last};
    insert.seqendH.ref = seqEndHandle;

    dwgBufferW insertBody;
    if (!insert.encodeDwg(DRW::AC1015, &insertBody, 0, nullptr, nullptr))
        return {};
    if (asymmetricBoundaries) {
        const auto& occurrences = insertBody.handleOccurrences();
        if (occurrences.size() < 4u)
            return {};
        const auto& firstAttribute = occurrences[occurrences.size() - 3u];
        const std::uint8_t nullPayload[2] = {0, 0};
        if (!insertBody.patchRawBytesAtBit(
                static_cast<std::size_t>(firstAttribute.startBit + 8u),
                nullPayload, sizeof(nullPayload)))
            return {};
    }
    const auto insertFrame = makeEntityFrame(insertBody);
    const auto seqEndFrame = makeSeqEndFrame(seqEndHandle, insertHandle);
    if (insertFrame.empty() || seqEndFrame.empty())
        return {};

    LegacyEmptyInsertFrameFixture fixture;
    fixture.seqEndOffset = static_cast<std::uint32_t>(insertFrame.size());
    fixture.bytes.reserve(insertFrame.size() + seqEndFrame.size());
    fixture.bytes.insert(fixture.bytes.end(), insertFrame.cbegin(),
                         insertFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), seqEndFrame.cbegin(),
                         seqEndFrame.cend());
    return fixture;
}

std::pair<std::vector<std::uint8_t>, std::uint32_t>
makeEmbeddedAttribBody(bool truncateEmbeddedBody) {
    DwgAttribWriterProbe attrib;
    attrib.handle = 0x2F2u;
    attrib.text = "value";
    attrib.tag = "TAG";
    attrib.height = 1.0;
    attrib.m_attributeType = 2;
    attrib.mtext = std::make_unique<DRW_MText>();
    attrib.mtext->text = "EMBEDDED_ATTRIB";
    attrib.mtext->height = 1.0;
    attrib.mtext->widthscale = 1.0;
    attrib.mtext->interlin = 1.0;
    attrib.mtext->color = 256;
    attrib.mtext->visible = true;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    if (!attrib.encodeDwg(DRW::AC1032, &body, 0, &strings, &handles))
        return {};

    body.alignToByte();
    if (truncateEmbeddedBody) {
        constexpr std::size_t bytesToRemove = 16;
        if (body.data().size() <= bytesToRemove)
            return {};
        body.data().resize(body.data().size() - bytesToRemove);
    }
    strings.alignToByte();
    if (!strings.data().empty())
        body.putBytes(strings.data().data(), strings.data().size());
    for (int i = 0; i < 7; ++i)
        body.putBit(0);
    const auto stringBitSize = static_cast<std::uint16_t>(
        strings.data().empty() ? 0 : strings.data().size() * 8u + 7u);
    body.putRawShort16(stringBitSize);
    body.putBit(!strings.data().empty());
    body.alignToByte();

    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    if (handleBits == 0 || handles.data().empty())
        return {};
    body.putBytes(handles.data().data(), handles.data().size());
    return {body.data(), handleBits};
}

template <typename T>
std::vector<std::uint8_t> makeTruncatedAttributeBody(T& attribute) {
    dwgBufferW body;
    if (!attribute.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr)
        || body.data().size() < 2)
        return {};
    finalizeEncodedEntityBody(body);
    body.data().pop_back();
    return body.data();
}

struct PolylineFrameFixture {
    std::vector<std::uint8_t> bytes;
    std::uint32_t polylineOffset = 0;
    std::uint32_t vertexOffset = 0;
    std::uint32_t secondVertexOffset = 0;
    std::uint32_t seqEndOffset = 0;
};

PolylineFrameFixture makePolylineFrameFixture(bool corruptVertex,
                                              bool duplicateVertex = false,
                                              bool wrongSeqEndOwner = false,
                                              bool wrongVertexOwner = false,
                                              double elevation = 0.0,
                                              bool includeSecondVertex = false,
                                              DRW::Version version = DRW::AC1018,
                                              DRW_Vertex::DwgSubtype vertexSubtype =
                                                  DRW_Vertex::DwgSubtype::Vertex2D,
                                              std::uint32_t owner = DRW::NoHandle) {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t secondVertexHandle = 0x102;
    constexpr std::uint32_t seqEndHandle = 0x103;

    auto vertex = std::make_shared<DwgVertexWriterProbe>();
    vertex->handle = vertexHandle;
    vertex->parentHandle = wrongVertexOwner ? polylineHandle + 1
                                            : polylineHandle;
    vertex->basePoint = DRW_Coord(10.0, 20.0, elevation);
    vertex->setDwgSubtype(vertexSubtype);

    DwgPolylineWriterProbe polyline;
    polyline.handle = polylineHandle;
    polyline.parentHandle = owner;
    polyline.basePoint.z = elevation;
    polyline.appendVertex(vertex);
    if (duplicateVertex)
        polyline.appendVertex(vertex);
    std::shared_ptr<DwgVertexWriterProbe> secondVertex;
    if (includeSecondVertex) {
        secondVertex = std::make_shared<DwgVertexWriterProbe>();
        secondVertex->handle = secondVertexHandle;
        secondVertex->parentHandle = polylineHandle;
        secondVertex->basePoint = DRW_Coord(30.0, 40.0, elevation);
        secondVertex->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);
        polyline.appendVertex(secondVertex);
    }
    polyline.setDwgSeqEndHandle(
        includeSecondVertex ? seqEndHandle : secondVertexHandle);

    dwgBufferW polylineBody;
    dwgBufferW polylineHandles;
    dwgBufferW* const polylineHandleStream = version > DRW::AC1018
        ? &polylineHandles : nullptr;
    if (!polyline.encodeDwg(
            version, &polylineBody, 0, nullptr, polylineHandleStream))
        return {};

    dwgBufferW vertexBody;
    dwgBufferW vertexHandles;
    dwgBufferW* const vertexHandleStream = version > DRW::AC1018
        ? &vertexHandles : nullptr;
    if (!vertex->encodeDwg(
            version, &vertexBody, 0, nullptr, vertexHandleStream))
        return {};

    dwgBufferW secondVertexBody;
    dwgBufferW secondVertexHandles;
    dwgBufferW* const secondVertexHandleStream = version > DRW::AC1018
        ? &secondVertexHandles : nullptr;
    if (includeSecondVertex
        && !secondVertex->encodeDwg(
            version, &secondVertexBody, 0, nullptr, secondVertexHandleStream)) {
        return {};
    }

    DwgSequenceWriterProbe seqEnd;
    seqEnd.handle = includeSecondVertex ? seqEndHandle : secondVertexHandle;
    seqEnd.parentHandle = wrongSeqEndOwner ? polylineHandle + 1
                                           : polylineHandle;
    dwgBufferW seqEndBody;
    dwgBufferW seqEndHandles;
    dwgBufferW* const seqEndHandleStream = version > DRW::AC1018
        ? &seqEndHandles : nullptr;
    if (!seqEnd.encodeDwg(
            version, &seqEndBody, 0, nullptr, seqEndHandleStream))
        return {};

    auto polylineFrame = makeModernEntityFrame(
        version, polylineBody, polylineHandles);
    auto vertexFrame = makeModernEntityFrame(
        version, vertexBody, vertexHandles);
    auto secondVertexFrame = includeSecondVertex
        ? makeModernEntityFrame(
            version, secondVertexBody, secondVertexHandles)
        : std::vector<std::uint8_t>{};
    auto seqEndFrame = makeModernEntityFrame(
        version, seqEndBody, seqEndHandles);
    if (polylineFrame.empty() || vertexFrame.empty() || seqEndFrame.empty()
        || (includeSecondVertex && secondVertexFrame.empty())) {
        return {};
    }

    PolylineFrameFixture fixture;
    fixture.vertexOffset = static_cast<std::uint32_t>(polylineFrame.size());
    fixture.secondVertexOffset = fixture.vertexOffset
        + static_cast<std::uint32_t>(vertexFrame.size());
    fixture.seqEndOffset = fixture.secondVertexOffset
        + static_cast<std::uint32_t>(secondVertexFrame.size());
    fixture.bytes.reserve(polylineFrame.size() + vertexFrame.size()
                          + secondVertexFrame.size() + seqEndFrame.size());
    fixture.bytes.insert(fixture.bytes.end(), polylineFrame.cbegin(),
                         polylineFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), vertexFrame.cbegin(),
                         vertexFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), secondVertexFrame.cbegin(),
                         secondVertexFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), seqEndFrame.cbegin(),
                         seqEndFrame.cend());
    if (corruptVertex)
        fixture.bytes[fixture.vertexOffset + vertexFrame.size() - 1] ^= 0xFF;
    return fixture;
}

struct LegacyPolylineFrameFixture {
    std::vector<std::uint8_t> bytes;
    std::uint32_t vertexOffset = 0;
    std::uint32_t secondVertexOffset = 0;
    std::uint32_t seqEndOffset = 0;
};

std::size_t legacyNoLinksBit(const dwgBufferW& body) {
    dwgBuffer cursor(
        const_cast<std::uint8_t*>(body.data().data()), body.data().size());
    cursor.getBitShort(); // object type
    cursor.getRawLong32(); // R2000 object-size field
    cursor.getHandle(); // object handle
    std::vector<DwgEedChunk> eed;
    REQUIRE(readDwgEed(DRW::AC1015, cursor, eed, body.data().size() * 8));
    cursor.getBit(); // graph flag
    cursor.get2Bits(); // entity mode
    cursor.getBitLong(); // reactor count
    return static_cast<std::size_t>(cursor.getPosition() * 8
                                    + cursor.getBitPos());
}

void clearPackedBit(dwgBufferW& body, std::size_t bit) {
    REQUIRE(bit < body.data().size() * 8);
    body.data()[bit / 8] &= static_cast<std::uint8_t>(
        ~(std::uint8_t{1} << (7 - (bit % 8))));
}

void putOffsetHandle(dwgBufferW& body, std::uint8_t code,
                     std::uint32_t ref) {
    dwgHandle handle = makeObjectHandle(ref);
    handle.code = code;
    body.putHandle(handle);
}

std::vector<std::uint8_t> makeLegacyVertexFrame(std::uint32_t handle,
                                                std::uint32_t owner,
                                                std::uint32_t next,
                                                std::uint32_t eedAppId = DRW::NoHandle,
                                                std::uint32_t eedLayer = DRW::NoHandle) {
    DwgVertexWriterProbe vertex;
    vertex.handle = handle;
    vertex.parentHandle = owner;
    vertex.basePoint = DRW_Coord(10.0, 20.0, 0.0);
    vertex.setObjectType(dwgType::VERTEX_2D);
    if (eedAppId != DRW::NoHandle && eedLayer != DRW::NoHandle)
        vertex.setEedReferences(eedAppId, eedLayer);

    dwgBufferW body;
    if (!vertex.encodeDwgCommon(DRW::AC1015, &body))
        return {};
    clearPackedBit(body, legacyNoLinksBit(body));
    body.putRawChar8(0); // vertex flags
    body.put3BitDouble(vertex.basePoint);
    body.putBitDouble(0.0); // start width
    body.putBitDouble(0.0); // end width
    body.putBitDouble(0.0); // bulge
    body.putBitDouble(0.0); // tangent direction

    // AC1015 reads owner, xdictionary, prev, next, and layer handles in this
    // order after the common entity data.
    putOffsetHandle(body, 4, owner);
    putOffsetHandle(body, 3, 0);
    putOffsetHandle(body, 4, 0); // previous entity
    putOffsetHandle(body, 4, next);
    putOffsetHandle(body, 0, 0); // layer
    return makeEntityFrame(body);
}

LegacyPolylineFrameFixture makeLegacyPolylineFrameFixture(bool cycle) {
    constexpr std::uint32_t polylineHandle = 0x120;
    constexpr std::uint32_t firstVertexHandle = 0x121;
    constexpr std::uint32_t lastVertexHandle = 0x122;
    constexpr std::uint32_t seqEndHandle = 0x123;

    auto firstVertex = std::make_shared<DwgVertexWriterProbe>();
    firstVertex->handle = firstVertexHandle;
    firstVertex->parentHandle = polylineHandle;
    firstVertex->basePoint = DRW_Coord(10.0, 20.0, 0.0);
    firstVertex->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);

    auto lastVertex = std::make_shared<DwgVertexWriterProbe>();
    lastVertex->handle = lastVertexHandle;
    lastVertex->parentHandle = polylineHandle;
    lastVertex->basePoint = DRW_Coord(30.0, 40.0, 0.0);
    lastVertex->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);

    DwgPolylineWriterProbe polyline;
    polyline.handle = polylineHandle;
    polyline.appendVertex(firstVertex);
    polyline.appendVertex(lastVertex);
    polyline.setDwgSeqEndHandle(seqEndHandle);

    dwgBufferW polylineBody;
    if (!polyline.encodeDwg(DRW::AC1015, &polylineBody, 0, nullptr, nullptr))
        return {};
    const auto polylineFrame = makeEntityFrame(polylineBody);

    const auto firstVertexFrame = makeLegacyVertexFrame(
        firstVertexHandle, polylineHandle,
        cycle ? firstVertexHandle : 0);
    const auto seqEndFrame = makeSeqEndFrame(seqEndHandle, polylineHandle);
    if (polylineFrame.empty() || firstVertexFrame.empty() || seqEndFrame.empty())
        return {};

    LegacyPolylineFrameFixture fixture;
    fixture.vertexOffset = static_cast<std::uint32_t>(polylineFrame.size());
    fixture.seqEndOffset = fixture.vertexOffset
        + static_cast<std::uint32_t>(firstVertexFrame.size());
    fixture.bytes.reserve(polylineFrame.size() + firstVertexFrame.size()
                          + seqEndFrame.size());
    fixture.bytes.insert(fixture.bytes.end(), polylineFrame.cbegin(),
                         polylineFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), firstVertexFrame.cbegin(),
                         firstVertexFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), seqEndFrame.cbegin(),
                         seqEndFrame.cend());
    return fixture;
}

LegacyPolylineFrameFixture makeCompleteLegacyPolylineFrameFixture(
    bool wrongSeqEndOwner = false, bool wrongFirstVertexOwner = false,
    bool firstVertexHasEed = false) {
    constexpr std::uint32_t polylineHandle = 0x120;
    constexpr std::uint32_t firstVertexHandle = 0x121;
    constexpr std::uint32_t lastVertexHandle = 0x122;
    constexpr std::uint32_t seqEndHandle = 0x123;

    auto firstVertex = std::make_shared<DwgVertexWriterProbe>();
    firstVertex->handle = firstVertexHandle;
    firstVertex->parentHandle = polylineHandle;
    firstVertex->basePoint = DRW_Coord(10.0, 20.0, 0.0);
    firstVertex->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);

    auto lastVertex = std::make_shared<DwgVertexWriterProbe>();
    lastVertex->handle = lastVertexHandle;
    lastVertex->parentHandle = polylineHandle;
    lastVertex->basePoint = DRW_Coord(30.0, 40.0, 0.0);
    lastVertex->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);

    DwgPolylineWriterProbe polyline;
    polyline.handle = polylineHandle;
    polyline.appendVertex(firstVertex);
    polyline.appendVertex(lastVertex);
    polyline.setDwgSeqEndHandle(seqEndHandle);
    dwgBufferW polylineBody;
    if (!polyline.encodeDwg(DRW::AC1015, &polylineBody, 0, nullptr, nullptr))
        return {};

    const auto firstVertexFrame = makeLegacyVertexFrame(
        firstVertexHandle,
        wrongFirstVertexOwner ? polylineHandle + 1 : polylineHandle,
        lastVertexHandle, firstVertexHasEed ? 0x150 : DRW::NoHandle,
        firstVertexHasEed ? 0x151 : DRW::NoHandle);
    const auto lastVertexFrame = makeLegacyVertexFrame(
        lastVertexHandle, polylineHandle, 0);
    const auto seqEndFrame = makeSeqEndFrame(
        seqEndHandle, wrongSeqEndOwner ? polylineHandle + 1 : polylineHandle);
    const auto polylineFrame = makeEntityFrame(polylineBody);
    if (polylineFrame.empty() || firstVertexFrame.empty()
        || lastVertexFrame.empty() || seqEndFrame.empty()) {
        return {};
    }

    LegacyPolylineFrameFixture fixture;
    fixture.vertexOffset = static_cast<std::uint32_t>(polylineFrame.size());
    fixture.secondVertexOffset = fixture.vertexOffset
        + static_cast<std::uint32_t>(firstVertexFrame.size());
    fixture.seqEndOffset = fixture.secondVertexOffset
        + static_cast<std::uint32_t>(lastVertexFrame.size());
    fixture.bytes.reserve(polylineFrame.size() + firstVertexFrame.size()
                          + lastVertexFrame.size() + seqEndFrame.size());
    fixture.bytes.insert(fixture.bytes.end(), polylineFrame.cbegin(),
                         polylineFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), firstVertexFrame.cbegin(),
                         firstVertexFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), lastVertexFrame.cbegin(),
                         lastVertexFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), seqEndFrame.cbegin(),
                         seqEndFrame.cend());
    return fixture;
}

LegacyPolylineFrameFixture makeLegacyPolylineWrongTailFrameFixture() {
    constexpr std::uint32_t polylineHandle = 0x120;
    constexpr std::uint32_t lastVertexHandle = 0x122;
    constexpr std::uint32_t seqEndHandle = 0x123;
    const LegacyPolylineFrameFixture valid =
        makeCompleteLegacyPolylineFrameFixture();
    if (valid.bytes.empty())
        return {};

    const auto invalidTail = makeSeqEndFrame(lastVertexHandle, polylineHandle);
    const auto sequenceEnd = makeSeqEndFrame(seqEndHandle, polylineHandle);
    if (invalidTail.empty() || sequenceEnd.empty())
        return {};

    LegacyPolylineFrameFixture fixture;
    fixture.vertexOffset = valid.vertexOffset;
    fixture.secondVertexOffset = valid.secondVertexOffset;
    fixture.seqEndOffset = fixture.secondVertexOffset
        + static_cast<std::uint32_t>(invalidTail.size());
    fixture.bytes.insert(
        fixture.bytes.end(), valid.bytes.cbegin(),
        valid.bytes.cbegin() + static_cast<std::ptrdiff_t>(valid.secondVertexOffset));
    fixture.bytes.insert(
        fixture.bytes.end(), invalidTail.cbegin(), invalidTail.cend());
    fixture.bytes.insert(
        fixture.bytes.end(), sequenceEnd.cbegin(), sequenceEnd.cend());
    return fixture;
}

std::uint32_t readLittleEndian32(const std::uint8_t* bytes);
void writeLittleEndian32(std::uint8_t* bytes, std::uint32_t value);

std::vector<std::uint8_t> makeAc18DataPage(
    std::uint64_t address, std::uint64_t startOffset,
    const std::vector<std::uint8_t>& payload, std::uint32_t sectionNumber = 1,
    std::uint64_t uncompressedSizeOverride = 0) {
    const auto uncompressedSize = uncompressedSizeOverride == 0
        ? payload.size() : uncompressedSizeOverride;
    dwgBufferW header;
    header.putRawLong32(0x4163043B);
    header.putRawLong32(sectionNumber);
    header.putRawLong32(static_cast<std::uint32_t>(payload.size()));
    header.putRawLong32(static_cast<std::uint32_t>(uncompressedSize));
    header.putRawLong32(static_cast<std::uint32_t>(startOffset));
    header.putRawLong32(0);
    header.putRawLong32(0);
    header.putRawLong32(dwgUtil::checksum18(0, payload.data(), payload.size()));
    auto bytes = header.data();
    const auto dataChecksum = readLittleEndian32(bytes.data() + 28);
    std::fill_n(bytes.data() + 24, 4, std::uint8_t{0});
    writeLittleEndian32(
        bytes.data() + 24,
        dwgUtil::checksum18(dataChecksum, bytes.data(), bytes.size()));
    dwgCompressor::decrypt18Hdr(bytes.data(), bytes.size(), address);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

std::vector<std::uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::vector<std::uint8_t>(
        std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

std::uint32_t readLittleEndian32(const std::uint8_t* bytes) {
    return static_cast<std::uint32_t>(bytes[0])
        | (static_cast<std::uint32_t>(bytes[1]) << 8)
        | (static_cast<std::uint32_t>(bytes[2]) << 16)
        | (static_cast<std::uint32_t>(bytes[3]) << 24);
}

std::uint64_t readLittleEndian64(const std::uint8_t* bytes) {
    std::uint64_t value = 0;
    for (unsigned int i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(bytes[i]) << (i * 8);
    return value;
}

void writeLittleEndian32(std::uint8_t* bytes, std::uint32_t value) {
    bytes[0] = static_cast<std::uint8_t>(value);
    bytes[1] = static_cast<std::uint8_t>(value >> 8);
    bytes[2] = static_cast<std::uint8_t>(value >> 16);
    bytes[3] = static_cast<std::uint8_t>(value >> 24);
}

void writeLittleEndian64(std::uint8_t* bytes, std::uint64_t value) {
    for (unsigned int i = 0; i < 8; ++i)
        bytes[i] = static_cast<std::uint8_t>(value >> (i * 8));
}

constexpr std::size_t r2007FileHeaderOffset = 0x80;
constexpr std::size_t r2007EncodedHeaderSize = 0x2FD;
constexpr std::size_t r2007DecodedHeaderSize = 0x2CD;
constexpr std::size_t r2007HeaderPreambleSize = 32;
constexpr std::size_t r2007FileHeaderDataSize = 0x110;

bool decodeR2007FileHeader(
    const std::vector<std::uint8_t>& bytes,
    std::array<std::uint8_t, r2007HeaderPreambleSize>& preamble,
    std::array<std::uint8_t, r2007FileHeaderDataSize>& fileHeaderData) {
    if (bytes.size() < r2007FileHeaderOffset + r2007EncodedHeaderSize)
        return false;

    std::array<std::uint8_t, r2007EncodedHeaderSize> encoded{};
    std::copy_n(
        bytes.cbegin() + static_cast<std::ptrdiff_t>(r2007FileHeaderOffset),
        encoded.size(), encoded.begin());
    std::array<std::uint8_t, r2007DecodedHeaderSize> decoded{};
    if (!dwgRSCodec::decode239I(encoded.data(), decoded.data(), 3))
        return false;

    std::copy_n(decoded.cbegin(), preamble.size(), preamble.begin());
    const auto compressedLength = static_cast<std::int32_t>(
        readLittleEndian32(decoded.data() + 24));
    if (compressedLength < 0) {
        if (compressedLength == std::numeric_limits<std::int32_t>::min())
            return false;
        const auto rawLength = static_cast<std::size_t>(-compressedLength);
        if (rawLength < fileHeaderData.size()
            || rawLength > decoded.size() - r2007HeaderPreambleSize)
            return false;
        std::copy_n(decoded.cbegin() + r2007HeaderPreambleSize,
                    fileHeaderData.size(), fileHeaderData.begin());
        return true;
    }

    if (compressedLength == 0
        || static_cast<std::size_t>(compressedLength)
               > decoded.size() - r2007HeaderPreambleSize)
        return false;
    dwgCompressor compressor;
    return compressor.decompress21(
        decoded.data() + r2007HeaderPreambleSize, fileHeaderData.data(),
        static_cast<std::size_t>(compressedLength), fileHeaderData.size());
}

bool encodeR2007FileHeader(
    std::vector<std::uint8_t>& bytes,
    const std::array<std::uint8_t, r2007HeaderPreambleSize>& preamble,
    const std::array<std::uint8_t, r2007FileHeaderDataSize>& fileHeaderData) {
    if (bytes.size() < r2007FileHeaderOffset + r2007EncodedHeaderSize)
        return false;

    std::array<std::uint8_t, r2007DecodedHeaderSize> decoded{};
    std::copy(preamble.cbegin(), preamble.cend(), decoded.begin());
    // The test uses the legal uncompressed inner-header form so it can mutate
    // fields without adding a second R2007 LZ77 compressor to libdxfrw.
    writeLittleEndian32(
        decoded.data() + 24,
        static_cast<std::uint32_t>(-
            static_cast<std::int32_t>(fileHeaderData.size())));
    auto repairedHeader = fileHeaderData;
    std::fill(repairedHeader.begin() + 264, repairedHeader.begin() + 272,
              std::uint8_t{0});
    writeLittleEndian64(
        repairedHeader.data() + 264,
        dwgUtil::crc64Normal(
            dwgUtil::updateSeed2(0, repairedHeader.size()),
            repairedHeader.data(), repairedHeader.size()));
    std::copy(repairedHeader.cbegin(), repairedHeader.cend(),
              decoded.begin() + static_cast<std::ptrdiff_t>(
                  r2007HeaderPreambleSize));

    RScodec codec(0x96, 8, 8);
    std::array<std::uint8_t, r2007EncodedHeaderSize> reencoded{};
    for (std::size_t block = 0; block < 3; ++block) {
        std::array<std::uint8_t, 239> data{};
        std::array<std::uint8_t, 16> parity{};
        std::copy_n(decoded.cbegin() + static_cast<std::ptrdiff_t>(block * 239),
                    data.size(), data.begin());
        if (!codec.encode(data.data(), parity.data()))
            return false;
        for (std::size_t i = 0; i < data.size(); ++i)
            reencoded[block + i * 3] = data[i];
        for (std::size_t i = 0; i < parity.size(); ++i)
            reencoded[block + (data.size() + i) * 3] = parity[i];
    }

    std::array<std::uint8_t, r2007DecodedHeaderSize> roundTrip{};
    if (!dwgRSCodec::decode239I(reencoded.data(), roundTrip.data(), 3)
        || !std::equal(decoded.cbegin(), decoded.cend(), roundTrip.cbegin()))
        return false;
    std::copy(reencoded.cbegin(), reencoded.cend(),
              bytes.begin() + static_cast<std::ptrdiff_t>(
                  r2007FileHeaderOffset));
    return true;
}

bool reencodeR2007PageMapCompressedCrc(std::vector<std::uint8_t>& bytes) {
    constexpr std::size_t pageMapCompressedCrcOffset = 16;
    std::array<std::uint8_t, r2007HeaderPreambleSize> preamble{};
    std::array<std::uint8_t, r2007FileHeaderDataSize> fileHeaderData{};
    if (!decodeR2007FileHeader(bytes, preamble, fileHeaderData)
        || pageMapCompressedCrcOffset + sizeof(std::uint64_t)
               > fileHeaderData.size())
        return false;

    const auto storedCrc = readLittleEndian64(
        fileHeaderData.data() + pageMapCompressedCrcOffset);
    writeLittleEndian64(fileHeaderData.data() + pageMapCompressedCrcOffset,
                        storedCrc ^ 1u);
    return encodeR2007FileHeader(bytes, preamble, fileHeaderData);
}

// Re-encode the fixed R2007 file-header envelope after changing a decoded
// header field. The test deliberately switches the inner header to the
// permitted uncompressed form, avoiding a second, untested LZ77 compressor.
bool reencodeR2007FileHeader(std::vector<std::uint8_t>& bytes,
                             std::uint64_t pagesMaxId) {
    constexpr std::size_t pagesMaxIdOffset = 104;

    std::array<std::uint8_t, r2007HeaderPreambleSize> preamble{};
    std::array<std::uint8_t, r2007FileHeaderDataSize> fileHeaderData{};
    if (!decodeR2007FileHeader(bytes, preamble, fileHeaderData))
        return false;
    if (pagesMaxIdOffset + sizeof(std::uint64_t) > fileHeaderData.size())
        return false;
    writeLittleEndian64(fileHeaderData.data() + pagesMaxIdOffset, pagesMaxId);
    return encodeR2007FileHeader(bytes, preamble, fileHeaderData);
}

bool reencodeR2007PagesAmount(std::vector<std::uint8_t>& bytes,
                              std::uint64_t pagesAmount) {
    constexpr std::size_t pagesAmountOffset = 96;
    std::array<std::uint8_t, r2007HeaderPreambleSize> preamble{};
    std::array<std::uint8_t, r2007FileHeaderDataSize> fileHeaderData{};
    if (!decodeR2007FileHeader(bytes, preamble, fileHeaderData)
        || pagesAmountOffset + sizeof(std::uint64_t) > fileHeaderData.size())
        return false;
    writeLittleEndian64(fileHeaderData.data() + pagesAmountOffset, pagesAmount);
    return encodeR2007FileHeader(bytes, preamble, fileHeaderData);
}

bool reencodeR2007SectionsAmount(std::vector<std::uint8_t>& bytes,
                                 std::uint64_t sectionsAmount) {
    constexpr std::size_t sectionsAmountOffset = 160;
    std::array<std::uint8_t, r2007HeaderPreambleSize> preamble{};
    std::array<std::uint8_t, r2007FileHeaderDataSize> fileHeaderData{};
    if (!decodeR2007FileHeader(bytes, preamble, fileHeaderData)
        || sectionsAmountOffset + sizeof(std::uint64_t) > fileHeaderData.size())
        return false;
    writeLittleEndian64(fileHeaderData.data() + sectionsAmountOffset,
                        sectionsAmount);
    return encodeR2007FileHeader(bytes, preamble, fileHeaderData);
}

bool encodeR2007SystemPage(const std::vector<std::uint8_t>& data,
                           std::vector<std::uint8_t>& encoded) {
    if (data.empty())
        return false;
    const auto alignedSize = ((data.size() + 7) / 8) * 8;
    const auto blockCount = (alignedSize + 238) / 239;
    if (blockCount == 0 || blockCount > std::numeric_limits<std::uint32_t>::max())
        return false;

    std::vector<std::uint8_t> rsData(blockCount * 239, 0);
    std::copy(data.cbegin(), data.cend(), rsData.begin());
    encoded.assign(blockCount * 255, 0);
    RScodec codec(0x96, 8, 8);
    for (std::size_t block = 0; block < blockCount; ++block) {
        std::array<std::uint8_t, 239> dataBlock{};
        std::array<std::uint8_t, 16> parity{};
        std::copy_n(rsData.cbegin() + static_cast<std::ptrdiff_t>(block * 239),
                    dataBlock.size(), dataBlock.begin());
        if (!codec.encode(dataBlock.data(), parity.data()))
            return false;
        for (std::size_t i = 0; i < dataBlock.size(); ++i)
            encoded[block + i * blockCount] = dataBlock[i];
        for (std::size_t i = 0; i < parity.size(); ++i)
            encoded[block + (dataBlock.size() + i) * blockCount] = parity[i];
    }
    return true;
}

bool appendR2007OutOfRangePageId(std::vector<std::uint8_t>& bytes) {
    std::array<std::uint8_t, r2007HeaderPreambleSize> preamble{};
    std::array<std::uint8_t, r2007FileHeaderDataSize> fileHeaderData{};
    if (!decodeR2007FileHeader(bytes, preamble, fileHeaderData))
        return false;

    const auto compressedSize = readLittleEndian64(fileHeaderData.data() + 80);
    const auto uncompressedSize = readLittleEndian64(fileHeaderData.data() + 88);
    const auto correctionFactor = readLittleEndian64(fileHeaderData.data() + 24);
    const auto pageMapOffset = readLittleEndian64(fileHeaderData.data() + 56);
    const auto pageMapAddress = std::uint64_t{0x480} + pageMapOffset;
    if (compressedSize == 0 || uncompressedSize == 0 || correctionFactor == 0
        || compressedSize > std::numeric_limits<std::size_t>::max()
        || uncompressedSize > std::numeric_limits<std::size_t>::max())
        return false;
    const auto alignedCompressed = ((compressedSize + 7) / 8) * 8;
    if (alignedCompressed > std::numeric_limits<std::uint64_t>::max() / correctionFactor)
        return false;
    const auto blockCount =
        (alignedCompressed * correctionFactor + 238) / 239;
    if (blockCount == 0
        || blockCount > std::numeric_limits<std::size_t>::max() / 255
        || !dwgSafety::range(pageMapAddress, blockCount * 255, bytes.size()))
        return false;

    std::vector<std::uint8_t> encodedMap(blockCount * 255);
    std::copy_n(bytes.cbegin() + static_cast<std::ptrdiff_t>(pageMapAddress),
                encodedMap.size(), encodedMap.begin());
    std::vector<std::uint8_t> decodedMap(blockCount * 239);
    if (!dwgRSCodec::decode239I(encodedMap.data(), decodedMap.data(),
                                static_cast<std::uint32_t>(blockCount)))
        return false;

    std::vector<std::uint8_t> pageMap(static_cast<std::size_t>(uncompressedSize));
    if (compressedSize < uncompressedSize) {
        dwgCompressor compressor;
        if (!compressor.decompress21(decodedMap.data(), pageMap.data(),
                                     compressedSize, uncompressedSize))
            return false;
    } else {
        if (uncompressedSize > decodedMap.size())
            return false;
        std::copy_n(decodedMap.cbegin(), pageMap.size(), pageMap.begin());
    }
    if (pageMap.size() < 16)
        return false;

    const auto pagesMaxId = readLittleEndian64(fileHeaderData.data() + 104);
    if (pagesMaxId == std::numeric_limits<std::uint64_t>::max())
        return false;
    writeLittleEndian64(pageMap.data() + 8, pagesMaxId + 1);

    std::vector<std::uint8_t> replacement;
    if (!encodeR2007SystemPage(pageMap, replacement))
        return false;
    const auto replacementOffset = bytes.size();
    if (replacementOffset < 0x480
        || replacementOffset - 0x480 > std::numeric_limits<std::uint64_t>::max())
        return false;
    writeLittleEndian64(fileHeaderData.data() + 8,
                        replacementOffset + replacement.size());
    writeLittleEndian64(fileHeaderData.data() + 24, 1);
    writeLittleEndian64(fileHeaderData.data() + 56,
                        replacementOffset - 0x480);
    writeLittleEndian64(fileHeaderData.data() + 80, pageMap.size());
    writeLittleEndian64(fileHeaderData.data() + 88, pageMap.size());
    bytes.insert(bytes.end(), replacement.cbegin(), replacement.cend());
    return encodeR2007FileHeader(bytes, preamble, fileHeaderData);
}

bool refreshAc1018PageChecksum(std::vector<std::uint8_t>& bytes,
                               std::size_t payloadPosition) {
    constexpr std::uint32_t dataPageType = 0x4163043B;
    constexpr std::size_t pageHeaderSize = 32;
    constexpr std::size_t headerChecksumOffset = 24;
    constexpr std::size_t dataChecksumOffset = 28;

    for (std::size_t headerPosition = 0;
         headerPosition + pageHeaderSize <= payloadPosition; ++headerPosition) {
        std::uint8_t header[pageHeaderSize];
        std::copy_n(bytes.cbegin() + static_cast<std::ptrdiff_t>(headerPosition),
                    pageHeaderSize, std::begin(header));
        dwgCompressor::decrypt18Hdr(header, pageHeaderSize, headerPosition);
        if (readLittleEndian32(header) != dataPageType)
            continue;

        const auto compressedSize = static_cast<std::size_t>(
            readLittleEndian32(header + 8));
        if (headerPosition + pageHeaderSize > bytes.size()
            || compressedSize > bytes.size() - headerPosition - pageHeaderSize
            || payloadPosition < headerPosition + pageHeaderSize
            || payloadPosition >= headerPosition + pageHeaderSize + compressedSize)
            continue;

        const auto payload = bytes.data() + headerPosition + pageHeaderSize;
        const auto dataChecksum = dwgUtil::checksum18(
            0, payload, compressedSize);
        std::fill_n(header + headerChecksumOffset, 4, std::uint8_t{0});
        writeLittleEndian32(header + dataChecksumOffset, dataChecksum);
        const auto headerChecksum = dwgUtil::checksum18(
            dataChecksum, header, pageHeaderSize);
        writeLittleEndian32(header + headerChecksumOffset, headerChecksum);
        dwgCompressor::decrypt18Hdr(header, pageHeaderSize, headerPosition);
        std::copy_n(std::begin(header), pageHeaderSize,
                    bytes.begin() + static_cast<std::ptrdiff_t>(headerPosition));
        return true;
    }
    return false;
}

} // namespace

TEST_CASE("DWG checked arithmetic rejects overflow", "[dwg][safety]") {
    std::uint64_t result = 0;
    CHECK_FALSE(dwgSafety::add(std::numeric_limits<std::uint64_t>::max(), 1, result));
    CHECK_FALSE(dwgSafety::multiply(std::numeric_limits<std::uint64_t>::max(), 2, result));
    CHECK_FALSE(dwgSafety::alignUp8(std::numeric_limits<std::uint64_t>::max(), result));
    CHECK(dwgSafety::range(4, 6, 10));
    CHECK_FALSE(dwgSafety::range(5, 6, 10));
}

TEST_CASE("DWG R2004 header XOR handles unaligned large buffers",
          "[dwg][safety][primitives]") {
    constexpr std::size_t wordCount = 256;
    constexpr std::size_t byteCount = wordCount * 4;
    std::vector<std::uint8_t> bytes(byteCount + 2, 0);
    for (std::size_t i = 0; i < byteCount; ++i)
        bytes[i + 1] = static_cast<std::uint8_t>(i * 37u + 11u);
    const std::vector<std::uint8_t> original = bytes;

    dwgCompressor::decrypt18Hdr(bytes.data() + 1, byteCount, 0x1234u);
    CHECK(bytes != original);
    CHECK(bytes.front() == original.front());
    CHECK(bytes.back() == original.back());

    dwgCompressor::decrypt18Hdr(bytes.data() + 1, byteCount, 0x1234u);
    CHECK(bytes == original);
}

TEST_CASE("DWG R2007 CRC primitives follow the specification",
          "[dwg][safety][fixture]") {
    CHECK(dwgUtil::updateSeed2(0, 0x110) == 0xFC61189A45A9E6E5ULL);
    CHECK(dwgUtil::decodeCrcSeed(0xb4490d12407037daULL) == 0);

    if (!hasLocalFixture("visualstyle_r2007.dwg")) {
        SKIP("visualstyle_r2007.dwg fixture absent; skipping");
    }
    const auto bytes = readFile(localFixture("visualstyle_r2007.dwg"));
    REQUIRE(bytes.size() >= r2007FileHeaderOffset + r2007EncodedHeaderSize);
    std::array<std::uint8_t, r2007HeaderPreambleSize> preamble{};
    std::array<std::uint8_t, r2007FileHeaderDataSize> header{};
    REQUIRE(decodeR2007FileHeader(bytes, preamble, header));
    const auto expected = readLittleEndian64(header.data() + 264);
    std::fill(header.begin() + 264, header.begin() + 272, 0);
    CHECK(dwgUtil::crc64Normal(
              dwgUtil::updateSeed2(0, header.size()), header.data(),
              header.size()) == expected);
}

TEST_CASE("DWG integrity diagnostics retain a bounded prefix",
          "[dwg][safety][primitives]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(std::make_unique<dwgBuffer>(
        dummy, sizeof(dummy)));
    for (std::uint32_t index = 0;
         index < dwgSafety::MaxIntegrityDiagnostics + 8u; ++index) {
        DwgIntegrityDiagnostic diagnostic;
        diagnostic.kind = DwgIntegrityCheckKind::PageRange;
        diagnostic.observed = index;
        diagnostic.hasObserved = true;
        reader.addIntegrityDiagnostic(std::move(diagnostic));
    }
    REQUIRE(reader.m_integrityDiagnostics.size()
            == dwgSafety::MaxIntegrityDiagnostics);
    CHECK(reader.m_integrityDiagnosticsDropped == 8u);
    CHECK(reader.m_integrityDiagnostics.front().observed == 0u);
    CHECK(reader.m_integrityDiagnostics.back().observed
          == dwgSafety::MaxIntegrityDiagnostics - 1u);
}

TEST_CASE("DWG object frame failures carry caller context",
          "[dwg][safety][primitives]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(std::make_unique<dwgBuffer>(
        dummy, sizeof(dummy)));
    const objHandle object(0, 0x42, 7);

    reader.recordObjectFrameFailure(
        object, DwgIntegrityAddressSpace::DecodedBuffer);

    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    const auto& diagnostic = reader.m_integrityDiagnostics.front();
    CHECK(diagnostic.kind == DwgIntegrityCheckKind::ObjectFrameBounds);
    CHECK(diagnostic.phase == DwgIntegrityPhase::ObjectFrame);
    CHECK(diagnostic.severity == DwgIntegritySeverity::Error);
    CHECK(diagnostic.offsetSpace == DwgIntegrityAddressSpace::DecodedBuffer);
    CHECK(diagnostic.hasFileOffset);
    CHECK(diagnostic.fileOffset == 7);
    CHECK(diagnostic.hasLogicalHandle);
    CHECK(diagnostic.logicalHandle == 0x42);
}

TEST_CASE("DWG entity and object frame failures keep explicit address space",
          "[dwg][safety][primitives]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe entityReader(std::make_unique<dwgBuffer>(
        dummy, sizeof(dummy)));
    entityReader.setVersionForTest(DRW::AC1018);
    DwgReadProbe entityInterface;
    objHandle entity(dwgType::LINE, 0x43, 0x1234);
    bool entityFrameFailure = false;
    CHECK_FALSE(entityReader.readDwgEntity(
        nullptr, entity, entityInterface, &entityFrameFailure,
        DwgIntegrityAddressSpace::PhysicalFile));
    CHECK(entityFrameFailure);
    REQUIRE(entityReader.m_integrityDiagnostics.size() == 1);
    CHECK(entityReader.m_integrityDiagnostics.front().offsetSpace
          == DwgIntegrityAddressSpace::PhysicalFile);
    CHECK(entityReader.m_integrityDiagnostics.front().fileOffset
          == entity.loc);
    CHECK(entityInterface.unsupportedObjects.empty());

    DwgEntityReaderProbe objectReader(std::make_unique<dwgBuffer>(
        dummy, sizeof(dummy)));
    objectReader.setVersionForTest(DRW::AC1018);
    DwgReadProbe objectInterface;
    objHandle object(dwgObjType::DICTIONARY, 0x44, 0x5678);
    bool objectFrameFailure = false;
    CHECK_FALSE(objectReader.readDwgObject(
        nullptr, object, objectInterface, &objectFrameFailure,
        DwgIntegrityAddressSpace::DecodedBuffer));
    CHECK(objectFrameFailure);
    REQUIRE(objectReader.m_integrityDiagnostics.size() == 1);
    CHECK(objectReader.m_integrityDiagnostics.front().offsetSpace
          == DwgIntegrityAddressSpace::DecodedBuffer);
    CHECK(objectReader.m_integrityDiagnostics.front().fileOffset
          == object.loc);
    CHECK(objectInterface.unsupportedObjects.empty());
}

TEST_CASE("DWG missing object type is a structural frame failure",
          "[dwg][safety][primitives]") {
    dwgBufferW emptyBody;
    const auto bytes = makeEntityFrame(emptyBody);
    REQUIRE(!bytes.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(bytes.data()), bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    DwgReadProbe interface;
    objHandle entity(dwgType::LINE, 0x45, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
    bool frameFailure = false;
    CHECK_FALSE(reader.readDwgEntity(
        &buffer, entity, interface, &frameFailure,
        DwgIntegrityAddressSpace::DecodedBuffer));
    CHECK(frameFailure);
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    CHECK(reader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::ObjectFrameBounds);
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG R2007 system-page geometry failures are diagnosed",
          "[dwg][safety][primitives]") {
    std::uint8_t input[] = {0};
    std::uint8_t output[] = {0};
    DwgSystemPageReaderProbe reader(
        std::make_unique<dwgBuffer>(input, sizeof(input)));

    CHECK_FALSE(reader.parseSysPage(
        0, 1, 1, 0, 0, 0, 0, output));
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    const auto& diagnostic = reader.m_integrityDiagnostics.front();
    CHECK(diagnostic.kind == DwgIntegrityCheckKind::PageGeometry);
    CHECK(diagnostic.phase == DwgIntegrityPhase::PageMap);
    CHECK(diagnostic.severity == DwgIntegritySeverity::Error);
    CHECK(diagnostic.offsetSpace == DwgIntegrityAddressSpace::PhysicalFile);
    CHECK(diagnostic.hasFileOffset);
    CHECK(diagnostic.fileOffset == 0);
}

TEST_CASE("DWG R2007 class phase reports a missing section",
          "[dwg][safety][primitives]") {
    std::uint8_t input[] = {0};
    DwgClassesReader21Probe reader(
        std::make_unique<dwgBuffer>(input, sizeof(input)));
    reader.setVersionForTest(DRW::AC1021);

    CHECK_FALSE(reader.readDwgClasses());
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    const auto& diagnostic = reader.m_integrityDiagnostics.front();
    CHECK(diagnostic.kind == DwgIntegrityCheckKind::SectionPageReference);
    CHECK(diagnostic.phase == DwgIntegrityPhase::SectionParser);
    CHECK(diagnostic.severity == DwgIntegritySeverity::Error);
    CHECK(diagnostic.logicalSectionId == secEnum::CLASSES);
}

TEST_CASE("DWG R2007 page CRC mismatch is surfaced", "[dwg][safety]") {
    const std::vector<std::uint8_t> payload{
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    std::vector<std::uint8_t> encoded;
    REQUIRE(encodeR2007SystemPage(payload, encoded));

    const std::uint64_t seed = 0x1234;
    const auto crc = dwgUtil::crc64Mirrored(
        dwgUtil::updateSeed1(seed, payload.size()), payload.data(),
        payload.size());
    std::vector<std::uint8_t> decoded(payload.size(), 0);
    DwgSystemPageReaderProbe reader(
        std::make_unique<dwgBuffer>(encoded.data(), encoded.size()));
    REQUIRE(reader.parseSysPage(
        payload.size(), payload.size(), 1, 0, seed, crc ^ 1, crc,
        decoded.data()));
    CHECK(decoded == payload);
    CHECK(reader.crcMismatchCount() == 1);
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    const auto& diagnostic = reader.m_integrityDiagnostics.front();
    CHECK(diagnostic.kind == DwgIntegrityCheckKind::SystemPageCrc);
    CHECK(diagnostic.severity == DwgIntegritySeverity::Warning);
    CHECK(diagnostic.phase == DwgIntegrityPhase::PageMap);
    CHECK(diagnostic.offsetSpace == DwgIntegrityAddressSpace::DecodedBuffer);
    CHECK_FALSE(diagnostic.hasFileOffset);
    CHECK(diagnostic.hasExpected);
    CHECK(diagnostic.hasObserved);
    CHECK(diagnostic.expected == (crc ^ 1));
    CHECK(diagnostic.observed == crc);
}

TEST_CASE("DWG R2007 integrity diagnostics are exposed", "[dwg][safety]") {
    const auto source = localFixture("visualstyle_r2007.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("visualstyle_r2007.dwg fixture absent; skipping");
    }
    DwgReadProbe interface;
    dwgRW reader(source.string().c_str());
    REQUIRE(reader.read(&interface, true));
    CHECK(reader.getR2007CrcMismatch() > 0u);
    const auto diagnostics = reader.getIntegrityDiagnostics();
    REQUIRE_FALSE(diagnostics.empty());
    CHECK(std::all_of(diagnostics.cbegin(), diagnostics.cend(),
                      [](const DwgIntegrityDiagnostic& diagnostic) {
                          return diagnostic.version == DRW::AC1021
                              && diagnostic.severity
                                  == DwgIntegritySeverity::Warning;
                      }));
}

TEST_CASE("DWG R2004 page-map gaps are not resolvable pages",
          "[dwg][r2004][safety]") {
    std::vector<std::uint8_t> map;
    const auto append32 = [&map](std::uint32_t value) {
        const std::size_t offset = map.size();
        map.resize(offset + sizeof(value));
        writeLittleEndian32(map.data() + offset, value);
    };
    const auto appendEntry = [&append32](std::int32_t id,
                                         std::uint32_t pageSize) {
        append32(static_cast<std::uint32_t>(id));
        append32(pageSize);
    };

    appendEntry(1, 0x20);
    appendEntry(-1, 0x10);
    append32(0);
    append32(0);
    append32(0);
    append32(0);
    appendEntry(2, 0x30);

    std::unordered_map<std::uint32_t, dwgPageInfo> pages;
    REQUIRE(DwgDataPageReader18Probe::parseSectionPageMap(
        map.data(), map.size(), 0x100, pages, 0x200));
    REQUIRE(pages.size() == 2);
    REQUIRE(pages.find(1) != pages.end());
    REQUIRE(pages.find(2) != pages.end());
    CHECK(pages.at(1).address == 0x100);
    CHECK(pages.at(2).address == 0x130);
    CHECK(pages.find(std::numeric_limits<std::uint32_t>::max()) ==
          pages.end());

    std::unordered_map<std::uint32_t, dwgPageInfo> outOfRange;
    outOfRange.emplace(6, dwgPageInfo(6, 0x666, 0x20));
    CHECK_FALSE(DwgDataPageReader18Probe::parseSectionPageMap(
        map.data(), map.size(), 0x100, outOfRange, 0x150));
    REQUIRE(outOfRange.size() == 1);
    CHECK(outOfRange.at(6).address == 0x666);

    auto nonZeroGapMetadata = map;
    constexpr std::size_t gapTerminatorWord = 7;
    writeLittleEndian32(nonZeroGapMetadata.data()
                            + gapTerminatorWord * sizeof(std::uint32_t),
                        1);
    std::unordered_map<std::uint32_t, dwgPageInfo> nonZeroGapPages;
    REQUIRE(DwgDataPageReader18Probe::parseSectionPageMap(
        nonZeroGapMetadata.data(), nonZeroGapMetadata.size(), 0x100,
        nonZeroGapPages, 0x200));
    REQUIRE(nonZeroGapPages.size() == 2);
    CHECK(nonZeroGapPages.at(2).address == 0x130);

    std::vector<std::uint8_t> truncated;
    const auto appendTruncated32 = [&truncated](std::uint32_t value) {
        const std::size_t offset = truncated.size();
        truncated.resize(offset + sizeof(value));
        writeLittleEndian32(truncated.data() + offset, value);
    };
    appendTruncated32(static_cast<std::uint32_t>(-2));
    appendTruncated32(0x10);

    std::unordered_map<std::uint32_t, dwgPageInfo> preserved;
    preserved.emplace(7, dwgPageInfo(7, 0x777, 0x20));
    CHECK_FALSE(DwgDataPageReader18Probe::parseSectionPageMap(
        truncated.data(), truncated.size(), 0x100, preserved));
    REQUIRE(preserved.size() == 1);
    CHECK(preserved.at(7).address == 0x777);
}

TEST_CASE("DWG R2004 page-map rejects duplicate page IDs transactionally",
          "[dwg][r2004][safety]") {
    std::vector<std::uint8_t> map;
    const auto append32 = [&map](std::uint32_t value) {
        const std::size_t offset = map.size();
        map.resize(offset + sizeof(value));
        writeLittleEndian32(map.data() + offset, value);
    };

    append32(4);
    append32(0x20);
    append32(4);
    append32(0x40);

    std::unordered_map<std::uint32_t, dwgPageInfo> pages;
    pages.emplace(9, dwgPageInfo(9, 0x900, 0x10));
    CHECK_FALSE(DwgDataPageReader18Probe::parseSectionPageMap(
        map.data(), map.size(), 0x100, pages));
    REQUIRE(pages.size() == 1);
    CHECK(pages.at(9).address == 0x900);
    CHECK(pages.at(9).size == 0x10);
}

TEST_CASE("DWG R2007 page-map gaps are not resolvable pages",
          "[dwg][r2007][safety]") {
    std::vector<std::uint8_t> map;
    const auto append64 = [&map](std::uint64_t value) {
        const std::size_t offset = map.size();
        map.resize(offset + sizeof(value));
        writeLittleEndian64(map.data() + offset, value);
    };
    const auto appendEntry = [&append64](std::uint64_t pageSize,
                                         std::uint64_t id) {
        append64(pageSize);
        append64(id);
    };

    appendEntry(0x20, 1);
    appendEntry(0x10, static_cast<std::uint64_t>(-2));
    appendEntry(0x30, 3);

    std::unordered_map<std::uint64_t, dwgPageInfo> pages;
    REQUIRE(DwgDataPageReaderProbe::parseSectionPageMap(
        map.data(), map.size(), 3, 3, 0x1000, pages));
    REQUIRE(pages.size() == 2);
    REQUIRE(pages.find(1) != pages.end());
    REQUIRE(pages.find(3) != pages.end());
    CHECK(pages.at(1).address == 0x480);
    CHECK(pages.at(3).address == 0x4B0);
    CHECK(pages.find(2) == pages.end());

    std::unordered_map<std::uint64_t, dwgPageInfo> preserved;
    preserved.emplace(7, dwgPageInfo(7, 0x777, 0x20));
    CHECK_FALSE(DwgDataPageReaderProbe::parseSectionPageMap(
        map.data(), map.size(), 2, 3, 0x1000, preserved));
    REQUIRE(preserved.size() == 1);
    CHECK(preserved.at(7).address == 0x777);
}

TEST_CASE("DWG R2007 page-map accepts only zero padding after pairs",
          "[dwg][r2007][safety]") {
    const auto append64 = [](std::vector<std::uint8_t>& map,
                             std::uint64_t value) {
        const std::size_t offset = map.size();
        map.resize(offset + sizeof(value));
        writeLittleEndian64(map.data() + offset, value);
    };

    std::vector<std::uint8_t> padded;
    append64(padded, 0x20);
    append64(padded, 1);
    padded.resize(padded.size() + 7, 0);

    std::unordered_map<std::uint64_t, dwgPageInfo> pages;
    REQUIRE(DwgDataPageReaderProbe::parseSectionPageMap(
        padded.data(), padded.size(), 1, 1, 0x1000, pages));
    REQUIRE(pages.size() == 1);
    CHECK(pages.at(1).address == 0x480);
    CHECK(pages.at(1).size == 0x20);

    auto nonZeroTail = padded;
    nonZeroTail.back() = 1;
    pages.emplace(7, dwgPageInfo(7, 0x777, 0x20));
    CHECK_FALSE(DwgDataPageReaderProbe::parseSectionPageMap(
        nonZeroTail.data(), nonZeroTail.size(), 1, 1, 0x1000, pages));
    REQUIRE(pages.size() == 2);
    CHECK(pages.at(7).address == 0x777);

    std::vector<std::uint8_t> zeroPage;
    append64(zeroPage, 0);
    append64(zeroPage, 1);
    pages.clear();
    CHECK_FALSE(DwgDataPageReaderProbe::parseSectionPageMap(
        zeroPage.data(), zeroPage.size(), 1, 1, 0x1000, pages));
    CHECK(pages.empty());
}

TEST_CASE("DWG R2007 page-map rejects pages past the file boundary",
          "[dwg][r2007][safety]") {
    std::vector<std::uint8_t> map;
    const auto append64 = [&map](std::uint64_t value) {
        const std::size_t offset = map.size();
        map.resize(offset + sizeof(value));
        writeLittleEndian64(map.data() + offset, value);
    };
    append64(0x20);
    append64(1);

    std::unordered_map<std::uint64_t, dwgPageInfo> pages;
    pages.emplace(7, dwgPageInfo(7, 0x777, 0x20));
    DwgDataPageReaderProbe::PageMapFailure failure =
        DwgDataPageReaderProbe::PageMapFailure::None;
    CHECK_FALSE(DwgDataPageReaderProbe::parseSectionPageMap(
        map.data(), map.size(), 1, 1, 0x490, pages, &failure));
    CHECK(failure == DwgDataPageReaderProbe::PageMapFailure::PageRange);
    REQUIRE(pages.size() == 1);
    CHECK(pages.at(7).address == 0x777);
}

TEST_CASE("DWG R2004 file-header CRC mismatch is surfaced",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("xline/constructionline_2004.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("constructionline_2004.dwg fixture absent; skipping");
    }
    auto bytes = readFile(source);
    REQUIRE(bytes.size() > 0x80 + 36);
    const auto original = bytes;

    // Change the encrypted byte for the non-address header field at 0x24.
    // Section-map addresses and page checksums remain untouched, isolating
    // the file-header CRC diagnostic from structural parsing.
    bytes[0x80 + 36] ^= 0x01;

    DwgReadProbe interface;
    dwgRW reader(source.string().c_str());
    REQUIRE(reader.readBuffer(bytes.data(), bytes.size(), &interface, true));
    CHECK(reader.getR2004CrcMismatch() == 1u);
    const auto diagnostics = reader.getIntegrityDiagnostics();
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics.front().kind == DwgIntegrityCheckKind::FileHeaderCrc);
    CHECK(diagnostics.front().version == DRW::AC1018);
    CHECK(diagnostics.front().offsetSpace
          == DwgIntegrityAddressSpace::PhysicalFile);
    CHECK(diagnostics.front().hasFileOffset);
    CHECK(diagnostics.front().fileOffset == 0x80);

    DwgReadProbe cleanInterface;
    REQUIRE(reader.readBuffer(original.data(), original.size(),
                              &cleanInterface, true));
    CHECK(reader.getIntegrityDiagnostics().empty());
}

TEST_CASE("DWG R2004 file-header tail magic is required",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("xline/constructionline_2004.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("constructionline_2004.dwg fixture absent; skipping");
    }
    auto bytes = readFile(source);
    REQUIRE(bytes.size() > 0xEC);
    bytes[0xEC] ^= 0x01;

    DwgReadProbe interface;
    dwgRW reader(source.string().c_str());
    CHECK_FALSE(reader.readBuffer(bytes.data(), bytes.size(), &interface, true));
}

TEST_CASE("DWG R2004 file-header constants are required",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("xline/constructionline_2004.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("constructionline_2004.dwg fixture absent; skipping");
    }
    auto bytes = readFile(source);
    REQUIRE(bytes.size() > 0x80 + 20);

    // The encrypted byte at 0x90 decodes the documented 0x6c header-size
    // field at offset 0x10. The page map remains untouched.
    bytes[0x80 + 16] ^= 0x01;

    DwgReadProbe interface;
    dwgRW reader(source.string().c_str());
    CHECK_FALSE(reader.readBuffer(bytes.data(), bytes.size(), &interface, true));
}

TEST_CASE("DWG optional version fixtures reject truncated containers",
          "[dwg][versions][malformed][fixture]") {
    struct Fixture {
        const char *file;
        DRW::Version version;
    };
    const Fixture fixtures[] = {
        {"xline/constructionline_2000.dwg", DRW::AC1015},
        {"xline/constructionline_2004.dwg", DRW::AC1018},
        {"visualstyle_r2007.dwg", DRW::AC1021},
        {"xline/constructionline_2010.dwg", DRW::AC1024},
        {"xline/constructionline_2013.dwg", DRW::AC1027},
        {"xline/constructionline_2018.dwg", DRW::AC1032},
    };

    bool testedFixture = false;
    for (const Fixture &fixture : fixtures) {
        const auto source = localFixture(fixture.file);
        if (!std::filesystem::is_regular_file(source))
            continue;
        testedFixture = true;
        INFO("fixture: " << fixture.file);
        auto bytes = readFile(source);
        REQUIRE(bytes.size() > 12);
        // A one-byte tail trim can remove only padding or a redundant footer;
        // cutting the committed container in half necessarily removes a
        // required section/page while retaining a valid version prefix.
        bytes.resize(bytes.size() / 2);

        DwgReadProbe interface;
        dwgRW reader(source.string().c_str());
        CHECK_FALSE(reader.readBuffer(bytes.data(), bytes.size(), &interface,
                                      /*ext=*/true));
        CHECK(reader.getVersion() == fixture.version);
    }
    if (!testedFixture)
        SKIP("source-controlled DWG fixtures absent; skipping");
}

TEST_CASE("DWG CLASSES CRC covers the section payload", "[dwg][safety]") {
    dwgBufferW writer;
    writer.putBytes(std::vector<std::uint8_t>(16, 0xA5).data(), 16);
    writer.putRawLong32(12);
    writer.putBytes(std::vector<std::uint8_t>{
        0x10, 0x20, 0x30, 0x40, 0x50, 0x60,
        0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0}.data(), 12);

    dwgBuffer reader(writer.data().data(), writer.data().size());
    const auto before = reader.getPosition();
    const auto crc = reader.crc8(
        0xC0C1, 16, static_cast<std::int32_t>(writer.data().size()));
    CHECK(crc == writer.crc16(0xC0C1, 16, writer.data().size()));
    CHECK(reader.getPosition() == before);
    CHECK(crc != reader.crc8(
        0xC0C1, 16, static_cast<std::int32_t>(writer.data().size() - 1)));
}

TEST_CASE("DWG R2004 CLASSES rejects an end sentinel outside the section",
          "[dwg][safety]") {
    dwgBufferW payload;
    payload.putBytes(dwgSentinels::CLASSES_BEGIN, 16);
    payload.putRawLong32(0);
    payload.putBitShort(499);
    payload.putRawChar8(0);
    payload.putRawChar8(0);
    payload.putBit(0);
    payload.alignToByte();
    const auto crcPosition = payload.data().size();
    dwgBuffer crcBuffer(payload.data().data(), payload.data().size());
    payload.putRawShort16(crcBuffer.crc8(
        0xC0C1, 16, static_cast<std::int32_t>(crcPosition)));
    payload.putBytes(dwgSentinels::CLASSES_END, 16);

    auto page = makeAc18DataPage(0, 0, payload.data());
    DwgClassesReader18Probe reader(std::make_unique<dwgBuffer>(
        page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setClassesPage(payload.data().size() - 16);
    reader.setDecodedStateForTest({0xA5, 0x5A});
    CHECK_FALSE(reader.readDwgClasses());
    CHECK(reader.decodedStateForTest().empty());
}

TEST_CASE("DWG R2007 CLASSES stays inside its declared payload",
          "[dwg][safety]") {
    const auto validSection = makeAc21ClassesSection();
    const auto validPage = makeAc18DataPage(0, 0, validSection);
    DwgClassesReader21Probe validReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(validPage.data()), validPage.size()));
    validReader.setVersionForTest(DRW::AC1021);
    validReader.setClassesPage(validSection.size());
    REQUIRE(validReader.readDwgClasses());
    CHECK(validReader.classCountForTest() == 1);
    const DRW_Class* parsedClass = validReader.classForTest(500);
    REQUIRE(parsedClass != nullptr);
    CHECK(parsedClass->proxyFlag == 0x1234);
    CHECK(parsedClass->appName == "TEST_APP");
    CHECK(parsedClass->wasaProxyFlag == 1);
    CHECK(parsedClass->entityFlag == 1);
    CHECK(parsedClass->entityFlagRaw == 0x1F2);
    CHECK(parsedClass->instanceCount == 7);
    CHECK(parsedClass->dwgVersion == 1027);
    CHECK(parsedClass->maintenanceVersion == 329);
    CHECK(parsedClass->unknown1 == 17);
    CHECK(parsedClass->unknown2 == 23);
    const auto& validCoverage = validReader.classCoverageReportForTest();
    REQUIRE(validCoverage.m_status == DRW_DwgClassCoverageStatus::InProgress);
    REQUIRE(validCoverage.m_entries.size() == 1);
    const auto& entry = validCoverage.m_entries.front();
    CHECK(entry.m_streamOrdinal == 0);
    CHECK(entry.m_classNumber == 500);
    CHECK(entry.m_state == DRW_DwgClassCoverageState::Published);
    CHECK(entry.m_reason == DRW_DwgClassCoverageReason::None);
    CHECK(entry.m_dataRange.m_present);
    CHECK(entry.m_stringRange.m_present);
    CHECK(entry.m_dataRange.m_offsetSpace
          == DRW_DwgFrameOffsetSpace::DecodedBuffer);
    CHECK(entry.m_stringRange.m_offsetSpace
          == DRW_DwgFrameOffsetSpace::DecodedBuffer);
    CHECK(entry.m_dataRange.m_sectionRelative);
    CHECK(entry.m_stringRange.m_sectionRelative);
    CHECK(entry.m_dataRange.m_startBit != entry.m_stringRange.m_startBit);

    auto truncatedSection = validSection;
    const auto declaredSize = readLittleEndian32(truncatedSection.data() + 16);
    REQUIRE(declaredSize > 1);
    writeLittleEndian32(truncatedSection.data() + 16, declaredSize - 1);
    const auto truncatedPage = makeAc18DataPage(0, 0, truncatedSection);
    DwgClassesReader21Probe truncatedReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(truncatedPage.data()), truncatedPage.size()));
    truncatedReader.setVersionForTest(DRW::AC1021);
    truncatedReader.setClassesPage(truncatedSection.size());
    CHECK_FALSE(truncatedReader.readDwgClasses());
    CHECK(truncatedReader.classCountForTest() == 0);
}

TEST_CASE("DWG R2004 CLASSES records shared decoded provenance",
          "[dwg][safety][classes]") {
    const auto validSection = makeAc18ClassesSection();
    const auto validPage = makeAc18DataPage(0, 0, validSection);
    DwgClassesReader18Probe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(validPage.data()), validPage.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setClassesPage(validSection.size());

    REQUIRE(reader.readDwgClasses());
    const auto& report = reader.classCoverageReportForTest();
    REQUIRE(report.m_entries.size() == 1);
    const auto& entry = report.m_entries.front();
    CHECK(entry.m_state == DRW_DwgClassCoverageState::Published);
    CHECK(entry.m_dataRange.m_present);
    CHECK(entry.m_stringRange.m_present);
    CHECK(entry.m_dataRange.m_offsetSpace
          == DRW_DwgFrameOffsetSpace::DecodedBuffer);
    CHECK(entry.m_dataRange.m_sectionRelative);
    CHECK(entry.m_dataRange.m_startBit == entry.m_stringRange.m_startBit);
    CHECK(entry.m_dataRange.m_endBit == entry.m_stringRange.m_endBit);
}

TEST_CASE("DWG R2000 CLASSES publication is transactional",
          "[dwg][safety]") {
    const auto emptySection = makeLegacyClassesSection(false, false);
    DwgClassesReader15Probe emptyReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(emptySection.data()), emptySection.size()));
    emptyReader.setVersionForTest(DRW::AC1015);
    emptyReader.setClassesPage(emptySection.size());
    REQUIRE(emptyReader.readDwgClasses());
    CHECK(emptyReader.classCountForTest() == 0);

    const auto validSection = makeLegacyClassesSection(false);
    DwgClassesReader15Probe validReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(validSection.data()), validSection.size()));
    validReader.setVersionForTest(DRW::AC1015);
    validReader.setClassesPage(validSection.size());
    REQUIRE(validReader.readDwgClasses());
    CHECK(validReader.classCountForTest() == 1);
    const auto& validCoverage = validReader.classCoverageReportForTest();
    REQUIRE(validCoverage.m_entries.size() == 1);
    const auto& validEntry = validCoverage.m_entries.front();
    CHECK(validEntry.m_state == DRW_DwgClassCoverageState::Published);
    CHECK(validEntry.m_dataRange.m_present);
    CHECK(validEntry.m_stringRange.m_present);
    CHECK(validEntry.m_dataRange.m_offsetSpace
          == DRW_DwgFrameOffsetSpace::PhysicalFile);
    CHECK_FALSE(validEntry.m_dataRange.m_sectionRelative);
    CHECK(validEntry.m_dataRange.m_startBit
          == validEntry.m_stringRange.m_startBit);
    CHECK(validEntry.m_dataRange.m_endBit
          == validEntry.m_stringRange.m_endBit);

    const auto malformedSection = makeLegacyClassesSection(true);
    DwgClassesReader15Probe malformedReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(malformedSection.data()),
        malformedSection.size()));
    malformedReader.setVersionForTest(DRW::AC1015);
    malformedReader.setClassesPage(malformedSection.size());
    CHECK_FALSE(malformedReader.readDwgClasses());
    CHECK(malformedReader.classCountForTest() == 0);

    DwgClassesReader15Probe shortReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(emptySection.data()), emptySection.size()));
    shortReader.setVersionForTest(DRW::AC1015);
    shortReader.setClassesPage(16);
    CHECK_FALSE(shortReader.readDwgClasses());
    CHECK(shortReader.classCountForTest() == 0);
}

TEST_CASE("DWG CLASSES keeps a parsed prefix out of dispatch after a duplicate",
          "[dwg][safety][classes]") {
    const auto section = makeLegacyClassesSectionWithNumbers(
        false, {500, 500});
    DwgClassesReader15Probe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(section.data()), section.size()));
    reader.setVersionForTest(DRW::AC1015);
    reader.setClassesPage(section.size());

    CHECK_FALSE(reader.readDwgClasses());
    CHECK(reader.classCountForTest() == 0);
    const auto& report = reader.classCoverageReportForTest();
    REQUIRE(report.m_entries.size() == 2);
    CHECK(report.m_entries[0].m_state == DRW_DwgClassCoverageState::Parsed);
    CHECK(report.m_entries[0].m_reason == DRW_DwgClassCoverageReason::None);
    CHECK(report.m_entries[1].m_state == DRW_DwgClassCoverageState::Failed);
    CHECK(report.m_entries[1].m_reason
          == DRW_DwgClassCoverageReason::Duplicate);

    DwgReadProbe interface;
    reader.finalizeClassCoverageForTest(interface, false);
    REQUIRE(interface.classCoverageReports.size() == 1);
    CHECK(report.m_status == DRW_DwgClassCoverageStatus::FinalizedPartial);
    CHECK_FALSE(report.m_complete);
}

TEST_CASE("DWG CLASSES report finalization is independent of DXF classes",
          "[dwg][safety][classes][fixture]") {
    const auto source = localFixture("visualstyle_r2007.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("visualstyle_r2007.dwg fixture absent; skipping");
    }
    DwgReadProbe interface;
    dwgRW reader(source.string().c_str());
    REQUIRE(reader.read(&interface, true));
    REQUIRE(interface.classCoverageReports.size() == 1);
    const auto& report = interface.classCoverageReports.front();
    CHECK(report.m_status == DRW_DwgClassCoverageStatus::FinalizedComplete);
    CHECK(report.m_complete);
    CHECK_FALSE(report.m_entries.empty());
    CHECK(interface.dxfClassCount == 0);
    const auto customReceipt = std::find_if(
        interface.framePublications.cbegin(),
        interface.framePublications.cend(),
        [](const DRW_DwgFramePublication& publication) {
            return publication.m_isCustomClass
                && publication.m_classStreamOrdinal.has_value();
        });
    REQUIRE(customReceipt != interface.framePublications.cend());
    REQUIRE(*customReceipt->m_classStreamOrdinal < report.m_entries.size());
    CHECK(report.m_entries[*customReceipt->m_classStreamOrdinal].m_classNumber
          == customReceipt->m_encodedType);
}

TEST_CASE("DWG CLASSES report callback failure remains internally partial",
          "[dwg][safety][classes]") {
    const auto section = makeLegacyClassesSection(false);
    DwgClassesReader15Probe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(section.data()), section.size()));
    reader.setVersionForTest(DRW::AC1015);
    reader.setClassesPage(section.size());
    REQUIRE(reader.readDwgClasses());

    DwgThrowingClassCoverageProbe interface;
    reader.finalizeClassCoverageForTest(interface, true);
    CHECK(interface.attempts == 1);
    const auto& report = reader.classCoverageReportForTest();
    CHECK(report.m_status == DRW_DwgClassCoverageStatus::FinalizedPartial);
    CHECK_FALSE(report.m_complete);
    REQUIRE(report.m_entries.size() == 1);
    CHECK(report.m_entries.front().m_state
          == DRW_DwgClassCoverageState::Published);
    CHECK(report.m_entries.front().m_reason
          == DRW_DwgClassCoverageReason::Callback);
}

TEST_CASE("DWG process keeps a class-report callback failure best effort",
          "[dwg][safety][classes][fixture]") {
    const auto source = localFixture("visualstyle_r2007.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("visualstyle_r2007.dwg fixture absent; skipping");
    }
    DwgThrowingClassCoverageProbe interface;
    dwgRW reader(source.string().c_str());
    REQUIRE(reader.read(&interface, true));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(interface.attempts == 1);
}

TEST_CASE("DWG R2000 file header bounds its section record count",
          "[dwg][safety]") {
    std::vector<std::uint8_t> bytes(25, 0);
    bytes[21] = 0xFF;
    bytes[22] = 0xFF;
    bytes[23] = 0xFF;
    bytes[24] = 0xFF;
    DwgClassesReader15Probe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));

    CHECK_FALSE(reader.readFileHeader());
}

TEST_CASE("DWG metadata rejects truncated fixed fields before file-header parsing",
          "[dwg][metadata][malformed]") {
    const char *versions[] = {"AC1015", "AC1018", "AC1021"};
    for (const char *version : versions) {
        INFO("version: " << version);
        std::vector<std::uint8_t> bytes(13, 0);
        std::copy(version, version + 6, bytes.begin());

        DwgReadProbe interface;
        dwgRW reader("metadata-truncated.dwg");
        CHECK_FALSE(reader.readBuffer(bytes.data(), bytes.size(),
                                      &interface, true));
        CHECK(reader.getError() == DRW::BAD_READ_METADATA);
    }
}

TEST_CASE("DWG R2000 rejects unsupported section locator counts",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("xline/constructionline_2000.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("constructionline_2000.dwg fixture absent; skipping");
    }
    auto bytes = readFile(source);
    REQUIRE(bytes.size() > 80u);

    bytes[21] = 7;
    DwgClassesReader15Probe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));

    CHECK_FALSE(reader.readFileHeader());
    CHECK(reader.sectionCountForTest() == 0u);
}

TEST_CASE("DWG R2000 rejects a corrupt file-header sentinel",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("xline/constructionline_2000.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("constructionline_2000.dwg fixture absent; skipping");
    }
    auto bytes = readFile(source);
    constexpr std::size_t locatorOffset = 21u;
    constexpr std::size_t locatorCountSize = 4u;
    constexpr std::size_t locatorRecordSize = 9u;
    constexpr std::size_t crcSize = 2u;
    REQUIRE(bytes.size() >= locatorOffset + locatorCountSize);
    const std::size_t locatorRecordCount =
        readLittleEndian32(bytes.data() + locatorOffset);
    const std::size_t sentinelOffset = locatorOffset + locatorCountSize
        + locatorRecordCount * locatorRecordSize + crcSize;
    REQUIRE(bytes.size() > sentinelOffset);

    bytes[sentinelOffset] ^= 0x01u;
    DwgClassesReader15Probe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));

    CHECK_FALSE(reader.readFileHeader());
    CHECK(reader.sectionCountForTest() == 0u);
}

TEST_CASE("DWG R2000 rejects a corrupt section locator CRC",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("xline/constructionline_2000.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("constructionline_2000.dwg fixture absent; skipping");
    }
    auto bytes = readFile(source);
    constexpr std::size_t locatorOffset = 21u;
    constexpr std::size_t locatorCountSize = 4u;
    constexpr std::size_t locatorRecordSize = 9u;
    REQUIRE(bytes.size() >= locatorOffset + locatorCountSize);
    const std::size_t locatorRecordCount =
        readLittleEndian32(bytes.data() + locatorOffset);
    const std::size_t crcOffset = locatorOffset + locatorCountSize
        + locatorRecordCount * locatorRecordSize;
    REQUIRE(bytes.size() >= crcOffset + sizeof(std::uint16_t));

    bytes[crcOffset] ^= 0x01u;
    DwgClassesReader15Probe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));

    CHECK_FALSE(reader.readFileHeader());
    CHECK(reader.sectionCountForTest() == 0u);
}

TEST_CASE("DWG R2000 section locators publish transactionally",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("xline/constructionline_2000.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("constructionline_2000.dwg fixture absent; skipping");
    }
    const auto original = readFile(source);
    REQUIRE(original.size() > 80u);

    auto run = [](std::vector<std::uint8_t> bytes) {
        DwgClassesReader15Probe reader(std::make_unique<dwgBuffer>(
            bytes.data(), bytes.size()));
        reader.seedSectionForTest();
        const bool ok = reader.readFileHeader();
        return std::pair<bool, std::size_t>{ok, reader.sectionCountForTest()};
    };

    const auto valid = run(original);
    CHECK(valid.first);
    CHECK(valid.second == 6u);

    auto outOfBounds = original;
    outOfBounds[26] = 0xff;
    outOfBounds[27] = 0xff;
    outOfBounds[28] = 0xff;
    outOfBounds[29] = 0xff;
    const auto invalidRange = run(std::move(outOfBounds));
    CHECK_FALSE(invalidRange.first);
    CHECK(invalidRange.second == 0u);

    auto duplicate = original;
    duplicate[25u + 5u * 9u] = 0;
    const auto duplicateId = run(std::move(duplicate));
    CHECK_FALSE(duplicateId.first);
    CHECK(duplicateId.second == 0u);
}

TEST_CASE("DWG modern header retry clears DataStorage link state",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};

    DwgDataStorageResetReader18Probe reader18(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    reader18.seedLinkedRecordForTest();
    reader18.seedSectionForTest();
    CHECK(reader18.linkedRecordCountForTest() == 1u);
    CHECK(reader18.sectionCountForTest() == 1u);
    CHECK_FALSE(reader18.readFileHeader());
    CHECK(reader18.linkedRecordCountForTest() == 0u);
    CHECK(reader18.sectionCountForTest() == 0u);

    DwgDataStorageResetReader21Probe reader21(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    reader21.seedLinkedRecordForTest();
    reader21.seedSectionForTest();
    CHECK(reader21.linkedRecordCountForTest() == 1u);
    CHECK(reader21.sectionCountForTest() == 1u);
    CHECK_FALSE(reader21.readFileHeader());
    CHECK(reader21.linkedRecordCountForTest() == 0u);
    CHECK(reader21.sectionCountForTest() == 0u);
}

TEST_CASE("DWG R2004+ captures the prototype section without VBA",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    SECTION("R2004") {
        DwgDataPageReader18Probe reader(std::make_unique<dwgBuffer>(
            dummy, sizeof(dummy)));
        reader.setVersionForTest(DRW::AC1018);
        reader.setEmptyPrototypeSectionForTest();

        REQUIRE(reader.captureRawDwgDataSections());
        CHECK(reader.rawSectionCountForTest() == 1u);
        CHECK(reader.dataStorageSectionCountForTest() == 1u);
    }

    SECTION("R2007") {
        DwgDataPageReaderProbe reader(std::make_unique<dwgBuffer>(
            dummy, sizeof(dummy)));
        reader.setVersionForTest(DRW::AC1021);
        reader.setEmptyPrototypeSectionForTest();

        REQUIRE(reader.captureRawDwgDataSections());
        CHECK(reader.rawSectionCountForTest() == 1u);
        CHECK(reader.dataStorageSectionCountForTest() == 1u);
    }
}

TEST_CASE("DWG R2007 object phases require decoded OBJECTS data",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgDataPageReaderProbe reader(std::make_unique<dwgBuffer>(
        dummy, sizeof(dummy)));
    reader.setMalformedObjectsSectionForTest();

    DRW_Header header;
    DwgReadProbe interface;
    CHECK_FALSE(reader.readDwgTables(header));
    CHECK_FALSE(reader.readDwgBlocks(interface));
    CHECK_FALSE(reader.readDwgEntities(interface));
    CHECK_FALSE(reader.readDwgObjects(interface));
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG R2004 object phases require decoded OBJECTS data",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgDataPageReader18Probe reader(std::make_unique<dwgBuffer>(
        dummy, sizeof(dummy)));
    reader.setMalformedObjectsSectionForTest();

    DRW_Header header;
    DwgReadProbe interface;
    CHECK_FALSE(reader.readDwgTables(header));
    CHECK_FALSE(reader.readDwgBlocks(interface));
    CHECK_FALSE(reader.readDwgEntities(interface));
    CHECK_FALSE(reader.readDwgObjects(interface));
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG R2004 phase lookup does not insert missing sections",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgDataPageReader18Probe reader(std::make_unique<dwgBuffer>(
        dummy, sizeof(dummy)));
    DRW_Header header;

    CHECK(reader.sectionCountForTest() == 0u);
    CHECK_FALSE(reader.readDwgHeader(header));
    CHECK_FALSE(reader.readDwgClasses());
    CHECK_FALSE(reader.readDwgHandles());
    CHECK_FALSE(reader.readDwgTables(header));
    CHECK(reader.sectionCountForTest() == 0u);
}

TEST_CASE("DWG R2004 missing phase clears decoded section state",
          "[dwg][safety]") {
    const std::vector<std::uint8_t> previous{0xA5};
    DRW_Header header;

    const auto verify = [&](auto invoke) {
        std::uint8_t dummy[] = {0};
        DwgDataPageReader18Probe reader(std::make_unique<dwgBuffer>(
            dummy, sizeof(dummy)));
        reader.setDecodedStateForTest(previous);
        REQUIRE(reader.decodedStateForTest() == previous);
        CHECK_FALSE(invoke(reader));
        CHECK(reader.decodedStateForTest().empty());
    };

    verify([&](DwgDataPageReader18Probe& reader) {
        return reader.readDwgHeader(header);
    });
    verify([](DwgDataPageReader18Probe& reader) {
        return reader.readDwgClasses();
    });
    verify([](DwgDataPageReader18Probe& reader) {
        return reader.readDwgHandles();
    });
}

TEST_CASE("DWG R2000 phase lookup does not insert missing sections",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgSectionLookupReader15Probe reader(std::make_unique<dwgBuffer>(
        dummy, sizeof(dummy)));
    DRW_Header header;

    CHECK(reader.sectionCountForTest() == 0u);
    CHECK_FALSE(reader.readDwgHeader(header));
    CHECK_FALSE(reader.readDwgClasses());
    CHECK_FALSE(reader.readDwgHandles());
    CHECK(reader.sectionCountForTest() == 0u);
}

TEST_CASE("DWG R2000 HEADER rejects oversized direct section descriptors",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgSectionLookupReader15Probe reader(std::make_unique<dwgBuffer>(
        dummy, sizeof(dummy)));
    reader.setHeaderSectionForTest(dwgSafety::MaxBufferSize + 1u);

    DRW_Header header;
    CHECK_FALSE(reader.readDwgHeader(header));
}

TEST_CASE("DWG AC1018 CLASSES CRC mismatch is reported after parsing",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("xline/constructionline_2004.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("constructionline_2004.dwg fixture absent; skipping");
    }
    const auto original = readFile(source);
    REQUIRE(!original.empty());

    const auto begin = std::search(
        original.cbegin(), original.cend(), std::cbegin(dwgSentinels::CLASSES_BEGIN),
        std::cend(dwgSentinels::CLASSES_BEGIN));
    REQUIRE(begin != original.cend());

    const auto end = std::search(
        begin + 16, original.cend(), std::cbegin(dwgSentinels::CLASSES_END),
        std::cend(dwgSentinels::CLASSES_END));
    REQUIRE(end != original.cend());
    REQUIRE(end - original.cbegin() >= 2);
    const auto crcPosition = static_cast<std::size_t>(end - original.cbegin()) - 2;

    DwgReadProbe pristineInterface;
    dwgRW pristine(source.string().c_str());
    REQUIRE(pristine.read(&pristineInterface, true));
    const auto baseline = pristine.getClassesCrcMismatch();

    auto corrupted = original;
    corrupted[crcPosition] ^= 0xFF;
    REQUIRE(refreshAc1018PageChecksum(corrupted, crcPosition));
    const auto temporary = std::filesystem::temp_directory_path() /
                           "librecad-ac1018-classes-crc.dwg";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        REQUIRE(stream.good());
        stream.write(reinterpret_cast<const char*>(corrupted.data()),
                     static_cast<std::streamsize>(corrupted.size()));
        REQUIRE(stream.good());
    }

    DwgReadProbe corruptedInterface;
    dwgRW reader(temporary.string().c_str());
    REQUIRE(reader.read(&corruptedInterface, true));
    CHECK(reader.getClassesCrcMismatch() == baseline + 1);
    REQUIRE(corruptedInterface.classCoverageReports.size() == 1);
    CHECK(corruptedInterface.classCoverageReports.front().m_status
          == DRW_DwgClassCoverageStatus::FinalizedComplete);
    CHECK(corruptedInterface.classCoverageReports.front().m_complete);
    const auto diagnostics = reader.getIntegrityDiagnostics();
    REQUIRE_FALSE(diagnostics.empty());
    CHECK(std::any_of(diagnostics.cbegin(), diagnostics.cend(),
                      [](const DwgIntegrityDiagnostic& diagnostic) {
                          return diagnostic.kind
                                     == DwgIntegrityCheckKind::ClassesCrc
                              && diagnostic.severity
                                     == DwgIntegritySeverity::Warning
                              && diagnostic.logicalSectionId == secEnum::CLASSES;
                      }));

    std::error_code error;
    std::filesystem::remove(temporary, error);
}

TEST_CASE("DWG INSERT callbacks contain complete ATTRIB sequences",
          "[dwg][safety][fixture]") {
    const std::filesystem::path source =
        std::filesystem::path(LIBRECAD_TEST_DIR) / "dynblock_r2018.dwg";
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("dynamic-block fixture not present; skipping");
    }

    DwgReadProbe interface;
    dwgRW reader(source.string().c_str());
    REQUIRE(reader.read(&interface, true));
    CHECK(reader.getEntityParseFailures() == 0);

    std::size_t attributedInsertCount = 0;
    for (const DRW_Insert& insert : interface.inserts) {
        if (insert.attribHandles.empty())
            continue;
        ++attributedInsertCount;
        REQUIRE(insert.attlist.size() == insert.attribHandles.size());
        for (const dwgHandle& expected : insert.attribHandles) {
            CHECK(std::any_of(
                insert.attlist.cbegin(), insert.attlist.cend(),
                [&](const auto& actual) {
                    return actual && actual->handle == expected.ref;
                }));
        }
    }
    CHECK(attributedInsertCount > 0);
}

TEST_CASE("DWG R2018 POLYLINE trace is bounded by its object-map frame",
          "[dwg][safety][fixture][polyline]") {
    const std::filesystem::path source =
        std::filesystem::path(LIBRECAD_TEST_DIR) / "dynblock_r2018.dwg";
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("dynamic-block fixture not present; skipping");
    }

    auto bytes = readFile(source);
    REQUIRE_FALSE(bytes.empty());
    DwgR2018FixtureReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    REQUIRE(reader.readMetaData());
    REQUIRE(reader.readFileHeader());
    REQUIRE(reader.readDwgHandles());

    constexpr std::uint32_t polylineHandle = 0x3B;
    const std::size_t objectMapSize = reader.objectMapSizeForTest();
    DwgObjectFrame frame;
    DwgPolylineTraceTestAccess::Result trace;
    REQUIRE(reader.tracePolylineForTest(polylineHandle, trace, frame));
    CHECK(reader.objectMapSizeForTest() == objectMapSize);
    CHECK(frame.bodyBitSize() > 0);
    INFO("phase=" << static_cast<int>(trace.phase)
         << " type=" << trace.objectType
         << " bodyEnd=" << trace.bodyEndBit
         << " handleEnd=" << trace.handleEndBit
         << " owned=" << trace.ownedObjectCount
         << " eedSize=" << trace.firstEedDataSize
         << " eedCode=" << static_cast<unsigned int>(trace.firstEedItemCode)
         << " stringBounds=" << trace.stringStreamBoundsValid
         << " commonWithString=" << trace.commonDataWithStringStream
         << " commonWithoutString=" << trace.commonDataWithoutStringStream);
    CHECK(trace.objectType == 0x0F);
    CHECK(trace.phase == DwgPolylineTraceTestAccess::Phase::Complete);
}

TEST_CASE("DWG AC1032 fixture commits direct POLYLINE child sequences",
          "[dwg][safety][fixture][compound]") {
    const std::filesystem::path source =
        std::filesystem::path(LIBRECAD_TEST_DIR) / "dynblock_r2018.dwg";
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("dynamic-block fixture not present; skipping");
    }

    auto bytes = readFile(source);
    REQUIRE_FALSE(bytes.empty());
    DwgR2018FixtureReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    DwgReadProbe interface;
    REQUIRE(reader.readThroughBlocksForTest(interface));
    CHECK(reader.readStageForTest()
          == DwgR2018FixtureReaderProbe::ReadStage::Complete);
    CHECK(reader.entityParseFailuresForTest() == 0u);
    CHECK(reader.entityFailureDiagnosticsForTest().empty());
    CHECK(std::find(interface.polylineHandles.cbegin(),
                    interface.polylineHandles.cend(), 0x3F6u)
          != interface.polylineHandles.cend());
}

TEST_CASE("DWG R2018 common handles preserve block owners and DBCOLOR references",
          "[dwg][safety][handles][dbcolor][fixture]") {
    const std::filesystem::path source =
        std::filesystem::path(LIBRECAD_TEST_DIR) / "dynblock_r2018.dwg";
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("dynamic-block fixture not present; skipping");
    }

    // dwg-parser and LibreDWG decode the R2004+ common handle stream as
    // owner/reactors/xdictionary/DBCOLOR. These two entities carry both
    // references and catch accidental reordering of the optional color H.
    DwgReadProbe interface;
    dwgRW reader(source.string().c_str());
    REQUIRE(reader.read(&interface, true));
    CHECK(reader.getEntityParseFailures() == 0);

    const auto findPolyline = [&](std::uint32_t handle) {
        return std::find_if(
            interface.lwPolylines.cbegin(), interface.lwPolylines.cend(),
            [handle](const DRW_LWPolyline& polyline) {
                return polyline.handle == handle;
            });
    };
    const auto first = findPolyline(0x67A);
    const auto second = findPolyline(0x67F);
    REQUIRE(first != interface.lwPolylines.cend());
    REQUIRE(second != interface.lwPolylines.cend());
    CHECK(first->parentHandle == 0x678);
    CHECK(first->dwgAcDbColorHandle() == 0x154);
    CHECK(second->parentHandle == 0x67D);
    CHECK(second->dwgAcDbColorHandle() == 0x15F);
}

TEST_CASE("DWG source-less POLYLINE leaves owned children untouched",
          "[dwg][safety]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;

    auto validFixture = makePolylineFrameFixture(false);
    REQUIRE(!validFixture.bytes.empty());
    DwgEntityReaderProbe validReader(std::make_unique<dwgBuffer>(
        validFixture.bytes.data(), validFixture.bytes.size()));
    validReader.setVersionForTest(DRW::AC1018);
    validReader.ObjectMap.emplace(
        polylineHandle,
        objHandle(dwgType::POLYLINE_2D, polylineHandle,
                  validFixture.polylineOffset));
    validReader.ObjectMap.emplace(
        vertexHandle,
        objHandle(0x0A, vertexHandle, validFixture.vertexOffset));
    validReader.ObjectMap.emplace(
        seqEndHandle,
        objHandle(dwgType::SEQEND, seqEndHandle, validFixture.seqEndOffset));
    dwgBuffer validBuffer(validFixture.bytes.data(), validFixture.bytes.size());
    DwgReadProbe validInterface;
    objHandle polylineObject(
        dwgType::POLYLINE_2D, polylineHandle, validFixture.polylineOffset);
    CHECK_FALSE(validReader.readDwgEntity(
        &validBuffer, polylineObject, validInterface));
    CHECK(validInterface.polylineCount == 0);
    CHECK(validReader.ObjectMap.size() == 3);
    CHECK(validReader.ObjectMap.find(polylineHandle)
          != validReader.ObjectMap.end());

    auto corruptFixture = makePolylineFrameFixture(true);
    REQUIRE(!corruptFixture.bytes.empty());
    DwgEntityReaderProbe corruptReader(std::make_unique<dwgBuffer>(
        corruptFixture.bytes.data(), corruptFixture.bytes.size()));
    corruptReader.setVersionForTest(DRW::AC1018);
    corruptReader.ObjectMap.emplace(
        polylineHandle,
        objHandle(dwgType::POLYLINE_2D, polylineHandle,
                  corruptFixture.polylineOffset));
    corruptReader.ObjectMap.emplace(
        vertexHandle,
        objHandle(0x0A, vertexHandle, corruptFixture.vertexOffset));
    corruptReader.ObjectMap.emplace(
        seqEndHandle,
        objHandle(dwgType::SEQEND, seqEndHandle, corruptFixture.seqEndOffset));
    dwgBuffer corruptBuffer(
        corruptFixture.bytes.data(), corruptFixture.bytes.size());
    DwgReadProbe corruptInterface;
    objHandle corruptPolyline(
        dwgType::POLYLINE_2D, polylineHandle, corruptFixture.polylineOffset);
    CHECK_FALSE(corruptReader.readDwgEntity(
        &corruptBuffer, corruptPolyline, corruptInterface));
    CHECK(corruptInterface.polylineCount == 0);
    CHECK(corruptReader.ObjectMap.size() == 3);
    CHECK(corruptReader.ObjectMap.find(vertexHandle)
          != corruptReader.ObjectMap.end());
    CHECK(corruptReader.ObjectMap.find(seqEndHandle)
          != corruptReader.ObjectMap.end());
    CHECK(corruptReader.m_quarantinedEntityHandles.empty());

    auto missingFixture = makePolylineFrameFixture(false);
    REQUIRE(!missingFixture.bytes.empty());
    DwgEntityReaderProbe missingReader(std::make_unique<dwgBuffer>(
        missingFixture.bytes.data(), missingFixture.bytes.size()));
    missingReader.setVersionForTest(DRW::AC1018);
    missingReader.ObjectMap.emplace(
        polylineHandle,
        objHandle(dwgType::POLYLINE_2D, polylineHandle,
                  missingFixture.polylineOffset));
    missingReader.ObjectMap.emplace(
        seqEndHandle,
        objHandle(dwgType::SEQEND, seqEndHandle, missingFixture.seqEndOffset));
    dwgBuffer missingBuffer(
        missingFixture.bytes.data(), missingFixture.bytes.size());
    DwgReadProbe missingInterface;
    objHandle missingPolyline(
        dwgType::POLYLINE_2D, polylineHandle, missingFixture.polylineOffset);
    CHECK_FALSE(missingReader.readDwgEntity(
        &missingBuffer, missingPolyline, missingInterface));
    CHECK(missingInterface.polylineCount == 0);
    CHECK(missingReader.ObjectMap.size() == 2);

    auto duplicateFixture = makePolylineFrameFixture(false, true);
    REQUIRE(!duplicateFixture.bytes.empty());
    DwgEntityReaderProbe duplicateReader(std::make_unique<dwgBuffer>(
        duplicateFixture.bytes.data(), duplicateFixture.bytes.size()));
    duplicateReader.setVersionForTest(DRW::AC1018);
    duplicateReader.ObjectMap.emplace(
        polylineHandle,
        objHandle(dwgType::POLYLINE_2D, polylineHandle,
                  duplicateFixture.polylineOffset));
    duplicateReader.ObjectMap.emplace(
        vertexHandle,
        objHandle(0x0A, vertexHandle, duplicateFixture.vertexOffset));
    duplicateReader.ObjectMap.emplace(
        seqEndHandle,
        objHandle(dwgType::SEQEND, seqEndHandle,
                  duplicateFixture.seqEndOffset));
    dwgBuffer duplicateBuffer(
        duplicateFixture.bytes.data(), duplicateFixture.bytes.size());
    DwgReadProbe duplicateInterface;
    objHandle duplicatePolyline(
        dwgType::POLYLINE_2D, polylineHandle,
        duplicateFixture.polylineOffset);
    CHECK_FALSE(duplicateReader.readDwgEntity(
        &duplicateBuffer, duplicatePolyline, duplicateInterface));
    CHECK(duplicateInterface.polylineCount == 0);
    CHECK(duplicateReader.ObjectMap.size() == 3);

    auto wrongOwnerFixture = makePolylineFrameFixture(false, false, true);
    REQUIRE(!wrongOwnerFixture.bytes.empty());
    DwgEntityReaderProbe wrongOwnerReader(std::make_unique<dwgBuffer>(
        wrongOwnerFixture.bytes.data(), wrongOwnerFixture.bytes.size()));
    wrongOwnerReader.setVersionForTest(DRW::AC1018);
    wrongOwnerReader.ObjectMap.emplace(
        polylineHandle,
        objHandle(dwgType::POLYLINE_2D, polylineHandle,
                  wrongOwnerFixture.polylineOffset));
    wrongOwnerReader.ObjectMap.emplace(
        vertexHandle,
        objHandle(0x0A, vertexHandle, wrongOwnerFixture.vertexOffset));
    wrongOwnerReader.ObjectMap.emplace(
        seqEndHandle,
        objHandle(dwgType::SEQEND, seqEndHandle,
                  wrongOwnerFixture.seqEndOffset));
    dwgBuffer wrongOwnerBuffer(
        wrongOwnerFixture.bytes.data(), wrongOwnerFixture.bytes.size());
    DwgReadProbe wrongOwnerInterface;
    objHandle wrongOwnerPolyline(
        dwgType::POLYLINE_2D, polylineHandle,
        wrongOwnerFixture.polylineOffset);
    CHECK_FALSE(wrongOwnerReader.readDwgEntity(
        &wrongOwnerBuffer, wrongOwnerPolyline, wrongOwnerInterface));
    CHECK(wrongOwnerInterface.polylineCount == 0);
    CHECK(wrongOwnerReader.ObjectMap.size() == 3);
}

TEST_CASE("DWG SEQEND routing requires a proven INSERT owner",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t insertHandle = 0x181;
    constexpr std::uint32_t attribHandle = 0x182;
    constexpr std::uint32_t insertSeqEndHandle = 0x183;
    const auto insertFrame = makeInsertWithChildrenFrame(
        insertHandle, DRW::NoHandle, attribHandle, insertSeqEndHandle);
    const auto insertSeqEndFrame = makeSeqEndFrame(
        insertSeqEndHandle, insertHandle);
    REQUIRE(!insertFrame.empty());
    REQUIRE(!insertSeqEndFrame.empty());

    std::vector<std::uint8_t> insertBytes;
    insertBytes.insert(insertBytes.end(), insertFrame.cbegin(), insertFrame.cend());
    const auto insertSeqEndOffset = static_cast<std::uint32_t>(
        insertBytes.size());
    insertBytes.insert(insertBytes.end(), insertSeqEndFrame.cbegin(),
                      insertSeqEndFrame.cend());

    DwgEntityReaderProbe insertReader(std::make_unique<dwgBuffer>(
        insertBytes.data(), insertBytes.size()));
    insertReader.setVersionForTest(DRW::AC1018);
    REQUIRE(insertReader.ObjectMap.emplace(
        insertHandle, objHandle(dwgType::INSERT, insertHandle, 0)).second);
    const objHandle insertSeqEnd(
        dwgType::SEQEND, insertSeqEndHandle, insertSeqEndOffset);
    REQUIRE(insertReader.ObjectMap.emplace(
        insertSeqEndHandle, insertSeqEnd).second);
    dwgBuffer insertBuffer(insertBytes.data(), insertBytes.size());
    CHECK_FALSE(insertReader.requiresLegacyCompoundHandling(
        &insertBuffer, insertSeqEnd));
    CHECK(insertReader.ObjectMap.size() == 2u);

    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t polylineSeqEndHandle = 0x102;
    const auto polylineFixture = makePolylineFrameFixture(false);
    REQUIRE(!polylineFixture.bytes.empty());
    DwgEntityReaderProbe polylineReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(polylineFixture.bytes.data()),
        polylineFixture.bytes.size()));
    polylineReader.setVersionForTest(DRW::AC1018);
    REQUIRE(polylineReader.ObjectMap.emplace(
        polylineHandle, objHandle(dwgType::POLYLINE_2D, polylineHandle,
                                  polylineFixture.polylineOffset)).second);
    const objHandle polylineSeqEnd(
        dwgType::SEQEND, polylineSeqEndHandle, polylineFixture.seqEndOffset);
    REQUIRE(polylineReader.ObjectMap.emplace(
        polylineSeqEndHandle, polylineSeqEnd).second);
    dwgBuffer polylineBuffer(
        const_cast<std::uint8_t*>(polylineFixture.bytes.data()),
        polylineFixture.bytes.size());
    CHECK_FALSE(polylineReader.requiresLegacyCompoundHandling(
        &polylineBuffer, polylineSeqEnd));
    CHECK(polylineReader.ObjectMap.size() == 2u);

    const auto unknownOwnerFrame = makeSeqEndFrame(0x184, 0x185);
    REQUIRE(!unknownOwnerFrame.empty());
    DwgEntityReaderProbe unknownOwnerReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(unknownOwnerFrame.data()),
        unknownOwnerFrame.size()));
    unknownOwnerReader.setVersionForTest(DRW::AC1018);
    const objHandle unknownOwnerSeqEnd(dwgType::SEQEND, 0x184, 0);
    unknownOwnerReader.ObjectMap.emplace(0x184, unknownOwnerSeqEnd);
    dwgBuffer unknownOwnerBuffer(
        const_cast<std::uint8_t*>(unknownOwnerFrame.data()),
        unknownOwnerFrame.size());
    CHECK_FALSE(unknownOwnerReader.requiresLegacyCompoundHandling(
        &unknownOwnerBuffer, unknownOwnerSeqEnd));
    CHECK(unknownOwnerReader.ObjectMap.size() == 1u);
}

TEST_CASE("DWG HANDLE-map SEQEND routing isolates INSERT and POLYLINE",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t polylineSeqEndHandle = 0x102;
    constexpr std::uint32_t insertHandle = 0x181;
    constexpr std::uint32_t attribHandle = 0x182;
    constexpr std::uint32_t insertSeqEndHandle = 0x183;

    const auto polyline = makePolylineFrameFixture(false);
    const auto insertFrame = makeInsertWithChildrenFrame(
        insertHandle, DRW::NoHandle, attribHandle, insertSeqEndHandle);
    const auto attribFrame = makeAttribFrame(attribHandle, insertHandle);
    const auto insertSeqEndFrame = makeSeqEndFrame(
        insertSeqEndHandle, insertHandle);
    REQUIRE(!polyline.bytes.empty());
    REQUIRE(!insertFrame.empty());
    REQUIRE(!attribFrame.empty());
    REQUIRE(!insertSeqEndFrame.empty());

    std::vector<std::uint8_t> objectData = polyline.bytes;
    const auto insertOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), insertFrame.cbegin(), insertFrame.cend());
    const auto attribOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), attribFrame.cbegin(), attribFrame.cend());
    const auto insertSeqEndOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), insertSeqEndFrame.cbegin(),
                      insertSeqEndFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(polyline.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(
        polylineSeqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(polyline.seqEndOffset)
        - polyline.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(insertHandle - polylineSeqEndHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(insertOffset) - polyline.seqEndOffset));
    REQUIRE(handleEntries.putUModularChar(attribHandle - insertHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(attribOffset) - insertOffset));
    REQUIRE(handleEntries.putUModularChar(insertSeqEndHandle - attribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(insertSeqEndOffset) - attribOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));

    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    CHECK_FALSE(reader.requiresLegacyCompoundHandling(
        &objectBuffer, reader.ObjectMap.at(polylineSeqEndHandle)));
    CHECK_FALSE(reader.requiresLegacyCompoundHandling(
        &objectBuffer, reader.ObjectMap.at(insertSeqEndHandle)));

    DRW_Block_Record record;
    record.name = "MIXED_COMPOUND_BLOCK";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0,
        {polylineHandle, vertexHandle, polylineSeqEndHandle, attribHandle,
         insertSeqEndHandle, insertHandle});
    DwgInsertReceiptProbe interface;
    REQUIRE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 1u);
    REQUIRE(interface.inserts.size() == 1u);
    CHECK(interface.inserts.front().handle == insertHandle);
    REQUIRE(interface.inserts.front().attlist.size() == 1u);
    CHECK(interface.inserts.front().attlist.front()->handle == attribHandle);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.objObjectMap.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedPendingPolylineHandleForTest() == DRW::NoHandle);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedPolylineVertexCountForTest(polylineHandle) == 0u);
    CHECK(reader.m_consumedCompoundChildHandles.count(vertexHandle) == 1u);
    CHECK(reader.m_dwgSourceFrameLedger[1].m_disposition
          == DRW_DwgFrameDisposition::Published);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.m_consumedSeqEndHandles.count(polylineSeqEndHandle) == 1u);
    CHECK(reader.m_consumedSeqEndHandles.count(insertSeqEndHandle) == 1u);
    REQUIRE(interface.publications.size() == 6u);
    for (const std::uint32_t handle : {polylineHandle, vertexHandle,
                                       polylineSeqEndHandle, insertHandle,
                                       attribHandle, insertSeqEndHandle}) {
        CHECK(std::count_if(
                  interface.publications.cbegin(), interface.publications.cend(),
                  [handle](const DRW_DwgFramePublication& publication) {
                      return publication.m_handle == handle;
                  }) == 1u);
    }
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 6u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG mapped POLYLINE commits a child-first ownership list",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.vertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "CHILD_FIRST_POLYLINE";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {vertexHandle, seqEndHandle, polylineHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    REQUIRE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 1u);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.m_consumedCompoundChildHandles.count(vertexHandle) == 1u);
    CHECK(reader.m_consumedSeqEndHandles.count(seqEndHandle) == 1u);
    REQUIRE(interface.publications.size() == 3u);
    const std::array<std::uint32_t, 3> expectedPublicationOrder = {
        polylineHandle, vertexHandle, seqEndHandle};
    for (std::size_t index = 0; index < expectedPublicationOrder.size(); ++index) {
        CHECK(interface.publications[index].m_handle
              == expectedPublicationOrder[index]);
    }
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG mapped parent-first POLYLINE claims declared children",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.vertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "PARENT_FIRST_POLYLINE";
    DwgBlockOwnershipTestAccess::setHandles(record, 0, 0, {polylineHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    REQUIRE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 1u);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.m_consumedCompoundChildHandles.count(vertexHandle) == 1u);
    CHECK(reader.m_consumedSeqEndHandles.count(seqEndHandle) == 1u);
    REQUIRE(interface.publications.size() == 3u);
    const std::array<std::uint32_t, 3> expectedPublicationOrder = {
        polylineHandle, vertexHandle, seqEndHandle};
    for (std::size_t index = 0; index < expectedPublicationOrder.size(); ++index) {
        CHECK(interface.publications[index].m_handle
              == expectedPublicationOrder[index]);
    }
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG child-first 2D POLYLINE restores parent elevation",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    constexpr double elevation = 37.5;
    const auto fixture = makePolylineFrameFixture(
        false, false, false, false, elevation);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.vertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "CHILD_FIRST_ELEVATION";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {vertexHandle, seqEndHandle, polylineHandle});
    DwgCompoundWriteProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    REQUIRE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    REQUIRE(interface.polylines.size() == 1u);
    REQUIRE(interface.polylines.front().vertlist.size() == 1u);
    CHECK(interface.polylines.front().basePoint.z == Catch::Approx(elevation));
    CHECK(interface.polylines.front().vertlist.front()->basePoint.z
          == Catch::Approx(elevation));
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
}

TEST_CASE("DWG POLYLINE commits vertices in declared order",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t firstVertexHandle = 0x101;
    constexpr std::uint32_t secondVertexHandle = 0x102;
    constexpr std::uint32_t seqEndHandle = 0x103;
    const auto fixture = makePolylineFrameFixture(
        false, false, false, false, 0.0, true);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(
        firstVertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(
        secondVertexHandle - firstVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.secondVertexOffset)
        - fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(
        seqEndHandle - secondVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.secondVertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "OUT_OF_ORDER_POLYLINE_CHILDREN";
    DwgBlockOwnershipTestAccess::setHandles(record, 0, 0,
                                             {secondVertexHandle, seqEndHandle,
                                              firstVertexHandle, polylineHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    REQUIRE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 1u);
    REQUIRE(interface.publications.size() == 4u);
    const std::array<std::uint32_t, 4> expectedPublicationOrder = {
        polylineHandle, firstVertexHandle, secondVertexHandle, seqEndHandle};
    for (std::size_t index = 0; index < expectedPublicationOrder.size(); ++index) {
        CHECK(interface.publications[index].m_handle
              == expectedPublicationOrder[index]);
    }
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 4u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG modern POLYLINE preserves child-first ownership through R2013",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t firstVertexHandle = 0x101;
    constexpr std::uint32_t secondVertexHandle = 0x102;
    constexpr std::uint32_t seqEndHandle = 0x103;
    const std::array<DRW::Version, 3> versions = {
        DRW::AC1021, DRW::AC1024, DRW::AC1027};

    for (const DRW::Version version : versions) {
        const auto fixture = makePolylineFrameFixture(
            false, false, false, false, 0.0, true, version);
        REQUIRE(!fixture.bytes.empty());

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(polylineHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(
            firstVertexHandle - polylineHandle));
        REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
        REQUIRE(handleEntries.putUModularChar(
            secondVertexHandle - firstVertexHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(fixture.secondVertexOffset)
            - fixture.vertexOffset));
        REQUIRE(handleEntries.putUModularChar(
            seqEndHandle - secondVertexHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(fixture.seqEndOffset)
            - fixture.secondVertexOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(version);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

        DRW_Block_Record record;
        record.name = "R2013_POLYLINE";
        DwgBlockOwnershipTestAccess::setHandles(
            record, 0, 0, {secondVertexHandle, seqEndHandle,
                            firstVertexHandle, polylineHandle});
        DwgCompoundWriteProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());

        REQUIRE(reader.walkBlockRecordEntities(
            &record, &objectBuffer, interface));
        REQUIRE(interface.polylines.size() == 1u);
        REQUIRE(interface.polylines.front().vertlist.size() == 2u);
        CHECK(interface.polylines.front().vertlist[0]->handle
              == firstVertexHandle);
        CHECK(interface.polylines.front().vertlist[1]->handle
              == secondVertexHandle);
        const std::array<std::uint32_t, 4> publicationOrder = {
            polylineHandle, firstVertexHandle, secondVertexHandle,
            seqEndHandle};
        REQUIRE(interface.publications.size() == publicationOrder.size());
        for (std::size_t index = 0; index < publicationOrder.size(); ++index) {
            CHECK(interface.publications[index].m_handle
                  == publicationOrder[index]);
        }
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == publicationOrder.size());
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
            CHECK(entry.m_publicationCount == 1u);
        }
    }
}

TEST_CASE("DWG modern POLYLINE rejects foreign children through R2013",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const std::array<DRW::Version, 3> versions = {
        DRW::AC1021, DRW::AC1024, DRW::AC1027};

    for (const DRW::Version version : versions) {
        const auto fixture = makePolylineFrameFixture(
            false, false, false, true, 0.0, false, version);
        REQUIRE(!fixture.bytes.empty());

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(polylineHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(
            vertexHandle - polylineHandle));
        REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
        REQUIRE(handleEntries.putUModularChar(
            seqEndHandle - vertexHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(fixture.seqEndOffset)
            - fixture.vertexOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(version);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

        DRW_Block_Record record;
        record.name = "FOREIGN_R2013_POLYLINE_VERTEX";
        DwgBlockOwnershipTestAccess::setHandles(
            record, 0, 0, {vertexHandle, seqEndHandle, polylineHandle});
        DwgInsertReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(
            &record, &objectBuffer, interface));
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
            CHECK(entry.m_publicationCount == 0u);
        }
    }
}

TEST_CASE("DWG modern POLYLINE rejects duplicate declarations through R2013",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const std::array<DRW::Version, 3> versions = {
        DRW::AC1021, DRW::AC1024, DRW::AC1027};

    for (const DRW::Version version : versions) {
        const auto fixture = makePolylineFrameFixture(
            false, true, false, false, 0.0, false, version);
        REQUIRE(!fixture.bytes.empty());

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(polylineHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(
            vertexHandle - polylineHandle));
        REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
        REQUIRE(handleEntries.putUModularChar(
            seqEndHandle - vertexHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(fixture.seqEndOffset)
            - fixture.vertexOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(version);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

        DRW_Block_Record record;
        record.name = "DUPLICATE_R2013_POLYLINE_VERTEX";
        DwgBlockOwnershipTestAccess::setHandles(
            record, 0, 0, {polylineHandle, vertexHandle, seqEndHandle});
        DwgInsertReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(
            &record, &objectBuffer, interface));
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
            CHECK(entry.m_publicationCount == 0u);
        }
    }
}

TEST_CASE("DWG modern POLYLINE rejects duplicate SEQEND delivery through R2013",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const std::array<DRW::Version, 3> versions = {
        DRW::AC1021, DRW::AC1024, DRW::AC1027};

    for (const DRW::Version version : versions) {
        const auto fixture = makePolylineFrameFixture(
            false, false, false, false, 0.0, false, version);
        REQUIRE(!fixture.bytes.empty());

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(polylineHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(
            vertexHandle - polylineHandle));
        REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
        REQUIRE(handleEntries.putUModularChar(
            seqEndHandle - vertexHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(fixture.seqEndOffset)
            - fixture.vertexOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(version);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

        DRW_Block_Record record;
        record.name = "DUPLICATE_R2013_POLYLINE_SEQEND";
        DwgBlockOwnershipTestAccess::setHandles(
            record, 0, 0, {vertexHandle, seqEndHandle, seqEndHandle,
                            polylineHandle});
        DwgInsertReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(
            &record, &objectBuffer, interface));
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
            CHECK(entry.m_publicationCount == 0u);
        }
    }
}

TEST_CASE("DWG modern POLYLINE rejects incompatible VERTEX subtypes through R2013",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const std::array<DRW::Version, 3> versions = {
        DRW::AC1021, DRW::AC1024, DRW::AC1027};

    for (const DRW::Version version : versions) {
        const auto fixture = makePolylineFrameFixture(
            false, false, false, false, 0.0, false, version,
            DRW_Vertex::DwgSubtype::Vertex3D);
        REQUIRE(!fixture.bytes.empty());

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(polylineHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(
            vertexHandle - polylineHandle));
        REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
        REQUIRE(handleEntries.putUModularChar(
            seqEndHandle - vertexHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(fixture.seqEndOffset)
            - fixture.vertexOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(version);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

        DRW_Block_Record record;
        record.name = "INCOMPATIBLE_R2013_POLYLINE_VERTEX";
        DwgBlockOwnershipTestAccess::setHandles(
            record, 0, 0, {vertexHandle, seqEndHandle, polylineHandle});
        DwgInsertReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(
            &record, &objectBuffer, interface));
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK((entry.m_disposition == DRW_DwgFrameDisposition::Failed
                   || entry.m_disposition
                       == DRW_DwgFrameDisposition::Quarantined));
            CHECK(entry.m_publicationCount == 0u);
        }
    }
}

TEST_CASE("DWG mapped POLYLINE rejects a duplicate declared VERTEX",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false, true);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.vertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "DUPLICATE_POLYLINE_VERTEX";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {polylineHandle, vertexHandle, seqEndHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 0u);
    CHECK(interface.publications.empty());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.m_invalidPolylineOwners.count(polylineHandle) == 1u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
        CHECK(entry.m_publicationCount == 0u);
    }
}

TEST_CASE("DWG mapped POLYLINE rejects an unclaimed child-first VERTEX",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false, false, false, true);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.vertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "UNCLAIMED_POLYLINE_VERTEX";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {vertexHandle, seqEndHandle, polylineHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 0u);
    CHECK(interface.publications.empty());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
        CHECK(entry.m_publicationCount == 0u);
    }
}

TEST_CASE("DWG foreign VERTEX owner leaves POLYLINE unpublished",
          "[dwg][safety]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    auto fixture = makePolylineFrameFixture(false, false, false, true);
    REQUIRE(!fixture.bytes.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        fixture.bytes.data(), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.ObjectMap.emplace(
        polylineHandle,
        objHandle(dwgType::POLYLINE_2D, polylineHandle,
                  fixture.polylineOffset));
    reader.ObjectMap.emplace(
        vertexHandle,
        objHandle(dwgType::VERTEX_2D, vertexHandle, fixture.vertexOffset));
    reader.ObjectMap.emplace(
        seqEndHandle,
        objHandle(dwgType::SEQEND, seqEndHandle, fixture.seqEndOffset));

    dwgBuffer buffer(fixture.bytes.data(), fixture.bytes.size());
    DwgReadProbe interface;
    objHandle polyline(
        dwgType::POLYLINE_2D, polylineHandle, fixture.polylineOffset);
    CHECK_FALSE(reader.readDwgEntity(&buffer, polyline, interface));
    CHECK(interface.polylineCount == 0);
    CHECK(reader.ObjectMap.size() == 3);
}

TEST_CASE("DWG failed POLYLINE children stay out of later sweeps",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    auto fixture = makePolylineFrameFixture(false, false, false, true);
    REQUIRE(!fixture.bytes.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        fixture.bytes.data(), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.ObjectMap.emplace(
        polylineHandle,
        objHandle(dwgType::POLYLINE_2D, polylineHandle,
                  fixture.polylineOffset));
    reader.ObjectMap.emplace(
        vertexHandle,
        objHandle(dwgType::VERTEX_2D, vertexHandle, fixture.vertexOffset));
    reader.ObjectMap.emplace(
        seqEndHandle,
        objHandle(dwgType::SEQEND, seqEndHandle, fixture.seqEndOffset));

    dwgBuffer entityBuffer(fixture.bytes.data(), fixture.bytes.size());
    DwgReadProbe entityInterface;
    objHandle polyline(
        dwgType::POLYLINE_2D, polylineHandle, fixture.polylineOffset);
    CHECK_FALSE(reader.readDwgEntity(
        &entityBuffer, polyline, entityInterface));
    CHECK(entityInterface.polylineCount == 0);
    CHECK(reader.ObjectMap.size() == 3);

    dwgBuffer sweepBuffer(fixture.bytes.data(), fixture.bytes.size());
    DwgReadProbe sweepInterface;
    CHECK_FALSE(reader.readDwgEntities(sweepInterface, &sweepBuffer));
    CHECK(sweepInterface.unsupportedObjects.empty());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.objObjectMap.empty());
}

TEST_CASE("DWG failed POLYLINE children stay out of the owning block walk",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    auto fixture = makePolylineFrameFixture(false, false, false, true);
    REQUIRE(!fixture.bytes.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        fixture.bytes.data(), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.ObjectMap.emplace(
        polylineHandle,
        objHandle(dwgType::POLYLINE_2D, polylineHandle,
                  fixture.polylineOffset));
    reader.ObjectMap.emplace(
        vertexHandle,
        objHandle(dwgType::VERTEX_2D, vertexHandle, fixture.vertexOffset));
    reader.ObjectMap.emplace(
        seqEndHandle,
        objHandle(dwgType::SEQEND, seqEndHandle, fixture.seqEndOffset));

    DRW_Block_Record block;
    DwgBlockOwnershipTestAccess::setHandles(
        block, 0, 0, {polylineHandle, vertexHandle, seqEndHandle});
    DwgReadProbe interface;
    dwgBuffer buffer(fixture.bytes.data(), fixture.bytes.size());
    CHECK_FALSE(reader.walkBlockRecordEntities(
        &block, &buffer, interface, DRW::NoHandle));
    CHECK(interface.polylineCount == 0);
    CHECK(interface.unsupportedObjects.empty());
    CHECK(reader.m_quarantinedEntityHandles.find(polylineHandle)
          != reader.m_quarantinedEntityHandles.end());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.m_entityParseFailures >= 1);
}

TEST_CASE("DWG modern POLYLINE owned-object counts are bounded",
          "[dwg][safety][compound]") {
    for (const std::int32_t count : {-1, 1000001}) {
        const auto frame = makeMalformedModernPolylineFrame(count);
        REQUIRE(!frame.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(DRW::AC1018);
        DwgReadProbe interface;
        dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
        objHandle polyline(dwgType::POLYLINE_2D, 0x220u, 0);

        CHECK_FALSE(reader.readDwgEntity(&buffer, polyline, interface));
        CHECK(interface.polylineCount == 0);
        CHECK(interface.unsupportedObjects.empty());
    }
}

TEST_CASE("DWG legacy POLYLINE chains reject cycles and early termination",
          "[dwg][safety][compound]") {
    for (const bool cycle : {false, true}) {
        const auto fixture = makeLegacyPolylineFrameFixture(cycle);
        REQUIRE(!fixture.bytes.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(DRW::AC1015);
        reader.ObjectMap.emplace(
            0x121u, objHandle(dwgType::VERTEX_2D, 0x121u,
                              fixture.vertexOffset));
        reader.ObjectMap.emplace(
            0x123u, objHandle(dwgType::SEQEND, 0x123u,
                              fixture.seqEndOffset));

        dwgBuffer buffer(const_cast<std::uint8_t*>(fixture.bytes.data()),
                         fixture.bytes.size());
        DwgReadProbe interface;
        objHandle polyline(dwgType::POLYLINE_2D, 0x120u, 0);
        CHECK_FALSE(reader.readDwgEntity(&buffer, polyline, interface));
        CHECK(interface.polylineCount == 0);
        CHECK(reader.ObjectMap.size() == 2);
    }
}

TEST_CASE("DWG legacy INSERT block walk commits its isolated ATTRIB chain",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t insertHandle = 0x720;
    constexpr std::uint32_t firstAttribHandle = 0x721;
    constexpr std::uint32_t secondAttribHandle = 0x722;
    constexpr std::uint32_t lastAttribHandle = 0x723;
    constexpr std::uint32_t seqEndHandle = 0x724;
    const auto fixture = makeLegacyInsertFrameFixture();
    REQUIRE(!fixture.bytes.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(insertHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(firstAttribHandle - insertHandle));
    REQUIRE(handleEntries.putModularChar(fixture.firstAttribOffset));
    REQUIRE(handleEntries.putUModularChar(
        secondAttribHandle - firstAttribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.secondAttribOffset)
        - fixture.firstAttribOffset));
    REQUIRE(handleEntries.putUModularChar(lastAttribHandle - secondAttribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.lastAttribOffset)
        - fixture.secondAttribOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - lastAttribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.lastAttribOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "LEGACY_INSERT";
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        record, insertHandle, insertHandle);
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    REQUIRE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    REQUIRE(interface.inserts.size() == 1u);
    REQUIRE(interface.inserts.front().attlist.size() == 3u);
    CHECK(interface.inserts.front().attlist[0]->handle == firstAttribHandle);
    CHECK(interface.inserts.front().attlist[1]->handle == secondAttribHandle);
    CHECK(interface.inserts.front().attlist[2]->handle == lastAttribHandle);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.m_consumedCompoundChildHandles.count(firstAttribHandle) == 1u);
    CHECK(reader.m_consumedCompoundChildHandles.count(secondAttribHandle) == 1u);
    CHECK(reader.m_consumedCompoundChildHandles.count(lastAttribHandle) == 1u);
    CHECK(reader.m_consumedSeqEndHandles.count(seqEndHandle) == 1u);
    REQUIRE(interface.publications.size() == 5u);
    const std::array<std::uint32_t, 5> expectedPublicationOrder = {
        insertHandle, firstAttribHandle, secondAttribHandle, lastAttribHandle,
        seqEndHandle};
    for (std::size_t index = 0; index < expectedPublicationOrder.size(); ++index) {
        CHECK(interface.publications[index].m_handle
              == expectedPublicationOrder[index]);
    }
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 5u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG legacy INSERT rejects a foreign ATTRIB owner atomically",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t insertHandle = 0x720;
    constexpr std::uint32_t firstAttribHandle = 0x721;
    constexpr std::uint32_t secondAttribHandle = 0x722;
    constexpr std::uint32_t lastAttribHandle = 0x723;
    constexpr std::uint32_t seqEndHandle = 0x724;
    const auto fixture = makeLegacyInsertFrameFixture(true);
    REQUIRE(!fixture.bytes.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(insertHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(firstAttribHandle - insertHandle));
    REQUIRE(handleEntries.putModularChar(fixture.firstAttribOffset));
    REQUIRE(handleEntries.putUModularChar(
        secondAttribHandle - firstAttribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.secondAttribOffset)
        - fixture.firstAttribOffset));
    REQUIRE(handleEntries.putUModularChar(lastAttribHandle - secondAttribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.lastAttribOffset)
        - fixture.secondAttribOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - lastAttribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.lastAttribOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "MALFORMED_LEGACY_INSERT";
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        record, insertHandle, insertHandle);
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.inserts.empty());
    CHECK(interface.publications.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.ObjectMap.count(insertHandle) == 0u);
    CHECK(reader.m_quarantinedEntityHandles.count(firstAttribHandle) == 1u);
    CHECK(reader.m_quarantinedEntityHandles.count(secondAttribHandle) == 1u);
    CHECK(reader.m_quarantinedEntityHandles.count(lastAttribHandle) == 1u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 5u);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
          == DRW_DwgFrameDisposition::Failed);
    for (std::size_t index = 1; index < 4; ++index) {
        CHECK(reader.m_dwgSourceFrameLedger[index].m_disposition
              == DRW_DwgFrameDisposition::Quarantined);
        CHECK(reader.m_dwgSourceFrameLedger[index].m_publicationCount == 0u);
    }

    DwgReadProbe sweepInterface;
    dwgBuffer sweepBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    REQUIRE(reader.readDwgEntities(sweepInterface, &sweepBuffer));
    CHECK(sweepInterface.inserts.empty());
    CHECK(sweepInterface.framePublications.empty());
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 5u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK((entry.m_disposition == DRW_DwgFrameDisposition::Failed
               || entry.m_disposition == DRW_DwgFrameDisposition::Quarantined));
        CHECK(entry.m_publicationCount == 0u);
    }
}

TEST_CASE("DWG legacy INSERT commits a declared empty ATTRIB range",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t insertHandle = 0x730;
    constexpr std::uint32_t seqEndHandle = 0x732;
    const auto fixture = makeLegacyEmptyInsertFrameFixture();
    REQUIRE(!fixture.bytes.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(insertHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - insertHandle));
    REQUIRE(handleEntries.putModularChar(fixture.seqEndOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "LEGACY_EMPTY_INSERT";
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        record, insertHandle, insertHandle);
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    REQUIRE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    REQUIRE(interface.inserts.size() == 1u);
    REQUIRE(interface.inserts.front().attribHandles.size() == 2u);
    CHECK(interface.inserts.front().attribHandles[0].ref == DRW::NoHandle);
    CHECK(interface.inserts.front().attribHandles[1].ref == DRW::NoHandle);
    CHECK(interface.inserts.front().attlist.empty());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.m_consumedCompoundChildHandles.empty());
    CHECK(reader.m_consumedSeqEndHandles.count(seqEndHandle) == 1u);
    REQUIRE(interface.publications.size() == 2u);
    CHECK(interface.publications[0].m_handle == insertHandle);
    CHECK(interface.publications[1].m_handle == seqEndHandle);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 2u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG legacy INSERT rejects malformed empty ATTRIB boundaries",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t insertHandle = 0x730;

    SECTION("exactly one null boundary") {
        const auto fixture = makeLegacyEmptyInsertFrameFixture(true);
        REQUIRE(!fixture.bytes.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
        reader.setVersionForTest(DRW::AC1015);
        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(insertHandle));
        REQUIRE(handleEntries.putModularChar(0));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

        DRW_Block_Record record;
        DwgBlockOwnershipTestAccess::setLegacyEntityChain(
            record, insertHandle, insertHandle);
        DwgInsertReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer,
                                                   interface));
        CHECK(interface.inserts.empty());
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingInsertCountForTest() == 0u);
        CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
              == DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    }

    SECTION("declared SEQEND has no source frame") {
        const auto fixture = makeLegacyEmptyInsertFrameFixture();
        REQUIRE(!fixture.bytes.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
        reader.setVersionForTest(DRW::AC1015);
        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(insertHandle));
        REQUIRE(handleEntries.putModularChar(0));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

        DRW_Block_Record record;
        DwgBlockOwnershipTestAccess::setLegacyEntityChain(
            record, insertHandle, insertHandle);
        DwgInsertReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer,
                                                   interface));
        CHECK(interface.inserts.empty());
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingInsertCountForTest() == 0u);
        CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
              == DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    }
}

TEST_CASE("DWG legacy INSERT writer emits documented handle reference codes",
          "[dwg][safety][writer]") {
    constexpr std::uint32_t blockHandle = 0x740;
    constexpr std::uint32_t firstAttribHandle = 0x741;
    constexpr std::uint32_t lastAttribHandle = 0x742;
    constexpr std::uint32_t seqEndHandle = 0x743;

    const auto decodeHandles = [](const dwgBufferW& encoded,
                                  std::size_t count) {
        std::vector<dwgHandle> handles;
        const auto& bytes = encoded.data();
        dwgBuffer reader(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        handles.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
            handles.push_back(reader.getHandle());
            if (!reader.isGood())
                return std::vector<dwgHandle>{};
        }
        return handles;
    };

    const auto writeAttributedInsert = [&](bool minsert,
                                           std::uint32_t first,
                                           std::uint32_t last) {
        DwgInsertWriterProbe insert;
        insert.handle = 0x73Fu;
        insert.blockRecH.ref = blockHandle;
        insert.colcount = minsert ? 2 : 1;
        dwgHandle firstBoundary;
        firstBoundary.ref = first;
        dwgHandle lastBoundary;
        lastBoundary.ref = last;
        insert.attribHandles = {firstBoundary, lastBoundary};
        insert.seqendH.ref = seqEndHandle;

        dwgBufferW body;
        dwgBufferW handles;
        if (!insert.encodeDwg(DRW::AC1015, &body, 0, nullptr, &handles))
            return std::vector<dwgHandle>{};
        return decodeHandles(handles, 6u);
    };

    for (const bool minsert : {false, true}) {
        const auto handles = writeAttributedInsert(
            minsert, firstAttribHandle, lastAttribHandle);
        REQUIRE(handles.size() == 6u);
        CHECK(handles[2].code == DRW::DwgHardPointer);
        CHECK(handles[2].ref == blockHandle);
        CHECK(handles[3].code == DRW::DwgSoftPointer);
        CHECK(handles[3].ref == firstAttribHandle);
        CHECK(handles[4].code == DRW::DwgSoftPointer);
        CHECK(handles[4].ref == lastAttribHandle);
        CHECK(handles[5].code == DRW::DwgHardOwnership);
        CHECK(handles[5].ref == seqEndHandle);
    }

    SECTION("single ATTRIB preserves both legacy boundaries") {
        const auto handles = writeAttributedInsert(
            false, firstAttribHandle, firstAttribHandle);
        REQUIRE(handles.size() == 6u);
        CHECK(handles[3].ref == firstAttribHandle);
        CHECK(handles[4].ref == firstAttribHandle);
    }

    SECTION("null legacy boundaries use soft-pointer references") {
        const auto handles = writeAttributedInsert(false, DRW::NoHandle,
                                                   DRW::NoHandle);
        REQUIRE(handles.size() == 6u);
        CHECK(handles[3].code == DRW::DwgSoftPointer);
        CHECK(handles[3].size == 0u);
        CHECK(handles[3].ref == DRW::NoHandle);
        CHECK(handles[4].code == DRW::DwgSoftPointer);
        CHECK(handles[4].size == 0u);
        CHECK(handles[4].ref == DRW::NoHandle);
    }

    SECTION("attribute-free INSERT emits no sequence suffix") {
        DwgInsertWriterProbe insert;
        insert.handle = 0x73Fu;
        insert.blockRecH.ref = blockHandle;
        dwgBufferW body;
        dwgBufferW encodedHandles;
        REQUIRE(insert.encodeDwg(DRW::AC1015, &body, 0, nullptr,
                                 &encodedHandles));
        const auto handles = decodeHandles(encodedHandles, 3u);
        REQUIRE(handles.size() == 3u);
        CHECK(handles[2].code == DRW::DwgHardPointer);
        CHECK(handles[2].ref == blockHandle);
    }

    SECTION("modern INSERT and MINSERT use type-specific ownership codes") {
        const auto writeModernAttributedInsert =
            [&](DRW::Version version, bool minsert,
                std::size_t attributeCount) {
                DwgInsertWriterProbe insert;
                insert.handle = 0x73Fu;
                insert.blockRecH.ref = blockHandle;
                insert.colcount = minsert ? 2 : 1;
                if (attributeCount == 0u || attributeCount > 2u)
                    return std::vector<dwgHandle>{};
                dwgHandle firstAttribute;
                firstAttribute.ref = firstAttribHandle;
                insert.attribHandles.push_back(firstAttribute);
                if (attributeCount == 2u) {
                    dwgHandle lastAttribute;
                    lastAttribute.ref = lastAttribHandle;
                    insert.attribHandles.push_back(lastAttribute);
                }
                insert.seqendH.ref = seqEndHandle;
                dwgBufferW body;
                dwgBufferW encodedHandles;
                if (!insert.encodeDwg(version, &body, 0, nullptr,
                                      &encodedHandles))
                    return std::vector<dwgHandle>{};
                return decodeHandles(encodedHandles, 4u + attributeCount);
            };

        for (const DRW::Version version :
             {DRW::AC1018, DRW::AC1021, DRW::AC1032}) {
            for (const bool minsert : {false, true}) {
                for (const std::size_t attributeCount : {1u, 2u}) {
                    CAPTURE(static_cast<int>(version), minsert,
                            attributeCount);
                    const auto handles = writeModernAttributedInsert(
                        version, minsert, attributeCount);
                    REQUIRE(handles.size() == 4u + attributeCount);
                    CHECK(handles[2].code == DRW::DwgHardPointer);
                    CHECK(handles[2].ref == blockHandle);
                    for (std::size_t index = 0; index < attributeCount;
                         ++index) {
                        CHECK(handles[3u + index].code ==
                              (minsert ? DRW::DwgSoftPointer
                                       : DRW::DwgHardOwnership));
                        CHECK(handles[3u + index].ref ==
                              (index == 0u ? firstAttribHandle
                                           : lastAttribHandle));
                    }
                    CHECK(handles[3u + attributeCount].code ==
                          DRW::DwgHardOwnership);
                    CHECK(handles[3u + attributeCount].ref == seqEndHandle);
                }
            }
        }
    }

    SECTION("malformed legacy attribute metadata writes no partial frame") {
        DwgInsertWriterProbe asymmetric;
        asymmetric.handle = 0x73Fu;
        asymmetric.blockRecH.ref = blockHandle;
        dwgHandle nullBoundary;
        dwgHandle nonNullBoundary;
        nonNullBoundary.ref = firstAttribHandle;
        asymmetric.attribHandles = {nullBoundary, nonNullBoundary};
        asymmetric.seqendH.ref = seqEndHandle;
        dwgBufferW asymmetricBody;
        dwgBufferW asymmetricHandles;
        CHECK_FALSE(asymmetric.encodeDwg(
            DRW::AC1015, &asymmetricBody, 0, nullptr, &asymmetricHandles));
        CHECK(asymmetricBody.data().empty());
        CHECK(asymmetricHandles.data().empty());

        DwgInsertWriterProbe missingSeqEnd;
        missingSeqEnd.handle = 0x73Fu;
        missingSeqEnd.blockRecH.ref = blockHandle;
        missingSeqEnd.attribHandles = {nonNullBoundary, nonNullBoundary};
        dwgBufferW missingSeqEndBody;
        dwgBufferW missingSeqEndHandles;
        CHECK_FALSE(missingSeqEnd.encodeDwg(
            DRW::AC1015, &missingSeqEndBody, 0, nullptr,
            &missingSeqEndHandles));
        CHECK(missingSeqEndBody.data().empty());
        CHECK(missingSeqEndHandles.data().empty());

        DwgInsertWriterProbe modernNull;
        modernNull.handle = 0x73Fu;
        modernNull.blockRecH.ref = blockHandle;
        modernNull.attribHandles = {nullBoundary};
        modernNull.seqendH.ref = seqEndHandle;
        dwgBufferW modernNullBody;
        dwgBufferW modernNullHandles;
        CHECK_FALSE(modernNull.encodeDwg(
            DRW::AC1018, &modernNullBody, 0, nullptr, &modernNullHandles));
        CHECK(modernNullBody.data().empty());
        CHECK(modernNullHandles.data().empty());

        DwgInsertWriterProbe danglingSeqEnd;
        danglingSeqEnd.handle = 0x73Fu;
        danglingSeqEnd.blockRecH.ref = blockHandle;
        danglingSeqEnd.seqendH.ref = seqEndHandle;
        dwgBufferW danglingSeqEndBody;
        dwgBufferW danglingSeqEndHandles;
        CHECK_FALSE(danglingSeqEnd.encodeDwg(
            DRW::AC1018, &danglingSeqEndBody, 0, nullptr,
            &danglingSeqEndHandles));
        CHECK(danglingSeqEndBody.data().empty());
        CHECK(danglingSeqEndHandles.data().empty());
    }
}

TEST_CASE("DWG legacy POLYLINE block walk commits one staged aggregate",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x120;
    constexpr std::uint32_t firstVertexHandle = 0x121;
    constexpr std::uint32_t lastVertexHandle = 0x122;
    constexpr std::uint32_t seqEndHandle = 0x123;
    const auto fixture = makeCompleteLegacyPolylineFrameFixture();
    REQUIRE(!fixture.bytes.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(
        firstVertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(
        lastVertexHandle - firstVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.secondVertexOffset)
        - fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - lastVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.secondVertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "LEGACY_POLYLINE";
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        record, polylineHandle, polylineHandle);
    DwgCompoundWriteProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    REQUIRE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    REQUIRE(interface.polylines.size() == 1u);
    REQUIRE(interface.polylines.front().vertlist.size() == 2u);
    CHECK(interface.polylines.front().vertlist[0]->handle == firstVertexHandle);
    CHECK(interface.polylines.front().vertlist[1]->handle == lastVertexHandle);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(interface.publications.size() == 4u);
    const std::array<std::uint32_t, 4> expectedPublicationOrder = {
        polylineHandle, firstVertexHandle, lastVertexHandle, seqEndHandle};
    for (std::size_t index = 0; index < expectedPublicationOrder.size(); ++index) {
        CHECK(interface.publications[index].m_handle
              == expectedPublicationOrder[index]);
    }
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 4u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG legacy POLYLINE resolves staged VERTEX EED references",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x120;
    constexpr std::uint32_t firstVertexHandle = 0x121;
    constexpr std::uint32_t lastVertexHandle = 0x122;
    constexpr std::uint32_t seqEndHandle = 0x123;
    const auto fixture = makeCompleteLegacyPolylineFrameFixture(
        false, false, true);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(
        firstVertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(
        lastVertexHandle - firstVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.secondVertexOffset)
        - fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - lastVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.secondVertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    reader.addAppIdForTest(0x150, "RESOLVED_LEGACY_APPID");
    reader.addLayerForTest(0x151, "RESOLVED_LEGACY_LAYER");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "LEGACY_POLYLINE_EED";
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        record, polylineHandle, polylineHandle);
    DwgCompoundWriteProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    REQUIRE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    REQUIRE(interface.polylines.size() == 1u);
    REQUIRE(interface.polylines.front().vertlist.size() == 2u);
    const auto& vertex = interface.polylines.front().vertlist.front();
    REQUIRE(vertex != nullptr);
    REQUIRE(vertex->extData.size() == 2u);
    REQUIRE(vertex->extData[0] != nullptr);
    REQUIRE(vertex->extData[1] != nullptr);
    CHECK(vertex->extData[0]->code() == 1001);
    CHECK(std::string(vertex->extData[0]->c_str()) == "RESOLVED_LEGACY_APPID");
    CHECK(vertex->extData[1]->code() == 1003);
    CHECK(vertex->extData[1]->isLayerRef());
    CHECK(std::string(vertex->extData[1]->c_str()) == "RESOLVED_LEGACY_LAYER");
    CHECK(vertex->pendingAppIdResolutions.empty());
    CHECK(vertex->pendingLayerRefResolutions.empty());
}

TEST_CASE("DWG legacy POLYLINE block walk quarantines malformed chains",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x120;
    constexpr std::uint32_t vertexHandle = 0x121;
    constexpr std::uint32_t seqEndHandle = 0x123;
    for (const bool cycle : {false, true}) {
        const auto fixture = makeLegacyPolylineFrameFixture(cycle);
        REQUIRE(!fixture.bytes.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(DRW::AC1015);
        reader.ObjectMap.emplace(
            polylineHandle,
            objHandle(dwgType::POLYLINE_2D, polylineHandle, 0));
        reader.ObjectMap.emplace(
            vertexHandle,
            objHandle(dwgType::VERTEX_2D, vertexHandle,
                      fixture.vertexOffset));
        reader.ObjectMap.emplace(
            seqEndHandle,
            objHandle(dwgType::SEQEND, seqEndHandle, fixture.seqEndOffset));

        DRW_Block_Record record;
        record.name = "MALFORMED_LEGACY_POLYLINE";
        DwgBlockOwnershipTestAccess::setLegacyEntityChain(
            record, polylineHandle, polylineHandle);
        DwgCompoundWriteProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(
            &record, &objectBuffer, interface));
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.polylines.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        CHECK(reader.m_quarantinedEntityHandles.count(vertexHandle) == 1u);
        CHECK(reader.m_quarantinedEntityHandles.count(seqEndHandle) == 1u);

        DwgReadProbe sweepInterface;
        dwgBuffer sweepBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());
        REQUIRE(reader.readDwgEntities(sweepInterface, &sweepBuffer));
        CHECK(sweepInterface.polylineCount == 0u);
        CHECK(sweepInterface.unsupportedObjects.empty());
        CHECK(reader.ObjectMap.empty());
    }
}

TEST_CASE("DWG legacy POLYLINE block walk rejects a non-VERTEX tail",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x120;
    constexpr std::uint32_t firstVertexHandle = 0x121;
    constexpr std::uint32_t tailHandle = 0x122;
    constexpr std::uint32_t seqEndHandle = 0x123;
    const auto fixture = makeLegacyPolylineWrongTailFrameFixture();
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(
        firstVertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(tailHandle - firstVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.secondVertexOffset)
        - fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - tailHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.secondVertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "LEGACY_POLYLINE_WRONG_TAIL";
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        record, polylineHandle, polylineHandle);
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 0u);
    CHECK(interface.publications.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 4u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Failed);
    for (std::size_t index = 1; index < reader.m_dwgSourceFrameLedger.size();
         ++index) {
        const DRW_DwgFrameCoverageEntry& entry =
            reader.m_dwgSourceFrameLedger[index];
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
        CHECK(entry.m_publicationCount == 0u);
    }

    DwgReadProbe sweepInterface;
    dwgBuffer sweepBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    REQUIRE(reader.readDwgEntities(sweepInterface, &sweepBuffer));
    CHECK(sweepInterface.polylineCount == 0u);
    CHECK(sweepInterface.unsupportedObjects.empty());
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG legacy POLYLINE block walk rejects a foreign SEQEND",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x120;
    constexpr std::uint32_t firstVertexHandle = 0x121;
    constexpr std::uint32_t lastVertexHandle = 0x122;
    constexpr std::uint32_t seqEndHandle = 0x123;
    const auto fixture = makeCompleteLegacyPolylineFrameFixture(true);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(
        firstVertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(
        lastVertexHandle - firstVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.secondVertexOffset)
        - fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - lastVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.secondVertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "FOREIGN_LEGACY_SEQEND";
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        record, polylineHandle, polylineHandle);
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 0u);
    CHECK(interface.publications.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 4u);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_publicationCount == 0u);
    for (std::size_t index = 1; index < reader.m_dwgSourceFrameLedger.size();
         ++index) {
        const DRW_DwgFrameCoverageEntry& entry =
            reader.m_dwgSourceFrameLedger[index];
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
        CHECK(entry.m_publicationCount == 0u);
    }

    DwgReadProbe sweepInterface;
    dwgBuffer sweepBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    REQUIRE(reader.readDwgEntities(sweepInterface, &sweepBuffer));
    CHECK(sweepInterface.polylineCount == 0u);
    CHECK(sweepInterface.unsupportedObjects.empty());
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG legacy POLYLINE block walk rejects a foreign VERTEX",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x120;
    constexpr std::uint32_t firstVertexHandle = 0x121;
    constexpr std::uint32_t lastVertexHandle = 0x122;
    constexpr std::uint32_t seqEndHandle = 0x123;
    const auto fixture = makeCompleteLegacyPolylineFrameFixture(false, true);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(
        firstVertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(
        lastVertexHandle - firstVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.secondVertexOffset)
        - fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - lastVertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.secondVertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "FOREIGN_LEGACY_VERTEX";
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        record, polylineHandle, polylineHandle);
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 0u);
    CHECK(interface.publications.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 4u);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
          == DRW_DwgFrameDisposition::Failed);
    for (std::size_t index = 1; index < reader.m_dwgSourceFrameLedger.size();
         ++index) {
        const DRW_DwgFrameCoverageEntry& entry =
            reader.m_dwgSourceFrameLedger[index];
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
        CHECK(entry.m_publicationCount == 0u);
    }

    DwgReadProbe sweepInterface;
    dwgBuffer sweepBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    REQUIRE(reader.readDwgEntities(sweepInterface, &sweepBuffer));
    CHECK(sweepInterface.polylineCount == 0u);
    CHECK(sweepInterface.unsupportedObjects.empty());
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG MLINE nested counts are bounded",
          "[dwg][safety]") {
    for (const auto [vertexCount, parameterCount] : {
             std::pair<std::uint16_t, std::uint16_t>{5001, 0},
             std::pair<std::uint16_t, std::uint16_t>{5000, 0},
             std::pair<std::uint16_t, std::uint16_t>{1, 5001}}) {
        const auto frame = makeMalformedMLineFrame(vertexCount, parameterCount);
        REQUIRE(!frame.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(DRW::AC1018);
        DwgReadProbe interface;
        dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
        objHandle mline(dwgType::MLINE, 0x230u, 0);

        CHECK_FALSE(reader.readDwgEntity(&buffer, mline, interface));
        CHECK(interface.mlineCount == 0);
        CHECK(interface.unsupportedObjects.empty());
    }
}

TEST_CASE("DWG MLINE does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedMLineBody();
    REQUIRE(!body.empty());

    DwgMLineProbe mline;
    mline.scale = 8.0;
    mline.styleName = "STALE_STYLE";
    mline.styleHandle = 0x2FBu;
    mline.vertlist.emplace_back();
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(mline.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(mline.scale == 1.0);
    CHECK(mline.styleName.empty());
    CHECK(mline.styleHandle == 0);
    CHECK(mline.vertlist.empty());
}

TEST_CASE("DWG MLINE writer rejects invalid geometry",
          "[dwg][safety]") {
    DwgMLineWriterProbe writer;
    writer.handle = 0x2FBu;
    writer.numLines = 1;
    writer.numVerts = 1;
    DRW_MLineVertex vertex;
    vertex.position = DRW_Coord{std::numeric_limits<double>::quiet_NaN(),
                                2.0, 3.0};
    vertex.vertexDir = DRW_Coord{1.0, 0.0, 0.0};
    vertex.miterDir = DRW_Coord{0.0, 1.0, 0.0};
    vertex.segParms.resize(1);
    vertex.areaFillParms.resize(1);
    writer.vertlist.push_back(std::move(vertex));

    dwgBufferW body;
    CHECK_FALSE(writer.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr));
    CHECK(body.data().empty());

    DwgMLineWriterProbe mismatched;
    mismatched.numVerts = 1;
    dwgBufferW mismatchedBody;
    CHECK_FALSE(mismatched.encodeDwg(
        DRW::AC1018, &mismatchedBody, 0, nullptr, nullptr));
    CHECK(mismatchedBody.data().empty());
}

TEST_CASE("DWG VERTEX writer rejects invalid wire-domain values",
          "[dwg][safety]") {
    const auto rejectedWithoutPartialWrite =
        [](DwgVertexWriterProbe& vertex) {
            dwgBufferW body;
            body.putRawChar8(0xA5);
            const std::vector<std::uint8_t> before = body.data();
            CHECK_FALSE(vertex.encodeDwg(
                DRW::AC1027, &body, 0, nullptr, nullptr));
            CHECK(body.data() == before);
        };

    SECTION("vertex flags are an unsigned byte") {
        DwgVertexWriterProbe vertex;
        vertex.flags = 256;
        rejectedWithoutPartialWrite(vertex);
    }

    SECTION("face indices are signed bit-shorts") {
        DwgVertexWriterProbe vertex;
        vertex.setDwgSubtype(DRW_Vertex::DwgSubtype::PolyfaceFace);
        vertex.vindex1 = std::numeric_limits<std::int16_t>::max() + 1;
        rejectedWithoutPartialWrite(vertex);

        vertex.vindex1 = std::numeric_limits<std::int16_t>::min() - 1;
        rejectedWithoutPartialWrite(vertex);
    }

    SECTION("numeric payload is finite") {
        DwgVertexWriterProbe vertex;
        vertex.basePoint.x = std::numeric_limits<double>::quiet_NaN();
        rejectedWithoutPartialWrite(vertex);
    }
}

TEST_CASE("DWG POLYLINE writer rejects invalid wire-domain values",
          "[dwg][safety][compound]") {
    const auto rejectedWithoutPartialWrite =
        [](DwgPolylineWriterProbe& polyline) {
            dwgBufferW body;
            body.putRawChar8(0xA5);
            const std::vector<std::uint8_t> before = body.data();
            CHECK_FALSE(polyline.encodeDwg(
                DRW::AC1027, &body, 0, nullptr, nullptr));
            CHECK(body.data() == before);
        };

    SECTION("flags are an unsigned byte") {
        DwgPolylineWriterProbe polyline;
        polyline.flags = 256;
        rejectedWithoutPartialWrite(polyline);
    }

    SECTION("bit-short fields are representable") {
        DwgPolylineWriterProbe polyline;
        polyline.smoothM = std::numeric_limits<std::uint16_t>::max() + 1;
        rejectedWithoutPartialWrite(polyline);
    }

    SECTION("children are non-null") {
        DwgPolylineWriterProbe polyline;
        polyline.vertlist.push_back(nullptr);
        rejectedWithoutPartialWrite(polyline);
    }

    SECTION("numeric payload is finite") {
        DwgPolylineWriterProbe polyline;
        polyline.extPoint.z = std::numeric_limits<double>::infinity();
        rejectedWithoutPartialWrite(polyline);
    }
}

TEST_CASE("DWG LWPOLYLINE writer rejects invalid payloads",
          "[dwg][safety][compound]") {
    const auto rejectedWithoutPartialWrite =
        [](DwgLwPolylineProbe& polyline) {
            dwgBufferW body;
            body.putRawChar8(0xA5);
            const std::vector<std::uint8_t> before = body.data();
            CHECK_FALSE(polyline.encodeDwg(
                DRW::AC1027, &body, 0, nullptr, nullptr));
            CHECK(body.data() == before);
        };

    SECTION("vertices are non-null") {
        DwgLwPolylineProbe polyline;
        polyline.vertlist.push_back(nullptr);
        rejectedWithoutPartialWrite(polyline);
    }

    SECTION("numeric payload is finite") {
        DwgLwPolylineProbe polyline;
        polyline.width = std::numeric_limits<double>::quiet_NaN();
        rejectedWithoutPartialWrite(polyline);
    }

    SECTION("group 70 contains only LWPOLYLINE flags") {
        DwgLwPolylineProbe polyline;
        polyline.flags = 0x100;
        rejectedWithoutPartialWrite(polyline);
    }
}

TEST_CASE("DWG UNDERLAY does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedUnderlayBody();
    REQUIRE(!body.empty());

    DwgUnderlayProbe underlay;
    underlay.kind = DRW_Underlay::DGN;
    underlay.definitionHandle = 0x2FEu;
    underlay.clipBoundary.emplace_back(8.0, 9.0, 0.0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(underlay.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(underlay.kind == DRW_Underlay::DGN);
    CHECK(underlay.definitionHandle == 0);
    CHECK(underlay.clipBoundary.empty());
}

TEST_CASE("DWG UNDERLAY writer rejects non-finite geometry",
          "[dwg][safety]") {
    DwgUnderlayProbe writer;
    writer.handle = 0x2FEu;
    writer.kind = DRW_Underlay::DGN;
    writer.position = DRW_Coord{std::numeric_limits<double>::quiet_NaN(),
                                 2.0, 3.0};

    dwgBufferW body;
    CHECK_FALSE(writer.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr));
    CHECK(body.data().empty());
}

TEST_CASE("DWG IMAGE does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedImageBody();
    REQUIRE(!body.empty());

    DwgImageProbe image;
    image.m_classVersion = 7;
    image.ref = 0x302u;
    image.m_imageDefReactorHandle = 0x303u;
    image.clipPath.emplace_back(8.0, 9.0, 0.0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(image.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(image.m_classVersion == 0);
    CHECK(image.ref == 0);
    CHECK(image.m_imageDefReactorHandle == 0);
    CHECK(image.clipPath.empty());
}

TEST_CASE("DWG WIPEOUT rejects an image without a clip atomically",
          "[dwg][safety]") {
    DwgImageProbe image;
    image.handle = 0x310u;
    dwgBufferW encoded;
    REQUIRE(image.encodeDwg(DRW::AC1018, &encoded, 0, nullptr, nullptr));

    DwgWipeoutProbe wipeout;
    dwgBuffer buffer(const_cast<std::uint8_t*>(encoded.data().data()),
                     encoded.data().size());
    CHECK_FALSE(wipeout.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(buffer.getPosition() == 0);
    CHECK(buffer.getBitPos() == 0);
    CHECK(wipeout.clipPath.empty());
    CHECK(wipeout.ref == 0);
    CHECK(wipeout.m_imageDefReactorHandle == 0);
}

TEST_CASE("DWG INSERT owned ATTRIB counts are bounded",
          "[dwg][safety]") {
    for (const std::int32_t count : {-1, 1000001}) {
        const auto frame = makeMalformedInsertFrame(count);
        REQUIRE(!frame.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(DRW::AC1018);
        DwgReadProbe interface;
        dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
        objHandle insert(dwgType::INSERT, 0x250u, 0);

        CHECK_FALSE(reader.readDwgEntity(&buffer, insert, interface));
        CHECK(interface.inserts.empty());
        CHECK(interface.unsupportedObjects.empty());
    }
}

TEST_CASE("DWG INSERT does not publish partial ownership handles",
          "[dwg][safety]") {
    const auto body = makeMalformedInsertBody(0);
    REQUIRE(!body.empty());

    DwgInsertProbe insert;
    insert.blockRecH.ref = 0x251u;
    insert.seqendH.ref = 0x252u;
    insert.attribHandles.push_back(makeObjectHandle(0x253u));
    insert.thickness = 4.0;
    insert.xAxisAngle = 5.0;

    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(insert.parseDwg(DRW::AC1018, &buffer));
    CHECK_FALSE(buffer.isGood());
    CHECK(insert.blockRecH.ref == 0);
    CHECK(insert.seqendH.ref == 0);
    CHECK(insert.attribHandles.empty());
    CHECK(insert.thickness == 0.0);
    CHECK(insert.xAxisAngle == 0.0);
    CHECK(insert.extPoint.x == 0.0);
    CHECK(insert.extPoint.y == 0.0);
    CHECK(insert.extPoint.z == 1.0);
}

TEST_CASE("DWG INSERT rejects non-finite geometry",
          "[dwg][safety]") {
    DwgInsertWriterProbe writer;
    writer.handle = 0x254u;
    writer.basePoint = DRW_Coord{
        std::numeric_limits<double>::quiet_NaN(), 2.0, 3.0};

    dwgBufferW body;
    REQUIRE(writer.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr));
    finalizeEncodedEntityBody(body);

    DwgInsertProbe insert;
    insert.basePoint = DRW_Coord{8.0, 9.0, 10.0};
    insert.blockRecH.ref = 0x255u;
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data().data()),
                     body.data().size());
    CHECK_FALSE(insert.parseDwg(DRW::AC1018, &buffer));
    CHECK_FALSE(buffer.isGood());
    CHECK(insert.basePoint.x == 0.0);
    CHECK(insert.basePoint.y == 0.0);
    CHECK(insert.basePoint.z == 0.0);
    CHECK(insert.blockRecH.ref == 0);
}

TEST_CASE("DWG ACAD_TABLE owned ATTRIB counts are bounded",
          "[dwg][safety][table]") {
    for (const std::int32_t count : {-1, 1000001}) {
        const auto body = makeMalformedTableBody(count);
        REQUIRE(!body.empty());

        DwgTableReaderProbe table;
        dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
        CHECK_FALSE(table.parseDwg(DRW::AC1018, &buffer));
    }
}

TEST_CASE("DWG ACAD_TABLE rejects null and invalid grid input",
          "[dwg][safety][table]") {
    DwgTableReaderProbe table;
    CHECK_FALSE(table.parseDwg(DRW::AC1018, nullptr));

    DwgInsertWriterProbe tableHeader;
    tableHeader.handle = 0x261u;
    tableHeader.setObjectType(dwgType::INSERT);

    dwgBufferW body;
    REQUIRE(tableHeader.encodeDwgCommon(DRW::AC1018, &body));
    body.put3BitDouble(DRW_Coord());
    body.put2Bits(3);
    body.putBitDouble(0.0);
    body.putExtrusion(DRW_Coord(0.0, 0.0, 1.0), false);
    body.putBit(0); // no attributes
    body.putBitShort(0); // table value flags
    body.put3BitDouble(DRW_Coord(1.0, 0.0, 0.0));
    body.putBitLong(std::numeric_limits<std::int32_t>::max());
    body.putBitLong(0);

    const std::uint32_t handleStreamBit =
        static_cast<std::uint32_t>(body.bitCount());
    REQUIRE(tableHeader.encodeDwgEntHandle(DRW::AC1018, &body));
    dwgHandle blockRecord;
    blockRecord.code = 2;
    blockRecord.ref = 0;
    blockRecord.size = 0;
    body.putHandle(blockRecord);

    const std::uint8_t bsCode =
        static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
    const std::size_t objectSizeBit =
        bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
    body.patchRawLong32AtBit(objectSizeBit, handleStreamBit);

    dwgBuffer reader(body.data().data(), body.data().size());
    CHECK_FALSE(table.parseDwg(DRW::AC1018, &reader));
    CHECK_FALSE(reader.isGood());
}

TEST_CASE("DWG table-entry common header rejects null buffer",
          "[dwg][safety][object-header]") {
    DwgTableEntryProbe entry;
    CHECK_FALSE(entry.parseCommon(DRW::AC1027, nullptr));
}

TEST_CASE("DWG object frame header failure restores the cursor",
          "[dwg][safety][frame]") {
    const std::array<std::uint8_t, 1> truncatedHeader = {0};
    dwgBuffer buffer(const_cast<std::uint8_t*>(truncatedHeader.data()),
                     truncatedHeader.size());
    CHECK(buffer.setPosition(0));
    buffer.setBitPos(0);

    DwgObjectFrame frame;
    CHECK_FALSE(frame.readAt(buffer, DRW::AC1018, 0));
    CHECK(buffer.getPosition() == 0);
    CHECK(buffer.getBitPos() == 0);

    dwgBufferW zeroBodyBits;
    zeroBodyBits.putModularShort(1);
    zeroBodyBits.putUModularChar(8);
    zeroBodyBits.putRawChar8(0);
    const std::uint16_t crc = zeroBodyBits.crc16(
        0xC0C1, 0, zeroBodyBits.data().size());
    zeroBodyBits.putRawShort16(crc);
    dwgBuffer zeroBodyBitsBuffer(zeroBodyBits.data().data(),
                                 zeroBodyBits.data().size());
    DwgObjectFrame zeroBodyBitsFrame;
    CHECK_FALSE(zeroBodyBitsFrame.readAt(
        zeroBodyBitsBuffer, DRW::AC1027, 0));
    CHECK(zeroBodyBitsBuffer.getPosition() == 0);
}

TEST_CASE("DWG VIEWPORT frozen-layer counts are bounded",
          "[dwg][safety]") {
    for (const std::int32_t count : {-1, 513}) {
        const auto frame = makeMalformedViewportFrame(count);
        REQUIRE(!frame.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(DRW::AC1018);
        DwgReadProbe interface;
        dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
        objHandle viewport(dwgType::VIEWPORT, 0x240u, 0);

        CHECK_FALSE(reader.readDwgEntity(&buffer, viewport, interface));
        CHECK(interface.viewportCount == 0);
        CHECK(interface.unsupportedObjects.empty());
    }
}

TEST_CASE("DWG VIEWPORT does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedViewportBody();
    REQUIRE(!body.empty());

    DwgViewportReaderProbe viewport;
    viewport.pswidth = 8.0;
    viewport.styleSheet = "STALE_STYLE";
    viewport.vpHeaderHandle = 0x30Bu;
    viewport.frozenLayerHandles.push_back(0x30Cu);
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(viewport.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(viewport.pswidth == 205.0);
    CHECK(viewport.styleSheet.empty());
    CHECK(viewport.vpHeaderHandle == 0);
    CHECK(viewport.frozenLayerHandles.empty());
}

TEST_CASE("DWG HATCH boundary counts honor the object body",
          "[dwg][safety]") {
    const auto makeBody = [](bool oversizedPolyline) {
        DwgHatchProbe hatch;
        hatch.setObjectType(dwgType::HATCH);
        hatch.handle = 0x241u;

        dwgBufferW body;
        if (!hatch.encodeDwgCommon(DRW::AC1018, &body))
            return std::vector<std::uint8_t>{};
        body.putBitDouble(0.0); // elevation
        body.put3BitDouble(DRW_Coord{0.0, 0.0, 1.0}); // extrusion
        body.putVariableText(DRW::AC1018, "SOLID"); // pattern name
        body.putBit(1); // solid
        body.putBit(0); // not associative
        body.putBitLong(oversizedPolyline ? 1 : 10000); // loop count
        if (oversizedPolyline) {
            body.putBitLong(2); // polyline boundary
            body.putBit(0); // no bulges
            body.putBit(0); // open
            body.putBitLong(10000); // vertex count
        }
        const std::uint8_t bsCode =
            static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
        const std::size_t objectSizeBit =
            bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
        body.patchRawLong32AtBit(objectSizeBit, body.bitCount());
        return body.data();
    };

    for (const bool oversizedPolyline : {false, true}) {
        auto bytes = makeBody(oversizedPolyline);
        REQUIRE_FALSE(bytes.empty());
        DwgHatchProbe hatch;
        dwgBuffer reader(bytes.data(), bytes.size());
        CHECK_FALSE(hatch.parseDwg(DRW::AC1018, &reader, 0));
        CHECK(hatch.looplist.empty());
    }
}

TEST_CASE("DWG R2018 MTEXT column counts are bounded",
          "[dwg][safety]") {
    for (const std::int32_t count : {-1, 4097}) {
        const auto frame = makeMalformedMTextFrame(count);
        // The writer rejects invalid declarations before they can reach the
        // framed reader, avoiding an invalid BL on disk.
        CHECK(frame.empty());
    }
}

TEST_CASE("DWG MTEXT reusable carriers clear version-gated state",
          "[dwg][safety]") {
    DwgMTextProbe mtext;
    const auto parseFrame = [&](DRW::Version version,
                                const std::vector<std::uint8_t>& bytes) {
        dwgBuffer frameBuffer(const_cast<std::uint8_t*>(bytes.data()),
                              bytes.size());
        DwgObjectFrame frame;
        REQUIRE(frame.readAt(frameBuffer, version, 0));
        dwgBuffer body(frame.body().data(), frame.body().size());
        return mtext.parseDwg(version, &body, frame.bodyBitSize());
    };

    const auto modernFrame = makeEncodedMTextFrame(DRW::AC1032, 2);
    REQUIRE(!modernFrame.empty());
    REQUIRE(parseFrame(DRW::AC1032, modernFrame));
    CHECK(mtext.m_r2018IsNotAnnotative);
    CHECK(mtext.m_r2018ColumnType == 2);
    CHECK(mtext.m_r2018ColumnCount == 2);

    const auto olderFrame = makeEncodedMTextFrame(DRW::AC1024, 0);
    REQUIRE(!olderFrame.empty());
    REQUIRE(parseFrame(DRW::AC1024, olderFrame));
    CHECK_FALSE(mtext.m_r2018IsNotAnnotative);
    CHECK(mtext.m_r2018Version == 0);
    CHECK(mtext.m_r2018ColumnType == 0);
    CHECK(mtext.m_r2018ColumnCount == 0);
    CHECK(mtext.m_r2018ColumnHeights.empty());
    CHECK(mtext.m_r2018AnnotativeData.empty());
    CHECK(mtext.m_r2018AppIdHandle == 0);
}

TEST_CASE("DWG R2007 MTEXT string failures do not publish entities",
          "[dwg][safety]") {
    const auto frame = makeMalformedMTextFrame(0, true);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1032);
    DwgReadProbe interface;
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    objHandle mtext(dwgType::MTEXT, 0x270u, 0);

    CHECK_FALSE(reader.readDwgEntity(&buffer, mtext, interface));
    CHECK(interface.mtextCount == 0);
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG MTEXT does not publish a truncated handle tail",
          "[dwg][safety]") {
    DwgMTextProbe writer;
    writer.handle = 0x2EB;
    writer.text = "value";

    dwgBufferW body;
    REQUIRE(writer.encodeDwg(DRW::AC1015, &body, 0, nullptr, nullptr));
    finalizeEncodedEntityBody(body);
    REQUIRE(body.data().size() > 1);
    body.data().pop_back();

    DwgMTextProbe reader;
    reader.text = "STALE_TEXT";
    reader.m_r2018ColumnCount = 8;
    reader.styleH.ref = 0x2EC;
    dwgBuffer buffer(body.data().data(), body.data().size());
    CHECK_FALSE(reader.parseDwg(DRW::AC1015, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(reader.text.empty());
    CHECK(reader.m_r2018ColumnCount == 0);
    CHECK(reader.styleH.ref == 0);
}

TEST_CASE("DWG MTEXT body bounds reject truncated fields",
          "[dwg][safety]") {
    DwgMTextProbe writer;
    writer.handle = 0x2EF;
    writer.text = "value";

    dwgBufferW body;
    REQUIRE(writer.encodeDwg(DRW::AC1015, &body, 0, nullptr, nullptr));
    finalizeEncodedEntityBody(body);
    REQUIRE(!body.data().empty());

    const std::uint8_t bsCode =
        static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
    const std::size_t sizeBitOffset =
        bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
    body.patchRawLong32AtBit(sizeBitOffset, 1);

    DwgMTextProbe reader;
    reader.text = "STALE_TEXT";
    reader.basePoint = DRW_Coord{9.0, 8.0, 7.0};
    dwgBuffer buffer(body.data().data(), body.data().size());
    CHECK_FALSE(reader.parseDwg(DRW::AC1015, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(reader.text.empty());
    CHECK(reader.basePoint.x == 0.0);
    CHECK(reader.basePoint.y == 0.0);
    CHECK(reader.basePoint.z == 0.0);
}

TEST_CASE("DWG TEXT does not publish a truncated handle tail",
          "[dwg][safety]") {
    DwgTextProbe writer;
    writer.handle = 0x2ED;
    writer.text = "value";
    writer.height = 1.0;

    dwgBufferW body;
    REQUIRE(writer.encodeDwg(DRW::AC1015, &body, 0, nullptr, nullptr));
    finalizeEncodedEntityBody(body);
    REQUIRE(body.data().size() > 1);
    body.data().pop_back();

    DwgTextProbe reader;
    reader.text = "STALE_TEXT";
    reader.styleH.ref = 0x2EE;
    dwgBuffer buffer(body.data().data(), body.data().size());
    CHECK_FALSE(reader.parseDwg(DRW::AC1015, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(reader.text.empty());
    CHECK(reader.styleH.ref == 0);
}

TEST_CASE("DWG RTEXT does not publish a truncated handle tail",
          "[dwg][safety]") {
    DwgRTextProbe writer;
    writer.handle = 0x2EF;
    writer.text = "value";
    writer.height = 1.0;

    dwgBufferW body;
    REQUIRE(writer.encodeDwg(DRW::AC1015, &body, 0, nullptr, nullptr));
    finalizeEncodedEntityBody(body);
    REQUIRE(body.data().size() > 1);
    body.data().pop_back();

    DwgRTextProbe reader;
    reader.text = "STALE_TEXT";
    reader.styleH.ref = 0x2F0;
    reader.m_rTextFlags = 7;
    dwgBuffer buffer(body.data().data(), body.data().size());
    CHECK_FALSE(reader.parseDwg(DRW::AC1015, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(reader.text.empty());
    CHECK(reader.styleH.ref == 0);
    CHECK(reader.m_rTextFlags == 0);
}

TEST_CASE("DWG ARCALIGNEDTEXT does not publish a truncated handle tail",
          "[dwg][safety]") {
    DwgArcAlignedTextProbe writer;
    writer.handle = 0x2F1;
    writer.text = "value";
    writer.m_center = DRW_Coord{1.0, 2.0, 0.0};
    writer.m_radius = 1.0;
    writer.m_startAngle = 0.0;
    writer.m_endAngle = 1.0;

    dwgBufferW body;
    REQUIRE(writer.encodeDwg(DRW::AC1015, &body, 0, nullptr, nullptr));
    finalizeEncodedEntityBody(body);
    REQUIRE(body.data().size() > 1);
    body.data().pop_back();

    DwgArcAlignedTextProbe reader;
    reader.text = "STALE_TEXT";
    reader.m_center = DRW_Coord{8.0, 9.0, 10.0};
    reader.m_radius = 4.0;
    reader.m_arcHandle = 0x2F2;
    dwgBuffer buffer(body.data().data(), body.data().size());
    CHECK_FALSE(reader.parseDwg(DRW::AC1015, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(reader.text.empty());
    CHECK(reader.m_radius == 0.0);
    CHECK(reader.m_arcHandle == 0);
}

TEST_CASE("DWG MLEADER invalid class versions fail parsing",
          "[dwg][safety]") {
    const auto frame = makeMalformedMLeaderFrame(11);
    REQUIRE(!frame.empty());

    DwgMLeaderProbe mleader;
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    CHECK_FALSE(mleader.parseDwg(DRW::AC1027, &buffer, 0));
}

TEST_CASE("DWG MLEADER writes its handle tail in specification order",
          "[dwg][safety]") {
    DwgMLeaderProbe mleader;
    mleader.handle = 0x280u;
    mleader.context.hasTextContents = true;
    mleader.context.textStyleHandle.ref = 0x53u;
    mleader.styleHandle.ref = 0x54u;
    mleader.leaderLineTypeHandle.ref = 0x55u;
    mleader.arrowHeadHandle.ref = 0x56u;
    mleader.styleTextStyleHandle.ref = 0x57u;
    mleader.styleBlockHandle.ref = 0x58u;

    DRW_MLeaderRoot root;
    DRW_MLeaderLeaderLine firstLine;
    firstLine.lineTypeHandle.ref = 0x51u;
    firstLine.arrowHandle.ref = 0x52u;
    root.leaderLines.push_back(firstLine);
    DRW_MLeaderLeaderLine secondLine;
    secondLine.lineTypeHandle.ref = 0x61u;
    secondLine.arrowHandle.ref = 0x62u;
    root.leaderLines.push_back(secondLine);
    mleader.context.roots.push_back(root);

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    REQUIRE(mleader.encodeDwg(DRW::AC1027, &body, 0, &strings, &handles));

    const auto bytes = handles.data();
    dwgBuffer reader(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
    std::vector<std::uint32_t> references;
    while (reader.isGood() && reader.numRemainingBytes() > 0)
        references.push_back(reader.getHandle().ref);
    REQUIRE(reader.isGood());
    REQUIRE(references.size() >= 11u);

    const std::vector<std::uint32_t> expectedTail = {
        0x51u, 0x52u, 0x61u, 0x62u, 0x53u, 0x54u,
        0x55u, 0x56u, 0x57u, 0x58u,
    };
    REQUIRE(std::equal(expectedTail.crbegin(), expectedTail.crend(),
                       references.crbegin()));
}

TEST_CASE("DWG MLEADER does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedMLeaderBody();
    REQUIRE(!body.empty());

    DwgMLeaderProbe mleader;
    mleader.styleHandle.ref = 0x2F7u;
    mleader.leaderExtendedToText = true;
    mleader.context.roots.emplace_back();
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(mleader.parseDwg(DRW::AC1027, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(mleader.styleHandle.ref == 0);
    CHECK_FALSE(mleader.leaderExtendedToText);
    CHECK(mleader.context.roots.empty());
}

TEST_CASE("DWG MLEADER nested counts honor the object body",
          "[dwg][safety]") {
    const auto makeBody = [](bool oversizedLineCount) {
        DwgMLeaderProbe mleader;
        mleader.setObjectType(501);
        mleader.handle = 0x281u;

        dwgBufferW body;
        if (!mleader.encodeDwgCommon(DRW::AC1018, &body))
            return std::vector<std::uint8_t>{};
        body.putBitLong(1); // one leader root
        body.putBit(0); // no last leader-line point
        body.putBit(0); // no dogleg direction
        body.putBitLong(oversizedLineCount ? 0 : 5000); // break count
        if (oversizedLineCount) {
            body.putBitLong(0); // branch index
            body.putBitDouble(0.0); // landing distance
            body.putBitLong(5000); // leader-line count
        }

        const std::uint8_t bsCode =
            static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
        const std::size_t objectSizeBit =
            bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
        body.patchRawLong32AtBit(objectSizeBit, body.bitCount());
        return body.data();
    };

    for (const bool oversizedLineCount : {false, true}) {
        auto bytes = makeBody(oversizedLineCount);
        REQUIRE_FALSE(bytes.empty());
        DwgMLeaderProbe mleader;
        dwgBuffer reader(bytes.data(), bytes.size());
        CHECK_FALSE(mleader.parseDwg(DRW::AC1018, &reader, 0));
        CHECK(mleader.context.roots.empty());
    }
}

TEST_CASE("DWG MLEADER reusable carriers clear version-gated state",
          "[dwg][safety]") {
    DwgMLeaderProbe mleader;
    mleader.context.roots.emplace_back();
    mleader.arrowHeads.emplace_back();
    mleader.blockLabels.emplace_back();
    mleader.leaderExtendedToText = true;
    CHECK(mleader.leaderExtendedToText);

    const auto frame = makeMalformedMLeaderFrame(11);
    REQUIRE(!frame.empty());
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    CHECK_FALSE(mleader.parseDwg(DRW::AC1027, &buffer, 0));
    CHECK_FALSE(mleader.leaderExtendedToText);
    CHECK(mleader.context.roots.empty());
    CHECK(mleader.arrowHeads.empty());
    CHECK(mleader.blockLabels.empty());
}

TEST_CASE("DWG POINTCLOUD rejected parses clear optional branch state",
          "[dwg][safety]") {
    DwgPointCloudProbe pointCloud;
    pointCloud.sourceFileCount = 2;
    pointCloud.sourceFiles = {"first.rcp", "second.rcp"};
    pointCloud.showIntensity = true;
    pointCloud.intensityScheme = 3;
    pointCloud.intensityStyle.minIntensity = 1.0;
    pointCloud.showClipping = true;
    pointCloud.clippingCount = 1;
    pointCloud.clippings.emplace_back();
    pointCloud.definitionHandle = 0xB10;
    pointCloud.reactorHandle = 0xB11;

    std::vector<std::uint8_t> empty;
    dwgBuffer buffer(empty.data(), empty.size());
    CHECK_FALSE(pointCloud.parseDwg(DRW::AC1032, &buffer, 0));
    CHECK(pointCloud.sourceFileCount == 0);
    CHECK(pointCloud.sourceFiles.empty());
    CHECK_FALSE(pointCloud.showIntensity);
    CHECK(pointCloud.intensityScheme == 0);
    CHECK(pointCloud.intensityStyle.minIntensity == 0.0);
    CHECK_FALSE(pointCloud.showClipping);
    CHECK(pointCloud.clippingCount == 0);
    CHECK(pointCloud.clippings.empty());
    CHECK(pointCloud.definitionHandle == 0);
    CHECK(pointCloud.reactorHandle == 0);

    pointCloud.showIntensity = true;
    pointCloud.sourceFileCount = 1;
    CHECK_FALSE(pointCloud.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(pointCloud.showIntensity);
    CHECK(pointCloud.sourceFileCount == 0);
}

TEST_CASE("DWG POINTCLOUD does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedPointCloudBody();
    REQUIRE(!body.empty());

    DwgPointCloudProbe pointCloud;
    pointCloud.sourceFileCount = 2;
    pointCloud.sourceFiles = {"stale-first.rcp", "stale-second.rcp"};
    pointCloud.showIntensity = true;
    pointCloud.intensityScheme = 4;
    pointCloud.clippings.emplace_back();
    pointCloud.clippingCount = 1;
    pointCloud.definitionHandle = 0x3F0u;
    pointCloud.reactorHandle = 0x3F1u;

    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(pointCloud.parseDwg(DRW::AC1027, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(pointCloud.sourceFileCount == 0);
    CHECK(pointCloud.sourceFiles.empty());
    CHECK_FALSE(pointCloud.showIntensity);
    CHECK(pointCloud.intensityScheme == 0);
    CHECK(pointCloud.clippings.empty());
    CHECK(pointCloud.clippingCount == 0);
    CHECK(pointCloud.definitionHandle == 0);
    CHECK(pointCloud.reactorHandle == 0);
}

TEST_CASE("DWG POINTCLOUDEX rejected parses clear optional branch state",
          "[dwg][safety]") {
    DwgPointCloudExProbe pointCloud;
    pointCloud.unknownInt0 = 1;
    pointCloud.unknownInt1 = 2;
    pointCloud.stylizationType = 3;
    pointCloud.intensityColorScheme = "intensity";
    pointCloud.currentColorScheme = "current";
    pointCloud.classificationColorScheme = "classification";
    pointCloud.elevationMin = 1.0;
    pointCloud.elevationMax = 2.0;
    pointCloud.intensityMin = 3.0;
    pointCloud.intensityMax = 4.0;
    pointCloud.intensityOutOfRangeBehavior = 5;
    pointCloud.elevationOutOfRangeBehavior = 6;
    pointCloud.elevationApplyToFixedRange = true;
    pointCloud.intensityAsGradient = true;
    pointCloud.elevationAsGradient = true;
    pointCloud.croppingCount = 1;
    pointCloud.croppings.emplace_back();
    pointCloud.definitionHandle = 0xB20;
    pointCloud.reactorHandle = 0xB21;

    std::vector<std::uint8_t> empty;
    dwgBuffer buffer(empty.data(), empty.size());
    CHECK_FALSE(pointCloud.parseDwg(DRW::AC1032, &buffer, 0));
    CHECK(pointCloud.unknownInt0 == 0);
    CHECK(pointCloud.unknownInt1 == 0);
    CHECK(pointCloud.stylizationType == 0);
    CHECK(pointCloud.intensityColorScheme.empty());
    CHECK(pointCloud.currentColorScheme.empty());
    CHECK(pointCloud.classificationColorScheme.empty());
    CHECK(pointCloud.elevationMin == 0.0);
    CHECK(pointCloud.elevationMax == 0.0);
    CHECK(pointCloud.intensityMin == 0.0);
    CHECK(pointCloud.intensityMax == 0.0);
    CHECK(pointCloud.intensityOutOfRangeBehavior == 0);
    CHECK(pointCloud.elevationOutOfRangeBehavior == 0);
    CHECK_FALSE(pointCloud.elevationApplyToFixedRange);
    CHECK_FALSE(pointCloud.intensityAsGradient);
    CHECK_FALSE(pointCloud.elevationAsGradient);
    CHECK(pointCloud.croppingCount == 0);
    CHECK(pointCloud.croppings.empty());
    CHECK(pointCloud.definitionHandle == 0);
    CHECK(pointCloud.reactorHandle == 0);

    pointCloud.elevationMin = 9.0;
    pointCloud.croppingCount = 1;
    CHECK_FALSE(pointCloud.parseDwg(DRW::AC1024, &buffer, 0));
    CHECK(pointCloud.elevationMin == 0.0);
    CHECK(pointCloud.croppingCount == 0);
}

TEST_CASE("DWG POINTCLOUDEX does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedPointCloudExBody();
    REQUIRE(!body.empty());

    DwgPointCloudExProbe pointCloud;
    pointCloud.name = "STALE_NAME";
    pointCloud.showIntensity = true;
    pointCloud.showCropping = true;
    pointCloud.unknownInt0 = 9;
    pointCloud.intensityColorScheme = "stale-intensity";
    pointCloud.elevationMin = 8.0;
    pointCloud.croppingCount = 1;
    pointCloud.croppings.emplace_back();
    pointCloud.definitionHandle = 0x3F2u;
    pointCloud.reactorHandle = 0x3F3u;

    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(pointCloud.parseDwg(DRW::AC1027, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(pointCloud.name.empty());
    CHECK_FALSE(pointCloud.showIntensity);
    CHECK_FALSE(pointCloud.showCropping);
    CHECK(pointCloud.unknownInt0 == 0);
    CHECK(pointCloud.intensityColorScheme.empty());
    CHECK(pointCloud.elevationMin == 0.0);
    CHECK(pointCloud.croppingCount == 0);
    CHECK(pointCloud.croppings.empty());
    CHECK(pointCloud.definitionHandle == 0);
    CHECK(pointCloud.reactorHandle == 0);
}

TEST_CASE("DWG IMAGE and WIPEOUT rejected parses clear version-gated state",
          "[dwg][safety]") {
    DwgImageProbe image;
    image.clipMode = true;
    image.ref = 0xB30;
    image.m_imageDefReactorHandle = 0xB31;
    image.clipPath.emplace_back(1.0, 2.0, 0.0);

    std::vector<std::uint8_t> empty;
    dwgBuffer imageBuffer(empty.data(), empty.size());
    CHECK_FALSE(image.parseDwg(DRW::AC1021, &imageBuffer, 0));
    CHECK_FALSE(image.clipMode);
    CHECK(image.ref == 0);
    CHECK(image.m_imageDefReactorHandle == 0);
    CHECK(image.clipPath.empty());

    DwgWipeoutProbe wipeout;
    wipeout.clipMode = true;
    wipeout.ref = 0xB32;
    wipeout.m_imageDefReactorHandle = 0xB33;
    wipeout.clipPath.emplace_back(3.0, 4.0, 0.0);

    dwgBuffer wipeoutBuffer(empty.data(), empty.size());
    CHECK_FALSE(wipeout.parseDwg(DRW::AC1018, &wipeoutBuffer, 0));
    CHECK_FALSE(wipeout.clipMode);
    CHECK(wipeout.ref == 0);
    CHECK(wipeout.m_imageDefReactorHandle == 0);
    CHECK(wipeout.clipPath.empty());
}

TEST_CASE("DWG VIEWPORT rejected parses clear version-gated state",
          "[dwg][safety]") {
    DwgViewportReaderProbe viewport;
    viewport.m_sunHandle = 0xB40;

    std::vector<std::uint8_t> empty;
    dwgBuffer buffer(empty.data(), empty.size());
    CHECK_FALSE(viewport.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK(viewport.m_sunHandle == 0);
}

TEST_CASE("DWG NAVISWORKSMODEL does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedNavisworksModelBody();
    REQUIRE(!body.empty());

    DwgNavisworksModelProbe model;
    model.flags = 7;
    model.definitionHandle = 0x317u;
    model.transform[0] = 42.0;
    model.unitFactor = 9.0;
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(model.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(model.flags == 0);
    CHECK(model.definitionHandle == 0);
    CHECK(model.transform[0] == 1.0);
    CHECK(model.transform[1] == 0.0);
    CHECK(model.unitFactor == 1.0);
}

TEST_CASE("DWG NAVISWORKSMODEL rejected parses clear body state",
          "[dwg][safety]") {
    DwgNavisworksModelProbe model;
    model.flags = 7;
    model.definitionHandle = 0xB50;
    model.transform[0] = 42.0;
    model.transform[5] = 43.0;
    model.unitFactor = 9.0;

    std::vector<std::uint8_t> empty;
    dwgBuffer buffer(empty.data(), empty.size());
    CHECK_FALSE(model.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK(model.flags == 0);
    CHECK(model.definitionHandle == 0);
    CHECK(model.transform[0] == 1.0);
    CHECK(model.transform[5] == 1.0);
    CHECK(model.transform[1] == 0.0);
    CHECK(model.unitFactor == 1.0);

    model.flags = 8;
    model.definitionHandle = 0xB51;
    model.transform[0] = 44.0;
    model.unitFactor = 10.0;
    CHECK_FALSE(model.parseDwg(DRW::AC1014, &buffer, 0));
    CHECK(model.flags == 0);
    CHECK(model.definitionHandle == 0);
    CHECK(model.transform[0] == 1.0);
    CHECK(model.unitFactor == 1.0);
}

TEST_CASE("DWG optional entity trailers clear reused state",
          "[dwg][safety]") {
    std::vector<std::uint8_t> empty;

    DwgLightProbe light;
    light.m_hasPhotometricData = true;
    light.m_hasWebFile = true;
    light.m_webFile = "stale.ies";
    light.m_extendedLightRadius = 8.0;
    dwgBuffer lightBuffer(empty.data(), empty.size());
    CHECK_FALSE(light.parseDwg(DRW::AC1021, &lightBuffer, 0));
    CHECK_FALSE(light.m_hasPhotometricData);
    CHECK_FALSE(light.m_hasWebFile);
    CHECK(light.m_webFile.empty());
    CHECK(light.m_extendedLightRadius == 0.0);

    DwgCameraProbe camera;
    camera.m_viewHandle = 0xB60;
    dwgBuffer cameraBuffer(empty.data(), empty.size());
    CHECK_FALSE(camera.parseDwg(DRW::AC1018, &cameraBuffer, 0));
    CHECK(camera.m_viewHandle == 0);

    DwgSectionObjectProbe section;
    section.m_sectionSettingsHandle = 0xB61;
    dwgBuffer sectionBuffer(empty.data(), empty.size());
    CHECK_FALSE(section.parseDwg(DRW::AC1027, &sectionBuffer, 0));
    CHECK(section.m_sectionSettingsHandle == 0);

    DwgArcAlignedTextProbe arcText;
    arcText.m_arcHandle = 0xB62;
    dwgBuffer arcTextBuffer(empty.data(), empty.size());
    CHECK_FALSE(arcText.parseDwg(DRW::AC1024, &arcTextBuffer, 0));
    CHECK(arcText.m_arcHandle == 0);
}

TEST_CASE("DWG SURFACE clears reused DataStorage state",
          "[dwg][safety]") {
    DwgPlaneSurfaceProbe surface;
    surface.setHasDataStorageBinaryData(true);
    surface.hasDataStorageRecord = true;
    surface.dataStorageHandle = 0xB70;
    surface.dataStorageHandleKey = "B70";
    surface.dataStorageData = {0x01, 0x02, 0x03};
    surface.hasRawDwgBody = true;
    surface.rawDwgBodyBitSize = 24;
    surface.rawDwgBodyVersion = DRW::AC1027;
    surface.rawAcisData = {0x04, 0x05};
    surface.crossSectionHandles = {0xB71};
    surface.guideCurveHandles = {0xB72};
    surface.dwgPayloadDecoded = true;

    std::vector<std::uint8_t> empty;
    dwgBuffer buffer(empty.data(), empty.size());
    CHECK_FALSE(surface.parseDwg(DRW::AC1024, &buffer, 0));
    CHECK_FALSE(surface.hasDataStorageBinaryData());
    CHECK_FALSE(surface.hasDataStorageRecord);
    CHECK(surface.dataStorageHandle == 0);
    CHECK(surface.dataStorageHandleKey.empty());
    CHECK(surface.dataStorageData.empty());
    CHECK_FALSE(surface.hasRawDwgBody);
    CHECK(surface.rawDwgBodyBitSize == 0);
    CHECK(surface.rawDwgBodyVersion == DRW::UNKNOWNV);
    CHECK(surface.rawAcisData.empty());
    CHECK(surface.crossSectionHandles.empty());
    CHECK(surface.guideCurveHandles.empty());
    CHECK_FALSE(surface.dwgPayloadDecoded);
    CHECK_FALSE(surface.m_wireframeDecoded);
}

TEST_CASE("DWG SURFACE does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto encoded = makeTruncatedPlaneSurfaceBody();
    REQUIRE(!encoded.first.empty());

    DwgPlaneSurfaceProbe surface;
    surface.uIsolines = 7;
    surface.vIsolines = 8;
    surface.modelerFormatVersion = 9;
    surface.hasRawDwgBody = true;
    surface.rawDwgBodyBitSize = 64;
    surface.rawDwgBodyVersion = DRW::AC1027;
    surface.rawAcisData = {0x01, 0x02};
    surface.crossSectionHandles = {0x3F4u};
    surface.guideCurveHandles = {0x3F5u};
    surface.dwgPayloadDecoded = true;

    dwgBuffer buffer(const_cast<std::uint8_t*>(encoded.first.data()),
                     encoded.first.size());
    CHECK_FALSE(surface.parseDwg(DRW::AC1027, &buffer, encoded.second));
    CHECK_FALSE(buffer.isGood());
    CHECK(surface.uIsolines == 0);
    CHECK(surface.vIsolines == 0);
    CHECK(surface.modelerFormatVersion == 0);
    CHECK_FALSE(surface.hasRawDwgBody);
    CHECK(surface.rawDwgBodyBitSize == 0);
    CHECK(surface.rawDwgBodyVersion == DRW::UNKNOWNV);
    CHECK(surface.rawAcisData.empty());
    CHECK(surface.crossSectionHandles.empty());
    CHECK(surface.guideCurveHandles.empty());
    CHECK_FALSE(surface.dwgPayloadDecoded);
}

TEST_CASE("DWG SURFACE excludes detached strings from its raw body",
          "[dwg][safety][surface]") {
    DwgPlaneSurfaceProbe source;
    source.setDwgClassNum(DRW_PlaneSurface::kDwgClassNum);
    source.handle = 0x301u;
    source.uIsolines = 7;
    source.vIsolines = 8;

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    REQUIRE(source.encodeDwg(DRW::AC1027, &body, 0, &strings, &handles));

    const std::vector<std::uint8_t> marker = {0xD1u, 0xE2u, 0xF3u, 0x04u};
    body.alignToByte();
    strings.putBytes(marker.data(), marker.size());
    REQUIRE(DwgStringFooterWriterProbe::appendR2007StringStream(
        body, strings, true));
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    body.putBytes(handles.data().data(), handles.data().size());

    DwgPlaneSurfaceProbe parsed;
    dwgBuffer buffer(body.data().data(), body.data().size());
    REQUIRE(parsed.parseDwg(DRW::AC1027, &buffer, handleBits));
    CHECK(parsed.uIsolines == source.uIsolines);
    CHECK(parsed.vIsolines == source.vIsolines);
    CHECK(std::search(parsed.rawAcisData.cbegin(), parsed.rawAcisData.cend(),
                      marker.cbegin(), marker.cend())
          == parsed.rawAcisData.cend());
}

TEST_CASE("DWG MODELERGEOMETRY clears reused history state",
          "[dwg][safety]") {
    DwgModelerGeometryProbe geometry;
    geometry.m_modelerVersion = 4;
    geometry.m_bodyBitSize = 64;
    geometry.m_objectSize = 128;
    geometry.m_isEmpty = true;
    geometry.m_hasModelerData = true;
    geometry.m_modelerDataUnknownBit = true;
    geometry.m_hasWireframe = true;
    geometry.m_historyHandle = 0xB80;
    geometry.m_rawBytes = {0x01, 0x02};
    geometry.m_payloadRanges.emplace_back();
    geometry.m_wireframeDecoded = true;

    std::vector<std::uint8_t> empty;
    dwgBuffer buffer(empty.data(), empty.size());
    CHECK_FALSE(geometry.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK(geometry.m_modelerVersion == 0);
    CHECK(geometry.m_bodyBitSize == 0);
    CHECK(geometry.m_objectSize == 0);
    CHECK_FALSE(geometry.m_isEmpty);
    CHECK_FALSE(geometry.m_hasModelerData);
    CHECK_FALSE(geometry.m_modelerDataUnknownBit);
    CHECK_FALSE(geometry.m_hasWireframe);
    CHECK(geometry.m_historyHandle == 0);
    CHECK(geometry.m_rawBytes.empty());
    CHECK(geometry.m_payloadRanges.empty());
    CHECK_FALSE(geometry.m_wireframeDecoded);
}

TEST_CASE("DWG AC1015 modeler geometry rejects invalid opaque-body boundaries",
          "[dwg][safety][modeler]") {
    const auto patchObjectSize = [](dwgBufferW& body, std::uint32_t size) {
        const std::uint8_t objectTypeCode = static_cast<std::uint8_t>(
            (body.data().front() >> 6) & 0x03u);
        const std::size_t sizeBitOffset = objectTypeCode == 0x01u
            ? 10u : objectTypeCode == 0x00u ? 18u : 2u;
        body.patchRawLong32AtBit(sizeBitOffset, size);
    };
    const auto makeBody = [&patchObjectSize](
                              const std::function<void(dwgBufferW&,
                                                       std::uint32_t)>& mutate) {
        DwgModelerGeometryProbe source;
        source.setObjectType(dwgType::SOLID3D);
        source.handle = 0x381u;

        dwgBufferW body;
        if (!source.encodeDwgCommon(DRW::AC1015, &body))
            return std::vector<std::uint8_t>{};
        const std::uint32_t modelerStartBit = body.bitCount();
        body.putBit(false); // nonempty modeler data
        body.putBit(false); // unknown modeler flag
        body.putBitShort(1);
        body.putRawChar8(0xA5u); // opaque ACIS/SAT payload byte
        patchObjectSize(body, body.bitCount());
        if (!source.encodeDwgEntHandle(DRW::AC1015, &body))
            return std::vector<std::uint8_t>{};
        body.alignToByte();
        mutate(body, modelerStartBit);
        return body.data();
    };
    const auto validBytes = makeBody(
        [](dwgBufferW&, std::uint32_t) {});
    REQUIRE_FALSE(validBytes.empty());

    DwgModelerGeometryProbe valid;
    dwgBuffer validBuffer(const_cast<std::uint8_t*>(validBytes.data()),
                          validBytes.size());
    REQUIRE(valid.parseDwg(DRW::AC1015, &validBuffer, 0));
    CHECK(valid.handle == 0x381u);
    CHECK(valid.m_hasModelerData);
    CHECK(valid.m_modelerVersion == 1u);

    const auto assertRejected = [](const std::vector<std::uint8_t>& bytes) {
        DwgModelerGeometryProbe geometry;
        geometry.m_modelerVersion = 3;
        geometry.m_hasModelerData = true;
        dwgBuffer buffer(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        CHECK_FALSE(geometry.parseDwg(DRW::AC1015, &buffer, 0));
        CHECK_FALSE(buffer.isGood());
        CHECK(geometry.m_modelerVersion == 0u);
        CHECK_FALSE(geometry.m_hasModelerData);
        CHECK(geometry.m_rawBytes.empty());
    };

    const auto zeroSizeBytes = makeBody(
        [&patchObjectSize](dwgBufferW& body, std::uint32_t) {
            patchObjectSize(body, 0);
        });
    REQUIRE_FALSE(zeroSizeBytes.empty());
    assertRejected(zeroSizeBytes);

    const auto prefixCrossingBytes = makeBody(
        [&patchObjectSize](dwgBufferW& body, std::uint32_t modelerStartBit) {
            patchObjectSize(body, modelerStartBit);
        });
    REQUIRE_FALSE(prefixCrossingBytes.empty());
    assertRejected(prefixCrossingBytes);

    auto truncatedBytes = validBytes;
    truncatedBytes.pop_back();
    assertRejected(truncatedBytes);
}

TEST_CASE("DWG MESH clears reused topology state",
          "[dwg][safety]") {
    DwgMeshProbe mesh;
    mesh.version = 7;
    mesh.blendCrease = true;
    mesh.subdivisionLevel = 3;
    mesh.subdivVertices.emplace_back(1.0, 2.0, 3.0);
    mesh.vertices.emplace_back(4.0, 5.0, 6.0);
    mesh.faces.push_back({0, 1, 2});
    mesh.edges.emplace_back(0, 1);
    mesh.creases.push_back(0.5);
    mesh.unknown = 9;

    std::vector<std::uint8_t> empty;
    dwgBuffer buffer(empty.data(), empty.size());
    CHECK_FALSE(mesh.parseDwg(DRW::AC1027, &buffer, 0));
    CHECK(mesh.version == 2);
    CHECK_FALSE(mesh.blendCrease);
    CHECK(mesh.subdivisionLevel == 0);
    CHECK(mesh.subdivVertices.empty());
    CHECK(mesh.vertices.empty());
    CHECK(mesh.faces.empty());
    CHECK(mesh.edges.empty());
    CHECK(mesh.creases.empty());
    CHECK(mesh.unknown == 0);
}

TEST_CASE("DWG MESH rejects nested counts outside the object body",
          "[dwg][safety][mesh]") {
    const auto makeBody = [](int oversizedCount) {
        DwgMeshProbe mesh;
        mesh.handle = 0x300u;
        mesh.setObjectType(DRW_Mesh::kDwgClassNum);

        dwgBufferW body;
        if (!mesh.encodeDwgCommon(DRW::AC1018, &body))
            return std::vector<std::uint8_t>{};
        body.putBitShort(2); // mesh version
        body.putBit(false); // blend crease
        body.putBitLong(0); // subdivision level
        body.putBitLong(oversizedCount == 0 ? DRW_Mesh::kMaxMeshItems : 0);
        body.putBitLong(oversizedCount == 1 ? DRW_Mesh::kMaxMeshItems : 0);
        body.putBitLong(oversizedCount == 2 ? DRW_Mesh::kMaxMeshItems : 0);
        body.putBitLong(oversizedCount == 3 ? DRW_Mesh::kMaxMeshItems : 0);
        body.putBitLong(0); // trailing unknown
        if (!mesh.encodeDwgEntHandle(DRW::AC1018, &body))
            return std::vector<std::uint8_t>{};
        finalizeEncodedEntityBody(body);
        return body.data();
    };

    for (int oversizedCount = 0; oversizedCount < 4; ++oversizedCount) {
        const std::vector<std::uint8_t> bytes = makeBody(oversizedCount);
        REQUIRE_FALSE(bytes.empty());
        DwgMeshProbe mesh;
        dwgBuffer buffer(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        CHECK_FALSE(mesh.parseDwg(DRW::AC1018, &buffer, 0));
        CHECK(mesh.vertices.empty());
        CHECK(mesh.faces.empty());
        CHECK(mesh.edges.empty());
        CHECK(mesh.creases.empty());
    }
}

TEST_CASE("DWG MESH writer rejects invalid geometry before writing",
          "[dwg][write][safety][mesh]") {
    const auto rejectedWithoutPartialWrite = [](DwgMeshProbe& mesh) {
        dwgBufferW output;
        output.putRawChar8(0xA5);
        const std::vector<std::uint8_t> before = output.data();
        CHECK_FALSE(mesh.encodeDwg(DRW::AC1027, &output));
        CHECK(output.data() == before);
    };

    SECTION("non-finite vertex") {
        DwgMeshProbe mesh;
        mesh.handle = 0x304u;
        mesh.vertices = {DRW_Coord{
            std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0}};
        rejectedWithoutPartialWrite(mesh);
    }

    SECTION("non-finite crease") {
        DwgMeshProbe mesh;
        mesh.handle = 0x305u;
        mesh.creases = {std::numeric_limits<double>::infinity()};
        rejectedWithoutPartialWrite(mesh);
    }

    SECTION("face index outside vertex array") {
        DwgMeshProbe mesh;
        mesh.handle = 0x306u;
        mesh.vertices = {DRW_Coord{0.0, 0.0, 0.0},
                         DRW_Coord{1.0, 0.0, 0.0},
                         DRW_Coord{0.0, 1.0, 0.0}};
        mesh.faces = {{0, 1, 3}};
        rejectedWithoutPartialWrite(mesh);
    }

    SECTION("edge index outside vertex array") {
        DwgMeshProbe mesh;
        mesh.handle = 0x307u;
        mesh.vertices = {DRW_Coord{0.0, 0.0, 0.0}};
        mesh.edges = {{0, 1}};
        rejectedWithoutPartialWrite(mesh);
    }
}

TEST_CASE("DWG MESH rejects missing counts before detached strings",
          "[dwg][safety][mesh]") {
    DwgMeshProbe source;
    source.handle = 0x302u;
    source.setObjectType(DRW_Mesh::kDwgClassNum);

    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
    REQUIRE(source.encodeDwgCommon(DRW::AC1027, &body));
    body.putBitShort(2); // mesh version
    body.putBit(false); // blend crease
    body.putBitLong(0); // subdivision level
    while ((body.bitCount() & 7u) != 7u)
        body.putBit(false);
    body.alignToByte();
    const std::vector<std::uint8_t> marker = {0xD1u, 0xE2u, 0xF3u, 0x04u};
    strings.putBytes(marker.data(), marker.size());
    REQUIRE(DwgStringFooterWriterProbe::appendR2007StringStream(
        body, strings, true));
      REQUIRE(source.encodeDwgEntHandle(DRW::AC1027, &body, &handles));
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    body.putBytes(handles.data().data(), handles.data().size());

    DwgMeshProbe parsed;
    dwgBuffer buffer(body.data().data(), body.data().size());
    CHECK_FALSE(parsed.parseDwg(DRW::AC1027, &buffer, handleBits));
    CHECK(parsed.vertices.empty());
    CHECK(parsed.faces.empty());
    CHECK(parsed.edges.empty());
    CHECK(parsed.creases.empty());
}

TEST_CASE("DWG MESH does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto encoded = makeTruncatedMeshBody();
    REQUIRE(!encoded.first.empty());

    DwgMeshProbe mesh;
    mesh.version = 9;
    mesh.blendCrease = true;
    mesh.subdivisionLevel = 4;
    mesh.vertices.emplace_back(9.0, 9.0, 9.0);
    mesh.faces.push_back({0, 0, 0});
    mesh.edges.emplace_back(0, 0);
    mesh.creases.push_back(9.0);
    mesh.unknown = 9;

    dwgBuffer buffer(const_cast<std::uint8_t*>(encoded.first.data()),
                     encoded.first.size());
    CHECK_FALSE(mesh.parseDwg(DRW::AC1027, &buffer, encoded.second));
    CHECK_FALSE(buffer.isGood());
    CHECK(mesh.version == 2);
    CHECK_FALSE(mesh.blendCrease);
    CHECK(mesh.subdivisionLevel == 0);
    CHECK(mesh.vertices.empty());
    CHECK(mesh.faces.empty());
    CHECK(mesh.edges.empty());
    CHECK(mesh.creases.empty());
    CHECK(mesh.unknown == 0);
}

TEST_CASE("DWG PROXY_OBJECT excludes detached strings from opaque data",
          "[dwg][safety][proxy-object]") {
    dwgBufferW body;
    body.putObjType(DRW::AC1027, DRW_ProxyObject::kDwgType);
    dwgHandle objectHandle;
    objectHandle.code = 4;
    objectHandle.ref = 0x303u;
    body.putHandle(objectHandle);
    body.putBitShort(0); // no EED
    body.putBitLong(0); // no reactors
    body.putBit(false); // xdictionary handle follows
    body.putBit(false); // no R2013 binary data
    body.putBitLong(500); // proxy carrier id
    body.putBitLong(0x1234); // proxy drawing format
    body.putBit(true); // source was DXF
    body.putRawChar8(0xA5);
    body.putRawChar8(0x5A);

    dwgBufferW strings;
    strings.putVariableText(DRW::AC1027, "");
    const std::vector<std::uint8_t> marker = {0xD1u, 0xE2u, 0xF3u, 0x04u};
    strings.putBytes(marker.data(), marker.size());
    strings.alignToByte();
    body.alignToByte();
    REQUIRE(DwgStringFooterWriterProbe::appendR2007StringStream(
        body, strings, true));

    dwgBufferW handles;
    dwgHandle nullHandle;
    handles.putHandle(nullHandle); // owner
    handles.putHandle(nullHandle); // xdictionary
    handles.putHandle(nullHandle); // end of proxy object-id references
    handles.alignToByte();
    const std::uint32_t handleBits = handles.bitCount();
    body.putBytes(handles.data().data(), handles.data().size());

    DwgProxyObjectProbe parsed;
    dwgBuffer buffer(body.data().data(), body.data().size());
    REQUIRE(parsed.parseDwg(DRW::AC1027, &buffer, handleBits));
    CHECK(parsed.m_proxySubclass.empty());
    CHECK(parsed.m_objectDataBitSize >= 16);
    REQUIRE(parsed.m_objectData.size() >= 2);
    CHECK(parsed.m_objectData[0] == 0xA5u);
    CHECK(parsed.m_objectData[1] == 0x5Au);
    CHECK(std::search(parsed.m_objectData.cbegin(), parsed.m_objectData.cend(),
                      marker.cbegin(), marker.cend())
          == parsed.m_objectData.cend());
}

TEST_CASE("DWG HATCH and MPOLYGON clear reused body state",
          "[dwg][safety]") {
    std::vector<std::uint8_t> empty;

    DwgHatchProbe hatch;
    hatch.name = "STALE";
    hatch.loopsnum = 3;
    hatch.patternLines.push_back(DRW_Hatch::PatternLine{});
    hatch.looplist.push_back(std::make_shared<DRW_HatchLoop>(2));
    hatch.gradColors.push_back(DRW_Hatch::GradientStop{});
    hatch.seedPoints.emplace_back(1.0, 2.0, 0.0);
    hatch.pixelSize = 9.0;
    dwgBuffer hatchBuffer(empty.data(), empty.size());
    CHECK_FALSE(hatch.parseDwg(DRW::AC1027, &hatchBuffer, 0));
    CHECK(hatch.name.empty());
    CHECK(hatch.loopsnum == 0);
    CHECK(hatch.patternLines.empty());
    CHECK(hatch.looplist.empty());
    CHECK(hatch.gradColors.empty());
    CHECK(hatch.seedPoints.empty());
    CHECK(hatch.pixelSize == 0.0);

    DwgMPolygonProbe polygon;
    polygon.fillColorAci = 7;
    polygon.fillColorRgb = 0x112233;
    polygon.fillColorName = "STALE";
    polygon.xDirX = 4.0;
    polygon.xDirY = 5.0;
    polygon.degenerateLoops = 6;
    polygon.looplist.push_back(std::make_shared<DRW_HatchLoop>(2));
    dwgBuffer polygonBuffer(empty.data(), empty.size());
    CHECK_FALSE(polygon.parseDwg(DRW::AC1027, &polygonBuffer, 0));
    CHECK(polygon.fillColorAci == 0);
    CHECK(polygon.fillColorRgb == -1);
    CHECK(polygon.fillColorName.empty());
    CHECK(polygon.xDirX == 0.0);
    CHECK(polygon.xDirY == 0.0);
    CHECK(polygon.degenerateLoops == 0);
    CHECK(polygon.looplist.empty());
}

TEST_CASE("DWG LWPOLYLINE and MLINE publish fresh body state",
          "[dwg][safety]") {
    std::vector<std::uint8_t> empty;

    DwgLwPolylineProbe polyline;
    polyline.vertexnum = 4;
    polyline.flags = 1;
    polyline.width = 2.0;
    polyline.elevation = 3.0;
    polyline.thickness = 4.0;
    polyline.extPoint = DRW_Coord{5.0, 6.0, 7.0};
    polyline.vertlist.push_back(std::make_shared<DRW_Vertex2D>());
    polyline.vertex = polyline.vertlist.front();
    dwgBuffer polylineBuffer(empty.data(), empty.size());
    CHECK_FALSE(polyline.parseDwg(DRW::AC1027, &polylineBuffer, 0));
    CHECK(polyline.vertexnum == 0);
    CHECK(polyline.flags == 0);
    CHECK(polyline.width == 0.0);
    CHECK(polyline.elevation == 0.0);
    CHECK(polyline.thickness == 0.0);
    CHECK(polyline.extPoint.x == 0.0);
    CHECK(polyline.extPoint.y == 0.0);
    CHECK(polyline.extPoint.z == 1.0);
    CHECK(polyline.vertlist.empty());
    CHECK(polyline.vertex == nullptr);

    DwgMLineProbe mline;
    mline.layer = "STALE_LAYER";
    mline.handle = 0xB80;
    mline.scale = 8.0;
    mline.justification = 2;
    mline.basePoint = DRW_Coord{9.0, 10.0, 11.0};
    mline.extPoint = DRW_Coord{12.0, 13.0, 14.0};
    mline.openClosed = 0;
    mline.numLines = 3;
    mline.numVerts = 2;
    mline.styleName = "STALE";
    mline.styleHandle = 0xB90;
    mline.vertlist.emplace_back();
    dwgBuffer mlineBuffer(empty.data(), empty.size());
    CHECK_FALSE(mline.parseDwg(DRW::AC1027, &mlineBuffer, 0));
    CHECK(mline.scale == 1.0);
    CHECK(mline.layer == "0");
    CHECK(mline.handle == DRW::NoHandle);
    CHECK(mline.justification == 0);
    CHECK(mline.basePoint.x == 0.0);
    CHECK(mline.basePoint.y == 0.0);
    CHECK(mline.basePoint.z == 0.0);
    CHECK(mline.extPoint.x == 0.0);
    CHECK(mline.extPoint.y == 0.0);
    CHECK(mline.extPoint.z == 1.0);
    CHECK(mline.openClosed == 1);
    CHECK(mline.numLines == 0);
    CHECK(mline.numVerts == 0);
    CHECK(mline.styleName.empty());
    CHECK(mline.styleHandle == 0);
    CHECK(mline.vertlist.empty());
}

TEST_CASE("DWG sequence entities clear reused body state",
          "[dwg][safety]") {
    std::vector<std::uint8_t> empty;

    DwgInsertProbe insert;
    insert.layer = "STALE_LAYER";
    insert.handle = 0xA80;
    insert.name = "STALE";
    insert.xscale = 2.0;
    insert.yscale = 3.0;
    insert.zscale = 4.0;
    insert.angle = 5.0;
    insert.colcount = 6;
    insert.rowcount = 7;
    insert.colspace = 8.0;
    insert.rowspace = 9.0;
    insert.attribHandles.push_back(dwgHandle{});
    insert.attlist.push_back(std::make_shared<DRW_Attrib>());
    dwgBuffer insertBuffer(empty.data(), empty.size());
    CHECK_FALSE(insert.parseDwg(DRW::AC1027, &insertBuffer, 0));
    CHECK(insert.name.empty());
    CHECK(insert.layer == "0");
    CHECK(insert.handle == DRW::NoHandle);
    CHECK(insert.xscale == 1.0);
    CHECK(insert.yscale == 1.0);
    CHECK(insert.zscale == 1.0);
    CHECK(insert.angle == 0.0);
    CHECK(insert.colcount == 1);
    CHECK(insert.rowcount == 1);
    CHECK(insert.colspace == 0.0);
    CHECK(insert.rowspace == 0.0);
    CHECK(insert.attribHandles.empty());
    CHECK(insert.attlist.empty());

    DwgPolylineProbe polyline;
    polyline.flags = 12;
    polyline.defstawidth = 1.0;
    polyline.defendwidth = 2.0;
    polyline.vertexcount = 3;
    polyline.facecount = 4;
    polyline.smoothM = 5;
    polyline.smoothN = 6;
    polyline.curvetype = 7;
    polyline.vertlist.push_back(std::make_shared<DRW_Vertex>());
    dwgBuffer polylineBuffer(empty.data(), empty.size());
    CHECK_FALSE(polyline.parseDwg(DRW::AC1027, &polylineBuffer, 0));
    CHECK(polyline.flags == 0);
    CHECK(polyline.defstawidth == 0.0);
    CHECK(polyline.defendwidth == 0.0);
    CHECK(polyline.vertexcount == 0);
    CHECK(polyline.facecount == 0);
    CHECK(polyline.smoothM == 0);
    CHECK(polyline.smoothN == 0);
    CHECK(polyline.curvetype == 0);
    CHECK(polyline.vertlist.empty());

    DwgVertexProbe vertex;
    vertex.stawidth = 1.0;
    vertex.endwidth = 2.0;
    vertex.bulge = 3.0;
    vertex.flags = 4;
    vertex.tgdir = 5.0;
    vertex.vindex1 = 6;
    vertex.vindex2 = 7;
    vertex.vindex3 = 8;
    vertex.vindex4 = 9;
    vertex.identifier = 10;
    vertex.setDwgSubtype(DRW_Vertex::DwgSubtype::Mesh);
    dwgBuffer vertexBuffer(empty.data(), empty.size());
    CHECK_FALSE(vertex.parseDwg(DRW::AC1027, &vertexBuffer, 0, 0.0));
    CHECK(vertex.stawidth == 0.0);
    CHECK(vertex.endwidth == 0.0);
    CHECK(vertex.bulge == 0.0);
    CHECK(vertex.flags == 0);
    CHECK(vertex.tgdir == 0.0);
    CHECK(vertex.vindex1 == 0);
    CHECK(vertex.vindex2 == 0);
    CHECK(vertex.vindex3 == 0);
    CHECK(vertex.vindex4 == 0);
    CHECK(vertex.identifier == 0);
    CHECK(vertex.dwgSubtype() == DRW_Vertex::DwgSubtype::Auto);

    DwgLeaderProbe leader;
    leader.layer = "STALE_LAYER";
    leader.handle = 0xBA1;
    leader.style = "STALE";
    leader.arrow = 0;
    leader.leadertype = 1;
    leader.flag = 2;
    leader.hookline = 0;
    leader.hookflag = 0;
    leader.textheight = 4.0;
    leader.textwidth = 5.0;
    leader.vertnum = 6;
    leader.coloruse = 7;
    leader.annotHandle = 0xBA0;
    leader.vertexlist.push_back(std::make_shared<DRW_Coord>());
    dwgBuffer leaderBuffer(empty.data(), empty.size());
    CHECK_FALSE(leader.parseDwg(DRW::AC1027, &leaderBuffer, 0));
    CHECK(leader.style.empty());
    CHECK(leader.layer == "0");
    CHECK(leader.handle == DRW::NoHandle);
    CHECK(leader.arrow == 1);
    CHECK(leader.leadertype == 0);
    CHECK(leader.flag == 3);
    CHECK(leader.hookline == 1);
    CHECK(leader.hookflag == 1);
    CHECK(leader.textheight == 1.0);
    CHECK(leader.textwidth == 1.0);
    CHECK(leader.vertnum == 0);
    CHECK(leader.coloruse == 7);
    CHECK(leader.annotHandle == 0);
    CHECK(leader.vertexlist.empty());
}

TEST_CASE("DWG POLYLINE and VERTEX body parsing is transactional",
          "[dwg][safety][compound]") {
    const auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());

    dwgBuffer frameBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    DwgObjectFrame polylineFrame;
    REQUIRE(polylineFrame.readAt(
        frameBuffer, DRW::AC1018, fixture.polylineOffset));
    REQUIRE(polylineFrame.body().size() > 1);
    std::vector<std::uint8_t> truncatedPolylineBody = polylineFrame.body();
    truncatedPolylineBody.pop_back();

    DwgPolylineProbe polyline;
    polyline.flags = 12;
    polyline.defstawidth = 4.0;
    dwgBuffer polylineBuffer(truncatedPolylineBody.data(),
                             truncatedPolylineBody.size());
    CHECK_FALSE(polyline.parseDwg(
        DRW::AC1018, &polylineBuffer, polylineFrame.bodyBitSize()));
    CHECK(polylineBuffer.getPosition() == 0);
    CHECK(polyline.flags == 0);
    CHECK(polyline.defstawidth == 0.0);
    CHECK(polyline.vertlist.empty());

    dwgBuffer vertexFrameBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    DwgObjectFrame vertexFrame;
    REQUIRE(vertexFrame.readAt(
        vertexFrameBuffer, DRW::AC1018, fixture.vertexOffset));
    REQUIRE(vertexFrame.body().size() > 1);
    std::vector<std::uint8_t> truncatedVertexBody = vertexFrame.body();
    truncatedVertexBody.pop_back();

    DwgVertexProbe vertex;
    vertex.flags = 4;
    vertex.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    dwgBuffer vertexBuffer(truncatedVertexBody.data(),
                           truncatedVertexBody.size());
    CHECK_FALSE(vertex.parseDwg(
        DRW::AC1018, &vertexBuffer, vertexFrame.bodyBitSize(), 0.0));
    CHECK(vertexBuffer.getPosition() == 0);
    CHECK(vertex.flags == 0);
    CHECK(vertex.basePoint.x == 0.0);
    CHECK(vertex.basePoint.y == 0.0);
    CHECK(vertex.basePoint.z == 0.0);
    CHECK(vertex.dwgSubtype() == DRW_Vertex::DwgSubtype::Auto);
}

TEST_CASE("DWG dimensions clear reused body state", "[dwg][safety]") {
    std::array<std::uint8_t, 0> empty{};

    DwgDimensionProbe dimension;
    dimension.type = 255;
    dimension.setName("STALE_BLOCK");
    dimension.setDefPoint(DRW_Coord{1.0, 2.0, 3.0});
    dimension.setTextPoint(DRW_Coord{4.0, 5.0, 6.0});
    dimension.setText("STALE_TEXT");
    dimension.setStyle("STALE_STYLE");
    dimension.setAlign(9);
    dimension.setTextLineStyle(2);
    dimension.setTextLineFactor(2.0);
    dimension.setDir(3.0);
    dimension.setHDir(4.0);
    dimension.setExtrusion(DRW_Coord{7.0, 8.0, 9.0});
    dimension.setFlipArrow1(true);
    dimension.setFlipArrow2(true);
    dwgBuffer dimensionBuffer(empty.data(), empty.size());
    CHECK_FALSE(dimension.parseDwg(DRW::AC1027, &dimensionBuffer, 0));
    CHECK(dimension.type == 0);
    CHECK(dimension.getName().empty());
    CHECK(dimension.getDefPoint().x == 0.0);
    CHECK(dimension.getDefPoint().y == 0.0);
    CHECK(dimension.getDefPoint().z == 0.0);
    CHECK(dimension.getTextPoint().x == 0.0);
    CHECK(dimension.getTextPoint().y == 0.0);
    CHECK(dimension.getTextPoint().z == 0.0);
    CHECK(dimension.getText().empty());
    CHECK(dimension.getStyle() == "STANDARD");
    CHECK(dimension.getAlign() == 5);
    CHECK(dimension.getTextLineStyle() == 1);
    CHECK(dimension.getTextLineFactor() == 1.0);
    CHECK(dimension.getDir() == 0.0);
    CHECK(dimension.getHDir() == 0.0);
    CHECK(dimension.getExtrusion().x == 0.0);
    CHECK(dimension.getExtrusion().y == 0.0);
    CHECK(dimension.getExtrusion().z == 1.0);
    CHECK_FALSE(dimension.getFlipArrow1());
    CHECK_FALSE(dimension.getFlipArrow2());
    CHECK(dimension.getMeasureValue() == 0.0);

    DwgLargeRadialDimensionProbe largeRadial;
    largeRadial.overrideCenterPoint = DRW_Coord{1.0, 2.0, 3.0};
    largeRadial.jogPoint = DRW_Coord{4.0, 5.0, 6.0};
    largeRadial.jogAngle = 7.0;
    dwgBuffer largeRadialBuffer(empty.data(), empty.size());
    CHECK_FALSE(largeRadial.parseDwg(DRW::AC1027, &largeRadialBuffer, 0));
    CHECK(largeRadial.overrideCenterPoint.x == 0.0);
    CHECK(largeRadial.overrideCenterPoint.y == 0.0);
    CHECK(largeRadial.overrideCenterPoint.z == 0.0);
    CHECK(largeRadial.jogPoint.x == 0.0);
    CHECK(largeRadial.jogPoint.y == 0.0);
    CHECK(largeRadial.jogPoint.z == 0.0);
    CHECK(largeRadial.jogAngle == 0.0);

    DwgArcDimensionProbe arc;
    arc.leaderPt2 = DRW_Coord{1.0, 2.0, 3.0};
    arc.arcStartAngle = 4.0;
    arc.arcEndAngle = 5.0;
    arc.arcSymbol = 6;
    arc.isPartial = true;
    arc.hasLeader = true;
    dwgBuffer arcBuffer(empty.data(), empty.size());
    CHECK_FALSE(arc.parseDwg(DRW::AC1027, &arcBuffer, 0));
    CHECK(arc.leaderPt2.x == 0.0);
    CHECK(arc.leaderPt2.y == 0.0);
    CHECK(arc.leaderPt2.z == 0.0);
    CHECK(arc.arcStartAngle == 0.0);
    CHECK(arc.arcEndAngle == 0.0);
    CHECK(arc.arcSymbol == 0);
    CHECK_FALSE(arc.isPartial);
    CHECK_FALSE(arc.hasLeader);
}

TEST_CASE("DWG aligned dimensions do not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedAlignedDimensionBody();
    REQUIRE(!body.empty());

    DwgDimensionProbe dimension;
    dimension.setText("STALE_TEXT");
    dimension.setDimPoint(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setDef1Point(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setDef2Point(DRW_Coord{8.0, 9.0, 0.0});
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(dimension.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(dimension.getText().empty());
    CHECK(dimension.getDimPoint().x == 0.0);
    CHECK(dimension.getDef1Point().x == 0.0);
    CHECK(dimension.getDef2Point().x == 0.0);
}

TEST_CASE("DWG radial dimensions do not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedRadialDimensionBody();
    REQUIRE(!body.empty());

    DwgRadialDimensionProbe dimension;
    dimension.setCenterPoint(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setDiameterPoint(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setLeaderLength(8.0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(dimension.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(dimension.getCenterPoint().x == 0.0);
    CHECK(dimension.getDiameterPoint().x == 0.0);
    CHECK(dimension.getLeaderLength() == 0.0);
}

TEST_CASE("DWG large radial dimensions do not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedLargeRadialDimensionBody();
    REQUIRE(!body.empty());

    DwgLargeRadialDimensionProbe dimension;
    dimension.setCenterPoint(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setChordPoint(DRW_Coord{8.0, 9.0, 0.0});
    dimension.jogPoint = DRW_Coord{8.0, 9.0, 0.0};
    dimension.overrideCenterPoint = DRW_Coord{8.0, 9.0, 0.0};
    dimension.jogAngle = 8.0;
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(dimension.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(dimension.getCenterPoint().x == 0.0);
    CHECK(dimension.getChordPoint().x == 0.0);
    CHECK(dimension.jogPoint.x == 0.0);
    CHECK(dimension.overrideCenterPoint.x == 0.0);
    CHECK(dimension.jogAngle == 0.0);
}

TEST_CASE("DWG diametric dimensions do not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedDiametricDimensionBody();
    REQUIRE(!body.empty());

    DwgDiametricDimensionProbe dimension;
    dimension.setDiameter1Point(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setDiameter2Point(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setLeaderLength(8.0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(dimension.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(dimension.getDiameter1Point().x == 0.0);
    CHECK(dimension.getDiameter2Point().x == 0.0);
    CHECK(dimension.getLeaderLength() == 0.0);
}

TEST_CASE("DWG angular dimensions do not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedAngularDimensionBody();
    REQUIRE(!body.empty());

    DwgAngularDimensionProbe dimension;
    dimension.setDimPoint(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setFirstLine1(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setFirstLine2(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setSecondLine1(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setSecondLine2(DRW_Coord{8.0, 9.0, 0.0});
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(dimension.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(dimension.getDimPoint().x == 0.0);
    CHECK(dimension.getFirstLine1().x == 0.0);
    CHECK(dimension.getSecondLine2().x == 0.0);
}

TEST_CASE("DWG three-point angular dimensions do not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedAngular3pDimensionBody();
    REQUIRE(!body.empty());

    DwgAngular3pDimensionProbe dimension;
    dimension.setDimPoint(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setFirstLine(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setSecondLine(DRW_Coord{8.0, 9.0, 0.0});
    dimension.SetVertexPoint(DRW_Coord{8.0, 9.0, 0.0});
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(dimension.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(dimension.getDimPoint().x == 0.0);
    CHECK(dimension.getFirstLine().x == 0.0);
    CHECK(dimension.getVertexPoint().x == 0.0);
}

TEST_CASE("DWG ordinate dimensions do not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedOrdinateDimensionBody();
    REQUIRE(!body.empty());

    DwgOrdinateDimensionProbe dimension;
    dimension.setOriginPoint(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setFirstLine(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setSecondLine(DRW_Coord{8.0, 9.0, 0.0});
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(dimension.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(dimension.getOriginPoint().x == 0.0);
    CHECK(dimension.getFirstLine().x == 0.0);
    CHECK(dimension.getSecondLine().x == 0.0);
}

TEST_CASE("DWG arc dimensions do not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedArcDimensionBody();
    REQUIRE(!body.empty());

    DwgArcDimensionProbe dimension;
    dimension.setArcDefPoint(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setExtLine1(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setExtLine2(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setArcCenter(DRW_Coord{8.0, 9.0, 0.0});
    dimension.setLeaderPt1(DRW_Coord{8.0, 9.0, 0.0});
    dimension.leaderPt2 = DRW_Coord{8.0, 9.0, 0.0};
    dimension.arcStartAngle = 8.0;
    dimension.arcEndAngle = 9.0;
    dimension.arcSymbol = 10;
    dimension.isPartial = true;
    dimension.hasLeader = true;
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(dimension.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(dimension.getArcDefPoint().x == 0.0);
    CHECK(dimension.getArcCenter().x == 0.0);
    CHECK(dimension.leaderPt2.x == 0.0);
    CHECK(dimension.arcStartAngle == 0.0);
    CHECK(dimension.arcEndAngle == 0.0);
    CHECK(dimension.arcSymbol == 0);
    CHECK_FALSE(dimension.isPartial);
    CHECK_FALSE(dimension.hasLeader);
}

TEST_CASE("DWG text carriers clear reused body state", "[dwg][safety]") {
    std::array<std::uint8_t, 0> empty{};

    DwgTextProbe text;
    text.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    text.secPoint = DRW_Coord{4.0, 5.0, 6.0};
    text.thickness = 7.0;
    text.extPoint = DRW_Coord{8.0, 9.0, 10.0};
    text.height = 11.0;
    text.text = "STALE_TEXT";
    text.angle = 12.0;
    text.widthscale = 13.0;
    text.oblique = 14.0;
    text.style = "STALE_STYLE";
    text.textgen = 15;
    text.alignH = DRW_Text::HRight;
    text.alignV = DRW_Text::VTop;
    dwgBuffer textBuffer(empty.data(), empty.size());
    CHECK_FALSE(text.parseDwg(DRW::AC1027, &textBuffer, 0));
    CHECK(text.basePoint.x == 0.0);
    CHECK(text.basePoint.y == 0.0);
    CHECK(text.basePoint.z == 0.0);
    CHECK(text.secPoint.x == 0.0);
    CHECK(text.secPoint.y == 0.0);
    CHECK(text.secPoint.z == 0.0);
    CHECK(text.thickness == 0.0);
    CHECK(text.extPoint.x == 0.0);
    CHECK(text.extPoint.y == 0.0);
    CHECK(text.extPoint.z == 1.0);
    CHECK(text.height == 0.0);
    CHECK(text.text.empty());
    CHECK(text.angle == 0.0);
    CHECK(text.widthscale == 1.0);
    CHECK(text.oblique == 0.0);
    CHECK(text.style == "STANDARD");
    CHECK(text.textgen == 0);
    CHECK(text.alignH == DRW_Text::HLeft);
    CHECK(text.alignV == DRW_Text::VBaseLine);

    DwgAttribProbe attrib;
    attrib.tag = "STALE_TAG";
    attrib.attribFlags = 15;
    attrib.m_fieldLength = 16;
    attrib.lockPosition = true;
    attrib.attVersion = 17;
    attrib.m_attributeType = 4;
    attrib.keepDuplicateRecords = 18;
    attrib.mtext = std::make_unique<DRW_MText>();
    dwgBuffer attribBuffer(empty.data(), empty.size());
    CHECK_FALSE(attrib.parseDwg(DRW::AC1027, &attribBuffer, 0));
    CHECK(attrib.tag.empty());
    CHECK(attrib.attribFlags == 0);
    CHECK(attrib.m_fieldLength == 0);
    CHECK_FALSE(attrib.lockPosition);
    CHECK(attrib.attVersion == 0);
    CHECK(attrib.m_attributeType == 1);
    CHECK(attrib.keepDuplicateRecords == 0);
    CHECK(attrib.mtext == nullptr);

    DwgAttdefProbe attdef;
    attdef.prompt = "STALE_PROMPT";
    dwgBuffer attdefBuffer(empty.data(), empty.size());
    CHECK_FALSE(attdef.parseDwg(DRW::AC1027, &attdefBuffer, 0));
    CHECK(attdef.prompt.empty());

    DwgMTextProbe mtext;
    mtext.interlin = 19.0;
    mtext.linespacingStyle = 2;
    mtext.m_backgroundFlags = 3;
    mtext.m_backgroundScale = 20.0;
    mtext.m_backgroundColor = 21;
    mtext.m_backgroundTransparency = 22;
    mtext.text = "STALE_MTEXT";
    mtext.m_r2018ColumnHeights.push_back(23.0);
    mtext.m_r2018AnnotativeData.push_back(24);
    dwgBuffer mtextBuffer(empty.data(), empty.size());
    CHECK_FALSE(mtext.parseDwg(DRW::AC1027, &mtextBuffer, 0));
    CHECK(mtext.interlin == 1.0);
    CHECK(mtext.linespacingStyle == 1);
    CHECK(mtext.m_backgroundFlags == 0);
    CHECK(mtext.m_backgroundScale == 0.0);
    CHECK(mtext.m_backgroundColor == 0);
    CHECK(mtext.m_backgroundTransparency == 0);
    CHECK(mtext.text.empty());
    CHECK(mtext.m_r2018ColumnHeights.empty());
    CHECK(mtext.m_r2018AnnotativeData.empty());
}

TEST_CASE("DWG spline carriers clear reused body state", "[dwg][safety]") {
    std::array<std::uint8_t, 0> empty{};

    DwgSplineProbe spline;
    spline.normalVec = DRW_Coord{1.0, 2.0, 3.0};
    spline.tgStart = DRW_Coord{4.0, 5.0, 6.0};
    spline.tgEnd = DRW_Coord{7.0, 8.0, 9.0};
    spline.flags = 10;
    spline.degree = 11;
    spline.m_scenario = 12;
    spline.m_splineFlags1 = 13;
    spline.m_knotParam = 14;
    spline.nknots = 15;
    spline.ncontrol = 16;
    spline.nfit = 17;
    spline.tolknot = 18.0;
    spline.tolcontrol = 19.0;
    spline.tolfit = 20.0;
    spline.knotslist.push_back(21.0);
    spline.weightlist.push_back(22.0);
    spline.controllist.push_back(std::make_shared<DRW_Coord>());
    spline.fitlist.push_back(std::make_shared<DRW_Coord>());
    dwgBuffer splineBuffer(empty.data(), empty.size());
    CHECK_FALSE(spline.parseDwg(DRW::AC1027, &splineBuffer, 0));
    CHECK(spline.normalVec.x == 0.0);
    CHECK(spline.normalVec.y == 0.0);
    CHECK(spline.normalVec.z == 0.0);
    CHECK(spline.tgStart.x == 0.0);
    CHECK(spline.tgEnd.y == 0.0);
    CHECK(spline.flags == 0);
    CHECK(spline.degree == 0);
    CHECK(spline.m_scenario == 0);
    CHECK(spline.m_splineFlags1 == 0);
    CHECK(spline.m_knotParam == 15);
    CHECK(spline.nknots == 0);
    CHECK(spline.ncontrol == 0);
    CHECK(spline.nfit == 0);
    CHECK(spline.tolknot == 0.0000001);
    CHECK(spline.tolcontrol == 0.0000001);
    CHECK(spline.tolfit == 0.0000001);
    CHECK(spline.knotslist.empty());
    CHECK(spline.weightlist.empty());
    CHECK(spline.controllist.empty());
    CHECK(spline.fitlist.empty());

    DwgHelixProbe helix;
    helix.m_majorVersion = 23;
    helix.m_maintVersion = 24;
    helix.axisBasePt = DRW_Coord{1.0, 2.0, 3.0};
    helix.startPt = DRW_Coord{4.0, 5.0, 6.0};
    helix.axisVector = DRW_Coord{7.0, 8.0, 9.0};
    helix.radius = 25.0;
    helix.turns = 26.0;
    helix.turnHeight = 27.0;
    helix.handedness = true;
    helix.constraintType = 28;
    helix.controllist.push_back(std::make_shared<DRW_Coord>());
    dwgBuffer helixBuffer(empty.data(), empty.size());
    CHECK_FALSE(helix.parseDwg(DRW::AC1027, &helixBuffer, 0));
    CHECK(helix.m_majorVersion == 0);
    CHECK(helix.m_maintVersion == 0);
    CHECK(helix.axisBasePt.x == 0.0);
    CHECK(helix.startPt.y == 0.0);
    CHECK(helix.axisVector.z == 0.0);
    CHECK(helix.radius == 0.0);
    CHECK(helix.turns == 0.0);
    CHECK(helix.turnHeight == 0.0);
    CHECK_FALSE(helix.handedness);
    CHECK(helix.constraintType == 0);
    CHECK(helix.controllist.empty());
}

TEST_CASE("DWG SPLINE nested counts honor the object body", "[dwg][safety]") {
    auto oversizedKnots = makeMalformedSplineFrame(1000000, 0);
    REQUIRE_FALSE(oversizedKnots.empty());
    dwgBuffer knotBuffer(oversizedKnots.data(), oversizedKnots.size());
    DwgSplineProbe knotSpline;
    CHECK_FALSE(knotSpline.parseDwg(DRW::AC1018, &knotBuffer, 0));

    auto oversizedControls = makeMalformedSplineFrame(1, 1000000);
    REQUIRE_FALSE(oversizedControls.empty());
    dwgBuffer controlBuffer(oversizedControls.data(), oversizedControls.size());
    DwgSplineProbe controlSpline;
    CHECK_FALSE(controlSpline.parseDwg(DRW::AC1018, &controlBuffer, 0));

    auto oversizedFitPoints = makeMalformedFitSplineBody(1000000);
    REQUIRE_FALSE(oversizedFitPoints.empty());
    dwgBuffer fitBuffer(oversizedFitPoints.data(), oversizedFitPoints.size());
    DwgSplineProbe fitSpline;
    CHECK_FALSE(fitSpline.parseDwg(DRW::AC1027, &fitBuffer, 0));
}

TEST_CASE("DWG SPLINE writer rejects invalid payloads transactionally",
          "[dwg][safety][writer]") {
    const auto checkRejected = [](DwgSplineProbe spline) {
        dwgBufferW buffer;
        buffer.putRawChar8(0xA5);
        const auto before = buffer.data();
        CHECK_FALSE(spline.encodeDwg(DRW::AC1018, &buffer, 0, nullptr,
                                     nullptr));
        CHECK(buffer.data() == before);
    };

    DwgSplineProbe nullControl;
    nullControl.degree = 1;
    nullControl.knotslist = {0.0, 0.0, 1.0, 1.0};
    nullControl.controllist.push_back(nullptr);
    checkRejected(std::move(nullControl));

    DwgSplineProbe nonFiniteTolerance;
    nonFiniteTolerance.degree = 1;
    nonFiniteTolerance.knotslist = {0.0, 0.0, 1.0, 1.0};
    nonFiniteTolerance.controllist = {
        std::make_shared<DRW_Coord>(0.0, 0.0, 0.0),
        std::make_shared<DRW_Coord>(1.0, 1.0, 0.0)};
    nonFiniteTolerance.tolknot = std::numeric_limits<double>::quiet_NaN();
    checkRejected(std::move(nonFiniteTolerance));

    DwgSplineProbe invalidDegree;
    invalidDegree.degree = 0;
    checkRejected(std::move(invalidDegree));
}

TEST_CASE("DWG SPLINE does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedSplineBody();
    REQUIRE(!body.empty());

    DwgSplineProbe spline;
    spline.degree = 9;
    spline.knotslist.push_back(8.0);
    spline.controllist.push_back(std::make_shared<DRW_Coord>(8.0, 9.0, 0.0));
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(spline.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(spline.degree == 0);
    CHECK(spline.knotslist.empty());
    CHECK(spline.controllist.empty());
}

TEST_CASE("DWG SPLINE rejects a truncated payload before handles",
          "[dwg][safety]") {
    const auto body = makeTruncatedSplinePayloadBody();
    REQUIRE(!body.empty());

    DwgSplineProbe spline;
    spline.degree = 9;
    spline.knotslist.push_back(8.0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(spline.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(spline.degree == 0);
    CHECK(spline.knotslist.empty());
    CHECK(spline.controllist.empty());
}

TEST_CASE("DWG HELIX does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedHelixBody();
    REQUIRE(!body.empty());

    DwgHelixProbe helix;
    helix.m_majorVersion = 9;
    helix.radius = 8.0;
    helix.turns = 9.0;
    helix.turnHeight = 10.0;
    helix.knotslist.push_back(8.0);
    helix.controllist.push_back(std::make_shared<DRW_Coord>(8.0, 9.0, 0.0));
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(helix.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(helix.m_majorVersion == 0);
    CHECK(helix.radius == 0.0);
    CHECK(helix.turns == 0.0);
    CHECK(helix.turnHeight == 0.0);
    CHECK(helix.knotslist.empty());
    CHECK(helix.controllist.empty());
}

TEST_CASE("DWG HATCH variable counts honor the object body",
          "[dwg][safety]") {
    for (const int field : {0, 1, 2}) {
        auto bytes = makeMalformedHatchCountBody(field);
        REQUIRE_FALSE(bytes.empty());
        dwgBuffer buffer(bytes.data(), bytes.size());
        DwgHatchProbe hatch;
        hatch.hstyle = 9;
        hatch.name = "STALE";
        CHECK_FALSE(hatch.parseDwg(DRW::AC1018, &buffer, 0));
        CHECK(buffer.getPosition() == 0);
        CHECK(buffer.getBitPos() == 0);
        CHECK(hatch.hstyle == 0);
        CHECK(hatch.name.empty());
        CHECK(hatch.gradColors.empty());
        CHECK(hatch.patternLines.empty());
        CHECK(hatch.seedPoints.empty());
    }

    auto polygonBytes = makeMalformedMPolygonGradientBody();
    REQUIRE_FALSE(polygonBytes.empty());
    dwgBuffer polygonBuffer(polygonBytes.data(), polygonBytes.size());
    DwgMPolygonProbe polygon;
    polygon.hstyle = 9;
    polygon.fillColorAci = 7;
    CHECK_FALSE(polygon.parseDwg(DRW::AC1018, &polygonBuffer, 0));
    CHECK(polygonBuffer.getPosition() == 0);
    CHECK(polygonBuffer.getBitPos() == 0);
    CHECK(polygon.hstyle == 0);
    CHECK(polygon.fillColorAci == 0);
    CHECK(polygon.gradColors.empty());
}

TEST_CASE("DWG LWPOLYLINE nested counts honor the object body",
          "[dwg][safety]") {
    const std::array<std::pair<DRW::Version, int>, 4> cases = {{
        {DRW::AC1018, 0}, // vertex count
        {DRW::AC1018, 1}, // bulge count
        {DRW::AC1027, 2}, // vertex-ID count
        {DRW::AC1018, 3}, // width count
    }};
    for (const auto& testCase : cases) {
        auto bytes = makeMalformedLwPolylineBody(testCase.first,
                                                  testCase.second);
        REQUIRE_FALSE(bytes.empty());
        dwgBuffer buffer(bytes.data(), bytes.size());
        DwgLwPolylineProbe polyline;
        polyline.vertexnum = 8;
        polyline.width = 9.0;
        polyline.flags = 1;
        CHECK_FALSE(polyline.parseDwg(testCase.first, &buffer, 0));
        CHECK(buffer.getPosition() == 0);
        CHECK(buffer.getBitPos() == 0);
        CHECK(polyline.vertexnum == 0);
        CHECK(polyline.width == 0.0);
        CHECK(polyline.flags == 0);
        CHECK(polyline.extPoint.z == 1.0);
        CHECK(polyline.vertlist.empty());
    }
}

TEST_CASE("DWG LEADER point count honors the object body", "[dwg][safety]") {
    for (const DRW::Version version : {DRW::AC1018, DRW::AC1024}) {
        auto bytes = makeMalformedLeaderBody(version);
        REQUIRE_FALSE(bytes.empty());
        dwgBuffer buffer(bytes.data(), bytes.size());
        DwgLeaderProbe leader;
        leader.leadertype = 9;
        leader.textheight = 8.0;
        CHECK_FALSE(leader.parseDwg(version, &buffer, 0));
        CHECK(buffer.getPosition() == 0);
        CHECK(buffer.getBitPos() == 0);
        CHECK(leader.leadertype == 0);
        CHECK(leader.textheight == 1.0);
        CHECK(leader.vertexlist.empty());
    }
}

TEST_CASE("DWG LEADER does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeTruncatedLeaderBody();
    REQUIRE(!body.empty());

    DwgLeaderProbe leader;
    leader.style = "STALE_STYLE";
    leader.annotHandle = 0x2F4u;
    leader.vertexlist.push_back(std::make_shared<DRW_Coord>(8.0, 9.0, 0.0));
    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(leader.parseDwg(DRW::AC1015, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(buffer.getPosition() == 0);
    CHECK(buffer.getBitPos() == 0);
    CHECK(leader.style.empty());
    CHECK(leader.annotHandle == 0);
    CHECK(leader.vertexlist.empty());
}

TEST_CASE("DWG LWPOLYLINE consumes independent streams without vertices",
          "[dwg][safety]") {
    auto bytes = makeZeroVertexLwPolylineStreamBody();
    REQUIRE_FALSE(bytes.empty());
    dwgBuffer buffer(bytes.data(), bytes.size());
    DwgLwPolylineProbe polyline;
    REQUIRE(polyline.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK(polyline.vertexnum == 0);
    CHECK(polyline.vertlist.empty());
    CHECK(buffer.isGood());
}

TEST_CASE("DWG primitive carriers clear reused body state", "[dwg][safety]") {
    std::array<std::uint8_t, 0> empty{};

    DwgPointReaderProbe point;
    point.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    point.thickness = 4.0;
    point.extPoint = DRW_Coord{5.0, 6.0, 7.0};
    point.xAxisAngle = 8.0;
    dwgBuffer pointBuffer(empty.data(), empty.size());
    CHECK_FALSE(point.parseDwg(DRW::AC1027, &pointBuffer, 0));
    CHECK(point.basePoint.x == 0.0);
    CHECK(point.basePoint.y == 0.0);
    CHECK(point.basePoint.z == 0.0);
    CHECK(point.thickness == 0.0);
    CHECK(point.extPoint.z == 1.0);
    CHECK(point.xAxisAngle == 0.0);

    DwgLineReaderProbe line;
    line.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    line.secPoint = DRW_Coord{4.0, 5.0, 6.0};
    line.thickness = 7.0;
    dwgBuffer lineBuffer(empty.data(), empty.size());
    CHECK_FALSE(line.parseDwg(DRW::AC1027, &lineBuffer, 0));
    CHECK(line.basePoint.x == 0.0);
    CHECK(line.secPoint.x == 0.0);
    CHECK(line.secPoint.y == 0.0);
    CHECK(line.secPoint.z == 0.0);
    CHECK(line.thickness == 0.0);

    DwgCircleReaderProbe circle;
    circle.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    circle.radious = 9.0;
    dwgBuffer circleBuffer(empty.data(), empty.size());
    CHECK_FALSE(circle.parseDwg(DRW::AC1027, &circleBuffer, 0));
    CHECK(circle.basePoint.x == 0.0);
    CHECK(circle.radious == 0.0);

    DwgArcReaderProbe arc;
    arc.staangle = 10.0;
    arc.endangle = 11.0;
    arc.isccw = 0;
    dwgBuffer arcBuffer(empty.data(), empty.size());
    CHECK_FALSE(arc.parseDwg(DRW::AC1027, &arcBuffer, 0));
    CHECK(arc.staangle == 0.0);
    CHECK(arc.endangle == 0.0);
    CHECK(arc.isccw == 1);

    DwgEllipseReaderProbe ellipse;
    ellipse.ratio = 12.0;
    ellipse.staparam = 13.0;
    ellipse.endparam = 14.0;
    ellipse.isccw = 0;
    dwgBuffer ellipseBuffer(empty.data(), empty.size());
    CHECK_FALSE(ellipse.parseDwg(DRW::AC1027, &ellipseBuffer, 0));
    CHECK(ellipse.ratio == 1.0);
    CHECK(ellipse.staparam == 0.0);
    CHECK(ellipse.endparam == 0.0);
    CHECK(ellipse.isccw == 1);

    DwgTraceReaderProbe trace;
    trace.thirdPoint = DRW_Coord{1.0, 2.0, 3.0};
    trace.fourPoint = DRW_Coord{4.0, 5.0, 6.0};
    dwgBuffer traceBuffer(empty.data(), empty.size());
    CHECK_FALSE(trace.parseDwg(DRW::AC1027, &traceBuffer, 0));
    CHECK(trace.thirdPoint.x == 0.0);
    CHECK(trace.thirdPoint.y == 0.0);
    CHECK(trace.thirdPoint.z == 0.0);
    CHECK(trace.fourPoint.x == 0.0);
    CHECK(trace.fourPoint.y == 0.0);
    CHECK(trace.fourPoint.z == 0.0);

    Dwg3DFaceReaderProbe face;
    face.invisibleflag = DRW_3Dface::AllEdges;
    face.thirdPoint = DRW_Coord{7.0, 8.0, 9.0};
    dwgBuffer faceBuffer(empty.data(), empty.size());
    CHECK_FALSE(face.parseDwg(DRW::AC1027, &faceBuffer, 0));
    CHECK(face.invisibleflag == 0);
    CHECK(face.thirdPoint.x == 0.0);
    CHECK(face.thirdPoint.y == 0.0);
    CHECK(face.thirdPoint.z == 0.0);
}

TEST_CASE("DWG opaque and table carriers clear reused body state", "[dwg][safety]") {
    std::array<std::uint8_t, 0> empty{};

    DwgShapeReaderProbe shape;
    shape.layer = "STALE_LAYER";
    shape.handle = 0xC80;
    shape.m_insertionPoint = DRW_Coord{1.0, 2.0, 3.0};
    shape.m_scale = 4.0;
    shape.m_rotation = 5.0;
    shape.m_shapeIndex = 6;
    shape.m_shapeFileHandle = 7;
    shape.m_rawBytes.push_back(8);
    dwgBuffer shapeBuffer(empty.data(), empty.size());
    CHECK_FALSE(shape.parseDwg(DRW::AC1027, &shapeBuffer, 0));
    CHECK(shape.m_insertionPoint.x == 0.0);
    CHECK(shape.layer == "0");
    CHECK(shape.handle == DRW::NoHandle);
    CHECK(shape.m_insertionPoint.y == 0.0);
    CHECK(shape.m_insertionPoint.z == 0.0);
    CHECK(shape.m_scale == 1.0);
    CHECK(shape.m_rotation == 0.0);
    CHECK(shape.m_shapeIndex == 0);
    CHECK(shape.m_shapeFileHandle == 0);
    CHECK(shape.m_rawBytes.empty());

    DwgOle2FrameReaderProbe ole2;
    ole2.m_flags = 9;
    ole2.m_mode = 10;
    ole2.m_declaredPayloadLength = 11;
    ole2.m_payloadPresent = true;
    ole2.m_payloadBytes.push_back(12);
    ole2.m_pt1 = DRW_Coord{13.0, 14.0, 15.0};
    dwgBuffer ole2Buffer(empty.data(), empty.size());
    CHECK_FALSE(ole2.parseDwg(DRW::AC1027, &ole2Buffer, 0));
    CHECK(ole2.m_flags == 0);
    CHECK(ole2.m_mode == 0);
    CHECK(ole2.m_declaredPayloadLength == 0);
    CHECK_FALSE(ole2.m_payloadPresent);
    CHECK(ole2.m_payloadBytes.empty());
    CHECK(ole2.m_pt1.x == 0.0);
    CHECK(ole2.m_pt1.y == 0.0);
    CHECK(ole2.m_pt1.z == 0.0);

    DwgOleFrameReaderProbe ole;
    ole.m_flags = 16;
    ole.m_mode = 17;
    ole.m_declaredPayloadLength = 18;
    ole.m_payloadPresent = true;
    ole.m_payloadBytes.push_back(19);
    dwgBuffer oleBuffer(empty.data(), empty.size());
    CHECK_FALSE(ole.parseDwg(DRW::AC1027, &oleBuffer, 0));
    CHECK(ole.m_flags == 0);
    CHECK(ole.m_mode == 0);
    CHECK(ole.m_declaredPayloadLength == 0);
    CHECK_FALSE(ole.m_payloadPresent);
    CHECK(ole.m_payloadBytes.empty());

    DwgTableReaderProbe table;
    table.layer = "STALE_LAYER";
    table.handle = 0xC81;
    table.name = "STALE_TABLE";
    table.attribHandles.push_back(dwgHandle{});
    table.m_hasSemanticContent = true;
    table.m_semanticContentComplete = true;
    table.m_tableStyleHandle = 20;
    table.m_content.m_columns.push_back(DRW_TableColumn{});
    table.m_content.m_rows.push_back(DRW_TableRow{});
    dwgBuffer tableBuffer(empty.data(), empty.size());
    CHECK_FALSE(table.parseDwg(DRW::AC1027, &tableBuffer, 0));
    CHECK(table.name.empty());
    CHECK(table.layer == "0");
    CHECK(table.handle == DRW::NoHandle);
    CHECK(table.attribHandles.empty());
    CHECK_FALSE(table.m_hasSemanticContent);
    CHECK_FALSE(table.m_semanticContentComplete);
    CHECK(table.m_tableStyleHandle == 0);
    CHECK(table.m_content.m_columns.empty());
    CHECK(table.m_content.m_rows.empty());
}

TEST_CASE("DWG SHAPE publishes only a complete body and handle stream",
          "[dwg][safety]") {
    const auto complete = makeShapeBody();
    REQUIRE(!complete.first.empty());
    REQUIRE(complete.second >= 8);

    DwgShapeReaderProbe shape;
    dwgBuffer completeBuffer(const_cast<std::uint8_t*>(complete.first.data()),
                             complete.first.size());
    REQUIRE(shape.parseDwg(DRW::AC1027, &completeBuffer, complete.second));
    CHECK(shape.handle == 0x301u);
    CHECK(shape.m_shapeFileHandle == 0x302u);
    CHECK(shape.m_insertionPoint.x == Catch::Approx(3.0));
    CHECK(shape.m_insertionPoint.y == Catch::Approx(4.0));
    CHECK(shape.m_insertionPoint.z == Catch::Approx(5.0));

    const auto truncated = makeTruncatedShapeBody();
    REQUIRE(!truncated.first.empty());
    DwgShapeReaderProbe reused;
    reused.m_shapeFileHandle = 0x3F0u;
    reused.m_insertionPoint = DRW_Coord{8.0, 9.0, 10.0};
    dwgBuffer truncatedBuffer(
        const_cast<std::uint8_t*>(truncated.first.data()),
        truncated.first.size());
    CHECK_FALSE(reused.parseDwg(DRW::AC1027, &truncatedBuffer,
                                truncated.second));
    CHECK_FALSE(truncatedBuffer.isGood());
    CHECK(reused.m_shapeFileHandle == 0);
    CHECK(reused.m_insertionPoint.x == 0.0);
    CHECK(reused.m_insertionPoint.y == 0.0);
    CHECK(reused.m_insertionPoint.z == 0.0);
}

TEST_CASE("DWG OLE carriers publish only complete payloads and handles",
          "[dwg][safety]") {
    const auto ole2Complete = makeOle2FrameBody();
    REQUIRE(!ole2Complete.first.empty());
    REQUIRE(ole2Complete.second >= 8);
    DwgOle2FrameReaderProbe ole2;
    dwgBuffer ole2Buffer(
        const_cast<std::uint8_t*>(ole2Complete.first.data()),
        ole2Complete.first.size());
    REQUIRE(ole2.parseDwg(DRW::AC1027, &ole2Buffer, ole2Complete.second));
    CHECK(ole2.handle == 0x303u);
    CHECK(ole2.m_payloadBytes ==
          std::vector<std::uint8_t>{0x01, 0x02, 0x03, 0x04});

    const auto ole2Truncated = makeTruncatedOle2FrameBody();
    REQUIRE(!ole2Truncated.first.empty());
    DwgOle2FrameReaderProbe reusedOle2;
    reusedOle2.m_payloadPresent = true;
    reusedOle2.m_payloadBytes = {0xAA};
    reusedOle2.m_declaredPayloadLength = 1;
    dwgBuffer ole2TruncatedBuffer(
        const_cast<std::uint8_t*>(ole2Truncated.first.data()),
        ole2Truncated.first.size());
    CHECK_FALSE(reusedOle2.parseDwg(DRW::AC1027, &ole2TruncatedBuffer,
                                    ole2Truncated.second));
    CHECK_FALSE(ole2TruncatedBuffer.isGood());
    CHECK(reusedOle2.m_payloadBytes.empty());
    CHECK(reusedOle2.m_declaredPayloadLength == 0);

    const auto oleComplete = makeOleFrameBody();
    REQUIRE(!oleComplete.first.empty());
    REQUIRE(oleComplete.second >= 8);
    DwgOleFrameReaderProbe ole;
    dwgBuffer oleBuffer(const_cast<std::uint8_t*>(oleComplete.first.data()),
                        oleComplete.first.size());
    REQUIRE(ole.parseDwg(DRW::AC1027, &oleBuffer, oleComplete.second));
    CHECK(ole.handle == 0x304u);
    CHECK(ole.m_payloadBytes ==
          std::vector<std::uint8_t>{0x05, 0x06, 0x07, 0x08});

    const auto oleTruncated = makeTruncatedOleFrameBody();
    REQUIRE(!oleTruncated.first.empty());
    DwgOleFrameReaderProbe reusedOle;
    reusedOle.m_payloadPresent = true;
    reusedOle.m_payloadBytes = {0xBB};
    reusedOle.m_declaredPayloadLength = 1;
    dwgBuffer oleTruncatedBuffer(
        const_cast<std::uint8_t*>(oleTruncated.first.data()),
        oleTruncated.first.size());
    CHECK_FALSE(reusedOle.parseDwg(DRW::AC1027, &oleTruncatedBuffer,
                                   oleTruncated.second));
    CHECK_FALSE(oleTruncatedBuffer.isGood());
    CHECK(reusedOle.m_payloadBytes.empty());
    CHECK(reusedOle.m_declaredPayloadLength == 0);
}

TEST_CASE("DWG CAMERA publishes only a complete VIEW handle tail",
          "[dwg][safety]") {
    const auto complete = makeCameraBody();
    REQUIRE(!complete.first.empty());
    REQUIRE(complete.second >= 8);

    DwgCameraProbe camera;
    dwgBuffer completeBuffer(const_cast<std::uint8_t*>(complete.first.data()),
                             complete.first.size());
    REQUIRE(camera.parseDwg(DRW::AC1027, &completeBuffer, complete.second));
    CHECK(camera.handle == 0x305u);
    CHECK(camera.m_viewHandle == 0x306u);

    const auto truncated = makeTruncatedCameraBody();
    REQUIRE(!truncated.first.empty());
    DwgCameraProbe reused;
    reused.m_viewHandle = 0x3F1u;
    dwgBuffer truncatedBuffer(
        const_cast<std::uint8_t*>(truncated.first.data()),
        truncated.first.size());
    CHECK_FALSE(reused.parseDwg(DRW::AC1027, &truncatedBuffer,
                                truncated.second));
    CHECK_FALSE(truncatedBuffer.isGood());
    CHECK(reused.m_viewHandle == 0);
}

TEST_CASE("DWG LIGHT publishes only a complete body and handle stream",
          "[dwg][safety]") {
    const auto complete = makeLightBody();
    REQUIRE(!complete.first.empty());
    REQUIRE(complete.second >= 8);

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1027, false);
    DwgLightProbe light;
    dwgBuffer completeBuffer(
        const_cast<std::uint8_t*>(complete.first.data()),
        complete.first.size(), &codec);
    REQUIRE(light.parseDwg(DRW::AC1027, &completeBuffer, complete.second));
    CHECK(light.handle == 0x307u);
    CHECK(light.m_name == "TEST_LIGHT");
    CHECK(light.m_type == 2u);
    CHECK(light.m_position.x == Catch::Approx(1.0));
    CHECK(light.m_position.y == Catch::Approx(2.0));
    CHECK(light.m_position.z == Catch::Approx(3.0));

    const auto truncated = makeTruncatedLightBody();
    REQUIRE(!truncated.first.empty());
    DwgLightProbe reused;
    reused.m_name = "STALE_LIGHT";
    reused.m_type = 9;
    reused.m_hasPhotometricData = true;
    reused.m_webFile = "stale.ies";
    dwgBuffer truncatedBuffer(
        const_cast<std::uint8_t*>(truncated.first.data()),
        truncated.first.size(), &codec);
    CHECK_FALSE(reused.parseDwg(DRW::AC1027, &truncatedBuffer,
                                truncated.second));
    CHECK_FALSE(truncatedBuffer.isGood());
    CHECK(reused.m_name.empty());
    CHECK(reused.m_type == 0u);
    CHECK_FALSE(reused.m_hasPhotometricData);
    CHECK(reused.m_webFile.empty());
}

TEST_CASE("DWG GEOPOSITIONMARKER publishes only complete embedded MTEXT",
          "[dwg][safety]") {
    const auto complete = makeGeoPositionMarkerBody();
    REQUIRE(!complete.first.empty());
    REQUIRE(complete.second >= 8);

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);
    DwgGeoPositionMarkerWriterProbe marker;
    dwgBuffer completeBuffer(
        const_cast<std::uint8_t*>(complete.first.data()),
        complete.first.size(), &codec);
    REQUIRE(marker.parseDwg(DRW::AC1032, &completeBuffer, complete.second));
    CHECK(marker.handle == 0x308u);
    CHECK(marker.m_notes == "TEST_MARKER");
    CHECK(marker.m_position.x == Catch::Approx(10.0));
    CHECK(marker.m_position.y == Catch::Approx(20.0));
    REQUIRE(marker.mtext != nullptr);
    CHECK(marker.mtext->text == "EMBEDDED_MARKER");

    const auto truncated = makeTruncatedGeoPositionMarkerBody();
    REQUIRE(!truncated.first.empty());
    DwgGeoPositionMarkerWriterProbe reused;
    reused.m_notes = "STALE_MARKER";
    reused.m_enableFrameText = true;
    reused.mtext = std::make_unique<DRW_MText>();
    reused.mtext->text = "STALE_EMBEDDED";
    dwgBuffer truncatedBuffer(
        const_cast<std::uint8_t*>(truncated.first.data()),
        truncated.first.size(), &codec);
    CHECK_FALSE(reused.parseDwg(DRW::AC1032, &truncatedBuffer,
                                truncated.second));
    CHECK_FALSE(truncatedBuffer.isGood());
    CHECK(reused.m_notes.empty());
    CHECK_FALSE(reused.m_enableFrameText);
    CHECK(reused.mtext == nullptr);
}

TEST_CASE("DWG SECTIONOBJECT publishes only complete geometry and handles",
          "[dwg][safety]") {
    const auto complete = makeSectionObjectBody();
    REQUIRE(!complete.first.empty());
    REQUIRE(complete.second >= 8);

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1027, false);
    DwgSectionObjectProbe section;
    dwgBuffer completeBuffer(
        const_cast<std::uint8_t*>(complete.first.data()),
        complete.first.size(), &codec);
    REQUIRE(section.parseDwg(DRW::AC1027, &completeBuffer, complete.second));
    CHECK(section.handle == 0x309u);
    CHECK(section.m_name == "TEST_SECTION");
    CHECK(section.m_topHeight == Catch::Approx(10.0));
    CHECK(section.m_bottomHeight == Catch::Approx(-2.0));
    CHECK(section.m_verts.size() == 2);
    CHECK(section.m_verts[1].x == Catch::Approx(3.0));
    CHECK(section.m_blVerts.size() == 1);
    CHECK(section.m_sectionSettingsHandle == 0x30Au);

    const auto truncated = makeTruncatedSectionObjectBody();
    REQUIRE(!truncated.first.empty());
    DwgSectionObjectProbe reused;
    reused.m_name = "STALE_SECTION";
    reused.m_verts.emplace_back(8.0, 9.0, 10.0);
    reused.m_sectionSettingsHandle = 0x3F2u;
    dwgBuffer truncatedBuffer(
        const_cast<std::uint8_t*>(truncated.first.data()),
        truncated.first.size(), &codec);
    CHECK_FALSE(reused.parseDwg(DRW::AC1027, &truncatedBuffer,
                                truncated.second));
    CHECK_FALSE(truncatedBuffer.isGood());
    CHECK(reused.m_name.empty());
    CHECK(reused.m_verts.empty());
    CHECK(reused.m_blVerts.empty());
    CHECK(reused.m_sectionSettingsHandle == 0);
}

TEST_CASE("DWG TOLERANCE publishes only complete geometry and handles",
          "[dwg][safety]") {
    const auto complete = makeToleranceBody();
    REQUIRE(!complete.first.empty());
    REQUIRE(complete.second >= 8);

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1027, false);
    DwgToleranceProbe tolerance;
    dwgBuffer completeBuffer(
        const_cast<std::uint8_t*>(complete.first.data()),
        complete.first.size(), &codec);
    REQUIRE(tolerance.parseDwg(DRW::AC1027, &completeBuffer, complete.second));
    CHECK(tolerance.handle == 0x30Bu);
    CHECK(tolerance.insertionPoint.x == Catch::Approx(1.0));
    CHECK(tolerance.insertionPoint.y == Catch::Approx(2.0));
    CHECK(tolerance.insertionPoint.z == Catch::Approx(3.0));
    CHECK(tolerance.xAxisDirectionVector.y == Catch::Approx(1.0));
    CHECK(tolerance.extPoint.z == Catch::Approx(1.0));
    CHECK(tolerance.text == "TOLERANCE_TEXT");
    CHECK(tolerance.dimStyleH.ref == 0x30Cu);

    const auto truncated = makeTruncatedToleranceBody();
    REQUIRE(!truncated.first.empty());
    DwgToleranceProbe reused;
    reused.text = "STALE_TOLERANCE";
    reused.insertionPoint = DRW_Coord{8.0, 9.0, 10.0};
    reused.dimStyleH.ref = 0x3F3u;
    dwgBuffer truncatedBuffer(
        const_cast<std::uint8_t*>(truncated.first.data()),
        truncated.first.size(), &codec);
    CHECK_FALSE(reused.parseDwg(DRW::AC1027, &truncatedBuffer,
                                truncated.second));
    CHECK_FALSE(truncatedBuffer.isGood());
    CHECK(reused.text.empty());
    CHECK(reused.insertionPoint.x == 0.0);
    CHECK(reused.insertionPoint.y == 0.0);
    CHECK(reused.insertionPoint.z == 0.0);
    CHECK(reused.dimStyleH.ref == 0);
}

TEST_CASE("DWG 3DLINE publishes only complete raw geometry",
          "[dwg][safety]") {
    const auto complete = make3DLineBody();
    REQUIRE(!complete.first.empty());
    REQUIRE(complete.second >= 8);

    Dwg3DLineProbe line;
    dwgBuffer completeBuffer(
        const_cast<std::uint8_t*>(complete.first.data()),
        complete.first.size());
    REQUIRE(line.parseDwg(DRW::AC1027, &completeBuffer, complete.second));
    CHECK(line.handle == 0x30Du);
    CHECK(line.basePoint.x == Catch::Approx(1.0));
    CHECK(line.basePoint.y == Catch::Approx(2.0));
    CHECK(line.basePoint.z == Catch::Approx(3.0));
    CHECK(line.secPoint.z == Catch::Approx(6.0));
    CHECK(line.extPoint.z == Catch::Approx(1.0));
    CHECK(line.thickness == Catch::Approx(0.25));

    const auto truncated = makeTruncated3DLineBody();
    REQUIRE(!truncated.first.empty());
    Dwg3DLineProbe reused;
    reused.basePoint = DRW_Coord{8.0, 9.0, 10.0};
    reused.secPoint = DRW_Coord{11.0, 12.0, 13.0};
    reused.thickness = 7.0;
    dwgBuffer truncatedBuffer(
        const_cast<std::uint8_t*>(truncated.first.data()),
        truncated.first.size());
    CHECK_FALSE(reused.parseDwg(DRW::AC1027, &truncatedBuffer,
                                truncated.second));
    CHECK_FALSE(truncatedBuffer.isGood());
    CHECK(reused.basePoint.x == 0.0);
    CHECK(reused.basePoint.y == 0.0);
    CHECK(reused.basePoint.z == 0.0);
    CHECK(reused.secPoint.x == 0.0);
    CHECK(reused.thickness == 0.0);
}

TEST_CASE("DWG TRACE and 3DFACE publish only complete geometry",
          "[dwg][safety]") {
    const auto traceBody = makeTraceBody();
    REQUIRE(!traceBody.first.empty());
    REQUIRE(traceBody.second >= 8);

    DwgTraceReaderProbe trace;
    dwgBuffer traceBuffer(const_cast<std::uint8_t*>(traceBody.first.data()),
                          traceBody.first.size());
    REQUIRE(trace.parseDwg(DRW::AC1027, &traceBuffer, traceBody.second));
    CHECK(trace.basePoint.z == Catch::Approx(0.5));
    CHECK(trace.thirdPoint.x == Catch::Approx(7.0));
    CHECK(trace.fourPoint.y == Catch::Approx(11.0));
    CHECK(trace.thickness == Catch::Approx(0.25));

    const auto truncatedTrace = makeTruncatedTraceBody();
    REQUIRE(!truncatedTrace.first.empty());
    DwgTraceReaderProbe reusedTrace;
    reusedTrace.thirdPoint = DRW_Coord{20.0, 21.0, 22.0};
    dwgBuffer truncatedTraceBuffer(
        const_cast<std::uint8_t*>(truncatedTrace.first.data()),
        truncatedTrace.first.size());
    CHECK_FALSE(reusedTrace.parseDwg(DRW::AC1027, &truncatedTraceBuffer,
                                     truncatedTrace.second));
    CHECK_FALSE(truncatedTraceBuffer.isGood());
    CHECK(reusedTrace.thirdPoint.x == 0.0);
    CHECK(reusedTrace.thirdPoint.y == 0.0);
    CHECK(reusedTrace.thirdPoint.z == 0.0);

    const auto faceBody = make3DFaceBody();
    REQUIRE(!faceBody.first.empty());
    REQUIRE(faceBody.second >= 8);

    Dwg3DFaceReaderProbe face;
    dwgBuffer faceBuffer(const_cast<std::uint8_t*>(faceBody.first.data()),
                         faceBody.first.size());
    REQUIRE(face.parseDwg(DRW::AC1027, &faceBuffer, faceBody.second));
    CHECK(face.basePoint.z == Catch::Approx(3.0));
    CHECK(face.secPoint.x == Catch::Approx(4.0));
    CHECK(face.fourPoint.y == Catch::Approx(11.0));
    CHECK(face.invisibleflag == DRW_3Dface::AllEdges);

    const auto truncatedFace = makeTruncated3DFaceBody();
    REQUIRE(!truncatedFace.first.empty());
    Dwg3DFaceReaderProbe reusedFace;
    reusedFace.invisibleflag = DRW_3Dface::AllEdges;
    reusedFace.fourPoint = DRW_Coord{30.0, 31.0, 32.0};
    dwgBuffer truncatedFaceBuffer(
        const_cast<std::uint8_t*>(truncatedFace.first.data()),
        truncatedFace.first.size());
    CHECK_FALSE(reusedFace.parseDwg(DRW::AC1027, &truncatedFaceBuffer,
                                    truncatedFace.second));
    CHECK_FALSE(truncatedFaceBuffer.isGood());
    CHECK(reusedFace.fourPoint.x == 0.0);
    CHECK(reusedFace.fourPoint.y == 0.0);
    CHECK(reusedFace.fourPoint.z == 0.0);
    CHECK(reusedFace.invisibleflag == 0);
}

TEST_CASE("DWG block carriers preserve the delimiter role while resetting state",
          "[dwg][safety]") {
    std::array<std::uint8_t, 0> empty{};
    DwgBlockReaderProbe block;
    block.setIsEnd(false);
    block.name = "STALE_BLOCK";
    block.flags = 7;
    block.insUnits = 8;
    block.xrefPath = "STALE_XREF";
    block.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    block.thickness = 4.0;
    block.extPoint = DRW_Coord{5.0, 6.0, 7.0};
    block.previewData.push_back(9);
    dwgBuffer blockBuffer(empty.data(), empty.size());
    CHECK_FALSE(block.parseDwg(DRW::AC1027, &blockBuffer, 0));
    CHECK_FALSE(block.getIsEnd());
    CHECK(block.name == "*U0");
    CHECK(block.flags == 0);
    CHECK(block.insUnits == 0);
    CHECK(block.xrefPath.empty());
    CHECK(block.basePoint.x == 0.0);
    CHECK(block.basePoint.y == 0.0);
    CHECK(block.basePoint.z == 0.0);
    CHECK(block.thickness == 0.0);
    CHECK(block.extPoint.x == 0.0);
    CHECK(block.extPoint.y == 0.0);
    CHECK(block.extPoint.z == 1.0);
    CHECK(block.previewData.empty());

    block.name = "STALE_NULL";
    CHECK_FALSE(block.parseDwg(DRW::AC1027, nullptr, 0));
    CHECK(block.name == "*U0");
    CHECK_FALSE(block.getIsEnd());

    block.setIsEnd(true);
    block.name = "STALE_ENDBLK";
    dwgBuffer endBlockBuffer(empty.data(), empty.size());
    CHECK_FALSE(block.parseDwg(DRW::AC1027, &endBlockBuffer, 0));
    CHECK(block.getIsEnd());
    CHECK(block.name.empty());
}

TEST_CASE("DWG R2018 BLOCK names stay inside the string stream",
          "[dwg][safety]") {
    const auto complete = makeEncodedBlockBody(false);
    REQUIRE(!complete.first.empty());
    REQUIRE(complete.second >= 8);
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);

    DwgBlockReaderProbe block;
    dwgBuffer completeBuffer(
        const_cast<std::uint8_t*>(complete.first.data()),
        complete.first.size(), &codec);
    REQUIRE(block.parseDwg(DRW::AC1032, &completeBuffer, complete.second));
    CHECK(block.name == "*TEST_BLOCK");

    const auto endBlock = makeEncodedBlockBody(true);
    REQUIRE(!endBlock.first.empty());
    DwgBlockReaderProbe delimiter;
    delimiter.setIsEnd(true);
    delimiter.name = "STALE_NAME";
    dwgBuffer endBlockBuffer(
        const_cast<std::uint8_t*>(endBlock.first.data()),
        endBlock.first.size(), &codec);
    REQUIRE(delimiter.parseDwg(DRW::AC1032, &endBlockBuffer,
                               endBlock.second));
    CHECK(delimiter.name.empty());
}

TEST_CASE("DWG BLOCK does not publish a truncated handle tail",
          "[dwg][safety]") {
    const auto body = makeMalformedBlockBody();
    REQUIRE(!body.empty());

    DwgBlockReaderProbe block;
    block.setIsEnd(false);
    block.name = "STALE_BLOCK";
    block.thickness = 4.0;
    block.xAxisAngle = 5.0;

    dwgBuffer buffer(const_cast<std::uint8_t*>(body.data()), body.size());
    CHECK_FALSE(block.parseDwg(DRW::AC1018, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(block.name == "*U0");
    CHECK(block.thickness == 0.0);
    CHECK(block.xAxisAngle == 0.0);
    CHECK(block.extPoint.x == 0.0);
    CHECK(block.extPoint.y == 0.0);
    CHECK(block.extPoint.z == 1.0);
}

TEST_CASE("DWG attribute entities do not publish truncated handle tails",
          "[dwg][safety]") {
    DwgAttribWriterProbe attribWriter;
    attribWriter.handle = 0x2E1;
    attribWriter.parentHandle = 0x2E2;
    attribWriter.text = "value";
    attribWriter.tag = "TAG";
    attribWriter.height = 1.0;
    const auto attribBody = makeTruncatedAttributeBody(attribWriter);
    REQUIRE(!attribBody.empty());

    DwgAttribProbe attrib;
    attrib.tag = "STALE_TAG";
    attrib.styleH.ref = 0x2E3;
    dwgBuffer attribBuffer(
        const_cast<std::uint8_t*>(attribBody.data()), attribBody.size());
    CHECK_FALSE(attrib.parseDwg(DRW::AC1018, &attribBuffer, 0));
    CHECK_FALSE(attribBuffer.isGood());
    CHECK(attrib.tag.empty());
    CHECK(attrib.styleH.ref == 0);

    DwgAttdefWriterProbe attdefWriter;
    attdefWriter.handle = 0x2E4;
    attdefWriter.parentHandle = 0x2E5;
    attdefWriter.text = "value";
    attdefWriter.tag = "TAG";
    attdefWriter.prompt = "Prompt";
    attdefWriter.height = 1.0;
    const auto attdefBody = makeTruncatedAttributeBody(attdefWriter);
    REQUIRE(!attdefBody.empty());

    DwgAttdefProbe attdef;
    attdef.prompt = "STALE_PROMPT";
    attdef.styleH.ref = 0x2E6;
    dwgBuffer attdefBuffer(
        const_cast<std::uint8_t*>(attdefBody.data()), attdefBody.size());
    CHECK_FALSE(attdef.parseDwg(DRW::AC1018, &attdefBuffer, 0));
    CHECK_FALSE(attdefBuffer.isGood());
    CHECK(attdef.prompt.empty());
    CHECK(attdef.styleH.ref == 0);
}

TEST_CASE("DWG attribute body bounds reject truncated fields",
          "[dwg][safety]") {
    const auto makeShortBody = [](auto& writer) {
        dwgBufferW body;
        if (!writer.encodeDwg(DRW::AC1018, &body, 0, nullptr, nullptr))
            return std::vector<std::uint8_t>{};
        finalizeEncodedEntityBody(body);
        if (body.data().empty())
            return std::vector<std::uint8_t>{};
        const std::uint8_t bsCode =
            static_cast<std::uint8_t>((body.data().front() >> 6) & 0x03);
        const std::size_t sizeBitOffset =
            bsCode == 0x01 ? 10 : bsCode == 0x00 ? 18 : 2;
        body.patchRawLong32AtBit(sizeBitOffset, 1);
        return body.data();
    };

    DwgAttribWriterProbe attribWriter;
    attribWriter.handle = 0x2F0;
    attribWriter.text = "value";
    attribWriter.tag = "TAG";
    attribWriter.height = 1.0;
    const auto attribBody = makeShortBody(attribWriter);
    REQUIRE(!attribBody.empty());

    DwgAttribProbe attrib;
    attrib.text = "STALE_TEXT";
    attrib.basePoint = DRW_Coord{9.0, 8.0, 7.0};
    dwgBuffer attribBuffer(
        const_cast<std::uint8_t*>(attribBody.data()), attribBody.size());
    CHECK_FALSE(attrib.parseDwg(DRW::AC1018, &attribBuffer, 0));
    CHECK_FALSE(attribBuffer.isGood());
    CHECK(attrib.text.empty());
    CHECK(attrib.basePoint.x == 0.0);
    CHECK(attrib.basePoint.y == 0.0);
    CHECK(attrib.basePoint.z == 0.0);

    DwgAttdefWriterProbe attdefWriter;
    attdefWriter.handle = 0x2F1;
    attdefWriter.text = "value";
    attdefWriter.tag = "TAG";
    attdefWriter.prompt = "Prompt";
    attdefWriter.height = 1.0;
    const auto attdefBody = makeShortBody(attdefWriter);
    REQUIRE(!attdefBody.empty());

    DwgAttdefProbe attdef;
    attdef.prompt = "STALE_PROMPT";
    attdef.basePoint = DRW_Coord{9.0, 8.0, 7.0};
    dwgBuffer attdefBuffer(
        const_cast<std::uint8_t*>(attdefBody.data()), attdefBody.size());
    CHECK_FALSE(attdef.parseDwg(DRW::AC1018, &attdefBuffer, 0));
    CHECK_FALSE(attdefBuffer.isGood());
    CHECK(attdef.prompt.empty());
    CHECK(attdef.basePoint.x == 0.0);
    CHECK(attdef.basePoint.y == 0.0);
    CHECK(attdef.basePoint.z == 0.0);
}

TEST_CASE("DWG embedded ATTRIB MTEXT stays inside its body bounds",
          "[dwg][safety]") {
    const auto complete = makeEmbeddedAttribBody(false);
    REQUIRE(!complete.first.empty());
    REQUIRE(complete.second >= 8);

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);
    DwgAttribProbe attrib;
    dwgBuffer completeBuffer(
        const_cast<std::uint8_t*>(complete.first.data()),
        complete.first.size(), &codec);
    REQUIRE(attrib.parseDwg(DRW::AC1032, &completeBuffer, complete.second));
    CHECK(attrib.m_attributeType == 2);
    REQUIRE(attrib.mtext != nullptr);
    CHECK(attrib.mtext->text == "EMBEDDED_ATTRIB");

    const auto truncated = makeEmbeddedAttribBody(true);
    REQUIRE(!truncated.first.empty());
    DwgAttribProbe reused;
    reused.text = "STALE_TEXT";
    reused.tag = "STALE_TAG";
    reused.mtext = std::make_unique<DRW_MText>();
    dwgBuffer truncatedBuffer(
        const_cast<std::uint8_t*>(truncated.first.data()),
        truncated.first.size(), &codec);
    CHECK_FALSE(reused.parseDwg(DRW::AC1032, &truncatedBuffer,
                                truncated.second));
    CHECK_FALSE(truncatedBuffer.isGood());
    CHECK(reused.text.empty());
    CHECK(reused.tag.empty());
    CHECK(reused.mtext == nullptr);

    auto truncatedHandles = complete;
    REQUIRE(truncatedHandles.first.size() > 1);
    REQUIRE(truncatedHandles.second >= 8);
    truncatedHandles.first.pop_back();
    truncatedHandles.second -= 8;
    DwgAttribProbe handleReused;
    handleReused.mtext = std::make_unique<DRW_MText>();
    dwgBuffer truncatedHandleBuffer(
        const_cast<std::uint8_t*>(truncatedHandles.first.data()),
        truncatedHandles.first.size(), &codec);
    CHECK_FALSE(handleReused.parseDwg(DRW::AC1032, &truncatedHandleBuffer,
                                      truncatedHandles.second));
    CHECK_FALSE(truncatedHandleBuffer.isGood());
    CHECK(handleReused.mtext == nullptr);
}

TEST_CASE("DWG SEQEND does not publish a truncated handle tail",
          "[dwg][safety]") {
    DwgSequenceWriterProbe sequenceEndWriter;
    sequenceEndWriter.handle = 0x2E7;
    sequenceEndWriter.parentHandle = 0x2E8;

    dwgBufferW body;
    REQUIRE(sequenceEndWriter.encodeDwg(
        DRW::AC1015, &body, 0, nullptr, nullptr));
    finalizeEncodedEntityBody(body);
    REQUIRE(body.data().size() > 1);
    body.data().pop_back();

    DwgSequenceWriterProbe sequenceEnd;
    sequenceEnd.handle = 0x2E9;
    sequenceEnd.parentHandle = 0x2EA;
    dwgBuffer buffer(body.data().data(), body.data().size());
    CHECK_FALSE(sequenceEnd.parseDwg(DRW::AC1015, &buffer, 0));
    CHECK_FALSE(buffer.isGood());
    CHECK(sequenceEnd.handle == DRW::NoHandle);
    CHECK(sequenceEnd.parentHandle == DRW::NoHandle);
}

TEST_CASE("DWG entity preamble resets reused common state",
          "[dwg][safety]") {
    std::array<std::uint8_t, 0> empty{};

    DwgPointReaderProbe point;
    point.handle = 0x101;
    point.parentHandle = 0x102;
    point.layer = "STALE_LAYER";
    point.lineType = "STALE_LTYPE";
    point.material = 0x103;
    point.color = 7;
    point.ltypeScale = 4.0;
    point.visible = false;
    point.numProxyGraph = 3;
    point.proxyGraphics = "abc";
    point.color24 = 0x102030;
    point.colorName = "STALE_COLOR";
    point.transparency = 0x03000080;
    point.plotStyle = 0x104;
    point.shadowHandle = 0x105;
    point.fullVisualStyleHandle = 0x106;
    point.faceVisualStyleHandle = 0x107;
    point.edgeVisualStyleHandle = 0x108;
    point.reactorHandles.push_back(0x109);
    point.xDictHandle = 0x10A;
    point.hasDataStorageRecord = true;
    point.dataStorageHandle = 0x10B;
    point.dataStorageHandleKey = "10B";
    point.dataStorageData = {1, 2, 3};
    point.setHasDataStorageBinaryData(true);
    point.extData.push_back(std::make_shared<DRW_Variant>(1000, "STALE"));
    point.appData.emplace_back();

    dwgBuffer buffer(empty.data(), empty.size());
    CHECK_FALSE(point.parseDwg(DRW::AC1027, &buffer, 0));
    CHECK(point.handle == DRW::NoHandle);
    CHECK(point.parentHandle == DRW::NoHandle);
    CHECK(point.layer == "0");
    CHECK(point.lineType == "BYLAYER");
    CHECK(point.material == DRW::MaterialByLayer);
    CHECK(point.color == DRW::ColorByLayer);
    CHECK(point.ltypeScale == 1.0);
    CHECK(point.visible);
    CHECK(point.numProxyGraph == 0);
    CHECK(point.proxyGraphics.empty());
    CHECK(point.color24 == -1);
    CHECK(point.colorName.empty());
    CHECK(point.transparency == DRW::Opaque);
    CHECK(point.plotStyle == DRW::DefaultPlotStyle);
    CHECK(point.shadowHandle == 0);
    CHECK(point.fullVisualStyleHandle == 0);
    CHECK(point.faceVisualStyleHandle == 0);
    CHECK(point.edgeVisualStyleHandle == 0);
    CHECK(point.reactorHandles.empty());
    CHECK(point.xDictHandle == 0);
    CHECK_FALSE(point.hasDataStorageRecord);
    CHECK(point.dataStorageHandle == 0);
    CHECK(point.dataStorageHandleKey.empty());
    CHECK(point.dataStorageData.empty());
    CHECK_FALSE(point.hasDataStorageBinaryData());
    CHECK(point.extData.empty());
    CHECK(point.appData.empty());

    CHECK_FALSE(point.parseDwg(DRW::AC1027, nullptr, 0));
}

TEST_CASE("DWG entity frame without a type is not deferred",
          "[dwg][safety]") {
    auto frame = makeObjectFrame({});
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        frame.data(), frame.size()));
    reader.setVersionForTest(DRW::AC1018);
    DwgReadProbe interface;
    dwgBuffer buffer(frame.data(), frame.size());
    objHandle entity(0, 0x301, 0);
    bool frameFailure = false;

    CHECK_FALSE(reader.readDwgEntity(
        &buffer, entity, interface, &frameFailure));
    CHECK(frameFailure);
    CHECK(interface.unsupportedObjects.empty());
    CHECK(reader.objObjectMap.empty());
}

TEST_CASE("DWG source-less compound frames cannot establish ownership",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t insertHandle = 0x201;
    constexpr std::uint32_t attribHandle = 0x202;
    constexpr std::uint32_t seqEndHandle = 0x203;
    const auto insertFrame = makeInsertWithChildrenFrame(
        insertHandle, DRW::NoHandle, attribHandle, seqEndHandle);
    const auto attribFrame = makeAttribFrame(attribHandle, insertHandle);
    const auto seqEndFrame = makeSeqEndFrame(seqEndHandle, insertHandle);
    const auto polylineFixture = makePolylineFrameFixture(false);
    REQUIRE(!insertFrame.empty());
    REQUIRE(!attribFrame.empty());
    REQUIRE(!seqEndFrame.empty());
    REQUIRE(!polylineFixture.bytes.empty());

    const auto rejectsWithoutSource = [](
        const std::vector<std::uint8_t>& frame, std::int16_t type,
        std::uint32_t handle) {
        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(DRW::AC1018);
        DwgInsertReceiptProbe interface;
        dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
        objHandle object(type, handle, 0);

        CHECK_FALSE(reader.readDwgEntity(&buffer, object, interface));
        CHECK(interface.inserts.empty());
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingInsertCountForTest() == 0u);
        CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        CHECK(reader.m_dwgSourceFrameLedger.empty());
    };

    rejectsWithoutSource(insertFrame, dwgType::INSERT, insertHandle);
    rejectsWithoutSource(attribFrame, dwgType::ATTRIB, attribHandle);
    rejectsWithoutSource(seqEndFrame, dwgType::SEQEND, seqEndHandle);
    rejectsWithoutSource(polylineFixture.bytes, dwgType::POLYLINE_2D, 0x100);
}

TEST_CASE("DWG failed block scopes quarantine catch-all children",
          "[dwg][safety]") {
    constexpr std::uint32_t childHandle = 0x2A1;
    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        static_cast<std::uint8_t*>(nullptr), 0));
    reader.setVersionForTest(DRW::AC1018);
    reader.ObjectMap.emplace(childHandle,
                             objHandle(dwgType::LINE, childHandle, 0));
    reader.m_quarantinedEntityHandles.insert(childHandle);

    dwgBuffer buffer(static_cast<std::uint8_t*>(nullptr), 0);
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntities(interface, &buffer));
    CHECK(reader.m_entityParseFailures == 0);
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG duplicate modern block ownership is quarantined before callbacks",
          "[dwg][safety][ownership]") {
    constexpr std::uint32_t blockA = 0x2B0;
    constexpr std::uint32_t endBlockA = 0x2B1;
    constexpr std::uint32_t blockB = 0x2C0;
    constexpr std::uint32_t endBlockB = 0x2C1;
    constexpr std::uint32_t sharedEntity = 0x2D0;

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        static_cast<std::uint8_t*>(nullptr), 0));
    reader.setVersionForTest(DRW::AC1018);

    auto* first = new DRW_Block_Record();
    auto* second = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandles(
        *first, blockA, endBlockA, {sharedEntity});
    DwgBlockOwnershipTestAccess::setHandles(
        *second, blockB, endBlockB, {sharedEntity});
    reader.blockRecordmap.emplace(0x2A0, first);
    reader.blockRecordmap.emplace(0x2A1, second);

    reader.ObjectMap.emplace(
        blockA, objHandle(dwgType::BLOCK, blockA, 0));
    reader.ObjectMap.emplace(
        endBlockA, objHandle(dwgType::ENDBLK, endBlockA, 0));
    reader.ObjectMap.emplace(
        blockB, objHandle(dwgType::BLOCK, blockB, 0));
    reader.ObjectMap.emplace(
        endBlockB, objHandle(dwgType::ENDBLK, endBlockB, 0));
    reader.ObjectMap.emplace(
        sharedEntity, objHandle(dwgType::LINE, sharedEntity, 0));

    DwgReadProbe interface;
    dwgBuffer buffer(static_cast<std::uint8_t*>(nullptr), 0);
    CHECK_FALSE(reader.readDwgBlocks(interface, &buffer));
    CHECK(interface.blockCount == 0);
    CHECK(interface.unsupportedObjects.empty());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.m_quarantinedEntityHandles.find(sharedEntity)
          != reader.m_quarantinedEntityHandles.end());
}

TEST_CASE("DWG named block delimiters reject a foreign owner",
          "[dwg][safety][ownership]") {
    constexpr std::uint32_t recordHandle = 0x2A2u;
    constexpr std::uint32_t blockHandle = 0x2A3u;
    constexpr std::uint32_t endBlockHandle = 0x2A4u;
    constexpr std::uint32_t foreignOwner = 0x2A5u;

    const auto blockFrame = makeBlockFrame(
        blockHandle, foreignOwner, false);
    const auto endBlockFrame = makeBlockFrame(
        endBlockHandle, foreignOwner, true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> bytes = blockFrame;
    bytes.insert(bytes.end(), endBlockFrame.begin(), endBlockFrame.end());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle, {});
    reader.blockRecordmap.emplace(recordHandle, record);
    reader.ObjectMap.emplace(
        blockHandle, objHandle(dwgType::BLOCK, blockHandle, 0));
    reader.ObjectMap.emplace(
        endBlockHandle,
        objHandle(dwgType::ENDBLK, endBlockHandle,
                  static_cast<std::uint32_t>(blockFrame.size())));

    DwgReadProbe interface;
    dwgBuffer buffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgBlocks(interface, &buffer));
    CHECK(interface.blockCount == 0u);
    CHECK(interface.unsupportedObjects.empty());
    CHECK(reader.ObjectMap.empty());

    const auto validBlockFrame = makeBlockFrame(
        blockHandle, recordHandle, false);
    const auto validEndBlockFrame = makeBlockFrame(
        endBlockHandle, recordHandle, true);
    REQUIRE(!validBlockFrame.empty());
    REQUIRE(!validEndBlockFrame.empty());
    std::vector<std::uint8_t> validBytes = validBlockFrame;
    validBytes.insert(validBytes.end(), validEndBlockFrame.begin(),
                      validEndBlockFrame.end());

    DwgEntityReaderProbe validReader(std::make_unique<dwgBuffer>(
        validBytes.data(), validBytes.size()));
    validReader.setVersionForTest(DRW::AC1018);
    auto* validRecord = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*validRecord, recordHandle);
    DwgBlockOwnershipTestAccess::setHandles(
        *validRecord, blockHandle, endBlockHandle, {});
    validReader.blockRecordmap.emplace(recordHandle, validRecord);
    validReader.ObjectMap.emplace(
        blockHandle, objHandle(dwgType::BLOCK, blockHandle, 0));
    validReader.ObjectMap.emplace(
        endBlockHandle,
        objHandle(dwgType::ENDBLK, endBlockHandle,
                  static_cast<std::uint32_t>(validBlockFrame.size())));

    DwgReadProbe validInterface;
    dwgBuffer validBuffer(validBytes.data(), validBytes.size());
    REQUIRE(validReader.readDwgBlocks(validInterface, &validBuffer));
    CHECK(validInterface.blockCount == 1u);
    CHECK(validReader.ObjectMap.empty());
}

TEST_CASE("DWG entity frame failure fails the ENTITIES phase",
          "[dwg][safety]") {
    constexpr std::uint32_t entityHandle = 0x2C1;
    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        static_cast<std::uint8_t*>(nullptr), 0));
    reader.setVersionForTest(DRW::AC1018);
    reader.ObjectMap.emplace(
        entityHandle, objHandle(dwgType::LINE, entityHandle, 0));

    dwgBuffer buffer(static_cast<std::uint8_t*>(nullptr), 0);
    DwgReadProbe interface;
    CHECK_FALSE(reader.readDwgEntities(interface, &buffer));
    CHECK(reader.m_entityParseFailures == 1);
    CHECK(reader.ObjectMap.empty());
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG fixed OBJECTS types are excluded from entity ownership",
          "[dwg][safety]") {
    CHECK(dwgObjType::isFixedObject(dwgObjType::DICTIONARY));
    CHECK(dwgObjType::isFixedObject(dwgObjType::PROXY_OBJECT));
    CHECK(dwgObjType::isFixedObject(dwgObjType::BLOCKREPRESENTATION));
    CHECK_FALSE(dwgObjType::isFixedObject(dwgType::LINE));
    CHECK_FALSE(dwgObjType::isFixedObject(dwgType::WIPEOUT));
}

TEST_CASE("DWG fixed OBJECTS frames defer before entity dispatch",
          "[dwg][safety]") {
    const std::array<std::int16_t, 2> objectTypes = {
        dwgObjType::PROXY_OBJECT,
        dwgObjType::BLOCKREPRESENTATION
    };
    std::uint32_t handle = 0x7B0u;
    for (const std::int16_t objectType : objectTypes) {
        dwgBufferW body;
        body.putObjType(DRW::AC1027,
                        static_cast<std::uint16_t>(objectType));
        const auto frame = makeModernObjectFrame(body.data());
        REQUIRE(!frame.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(DRW::AC1027);
        DwgReadProbe interface;
        dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()),
                         frame.size());
        objHandle object(objectType, handle++, 0);

        REQUIRE(reader.readDwgEntity(&buffer, object, interface));
        CHECK(interface.unsupportedObjects.empty());
        REQUIRE(reader.objObjectMap.size() == 1);
        CHECK(reader.objObjectMap.begin()->second.type == objectType);
    }
}

TEST_CASE("DWG object deferral records its mapped source frame",
          "[dwg][safety]") {
    constexpr std::uint32_t objectHandle = 1;
    dwgBufferW body;
    body.putObjType(DRW::AC1027,
                    static_cast<std::uint16_t>(dwgObjType::PROXY_OBJECT));
    const auto frame = makeModernObjectFrame(body.data());
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1027);
    auto map = makeHandleMap(false);
    dwgBuffer mapBuffer(map.data(), map.size());
    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, map.size()));

    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntity(
        &buffer, reader.ObjectMap.at(objectHandle), interface));
    REQUIRE(reader.objObjectMap.size() == 1);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Deferred);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_reason
          == DRW_DwgFrameCoverageReason::CompoundDeferred);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0);
}

TEST_CASE("DWG entity sweep defers an exact mapped object frame",
          "[dwg][safety]") {
    constexpr std::uint32_t objectHandle = 1;
    dwgBufferW body;
    body.putObjType(DRW::AC1027,
                    static_cast<std::uint16_t>(dwgObjType::PROXY_OBJECT));
    const auto frame = makeModernObjectFrame(body.data());
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1027);
    auto map = makeHandleMap(false);
    dwgBuffer mapBuffer(map.data(), map.size());
    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, map.size()));

    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntities(interface, &buffer));
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.objObjectMap.size() == 1u);
    CHECK(reader.objObjectMap.begin()->first == objectHandle);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Deferred);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    CHECK(reader.m_integrityDiagnostics.empty());
}

TEST_CASE("DWG object sweep commits a detached covered frame after publication",
          "[dwg][safety]") {
    const auto makeDictionaryFrame = [](std::uint32_t handle) {
        dwgBufferW body;
        putDictionaryObjectPreamble(
            body, DRW::AC1015, dwgObjType::DICTIONARY, handle);
        body.putBitLong(0);
        body.putBitShort(0);
        body.putRawChar8(0);
        body.patchRawLong32AtBit(10, body.bitCount());
        body.putHandle(makeObjectHandle(0));
        body.putHandle(makeObjectHandle(0));
        return makeObjectFrame(body.data());
    };
    const auto firstFrame = makeDictionaryFrame(1);
    const auto secondFrame = makeDictionaryFrame(2);
    REQUIRE(!firstFrame.empty());
    REQUIRE(!secondFrame.empty());

    std::vector<std::uint8_t> objectData = firstFrame;
    objectData.insert(objectData.end(), secondFrame.cbegin(), secondFrame.cend());
    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(1));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(1));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(firstFrame.size())));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1015);
    dwgBuffer mapBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, handleMap.size()));

    dwgBuffer buffer(objectData.data(), objectData.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntities(interface, &buffer));
    REQUIRE(reader.objObjectMap.size() == 2u);
    for (const std::uint32_t objectHandle : {1u, 2u}) {
        CHECK(reader.objObjectMap.at(objectHandle).type == 0u);
        CHECK(reader.hasDeferredClassificationForTest(
            objectHandle, dwgObjType::DICTIONARY, dwgObjType::DICTIONARY));
        CHECK(reader.hasFramePhaseSnapshotForTest(
            objectHandle, dwgObjType::DICTIONARY, dwgObjType::DICTIONARY,
            false));
    }
    REQUIRE(reader.readDwgObjects(interface, &buffer));
    CHECK(reader.objObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 2u);
    for (const std::uint32_t objectHandle : {1u, 2u}) {
        CHECK_FALSE(reader.hasDeferredClassificationForTest(
            objectHandle, dwgObjType::DICTIONARY, dwgObjType::DICTIONARY));
        CHECK(reader.hasFramePhaseSnapshotForTest(
            objectHandle, dwgObjType::DICTIONARY, dwgObjType::DICTIONARY,
            true));
    }
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
    CHECK(reader.m_integrityDiagnostics.empty());
}

TEST_CASE("DWG OBJECTS rejects forged deferred frame classification",
          "[dwg][safety]") {
    constexpr std::uint32_t objectHandle = 1;
    dwgBufferW body;
    putDictionaryObjectPreamble(
        body, DRW::AC1015, dwgObjType::DICTIONARY, objectHandle);
    body.putBitLong(0);
    body.putBitShort(0);
    body.putRawChar8(0);
    body.patchRawLong32AtBit(10, body.bitCount());
    body.putHandle(makeObjectHandle(0));
    body.putHandle(makeObjectHandle(0));
    const auto frame = makeObjectFrame(body.data());
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1015);
    auto map = makeHandleMap(false);
    dwgBuffer mapBuffer(map.data(), map.size());
    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, map.size()));
    dwgBuffer buffer(
        const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntities(interface, &buffer));
    reader.forgeDeferredResolvedTypeForTest(objectHandle, dwgObjType::GROUP);

    CHECK_FALSE(reader.readDwgObjects(interface, &buffer));
    CHECK(reader.objObjectMap.empty());
    CHECK_FALSE(reader.hasDeferredClassificationForTest(
        objectHandle, dwgObjType::DICTIONARY, dwgObjType::GROUP));
    CHECK(interface.unsupportedObjects.empty());
    CHECK(reader.hasFailedDeferredObjectSnapshotForTest(
        objectHandle, dwgObjType::DICTIONARY, dwgObjType::GROUP));
}

TEST_CASE("DWG unknown custom objects are preserved through the object pass",
          "[dwg][safety][objects]") {
    constexpr std::uint16_t objectType = 700;
    constexpr std::uint32_t objectHandle = 0x7B8u;

    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1015, objectType,
                                objectHandle);
    const auto frame = makeObjectFrame(body.data());
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1015);
    reader.ObjectMap.emplace(
        objectHandle, objHandle(objectType, objectHandle, 0));

    DwgReadProbe interface;
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    REQUIRE(reader.readDwgEntities(interface, &buffer));
    CHECK(interface.unsupportedObjects.empty());
    REQUIRE(reader.objObjectMap.size() == 1u);

    REQUIRE(reader.readDwgObjects(interface, &buffer));
    REQUIRE(interface.unsupportedObjects.size() == 1u);
    CHECK(interface.unsupportedObjects.front().m_objectType == objectType);
    CHECK(interface.unsupportedObjects.front().m_handle == objectHandle);
    CHECK(interface.unsupportedObjects.front().m_rawBytes == body.data());
}

TEST_CASE("DWG unknown custom entities keep block reachability separate",
          "[dwg][safety][objects]") {
    constexpr std::uint16_t entityType = 701;
    constexpr std::uint32_t entityHandle = 0x7B9u;
    constexpr std::uint32_t ownerHandle = 0x7BAu;

    const auto frame = makeUnknownCustomEntityFrame(
        entityType, entityHandle, ownerHandle);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setExpectedBlockOwnerForTest(ownerHandle);
    reader.ObjectMap.emplace(
        entityHandle, objHandle(entityType, entityHandle, 0));

    DwgReadProbe interface;
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    REQUIRE(reader.readDwgEntities(interface, &buffer));
    REQUIRE(interface.unsupportedObjects.size() == 1u);
    CHECK(interface.unsupportedObjects.front().m_isEntity);
    CHECK(interface.unsupportedObjects.front().m_objectType == entityType);
    CHECK(interface.unsupportedObjects.front().m_handle == entityHandle);
    CHECK(interface.unsupportedObjects.front().m_parentHandle == DRW::NoHandle);
    CHECK(interface.unsupportedObjects.front().m_blockOwnerHandle == ownerHandle);
    CHECK(interface.unsupportedObjects.front().m_commonLinkEvidence
          == DRW_DwgCommonLinkEvidence::Unknown);
    CHECK_FALSE(interface.unsupportedObjects.front().m_commonHandleDataValidated);
    CHECK_FALSE(interface.unsupportedObjects.front().m_hasDataStorage);
}

TEST_CASE("DWG entity sweep separates opaque entities from custom OBJECTS",
          "[dwg][safety]") {
    constexpr std::uint32_t entityHandle = 0x7C0u;
    constexpr std::uint32_t objectHandle = 0x7C1u;
    constexpr std::uint32_t ownerHandle = 0x7C2u;
    constexpr std::uint16_t entityClass = 620;
    constexpr std::uint16_t objectClass = 621;

    auto makeLineFrame = [](std::uint16_t classNumber,
                            std::uint32_t handle,
                            std::uint32_t owner) {
        DwgLineWriterProbe line;
        line.handle = handle;
        line.parentHandle = owner;
        line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
        line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
        line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        line.setObjectType(static_cast<std::int16_t>(classNumber));
        dwgBufferW body;
        if (!line.encodeDwgCommon(DRW::AC1018, &body))
            return std::vector<std::uint8_t>{};
        body.putBit(1);
        body.putRawDouble(line.basePoint.x);
        body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
        body.putRawDouble(line.basePoint.y);
        body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
        body.putThickness(line.thickness, true);
        body.putExtrusion(line.extPoint, true);
        if (!line.encodeDwgEntHandle(DRW::AC1018, &body))
            return std::vector<std::uint8_t>{};
        return makeEntityFrame(body);
    };

    const auto entityFrame = makeLineFrame(entityClass, entityHandle,
                                           ownerHandle);
    REQUIRE(!entityFrame.empty());
    DwgEntityReaderProbe entityReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(entityFrame.data()), entityFrame.size()));
    entityReader.setVersionForTest(DRW::AC1018);
    entityReader.addCustomEntityClass(entityClass);
    entityReader.ObjectMap.emplace(
        entityHandle, objHandle(entityClass, entityHandle, 0));
    dwgBuffer entityBuffer(const_cast<std::uint8_t*>(entityFrame.data()),
                           entityFrame.size());
    DwgReadProbe entityInterface;
    CHECK_FALSE(entityReader.readDwgEntities(entityInterface, &entityBuffer));
    CHECK(entityInterface.unsupportedObjects.empty());
    CHECK(entityReader.objObjectMap.empty());

    const auto objectFrame = makeLineFrame(objectClass, objectHandle,
                                           ownerHandle);
    REQUIRE(!objectFrame.empty());
    DwgEntityReaderProbe objectReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(objectFrame.data()), objectFrame.size()));
    objectReader.setVersionForTest(DRW::AC1018);
    objectReader.addCustomObjectClass(objectClass);
    objectReader.ObjectMap.emplace(
        objectHandle, objHandle(objectClass, objectHandle, 0));
    dwgBuffer objectBuffer(const_cast<std::uint8_t*>(objectFrame.data()),
                           objectFrame.size());
    DwgReadProbe objectInterface;
    REQUIRE(objectReader.readDwgEntities(objectInterface, &objectBuffer));
    CHECK(objectInterface.unsupportedObjects.empty());
    REQUIRE(objectReader.objObjectMap.size() == 1);
    CHECK(objectReader.objObjectMap.begin()->second.type == objectClass);
}

TEST_CASE("DWG typed entity failure publishes no raw callback",
          "[dwg][safety]") {
    dwgBufferW body;
    body.putObjType(DRW::AC1015, dwgType::LINE);
    auto frame = makeObjectFrame(body.data());
    REQUIRE(!frame.empty());

    constexpr std::uint32_t entityHandle = 0x2C0;
    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        frame.data(), frame.size()));
    reader.setVersionForTest(DRW::AC1015);
    reader.ObjectMap.emplace(
        entityHandle, objHandle(dwgType::LINE, entityHandle, 0));

    dwgBuffer buffer(frame.data(), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntities(interface, &buffer));
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG object frame failure fails the OBJECTS phase",
          "[dwg][safety]") {
    constexpr std::uint32_t objectHandle = 0x2B1;
    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        static_cast<std::uint8_t*>(nullptr), 0));
    reader.setVersionForTest(DRW::AC1018);
    reader.objObjectMap.emplace(
        objectHandle, objHandle(dwgObjType::DICTIONARY, objectHandle, 0));
    DRW_UnsupportedObject deferred;
    deferred.m_handle = objectHandle - 1;
    deferred.m_rawBytes = {0xA5};
    reader.m_deferredRawObjects.push_back(deferred);

    dwgBuffer buffer(static_cast<std::uint8_t*>(nullptr), 0);
    DwgReadProbe interface;
    CHECK_FALSE(reader.readDwgObjects(interface, &buffer));
    CHECK(reader.objObjectMap.empty());
    CHECK(reader.m_deferredRawObjects.empty());
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG R2004 object phases honor logical section size",
          "[dwg][safety]") {
    dwgBufferW body;
    body.putObjType(DRW::AC1015, dwgType::LINE);
    const auto frame = makeObjectFrame(body.data());
    REQUIRE(!frame.empty());

    constexpr std::uint32_t logicalSize = 4;
    std::vector<std::uint8_t> padded(logicalSize, 0u);
    padded.insert(padded.end(), frame.begin(), frame.end());

    DwgReader18ObjectBoundsProbe reader(std::make_unique<dwgBuffer>(
        static_cast<std::uint8_t*>(nullptr), 0));
    reader.setVersionForTest(DRW::AC1015);
    reader.setObjectDataForTest(padded, logicalSize);
    reader.ObjectMap.emplace(
        0x2C2u, objHandle(dwgType::LINE, 0x2C2u, logicalSize));

    DwgReadProbe interface;
    CHECK_FALSE(reader.readDwgEntities(interface));
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG R2013 object phases honor logical section size",
          "[dwg][safety]") {
    dwgBufferW body;
    body.putObjType(DRW::AC1027, dwgType::LINE);
    const auto frame = makeModernObjectFrame(body.data());
    REQUIRE(!frame.empty());

    constexpr std::uint32_t logicalSize = 4;
    std::vector<std::uint8_t> padded(logicalSize, 0u);
    padded.insert(padded.end(), frame.begin(), frame.end());

    DwgReader27ObjectBoundsProbe reader(std::make_unique<dwgBuffer>(
        static_cast<std::uint8_t*>(nullptr), 0));
    reader.setVersionForTest(DRW::AC1027);
    reader.setObjectDataForTest(padded, logicalSize);
    reader.ObjectMap.emplace(
        0x2C3u, objHandle(dwgType::LINE, 0x2C3u, logicalSize));

    DwgReadProbe interface;
    CHECK_FALSE(reader.readDwgEntities(interface));
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG typed object failure publishes no raw callback",
          "[dwg][safety]") {
    const auto body = makeMalformedDictionaryBody();
    auto frame = makeObjectFrame(body);
    REQUIRE(!frame.empty());

    constexpr std::uint32_t objectHandle = 0x7A0;
    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        frame.data(), frame.size()));
    reader.setVersionForTest(DRW::AC1015);
    reader.objObjectMap.emplace(
        objectHandle, objHandle(dwgObjType::DICTIONARY, objectHandle, 0));

    dwgBuffer buffer(frame.data(), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgObjects(interface, &buffer));
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG DICTIONARYWDFLT receipt preflight rejects a missing class ordinal",
          "[dwg][safety][dictionary-wdflt]") {
    constexpr std::uint16_t classNumber =
        DRW_DictionaryWithDefault::kDwgClassNum;
    constexpr std::uint32_t objectHandle = 0x7A3u;

    DwgDictionaryWithDefaultWriterProbe dictionary;
    dictionary.handle = objectHandle;
    dictionary.parentHandle = DRW::NoHandle;
    dictionary.cloning = 0;
    dictionary.hardOwner = 0;
    // Keep the payload valid so the failure under test is the missing class
    // ordinal at receipt admission, not an unrelated null default handle.
    dictionary.m_defaultEntryHandle = 0x71u;
    dwgBufferW body;
    dwgBufferW handles;
    body.putObjType(DRW::AC1015, classNumber);
    const std::uint64_t objectSizeOffset = body.bitCount();
    body.putRawLong32(0);
    body.putHandle(makeObjectHandle(objectHandle));
    body.putBitShort(0);
    body.putBitLong(0);
    REQUIRE(dictionary.encodeDwg(DRW::AC1015, &body, nullptr, &handles));
    body.patchRawLong32AtBit(objectSizeOffset, body.bitCount());
    handles.alignToByte();
    body.putBytes(handles.data().data(), handles.data().size());
    body.alignToByte();
    DwgDictionaryWithDefaultReaderProbe decoded;
    dwgBuffer decodedBuffer(body.data().data(), body.data().size());
    REQUIRE(decoded.parseDwg(DRW::AC1015, &decodedBuffer));
    CHECK(decoded.hasCompleteDwgEntries());
    CHECK(decoded.m_defaultEntryHandle == 0x71u);
    const auto frame = makeObjectFrame(body.data());
    REQUIRE(!frame.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(objectHandle));
    REQUIRE(handleEntries.putModularChar(0));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1015);
    reader.addCustomObjectClass(classNumber, "ACDBDICTIONARYWDFLT");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), frame.size()));

    DwgDictionaryWithDefaultReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(frame.data()), frame.size());
    REQUIRE(reader.readDwgEntities(interface, &objectBuffer));
    REQUIRE(reader.objObjectMap.size() == 1u);
    REQUIRE(reader.hasDeferredClassificationForTest(
        objectHandle, classNumber, classNumber));
    REQUIRE(reader.readDwgObjects(interface, &objectBuffer));

    CHECK(interface.dictionaries.empty());
    CHECK(interface.unsupportedObjects.empty());
    CHECK(interface.framePublications.empty());
    CHECK(interface.typedReferences.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_reason
          == DRW_DwgFrameCoverageReason::ReceiptFailure);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
}

TEST_CASE("DWG FIELDLIST callbacks are receipt-gated",
          "[dwg][safety][fieldlist]") {
    constexpr std::uint16_t classNumber = DRW_FieldList::kDwgClassNum;
    constexpr std::uint32_t objectHandle = 0x7A4u;

    dwgBufferW body;
    body.putObjType(DRW::AC1015, classNumber);
    const std::uint64_t objectSizeOffset = body.bitCount();
    body.putRawLong32(0);
    body.putHandle(makeObjectHandle(objectHandle));
    body.putBitShort(0);
    body.putBitLong(0);
    body.putBitLong(2);
    body.putBit(1);
    body.patchRawLong32AtBit(objectSizeOffset, body.bitCount());
    body.putHandle(makeObjectHandle(0));
    body.putHandle(makeObjectHandle(0));
    dwgHandle fieldHandle = makeObjectHandle(0x7A5u);
    fieldHandle.code = DRW::DwgSoftPointer;
    body.putHandle(fieldHandle);
    body.putHandle(makeObjectHandle(DRW::NoHandle));
    const auto frame = makeObjectFrame(body.data());
    REQUIRE(!frame.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(objectHandle));
    REQUIRE(handleEntries.putModularChar(0));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    SECTION("valid receipt publishes the complete field family in order") {
        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(DRW::AC1015);
        reader.addFieldListObjectClass(classNumber);
        reader.setClassStreamOrdinalForTest(classNumber, 1u);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), frame.size()));

        DwgFieldListReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        REQUIRE(reader.readDwgEntities(interface, &objectBuffer));
        REQUIRE(reader.objObjectMap.size() == 1u);
        REQUIRE(reader.readDwgObjects(interface, &objectBuffer));

        REQUIRE(interface.fieldLists.size() == 1u);
        CHECK(interface.fieldLists.front().m_fieldHandles ==
              std::vector<std::uint32_t>{0x7A5u, DRW::NoHandle});
        REQUIRE(interface.memberships.size() == 1u);
        CHECK(interface.memberships.front().m_entries.size() == 2u);
        REQUIRE(interface.callbackOrder.size() == 4u);
        CHECK(interface.callbackOrder[0] == "frame");
        CHECK(interface.callbackOrder[1] == "membership");
        CHECK(interface.callbackOrder[2] == "list");
        CHECK(interface.callbackOrder[3] == "raw");
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition ==
              DRW_DwgFrameDisposition::Published);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 1u);
    }

    SECTION("missing class ordinal publishes no field family callback") {
        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(DRW::AC1015);
        reader.addFieldListObjectClass(classNumber);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), frame.size()));

        DwgFieldListReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        REQUIRE(reader.readDwgEntities(interface, &objectBuffer));
        REQUIRE(reader.objObjectMap.size() == 1u);
        REQUIRE(reader.readDwgObjects(interface, &objectBuffer));

        CHECK(interface.fieldLists.empty());
        CHECK(interface.memberships.empty());
        CHECK(interface.callbackOrder.empty());
        CHECK(interface.framePublications.empty());
        CHECK(interface.unsupportedObjects.empty());
        CHECK(reader.objObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition ==
              DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_reason ==
              DRW_DwgFrameCoverageReason::ReceiptFailure);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    }
}

TEST_CASE("DWG FIELD count failures publish no callback",
          "[dwg][safety][field][count]") {
    constexpr std::uint16_t classNumber = DRW_Field::kDwgClassNum;
    constexpr std::uint32_t objectHandle = 0x7A6u;

    const auto makeFrame = [](DRW::Version version, bool keyedChildValues) {
        dwgBufferW body;
        dwgBufferW strings;
        dwgBufferW handles;
        body.putObjType(version, classNumber);
        const std::uint64_t objectSizeOffset = body.bitCount();
        if (version < DRW::AC1024)
            body.putRawLong32(0);
        body.putHandle(makeObjectHandle(objectHandle));
        body.putBitShort(0);
        body.putBitLong(0);
        if (version > DRW::AC1015)
            body.putBit(1);
        if (version > DRW::AC1024)
            body.putBit(0);
        dwgBufferW* const text = version > DRW::AC1018 ? &strings : &body;
        text->putVariableText(version, "AcVariable");
        text->putVariableText(version, "%<\\AcVar Area>%");
        body.putBitLong(0);
        body.putBitLong(0);
        if (keyedChildValues) {
            if (version < DRW::AC1021)
                text->putVariableText(version, "%lf");
            for (int index = 0; index < 5; ++index)
                body.putBitLong(0);
            text->putVariableText(version, "");
            body.putBitLong(0);
            body.putBitLong(0);
            text->putVariableText(version, "");
            body.putBitLong(0);
        }
        body.putBitLong(static_cast<std::int32_t>(DRW_Field::kMaxItems + 1u));
        if (!keyedChildValues)
            body.putBitLong(0);
        if (version <= DRW::AC1018) {
            body.patchRawLong32AtBit(objectSizeOffset, body.bitCount());
            body.putHandle(makeObjectHandle(0));
            if (version <= DRW::AC1015)
                body.putHandle(makeObjectHandle(0));
            return makeObjectFrame(body.data());
        }

        body.alignToByte();
        strings.alignToByte();
        const std::uint32_t stringBytes =
            static_cast<std::uint32_t>(strings.data().size());
        if (stringBytes != 0)
            body.putBytes(strings.data().data(), stringBytes);
        for (int index = 0; index < 7; ++index)
            body.putBit(0);
        body.putRawShort16(static_cast<std::uint16_t>(stringBytes * 8u + 7u));
        body.putBit(1);
        body.alignToByte();
        handles.putHandle(makeObjectHandle(0));
        handles.alignToByte();
        if (version == DRW::AC1021)
            return makeR2007EntityFrame(body, handles);
        return makeR2013EntityFrame(body, handles);
    };

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(objectHandle));
    REQUIRE(handleEntries.putModularChar(0));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    const auto assertRejected = [&](DRW::Version version,
                                    const std::vector<std::uint8_t>& frame) {
        REQUIRE(!frame.empty());
        DwgFieldReceiptProbe directInterface;
        DwgEntityReaderProbe directReader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        directReader.setVersionForTest(version);
        directReader.addFieldObjectClass(classNumber);
        objHandle directObject(classNumber, objectHandle, 0);
        dwgBuffer directBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        CHECK_FALSE(directReader.readDwgObject(
            &directBuffer, directObject, directInterface));
        CHECK(directInterface.fields.empty());
        CHECK(directInterface.receipts.empty());
        CHECK(directInterface.unsupportedObjects.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(version);
        reader.addFieldObjectClass(classNumber);
        reader.setClassStreamOrdinalForTest(classNumber, 1u);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), frame.size()));

        DwgFieldReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        REQUIRE(reader.readDwgEntities(interface, &objectBuffer));
        REQUIRE(reader.objObjectMap.size() == 1u);
        REQUIRE(reader.readDwgObjects(interface, &objectBuffer));
        CHECK(reader.m_objectParseFailures == 1u);
        CHECK(interface.fields.empty());
        CHECK(interface.receipts.empty());
        CHECK(interface.unsupportedObjects.empty());
        CHECK(interface.framePublications.empty());
        CHECK(reader.objObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition ==
              DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    };

    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};
    for (DRW::Version version : versions) {
        CAPTURE(version);
        assertRejected(version, makeFrame(version, false));
        assertRejected(version, makeFrame(version, true));
    }
}

TEST_CASE("DWG FIELDLIST count failures publish no callback",
          "[dwg][safety][fieldlist][count]") {
    constexpr std::uint16_t classNumber = DRW_FieldList::kDwgClassNum;
    constexpr std::uint32_t objectHandle = 0x7A8u;

    const auto makeFrame = [](DRW::Version version) {
        dwgBufferW body;
        dwgBufferW handles;
        body.putObjType(version, classNumber);
        const std::uint64_t objectSizeOffset = body.bitCount();
        if (version < DRW::AC1024)
            body.putRawLong32(0);
        body.putHandle(makeObjectHandle(objectHandle));
        body.putBitShort(0);
        body.putBitLong(0);
        if (version > DRW::AC1015)
            body.putBit(1);
        if (version > DRW::AC1024)
            body.putBit(0);
        body.putBitLong(static_cast<std::int32_t>(DRW_Field::kMaxItems + 1u));
        body.putBit(0);
        if (version <= DRW::AC1018) {
            body.patchRawLong32AtBit(objectSizeOffset, body.bitCount());
            body.putHandle(makeObjectHandle(0));
            if (version <= DRW::AC1015)
                body.putHandle(makeObjectHandle(0));
            return makeObjectFrame(body.data());
        }

        body.alignToByte();
        for (int index = 0; index < 7; ++index)
            body.putBit(0);
        body.putRawShort16(0);
        body.putBit(0);
        body.alignToByte();
        handles.putHandle(makeObjectHandle(0));
        handles.alignToByte();
        if (version == DRW::AC1021)
            return makeR2007EntityFrame(body, handles);
        return makeR2013EntityFrame(body, handles);
    };

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(objectHandle));
    REQUIRE(handleEntries.putModularChar(0));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    const auto assertRejected = [&](DRW::Version version,
                                    const std::vector<std::uint8_t>& frame) {
        REQUIRE(!frame.empty());
        DwgFieldListReceiptProbe directInterface;
        DwgEntityReaderProbe directReader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        directReader.setVersionForTest(version);
        directReader.addFieldListObjectClass(classNumber);
        objHandle directObject(classNumber, objectHandle, 0);
        dwgBuffer directBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        CHECK_FALSE(directReader.readDwgObject(
            &directBuffer, directObject, directInterface));
        CHECK(directInterface.fieldLists.empty());
        CHECK(directInterface.memberships.empty());
        CHECK(directInterface.callbackOrder.empty());
        CHECK(directInterface.unsupportedObjects.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(version);
        reader.addFieldListObjectClass(classNumber);
        reader.setClassStreamOrdinalForTest(classNumber, 1u);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), frame.size()));

        DwgFieldListReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        REQUIRE(reader.readDwgEntities(interface, &objectBuffer));
        REQUIRE(reader.objObjectMap.size() == 1u);
        REQUIRE(reader.readDwgObjects(interface, &objectBuffer));
        CHECK(interface.fieldLists.empty());
        CHECK(interface.memberships.empty());
        CHECK(interface.callbackOrder.empty());
        CHECK(interface.framePublications.empty());
        CHECK(interface.unsupportedObjects.empty());
        CHECK(reader.objObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition ==
              DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    };

    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};
    for (DRW::Version version : versions) {
        CAPTURE(version);
        assertRejected(version, makeFrame(version));
    }
}

TEST_CASE("DWG FIELD handle failures publish no callback",
          "[dwg][safety][field][handle]") {
    enum class Target {
        Child,
        Object,
        CadValue
    };

    constexpr std::uint16_t classNumber = DRW_Field::kDwgClassNum;
    constexpr std::uint32_t objectHandle = 0x7AAu;

    const auto makeFrame = [](DRW::Version version, Target target,
                              bool nullHandle) {
        DwgFieldWriterProbe field;
        field.handle = objectHandle;
        field.parentHandle = DRW::NoHandle;
        field.setDwgCommonObjectState(0, 1, false);
        field.m_evaluatorId = "AcVariable";
        field.m_fieldCode = "%<\\AcVar Area>%";
        field.m_formatString = "%lf";
        field.m_value.m_formatFlags = 0;
        field.m_value.m_dataType = 1;
        field.m_value.m_value.addInt(91, 0);
        switch (target) {
        case Target::Child:
            field.m_childHandles = {0x7ABu};
            break;
        case Target::Object:
            field.m_objectHandles = {0x7ACu};
            break;
        case Target::CadValue:
            field.m_value.m_dataType = 64;
            field.m_value.m_handle = 0x7ADu;
            break;
        }

        dwgBufferW data;
        dwgBufferW strings;
        dwgBufferW handles;
        data.putObjType(version, classNumber);
        const std::uint64_t objectSizeOffset = data.bitCount();
        if (version < DRW::AC1024)
            data.putRawLong32(0);
        data.putHandle(makeObjectHandle(objectHandle));
        data.putBitShort(0);
        data.putBitLong(0);
        if (version > DRW::AC1015)
            data.putBit(1);
        if (version > DRW::AC1024)
            data.putBit(0);
        if (!field.encodeDwg(version, &data,
                             version > DRW::AC1018 ? &strings : nullptr,
                             &handles)) {
            return std::vector<std::uint8_t>{};
        }

        dwgBuffer handleReader(handles.data().data(), handles.data().size());
        handleReader.getHandle();
        if (!handleReader.isGood() || handleReader.getBitPos() != 0)
            return std::vector<std::uint8_t>{};
        const std::size_t targetOffset = handleReader.getPosition();
        const dwgHandle encodedTarget = handleReader.getHandle();
        const DRW_Field::DwgReferenceKind referenceKind =
            target == Target::Child
                ? DRW_Field::DwgReferenceKind::ChildField
                : target == Target::Object
                      ? DRW_Field::DwgReferenceKind::Object
                      : DRW_Field::DwgReferenceKind::CadValueObjectId;
        const DRW_Field::DwgReferencePolicy policy =
            DRW_Field::dwgReferencePolicy(
                version, referenceKind,
                target == Target::CadValue ? &field.m_value : nullptr);
        if (!handleReader.isGood() || encodedTarget.code != policy.wireHandleCode ||
            targetOffset >= handles.data().size()) {
            return std::vector<std::uint8_t>{};
        }
        if (nullHandle) {
            handles.data()[targetOffset] = 0;
        } else {
            const DRW::DwgHandleReferenceCode incorrectCode =
                policy.wireHandleCode == DRW::DwgSoftPointer
                    ? DRW::DwgHardPointer
                    : DRW::DwgSoftPointer;
            handles.data()[targetOffset] = static_cast<std::uint8_t>(
                (handles.data()[targetOffset] & 0x0Fu) |
                (static_cast<std::uint8_t>(incorrectCode) << 4u));
        }

        if (version <= DRW::AC1018) {
            data.alignToByte();
            data.patchRawLong32AtBit(
                objectSizeOffset, static_cast<std::uint32_t>(data.bitCount()));
            handles.alignToByte();
            data.putBytes(handles.data().data(), handles.data().size());
            return makeObjectFrame(data.data());
        }

        data.alignToByte();
        strings.alignToByte();
        const std::uint32_t stringBytes =
            static_cast<std::uint32_t>(strings.data().size());
        if (stringBytes != 0)
            data.putBytes(strings.data().data(), stringBytes);
        for (int index = 0; index < 7; ++index)
            data.putBit(0);
        data.putRawShort16(static_cast<std::uint16_t>(stringBytes * 8u + 7u));
        data.putBit(1);
        data.alignToByte();
        handles.alignToByte();
        if (version == DRW::AC1021)
            return makeR2007EntityFrame(data, handles);
        return makeR2013EntityFrame(data, handles);
    };

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(objectHandle));
    REQUIRE(handleEntries.putModularChar(0));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    const auto assertRejected = [&](DRW::Version version,
                                    const std::vector<std::uint8_t>& frame) {
        REQUIRE(!frame.empty());
        DwgFieldReceiptProbe directInterface;
        DwgEntityReaderProbe directReader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        directReader.setVersionForTest(version);
        directReader.addFieldObjectClass(classNumber);
        objHandle directObject(classNumber, objectHandle, 0);
        dwgBuffer directBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        CHECK_FALSE(directReader.readDwgObject(
            &directBuffer, directObject, directInterface));
        CHECK(directInterface.fields.empty());
        CHECK(directInterface.receipts.empty());
        CHECK(directInterface.unsupportedObjects.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(version);
        reader.addFieldObjectClass(classNumber);
        reader.setClassStreamOrdinalForTest(classNumber, 1u);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), frame.size()));

        DwgFieldReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        REQUIRE(reader.readDwgEntities(interface, &objectBuffer));
        REQUIRE(reader.objObjectMap.size() == 1u);
        REQUIRE(reader.readDwgObjects(interface, &objectBuffer));
        CHECK(interface.fields.empty());
        CHECK(interface.receipts.empty());
        CHECK(interface.unsupportedObjects.empty());
        CHECK(interface.framePublications.empty());
        CHECK(reader.objObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition ==
              DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    };

    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};
    const Target targets[] = {Target::Child, Target::Object, Target::CadValue};
    for (DRW::Version version : versions) {
        for (Target target : targets) {
            CAPTURE(version, static_cast<int>(target));
            assertRejected(version, makeFrame(version, target, false));
            assertRejected(version, makeFrame(version, target, true));
        }
    }
}

TEST_CASE("DWG FIELD unsupported values publish raw only",
          "[dwg][safety][field][raw]") {
    enum class FrameKind {
        UnsupportedValue,
        MissingValueDiscriminator,
        MissingCommonTail,
        InvalidStringFooter,
    };

    constexpr std::uint16_t classNumber = DRW_Field::kDwgClassNum;
    constexpr std::uint32_t objectHandle = 0x7B0u;

    const auto makeFrame = [](DRW::Version version, std::int32_t valueType,
                              FrameKind kind) {
        dwgBufferW body;
        dwgBufferW strings;
        dwgBufferW handles;
        body.putObjType(version, classNumber);
        const std::uint64_t objectSizeOffset = body.bitCount();
        if (version < DRW::AC1024)
            body.putRawLong32(0);
        body.putHandle(makeObjectHandle(objectHandle));
        body.putBitShort(0);
        body.putBitLong(0);
        if (version > DRW::AC1015)
            body.putBit(1);
        if (version > DRW::AC1024)
            body.putBit(0);

        dwgBufferW* const text = version > DRW::AC1018 ? &strings : &body;
        text->putVariableText(version, "AcVariable");
        text->putVariableText(version, "%<\\AcVar Area>%");
        body.putBitLong(0); // child FIELD count
        body.putBitLong(0); // object reference count
        if (version < DRW::AC1021)
            text->putVariableText(version, "%lf");
        for (int index = 0; index < 5; ++index)
            body.putBitLong(0);
        text->putVariableText(version, "");

        if (kind != FrameKind::MissingValueDiscriminator) {
            if (version > DRW::AC1018)
                body.putBitLong(0); // CadValue format flags
            body.putBitLong(valueType);
        }

        const bool hasCommonTail =
            kind != FrameKind::MissingCommonTail &&
            (kind != FrameKind::MissingValueDiscriminator ||
             version > DRW::AC1018);
        if (version <= DRW::AC1018) {
            body.patchRawLong32AtBit(
                objectSizeOffset, static_cast<std::uint32_t>(body.bitCount()));
            if (hasCommonTail) {
                body.putHandle(makeObjectHandle(0));
                if (version <= DRW::AC1015)
                    body.putHandle(makeObjectHandle(0));
            }
            return makeObjectFrame(body.data());
        }

        body.alignToByte();
        strings.alignToByte();
        const std::uint32_t stringBytes =
            static_cast<std::uint32_t>(strings.data().size());
        if (stringBytes != 0)
            body.putBytes(strings.data().data(), stringBytes);
        for (int index = 0; index < 7; ++index)
            body.putBit(0);
        if (kind == FrameKind::InvalidStringFooter) {
            body.putRawShort16(0xFFFFu); // extended string size high word
            body.putRawShort16(0xFFFFu); // impossible extended low word
        } else {
            body.putRawShort16(
                static_cast<std::uint16_t>(stringBytes * 8u + 7u));
        }
        body.putBit(1);
        body.alignToByte();
        if (hasCommonTail)
            handles.putHandle(makeObjectHandle(0));
        handles.alignToByte();
        if (version == DRW::AC1021)
            return makeR2007EntityFrame(body, handles);
        return makeR2013EntityFrame(body, handles);
    };

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(objectHandle));
    REQUIRE(handleEntries.putModularChar(0));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    const auto checkRaw = [&](DRW::Version version,
                              const std::vector<std::uint8_t>& bytes,
                              const DwgFieldReceiptProbe& interface) {
        REQUIRE(interface.fields.empty());
        REQUIRE(interface.receipts.empty());
        REQUIRE(interface.unsupportedObjects.size() == 1u);
        const DRW_UnsupportedObject& raw = interface.unsupportedObjects.front();
        CHECK(raw.m_version == version);
        CHECK(raw.m_objectType == classNumber);
        CHECK(raw.m_handle == objectHandle);
        CHECK(raw.m_isCustomClass);
        CHECK(raw.m_recordName == "FIELD");
        CHECK(raw.m_className == "AcDbField");
        CHECK_FALSE(raw.m_typedPayloadValidated);
        dwgBuffer source(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        DwgObjectFrame sourceFrame;
        REQUIRE(sourceFrame.readAt(source, version, 0));
        CHECK(raw.m_rawBytes == sourceFrame.body());
        CHECK(raw.m_bodyBitSize == sourceFrame.bodyBitSize());
    };

    const auto assertRawOnly = [&](DRW::Version version,
                                   std::int32_t valueType) {
        const auto frame = makeFrame(version, valueType,
                                     FrameKind::UnsupportedValue);
        REQUIRE(!frame.empty());

        DwgFieldReceiptProbe directInterface;
        DwgEntityReaderProbe directReader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        directReader.setVersionForTest(version);
        directReader.addFieldObjectClass(classNumber);
        objHandle directObject(classNumber, objectHandle, 0);
        dwgBuffer directBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        REQUIRE(directReader.readDwgObject(&directBuffer, directObject,
                                           directInterface));
        CHECK(directInterface.framePublications.empty());
        checkRaw(version, frame, directInterface);

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(version);
        reader.addFieldObjectClass(classNumber);
        reader.setClassStreamOrdinalForTest(classNumber, 1u);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), frame.size()));

        DwgFieldReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        REQUIRE(reader.readDwgEntities(interface, &objectBuffer));
        REQUIRE(reader.objObjectMap.size() == 1u);
        REQUIRE(reader.readDwgObjects(interface, &objectBuffer));
        CHECK(reader.objObjectMap.empty());
        checkRaw(version, frame, interface);
        REQUIRE(interface.framePublications.size() == 1u);
        CHECK(interface.framePublications.front().m_carrier ==
              DRW_DwgFramePublication::Carrier::Raw);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition ==
              DRW_DwgFrameDisposition::Published);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 1u);
    };

    const auto assertRejected = [&](DRW::Version version, FrameKind kind) {
        const auto frame = makeFrame(version, 128, kind);
        REQUIRE(!frame.empty());

        DwgFieldReceiptProbe directInterface;
        DwgEntityReaderProbe directReader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        directReader.setVersionForTest(version);
        directReader.addFieldObjectClass(classNumber);
        objHandle directObject(classNumber, objectHandle, 0);
        dwgBuffer directBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        CHECK_FALSE(directReader.readDwgObject(
            &directBuffer, directObject, directInterface));
        CHECK(directInterface.fields.empty());
        CHECK(directInterface.receipts.empty());
        CHECK(directInterface.unsupportedObjects.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(version);
        reader.addFieldObjectClass(classNumber);
        reader.setClassStreamOrdinalForTest(classNumber, 1u);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), frame.size()));

        DwgFieldReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        REQUIRE(reader.readDwgEntities(interface, &objectBuffer));
        REQUIRE(reader.objObjectMap.size() == 1u);
        // OBJECTS continues after bounded object failures, but a missing
        // frame component may instead make the phase itself fail.
        (void)reader.readDwgObjects(interface, &objectBuffer);
        CHECK(interface.fields.empty());
        CHECK(interface.receipts.empty());
        CHECK(interface.unsupportedObjects.empty());
        CHECK(interface.framePublications.empty());
        CHECK(reader.objObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition ==
              DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    };

    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};
    for (const DRW::Version version : versions) {
        CAPTURE(version);
        assertRawOnly(version, 128);
        assertRawOnly(version, 256);
        if (version <= DRW::AC1018)
            assertRawOnly(version, 512);
        assertRejected(version, FrameKind::MissingValueDiscriminator);
        assertRejected(version, FrameKind::MissingCommonTail);
        if (version > DRW::AC1018)
            assertRejected(version, FrameKind::InvalidStringFooter);
    }
}

TEST_CASE("DWG AC1015 FIELD/FIELDLIST fixture publishes typed receipts",
          "[dwg][safety][field][fieldlist][fixture]") {
    const std::filesystem::path source = findTs1Fixture();
    if (source.empty()) {
        SKIP("TS1.dwg is an external fixture; set "
                "LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR to run this case");
    }
    REQUIRE(std::filesystem::file_size(source) == 418007u);

    auto bytes = readFile(source);
    REQUIRE_FALSE(bytes.empty());

    DwgAc1015FixtureReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    DRW_Header header;
    REQUIRE(reader.readMetaData());
    REQUIRE(reader.readFileHeader());
    REQUIRE(reader.readDwgHeader(header));
    REQUIRE(reader.readDwgClasses());
    REQUIRE(reader.readDwgHandles());

    DwgPhysicalFieldFixtureProbe interface;
    dwgBuffer objects(bytes.data(), bytes.size());
    for (const std::uint32_t handle :
         {0x14Eu, 0x14Fu, 0x150u, 0x15Eu, 0x15Fu}) {
        const auto object = reader.ObjectMap.find(handle);
        REQUIRE(object != reader.ObjectMap.cend());
        REQUIRE(reader.readDwgObject(&objects, object->second, interface));
    }

    REQUIRE(interface.fields.size() == 4u);
    REQUIRE(interface.fieldReceipts.size() == 4u);
    REQUIRE(interface.fieldLists.size() == 1u);
    REQUIRE(interface.fieldListMemberships.size() == 1u);

    const auto fieldByHandle = [&interface](std::uint32_t handle) {
        return std::find_if(interface.fields.cbegin(), interface.fields.cend(),
                            [handle](const DRW_Field& field) {
                                return field.handle == handle;
                            });
    };
    const auto receiptByHandle = [&interface](std::uint32_t handle) {
        return std::find_if(
            interface.fieldReceipts.cbegin(), interface.fieldReceipts.cend(),
            [handle](const DRW_DwgFieldPayloadReceipt& receipt) {
                return receipt.m_fieldHandle == handle;
            });
    };

    const auto field14e = fieldByHandle(0x14Eu);
    REQUIRE(field14e != interface.fields.cend());
    CHECK(field14e->m_evaluatorId == "_text");
    CHECK(field14e->m_fieldCode == "%<\\_FldIdx 0>%");
    CHECK(field14e->m_childHandles == std::vector<std::uint32_t>{0x14Fu});
    REQUIRE(field14e->m_childValues.size() == 2u);
    CHECK(field14e->m_childValues[0].m_key == "ACFD_FIELDTEXT_ATTDEF");
    CHECK(field14e->m_childValues[0].m_value.m_dataType == 1);
    CHECK(field14e->m_childValues[1].m_key == "ACFD_FIELDTEXT_CHECKSUM");
    CHECK(field14e->m_childValues[1].m_value.m_dataType == 2);

    const auto field14f = fieldByHandle(0x14Fu);
    REQUIRE(field14f != interface.fields.cend());
    CHECK(field14f->m_evaluatorId == "AcSm");
    CHECK(field14f->m_fieldCode == "\\AcSm Sheet.Number \\f \"%tc1\"");
    CHECK(field14f->m_formatString == "%tc1");
    CHECK(field14f->m_valueString == "####");
    REQUIRE(field14f->m_childValues.size() == 2u);
    CHECK(field14f->m_childValues[0].m_key == "SheetSetCompName");
    CHECK(field14f->m_childValues[0].m_value.m_dataType == 4);
    CHECK(field14f->m_childValues[1].m_key == "SheetSetPropertyName");
    CHECK(field14f->m_childValues[1].m_value.m_dataType == 4);

    for (const std::uint32_t handle : {0x14Eu, 0x14Fu, 0x15Eu, 0x15Fu}) {
        const auto receipt = receiptByHandle(handle);
        REQUIRE(receipt != interface.fieldReceipts.cend());
        CHECK(receipt->m_version == DRW::AC1015);
        CHECK(receipt->m_complete);
        CHECK(receipt->m_recordName == "FIELD");
        CHECK(receipt->m_className == "AcDbField");
        CHECK(receipt->m_field.handle == handle);
    }

    const DRW_FieldList& fieldList = interface.fieldLists.front();
    CHECK(fieldList.handle == 0x150u);
    CHECK(fieldList.m_fieldHandles ==
          std::vector<std::uint32_t>{0x14Eu, 0x14Fu, 0x15Eu, 0x15Fu});
    CHECK(fieldList.hasCompleteDwgEntries());

    const DRW_DwgFieldListMembership& membership =
        interface.fieldListMemberships.front();
    CHECK(membership.m_version == DRW::AC1015);
    CHECK(membership.m_listHandle == 0x150u);
    CHECK(membership.m_complete);
    REQUIRE(membership.m_entries.size() == 4u);
    for (std::size_t index = 0; index < membership.m_entries.size(); ++index) {
        CHECK(membership.m_entries[index].m_ordinal == index);
        CHECK(membership.m_entries[index].m_fieldHandle ==
              fieldList.m_fieldHandles[index]);
    }
}

TEST_CASE("DWG AC1015 3DSOLID skips opaque ACIS data before common handles",
          "[dwg][safety][modeler][fixture]") {
    const std::filesystem::path source = findTs1Fixture();
    if (source.empty()) {
        SKIP("TS1.dwg is an external fixture; set "
                "LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR to run this case");
    }

    auto bytes = readFile(source);
    REQUIRE_FALSE(bytes.empty());

    DwgAc1015FixtureReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    DRW_Header header;
    REQUIRE(reader.readMetaData());
    REQUIRE(reader.readFileHeader());
    REQUIRE(reader.readDwgHeader(header));
    REQUIRE(reader.readDwgClasses());
    REQUIRE(reader.readDwgHandles());

    DwgReadProbe interface;
    const auto object = reader.ObjectMap.find(0x21Du);
    REQUIRE(object != reader.ObjectMap.cend());
    dwgBuffer objects(bytes.data(), bytes.size());
    REQUIRE(reader.readDwgEntity(&objects, object->second, interface));

    const auto modeler = std::find_if(
        interface.modelerGeometry.cbegin(), interface.modelerGeometry.cend(),
        [](const DRW_ModelerGeometry& geometry) {
            return geometry.handle == 0x21Du;
        });
    REQUIRE(modeler != interface.modelerGeometry.cend());
    CHECK(modeler->eType == DRW::E3DSOLID);
    CHECK(modeler->m_hasModelerData);
    CHECK_FALSE(modeler->m_isEmpty);
    CHECK(modeler->m_modelerVersion == 1u);
    CHECK_FALSE(modeler->m_rawBytes.empty());

    const auto raw = std::find_if(
        interface.unsupportedObjects.cbegin(), interface.unsupportedObjects.cend(),
        [](const DRW_UnsupportedObject& object) {
            return object.m_handle == 0x21Du
                && object.m_objectType == static_cast<int>(dwgType::SOLID3D);
        });
    REQUIRE(raw != interface.unsupportedObjects.cend());
    CHECK(raw->m_isEntity);
    CHECK(raw->m_rawBytes == modeler->m_rawBytes);
}

TEST_CASE("DWG AC1015 LEADER reads its mandatory arrowhead type",
          "[dwg][safety][leader][fixture]") {
    const std::filesystem::path source = findTs1Fixture();
    if (source.empty()) {
        SKIP("TS1.dwg is an external fixture; set "
                "LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR to run this case");
    }

    auto bytes = readFile(source);
    REQUIRE_FALSE(bytes.empty());

    DwgAc1015FixtureReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    DRW_Header header;
    REQUIRE(reader.readMetaData());
    REQUIRE(reader.readFileHeader());
    REQUIRE(reader.readDwgHeader(header));
    REQUIRE(reader.readDwgClasses());
    REQUIRE(reader.readDwgHandles());

    DwgReadProbe interface;
    const auto object = reader.ObjectMap.find(0x22Eu);
    REQUIRE(object != reader.ObjectMap.cend());
    dwgBuffer objects(bytes.data(), bytes.size());
    REQUIRE(reader.readDwgEntity(&objects, object->second, interface));

    REQUIRE(interface.leaders.size() == 1u);
    const DRW_Leader& leader = interface.leaders.front();
    CHECK(leader.handle == 0x22Eu);
    CHECK(leader.flag == 3);
    CHECK(leader.leadertype == 0);
    CHECK(leader.vertnum == 3);
    REQUIRE(leader.vertexlist.size() == 3u);
    CHECK(leader.vertexlist[0]->x == Catch::Approx(29.2808));
    CHECK(leader.vertexlist[0]->y == Catch::Approx(-1.00917));
    CHECK(leader.vertexlist[2]->x == Catch::Approx(31.4113));
    CHECK(leader.vertexlist[2]->y == Catch::Approx(-2.17332));
    CHECK(leader.textheight == 0.0);
    CHECK(leader.textwidth == 0.0);
    CHECK(leader.hookline == 1);
    CHECK(leader.arrow == 1);
    CHECK(leader.annotHandle == 0u);
}

TEST_CASE("DWG AC1015 model space completes legacy INSERT attributes",
          "[dwg][safety][compound][fixture]") {
    const std::filesystem::path source = findTs1Fixture();
    if (source.empty()) {
        SKIP("TS1.dwg is an external fixture; set "
                "LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR to run this case");
    }
    REQUIRE(std::filesystem::file_size(source) == 418007u);

    auto bytes = readFile(source);
    REQUIRE_FALSE(bytes.empty());

    DwgAc1015FixtureReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    DRW_Header header;
    REQUIRE(reader.readMetaData());
    REQUIRE(reader.readFileHeader());
    REQUIRE(reader.readDwgHeader(header));
    REQUIRE(reader.readDwgClasses());
    REQUIRE(reader.readDwgHandles());
    REQUIRE(reader.readDwgTables(header));

    const auto modelSpace = std::find_if(
        reader.blockRecordmap.cbegin(), reader.blockRecordmap.cend(),
        [](const auto& entry) {
            return entry.second != nullptr && entry.second->name == "*Model_Space";
        });
    REQUIRE(modelSpace != reader.blockRecordmap.cend());

    DwgInsertReceiptProbe interface;
    dwgBuffer objects(bytes.data(), bytes.size());
    REQUIRE(reader.walkBlockRecordEntities(
        modelSpace->second, &objects, interface, DRW::NoHandle,
        modelSpace->second->handle, DwgIntegrityAddressSpace::PhysicalFile));

    const auto insert = std::find_if(
        interface.inserts.cbegin(), interface.inserts.cend(),
        [](const DRW_Insert& value) { return value.handle == 0x160u; });
    REQUIRE(insert != interface.inserts.cend());
    REQUIRE(insert->attlist.size() == 1u);
    REQUIRE(insert->attlist.front() != nullptr);
    CHECK(insert->attlist.front()->handle == 0x162u);

    const auto publicationCount = [&interface](std::uint32_t handle) {
        return static_cast<std::size_t>(std::count_if(
            interface.publications.cbegin(), interface.publications.cend(),
            [handle](const DRW_DwgFramePublication& publication) {
                return publication.m_handle == handle;
            }));
    };
    CHECK(publicationCount(0x160u) == 1u);
    CHECK(publicationCount(0x161u) == 1u);
    CHECK(publicationCount(0x162u) == 1u);
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
}

TEST_CASE("DWG AC1015 fixture completes the BLOCKS phase",
          "[dwg][safety][compound][fixture]") {
    const std::filesystem::path source = findTs1Fixture();
    if (source.empty()) {
        SKIP("TS1.dwg is an external fixture; set "
                "LIBRECAD_EXTERNAL_DWG_FIXTURE_DIR to run this case");
    }

    auto bytes = readFile(source);
    REQUIRE_FALSE(bytes.empty());

    DwgAc1015FixtureReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    DRW_Header header;
    REQUIRE(reader.readMetaData());
    REQUIRE(reader.readFileHeader());
    REQUIRE(reader.readDwgHeader(header));
    REQUIRE(reader.readDwgClasses());
    REQUIRE(reader.readDwgHandles());
    REQUIRE(reader.readDwgTables(header));

    DwgInsertReceiptProbe interface;
    REQUIRE(reader.readDwgBlocks(interface));
    const auto insert = std::find_if(
        interface.inserts.cbegin(), interface.inserts.cend(),
        [](const DRW_Insert& value) { return value.handle == 0x160u; });
    REQUIRE(insert != interface.inserts.cend());
    REQUIRE(insert->attlist.size() == 1u);
    REQUIRE(insert->attlist.front() != nullptr);
    CHECK(insert->attlist.front()->handle == 0x162u);
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
}

TEST_CASE("DWG LEADER writes its arrowhead type before the R2000 tail",
          "[dwg][safety][leader]") {
    DwgLeaderProbe source;
    source.handle = 0x2F4u;
    source.flag = 3;
    source.leadertype = 0;
    source.hookline = 1;
    source.arrow = 1;
    source.textheight = 2.0;
    source.textwidth = 3.0;
    source.vertexlist.push_back(
        std::make_shared<DRW_Coord>(1.0, 2.0, 0.0));

    dwgBufferW body;
    REQUIRE(source.encodeDwg(DRW::AC1015, &body, 0, nullptr, nullptr));
    finalizeEncodedEntityBody(body);
    dwgBuffer buffer(body.data().data(), body.data().size());

    DwgLeaderProbe parsed;
    REQUIRE(parsed.parseDwg(DRW::AC1015, &buffer, 0));
    CHECK(parsed.handle == source.handle);
    CHECK(parsed.flag == source.flag);
    CHECK(parsed.leadertype == source.leadertype);
    CHECK(parsed.hookline == source.hookline);
    CHECK(parsed.arrow == source.arrow);
    CHECK(parsed.textheight == source.textheight);
    CHECK(parsed.textwidth == source.textwidth);
    REQUIRE(parsed.vertexlist.size() == 1u);
    CHECK(parsed.vertexlist.front()->x == 1.0);
    CHECK(parsed.vertexlist.front()->y == 2.0);
    CHECK(parsed.annotHandle == 0u);
}

TEST_CASE("DWG FIELDLIST member handle-code failures publish no callback",
          "[dwg][safety][fieldlist][handle]") {
    constexpr std::uint16_t classNumber = DRW_FieldList::kDwgClassNum;
    constexpr std::uint32_t objectHandle = 0x7AEu;

    const auto makeFrame = [](DRW::Version version) {
        dwgBufferW body;
        dwgBufferW handles;
        body.putObjType(version, classNumber);
        const std::uint64_t objectSizeOffset = body.bitCount();
        if (version < DRW::AC1024)
            body.putRawLong32(0);
        body.putHandle(makeObjectHandle(objectHandle));
        body.putBitShort(0);
        body.putBitLong(0);
        if (version > DRW::AC1015)
            body.putBit(1);
        if (version > DRW::AC1024)
            body.putBit(0);
        body.putBitLong(1);
        body.putBit(0);

        handles.putHandle(makeObjectHandle(0));
        if (version <= DRW::AC1015)
            handles.putHandle(makeObjectHandle(0));
        dwgHandle fieldHandle = makeObjectHandle(0x7AFu);
        fieldHandle.code = DRW::DwgSoftPointer;
        handles.putHandle(fieldHandle);
        dwgBuffer handleReader(handles.data().data(), handles.data().size());
        handleReader.getHandle();
        if (version <= DRW::AC1015)
            handleReader.getHandle();
        if (!handleReader.isGood() || handleReader.getBitPos() != 0)
            return std::vector<std::uint8_t>{};
        const std::size_t fieldOffset = handleReader.getPosition();
        const dwgHandle encodedField = handleReader.getHandle();
        if (!handleReader.isGood() || encodedField.code != DRW::DwgSoftPointer ||
            fieldOffset >= handles.data().size()) {
            return std::vector<std::uint8_t>{};
        }
        handles.data()[fieldOffset] = static_cast<std::uint8_t>(
            (handles.data()[fieldOffset] & 0x0Fu) |
            (static_cast<std::uint8_t>(DRW::DwgHardPointer) << 4u));

        if (version <= DRW::AC1018) {
            body.alignToByte();
            body.patchRawLong32AtBit(
                objectSizeOffset, static_cast<std::uint32_t>(body.bitCount()));
            handles.alignToByte();
            body.putBytes(handles.data().data(), handles.data().size());
            return makeObjectFrame(body.data());
        }

        body.alignToByte();
        for (int index = 0; index < 7; ++index)
            body.putBit(0);
        body.putRawShort16(0);
        body.putBit(0);
        body.alignToByte();
        handles.alignToByte();
        if (version == DRW::AC1021)
            return makeR2007EntityFrame(body, handles);
        return makeR2013EntityFrame(body, handles);
    };

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(objectHandle));
    REQUIRE(handleEntries.putModularChar(0));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    const auto assertRejected = [&](DRW::Version version,
                                    const std::vector<std::uint8_t>& frame) {
        REQUIRE(!frame.empty());
        DwgFieldListReceiptProbe directInterface;
        DwgEntityReaderProbe directReader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        directReader.setVersionForTest(version);
        directReader.addFieldListObjectClass(classNumber);
        objHandle directObject(classNumber, objectHandle, 0);
        dwgBuffer directBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        CHECK_FALSE(directReader.readDwgObject(
            &directBuffer, directObject, directInterface));
        CHECK(directInterface.fieldLists.empty());
        CHECK(directInterface.memberships.empty());
        CHECK(directInterface.callbackOrder.empty());
        CHECK(directInterface.unsupportedObjects.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(version);
        reader.addFieldListObjectClass(classNumber);
        reader.setClassStreamOrdinalForTest(classNumber, 1u);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), frame.size()));

        DwgFieldListReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        REQUIRE(reader.readDwgEntities(interface, &objectBuffer));
        REQUIRE(reader.objObjectMap.size() == 1u);
        REQUIRE(reader.readDwgObjects(interface, &objectBuffer));
        CHECK(interface.fieldLists.empty());
        CHECK(interface.memberships.empty());
        CHECK(interface.callbackOrder.empty());
        CHECK(interface.framePublications.empty());
        CHECK(interface.unsupportedObjects.empty());
        CHECK(reader.objObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition ==
              DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    };

    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};
    for (DRW::Version version : versions) {
        CAPTURE(version);
        assertRejected(version, makeFrame(version));
    }
}

TEST_CASE("DWG OBJECTS dispatch rejects entity frames independent of cache",
          "[dwg][safety]") {
    dwgBufferW body;
    body.putObjType(DRW::AC1015, dwgType::LINE);
    auto frame = makeObjectFrame(body.data());
    REQUIRE(!frame.empty());

    constexpr std::uint32_t objectHandle = 0x7A1;
    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        frame.data(), frame.size()));
    reader.setVersionForTest(DRW::AC1015);
    objHandle object(dwgObjType::DICTIONARY, objectHandle, 0);
    dwgBuffer buffer(frame.data(), frame.size());
    DwgReadProbe interface;
    bool frameFailure = false;

    CHECK_FALSE(reader.readDwgObject(
        &buffer, object, interface, &frameFailure));
    CHECK(frameFailure);
    CHECK(interface.unsupportedObjects.empty());

    DwgEntityReaderProbe phaseReader(std::make_unique<dwgBuffer>(
        frame.data(), frame.size()));
    phaseReader.setVersionForTest(DRW::AC1015);
    phaseReader.objObjectMap.emplace(objectHandle, object);
    dwgBuffer phaseBuffer(frame.data(), frame.size());
    DwgReadProbe phaseInterface;
    CHECK_FALSE(phaseReader.readDwgObjects(phaseInterface, &phaseBuffer));
    CHECK(phaseInterface.unsupportedObjects.empty());
}

TEST_CASE("DWG OBJECTS rejects misplaced block delimiters", "[dwg][safety]") {
    dwgBufferW body;
    body.putObjType(DRW::AC1015, dwgType::BLOCK);
    const auto frame = makeObjectFrame(body.data());
    REQUIRE(!frame.empty());

    constexpr std::uint32_t objectHandle = 0x7A2;
    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1015);
    objHandle object(dwgType::BLOCK, objectHandle, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    bool frameFailure = false;

    CHECK_FALSE(reader.readDwgObject(
        &buffer, object, interface, &frameFailure));
    CHECK(frameFailure);
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG object handle must match its object-map key",
          "[dwg][safety]") {
    constexpr std::uint32_t mappedHandle = 0x7A1;
    const auto body = makeMalformedDictionaryBody();
    auto frame = makeObjectFrame(body);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        frame.data(), frame.size()));
    reader.setVersionForTest(DRW::AC1015);

    dwgBuffer buffer(frame.data(), frame.size());
    DwgReadProbe interface;
    objHandle object(dwgObjType::DICTIONARY, mappedHandle, 0);
    bool frameFailure = false;
    CHECK_FALSE(reader.readDwgObject(
        &buffer, object, interface, &frameFailure));
    CHECK(frameFailure);
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG legacy reader publishes deferred raw controls",
          "[dwg][safety][pre-r13]") {
    DwgLegacyReaderProbe reader(std::make_unique<dwgBuffer>(
        static_cast<std::uint8_t*>(nullptr), 0));
    DRW_UnsupportedObject deferred;
    deferred.m_handle = 0x2D1;
    deferred.m_objectType = DRW_ViewportEntityHeader::kDwgControlType;
    deferred.m_rawBytes = {0x5A, 0xA5};
    reader.m_deferredRawObjects.push_back(deferred);

    DwgReadProbe interface;
    REQUIRE(reader.readDwgObjects(interface));
    REQUIRE(interface.unsupportedObjects.size() == 1);
    CHECK(interface.unsupportedObjects.front().m_handle == 0x2D1);
    CHECK(interface.unsupportedObjects.front().m_rawBytes
          == std::vector<std::uint8_t>{0x5A, 0xA5});
    CHECK(reader.m_deferredRawObjects.empty());
}

TEST_CASE("DWG custom entity is emitted once and not deferred as an object",
          "[dwg][safety]") {
    constexpr std::uint32_t entityHandle = 0x2C1;
    constexpr std::uint16_t classNumber = 550;

    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.setObjectType(static_cast<std::int16_t>(classNumber));
    dwgBufferW body;
    REQUIRE(line.encodeDwgCommon(DRW::AC1018, &body));
    body.putBit(1);  // both Z coordinates are zero
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1018, &body));
    const auto frame = makeEntityFrame(body);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.addCustomEntityClass(classNumber);
    CHECK(reader.customEntityDwgType(classNumber) == 0);

    objHandle entity(dwgType::LINE, entityHandle, 0);
    entity.type = classNumber;
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntity(&buffer, entity, interface));
    CHECK(entity.type == classNumber);
    CHECK(reader.objObjectMap.empty());
    REQUIRE(interface.unsupportedObjects.size() == 1);
    CHECK(interface.unsupportedObjects.front().m_isEntity);
    CHECK(interface.unsupportedObjects.front().m_handle == entityHandle);
    const DRW_UnsupportedObject& raw = interface.unsupportedObjects.front();
    CHECK(raw.m_hasClassDefinition);
    CHECK(raw.m_classAppName == "TEST_APP");
    CHECK(raw.m_classProxyFlag == 0x1234);
    CHECK(raw.m_classWasProxy);
    CHECK(raw.m_classEntityFlagRaw == 0x1F2);
    CHECK(raw.m_classDwgVersion == 1027);
    CHECK(raw.m_classMaintenanceVersion == 329);
    CHECK(raw.m_classUnknown1 == 17);
    CHECK(raw.m_classUnknown2 == 23);
}

TEST_CASE("DWG entity encoders reject null buffers",
          "[dwg][safety][writer]") {
    DwgLineWriterProbe line;

    CHECK_FALSE(line.encodeDwg(DRW::AC1018, nullptr, 0, nullptr, nullptr));
    CHECK_FALSE(line.encodeDwgCommon(DRW::AC1018, nullptr));
    CHECK_FALSE(line.encodeDwgEntHandle(DRW::AC1018, nullptr));
}

TEST_CASE("DWG entity encoders propagate poisoned buffers",
          "[dwg][safety][writer]") {
    DwgLineWriterProbe line;
    dwgBufferW badBody;
    badBody.putBitLongLong(0xFF00000000000000ULL);
    CHECK_FALSE(line.encodeDwgCommon(DRW::AC1018, &badBody));

    dwgBufferW body;
    dwgBufferW badHandles;
    badHandles.putBitLongLong(0xFF00000000000000ULL);
    CHECK_FALSE(line.encodeDwgEntHandle(DRW::AC1018, &body, &badHandles));
}

TEST_CASE("DWG entity encoders reject oversized reactor lists",
          "[dwg][safety][writer]") {
    DwgLineWriterProbe line;
    line.reactorHandles.resize(
        static_cast<std::size_t>(dwgSafety::MaxReactorCount) + 1u);
    dwgBufferW body;

    CHECK_FALSE(line.encodeDwgCommon(DRW::AC1018, &body));
    CHECK(body.data().empty());
    CHECK_FALSE(line.encodeDwgEntHandle(DRW::AC1018, &body));
}

TEST_CASE("DWG GROUP encoder rejects invalid common-object state",
          "[dwg][safety][writer]") {
    DwgGroupWriterProbe group;
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;

    group.setDwgCommonObjectState(-1, 0, false);
    CHECK_FALSE(group.encodeDwg(DRW::AC1018, &body, &strings, &handles));
    CHECK(body.data().empty());
    CHECK(strings.data().empty());
    CHECK(handles.data().empty());

    group.setDwgCommonObjectState(0, 2, false);
    CHECK_FALSE(group.encodeDwg(DRW::AC1018, &body, &strings, &handles));
    CHECK(body.data().empty());
    CHECK(strings.data().empty());
    CHECK(handles.data().empty());

    DwgGroupWriterProbe nullMember;
    nullMember.m_entityHandles = {DRW::NoHandle};
    dwgBufferW nullBody;
    dwgBufferW nullStrings;
    dwgBufferW nullHandles;
    CHECK_FALSE(nullMember.encodeDwg(
        DRW::AC1018, &nullBody, &nullStrings, &nullHandles));
    CHECK(nullBody.data().empty());
    CHECK(nullStrings.data().empty());
    CHECK(nullHandles.data().empty());
}

TEST_CASE("DWG GROUP parser rejects a null member before callbacks",
          "[dwg][safety][group]") {
    constexpr std::uint32_t groupHandle = 0x2E7u;
    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1015, dwgObjType::GROUP,
                                groupHandle);
    body.putVariableText(DRW::AC1015, "group");
    body.putBitShort(0);
    body.putBitShort(1);
    body.putBitLong(1);
    body.patchRawLong32AtBit(10, body.bitCount());
    body.putHandle(makeObjectHandle(0));
    body.putHandle(makeObjectHandle(0));
    body.putHandle(makeObjectHandle(DRW::NoHandle));
    const std::vector<std::uint8_t> frame = makeObjectFrame(body.data());
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1015);
    objHandle object(dwgObjType::GROUP, groupHandle, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgGroupReceiptProbe interface;

    CHECK_FALSE(reader.readDwgObject(&buffer, object, interface));
    CHECK(interface.groups.empty());
    CHECK(interface.memberships.empty());
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG common object encoders reject invalid shared state",
          "[dwg][safety][writer]") {
    DwgUcsWriterProbe ucs;
    dwgBufferW ucsBody;
    dwgBufferW ucsStrings;
    dwgBufferW ucsHandles;

    ucs.setDwgCommonObjectState(-1, 0, false);
    CHECK_FALSE(ucs.encodeDwg(DRW::AC1018, &ucsBody, &ucsStrings,
                             &ucsHandles));
    CHECK(ucsBody.data().empty());
    CHECK(ucsStrings.data().empty());
    CHECK(ucsHandles.data().empty());

    DwgLightListWriterProbe lightList;
    dwgBufferW lightBody;
    dwgBufferW lightStrings;
    dwgBufferW lightHandles;
    lightList.setDwgCommonObjectState(0, 2, false);
    CHECK_FALSE(lightList.encodeDwg(DRW::AC1018, &lightBody, &lightStrings,
                                    &lightHandles));
    CHECK(lightBody.data().empty());
    CHECK(lightStrings.data().empty());
    CHECK(lightHandles.data().empty());
}

TEST_CASE("DWG LIGHTLIST encoder propagates poisoned buffers",
          "[dwg][safety][writer]") {
    DwgLightListWriterProbe lightList;
    dwgBufferW badBody;
    badBody.putBitLongLong(0xFF00000000000000ULL);

    CHECK_FALSE(lightList.encodeDwg(DRW::AC1018, &badBody));
}

TEST_CASE("DWG CLASSES writer rejects an oversized class string",
          "[dwg][safety][writer]") {
    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1015;
    raw.m_objectType = 702;
    raw.m_handle = 0x2C2;
    raw.m_isCustomClass = true;
    raw.m_isEntity = false;
    raw.m_className = std::string(0xFFFFu, 'X');
    raw.m_recordName = "OVERSIZED_CLASS";

    DRW_Header legacyHeader;
    std::ofstream legacyStream;
    DwgClassPhaseWriterProbe legacy(&legacyStream, &legacyHeader);
    legacy.setVersionForTest(DRW::AC1015);
    REQUIRE(legacy.registerRawObjectClass(raw));
    CHECK_FALSE(legacy.writeDwgClasses());

    raw.m_version = DRW::AC1024;
    DRW_Header modernHeader;
    std::ofstream modernStream;
    DwgClassPhaseWriter24Probe modern(&modernStream, &modernHeader);
    REQUIRE(modern.registerRawObjectClass(raw));
    CHECK_FALSE(modern.writeDwgClasses());
}

TEST_CASE("DWG block admission rolls back failed BLOCK frames",
          "[dwg][safety][writer][ownership]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1018);
    writer.poisonOutputForTest();

    CHECK(writer.defineBlock("FAILED_BLOCK", DRW_Coord{0.0, 0.0, 0.0}) == 0);
    CHECK(writer.defineBlock("FAILED_BLOCK", DRW_Coord{0.0, 0.0, 0.0}) == 0);
    CHECK(writer.objectMapSizeForTest() == 0);
}

TEST_CASE("DWG block rollback restores the name index for retry",
          "[dwg][safety][writer][ownership]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1018);

    const auto checkpoint = writer.checkpointCompoundWrite();
    const auto first =
        writer.defineBlock("RETRY_BLOCK", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(first != 0);
    REQUIRE(writer.beginBlockContent(first));
    REQUIRE(writer.endBlockContent());

    writer.rollbackCompoundWrite(checkpoint);

    const auto retry =
        writer.defineBlock("RETRY_BLOCK", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(retry != 0);
    CHECK(writer.beginBlockContent(retry));
    CHECK(writer.endBlockContent());
}

TEST_CASE("DWG block admission rejects unrepresentable insertion units",
          "[dwg][safety][writer]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1024);

    CHECK(writer.defineBlock("NEGATIVE_UNITS", DRW_Coord{0.0, 0.0, 0.0}, -1)
          == 0);
    CHECK(writer.defineBlock("WIDE_UNITS", DRW_Coord{0.0, 0.0, 0.0},
                             static_cast<int>(std::numeric_limits<std::uint16_t>::max()) + 1)
          == 0);
}

TEST_CASE("DWG deferred block control rolls back partial ownership writes",
          "[dwg][safety][writer][ownership]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1018);
    const auto firstBlock =
        writer.defineBlock("FIRST_BLOCK", DRW_Coord{0.0, 0.0, 0.0});
    const auto secondBlock =
        writer.defineBlock("SECOND_BLOCK", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(firstBlock != 0);
    REQUIRE(secondBlock != 0);

    const std::size_t bufferSize = writer.bufferSizeForTest();
    writer.addObjectMapEntryForTest(secondBlock, 0);
    const std::size_t objectMapSize = writer.objectMapSizeForTest();

    DRW::DwgBlockWriteResult result;
    CHECK_FALSE(writer.getEmittedDwgBlockWriteResult(result));
    CHECK_FALSE(writer.emitDeferredBlockControl());
    CHECK_FALSE(writer.getEmittedDwgBlockWriteResult(result));
    CHECK(writer.bufferSizeForTest() == bufferSize);
    CHECK(writer.objectMapSizeForTest() == objectMapSize);
}

TEST_CASE("DWG entity admission rolls back an invalid block owner",
          "[dwg][safety][writer][ownership]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1018);
    const auto blockRecord =
        writer.defineBlock("INVALID_OWNER", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(blockRecord != 0);
    REQUIRE(writer.beginBlockContent(blockRecord));

    const std::size_t objectMapSize = writer.objectMapSizeForTest();
    DRW_Line line;
    line.parentHandle = 0xDEADu;
    CHECK_FALSE(writer.encodeEntity(&line));
    CHECK(writer.objectMapSizeForTest() == objectMapSize);
    CHECK(writer.endBlockContent());
}

TEST_CASE("DWG writer rejects unrepresentable owned-entity counts",
          "[dwg][safety][writer][ownership]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1018);

    const std::size_t initialSize = writer.bufferSizeForTest();
    std::vector<std::uint32_t> tooManyEntities(
        static_cast<std::size_t>(dwgSafety::MaxOwnedObjectCount) + 1u, 0x2C1u);
    CHECK_FALSE(writer.emitBlockRecord(
        0x2C2u, "OVERFULL_BLOCK", DRW_Coord{0.0, 0.0, 0.0},
        0x2C3u, 0x2C4u, tooManyEntities));
    CHECK(writer.bufferSizeForTest() == initialSize);

    CHECK_FALSE(writer.emitControlObject(
        0x01u, 0x2C5u,
        static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())
            + 1u,
        {}));
    CHECK(writer.bufferSizeForTest() == initialSize);
}

TEST_CASE("DWG opaque block entities validate their owner handle",
          "[dwg][safety]") {
    constexpr std::uint32_t entityHandle = 0x2D1;
    constexpr std::uint32_t ownerHandle = 0x2D2;
    constexpr std::uint32_t foreignOwnerHandle = 0x2D3;
    constexpr std::uint16_t classNumber = 551;

    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.parentHandle = ownerHandle;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.setObjectType(static_cast<std::int16_t>(classNumber));
    dwgBufferW body;
    dwgBufferW handleStream;
    REQUIRE(line.encodeDwgCommon(DRW::AC1021, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1021, &body, &handleStream));
    const auto frame = makeR2007EntityFrame(body, handleStream);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe validReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    validReader.setVersionForTest(DRW::AC1021);
    validReader.addCustomEntityClass(classNumber);
    validReader.setExpectedBlockOwnerForTest(ownerHandle);
    objHandle validEntity(classNumber, entityHandle, 0);
    dwgBuffer validBuffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe validInterface;
    REQUIRE(validReader.readDwgEntity(
        &validBuffer, validEntity, validInterface));
    REQUIRE(validInterface.unsupportedObjects.size() == 1);
    CHECK(validInterface.unsupportedObjects.front().m_handle == entityHandle);
    CHECK(validInterface.unsupportedObjects.front().m_parentHandle == ownerHandle);
    CHECK(validInterface.unsupportedObjects.front().m_blockOwnerHandle == ownerHandle);

    DwgEntityReaderProbe foreignReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    foreignReader.setVersionForTest(DRW::AC1021);
    foreignReader.addCustomEntityClass(classNumber);
    foreignReader.setExpectedBlockOwnerForTest(foreignOwnerHandle);
    objHandle foreignEntity(classNumber, entityHandle, 0);
    dwgBuffer foreignBuffer(
        const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe foreignInterface;
    CHECK_FALSE(foreignReader.readDwgEntity(
        &foreignBuffer, foreignEntity, foreignInterface));
    CHECK(foreignInterface.unsupportedObjects.empty());
}

TEST_CASE("DWG raw custom entities publish only owned block entries",
          "[dwg][safety][writer]") {
    constexpr std::uint16_t classNumber = 702;
    constexpr std::uint32_t entityHandle = 0x2E1u;

    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1018);
    const std::uint32_t blockRecord =
        writer.defineBlock("RAW_BLOCK", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(blockRecord != 0);

    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.parentHandle = blockRecord;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.setObjectType(static_cast<std::int16_t>(classNumber));
    dwgBufferW body;
    REQUIRE(line.encodeDwgCommon(DRW::AC1018, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1018, &body));

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1018;
    raw.m_objectType = classNumber;
    raw.m_handle = entityHandle;
    raw.m_parentHandle = blockRecord;
    raw.m_blockOwnerHandle = blockRecord;
    raw.m_objectSize = static_cast<std::uint32_t>(body.data().size());
    raw.m_isEntity = true;
    raw.m_isCustomClass = true;
    raw.m_recordName = "RAW_ENTITY";
    raw.m_className = "AcDbRawEntity";
    raw.m_rawBytes = body.data();

    REQUIRE(writer.registerRawObjectClass(raw));
    REQUIRE(writer.replayRawObject(raw));

    DRW_UnsupportedObject duplicate = raw;
    CHECK_FALSE(writer.replayRawObject(duplicate));

    DRW_UnsupportedObject remapped = raw;
    remapped.m_handle = entityHandle + 2;
    remapped.m_parentHandle = blockRecord + 0x100;
    remapped.m_blockOwnerHandle = blockRecord;
    CHECK_FALSE(writer.replayRawObject(remapped));

    DRW_UnsupportedObject unowned = raw;
    unowned.m_handle = entityHandle + 1;
    unowned.m_parentHandle = DRW::NoHandle;
    unowned.m_blockOwnerHandle = DRW::NoHandle;
    CHECK_FALSE(writer.replayRawObject(unowned));
}

TEST_CASE("DWG space walks reject foreign entity owners",
          "[dwg][safety][ownership]") {
    constexpr std::uint16_t classNumber = 706;
    constexpr std::uint32_t entityHandle = 0x2E8u;
    constexpr std::uint32_t foreignOwner = 0x7C3u;

    const auto frame = makeUnknownCustomEntityFrame(
        classNumber, entityHandle, foreignOwner);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setOwnerlessSpaceWalkForTest(true);
    reader.addCustomEntityClass(classNumber);
    objHandle entity(classNumber, entityHandle, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;

    CHECK_FALSE(reader.readDwgEntity(&buffer, entity, interface));
    CHECK(interface.unsupportedObjects.empty());
    CHECK(reader.parsedHandleMismatchForTest() == false);

    const auto ownerlessFrame = makeUnknownCustomEntityFrame(
        classNumber, entityHandle + 1u, DRW::NoHandle);
    REQUIRE(!ownerlessFrame.empty());
    DwgEntityReaderProbe ownerlessReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(ownerlessFrame.data()), ownerlessFrame.size()));
    ownerlessReader.setVersionForTest(DRW::AC1018);
    ownerlessReader.setOwnerlessSpaceWalkForTest(true);
    ownerlessReader.addCustomEntityClass(classNumber);
    objHandle ownerlessEntity(classNumber, entityHandle + 1u, 0);
    dwgBuffer ownerlessBuffer(
        const_cast<std::uint8_t*>(ownerlessFrame.data()), ownerlessFrame.size());
    DwgReadProbe ownerlessInterface;

    REQUIRE(ownerlessReader.readDwgEntity(
        &ownerlessBuffer, ownerlessEntity, ownerlessInterface));
    REQUIRE(ownerlessInterface.unsupportedObjects.size() == 1u);
    CHECK(ownerlessInterface.unsupportedObjects.front().m_parentHandle
          == DRW::NoHandle);
}

TEST_CASE("DWG raw custom entities publish model and paper space entries",
          "[dwg][safety][writer]") {
    constexpr std::uint16_t classNumber = 703;
    constexpr std::uint32_t firstHandle = 0x2E3u;

    const std::array<std::uint32_t, 2> owners = {
        DRW::DwgModelSpaceBlockRecordHandle,
        DRW::DwgPaperSpaceBlockRecordHandle};
    for (std::size_t index = 0; index < owners.size(); ++index) {
        const std::uint32_t entityHandle =
            firstHandle + static_cast<std::uint32_t>(index);
        DwgLineWriterProbe line;
        line.handle = entityHandle;
        line.parentHandle = DRW::NoHandle;
        line.space = index == 0 ? DRW::ModelSpace : DRW::PaperSpace;
        line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
        line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
        line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        line.setObjectType(static_cast<std::int16_t>(classNumber));
        dwgBufferW body;
        dwgBufferW handleStream;
        REQUIRE(line.encodeDwgCommon(DRW::AC1021, &body));
        body.putBit(1);
        body.putRawDouble(line.basePoint.x);
        body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
        body.putRawDouble(line.basePoint.y);
        body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
        body.putThickness(line.thickness, true);
        body.putExtrusion(line.extPoint, true);
        REQUIRE(line.encodeDwgEntHandle(DRW::AC1021, &body,
                                        &handleStream));
        const auto sourceFrame = makeR2007EntityFrame(body, handleStream);
        REQUIRE(!sourceFrame.empty());
        DwgObjectFrame source;
        dwgBuffer sourceBuffer(
            const_cast<std::uint8_t*>(sourceFrame.data()),
            sourceFrame.size());
        REQUIRE(source.readAt(sourceBuffer, DRW::AC1021, 0));

        DRW_UnsupportedObject raw;
        raw.m_version = DRW::AC1021;
        raw.m_objectType = classNumber;
        raw.m_handle = entityHandle;
        raw.m_blockOwnerHandle = owners[index];
        raw.m_objectSize = static_cast<std::uint32_t>(source.body().size());
        raw.m_isEntity = true;
        raw.m_isCustomClass = true;
        raw.m_recordName = "RAW_SPACE_ENTITY";
        raw.m_className = "AcDbRawSpaceEntity";
        raw.m_rawBytes = source.body();

        DRW_Header header;
        std::ofstream stream;
        DwgClassPhaseWriterProbe writer(&stream, &header);
        writer.setVersionForTest(DRW::AC1021);
        REQUIRE(writer.registerRawObjectClass(raw));
        REQUIRE(writer.replayRawObject(raw));
        CHECK_FALSE(writer.replayRawObject(raw));
    }
}

TEST_CASE("DWG raw AC1021 entities patch remapped block owners",
          "[dwg][safety][writer]") {
    constexpr std::uint16_t classNumber = 704;
    constexpr std::uint32_t entityHandle = 0x2E6u;
    constexpr std::uint32_t sourceOwner = 0x700u;

    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.parentHandle = sourceOwner;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.setObjectType(static_cast<std::int16_t>(classNumber));
    dwgBufferW body;
    REQUIRE(line.encodeDwgCommon(DRW::AC1021, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    dwgBufferW handleStream;
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1021, &body, &handleStream));
    const auto sourceFrame = makeR2007EntityFrame(body, handleStream);
    REQUIRE(!sourceFrame.empty());
    DwgObjectFrame source;
    dwgBuffer sourceBuffer(
        const_cast<std::uint8_t*>(sourceFrame.data()), sourceFrame.size());
    REQUIRE(source.readAt(sourceBuffer, DRW::AC1021, 0));
    DwgLineReaderProbe sourceParsed;
    dwgBuffer sourceBody(source.body().data(), source.body().size());
    REQUIRE(sourceParsed.parseDwg(
        DRW::AC1021, &sourceBody, source.bodyBitSize()));
    CHECK(sourceParsed.parentHandle == sourceOwner);

    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1021);
    const std::uint32_t targetOwner =
        writer.defineBlock("RAW_REMAP", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(targetOwner != 0);
    REQUIRE(targetOwner != sourceOwner);

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1021;
    raw.m_objectType = classNumber;
    raw.m_handle = entityHandle;
    raw.m_parentHandle = sourceOwner;
    raw.m_blockOwnerHandle = targetOwner;
    raw.m_objectSize = static_cast<std::uint32_t>(source.body().size());
    raw.m_isEntity = true;
    raw.m_isCustomClass = true;
    raw.m_recordName = "RAW_REMAP_ENTITY";
    raw.m_className = "AcDbRawRemapEntity";
    raw.m_rawBytes = source.body();

    REQUIRE(writer.registerRawObjectClass(raw));
    REQUIRE(writer.replayRawObject(raw));

    DwgObjectFrame emitted;
    dwgBuffer emittedBuffer(
        const_cast<std::uint8_t*>(writer.buffer().data()),
        writer.buffer().size());
    const std::uint32_t emittedOffset = writer.lastObjectOffsetForTest();
    REQUIRE(emittedOffset != 0);
    REQUIRE(emitted.readAt(emittedBuffer, DRW::AC1021, emittedOffset));
    dwgBuffer ownerProbe(emitted.body().data(), emitted.body().size());
    (void)ownerProbe.getObjType(DRW::AC1021);
    const std::uint32_t emittedObjectDataBits = ownerProbe.getRawLong32();
    REQUIRE(ownerProbe.setPosition(emittedObjectDataBits >> 3));
    ownerProbe.setBitPos(static_cast<std::uint8_t>(emittedObjectDataBits & 7u));
    const dwgHandle emittedOwner = ownerProbe.getOffsetHandle(entityHandle);
    REQUIRE(ownerProbe.isGood());
    CHECK(emittedOwner.ref == targetOwner);
    DwgLineReaderProbe parsed;
    dwgBuffer emittedBody(
        emitted.body().data(), emitted.body().size());
    REQUIRE(parsed.parseDwg(
        DRW::AC1021, &emittedBody, emitted.bodyBitSize()));
    CHECK(parsed.parentHandle == targetOwner);
    CHECK(parsed.parentHandle != sourceOwner);
}

TEST_CASE("DWG raw AC1024 entities patch remapped block owners",
          "[dwg][safety][writer]") {
    constexpr std::uint16_t classNumber = 705;
    constexpr std::uint32_t entityHandle = 0x2E7u;
    constexpr std::uint32_t sourceOwner = 0x700u;

    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.parentHandle = sourceOwner;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.setObjectType(static_cast<std::int16_t>(classNumber));
    dwgBufferW body;
    dwgBufferW handleStream;
    REQUIRE(line.encodeDwgCommon(DRW::AC1024, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1024, &body, &handleStream));
    body.alignToByte();
    const auto sourceFrame = makeR2013EntityFrame(body, handleStream);
    REQUIRE(!sourceFrame.empty());
    DwgObjectFrame source;
    dwgBuffer sourceBuffer(
        const_cast<std::uint8_t*>(sourceFrame.data()), sourceFrame.size());
    REQUIRE(source.readAt(sourceBuffer, DRW::AC1024, 0));
    REQUIRE(source.bodyBitSize() != 0);
    DwgLineReaderProbe sourceParsed;
    dwgBuffer sourceBody(source.body().data(), source.body().size());
    REQUIRE(sourceParsed.parseDwg(
        DRW::AC1024, &sourceBody, source.bodyBitSize()));
    CHECK(sourceParsed.parentHandle == sourceOwner);

    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriter24Probe writer(&stream, &header);
    const std::uint32_t targetOwner =
        writer.defineBlock("RAW_REMAP_24", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(targetOwner != 0);
    REQUIRE(targetOwner != sourceOwner);

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1024;
    raw.m_objectType = classNumber;
    raw.m_handle = entityHandle;
    raw.m_parentHandle = sourceOwner;
    raw.m_blockOwnerHandle = targetOwner;
    raw.m_bodyBitSize = source.bodyBitSize();
    raw.m_objectSize = static_cast<std::uint32_t>(source.body().size());
    raw.m_isEntity = true;
    raw.m_isCustomClass = true;
    raw.m_recordName = "RAW_REMAP_ENTITY_24";
    raw.m_className = "AcDbRawRemapEntity24";
    raw.m_rawBytes = source.body();

    REQUIRE(writer.registerRawObjectClass(raw));
    REQUIRE(writer.replayRawObject(raw));

    DwgObjectFrame emitted;
    dwgBuffer emittedBuffer(
        const_cast<std::uint8_t*>(writer.buffer().data()),
        writer.buffer().size());
    const std::uint32_t emittedOffset = writer.lastObjectOffsetForTest();
    REQUIRE(emittedOffset != 0);
    REQUIRE(emitted.readAt(emittedBuffer, DRW::AC1024, emittedOffset));
    CHECK(emitted.bodyBitSize() != source.bodyBitSize());
    DwgLineReaderProbe parsed;
    dwgBuffer emittedBody(emitted.body().data(), emitted.body().size());
    REQUIRE(parsed.parseDwg(
        DRW::AC1024, &emittedBody, emitted.bodyBitSize()));
    CHECK(parsed.parentHandle == targetOwner);
    CHECK(parsed.parentHandle != sourceOwner);
}

TEST_CASE("DWG AC1021 raw replay adjusts a changed own-handle width",
          "[dwg][safety][writer]") {
    constexpr std::uint16_t classNumber = 706;
    constexpr std::uint32_t sourceHandle = 0x701u;
    constexpr std::uint32_t targetHandle = 0x33u;
    constexpr std::uint32_t sourceOwner = 0x700u;

    DwgLineWriterProbe line;
    line.handle = sourceHandle;
    line.parentHandle = sourceOwner;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.setObjectType(static_cast<std::int16_t>(classNumber));
    dwgBufferW body;
    dwgBufferW handleStream;
    REQUIRE(line.encodeDwgCommon(DRW::AC1021, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1021, &body, &handleStream));
    const auto sourceFrame = makeR2007EntityFrame(body, handleStream);
    REQUIRE(!sourceFrame.empty());
    DwgObjectFrame source;
    dwgBuffer sourceBuffer(
        const_cast<std::uint8_t*>(sourceFrame.data()), sourceFrame.size());
    REQUIRE(source.readAt(sourceBuffer, DRW::AC1021, 0));

    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1021);
    const std::uint32_t targetOwner =
        writer.defineBlock("RAW_OWN_HANDLE", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(targetOwner != 0);

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1021;
    raw.m_objectType = classNumber;
    raw.m_handle = targetHandle;
    raw.m_parentHandle = sourceOwner;
    raw.m_blockOwnerHandle = targetOwner;
    raw.m_objectSize = static_cast<std::uint32_t>(source.body().size());
    raw.m_isEntity = true;
    raw.m_isCustomClass = true;
    raw.m_recordName = "RAW_OWN_HANDLE_ENTITY";
    raw.m_className = "AcDbRawOwnHandleEntity";
    raw.m_rawBytes = source.body();

    REQUIRE(writer.registerRawObjectClass(raw));
    REQUIRE(writer.replayRawObject(raw));

    DwgObjectFrame emitted;
    dwgBuffer emittedBuffer(
        const_cast<std::uint8_t*>(writer.buffer().data()),
        writer.buffer().size());
    const std::uint32_t emittedOffset = writer.lastObjectOffsetForTest();
    REQUIRE(emittedOffset != 0);
    REQUIRE(emitted.readAt(emittedBuffer, DRW::AC1021, emittedOffset));
    DwgLineReaderProbe parsed;
    dwgBuffer emittedBody(emitted.body().data(), emitted.body().size());
    REQUIRE(parsed.parseDwg(
        DRW::AC1021, &emittedBody, emitted.bodyBitSize()));
    CHECK(parsed.handle == targetHandle);
    CHECK(parsed.parentHandle == targetOwner);
}

TEST_CASE("DWG AC1024 raw replay keeps owner after class ordinal remap",
          "[dwg][safety][writer]") {
    constexpr std::uint16_t sourceClass = 800;
    constexpr std::uint32_t entityHandle = 0x34u;
    constexpr std::uint32_t sourceOwner = 0x700u;

    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.parentHandle = sourceOwner;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.setObjectType(static_cast<std::int16_t>(sourceClass));
    dwgBufferW body;
    dwgBufferW handleStream;
    REQUIRE(line.encodeDwgCommon(DRW::AC1024, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1024, &body, &handleStream));
    body.alignToByte();
    const auto sourceFrame = makeR2013EntityFrame(body, handleStream);
    REQUIRE(!sourceFrame.empty());
    DwgObjectFrame source;
    dwgBuffer sourceBuffer(
        const_cast<std::uint8_t*>(sourceFrame.data()), sourceFrame.size());
    REQUIRE(source.readAt(sourceBuffer, DRW::AC1024, 0));

    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriter24Probe writer(&stream, &header);
    const std::uint32_t targetOwner =
        writer.defineBlock("RAW_CLASS_REMAP", DRW_Coord{0.0, 0.0, 0.0});
    REQUIRE(targetOwner != 0);

    DRW_UnsupportedObject firstClass;
    firstClass.m_version = DRW::AC1024;
    firstClass.m_objectType = sourceClass;
    firstClass.m_handle = 0x35u;
    firstClass.m_isEntity = true;
    firstClass.m_isCustomClass = true;
    firstClass.m_recordName = "RAW_CLASS_A";
    firstClass.m_className = "AcDbRawClassA";
    REQUIRE(writer.registerRawObjectClass(firstClass));

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1024;
    raw.m_objectType = sourceClass;
    raw.m_handle = entityHandle;
    raw.m_parentHandle = sourceOwner;
    raw.m_blockOwnerHandle = targetOwner;
    raw.m_bodyBitSize = source.bodyBitSize();
    raw.m_objectSize = static_cast<std::uint32_t>(source.body().size());
    raw.m_isEntity = true;
    raw.m_isCustomClass = true;
    raw.m_recordName = "RAW_CLASS_B";
    raw.m_className = "AcDbRawClassB";
    raw.m_rawBytes = source.body();
    REQUIRE(writer.registerRawObjectClass(raw));
    const std::uint16_t targetClass = writer.remappedRawClassNum(raw);
    REQUIRE(targetClass != sourceClass);
    REQUIRE(writer.replayRawObject(raw));

    DwgObjectFrame emitted;
    dwgBuffer emittedBuffer(
        const_cast<std::uint8_t*>(writer.buffer().data()),
        writer.buffer().size());
    const std::uint32_t emittedOffset = writer.lastObjectOffsetForTest();
    REQUIRE(emittedOffset != 0);
    REQUIRE(emitted.readAt(emittedBuffer, DRW::AC1024, emittedOffset));
    dwgBuffer typeProbe(emitted.body().data(), emitted.body().size());
    CHECK(typeProbe.getObjType(DRW::AC1024) == targetClass);
    DwgLineReaderProbe parsed;
    dwgBuffer emittedBody(emitted.body().data(), emitted.body().size());
    REQUIRE(parsed.parseDwg(
        DRW::AC1024, &emittedBody, emitted.bodyBitSize()));
    CHECK(parsed.handle == entityHandle);
    CHECK(parsed.parentHandle == targetOwner);
}

TEST_CASE("DWG fixed entity shells require validated standard framing",
          "[dwg][safety][writer]") {
    constexpr std::uint32_t entityHandle = 0x2D4;
    constexpr std::uint32_t ownerHandle = 0x2D5;
    constexpr int shellType = DRW_UnsupportedObject::kFixedEntityShellFirstType;

    DwgLineWriterProbe shell;
    shell.handle = entityHandle;
    shell.parentHandle = ownerHandle;
    shell.setObjectType(static_cast<std::int16_t>(shellType));
    dwgBufferW body;
    dwgBufferW handleStream;
    REQUIRE(shell.encodeDwgCommon(DRW::AC1021, &body));
    REQUIRE(shell.encodeDwgEntHandle(DRW::AC1021, &body, &handleStream));
    const auto frame = makeR2007EntityFrame(body, handleStream);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1021);
    objHandle entity(shellType, entityHandle, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntity(&buffer, entity, interface));
    REQUIRE(interface.unsupportedObjects.size() == 1);
    const DRW_UnsupportedObject& raw = interface.unsupportedObjects.front();
    CHECK(raw.m_isEntity);
    CHECK_FALSE(raw.m_isCustomClass);
    CHECK(raw.m_objectType == shellType);
    CHECK(raw.m_recordName == "ALIGNMENTPARAMETERENTITY");
    std::vector<std::uint8_t> expectedRaw = body.data();
    expectedRaw.insert(expectedRaw.end(), handleStream.data().begin(),
                       handleStream.data().end());
    CHECK(raw.m_rawBytes == expectedRaw);

    DwgLineWriterProbe incompleteShell;
    incompleteShell.handle = entityHandle;
    incompleteShell.setObjectType(static_cast<std::int16_t>(shellType));
    dwgBufferW incompleteBody;
    REQUIRE(incompleteShell.encodeDwgCommon(DRW::AC1021, &incompleteBody));
    dwgBufferW noHandles;
    const auto incompleteFrame = makeR2007EntityFrame(incompleteBody, noHandles);
    REQUIRE(!incompleteFrame.empty());

    DwgEntityReaderProbe incompleteReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(incompleteFrame.data()), incompleteFrame.size()));
    incompleteReader.setVersionForTest(DRW::AC1021);
    objHandle incompleteEntity(shellType, entityHandle, 0);
    dwgBuffer incompleteBuffer(
        const_cast<std::uint8_t*>(incompleteFrame.data()), incompleteFrame.size());
    DwgReadProbe incompleteInterface;
    CHECK_FALSE(incompleteReader.readDwgEntity(
        &incompleteBuffer, incompleteEntity, incompleteInterface));
    CHECK(incompleteInterface.unsupportedObjects.empty());

    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1021);
    REQUIRE(writer.replayRawObject(raw));
    DRW_UnsupportedObject unsupported = raw;
    unsupported.m_objectType = shellType + 19;
    CHECK_FALSE(writer.replayRawObject(unsupported));
}

TEST_CASE("DWG fixed ACME object shells require validated standard framing",
          "[dwg][safety][writer]") {
    constexpr int shellType =
        DRW_UnsupportedObject::kFixedObjectShellFirstType;
    constexpr std::uint32_t objectHandle = 0x2D6;

    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1021,
                                static_cast<std::uint16_t>(shellType),
                                objectHandle);
    body.putBit(0);  // R2007 string-stream footer: no strings.

    dwgBufferW handleStream;
    handleStream.putHandle(makeObjectHandle(0));  // owner
    handleStream.putHandle(makeObjectHandle(0));  // xdictionary
    const auto frame = makeR2007EntityFrame(body, handleStream);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1021);
    objHandle object(shellType, objectHandle, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgObject(&buffer, object, interface));
    REQUIRE(interface.unsupportedObjects.size() == 1);
    const DRW_UnsupportedObject& raw = interface.unsupportedObjects.front();
    CHECK_FALSE(raw.m_isEntity);
    CHECK_FALSE(raw.m_isCustomClass);
    CHECK(raw.m_objectType == shellType);
    CHECK(raw.m_recordName == "ACMECOMMANDHISTORY");
    std::vector<std::uint8_t> expectedRaw = body.data();
    expectedRaw.insert(expectedRaw.end(), handleStream.data().begin(),
                       handleStream.data().end());
    CHECK(raw.m_rawBytes == expectedRaw);

    dwgBufferW noHandles;
    const auto incompleteFrame = makeR2007EntityFrame(body, noHandles);
    REQUIRE(!incompleteFrame.empty());
    DwgEntityReaderProbe incompleteReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(incompleteFrame.data()),
        incompleteFrame.size()));
    incompleteReader.setVersionForTest(DRW::AC1021);
    objHandle incompleteObject(shellType, objectHandle, 0);
    dwgBuffer incompleteBuffer(
        const_cast<std::uint8_t*>(incompleteFrame.data()),
        incompleteFrame.size());
    DwgReadProbe incompleteInterface;
    CHECK_FALSE(incompleteReader.readDwgObject(
        &incompleteBuffer, incompleteObject, incompleteInterface));
    CHECK(incompleteInterface.unsupportedObjects.empty());

    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1021);
    REQUIRE(writer.replayRawObject(raw));
    DRW_UnsupportedObject crossVersion = raw;
    crossVersion.m_version = DRW::AC1024;
    CHECK_FALSE(writer.replayRawObject(crossVersion));
}

TEST_CASE("DWG fixed AEC shells retain their entity and OBJECTS framing",
          "[dwg][safety][writer]") {
    constexpr int entityShellType =
        DRW_UnsupportedObject::kFixedAecEntityShellType;
    constexpr std::uint32_t entityHandle = 0x2D7;
    constexpr std::uint32_t entityOwnerHandle = 0x2D8;

    DwgLineWriterProbe entityShell;
    entityShell.handle = entityHandle;
    entityShell.parentHandle = entityOwnerHandle;
    entityShell.setObjectType(static_cast<std::int16_t>(entityShellType));
    dwgBufferW entityBody;
    dwgBufferW entityHandleStream;
    REQUIRE(entityShell.encodeDwgCommon(DRW::AC1021, &entityBody));
    REQUIRE(entityShell.encodeDwgEntHandle(DRW::AC1021, &entityBody,
                                           &entityHandleStream));
    const auto entityFrame = makeR2007EntityFrame(entityBody,
                                                   entityHandleStream);
    REQUIRE(!entityFrame.empty());

    DwgEntityReaderProbe entityReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(entityFrame.data()), entityFrame.size()));
    entityReader.setVersionForTest(DRW::AC1021);
    objHandle entity(entityShellType, entityHandle, 0);
    dwgBuffer entityBuffer(const_cast<std::uint8_t*>(entityFrame.data()),
                           entityFrame.size());
    DwgReadProbe entityInterface;
    REQUIRE(entityReader.readDwgEntity(&entityBuffer, entity, entityInterface));
    REQUIRE(entityInterface.unsupportedObjects.size() == 1);
    const DRW_UnsupportedObject& rawEntity =
        entityInterface.unsupportedObjects.front();
    CHECK(rawEntity.m_isEntity);
    CHECK(rawEntity.m_recordName == "AEC_WALL");

    dwgBufferW noEntityHandles;
    const auto incompleteEntityFrame = makeR2007EntityFrame(entityBody,
                                                             noEntityHandles);
    REQUIRE(!incompleteEntityFrame.empty());
    DwgEntityReaderProbe incompleteEntityReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(incompleteEntityFrame.data()),
        incompleteEntityFrame.size()));
    incompleteEntityReader.setVersionForTest(DRW::AC1021);
    objHandle incompleteEntity(entityShellType, entityHandle, 0);
    dwgBuffer incompleteEntityBuffer(
        const_cast<std::uint8_t*>(incompleteEntityFrame.data()),
        incompleteEntityFrame.size());
    DwgReadProbe incompleteEntityInterface;
    CHECK_FALSE(incompleteEntityReader.readDwgEntity(
        &incompleteEntityBuffer, incompleteEntity, incompleteEntityInterface));
    CHECK(incompleteEntityInterface.unsupportedObjects.empty());

    const std::array<std::pair<int, const char*>, 3> objectShells = {{
        {1128, "AEC_WALL_STYLE"},
        {1129, "AEC_CLEANUP_GROUP"},
        {1130, "AECDBBINRECORD"},
    }};
    for (const auto& [shellType, name] : objectShells) {
        const std::uint32_t objectHandle =
            0x2D9u + static_cast<std::uint32_t>(shellType - 1128);
        dwgBufferW objectBody;
        putDictionaryObjectPreamble(objectBody, DRW::AC1021,
                                    static_cast<std::uint16_t>(shellType),
                                    objectHandle);
        objectBody.putBit(0);
        dwgBufferW objectHandleStream;
        objectHandleStream.putHandle(makeObjectHandle(0));
        objectHandleStream.putHandle(makeObjectHandle(0));
        const auto objectFrame = makeR2007EntityFrame(objectBody,
                                                       objectHandleStream);
        REQUIRE(!objectFrame.empty());

        DwgEntityReaderProbe objectReader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(objectFrame.data()), objectFrame.size()));
        objectReader.setVersionForTest(DRW::AC1021);
        objHandle object(shellType, objectHandle, 0);
        dwgBuffer objectBuffer(const_cast<std::uint8_t*>(objectFrame.data()),
                               objectFrame.size());
        DwgReadProbe objectInterface;
        REQUIRE(objectReader.readDwgObject(&objectBuffer, object,
                                           objectInterface));
        REQUIRE(objectInterface.unsupportedObjects.size() == 1);
        const DRW_UnsupportedObject& rawObject =
            objectInterface.unsupportedObjects.front();
        CHECK_FALSE(rawObject.m_isEntity);
        CHECK(rawObject.m_recordName == name);

        DwgEntityReaderProbe deferredReader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(objectFrame.data()), objectFrame.size()));
        deferredReader.setVersionForTest(DRW::AC1021);
        objHandle deferredObject(shellType, objectHandle, 0);
        dwgBuffer deferredBuffer(
            const_cast<std::uint8_t*>(objectFrame.data()), objectFrame.size());
        DwgReadProbe deferredInterface;
        REQUIRE(deferredReader.readDwgEntity(&deferredBuffer, deferredObject,
                                              deferredInterface));
        CHECK(deferredInterface.unsupportedObjects.empty());
        REQUIRE(deferredReader.objObjectMap.size() == 1);
        CHECK(deferredReader.objObjectMap.begin()->second.type == shellType);
    }

    dwgBufferW incompleteObjectBody;
    putDictionaryObjectPreamble(incompleteObjectBody, DRW::AC1021, 1128,
                                0x2D9u);
    incompleteObjectBody.putBit(0);
    dwgBufferW noObjectHandles;
    const auto incompleteObjectFrame = makeR2007EntityFrame(
        incompleteObjectBody, noObjectHandles);
    REQUIRE(!incompleteObjectFrame.empty());
    DwgEntityReaderProbe incompleteObjectReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(incompleteObjectFrame.data()),
        incompleteObjectFrame.size()));
    incompleteObjectReader.setVersionForTest(DRW::AC1021);
    objHandle incompleteObject(1128, 0x2D9u, 0);
    dwgBuffer incompleteObjectBuffer(
        const_cast<std::uint8_t*>(incompleteObjectFrame.data()),
        incompleteObjectFrame.size());
    DwgReadProbe incompleteObjectInterface;
    CHECK_FALSE(incompleteObjectReader.readDwgObject(
        &incompleteObjectBuffer, incompleteObject, incompleteObjectInterface));
    CHECK(incompleteObjectInterface.unsupportedObjects.empty());

    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1021);
    REQUIRE(writer.replayRawObject(rawEntity));
}

TEST_CASE("DWG custom OBJECTS shells require validated common handle data",
          "[dwg][safety][writer]") {
    const std::array<std::pair<std::uint16_t, const char*>, 4> shellClasses = {{
        {560, "ACDBLINERES"},
        {561, "ACDBCIRCARCRES"},
        {562, "TABLETEMPLATE"},
        {563, "ACDBCENTERLINEACTIONBODY"},
    }};
    std::uint32_t objectHandle = 0x2DC;
    for (const auto& [classNumber, recordName] : shellClasses) {
        dwgBufferW body;
        putDictionaryObjectPreamble(body, DRW::AC1021, classNumber,
                                    objectHandle);
        body.putBit(0);
        dwgBufferW handleStream;
        handleStream.putHandle(makeObjectHandle(0));
        handleStream.putHandle(makeObjectHandle(0));
        const auto frame = makeR2007EntityFrame(body, handleStream);
        REQUIRE(!frame.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(DRW::AC1021);
        reader.addCustomObjectClass(classNumber, recordName);
        objHandle object(classNumber, objectHandle, 0);
        dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
        DwgReadProbe interface;
        REQUIRE(reader.readDwgObject(&buffer, object, interface));
        REQUIRE(interface.unsupportedObjects.size() == 1);
        const DRW_UnsupportedObject& raw = interface.unsupportedObjects.front();
        CHECK_FALSE(raw.m_isEntity);
        CHECK(raw.m_recordName == recordName);

        dwgBufferW noHandles;
        const auto incompleteFrame = makeR2007EntityFrame(body, noHandles);
        REQUIRE(!incompleteFrame.empty());
        DwgEntityReaderProbe incompleteReader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(incompleteFrame.data()),
            incompleteFrame.size()));
        incompleteReader.setVersionForTest(DRW::AC1021);
        incompleteReader.addCustomObjectClass(classNumber, recordName);
        objHandle incompleteObject(classNumber, objectHandle, 0);
        dwgBuffer incompleteBuffer(
            const_cast<std::uint8_t*>(incompleteFrame.data()),
            incompleteFrame.size());
        DwgReadProbe incompleteInterface;
        CHECK_FALSE(incompleteReader.readDwgObject(
            &incompleteBuffer, incompleteObject, incompleteInterface));
        CHECK(incompleteInterface.unsupportedObjects.empty());

        ++objectHandle;
    }
}

TEST_CASE("DWG generic R2007 custom OBJECTS raw fallback validates its tail",
          "[dwg][safety]") {
    constexpr std::uint16_t classNumber = 564;
    constexpr std::uint32_t objectHandle = 0x2E0;

    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1021, classNumber, objectHandle);
    body.putBit(0);
    dwgBufferW handleStream;
    handleStream.putHandle(makeObjectHandle(0));
    handleStream.putHandle(makeObjectHandle(0));
    const auto frame = makeR2007EntityFrame(body, handleStream);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1021);
    reader.addCustomObjectClass(classNumber, "UNRECOGNIZED_OBJECT");
    objHandle object(classNumber, objectHandle, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgObject(&buffer, object, interface));
    REQUIRE(interface.unsupportedObjects.size() == 1);
    const DRW_UnsupportedObject& raw = interface.unsupportedObjects.front();
    CHECK(raw.m_recordName == "UNRECOGNIZED_OBJECT");
    CHECK(raw.m_hasClassDefinition);
    CHECK(raw.m_classAppName == "TEST_APP");
    CHECK(raw.m_classProxyFlag == 0x1234);
    CHECK(raw.m_classWasProxy);
    CHECK(raw.m_classEntityFlagRaw == 0x1F3);
    CHECK(raw.m_classDwgVersion == 1027);
    CHECK(raw.m_classMaintenanceVersion == 329);
    CHECK(raw.m_classUnknown1 == 17);
    CHECK(raw.m_classUnknown2 == 23);
    REQUIRE(reader.remainingMap.size() == 1);

    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1021);
    REQUIRE(writer.replayRawObject(raw));
    CHECK_FALSE(writer.replayRawObject(raw));

    dwgBufferW noHandles;
    const auto incompleteFrame = makeR2007EntityFrame(body, noHandles);
    REQUIRE(!incompleteFrame.empty());
    DwgEntityReaderProbe incompleteReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(incompleteFrame.data()),
        incompleteFrame.size()));
    incompleteReader.setVersionForTest(DRW::AC1021);
    incompleteReader.addCustomObjectClass(classNumber, "UNRECOGNIZED_OBJECT");
    objHandle incompleteObject(classNumber, objectHandle, 0);
    dwgBuffer incompleteBuffer(
        const_cast<std::uint8_t*>(incompleteFrame.data()),
        incompleteFrame.size());
    DwgReadProbe incompleteInterface;
    CHECK_FALSE(incompleteReader.readDwgObject(
        &incompleteBuffer, incompleteObject, incompleteInterface));
    CHECK(incompleteInterface.unsupportedObjects.empty());
    CHECK(incompleteReader.remainingMap.empty());
}

TEST_CASE("DWG typed frame rejects duplicate object handles before publication",
          "[dwg][safety][writer]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriterProbe writer(&stream, &header);
    writer.setVersionForTest(DRW::AC1018);

    writer.beginObject(0x2E1u).putObjType(DRW::AC1018, dwgType::LINE);
    writer.finishObject();
    REQUIRE(writer.objectMapSizeForTest() == 1);
    CHECK_FALSE(writer.frameWriteFailedForTest());
    const auto firstOffset = writer.lastObjectOffsetForTest();

    writer.beginObject(0x2E1u);
    writer.finishObject();
    CHECK(writer.objectMapSizeForTest() == 1);
    CHECK(writer.lastObjectOffsetForTest() == firstOffset);
    CHECK(writer.frameWriteFailedForTest());
}

TEST_CASE("DWG HANDLES rejects offsets before the object base",
          "[dwg][safety][writer]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriter24Probe writer(&stream, &header);
    writer.addObjectMapEntryForTest(0x2E2u, 99u);

    CHECK_FALSE(writer.writeDwgHandles());
    CHECK(writer.bufferSizeForTest() == 0);
}

TEST_CASE("DWG HANDLES rejects a missing modern object base",
          "[dwg][safety][writer]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriter24Probe writer(&stream, &header);
    writer.addObjectMapEntryWithoutSectionsForTest(0x2E2u, 99u);

    CHECK_FALSE(writer.writeDwgHandles());
    CHECK(writer.bufferSizeForTest() == 0);
}

TEST_CASE("DWG modern typed frame rejects duplicate object handles",
          "[dwg][safety][writer]") {
    DRW_Header header;
    std::ofstream stream;
    DwgClassPhaseWriter24Probe writer(&stream, &header);

    writer.beginObject(0x2E3u);
    writer.finishObject();
    REQUIRE(writer.objectMapSizeForTest() == 1);
    CHECK_FALSE(writer.objectWriteFailedForTest());
    const auto firstOffset = writer.lastObjectOffsetForTest();

    writer.beginObject(0x2E3u);
    writer.finishObject();
    CHECK(writer.objectMapSizeForTest() == 1);
    CHECK(writer.lastObjectOffsetForTest() == firstOffset);
    CHECK(writer.objectWriteFailedForTest());
}

TEST_CASE("DWG OBJECTS pass rejects entity class records",
          "[dwg][safety][ownership]") {
    constexpr std::uint16_t classNumber = 567;
    constexpr std::uint32_t entityHandle = 0x2E6;

    dwgBufferW body;
    putDictionaryObjectPreamble(body, DRW::AC1021, classNumber, entityHandle);
    body.putBit(0);
    dwgBufferW noHandles;
    const auto frame = makeR2007EntityFrame(body, noHandles);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1021);
    reader.addCustomEntityClass(classNumber, "MISROUTED_ENTITY");
    objHandle object(classNumber, entityHandle, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    bool frameFailure = false;

    CHECK_FALSE(reader.readDwgObject(
        &buffer, object, interface, &frameFailure));
    CHECK(frameFailure);
    CHECK(interface.unsupportedObjects.empty());
}

TEST_CASE("DWG generic R2013 custom ENTITY preserves its data-storage bit",
          "[dwg][safety]") {
    constexpr std::uint16_t classNumber = 565;
    constexpr std::uint32_t entityHandle = 0x2E1;

    DwgLineWriterProbe entityShell;
    entityShell.handle = entityHandle;
    entityShell.setHasDataStorageBinaryData(true);
    entityShell.setObjectType(static_cast<std::int16_t>(classNumber));
    dwgBufferW body;
    dwgBufferW handleStream;
    REQUIRE(entityShell.encodeDwgCommon(DRW::AC1027, &body));
    REQUIRE(entityShell.encodeDwgEntHandle(DRW::AC1027, &body,
                                           &handleStream));
    const auto frame = makeR2013EntityFrame(body, handleStream);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1027);
    reader.addCustomEntityClass(classNumber, "UNRECOGNIZED_ENTITY");
    objHandle entity(classNumber, entityHandle, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntity(&buffer, entity, interface));
    REQUIRE(interface.unsupportedObjects.size() == 1);
    CHECK(interface.unsupportedObjects.front().m_recordName
          == "UNRECOGNIZED_ENTITY");
    CHECK(interface.unsupportedObjects.front().m_hasDataStorage);

    dwgBufferW noHandles;
    const auto incompleteFrame = makeR2013EntityFrame(body, noHandles);
    REQUIRE(!incompleteFrame.empty());
    DwgEntityReaderProbe incompleteReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(incompleteFrame.data()),
        incompleteFrame.size()));
    incompleteReader.setVersionForTest(DRW::AC1027);
    incompleteReader.addCustomEntityClass(classNumber, "UNRECOGNIZED_ENTITY");
    objHandle incompleteEntity(classNumber, entityHandle, 0);
    dwgBuffer incompleteBuffer(
        const_cast<std::uint8_t*>(incompleteFrame.data()),
        incompleteFrame.size());
    DwgReadProbe incompleteInterface;
    CHECK_FALSE(incompleteReader.readDwgEntity(
        &incompleteBuffer, incompleteEntity, incompleteInterface));
    CHECK(incompleteInterface.unsupportedObjects.empty());
}

TEST_CASE("DWG opaque AC1018 entity identity matches its object-map key",
          "[dwg][safety][ownership]") {
    constexpr std::uint16_t classNumber = 566;
    constexpr std::uint32_t entityHandle = 0x2E5;

    DwgLineWriterProbe entityShell;
    entityShell.handle = entityHandle;
    entityShell.setObjectType(static_cast<std::int16_t>(classNumber));
    dwgBufferW body;
    REQUIRE(entityShell.encodeDwgCommon(DRW::AC1018, &body));
    const auto frame = makeEntityFrame(body);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe validReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    validReader.setVersionForTest(DRW::AC1018);
    validReader.addCustomEntityClass(classNumber, "AC1018_ENTITY");
    objHandle validEntity(classNumber, entityHandle, 0);
    dwgBuffer validBuffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe validInterface;
    REQUIRE(validReader.readDwgEntity(
        &validBuffer, validEntity, validInterface));
    REQUIRE(validInterface.unsupportedObjects.size() == 1);
    CHECK(validInterface.unsupportedObjects.front().m_handle == entityHandle);

    DwgEntityReaderProbe mismatchReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    mismatchReader.setVersionForTest(DRW::AC1018);
    mismatchReader.addCustomEntityClass(classNumber, "AC1018_ENTITY");
    objHandle mismatchEntity(classNumber, entityHandle + 1, 0);
    dwgBuffer mismatchBuffer(
        const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe mismatchInterface;
    CHECK_FALSE(mismatchReader.readDwgEntity(
        &mismatchBuffer, mismatchEntity, mismatchInterface));
    CHECK(mismatchReader.parsedHandleMismatchForTest());
    CHECK(mismatchInterface.unsupportedObjects.empty());
}

TEST_CASE("DWG fixed type class aliases retain typed entity dispatch",
          "[dwg][safety]") {
    constexpr std::uint16_t classNumber = 550;
    const auto frame = makeGeoPositionMarkerFrame(classNumber);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1027);
    reader.addGeoPositionMarkerClass(classNumber);

    objHandle entity(DRW_GeoPositionMarker::kDwgType, 0x89, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntity(&buffer, entity, interface));
    CHECK(interface.geoPositionMarkerCount == 1);
    REQUIRE(interface.unsupportedObjects.size() == 1);
    CHECK(interface.unsupportedObjects.front().m_recordName
          == "GEOPOSITIONMARKER");
    CHECK(reader.objObjectMap.empty());
}

TEST_CASE("DWG typed entity handle must match its object-map key",
          "[dwg][safety]") {
    constexpr std::uint32_t encodedHandle = 0x2C4;
    constexpr std::uint32_t mappedHandle = 0x2C5;

    DwgLineWriterProbe line;
    line.handle = encodedHandle;
    line.setObjectType(dwgType::LINE);
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    dwgBufferW body;
    REQUIRE(line.encodeDwgCommon(DRW::AC1018, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1018, &body));
    DwgLineReaderProbe parsedLine;
    dwgBuffer bodyBuffer(body.data().data(), body.data().size());
    REQUIRE(parsedLine.parseDwg(
        DRW::AC1018, &bodyBuffer, body.bitCount()));
    CHECK(parsedLine.handle == encodedHandle);
    auto frame = makeEntityFrame(body);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        frame.data(), frame.size()));
    reader.setVersionForTest(DRW::AC1018);
    objHandle entity(dwgType::LINE, mappedHandle, 0);
    dwgBuffer buffer(frame.data(), frame.size());
    DwgReadProbe interface;
    CHECK_FALSE(reader.readDwgEntity(&buffer, entity, interface));
    CHECK(reader.parsedHandleMismatchForTest());
    if (!interface.unsupportedObjects.empty())
        CHECK(interface.unsupportedObjects.front().m_objectType == dwgType::LINE);
    CHECK(interface.unsupportedObjects.empty());

    DwgEntityReaderProbe sectionReader(std::make_unique<dwgBuffer>(
        frame.data(), frame.size()));
    sectionReader.setVersionForTest(DRW::AC1018);
    sectionReader.ObjectMap.emplace(
        mappedHandle, objHandle(dwgType::LINE, mappedHandle, 0));
    dwgBuffer sectionBuffer(frame.data(), frame.size());
    DwgReadProbe sectionInterface;
    CHECK_FALSE(sectionReader.readDwgEntities(
        sectionInterface, &sectionBuffer));
    CHECK(sectionReader.parsedHandleMismatchForTest());
    CHECK(sectionInterface.unsupportedObjects.empty());
}

TEST_CASE("DWG block owner mismatch does not publish a raw entity",
          "[dwg][safety]") {
    constexpr std::uint32_t entityHandle = 0x2C6;
    constexpr std::uint32_t expectedOwner = 0x500;

    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.parentHandle = expectedOwner + 1;
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    line.setObjectType(dwgType::LINE);
    dwgBufferW body;
    REQUIRE(line.encodeDwgCommon(DRW::AC1018, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1018, &body));
    const auto frame = makeEntityFrame(body);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setExpectedBlockOwnerForTest(expectedOwner);
    objHandle entity(dwgType::LINE, entityHandle, 0);
    dwgBuffer buffer(const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    CHECK_FALSE(reader.readDwgEntity(&buffer, entity, interface));
    CHECK(interface.unsupportedObjects.empty());

    DwgEntityReaderProbe sweepReader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    sweepReader.setVersionForTest(DRW::AC1018);
    sweepReader.ObjectMap.emplace(
        entityHandle, objHandle(dwgType::LINE, entityHandle, 0));
    dwgBuffer sweepBuffer(
        const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe sweepInterface;
    CHECK_FALSE(sweepReader.readDwgEntities(sweepInterface, &sweepBuffer));
    CHECK(sweepInterface.unsupportedObjects.empty());
}

TEST_CASE("DWG dictionaries bound counts and honor legacy WDFLT fields",
          "[dwg][safety]") {
    std::vector<std::uint8_t> malformed = makeMalformedDictionaryBody();
    REQUIRE(!malformed.empty());

    DwgDictionaryReaderProbe dictionary;
    dictionary.m_entries.push_back({"stale", 0x123u});
    dwgBuffer malformedBuffer(malformed.data(), malformed.size());
    CHECK_FALSE(dictionary.parseDwg(DRW::AC1015, &malformedBuffer));
    CHECK(dictionary.m_entries.empty());

    std::vector<std::uint8_t> impossible = makeImpossibleDictionaryCountBody();
    REQUIRE(!impossible.empty());
    DwgDictionaryReaderProbe impossibleDictionary;
    dwgBuffer impossibleBuffer(impossible.data(), impossible.size());
    CHECK_FALSE(impossibleDictionary.parseDwg(
        DRW::AC1015, &impossibleBuffer, /*bs=*/0));
    CHECK(impossibleDictionary.m_entries.empty());

    std::vector<std::uint8_t> capped = makeCappedDictionaryCountBody();
    REQUIRE(!capped.empty());
    DwgDictionaryReaderProbe cappedDictionary;
    dwgBuffer cappedBuffer(capped.data(), capped.size());
    CHECK_FALSE(cappedDictionary.parseDwg(
        DRW::AC1015, &cappedBuffer, /*bs=*/0));
    CHECK_FALSE(cappedBuffer.isGood());
    CHECK(cappedDictionary.m_entries.empty());
    CHECK_FALSE(cappedDictionary.countCap.has_value());

    std::vector<std::uint8_t> truncated = makeTruncatedDictionaryBody();
    REQUIRE(!truncated.empty());
    DwgDictionaryReaderProbe truncatedDictionary;
    dwgBuffer truncatedBuffer(truncated.data(), truncated.size());
    CHECK_FALSE(truncatedDictionary.parseDwg(
        DRW::AC1015, &truncatedBuffer, /*bs=*/0));
    CHECK(truncatedDictionary.m_entries.empty());

    std::vector<std::uint8_t> outOfBounds = makeOutOfBoundsDictionaryNameBody();
    REQUIRE(!outOfBounds.empty());
    DwgDictionaryReaderProbe boundedDictionary;
    dwgBuffer outOfBoundsBuffer(outOfBounds.data(), outOfBounds.size());
    CHECK_FALSE(boundedDictionary.parseDwg(
        DRW::AC1015, &outOfBoundsBuffer, /*bs=*/0));
    CHECK(boundedDictionary.m_entries.empty());

    std::vector<std::uint8_t> outOfBoundsVar = makeOutOfBoundsDictionaryVarBody();
    REQUIRE(!outOfBoundsVar.empty());
    DwgDictionaryVarReaderProbe boundedVar;
    boundedVar.m_schema = 7;
    boundedVar.m_value = "stale";
    dwgBuffer outOfBoundsVarBuffer(outOfBoundsVar.data(), outOfBoundsVar.size());
    const bool parsedOutOfBoundsVar = boundedVar.parseDwg(
        DRW::AC1015, &outOfBoundsVarBuffer, /*bs=*/0);
    CHECK_FALSE(parsedOutOfBoundsVar);
    CHECK(boundedVar.handle == 0u);
    CHECK(boundedVar.m_schema == 0);
    CHECK(boundedVar.m_value.empty());

    std::vector<std::uint8_t> legacy = makeR14DictionaryWithDefaultBody();
    REQUIRE(!legacy.empty());
    DwgDictionaryWithDefaultReaderProbe legacyDictionary;
    dwgBuffer legacyBuffer(legacy.data(), legacy.size());
    REQUIRE(legacyDictionary.parseDwg(
        DRW::AC1014, &legacyBuffer, /*bs=*/0));
    CHECK(legacyDictionary.handle == 0x860u);
    CHECK(legacyDictionary.parentHandle == 0x0Cu);
    CHECK(legacyDictionary.cloning == 1);
    CHECK(legacyDictionary.hardOwner == 1);
    CHECK(legacyDictionary.m_entries.empty());
    CHECK(legacyDictionary.m_defaultEntryHandle == 0x1202u);

    // A reused carrier must not retain a prior default handle when the next
    // object is truncated in its trailing default-handle field.
    REQUIRE(legacy.size() > 2u);
    legacy.resize(legacy.size() - 2u);
    DwgDictionaryWithDefaultReaderProbe reusedDictionary;
    reusedDictionary.m_defaultEntryHandle = 0xBEEFu;
    reusedDictionary.m_entries.push_back({"stale", 0xBEEFu});
    dwgBuffer reusedBuffer(legacy.data(), legacy.size());
    CHECK_FALSE(reusedDictionary.parseDwg(
        DRW::AC1014, &reusedBuffer, /*bs=*/0));
    CHECK(reusedDictionary.handle == 0u);
    CHECK(reusedDictionary.m_entries.empty());
    CHECK(reusedDictionary.m_defaultEntryHandle == 0u);
    CHECK(reusedDictionary.cloning == 0);
    CHECK(reusedDictionary.hardOwner == 0);
}

TEST_CASE("DWG table controls decode BS entry counts and reset handles",
          "[dwg][safety][control]") {
    std::vector<std::uint8_t> bytes = makeLargeBitShortControlBody();
    REQUIRE(!bytes.empty());

    DwgObjectControlReaderProbe control;
    control.handlesList.push_back(0xDEADu);
    dwgBuffer buffer(bytes.data(), bytes.size());
    REQUIRE(control.parseDwg(DRW::AC1015, &buffer, /*bs=*/0));
    CHECK(control.handle == 0x500u);
    REQUIRE(control.handlesList.size() == 258u);
    CHECK(control.handlesList.front() == 0x600u);
    CHECK(control.handlesList.back() == 0x701u);

    control.reset();
    CHECK(control.handlesList.empty());
    CHECK(control.handle == 0u);

    std::vector<std::uint8_t> malformed = makeImpossibleBitLongControlBody();
    dwgBuffer malformedBuffer(malformed.data(), malformed.size());
    control.handlesList.push_back(0xDEADu);
    CHECK_FALSE(control.parseDwg(DRW::AC1015, &malformedBuffer, /*bs=*/0));
    CHECK(control.handlesList.empty());
    CHECK_FALSE(malformedBuffer.isGood());
}

TEST_CASE("DWG table controls reject duplicate owned handles",
          "[dwg][safety][control]") {
    std::vector<std::uint8_t> bytes = makeDuplicateBitShortControlBody();
    REQUIRE(!bytes.empty());

    DwgObjectControlReaderProbe control;
    dwgBuffer buffer(bytes.data(), bytes.size());
    CHECK_FALSE(control.parseDwg(DRW::AC1015, &buffer, /*bs=*/0));
    CHECK(control.handlesList.empty());
    CHECK_FALSE(buffer.isGood());
}

TEST_CASE("DWG classes reject negative instance counts",
          "[dwg][safety][classes]") {
    std::vector<std::uint8_t> bytes = makeNegativeClassInstanceBody();
    REQUIRE(!bytes.empty());

    DRW_Class klass;
    dwgBuffer buffer(bytes.data(), bytes.size());
    CHECK_FALSE(klass.parseDwg(DRW::AC1018, &buffer, &buffer));
    CHECK_FALSE(buffer.isGood());
}

TEST_CASE("DWG table failures leave no table state or consumed handles",
          "[dwg][safety][table]") {
    std::uint8_t bytes[] = {0};
    DwgTableStateReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes, sizeof(bytes)));
    reader.seedTableStateForTest();

    constexpr std::uint32_t controlHandle = DRW::NoHandle;
    reader.ObjectMap.emplace(
        controlHandle, objHandle(0x38u, controlHandle, 0u));
    DRW_Header header;
    dwgBuffer buffer(bytes, sizeof(bytes));

    CHECK_FALSE(reader.readDwgTables(header, &buffer));
    CHECK(reader.tableStateEmptyForTest());
    REQUIRE(reader.ObjectMap.size() == 1u);
    const auto restored = reader.ObjectMap.find(controlHandle);
    REQUIRE(restored != reader.ObjectMap.end());
    CHECK(restored->second.type == 0x38u);
    CHECK(restored->second.handle == controlHandle);
    CHECK(restored->second.loc == 0u);
}

TEST_CASE("DWG malformed table records roll back the table phase",
          "[dwg][safety][table][fixture]") {
    auto bytes = readFile(std::filesystem::path(LIBRECAD_TEST_DIR) /
                          "ordinary_enc_AC1015.dwg");
    REQUIRE(!bytes.empty());

    DwgTableStateReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    REQUIRE(reader.readFileHeader());

    DRW_Header header;
    REQUIRE(reader.readDwgHeader(header));
    REQUIRE(reader.readDwgClasses());
    REQUIRE(reader.readDwgHandles());

    const auto originalObjectMap = reader.ObjectMap;
    std::uint32_t recordHandle = 0;
    std::uint64_t recordOffset = 0;
    REQUIRE(reader.firstLinetypeRecordForTest(recordHandle, recordOffset));
    REQUIRE(recordOffset < bytes.size());

    // Corrupt the frame prefix without changing the already-decoded handle
    // map. The table phase must fail and restore every consumed handle.
    bytes[static_cast<std::size_t>(recordOffset)] ^= 0x80u;

    dwgBuffer tableBuffer(bytes.data(), bytes.size());
    CHECK_FALSE(reader.readDwgTables(header, &tableBuffer));
    CHECK(reader.tableStateEmptyForTest());
    REQUIRE(reader.ObjectMap.size() == originalObjectMap.size());
    for (const auto& entry : originalObjectMap) {
        const auto restored = reader.ObjectMap.find(entry.first);
        REQUIRE(restored != reader.ObjectMap.end());
        CHECK(restored->second.type == entry.second.type);
        CHECK(restored->second.handle == entry.second.handle);
        CHECK(restored->second.loc == entry.second.loc);
    }
    CHECK(reader.ObjectMap.find(recordHandle) != reader.ObjectMap.end());
    const auto source = std::find_if(
        reader.m_dwgSourceFrameLedger.cbegin(),
        reader.m_dwgSourceFrameLedger.cend(),
        [recordHandle](const DRW_DwgFrameCoverageEntry& entry) {
            return entry.m_handle == recordHandle;
        });
    REQUIRE(source != reader.m_dwgSourceFrameLedger.cend());
    CHECK(source->m_disposition == DRW_DwgFrameDisposition::Pending);
    CHECK(source->m_reason == DRW_DwgFrameCoverageReason::None);
}

TEST_CASE("DWG missing table records roll back the table phase",
          "[dwg][safety][table][fixture]") {
    auto bytes = readFile(std::filesystem::path(LIBRECAD_TEST_DIR) /
                          "ordinary_enc_AC1015.dwg");
    REQUIRE(!bytes.empty());

    DwgTableStateReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    REQUIRE(reader.readFileHeader());

    DRW_Header header;
    REQUIRE(reader.readDwgHeader(header));
    REQUIRE(reader.readDwgClasses());
    REQUIRE(reader.readDwgHandles());

    std::uint32_t recordHandle = 0;
    std::uint64_t unusedOffset = 0;
    REQUIRE(reader.firstLinetypeRecordForTest(recordHandle, unusedOffset));
    reader.removeObjectForTest(recordHandle);
    const auto objectMapAfterRemoval = reader.ObjectMap;

    dwgBuffer tableBuffer(bytes.data(), bytes.size());
    CHECK_FALSE(reader.readDwgTables(header, &tableBuffer));
    CHECK(reader.tableStateEmptyForTest());
    REQUIRE(reader.ObjectMap.size() == objectMapAfterRemoval.size());
    for (const auto& entry : objectMapAfterRemoval) {
        const auto restored = reader.ObjectMap.find(entry.first);
        REQUIRE(restored != reader.ObjectMap.end());
        CHECK(restored->second.type == entry.second.type);
        CHECK(restored->second.handle == entry.second.handle);
        CHECK(restored->second.loc == entry.second.loc);
    }
    CHECK(reader.ObjectMap.find(recordHandle) == reader.ObjectMap.end());
}

TEST_CASE("DWG TABLES rejects a mismatched control type before record parsing",
          "[dwg][safety][table][fixture]") {
    auto bytes = readFile(std::filesystem::path(LIBRECAD_TEST_DIR) /
                          "ordinary_enc_AC1015.dwg");
    REQUIRE(!bytes.empty());

    DwgTableStateReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    reader.setVersionForTest(DRW::AC1015);
    REQUIRE(reader.readFileHeader());

    DRW_Header header;
    REQUIRE(reader.readDwgHeader(header));
    REQUIRE(reader.readDwgClasses());
    REQUIRE(reader.readDwgHandles());
    const auto originalObjectMap = reader.ObjectMap;
    REQUIRE(reader.replaceLinetypeControlTypeForTest(
        bytes, DRW::DwgLayerControlObjectType));

    dwgBuffer tableBuffer(bytes.data(), bytes.size());
    CHECK_FALSE(reader.readDwgTables(header, &tableBuffer));
    CHECK(reader.tableStateEmptyForTest());
    REQUIRE(reader.ObjectMap.size() == originalObjectMap.size());
    for (const auto& entry : originalObjectMap) {
        const auto restored = reader.ObjectMap.find(entry.first);
        REQUIRE(restored != reader.ObjectMap.end());
        CHECK(restored->second.type == entry.second.type);
        CHECK(restored->second.handle == entry.second.handle);
        CHECK(restored->second.loc == entry.second.loc);
    }
    CHECK(std::all_of(
        reader.m_dwgSourceFrameLedger.cbegin(),
        reader.m_dwgSourceFrameLedger.cend(),
        [](const DRW_DwgFrameCoverageEntry& entry) {
            return entry.m_disposition == DRW_DwgFrameDisposition::Pending
                && entry.m_reason == DRW_DwgFrameCoverageReason::None
                && entry.m_publicationCount == 0;
        }));
}

TEST_CASE("DWG R2007 AC18 page rejects header size larger than stored page",
          "[dwg][safety]") {
    auto page = makeAc18DataPage(0, 0, {0x11, 0x22}, 1, 35);

    dwgSectionInfo section;
    section.size = 2;
    section.maxSize = 2;
    section.compressed = 1;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, page.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = 2;
    pageInfo.cSize = 2;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(section.size, 0);
    CHECK_FALSE(reader.parseDataPage(section, output.data()));
}

TEST_CASE("DWG R2007 AC18 page rejects a mismatched header section number",
          "[dwg][safety]") {
    auto page = makeAc18DataPage(0, 0, {0x11, 0x22}, 2);

    dwgSectionInfo section;
    section.Id = 1;
    section.size = 2;
    section.maxSize = 2;
    section.compressed = 1;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, page.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = 2;
    pageInfo.cSize = 2;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(section.size, 0xA5);
    CHECK_FALSE(reader.parseDataPage(section, output.data()));
    CHECK(std::all_of(output.cbegin(), output.cend(),
                      [](std::uint8_t value) { return value == 0xA5; }));
}

TEST_CASE("DWG R2007 AC18 store page rejects a logical size beyond its page capacity",
          "[dwg][safety]") {
    auto page = makeAc18DataPage(0, 0, {0x11, 0x22}, 1, 33);

    dwgSectionInfo section;
    section.Id = 1;
    section.size = 2;
    section.maxSize = 2;
    section.compressed = 1;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, page.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = 2;
    pageInfo.cSize = 2;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(section.size, 0xA5);
    CHECK_FALSE(reader.parseDataPage(section, output.data()));
    CHECK(std::all_of(output.cbegin(), output.cend(),
                      [](std::uint8_t value) { return value == 0xA5; }));
}

TEST_CASE("DWG R2004 page rejects header size larger than stored page",
          "[dwg][safety]") {
    auto page = makeAc18DataPage(0, 0, {0x11, 0x22}, 1, 35);

    dwgSectionInfo section;
    section.size = 2;
    section.maxSize = 2;
    section.compressed = 1;
    section.pageCount = 1;
    section.Id = 1;
    dwgPageInfo pageInfo(1, 0, page.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = 2;
    pageInfo.cSize = 2;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReader18Probe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1018);
    const std::vector<std::uint8_t> previousState{0xA5, 0x5A};
    reader.setDecodedStateForTest(previousState);
    CHECK_FALSE(reader.parseDataPage(section));
    CHECK(reader.decodedStateForTest() == previousState);
}

TEST_CASE("DWG R2004 page rejects unsupported compression mode",
          "[dwg][safety]") {
    auto page = makeAc18DataPage(0, 0, {0x11, 0x22});

    dwgSectionInfo section;
    section.Id = 1;
    section.size = 2;
    section.maxSize = 2;
    section.compressed = 3;
    section.pageCount = 1;
    section.pages.emplace(1, dwgPageInfo(1, 0, page.size()));

    DwgDataPageReader18Probe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1018);
    CHECK_FALSE(reader.parseDataPage(section));
}

TEST_CASE("DWG R2004 page validates declared compressed size",
          "[dwg][safety]") {
    auto page = makeAc18DataPage(0, 0, {0x11, 0x22});

    dwgSectionInfo section;
    section.Id = 1;
    section.size = 2;
    section.maxSize = 2;
    section.compressed = 1;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, page.size());
    pageInfo.dataSize = 1;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReader18Probe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1018);
    CHECK_FALSE(reader.parseDataPage(section));
}

TEST_CASE("DWG R2004 page checksum failure is diagnostic",
          "[dwg][safety]") {
    auto page = makeAc18DataPage(0, 0, {0x11, 0x22});
    REQUIRE(page.size() > 32);
    page.back() ^= 0x01;

    dwgSectionInfo section;
    section.Id = 1;
    section.size = 2;
    section.maxSize = 2;
    section.compressed = 1;
    section.pageCount = 1;
    section.pages.emplace(1, dwgPageInfo(1, 0, page.size()));

    DwgDataPageReader18Probe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1018);
    const std::vector<std::uint8_t> previousState{0xA5, 0x5A};
    reader.setDecodedStateForTest(previousState);
    CHECK_FALSE(reader.parseDataPage(section));
    CHECK(reader.decodedStateForTest() == previousState);
    REQUIRE_FALSE(reader.m_integrityDiagnostics.empty());
    CHECK(std::all_of(
        reader.m_integrityDiagnostics.cbegin(),
        reader.m_integrityDiagnostics.cend(),
        [](const DwgIntegrityDiagnostic& diagnostic) {
            return diagnostic.kind == DwgIntegrityCheckKind::DataPageChecksum
                && diagnostic.phase == DwgIntegrityPhase::DataPage
                && diagnostic.severity == DwgIntegritySeverity::Error
                && diagnostic.offsetSpace
                    == DwgIntegrityAddressSpace::PhysicalFile
                && diagnostic.hasPageId && diagnostic.pageId == 1
                && diagnostic.hasFileOffset && diagnostic.fileOffset == 0
                && diagnostic.hasExpected && diagnostic.hasObserved;
        }));
}

TEST_CASE("DWG R2004 compressed page may exceed physical page size",
          "[dwg][safety]") {
    std::vector<std::uint8_t> compressed{0x01, 0xAB, 0xCD, 0xEF, 0x01};
    constexpr std::size_t copyCount = 100;
    for (std::size_t i = 0; i < copyCount; ++i) {
        compressed.push_back(0x41);
        compressed.push_back(0x00);
        compressed.push_back(0x11);
    }
    constexpr std::size_t uncompressedSize = 4 + copyCount * 4;
    auto page = makeAc18DataPage(0, 0, compressed, 1, uncompressedSize);

    dwgSectionInfo section;
    section.Id = 1;
    section.size = uncompressedSize;
    section.maxSize = uncompressedSize;
    section.compressed = 2;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, page.size());
    pageInfo.dataSize = compressed.size();
    pageInfo.startOffset = 0;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReader18Probe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1018);
    REQUIRE(reader.parseDataPage(section));
    CHECK(reader.decodedStateForTest().size() == uncompressedSize);
}

TEST_CASE("DWG R2004 page rejects a mismatched header section number",
          "[dwg][safety]") {
    auto page = makeAc18DataPage(0, 0, {0x11, 0x22}, 2, 34);

    dwgSectionInfo section;
    section.Id = 1;
    section.size = 2;
    section.maxSize = 2;
    section.compressed = 1;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, page.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = 2;
    pageInfo.cSize = 2;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReader18Probe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1018);
    CHECK_FALSE(reader.parseDataPage(section));
}

TEST_CASE("DWG R2007 raw page rejects a logical size beyond its stored bytes",
          "[dwg][safety]") {
    std::vector<std::uint8_t> page{0x11, 0x22, 0x33};

    dwgSectionInfo section;
    section.size = 4;
    section.maxSize = 4;
    section.compressed = 4;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, page.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = 4;
    pageInfo.cSize = 3;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(page.data(), page.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(section.size, 0xA5);
    CHECK_FALSE(reader.parseDataPage(section, output.data()));
    CHECK(std::all_of(output.cbegin(), output.cend(),
                      [](std::uint8_t value) { return value == 0xA5; }));
}

TEST_CASE("DWG R2007 page rejects inconsistent physical size declarations",
          "[dwg][safety]") {
    std::vector<std::uint8_t> rsPayload(251);
    for (std::size_t i = 0; i < rsPayload.size(); ++i)
        rsPayload[i] = static_cast<std::uint8_t>(0x17u + (i * 19u) % 0xC0u);
    const std::vector<std::uint8_t> payload(rsPayload.cbegin(),
                                            rsPayload.cbegin() + 248);

    std::vector<std::uint8_t> encoded(255, 0);
    REQUIRE(dwgRSCodec::encode251I(rsPayload.data(), encoded.data(), 1));

    dwgSectionInfo section;
    section.size = payload.size();
    section.maxSize = payload.size();
    section.compressed = 4;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, encoded.size());
    pageInfo.dataSize = payload.size() - 1;
    pageInfo.startOffset = 0;
    pageInfo.uSize = payload.size();
    pageInfo.cSize = payload.size();
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(encoded.data(), encoded.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(section.size, 0xA5);
    CHECK_FALSE(reader.parseDataPage(section, output.data()));
    CHECK(std::all_of(output.cbegin(), output.cend(),
                      [](std::uint8_t value) { return value == 0xA5; }));
}

TEST_CASE("DWG R2007 page size stays within the allocation limit",
          "[dwg][safety]") {
    std::vector<std::uint8_t> rsPayload(251);
    for (std::size_t i = 0; i < rsPayload.size(); ++i)
        rsPayload[i] = static_cast<std::uint8_t>(0x19u + (i * 23u) % 0xB0u);
    const std::vector<std::uint8_t> payload(rsPayload.cbegin(),
                                            rsPayload.cbegin() + 248);

    std::vector<std::uint8_t> encoded(255, 0);
    REQUIRE(dwgRSCodec::encode251I(rsPayload.data(), encoded.data(), 1));

    dwgSectionInfo section;
    section.size = payload.size();
    section.maxSize = payload.size();
    section.compressed = 4;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, encoded.size());
    pageInfo.dataSize = dwgSafety::MaxBufferSize + 1;
    pageInfo.startOffset = 0;
    pageInfo.uSize = payload.size();
    pageInfo.cSize = payload.size();
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(encoded.data(), encoded.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(section.size, 0xA5);
    CHECK_FALSE(reader.parseDataPage(section, output.data()));
    CHECK(std::all_of(output.cbegin(), output.cend(),
                      [](std::uint8_t value) { return value == 0xA5; }));
}

TEST_CASE("DWG R2007 stored RS page is deinterleaved before copying",
          "[dwg][safety]") {
    // One RS(255,251) block carries at most 248 logical bytes after the
    // required 8-byte alignment used to derive the block count.
    std::vector<std::uint8_t> rsPayload(251);
    for (std::size_t i = 0; i < rsPayload.size(); ++i)
        rsPayload[i] = static_cast<std::uint8_t>(0x20u + (i * 13u) % 0xB0u);
    const std::vector<std::uint8_t> payload(rsPayload.cbegin(),
                                            rsPayload.cbegin() + 248);

    std::vector<std::uint8_t> encoded(255, 0);
    REQUIRE(dwgRSCodec::encode251I(rsPayload.data(), encoded.data(), 1));

    dwgSectionInfo section;
    section.size = payload.size();
    section.maxSize = payload.size();
    section.compressed = 4;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, encoded.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = payload.size();
    pageInfo.cSize = payload.size();
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(encoded.data(), encoded.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(payload.size(), 0);
    REQUIRE(reader.parseDataPage(section, output.data()));
    CHECK(output == payload);
}

TEST_CASE("DWG AC1027 RS fallback honors the R2004 compression flag",
          "[dwg][safety]") {
    constexpr std::size_t blockCount = 3;
    constexpr std::size_t logicalBlockSize = 251;
    constexpr std::size_t logicalSize = blockCount * logicalBlockSize - 1;
    constexpr std::size_t encodedSize = blockCount * 255;

    std::vector<std::uint8_t> input(logicalSize, 0);
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<std::uint8_t>(i * 23u + 9u);

    std::vector<std::uint8_t> rsInput(blockCount * logicalBlockSize, 0);
    std::copy(input.cbegin(), input.cend(), rsInput.begin());
    std::vector<std::uint8_t> encoded(encodedSize, 0);
    REQUIRE(dwgRSCodec::encode251I(rsInput.data(), encoded.data(), blockCount));

    dwgSectionInfo section;
    section.Id = 1;
    section.size = input.size();
    section.maxSize = input.size();
    section.compressed = 2;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, encoded.size());
    pageInfo.dataSize = input.size();
    pageInfo.startOffset = 0;
    pageInfo.uSize = input.size();
    pageInfo.cSize = input.size();
    section.pages.emplace(1, pageInfo);

    DwgDataPageReader18Probe reader(
        std::make_unique<dwgBuffer>(encoded.data(), encoded.size()));
    reader.setVersionForTest(DRW::AC1027);
    REQUIRE(reader.parseDataPage(section));
    CHECK(reader.decodedStateForTest() == input);

    section.compressed = 4; // R2007-only encoding selector is invalid here.
    CHECK_FALSE(reader.parseDataPage(section));
    CHECK(reader.decodedStateForTest() == input);
}

TEST_CASE("DWG R2007 sparse sections keep implicit zero pages",
          "[dwg][safety]") {
    std::vector<std::uint8_t> rsPayload(251);
    for (std::size_t i = 0; i < rsPayload.size(); ++i)
        rsPayload[i] = static_cast<std::uint8_t>(0x45u + (i * 17u) % 0x90u);
    const std::vector<std::uint8_t> payload(rsPayload.cbegin(),
                                            rsPayload.cbegin() + 248);

    std::vector<std::uint8_t> encoded(255, 0);
    REQUIRE(dwgRSCodec::encode251I(rsPayload.data(), encoded.data(), 1));

    dwgSectionInfo section;
    section.size = payload.size() * 2;
    section.maxSize = payload.size();
    section.compressed = 4;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, encoded.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = payload.size();
    pageInfo.cSize = payload.size();
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(encoded.data(), encoded.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(section.size, 0xA5);
    REQUIRE(reader.parseDataPage(section, output.data()));
    CHECK(std::equal(payload.cbegin(), payload.cend(), output.cbegin()));
    CHECK(std::all_of(output.cbegin() + static_cast<std::ptrdiff_t>(payload.size()),
                      output.cend(), [](std::uint8_t value) { return value == 0; }));
}

TEST_CASE("DWG R2007 sparse sections keep interior zero pages",
          "[dwg][r2007][safety]") {
    constexpr std::size_t pageDataSize = 248;
    constexpr std::size_t sectionSize = pageDataSize * 3;
    std::vector<std::uint8_t> firstPayload(pageDataSize);
    std::vector<std::uint8_t> secondPayload(pageDataSize);
    for (std::size_t i = 0; i < pageDataSize; ++i) {
        firstPayload[i] = static_cast<std::uint8_t>(0x10u + i % 0x70u);
        secondPayload[i] = static_cast<std::uint8_t>(0x90u + i % 0x60u);
    }

    const auto makeStoredPage = [](const std::vector<std::uint8_t>& payload) {
        std::vector<std::uint8_t> rsInput(251, 0);
        std::copy(payload.cbegin(), payload.cend(), rsInput.begin());
        std::vector<std::uint8_t> encoded(255, 0);
        if (!dwgRSCodec::encode251I(rsInput.data(), encoded.data(), 1))
            return std::vector<std::uint8_t>{};
        return encoded;
    };
    const auto firstPage = makeStoredPage(firstPayload);
    const auto secondPage = makeStoredPage(secondPayload);
    REQUIRE(!firstPage.empty());
    REQUIRE(!secondPage.empty());
    std::vector<std::uint8_t> physical = firstPage;
    const auto secondAddress = physical.size();
    physical.insert(physical.end(), secondPage.cbegin(), secondPage.cend());

    dwgSectionInfo section;
    section.Id = 1;
    section.size = sectionSize;
    section.maxSize = pageDataSize;
    section.compressed = 4;
    section.pageCount = 2;
    dwgPageInfo first(1, 0, firstPage.size());
    first.dataSize = pageDataSize;
    first.startOffset = 0;
    first.uSize = pageDataSize;
    first.cSize = pageDataSize;
    section.pages.emplace(first.Id, first);
    dwgPageInfo second(2, secondAddress, secondPage.size());
    second.dataSize = pageDataSize;
    second.startOffset = pageDataSize * 2;
    second.uSize = pageDataSize;
    second.cSize = pageDataSize;
    section.pages.emplace(second.Id, second);

    DwgDataPageReaderProbe reader(std::make_unique<dwgBuffer>(
        physical.data(), physical.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(sectionSize, 0xA5);
    REQUIRE(reader.parseDataPage(section, output.data()));
    CHECK(std::equal(firstPayload.cbegin(), firstPayload.cend(),
                     output.cbegin()));
    CHECK(std::all_of(
        output.cbegin() + static_cast<std::ptrdiff_t>(pageDataSize),
        output.cbegin() + static_cast<std::ptrdiff_t>(pageDataSize * 2),
        [](std::uint8_t value) { return value == 0; }));
    CHECK(std::equal(secondPayload.cbegin(), secondPayload.cend(),
                     output.cbegin() + static_cast<std::ptrdiff_t>(
                         pageDataSize * 2)));
}

TEST_CASE("DWG R2007 page cannot extend past the logical section",
          "[dwg][r2007][safety]") {
    std::vector<std::uint8_t> encoded(255, 0);
    dwgSectionInfo section;
    section.Id = 1;
    section.size = 249;
    section.maxSize = 248;
    section.compressed = 4;
    section.pageCount = 1;
    dwgPageInfo page(1, 0, encoded.size());
    page.dataSize = 248;
    page.startOffset = 248;
    page.uSize = 248;
    page.cSize = 248;
    section.pages.emplace(page.Id, page);

    DwgDataPageReaderProbe reader(std::make_unique<dwgBuffer>(
        encoded.data(), encoded.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(section.size, 0xA5);
    CHECK_FALSE(reader.parseDataPage(section, output.data()));
    CHECK(std::all_of(output.cbegin(), output.cend(),
                      [](std::uint8_t value) { return value == 0xA5; }));
}

TEST_CASE("DWG R2004 page cannot extend past the logical section",
          "[dwg][r2004][safety]") {
    const auto encoded = makeAc18DataPage(0, 2, {0x11, 0x22});
    dwgSectionInfo section;
    section.Id = 1;
    section.size = 3;
    section.maxSize = 2;
    section.compressed = 1;
    section.pageCount = 1;
    dwgPageInfo page(1, 0, encoded.size());
    page.startOffset = 2;
    section.pages.emplace(page.Id, page);

    DwgDataPageReader18Probe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(encoded.data()), encoded.size()));
    reader.setVersionForTest(DRW::AC1018);
    const std::vector<std::uint8_t> previousState{0xA5, 0x5A, 0xC3};
    reader.setDecodedStateForTest(previousState);
    CHECK_FALSE(reader.parseDataPage(section));
    CHECK(reader.decodedStateForTest() == previousState);
}

TEST_CASE("DWG section capacity accounts for omitted zero pages",
          "[dwg][safety]") {
    std::uint64_t capacity = 0;
    REQUIRE(dwgSafety::sectionBufferCapacity(248 * 2, 1, 248, capacity));
    CHECK(capacity == 248 * 2);

    REQUIRE(dwgSafety::sectionBufferCapacity(249, 1, 248, capacity));
    CHECK(capacity == 248 * 2);

    CHECK_FALSE(dwgSafety::sectionBufferCapacity(1, 1, 0, capacity));
}

TEST_CASE("DWG R2007 data-page CRC mismatch is diagnostic",
          "[dwg][safety]") {
    std::vector<std::uint8_t> rsPayload(251);
    for (std::size_t i = 0; i < rsPayload.size(); ++i)
        rsPayload[i] = static_cast<std::uint8_t>(0x30u + (i * 11u) % 0xA0u);
    const std::vector<std::uint8_t> payload(rsPayload.cbegin(),
                                            rsPayload.cbegin() + 248);

    std::vector<std::uint8_t> encoded(255, 0);
    REQUIRE(dwgRSCodec::encode251I(rsPayload.data(), encoded.data(), 1));

    dwgSectionInfo section;
    section.size = payload.size();
    section.maxSize = payload.size();
    section.compressed = 4;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, encoded.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = payload.size();
    pageInfo.cSize = payload.size();
    pageInfo.crc = dwgUtil::crc64Mirrored(
        dwgUtil::updateSeed1(0, payload.size()), payload.data(),
        payload.size()) ^ 1;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(encoded.data(), encoded.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(payload.size(), 0);
    REQUIRE(reader.parseDataPage(section, output.data()));
    CHECK(output == payload);
    CHECK(reader.crcMismatchCount() == 1);
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    const auto& diagnostic = reader.m_integrityDiagnostics.front();
    CHECK(diagnostic.kind == DwgIntegrityCheckKind::DataPageCrc);
    CHECK(diagnostic.phase == DwgIntegrityPhase::DataPage);
    CHECK(diagnostic.offsetSpace == DwgIntegrityAddressSpace::PhysicalFile);
    CHECK(diagnostic.hasPageId);
    CHECK(diagnostic.pageId == 1);
    CHECK(diagnostic.hasFileOffset);
    CHECK(diagnostic.fileOffset == 0);
    CHECK(diagnostic.logicalSectionId == secEnum::UNKNOWNS);
    CHECK(diagnostic.sectionDescriptorId == -1);
    CHECK(diagnostic.hasExpected);
    CHECK(diagnostic.hasObserved);
}

TEST_CASE("DWG R2007 data-page checksum mismatch is diagnostic",
          "[dwg][safety]") {
    std::vector<std::uint8_t> rsPayload(251);
    for (std::size_t i = 0; i < rsPayload.size(); ++i)
        rsPayload[i] = static_cast<std::uint8_t>(0x40u + (i * 7u) % 0x90u);
    const std::vector<std::uint8_t> payload(rsPayload.cbegin(),
                                            rsPayload.cbegin() + 248);

    std::vector<std::uint8_t> encoded(255, 0);
    REQUIRE(dwgRSCodec::encode251I(rsPayload.data(), encoded.data(), 1));

    dwgSectionInfo section;
    section.size = payload.size();
    section.maxSize = payload.size();
    section.compressed = 4;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, encoded.size());
    pageInfo.startOffset = 0;
    pageInfo.uSize = payload.size();
    pageInfo.cSize = payload.size();
    pageInfo.checksum = dwgUtil::checksum21(0, payload.data(),
                                             payload.size()) ^ 1;
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(encoded.data(), encoded.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(payload.size(), 0);
    REQUIRE(reader.parseDataPage(section, output.data()));
    CHECK(output == payload);
    CHECK(reader.crcMismatchCount() == 1);
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    const auto& diagnostic = reader.m_integrityDiagnostics.front();
    CHECK(diagnostic.kind == DwgIntegrityCheckKind::DataPageChecksum);
    CHECK(diagnostic.phase == DwgIntegrityPhase::DataPage);
    CHECK(diagnostic.severity == DwgIntegritySeverity::Warning);
    CHECK(diagnostic.offsetSpace == DwgIntegrityAddressSpace::PhysicalFile);
    CHECK(diagnostic.hasPageId);
    CHECK(diagnostic.pageId == 1);
    CHECK(diagnostic.hasFileOffset);
    CHECK(diagnostic.fileOffset == 0);
    CHECK(diagnostic.hasExpected);
    CHECK(diagnostic.hasObserved);
}

TEST_CASE("DWG R2007 page-map header bounds are enforced before allocation",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("visualstyle_r2007.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("visualstyle_r2007.dwg fixture absent; skipping");
    }
    const auto original = readFile(source);
    REQUIRE(!original.empty());

    DwgReadProbe pristineInterface;
    dwgRW pristine(source.string().c_str());
    REQUIRE(pristine.read(&pristineInterface, true));

    const std::array<std::uint64_t, 2> invalidPageLimits{
        0, dwgSafety::MaxPageCount + 1};
    for (std::size_t index = 0; index < invalidPageLimits.size(); ++index) {
        auto corrupted = original;
        REQUIRE(reencodeR2007FileHeader(corrupted, invalidPageLimits[index]));

        const auto temporary = std::filesystem::temp_directory_path() /
            ("librecad-r2007-page-map-limit-" + std::to_string(index) + ".dwg");
        {
            std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
            REQUIRE(stream.good());
            stream.write(reinterpret_cast<const char*>(corrupted.data()),
                         static_cast<std::streamsize>(corrupted.size()));
            REQUIRE(stream.good());
        }

        DwgReadProbe interface;
        dwgRW reader(temporary.string().c_str());
        CHECK_FALSE(reader.read(&interface, true));

        std::error_code error;
        std::filesystem::remove(temporary, error);
    }
}

TEST_CASE("DWG R2007 page-map count must match the decoded map",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("visualstyle_r2007.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("visualstyle_r2007.dwg fixture absent; skipping");
    }
    const auto original = readFile(source);
    REQUIRE(!original.empty());

    std::array<std::uint8_t, r2007HeaderPreambleSize> preamble{};
    std::array<std::uint8_t, r2007FileHeaderDataSize> header{};
    REQUIRE(decodeR2007FileHeader(original, preamble, header));
    const auto pageAmount = readLittleEndian64(header.data() + 96);
    REQUIRE(pageAmount > 0);

    const std::array<std::uint64_t, 2> invalidAmounts{0, pageAmount + 1};
    for (const std::uint64_t invalidAmount : invalidAmounts) {
        auto corrupted = original;
        REQUIRE(reencodeR2007PagesAmount(corrupted, invalidAmount));

        DwgReadProbe interface;
        dwgRW reader(source.string().c_str());
        CHECK_FALSE(reader.readBuffer(corrupted.data(), corrupted.size(),
                                      &interface, true));
        CHECK(interface.inserts.empty());
        CHECK(interface.polylineCount == 0);
    }
}

TEST_CASE("DWG R2007 section-map count must match decoded descriptors",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("visualstyle_r2007.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("visualstyle_r2007.dwg fixture absent; skipping");
    }
    const auto original = readFile(source);
    REQUIRE(!original.empty());

    std::array<std::uint8_t, r2007HeaderPreambleSize> preamble{};
    std::array<std::uint8_t, r2007FileHeaderDataSize> header{};
    REQUIRE(decodeR2007FileHeader(original, preamble, header));
    constexpr std::size_t sectionsAmountOffset = 160;
    const auto sectionsAmount =
        readLittleEndian64(header.data() + sectionsAmountOffset);
    REQUIRE(sectionsAmount > 1);

    const std::array<std::uint64_t, 3> invalidAmounts{
        0, sectionsAmount - 1, sectionsAmount + 1};
    for (const std::uint64_t invalidAmount : invalidAmounts) {
        auto corrupted = original;
        REQUIRE(reencodeR2007SectionsAmount(corrupted, invalidAmount));

        std::array<std::uint8_t, r2007HeaderPreambleSize> mutatedPreamble{};
        std::array<std::uint8_t, r2007FileHeaderDataSize> mutatedHeader{};
        REQUIRE(decodeR2007FileHeader(corrupted, mutatedPreamble, mutatedHeader));
        CHECK(readLittleEndian64(mutatedHeader.data() + sectionsAmountOffset)
              == invalidAmount);

        DwgReadProbe interface;
        dwgRW reader(source.string().c_str());
        CHECK_FALSE(reader.readBuffer(corrupted.data(), corrupted.size(),
                                      &interface, true));
        CHECK(interface.inserts.empty());
        CHECK(interface.polylineCount == 0);
    }
}

TEST_CASE("DWG R2007 page-map rejects an out-of-range page ID",
          "[dwg][safety][fixture]") {
    const auto source = localFixture("visualstyle_r2007.dwg");
    if (!std::filesystem::is_regular_file(source)) {
        SKIP("visualstyle_r2007.dwg fixture absent; skipping");
    }
    const auto original = readFile(source);
    REQUIRE(!original.empty());

    auto corrupted = original;
    REQUIRE(appendR2007OutOfRangePageId(corrupted));
    const auto temporary = std::filesystem::temp_directory_path() /
                           "librecad-r2007-page-map-id.dwg";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        REQUIRE(stream.good());
        stream.write(reinterpret_cast<const char*>(corrupted.data()),
                     static_cast<std::streamsize>(corrupted.size()));
        REQUIRE(stream.good());
    }

    DwgReadProbe interface;
    dwgRW reader(temporary.string().c_str());
    CHECK_FALSE(reader.read(&interface, true));
    const auto diagnostics = reader.getIntegrityDiagnostics();
    REQUIRE_FALSE(diagnostics.empty());
    CHECK(std::any_of(diagnostics.cbegin(), diagnostics.cend(),
                      [](const DwgIntegrityDiagnostic& diagnostic) {
                          return diagnostic.kind
                                     == DwgIntegrityCheckKind::PageMapReference
                              && diagnostic.phase == DwgIntegrityPhase::PageMap
                              && diagnostic.severity == DwgIntegritySeverity::Error;
                      }));

    std::error_code error;
    std::filesystem::remove(temporary, error);
}

TEST_CASE("DWG R2007 writer output rejects a generated page-map ID mutation",
          "[dwg][safety][write]") {
    const auto source = std::filesystem::temp_directory_path() /
                        "librecad-r2007-generated-page-map.dwg";
    const auto corrupted = std::filesystem::temp_directory_path() /
                           "librecad-r2007-generated-page-map-corrupt.dwg";
    std::error_code error;
    std::filesystem::remove(source, error);
    std::filesystem::remove(corrupted, error);

    DwgReadProbe writeInterface;
    {
        dwgRW writer(source.string().c_str());
        REQUIRE(writer.write(&writeInterface, DRW::AC1021, false));
    }

    auto bytes = readFile(source);
    REQUIRE(!bytes.empty());
    REQUIRE(appendR2007OutOfRangePageId(bytes));
    {
        std::ofstream stream(corrupted, std::ios::binary | std::ios::trunc);
        REQUIRE(stream.good());
        stream.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        REQUIRE(stream.good());
    }

    DwgReadProbe readInterface;
    dwgRW reader(corrupted.string().c_str());
    CHECK_FALSE(reader.read(&readInterface, true));
    CHECK(readInterface.inserts.empty());
    CHECK(readInterface.polylineCount == 0);

    std::filesystem::remove(source, error);
    std::filesystem::remove(corrupted, error);
}

TEST_CASE("DWG R2007 generated page-map CRC mismatch is advisory",
          "[dwg][safety][write]") {
    const auto source = std::filesystem::temp_directory_path() /
                        "librecad-r2007-generated-page-map-crc.dwg";
    const auto corrupted = std::filesystem::temp_directory_path() /
                           "librecad-r2007-generated-page-map-crc-corrupt.dwg";
    std::error_code error;
    std::filesystem::remove(source, error);
    std::filesystem::remove(corrupted, error);

    DwgReadProbe writeInterface;
    {
        dwgRW writer(source.string().c_str());
        REQUIRE(writer.write(&writeInterface, DRW::AC1021, false));
    }

    DwgReadProbe pristineInterface;
    {
        dwgRW pristine(source.string().c_str());
        REQUIRE(pristine.read(&pristineInterface, true));
        CHECK(pristine.getR2007CrcMismatch() == 0u);
    }

    auto bytes = readFile(source);
    REQUIRE(!bytes.empty());
    REQUIRE(reencodeR2007PageMapCompressedCrc(bytes));
    {
        std::ofstream stream(corrupted, std::ios::binary | std::ios::trunc);
        REQUIRE(stream.good());
        stream.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        REQUIRE(stream.good());
    }

    DwgReadProbe readInterface;
    dwgRW reader(corrupted.string().c_str());
    REQUIRE(reader.read(&readInterface, true));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getR2007CrcMismatch() == 1u);
    CHECK(readInterface.inserts.empty());
    CHECK(readInterface.polylineCount == 0);

    std::filesystem::remove(source, error);
    std::filesystem::remove(corrupted, error);
}

TEST_CASE("DWG negative bit seek is transactional", "[dwg][safety]") {
    std::uint8_t bytes[] = {0xA5};
    dwgBuffer buffer(bytes, sizeof(bytes));
    buffer.setBitPos(3);
    const auto oldPosition = buffer.getPosition();
    const auto oldBitPos = buffer.getBitPos();

    CHECK_FALSE(buffer.moveBitPos(-4));
    CHECK(buffer.getPosition() == oldPosition);
    CHECK(buffer.getBitPos() == oldBitPos);

    CHECK(buffer.moveBitPos(5));
    CHECK(buffer.getPosition() == 1);
    CHECK(buffer.getBitPos() == 0);
}

TEST_CASE("DWG R2007 object string footer locates normal stream", "[dwg][safety]") {
    dwgBufferW writer;
    writer.putRawChar8(0x5A);
    for (int i = 0; i < 7; ++i)
        writer.putBit(0);
    writer.putRawShort16(15); // one byte of string data plus seven pad bits
    writer.putBit(1);
    writer.alignToByte();

    auto bytes = writer.data();
    dwgBuffer buffer(bytes.data(), bytes.size());
    REQUIRE(buffer.seekR2007StringStream(bytes.size() * 8));
    CHECK(buffer.getPosition() == 0);
    CHECK(buffer.getBitPos() == 0);
    CHECK(buffer.getRawChar8() == 0x5A);
}

TEST_CASE("DWG R2007 object string footer supports extended size", "[dwg][safety]") {
    constexpr std::uint64_t stringBytes = 4096;
    constexpr std::uint64_t encodedSize = stringBytes * 8 + 7;
    dwgBufferW writer;
    for (std::uint64_t i = 0; i < stringBytes; ++i)
        writer.putRawChar8(static_cast<std::uint8_t>(i == 0 ? 0x5A : 0xA5));
    for (int i = 0; i < 7; ++i)
        writer.putBit(0);
    writer.putRawShort16(static_cast<std::uint16_t>(encodedSize >> 15));
    writer.putRawShort16(static_cast<std::uint16_t>(0x8000U | (encodedSize & 0x7FFFU)));
    writer.putBit(1);
    writer.alignToByte();

    auto bytes = writer.data();
    dwgBuffer buffer(bytes.data(), bytes.size());
    REQUIRE(buffer.seekR2007StringStream(bytes.size() * 8));
    CHECK(buffer.getPosition() == 0);
    CHECK(buffer.getBitPos() == 0);
    CHECK(buffer.getRawChar8() == 0x5A);
    REQUIRE(buffer.setPosition(stringBytes - 1));
    CHECK(buffer.getRawChar8() == 0xA5);
}

TEST_CASE("DWG R2010 writer preserves an absent string footer",
          "[dwg][r2010][safety][write]") {
    dwgBufferW data;
    dwgBufferW strings;
    REQUIRE(DwgStringFooterWriterProbe::appendR2007StringStream(
        data, strings, true));
    CHECK(data.data() == std::vector<std::uint8_t>{0, 0, 0});

    auto bytes = data.data();
    dwgBuffer reader(bytes.data(), bytes.size());
    REQUIRE(reader.seekR2007StringStream(bytes.size() * 8));
    CHECK(reader.getPosition() == bytes.size());
    CHECK(reader.getBitPos() == 0);
}

TEST_CASE("DWG R2007 writer emits extended object string footers",
          "[dwg][r2007][safety][write]") {
    const auto path = std::filesystem::temp_directory_path() /
                      "librecad-r2007-extended-string-footer.dwg";
    std::error_code error;
    std::filesystem::remove(path, error);

    DwgLongTextProbe source;
    {
        dwgRW writer(path.string().c_str());
        source.writer = &writer;
        REQUIRE(writer.write(&source, DRW::AC1021, false));
    }
    REQUIRE(source.writeSucceeded);

    DwgLongTextProbe result;
    {
        dwgRW reader(path.string().c_str());
        REQUIRE(reader.read(&result, true));
    }
    REQUIRE(result.texts.size() == 1);
    CHECK(result.texts.front() == std::string(2048, 'A'));
    const auto controlReceipts = std::count_if(
        result.framePublications.cbegin(), result.framePublications.cend(),
        [](const DRW_DwgFramePublication& publication) {
            return publication.m_carrier
                == DRW_DwgFramePublication::Carrier::Control;
        });
    REQUIRE(controlReceipts > 0);
    CHECK(std::all_of(
        result.framePublications.cbegin(), result.framePublications.cend(),
        [](const DRW_DwgFramePublication& publication) {
            return publication.m_carrier
                    != DRW_DwgFramePublication::Carrier::Control
                || publication.m_commonLinkEvidence
                    == DRW_DwgCommonLinkEvidence::NotApplicable;
        }));

    std::filesystem::remove(path, error);
}

TEST_CASE("DWG R2010 writer emits extended object string footers",
          "[dwg][r2010][safety][write]") {
    const auto path = std::filesystem::temp_directory_path() /
                      "librecad-r2010-extended-string-footer.dwg";
    std::error_code error;
    std::filesystem::remove(path, error);

    DwgLongTextProbe source;
    {
        dwgRW writer(path.string().c_str());
        source.writer = &writer;
        REQUIRE(writer.write(&source, DRW::AC1024, false));
    }
    REQUIRE(source.writeSucceeded);

    DwgLongTextProbe result;
    {
        dwgRW reader(path.string().c_str());
        const bool readOk = reader.read(&result, true);
        for (const auto& diagnostic : reader.getIntegrityDiagnostics()) {
            INFO("diagnostic kind=" << static_cast<int>(diagnostic.kind)
                 << " phase=" << static_cast<int>(diagnostic.phase)
                 << " section=" << diagnostic.sectionName
                 << " page=" << diagnostic.pageId
                 << " offset=" << diagnostic.fileOffset);
        }
        REQUIRE(readOk);
    }
    REQUIRE(result.texts.size() == 1);
    CHECK(result.texts.front() == std::string(2048, 'A'));

    std::filesystem::remove(path, error);
}

TEST_CASE("DWG writers keep independent modern version markers",
          "[dwg][versions][safety][write]") {
    struct VersionCase {
        DRW::Version version;
        const char* marker;
    };
    const VersionCase versions[] = {
        {DRW::AC1018, "AC1018"},
        {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"},
    };

    for (const VersionCase& version : versions) {
        const auto path = std::filesystem::temp_directory_path() /
                          (std::string("librecad-") + version.marker
                           + "-version-marker.dwg");
        std::error_code error;
        std::filesystem::remove(path, error);

        DwgLongTextProbe source;
        {
        dwgRW writer(path.string().c_str());
            source.writer = &writer;
            REQUIRE(writer.write(&source, version.version, false));
        }
        REQUIRE(source.writeSucceeded);

        const auto bytes = readFile(path);
        REQUIRE(bytes.size() >= 6);
        CHECK(std::equal(version.marker, version.marker + 6, bytes.cbegin()));

        DwgLongTextProbe result;
        {
        dwgRW reader(path.string().c_str());
            REQUIRE(reader.read(&result, true));
        }
        REQUIRE(result.texts.size() == 1);
        CHECK(result.texts.front() == std::string(2048, 'A'));
        std::filesystem::remove(path, error);
    }
}

TEST_CASE("DWG R2007 object string footer rejects oversized data transactionally",
          "[dwg][safety]") {
    dwgBufferW writer;
    writer.putRawChar8(0x5A);
    for (int i = 0; i < 7; ++i)
        writer.putBit(0);
    writer.putRawShort16(0xFFFF);
    writer.putRawShort16(0x8007);
    writer.putBit(1);
    writer.alignToByte();

    auto bytes = writer.data();
    dwgBuffer buffer(bytes.data(), bytes.size());
    REQUIRE(buffer.setPosition(0));
    buffer.setBitPos(3);
    const auto oldPosition = buffer.getPosition();
    const auto oldBitPos = buffer.getBitPos();

    CHECK_FALSE(buffer.seekR2007StringStream(bytes.size() * 8));
    CHECK(buffer.getPosition() == oldPosition);
    CHECK(buffer.getBitPos() == oldBitPos);
}

TEST_CASE("DWG class string footer uses checked backward offsets", "[dwg][safety]") {
    std::uint8_t bytes[8] = {0};
    bytes[3] = 4;
    dwgBuffer buffer(bytes, sizeof(bytes));
    std::uint64_t stringStartBit = 0;
    std::uint64_t stringSize = 0;

    CHECK(readDwgClassStringFooter(buffer, 40, stringStartBit, stringSize));
    CHECK(stringStartBit == 20);
    CHECK(stringSize == 4);
    CHECK(buffer.getPosition() == 2);
    CHECK(buffer.getBitPos() == 4);
}

TEST_CASE("DWG class string footer rejects underflow transactionally", "[dwg][safety]") {
    std::uint8_t bytes[8] = {0};
    bytes[1] = 2;
    bytes[3] = 0x01;
    bytes[4] = 0x80;
    dwgBuffer buffer(bytes, sizeof(bytes));
    REQUIRE(buffer.setPosition(1));
    buffer.setBitPos(2);
    const auto oldPosition = buffer.getPosition();
    const auto oldBitPos = buffer.getBitPos();
    std::uint64_t stringStartBit = 123;
    std::uint64_t stringSize = 456;

    CHECK_FALSE(readDwgClassStringFooter(buffer, 40, stringStartBit, stringSize));
    CHECK(stringStartBit == 123);
    CHECK(stringSize == 456);
    CHECK(buffer.getPosition() == oldPosition);
    CHECK(buffer.getBitPos() == oldBitPos);
}

TEST_CASE("DWG handle map publishes only after all groups validate", "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(true);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG handle map decodes a terminated valid group", "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    REQUIRE(reader.ObjectMap.size() == 1);
    CHECK(reader.ObjectMap.at(1).loc == 0);
}

TEST_CASE("DWG frame ledger rejects a mismatched source identity",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    DwgSourceFrameId source = sourceFrameId(reader.ObjectMap.at(1));
    ++source.offset;

    CHECK_FALSE(reader.markDwgFrameOutcome(
        source, DRW_DwgFrameDisposition::Published));
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0);
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    CHECK(reader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
}

TEST_CASE("DWG source-frame leases retain exact map identity",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    auto sourceIt = reader.ObjectMap.find(1);
    REQUIRE(sourceIt != reader.ObjectMap.end());
    DwgHandleReaderProbe::DwgSourceFrameLease lease;
    REQUIRE(reader.takeDwgSourceFrame(reader.ObjectMap, sourceIt, lease));
    CHECK(reader.ObjectMap.empty());
    CHECK(lease.hasCoverage);
    CHECK(lease.source == sourceFrameId(lease.object));
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);

    DwgHandleReaderProbe forgedReader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    dwgBuffer forgedMapBuffer(bytes.data(), bytes.size());
    REQUIRE(forgedReader.readDwgHandles(
        &forgedMapBuffer, 0, bytes.size()));
    auto forgedIt = forgedReader.ObjectMap.find(1);
    REQUIRE(forgedIt != forgedReader.ObjectMap.end());
    ++forgedIt->second.sourceOrdinal;
    CHECK_FALSE(forgedReader.takeDwgSourceFrame(
        forgedReader.ObjectMap, forgedIt, lease));
    CHECK(forgedReader.ObjectMap.size() == 1u);
    CHECK(forgedReader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    REQUIRE(forgedReader.m_integrityDiagnostics.size() == 1u);
    CHECK(forgedReader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
}

TEST_CASE("DWG frame-map leases restore detached source frames",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DwgHandleReaderProbe::DwgFrameMapLease lease;
    REQUIRE(reader.detachDwgSourceFrame(
        reader.ObjectMap, reader.ObjectMap.find(object.handle), lease));
    CHECK(reader.ObjectMap.empty());
    CHECK(lease.isDetached());
    CHECK(lease.object.handle == object.handle);
    CHECK(lease.source == sourceFrameId(object));
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);

    REQUIRE(reader.restoreDwgSourceFrame(lease));
    CHECK_FALSE(lease.isDetached());
    REQUIRE(reader.ObjectMap.size() == 1u);
    CHECK(sourceFrameId(reader.ObjectMap.at(object.handle))
          == sourceFrameId(object));
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    CHECK(reader.m_integrityDiagnostics.empty());
}

TEST_CASE("DWG frame-map leases defer detached source frames",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DwgHandleReaderProbe::DwgFrameMapLease lease;
    REQUIRE(reader.detachDwgSourceFrame(
        reader.ObjectMap, reader.ObjectMap.find(object.handle), lease));
    REQUIRE(reader.deferDetachedDwgSourceFrame(lease, reader.objObjectMap));

    CHECK_FALSE(lease.isDetached());
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.objObjectMap.size() == 1u);
    CHECK(sourceFrameId(reader.objObjectMap.at(object.handle))
          == sourceFrameId(object));
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Deferred);
    CHECK(reader.m_integrityDiagnostics.empty());
}

TEST_CASE("DWG frame-map leases preserve detached nodes on collisions",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DwgHandleReaderProbe::DwgFrameMapLease lease;
    REQUIRE(reader.detachDwgSourceFrame(
        reader.ObjectMap, reader.ObjectMap.find(object.handle), lease));
    REQUIRE(reader.ObjectMap.emplace(object.handle, object).second);

    CHECK_FALSE(reader.restoreDwgSourceFrame(lease));
    CHECK(lease.isDetached());
    CHECK(reader.ObjectMap.size() == 1u);
    CHECK(reader.objObjectMap.empty());
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    REQUIRE(reader.m_integrityDiagnostics.size() == 1u);
    CHECK(reader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
    reader.ObjectMap.erase(object.handle);
    REQUIRE(reader.restoreDwgSourceFrame(lease));
    CHECK_FALSE(lease.isDetached());
    CHECK(reader.ObjectMap.size() == 1u);

    DwgHandleReaderProbe deferredReader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    dwgBuffer deferredMapBuffer(bytes.data(), bytes.size());
    REQUIRE(deferredReader.readDwgHandles(
        &deferredMapBuffer, 0, bytes.size()));
    const objHandle deferredObject = deferredReader.ObjectMap.at(1);
    DwgHandleReaderProbe::DwgFrameMapLease deferredLease;
    REQUIRE(deferredReader.detachDwgSourceFrame(
        deferredReader.ObjectMap,
        deferredReader.ObjectMap.find(deferredObject.handle), deferredLease));
    REQUIRE(deferredReader.objObjectMap.emplace(
        deferredObject.handle, deferredObject).second);

    CHECK_FALSE(deferredReader.deferDetachedDwgSourceFrame(
        deferredLease, deferredReader.objObjectMap));
    CHECK(deferredLease.isDetached());
    CHECK(deferredReader.ObjectMap.empty());
    CHECK(deferredReader.objObjectMap.size() == 1u);
    CHECK(deferredReader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    REQUIRE(deferredReader.m_integrityDiagnostics.size() == 1u);
    CHECK(deferredReader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
    deferredReader.objObjectMap.erase(deferredObject.handle);
    REQUIRE(deferredReader.restoreDwgSourceFrame(deferredLease));
    CHECK_FALSE(deferredLease.isDetached());
    CHECK(deferredReader.ObjectMap.size() == 1u);
}

TEST_CASE("DWG frame-map leases require a terminal disposition to discard",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DwgHandleReaderProbe::DwgFrameMapLease lease;
    REQUIRE(reader.detachDwgSourceFrame(
        reader.ObjectMap, reader.ObjectMap.find(object.handle), lease));
    CHECK_FALSE(reader.discardDetachedDwgSourceFrame(lease));
    CHECK(lease.isDetached());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);

    REQUIRE(reader.markDwgFrameOutcome(
        lease.source, DRW_DwgFrameDisposition::Failed));
    REQUIRE(reader.discardDetachedDwgSourceFrame(lease));
    CHECK_FALSE(lease.isDetached());
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Failed);

    DwgHandleReaderProbe publishedReader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    dwgBuffer publishedMapBuffer(bytes.data(), bytes.size());
    REQUIRE(publishedReader.readDwgHandles(
        &publishedMapBuffer, 0, bytes.size()));
    REQUIRE(publishedReader.markDwgFrameOutcome(
        sourceFrameId(publishedReader.ObjectMap.at(1)),
        DRW_DwgFrameDisposition::Published));
    DwgHandleReaderProbe::DwgFrameMapLease publishedLease;
    CHECK_FALSE(publishedReader.detachDwgSourceFrame(
        publishedReader.ObjectMap, publishedReader.ObjectMap.find(1),
        publishedLease));
    CHECK(publishedReader.ObjectMap.size() == 1u);
    CHECK_FALSE(publishedLease.isDetached());
    CHECK(publishedReader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Published);
}

TEST_CASE("DWG staged frame leases preserve exact residency",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    DwgHandleReaderProbe::DwgFrameMapLease lease;
    REQUIRE(reader.detachDwgSourceFrame(
        reader.ObjectMap, reader.ObjectMap.find(1), lease));
    REQUIRE(reader.stageDetachedDwgSourceFrame(lease));
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.objObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Staged);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_reason
          == DRW_DwgFrameCoverageReason::CompoundStaged);

    REQUIRE(reader.restoreDwgSourceFrame(lease));
    CHECK_FALSE(lease.isDetached());
    CHECK(reader.ObjectMap.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_reason
          == DRW_DwgFrameCoverageReason::None);

    REQUIRE(reader.detachDwgSourceFrame(
        reader.ObjectMap, reader.ObjectMap.find(1), lease));
    REQUIRE(reader.stageDetachedDwgSourceFrame(lease));
    REQUIRE(reader.markDwgFrameOutcome(
        lease.source, DRW_DwgFrameDisposition::Published));
    REQUIRE(reader.discardDetachedDwgSourceFrame(lease));
    CHECK_FALSE(lease.isDetached());
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Published);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 1u);
}

TEST_CASE("DWG staged frame aggregates retain one exact receipt and lease",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DRW_DwgFramePublication publication;
    publication.m_handle = object.handle;
    publication.m_sourceOffset = object.loc;
    publication.m_sourceMapOrdinal = object.sourceOrdinal;
    publication.m_sourceOffsetSpace = object.sourceOffsetSpace;
    publication.m_hasSourceLocation = true;

    DwgHandleReaderProbe::DwgStagedFrame staged;
    CHECK_FALSE(reader.stageCurrentEntityFrame(staged, publication));
    CHECK_FALSE(staged.publication.has_value());
    CHECK_FALSE(staged.hasDetachedLease());
    REQUIRE(reader.restoreStagedFrame(staged));
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);

    DwgHandleReaderProbe::DwgFrameMapLease lease;
    REQUIRE(reader.detachDwgSourceFrame(
        reader.ObjectMap, reader.ObjectMap.find(object.handle), lease));
    REQUIRE(reader.stageDetachedDwgSourceFrame(lease));
    staged.lease.emplace(std::move(lease));
    REQUIRE(reader.stageCurrentEntityFrame(staged, publication));
    CHECK(staged.hasDetachedLease());
    REQUIRE(reader.abandonStagedFrame(staged));
    CHECK_FALSE(staged.hasDetachedLease());
    CHECK_FALSE(staged.publication.has_value());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Quarantined);
}

TEST_CASE("DWG staged INSERT aggregate owns its map frame and receipt",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DRW_Insert insert;
    insert.handle = object.handle;
    DRW_DwgFramePublication publication;
    publication.m_handle = object.handle;
    publication.m_sourceOffset = object.loc;
    publication.m_sourceMapOrdinal = object.sourceOrdinal;
    publication.m_sourceOffsetSpace = object.sourceOffsetSpace;
    publication.m_hasSourceLocation = true;

    REQUIRE(reader.stagePendingInsertForTest(insert, publication));
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.objObjectMap.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 1u);
    CHECK(reader.validateDeferredCompoundState());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Staged);

    reader.abandonDeferredCompoundState();
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Quarantined);
}

TEST_CASE("DWG staged frame cleanup preserves a callback failure",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DRW_DwgFramePublication publication;
    publication.m_handle = object.handle;
    publication.m_sourceOffset = object.loc;
    publication.m_sourceMapOrdinal = object.sourceOrdinal;
    publication.m_sourceOffsetSpace = object.sourceOffsetSpace;
    publication.m_hasSourceLocation = true;
    DwgHandleReaderProbe::DwgFrameMapLease lease;
    DwgHandleReaderProbe::DwgStagedFrame staged;
    REQUIRE(reader.detachDwgSourceFrame(
        reader.ObjectMap, reader.ObjectMap.find(object.handle), lease));
    REQUIRE(reader.stageDetachedDwgSourceFrame(lease));
    staged.lease.emplace(std::move(lease));
    REQUIRE(reader.stageCurrentEntityFrame(staged, publication));
    REQUIRE(reader.markDwgFrameOutcome(
        publication.m_handle, DRW_DwgFrameDisposition::Failed,
        DRW_DwgFrameCoverageReason::CallbackException));

    REQUIRE(reader.abandonStagedFrame(staged));
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_reason
          == DRW_DwgFrameCoverageReason::CallbackException);
}

TEST_CASE("DWG staged aggregates reject receiptless and duplicate custody",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe receiptlessReader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(receiptlessReader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    DRW_Insert insert;
    insert.handle = 1;
    REQUIRE(receiptlessReader.stageReceiptlessPendingInsertForTest(insert));
    CHECK_FALSE(receiptlessReader.validateDeferredCompoundState());
    CHECK(receiptlessReader.ObjectMap.empty());
    CHECK(receiptlessReader.stagedPendingInsertCountForTest() == 1u);
    receiptlessReader.abandonDeferredCompoundState();
    CHECK(receiptlessReader.stagedPendingInsertCountForTest() == 0u);

    DwgEntityReaderProbe parallelReader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    mapBuffer.resetPosition();
    REQUIRE(parallelReader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = parallelReader.ObjectMap.at(1);
    DRW_DwgFramePublication publication;
    publication.m_handle = object.handle;
    publication.m_sourceOffset = object.loc;
    publication.m_sourceMapOrdinal = object.sourceOrdinal;
    publication.m_sourceOffsetSpace = object.sourceOffsetSpace;
    publication.m_hasSourceLocation = true;
    REQUIRE(parallelReader.stagePendingInsertForTest(insert, publication));
    CHECK_FALSE(parallelReader.stagePendingInsertForTest(insert, publication));
    CHECK(parallelReader.validateDeferredCompoundState());
    CHECK(parallelReader.stagedPendingInsertCountForTest() == 1u);
    parallelReader.abandonDeferredCompoundState();
    CHECK(parallelReader.stagedPendingInsertCountForTest() == 0u);
}

TEST_CASE("DWG terminalizer marker failures preserve staged frame custody",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0, 1, 1, 1, 1});
    appendHandleMapPage(bytes, {});

    const auto stagePendingInsertAndSeqEnd = [&bytes, &dummy](
        DwgEntityReaderProbe& reader) {
        dwgBuffer mapBuffer(bytes.data(), bytes.size());
        if (!reader.readDwgHandles(&mapBuffer, 0, bytes.size()))
            return false;
        reader.setVersionForTest(DRW::AC1018);
        DRW_Insert insert;
        insert.handle = 1u;
        dwgHandle attributeHandle;
        attributeHandle.ref = 2u;
        insert.attribHandles.push_back(attributeHandle);
        insert.seqendH.ref = 3u;
        DwgInsertReceiptProbe interface;
        bool committed = false;
        return reader.stageInsertForTest(
                   std::move(insert), reader.publicationForTest(1u),
                   interface, committed)
            && !committed
            && reader.stageSeqEndForTest(
                3u, 1u, reader.publicationForTest(3u), interface, committed)
            && !committed;
    };

    SECTION("parent SEQEND marker") {
        DwgEntityReaderProbe reader(
            std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
        REQUIRE(stagePendingInsertAndSeqEnd(reader));
        reader.failBeforeSeqEndTerminalizerMarkerForTest();
        CHECK_FALSE(reader.abandonStagedCompoundStateForTest());
        CHECK(reader.stagedPendingInsertCountForTest() == 1u);
        CHECK(reader.stagedSeqEndCountForTest() == 1u);
        CHECK(reader.m_invalidInsertOwners.empty());
        CHECK(reader.m_invalidSeqEndHandles.empty());
        CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
              == DRW_DwgFrameDisposition::Staged);
        CHECK(reader.m_dwgSourceFrameLedger[2].m_disposition
              == DRW_DwgFrameDisposition::Staged);

        CHECK(reader.abandonStagedCompoundStateForTest());
        CHECK(reader.stagedPendingInsertCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        CHECK(reader.m_invalidInsertOwners.count(1u) == 1u);
        CHECK(reader.m_invalidSeqEndHandles.count(3u) == 1u);
    }

    SECTION("parent owner marker rolls back the SEQEND marker") {
        DwgEntityReaderProbe reader(
            std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
        REQUIRE(stagePendingInsertAndSeqEnd(reader));
        reader.failBeforeInsertOwnerTerminalizerMarkerForTest();
        CHECK_FALSE(reader.abandonStagedCompoundStateForTest());
        CHECK(reader.stagedPendingInsertCountForTest() == 1u);
        CHECK(reader.stagedSeqEndCountForTest() == 1u);
        CHECK(reader.m_invalidInsertOwners.empty());
        CHECK(reader.m_invalidSeqEndHandles.empty());
        CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
              == DRW_DwgFrameDisposition::Staged);
        CHECK(reader.m_dwgSourceFrameLedger[2].m_disposition
              == DRW_DwgFrameDisposition::Staged);

        CHECK(reader.abandonStagedCompoundStateForTest());
        CHECK(reader.stagedPendingInsertCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        CHECK(reader.m_invalidInsertOwners.count(1u) == 1u);
        CHECK(reader.m_invalidSeqEndHandles.count(3u) == 1u);
    }

    SECTION("orphan ATTRIB owner marker") {
        DwgEntityReaderProbe reader(
            std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
        dwgBuffer mapBuffer(bytes.data(), bytes.size());
        REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
        reader.setVersionForTest(DRW::AC1018);
        auto attribute = std::make_shared<DRW_Attrib>();
        attribute->handle = 1u;
        attribute->parentHandle = 0x800u;
        DwgInsertReceiptProbe interface;
        bool committed = false;
        REQUIRE(reader.stageAttributeForTest(
            std::move(attribute), reader.publicationForTest(1u), interface,
            committed));
        CHECK_FALSE(committed);

        reader.failBeforeOrphanOwnerTerminalizerMarkerForTest();
        CHECK_FALSE(reader.abandonStagedCompoundStateForTest());
        CHECK(reader.stagedOrphanAttribCountForTest() == 1u);
        CHECK(reader.m_invalidInsertOwners.empty());
        CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
              == DRW_DwgFrameDisposition::Staged);

        CHECK(reader.abandonStagedCompoundStateForTest());
        CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
        CHECK(reader.m_invalidInsertOwners.count(0x800u) == 1u);
        CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
              == DRW_DwgFrameDisposition::Quarantined);
    }

    SECTION("EOF SEQEND marker") {
        DwgEntityReaderProbe reader(
            std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
        dwgBuffer mapBuffer(bytes.data(), bytes.size());
        REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
        reader.setVersionForTest(DRW::AC1018);
        DwgInsertReceiptProbe interface;
        bool committed = false;
        REQUIRE(reader.stageSeqEndForTest(
            3u, 0x900u, reader.publicationForTest(3u), interface,
            committed));
        CHECK_FALSE(committed);

        reader.failBeforeSeqEndTerminalizerMarkerForTest();
        CHECK_FALSE(reader.abandonStagedCompoundStateForTest());
        CHECK(reader.stagedSeqEndCountForTest() == 1u);
        CHECK(reader.m_invalidSeqEndHandles.empty());
        CHECK(reader.m_dwgSourceFrameLedger[2].m_disposition
              == DRW_DwgFrameDisposition::Staged);

        CHECK(reader.abandonStagedCompoundStateForTest());
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        CHECK(reader.m_invalidSeqEndHandles.count(3u) == 1u);
        CHECK(reader.m_dwgSourceFrameLedger[2].m_disposition
              == DRW_DwgFrameDisposition::Quarantined);
    }
}

TEST_CASE("DWG pending INSERT receipts are scoped by BLOCK_RECORD",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    constexpr std::uint32_t namedRecordHandle = 0x711;
    constexpr std::uint32_t namedInsertHandle = 0x712;
    constexpr std::uint32_t otherRecordHandle = 0x713;

    DwgEntityReaderProbe namedReader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    namedReader.setVersionForTest(DRW::AC1018);
    namedReader.ObjectMap.emplace(
        namedInsertHandle,
        objHandle(dwgType::INSERT, namedInsertHandle, 0));
    DRW_Insert namedInsert;
    namedInsert.handle = namedInsertHandle;
    namedInsert.parentHandle = namedRecordHandle;
    REQUIRE(namedReader.stageReceiptlessPendingInsertForTest(namedInsert));

    DRW_Block_Record namedBlock;
    DwgBlockOwnershipTestAccess::setHandle(namedBlock, namedRecordHandle);
    namedBlock.name = "NAMED_BLOCK";
    DRW_Block_Record otherBlock;
    DwgBlockOwnershipTestAccess::setHandle(otherBlock, otherRecordHandle);
    otherBlock.name = "OTHER_BLOCK";
    DRW_Block_Record modelBlock;
    DwgBlockOwnershipTestAccess::setHandle(modelBlock, 0x714);
    modelBlock.name = "*MODEL_SPACE";
    CHECK(namedReader.hasPendingCompoundStateForBlock(namedBlock));
    CHECK_FALSE(namedReader.hasPendingCompoundStateForBlock(otherBlock));
    CHECK_FALSE(namedReader.hasPendingCompoundStateForBlock(modelBlock));

    constexpr std::uint32_t modelInsertHandle = 0x715;
    DwgEntityReaderProbe modelReader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    modelReader.setVersionForTest(DRW::AC1018);
    modelReader.ObjectMap.emplace(
        modelInsertHandle,
        objHandle(dwgType::INSERT, modelInsertHandle, 0));
    DRW_Insert modelInsert;
    modelInsert.handle = modelInsertHandle;
    REQUIRE(modelReader.stageReceiptlessPendingInsertForTest(modelInsert));
    CHECK(modelReader.hasPendingCompoundStateForBlock(modelBlock));
    CHECK_FALSE(modelReader.hasPendingCompoundStateForBlock(namedBlock));
    CHECK_FALSE(modelReader.hasPendingCompoundStateForBlock(otherBlock));
}

TEST_CASE("DWG block delimiters ignore a pending INSERT in another block",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t recordHandle = 0x721;
    constexpr std::uint32_t blockHandle = 0x722;
    constexpr std::uint32_t endBlockHandle = 0x723;
    constexpr std::uint32_t pendingInsertHandle = 0x724;
    constexpr std::uint32_t otherRecordHandle = 0x725;
    const auto blockFrame = makeBlockFrame(blockHandle, recordHandle, false);
    const auto endBlockFrame = makeBlockFrame(
        endBlockHandle, recordHandle, true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> bytes = blockFrame;
    const auto endBlockOffset = static_cast<std::uint32_t>(bytes.size());
    bytes.insert(bytes.end(), endBlockFrame.cbegin(), endBlockFrame.cend());
    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        bytes.data(), bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle, {});
    reader.blockRecordmap.emplace(recordHandle, record);
    reader.ObjectMap.emplace(
        blockHandle, objHandle(dwgType::BLOCK, blockHandle, 0));
    reader.ObjectMap.emplace(
        endBlockHandle,
        objHandle(dwgType::ENDBLK, endBlockHandle, endBlockOffset));
    reader.ObjectMap.emplace(
        pendingInsertHandle,
        objHandle(dwgType::INSERT, pendingInsertHandle, 0));
    DRW_Insert pendingInsert;
    pendingInsert.handle = pendingInsertHandle;
    pendingInsert.parentHandle = otherRecordHandle;
    REQUIRE(reader.stageReceiptlessPendingInsertForTest(pendingInsert));

    DwgReadProbe interface;
    dwgBuffer buffer(bytes.data(), bytes.size());
    REQUIRE(reader.readDwgBlocks(interface, &buffer));
    CHECK(interface.blockCount == 1u);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 1u);
}

TEST_CASE("DWG mapped child-first INSERT commits covered source frames",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t recordHandle = 0x731;
    constexpr std::uint32_t insertHandle = 0x732;
    constexpr std::uint32_t attribHandle = 0x733;
    constexpr std::uint32_t seqEndHandle = 0x734;
    const auto insertFrame = makeInsertWithChildrenFrame(
        insertHandle, recordHandle, attribHandle, seqEndHandle);
    const auto attribFrame = makeAttribFrame(attribHandle, insertHandle);
    const auto seqEndFrame = makeSeqEndFrame(seqEndHandle, insertHandle);
    REQUIRE(!insertFrame.empty());
    REQUIRE(!attribFrame.empty());
    REQUIRE(!seqEndFrame.empty());

    std::vector<std::uint8_t> objectData = insertFrame;
    const auto attribOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), attribFrame.cbegin(), attribFrame.cend());
    const auto seqEndOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), seqEndFrame.cbegin(), seqEndFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(insertHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(attribHandle - insertHandle));
    REQUIRE(handleEntries.putModularChar(attribOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - attribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(seqEndOffset) - attribOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));

    DRW_Block_Record record;
    DwgBlockOwnershipTestAccess::setHandle(record, recordHandle);
    record.name = "CHILD_FIRST_BLOCK";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {attribHandle, seqEndHandle, insertHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    REQUIRE(reader.walkBlockRecordEntities(
        &record, &objectBuffer, interface, recordHandle, recordHandle));
    REQUIRE(interface.inserts.size() == 1u);
    REQUIRE(interface.inserts.front().attlist.size() == 1u);
    CHECK(interface.inserts.front().handle == insertHandle);
    CHECK(interface.inserts.front().attlist.front()->handle == attribHandle);
    REQUIRE(interface.publications.size() == 3u);
    CHECK(interface.publications[0].m_handle == insertHandle);
    CHECK(interface.publications[1].m_handle == attribHandle);
    CHECK(interface.publications[2].m_handle == seqEndHandle);
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG mapped parent-first INSERT claims declared children",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t recordHandle = 0x735;
    constexpr std::uint32_t insertHandle = 0x736;
    constexpr std::uint32_t attribHandle = 0x737;
    constexpr std::uint32_t seqEndHandle = 0x738;
    const auto insertFrame = makeInsertWithChildrenFrame(
        insertHandle, recordHandle, attribHandle, seqEndHandle);
    const auto attribFrame = makeAttribFrame(attribHandle, insertHandle);
    const auto seqEndFrame = makeSeqEndFrame(seqEndHandle, insertHandle);
    REQUIRE(!insertFrame.empty());
    REQUIRE(!attribFrame.empty());
    REQUIRE(!seqEndFrame.empty());

    std::vector<std::uint8_t> objectData = insertFrame;
    const auto attribOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), attribFrame.cbegin(), attribFrame.cend());
    const auto seqEndOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), seqEndFrame.cbegin(), seqEndFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(insertHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(attribHandle - insertHandle));
    REQUIRE(handleEntries.putModularChar(attribOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - attribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(seqEndOffset) - attribOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));

    DRW_Block_Record record;
    DwgBlockOwnershipTestAccess::setHandle(record, recordHandle);
    record.name = "PARENT_FIRST_BLOCK";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {insertHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    REQUIRE(reader.walkBlockRecordEntities(
        &record, &objectBuffer, interface, recordHandle, recordHandle));
    REQUIRE(interface.inserts.size() == 1u);
    REQUIRE(interface.inserts.front().attlist.size() == 1u);
    CHECK(interface.inserts.front().handle == insertHandle);
    CHECK(interface.inserts.front().attlist.front()->handle == attribHandle);
    REQUIRE(interface.publications.size() == 3u);
    CHECK(interface.publications[0].m_handle == insertHandle);
    CHECK(interface.publications[1].m_handle == attribHandle);
    CHECK(interface.publications[2].m_handle == seqEndHandle);
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG mapped zero-ATTRIB INSERT publishes immediately",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t recordHandle = 0x741;
    constexpr std::uint32_t insertHandle = 0x742;
    auto insertFrame = makeInsertFrame(insertHandle, recordHandle);
    REQUIRE(!insertFrame.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(insertHandle));
    REQUIRE(handleEntries.putModularChar(0));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        insertFrame.data(), insertFrame.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), insertFrame.size()));

    DRW_Block_Record record;
    DwgBlockOwnershipTestAccess::setHandle(record, recordHandle);
    record.name = "ZERO_ATTRIB_BLOCK";
    DwgBlockOwnershipTestAccess::setHandles(record, 0, 0, {insertHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(insertFrame.data(), insertFrame.size());
    REQUIRE(reader.walkBlockRecordEntities(
        &record, &objectBuffer, interface, recordHandle, recordHandle));
    REQUIRE(interface.inserts.size() == 1u);
    CHECK(interface.inserts.front().handle == insertHandle);
    CHECK(interface.inserts.front().attlist.empty());
    REQUIRE(interface.publications.size() == 1u);
    CHECK(interface.publications.front().m_handle == insertHandle);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Published);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 1u);
}

TEST_CASE("DWG modern simple block with a bad child publishes nothing",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t recordHandle = 0x749u;
    constexpr std::uint32_t blockHandle = 0x74Au;
    constexpr std::uint32_t firstLineHandle = 0x74Bu;
    constexpr std::uint32_t badLineHandle = 0x74Cu;
    constexpr std::uint32_t endBlockHandle = 0x74Du;
    constexpr std::uint32_t wrongOwner = 0x74Eu;

    const auto blockFrame = makeBlockFrame(
        blockHandle, recordHandle, false, "JOURNALED_LINES");
    const auto firstLineFrame = makeLineFrame(firstLineHandle, recordHandle);
    const auto badLineFrame = makeLineFrame(badLineHandle, wrongOwner);
    const auto endBlockFrame = makeBlockFrame(endBlockHandle, recordHandle,
                                              true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!firstLineFrame.empty());
    REQUIRE(!badLineFrame.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> objectData = blockFrame;
    const auto firstLineOffset =
        static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), firstLineFrame.cbegin(),
                      firstLineFrame.cend());
    const auto badLineOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), badLineFrame.cbegin(),
                      badLineFrame.cend());
    const auto endBlockOffset =
        static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                      endBlockFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(blockHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(firstLineHandle - blockHandle));
    REQUIRE(handleEntries.putModularChar(firstLineOffset));
    REQUIRE(handleEntries.putUModularChar(badLineHandle - firstLineHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(badLineOffset) - firstLineOffset));
    REQUIRE(handleEntries.putUModularChar(endBlockHandle - badLineHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(endBlockOffset) - badLineOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setCodePageForTest("ANSI_1252");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));

    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    record->name = "JOURNALED_LINES";
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle,
        {firstLineHandle, badLineHandle});
    REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

    DwgBlockJournalProbe interface;
    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
    CHECK(interface.callbacks.empty());
    CHECK(interface.blocks.empty());
    CHECK(interface.lines.empty());
    CHECK(interface.publications.empty());
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 4u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger)
        CHECK(entry.m_publicationCount == 0u);
}

TEST_CASE("DWG modern simple blocks retain block-scope callback order",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t recordHandle = 0x74Fu;
    constexpr std::uint32_t blockHandle = 0x750u;
    constexpr std::uint32_t lineHandle = 0x751u;
    constexpr std::uint32_t endBlockHandle = 0x752u;

    const auto runScope = [&](const UTF8STRING& name,
                              std::uint32_t owner,
                              const std::vector<std::string>& expectedOrder,
                              bool failReservation = false) {
        const auto blockFrame = makeBlockFrame(blockHandle, owner, false, name);
        const auto lineFrame = makeLineFrame(lineHandle, owner);
        const auto endBlockFrame = makeBlockFrame(endBlockHandle, owner, true);
        REQUIRE(!blockFrame.empty());
        REQUIRE(!lineFrame.empty());
        REQUIRE(!endBlockFrame.empty());

        std::vector<std::uint8_t> objectData = blockFrame;
        const auto lineOffset = static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), lineFrame.cbegin(), lineFrame.cend());
        const auto endBlockOffset =
            static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                          endBlockFrame.cend());

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(blockHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(lineHandle - blockHandle));
        REQUIRE(handleEntries.putModularChar(lineOffset));
        REQUIRE(handleEntries.putUModularChar(endBlockHandle - lineHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(endBlockOffset) - lineOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            objectData.data(), objectData.size()));
        reader.setVersionForTest(DRW::AC1018);
        reader.setCodePageForTest("ANSI_1252");
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), objectData.size()));
        auto* record = new DRW_Block_Record();
        DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
        record->name = name;
        DwgBlockOwnershipTestAccess::setHandles(
            *record, blockHandle, endBlockHandle, {lineHandle});
        REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

        DwgBlockJournalProbe interface;
        dwgBuffer objectBuffer(objectData.data(), objectData.size());
        if (failReservation) {
            reader.failBlockJournalReservationForTest();
            CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
            CHECK(interface.callbacks.empty());
            CHECK(interface.blocks.empty());
            CHECK(interface.lines.empty());
            CHECK(interface.publications.empty());
            CHECK(reader.ObjectMap.empty());
            REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
            for (const DRW_DwgFrameCoverageEntry& entry
                 : reader.m_dwgSourceFrameLedger) {
                CHECK(entry.m_disposition
                      == DRW_DwgFrameDisposition::Quarantined);
                CHECK(entry.m_publicationCount == 0u);
            }
            return;
        }
        REQUIRE(reader.readDwgBlocks(interface, &objectBuffer));
        CHECK(interface.callbacks == expectedOrder);
        REQUIRE(interface.blocks.size() == 1u);
        REQUIRE(interface.lines.size() == 1u);
        CHECK(interface.lines.front().handle == lineHandle);
        CHECK(interface.lines.front().parentHandle == owner);
        REQUIRE(interface.publications.size() == 3u);
        CHECK(reader.ObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
            CHECK(entry.m_publicationCount == 1u);
        }
    };

    SECTION("named block") {
        runScope("JOURNALED_NAMED_BLOCK", recordHandle,
                 {"block", "line", "endBlock"});
    }

    SECTION("model space") {
        runScope("*Model_Space", DRW::NoHandle,
                 {"block", "endBlock", "line"});
    }

    SECTION("reservation failure") {
        runScope("JOURNALED_RESERVATION_FAILURE", recordHandle, {}, true);
    }
}

TEST_CASE("DWG journal publishes modern BLOCK_RECORD reachability",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t recordHandle = 0x74Cu;
    constexpr std::uint32_t blockHandle = 0x74Du;
    constexpr std::uint32_t lineHandle = 0x74Eu;
    constexpr std::uint32_t endBlockHandle = 0x74Fu;
    const auto blockFrame = makeBlockFrame(
        blockHandle, recordHandle, false, "JOURNALED_REACHABILITY");
    const auto lineFrame = makeLineFrame(lineHandle, recordHandle);
    const auto endBlockFrame = makeBlockFrame(
        endBlockHandle, recordHandle, true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!lineFrame.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> objectData = blockFrame;
    const auto lineOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), lineFrame.cbegin(), lineFrame.cend());
    const auto endBlockOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                      endBlockFrame.cend());
    const auto recordOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.push_back(0u);

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(recordHandle));
    REQUIRE(handleEntries.putModularChar(recordOffset));
    REQUIRE(handleEntries.putUModularChar(blockHandle - recordHandle));
    REQUIRE(handleEntries.putModularChar(-static_cast<std::int64_t>(recordOffset)));
    REQUIRE(handleEntries.putUModularChar(lineHandle - blockHandle));
    REQUIRE(handleEntries.putModularChar(lineOffset));
    REQUIRE(handleEntries.putUModularChar(endBlockHandle - lineHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(endBlockOffset) - lineOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setCodePageForTest("ANSI_1252");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));

    DwgBlockJournalProbe interface;
    auto recordPublication = reader.publicationForTest(recordHandle);
    recordPublication.m_version = DRW::AC1018;
    recordPublication.m_encodedType = DRW::DwgBlockRecordObjectType;
    recordPublication.m_resolvedType = DRW::DwgBlockRecordObjectType;
    recordPublication.setCommonLinkEvidence(
        DRW_DwgCommonLinkEvidence::ValidatedAbsent);
    REQUIRE(reader.publishFrameForTest(interface, recordPublication));
    reader.ObjectMap.erase(recordHandle);

    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    record->name = "JOURNALED_REACHABILITY";
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle, {lineHandle});
    REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    REQUIRE(reader.readDwgBlocks(interface, &objectBuffer));
    CHECK(interface.callbacks == std::vector<std::string>{
        "block", "line", "endBlock", "reachability"});
    REQUIRE(interface.reachabilities.size() == 1u);
    const DRW_DwgBlockReachability& reachability =
        interface.reachabilities.front();
    CHECK(reachability.m_complete);
    CHECK(reachability.m_version == DRW::AC1018);
    CHECK(reachability.m_blockRecord.m_handle == recordHandle);
    CHECK(reachability.m_block.m_handle == blockHandle);
    CHECK(reachability.m_endBlock.m_handle == endBlockHandle);
    REQUIRE(reachability.m_entities.size() == 1u);
    CHECK(reachability.m_entities.front().m_handle == lineHandle);
    REQUIRE(interface.publications.size() == 4u);
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 4u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG journal publishes empty modern BLOCK_RECORD reachability",
          "[dwg][safety][journal]") {
    const auto runScope = [](const UTF8STRING& name, std::uint32_t owner,
                             std::uint32_t recordHandle,
                             std::uint32_t blockHandle,
                             std::uint32_t endBlockHandle) {
        CAPTURE(name);
        const auto blockFrame = makeBlockFrame(blockHandle, owner, false, name);
        const auto endBlockFrame = makeBlockFrame(endBlockHandle, owner, true);
        REQUIRE(!blockFrame.empty());
        REQUIRE(!endBlockFrame.empty());

        std::vector<std::uint8_t> objectData = blockFrame;
        const auto endBlockOffset =
            static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                          endBlockFrame.cend());
        const auto recordOffset = static_cast<std::uint32_t>(objectData.size());
        objectData.push_back(0u);

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(recordHandle));
        REQUIRE(handleEntries.putModularChar(recordOffset));
        REQUIRE(handleEntries.putUModularChar(blockHandle - recordHandle));
        REQUIRE(handleEntries.putModularChar(
            -static_cast<std::int64_t>(recordOffset)));
        REQUIRE(handleEntries.putUModularChar(endBlockHandle - blockHandle));
        REQUIRE(handleEntries.putModularChar(endBlockOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            objectData.data(), objectData.size()));
        reader.setVersionForTest(DRW::AC1018);
        reader.setCodePageForTest("ANSI_1252");
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), objectData.size()));

        DwgBlockJournalProbe interface;
        auto recordPublication = reader.publicationForTest(recordHandle);
        recordPublication.m_version = DRW::AC1018;
        recordPublication.m_encodedType = DRW::DwgBlockRecordObjectType;
        recordPublication.m_resolvedType = DRW::DwgBlockRecordObjectType;
        recordPublication.setCommonLinkEvidence(
            DRW_DwgCommonLinkEvidence::ValidatedAbsent);
        REQUIRE(reader.publishFrameForTest(interface, recordPublication));
        reader.ObjectMap.erase(recordHandle);

        auto* record = new DRW_Block_Record();
        DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
        record->name = name;
        DwgBlockOwnershipTestAccess::setHandles(
            *record, blockHandle, endBlockHandle, {});
        REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

        dwgBuffer objectBuffer(objectData.data(), objectData.size());
        REQUIRE(reader.readDwgBlocks(interface, &objectBuffer));
        CHECK(interface.callbacks == std::vector<std::string>{
            "block", "endBlock", "reachability"});
        REQUIRE(interface.blocks.size() == 1u);
        CHECK(interface.blocks.front().parentHandle == recordHandle);
        REQUIRE(interface.reachabilities.size() == 1u);
        const DRW_DwgBlockReachability& reachability =
            interface.reachabilities.front();
        CHECK(reachability.m_complete);
        CHECK(reachability.m_version == DRW::AC1018);
        CHECK(reachability.m_blockRecord.m_handle == recordHandle);
        CHECK(reachability.m_block.m_handle == blockHandle);
        CHECK(reachability.m_endBlock.m_handle == endBlockHandle);
        CHECK(reachability.m_entities.empty());
        REQUIRE(interface.publications.size() == 3u);
        CHECK(reader.ObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry
             : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
            CHECK(entry.m_publicationCount == 1u);
        }
    };

    SECTION("named block") {
        runScope("JOURNALED_EMPTY", 0x760u, 0x760u, 0x761u, 0x762u);
    }

    SECTION("model space") {
        runScope("*Model_Space", DRW::NoHandle, 0x763u, 0x764u, 0x765u);
    }

    SECTION("paper space") {
        runScope("*Paper_Space", DRW::NoHandle, 0x766u, 0x767u, 0x768u);
    }
}

TEST_CASE("DWG journal quarantines a BLOCK when reachability delivery fails",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t recordHandle = 0x750u;
    constexpr std::uint32_t blockHandle = 0x751u;
    constexpr std::uint32_t lineHandle = 0x752u;
    constexpr std::uint32_t endBlockHandle = 0x753u;
    const auto blockFrame = makeBlockFrame(
        blockHandle, recordHandle, false, "JOURNALED_REACHABILITY_THROW");
    const auto lineFrame = makeLineFrame(lineHandle, recordHandle);
    const auto endBlockFrame = makeBlockFrame(
        endBlockHandle, recordHandle, true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!lineFrame.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> objectData = blockFrame;
    const auto lineOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), lineFrame.cbegin(), lineFrame.cend());
    const auto endBlockOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                      endBlockFrame.cend());
    const auto recordOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.push_back(0u);

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(recordHandle));
    REQUIRE(handleEntries.putModularChar(recordOffset));
    REQUIRE(handleEntries.putUModularChar(blockHandle - recordHandle));
    REQUIRE(handleEntries.putModularChar(-static_cast<std::int64_t>(recordOffset)));
    REQUIRE(handleEntries.putUModularChar(lineHandle - blockHandle));
    REQUIRE(handleEntries.putModularChar(lineOffset));
    REQUIRE(handleEntries.putUModularChar(endBlockHandle - lineHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(endBlockOffset) - lineOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setCodePageForTest("ANSI_1252");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));

    DwgThrowingBlockJournalProbe interface;
    interface.throwReachability = true;
    auto recordPublication = reader.publicationForTest(recordHandle);
    recordPublication.m_version = DRW::AC1018;
    recordPublication.m_encodedType = DRW::DwgBlockRecordObjectType;
    recordPublication.m_resolvedType = DRW::DwgBlockRecordObjectType;
    recordPublication.setCommonLinkEvidence(
        DRW_DwgCommonLinkEvidence::ValidatedAbsent);
    REQUIRE(reader.publishFrameForTest(interface, recordPublication));
    reader.ObjectMap.erase(recordHandle);

    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    record->name = "JOURNALED_REACHABILITY_THROW";
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle, {lineHandle});
    REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
    CHECK(interface.callbacks == std::vector<std::string>{
        "block", "line", "endBlock"});
    CHECK(interface.reachabilities.empty());
    REQUIRE(interface.publications.size() == 3u);
    CHECK(interface.publications[0].m_handle == recordHandle);
    CHECK(interface.publications[1].m_handle == lineHandle);
    CHECK(interface.publications[2].m_handle == blockHandle);
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 4u);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
          == DRW_DwgFrameDisposition::Published);
    CHECK(reader.m_dwgSourceFrameLedger[1].m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger[1].m_reason
          == DRW_DwgFrameCoverageReason::CallbackException);
    CHECK(reader.m_dwgSourceFrameLedger[2].m_disposition
          == DRW_DwgFrameDisposition::Published);
    CHECK(reader.m_dwgSourceFrameLedger[3].m_disposition
          == DRW_DwgFrameDisposition::Quarantined);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger)
        CHECK(entry.m_publicationCount <= 1u);
}

TEST_CASE("DWG journal stages custom proxy graphics with its raw carrier",
          "[dwg][safety][journal][proxy]") {
    constexpr std::uint16_t classNumber = 500u;
    constexpr std::uint32_t recordHandle = 0x75Bu;
    constexpr std::uint32_t blockHandle = 0x75Cu;
    constexpr std::uint32_t entityHandle = 0x75Du;
    constexpr std::uint32_t endBlockHandle = 0x75Eu;

    const std::string graphics = makeProxyCircleGraphics(2u);
    REQUIRE(!graphics.empty());
    const auto blockFrame = makeBlockFrame(
        blockHandle, recordHandle, false, "JOURNALED_PROXY");
    const auto entityFrame = makeUnknownCustomEntityFrame(
        classNumber, entityHandle, recordHandle, graphics);
    const auto endBlockFrame = makeBlockFrame(
        endBlockHandle, recordHandle, true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!entityFrame.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> objectData = blockFrame;
    const auto entityOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), entityFrame.cbegin(), entityFrame.cend());
    const auto endBlockOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), endBlockFrame.cbegin(), endBlockFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(blockHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(entityHandle - blockHandle));
    REQUIRE(handleEntries.putModularChar(entityOffset));
    REQUIRE(handleEntries.putUModularChar(endBlockHandle - entityHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(endBlockOffset) - entityOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    const auto readBlock = [&](bool failProxyAdmission) {
        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            objectData.data(), objectData.size()));
        reader.setVersionForTest(DRW::AC1018);
        reader.setCodePageForTest("ANSI_1252");
        reader.addCustomEntityClass(classNumber, "PROXY_CUSTOM_ENTITY");
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), objectData.size()));
        auto* record = new DRW_Block_Record();
        DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
        record->name = "JOURNALED_PROXY";
        DwgBlockOwnershipTestAccess::setHandles(
            *record, blockHandle, endBlockHandle, {entityHandle});
        REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

        DwgBlockJournalProbe interface;
        dwgBuffer objectBuffer(objectData.data(), objectData.size());
        if (failProxyAdmission) {
            // BLOCK and ENDBLK consume the first two admissions; the host's
            // base reservation is third and the extra proxy primitive slot is
            // fourth.
            reader.failBlockJournalReservationForTest(4u);
            CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
            CHECK(interface.callbacks.empty());
            CHECK(interface.publications.empty());
            REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
            for (const DRW_DwgFrameCoverageEntry& entry
                 : reader.m_dwgSourceFrameLedger) {
                CHECK(entry.m_publicationCount == 0u);
            }
            return;
        }

        REQUIRE(reader.readDwgBlocks(interface, &objectBuffer));
        CHECK(interface.callbacks == std::vector<std::string>{
            "block", "circle", "circle", "raw", "endBlock"});
        REQUIRE(interface.circles.size() == 2u);
        CHECK(interface.circles[0].handle == entityHandle);
        CHECK(interface.circles[0].parentHandle == recordHandle);
        CHECK(interface.circles[1].basePoint.x == Catch::Approx(1.0));
        REQUIRE(interface.rawObjects.size() == 1u);
        CHECK(interface.rawObjects.front().m_handle == entityHandle);
        CHECK(interface.rawObjects.front().m_recordName == "PROXY_CUSTOM_ENTITY");
        REQUIRE(interface.publications.size() == 3u);
        CHECK(std::any_of(interface.publications.cbegin(),
                          interface.publications.cend(),
                          [entityHandle](const DRW_DwgFramePublication& value) {
                              return value.m_handle == entityHandle;
                          }));
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry
             : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
            CHECK(entry.m_publicationCount == 1u);
        }
    };

    SECTION("successful replay") {
        readBlock(false);
    }

    SECTION("proxy event admission failure") {
        readBlock(true);
    }
}

TEST_CASE("DWG journal stages built-in proxy entities with their graphics",
          "[dwg][safety][journal][proxy]") {
    constexpr std::uint32_t recordHandle = 0x75Fu;
    constexpr std::uint32_t blockHandle = 0x760u;
    constexpr std::uint32_t entityHandle = 0x761u;
    constexpr std::uint32_t endBlockHandle = 0x762u;

    const std::string graphics = makeProxyCircleGraphics(2u);
    REQUIRE(!graphics.empty());
    const auto blockFrame = makeBlockFrame(
        blockHandle, recordHandle, false, "JOURNALED_PROXY_ENTITY");
    const auto entityFrame = makeProxyEntityFrame(
        entityHandle, recordHandle, graphics);
    const auto endBlockFrame = makeBlockFrame(endBlockHandle, recordHandle,
                                              true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!entityFrame.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> objectData = blockFrame;
    const auto entityOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), entityFrame.cbegin(), entityFrame.cend());
    const auto endBlockOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), endBlockFrame.cbegin(), endBlockFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(blockHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(entityHandle - blockHandle));
    REQUIRE(handleEntries.putModularChar(entityOffset));
    REQUIRE(handleEntries.putUModularChar(endBlockHandle - entityHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(endBlockOffset) - entityOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setCodePageForTest("ANSI_1252");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));
    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    record->name = "JOURNALED_PROXY_ENTITY";
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle, {entityHandle});
    REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

    DwgBlockJournalProbe interface;
    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    REQUIRE(reader.readDwgBlocks(interface, &objectBuffer));
    CHECK(interface.callbacks == std::vector<std::string>{
        "block", "circle", "circle", "proxy", "raw", "endBlock"});
    REQUIRE(interface.circles.size() == 2u);
    CHECK(interface.circles[0].handle == entityHandle);
    CHECK(interface.circles[0].parentHandle == recordHandle);
    REQUIRE(interface.proxyEntities.size() == 1u);
    CHECK(interface.proxyEntities.front().handle == entityHandle);
    REQUIRE(interface.rawObjects.size() == 1u);
    CHECK(interface.rawObjects.front().m_handle == entityHandle);
    CHECK(interface.rawObjects.front().m_recordName == "ACAD_PROXY_ENTITY");
    REQUIRE(interface.publications.size() == 3u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG proxy blocks hide staged carriers after a bad later sibling",
          "[dwg][safety][journal][proxy]") {
    constexpr std::uint16_t classNumber = 502u;
    constexpr std::uint32_t recordHandle = 0x766u;
    constexpr std::uint32_t blockHandle = 0x767u;
    constexpr std::uint32_t entityHandle = 0x768u;
    constexpr std::uint32_t badLineHandle = 0x769u;
    constexpr std::uint32_t endBlockHandle = 0x76Au;
    constexpr std::uint32_t wrongOwner = 0x76Bu;
    const std::string graphics = makeProxyCircleGraphics(2u);
    REQUIRE(!graphics.empty());

    const auto runCase = [&](bool builtInCarrier) {
        CAPTURE(builtInCarrier);
        const auto blockFrame = makeBlockFrame(
            blockHandle, recordHandle, false,
            builtInCarrier ? "JOURNALED_BUILTIN_PROXY_BAD"
                           : "JOURNALED_CUSTOM_PROXY_BAD");
        const auto entityFrame = builtInCarrier
            ? makeProxyEntityFrame(entityHandle, recordHandle, graphics)
            : makeUnknownCustomEntityFrame(
                classNumber, entityHandle, recordHandle, graphics);
        const auto badLineFrame = makeLineFrame(badLineHandle, wrongOwner);
        const auto endBlockFrame = makeBlockFrame(
            endBlockHandle, recordHandle, true);
        REQUIRE(!blockFrame.empty());
        REQUIRE(!entityFrame.empty());
        REQUIRE(!badLineFrame.empty());
        REQUIRE(!endBlockFrame.empty());

        std::vector<std::uint8_t> objectData = blockFrame;
        const auto entityOffset = static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), entityFrame.cbegin(),
                          entityFrame.cend());
        const auto badLineOffset = static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), badLineFrame.cbegin(),
                          badLineFrame.cend());
        const auto endBlockOffset =
            static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                          endBlockFrame.cend());

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(blockHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(entityHandle - blockHandle));
        REQUIRE(handleEntries.putModularChar(entityOffset));
        REQUIRE(handleEntries.putUModularChar(badLineHandle - entityHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(badLineOffset) - entityOffset));
        REQUIRE(handleEntries.putUModularChar(endBlockHandle - badLineHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(endBlockOffset) - badLineOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            objectData.data(), objectData.size()));
        reader.setVersionForTest(DRW::AC1018);
        reader.setCodePageForTest("ANSI_1252");
        if (!builtInCarrier)
            reader.addCustomEntityClass(classNumber, "PROXY_CUSTOM_ENTITY");
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), objectData.size()));
        auto* record = new DRW_Block_Record();
        DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
        record->name = builtInCarrier ? "JOURNALED_BUILTIN_PROXY_BAD"
                                      : "JOURNALED_CUSTOM_PROXY_BAD";
        DwgBlockOwnershipTestAccess::setHandles(
            *record, blockHandle, endBlockHandle,
            {entityHandle, badLineHandle});
        REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

        DwgBlockJournalProbe interface;
        dwgBuffer objectBuffer(objectData.data(), objectData.size());
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        CHECK(interface.callbacks.empty());
        CHECK(interface.blocks.empty());
        CHECK(interface.circles.empty());
        CHECK(interface.proxyEntities.empty());
        CHECK(interface.rawObjects.empty());
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 4u);
        for (const DRW_DwgFrameCoverageEntry& entry
             : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_publicationCount == 0u);
            CHECK((entry.m_disposition
                   == DRW_DwgFrameDisposition::Failed
                   || entry.m_disposition
                          == DRW_DwgFrameDisposition::Quarantined));
        }
    };

    runCase(false);
    runCase(true);
}

TEST_CASE("DWG proxy carrier callback failures retain only replayed prefix",
          "[dwg][safety][journal][proxy]") {
    constexpr std::uint16_t classNumber = 503u;
    constexpr std::uint32_t recordHandle = 0x76Cu;
    constexpr std::uint32_t blockHandle = 0x76Du;
    constexpr std::uint32_t entityHandle = 0x76Eu;
    constexpr std::uint32_t endBlockHandle = 0x76Fu;
    const std::string graphics = makeProxyCircleGraphics(2u);
    REQUIRE(!graphics.empty());

    const auto runCase = [&](bool builtInCarrier) {
        CAPTURE(builtInCarrier);
        const auto blockFrame = makeBlockFrame(
            blockHandle, recordHandle, false,
            builtInCarrier ? "JOURNALED_BUILTIN_PROXY_THROW"
                           : "JOURNALED_CUSTOM_PROXY_THROW");
        const auto entityFrame = builtInCarrier
            ? makeProxyEntityFrame(entityHandle, recordHandle, graphics)
            : makeUnknownCustomEntityFrame(
                classNumber, entityHandle, recordHandle, graphics);
        const auto endBlockFrame = makeBlockFrame(
            endBlockHandle, recordHandle, true);
        REQUIRE(!blockFrame.empty());
        REQUIRE(!entityFrame.empty());
        REQUIRE(!endBlockFrame.empty());

        std::vector<std::uint8_t> objectData = blockFrame;
        const auto entityOffset = static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), entityFrame.cbegin(),
                          entityFrame.cend());
        const auto endBlockOffset =
            static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                          endBlockFrame.cend());

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(blockHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(entityHandle - blockHandle));
        REQUIRE(handleEntries.putModularChar(entityOffset));
        REQUIRE(handleEntries.putUModularChar(endBlockHandle - entityHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(endBlockOffset) - entityOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            objectData.data(), objectData.size()));
        reader.setVersionForTest(DRW::AC1018);
        reader.setCodePageForTest("ANSI_1252");
        if (!builtInCarrier)
            reader.addCustomEntityClass(classNumber, "PROXY_CUSTOM_ENTITY");
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), objectData.size()));
        auto* record = new DRW_Block_Record();
        DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
        record->name = builtInCarrier ? "JOURNALED_BUILTIN_PROXY_THROW"
                                      : "JOURNALED_CUSTOM_PROXY_THROW";
        DwgBlockOwnershipTestAccess::setHandles(
            *record, blockHandle, endBlockHandle, {entityHandle});
        REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

        DwgThrowingBlockJournalProbe interface;
        interface.throwProxyCarrier = builtInCarrier;
        interface.throwRawCarrier = !builtInCarrier;
        dwgBuffer objectBuffer(objectData.data(), objectData.size());
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        CHECK(interface.callbacks == std::vector<std::string>{
            "block", "circle", "circle",
            builtInCarrier ? "proxy" : "raw"});
        CHECK(interface.blocks.size() == 1u);
        CHECK(interface.circles.size() == 2u);
        CHECK(interface.proxyEntities.size()
              == (builtInCarrier ? 1u : 0u));
        CHECK(interface.rawObjects.size()
              == (builtInCarrier ? 0u : 1u));
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
              == DRW_DwgFrameDisposition::Quarantined);
        CHECK(reader.m_dwgSourceFrameLedger[1].m_disposition
              == DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger[1].m_reason
              == DRW_DwgFrameCoverageReason::CallbackException);
        CHECK(reader.m_dwgSourceFrameLedger[2].m_disposition
              == DRW_DwgFrameDisposition::Quarantined);
        for (const DRW_DwgFrameCoverageEntry& entry
             : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_publicationCount == 0u);
        }
    };

    runCase(false);
    runCase(true);
}

TEST_CASE("DWG modern common headers preserve custom proxy graphics",
          "[dwg][safety][proxy]") {
    constexpr std::uint16_t classNumber = 500u;
    constexpr std::uint32_t entityHandle = 0x763u;
    constexpr std::uint32_t ownerHandle = 0x764u;
    const std::string graphics = makeProxyCircleGraphics(2u);
    REQUIRE(!graphics.empty());

    for (const DRW::Version version : {DRW::AC1024, DRW::AC1027}) {
        const auto frame = makeModernUnknownCustomEntityFrame(
            version, classNumber, entityHandle, ownerHandle, graphics);
        REQUIRE(!frame.empty());

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(frame.data()), frame.size()));
        reader.setVersionForTest(version);
        reader.setCodePageForTest("ANSI_1252");
        reader.addCustomEntityClass(classNumber, "MODERN_PROXY_CUSTOM");
        objHandle entity(classNumber, entityHandle, 0);
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(frame.data()), frame.size());
        DwgBlockJournalProbe interface;
        REQUIRE(reader.readDwgEntity(&objectBuffer, entity, interface));
        REQUIRE(interface.circles.size() == 2u);
        CHECK(interface.circles.front().handle == entityHandle);
        CHECK(interface.circles.front().parentHandle == ownerHandle);
        REQUIRE(interface.rawObjects.size() == 1u);
        CHECK(interface.rawObjects.front().m_handle == entityHandle);
        CHECK(interface.rawObjects.front().m_recordName == "MODERN_PROXY_CUSTOM");
        CHECK(interface.rawObjects.front().m_commonLinkEvidence
              == DRW_DwgCommonLinkEvidence::Validated);
        CHECK(interface.rawObjects.front().m_commonHandleDataValidated);
    }
}

TEST_CASE("DWG common-link evidence preserves validated absence",
          "[dwg][safety][metadata]") {
    constexpr std::uint16_t classNumber = 501u;
    constexpr std::uint32_t entityHandle = 0x765u;

    const auto frame = makeModernUnknownCustomEntityFrame(
        DRW::AC1024, classNumber, entityHandle, DRW::NoHandle, {});
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1024);
    reader.setCodePageForTest("ANSI_1252");
    reader.addCustomEntityClass(classNumber, "MODERN_EMPTY_CUSTOM");
    objHandle entity(classNumber, entityHandle, 0);
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgReadProbe interface;
    REQUIRE(reader.readDwgEntity(&objectBuffer, entity, interface));
    REQUIRE(interface.unsupportedObjects.size() == 1u);
    const DRW_UnsupportedObject& raw = interface.unsupportedObjects.front();
    CHECK(raw.m_parentHandle == DRW::NoHandle);
    CHECK(raw.m_reactorHandles.empty());
    CHECK(raw.m_xDictHandle == DRW::NoHandle);
    CHECK(raw.m_commonLinkEvidence
          == DRW_DwgCommonLinkEvidence::ValidatedAbsent);
    CHECK(raw.m_commonHandleDataValidated);

    LC_DwgAdvancedMetadata metadata;
    DRW_DwgFramePublication typed;
    typed.m_version = DRW::AC1024;
    typed.m_handle = entityHandle;
    typed.m_encodedType = classNumber;
    typed.m_resolvedType = classNumber;
    typed.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    typed.m_parentHandle = 0x766u;
    typed.m_carrier = DRW_DwgFramePublication::Carrier::Typed;
    metadata.addDwgFramePublication(typed);

    DRW_DwgFramePublication rawReceipt = typed;
    rawReceipt.setCommonLinkEvidence(
        DRW_DwgCommonLinkEvidence::ValidatedAbsent);
    rawReceipt.m_parentHandle = DRW::NoHandle;
    rawReceipt.m_carrier = DRW_DwgFramePublication::Carrier::Raw;
    metadata.addDwgFramePublication(rawReceipt);

    CHECK(metadata.dwgFramePublications().size() == 2u);
    CHECK_FALSE(metadata.dwgFramePublicationCoverageComplete());
    CHECK(metadata.dwgFramePublicationConflicts()
          == std::vector<std::uint32_t>{entityHandle});
}

TEST_CASE("DWG journalled INSERT aggregate stays private on block failure",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t recordHandle = 0x753u;
    constexpr std::uint32_t blockHandle = 0x754u;
    constexpr std::uint32_t insertHandle = 0x755u;
    constexpr std::uint32_t attribHandle = 0x756u;
    constexpr std::uint32_t seqEndHandle = 0x757u;
    constexpr std::uint32_t badLineHandle = 0x758u;
    constexpr std::uint32_t endBlockHandle = 0x759u;
    constexpr std::uint32_t wrongOwner = 0x75Au;

    const auto blockFrame = makeBlockFrame(
        blockHandle, recordHandle, false, "JOURNALED_INSERT");
    const auto insertFrame = makeInsertWithChildrenFrame(
        insertHandle, recordHandle, attribHandle, seqEndHandle);
    const auto attribFrame = makeAttribFrame(attribHandle, insertHandle);
    const auto seqEndFrame = makeSeqEndFrame(seqEndHandle, insertHandle);
    const auto badLineFrame = makeLineFrame(badLineHandle, wrongOwner);
    const auto endBlockFrame = makeBlockFrame(endBlockHandle, recordHandle,
                                              true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!insertFrame.empty());
    REQUIRE(!attribFrame.empty());
    REQUIRE(!seqEndFrame.empty());
    REQUIRE(!badLineFrame.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> objectData = blockFrame;
    const auto insertOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), insertFrame.cbegin(), insertFrame.cend());
    const auto attribOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), attribFrame.cbegin(), attribFrame.cend());
    const auto seqEndOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), seqEndFrame.cbegin(), seqEndFrame.cend());
    const auto badLineOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), badLineFrame.cbegin(), badLineFrame.cend());
    const auto endBlockOffset =
        static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                      endBlockFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(blockHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(insertHandle - blockHandle));
    REQUIRE(handleEntries.putModularChar(insertOffset));
    REQUIRE(handleEntries.putUModularChar(attribHandle - insertHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(attribOffset) - insertOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - attribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(seqEndOffset) - attribOffset));
    REQUIRE(handleEntries.putUModularChar(badLineHandle - seqEndHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(badLineOffset) - seqEndOffset));
    REQUIRE(handleEntries.putUModularChar(endBlockHandle - badLineHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(endBlockOffset) - badLineOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setCodePageForTest("ANSI_1252");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));
    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    record->name = "JOURNALED_INSERT";
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle,
        {insertHandle, attribHandle, seqEndHandle, badLineHandle});
    REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

    DwgBlockInsertOrderProbe interface;
    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    const auto checkPrivate = [&]() {
        CHECK(interface.callbacks.empty());
        CHECK(interface.blocks.empty());
        CHECK(interface.inserts.empty());
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 6u);
        for (const DRW_DwgFrameCoverageEntry& entry
             : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_publicationCount == 0u);
        }
    };

    SECTION("malformed later sibling") {
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        checkPrivate();
    }

    SECTION("aggregate admission") {
        reader.failBlockJournalReservationForTest(3u);
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        checkPrivate();
        CHECK(reader.consumedPolylineChildCountForTest() == 0u);
        CHECK(reader.consumedSeqEndCountForTest() == 0u);
    }

    SECTION("aggregate transfer") {
        reader.failBlockJournalAdoptionForTest(4u);
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        checkPrivate();
        CHECK(reader.consumedPolylineChildCountForTest() == 0u);
        CHECK(reader.consumedSeqEndCountForTest() == 0u);
    }
}

TEST_CASE("DWG journalled POLYLINE aggregate stays private on block failure",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t recordHandle = 0xE0u;
    constexpr std::uint32_t blockHandle = 0xF0u;
    constexpr std::uint32_t polylineHandle = 0x100u;
    constexpr std::uint32_t vertexHandle = 0x101u;
    constexpr std::uint32_t seqEndHandle = 0x102u;
    constexpr std::uint32_t badLineHandle = 0x103u;
    constexpr std::uint32_t endBlockHandle = 0x104u;
    constexpr std::uint32_t wrongOwner = 0x105u;

    const auto blockFrame = makeBlockFrame(
        blockHandle, recordHandle, false, "JOURNALED_POLYLINE");
    const auto polyline = makePolylineFrameFixture(
        false, false, false, false, 0.0, false, DRW::AC1018,
        DRW_Vertex::DwgSubtype::Vertex2D, recordHandle);
    const auto badLineFrame = makeLineFrame(badLineHandle, wrongOwner);
    const auto endBlockFrame = makeBlockFrame(endBlockHandle, recordHandle,
                                              true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!polyline.bytes.empty());
    REQUIRE(!badLineFrame.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> objectData = blockFrame;
    const auto polylineOffset =
        static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), polyline.bytes.cbegin(),
                      polyline.bytes.cend());
    const auto badLineOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), badLineFrame.cbegin(), badLineFrame.cend());
    const auto endBlockOffset =
        static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                      endBlockFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(blockHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(polylineHandle - blockHandle));
    REQUIRE(handleEntries.putModularChar(polylineOffset));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(polylineOffset + polyline.vertexOffset)
        - polylineOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(polylineOffset + polyline.seqEndOffset)
        - (polylineOffset + polyline.vertexOffset)));
    REQUIRE(handleEntries.putUModularChar(badLineHandle - seqEndHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(badLineOffset)
        - (polylineOffset + polyline.seqEndOffset)));
    REQUIRE(handleEntries.putUModularChar(endBlockHandle - badLineHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(endBlockOffset) - badLineOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setCodePageForTest("ANSI_1252");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));
    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    record->name = "JOURNALED_POLYLINE";
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle,
        {polylineHandle, vertexHandle, seqEndHandle, badLineHandle});
    REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

    DwgReadProbe interface;
    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    const auto checkPrivate = [&]() {
        CHECK(interface.blockCount == 0u);
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.framePublications.empty());
        CHECK(reader.ObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 6u);
        for (const DRW_DwgFrameCoverageEntry& entry
             : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_publicationCount == 0u);
        }
    };

    SECTION("malformed later sibling") {
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        checkPrivate();
    }

    SECTION("aggregate admission") {
        reader.failBlockJournalReservationForTest(3u);
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        checkPrivate();
        CHECK(reader.consumedPolylineChildCountForTest() == 0u);
        CHECK(reader.consumedSeqEndCountForTest() == 0u);
    }

    SECTION("aggregate transfer") {
        reader.failBlockJournalAdoptionForTest(4u);
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        checkPrivate();
        CHECK(reader.consumedPolylineChildCountForTest() == 0u);
        CHECK(reader.consumedSeqEndCountForTest() == 0u);
    }
}

TEST_CASE("DWG journalled POLYLINE aggregate publishes complete sources",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t recordHandle = 0xE1u;
    constexpr std::uint32_t blockHandle = 0xF0u;
    constexpr std::uint32_t polylineHandle = 0x100u;
    constexpr std::uint32_t vertexHandle = 0x101u;
    constexpr std::uint32_t seqEndHandle = 0x102u;
    constexpr std::uint32_t endBlockHandle = 0x103u;

    const auto blockFrame = makeBlockFrame(
        blockHandle, recordHandle, false, "JOURNALED_POLYLINE_OK");
    const auto polyline = makePolylineFrameFixture(
        false, false, false, false, 0.0, false, DRW::AC1018,
        DRW_Vertex::DwgSubtype::Vertex2D, recordHandle);
    const auto endBlockFrame = makeBlockFrame(endBlockHandle, recordHandle,
                                              true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!polyline.bytes.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> objectData = blockFrame;
    const auto polylineOffset =
        static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), polyline.bytes.cbegin(),
                      polyline.bytes.cend());
    const auto endBlockOffset =
        static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                      endBlockFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(blockHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(polylineHandle - blockHandle));
    REQUIRE(handleEntries.putModularChar(polylineOffset));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(polyline.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(polyline.seqEndOffset)
        - polyline.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(endBlockHandle - seqEndHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(endBlockOffset)
        - (polylineOffset + polyline.seqEndOffset)));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setCodePageForTest("ANSI_1252");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));
    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    record->name = "JOURNALED_POLYLINE_OK";
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle,
        {polylineHandle, vertexHandle, seqEndHandle});
    REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

    DwgReadProbe interface;
    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    REQUIRE(reader.readDwgBlocks(interface, &objectBuffer));
    CHECK(interface.blockCount == 1u);
    CHECK(interface.polylineCount == 1u);
    REQUIRE(interface.polylineHandles.size() == 1u);
    CHECK(interface.polylineHandles.front() == polylineHandle);
    REQUIRE(interface.framePublications.size() == 5u);
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 5u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
}

TEST_CASE("DWG journalled POLYLINE replay failures quarantine pending sources",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t recordHandle = 0xE2u;
    constexpr std::uint32_t blockHandle = 0xF0u;
    constexpr std::uint32_t polylineHandle = 0x100u;
    constexpr std::uint32_t vertexHandle = 0x101u;
    constexpr std::uint32_t seqEndHandle = 0x102u;
    constexpr std::uint32_t endBlockHandle = 0x103u;

    const auto blockFrame = makeBlockFrame(
        blockHandle, recordHandle, false, "JOURNALED_POLYLINE_THROW");
    const auto polyline = makePolylineFrameFixture(
        false, false, false, false, 0.0, false, DRW::AC1018,
        DRW_Vertex::DwgSubtype::Vertex2D, recordHandle);
    const auto endBlockFrame = makeBlockFrame(endBlockHandle, recordHandle,
                                              true);
    REQUIRE(!blockFrame.empty());
    REQUIRE(!polyline.bytes.empty());
    REQUIRE(!endBlockFrame.empty());

    std::vector<std::uint8_t> objectData = blockFrame;
    const auto polylineOffset =
        static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), polyline.bytes.cbegin(),
                      polyline.bytes.cend());
    const auto endBlockOffset =
        static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), endBlockFrame.cbegin(),
                      endBlockFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(blockHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(polylineHandle - blockHandle));
    REQUIRE(handleEntries.putModularChar(polylineOffset));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(polyline.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(polyline.seqEndOffset)
        - polyline.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(endBlockHandle - seqEndHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(endBlockOffset)
        - (polylineOffset + polyline.seqEndOffset)));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    reader.setCodePageForTest("ANSI_1252");
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));
    auto* record = new DRW_Block_Record();
    DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
    record->name = "JOURNALED_POLYLINE_THROW";
    DwgBlockOwnershipTestAccess::setHandles(
        *record, blockHandle, endBlockHandle,
        {polylineHandle, vertexHandle, seqEndHandle});
    REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

    DwgThrowingJournalPolylineProbe interface;
    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    const auto checkFailure = [&](std::size_t expectedPolylines) {
        CHECK(interface.blockCount == 1u);
        CHECK(interface.polylineCount == expectedPolylines);
        CHECK(interface.framePublications.empty());
        CHECK(reader.ObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 5u);
        CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
              == DRW_DwgFrameDisposition::Quarantined);
        CHECK(reader.m_dwgSourceFrameLedger[1].m_disposition
              == DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger[1].m_reason
              == DRW_DwgFrameCoverageReason::CallbackException);
        for (const DRW_DwgFrameCoverageEntry& entry
             : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_publicationCount == 0u);
        }
        for (std::size_t index = 2;
             index < reader.m_dwgSourceFrameLedger.size(); ++index) {
            CHECK(reader.m_dwgSourceFrameLedger[index].m_disposition
                  == DRW_DwgFrameDisposition::Quarantined);
        }
    };

    SECTION("semantic callback") {
        interface.throwPolyline = true;
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        checkFailure(0u);
    }

    SECTION("parent receipt") {
        interface.throwReceipt = true;
        CHECK_FALSE(reader.readDwgBlocks(interface, &objectBuffer));
        checkFailure(1u);
    }
}

TEST_CASE("DWG mapped INSERT respects named and model-space block scopes",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    constexpr std::uint32_t recordHandle = 0x751;
    constexpr std::uint32_t blockHandle = 0x752;
    constexpr std::uint32_t insertHandle = 0x753;
    constexpr std::uint32_t attribHandle = 0x754;
    constexpr std::uint32_t seqEndHandle = 0x755;
    constexpr std::uint32_t endBlockHandle = 0x756;

    const auto runScope = [&](const UTF8STRING& name,
                              std::uint32_t owner,
                              const std::vector<std::string>& expectedOrder) {
        auto blockFrame = makeBlockFrame(blockHandle, owner, false, name);
        auto insertFrame = makeInsertWithChildrenFrame(
            insertHandle, owner, attribHandle, seqEndHandle);
        auto attribFrame = makeAttribFrame(attribHandle, insertHandle);
        auto seqEndFrame = makeSeqEndFrame(seqEndHandle, insertHandle);
        auto endBlockFrame = makeBlockFrame(endBlockHandle, owner, true);
        REQUIRE(!blockFrame.empty());
        REQUIRE(!insertFrame.empty());
        REQUIRE(!attribFrame.empty());
        REQUIRE(!seqEndFrame.empty());
        REQUIRE(!endBlockFrame.empty());

        std::vector<std::uint8_t> objectData = blockFrame;
        const auto insertOffset = static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), insertFrame.cbegin(), insertFrame.cend());
        const auto attribOffset = static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), attribFrame.cbegin(), attribFrame.cend());
        const auto seqEndOffset = static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), seqEndFrame.cbegin(), seqEndFrame.cend());
        const auto endBlockOffset = static_cast<std::uint32_t>(objectData.size());
        objectData.insert(objectData.end(), endBlockFrame.cbegin(), endBlockFrame.cend());

        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(blockHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(insertHandle - blockHandle));
        REQUIRE(handleEntries.putModularChar(insertOffset));
        REQUIRE(handleEntries.putUModularChar(attribHandle - insertHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(attribOffset) - insertOffset));
        REQUIRE(handleEntries.putUModularChar(seqEndHandle - attribHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(seqEndOffset) - attribOffset));
        REQUIRE(handleEntries.putUModularChar(endBlockHandle - seqEndHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(endBlockOffset) - seqEndOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            objectData.data(), objectData.size()));
        reader.setVersionForTest(DRW::AC1018);
        reader.setCodePageForTest("ANSI_1252");
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), objectData.size()));
        auto* record = new DRW_Block_Record();
        DwgBlockOwnershipTestAccess::setHandle(*record, recordHandle);
        record->name = name;
        DwgBlockOwnershipTestAccess::setHandles(
            *record, blockHandle, endBlockHandle,
            {insertHandle, attribHandle, seqEndHandle});
        REQUIRE(reader.blockRecordmap.emplace(recordHandle, record).second);

        DwgBlockInsertOrderProbe interface;
        dwgBuffer objectBuffer(objectData.data(), objectData.size());
        const bool readBlocks = reader.readDwgBlocks(interface, &objectBuffer);
        REQUIRE(readBlocks);
        REQUIRE(interface.callbacks == expectedOrder);
        REQUIRE(interface.blocks.size() == 1u);
        REQUIRE(interface.inserts.size() == 1u);
        CHECK(interface.inserts.front().handle == insertHandle);
        CHECK(interface.inserts.front().parentHandle == owner);
        REQUIRE(interface.inserts.front().attlist.size() == 1u);
        CHECK(interface.inserts.front().attlist.front()->handle == attribHandle);
        REQUIRE(interface.publications.size() == 5u);
        CHECK(reader.ObjectMap.empty());
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 5u);
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
            CHECK(entry.m_publicationCount == 1u);
        }
    };

    SECTION("named block") {
        runScope("NESTED_INSERT_BLOCK", recordHandle,
                 {"block", "insert", "endBlock"});
    }

    SECTION("model space") {
        runScope("*Model_Space", DRW::NoHandle,
                 {"block", "endBlock", "insert"});
    }
}

TEST_CASE("DWG mapped wrong-owner ATTRIB fails at the entity boundary",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t recordHandle = 0x761;
    constexpr std::uint32_t insertHandle = 0x762;
    constexpr std::uint32_t attribHandle = 0x763;
    constexpr std::uint32_t seqEndHandle = 0x764;
    constexpr std::uint32_t wrongOwner = 0x765;
    auto insertFrame = makeInsertWithChildrenFrame(
        insertHandle, recordHandle, attribHandle, seqEndHandle);
    auto attribFrame = makeAttribFrame(attribHandle, wrongOwner);
    auto seqEndFrame = makeSeqEndFrame(seqEndHandle, insertHandle);
    REQUIRE(!insertFrame.empty());
    REQUIRE(!attribFrame.empty());
    REQUIRE(!seqEndFrame.empty());

    std::vector<std::uint8_t> objectData = insertFrame;
    const auto attribOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), attribFrame.cbegin(), attribFrame.cend());
    const auto seqEndOffset = static_cast<std::uint32_t>(objectData.size());
    objectData.insert(objectData.end(), seqEndFrame.cbegin(), seqEndFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(insertHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(attribHandle - insertHandle));
    REQUIRE(handleEntries.putModularChar(attribOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - attribHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(seqEndOffset) - attribOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        objectData.data(), objectData.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), objectData.size()));

    DRW_Block_Record record;
    DwgBlockOwnershipTestAccess::setHandle(record, recordHandle);
    record.name = "WRONG_ATTRIB_OWNER";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {attribHandle, seqEndHandle, insertHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(objectData.data(), objectData.size());
    REQUIRE_FALSE(reader.walkBlockRecordEntities(
        &record, &objectBuffer, interface, recordHandle, recordHandle));
    CHECK(interface.inserts.empty());
    CHECK(interface.publications.empty());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.m_invalidInsertOwners.count(insertHandle) == 1u);
    CHECK(reader.m_invalidInsertOwners.count(wrongOwner) == 1u);
    CHECK(reader.m_invalidSeqEndHandles.count(seqEndHandle) == 1u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
        CHECK(entry.m_publicationCount == 0u);
    }
}

TEST_CASE("DWG staged INSERT helpers commit exact parent and child frames",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0, 1, 1, 1, 1});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    REQUIRE(reader.ObjectMap.size() == 3u);
    reader.setVersionForTest(DRW::AC1018);
    DwgInsertReceiptProbe interface;

    DRW_Insert insert;
    insert.handle = 1;
    dwgHandle attributeHandle;
    attributeHandle.ref = 2;
    insert.attribHandles.push_back(attributeHandle);
    insert.seqendH.ref = 3;
    bool committed = false;
    REQUIRE(reader.stageInsertForTest(
        std::move(insert), reader.publicationForTest(1), interface, committed));
    CHECK_FALSE(committed);
    CHECK(reader.stagedPendingInsertCountForTest() == 1u);
    CHECK(reader.ObjectMap.size() == 2u);

    auto attribute = std::make_shared<DRW_Attrib>();
    attribute->handle = 2;
    attribute->parentHandle = 1;
    REQUIRE(reader.stageAttributeForTest(
        std::move(attribute), reader.publicationForTest(2), interface,
        committed));
    CHECK_FALSE(committed);
    CHECK(reader.ObjectMap.size() == 1u);

    REQUIRE(reader.stageSeqEndForTest(
        3, 1, reader.publicationForTest(3), interface, committed));
    CHECK(committed);
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.ObjectMap.empty());
    REQUIRE(interface.inserts.size() == 1u);
    REQUIRE(interface.inserts.front().attlist.size() == 1u);
    CHECK(interface.inserts.front().attlist.front()->handle == 2u);
    REQUIRE(interface.publications.size() == 3u);
    CHECK(interface.publications[0].m_handle == 1u);
    CHECK(interface.publications[1].m_handle == 2u);
    CHECK(interface.publications[2].m_handle == 3u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
        CHECK(entry.m_publicationCount == 1u);
    }
    CHECK(reader.validateDeferredCompoundState());
}

TEST_CASE("DWG staged INSERT helpers adopt orphan ATTRIB frames",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0, 1, 1, 1, 1});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    DwgInsertReceiptProbe interface;
    bool committed = false;

    auto attribute = std::make_shared<DRW_Attrib>();
    attribute->handle = 2;
    attribute->parentHandle = 1;
    REQUIRE(reader.stageAttributeForTest(
        std::move(attribute), reader.publicationForTest(2), interface,
        committed));
    CHECK_FALSE(committed);
    CHECK(reader.stagedOrphanAttribCountForTest() == 1u);

    DRW_Insert insert;
    insert.handle = 1;
    dwgHandle attributeHandle;
    attributeHandle.ref = 2;
    insert.attribHandles.push_back(attributeHandle);
    insert.seqendH.ref = 3;
    REQUIRE(reader.stageInsertForTest(
        std::move(insert), reader.publicationForTest(1), interface, committed));
    CHECK_FALSE(committed);
    CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
    CHECK(reader.stagedPendingInsertCountForTest() == 1u);

    REQUIRE(reader.stageSeqEndForTest(
        3, 1, reader.publicationForTest(3), interface, committed));
    CHECK(committed);
    REQUIRE(interface.inserts.size() == 1u);
    REQUIRE(interface.inserts.front().attlist.size() == 1u);
    REQUIRE(interface.publications.size() == 3u);
    CHECK(interface.publications[0].m_handle == 1u);
    CHECK(interface.publications[1].m_handle == 2u);
    CHECK(interface.publications[2].m_handle == 3u);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.validateDeferredCompoundState());
}

TEST_CASE("DWG staged INSERT helper terminalizes rejected orphan ATTRIBs",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0, 1, 1, 1, 1});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    DwgInsertReceiptProbe interface;
    bool committed = false;

    for (const std::uint32_t handle : {2u, 3u}) {
        auto attribute = std::make_shared<DRW_Attrib>();
        attribute->handle = handle;
        attribute->parentHandle = 1;
        REQUIRE(reader.stageAttributeForTest(
            std::move(attribute), reader.publicationForTest(handle), interface,
            committed));
    }
    CHECK_FALSE(committed);
    CHECK(reader.stagedOrphanAttribCountForTest() == 1u);

    DRW_Insert insert;
    insert.handle = 1;
    dwgHandle attributeHandle;
    attributeHandle.ref = 2;
    insert.attribHandles.push_back(attributeHandle);
    CHECK_FALSE(reader.stageInsertForTest(
        std::move(insert), reader.publicationForTest(1), interface, committed));
    CHECK_FALSE(committed);
    CHECK(interface.inserts.empty());
    CHECK(interface.publications.empty());
    CHECK(reader.stagedOrphanAttribCountForTest() == 0u);
    CHECK(reader.m_invalidInsertOwners.find(1u)
          != reader.m_invalidInsertOwners.end());
    CHECK(reader.ObjectMap.size() == 1u);
    CHECK(reader.ObjectMap.find(1u) != reader.ObjectMap.end());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
          == DRW_DwgFrameDisposition::Pending);
    for (std::size_t index = 1; index < reader.m_dwgSourceFrameLedger.size();
         ++index) {
        CHECK(reader.m_dwgSourceFrameLedger[index].m_disposition
              == DRW_DwgFrameDisposition::Quarantined);
        CHECK(reader.m_dwgSourceFrameLedger[index].m_reason
              == DRW_DwgFrameCoverageReason::Quarantined);
        CHECK(reader.m_dwgSourceFrameLedger[index].m_publicationCount == 0u);
    }
}

TEST_CASE("DWG staged INSERT helpers terminalize a throwing callback",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0, 1, 1, 1, 1});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    DwgThrowingInsertReceiptProbe interface;
    bool committed = false;

    DRW_Insert insert;
    insert.handle = 1;
    dwgHandle attributeHandle;
    attributeHandle.ref = 2;
    insert.attribHandles.push_back(attributeHandle);
    insert.seqendH.ref = 3;
    REQUIRE(reader.stageInsertForTest(
        std::move(insert), reader.publicationForTest(1), interface, committed));

    auto attribute = std::make_shared<DRW_Attrib>();
    attribute->handle = 2;
    attribute->parentHandle = 1;
    REQUIRE(reader.stageAttributeForTest(
        std::move(attribute), reader.publicationForTest(2), interface,
        committed));
    CHECK_FALSE(reader.stageSeqEndForTest(
        3, 1, reader.publicationForTest(3), interface, committed));
    CHECK_FALSE(committed);
    CHECK(interface.inserts.empty());
    CHECK(interface.publications.empty());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Failed);
        CHECK(entry.m_reason == DRW_DwgFrameCoverageReason::CallbackException);
        CHECK(entry.m_publicationCount == 0u);
    }
}

TEST_CASE("DWG staged INSERT helpers terminalize a throwing receipt",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0, 1, 1, 1, 1});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    DwgThrowingFrameReceiptProbe interface;
    bool committed = false;

    DRW_Insert insert;
    insert.handle = 1;
    dwgHandle attributeHandle;
    attributeHandle.ref = 2;
    insert.attribHandles.push_back(attributeHandle);
    insert.seqendH.ref = 3;
    REQUIRE(reader.stageInsertForTest(
        std::move(insert), reader.publicationForTest(1), interface, committed));

    auto attribute = std::make_shared<DRW_Attrib>();
    attribute->handle = 2;
    attribute->parentHandle = 1;
    REQUIRE(reader.stageAttributeForTest(
        std::move(attribute), reader.publicationForTest(2), interface,
        committed));
    CHECK_FALSE(reader.stageSeqEndForTest(
        3, 1, reader.publicationForTest(3), interface, committed));
    CHECK_FALSE(committed);
    REQUIRE(interface.inserts.size() == 1u);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_reason
          == DRW_DwgFrameCoverageReason::CallbackException);
    for (std::size_t index = 1; index < reader.m_dwgSourceFrameLedger.size();
         ++index) {
        CHECK(reader.m_dwgSourceFrameLedger[index].m_disposition
              == DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger[index].m_reason
              == DRW_DwgFrameCoverageReason::ReceiptFailure);
        CHECK(reader.m_dwgSourceFrameLedger[index].m_publicationCount == 0u);
    }
}

TEST_CASE("DWG staged POLYLINE helpers terminalize a throwing callback",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.vertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "THROWING_POLYLINE_CALLBACK";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {polylineHandle, vertexHandle, seqEndHandle});
    DwgThrowingPolylineProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 0u);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Failed);
        CHECK(entry.m_reason == DRW_DwgFrameCoverageReason::CallbackException);
        CHECK(entry.m_publicationCount == 0u);
    }
}

TEST_CASE("DWG staged POLYLINE helpers terminalize a throwing receipt",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.vertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "THROWING_POLYLINE_RECEIPT";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {polylineHandle, vertexHandle, seqEndHandle});
    DwgThrowingFrameReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());
    CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 1u);
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_reason
          == DRW_DwgFrameCoverageReason::CallbackException);
    for (std::size_t index = 1; index < reader.m_dwgSourceFrameLedger.size();
         ++index) {
        CHECK(reader.m_dwgSourceFrameLedger[index].m_disposition
              == DRW_DwgFrameDisposition::Failed);
        CHECK(reader.m_dwgSourceFrameLedger[index].m_reason
              == DRW_DwgFrameCoverageReason::ReceiptFailure);
        CHECK(reader.m_dwgSourceFrameLedger[index].m_publicationCount == 0u);
    }
}

TEST_CASE("DWG staged POLYLINE stops at the failing frame receipt",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());

    for (const std::size_t throwOn : {2u, 3u}) {
        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(polylineHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
        REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
        REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(fixture.seqEndOffset)
            - fixture.vertexOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(DRW::AC1018);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

        DRW_Block_Record record;
        record.name = "THROWING_LATER_POLYLINE_RECEIPT";
        DwgBlockOwnershipTestAccess::setHandles(
            record, 0, 0, {polylineHandle, vertexHandle, seqEndHandle});
        DwgNthThrowingFrameReceiptProbe interface;
        interface.throwOn = throwOn;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(
            &record, &objectBuffer, interface));
        CHECK(interface.polylineCount == 1u);
        REQUIRE(interface.publications.size() == throwOn - 1u);
        for (std::size_t index = 0; index < interface.publications.size();
             ++index) {
            CHECK(interface.publications[index].m_handle
                  == std::array<std::uint32_t, 3>{
                      polylineHandle, vertexHandle, seqEndHandle}[index]);
        }
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (std::size_t index = 0; index < reader.m_dwgSourceFrameLedger.size();
             ++index) {
            const DRW_DwgFrameCoverageEntry& entry =
                reader.m_dwgSourceFrameLedger[index];
            if (index < throwOn - 1u) {
                CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Published);
                CHECK(entry.m_publicationCount == 1u);
            } else {
                CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Failed);
                CHECK(entry.m_reason
                      == (index == throwOn - 1u
                          ? DRW_DwgFrameCoverageReason::CallbackException
                          : DRW_DwgFrameCoverageReason::ReceiptFailure));
                CHECK(entry.m_publicationCount == 0u);
            }
        }
    }
}

TEST_CASE("DWG staged POLYLINE preparation failures roll back markers",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());
    const std::array<std::uint8_t, 3> failurePoints = {0, 1, 2};

    for (const auto point : failurePoints) {
        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(polylineHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
        REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
        REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(fixture.seqEndOffset)
            - fixture.vertexOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(DRW::AC1018);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));
        REQUIRE(reader.failPolylinePreparationForTest(point));

        DRW_Block_Record record;
        record.name = "POLYLINE_PREPARE_FAILURE";
        DwgBlockOwnershipTestAccess::setHandles(
            record, 0, 0, {polylineHandle, vertexHandle, seqEndHandle});
        DwgInsertReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(
            &record, &objectBuffer, interface));
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        CHECK(reader.consumedPolylineChildCountForTest() == 0u);
        CHECK(reader.consumedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK((entry.m_disposition == DRW_DwgFrameDisposition::Failed
                   || entry.m_disposition
                       == DRW_DwgFrameDisposition::Quarantined));
            CHECK(entry.m_publicationCount == 0u);
        }
    }
}

TEST_CASE("DWG staged POLYLINE entry failures clean child-first state",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());

    for (const std::uint8_t point : {0, 1, 2, 3}) {
        dwgBufferW handleEntries;
        REQUIRE(handleEntries.putUModularChar(polylineHandle));
        REQUIRE(handleEntries.putModularChar(0));
        REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
        REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
        REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
        REQUIRE(handleEntries.putModularChar(
            static_cast<std::int64_t>(fixture.seqEndOffset)
            - fixture.vertexOffset));
        std::vector<std::uint8_t> handleMap;
        appendHandleMapPage(handleMap, handleEntries.data());
        appendHandleMapPage(handleMap, {});

        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(DRW::AC1018);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));
        REQUIRE(reader.failPolylineStagingForTest(point));

        DRW_Block_Record record;
        record.name = "POLYLINE_STAGE_FAILURE";
        DwgBlockOwnershipTestAccess::setHandles(
            record, 0, 0, {vertexHandle, polylineHandle, seqEndHandle});
        DwgInsertReceiptProbe interface;
        dwgBuffer objectBuffer(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size());

        CHECK_FALSE(reader.walkBlockRecordEntities(
            &record, &objectBuffer, interface));
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        CHECK(reader.consumedPolylineChildCountForTest() == 0u);
        CHECK(reader.consumedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK((entry.m_disposition == DRW_DwgFrameDisposition::Failed
                   || entry.m_disposition
                       == DRW_DwgFrameDisposition::Quarantined));
            CHECK(entry.m_publicationCount == 0u);
        }
    }
}

TEST_CASE("DWG POLYLINE stage failures leave no EOF-replayable source",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t polylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t seqEndHandle = 0x102;
    const auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(polylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(vertexHandle - polylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(seqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.vertexOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    for (const std::uint8_t point : {0, 1, 2}) {
        DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
            const_cast<std::uint8_t*>(fixture.bytes.data()),
            fixture.bytes.size()));
        reader.setVersionForTest(DRW::AC1018);
        dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
        REQUIRE(reader.readDwgHandles(
            &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));
        REQUIRE(reader.failPolylineStagingForTest(point));

        DwgInsertReceiptProbe interface;
        CHECK_FALSE(reader.readDwgEntities(interface));
        CHECK(interface.polylineCount == 0u);
        CHECK(interface.publications.empty());
        CHECK(reader.ObjectMap.empty());
        CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
        CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
        CHECK(reader.stagedSeqEndCountForTest() == 0u);
        CHECK(reader.consumedPolylineChildCountForTest() == 0u);
        CHECK(reader.consumedSeqEndCountForTest() == 0u);
        REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
        for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
            CHECK((entry.m_disposition == DRW_DwgFrameDisposition::Failed
                   || entry.m_disposition
                       == DRW_DwgFrameDisposition::Quarantined));
            CHECK(entry.m_publicationCount == 0u);
        }
    }
}

TEST_CASE("DWG staged POLYLINE rejects a VERTEX claimed by two pending parents",
          "[dwg][safety][compound]") {
    constexpr std::uint32_t firstPolylineHandle = 0x100;
    constexpr std::uint32_t vertexHandle = 0x101;
    constexpr std::uint32_t firstSeqEndHandle = 0x102;
    constexpr std::uint32_t secondPolylineHandle = 0x103;
    constexpr std::uint32_t secondSeqEndHandle = 0x104;
    auto fixture = makePolylineFrameFixture(false);
    REQUIRE(!fixture.bytes.empty());

    DwgPolylineWriterProbe secondPolyline;
    secondPolyline.handle = secondPolylineHandle;
    auto declaredVertex = std::make_shared<DwgVertexWriterProbe>();
    declaredVertex->handle = vertexHandle;
    secondPolyline.appendVertex(declaredVertex);
    secondPolyline.setDwgSeqEndHandle(secondSeqEndHandle);
    dwgBufferW secondPolylineBody;
    REQUIRE(secondPolyline.encodeDwg(
        DRW::AC1018, &secondPolylineBody, 0, nullptr, nullptr));
    const auto secondPolylineFrame = makeEntityFrame(secondPolylineBody);
    REQUIRE(!secondPolylineFrame.empty());

    DwgSequenceWriterProbe secondSeqEnd;
    secondSeqEnd.handle = secondSeqEndHandle;
    secondSeqEnd.parentHandle = secondPolylineHandle;
    dwgBufferW secondSeqEndBody;
    REQUIRE(secondSeqEnd.encodeDwg(
        DRW::AC1018, &secondSeqEndBody, 0, nullptr, nullptr));
    const auto secondSeqEndFrame = makeEntityFrame(secondSeqEndBody);
    REQUIRE(!secondSeqEndFrame.empty());

    const std::uint32_t secondPolylineOffset =
        static_cast<std::uint32_t>(fixture.bytes.size());
    const std::uint32_t secondSeqEndOffset = secondPolylineOffset
        + static_cast<std::uint32_t>(secondPolylineFrame.size());
    fixture.bytes.insert(fixture.bytes.end(), secondPolylineFrame.cbegin(),
                         secondPolylineFrame.cend());
    fixture.bytes.insert(fixture.bytes.end(), secondSeqEndFrame.cbegin(),
                         secondSeqEndFrame.cend());

    dwgBufferW handleEntries;
    REQUIRE(handleEntries.putUModularChar(firstPolylineHandle));
    REQUIRE(handleEntries.putModularChar(0));
    REQUIRE(handleEntries.putUModularChar(
        vertexHandle - firstPolylineHandle));
    REQUIRE(handleEntries.putModularChar(fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(
        firstSeqEndHandle - vertexHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(fixture.seqEndOffset)
        - fixture.vertexOffset));
    REQUIRE(handleEntries.putUModularChar(
        secondPolylineHandle - firstSeqEndHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(secondPolylineOffset)
        - fixture.seqEndOffset));
    REQUIRE(handleEntries.putUModularChar(
        secondSeqEndHandle - secondPolylineHandle));
    REQUIRE(handleEntries.putModularChar(
        static_cast<std::int64_t>(secondSeqEndOffset)
        - secondPolylineOffset));
    std::vector<std::uint8_t> handleMap;
    appendHandleMapPage(handleMap, handleEntries.data());
    appendHandleMapPage(handleMap, {});

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    dwgBuffer handleBuffer(handleMap.data(), handleMap.size());
    REQUIRE(reader.readDwgHandles(
        &handleBuffer, 0, handleMap.size(), fixture.bytes.size()));

    DRW_Block_Record record;
    record.name = "POLYLINE_TWO_PARENT_VERTEX";
    DwgBlockOwnershipTestAccess::setHandles(
        record, 0, 0, {firstPolylineHandle, secondPolylineHandle,
                        vertexHandle, firstSeqEndHandle, secondSeqEndHandle});
    DwgInsertReceiptProbe interface;
    dwgBuffer objectBuffer(
        const_cast<std::uint8_t*>(fixture.bytes.data()), fixture.bytes.size());

    CHECK_FALSE(reader.walkBlockRecordEntities(&record, &objectBuffer, interface));
    CHECK(interface.polylineCount == 0u);
    CHECK(interface.publications.empty());
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.stagedPendingPolylineCountForTest() == 0u);
    CHECK(reader.stagedOrphanPolylineVertexCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.consumedPolylineChildCountForTest() == 0u);
    CHECK(reader.consumedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 5u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK((entry.m_disposition == DRW_DwgFrameDisposition::Failed
               || entry.m_disposition == DRW_DwgFrameDisposition::Quarantined));
        CHECK(entry.m_publicationCount == 0u);
    }
}

TEST_CASE("DWG staged INSERT preparation rejects consumed child collisions",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0, 1, 1, 1, 1});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    DwgInsertReceiptProbe interface;
    bool committed = false;

    DRW_Insert insert;
    insert.handle = 1;
    dwgHandle attributeHandle;
    attributeHandle.ref = 2;
    insert.attribHandles.push_back(attributeHandle);
    insert.seqendH.ref = 3;
    REQUIRE(reader.stageInsertForTest(
        std::move(insert), reader.publicationForTest(1), interface, committed));

    auto attribute = std::make_shared<DRW_Attrib>();
    attribute->handle = 2;
    attribute->parentHandle = 1;
    REQUIRE(reader.stageAttributeForTest(
        std::move(attribute), reader.publicationForTest(2), interface,
        committed));
    REQUIRE(reader.m_consumedCompoundChildHandles.insert(2).second);

    CHECK_FALSE(reader.stageSeqEndForTest(
        3, 1, reader.publicationForTest(3), interface, committed));
    CHECK_FALSE(committed);
    CHECK(interface.inserts.empty());
    CHECK(interface.publications.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (const DRW_DwgFrameCoverageEntry& entry : reader.m_dwgSourceFrameLedger) {
        CHECK(entry.m_disposition == DRW_DwgFrameDisposition::Quarantined);
        CHECK(entry.m_reason == DRW_DwgFrameCoverageReason::Quarantined);
        CHECK(entry.m_publicationCount == 0u);
    }
}

TEST_CASE("DWG staged INSERT helper terminalizes an unexpected SEQEND",
          "[dwg][safety][compound]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0, 1, 1, 1, 1});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    reader.setVersionForTest(DRW::AC1018);
    DwgInsertReceiptProbe interface;
    bool committed = false;

    DRW_Insert insert;
    insert.handle = 1;
    dwgHandle attributeHandle;
    attributeHandle.ref = 2;
    insert.attribHandles.push_back(attributeHandle);
    insert.seqendH.ref = 4;
    REQUIRE(reader.stageInsertForTest(
        std::move(insert), reader.publicationForTest(1), interface, committed));

    auto attribute = std::make_shared<DRW_Attrib>();
    attribute->handle = 2;
    attribute->parentHandle = 1;
    REQUIRE(reader.stageAttributeForTest(
        std::move(attribute), reader.publicationForTest(2), interface,
        committed));
    CHECK_FALSE(reader.stageSeqEndForTest(
        3, 1, reader.publicationForTest(3), interface, committed));
    CHECK_FALSE(committed);
    CHECK(interface.inserts.empty());
    CHECK(interface.publications.empty());
    CHECK(reader.stagedPendingInsertCountForTest() == 0u);
    CHECK(reader.stagedSeqEndCountForTest() == 0u);
    CHECK(reader.ObjectMap.find(3u) != reader.ObjectMap.end());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 3u);
    for (std::size_t index = 0; index < 2; ++index) {
        CHECK(reader.m_dwgSourceFrameLedger[index].m_disposition
              == DRW_DwgFrameDisposition::Quarantined);
        CHECK(reader.m_dwgSourceFrameLedger[index].m_reason
              == DRW_DwgFrameCoverageReason::Quarantined);
    }
    CHECK(reader.m_dwgSourceFrameLedger[2].m_disposition
          == DRW_DwgFrameDisposition::Pending);
}

TEST_CASE("DWG source-frame leases reject dual parser-map residency",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    REQUIRE(reader.objObjectMap.emplace(object.handle, object).second);

    const auto sourceIt = reader.ObjectMap.find(object.handle);
    REQUIRE(sourceIt != reader.ObjectMap.end());
    DwgHandleReaderProbe::DwgSourceFrameLease lease;
    CHECK_FALSE(reader.takeDwgSourceFrame(reader.ObjectMap, sourceIt, lease));
    REQUIRE(reader.ObjectMap.size() == 1u);
    REQUIRE(reader.objObjectMap.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    REQUIRE(reader.m_integrityDiagnostics.size() == 1u);
    CHECK(reader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
}

TEST_CASE("DWG source-frame deferral rolls back a forged source",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DwgHandleReaderProbe::DwgSourceFrameLease lease;
    lease.object = object;
    lease.source = sourceFrameId(object);
    ++lease.source.offset;
    lease.hasCoverage = true;

    dwgReader::DwgObjectMap deferred;
    CHECK_FALSE(reader.deferDwgSourceFrame(lease, deferred));
    CHECK(deferred.empty());
    CHECK(reader.ObjectMap.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    REQUIRE(reader.m_integrityDiagnostics.size() == 1u);
    CHECK(reader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
}

TEST_CASE("DWG mapped source-frame deferral transfers one exact map entry",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DwgHandleReaderProbe::DwgSourceFrameLease lease;
    lease.object = object;
    lease.source = sourceFrameId(object);
    lease.hasCoverage = true;

    dwgReader::DwgObjectMap deferred;
    REQUIRE(reader.deferDwgSourceFrame(lease, deferred));
    CHECK(reader.ObjectMap.empty());
    REQUIRE(deferred.size() == 1u);
    CHECK(sourceFrameId(deferred.at(1)) == lease.source);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Deferred);
    CHECK(reader.m_integrityDiagnostics.empty());
}

TEST_CASE("DWG mapped source-frame deferral preserves source on target collision",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    DwgHandleReaderProbe::DwgSourceFrameLease lease;
    lease.object = object;
    lease.source = sourceFrameId(object);
    lease.hasCoverage = true;

    dwgReader::DwgObjectMap deferred;
    REQUIRE(deferred.emplace(object.handle, object).second);
    CHECK_FALSE(reader.deferDwgSourceFrame(lease, deferred));
    REQUIRE(reader.ObjectMap.size() == 1u);
    CHECK(sourceFrameId(reader.ObjectMap.at(1)) == lease.source);
    REQUIRE(deferred.size() == 1u);
    CHECK(sourceFrameId(deferred.at(1)) == lease.source);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    REQUIRE(reader.m_integrityDiagnostics.size() == 1u);
    CHECK(reader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
}

TEST_CASE("DWG mapped source-frame discard transitions before erasing",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    reader.m_quarantinedEntityHandles.insert(1);
    const auto sourceIt = reader.ObjectMap.find(1);
    REQUIRE(sourceIt != reader.ObjectMap.end());
    REQUIRE(reader.discardDwgSourceFrame(reader.ObjectMap, sourceIt));
    CHECK(reader.ObjectMap.empty());
    CHECK(reader.m_quarantinedEntityHandles.count(1) == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Quarantined);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0);
    CHECK(reader.m_integrityDiagnostics.empty());
}

TEST_CASE("DWG queued receipt suppression requires its exact source frame",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle object = reader.ObjectMap.at(1);
    const DwgSourceFrameId source = sourceFrameId(object);
    REQUIRE(reader.markDwgFrameOutcome(
        source, DRW_DwgFrameDisposition::Deferred));

    DRW_DwgFramePublication publication;
    publication.m_handle = source.handle;
    publication.m_sourceOffset = source.offset;
    publication.m_sourceMapOrdinal = source.ordinal;
    publication.m_sourceOffsetSpace = source.offsetSpace;
    publication.m_hasSourceLocation = true;
    REQUIRE(reader.suppressDwgFramePublication(publication));
    CHECK(reader.ObjectMap.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Quarantined);

    DwgHandleReaderProbe forgedReader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    dwgBuffer forgedMapBuffer(bytes.data(), bytes.size());
    REQUIRE(forgedReader.readDwgHandles(
        &forgedMapBuffer, 0, bytes.size()));
    REQUIRE(forgedReader.markDwgFrameOutcome(
        sourceFrameId(forgedReader.ObjectMap.at(1)),
        DRW_DwgFrameDisposition::Deferred));
    ++publication.m_sourceOffset;
    CHECK_FALSE(forgedReader.suppressDwgFramePublication(publication));
    CHECK(forgedReader.ObjectMap.size() == 1u);
    CHECK(forgedReader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Deferred);
    REQUIRE(forgedReader.m_integrityDiagnostics.size() == 1u);
    CHECK(forgedReader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
}

TEST_CASE("DWG quarantine rejects a mismatched source identity",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    DwgSourceFrameId source = sourceFrameId(reader.ObjectMap.at(1));
    ++source.ordinal;

    CHECK_FALSE(reader.quarantineDwgFrame(source));
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Pending);
    CHECK(reader.m_quarantinedEntityHandles.empty());
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    CHECK(reader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
}

TEST_CASE("DWG receipt callback failures preserve partial frame coverage",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());
    DwgThrowingFrameReceiptProbe receiver;

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    const objHandle& object = reader.ObjectMap.at(1);
    DRW_DwgFramePublication publication;
    publication.m_handle = object.handle;
    publication.m_sourceOffset = object.loc;
    publication.m_sourceMapOrdinal = object.sourceOrdinal;
    publication.m_sourceOffsetSpace = object.sourceOffsetSpace;
    publication.m_hasSourceLocation = true;

    CHECK_FALSE(reader.publishDwgFramePublication(receiver, publication));
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_reason
          == DRW_DwgFrameCoverageReason::CallbackException);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0);
}

TEST_CASE("DWG source-less entity callbacks do not fabricate receipts",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    DwgTableReceiptProbe receiver;
    DRW_DwgFramePublication publication;
    publication.m_handle = 0x2C1;
    publication.m_sourceOffset = 0;
    publication.m_hasSourceLocation = true;

    REQUIRE(reader.publishDwgFramePublication(receiver, publication));
    CHECK(receiver.publications.empty());
    CHECK(reader.m_integrityDiagnostics.empty());
}

TEST_CASE("DWG mapped simple entity remains staged until journal replay",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t entityHandle = 1;
    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.setObjectType(dwgType::LINE);
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    dwgBufferW body;
    REQUIRE(line.encodeDwgCommon(DRW::AC1018, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1018, &body));
    const auto frame = makeEntityFrame(body);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1018);
    auto map = makeHandleMap(false);
    dwgBuffer mapBuffer(map.data(), map.size());
    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, map.size()));

    dwgBuffer entityBuffer(
        const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgEntityReaderProbe::DwgBlockScopeTransaction transaction(reader);
    DwgEntityReaderProbe::DwgFrameMapLease lease;
    DwgJournalLineProbe receiver;
    REQUIRE(reader.journalMappedEntityForTest(
        &entityBuffer, entityHandle, receiver, transaction,
        transaction.output(), lease));
    REQUIRE(transaction.adopt(lease));
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Staged);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
    CHECK_FALSE(lease.isDetached());
    CHECK(transaction.sourceCount() == 1u);
    CHECK(transaction.output().size() == 2u);
    CHECK(receiver.lines.empty());
    CHECK(receiver.framePublications.empty());

    REQUIRE(transaction.replay(receiver));
    REQUIRE(receiver.lines.size() == 1u);
    CHECK(receiver.lines.front().handle == entityHandle);
    CHECK(receiver.lines.front().basePoint.x == line.basePoint.x);
    REQUIRE(receiver.framePublications.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Published);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 1u);
}

TEST_CASE("DWG block journal admission bounds every resource",
          "[dwg][safety][journal]") {
    std::uint8_t dummy[] = {0};
    DwgEntityReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    DwgEntityReaderProbe::DwgBlockScopeTransaction transaction(reader);

    CHECK_FALSE(transaction.reserveAdmission(
        static_cast<std::size_t>(dwgSafety::MaxOwnedObjectCount) + 3u,
        0u, 0u));
    CHECK_FALSE(transaction.reserveAdmission(
        0u, static_cast<std::size_t>(dwgSafety::MaxBlockJournalEventCount)
                + 1u,
        0u));
    CHECK_FALSE(transaction.reserveAdmission(
        0u, 0u, dwgSafety::MaxBufferSize + 1u));
    CHECK(transaction.sourceCount() == 0u);
    CHECK(transaction.output().empty());
}

TEST_CASE("DWG block journal quarantines a throwing mapped callback",
          "[dwg][safety][journal]") {
    constexpr std::uint32_t entityHandle = 1;
    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.setObjectType(dwgType::LINE);
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    dwgBufferW body;
    REQUIRE(line.encodeDwgCommon(DRW::AC1018, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1018, &body));
    const auto frame = makeEntityFrame(body);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1018);
    auto map = makeHandleMap(false);
    dwgBuffer mapBuffer(map.data(), map.size());
    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, map.size()));

    dwgBuffer entityBuffer(
        const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgEntityReaderProbe::DwgBlockScopeTransaction transaction(reader);
    DwgEntityReaderProbe::DwgFrameMapLease lease;
    DwgThrowingLineProbe receiver;
    REQUIRE(reader.journalMappedEntityForTest(
        &entityBuffer, entityHandle, receiver, transaction,
        transaction.output(), lease));
    REQUIRE(transaction.adopt(lease));
    CHECK_FALSE(transaction.replay(receiver));
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1u);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_reason
          == DRW_DwgFrameCoverageReason::CallbackException);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0u);
}

TEST_CASE("DWG entity callback failures mark the mapped frame",
          "[dwg][safety]") {
    constexpr std::uint32_t entityHandle = 1;
    DwgLineWriterProbe line;
    line.handle = entityHandle;
    line.setObjectType(dwgType::LINE);
    line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line.extPoint = DRW_Coord(0.0, 0.0, 1.0);
    dwgBufferW body;
    REQUIRE(line.encodeDwgCommon(DRW::AC1018, &body));
    body.putBit(1);
    body.putRawDouble(line.basePoint.x);
    body.putDefaultDouble(line.basePoint.x, line.secPoint.x);
    body.putRawDouble(line.basePoint.y);
    body.putDefaultDouble(line.basePoint.y, line.secPoint.y);
    body.putThickness(line.thickness, true);
    body.putExtrusion(line.extPoint, true);
    REQUIRE(line.encodeDwgEntHandle(DRW::AC1018, &body));
    const auto frame = makeEntityFrame(body);
    REQUIRE(!frame.empty());

    DwgEntityReaderProbe reader(std::make_unique<dwgBuffer>(
        const_cast<std::uint8_t*>(frame.data()), frame.size()));
    reader.setVersionForTest(DRW::AC1018);
    auto map = makeHandleMap(false);
    dwgBuffer mapBuffer(map.data(), map.size());
    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, map.size()));

    dwgBuffer entityBuffer(
        const_cast<std::uint8_t*>(frame.data()), frame.size());
    DwgThrowingLineProbe receiver;
    CHECK_FALSE(reader.readDwgEntity(
        &entityBuffer, reader.ObjectMap.at(entityHandle), receiver));
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_reason
          == DRW_DwgFrameCoverageReason::CallbackException);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0);
}

TEST_CASE("DWG table callback failures retain the table phase error",
          "[dwg][safety][fixture]") {
    const auto source = std::filesystem::path(LIBRECAD_TEST_DIR)
        / "ordinary_enc_AC1015.dwg";
    DwgThrowingLayerProbe receiver;
    dwgRW reader(source.string().c_str());

    CHECK_FALSE(reader.read(&receiver, /*ext=*/true));
    CHECK(reader.getError() == DRW::BAD_READ_TABLES);
    CHECK(receiver.publications.empty());
    REQUIRE(receiver.coverageReports.size() == 1);
    CHECK_FALSE(receiver.coverageReports.front().m_complete);
    CHECK(receiver.coverageReports.front().m_status
          == DRW_DwgFrameCoverageStatus::FinalizedPartial);
}

TEST_CASE("DWG table receipts follow accepted table callbacks",
          "[dwg][safety][fixture]") {
    const auto source = std::filesystem::path(LIBRECAD_TEST_DIR)
        / "ordinary_enc_AC1015.dwg";
    DwgTableReceiptProbe receiver;
    dwgRW reader(source.string().c_str());

    REQUIRE(reader.read(&receiver, /*ext=*/true));
    const auto layerReceipt = std::find_if(
        receiver.publications.cbegin(), receiver.publications.cend(),
        [](const DRW_DwgFramePublication& publication) {
            return !publication.m_isEntity
                && publication.m_recordName == "layer";
        });
    REQUIRE(layerReceipt != receiver.publications.cend());
    CHECK(receiver.layerDelivered);
    CHECK(receiver.layerReceiptAfterCallback);
    CHECK(layerReceipt->m_hasSourceLocation);
    CHECK(layerReceipt->m_carrier
          == DRW_DwgFramePublication::Carrier::Typed);
    CHECK(layerReceipt->m_encodedType == DRW::DwgLayerObjectType);
    CHECK(layerReceipt->m_resolvedType == DRW::DwgLayerObjectType);
    CHECK_FALSE(layerReceipt->m_commonHandleDataValidated);

    const std::array<std::pair<const char*, std::int16_t>, 9> controls{{
        {"LTYPE_CONTROL", DRW::DwgLTypeControlObjectType},
        {"LAYER_CONTROL", DRW::DwgLayerControlObjectType},
        {"STYLE_CONTROL", DRW::DwgStyleControlObjectType},
        {"DIMSTYLE_CONTROL", DRW::DwgDimStyleControlObjectType},
        {"VPORT_CONTROL", DRW::DwgVPortControlObjectType},
        {"BLOCK_CONTROL", DRW::DwgBlockControlObjectType},
        {"APPID_CONTROL", DRW::DwgAppIdControlObjectType},
        {"VIEW_CONTROL", DRW::DwgViewControlObjectType},
        {"UCS_CONTROL", DRW::DwgUcsControlObjectType},
    }};
    for (const auto& expected : controls) {
        const auto control = std::find_if(
            receiver.publications.cbegin(), receiver.publications.cend(),
            [&expected](const DRW_DwgFramePublication& publication) {
                return publication.m_recordName == expected.first;
            });
        INFO(expected.first);
        REQUIRE(control != receiver.publications.cend());
        CHECK(control->m_carrier
              == DRW_DwgFramePublication::Carrier::Control);
        CHECK(control->m_encodedType == expected.second);
        CHECK(control->m_resolvedType == expected.second);
        if (std::string_view(expected.first) == "VIEW_CONTROL"
            || std::string_view(expected.first) == "UCS_CONTROL") {
            CHECK(control->m_controlHandles.empty());
        } else {
            CHECK_FALSE(control->m_controlHandles.empty());
        }
    }

    std::vector<DRW_DwgFramePublication> tablePublications;
    std::copy_if(receiver.publications.cbegin(), receiver.publications.cend(),
                 std::back_inserter(tablePublications),
                 [](const DRW_DwgFramePublication& publication) {
                     return publication.m_carrier
                             == DRW_DwgFramePublication::Carrier::Control
                         || (!publication.m_isEntity
                             && publication.m_carrier
                                    == DRW_DwgFramePublication::Carrier::Typed);
                 });
    CHECK(std::is_sorted(
        tablePublications.cbegin(), tablePublications.cend(),
        [](const DRW_DwgFramePublication& lhs,
           const DRW_DwgFramePublication& rhs) {
            return lhs.m_sourceMapOrdinal < rhs.m_sourceMapOrdinal;
        }));
    REQUIRE(receiver.coverageReports.size() == 1);
    CHECK(receiver.coverageReports.front().m_complete);
    CHECK(receiver.coverageReports.front().m_status
          == DRW_DwgFrameCoverageStatus::FinalizedComplete);
}

TEST_CASE("DWG deferred raw controls publish a source-bound receipt",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());
    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1015;
    raw.m_handle = 1;
    raw.m_objectType = DRW_ViewportEntityHeader::kDwgControlType;
    raw.m_recordName = "VPORT_ENTITY_HEADER";
    raw.m_rawBytes = {0xA5};
    std::vector<DRW_UnsupportedObject> objects;
    objects.push_back(raw);
    DwgTableReceiptProbe receiver;

    REQUIRE(reader.publishDeferredRawObjects(receiver, objects));
    CHECK(objects.empty());
    REQUIRE(receiver.unsupportedObjects.size() == 1);
    REQUIRE(receiver.publications.size() == 1);
    const DRW_DwgFramePublication& publication =
        receiver.publications.front();
    CHECK(publication.m_handle == raw.m_handle);
    CHECK(publication.m_recordName == raw.m_recordName);
    CHECK(publication.m_carrier
          == DRW_DwgFramePublication::Carrier::Raw);
    CHECK(publication.m_hasSourceLocation);
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Published);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 1);
}

TEST_CASE("DWG deferred raw callback failures mark the mapped frame",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());
    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));

    DRW_UnsupportedObject raw;
    raw.m_handle = 1;
    raw.m_objectType = DRW_ViewportEntityHeader::kDwgControlType;
    std::vector<DRW_UnsupportedObject> objects;
    objects.push_back(raw);
    DwgThrowingRawObjectProbe receiver;

    CHECK_FALSE(reader.publishDeferredRawObjects(receiver, objects));
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_reason
          == DRW_DwgFrameCoverageReason::CallbackException);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_publicationCount == 0);
}

TEST_CASE("DWG finalization absorbs coverage callback failures",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());
    DwgThrowingCoverageProbe receiver;

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    REQUIRE(reader.markDwgFrameOutcome(
        sourceFrameId(reader.ObjectMap.at(1)),
        DRW_DwgFrameDisposition::Published));

    reader.finalizeDwgFrameCoverageNoThrow(receiver, true);

    CHECK(reader.m_dwgFrameCoverageStatus
          == DRW_DwgFrameCoverageStatus::FinalizedPartial);
    CHECK(reader.m_dwgFrameCoveragePublished);
    CHECK(reader.m_dwgFrameCoverageIntegrityViolation);
}

TEST_CASE("DWG frame ledger preserves ordinals across handle-map groups",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0});
    appendHandleMapPage(bytes, {2, 1});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 2);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_handle == 1);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_sourceOffset == 0);
    CHECK(reader.m_dwgSourceFrameLedger[0].m_sourceMapOrdinal == 0);
    CHECK(reader.m_dwgSourceFrameLedger[1].m_handle == 2);
    CHECK(reader.m_dwgSourceFrameLedger[1].m_sourceOffset == 1);
    CHECK(reader.m_dwgSourceFrameLedger[1].m_sourceMapOrdinal == 1);
}

TEST_CASE("DWG quarantine keeps failed frames terminal and idempotent",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    REQUIRE(reader.markDwgFrameOutcome(
        sourceFrameId(reader.ObjectMap.at(1)),
        DRW_DwgFrameDisposition::Failed));

    CHECK(reader.quarantineDwgFrame(1));
    CHECK(reader.quarantineDwgFrame(1));

    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Failed);
    CHECK(reader.m_quarantinedEntityHandles.count(1) == 1);
    CHECK(reader.m_integrityDiagnostics.empty());
}

TEST_CASE("DWG quarantine rejects missing and published source frames",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    REQUIRE(reader.markDwgFrameOutcome(
        sourceFrameId(reader.ObjectMap.at(1)),
        DRW_DwgFrameDisposition::Published));

    CHECK_FALSE(reader.quarantineDwgFrame(1));
    CHECK_FALSE(reader.quarantineDwgFrame(2));

    REQUIRE(reader.m_dwgSourceFrameLedger.size() == 1);
    CHECK(reader.m_dwgSourceFrameLedger.front().m_disposition
          == DRW_DwgFrameDisposition::Published);
    CHECK(reader.m_quarantinedEntityHandles.empty());
    REQUIRE(reader.m_integrityDiagnostics.size() == 2);
    CHECK(reader.m_integrityDiagnostics[0].kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
    CHECK(reader.m_integrityDiagnostics[1].kind
          == DwgIntegrityCheckKind::FrameLedgerTransition);
}

TEST_CASE("DWG handle map rejects duplicate offsets in one group",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0, 1, 0});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    CHECK(reader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::ObjectMapDuplicateOffset);
}

TEST_CASE("DWG handle map rejects duplicate offsets across groups",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 0});
    appendHandleMapPage(bytes, {2, 0});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    CHECK(reader.ObjectMap.empty());
    REQUIRE(reader.m_integrityDiagnostics.size() == 1);
    CHECK(reader.m_integrityDiagnostics.front().kind
          == DwgIntegrityCheckKind::ObjectMapDuplicateOffset);
}

TEST_CASE("DWG handle map accepts the R2010 empty-page trailer",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    REQUIRE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    REQUIRE(reader.ObjectMap.size() == 1);
    CHECK(reader.ObjectMap.at(1).loc == 0);
}

TEST_CASE("DWG handle map rejects an invalid empty-page trailer CRC",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    appendHandleMapPage(bytes, {});
    ++bytes.back();
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG handle map rejects offsets outside the object buffer",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {1, 2});
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgHandles(&mapBuffer, 0, bytes.size(), 2));
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG handle map rejects bytes after its terminator", "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    bytes.push_back(0xAA);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG handle map does not read a CRC past its section",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    auto bytes = makeHandleMap(false);
    REQUIRE(bytes.size() > 1);
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgHandles(&mapBuffer, 0, bytes.size() - 1));
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG handle map rejects a terminator-only section", "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes, {});
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG handle map rejects oversized groups before decoding",
          "[dwg][safety]") {
    std::uint8_t dummy[] = {0};
    DwgHandleReaderProbe reader(
        std::make_unique<dwgBuffer>(dummy, sizeof(dummy)));
    std::vector<std::uint8_t> bytes;
    appendHandleMapPage(bytes,
                        std::vector<std::uint8_t>(2051u - 2u, 0u));
    dwgBuffer mapBuffer(bytes.data(), bytes.size());

    CHECK_FALSE(reader.readDwgHandles(&mapBuffer, 0, bytes.size()));
    CHECK(reader.ObjectMap.empty());
}

TEST_CASE("DWG EED uses exact pre-R2007 string length", "[dwg][safety]") {
    // The string length excludes a NUL. The following control item must not
    // be consumed as an alleged terminator.
    auto bytes = makeEed(
        {0, 1, 0, 30, 'A', 2, 0});
    dwgBuffer buffer(bytes.data(), bytes.size());
    std::vector<DwgEedChunk> chunks;

    REQUIRE(readDwgEed(DRW::AC1018, buffer, chunks));
    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks.front().items.size() == 2);
    CHECK(chunks.front().items[0].code() == 1000);
    CHECK(std::string(chunks.front().items[0].c_str()) == "A");
    CHECK(chunks.front().items[1].code() == 1002);
    CHECK(std::string(chunks.front().items[1].c_str()) == "{");
}

TEST_CASE("DWG EED rejects truncated and unknown items transactionally", "[dwg][safety]") {
    DwgEedChunk prior;
    prior.appHandle = 7;
    std::vector<DwgEedChunk> chunks{prior};

    auto truncated = makeEed({4, 5, 0xAA});
    dwgBuffer truncatedBuffer(truncated.data(), truncated.size());
    CHECK_FALSE(readDwgEed(DRW::AC1018, truncatedBuffer, chunks));
    CHECK(chunks.size() == 1);
    CHECK(chunks.front().appHandle == 7);

    auto unknown = makeEed({1});
    dwgBuffer unknownBuffer(unknown.data(), unknown.size());
    CHECK_FALSE(readDwgEed(DRW::AC1018, unknownBuffer, chunks));
    CHECK(chunks.size() == 1);
}

TEST_CASE("DWG R2007 EED decodes surrogate pairs", "[dwg][safety]") {
    auto bytes = makeEed(
        {0, 2, 0x00, 0x3D, 0xD8, 0x00, 0xDE});
    dwgBuffer buffer(bytes.data(), bytes.size());
    std::vector<DwgEedChunk> chunks;

    REQUIRE(readDwgEed(DRW::AC1021, buffer, chunks));
    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks.front().items.size() == 1);
    CHECK(std::string(chunks.front().items.front().c_str()) == "\xF0\x9F\x98\x80");
}

TEST_CASE("DWG EED preserves wide layer references", "[dwg][safety]") {
    auto bytes = makeEed(
        {3, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01});
    dwgBuffer buffer(bytes.data(), bytes.size());
    std::vector<DwgEedChunk> chunks;

    REQUIRE(readDwgEed(DRW::AC1032, buffer, chunks));
    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks.front().items.size() == 1);
    CHECK(chunks.front().items.front().code() == 1003);
    CHECK(chunks.front().items.front().isLayerRef());
    CHECK(std::string(chunks.front().items.front().c_str())
          == "102030405060708");
    CHECK(chunks.front().items.front().canReplayDwgRawLayerReference(
        DRW::AC1032));
    CHECK(chunks.front().items.front().rawDwgLayerReference()
          == 0x0102030405060708ULL);
    DRW_Variant copied(chunks.front().items.front());
    CHECK(copied.canReplayDwgRawLayerReference(DRW::AC1032));
    CHECK(copied.rawDwgLayerReference() == 0x0102030405060708ULL);
    CHECK(chunks.front().layerRefs.empty());
}

TEST_CASE("DWG EED rejects excessive repeated chunks", "[dwg][safety][slow]") {
    dwgBufferW writer;
    dwgHandle appHandle;
    appHandle.code = 0;
    appHandle.ref = 0x2A;
    const std::uint8_t item[] = {2, 0}; // EED control item: open group.
    for (std::uint32_t i = 0; i <= dwgSafety::MaxEedChunks; ++i) {
        writer.putBitShort(static_cast<std::uint16_t>(sizeof(item)));
        writer.putHandle(appHandle);
        writer.putBytes(item, sizeof(item));
    }
    writer.putBitShort(0);

    DwgEedChunk prior;
    prior.appHandle = 7;
    std::vector<DwgEedChunk> chunks{prior};
    auto bytes = writer.data();
    dwgBuffer buffer(bytes.data(), bytes.size());

    CHECK_FALSE(readDwgEed(DRW::AC1018, buffer, chunks));
    REQUIRE(chunks.size() == 1);
    CHECK(chunks.front().appHandle == 7);
}

TEST_CASE("DWG EED rejects excessive aggregate items",
          "[dwg][safety][slow]") {
    constexpr std::uint32_t itemsPerChunk = 32767;
    dwgBufferW writer;
    dwgHandle appHandle;
    appHandle.code = 0;
    appHandle.ref = 0x2A;
    std::uint32_t itemCount = 0;
    while (itemCount <= dwgSafety::MaxEedTotalItems) {
        const std::uint32_t remaining = dwgSafety::MaxEedTotalItems + 1
            - itemCount;
        const std::uint32_t chunkItems = std::min(itemsPerChunk, remaining);
        std::vector<std::uint8_t> chunkData(chunkItems * 2);
        for (std::uint32_t i = 0; i < chunkItems; ++i) {
            chunkData[2 * i] = 2;
            chunkData[2 * i + 1] = 0;
        }
        writer.putBitShort(static_cast<std::uint16_t>(chunkData.size()));
        writer.putHandle(appHandle);
        writer.putBytes(chunkData.data(), chunkData.size());
        itemCount += chunkItems;
    }
    writer.putBitShort(0);

    DwgEedChunk prior;
    prior.appHandle = 7;
    std::vector<DwgEedChunk> chunks{prior};
    auto bytes = writer.data();
    dwgBuffer buffer(bytes.data(), bytes.size());

    CHECK_FALSE(readDwgEed(DRW::AC1018, buffer, chunks));
    REQUIRE(chunks.size() == 1);
    CHECK(chunks.front().appHandle == 7);
}

TEST_CASE("DWG handle width and offset arithmetic are checked", "[dwg][safety]") {
    std::uint8_t wideBytes[] = {0x05, 1, 2, 3, 4, 5};
    dwgBuffer wide(wideBytes, sizeof(wideBytes));
    const dwgHandle wideHandle = wide.getHandle();
    CHECK(wideHandle.size == 5);
    CHECK(wideHandle.ref == 0x02030405u);
    CHECK(wideHandle.ref64 == 0x0102030405ULL);
    CHECK(wide.isGood());

    dwgBufferW wideWriter;
    wideWriter.putHandle(wideHandle);
    CHECK(wideWriter.data() == std::vector<std::uint8_t>{
        0x05, 0x01, 0x02, 0x03, 0x04, 0x05});

    std::uint8_t wideOffsetBytes[] = {0xA5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    dwgBuffer wideOffset(wideOffsetBytes, sizeof(wideOffsetBytes));
    const dwgHandle wideOffsetHandle = wideOffset.getOffsetHandle(1);
    CHECK(wideOffsetHandle.ref64 == 0x10000000000ULL);
    CHECK(wideOffsetHandle.ref == 0);
    CHECK(wideOffset.isGood());

    std::uint8_t oversizedBytes[] = {0x09, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    dwgBuffer oversized(oversizedBytes, sizeof(oversizedBytes));
    oversized.getHandle();
    CHECK_FALSE(oversized.isGood());

    dwgBufferW wideOffsetWriter;
    wideOffsetWriter.putHandle(wideOffsetHandle);
    CHECK(wideOffsetWriter.data() == std::vector<std::uint8_t>{
        0x76, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00});

    std::uint8_t underflowBytes[] = {0xC1, 0x10};
    dwgBuffer underflow(underflowBytes, sizeof(underflowBytes));
    underflow.getOffsetHandle(0x0F);
    CHECK_FALSE(underflow.isGood());

    std::uint8_t overflowBytes[] = {0xA1, 0x01};
    dwgBuffer overflow(overflowBytes, sizeof(overflowBytes));
    overflow.getOffsetHandle(std::numeric_limits<std::uint64_t>::max());
    CHECK_FALSE(overflow.isGood());
}

TEST_CASE("DWG code E handle references resolve to the containing object",
          "[dwg][safety]") {
    std::uint8_t bytes[] = {0xE1, 0xFF};
    dwgBuffer buffer(bytes, sizeof(bytes));
    const dwgHandle resolved = buffer.getOffsetHandle(0x12345678);

    CHECK(buffer.isGood());
    CHECK(resolved.code == 7);
    CHECK(resolved.ref64 == 0x12345678ULL);
    CHECK(resolved.ref == 0x12345678U);
}

TEST_CASE("DWG handle lists publish only after every handle validates", "[dwg][safety]") {
    std::uint8_t bytes[] = {0x00};
    dwgBuffer buffer(bytes, sizeof(bytes));
    std::vector<std::uint32_t> refs{0x42};

    CHECK_FALSE(readDwgHandleList(buffer, 0, 2, false, &refs));
    REQUIRE(refs.size() == 1);
    CHECK(refs.front() == 0x42);
    CHECK_FALSE(buffer.isGood());

    std::uint8_t countBytes[] = {0x00};
    dwgBuffer countBuffer(countBytes, sizeof(countBytes));
    CHECK_FALSE(readDwgHandleList(countBuffer, 0, -1, false));
    CHECK_FALSE(countBuffer.isGood());

    dwgBuffer excessiveCount(countBytes, sizeof(countBytes));
    CHECK_FALSE(readDwgHandleList(
        excessiveCount, 0,
        static_cast<std::int32_t>(dwgSafety::MaxReactorCount) + 1,
        false));
    CHECK_FALSE(excessiveCount.isGood());
}

TEST_CASE("DWG copied cursors share structural handle failures", "[dwg][safety]") {
    std::uint8_t bytes[] = {0xC1, 0x10};
    dwgBuffer source(bytes, sizeof(bytes));
    dwgBuffer fork = source;

    fork.getOffsetHandle(0x0F);
    CHECK_FALSE(fork.isGood());
    CHECK_FALSE(source.isGood());
}

TEST_CASE("DWG interleaved RS encoders round-trip through decoders", "[dwg][safety][rscodec]") {
    std::vector<std::uint8_t> systemData(239 * 3);
    for (std::size_t i = 0; i < systemData.size(); ++i)
        systemData[i] = static_cast<std::uint8_t>(i * 17u + 3u);
    std::vector<std::uint8_t> systemEncoded(255 * 3);
    std::vector<std::uint8_t> systemDecoded(systemData.size());
    REQUIRE(dwgRSCodec::encode239I(systemData.data(), systemEncoded.data(), 3));
    REQUIRE(dwgRSCodec::decode239I(
        systemEncoded.data(), systemDecoded.data(), 3));
    CHECK(systemDecoded == systemData);

    std::vector<std::uint8_t> dataPage(251 * 2);
    for (std::size_t i = 0; i < dataPage.size(); ++i)
        dataPage[i] = static_cast<std::uint8_t>(i * 29u + 11u);
    std::vector<std::uint8_t> dataPageEncoded(255 * 2);
    std::vector<std::uint8_t> dataPageDecoded(dataPage.size());
    REQUIRE(dwgRSCodec::encode251I(
        dataPage.data(), dataPageEncoded.data(), 2));
    REQUIRE(dwgRSCodec::decode251I(
        dataPageEncoded.data(), dataPageDecoded.data(), 2));
    CHECK(dataPageDecoded == dataPage);

    auto uncorrectable = dataPageEncoded;
    for (std::size_t symbol = 0; symbol < 10; ++symbol)
        uncorrectable[symbol * 2] ^= static_cast<std::uint8_t>(symbol + 1);
    CHECK_FALSE(dwgRSCodec::decode251I(
        uncorrectable.data(), dataPageDecoded.data(), 2));

    CHECK_FALSE(dwgRSCodec::encode239I(nullptr, systemEncoded.data(), 1));
    CHECK_FALSE(dwgRSCodec::encode251I(dataPage.data(), nullptr, 1));
}

TEST_CASE("DWG non-interleaved R2007 data pages decode by section encoding",
          "[dwg][safety][rscodec][r2007]") {
    constexpr std::size_t blockCount = 3;
    constexpr std::size_t logicalBlockSize = 251;
    constexpr std::size_t logicalSize = blockCount * logicalBlockSize - 1;
    constexpr std::size_t encodedSize = blockCount * 255;

    std::vector<std::uint8_t> input(logicalSize, 0);
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<std::uint8_t>(i * 31u + 7u);

    std::vector<std::uint8_t> rsInput(blockCount * logicalBlockSize, 0);
    std::copy(input.cbegin(), input.cend(), rsInput.begin());
    std::vector<std::uint8_t> encoded(encodedSize, 0);
    REQUIRE(dwgRSCodec::encode251(rsInput.data(), encoded.data(), blockCount));

    dwgSectionInfo section;
    section.size = input.size();
    section.maxSize = input.size();
    section.compressed = 1;
    section.pageCount = 1;
    dwgPageInfo pageInfo(1, 0, encoded.size());
    pageInfo.dataSize = input.size();
    pageInfo.startOffset = 0;
    pageInfo.uSize = input.size();
    pageInfo.cSize = input.size();
    section.pages.emplace(1, pageInfo);

    DwgDataPageReaderProbe reader(
        std::make_unique<dwgBuffer>(encoded.data(), encoded.size()));
    reader.setVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> output(input.size(), 0);
    REQUIRE(reader.parseDataPage(section, output.data()));
    CHECK(output == input);
}

TEST_CASE("DWG short R2007 data pages preserve zero-padded handles",
          "[dwg][safety][rscodec]") {
    const std::vector<std::uint8_t> handles = {
        0x00, 0x41, 0x01, 0xD2, 0x07, 0x01, 0xC0, 0x46, 0x01,
        0x36, 0x02, 0xC4, 0x41, 0x01, 0x85, 0x02, 0x01, 0x0F,
        0x01, 0x0F, 0x01, 0xC6, 0x01, 0x01, 0x31, 0x01, 0xB1,
        0x01, 0x04, 0xB5, 0x45, 0x01, 0x26, 0x01, 0x26, 0x01,
        0x3E, 0x01, 0x36, 0x01, 0xA5, 0x02, 0x01, 0x32, 0x01,
        0xF8, 0x41, 0x01, 0xAB, 0x04, 0x01, 0x37, 0x03, 0xBD,
        0x41, 0x01, 0x30, 0x01, 0x13, 0x01, 0x30, 0xA2, 0x10,
        0x94, 0x01
    };
    std::vector<std::uint8_t> input(251, 0);
    std::copy(handles.cbegin(), handles.cend(), input.begin());
    std::vector<std::uint8_t> encoded(255);
    std::vector<std::uint8_t> decoded(251);
    REQUIRE(dwgRSCodec::encode251I(input.data(), encoded.data(), 1));
    REQUIRE(dwgRSCodec::decode251I(encoded.data(), decoded.data(), 1));
    CHECK(std::equal(handles.cbegin(), handles.cend(), decoded.cbegin()));
}
