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

#include <sstream>
#include "drw_dbg.h"
#include "dwgutil.h"
#include "rscodec.h"
#include "../libdwgr.h"

// Section sentinel byte sequences. Values verified against libreDWG
// common.c:100-177 and the ODA Open Design Specification v5.4.1.
// Each pair of BEGIN/END is the byte-wise XOR-0xFF of the other.
namespace dwgSentinels {
    const std::uint8_t FILE_HEADER_END[16] = {
        0x95, 0xA0, 0x4E, 0x28, 0x99, 0x82, 0x1A, 0xE5,
        0x5E, 0x41, 0xE0, 0x5F, 0x9D, 0x3A, 0x4D, 0x00
    };
    const std::uint8_t HEADER_BEGIN[16] = {
        0xCF, 0x7B, 0x1F, 0x23, 0xFD, 0xDE, 0x38, 0xA9,
        0x5F, 0x7C, 0x68, 0xB8, 0x4E, 0x6D, 0x33, 0x5F
    };
    const std::uint8_t HEADER_END[16] = {
        0x30, 0x84, 0xE0, 0xDC, 0x02, 0x21, 0xC7, 0x56,
        0xA0, 0x83, 0x97, 0x47, 0xB1, 0x92, 0xCC, 0xA0
    };
    const std::uint8_t CLASSES_BEGIN[16] = {
        0x8D, 0xA1, 0xC4, 0xB8, 0xC4, 0xA9, 0xF8, 0xC5,
        0xC0, 0xDC, 0xF4, 0x5F, 0xE7, 0xCF, 0xB6, 0x8A
    };
    const std::uint8_t CLASSES_END[16] = {
        0x72, 0x5E, 0x3B, 0x47, 0x3B, 0x56, 0x07, 0x3A,
        0x3F, 0x23, 0x0B, 0xA0, 0x18, 0x30, 0x49, 0x75
    };
    const std::uint8_t PREVIEW_BEGIN[16] = {
        0x1F, 0x25, 0x6D, 0x07, 0xD4, 0x36, 0x28, 0x28,
        0x9D, 0x57, 0xCA, 0x3F, 0x9D, 0x44, 0x10, 0x2B
    };
    const std::uint8_t PREVIEW_END[16] = {
        0xE0, 0xDA, 0x92, 0xF8, 0x2B, 0xC9, 0xD7, 0xD7,
        0x62, 0xA8, 0x35, 0xC0, 0x62, 0xBB, 0xEF, 0xD4
    };
    const std::uint8_t SECOND_HEADER_BEGIN[16] = {
        0xD4, 0x7B, 0x21, 0xCE, 0x28, 0x93, 0x9F, 0xBF,
        0x53, 0x24, 0x40, 0x09, 0x12, 0x3C, 0xAA, 0x01
    };
    const std::uint8_t SECOND_HEADER_END[16] = {
        0x2B, 0x84, 0xDE, 0x31, 0xD7, 0x6C, 0x60, 0x40,
        0xAC, 0xDB, 0xBF, 0xF6, 0xED, 0xC3, 0x55, 0xFE
    };
}

namespace dwgVersionString {
    const char R12[6]   = {'A','C','1','0','0','9'};
    const char R13[6]   = {'A','C','1','0','1','2'};
    const char R14[6]   = {'A','C','1','0','1','4'};
    const char R2000[6] = {'A','C','1','0','1','5'};
    const char R2004[6] = {'A','C','1','0','1','8'};
    const char R2007[6] = {'A','C','1','0','2','1'};
    const char R2010[6] = {'A','C','1','0','2','4'};
    const char R2013[6] = {'A','C','1','0','2','7'};
    const char R2018[6] = {'A','C','1','0','3','2'};
}

/** utility function
 * convert a int to string in hex
 **/
namespace DRW {
std::string toHexStr(int n){
#if defined(__APPLE__)
    char buffer[9]= {'\0'};
    snprintf(buffer,9, "%X", n);
    return std::string(buffer);
#else
    std::ostringstream Convert;
    Convert << std::uppercase << std::hex << n;
    return Convert.str();
#endif
}
}

/**
 * @brief dwgRSCodec::decode239I
 * @param in : input data (at least 255*blk bytes)
 * @param out : output data (at least 239*blk bytes)
 * @param blk number of codewords ( 1 cw == 255 bytes)
 */
bool dwgRSCodec::decode239I(unsigned char *in, unsigned char *out, std::uint32_t blk){
    int k=0;
    bool allOk = true;
    unsigned char data[255];
    RScodec rsc(0x96, 8, 8); //(255, 239)
    for (std::uint32_t i=0; i<blk; i++){
        k = i;
        for (int j=0; j<255; j++) {
            data[j] = in[k];
            k +=blk;
        }
        int r = rsc.decode(data);
        if (r<0) {
            DRW_DBG("\nWARNING: dwgRSCodec::decode239I, can't correct all errors");
            allOk = false;
        }
        k = i*239;
        for (int j=0; j<239; j++) {
            out[k++] = data[j];
        }
    }
    return allOk;
}

/**
 * @brief dwgRSCodec::decode251I
 * @param in : input data (at least 255*blk bytes)
 * @param out : output data (at least 251*blk bytes)
 * @param blk number of codewords ( 1 cw == 255 bytes)
 */
bool dwgRSCodec::decode251I(unsigned char *in, unsigned char *out, std::uint32_t blk){
    int k=0;
    bool allOk = true;
    unsigned char data[255];
    RScodec rsc(0xB8, 8, 2); //(255, 251)
    for (std::uint32_t i=0; i<blk; i++){
        k = i;
        for (int j=0; j<255; j++) {
            data[j] = in[k];
            k +=blk;
        }
        int r = rsc.decode(data);
        if (r<0) {
            DRW_DBG("\nWARNING: dwgRSCodec::decode251I, can't correct all errors");
            allOk = false;
        }
        k = i*251;
        for (int j=0; j<251; j++) {
            out[k++] = data[j];
        }
    }
    return allOk;
}

std::uint8_t *dwgCompressor::compressedBuffer {nullptr};
std::uint32_t dwgCompressor::compressedSize {0};
std::uint32_t dwgCompressor::compressedPos {0};
bool    dwgCompressor::compressedGood {true};
std::uint8_t *dwgCompressor::decompBuffer {nullptr};
std::uint32_t dwgCompressor::decompSize {0};
std::uint32_t dwgCompressor::decompPos {0};
bool    dwgCompressor::decompGood {true};

std::uint32_t dwgCompressor::twoByteOffset(std::uint32_t *ll){
    std::uint32_t cont = 0;
    std::uint8_t fb = compressedByte();
    cont = (fb >> 2) | (compressedByte() << 6);
    *ll = (fb & 0x03);
    return cont;
}

std::uint32_t dwgCompressor::longCompressionOffset(){
    std::uint32_t cont = 0;
    std::uint8_t ll = compressedByte();
    while (ll == 0x00 && compressedGood) {
        cont += 0xFF;
        ll = compressedByte();
    }
    cont += ll;
    return cont;
}

std::uint32_t dwgCompressor::long20CompressionOffset(){
//    std::uint32_t cont = 0;
    std::uint32_t cont = 0x0F;
    std::uint8_t ll = compressedByte();
    while (ll == 0x00 && compressedGood){
//        cont += 0xFF;
        ll = compressedByte();
    }
    cont += ll;
    return cont;
}

std::uint32_t dwgCompressor::litLength18(){
    std::uint32_t cont = 0;
    std::uint8_t ll = compressedByte();
    //no literal length, this byte is next opCode
    if (ll > 0x0F) {
        --compressedPos;
        return 0;
    }

    if (ll == 0x00) {
        cont = 0x0F;
        ll = compressedByte();
        while (ll == 0x00 && compressedGood) {//repeat until ll != 0x00
            cont += 0xFF;
            ll = compressedByte();
        }
    }

    return cont + ll + 3;
}

bool dwgCompressor::decompress18(std::uint8_t *cbuf, std::uint8_t *dbuf, std::uint64_t csize, std::uint64_t dsize){
    compressedBuffer = cbuf;
    decompBuffer = dbuf;
    compressedSize = csize;
    decompSize = dsize;
    compressedPos = 0;
    decompPos = 0;
    compressedGood = true;
    decompGood = true;

    // A <2-byte compressed page cannot hold the minimum opcode stream and the
    // trailing-byte peek below would read out of bounds (compressedSize is a
    // std::uint32_t, so compressedSize-2 underflows to a huge index). Fail the page;
    // callers (parseDataPage/parseSysPage) propagate false as a read failure.
    if (compressedBuffer == nullptr || compressedSize < 2)
        return false;

    DRW_DBG("dwgCompressor::decompress, last 2 bytes: ");
    DRW_DBGH(compressedBuffer[compressedSize - 2]);DRW_DBG(" ");DRW_DBGH(compressedBuffer[compressedSize - 1]);DRW_DBG("\n");

    std::uint32_t compBytes {0};
    std::uint32_t compOffset {0};
    std::uint32_t litCount {litLength18()};

    //copy first literal length
    for (std::uint32_t i = 0; i < litCount && buffersGood(); ++i) {
        decompSet( compressedByte());
    }

    while (buffersGood()) {
        std::uint8_t oc = compressedByte(); //next opcode
        if (oc == 0x10){
            compBytes = longCompressionOffset()+ 9;
            compOffset = twoByteOffset(&litCount) + 0x3FFF;
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc > 0x11 && oc< 0x20){
            compBytes = (oc & 0x0F) + 2;
            compOffset = twoByteOffset(&litCount) + 0x3FFF;
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc == 0x20){
            compBytes = longCompressionOffset() + 0x21;
            compOffset = twoByteOffset(&litCount);
            if (litCount == 0)
                litCount= litLength18();
        } else if (oc > 0x20 && oc< 0x40){
            compBytes = oc - 0x1E;
            compOffset = twoByteOffset(&litCount);
            if (litCount == 0)
                litCount= litLength18();
        } else if ( oc > 0x3F){
            compBytes = ((oc & 0xF0) >> 4) - 1;
            std::uint8_t ll2 = compressedByte();
            compOffset =  (ll2 << 2) | ((oc & 0x0C) >> 2);
            litCount = oc & 0x03;
            if (litCount < 1){
                litCount= litLength18();}
        } else if (oc == 0x11){
            DRW_DBG("dwgCompressor::decompress, end of input stream, Cpos: ");
            DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG("\n");
            return true; //end of input stream
        } else { //ll < 0x10
            DRW_DBG("WARNING dwgCompressor::decompress, failed, illegal char: "); DRW_DBGH(oc);
            DRW_DBG(", Cpos: "); DRW_DBG(compressedPos);
            DRW_DBG(", Dpos: "); DRW_DBG(decompPos); DRW_DBG("\n");
            return false; //fails, not valid
        }

        //copy "compressed data", if size allows
        if (decompSize < decompPos + compBytes) {
            DRW_DBG("WARNING dwgCompressor::decompress18, bad compBytes size, Cpos: ");
            DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG(", need ");DRW_DBG(compBytes);DRW_DBG(", available ");DRW_DBG(decompSize - decompPos);DRW_DBG("\n");
            // only copy what we can fit
            compBytes = decompSize - decompPos;
        }
        std::uint32_t j {decompPos - compOffset - 1};
        for (std::uint32_t i = 0; i < compBytes && buffersGood(); i++) {
            decompSet( decompByte( j++));
        }

        //copy "uncompressed data", if size allows
        if (decompSize < decompPos + litCount) {
            DRW_DBG("WARNING dwgCompressor::decompress18, bad litCount size, Cpos: ");
            DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG(", need ");DRW_DBG(litCount);DRW_DBG(", available ");DRW_DBG(decompSize - decompPos);DRW_DBG("\n");
            // only copy what we can fit
            litCount = decompSize - decompPos;
        }
        for (std::uint32_t i=0; i < litCount && buffersGood(); i++) {
            decompSet( compressedByte());
        }
    }

    DRW_DBG("WARNING dwgCompressor::decompress, bad out, Cpos: ");DRW_DBG(compressedPos);DRW_DBG(", Dpos: ");DRW_DBG(decompPos);DRW_DBG("\n");
    return false;
}

std::uint8_t dwgCompressor::compressedByte(void)
{
    std::uint8_t result {0};

    compressedGood = (compressedPos < compressedSize);
    if (compressedGood) {
        result = compressedBuffer[compressedPos];
        ++compressedPos;
    }

    return result;
}

std::uint8_t dwgCompressor::compressedByte(const std::uint32_t index)
{
    if (index < compressedSize) {
        return compressedBuffer[index];
    }

    return 0;
}

std::uint32_t dwgCompressor::compressedHiByte(void)
{
    return static_cast<std::uint32_t>(compressedByte()) << 8;
}

bool dwgCompressor::compressedInc(const std::int32_t inc /*= 1*/)
{
    compressedPos += inc;
    compressedGood = (compressedPos <= compressedSize);

    return compressedGood;
}

std::uint8_t dwgCompressor::decompByte(const std::uint32_t index)
{
    if (index < decompSize) {
        return decompBuffer[index];
    }

    return 0;
}

void dwgCompressor::decompSet(const std::uint8_t value)
{
    decompGood = (decompPos < decompSize);
    if (decompGood) {
        decompBuffer[decompPos] = value;
        ++decompPos;
    }
}

bool dwgCompressor::buffersGood(void)
{
    return compressedGood && decompGood;
}

std::uint32_t dwgUtil::checksum18(std::uint32_t seed, const std::uint8_t* data, std::uint64_t sz) {
    std::uint32_t sum1 = seed & 0xffff;
    std::uint32_t sum2 = seed >> 0x10;
    while (sz != 0) {
        std::uint64_t chunk = sz < 0x15b0 ? sz : 0x15b0;
        sz -= chunk;
        for (std::uint64_t i = 0; i < chunk; ++i) {
            sum1 += *data++;
            sum2 += sum1;
        }
        sum1 %= 0xFFF1;
        sum2 %= 0xFFF1;
    }
    return (sum2 << 0x10) | (sum1 & 0xffff);
}

std::uint32_t dwgUtil::crc32(std::uint32_t seed, const std::uint8_t* data, std::uint32_t sz) {
    // Standard CRC-32/ISO-HDLC, polynomial 0xEDB88320 (matches dwgBuffer::crc32).
    static std::uint32_t kTable[256];
    static bool kInit = false;
    if (!kInit) {
        for (int i = 0; i < 256; ++i) {
            std::uint32_t c = static_cast<std::uint32_t>(i);
            for (int j = 0; j < 8; ++j)
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            kTable[i] = c;
        }
        kInit = true;
    }
    std::uint32_t crc = ~seed;
    while (sz-- > 0)
        crc = (crc >> 8) ^ kTable[(crc ^ *data++) & 0xFF];
    return ~crc;
}

void dwgCompressor::decrypt18Hdr(std::uint8_t *buf, std::uint64_t size, std::uint64_t offset){
    std::uint8_t max = size / 4;
    std::uint32_t secMask = 0x4164536b ^ offset;
    std::uint32_t* pHdr = reinterpret_cast<std::uint32_t*>(buf);
    for (std::uint8_t j = 0; j < max; j++)
        *pHdr++ ^= secMask;
}

/*void dwgCompressor::decrypt18Data(std::uint8_t *buf, std::uint32_t size, std::uint32_t offset){
    std::uint8_t max = size / 4;
    std::uint32_t secMask = 0x4164536b ^ offset;
    std::uint32_t* pHdr = (std::uint32_t*)buf;
    for (std::uint8_t j = 0; j < max; j++)
        *pHdr++ ^= secMask;
}*/

std::uint32_t dwgCompressor::litLength21(std::uint8_t opCode)
{
    std::uint32_t length = 8u + opCode;
    if (0x17 == length) {
        std::uint32_t n = compressedByte();
        length += n;
        if (0xffu == n) {
            do {
                n = compressedByte();
                n |= compressedHiByte();
                length += n;
            }
            while (0xffffu == n);
        }
    }

    return length;
}

bool dwgCompressor::decompress21(std::uint8_t *cbuf, std::uint8_t *dbuf, std::uint64_t csize, std::uint64_t dsize){
    compressedBuffer = cbuf;
    decompBuffer = dbuf;
    compressedSize = csize;
    decompSize = dsize;
    compressedPos = 0;
    decompPos = 0;
    compressedGood = true;
    decompGood = true;

    std::uint32_t length {0};
    std::uint32_t sourceOffset {0};
    std::uint8_t opCode {compressedByte()};
    if ((opCode >> 4) == 2){
        compressedInc( 2);
        length = compressedByte() & 0x07;
    }

    while (buffersGood()) {
        if (length == 0) {
            length = litLength21(opCode);
        }
        copyCompBytes21( length);

        if (decompPos >= decompSize) {
            break; //check if last chunk are compressed & terminate
        }

        length = 0;
        opCode = compressedByte();
        readInstructions21( opCode,  sourceOffset,  length);
        while (true) {
            //prevent crash with corrupted data
            if (sourceOffset > decompPos) {
                DRW_DBG("\nWARNING dwgCompressor::decompress21 => sourceOffset> dstIndex.\n");
                DRW_DBG("csize = "); DRW_DBG(compressedSize); DRW_DBG("  srcIndex = "); DRW_DBG(compressedPos);
                DRW_DBG("\ndsize = "); DRW_DBG(decompSize); DRW_DBG("  dstIndex = "); DRW_DBG(decompPos);
                sourceOffset = decompPos;
            }
            //prevent crash with corrupted data
            if (length > decompSize - decompPos){
                DRW_DBG("\nWARNING dwgCompressor::decompress21 => length > dsize - dstIndex.\n");
                DRW_DBG("csize = "); DRW_DBG(compressedSize); DRW_DBG("  srcIndex = "); DRW_DBG(compressedPos);
                DRW_DBG("\ndsize = "); DRW_DBG(decompSize); DRW_DBG("  dstIndex = "); DRW_DBG(decompPos);
                length = decompSize - decompPos;
                compressedPos = compressedSize; //force exit
                compressedGood = false;
            }
            sourceOffset = decompPos - sourceOffset;
            for (std::uint32_t i=0; i< length; i++)
                decompSet( decompByte( sourceOffset + i));

            length = opCode & 7;
            if ((length != 0) || (compressedPos >= compressedSize)) {
                break;
            }
            opCode = compressedByte();
            if ((opCode >> 4) == 0) {
                break;
            }
            if ((opCode >> 4) == 15) {
                opCode &= 15;
            }
            readInstructions21( opCode, sourceOffset, length);
        }

        if (compressedPos >= compressedSize) {
            break;
        }
    }
    DRW_DBG("\ncsize = "); DRW_DBG(compressedSize); DRW_DBG("  srcIndex = "); DRW_DBG(compressedPos);
    DRW_DBG("\ndsize = "); DRW_DBG(decompSize); DRW_DBG("  dstIndex = "); DRW_DBG(decompPos);DRW_DBG("\n");

    return buffersGood();
}

void dwgCompressor::readInstructions21(std::uint8_t &opCode, std::uint32_t &sourceOffset, std::uint32_t &length){

    switch (opCode >> 4) {
    case 0:
        length = (opCode & 0x0f) + 0x13;
        sourceOffset = compressedByte();
        opCode = compressedByte();
        length = ((opCode >> 3) & 0x10) + length;
        sourceOffset = ((opCode & 0x78) << 5) + 1 + sourceOffset;
        break;
    case 1:
        length = (opCode & 0xf) + 3;
        sourceOffset = compressedByte();
        opCode = compressedByte();
        sourceOffset = ((opCode & 0xf8) << 5) + 1 + sourceOffset;
        break;
    case 2:
        sourceOffset = compressedByte();
        sourceOffset = (compressedHiByte() & 0xff00) | sourceOffset;
        length = opCode & 7;
        if ((opCode & 8) == 0) {
            opCode = compressedByte();
            length = (opCode & 0xf8) + length;
        } else {
            ++sourceOffset;
            length = (static_cast<std::uint32_t>(compressedByte()) << 3) + length;
            opCode = compressedByte();
            length = (((opCode & 0xf8) << 8) + length) + 0x100;
        }
        break;
    default:
        length = opCode >> 4;
        sourceOffset = opCode & 15;
        opCode = compressedByte();
        sourceOffset = (((opCode & 0xf8) << 1) + sourceOffset) + 1;
        break;
    }
}

const std::uint8_t dwgCompressor::CopyOrder21_01[] = {0};
const std::uint8_t dwgCompressor::CopyOrder21_02[] = {1,0};
const std::uint8_t dwgCompressor::CopyOrder21_03[] = {2,1,0};
const std::uint8_t dwgCompressor::CopyOrder21_04[] = {0,1,2,3};
const std::uint8_t dwgCompressor::CopyOrder21_05[] = {4,0,1,2,3};
const std::uint8_t dwgCompressor::CopyOrder21_06[] = {5,1,2,3,4,0};
const std::uint8_t dwgCompressor::CopyOrder21_07[] = {6,5,1,2,3,4,0};
const std::uint8_t dwgCompressor::CopyOrder21_08[] = {0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_09[] = {8,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_10[] = {9,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_11[] = {10,9,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_12[] = {8,9,10,11,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_13[] = {12,8,9,10,11,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_14[] = {13,9,10,11,12,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_15[] = {14,13,9,10,11,12,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_16[] = {8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_17[] = {9,10,11,12,13,14,15,16,8,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_18[] = {17,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,0};
const std::uint8_t dwgCompressor::CopyOrder21_19[] = {18,17,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_20[] = {16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_21[] = {20,16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_22[] = {21,20,16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_23[] = {22,21,20,16,17,18,19,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_24[] = {16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_25[] = {17,18,19,20,21,22,23,24,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_26[] = {25,17,18,19,20,21,22,23,24,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_27[] = {26,25,17,18,19,20,21,22,23,24,16,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_28[] = {24,25,26,27,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_29[] = {28,24,25,26,27,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_30[] = {29,28,24,25,26,27,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t dwgCompressor::CopyOrder21_31[] = {30,26,27,28,29,18,19,20,21,22,23,24,25,10,11,12,13,14,15,16,17,2,3,4,5,6,7,8,9,1,0};
const std::uint8_t dwgCompressor::CopyOrder21_32[] = {24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7};
const std::uint8_t *dwgCompressor::CopyOrder21[dwgCompressor::Block21OrderArray] = {
        nullptr,
        CopyOrder21_01, CopyOrder21_02, CopyOrder21_03, CopyOrder21_04,
        CopyOrder21_05, CopyOrder21_06, CopyOrder21_07, CopyOrder21_08,
        CopyOrder21_09, CopyOrder21_10, CopyOrder21_11, CopyOrder21_12,
        CopyOrder21_13, CopyOrder21_14, CopyOrder21_15, CopyOrder21_16,
        CopyOrder21_17, CopyOrder21_18, CopyOrder21_19, CopyOrder21_20,
        CopyOrder21_21, CopyOrder21_22, CopyOrder21_23, CopyOrder21_24,
        CopyOrder21_25, CopyOrder21_26, CopyOrder21_27, CopyOrder21_28,
        CopyOrder21_29, CopyOrder21_30, CopyOrder21_31, CopyOrder21_32
};

void dwgCompressor::copyBlock21(const std::uint32_t length)
{
    if (MaxBlock21Length < length) {
        return;
    }

    const std::uint8_t *order {CopyOrder21[length]};
    if (nullptr == order) {
        return;
    }

    for (std::uint32_t index = 0; (length > index) && buffersGood(); ++index) {
        decompSet( compressedByte( compressedPos + order[index]));
    }
    compressedInc( length);
}

bool dwgCompressor::copyCompBytes21(std::uint32_t length)
{
    DRW_DBG("\ncopyCompBytes21() "); DRW_DBG(length); DRW_DBG("\n");

    while (length >= MaxBlock21Length) {
        copyBlock21( MaxBlock21Length);
        length -= MaxBlock21Length;
    }

    copyBlock21( length);

    return buffersGood();
}


secEnum::DWGSection secEnum::getEnum(const std::string &nameSec){
    //TODO: complete it
    if (nameSec=="AcDb:Header"){
        return HEADER;
    } else if (nameSec=="AcDb:Classes"){
        return CLASSES;
    } else if (nameSec=="AcDb:SummaryInfo"){
        return SUMARYINFO;
    } else if (nameSec=="AcDb:Preview"){
        return PREVIEW;
    } else if (nameSec=="AcDb:VBAProject"){
        return VBAPROY;
    } else if (nameSec=="AcDb:AppInfo"){
        return APPINFO;
    } else if (nameSec=="AcDb:FileDepList"){
        return FILEDEP;
    } else if (nameSec=="AcDb:RevHistory"){
        return REVHISTORY;
    } else if (nameSec=="AcDb:Security"){
        return SECURITY;
    } else if (nameSec=="AcDb:AcDbObjects"){
        return OBJECTS;
    } else if (nameSec=="AcDb:ObjFreeSpace"){
        return OBJFREESPACE;
    } else if (nameSec=="AcDb:Template"){
        return TEMPLATE;
    } else if (nameSec=="AcDb:Handles"){
        return HANDLES;
    } else if (nameSec=="AcDb:AcDsPrototype_1b"){
        return PROTOTYPE;
    } else if (nameSec=="AcDb:AuxHeader"){
        return AUXHEADER;
    } else if (nameSec=="AcDb:Signature"){
        return SIGNATURE;
    } else if (nameSec=="AcDb:AppInfoHistory"){ //in ac1021
        return APPINFOHISTORY;
//    } else if (nameSec=="AcDb:Extended Entity Data"){
//        return EXTEDATA;
//    } else if (nameSec=="AcDb:PROXY ENTITY GRAPHICS"){
//        return PROXYGRAPHICS;
    }
    return UNKNOWNS;
}
