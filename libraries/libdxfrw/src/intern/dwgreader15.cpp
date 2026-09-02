/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "drw_dbg.h"
#include "drw_reserve.h"
#include "dwgreader15.h"
#include "drw_textcodec.h"
#include "dwgsafety.h"
#include "../libdwgr.h"

bool dwgReader15::readMetaData() {
    version = parent->getVersion();
    decoder.setVersion(version, false);
    DRW_DBG("dwgReader15::readMetaData\n");
    if (! fileBuf->setPosition(13))
        return false;
    previewImagePos = fileBuf->getRawLong32();
    DRW_DBG("previewImagePos (seekerImageData) = "); DRW_DBG(previewImagePos);
    /* MEASUREMENT system variable 2 bytes*/
    std::uint16_t meas = fileBuf->getRawShort16();
    DRW_DBG("\nMEASUREMENT (0 = English, 1 = Metric)= "); DRW_DBG(meas);
    std::uint16_t cp = fileBuf->getRawShort16();
    DRW_DBG("\ncodepage= "); DRW_DBG(cp); DRW_DBG("\n");
    if (const char* cpName = dwgCodePageName(cp)) {
        decoder.setCodePage(cpName, false);
        decoder.setByteCodePage(cpName);
    }
    return fileBuf->isGood();
}

bool dwgReader15::readFileHeader() {
    DRW_DBG("dwgReader15::readFileHeader\n");
    sections.clear();
    if (! fileBuf->setPosition(21))
        return false;
    std::uint32_t count = fileBuf->getRawLong32();
    if (!fileBuf->isGood())
        return false;
    if (count < 3 || count > 6)
        return false;
    std::uint64_t recordsSize = 0;
    const std::uint64_t remainingSize = fileBuf->size() - fileBuf->getPosition();
    if (!dwgSafety::multiply(count, 9, recordsSize)
        || recordsSize > remainingSize
        || remainingSize - recordsSize < 18)
        return false;
    DRW_DBG("count records= "); DRW_DBG(count); DRW_DBG("\n");

    std::unordered_map<int, dwgSectionInfo> stagedSections;
    std::unordered_set<int> seenRecordIds;
    for (unsigned int i = 0; i < count; i++) {
        std::uint8_t rec = fileBuf->getRawChar8();
        std::uint32_t address = fileBuf->getRawLong32();
        std::uint32_t size = fileBuf->getRawLong32();
        if (!fileBuf->isGood()
            || !dwgSafety::range(address, size, fileBuf->size())
            || !seenRecordIds.emplace(rec).second)
            return false;
        dwgSectionInfo si;
        si.Id = rec;
        si.size = size;
        si.address = address;
        if (rec == 0) {
            DRW_DBG("\nSection HEADERS address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            stagedSections[secEnum::HEADER] = si;
        } else if (rec == 1) {
            DRW_DBG("\nSection CLASSES address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            stagedSections[secEnum::CLASSES] = si;
        } else if (rec == 2) {
            DRW_DBG("\nSection OBJECTS (handles) address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            stagedSections[secEnum::HANDLES] = si;
        } else if (rec == 3) {
            DRW_DBG("\nSection UNKNOWN address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            stagedSections[secEnum::UNKNOWNS] = si;
        } else if (rec == 4) {
            DRW_DBG("\nSection R14DATA (AcDb:Template) address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            stagedSections[secEnum::TEMPLATE] = si;
        } else if (rec == 5) {
            DRW_DBG("\nSection R14REC5 (AcDb:AuxHeader) address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            stagedSections[secEnum::AUXHEADER] = si;
        } else {
            std::cerr << "\nUnsupported section number\n";
        }
    }
    if (! fileBuf->isGood())
        return false;
    DRW_DBG("\nposition after read section locator records= "); DRW_DBG(fileBuf->getPosition());
    DRW_DBG(", bit are= "); DRW_DBG(fileBuf->getBitPos());
    std::uint32_t ckcrc = fileBuf->crc8(0,0,fileBuf->getPosition());
    DRW_DBG("\nfile header crc8 0 result= "); DRW_DBG(ckcrc);
    switch (count){
    case 3:
        ckcrc = ckcrc ^ 0xA598;
        break;
    case 4:
        ckcrc = ckcrc ^ 0x8101;
        break;
    case 5:
        ckcrc = ckcrc ^ 0x3CC4;
        break;
    case 6:
        ckcrc = ckcrc ^ 0x8461;
    }
    DRW_DBG("\nfile header crc8 xor result= "); DRW_DBG(ckcrc);
    const std::uint16_t storedCrc = fileBuf->getRawShort16();
    DRW_DBG("\nfile header CRC= "); DRW_DBG(storedCrc);
    DRW_DBG("\nfile header sentinel= ");
    if (!fileBuf->isGood() || ckcrc != storedCrc
        || !checkSentinel(fileBuf.get(), secEnum::FILEHEADER, false)
        || !fileBuf->isGood())
        return false;

    DRW_DBG("\nposition after read file header sentinel= "); DRW_DBG(fileBuf->getPosition());
    DRW_DBG(", bit are= "); DRW_DBG(fileBuf->getBitPos());

    sections = std::move(stagedSections);
    DRW_DBG("\ndwgReader15::readFileHeader END\n");
    return true;
}

bool dwgReader15::readDwgHeader(DRW_Header& hdr){
    DRW_DBG("dwgReader15::readDwgHeader\n");
    const auto sectionIt = sections.find(secEnum::HEADER);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0)
        return false;
    const dwgSectionInfo& si = sectionIt->second;
    if (si.size == 0 || si.size > dwgSafety::MaxBufferSize
        || si.size > std::numeric_limits<std::size_t>::max()
        || !dwgSafety::range(si.address, si.size, fileBuf->size())
        || !fileBuf->setPosition(si.address))
        return false;
    std::vector<std::uint8_t> tmpByteStr;
    if (!DRW::resize(tmpByteStr, static_cast<int>(si.size)))
        return false;
    if (!fileBuf->getBytes(tmpByteStr.data(), si.size))
        return false;
    dwgBuffer buff(tmpByteStr.data(), si.size, &decoder);
    DRW_DBG("Header section sentinel= ");
    if (!checkSentinel(&buff, secEnum::HEADER, true))
        return false;
    bool ret = dwgReader::readDwgHeader(hdr, &buff, &buff);
    return ret;
}


bool dwgReader15::readDwgClasses(){
    DRW_DBG("\ndwgReader15::readDwgClasses\n");
    beginDwgClassCoverage();
    const auto sectionIt = sections.find(secEnum::CLASSES);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0)
        return false;
    const dwgSectionInfo& si = sectionIt->second;
    if (si.size < 38
        || !dwgSafety::range(si.address, si.size, fileBuf->size()))
        return false;
    if (!fileBuf->setPosition(si.address))
        return false;

    DRW_DBG("classes section sentinel= ");
    if (!checkSentinel(fileBuf.get(), secEnum::CLASSES, true))
        return false;

    std::uint32_t size = fileBuf->getRawLong32();
    if (!fileBuf->isGood() || size > dwgSafety::MaxBufferSize)
        return false;
    const std::uint64_t dataStart = fileBuf->getPosition();
    std::uint64_t classDataBaseBit = 0;
    if (!dwgSafety::multiply(dataStart, 8, classDataBaseBit))
        return false;
    std::uint64_t dataAndTrailerSize = 0;
    if (!dwgSafety::add(size, 18, dataAndTrailerSize)
        || !dwgSafety::range(dataStart, dataAndTrailerSize, fileBuf->size()))
        return false;
    if (size != (si.size - 38)) {
        DRW_DBG("\nWARNING dwgReader15::readDwgClasses size are "); DRW_DBG(size);
        DRW_DBG(" and secSize - 38 are "); DRW_DBG(si.size - 38); DRW_DBG("\n");
    }
    const std::uint32_t classDataSize = size;  // 1.5a: preserve before the -- below
    std::vector<std::uint8_t> tmpByteStr;
    if (!DRW::resize(tmpByteStr, static_cast<int>(size)))
        return false;
    if (!fileBuf->getBytes(tmpByteStr.data(), size))
        return false;
    dwgBuffer buff(tmpByteStr.data(), size, &decoder);
    std::vector<DwgStagedClass> stagedClasses;
    std::unordered_map<std::uint32_t, DRW_Class*> stagedMap;
    if (size > 0)
        --size; //reduce 1 byte instead of check pos + bitPos
    while (size > buff.getPosition()) {
        std::uint64_t dataRangeStart = 0;
        if (!dwgClassBitPosition(buff, dataRangeStart)) {
            DRW_DwgClassCoverageEntry failed;
            failed.m_sectionDescriptorId = si.Id;
            recordDwgClassCoverageFailure(
                std::move(failed), DRW_DwgClassCoverageReason::Bounds);
            return false;
        }
        auto cl = std::make_unique<DRW_Class>();
        const bool parsed = cl->parseDwg(version, &buff, &buff);
        std::uint64_t dataRangeEnd = 0;
        const bool cursorValid = parsed
            && dwgClassBitPosition(buff, dataRangeEnd);
        if (!parsed || !buff.isGood() || !cursorValid) {
            DRW_DwgClassCoverageEntry failed;
            failed.m_sectionDescriptorId = si.Id;
            recordDwgClassCoverageFailure(
                std::move(failed), DRW_DwgClassCoverageReason::Parse);
            return false;
        }
        DRW_DwgClassCoverageEntry coverage;
        try {
            coverage = makeDwgClassCoverageEntry(*cl, si.Id);
        } catch (...) {
            m_dwgClassCoverageCaptureFailed = true;
            return false;
        }
        std::uint64_t physicalStart = 0;
        std::uint64_t physicalEnd = 0;
        if (!dwgSafety::add(classDataBaseBit, dataRangeStart, physicalStart)
            || !dwgSafety::add(classDataBaseBit, dataRangeEnd, physicalEnd)
            || !setDwgClassBitRange(
                coverage.m_dataRange, physicalStart, physicalEnd,
                DRW_DwgFrameOffsetSpace::PhysicalFile, false)) {
            recordDwgClassCoverageFailure(
                std::move(coverage), DRW_DwgClassCoverageReason::Bounds);
            return false;
        }
        coverage.m_stringRange = coverage.m_dataRange;
        if (cl->classNum < 500
            || classesmap.find(cl->classNum) != classesmap.end()
            || stagedMap.find(cl->classNum) != stagedMap.end()) {
            recordDwgClassCoverageFailure(
                std::move(coverage), DRW_DwgClassCoverageReason::Duplicate);
            return false;
        }
        try {
            stagedMap.emplace(cl->classNum, cl.get());
        } catch (...) {
            recordDwgClassCoverageFailure(
                std::move(coverage), DRW_DwgClassCoverageReason::Publish);
            return false;
        }
        if (!stageDwgClass(stagedClasses, std::move(cl), std::move(coverage)))
            return false;
    }
     // 1.5a: validate the R13/R15 CLASSES CRC (crc16 0xC0C1). The writer
     // (dwgwriter15.cpp) covers [sectionStart+16, end) = the RL size field
     // (4 bytes) + class data, matching libreDWG's [address+16, address+
     // size-18]. Here that is [si.address+16, si.address+20+classDataSize].
     // crc8 saves/restores fileBuf's position, so it is safe to call before
     // reading the stored CRC. The 1.1 negative-range guard protects against
     // a corrupt size. (crc8 returns 0 on a read failure; treat that as a
     // mismatch only when the stored CRC is non-zero.)
     std::uint64_t crcStart = 0;
     std::uint64_t crcEnd = 0;
     if (!dwgSafety::add(si.address, 16, crcStart)
         || !dwgSafety::add(si.address, 20, crcEnd)
         || !dwgSafety::add(crcEnd, classDataSize, crcEnd)
         || crcEnd > static_cast<std::uint64_t>(
                std::numeric_limits<std::int32_t>::max())
         || crcStart > static_cast<std::uint64_t>(
                std::numeric_limits<std::int32_t>::max()))
         return false;
     std::uint16_t crcCalc = fileBuf->crc8(0xc0c1,
                                     static_cast<std::int32_t>(crcStart),
                                     static_cast<std::int32_t>(crcEnd));
     std::uint16_t crcRead = fileBuf->getRawShort16();
     bool crcOk = (crcCalc == crcRead);
     if (!crcOk) {
         // WARN-ONLY: a CLASSES CRC mismatch is non-fatal — failing it discarded
         // the WHOLE drawing (processDwg short-circuits all later sections) for a
         // single drifted byte from a deviating third-party writer, inconsistent
         // with the warn-only BEGIN sentinel (:145). The class-number map still
         // parsed; track the mismatch as a diagnostic instead. (crc8 returns 0 on
         // a stream read failure; a spurious 0==0 match there is acceptable for a
         // diagnostic-only counter.)
         ++m_classesCrcMismatch;
         try {
             DwgIntegrityDiagnostic diagnostic;
             diagnostic.severity = DwgIntegritySeverity::Warning;
             diagnostic.offsetSpace = DwgIntegrityAddressSpace::PhysicalFile;
             diagnostic.phase = DwgIntegrityPhase::SectionParser;
             diagnostic.kind = DwgIntegrityCheckKind::ClassesCrc;
             diagnostic.logicalSectionId = secEnum::CLASSES;
             diagnostic.sectionDescriptorId = si.Id;
             diagnostic.sectionName = "AcDb:Classes";
             diagnostic.fileOffset = crcStart;
             diagnostic.hasFileOffset = true;
             diagnostic.expected = crcRead;
             diagnostic.observed = crcCalc;
             diagnostic.hasExpected = true;
             diagnostic.hasObserved = true;
             addIntegrityDiagnostic(std::move(diagnostic));
         } catch (...) {
             // Integrity reporting is best-effort and must not alter parsing.
         }
         DRW_DBG("\nWARNING dwgReader15::readDwgClasses CRC mismatch: calc=");
         DRW_DBGH(crcCalc); DRW_DBG(" read="); DRW_DBGH(crcRead); DRW_DBG("\n");
     }
     DRW_DBG("\nclasses section end sentinel= ");
     // 1.4: honor the END sentinel (fail on mismatch). The BEGIN sentinel
     // (:145) and the CRC above stay warn-only to tolerate benign drift.
     bool endOk = checkSentinel(fileBuf.get(), secEnum::CLASSES, false);
     if (!buff.isGood() || !endOk)
         return false;
     return publishDwgClasses(stagedClasses);
}

bool dwgReader15::readDwgHandles() {
    DRW_DBG("\ndwgReader15::readDwgHandles\n");
    const auto sectionIt = sections.find(secEnum::HANDLES);
    if (sectionIt == sections.end() || sectionIt->second.Id < 0)
        return false;
    const dwgSectionInfo& si = sectionIt->second;

    bool ret = dwgReader::readDwgHandles(
        fileBuf.get(), si.address, si.size, fileBuf->size(),
        DwgIntegrityAddressSpace::PhysicalFile, si.Id);
    return ret;
}

/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader15::readDwgTables(DRW_Header& hdr) {
    bool ret = dwgReader::readDwgTables(
        hdr, fileBuf.get(), DwgIntegrityAddressSpace::PhysicalFile);

    return ret;
}

/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader15::readDwgBlocks(DRW_Interface& intfa) {
    bool ret = true;
    ret = dwgReader::readDwgBlocks(
        intfa, fileBuf.get(), DwgIntegrityAddressSpace::PhysicalFile);
    return ret;
}
