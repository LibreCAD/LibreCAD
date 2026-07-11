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
#include <string>
#include <sstream>
#include <vector>
#include "drw_dbg.h"
#include "dwgreader15.h"
#include "drw_textcodec.h"
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
    if (const char* cpName = dwgCodePageName(cp))
        decoder.setCodePage(cpName, false);
    return true;
}

bool dwgReader15::readFileHeader() {
    bool ret = true;
    DRW_DBG("dwgReader15::readFileHeader\n");
    if (! fileBuf->setPosition(21))
        return false;
    std::uint32_t count = fileBuf->getRawLong32();
    DRW_DBG("count records= "); DRW_DBG(count); DRW_DBG("\n");

    for (unsigned int i = 0; i < count; i++) {
        std::uint8_t rec = fileBuf->getRawChar8();
        std::uint32_t address = fileBuf->getRawLong32();
        std::uint32_t size = fileBuf->getRawLong32();
        dwgSectionInfo si;
        si.Id = rec;
        si.size = size;
        si.address = address;
        if (rec == 0) {
            DRW_DBG("\nSection HEADERS address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            sections[secEnum::HEADER] = si;
        } else if (rec == 1) {
            DRW_DBG("\nSection CLASSES address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            sections[secEnum::CLASSES] = si;
        } else if (rec == 2) {
            DRW_DBG("\nSection OBJECTS (handles) address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            sections[secEnum::HANDLES] = si;
        } else if (rec == 3) {
            DRW_DBG("\nSection UNKNOWN address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            sections[secEnum::UNKNOWNS] = si;
        } else if (rec == 4) {
            DRW_DBG("\nSection R14DATA (AcDb:Template) address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            sections[secEnum::TEMPLATE] = si;
        } else if (rec == 5) {
            DRW_DBG("\nSection R14REC5 (AcDb:AuxHeader) address= ");
            DRW_DBG(address); DRW_DBG(" size= "); DRW_DBG(size);
            sections[secEnum::AUXHEADER] = si;
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
    DRW_DBG("\nfile header CRC= "); DRW_DBG(fileBuf->getRawShort16());
    DRW_DBG("\nfile header sentinel= ");
    checkSentinel(fileBuf.get(), secEnum::FILEHEADER, false);

    DRW_DBG("\nposition after read file header sentinel= "); DRW_DBG(fileBuf->getPosition());
    DRW_DBG(", bit are= "); DRW_DBG(fileBuf->getBitPos());

    DRW_DBG("\ndwgReader15::readFileHeader END\n");
    return ret;
}

bool dwgReader15::readDwgHeader(DRW_Header& hdr){
    DRW_DBG("dwgReader15::readDwgHeader\n");
    dwgSectionInfo si = sections[secEnum::HEADER];
    if (si.Id<0)//not found, ends
        return false;
    if (!fileBuf->setPosition(si.address))
        return false;
    std::vector<std::uint8_t> tmpByteStr(si.size);
    fileBuf->getBytes(tmpByteStr.data(), si.size);
    dwgBuffer buff(tmpByteStr.data(), si.size, &decoder);
    DRW_DBG("Header section sentinel= ");
    checkSentinel(&buff, secEnum::HEADER, true);
    bool ret = dwgReader::readDwgHeader(hdr, &buff, &buff);
    return ret;
}


bool dwgReader15::readDwgClasses(){
    DRW_DBG("\ndwgReader15::readDwgClasses\n");
    dwgSectionInfo si = sections[secEnum::CLASSES];
    if (si.Id<0)//not found, ends
        return false;
    if (!fileBuf->setPosition(si.address))
        return false;

    DRW_DBG("classes section sentinel= ");
    checkSentinel(fileBuf.get(), secEnum::CLASSES, true);

    std::uint32_t size = fileBuf->getRawLong32();
    if (size != (si.size - 38)) {
        DRW_DBG("\nWARNING dwgReader15::readDwgClasses size are "); DRW_DBG(size);
        DRW_DBG(" and secSize - 38 are "); DRW_DBG(si.size - 38); DRW_DBG("\n");
    }
    const std::uint32_t classDataSize = size;  // 1.5a: preserve before the -- below
    std::vector<std::uint8_t> tmpByteStr(size);
    fileBuf->getBytes(tmpByteStr.data(), size);
    dwgBuffer buff(tmpByteStr.data(), size, &decoder);
    size--; //reduce 1 byte instead of check pos + bitPos
    while (size > buff.getPosition()) {
        DRW_Class *cl = new DRW_Class();
        if (!cl->parseDwg(version, &buff, &buff)) {
            delete cl;
            return false;
        }
        classesmap[cl->classNum] = cl;
    }
     // 1.5a: validate the R13/R15 CLASSES CRC (crc16 0xC0C1). The writer
     // (dwgwriter15.cpp) covers [sectionStart+16, end) = the RL size field
     // (4 bytes) + class data, matching libreDWG's [address+16, address+
     // size-18]. Here that is [si.address+16, si.address+20+classDataSize].
     // crc8 saves/restores fileBuf's position, so it is safe to call before
     // reading the stored CRC. The 1.1 negative-range guard protects against
     // a corrupt size. (crc8 returns 0 on a read failure; treat that as a
     // mismatch only when the stored CRC is non-zero.)
     std::uint16_t crcCalc = fileBuf->crc8(0xc0c1,
                                     static_cast<std::int32_t>(si.address + 16),
                                     static_cast<std::int32_t>(si.address + 20 + classDataSize));
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
         DRW_DBG("\nWARNING dwgReader15::readDwgClasses CRC mismatch: calc=");
         DRW_DBGH(crcCalc); DRW_DBG(" read="); DRW_DBGH(crcRead); DRW_DBG("\n");
     }
     DRW_DBG("\nclasses section end sentinel= ");
     // 1.4: honor the END sentinel (fail on mismatch). The BEGIN sentinel
     // (:145) and the CRC above stay warn-only to tolerate benign drift.
     bool endOk = checkSentinel(fileBuf.get(), secEnum::CLASSES, false);
     return buff.isGood() && endOk;
}

bool dwgReader15::readDwgHandles() {
    DRW_DBG("\ndwgReader15::readDwgHandles\n");
    dwgSectionInfo si = sections[secEnum::HANDLES];
    if (si.Id<0)//not found, ends
        return false;

    bool ret = dwgReader::readDwgHandles(fileBuf.get(), si.address, si.size);
    return ret;
}

/*********** objects ************************/
/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader15::readDwgTables(DRW_Header& hdr) {
    bool ret = dwgReader::readDwgTables(hdr, fileBuf.get());

    return ret;
}

/**
 * Reads all the object referenced in the object map section of the DWG file
 * (using their object file offsets)
 */
bool dwgReader15::readDwgBlocks(DRW_Interface& intfa) {
    bool ret = true;
    ret = dwgReader::readDwgBlocks(intfa, fileBuf.get());
    return ret;
}
