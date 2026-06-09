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
    bool decode239I(duint8 *in, duint8 *out, duint32 blk);
    bool decode251I(duint8 *in, duint8 *out, duint32 blk);
}

class dwgCompressor {
    enum R21Consts {
        MaxBlock21Length = 32,
        Block21OrderArray,
    };

public:
    dwgCompressor()=default;

    bool decompress18(duint8 *cbuf, duint8 *dbuf, duint64 csize, duint64 dsize);
    static void decrypt18Hdr(duint8 *buf, duint64 size, duint64 offset);
//    static void decrypt18Data(duint8 *buf, duint32 size, duint32 offset);
    static bool decompress21(duint8 *cbuf, duint8 *dbuf, duint64 csize, duint64 dsize);

private:
    duint32 litLength18();
    static duint32 litLength21(duint8 opCode);
    static bool copyCompBytes21(duint32 length);
    static void readInstructions21(duint8 &opCode, duint32 &sourceOffset, duint32 &length);

    duint32 longCompressionOffset();
    duint32 long20CompressionOffset();
    duint32 twoByteOffset(duint32 *ll);

    static duint8 compressedByte();
    static duint8 compressedByte(duint32 index);
    static duint32 compressedHiByte();
    static bool compressedInc(dint32 inc = 1);
    static duint8 decompByte(duint32 index);
    static void decompSet(duint8 value);
    static bool buffersGood();
    static void copyBlock21(duint32 length);

    static duint8 *compressedBuffer;
    static duint32 compressedSize;
    static duint32 compressedPos;
    static bool    compressedGood;
    static duint8 *decompBuffer;
    static duint32 decompSize;
    static duint32 decompPos;
    static bool    decompGood;

    static const duint8 CopyOrder21_01[];
    static const duint8 CopyOrder21_02[];
    static const duint8 CopyOrder21_03[];
    static const duint8 CopyOrder21_04[];
    static const duint8 CopyOrder21_05[];
    static const duint8 CopyOrder21_06[];
    static const duint8 CopyOrder21_07[];
    static const duint8 CopyOrder21_08[];
    static const duint8 CopyOrder21_09[];
    static const duint8 CopyOrder21_10[];
    static const duint8 CopyOrder21_11[];
    static const duint8 CopyOrder21_12[];
    static const duint8 CopyOrder21_13[];
    static const duint8 CopyOrder21_14[];
    static const duint8 CopyOrder21_15[];
    static const duint8 CopyOrder21_16[];
    static const duint8 CopyOrder21_17[];
    static const duint8 CopyOrder21_18[];
    static const duint8 CopyOrder21_19[];
    static const duint8 CopyOrder21_20[];
    static const duint8 CopyOrder21_21[];
    static const duint8 CopyOrder21_22[];
    static const duint8 CopyOrder21_23[];
    static const duint8 CopyOrder21_24[];
    static const duint8 CopyOrder21_25[];
    static const duint8 CopyOrder21_26[];
    static const duint8 CopyOrder21_27[];
    static const duint8 CopyOrder21_28[];
    static const duint8 CopyOrder21_29[];
    static const duint8 CopyOrder21_30[];
    static const duint8 CopyOrder21_31[];
    static const duint8 CopyOrder21_32[];
    static const duint8 *CopyOrder21[Block21OrderArray];
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
    extern const duint8 FILE_HEADER_END[16];

    /// HEADER section (AcDb:Header — header variables). LibreDWG names
    /// these VARIABLE_BEGIN / VARIABLE_END.
    extern const duint8 HEADER_BEGIN[16];
    extern const duint8 HEADER_END[16];

    /// CLASSES section (AcDb:Classes). LibreDWG names CLASS_BEGIN/END.
    extern const duint8 CLASSES_BEGIN[16];
    extern const duint8 CLASSES_END[16];

    /// PREVIEW section (the thumbnail image block, when present).
    /// LibreDWG names THUMBNAIL_BEGIN/END.
    extern const duint8 PREVIEW_BEGIN[16];
    extern const duint8 PREVIEW_END[16];

    /// Second-header block. Appears between the OBJECTS section and
    /// the object map / handles section in R13/R14/R2000.
    extern const duint8 SECOND_HEADER_BEGIN[16];
    extern const duint8 SECOND_HEADER_END[16];
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

#endif
