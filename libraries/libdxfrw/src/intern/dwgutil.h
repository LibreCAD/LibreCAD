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

#ifndef DWGUTIL_H
#define DWGUTIL_H

#include "../drw_base.h"

namespace DRW {
    std::string toHexStr(int n);
}

namespace dwgRSCodec {
    void decode239I(duint8 *in, duint8 *out, duint32 blk);
    void decode251I(duint8 *in, duint8 *out, duint32 blk);
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

    static duint8 compressedByte(void);
    static duint8 compressedByte(const duint32 index);
    static duint32 compressedHiByte(void);
    static bool compressedInc(const dint32 inc = 1);
    static duint8 decompByte(const duint32 index);
    static void decompSet(const duint8 value);
    static bool buffersGood(void);
    static void copyBlock21(const duint32 length);

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
