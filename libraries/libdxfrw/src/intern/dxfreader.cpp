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

#include <cerrno>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <sstream>
#include <locale>
#include "dxfreader.h"
#include "drw_textcodec.h"
#include "drw_dbg.h"
#include "drw_reserve.h"

namespace {

bool parseDxfHandle(const std::string &text, std::uint32_t &value) {
    if (text.empty())
        return false;
    const char *begin = text.data();
    const char *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 16);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parseDxfHandleLexeme(const std::string &text, std::uint64_t &value) {
    // A DWG handle is at most eight bytes. Preserve that width for raw DXF
    // replay, but do not let a value outside the typed 32-bit model leak into
    // ordinary object/entity readers.
    if (text.empty() || text.size() > 16)
        return false;
    const char *begin = text.data();
    const char *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 16);
    return result.ec == std::errc{} && result.ptr == end;
}

bool isDxfHexString(const std::string &text) {
    if ((text.size() & 1u) != 0u || text.size() / 2u > 127u)
        return false;
    for (char ch : text) {
        if (!std::isxdigit(static_cast<unsigned char>(ch)))
            return false;
    }
    return true;
}

bool isUnambiguousDxfHandleCode(int code) {
    return code == 105 || code == 1005 ||
           (code >= 320 && code <= 369) ||
           (code >= 390 && code <= 399) ||
           (code >= 480 && code <= 481);
}


// Group-code value types.
//
// This used to be a long if/else chain whose ranges overlapped by accident:
// codes 260-269 fell through to the boolean branch written for 290-299, and
// codes 482-998 fell through to the 1010-1059 double branch. Stating each
// range once makes the assignments, and the unassigned spans, visible.
//
// This table reproduces the previous dispatch exactly; it is a restatement,
// not a behaviour change. Order matters: the first matching row wins, so 1004
// (binary chunk) precedes the 999-1008 string range, and that range precedes
// the unassigned 482-998 span.
//
// Two things here are known to be unfinished, and both need a decision rather
// than a quiet edit:
//
//  * 482-998 is unassigned, so decoding it as a double is a guess. Because
//    every code in the span matches a row, the explicit unknown-code fallback
//    at the end of readRec() is unreachable for it.
//  * classifyDxfCode() in libdxfrw.cpp is a second copy of this map - its own
//    comment describes it as mirroring this dispatch - and the two have
//    already drifted: it classifies 260-269 as Int16 where this table says
//    boolean. The two agree today only because captureRawGroup() funnels
//    Int16, Int32 and Bool to the same getter. They are NOT interchangeable
//    elsewhere: the raw re-emit path writes a Bool through writeBool(), which
//    would turn a code-260 value other than 0 or 1 into a boolean, and
//    isValidRawDxfNumericString() validates Int16 against the int16 range but
//    Bool against the int32 range. Unifying the two maps therefore has to pick
//    a kind for 260-269 deliberately.
enum class DxfValueKind { Str, Dbl, I16, I32, I64, Bln, Bin, Unknown };

struct DxfCodeRange {
    int lo;
    int hi;
    DxfValueKind kind;
};

constexpr DxfCodeRange kDxfCodeRanges[] = {
    {   0,    9, DxfValueKind::Str},
    {  10,   59, DxfValueKind::Dbl},
    {  60,   79, DxfValueKind::I16},
    {  80,   89, DxfValueKind::Str},   // reserved
    {  90,   99, DxfValueKind::I32},
    { 100,  109, DxfValueKind::Str},   // subclass/control/embedded markers
    { 110,  149, DxfValueKind::Dbl},
    { 150,  159, DxfValueKind::Str},   // reserved
    { 160,  169, DxfValueKind::I64},
    { 170,  179, DxfValueKind::I16},
    { 180,  209, DxfValueKind::Str},   // reserved
    { 210,  259, DxfValueKind::Dbl},   // 3D point coordinates, incl. 240/242
    { 260,  269, DxfValueKind::Bln},   // not in the published table, but the
                                       // reader has always decoded these as a
                                       // boolean/integer and dxf_object_tests
                                       // pins code 260 as an INTEGER variant
    { 270,  289, DxfValueKind::I16},
    { 290,  299, DxfValueKind::Bln},
    { 300,  309, DxfValueKind::Str},
    { 310,  319, DxfValueKind::Bin},
    { 320,  369, DxfValueKind::Str},   // handles
    { 370,  389, DxfValueKind::I16},
    { 390,  399, DxfValueKind::Str},   // handles
    { 400,  409, DxfValueKind::I16},
    { 410,  419, DxfValueKind::Str},
    { 420,  429, DxfValueKind::I32},
    { 430,  439, DxfValueKind::Str},
    { 440,  449, DxfValueKind::I32},
    { 450,  459, DxfValueKind::I32},
    { 460,  469, DxfValueKind::Dbl},
    { 470,  481, DxfValueKind::Str},
    {1004, 1004, DxfValueKind::Bin},   // must precede the 999-1008 range
    { 999, 1008, DxfValueKind::Str},
    { 482,  998, DxfValueKind::Dbl},   // unassigned; see the note above
    {1009, 1059, DxfValueKind::Dbl},
    {1060, 1070, DxfValueKind::I16},
    {1071, 1071, DxfValueKind::I32},
};

DxfValueKind dxfValueKindForCode(int code) {
    for (const DxfCodeRange& range : kDxfCodeRanges) {
        if (code >= range.lo && code <= range.hi)
            return range.kind;
    }
    return DxfValueKind::Unknown;
}

}  // namespace

bool dxfReader::readRec(int *codeData) {
//    std::string text;
    int code;

    if (codeData == nullptr || filestr == nullptr)
        return false;

    auto invalidateRecord = [this]() {
        type = INVALID;
        strData.clear();
        rawData.clear();
        doubleData = 0.0;
        intData = 0;
        int64 = 0;
        m_currentSelfHandle = 0;
        m_currentSelfHandleRegistered = false;
    };
    invalidateRecord();

    // Comments are ignored only after the first SECTION marker. Skip a run
    // iteratively so hostile input cannot grow the call stack.
    do {
        if (!readCode(&code))
            return false;
        if (code != 999 || !m_bIgnoreComments)
            break;
        if (!readString()) {
            invalidateRecord();
            return false;
        }
    } while (true);

    *codeData = code;

    bool valueOk = true;
    switch (dxfValueKindForCode(code)) {
    case DxfValueKind::Str:    valueOk = readString(); break;
    case DxfValueKind::Dbl:    valueOk = readDouble(); break;
    case DxfValueKind::I16:    valueOk = readInt16();  break;
    case DxfValueKind::I32:    valueOk = readInt32();  break;
    case DxfValueKind::I64:    valueOk = readInt64();  break;
    case DxfValueKind::Bln:    valueOk = readBool();   break;
    case DxfValueKind::Bin:    valueOk = readBinary(); break;
    case DxfValueKind::Unknown:
        if (skip) {
            //skip safely this dxf entry ( ok for ascii dxf)
            valueOk = readString();
            break;
        }
        //break in binary files because the conduct is unpredictable
        invalidateRecord();
        return false;
    }

    // Use !fail() not good(): std::getline that reads a final record WITHOUT a
    // trailing newline sets eofbit (good()==false) on an otherwise SUCCESSFUL
    // extraction (fail()==false). good() would wrongly reject that last record;
    // a genuine failed read sets failbit, which !fail() still catches. (Binary
    // readers gate on their own good() check, so this is a no-op for them.)
    if (!valueOk || filestr->fail()) {
        invalidateRecord();
        return false;
    }
    // Code 5 is intentionally excluded: DIMSTYLE uses it for a block-name
    // string in valid files. All other pointer/handle ranges are unambiguous.
    const bool validHandle = m_allowWideHandleLexemes
        ? isValidHandleLexeme()
        : isValidHandleString();
    if (isUnambiguousDxfHandleCode(code) && !validHandle) {
        invalidateRecord();
        return false;
    }
    return true;
}
std::uint32_t dxfReader::getHandleString(){
    std::uint32_t value = 0;
    if (!parseDxfHandle(strData, value))
        return 0;
    return value;
}

bool dxfReader::isValidHandleString() const {
    std::uint32_t value = 0;
    return parseDxfHandle(strData, value);
}

bool dxfReader::isValidHandleLexeme() const {
    std::uint64_t value = 0;
    return parseDxfHandleLexeme(strData, value);
}

bool dxfReader::registerSelfHandle() {
    std::uint64_t value = 0;
    if (!parseDxfHandleLexeme(strData, value))
        return false;
    // Zero is the null handle, not a drawing object identity.
    if (value == 0)
        return true;
    // Raw proxy entities admit code 5 once through their proxy host and once
    // through the lossless carrier. Treat those two admissions of the same
    // record as one registration, while still rejecting a later record.
    if (m_currentSelfHandleRegistered && m_currentSelfHandle == value)
        return true;
    if (!m_selfHandles.insert(value).second)
        return false;
    m_currentSelfHandle = value;
    m_currentSelfHandleRegistered = true;
    return true;
}

bool dxfReaderBinary::readCode(int *code) {
    unsigned char buffer[2] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 2);
    if (!filestr->good())
        return false;
    *code = static_cast<int>(buffer[0])
        | (static_cast<int>(buffer[1]) << 8);
    DRW_DBG(*code); DRW_DBG("\n");

    return true;
}

bool dxfReaderBinaryR12::readCode(int *code) {
    unsigned char b = 0;
    filestr->read(reinterpret_cast<char*>(&b), 1);
    if (!filestr->good())
        return false;
    if (b == 255) {
        // 0xFF is the extended-data escape: the real 16-bit LE group code
        // follows in the next two bytes (R12 carries xdata codes >= 255 this
        // way). Matches ezdxf binary_tags_loader.
        unsigned char buffer[2] = {};
        filestr->read(reinterpret_cast<char*>(buffer), 2);
        if (!filestr->good())
            return false;
        *code = static_cast<int>(buffer[0])
            | (static_cast<int>(buffer[1]) << 8);
    } else {
        *code = static_cast<int>(b);
    }
    DRW_DBG(*code); DRW_DBG("\n");

    return true;
}

bool dxfReaderBinary::readString() {
    type = STRING;
    std::getline(*filestr, strData, '\0');
    rawData = strData;
    DRW_DBG(strData); DRW_DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readString(std::string *text) {
    type = STRING;
    std::getline(*filestr, *text, '\0');
    rawData = *text;
    DRW_DBG(*text); DRW_DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readBinary() {
    type = BINARY;
    unsigned char chunklen {0};

    filestr->read( reinterpret_cast<char *>(&chunklen), 1);
    if (!filestr->good())
        return false;
    // Binary DXF stores binary chunks as a one-byte length, but the DXF
    // format limits each 310-319/1004 chunk to 127 bytes.
    if (chunklen > 127)
        return false;
    // Capture the chunk bytes as an upper-hex string — the canonical ASCII form
    // of binary codes (310-319/1004) — so getString() returns the real data.
    // Previously this seeked past the chunk and never wrote strData, so any
    // binary group on a binary read (typed entity OR raw-net object) re-emitted
    // a STALE strData (the previous record's value). Same net stream advance.
    strData.clear();
    if (!DRW::reserve(strData, static_cast<int>(chunklen) * 2))
        return false;
    static const char hex[] = "0123456789ABCDEF";
    for (unsigned i = 0; i < chunklen; ++i) {
        char b = 0;
        filestr->read(&b, 1);
        const unsigned char u = static_cast<unsigned char>(b);
        strData.push_back(hex[(u >> 4) & 0xF]);
        strData.push_back(hex[u & 0xF]);
    }
    rawData = strData;
    DRW_DBG( chunklen); DRW_DBG( " byte(s) binary data read\n");

    return (filestr->good());
}

bool dxfReaderBinary::readInt16() {
    type = INT32;
    unsigned char buffer[2] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 2);
    if (!filestr->good())
        return false;
    intData = static_cast<std::int16_t>((static_cast<unsigned char>(buffer[1]) << 8)
                                  | static_cast<unsigned char>(buffer[0]));
    DRW_DBG(intData); DRW_DBG("\n");
    return true;
}

bool dxfReaderBinary::readInt32() {
    type = INT32;
    unsigned char buffer[4] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 4);
    if (!filestr->good())
        return false;
    std::uint32_t value = static_cast<std::uint32_t>(buffer[0])
        | (static_cast<std::uint32_t>(buffer[1]) << 8)
        | (static_cast<std::uint32_t>(buffer[2]) << 16)
        | (static_cast<std::uint32_t>(buffer[3]) << 24);
    intData = static_cast<std::int32_t>(value);
    DRW_DBG(intData); DRW_DBG("\n");
    return true;
}

bool dxfReaderBinary::readInt64() {
    type = INT64;
    unsigned char buffer[8] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 8);
    if (!filestr->good())
        return false;
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(buffer[i]) << (8 * i);
    std::memcpy(&int64, &value, sizeof(int64));
    DRW_DBG(int64); DRW_DBG(" int64\n");
    return true;
}

bool dxfReaderBinary::readDouble() {
    type = DOUBLE;
    unsigned char buffer[8] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 8);
    if (!filestr->good())
        return false;
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(buffer[i]) << (8 * i);
    std::memcpy(&doubleData, &value, sizeof(doubleData));
    DRW_DBG(doubleData); DRW_DBG("\n");
    return std::isfinite(doubleData);
}

//saved as int or add a bool member??
bool dxfReaderBinary::readBool() {
    type = BOOL;
    unsigned char buffer[1] = {};
    filestr->read(reinterpret_cast<char*>(buffer), 1);
    if (!filestr->good())
        return false;
    intData = static_cast<int>(buffer[0]);
    DRW_DBG(intData); DRW_DBG("\n");
    return true;
}

bool dxfReaderAscii::readCode(int *code) {
    std::string text;
    std::getline(*filestr, text);
    if (filestr->fail())  // !fail(): accept a final newline-less line (eofbit set, fail() clear)
        return false;
    if (!text.empty() && text.at(text.size()-1) == '\r')
        text.erase(text.size()-1);
    char *end = nullptr;
    errno = 0;
    long parsed = std::strtol(text.c_str(), &end, 10);
    while (end != nullptr && *end != '\0'
           && std::isspace(static_cast<unsigned char>(*end))) {
        ++end;
    }
    if (end == text.c_str() || end == nullptr || *end != '\0'
        || errno == ERANGE || parsed < 0 || parsed > 1071) {
        return false;
    }
    *code = static_cast<int>(parsed);
    DRW_DBG(*code); DRW_DBG("\n");
    return true;
}
bool dxfReaderAscii::readString(std::string *text) {
    type = STRING;
    std::getline(*filestr, *text);
    if (!text->empty() && text->at(text->size()-1) == '\r')
        text->erase(text->size()-1);
    rawData = *text;
    return (!filestr->fail());
}

bool dxfReaderAscii::readString() {
    type = STRING;
    std::getline(*filestr, strData);
    if (!strData.empty() && strData.at(strData.size()-1) == '\r')
        strData.erase(strData.size()-1);
    rawData = strData;
    DRW_DBG(strData); DRW_DBG("\n");
    return (!filestr->fail());
}

bool dxfReaderAscii::readBinary() {
    if (!readString())
        return false;
    // ASCII DXF binary groups are complete hexadecimal byte pairs. Rejecting
    // malformed data here keeps typed and raw record paths transactional.
    if (!isDxfHexString(strData))
        return false;
    type = BINARY;
    return true;
}

bool dxfReaderAscii::readInt16() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        const long long parsed = std::strtoll(text.c_str(), &end, 10);
        while (end != nullptr && *end != '\0'
               && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || *end != '\0'
            || errno == ERANGE
            // DXF 16-bit flag fields may be written as an unsigned bit
            // pattern (for example 0x8001). Keep both signed values and all
            // values representable by the two-byte field.
            || parsed < std::numeric_limits<std::int16_t>::min()
            || parsed > std::numeric_limits<std::uint16_t>::max()) {
            return false;
        }
        type = INT32;
        intData = static_cast<int>(parsed);
        DRW_DBG(intData); DRW_DBG("\n");
        return true;
    }
    return false;
}

bool dxfReaderAscii::readInt32() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        const long long parsed = std::strtoll(text.c_str(), &end, 10);
        while (end != nullptr && *end != '\0'
               && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || *end != '\0'
            || errno == ERANGE
            || parsed < std::numeric_limits<std::int32_t>::min()
            || parsed > std::numeric_limits<std::int32_t>::max()) {
            return false;
        }
        type = INT32;
        intData = static_cast<int>(parsed);
        DRW_DBG(intData); DRW_DBG("\n");
        return true;
    }
    return false;
}

bool dxfReaderAscii::readInt64() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        const long long parsed = std::strtoll(text.c_str(), &end, 10);
        while (end != nullptr && *end != '\0'
               && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || *end != '\0'
            || errno == ERANGE
            || parsed < std::numeric_limits<std::int64_t>::min()
            || parsed > std::numeric_limits<std::int64_t>::max()) {
            return false;
        }
        type = INT64;
        int64 = static_cast<std::int64_t>(parsed);
        DRW_DBG(int64); DRW_DBG(" int64\n");
        return true;
    }
    return false;
}

bool dxfReaderAscii::readDouble() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        double parsed = std::strtod(text.c_str(), &end);
        while (end != nullptr && *end != '\0'
               && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || *end != '\0'
            || errno == ERANGE || !std::isfinite(parsed)) {
            DRW_DBG("dxfReaderAscii::readDouble(): reading double error: ");
            DRW_DBG(text);
            DRW_DBG('\n');
            return false;
        }
        type = DOUBLE;
        doubleData = parsed;
        DRW_DBG(doubleData); DRW_DBG('\n');
        return true;
    }
    return false;
}

//saved as int or add a bool member??
bool dxfReaderAscii::readBool() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        long parsed = std::strtol(text.c_str(), &end, 10);
        while (end != nullptr && *end != '\0'
               && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || *end != '\0'
            || errno == ERANGE
            || parsed < std::numeric_limits<std::int32_t>::min()
            || parsed > std::numeric_limits<std::int32_t>::max()) {
            return false;
        }
        type = BOOL;
        intData = static_cast<int>(parsed);
        DRW_DBG(intData); DRW_DBG("\n");
        return true;
    }
    return false;
}
