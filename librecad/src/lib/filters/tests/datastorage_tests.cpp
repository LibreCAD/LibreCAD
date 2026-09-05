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
 * PR-2a: AcDb:AcDsPrototype_1b typed DataStorage index tests.
 *
 * - short-read / truncated section
 * - absurd segmentIndexEntryCount hardMax clamp
 * - dynblock_point.dwg fixture: records.size() > 0
 */

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <QCoreApplication>
#include <QCryptographicHash>

#include "drw_datastorage.h"
#include "drw_header.h"
#include "drw_interface.h"
#include "drw_objects.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgreader15.h"
#include "lc_dwgadvancedmetadata.h"
#include "libdwgr.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"

class DrwDataStorageTestAccess {
public:
  static void setDataStorageFlag(DRW_Entity &entity, bool value) {
    entity.hasDsData = value ? 1u : 0u;
  }
};

namespace {

void ensureQtContext() {
  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp =
      QCoreApplication::instance()
          ? QCoreApplication::instance()
          : new QCoreApplication(qargc, qargv);
  (void)qapp;
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)settingsReady;
}

std::string sha256Hex(const std::vector<std::uint8_t>& data) {
  const QByteArray bytes(reinterpret_cast<const char*>(data.data()),
                         static_cast<qsizetype>(data.size()));
  return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256)
      .toHex()
      .toStdString();
}

class DwgDataStorageReaderProbe final : public dwgReader15 {
public:
  using dwgReader::linkDataStorage;
  using dwgReader::finalizeDataStorageLinks;

  DwgDataStorageReaderProbe()
      : dwgReader15(std::make_unique<dwgBuffer>(nullptr, 0), nullptr) {}

  void setVersionForTest(DRW::Version value) { version = value; }
};

class StubInterface : public DRW_Interface {
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
  void addHatch(const DRW_Hatch *) override {}
  void addViewport(const DRW_Viewport &) override {}
  void addImage(const DRW_Image *) override {}
  void addWipeout(const DRW_Wipeout *) override {}
  void addMLeader(const DRW_MLeader *) override {}
  void addMLeaderStyle(const DRW_MLeaderStyle *) override {}
  void addModelerGeometry(const DRW_ModelerGeometry &geometry) override {
    m_modelers.push_back(geometry);
  }
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

  std::vector<DRW_DataStorageSection> m_storages;
  std::vector<DRW_RawDwgSection> m_rawSections;
  std::vector<DRW_ModelerGeometry> m_modelers;
  std::vector<DRW_UnsupportedObject> m_rawObjects;
  std::vector<DRW::ETYPE> m_surfaces;
  std::vector<DRW_Surface> m_surfaceValues;
  std::size_t m_surfacesWithRawPayload = 0;
  std::vector<std::uint32_t> m_surfaceDataStorageHandles;
  void addUnsupportedObject(const DRW_UnsupportedObject &object) override {
    m_rawObjects.push_back(object);
  }
  void addSurface(const DRW_Surface *surface) override {
    if (surface != nullptr) {
      m_surfaces.push_back(surface->eType);
      m_surfaceValues.push_back(*surface);
      if (!surface->rawAcisData.empty())
        ++m_surfacesWithRawPayload;
      if (surface->hasDataStorageRecord)
        m_surfaceDataStorageHandles.push_back(surface->dataStorageHandle);
    }
  }
  void addDataStorage(const DRW_DataStorageSection &s) override {
    m_storages.push_back(s);
  }
  void addRawDwgSection(const DRW_RawDwgSection &s) override {
    m_rawSections.push_back(s);
  }
};

void writeU32(std::vector<std::uint8_t> &buf, std::size_t off, std::uint32_t v) {
  if (off + 4 > buf.size())
    buf.resize(off + 4);
  buf[off] = static_cast<std::uint8_t>(v & 0xff);
  buf[off + 1] = static_cast<std::uint8_t>((v >> 8) & 0xff);
  buf[off + 2] = static_cast<std::uint8_t>((v >> 16) & 0xff);
  buf[off + 3] = static_cast<std::uint8_t>((v >> 24) & 0xff);
}

void writeU16(std::vector<std::uint8_t> &buf, std::size_t off, std::uint16_t v) {
  if (off + 2 > buf.size())
    buf.resize(off + 2);
  buf[off] = static_cast<std::uint8_t>(v & 0xffu);
  buf[off + 1] = static_cast<std::uint8_t>((v >> 8) & 0xffu);
}

void writeI32(std::vector<std::uint8_t> &buf, std::size_t off, std::int32_t v) {
  writeU32(buf, off, static_cast<std::uint32_t>(v));
}

void writeU64(std::vector<std::uint8_t> &buf, std::size_t off,
              std::uint64_t v) {
  if (off + 8 > buf.size())
    buf.resize(off + 8);
  for (unsigned i = 0; i < 8; ++i)
    buf[off + i] = static_cast<std::uint8_t>(v >> (8u * i));
}

void writeName(std::vector<std::uint8_t> &buf, std::size_t off,
               const char *name) {
  for (std::size_t i = 0; i < 6 && name[i] != '\0'; ++i)
    buf[off + i] = static_cast<std::uint8_t>(name[i]);
}

DRW_RawDwgSection makeDataStorageReplaySection(std::uint32_t handle) {
  using namespace DRW_DataStorageConst;
  constexpr std::uint32_t segmentCount = 3;
  constexpr std::uint32_t dataIndexHeaderSize = 8;
  constexpr char payload[] = "ACIS BinaryFile";
  constexpr std::uint32_t payloadSize = sizeof(payload) - 1u;
  constexpr std::uint32_t segmentIndexOffset = HEADER_SIZE;
  constexpr std::uint32_t segmentIndexSize =
      SEGMENT_HEADER_SIZE + segmentCount * SEGMENT_INDEX_ENTRY_SIZE;
  constexpr std::uint32_t dataIndexOffset =
      segmentIndexOffset + segmentIndexSize;
  constexpr std::uint32_t dataIndexSize =
      SEGMENT_HEADER_SIZE + dataIndexHeaderSize + DATA_INDEX_ENTRY_SIZE;
  constexpr std::uint32_t dataSegmentOffset = dataIndexOffset + dataIndexSize;
  constexpr std::uint32_t dataRegionOffset =
      (SEGMENT_HEADER_SIZE + DATA_RECORD_HEADER_SIZE + 15u) & ~15u;
  constexpr std::uint32_t dataSegmentSize =
      dataRegionOffset + sizeof(std::uint32_t) + payloadSize;
  constexpr std::uint32_t fileSize = dataSegmentOffset + dataSegmentSize;

  std::vector<std::uint8_t> data(fileSize, 0);
  writeU32(data, 0, 0x44535731u);
  writeI32(data, 4, HEADER_SIZE);
  writeU32(data, 12, 1u);
  writeU32(data, 20, 1u);
  writeU32(data, 24, segmentIndexOffset);
  writeU32(data, 32, segmentCount);
  writeI32(data, 36, -1);
  writeI32(data, 40, 1);
  writeI32(data, 44, -1);
  writeI32(data, 48, -1);
  writeU32(data, 52, fileSize);

  const auto putSegmentHeader = [&](std::uint32_t offset,
                                    std::int32_t index,
                                    const char *name,
                                    std::uint32_t size) {
    writeU16(data, offset, SEGMENT_SIGNATURE);
    writeName(data, offset + 2, name);
    writeI32(data, offset + 8, index);
    writeU32(data, offset + 16, size);
    writeU32(data, offset + 24, 1u);
  };
  putSegmentHeader(segmentIndexOffset, 0, "segidx", segmentIndexSize);
  putSegmentHeader(dataIndexOffset, 1, "datidx", dataIndexSize);
  putSegmentHeader(dataSegmentOffset, 2, "_data_", dataSegmentSize);

  const std::uint32_t segmentEntriesOffset =
      segmentIndexOffset + SEGMENT_HEADER_SIZE;
  const auto putSegmentEntry = [&](std::uint32_t entry,
                                   std::uint32_t offset,
                                   std::uint32_t size) {
    const std::uint32_t entryOffset =
        segmentEntriesOffset + entry * SEGMENT_INDEX_ENTRY_SIZE;
    writeU64(data, entryOffset, offset);
    writeU32(data, entryOffset + 8, size);
  };
  putSegmentEntry(0, segmentIndexOffset, segmentIndexSize);
  putSegmentEntry(1, dataIndexOffset, dataIndexSize);
  putSegmentEntry(2, dataSegmentOffset, dataSegmentSize);

  const std::uint32_t dataIndexPayload = dataIndexOffset + SEGMENT_HEADER_SIZE;
  writeU32(data, dataIndexPayload, 1u);
  const std::uint32_t dataIndexEntry = dataIndexPayload + dataIndexHeaderSize;
  writeU32(data, dataIndexEntry, 2u);
  writeU32(data, dataIndexEntry + 4, 0u);
  writeU32(data, dataIndexEntry + 8, 7u);

  const std::uint32_t recordHeader = dataSegmentOffset + SEGMENT_HEADER_SIZE;
  writeU32(data, recordHeader, DATA_RECORD_HEADER_SIZE);
  writeU64(data, recordHeader + 8, handle);
  writeU32(data, recordHeader + 16, 0u);
  const std::uint32_t recordData = dataSegmentOffset + dataRegionOffset;
  writeU32(data, recordData, payloadSize);
  for (std::uint32_t i = 0; i < payloadSize; ++i)
    data[recordData + 4u + i] = static_cast<std::uint8_t>(payload[i]);

  DRW_RawDwgSection section;
  section.m_name = "AcDb:AcDsPrototype_1b";
  section.m_version = DRW::AC1027;
  section.m_data = std::move(data);
  return section;
}

DRW_RawDwgSection makeDataStorageSchemaSection() {
  using namespace DRW_DataStorageConst;
  constexpr std::uint32_t segmentCount = 3u;
  constexpr std::uint32_t segmentSize = 128u;
  constexpr std::uint32_t schemaDataSize = 192u;
  constexpr std::uint32_t segmentIndexOffset = HEADER_SIZE;
  constexpr std::uint32_t schemaIndexOffset =
      segmentIndexOffset + segmentSize;
  constexpr std::uint32_t schemaDataOffset =
      schemaIndexOffset + segmentSize;
  constexpr std::uint32_t fileSize = schemaDataOffset + schemaDataSize;
  constexpr std::uint32_t schemaPayloadOffset = SEGMENT_HEADER_SIZE;
  constexpr std::uint32_t schemaLocalOffset = 16u;
  constexpr std::uint32_t namesLocalOffset = 128u;

  std::vector<std::uint8_t> data(fileSize, 0);
  writeU32(data, 0, 0x44535731u);
  writeI32(data, 4, HEADER_SIZE);
  writeU32(data, 12, 1u);
  writeU32(data, 20, 1u);
  writeU32(data, 24, segmentIndexOffset);
  writeU32(data, 32, segmentCount);
  writeI32(data, 36, 1);
  writeI32(data, 40, -1);
  writeI32(data, 44, -1);
  writeI32(data, 48, -1);
  writeU32(data, 52, fileSize);

  const auto putSegmentHeader = [&](std::uint32_t offset,
                                    std::int32_t index,
                                    const char *name,
                                    std::uint32_t size,
                                    std::uint32_t systemDataAlignment) {
    writeU16(data, offset, SEGMENT_SIGNATURE);
    writeName(data, offset + 2u, name);
    writeI32(data, offset + 8u, index);
    writeU32(data, offset + 16u, size);
    writeU32(data, offset + 24u, 1u);
    writeU32(data, offset + 32u, systemDataAlignment);
  };
  putSegmentHeader(segmentIndexOffset, 0, "segidx", segmentSize, 0u);
  putSegmentHeader(schemaIndexOffset, 1, "schidx", segmentSize, 0u);
  putSegmentHeader(schemaDataOffset, 2, "schdat", schemaDataSize, 8u);

  const std::uint32_t segmentEntriesOffset =
      segmentIndexOffset + SEGMENT_HEADER_SIZE;
  const auto putSegmentEntry = [&](std::uint32_t entry,
                                   std::uint32_t offset,
                                   std::uint32_t size) {
    const std::uint32_t entryOffset =
        segmentEntriesOffset + entry * SEGMENT_INDEX_ENTRY_SIZE;
    writeU64(data, entryOffset, offset);
    writeU32(data, entryOffset + 8u, size);
  };
  putSegmentEntry(0u, segmentIndexOffset, segmentSize);
  putSegmentEntry(1u, schemaIndexOffset, segmentSize);
  putSegmentEntry(2u, schemaDataOffset, schemaDataSize);

  const std::uint32_t schemaIndexPayload =
      schemaIndexOffset + schemaPayloadOffset;
  writeU32(data, schemaIndexPayload, 1u);
  writeU32(data, schemaIndexPayload + 4u, 0u);
  writeU32(data, schemaIndexPayload + 8u, 7u);
  writeU32(data, schemaIndexPayload + 12u, 2u);
  writeU32(data, schemaIndexPayload + 16u, schemaLocalOffset);
  writeU64(data, schemaIndexPayload + 20u, 0x0af10cu);
  writeU32(data, schemaIndexPayload + 28u, 2u);
  writeU32(data, schemaIndexPayload + 32u, 0u);
  writeU32(data, schemaIndexPayload + 36u, 2u);
  writeU32(data, schemaIndexPayload + 40u, 0u);
  writeU32(data, schemaIndexPayload + 44u, 0u);
  writeU32(data, schemaIndexPayload + 48u, 2u);
  writeU32(data, schemaIndexPayload + 52u, 8u);
  writeU32(data, schemaIndexPayload + 56u, 1u);

  const std::uint32_t schemaDataPayload =
      schemaDataOffset + schemaPayloadOffset;
  writeU32(data, schemaDataPayload, 8u);
  writeU32(data, schemaDataPayload + 4u, 1u);
  writeU32(data, schemaDataPayload + 8u, 8u);
  writeU32(data, schemaDataPayload + 12u, 0u);
  const std::uint32_t schema = schemaDataPayload + schemaLocalOffset;
  writeU16(data, schema, 1u);
  writeU64(data, schema + 2u, 0x123456789abcdef0ull);
  writeU16(data, schema + 10u, 1u);
  const std::uint32_t property = schema + 12u;
  writeU32(data, property, 0u);
  writeU32(data, property + 4u, 0u);
  writeU32(data, property + 8u, 0xeu);
  writeU32(data, property + 12u, 2u);
  writeU16(data, property + 16u, 2u);
  data[property + 18u] = 0xaau;
  data[property + 19u] = 0xbbu;
  data[property + 20u] = 0xccu;
  data[property + 21u] = 0xddu;

  const std::uint32_t names = schemaDataOffset + namesLocalOffset;
  writeU32(data, names, 1u);
  writeName(data, names + 4u, "TestPr");
  data[names + 10u] = 'o';
  data[names + 11u] = 'p';
  data[names + 12u] = 'e';
  data[names + 13u] = 'r';
  data[names + 14u] = 't';
  data[names + 15u] = 'y';

  DRW_RawDwgSection section;
  section.m_name = "AcDb:AcDsPrototype_1b";
  section.m_version = DRW::AC1027;
  section.m_data = std::move(data);
  return section;
}

DRW_RawDwgSection makeDataStorageBlobReferenceSection(
    std::uint32_t handle, const std::vector<std::uint32_t> &pageSizes) {
  using namespace DRW_DataStorageConst;
  constexpr std::uint32_t dataIndexHeaderSize = 8u;
  const std::vector<std::uint32_t> pages = pageSizes.empty()
      ? std::vector<std::uint32_t>{0x50000u} : pageSizes;
  const std::uint32_t pageCount =
      static_cast<std::uint32_t>(pages.size());
  const std::uint32_t segmentCount = 3u + pageCount;
  std::uint64_t totalByteLength = 0;
  for (std::uint32_t pageSize : pages)
    totalByteLength += pageSize;
  constexpr std::uint32_t segmentIndexOffset = HEADER_SIZE;
  const std::uint32_t segmentIndexSize = SEGMENT_HEADER_SIZE
      + segmentCount * SEGMENT_INDEX_ENTRY_SIZE;
  const std::uint32_t dataIndexOffset = segmentIndexOffset
      + segmentIndexSize;
  const std::uint32_t dataIndexSize = SEGMENT_HEADER_SIZE
      + dataIndexHeaderSize + DATA_INDEX_ENTRY_SIZE;
  const std::uint32_t dataSegmentOffset = dataIndexOffset + dataIndexSize;
  const std::size_t dataRegionOffset =
      ((dataSegmentOffset + SEGMENT_HEADER_SIZE + DATA_RECORD_HEADER_SIZE
        + 15u) & ~15u) - dataSegmentOffset;
  const std::size_t recordData = dataSegmentOffset + dataRegionOffset;
  const std::uint32_t descriptorSize =
      DATA_BLOB_REFERENCE_FIXED_SIZE + pageCount * DATA_BLOB_PAGE_ENTRY_SIZE;
  const std::uint32_t newDataSegmentSize = static_cast<std::uint32_t>(
      dataRegionOffset + 4u + descriptorSize);
  std::vector<std::uint32_t> blobSegmentOffsets;
  blobSegmentOffsets.reserve(pages.size());
  std::size_t nextBlobSegmentOffset = dataSegmentOffset
      + newDataSegmentSize;
  for (std::uint32_t pageSize : pages) {
    blobSegmentOffsets.push_back(
        static_cast<std::uint32_t>(nextBlobSegmentOffset));
    nextBlobSegmentOffset += SEGMENT_HEADER_SIZE
        + DATA_BLOB_PAGE_HEADER_SIZE + pageSize;
  }
  const std::uint32_t fileSize =
      static_cast<std::uint32_t>(nextBlobSegmentOffset);
  std::vector<std::uint8_t> data(fileSize, 0);
  writeU32(data, 0, 0x44535731u);
  writeI32(data, 4, HEADER_SIZE);
  writeU32(data, 12, 1u);
  writeU32(data, 20, 1u);
  writeU32(data, 24, segmentIndexOffset);
  writeU32(data, 32, segmentCount);
  writeI32(data, 36, -1);
  writeI32(data, 40, 1);
  writeI32(data, 44, -1);
  writeI32(data, 48, -1);
  writeU32(data, 52, fileSize);

  const auto putSegmentHeader = [&](std::uint32_t offset,
                                    std::int32_t index,
                                    const char *name,
                                    std::uint32_t size) {
    writeU16(data, offset, SEGMENT_SIGNATURE);
    writeName(data, offset + 2u, name);
    writeI32(data, offset + 8u, index);
    writeU32(data, offset + 16u, size);
    writeU32(data, offset + 24u, 1u);
  };
  putSegmentHeader(segmentIndexOffset, 0, "segidx", segmentIndexSize);
  putSegmentHeader(dataIndexOffset, 1, "datidx", dataIndexSize);
  putSegmentHeader(dataSegmentOffset, 2, "_data_", newDataSegmentSize);
  for (std::uint32_t page = 0; page < pageCount; ++page) {
    const std::uint32_t segmentSize = SEGMENT_HEADER_SIZE
        + DATA_BLOB_PAGE_HEADER_SIZE + pages[page];
    putSegmentHeader(blobSegmentOffsets[page],
                     static_cast<std::int32_t>(3u + page), "blob01",
                     segmentSize);
  }

  const std::uint32_t segmentEntriesOffset = segmentIndexOffset
      + SEGMENT_HEADER_SIZE;
  const auto putSegmentEntry = [&](std::uint32_t entry,
                                   std::uint32_t offset,
                                   std::uint32_t size) {
    const std::uint32_t entryOffset = segmentEntriesOffset
        + entry * SEGMENT_INDEX_ENTRY_SIZE;
    writeU64(data, entryOffset, offset);
    writeU32(data, entryOffset + 8u, size);
  };
  putSegmentEntry(0, segmentIndexOffset, segmentIndexSize);
  putSegmentEntry(1, dataIndexOffset, dataIndexSize);
  putSegmentEntry(2, dataSegmentOffset, newDataSegmentSize);
  for (std::uint32_t page = 0; page < pageCount; ++page) {
    const std::uint32_t segmentSize = SEGMENT_HEADER_SIZE
        + DATA_BLOB_PAGE_HEADER_SIZE + pages[page];
    putSegmentEntry(3u + page, blobSegmentOffsets[page], segmentSize);
  }

  const std::uint32_t dataIndexPayload = dataIndexOffset + SEGMENT_HEADER_SIZE;
  writeU32(data, dataIndexPayload, 1u);
  const std::uint32_t dataIndexEntry = dataIndexPayload + dataIndexHeaderSize;
  writeU32(data, dataIndexEntry, 2u);
  writeU32(data, dataIndexEntry + 4u, 0u);
  writeU32(data, dataIndexEntry + 8u, 7u);

  const std::uint32_t recordHeader = dataSegmentOffset + SEGMENT_HEADER_SIZE;
  writeU32(data, recordHeader, DATA_RECORD_HEADER_SIZE);
  writeU64(data, recordHeader + 8u, handle);
  writeU32(data, recordHeader + 16u, 0u);
  writeU32(data, recordData, DATA_BLOB_REFERENCE_MARKER);
  writeU64(data, recordData + 4u, totalByteLength);
  writeU32(data, recordData + 12u, pageCount);
  writeU32(data, recordData + 16u, descriptorSize);
  writeU32(data, recordData + 20u, pages.front());
  writeU32(data, recordData + 24u, pages.back());
  writeU32(data, recordData + 28u, 0u);
  writeU32(data, recordData + 32u, 0u);
  std::uint64_t pageStartOffset = 0;
  for (std::uint32_t page = 0; page < pageCount; ++page) {
    const std::uint32_t pageSize = pages[page];
    const std::uint32_t pageSegmentIndex = 3u + page;
    writeU32(data, recordData + 4u + DATA_BLOB_REFERENCE_FIXED_SIZE
                 + page * DATA_BLOB_PAGE_ENTRY_SIZE,
             pageSegmentIndex);
    writeU32(data, recordData + 4u + DATA_BLOB_REFERENCE_FIXED_SIZE
                 + page * DATA_BLOB_PAGE_ENTRY_SIZE + 4u,
             pageSize);

    const std::uint32_t blobData =
        blobSegmentOffsets[page] + SEGMENT_HEADER_SIZE;
    writeU64(data, blobData, totalByteLength);
    writeU64(data, blobData + 8u, pageStartOffset);
    writeI32(data, blobData + 16u, static_cast<std::int32_t>(page));
    writeI32(data, blobData + 20u, static_cast<std::int32_t>(pageCount));
    writeU64(data, blobData + 24u, pageSize);
    for (std::uint32_t i = 0; i < pageSize; ++i)
      data[blobData + DATA_BLOB_PAGE_HEADER_SIZE + i] =
          static_cast<std::uint8_t>((page + i) & 0xffu);
    constexpr char marker[] = "ACIS BinaryFile";
    for (std::size_t i = 0;
         page == 0u && i + 1u < sizeof(marker) && i < pageSize; ++i)
      data[blobData + DATA_BLOB_PAGE_HEADER_SIZE + i] =
          static_cast<std::uint8_t>(marker[i]);
    pageStartOffset += pageSize;
  }

  DRW_RawDwgSection section;
  section.m_name = "AcDb:AcDsPrototype_1b";
  section.m_version = DRW::AC1027;
  section.m_data = std::move(data);
  return section;
}

DRW_RawDwgSection makeDataStorageBlobReferenceSection(std::uint32_t handle) {
  return makeDataStorageBlobReferenceSection(handle, {0x50000u});
}

DRW_UnsupportedObject makeDataStorageModeler(std::uint32_t objectHandle) {
  dwgBufferW body;
  body.putObjType(DRW::AC1027, 38);
  dwgHandle entityHandle;
  entityHandle.code = 0;
  entityHandle.ref = objectHandle;
  entityHandle.size = 0;
  body.putHandle(entityHandle);
  body.putBitShort(0); // EED size
  body.putBit(0);       // graph flag
  body.put2Bits(2);     // model-space entity without owner
  body.putBitLong(0);   // reactor count
  body.putBit(0);       // no extension dictionary
  body.putBit(1);       // has_ds_data
  body.putEnColor(DRW::AC1027, 256);
  body.putBitDouble(1.0);
  body.put2Bits(0);     // BYLAYER linetype
  body.put2Bits(0);     // BYLAYER plot style
  body.put2Bits(0);     // BYLAYER material
  body.putRawChar8(0);  // inherited shadow mode
  body.putBit(0);       // no full visual style
  body.putBit(0);       // no face visual style
  body.putBit(0);       // no edge visual style
  body.putBitShort(0);  // visible
  body.putRawChar8(29); // BYLAYER lineweight
  body.putBit(1);       // empty modeler payload
  body.putBit(0);       // modeler-data unknown bit

  const std::uint32_t handleStartBit = body.bitCount();
  dwgHandle nullHandle;
  nullHandle.code = 0;
  nullHandle.ref = 0;
  nullHandle.size = 0;
  body.putHandle(nullHandle); // extension dictionary
  body.putHandle(nullHandle); // layer

  DRW_UnsupportedObject object;
  object.m_version = DRW::AC1027;
  object.m_objectType = 38;
  object.m_handle = objectHandle;
  object.m_bodyBitSize = static_cast<std::uint32_t>(
      body.bitCount() - handleStartBit);
  object.m_objectSize = static_cast<std::uint32_t>(body.data().size());
  object.m_isEntity = true;
  object.m_hasDataStorage = true;
  object.m_rawBytes = body.data();
  return object;
}

DRW_UnsupportedObject makeDataStorageSurface(std::uint32_t objectHandle) {
  constexpr std::uint16_t objectType = 535; // PLANESURFACE, libreDWG/ODA
  dwgBufferW body;
  body.putObjType(DRW::AC1027, objectType);

  dwgHandle ownHandle;
  ownHandle.code = 0;
  ownHandle.ref = objectHandle;
  ownHandle.size = 0;
  body.putHandle(ownHandle);
  body.putBitShort(0); // EED size
  body.putBit(0);       // graph flag
  body.put2Bits(2);     // model-space entity without owner
  body.putBitLong(0);   // reactor count
  body.putBit(0);       // no extension dictionary
  body.putBit(1);       // has_ds_data
  body.putEnColor(DRW::AC1027, 256);
  body.putBitDouble(1.0);
  body.put2Bits(0);     // BYLAYER linetype
  body.put2Bits(0);     // BYLAYER plot style
  body.put2Bits(0);     // BYLAYER material
  body.putRawChar8(0);  // inherited shadow mode
  body.putBit(0);       // no full visual style
  body.putBit(0);       // no face visual style
  body.putBit(0);       // no edge visual style
  body.putBitShort(0);  // visible
  body.putRawChar8(29); // BYLAYER lineweight

  const std::uint32_t handleStartBit = body.bitCount();
  dwgHandle nullHandle;
  nullHandle.code = 0;
  nullHandle.ref = 0;
  nullHandle.size = 0;
  body.putHandle(nullHandle); // extension dictionary
  body.putHandle(nullHandle); // layer

  DRW_UnsupportedObject object;
  object.m_version = DRW::AC1027;
  object.m_objectType = objectType;
  object.m_handle = objectHandle;
  object.m_bodyBitSize = static_cast<std::uint32_t>(
      body.bitCount() - handleStartBit);
  object.m_objectSize = static_cast<std::uint32_t>(body.data().size());
  object.m_isEntity = true;
  object.m_isCustomClass = true;
  object.m_recordName = "PLANESURFACE";
  object.m_className = "AcDbPlaneSurface";
  object.m_hasDataStorage = true;
  object.m_rawBytes = body.data();
  return object;
}

class ModelerDataStorageReplayInterface final : public StubInterface {
public:
  ModelerDataStorageReplayInterface()
      : m_modeler(makeDataStorageModeler(0x961u)),
        m_section(makeDataStorageReplaySection(m_modeler.m_handle)) {}

  dwgRW *writer = nullptr;
  DRW_UnsupportedObject m_modeler;
  DRW_RawDwgSection m_section;
  bool expectReplay = true;

  void writeDwgClasses() override {
    if (writer != nullptr)
      REQUIRE(writer->registerRawDwgObjectClass(&m_modeler));
  }

  void writeObjects() override {
    if (writer == nullptr)
      return;
    if (expectReplay) {
      REQUIRE(writer->writeRawDwgObject(&m_modeler));
      REQUIRE(writer->writeRawDwgSection(&m_section));
    } else {
      CHECK_FALSE(writer->writeRawDwgObject(&m_modeler));
      CHECK_FALSE(writer->writeRawDwgSection(&m_section));
    }
  }
};

class SurfaceDataStorageReplayInterface final : public StubInterface {
public:
  SurfaceDataStorageReplayInterface()
      : m_surface(makeDataStorageSurface(0xA70u)),
        m_section(makeDataStorageReplaySection(m_surface.m_handle)) {}

  dwgRW *writer = nullptr;
  DRW_UnsupportedObject m_surface;
  DRW_RawDwgSection m_section;

  void writeDwgClasses() override {
    if (writer != nullptr)
      REQUIRE(writer->registerRawDwgObjectClass(&m_surface));
  }

  void writeObjects() override {
    if (writer == nullptr)
      return;
    REQUIRE(writer->writeRawDwgObject(&m_surface));
    REQUIRE(writer->writeRawDwgSection(&m_section));
  }
};

} // namespace

TEST_CASE("DataStorage short-read below header size",
          "[cross-read][datastorage][short-read]") {
  std::vector<std::uint8_t> tiny(16, 0);
  const DRW_DataStorageSection section =
      DRW_parseDataStorage(tiny.data(), tiny.size());
  REQUIRE(section.parseFailed);
  REQUIRE(section.records.empty());
  REQUIRE_FALSE(section.diagnostics.empty());
  bool found = false;
  for (const auto &d : section.diagnostics) {
    if (d.code == "datastorage-section-too-small")
      found = true;
  }
  REQUIRE(found);
}

TEST_CASE("DataStorage null pointer is short-read",
          "[cross-read][datastorage][short-read]") {
  const DRW_DataStorageSection section = DRW_parseDataStorage(nullptr, 0);
  REQUIRE(section.parseFailed);
  REQUIRE(section.records.empty());
}

TEST_CASE("DataStorage absurd segmentIndexEntryCount is capped",
          "[cross-read][datastorage][absurd-count]") {
  // Minimal 56-byte header + claim absurd segment index count.
  std::vector<std::uint8_t> buf(DRW_DataStorageConst::HEADER_SIZE, 0);
  // segmentIndexOffset = HEADER_SIZE (entries would start after a 48-byte
  // segidx segment header, which is outside this tiny buffer).
  writeU32(buf, 24, DRW_DataStorageConst::HEADER_SIZE);
  writeU32(buf, 32, 0xFFFFFFFFu); // absurd count
  writeI32(buf, 40, -1);          // no data index segment
  writeU32(buf, 52, static_cast<std::uint32_t>(buf.size()));

  const DRW_DataStorageSection section =
      DRW_parseDataStorage(buf.data(), buf.size());
  REQUIRE_FALSE(section.parseFailed);
  // Must not allocate millions of entries.
  REQUIRE(section.segments.size()
          <= DRW_DataStorageConst::HARD_MAX_ENTRIES);
  // Truncation / cap diagnostics expected when remaining space is small.
  bool cappedOrTruncated = false;
  for (const auto &d : section.diagnostics) {
    if (d.code == "datastorage-count-capped"
        || d.code == "datastorage-segment-index-truncated") {
      cappedOrTruncated = true;
    }
  }
  REQUIRE(cappedOrTruncated);
}

TEST_CASE("DataStorage decodes schema index and schema data",
          "[cross-read][datastorage][schema]") {
  const DRW_RawDwgSection raw = makeDataStorageSchemaSection();
  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);

  REQUIRE(parsed.schemaIndexEntries.size() == 1u);
  CHECK(parsed.schemaIndexEntries.front().index == 7u);
  CHECK(parsed.schemaIndexEntries.front().segmentIndex == 2u);
  CHECK(parsed.schemaIndexEntries.front().localOffset == 16u);
  REQUIRE(parsed.schemaPropertyEntries.size() == 2u);
  CHECK(parsed.schemaPropertyEntries.front().segmentIndex == 2u);
  CHECK(parsed.schemaPropertyEntries.front().localOffset == 0u);
  CHECK(parsed.schemaPropertyEntries.front().index == 0u);
  CHECK(parsed.schemaPropertyEntries[1].localOffset == 8u);
  CHECK(parsed.schemaPropertyEntries[1].index == 1u);

  REQUIRE(parsed.schemaUnknownProperties.size() == 2u);
  CHECK(parsed.schemaUnknownProperties.front().dataSize == 8u);
  CHECK(parsed.schemaUnknownProperties.front().flags == 1u);
  CHECK(parsed.schemaUnknownProperties.front().segmentIndex == 2u);
  CHECK(parsed.schemaUnknownProperties[1].index == 1u);
  CHECK(parsed.schemaUnknownProperties[1].localOffset == 8u);
  REQUIRE(parsed.schemas.size() == 1u);
  const DRW_DataStorageSchema &schema = parsed.schemas.front();
  CHECK(schema.index == 7u);
  REQUIRE(schema.indexes.size() == 1u);
  CHECK(schema.indexes.front() == 0x123456789abcdef0ull);
  REQUIRE(schema.properties.size() == 1u);
  const DRW_DataStorageSchemaProperty &property = schema.properties.front();
  CHECK(property.name == "TestProperty");
  CHECK(property.type == 0xeu);
  CHECK(property.customTypeSize == 2u);
  CHECK(property.typeSize == 2u);
  CHECK(property.valueCount == 2u);
  REQUIRE(property.values.size() == 2u);
  CHECK(property.values[0] == std::vector<std::uint8_t>{0xaau, 0xbbu});
  CHECK(property.values[1] == std::vector<std::uint8_t>{0xccu, 0xddu});
  CHECK(parsed.replayAllowed);
}

TEST_CASE("DataStorage rejects truncated schema property values",
          "[cross-read][datastorage][schema][malformed]") {
  DRW_RawDwgSection raw = makeDataStorageSchemaSection();
  using namespace DRW_DataStorageConst;
  constexpr std::uint32_t schemaDataOffset =
      HEADER_SIZE + 128u + 128u;
  constexpr std::uint32_t schemaPayload =
      schemaDataOffset + SEGMENT_HEADER_SIZE;
  constexpr std::uint32_t propertyValueCount = schemaPayload + 16u + 12u + 16u;
  writeU16(raw.m_data, propertyValueCount, 0xffffu);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  CHECK(parsed.schemas.empty());
  bool found = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    if (diagnostic.code == "datastorage-schema-property-values-truncated") {
      found = true;
      break;
    }
  }
  CHECK(found);
}

TEST_CASE("DataStorage rejects schema unknown-property references past schemas",
          "[cross-read][datastorage][schema][malformed]") {
  DRW_RawDwgSection raw = makeDataStorageSchemaSection();
  using namespace DRW_DataStorageConst;
  constexpr std::uint32_t schemaIndexOffset = HEADER_SIZE + 128u;
  constexpr std::uint32_t schemaIndexPayload =
      schemaIndexOffset + SEGMENT_HEADER_SIZE;

  // Both second-table references now point at the schema area, not at an
  // 8-byte unknown-property header preceding it.
  writeU32(raw.m_data, schemaIndexPayload + 40u, 16u);
  writeU32(raw.m_data, schemaIndexPayload + 52u, 24u);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  CHECK(parsed.schemaUnknownProperties.empty());
  CHECK(parsed.schemaUnknownPropertyCount == 0u);
  bool found = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    if (diagnostic.code ==
        "datastorage-schema-unknown-property-offset-invalid") {
      found = true;
      break;
    }
  }
  CHECK(found);
}

TEST_CASE("DataStorage segment index stays within its segment",
          "[cross-read][datastorage][bounds]") {
  DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  using namespace DRW_DataStorageConst;

  constexpr std::size_t segmentEntriesOffset =
      HEADER_SIZE + SEGMENT_HEADER_SIZE;

  // The first segidx entry self-describes a segment containing only that
  // entry. A section-wide reader would consume the following segment entries.
  writeU32(raw.m_data, segmentEntriesOffset + 8u,
           SEGMENT_HEADER_SIZE + SEGMENT_INDEX_ENTRY_SIZE);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE(parsed.segments.size() == 1u);
  REQUIRE(parsed.records.empty());

  bool found = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    if (diagnostic.code == "datastorage-count-capped") {
      found = true;
      break;
    }
  }
  CHECK(found);
}

TEST_CASE("DataStorage segment header stays within its declared segment",
          "[cross-read][datastorage][bounds]") {
  DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  using namespace DRW_DataStorageConst;

  constexpr std::size_t segmentEntriesOffset =
      HEADER_SIZE + SEGMENT_HEADER_SIZE;
  constexpr std::size_t dataSegmentEntry =
      segmentEntriesOffset + 2u * SEGMENT_INDEX_ENTRY_SIZE;

  // Leave the data segment's bytes present in the section but declare only a
  // partial header. The parser must not classify those bytes as a data
  // segment or use them to construct a record.
  writeU32(raw.m_data, dataSegmentEntry + 8u, SEGMENT_HEADER_SIZE - 1u);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE(parsed.segments.size() == 3u);
  REQUIRE(parsed.records.empty());

  bool found = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    if (diagnostic.code == "datastorage-segment-size-too-small") {
      found = true;
      break;
    }
  }
  CHECK(found);
}

TEST_CASE("DataStorage reports segment header size mismatches",
          "[cross-read][datastorage][bounds]") {
  DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  using namespace DRW_DataStorageConst;

  constexpr std::size_t dataSegmentOffset = HEADER_SIZE + SEGMENT_HEADER_SIZE
      + 3u * SEGMENT_INDEX_ENTRY_SIZE + SEGMENT_HEADER_SIZE + 8u
      + DATA_INDEX_ENTRY_SIZE;

  // Keep the indexed segment valid but make its embedded header metadata
  // disagree with the segment-index entry.
  writeU32(raw.m_data, dataSegmentOffset + 16u, SEGMENT_HEADER_SIZE);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE_FALSE(parsed.parseFailed);
  REQUIRE(parsed.records.size() == 1u);
  CHECK(parsed.replayAllowed);

  bool found = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    if (diagnostic.code == "datastorage-segment-header-size-mismatch") {
      found = true;
      break;
    }
  }
  CHECK(found);
}

TEST_CASE("DataStorage validates segment header identity",
          "[cross-read][datastorage][bounds]") {
  DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  using namespace DRW_DataStorageConst;

  constexpr std::size_t dataSegmentOffset = HEADER_SIZE + SEGMENT_HEADER_SIZE
      + 3u * SEGMENT_INDEX_ENTRY_SIZE + SEGMENT_HEADER_SIZE + 8u
      + DATA_INDEX_ENTRY_SIZE;
  writeU16(raw.m_data, dataSegmentOffset, 0u);
  writeI32(raw.m_data, dataSegmentOffset + 8u, 3);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE_FALSE(parsed.parseFailed);
  REQUIRE(parsed.records.empty());
  CHECK(parsed.replayAllowed);

  bool signatureFound = false;
  bool indexFound = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    signatureFound |=
        diagnostic.code == "datastorage-segment-signature-invalid";
    indexFound |= diagnostic.code == "datastorage-segment-index-mismatch";
  }
  CHECK(signatureFound);
  CHECK(indexFound);
}

TEST_CASE("DataStorage reports unaligned segment sizes",
          "[cross-read][datastorage][bounds]") {
  DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  using namespace DRW_DataStorageConst;

  constexpr std::size_t segmentEntriesOffset =
      HEADER_SIZE + SEGMENT_HEADER_SIZE;
  constexpr std::size_t dataSegmentEntry =
      segmentEntriesOffset + 2u * SEGMENT_INDEX_ENTRY_SIZE;
  constexpr std::size_t dataSegmentOffset = HEADER_SIZE + SEGMENT_HEADER_SIZE
      + 3u * SEGMENT_INDEX_ENTRY_SIZE + SEGMENT_HEADER_SIZE + 8u
      + DATA_INDEX_ENTRY_SIZE;
  raw.m_data.push_back(0u);
  writeU32(raw.m_data, 52u,
           static_cast<std::uint32_t>(raw.m_data.size()));
  writeU32(raw.m_data, dataSegmentEntry + 8u,
           static_cast<std::uint32_t>(raw.m_data.size()
                                      - dataSegmentOffset));

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  bool found = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    if (diagnostic.code == "datastorage-segment-size-not-aligned"
        && diagnostic.hasOffset
        && diagnostic.offset == parsed.segments[2].offset) {
      found = true;
      break;
    }
  }
  CHECK(found);
  CHECK(parsed.replayAllowed);
}

TEST_CASE("DataStorage ignores opaque headers but rejects invalid indexes",
          "[cross-read][datastorage][header][malformed]") {
  DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  using namespace DRW_DataStorageConst;
  writeU32(raw.m_data, 0u, 0u);
  writeI32(raw.m_data, 4u, static_cast<std::int32_t>(HEADER_SIZE - 4u));
  writeI32(raw.m_data, 36u, 0x7fffffff);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE_FALSE(parsed.parseFailed);
  CHECK(parsed.signature == 0u);
  CHECK(parsed.headerSize == static_cast<std::int32_t>(HEADER_SIZE - 4u));
  CHECK_FALSE(parsed.structurallyValid);
  CHECK_FALSE(parsed.replayAllowed);

  bool opaqueHeaderMisdiagnosed = false;
  bool indexReferenceFound = false;
  for (const auto& diagnostic : parsed.diagnostics) {
    opaqueHeaderMisdiagnosed |=
        diagnostic.code == "datastorage-file-signature-invalid"
        || diagnostic.code == "datastorage-file-header-size-invalid";
    indexReferenceFound |=
        diagnostic.code == "datastorage-segment-index-reference-invalid";
  }
  CHECK_FALSE(opaqueHeaderMisdiagnosed);
  CHECK(indexReferenceFound);
}

TEST_CASE("DataStorage rejects record headers outside their segment",
          "[cross-read][datastorage][malformed]") {
  DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  using namespace DRW_DataStorageConst;

  // Keep the forged header inside the section bytes while placing it beyond
  // the data segment's declared end.
  raw.m_data.resize(raw.m_data.size() + DATA_RECORD_HEADER_SIZE, 0);
  writeU32(raw.m_data, 52, static_cast<std::uint32_t>(raw.m_data.size()));

  constexpr std::size_t dataIndexOffset = HEADER_SIZE
      + SEGMENT_HEADER_SIZE + 3u * SEGMENT_INDEX_ENTRY_SIZE;
  constexpr std::size_t dataIndexEntry = dataIndexOffset
      + SEGMENT_HEADER_SIZE + 8u;
  writeU32(raw.m_data, dataIndexEntry + 4u, 40u);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE(parsed.records.empty());

  bool found = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    if (diagnostic.code == "datastorage-record-header-truncated"
        && diagnostic.message.find("referenced segment") != std::string::npos) {
      found = true;
      break;
    }
  }
  CHECK(found);
}

TEST_CASE("DataStorage data-index entries stay within their segment",
          "[cross-read][datastorage][malformed]") {
  DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  using namespace DRW_DataStorageConst;

  constexpr std::size_t segmentEntriesOffset =
      HEADER_SIZE + SEGMENT_HEADER_SIZE;
  constexpr std::size_t dataIndexSegmentEntry =
      segmentEntriesOffset + SEGMENT_INDEX_ENTRY_SIZE;
  constexpr std::size_t dataIndexOffset = segmentEntriesOffset
      + 3u * SEGMENT_INDEX_ENTRY_SIZE;
  constexpr std::size_t dataSegmentOffset = dataIndexOffset
      + SEGMENT_HEADER_SIZE + 8u + DATA_INDEX_ENTRY_SIZE;

  // Leave only the data-index segment header in its declared range. Forge a
  // count in the next segment so a section-wide reader would consume the next
  // segment header as a data-index entry and fabricate the real record.
  writeU32(raw.m_data, dataIndexSegmentEntry + 8u, SEGMENT_HEADER_SIZE);
  writeU32(raw.m_data, dataSegmentOffset, 1u);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  CHECK(parsed.dataIndexEntries.empty());
  CHECK(parsed.records.empty());

  bool found = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    if (diagnostic.code == "datastorage-data-index-truncated") {
      found = true;
      break;
    }
  }
  CHECK(found);
}

TEST_CASE("DataStorage rejects overflowing segment ranges",
          "[cross-read][datastorage][malformed]") {
  DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  using namespace DRW_DataStorageConst;

  constexpr std::size_t segmentEntry = HEADER_SIZE + SEGMENT_HEADER_SIZE
      + 2u * SEGMENT_INDEX_ENTRY_SIZE;
  writeU64(raw.m_data, segmentEntry,
           std::numeric_limits<std::uint64_t>::max() - 7u);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE(parsed.records.empty());

  bool found = false;
  for (const auto &diagnostic : parsed.diagnostics) {
    if (diagnostic.code == "datastorage-segment-size-out-of-bounds") {
      found = true;
      break;
    }
  }
  CHECK(found);
}

TEST_CASE("DataStorage retains large blob reference metadata",
          "[cross-read][datastorage][blob]") {
  const DRW_RawDwgSection raw = makeDataStorageBlobReferenceSection(0x4321u);
  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);

  REQUIRE_FALSE(parsed.parseFailed);
  REQUIRE(parsed.records.size() == 1u);
  const DRW_DataStorageRecord &record = parsed.records.front();
  CHECK(record.handle == 0x4321u);
  CHECK(record.isBlobReference);
  CHECK(record.blobTotalByteLength == 0x50000u);
  CHECK(record.blobPageCount == 1u);
  CHECK(record.blobRecordByteLength
        == DRW_DataStorageConst::DATA_BLOB_REFERENCE_FIXED_SIZE
             + DRW_DataStorageConst::DATA_BLOB_PAGE_ENTRY_SIZE);
  CHECK(record.blobPageByteSize == 0x50000u);
  CHECK(record.blobLastPageByteSize == 0x50000u);
  CHECK(record.dataByteLength == 0x50000u);
  REQUIRE(record.blobPages.size() == 1u);
  CHECK(record.blobPages.front().segmentIndex == 3u);
  CHECK(record.blobPages.front().byteLength == 0x50000u);
  CHECK(record.blobPayloadRetained);
  REQUIRE(record.payload.size() == 0x50000u);
  const std::string expectedMarker = "ACIS BinaryFile";
  CHECK(std::equal(expectedMarker.begin(), expectedMarker.end(),
                   record.payload.begin()));
  CHECK(record.hasPayloadMarker);
  CHECK(record.payloadMarkerOffset == 0u);
}

TEST_CASE("DataStorage reassembles multi-page blob payloads",
          "[cross-read][datastorage][blob]") {
  const DRW_RawDwgSection raw = makeDataStorageBlobReferenceSection(
      0x4321u, {0x40000u, 0x1234u});
  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);

  REQUIRE_FALSE(parsed.parseFailed);
  REQUIRE(parsed.records.size() == 1u);
  const DRW_DataStorageRecord &record = parsed.records.front();
  CHECK(record.isBlobReference);
  CHECK(record.blobTotalByteLength == 0x41234u);
  CHECK(record.blobPageCount == 2u);
  REQUIRE(record.blobPages.size() == 2u);
  CHECK(record.blobPages[0].segmentIndex == 3u);
  CHECK(record.blobPages[0].byteLength == 0x40000u);
  CHECK(record.blobPages[1].segmentIndex == 4u);
  CHECK(record.blobPages[1].byteLength == 0x1234u);
  CHECK(record.blobPayloadRetained);
  REQUIRE(record.payload.size() == 0x41234u);
  const std::string expectedMarker = "ACIS BinaryFile";
  CHECK(std::equal(expectedMarker.begin(), expectedMarker.end(),
                   record.payload.begin()));
  CHECK(record.payload[0x40000u] == 1u);
}

TEST_CASE("DataStorage rejects truncated blob reference pages",
          "[cross-read][datastorage][blob][malformed]") {
  DRW_RawDwgSection raw = makeDataStorageBlobReferenceSection(0x4321u);
  constexpr std::size_t dataSegmentOffset =
      DRW_DataStorageConst::HEADER_SIZE
      + (DRW_DataStorageConst::SEGMENT_HEADER_SIZE
         + 4u * DRW_DataStorageConst::SEGMENT_INDEX_ENTRY_SIZE)
      + (DRW_DataStorageConst::SEGMENT_HEADER_SIZE + 8u
         + DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE);
  constexpr std::size_t recordData = dataSegmentOffset
      + (((dataSegmentOffset + DRW_DataStorageConst::SEGMENT_HEADER_SIZE
           + DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE + 15u) & ~15u)
         - dataSegmentOffset);
  writeU32(raw.m_data, recordData + 12u, 2u);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  CHECK(parsed.records.empty());
  CHECK(std::any_of(
      parsed.diagnostics.cbegin(), parsed.diagnostics.cend(),
      [](const DRW_DataStorageDiagnostic &diagnostic) {
        return diagnostic.code == "datastorage-blob-reference-invalid";
      }));
}

TEST_CASE("DataStorage rejects inconsistent blob page headers",
          "[cross-read][datastorage][blob][malformed]") {
  DRW_RawDwgSection raw = makeDataStorageBlobReferenceSection(0x4321u);
  const DRW_DataStorageSection baseline =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE(baseline.segments.size() > 3u);
  const DRW_DataStorageSegment &blobSegment = baseline.segments[3];
  // The blob page data-size field is the final u64 in its 32-byte page header.
  const std::size_t pageDataSize = static_cast<std::size_t>(blobSegment.offset)
      + DRW_DataStorageConst::SEGMENT_HEADER_SIZE + 24u;
  writeU64(raw.m_data, pageDataSize, 0x4FFFFu);

  const DRW_DataStorageSection parsed =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  CHECK(parsed.records.empty());
  CHECK(std::any_of(
      parsed.diagnostics.cbegin(), parsed.diagnostics.cend(),
      [](const DRW_DataStorageDiagnostic &diagnostic) {
        return diagnostic.code == "datastorage-blob-page-segment-invalid";
      }));
}

TEST_CASE("AC1027 blob DataStorage raw replay remains byte exact",
          "[dwg-write][datastorage][blob][replay]") {
  const std::filesystem::path path = std::filesystem::temp_directory_path()
      / "libdxfrw_blob_datastorage_r2013.dwg";
  std::error_code error;
  std::filesystem::remove(path, error);

  SurfaceDataStorageReplayInterface writeIface;
  writeIface.m_section =
      makeDataStorageBlobReferenceSection(writeIface.m_surface.m_handle,
                                          {0x40000u, 0x1234u});
  {
    dwgRW writer(path.string().c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, /*bin=*/false));
  }

  StubInterface readIface;
  dwgRW reader(path.string().c_str());
  REQUIRE(reader.read(&readIface, /*ext=*/true));
  REQUIRE(reader.getVersion() == DRW::AC1027);
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  REQUIRE(readIface.m_rawSections.size() == 1u);
  CHECK(readIface.m_rawSections.front().m_data == writeIface.m_section.m_data);
  REQUIRE(readIface.m_storages.size() == 1u);
  REQUIRE(readIface.m_storages.front().records.size() == 1u);
  const DRW_DataStorageRecord &record =
      readIface.m_storages.front().records.front();
  CHECK(record.isBlobReference);
  CHECK(record.blobTotalByteLength == 0x41234u);
  CHECK(record.blobPages.size() == 2u);
  CHECK(record.blobPayloadRetained);
  REQUIRE(record.payload.size() == 0x41234u);
  CHECK(record.hasPayloadMarker);
  REQUIRE(readIface.m_surfaceValues.size() == 1u);
  CHECK(readIface.m_surfaceValues.front().hasDataStorageRecord);
  CHECK(readIface.m_surfaceValues.front().dataStorageData.size() == 0x41234u);
  CHECK(readIface.m_surfaceDataStorageHandles
        == std::vector<std::uint32_t>{writeIface.m_surface.m_handle});
  CHECK(reader.getDataStorageLinkFailures() == 0u);

  std::filesystem::remove(path, error);
}

TEST_CASE("DataStorage metadata store retains handle list",
          "[cross-read][datastorage]") {
  LC_DwgAdvancedMetadata meta;
  DRW_DataStorageSection section;
  section.m_name = "AcDb:AcDsPrototype_1b";
  section.fileSize = 100;
  DRW_DataStorageRecord rec;
  rec.handle = 0xABCDEF;
  rec.dataByteLength = 16;
  section.records.push_back(rec);
  section.orphanRecordCount = 1u;
  meta.addDataStorage(section);
  REQUIRE(meta.dataStorages().size() == 1);
  REQUIRE(meta.dataStorages().front().recordCount == 1);
  REQUIRE(meta.dataStorages().front().recordHandles.front() == 0xABCDEF);
  REQUIRE(meta.dataStorages().front().orphanRecordCount == 1u);
}

TEST_CASE("DataStorage parse failure blocks raw section replay",
          "[cross-read][datastorage][replay-safety]") {
  constexpr const char *sectionName = "AcDb:AcDsPrototype_1b";

  DRW_RawDwgSection raw;
  raw.m_name = sectionName;
  raw.m_version = DRW::AC1027;
  raw.m_data = {0x01u, 0x02u, 0x03u};

  DRW_DataStorageSection storage;
  storage.m_name = sectionName;
  storage.m_version = DRW::AC1027;
  storage.parseFailed = true;

  LC_DwgAdvancedMetadata rawFirst;
  rawFirst.addRawDwgSection(raw);
  rawFirst.addDataStorage(storage);
  REQUIRE(rawFirst.rawDwgSections().size() == 1u);
  CHECK(rawFirst.rawDwgSections().front().replayState
        == LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);

  LC_DwgAdvancedMetadata storageFirst;
  storageFirst.addDataStorage(storage);
  storageFirst.addRawDwgSection(raw);
  REQUIRE(storageFirst.rawDwgSections().size() == 1u);
  CHECK(storageFirst.rawDwgSections().front().replayState
        == LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
}

TEST_CASE("DataStorage presence flags require same-version replay data",
          "[cross-read][datastorage][replay-safety][version-policy]") {
  const DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  DRW_DataStorageSection storage;
  storage.m_name = raw.m_name;
  storage.m_version = DRW::AC1027;

  LC_DwgAdvancedMetadata metadata;
  metadata.addRawDwgSection(raw);
  metadata.addDataStorage(storage);
  CHECK(metadata.hasReplayableDataStorage(DRW::AC1027));
  CHECK_FALSE(metadata.hasReplayableDataStorage(DRW::AC1032));
  CHECK_FALSE(metadata.hasReplayableDataStorage(DRW::AC1024));

  DRW_DataStorageSection failed = storage;
  failed.parseFailed = true;
  LC_DwgAdvancedMetadata failedMetadata;
  failedMetadata.addRawDwgSection(raw);
  failedMetadata.addDataStorage(failed);
  CHECK_FALSE(failedMetadata.hasReplayableDataStorage(DRW::AC1027));

  LC_DwgAdvancedMetadata noRawMetadata;
  noRawMetadata.addDataStorage(storage);
  CHECK_FALSE(noRawMetadata.hasReplayableDataStorage(DRW::AC1027));
}

TEST_CASE("Empty DataStorage sections replay at the same version",
          "[dwg-write][datastorage][replay-safety]") {
  ensureQtContext();

  struct Target {
    RS2::FormatType format;
    DRW::Version version;
    const char *suffix;
  };
  // AcDb:AcDsPrototype_1b is introduced in AC1027; earlier writers must
  // reject the section rather than emit an invalid R2004 container.
  const Target targets[] = {{RS2::FormatDWG2013, DRW::AC1027, "r2013"},
                            {RS2::FormatDWG2018, DRW::AC1032, "r2018"}};

  for (const Target &target : targets) {
    RS_Graphic source;
    source.initForNewDocument();
    auto &metadata = source.dwgAdvancedMetadata();

    DRW_RawDwgSection raw;
    raw.m_name = "AcDb:AcDsPrototype_1b";
    raw.m_version = target.version;
    metadata.addRawDwgSection(raw);

    DRW_DataStorageSection storage;
    storage.m_name = raw.m_name;
    storage.m_version = raw.m_version;
    storage.sectionByteLength = 0;
    metadata.addDataStorage(storage);

    const std::filesystem::path path = std::filesystem::temp_directory_path()
        / (std::string("librecad_empty_datastorage_") + target.suffix +
           ".dwg");
    std::error_code error;
    std::filesystem::remove(path, error);

    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(source, QString::fromStdString(path.string()),
                                target.format));
    }

    StubInterface readIface;
    dwgRW reader(path.string().c_str());
    REQUIRE(reader.read(&readIface, /*ext=*/true));
    REQUIRE(readIface.m_rawSections.size() == 1u);
    CHECK(readIface.m_rawSections.front().m_name == raw.m_name);
    CHECK(readIface.m_rawSections.front().m_data.empty());
    REQUIRE(readIface.m_storages.size() == 1u);
    CHECK(readIface.m_storages.front().records.empty());

    std::filesystem::remove(path, error);
  }
}

TEST_CASE("DataStorage presence flags require the object's payload record",
          "[cross-read][datastorage][replay-safety][handle-link]") {
  const DRW_RawDwgSection raw = makeDataStorageReplaySection(0x1234u);
  const DRW_DataStorageSection storage =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE_FALSE(storage.parseFailed);
  REQUIRE(storage.findRecordByHandle(0x1234u) != nullptr);

  LC_DwgAdvancedMetadata metadata;
  metadata.addRawDwgSection(raw);
  metadata.addDataStorage(storage);
  CHECK(metadata.hasReplayableDataStorage(DRW::AC1027, 0x1234u));
  CHECK_FALSE(metadata.hasReplayableDataStorage(DRW::AC1027, 0x5678u));
  CHECK_FALSE(metadata.hasReplayableDataStorage(DRW::AC1032, 0x1234u));
}

TEST_CASE("RS_FilterDXFRW rejects typed DataStorage presence without an opaque carrier",
          "[dwg-write][datastorage][filter][replay-safety]") {
  ensureQtContext();

  RS_Graphic source;
  source.initForNewDocument();
  auto &metadata = source.dwgAdvancedMetadata();

  const DRW_RawDwgSection raw = makeDataStorageReplaySection(0x9B0u);
  const DRW_DataStorageSection storage =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE_FALSE(storage.parseFailed);
  metadata.addRawDwgSection(raw);
  metadata.addDataStorage(storage);

  DRW_Section section;
  section.handle = 0x9B0u;
  section.parentHandle =
      static_cast<int>(DRW::DwgNamedObjectsDictionaryHandle);
  section.m_kind = DRW_Section::Manager;
  section.setDwgCommonObjectState(0, 0, true);
  metadata.addSection(section);

  const std::filesystem::path sameVersionPath =
      std::filesystem::temp_directory_path()
      / "librecad_datastorage_same_version.dwg";
  const std::filesystem::path crossVersionPath =
      std::filesystem::temp_directory_path()
      / "librecad_datastorage_cross_version.dwg";
  std::error_code error;
  std::filesystem::remove(sameVersionPath, error);
  std::filesystem::remove(crossVersionPath, error);

  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(
        source, QString::fromStdString(sameVersionPath.string()),
        RS2::FormatDWG2013));
  }
  CHECK_FALSE(std::filesystem::exists(sameVersionPath));

  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(
        source, QString::fromStdString(crossVersionPath.string()),
        RS2::FormatDWG2018));
  }
  CHECK_FALSE(std::filesystem::exists(crossVersionPath));

  std::filesystem::remove(sameVersionPath, error);
  std::filesystem::remove(crossVersionPath, error);
}

TEST_CASE("RS_FilterDXFRW rejects duplicate DataStorage record identities",
          "[dwg-write][datastorage][filter][replay-safety][malformed]") {
  ensureQtContext();

  RS_Graphic source;
  source.initForNewDocument();
  auto &metadata = source.dwgAdvancedMetadata();

  const DRW_RawDwgSection raw = makeDataStorageReplaySection(0x9B1u);
  DRW_DataStorageSection storage =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE_FALSE(storage.parseFailed);
  REQUIRE(storage.records.size() == 1u);
  storage.records.push_back(storage.records.front());
  metadata.addRawDwgSection(raw);
  metadata.addDataStorage(storage);

  DRW_Section section;
  section.handle = 0x9B1u;
  section.parentHandle =
      static_cast<int>(DRW::DwgNamedObjectsDictionaryHandle);
  section.m_kind = DRW_Section::Manager;
  section.setDwgCommonObjectState(0, 0, true);
  metadata.addSection(section);

  const std::filesystem::path path = std::filesystem::temp_directory_path()
      / "librecad_datastorage_duplicate_record.dwg";
  std::error_code error;
  std::filesystem::remove(path, error);

  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(source, QString::fromStdString(path.string()),
                                RS2::FormatDWG2013));
  CHECK_FALSE(std::filesystem::exists(path));
  std::filesystem::remove(path, error);
}

TEST_CASE("RS_FilterDXFRW rejects an unmatched DataStorage producer",
          "[dwg-write][datastorage][filter][replay-safety][malformed]") {
  ensureQtContext();

  RS_Graphic source;
  source.initForNewDocument();
  auto &metadata = source.dwgAdvancedMetadata();

  const DRW_RawDwgSection raw = makeDataStorageReplaySection(0x9B2u);
  const DRW_DataStorageSection storage =
      DRW_parseDataStorage(raw.m_data, DRW::AC1027);
  REQUIRE_FALSE(storage.parseFailed);
  metadata.addRawDwgSection(raw);
  metadata.addDataStorage(storage);

  DRW_Section section;
  section.handle = 0x9B3u;
  section.parentHandle =
      static_cast<int>(DRW::DwgNamedObjectsDictionaryHandle);
  section.m_kind = DRW_Section::Manager;
  section.setDwgCommonObjectState(0, 0, true);
  metadata.addSection(section);

  const std::filesystem::path path = std::filesystem::temp_directory_path()
      / "librecad_datastorage_unmatched_producer.dwg";
  std::error_code error;
  std::filesystem::remove(path, error);

  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(source, QString::fromStdString(path.string()),
                                RS2::FormatDWG2013));
  CHECK_FALSE(std::filesystem::exists(path));
  std::filesystem::remove(path, error);
}

TEST_CASE("RS_FilterDXFRW rejects DataStorage without raw replay bytes",
          "[dwg-write][datastorage][filter][replay-safety]") {
  ensureQtContext();

  RS_Graphic source;
  source.initForNewDocument();
  DRW_DataStorageSection storage;
  storage.m_version = DRW::AC1027;
  storage.sectionByteLength = 56u;
  source.dwgAdvancedMetadata().addDataStorage(storage);

  const std::filesystem::path path =
      std::filesystem::temp_directory_path()
      / "librecad_datastorage_missing_raw.dwg";
  std::error_code error;
  std::filesystem::remove(path, error);

  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(source, QString::fromStdString(path.string()),
                                RS2::FormatDWG2013));

  std::filesystem::remove(path, error);
}

TEST_CASE("DataStorage handle keys and surface payload metadata are retained",
          "[cross-read][datastorage][surface]") {
  DRW_DataStorageSection section;
  DRW_DataStorageRecord record;
  record.handle = 0x1ABCDEF0u;
  record.handleKey = "1ABCDEF0";
  section.records.push_back(record);
  section.recordIndexByHandle.emplace(record.handle, 0u);
  section.recordIndexByHandleKey.emplace(record.handleKey, 0u);
  REQUIRE(section.findRecordByHandleKey("1ABCDEF0") == &section.records.front());

  DRW_PlaneSurface surface;
  surface.handle = 0x221u;
  surface.parentHandle = 0x222u;
  surface.uIsolines = 3;
  surface.vIsolines = 4;
  surface.modelerFormatVersion = 2;
  surface.acisVersion = 7;
  surface.hasDataStorageRecord = true;
  DrwDataStorageTestAccess::setDataStorageFlag(surface, true);
  surface.dataStorageHandle = record.handle;
  surface.dataStorageHandleKey = record.handleKey;
  surface.dataStorageData = {0x41u, 0x43u, 0x49u, 0x53u};
  surface.dataStorageSegmentIndex = 5u;
  surface.dataStorageSchemaIndex = 6u;
  surface.hasDataStoragePayloadMarker = true;
  surface.dataStoragePayloadMarkerLength = 15u;
  surface.dataStoragePayloadMarkerSection = "datastore";
  surface.rawAcisData = {0x10u, 0x20u};

  LC_DwgAdvancedMetadata metadata;
  metadata.addSurface(surface);
  REQUIRE(metadata.surfaceGeometry().size() == 1u);
  const auto &saved = metadata.surfaceGeometry().front();
  CHECK(saved.type == DRW::PLANESURFACE);
  CHECK(saved.handle == surface.handle);
  CHECK(saved.parentHandle == surface.parentHandle);
  CHECK(saved.uIsolines == 3);
  CHECK(saved.vIsolines == 4);
  CHECK(saved.modelerFormatVersion == 2);
  CHECK(saved.acisVersion == 7);
  CHECK(saved.hasDataStorageRecord);
  CHECK(saved.hasDsData);
  CHECK(saved.dataStorageHandleKey == "1ABCDEF0");
  CHECK(saved.dataStorageData == surface.dataStorageData);
  CHECK(saved.payloadKind == LC_DwgAdvancedMetadata::ModelerPayloadKind::Sab);
  CHECK(saved.markerLength == 15u);
  CHECK(saved.markerText == "ACIS BinaryFile");
  CHECK(saved.markerSection ==
        LC_DwgAdvancedMetadata::ModelerPayloadSection::DataStorage);
  CHECK(saved.rawAcisData == surface.rawAcisData);
}

TEST_CASE("Surface inline ACIS payload metadata is classified",
          "[cross-read][surface][acis]") {
  DRW_PlaneSurface surface;
  surface.handle = 0x321u;
  surface.rawAcisData = {
      0x10u, 'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y',
      'F', 'i', 'l', 'e', 0x20u};

  LC_DwgAdvancedMetadata metadata;
  metadata.addSurface(surface);
  REQUIRE(metadata.surfaceGeometry().size() == 1u);
  const auto &saved = metadata.surfaceGeometry().front();
  CHECK(saved.payloadKind ==
        LC_DwgAdvancedMetadata::ModelerPayloadKind::Sab);
  CHECK(saved.markerOffset == 1u);
  CHECK(saved.markerLength == 15u);
  CHECK(saved.markerText == "ACIS BinaryFile");
  CHECK(saved.markerSection ==
        LC_DwgAdvancedMetadata::ModelerPayloadSection::Body);
}

TEST_CASE("DWG surface entities retain typed and raw carriers",
          "[cross-read][surface][raw]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/assoc_surface_r2004.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("assoc_surface_r2004.dwg fixture not found; skipping");
  }

  StubInterface cap;
  dwgRW reader(path.c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getVersion() == DRW::AC1018);
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  REQUIRE(cap.m_surfaces.size() >= 5u);

  static const char *surfaceNames[] = {
      "PLANESURFACE", "EXTRUDEDSURFACE", "REVOLVEDSURFACE",
      "SWEPTSURFACE", "LOFTEDSURFACE", "NURBSURFACE"};
  std::size_t rawSurfaceCount = 0;
  for (const DRW_UnsupportedObject &raw : cap.m_rawObjects) {
    bool isSurface = false;
    for (const char *name : surfaceNames) {
      if (raw.m_recordName == name) {
        isSurface = true;
        break;
      }
    }
    if (!isSurface)
      continue;
    ++rawSurfaceCount;
    CHECK(raw.m_isEntity);
    CHECK(raw.m_isCustomClass);
    CHECK(raw.m_objectType >= 500);
    CHECK_FALSE(raw.m_rawBytes.empty());
    CHECK_FALSE(raw.m_hasDataStorage);
  }
  CHECK(rawSurfaceCount == cap.m_surfaces.size());
  CHECK(cap.m_surfacesWithRawPayload == cap.m_surfaces.size());

  const auto meshRaw = std::find_if(
      cap.m_rawObjects.cbegin(), cap.m_rawObjects.cend(),
      [](const DRW_UnsupportedObject &raw) {
        return raw.m_isEntity && raw.m_isCustomClass &&
               raw.m_recordName == "MESH";
      });
  REQUIRE(meshRaw != cap.m_rawObjects.cend());
  CHECK_FALSE(meshRaw->m_rawBytes.empty());

  const auto findSurface = [&cap](DRW::ETYPE type) -> const DRW_Surface * {
    for (const DRW_Surface &surface : cap.m_surfaceValues) {
      if (surface.eType == type)
        return &surface;
    }
    return nullptr;
  };
  const DRW_Surface *extruded = findSurface(DRW::EXTRUDEDSURFACE);
  const DRW_Surface *lofted = findSurface(DRW::LOFTEDSURFACE);
  const DRW_Surface *revolved = findSurface(DRW::REVOLVEDSURFACE);
  const DRW_Surface *swept = findSurface(DRW::SWEPTSURFACE);
  const DRW_Surface *plane = findSurface(DRW::PLANESURFACE);
  REQUIRE(extruded != nullptr);
  REQUIRE(lofted != nullptr);
  REQUIRE(revolved != nullptr);
  REQUIRE(swept != nullptr);
  REQUIRE(plane != nullptr);

  // Values are from the byte-identical libreDWG Surface.dwg fixture. The
  // modeler field is intentionally absent for EXTRUDEDSURFACE. The fixture's
  // LOFTED modeler value violates its spec bound and the PLANESURFACE object
  // size is too short for its trailer; those values remain unpublished while
  // their raw bodies are preserved.
  CHECK(extruded->modelerFormatVersion == 0);
  CHECK(extruded->uIsolines == 8321);
  CHECK(extruded->vIsolines == 0);
  CHECK(lofted->modelerFormatVersion == 0);
  CHECK(lofted->uIsolines == 0);
  CHECK(lofted->vIsolines == 0);
  CHECK(revolved->modelerFormatVersion == 6);
  CHECK(revolved->uIsolines == 6);
  CHECK(revolved->vIsolines == 0);
  CHECK(swept->modelerFormatVersion == 6);
  CHECK(swept->uIsolines == 6);
  CHECK(swept->vIsolines == 0);
  CHECK(plane->modelerFormatVersion == 0);
  CHECK(plane->uIsolines == 0);
  CHECK(plane->vIsolines == 0);
}

TEST_CASE("DWG surface raw carriers replay through the filter",
          "[dwg-write][surface][raw]") {
  const std::string sourcePath =
      std::string(LIBRECAD_TEST_DIR) + "/assoc_surface_r2004.dwg";
  if (!std::filesystem::is_regular_file(sourcePath)) {
    SKIP("assoc_surface_r2004.dwg fixture not found; skipping");
  }
  ensureQtContext();

  const std::filesystem::path outputPath =
      std::filesystem::temp_directory_path() / "librecad_surface_r2004.dwg";
  std::error_code error;
  std::filesystem::remove(outputPath, error);

  RS_Graphic graphic;
  RS_FilterDXFRW input;
  REQUIRE(input.fileImport(graphic, QString::fromStdString(sourcePath),
                           RS2::FormatDWG));
  REQUIRE(graphic.dwgAdvancedMetadata().surfaceGeometry().size() >= 5u);
  RS_FilterDXFRW output;
  REQUIRE(output.fileExport(graphic, QString::fromStdString(outputPath.string()),
                            RS2::FormatDWG2004));

  StubInterface cap;
  dwgRW reader(outputPath.string().c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getVersion() == DRW::AC1018);
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  CHECK(cap.m_surfaces.size() >= 5u);

  std::size_t rawSurfaceCount = 0;
  for (const DRW_UnsupportedObject &raw : cap.m_rawObjects) {
    if (raw.m_isEntity && raw.m_isCustomClass
        && (raw.m_recordName == "PLANESURFACE"
            || raw.m_recordName == "EXTRUDEDSURFACE"
            || raw.m_recordName == "REVOLVEDSURFACE"
            || raw.m_recordName == "SWEPTSURFACE"
            || raw.m_recordName == "LOFTEDSURFACE"
            || raw.m_recordName == "NURBSURFACE")) {
      ++rawSurfaceCount;
      CHECK_FALSE(raw.m_rawBytes.empty());
    }
  }
  CHECK(rawSurfaceCount >= 5u);
  std::filesystem::remove(outputPath, error);
}

TEST_CASE("DWG dynblock_point has non-empty DataStorage records",
          "[cross-read][datastorage][fixture]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/dynblock_point.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("dynblock_point.dwg fixture not found; skipping");
  }

  StubInterface cap;
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  REQUIRE(ok);
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  // Raw section should still be delivered for replay consumers.
  bool hasRawPrototype = false;
  for (const auto &s : cap.m_rawSections) {
    if (s.m_name.find("AcDs") != std::string::npos
        || s.m_name.find("AcDb:AcDs") != std::string::npos
        || s.m_name == "AcDb:AcDsPrototype_1b") {
      hasRawPrototype = true;
      REQUIRE_FALSE(s.m_data.empty());
    }
  }
  REQUIRE(hasRawPrototype);

  REQUIRE_FALSE(cap.m_storages.empty());
  const DRW_DataStorageSection &storage = cap.m_storages.front();
  REQUIRE_FALSE(storage.parseFailed);
  REQUIRE(storage.records.size() > 0);
  REQUIRE(storage.recordIndexByHandle.size() == storage.records.size());
  REQUIRE(storage.recordIndexByHandleKey.size() == storage.records.size());
  REQUIRE(storage.recordIndexesByHandle.size() == storage.records.size());
  REQUIRE(storage.recordIndexesByHandleKey.size() == storage.records.size());
  REQUIRE(storage.duplicateRecordHandleKeys.empty());
  // Handles should be stable non-zero for real records.
  size_t nonZeroHandles = 0;
  for (const auto &r : storage.records) {
    if (r.handle != 0)
      ++nonZeroHandles;
    REQUIRE(storage.findRecordByHandle(r.handle) != nullptr);
    REQUIRE(storage.findRecordByHandleKey(r.handleKey) != nullptr);
    REQUIRE(storage.recordIndexesByHandle.at(r.handle).size() == 1u);
    REQUIRE(storage.recordIndexesByHandleKey.at(r.handleKey).size() == 1u);
  }
  REQUIRE(nonZeroHandles > 0);
  CHECK(reader.getDataStorageOrphanRecords() == storage.records.size());
}

TEST_CASE("DWG DataStorage counts agree with independent TS reader",
          "[cross-read][datastorage][external-reader]") {
  struct FixtureExpectation {
    const char *name;
    DRW::Version version;
    std::size_t segments;
    std::size_t dataIndexEntries;
    std::size_t records;
  };
  const FixtureExpectation fixtures[] = {
      {"section_object_r2018.dwg", DRW::AC1032, 54u, 3u, 3u},
      {"dynblock_point.dwg", DRW::AC1032, 79u, 1u, 1u},
      {"visualstyle_r2013.dwg", DRW::AC1027, 12u, 1u, 1u},
  };

  std::size_t checked = 0;
  for (const FixtureExpectation &fixture : fixtures) {
    const std::string path = std::string(LIBRECAD_TEST_DIR) + "/"
        + fixture.name;
    if (!std::filesystem::is_regular_file(path))
      continue;

    StubInterface cap;
    dwgR reader(path.c_str());
    REQUIRE(reader.read(&cap, /*ext=*/true));
    REQUIRE(reader.getError() == DRW::BAD_NONE);
    REQUIRE(reader.getVersion() == fixture.version);
    REQUIRE(cap.m_storages.size() == 1u);
    const DRW_DataStorageSection &storage = cap.m_storages.front();
    REQUIRE_FALSE(storage.parseFailed);
    CHECK(storage.segments.size() == fixture.segments);
    CHECK(storage.dataIndexEntries.size() == fixture.dataIndexEntries);
    CHECK(storage.records.size() == fixture.records);
    if (!storage.schemaIndexEntries.empty()) {
      // The independent decode reports five schema-index references. Its
      // schdat unknown-property count is a known FIXME, so derive the count
      // from the bounded referenced headers instead.
      CHECK(storage.schemaIndexEntries.size() == 5u);
      CHECK(storage.schemaPropertyEntries.size() == 4u);
      CHECK(storage.schemaCount == storage.schemas.size());
      CHECK(storage.schemaCount == 5u);
      for (const auto &schema : storage.schemas) {
        CHECK(schema.segmentIndex < storage.segments.size());
        CHECK(schema.localOffset < storage.segments[schema.segmentIndex].size);
      }
      CHECK(storage.schemaUnknownPropertyCount == 4u);
    }
    ++checked;
  }
  if (checked == 0u)
    SKIP("DataStorage corpus fixtures not found; skipping");
}

TEST_CASE("DWG AC1027 VISUALSTYLE keeps its physical DataStorage boundary",
          "[dwg][datastorage][fixture][ac1027]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/visualstyle_r2013.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("visualstyle_r2013.dwg fixture not found; skipping");
  }

  StubInterface cap;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  REQUIRE(reader.getVersion() == DRW::AC1027);

  REQUIRE(cap.m_rawSections.size() == 1u);
  const DRW_RawDwgSection& raw = cap.m_rawSections.front();
  REQUIRE(raw.m_name == "AcDb:AcDsPrototype_1b");
  REQUIRE(raw.m_version == DRW::AC1027);
  REQUIRE_FALSE(raw.m_data.empty());
  CHECK(raw.m_data.size() == 4096u);
  CHECK(sha256Hex(raw.m_data)
        == "3e385b5e8432ef39ca36f98104bf3fe2e567741760a6d76b35c0037c4078c281");

  REQUIRE(cap.m_storages.size() == 1u);
  const DRW_DataStorageSection& storage = cap.m_storages.front();
  CHECK_FALSE(storage.parseFailed);
  CHECK(storage.segments.size() == 12u);
  CHECK(storage.dataIndexEntries.size() == 1u);
  CHECK(storage.records.size() == 1u);
  CHECK(reader.getDataStorageLinkFailures() == 0u);
  CHECK(reader.getDataStorageOrphanRecords() == 1u);
  CHECK(storage.orphanRecordCount == 1u);
}

TEST_CASE("DWG AC1027 modeler fixture binds DataStorage owner records",
          "[dwg][datastorage][fixture][ac1027]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/datastorage_modeler_r2013.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("datastorage_modeler_r2013.dwg fixture not found; skipping");
  }

  StubInterface cap;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  // The fixture has three unsupported non-DataStorage objects. They are
  // retained as bounded warnings; the DataStorage section and all modeler
  // records below parse without a failure.
  CHECK(reader.getObjectParseFailures() == 3u);

  REQUIRE(cap.m_rawSections.size() == 1u);
  const DRW_RawDwgSection& raw = cap.m_rawSections.front();
  CHECK(raw.m_name == "AcDb:AcDsPrototype_1b");
  CHECK(raw.m_version == DRW::AC1027);
  CHECK(raw.m_data.size() == 20352u);
  CHECK(sha256Hex(raw.m_data)
        == "6447fe7ae583c89ef0ef4cd28446848968363b600fce7012df8f1f391d078ee8");

  REQUIRE(cap.m_storages.size() == 1u);
  const DRW_DataStorageSection& storage = cap.m_storages.front();
  CHECK_FALSE(storage.parseFailed);
  CHECK(storage.replayAllowed);
  CHECK(storage.segments.size() == 13u);
  CHECK(storage.dataIndexEntries.size() == 5u);
  CHECK(storage.records.size() == 4u);
  CHECK(storage.duplicateRecordHandleKeys.empty());
  CHECK(reader.getDataStorageLinkFailures() == 0u);
  CHECK(reader.getDataStorageOrphanRecords() == 1u);
  CHECK(storage.orphanRecordCount == 1u);

  struct ExpectedModeler {
    DRW::ETYPE type;
    std::uint32_t handle;
    const char* handleKey;
    std::size_t payloadSize;
    const char* payloadSha256;
  };
  const ExpectedModeler expectedModelers[] = {
      {DRW::REGION, 0x176u, "176", 1709u,
       "596d37aaf81ebe9662bd22ba199bce876403dec8da71dd3287b0255adb354782"},
      {DRW::E3DSOLID, 0x2E1u, "2E1", 8798u,
       "5ca4756434d068b932ac6e0100e5ff07eae36c7eed29a43367eeff8113cca0ec"},
      {DRW::REGION, 0x37Du, "37D", 2196u,
       "fabe3e9804c44472907987ea96409b30b7db35575a34688430b0157d1b6c5566"},
  };
  REQUIRE(cap.m_modelers.size() == std::size(expectedModelers));
  for (const ExpectedModeler& expected : expectedModelers) {
    const auto modeler = std::find_if(
        cap.m_modelers.cbegin(), cap.m_modelers.cend(),
        [&expected](const DRW_ModelerGeometry& candidate) {
          return candidate.handle == expected.handle;
        });
    REQUIRE(modeler != cap.m_modelers.cend());
    CHECK(modeler->eType == expected.type);
    CHECK(modeler->hasDataStorageBinaryData());
    CHECK(modeler->hasDataStorageRecord);
    CHECK(modeler->dataStorageHandle == expected.handle);
    CHECK(modeler->dataStorageHandleKey == expected.handleKey);

    const DRW_DataStorageRecord* record =
        storage.findRecordByHandle(expected.handle);
    REQUIRE(record != nullptr);
    CHECK(record->handleKey == expected.handleKey);
    CHECK(record->payload.size() == expected.payloadSize);
    CHECK(sha256Hex(record->payload) == expected.payloadSha256);
    CHECK(record->hasPayloadMarker);
    CHECK(modeler->dataStorageData == record->payload);
    CHECK(modeler->hasDataStoragePayloadMarker == record->hasPayloadMarker);
    CHECK(modeler->dataStoragePayloadMarkerOffset
          == record->payloadMarkerOffset);
    CHECK(modeler->dataStoragePayloadMarkerLength
          == record->payloadMarkerLength);
    CHECK(modeler->dataStoragePayloadMarkerSection
          == record->payloadMarkerSection);
  }

  const DRW_DataStorageRecord* orphan = storage.findRecordByHandle(0x22u);
  REQUIRE(orphan != nullptr);
  CHECK(orphan->handleKey == "22");
  CHECK(orphan->payload.size() == 2279u);
  CHECK(sha256Hex(orphan->payload)
        == "7564885c5419f3acdc48ee28bd18802df2bbf63c34f227dbcb7185918f9d542b");
  CHECK_FALSE(orphan->hasPayloadMarker);
}

TEST_CASE("DWG AC1027 fixture-derived DataStorage corruption stops publication",
          "[dwg][datastorage][fixture][malformed][ac1027]") {
  const std::filesystem::path sourcePath =
      std::string(LIBRECAD_TEST_DIR) + "/datastorage_modeler_r2013.dwg";
  if (!std::filesystem::is_regular_file(sourcePath)) {
    SKIP("datastorage_modeler_r2013.dwg fixture not found; skipping");
  }

  StubInterface source;
  dwgR sourceReader(sourcePath.string().c_str());
  REQUIRE(sourceReader.read(&source, /*ext=*/true));
  REQUIRE(source.m_rawSections.size() == 1u);
  REQUIRE(source.m_storages.size() == 1u);

  const DRW_RawDwgSection& raw = source.m_rawSections.front();
  const DRW_DataStorageSection& storage = source.m_storages.front();
  REQUIRE(raw.m_name == "AcDb:AcDsPrototype_1b");
  REQUIRE(storage.structurallyValid);
  REQUIRE(storage.replayAllowed);

  const auto directRecord = std::find_if(
      storage.records.cbegin(), storage.records.cend(),
      [](const DRW_DataStorageRecord& record) {
        return !record.isBlobReference && record.dataByteLength != 0u;
      });
  REQUIRE(directRecord != storage.records.cend());
  REQUIRE(directRecord->dataOffset < raw.m_data.size());
  REQUIRE(directRecord->dataByteLength > 0u);
  REQUIRE(directRecord->dataOffset + directRecord->dataByteLength
          <= raw.m_data.size());

  DRW_RawDwgSection truncatedPayload = raw;
  truncatedPayload.m_data.resize(static_cast<std::size_t>(
      directRecord->dataOffset + directRecord->dataByteLength - 1u));

  REQUIRE(storage.dataIndexSegmentIndex >= 0);
  REQUIRE(static_cast<std::size_t>(storage.dataIndexSegmentIndex)
          < storage.segments.size());
  const auto recordIndex = std::find_if(
      storage.records.cbegin(), storage.records.cend(),
      [&directRecord](const DRW_DataStorageRecord& record) {
        return record.segmentIndex == directRecord->segmentIndex
            && record.localOffset == directRecord->localOffset
            && record.schemaIndex == directRecord->schemaIndex;
      });
  REQUIRE(recordIndex != storage.records.cend());
  const auto dataIndexEntry = std::find_if(
      storage.dataIndexEntries.cbegin(), storage.dataIndexEntries.cend(),
      [&recordIndex](const DRW_DataStorageIndexEntry& entry) {
        return entry.segmentIndex == recordIndex->segmentIndex
            && entry.localOffset == recordIndex->localOffset
            && entry.schemaIndex == recordIndex->schemaIndex;
      });
  REQUIRE(dataIndexEntry != storage.dataIndexEntries.cend());
  REQUIRE(static_cast<std::size_t>(dataIndexEntry->segmentIndex)
          < storage.segments.size());
  const std::size_t dataIndexEntryNumber = static_cast<std::size_t>(
      std::distance(storage.dataIndexEntries.cbegin(), dataIndexEntry));
  const DRW_DataStorageSegment& dataIndexSegment = storage.segments[
      static_cast<std::size_t>(storage.dataIndexSegmentIndex)];
  const std::uint64_t dataIndexEntryOffset = dataIndexSegment.offset
      + DRW_DataStorageConst::SEGMENT_HEADER_SIZE + 8u
      + dataIndexEntryNumber * DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE;
  REQUIRE(dataIndexEntryOffset
          + DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE <= raw.m_data.size());

  DRW_RawDwgSection outOfRangeIndex = raw;
  writeU32(outOfRangeIndex.m_data,
           static_cast<std::size_t>(dataIndexEntryOffset + 4u),
           storage.segments[dataIndexEntry->segmentIndex].size);

  const auto requireStructuralFailure = [](const DRW_RawDwgSection& section,
                                           const char* diagnostic) {
    const DRW_DataStorageSection parsed = DRW_parseDataStorage(
        section.m_data, DRW::AC1027);
    CHECK_FALSE(parsed.parseFailed);
    CHECK_FALSE(parsed.structurallyValid);
    CHECK_FALSE(parsed.replayAllowed);
    CHECK(std::any_of(parsed.diagnostics.cbegin(), parsed.diagnostics.cend(),
                      [diagnostic](const DRW_DataStorageDiagnostic& value) {
                        return value.code == diagnostic;
                      }));
  };
  requireStructuralFailure(truncatedPayload, "datastorage-file-size-mismatch");
  requireStructuralFailure(outOfRangeIndex,
                           "datastorage-record-header-truncated");

  const auto requireReaderFailure = [](const char* name,
                                       DRW_RawDwgSection section) {
    const std::filesystem::path path = std::filesystem::temp_directory_path()
        / (std::string("libdxfrw_datastorage_fixture_") + name + ".dwg");
    std::error_code error;
    std::filesystem::remove(path, error);

    ModelerDataStorageReplayInterface writeIface;
    writeIface.m_section = std::move(section);
    {
      dwgRW writer(path.string().c_str());
      writeIface.writer = &writer;
      REQUIRE(writer.write(&writeIface, DRW::AC1027, /*bin=*/false));
    }

    StubInterface target;
    dwgRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&target, /*ext=*/true));
    CHECK(target.m_modelers.empty());
    CHECK(target.m_rawObjects.empty());
    CHECK(target.m_rawSections.empty());
    CHECK(target.m_storages.empty());
    std::filesystem::remove(path, error);
  };
  requireReaderFailure("truncated_payload", std::move(truncatedPayload));
  requireReaderFailure("out_of_range_index", std::move(outOfRangeIndex));
}

TEST_CASE("DWG AC1032 modeler fixture binds DataStorage owner records",
          "[dwg][datastorage][fixture][ac1032]") {
  const std::string path = std::string(LIBRECAD_TEST_DIR)
      + "/datastorage_modeler_r2018.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("datastorage_modeler_r2018.dwg fixture not found; skipping");
  }

  StubInterface cap;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getVersion() == DRW::AC1032);
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  REQUIRE(cap.m_rawSections.size() == 1u);
  const DRW_RawDwgSection& raw = cap.m_rawSections.front();
  CHECK(raw.m_name == "AcDb:AcDsPrototype_1b");
  CHECK(raw.m_version == DRW::AC1032);
  CHECK(raw.m_data.size() == 16768u);
  CHECK(sha256Hex(raw.m_data)
        == "66b0bd6784b8c6ac13747c05253ac671e3b5c1e5bfd6cffefdb9063ed9b485ce");

  REQUIRE(cap.m_storages.size() == 1u);
  const DRW_DataStorageSection& storage = cap.m_storages.front();
  CHECK_FALSE(storage.parseFailed);
  CHECK(storage.structurallyValid);
  CHECK(storage.replayAllowed);
  CHECK(storage.segments.size() == 7u);
  CHECK(storage.dataIndexEntries.size() == 4u);
  CHECK(storage.records.size() == 4u);
  CHECK(reader.getDataStorageLinkFailures() == 0u);
  CHECK(reader.getDataStorageOrphanRecords() == 1u);
  CHECK(storage.orphanRecordCount == 1u);

  struct ExpectedModeler {
    DRW::ETYPE type;
    std::uint32_t handle;
    const char* handleKey;
    std::size_t payloadSize;
    const char* payloadSha256;
  };
  const ExpectedModeler expectedModelers[] = {
      {DRW::REGION, 0x176u, "176", 1709u,
       "5af828335d39747ff64725164dd2d3e0d2c001061815474f81435b0c249d09d6"},
      {DRW::E3DSOLID, 0x2E1u, "2E1", 8798u,
       "ffe032b83c51e68c4197cefd240f4281de4a36b20dc509f2cf720ea9c716c689"},
      {DRW::REGION, 0x37Du, "37D", 2196u,
       "c3e4bbc5eadb086865293ba5d9d06c05417c3910dd70f6f54739e5383e1043e3"},
  };
  REQUIRE(cap.m_modelers.size() == std::size(expectedModelers));
  for (const ExpectedModeler& expected : expectedModelers) {
    const auto modeler = std::find_if(
        cap.m_modelers.cbegin(), cap.m_modelers.cend(),
        [&expected](const DRW_ModelerGeometry& candidate) {
          return candidate.handle == expected.handle;
        });
    REQUIRE(modeler != cap.m_modelers.cend());
    CHECK(modeler->eType == expected.type);
    CHECK(modeler->hasDataStorageBinaryData());
    CHECK(modeler->hasDataStorageRecord);
    CHECK(modeler->dataStorageHandle == expected.handle);
    CHECK(modeler->dataStorageHandleKey == expected.handleKey);
    CHECK(modeler->dataStorageData.size() == expected.payloadSize);
    CHECK(sha256Hex(modeler->dataStorageData) == expected.payloadSha256);

    const DRW_DataStorageRecord* record =
        storage.findRecordByHandle(expected.handle);
    REQUIRE(record != nullptr);
    CHECK(record->handleKey == expected.handleKey);
    CHECK(record->payload == modeler->dataStorageData);
    CHECK(record->hasPayloadMarker == modeler->hasDataStoragePayloadMarker);
    CHECK(record->payloadMarkerOffset
          == modeler->dataStoragePayloadMarkerOffset);
    CHECK(record->payloadMarkerLength
          == modeler->dataStoragePayloadMarkerLength);
    CHECK(record->payloadMarkerSection
          == modeler->dataStoragePayloadMarkerSection);
  }
}

TEST_CASE("DWG AC1027 modeler fixture replays DataStorage through the filter",
          "[dwg-write][datastorage][fixture][ac1027]") {
  ensureQtContext();
  const std::filesystem::path sourcePath =
      std::string(LIBRECAD_TEST_DIR) + "/datastorage_modeler_r2013.dwg";
  if (!std::filesystem::is_regular_file(sourcePath)) {
    SKIP("datastorage_modeler_r2013.dwg fixture not found; skipping");
  }

  const std::filesystem::path sameVersionPath =
      std::filesystem::temp_directory_path()
      / "librecad_datastorage_modeler_r2013_same.dwg";
  const std::filesystem::path crossVersionPath =
      std::filesystem::temp_directory_path()
      / "librecad_datastorage_modeler_r2018.dwg";
  const std::filesystem::path remappedPath =
      std::filesystem::temp_directory_path()
      / "librecad_datastorage_modeler_r2013_remapped.dwg";
  std::error_code error;
  for (const auto& path : {sameVersionPath, crossVersionPath, remappedPath})
    std::filesystem::remove(path, error);

  RS_Graphic source;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(source, QString::fromStdString(
                                    sourcePath.string()), RS2::FormatDWG));
  }
  const auto& metadata = source.dwgAdvancedMetadata();
  REQUIRE(metadata.rawDwgSections().size() == 1u);
  const auto& sourceRaw = metadata.rawDwgSections().front();
  CHECK(sourceRaw.name == "AcDb:AcDsPrototype_1b");
  CHECK(sourceRaw.version == DRW::AC1027);
  CHECK(sourceRaw.data.size() == 20352u);
  CHECK(sha256Hex(sourceRaw.data)
        == "6447fe7ae583c89ef0ef4cd28446848968363b600fce7012df8f1f391d078ee8");
  REQUIRE(metadata.dataStorages().size() == 1u);
  const auto& sourceStorage = metadata.dataStorages().front().full;
  CHECK(sourceStorage.records.size() == 4u);
  CHECK(sourceStorage.orphanRecordCount == 1u);
  REQUIRE(metadata.modelerGeometry().size() == 3u);

  // The source drawing contains unrelated unsupported objects. Build a clean
  // document around the fixture's exact DataStorage section and raw modeler
  // frames so this test isolates the same-version preservation contract.
  RS_Graphic replay;
  replay.initForNewDocument();
  auto& replayMetadata = replay.dwgAdvancedMetadata();
  replayMetadata.setSourceDwgVersion(DRW::AC1027);
  DRW_RawDwgSection replayRaw;
  replayRaw.m_name = sourceRaw.name;
  replayRaw.m_version = sourceRaw.version;
  replayRaw.m_data = sourceRaw.data;
  replayRaw.m_encoding = sourceRaw.encoding;
  replayRaw.m_encrypted = sourceRaw.encrypted;
  replayRaw.m_maxSize = sourceRaw.maxSize;
  replayMetadata.addRawDwgSection(replayRaw);
  replayMetadata.addDataStorage(sourceStorage);
  DRW_Layout layout;
  layout.handle = 0x22u;
  layout.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
  layout.name = "DataStorage layout";
  layout.setDwgCommonObjectState(0, 0, true);
  replayMetadata.addLayout(layout);

  const auto addModelerRawFrame = [&metadata, &replayMetadata](
                                      std::uint32_t handle) {
    const auto raw = std::find_if(
        metadata.rawObjects().cbegin(), metadata.rawObjects().cend(),
        [handle](const auto& candidate) {
          return candidate.handle == handle && candidate.hasDataStorage;
        });
    REQUIRE(raw != metadata.rawObjects().cend());
    DRW_UnsupportedObject frame;
    frame.m_objectType = raw->objectType;
    frame.m_handle = raw->handle;
    frame.m_parentHandle = raw->parentHandle;
    frame.m_blockOwnerHandle = raw->blockOwnerHandle;
    frame.m_bodyBitSize = raw->bodyBitSize;
    frame.m_objectOffset = raw->objectOffset;
    frame.m_objectSize = raw->objectSize;
    frame.m_isEntity = raw->isEntity;
    frame.m_isCustomClass = raw->isCustomClass;
    frame.m_recordName = raw->recordName;
    frame.m_className = raw->className;
    frame.m_rawBytes = raw->rawBytes;
    frame.m_version = raw->version;
    frame.m_hasDataStorage = raw->hasDataStorage;
    frame.m_hasClassDefinition = raw->hasClassDefinition;
    frame.m_classProxyFlag = raw->classProxyFlag;
    frame.m_classAppName = raw->classAppName;
    frame.m_classWasProxy = raw->classWasProxy;
    frame.m_classEntityFlagRaw = raw->classEntityFlagRaw;
    frame.m_classDwgVersion = raw->classDwgVersion;
    frame.m_classMaintenanceVersion = raw->classMaintenanceVersion;
    frame.m_classUnknown1 = raw->classUnknown1;
    frame.m_classUnknown2 = raw->classUnknown2;
    replayMetadata.addUnsupportedObject(frame);
  };
  for (const auto& modeler : metadata.modelerGeometry())
    addModelerRawFrame(modeler.handle);

  {
    RS_FilterDXFRW filter;
    const bool exported = filter.fileExport(
        replay, QString::fromStdString(sameVersionPath.string()),
        RS2::FormatDWG2013);
    const auto skips = filter.lastDwgWriteSkipCounters();
    CAPTURE(skips.entityWrites, skips.tableRecordWrites, skips.objectWrites,
            skips.classRegistrations, skips.rawObjectWrites,
            skips.rawSectionWrites, skips.blockDefinitions);
    REQUIRE(exported);
  }

  StubInterface sameVersionRead;
  {
    dwgRW reader(sameVersionPath.string().c_str());
    REQUIRE(reader.read(&sameVersionRead, /*ext=*/true));
    REQUIRE(reader.getVersion() == DRW::AC1027);
    REQUIRE(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getDataStorageLinkFailures() == 0u);
    CHECK(reader.getDataStorageOrphanRecords() == 1u);
  }
  REQUIRE(sameVersionRead.m_rawSections.size() == 1u);
  CHECK(sameVersionRead.m_rawSections.front().m_name
        == "AcDb:AcDsPrototype_1b");
  CHECK(sameVersionRead.m_rawSections.front().m_version == DRW::AC1027);
  CHECK(sameVersionRead.m_rawSections.front().m_data == sourceRaw.data);
  REQUIRE(sameVersionRead.m_storages.size() == 1u);
  const auto& sameVersionStorage = sameVersionRead.m_storages.front();
  REQUIRE(sameVersionStorage.records.size() == sourceStorage.records.size());
  CHECK(sameVersionStorage.orphanRecordCount == 1u);
  for (const auto& sourceRecord : sourceStorage.records) {
    const auto* outputRecord = sameVersionStorage.findRecordByHandle(
        sourceRecord.handle);
    REQUIRE(outputRecord != nullptr);
    CHECK(outputRecord->handleKey == sourceRecord.handleKey);
    CHECK(outputRecord->payload == sourceRecord.payload);
    CHECK(outputRecord->hasPayloadMarker == sourceRecord.hasPayloadMarker);
    CHECK(outputRecord->payloadMarkerOffset
          == sourceRecord.payloadMarkerOffset);
    CHECK(outputRecord->payloadMarkerLength
          == sourceRecord.payloadMarkerLength);
    CHECK(outputRecord->payloadMarkerSection
          == sourceRecord.payloadMarkerSection);
  }
  REQUIRE(sameVersionRead.m_modelers.size() == 3u);
  for (const auto& sourceModeler : metadata.modelerGeometry()) {
    const auto outputModeler = std::find_if(
        sameVersionRead.m_modelers.cbegin(), sameVersionRead.m_modelers.cend(),
        [&sourceModeler](const DRW_ModelerGeometry& candidate) {
          return candidate.handle == sourceModeler.handle;
        });
    REQUIRE(outputModeler != sameVersionRead.m_modelers.cend());
    CHECK(outputModeler->eType == sourceModeler.type);
    CHECK(outputModeler->hasDataStorageRecord);
    CHECK(outputModeler->dataStorageHandle == sourceModeler.dataStorageHandle);
    CHECK(outputModeler->dataStorageHandleKey
          == sourceModeler.dataStorageHandleKey);
    CHECK(outputModeler->dataStorageData == sourceModeler.dataStorageData);
  }

  // Opaque DataStorage rows may only accompany their source DWG version.
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(
        replay, QString::fromStdString(crossVersionPath.string()),
        RS2::FormatDWG2018));
  }
  CHECK_FALSE(std::filesystem::exists(crossVersionPath));

  // A newly emitted entity must not steal a source modeler's DataStorage
  // identity: that would require patching the modeler's opaque DWG frame.
  auto* line = new RS_Line(&replay,
                           RS_LineData(RS_Vector(0.0, 0.0),
                                       RS_Vector(10.0, 10.0)));
  line->setSourceHandle(0x176u);
  replay.addEntity(line);
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(replay,
                                  QString::fromStdString(remappedPath.string()),
                                  RS2::FormatDWG2013));
  }
  CHECK_FALSE(std::filesystem::exists(remappedPath));

  for (const auto& path : {sameVersionPath, crossVersionPath, remappedPath})
    std::filesystem::remove(path, error);
}

TEST_CASE("DWG AC1032 modeler fixture replays DataStorage through the filter",
          "[dwg-write][datastorage][fixture][ac1032]") {
  ensureQtContext();
  const std::filesystem::path sourcePath =
      std::string(LIBRECAD_TEST_DIR) + "/datastorage_modeler_r2018.dwg";
  if (!std::filesystem::is_regular_file(sourcePath)) {
    SKIP("datastorage_modeler_r2018.dwg fixture not found; skipping");
  }

  const std::filesystem::path sameVersionPath =
      std::filesystem::temp_directory_path()
      / "librecad_datastorage_modeler_r2018_same.dwg";
  const std::filesystem::path crossVersionPath =
      std::filesystem::temp_directory_path()
      / "librecad_datastorage_modeler_r2013_from_r2018.dwg";
  const std::filesystem::path remappedPath =
      std::filesystem::temp_directory_path()
      / "librecad_datastorage_modeler_r2018_remapped.dwg";
  std::error_code error;
  for (const auto& path : {sameVersionPath, crossVersionPath, remappedPath})
    std::filesystem::remove(path, error);

  RS_Graphic source;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(source, QString::fromStdString(
                                    sourcePath.string()), RS2::FormatDWG));
  }
  const auto& metadata = source.dwgAdvancedMetadata();
  REQUIRE(metadata.rawDwgSections().size() == 1u);
  const auto& sourceRaw = metadata.rawDwgSections().front();
  CHECK(sourceRaw.name == "AcDb:AcDsPrototype_1b");
  CHECK(sourceRaw.version == DRW::AC1032);
  CHECK(sourceRaw.data.size() == 16768u);
  CHECK(sha256Hex(sourceRaw.data)
        == "66b0bd6784b8c6ac13747c05253ac671e3b5c1e5bfd6cffefdb9063ed9b485ce");
  REQUIRE(metadata.dataStorages().size() == 1u);
  const auto& sourceStorage = metadata.dataStorages().front().full;
  CHECK(sourceStorage.records.size() == 4u);
  CHECK(sourceStorage.orphanRecordCount == 1u);
  REQUIRE(metadata.modelerGeometry().size() == 3u);

  RS_Graphic replay;
  replay.initForNewDocument();
  auto& replayMetadata = replay.dwgAdvancedMetadata();
  replayMetadata.setSourceDwgVersion(DRW::AC1032);
  DRW_RawDwgSection replayRaw;
  replayRaw.m_name = sourceRaw.name;
  replayRaw.m_version = sourceRaw.version;
  replayRaw.m_data = sourceRaw.data;
  replayRaw.m_encoding = sourceRaw.encoding;
  replayRaw.m_encrypted = sourceRaw.encrypted;
  replayRaw.m_maxSize = sourceRaw.maxSize;
  replayMetadata.addRawDwgSection(replayRaw);
  replayMetadata.addDataStorage(sourceStorage);
  DRW_Layout layout;
  layout.handle = 0x22u;
  layout.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
  layout.name = "DataStorage layout";
  layout.setDwgCommonObjectState(0, 0, true);
  replayMetadata.addLayout(layout);

  const auto addModelerRawFrame = [&metadata, &replayMetadata](
                                      std::uint32_t handle) {
    const auto raw = std::find_if(
        metadata.rawObjects().cbegin(), metadata.rawObjects().cend(),
        [handle](const auto& candidate) {
          return candidate.handle == handle && candidate.hasDataStorage;
        });
    REQUIRE(raw != metadata.rawObjects().cend());
    DRW_UnsupportedObject frame;
    frame.m_objectType = raw->objectType;
    frame.m_handle = raw->handle;
    frame.m_parentHandle = raw->parentHandle;
    frame.m_blockOwnerHandle = raw->blockOwnerHandle;
    frame.m_bodyBitSize = raw->bodyBitSize;
    frame.m_objectOffset = raw->objectOffset;
    frame.m_objectSize = raw->objectSize;
    frame.m_isEntity = raw->isEntity;
    frame.m_isCustomClass = raw->isCustomClass;
    frame.m_recordName = raw->recordName;
    frame.m_className = raw->className;
    frame.m_rawBytes = raw->rawBytes;
    frame.m_version = raw->version;
    frame.m_hasDataStorage = raw->hasDataStorage;
    frame.m_hasClassDefinition = raw->hasClassDefinition;
    frame.m_classProxyFlag = raw->classProxyFlag;
    frame.m_classAppName = raw->classAppName;
    frame.m_classWasProxy = raw->classWasProxy;
    frame.m_classEntityFlagRaw = raw->classEntityFlagRaw;
    frame.m_classDwgVersion = raw->classDwgVersion;
    frame.m_classMaintenanceVersion = raw->classMaintenanceVersion;
    frame.m_classUnknown1 = raw->classUnknown1;
    frame.m_classUnknown2 = raw->classUnknown2;
    replayMetadata.addUnsupportedObject(frame);
  };
  for (const auto& modeler : metadata.modelerGeometry())
    addModelerRawFrame(modeler.handle);

  {
    RS_FilterDXFRW filter;
    const bool exported = filter.fileExport(
        replay, QString::fromStdString(sameVersionPath.string()),
        RS2::FormatDWG2018);
    const auto skips = filter.lastDwgWriteSkipCounters();
    CAPTURE(skips.entityWrites, skips.tableRecordWrites, skips.objectWrites,
            skips.classRegistrations, skips.rawObjectWrites,
            skips.rawSectionWrites, skips.blockDefinitions);
    REQUIRE(exported);
  }

  StubInterface sameVersionRead;
  {
    dwgRW reader(sameVersionPath.string().c_str());
    REQUIRE(reader.read(&sameVersionRead, /*ext=*/true));
    REQUIRE(reader.getVersion() == DRW::AC1032);
    REQUIRE(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getDataStorageLinkFailures() == 0u);
    CHECK(reader.getDataStorageOrphanRecords() == 1u);
  }
  REQUIRE(sameVersionRead.m_rawSections.size() == 1u);
  CHECK(sameVersionRead.m_rawSections.front().m_name
        == "AcDb:AcDsPrototype_1b");
  CHECK(sameVersionRead.m_rawSections.front().m_version == DRW::AC1032);
  CHECK(sameVersionRead.m_rawSections.front().m_data == sourceRaw.data);
  REQUIRE(sameVersionRead.m_storages.size() == 1u);
  const auto& sameVersionStorage = sameVersionRead.m_storages.front();
  REQUIRE(sameVersionStorage.records.size() == sourceStorage.records.size());
  CHECK(sameVersionStorage.orphanRecordCount == 1u);
  for (const auto& sourceRecord : sourceStorage.records) {
    const auto* outputRecord = sameVersionStorage.findRecordByHandle(
        sourceRecord.handle);
    REQUIRE(outputRecord != nullptr);
    CHECK(outputRecord->handleKey == sourceRecord.handleKey);
    CHECK(outputRecord->payload == sourceRecord.payload);
    CHECK(outputRecord->hasPayloadMarker == sourceRecord.hasPayloadMarker);
    CHECK(outputRecord->payloadMarkerOffset
          == sourceRecord.payloadMarkerOffset);
    CHECK(outputRecord->payloadMarkerLength
          == sourceRecord.payloadMarkerLength);
    CHECK(outputRecord->payloadMarkerSection
          == sourceRecord.payloadMarkerSection);
  }
  REQUIRE(sameVersionRead.m_modelers.size() == 3u);
  for (const auto& sourceModeler : metadata.modelerGeometry()) {
    const auto outputModeler = std::find_if(
        sameVersionRead.m_modelers.cbegin(), sameVersionRead.m_modelers.cend(),
        [&sourceModeler](const DRW_ModelerGeometry& candidate) {
          return candidate.handle == sourceModeler.handle;
        });
    REQUIRE(outputModeler != sameVersionRead.m_modelers.cend());
    CHECK(outputModeler->eType == sourceModeler.type);
    CHECK(outputModeler->hasDataStorageRecord);
    CHECK(outputModeler->dataStorageHandle == sourceModeler.dataStorageHandle);
    CHECK(outputModeler->dataStorageHandleKey
          == sourceModeler.dataStorageHandleKey);
    CHECK(outputModeler->dataStorageData == sourceModeler.dataStorageData);
  }

  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(
        replay, QString::fromStdString(crossVersionPath.string()),
        RS2::FormatDWG2013));
  }
  CHECK_FALSE(std::filesystem::exists(crossVersionPath));

  auto* line = new RS_Line(&replay,
                           RS_LineData(RS_Vector(0.0, 0.0),
                                       RS_Vector(10.0, 10.0)));
  line->setSourceHandle(0x176u);
  replay.addEntity(line);
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(replay,
                                  QString::fromStdString(remappedPath.string()),
                                  RS2::FormatDWG2018));
  }
  CHECK_FALSE(std::filesystem::exists(remappedPath));

  for (const auto& path : {sameVersionPath, crossVersionPath, remappedPath})
    std::filesystem::remove(path, error);
}

TEST_CASE("AC1027 modeler and DataStorage replay link end to end",
          "[dwg-write][datastorage][fixture]") {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path()
      / "libdxfrw_modeler_datastorage_r2013.dwg";

  ModelerDataStorageReplayInterface writeIface;
  {
    dwgRW writer(path.string().c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, /*bin=*/false));
  }

  ModelerDataStorageReplayInterface readIface;
  dwgRW reader(path.string().c_str());
  REQUIRE(reader.read(&readIface, /*ext=*/false));
  REQUIRE(reader.getVersion() == DRW::AC1027);
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  REQUIRE(readIface.m_modelers.size() == 1u);
  const DRW_ModelerGeometry &modeler = readIface.m_modelers.front();
  CHECK(modeler.handle == writeIface.m_modeler.m_handle);
  CHECK(modeler.hasDataStorageRecord);
  CHECK(modeler.dataStorageHandle == writeIface.m_modeler.m_handle);
  CHECK(modeler.dataStorageHandleKey == "961");
  CHECK(modeler.dataStorageData
        == std::vector<std::uint8_t>({'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n',
                                      'a', 'r', 'y', 'F', 'i', 'l', 'e'}));
  CHECK(modeler.hasDataStoragePayloadMarker);
  CHECK(modeler.dataStoragePayloadMarkerOffset == 0u);
  CHECK(modeler.dataStoragePayloadMarkerLength == 15u);
  CHECK(modeler.dataStoragePayloadMarkerSection == "datastore");
  CHECK(modeler.dataStorageSegmentIndex == 2u);
  CHECK(modeler.dataStorageSchemaIndex == 7u);

  const auto modelerRaw = std::find_if(
      readIface.m_rawObjects.cbegin(), readIface.m_rawObjects.cend(),
      [&modeler](const auto &raw) {
        return raw.m_handle == modeler.handle && raw.m_objectType == 38 &&
               raw.m_isEntity;
      });
  REQUIRE(modelerRaw != readIface.m_rawObjects.cend());
  CHECK(modelerRaw->m_hasDataStorage);

  REQUIRE(readIface.m_storages.size() == 1u);
  const DRW_DataStorageSection &storage = readIface.m_storages.front();
  REQUIRE(storage.records.size() == 1u);
  REQUIRE(storage.findRecordByHandleKey("961") != nullptr);
  CHECK(storage.findRecordByHandleKey("961")->payload
        == modeler.dataStorageData);
  CHECK(storage.findRecordByHandleKey("961")->hasPayloadMarker);
  CHECK(storage.findRecordByHandleKey("961")->payloadMarkerOffset == 0u);
  CHECK(storage.findRecordByHandleKey("961")->payloadMarkerLength == 15u);
  CHECK(storage.findRecordByHandleKey("961")->payloadMarkerSection
        == "datastore");
  CHECK(storage.orphanRecordCount == 0u);
  CHECK(reader.getDataStorageLinkFailures() == 0u);
  CHECK(reader.getDataStorageOrphanRecords() == 0u);

  REQUIRE(readIface.m_rawSections.size() == 1u);
  CHECK(readIface.m_rawSections.front().m_name
        == "AcDb:AcDsPrototype_1b");
  CHECK(readIface.m_rawSections.front().m_version == DRW::AC1027);
  CHECK(readIface.m_rawSections.front().m_data == writeIface.m_section.m_data);

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST_CASE("AC1027 surface DataStorage carrier obeys remap and version policy",
          "[dwg-write][datastorage][surface][remap]") {
  ensureQtContext();
  constexpr std::uint32_t surfaceHandle = 0xA70u;
  const std::filesystem::path sourcePath =
      std::filesystem::temp_directory_path()
      / "libdxfrw_surface_datastorage_r2013.dwg";
  const std::filesystem::path sameVersionPath =
      std::filesystem::temp_directory_path()
      / "librecad_surface_datastorage_r2013_same.dwg";
  const std::filesystem::path crossVersionPath =
      std::filesystem::temp_directory_path()
      / "librecad_surface_datastorage_r2018.dwg";
  const std::filesystem::path remappedPath =
      std::filesystem::temp_directory_path()
      / "librecad_surface_datastorage_r2013_remapped.dwg";
  std::error_code error;
  for (const auto &path : {sourcePath, sameVersionPath, crossVersionPath,
                           remappedPath})
    std::filesystem::remove(path, error);

  SurfaceDataStorageReplayInterface writeIface;
  {
    dwgRW writer(sourcePath.string().c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, /*bin=*/false));
  }

  StubInterface sourceRead;
  {
    dwgRW reader(sourcePath.string().c_str());
    REQUIRE(reader.read(&sourceRead, /*ext=*/true));
    REQUIRE(reader.getVersion() == DRW::AC1027);
    REQUIRE(reader.getError() == DRW::BAD_NONE);
  }
  REQUIRE(sourceRead.m_surfaces.size() == 1u);
  REQUIRE(sourceRead.m_surfaceDataStorageHandles
          == std::vector<std::uint32_t>{surfaceHandle});
  const auto sourceRawSurface = std::find_if(
      sourceRead.m_rawObjects.cbegin(), sourceRead.m_rawObjects.cend(),
      [](const DRW_UnsupportedObject &object) {
        return object.m_recordName == "PLANESURFACE";
      });
  REQUIRE(sourceRawSurface != sourceRead.m_rawObjects.cend());
  CHECK(sourceRawSurface->m_hasDataStorage);
  REQUIRE(sourceRead.m_storages.size() == 1u);
  REQUIRE(sourceRead.m_storages.front().findRecordByHandle(surfaceHandle)
          != nullptr);

  RS_Graphic source;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(source,
                              QString::fromStdString(sourcePath.string()),
                              RS2::FormatDWG));
  }
  const auto sourceMetadataSurface = std::find_if(
      source.dwgAdvancedMetadata().rawObjects().cbegin(),
      source.dwgAdvancedMetadata().rawObjects().cend(),
      [](const LC_DwgAdvancedMetadata::RawObjectRecord &record) {
        return record.recordName == "PLANESURFACE";
      });
  REQUIRE(sourceMetadataSurface
          != source.dwgAdvancedMetadata().rawObjects().cend());
  CHECK(sourceMetadataSurface->hasDataStorage);

  // Stable source handle + exact AC1027 output retains the carrier bit and
  // the handle-keyed payload.
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(
        source, QString::fromStdString(sameVersionPath.string()),
        RS2::FormatDWG2013));
  }
  StubInterface sameVersionRead;
  {
    dwgRW reader(sameVersionPath.string().c_str());
    REQUIRE(reader.read(&sameVersionRead, /*ext=*/true));
    REQUIRE(reader.getVersion() == DRW::AC1027);
    REQUIRE(reader.getError() == DRW::BAD_NONE);
  }
  REQUIRE(sameVersionRead.m_surfaces.size() == 1u);
  REQUIRE(sameVersionRead.m_surfaceDataStorageHandles
          == std::vector<std::uint32_t>{surfaceHandle});
  const auto sameVersionRawSurface = std::find_if(
      sameVersionRead.m_rawObjects.cbegin(),
      sameVersionRead.m_rawObjects.cend(),
      [](const DRW_UnsupportedObject &object) {
        return object.m_recordName == "PLANESURFACE";
      });
  REQUIRE(sameVersionRawSurface != sameVersionRead.m_rawObjects.cend());
  CHECK(sameVersionRawSurface->m_hasDataStorage);
  REQUIRE(sameVersionRead.m_storages.size() == 1u);
  CHECK(sameVersionRead.m_storages.front().findRecordByHandle(surfaceHandle)
        != nullptr);

  // Cross-version opaque replay is rejected as a unit, and failed output is
  // removed instead of leaving a partial drawing behind.
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(
        source, QString::fromStdString(crossVersionPath.string()),
        RS2::FormatDWG2018));
  }
  CHECK_FALSE(std::filesystem::exists(crossVersionPath));

  // Reusing the same source handle for an emitted line forces the writer's
  // source->new entity map to remap it. The opaque surface cannot be patched
  // safely, so the export is rejected atomically.
  auto *line = new RS_Line(&source,
                           RS_LineData(RS_Vector(0.0, 0.0),
                                       RS_Vector(10.0, 10.0)));
  line->setSourceHandle(surfaceHandle);
  source.addEntity(line);
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(
        source, QString::fromStdString(remappedPath.string()),
        RS2::FormatDWG2013));
  }
  CHECK_FALSE(std::filesystem::exists(remappedPath));

  for (const auto &path : {sourcePath, sameVersionPath, crossVersionPath,
                           remappedPath})
    std::filesystem::remove(path, error);
}

TEST_CASE("DataStorage and modeler raw replay reject a version mismatch",
          "[dwg-write][datastorage][version-policy]") {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path()
      / "libdxfrw_modeler_datastorage_cross_version.dwg";

  ModelerDataStorageReplayInterface writeIface;
  writeIface.expectReplay = false;
  {
    dwgRW writer(path.string().c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1032, /*bin=*/false));
  }

  ModelerDataStorageReplayInterface readIface;
  dwgRW reader(path.string().c_str());
  REQUIRE(reader.read(&readIface, /*ext=*/false));
  REQUIRE(reader.getVersion() == DRW::AC1032);
  CHECK(readIface.m_modelers.empty());
  CHECK(readIface.m_storages.empty());
  CHECK(readIface.m_rawSections.empty());

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST_CASE("DWG DataStorage linkage is optional and handle keyed",
          "[cross-read][datastorage][linkage]") {
  const std::vector<std::string> candidates = {
      std::string(LIBRECAD_TEST_DIR) + "/dynblock_point.dwg",
      std::string(LIBRECAD_TEST_DIR) + "/tarch/t1.dwg",
      std::string(LIBRECAD_TEST_DIR) + "/acsh_r2007.dwg",
      std::string(LIBRECAD_TEST_DIR) + "/mpolygon_solid.dwg",
  };

  std::size_t filesWithStorage = 0;
  std::size_t linkedModelers = 0;
  for (const std::string &path : candidates) {
    if (!std::filesystem::is_regular_file(path))
      continue;

    StubInterface cap;
    dwgRW reader(path.c_str());
    if (!reader.read(&cap, /*ext=*/true))
      continue;
    if (cap.m_storages.empty())
      continue;

    ++filesWithStorage;
    for (const DRW_ModelerGeometry &modeler : cap.m_modelers) {
      if (!modeler.hasDataStorageRecord)
        continue;
      ++linkedModelers;
      REQUIRE(modeler.dataStorageHandle == modeler.handle);
      REQUIRE_FALSE(modeler.dataStorageHandleKey.empty());
      bool found = false;
      for (const DRW_DataStorageSection &storage : cap.m_storages) {
        const DRW_DataStorageRecord *record =
            storage.findRecordByHandle(modeler.handle);
        if (record == nullptr)
          continue;
        found = true;
        CHECK(record->handle == modeler.dataStorageHandle);
        CHECK(record->segmentIndex == modeler.dataStorageSegmentIndex);
        CHECK(record->schemaIndex == modeler.dataStorageSchemaIndex);
      }
      REQUIRE(found);
    }
  }

  if (filesWithStorage == 0) {
    SKIP("no DataStorage fixture available; linkage is optional");
  }
  if (linkedModelers == 0)
    SUCCEED("available DataStorage fixtures contain no modeler entity");
}

TEST_CASE("DWG DataStorage links modeler payloads by handle",
          "[dwg][datastorage][linkage]") {
  DwgDataStorageReaderProbe reader;
  reader.setVersionForTest(DRW::AC1027);

  DRW_DataStorageSection storage;
  storage.m_version = DRW::AC1027;
  storage.segments.push_back(DRW_DataStorageSegment{});
  DRW_DataStorageRecord record;
  record.handle = 0x1234u;
  record.handleKey = "1234";
  record.segmentIndex = 0;
  record.schemaIndex = 9;
  record.payload = {0x01u, 0x02u, 0x03u};
  storage.records.push_back(record);
  storage.recordIndexByHandle.emplace(record.handle, 0u);
  storage.recordIndexByHandleKey.emplace(record.handleKey, 0u);

  DRW_DataStorageRecord preferred = record;
  preferred.dataOffset = 1u;
  preferred.payload = {'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r',
                       'y', 'F', 'i', 'l', 'e'};
  preferred.hasPayloadMarker = true;
  preferred.payloadMarkerOffset = 0u;
  preferred.payloadMarkerLength = 15u;
  preferred.payloadMarkerSection = "datastore";
  storage.records.push_back(preferred);
  storage.recordIndexByHandle[record.handle] = 1u;
  storage.recordIndexByHandleKey[record.handleKey] = 1u;
  reader.m_dataStorageSections.push_back(storage);

  DRW_ModelerGeometry modeler(DRW::E3DSOLID);
  modeler.handle = 0x1234u;
  DrwDataStorageTestAccess::setDataStorageFlag(modeler, true);
  reader.linkDataStorage(modeler);

  REQUIRE(modeler.hasDataStorageRecord);
  CHECK(modeler.dataStorageHandle == 0x1234u);
  CHECK(modeler.dataStorageHandleKey == "1234");
  CHECK(modeler.dataStorageData ==
        std::vector<std::uint8_t>({'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n',
                                   'a', 'r', 'y', 'F', 'i', 'l', 'e'}));
  CHECK(modeler.hasDataStoragePayloadMarker);
  CHECK(modeler.dataStoragePayloadMarkerOffset == 0u);
  CHECK(modeler.dataStoragePayloadMarkerLength == 15u);
  CHECK(modeler.dataStoragePayloadMarkerSection == "datastore");
  CHECK(modeler.dataStorageSegmentIndex == 0u);
  CHECK(modeler.dataStorageSchemaIndex == 9u);
  CHECK(reader.m_dataStorageLinkFailures == 0u);

  LC_DwgAdvancedMetadata metadata;
  metadata.addModelerGeometry(modeler);
  REQUIRE(metadata.modelerGeometry().size() == 1u);
  const auto &modelerMetadata = metadata.modelerGeometry().front();
  CHECK(modelerMetadata.payloadKind ==
        LC_DwgAdvancedMetadata::ModelerPayloadKind::Sab);
  CHECK(modelerMetadata.markerOffset == 0u);
  CHECK(modelerMetadata.markerLength == 15u);
  CHECK(modelerMetadata.markerText == "ACIS BinaryFile");
  CHECK(modelerMetadata.markerSection ==
        LC_DwgAdvancedMetadata::ModelerPayloadSection::DataStorage);

  auto &linkedStorage = reader.m_dataStorageSections.front();
  reader.finalizeDataStorageLinks();
  REQUIRE(reader.m_dataStorageOrphanRecords == 1u);
  REQUIRE(linkedStorage.orphanRecordCount == 1u);
  bool foundOrphanDiagnostic = false;
  for (const auto &diagnostic : linkedStorage.diagnostics) {
    if (diagnostic.code == "datastorage-orphan-record"
        && diagnostic.handle == record.handle
        && diagnostic.offset == record.recordOffset) {
      foundOrphanDiagnostic = true;
    }
  }
  REQUIRE(foundOrphanDiagnostic);
  reader.finalizeDataStorageLinks();
  CHECK(reader.m_dataStorageOrphanRecords == 1u);
  CHECK(linkedStorage.orphanRecordCount == 1u);
  CHECK(std::count_if(linkedStorage.diagnostics.begin(),
                      linkedStorage.diagnostics.end(),
                      [](const auto &diagnostic) {
                        return diagnostic.code == "datastorage-orphan-record";
                      }) == 1);

  DRW_ModelerGeometry missing(DRW::REGION);
  missing.handle = 0x5678u;
  DrwDataStorageTestAccess::setDataStorageFlag(missing, true);
  reader.linkDataStorage(missing);
  CHECK_FALSE(missing.hasDataStorageRecord);
  CHECK(reader.m_dataStorageLinkFailures == 1u);

  LC_DwgAdvancedMetadata missingMetadata;
  missingMetadata.addModelerGeometry(missing);
  REQUIRE(missingMetadata.modelerGeometry().size() == 1u);
  CHECK(missingMetadata.modelerGeometry().front().hasDsData);
  CHECK_FALSE(missingMetadata.modelerGeometry().front().hasDataStorageRecord);
}

TEST_CASE("DWG DataStorage linkage enforces presence-bit and version gates",
          "[dwg][datastorage][linkage][presence]") {
  DwgDataStorageReaderProbe reader;
  reader.setVersionForTest(DRW::AC1027);

  DRW_DataStorageSection storage;
  storage.m_version = DRW::AC1027;
  storage.segments.push_back(DRW_DataStorageSegment{});
  DRW_DataStorageRecord record;
  record.handle = 0x1234u;
  record.handleKey = "1234";
  record.payload = {0x01u, 0x02u};
  storage.records.push_back(record);
  storage.recordIndexByHandle.emplace(record.handle, 0u);
  storage.recordIndexByHandleKey.emplace(record.handleKey, 0u);
  reader.m_dataStorageSections.push_back(storage);

  DRW_ModelerGeometry withoutPresence(DRW::REGION);
  withoutPresence.handle = record.handle;
  DrwDataStorageTestAccess::setDataStorageFlag(withoutPresence, false);
  reader.linkDataStorage(withoutPresence);
  CHECK_FALSE(withoutPresence.hasDataStorageRecord);
  CHECK(reader.m_dataStorageLinkFailures == 0u);

  DwgDataStorageReaderProbe legacy;
  legacy.setVersionForTest(DRW::AC1024);
  legacy.m_dataStorageSections.push_back(storage);
  DRW_ModelerGeometry legacyEntity(DRW::REGION);
  legacyEntity.handle = record.handle;
  DrwDataStorageTestAccess::setDataStorageFlag(legacyEntity, true);
  legacy.linkDataStorage(legacyEntity);
  CHECK_FALSE(legacyEntity.hasDataStorageRecord);
  CHECK(legacy.m_dataStorageLinkFailures == 0u);
}

TEST_CASE("DWG DataStorage linkage requires one explicit identity",
          "[dwg][datastorage][linkage]") {
  DwgDataStorageReaderProbe reader;
  reader.setVersionForTest(DRW::AC1027);

  DRW_DataStorageSection storage;
  storage.m_version = DRW::AC1027;
  storage.segments.push_back(DRW_DataStorageSegment{});

  DRW_DataStorageRecord byKey;
  byKey.handle = 0x111u;
  byKey.handleKey = "WIDE-111";
  byKey.payload = {0x11u};
  storage.records.push_back(byKey);
  storage.recordIndexByHandle.emplace(byKey.handle, 0u);
  storage.recordIndexByHandleKey.emplace(byKey.handleKey, 0u);

  DRW_DataStorageRecord byNumber;
  byNumber.handle = 0x222u;
  byNumber.handleKey = "WIDE-222";
  byNumber.payload = {0x22u};
  storage.records.push_back(byNumber);
  storage.recordIndexByHandle.emplace(byNumber.handle, 1u);
  storage.recordIndexByHandleKey.emplace(byNumber.handleKey, 1u);
  reader.m_dataStorageSections.push_back(storage);

  DRW_ModelerGeometry keyed(DRW::REGION);
  keyed.handle = 0x333u;
  keyed.dataStorageHandle = byNumber.handle;
  keyed.dataStorageHandleKey = byKey.handleKey;
  DrwDataStorageTestAccess::setDataStorageFlag(keyed, true);
  reader.linkDataStorage(keyed);
  CHECK_FALSE(keyed.hasDataStorageRecord);
  CHECK(reader.m_dataStorageLinkFailures == 1u);

  DRW_ModelerGeometry keyOnly(DRW::REGION);
  keyOnly.handle = 0x333u;
  keyOnly.dataStorageHandleKey = byKey.handleKey;
  DrwDataStorageTestAccess::setDataStorageFlag(keyOnly, true);
  reader.linkDataStorage(keyOnly);
  REQUIRE(keyOnly.hasDataStorageRecord);
  CHECK(keyOnly.dataStorageHandle == byKey.handle);
  CHECK(keyOnly.dataStorageHandleKey == byKey.handleKey);
  CHECK(keyOnly.dataStorageData == std::vector<std::uint8_t>({0x11u}));

  DRW_ModelerGeometry numeric(DRW::REGION);
  numeric.handle = 0x444u;
  numeric.dataStorageHandle = byNumber.handle;
  DrwDataStorageTestAccess::setDataStorageFlag(numeric, true);
  reader.linkDataStorage(numeric);
  REQUIRE(numeric.hasDataStorageRecord);
  CHECK(numeric.dataStorageHandle == byNumber.handle);
  CHECK(numeric.dataStorageData == std::vector<std::uint8_t>({0x22u}));
  CHECK(reader.m_dataStorageLinkFailures == 1u);

  DRW_ModelerGeometry missingKey(DRW::REGION);
  missingKey.handle = 0x555u;
  missingKey.dataStorageHandle = byNumber.handle;
  missingKey.dataStorageHandleKey = "MISSING";
  DrwDataStorageTestAccess::setDataStorageFlag(missingKey, true);
  reader.linkDataStorage(missingKey);
  CHECK_FALSE(missingKey.hasDataStorageRecord);
  CHECK(reader.m_dataStorageLinkFailures == 2u);
}

TEST_CASE("DWG DataStorage linkage rejects duplicate sections and claims",
          "[dwg][datastorage][linkage][safety]") {
  DwgDataStorageReaderProbe reader;
  reader.setVersionForTest(DRW::AC1027);

  DRW_DataStorageSection first;
  first.m_version = DRW::AC1027;
  DRW_DataStorageRecord record;
  record.handle = 0x1234u;
  record.handleKey = "1234";
  record.payload = {0x01u};
  first.records.push_back(record);
  first.recordIndexByHandle.emplace(record.handle, 0u);
  first.recordIndexByHandleKey.emplace(record.handleKey, 0u);
  reader.m_dataStorageSections.push_back(first);
  reader.m_dataStorageSections.push_back(first);

  DRW_ModelerGeometry ambiguous(DRW::REGION);
  ambiguous.handle = record.handle;
  DrwDataStorageTestAccess::setDataStorageFlag(ambiguous, true);
  reader.linkDataStorage(ambiguous);
  CHECK_FALSE(ambiguous.hasDataStorageRecord);
  CHECK(reader.m_dataStorageLinkFailures == 1u);

  reader.m_dataStorageSections.pop_back();
  DRW_ModelerGeometry owner(DRW::REGION);
  owner.handle = record.handle;
  DrwDataStorageTestAccess::setDataStorageFlag(owner, true);
  reader.linkDataStorage(owner);
  REQUIRE(owner.hasDataStorageRecord);
  CHECK(reader.m_dataStorageLinkFailures == 1u);

  DRW_ModelerGeometry duplicateClaim(DRW::REGION);
  duplicateClaim.handle = 0x9999u;
  duplicateClaim.dataStorageHandle = record.handle;
  DrwDataStorageTestAccess::setDataStorageFlag(duplicateClaim, true);
  reader.linkDataStorage(duplicateClaim);
  CHECK_FALSE(duplicateClaim.hasDataStorageRecord);
  CHECK(reader.m_dataStorageLinkFailures == 2u);
}
