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

#ifndef DWGUTIL_H
#define DWGUTIL_H

#include "../drw_base.h"

namespace DRW {
    std::string toHexStr(int n);
}

namespace dwgRSCodec {
    /// Returns true if all codewords decoded cleanly; false if any block had
    /// uncorrectable Reed-Solomon errors. Output buffer is still populated
    /// with whatever the codec recovered, so callers can decide whether to
    /// bail or continue best-effort.
    bool decode239I(std::uint8_t *in, std::uint8_t *out, std::uint32_t blk);
    bool decode251I(std::uint8_t *in, std::uint8_t *out, std::uint32_t blk);
}

namespace dwgUtil {
    /// Adler-32-variant checksum used by R2004 (AC1018) section pages
    /// and their headers.  Extracted from the private dwgReader18::checksum
    /// so the R2004 writer can produce matching checksums without subclassing.
    std::uint32_t checksum18(std::uint32_t seed, const std::uint8_t* data, std::uint64_t sz);

    /// Standard CRC-32/ISO-HDLC (same table as dwgBuffer::crc32).  Used
    /// by the R2004 writer to compute the encrypted variable-header CRC.
    std::uint32_t crc32(std::uint32_t seed, const std::uint8_t* data, std::uint32_t sz);
}

class dwgCompressor {
    enum R21Consts {
        MaxBlock21Length = 32,
        Block21OrderArray,
    };

public:
    dwgCompressor()=default;

    bool decompress18(std::uint8_t *cbuf, std::uint8_t *dbuf, std::uint64_t csize, std::uint64_t dsize);
    static void decrypt18Hdr(std::uint8_t *buf, std::uint64_t size, std::uint64_t offset);
//    static void decrypt18Data(std::uint8_t *buf, std::uint32_t size, std::uint32_t offset);
    bool decompress21(std::uint8_t *cbuf, std::uint8_t *dbuf, std::uint64_t csize, std::uint64_t dsize);

    // Number of decompressed bytes produced by the last decompress18/21 call.
    // Callers of fixed-size pages (parseSysPage) require an exact-window fill;
    // data pages are input-bounded so a partial fill is normal.
    std::uint32_t decompressedBytes() const { return decompPos; }

private:
    std::uint32_t litLength18();
    std::uint32_t litLength21(std::uint8_t opCode);
    bool copyCompBytes21(std::uint32_t length);
    void readInstructions21(std::uint8_t &opCode, std::uint32_t &sourceOffset, std::uint32_t &length);

    std::uint32_t longCompressionOffset();
    std::uint32_t long20CompressionOffset();
    std::uint32_t twoByteOffset(std::uint32_t *ll);

    std::uint8_t compressedByte(void);
    std::uint8_t compressedByte(const std::uint32_t index);
    std::uint32_t compressedHiByte(void);
    bool compressedInc(const std::int32_t inc = 1);
    std::uint8_t decompByte(const std::uint32_t index);
    void decompSet(const std::uint8_t value);
    bool buffersGood(void);
    void copyBlock21(const std::uint32_t length);

    // Decode state — instance members (formerly static, which made decompress
    // non-reentrant and was a verbose/non-verbose nondeterminism hazard).
    std::uint8_t *compressedBuffer{nullptr};
    std::uint32_t compressedSize{0};
    std::uint32_t compressedPos{0};
    bool    compressedGood{true};
    std::uint8_t *decompBuffer{nullptr};
    std::uint32_t decompSize{0};
    std::uint32_t decompPos{0};
    bool    decompGood{true};

    static const std::uint8_t CopyOrder21_01[];
    static const std::uint8_t CopyOrder21_02[];
    static const std::uint8_t CopyOrder21_03[];
    static const std::uint8_t CopyOrder21_04[];
    static const std::uint8_t CopyOrder21_05[];
    static const std::uint8_t CopyOrder21_06[];
    static const std::uint8_t CopyOrder21_07[];
    static const std::uint8_t CopyOrder21_08[];
    static const std::uint8_t CopyOrder21_09[];
    static const std::uint8_t CopyOrder21_10[];
    static const std::uint8_t CopyOrder21_11[];
    static const std::uint8_t CopyOrder21_12[];
    static const std::uint8_t CopyOrder21_13[];
    static const std::uint8_t CopyOrder21_14[];
    static const std::uint8_t CopyOrder21_15[];
    static const std::uint8_t CopyOrder21_16[];
    static const std::uint8_t CopyOrder21_17[];
    static const std::uint8_t CopyOrder21_18[];
    static const std::uint8_t CopyOrder21_19[];
    static const std::uint8_t CopyOrder21_20[];
    static const std::uint8_t CopyOrder21_21[];
    static const std::uint8_t CopyOrder21_22[];
    static const std::uint8_t CopyOrder21_23[];
    static const std::uint8_t CopyOrder21_24[];
    static const std::uint8_t CopyOrder21_25[];
    static const std::uint8_t CopyOrder21_26[];
    static const std::uint8_t CopyOrder21_27[];
    static const std::uint8_t CopyOrder21_28[];
    static const std::uint8_t CopyOrder21_29[];
    static const std::uint8_t CopyOrder21_30[];
    static const std::uint8_t CopyOrder21_31[];
    static const std::uint8_t CopyOrder21_32[];
    static const std::uint8_t *CopyOrder21[Block21OrderArray];
};

/// 16-byte section sentinel byte sequences used by R13/R14/R2000 DWG.
/// Each sentinel marks the start or end of a section in the on-disk
/// file. Byte values match libreDWG common.c:100-177 (and the ODA
/// Open Design Specification, which lists the same sequences).
/// Each BEGIN sentinel is the byte-wise XOR-0xFF complement of its END
/// pair (with the file-header end sentinel having no paired begin).
///
/// Used by the read path (dwgReader::checkSentinel) and by the
/// forthcoming write path to bracket sections in the output stream.
namespace dwgSentinels {
    /// Closing sentinel for the file header / section-locator block.
    /// Appears after the section locator records and their CRC16.
    /// No paired begin sentinel — the file header has no leading magic.
    extern const std::uint8_t FILE_HEADER_END[16];

    /// HEADER section (AcDb:Header — header variables). LibreDWG names
    /// these VARIABLE_BEGIN / VARIABLE_END.
    extern const std::uint8_t HEADER_BEGIN[16];
    extern const std::uint8_t HEADER_END[16];

    /// CLASSES section (AcDb:Classes). LibreDWG names CLASS_BEGIN/END.
    extern const std::uint8_t CLASSES_BEGIN[16];
    extern const std::uint8_t CLASSES_END[16];

    /// PREVIEW section (the thumbnail image block, when present).
    /// LibreDWG names THUMBNAIL_BEGIN/END.
    extern const std::uint8_t PREVIEW_BEGIN[16];
    extern const std::uint8_t PREVIEW_END[16];

    /// Second-header block. Appears between the OBJECTS section and
    /// the object map / handles section in R13/R14/R2000.
    extern const std::uint8_t SECOND_HEADER_BEGIN[16];
    extern const std::uint8_t SECOND_HEADER_END[16];
}

/// Padded version strings for DWG file header (offset 0, 6+5 bytes).
/// The version string is 6 bytes ("AC1015" for R2000) followed by 5
/// reserved bytes that are conventionally NUL.  Together they fill the
/// first 11 bytes of the file. Byte 12 is the maintenance-release byte
/// and is set separately by the writer.
namespace dwgVersionString {
    /// AC1009 — R12 (DXF-only in libdxfrw; here for completeness).
    extern const char R12[6];
    /// AC1012 — R13.
    extern const char R13[6];
    /// AC1014 — R14.
    extern const char R14[6];
    /// AC1015 — R2000 (R2000-only writer target).
    extern const char R2000[6];
    /// AC1018 — R2004.
    extern const char R2004[6];
    /// AC1021 — R2007.
    extern const char R2007[6];
    /// AC1024 — R2010.
    extern const char R2010[6];
    /// AC1027 — R2013.
    extern const char R2013[6];
    /// AC1032 — R2018.
    extern const char R2018[6];
}

namespace secEnum {
    enum DWGSection {
        UNKNOWNS,      /*!< UNKNOWN section. */
        FILEHEADER,    /*!< File Header (in R3-R15*/
        HEADER,        /*!< AcDb:Header */
        CLASSES,       /*!< AcDb:Classes */
        SUMARYINFO,    /*!< AcDb:SummaryInfo */
        PREVIEW,       /*!< AcDb:Preview */
        VBAPROY,       /*!< AcDb:VBAProject */
        APPINFO,       /*!< AcDb:AppInfo */
        FILEDEP,       /*!< AcDb:FileDepList */
        REVHISTORY,    /*!< AcDb:RevHistory */
        SECURITY,      /*!< AcDb:Security */
        OBJECTS,       /*!< AcDb:AcDbObjects */
        OBJFREESPACE,  /*!< AcDb:ObjFreeSpace */
        TEMPLATE,      /*!< AcDb:Template */
        HANDLES,       /*!< AcDb:Handles */
        PROTOTYPE,     /*!< AcDb:AcDsPrototype_1b */
        AUXHEADER,     /*!< AcDb:AuxHeader, in (R13-R15) second file header */
        SIGNATURE,     /*!< AcDb:Signature */
        APPINFOHISTORY,     /*!< AcDb:AppInfoHistory (in ac1021 may be a renamed section?*/
        EXTEDATA,      /*!< Extended Entity Data */
        PROXYGRAPHICS /*!< PROXY ENTITY GRAPHICS */
    };

    DWGSection getEnum(const std::string &nameSec);
}

#endif // DWGUTIL_H
