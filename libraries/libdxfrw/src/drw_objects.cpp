/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2022 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <iostream>
#include <cmath>
#include "drw_objects.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_dbg.h"
#include "intern/drw_reserve.h"
#include "intern/dwgutil.h"
#include "intern/dwgreader.h"

namespace {

// ODA handle reference codes (dwg_spec.txt §2.13): 2=soft ownership,
// 3=hard ownership, 4=soft pointer, 5=hard pointer.  Emits a null handle
// (code 0) when ref==0, which is the canonical NULL encoding.
dwgHandle makeRefW(std::uint32_t ref, std::uint8_t code) {
    dwgHandle h; h.code = ref ? code : 0; h.ref = ref; h.size = 0;
    for (std::uint32_t t = ref; t; t >>= 8) ++h.size;
    return h;
}
// Common object handle prefix per libreDWG common_object_handle_data.spec:
// ownerhandle = soft pointer (4), each reactor = soft pointer (4),
// xdictionary = hard owner (3).
void putCommonObjectHandlePrefix(dwgBufferW* hb, std::uint32_t ownerRef,
                                 std::int32_t numReactors, std::uint8_t xDictFlag) {
    hb->putHandle(makeRefW(ownerRef, 4));            // ownerhandle (330)
    for (std::int32_t i = 0; i < numReactors; ++i)
        hb->putHandle(makeRefW(0, 4));               // reactors
    if (xDictFlag != 1)
        hb->putHandle(makeRefW(0, 3));               // xdictionary
}
// DEPRECATED names (wrong codes per ODA) — retained only for not-yet-migrated
// call sites; emit the historical (pre-B2) codes so un-migrated sites are
// unchanged.  New code should use makeRefW(ref, code) with an explicit ODA code.
dwgHandle makeSoftOwnerW(std::uint32_t ref) { return makeRefW(ref, 3); }
dwgHandle makeHardPtrW(std::uint32_t ref) { return makeRefW(ref, 4); }
dwgHandle writeHandleOrHardPtr(const dwgHandle& handle) {
    if (handle.ref != 0 && handle.code == 0)
        return makeHardPtrW(handle.ref);
    return handle;
}
dwgHandle makeNullHandleW() { dwgHandle h; h.code = 0; h.size = 0; h.ref = 0; return h; }

void seekObjectHandleStream(DRW::Version version, dwgBuffer *buf, std::uint32_t objSize) {
    if (version > DRW::AC1018) {
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
}

void readCommonObjectHandles(dwgBuffer *buf, std::uint32_t baseHandle,
                             std::int32_t numReactors, std::uint8_t xDictFlag,
                             int *parentHandle = nullptr) {
    dwgHandle parentH = buf->getOffsetHandle(baseHandle);
    if (parentHandle)
        *parentHandle = parentH.ref;
    for (int i = 0; i < numReactors; ++i)
        buf->getOffsetHandle(baseHandle);
    if (xDictFlag != 1)
        buf->getOffsetHandle(baseHandle);
}

UTF8STRING readXRecordText(DRW::Version version, dwgBuffer *buf) {
    if (version > DRW::AC1018) {
        std::uint16_t chars = buf->getRawShort16();
        if (chars == 0)
            return UTF8STRING();
        std::vector<std::uint8_t> raw(static_cast<size_t>(chars) * 2);
        if (!buf->getBytes(raw.data(), raw.size()))
            return UTF8STRING();
        std::string s(reinterpret_cast<const char*>(raw.data()), raw.size());
        if (buf->decoder)
            s = buf->decoder->toUtf8(s);
        while (!s.empty() && s.back() == '\0')
            s.pop_back();
        return s;
    }

    std::uint16_t len = buf->getRawShort16();
    std::uint8_t codePage = buf->getRawChar8();
    DRW_UNUSED(codePage);
    if (len == 0)
        return UTF8STRING();
    std::vector<std::uint8_t> raw(len);
    if (!buf->getBytes(raw.data(), raw.size()))
        return UTF8STRING();
    std::string s(reinterpret_cast<const char*>(raw.data()), raw.size());
    if (buf->decoder)
        s = buf->decoder->toUtf8(s);
    while (!s.empty() && s.back() == '\0')
        s.pop_back();
    return s;
}

constexpr std::uint32_t kMaxCadValueBytes = 16 * 1024 * 1024;

bool readCadValueBytes(dwgBuffer *buf, std::vector<std::uint8_t>& raw, const char *label) {
    const std::uint32_t byteCount = buf->getBitLong();
    if (byteCount > kMaxCadValueBytes) {
        DRW_DBG(label); DRW_DBG(" too large: "); DRW_DBG(byteCount); DRW_DBG("\n");
        return false;
    }
    raw.resize(byteCount);
    return byteCount == 0 || buf->getBytes(raw.data(), raw.size());
}

UTF8STRING decodeCadValueText(DRW::Version version, dwgBuffer *buf, const std::vector<std::uint8_t>& raw) {
    if (raw.empty())
        return UTF8STRING();
    std::string s(reinterpret_cast<const char*>(raw.data()), raw.size());
    if (version > DRW::AC1018 && s.size() >= 2 && s[s.size() - 1] == '\0'
        && s[s.size() - 2] == '\0') {
        s.resize(s.size() - 2);
    } else {
        while (!s.empty() && s.back() == '\0')
            s.pop_back();
    }
    if (buf->decoder)
        s = buf->decoder->toUtf8(s);
    return s;
}

bool readCadValuePoint(dwgBuffer *buf, DRW_CadValue& value, int dimensions) {
    value.m_dataSize = static_cast<std::uint32_t>(buf->getBitLong());
    const std::uint32_t expectedSize = static_cast<std::uint32_t>(dimensions) * 8;
    if (value.m_dataSize > kMaxCadValueBytes)
        return false;
    if (value.m_dataSize < expectedSize) {
        value.m_rawData.resize(value.m_dataSize);
        if (value.m_dataSize > 0 && !buf->getBytes(value.m_rawData.data(), value.m_rawData.size()))
            return false;
        value.m_value.addBinary(310, value.m_rawData);
        return true;
    }

    DRW_Coord c;
    c.x = buf->getRawDouble();
    c.y = buf->getRawDouble();
    c.z = dimensions == 3 ? buf->getRawDouble() : 0.0;
    value.m_value.addCoord(11, c);

    const std::uint32_t extraBytes = value.m_dataSize - expectedSize;
    value.m_rawData.resize(extraBytes);
    return extraBytes == 0 || buf->getBytes(value.m_rawData.data(), value.m_rawData.size());
}

constexpr std::uint32_t kMaxTableStyleItems = 100000;
constexpr std::int32_t kMaxAssocItems = 100000;
constexpr std::int32_t kMaxAssocValueParams = 10000;
constexpr std::uint32_t kMaxAcShBlobBytes = 16 * 1024 * 1024;

std::uint64_t currentObjectDwgBit(const dwgBuffer *buf) {
    return buf->getPosition() * 8 + buf->getBitPos();
}

DRW_DwgSubrecordRange makeObjectSubrecordRange(const char *name, std::uint64_t startBit,
                                               std::uint64_t endBit, DRW::Version version,
                                               std::uint32_t count, bool parseComplete) {
    DRW_DwgSubrecordRange range;
    range.m_name = name;
    range.m_startBit = startBit;
    range.m_bitSize = endBit >= startBit ? endBit - startBit : 0;
    range.m_version = version;
    range.m_count = count;
    range.m_parseComplete = parseComplete;
    return range;
}

std::uint32_t readObjectHandleRef(dwgBuffer *hdlBuf) {
    if (hdlBuf == nullptr || !hdlBuf->isGood())
        return 0;
    dwgHandle h = hdlBuf->getHandle();
    return h.ref;
}

bool isValidAssocCount(std::int32_t count, std::int32_t maxCount = kMaxAssocItems) {
    return count >= 0 && count <= maxCount;
}

DRW_AssociativePrefixStatus makeAssocPrefixStatus(
    DRW_AssociativePrefixStatus::Kind kind, std::uint64_t startBit,
    std::uint64_t endBit, DRW_AssociativePrefixStatus::ParseStatus status,
    std::uint16_t classVersion, size_t decodedHandleCount = 0,
    size_t decodedValueCount = 0, std::int32_t decodedCountValue = 0) {
    DRW_AssociativePrefixStatus prefix;
    prefix.m_kind = kind;
    prefix.m_startBit = startBit;
    prefix.m_bitSize = endBit >= startBit ? endBit - startBit : 0;
    prefix.m_status = status;
    prefix.m_classVersion = classVersion;
    prefix.m_decodedHandleCount = decodedHandleCount;
    prefix.m_decodedValueCount = decodedValueCount;
    prefix.m_decodedCountValue = decodedCountValue;
    prefix.m_sourceAssumption = "ACadSharp/libreDWG";
    return prefix;
}

DRW_AssociativePrefixStatus::ParseStatus prefixStatusFromGood(bool complete) {
    return complete ? DRW_AssociativePrefixStatus::ParseStatus::Complete :
                      DRW_AssociativePrefixStatus::ParseStatus::Partial;
}

bool skipHandleRefs(dwgBuffer *hdlBuf, std::int32_t count) {
    if (!isValidAssocCount(count))
        return false;
    for (std::int32_t i = 0; i < count; ++i)
        readObjectHandleRef(hdlBuf);
    return hdlBuf == nullptr || hdlBuf->isGood();
}

enum class DwgValueKind {
    Invalid,
    Real,
    Int8,
    Int16,
    Int32,
    Int64,
    String,
    Point3d,
    Binary,
    Handle,
    ObjectId,
    Bool
};

DwgValueKind dwgResbufValueKind(std::int32_t groupCode) {
    if (groupCode >= 300) {
        if (groupCode >= 440) {
            if (groupCode >= 1000) {
                if (groupCode == 1004)
                    return DwgValueKind::Binary;
                if (groupCode <= 1009)
                    return DwgValueKind::String;
                if (groupCode <= 1039)
                    return DwgValueKind::Point3d;
                if (groupCode <= 1042)
                    return DwgValueKind::Real;
                if (groupCode <= 1069)
                    return DwgValueKind::Point3d;
                if (groupCode <= 1070)
                    return DwgValueKind::Int16;
                if (groupCode == 1071)
                    return DwgValueKind::Int32;
            } else {
                if (groupCode <= 459)
                    return DwgValueKind::Int32;
                if (groupCode <= 469)
                    return DwgValueKind::Real;
                if (groupCode <= 479)
                    return DwgValueKind::String;
                if (groupCode == 999)
                    return DwgValueKind::String;
            }
        } else if (groupCode >= 390) {
            if (groupCode <= 399)
                return DwgValueKind::Handle;
            if (groupCode <= 409)
                return DwgValueKind::Int16;
            if (groupCode <= 419)
                return DwgValueKind::String;
            if (groupCode <= 429)
                return DwgValueKind::Int32;
            if (groupCode <= 439)
                return DwgValueKind::String;
        } else {
            if (groupCode <= 309)
                return DwgValueKind::String;
            if (groupCode <= 319)
                return DwgValueKind::Binary;
            if (groupCode <= 329)
                return DwgValueKind::Handle;
            if (groupCode <= 369)
                return DwgValueKind::ObjectId;
            if (groupCode <= 389)
                return DwgValueKind::Int16;
        }
    } else if (groupCode >= 105) {
        if (groupCode >= 210) {
            if (groupCode <= 269)
                return DwgValueKind::Point3d;
            if (groupCode <= 279)
                return DwgValueKind::Int16;
            if (groupCode <= 289)
                return DwgValueKind::Int8;
            if (groupCode <= 299)
                return DwgValueKind::Bool;
        } else {
            if (groupCode == 105)
                return DwgValueKind::Handle;
            if (groupCode <= 139)
                return DwgValueKind::Point3d;
            if (groupCode <= 149)
                return DwgValueKind::Real;
            if (groupCode <= 169)
                return DwgValueKind::Int64;
            if (groupCode <= 179)
                return DwgValueKind::Int16;
        }
    } else if (groupCode >= 38) {
        if (groupCode <= 59)
            return DwgValueKind::Real;
        if (groupCode <= 79)
            return DwgValueKind::Int16;
        if (groupCode <= 99)
            return DwgValueKind::Int32;
        if (groupCode <= 102)
            return DwgValueKind::String;
    } else {
        if (groupCode < 0)
            return DwgValueKind::Handle;
        if (groupCode <= 4)
            return DwgValueKind::String;
        if (groupCode == 5)
            return DwgValueKind::Handle;
        if (groupCode <= 9)
            return DwgValueKind::String;
        if (groupCode <= 37)
            return DwgValueKind::Point3d;
    }
    return DwgValueKind::Invalid;
}

bool readBinaryBytes(dwgBuffer *buf, std::vector<std::uint8_t>& target,
                     std::uint32_t byteCount, std::uint32_t maxBytes) {
    if (byteCount > maxBytes)
        return false;
    target.resize(byteCount);
    return byteCount == 0 || buf->getBytes(target.data(), target.size());
}

bool skipEvalVariant(DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf, dwgBuffer *hBuff) {
    const std::int32_t groupCode = buf->getSBitShort();
    if (groupCode == 0)
        return buf->isGood();
    switch (dwgResbufValueKind(groupCode)) {
    case DwgValueKind::Real:
        buf->getBitDouble();
        break;
    case DwgValueKind::Int32:
        buf->getBitLong();
        break;
    case DwgValueKind::Int16:
        buf->getBitShort();
        break;
    case DwgValueKind::Int8:
        buf->getRawChar8();
        break;
    case DwgValueKind::String:
        (sBuf ? sBuf : buf)->getVariableText(version, false);
        break;
    case DwgValueKind::Handle:
        readObjectHandleRef(hBuff);
        break;
    case DwgValueKind::ObjectId:
    case DwgValueKind::Int64:
    case DwgValueKind::Point3d:
    case DwgValueKind::Bool:
    case DwgValueKind::Binary:
    case DwgValueKind::Invalid:
        break;
    }
    return buf->isGood() && (!sBuf || sBuf->isGood()) && (!hBuff || hBuff->isGood());
}

bool skipValueParam(DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf, dwgBuffer *hBuff) {
    buf->getBitLong();
    (sBuf ? sBuf : buf)->getVariableText(version, false);
    buf->getBitLong();
    const std::int32_t varCount = buf->getBitLong();
    if (!isValidAssocCount(varCount, kMaxAssocValueParams))
        return false;
    for (std::int32_t i = 0; i < varCount; ++i) {
        if (!skipEvalVariant(version, buf, sBuf, hBuff))
            return false;
        readObjectHandleRef(hBuff);
    }
    readObjectHandleRef(hBuff);
    return buf->isGood() && (!sBuf || sBuf->isGood()) && (!hBuff || hBuff->isGood());
}

bool skipAssocActionParamPrefix(DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf) {
    buf->getBitShort();
    if (version >= DRW::AC1027)
        buf->getBitLong();
    (sBuf ? sBuf : buf)->getVariableText(version, false);
    return buf->isGood() && (!sBuf || sBuf->isGood());
}

bool skipAssocCompoundActionParam(dwgBuffer *buf, dwgBuffer *hBuff) {
    buf->getBitShort();
    buf->getBitShort();
    const std::int32_t paramCount = buf->getBitLong();
    if (!skipHandleRefs(hBuff, paramCount))
        return false;
    const bool hasChildParam = buf->getBit() != 0;
    std::int32_t childId = 0;
    if (hasChildParam) {
        buf->getBitShort();
        childId = buf->getBitLong();
        readObjectHandleRef(hBuff);
    }
    if (childId != 0) {
        readObjectHandleRef(hBuff);
        buf->getBitLong();
        readObjectHandleRef(hBuff);
    }
    return buf->isGood() && (!hBuff || hBuff->isGood());
}

bool skipAssocSingleDependencyActionParam(dwgBuffer *buf, dwgBuffer *hBuff, std::uint32_t *depHandle) {
    buf->getBitLong();
    const std::uint32_t handle = readObjectHandleRef(hBuff);
    if (depHandle)
        *depHandle = handle;
    return buf->isGood() && (!hBuff || hBuff->isGood());
}

bool skipEvalExpr(DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf, dwgBuffer *hBuff) {
    buf->getBitLong();
    buf->getBitLong();
    buf->getBitLong();
    const std::int16_t valueCode = buf->getSBitShort();
    switch (valueCode) {
    case 40:
        buf->getBitDouble();
        break;
    case 10:
    case 11:
        buf->get2RawDouble();
        break;
    case 1:
        (sBuf ? sBuf : buf)->getVariableText(version, false);
        break;
    case 90:
        buf->getBitLong();
        break;
    case 91:
        readObjectHandleRef(hBuff);
        break;
    case 70:
        buf->getBitShort();
        break;
    default:
        break;
    }
    buf->getBitLong();
    return buf->isGood() && (!sBuf || sBuf->isGood()) && (!hBuff || hBuff->isGood());
}

bool skipShHistoryNode(DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf, dwgBuffer *hBuff,
                       std::uint32_t *major = nullptr, std::uint32_t *minor = nullptr) {
    const std::uint32_t nodeMajor = static_cast<std::uint32_t>(buf->getBitLong());
    const std::uint32_t nodeMinor = static_cast<std::uint32_t>(buf->getBitLong());
    if (major)
        *major = nodeMajor;
    if (minor)
        *minor = nodeMinor;
    for (int i = 0; i < 16; ++i)
        buf->getBitDouble();
    buf->getCmColor(version, nullptr, sBuf);
    buf->getBitLong();
    readObjectHandleRef(hBuff);
    return buf->isGood() && (!sBuf || sBuf->isGood()) && (!hBuff || hBuff->isGood());
}

int readObjectCmColor(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf) {
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    std::int32_t rgb = -1;
    UTF8STRING name;
    UTF8STRING book;
    return static_cast<int>(buf->getCmColor(version, &rgb, textBuf, &name, &book));
}

std::vector<double> readObject4x3Matrix(dwgBuffer *buf) {
    std::vector<double> matrix;
    matrix.reserve(12);
    for (int i = 0; i < 12; ++i)
        matrix.push_back(buf->getBitDouble());
    return matrix;
}

bool readTableStyleContentFormat(DRW::Version version, dwgBuffer *buf,
                                 dwgBuffer *strBuf, dwgBuffer *hdlBuf,
                                 DRW_TableStyleContentFormat& format,
                                 std::vector<DRW_DwgSubrecordRange> *ranges = nullptr) {
    const std::uint64_t startBit = currentObjectDwgBit(buf);
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    format.m_propertyOverrideFlags = static_cast<std::uint32_t>(buf->getBitLong());
    format.m_propertyFlags = static_cast<std::uint32_t>(buf->getBitLong());
    format.m_valueDataType = buf->getBitLong();
    format.m_valueUnitType = buf->getBitLong();
    format.m_valueFormatString = textBuf->getVariableText(version, false);
    format.m_rotation = buf->getBitDouble();
    format.m_blockScale = buf->getBitDouble();
    format.m_cellAlignment = buf->getBitLong();
    format.m_contentColor = readObjectCmColor(version, buf, strBuf);
    format.m_textStyleHandle = readObjectHandleRef(hdlBuf);
    format.m_textHeight = buf->getBitDouble();
    const bool good = buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
    if (ranges != nullptr) {
        ranges->push_back(makeObjectSubrecordRange(
            "table-style-content-format", startBit, currentObjectDwgBit(buf),
            version, 1, good));
    }
    return good;
}

bool readTableStyleCellStyle(DRW::Version version, dwgBuffer *buf,
                             dwgBuffer *strBuf, dwgBuffer *hdlBuf,
                             DRW_TableStyleCellStyle& style,
                             std::vector<DRW_DwgSubrecordRange> *ranges = nullptr) {
    const std::uint64_t startBit = currentObjectDwgBit(buf);
    style.m_type = buf->getBitLong();
    style.m_hasData = buf->getBitShort() != 0;
    if (!style.m_hasData) {
        if (ranges != nullptr) {
            ranges->push_back(makeObjectSubrecordRange(
                "table-style-cell-style", startBit, currentObjectDwgBit(buf),
                version, 0, buf->isGood()));
        }
        return buf->isGood();
    }

    style.m_propertyOverrideFlags = static_cast<std::uint32_t>(buf->getBitLong());
    style.m_mergeFlags = static_cast<std::uint32_t>(buf->getBitLong());
    style.m_backgroundColor = readObjectCmColor(version, buf, strBuf);
    style.m_contentLayout = static_cast<std::uint32_t>(buf->getBitLong());
    if (!readTableStyleContentFormat(version, buf, strBuf, hdlBuf,
                                     style.m_contentFormat, ranges))
        return false;

    style.m_marginOverrideFlags = buf->getBitShort();
    if (style.m_marginOverrideFlags != 0) {
        style.m_verticalMargin = buf->getBitDouble();
        style.m_horizontalMargin = buf->getBitDouble();
        style.m_bottomMargin = buf->getBitDouble();
        style.m_rightMargin = buf->getBitDouble();
        style.m_marginHorizontalSpacing = buf->getBitDouble();
        style.m_marginVerticalSpacing = buf->getBitDouble();
    }

    const std::uint32_t borderCount = static_cast<std::uint32_t>(buf->getBitLong());
    if (borderCount > 6) {
        if (ranges != nullptr) {
            ranges->push_back(makeObjectSubrecordRange(
                "table-style-cell-style", startBit, currentObjectDwgBit(buf),
                version, borderCount, false));
        }
        return false;
    }
    style.m_borders.clear();
    style.m_borders.reserve(borderCount);
    for (std::uint32_t i = 0; i < borderCount; ++i) {
        DRW_TableStyleBorder border;
        border.m_edgeFlags = buf->getBitLong();
        if (border.m_edgeFlags != 0) {
            border.m_propertyOverrideFlags = buf->getBitLong();
            border.m_borderType = buf->getBitLong();
            border.m_color = readObjectCmColor(version, buf, strBuf);
            border.m_lineWeight = buf->getBitLong();
            border.m_lineTypeHandle = readObjectHandleRef(hdlBuf);
            border.m_visible = buf->getBitLong();
            border.m_doubleLineSpacing = buf->getBitDouble();
        }
        style.m_borders.push_back(border);
    }

    const bool good = buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
    if (ranges != nullptr) {
        ranges->push_back(makeObjectSubrecordRange(
            "table-style-cell-style", startBit, currentObjectDwgBit(buf),
            version, borderCount, good));
    }
    return good;
}

bool readLegacyTableStyleRowStyle(DRW::Version version, dwgBuffer *buf,
                                  dwgBuffer *strBuf, dwgBuffer *hdlBuf,
                                  DRW_TableStyleRowStyle& rowStyle) {
    dwgBuffer *textBuf = strBuf ? strBuf : buf;
    rowStyle.m_textStyleHandle = readObjectHandleRef(hdlBuf);
    rowStyle.m_textHeight = buf->getBitDouble();
    rowStyle.m_textAlignment = buf->getBitShort();
    rowStyle.m_textColor = readObjectCmColor(version, buf, strBuf);
    rowStyle.m_fillColor = readObjectCmColor(version, buf, strBuf);
    rowStyle.m_hasBackgroundColor = buf->getBit() != 0;

    rowStyle.m_borders.clear();
    rowStyle.m_borders.reserve(6);
    for (int i = 0; i < 6; ++i) {
        DRW_TableStyleBorder border;
        border.m_edgeFlags = 1 << i;
        border.m_lineWeight = buf->getBitShort();
        border.m_visible = buf->getBit() != 0;
        border.m_color = readObjectCmColor(version, buf, strBuf);
        rowStyle.m_borders.push_back(border);
    }

    if (version > DRW::AC1018) {
        rowStyle.m_valueDataType = buf->getBitLong();
        rowStyle.m_valueUnitType = buf->getBitLong();
        rowStyle.m_valueFormatString = textBuf->getVariableText(version, false);
    }

    return buf->isGood() && (!strBuf || strBuf->isGood()) && (!hdlBuf || hdlBuf->isGood());
}

bool readCadValue(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf,
                  DRW_CadValue& value) {
    if (version > DRW::AC1018)
        value.m_formatFlags = buf->getBitLong();

    value.m_dataType = buf->getBitLong();
    const bool emptyR2007Value = version > DRW::AC1018 && (value.m_formatFlags & 3);
    if (!emptyR2007Value) {
        switch (value.m_dataType) {
        case 0:
        case 1:
            value.m_value.addInt(91, buf->getBitLong());
            break;
        case 2:
            value.m_value.addDouble(140, buf->getBitDouble());
            break;
        case 4:
        case 512:
            if (!readCadValueBytes(buf, value.m_rawData, "FIELD value byte payload"))
                return false;
            value.m_dataSize = static_cast<std::uint32_t>(value.m_rawData.size());
            value.m_value.addString(1, decodeCadValueText(version, buf, value.m_rawData));
            break;
        case 8: {
            if (!readCadValueBytes(buf, value.m_rawData, "FIELD value date payload"))
                return false;
            value.m_dataSize = static_cast<std::uint32_t>(value.m_rawData.size());
            value.m_value.addBinary(310, value.m_rawData);
            break;
        }
        case 16:
            if (!readCadValuePoint(buf, value, 2))
                return false;
            break;
        case 32:
            if (!readCadValuePoint(buf, value, 3))
                return false;
            break;
        case 64: {
            dwgHandle h = buf->getHandle();
            value.m_handle = h.ref;
            value.m_value.addInt(330, static_cast<std::uint32_t>(h.ref));
            break;
        }
        case 128:
        case 256:
            DRW_DBG("unsupported FIELD value buffer type: ");
            DRW_DBG(value.m_dataType); DRW_DBG("\n");
            return false;
        default:
            DRW_DBG("invalid FIELD value type: ");
            DRW_DBG(value.m_dataType); DRW_DBG("\n");
            return false;
        }
    }

    if (version > DRW::AC1018) {
        dwgBuffer *textBuf = strBuf ? strBuf : buf;
        value.m_unitType = buf->getBitLong();
        value.m_formatString = textBuf->getVariableText(version, false);
        value.m_valueString = textBuf->getVariableText(version, false);
    }

    return buf->isGood() && (!strBuf || strBuf->isGood());
}

bool xRecordCodeIsString(int code) {
    return (code >= 0 && code <= 9) || (code >= 100 && code <= 102) ||
           (code >= 300 && code <= 309) || (code >= 410 && code <= 419) ||
           (code >= 430 && code <= 439) || (code >= 470 && code <= 479) ||
           code == 999 ||
           (code >= 1000 && code <= 1003);
}

bool xRecordCodeIsPoint3D(int code) {
    return (code >= 10 && code <= 37) || (code >= 110 && code <= 139) ||
           (code >= 210 && code <= 269) || (code >= 1010 && code <= 1039);
}

bool xRecordCodeIsDouble(int code) {
    return (code >= 38 && code <= 59) || (code >= 140 && code <= 149) ||
           (code >= 460 && code <= 469) || (code >= 1040 && code <= 1042);
}

bool xRecordCodeIsInt16(int code) {
    return (code >= 60 && code <= 79) || (code >= 170 && code <= 179) ||
           (code >= 270 && code <= 279) || (code >= 370 && code <= 389) ||
           (code >= 400 && code <= 409) || (code >= 1060 && code <= 1070);
}

bool xRecordCodeIsInt32(int code) {
    return (code >= 90 && code <= 99) || (code >= 420 && code <= 429) ||
           (code >= 440 && code <= 459) || code == 1071;
}

bool xRecordCodeIsBool(int code) {
    return code >= 290 && code <= 299;
}

bool xRecordCodeIsByte(int code) {
    return code >= 280 && code <= 289;
}

bool xRecordCodeIsBinary(int code) {
    return (code >= 310 && code <= 319) || code == 1004;
}

bool xRecordCodeIsHandle(int code) {
    return (code >= 320 && code <= 369) || (code >= 390 && code <= 399) ||
           (code >= 480 && code <= 481) || code == 5 || code == 105 ||
           (code >= 1005 && code <= 1009);
}

} // namespace

//! Base class for tables entries
/*!
*  Base class for tables entries
*  @author Rallaz
*/
bool DRW_TableEntry::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case DRW::dxfCode::HANDLE:
        handle = reader->getHandleString();
        break;
    case DRW::dxfCode::OWNER_HANDLE:
        parentHandle = reader->getHandleString();
        break;
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
        extData.push_back(new DRW_Variant(code, reader->getString()));
        break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
        // don't trust in X, Y, Z order!
        if (nullptr != curr) {
            curr->setCoordX( reader->getDouble());
        }
        else {
            curr = new DRW_Variant( code, DRW_Coord( reader->getDouble(), 0.0, 0.0));
            extData.push_back(curr);
        }
        break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
        // don't trust in X, Y, Z order!
        if (nullptr != curr) {
            curr->setCoordY( reader->getDouble());
        }
        else {
            curr = new DRW_Variant( code, DRW_Coord( 0.0, reader->getDouble(), 0.0));
            extData.push_back(curr);
        }
        break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
        // don't trust in X, Y, Z order!
        if (nullptr != curr) {
            curr->setCoordZ( reader->getDouble());
        }
        else {
            curr = new DRW_Variant( code, DRW_Coord( 0.0, 0.0, reader->getDouble()));
            extData.push_back(curr);
        }
        break;
    case 1040:
    case 1041:
    case 1042:
        extData.push_back(new DRW_Variant(code, reader->getDouble()));
        break;
    case 1070:
    case 1071:
        extData.push_back(new DRW_Variant(code, reader->getInt32() ));
        break;
    default:
        break;
    }

    return true;
}

bool DRW_TableEntry::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer *strBuf, std::uint32_t bs){
DRW_DBG("\n***************************** parsing table entry *********************************************\n");
    objSize=0;
    oType = buf->getObjType(version);
    DRW_DBG("Object type: "); DRW_DBG(oType); DRW_DBG(", "); DRW_DBGH(oType);
    if (version > DRW::AC1014 && version < DRW::AC1024) {//2000 to 2007
        objSize = buf->getRawLong32();  //RL 32bits object size in bits
        DRW_DBG(" Object size: "); DRW_DBG(objSize); DRW_DBG("\n");
    }
    if (version > DRW::AC1021) {//2010+
        std::uint32_t ms = buf->size();
        if (bs > ms*8u)
            return false;
        objSize = ms*8 - bs;
        DRW_DBG(" Object size: "); DRW_DBG(objSize); DRW_DBG("\n");
    }
    if (strBuf != NULL && version > DRW::AC1018) {//2007+
        strBuf->moveBitPos(objSize-1);
        DRW_DBG(" strBuf strbit pos 2007: "); DRW_DBG(strBuf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(strBuf->getBitPos()); DRW_DBG("\n");
        if (strBuf->getBit() == 1){
            DRW_DBG("DRW_TableEntry::parseDwg string bit is 1\n");
            strBuf->moveBitPos(-17);
            std::uint16_t strDataSize = strBuf->getRawShort16();
            DRW_DBG("\nDRW_TableEntry::parseDwg string strDataSize: "); DRW_DBGH(strDataSize); DRW_DBG("\n");
            if ( (strDataSize& 0x8000) == 0x8000){
                DRW_DBG("\nDRW_TableEntry::parseDwg string 0x8000 bit is set");
                strBuf->moveBitPos(-32);
                std::uint16_t hiSize = strBuf->getRawShort16();
                strDataSize = ((strDataSize&0x7fff) | (hiSize<<15));
            }
            strBuf->moveBitPos( -strDataSize -16); //-14
            DRW_DBG("strBuf start strDataSize pos 2007: "); DRW_DBG(strBuf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(strBuf->getBitPos()); DRW_DBG("\n");
        } else
            DRW_DBG("\nDRW_TableEntry::parseDwg string bit is 0");
        DRW_DBG("strBuf start pos 2007: "); DRW_DBG(strBuf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(strBuf->getBitPos()); DRW_DBG("\n");
    }

    dwgHandle ho = buf->getHandle();
    handle = ho.ref;
    DRW_DBG("TableEntry Handle: "); DRW_DBGHL(ho.code, ho.size, ho.ref);
    std::int16_t extDataSize = buf->getBitShort(); //BS
    DRW_DBG(" ext data size: "); DRW_DBG(extDataSize);
    while (extDataSize>0 && buf->isGood()) {
        /* RLZ: TODO */
        dwgHandle ah = buf->getHandle();
        DRW_DBG("App Handle: "); DRW_DBGHL(ah.code, ah.size, ah.ref);
        std::vector<std::uint8_t> tmpExtData(static_cast<std::size_t>(extDataSize));
        if (!buf->getBytes(tmpExtData.data(), extDataSize))
            return false;
        dwgBuffer tmpExtDataBuf(tmpExtData.data(), extDataSize, buf->decoder);
        int pos = tmpExtDataBuf.getPosition();
        int bpos = tmpExtDataBuf.getBitPos();
        DRW_DBG("ext data pos: "); DRW_DBG(pos); DRW_DBG("."); DRW_DBG(bpos); DRW_DBG("\n");
        std::uint8_t dxfCode = tmpExtDataBuf.getRawChar8();
        DRW_DBG(" dxfCode: "); DRW_DBG(dxfCode);
        switch (dxfCode){
        case 0:{
            // ODA DWG spec §28 EED string item. Two encodings:
            //  - R2007+: 2-byte char count + UTF-16LE bytes (no per-string codepage).
            //  - R13–R2004: 1-byte length + 2-byte BE codepage hint + bytes + NUL.
            std::string s;
            if (version > DRW::AC1018) {
                if (tmpExtDataBuf.numRemainingBytes() < 2) break;
                std::uint16_t nChars = tmpExtDataBuf.getRawShort16();
                if (nChars > 0) {
                    std::uint64_t byteLen = static_cast<std::uint64_t>(nChars) * 2;
                    if ((std::uint64_t)tmpExtDataBuf.numRemainingBytes() < byteLen) break;
                    std::vector<std::uint8_t> bytes(byteLen);
                    tmpExtDataBuf.getBytes(bytes.data(), byteLen);
                    for (std::uint16_t i = 0; i < nChars; ++i) {
                        std::uint16_t c = static_cast<std::uint16_t>(bytes[2*i]) |
                                   (static_cast<std::uint16_t>(bytes[2*i+1]) << 8);
                        if (c < 0x80) {
                            s.push_back(static_cast<char>(c));
                        } else if (c < 0x800) {
                            s.push_back(static_cast<char>(0xC0 | (c >> 6)));
                            s.push_back(static_cast<char>(0x80 | (c & 0x3F)));
                        } else {
                            s.push_back(static_cast<char>(0xE0 | (c >> 12)));
                            s.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
                            s.push_back(static_cast<char>(0x80 | (c & 0x3F)));
                        }
                    }
                }
            } else {
                std::uint8_t strLength = tmpExtDataBuf.getRawChar8();
                DRW_DBG(" strLength: "); DRW_DBG(strLength);
                std::uint16_t cp = tmpExtDataBuf.getBERawShort16();
                DRW_DBG(" str codepage: "); DRW_DBG(cp);
                if (strLength > 0 && tmpExtDataBuf.numRemainingBytes() >= strLength) {
                    std::string raw(strLength, '\0');
                    tmpExtDataBuf.getBytes(reinterpret_cast<std::uint8_t*>(&raw[0]), strLength);
                    s = decodeEedString(cp, raw, tmpExtDataBuf.decoder);
                }
                if (tmpExtDataBuf.numRemainingBytes() > 0)
                    tmpExtDataBuf.getRawChar8(); // optional trailing NUL
            }
            extData.push_back(new DRW_Variant(1000, s));
            break;
        }
        default:
            /* RLZ: TODO */
            break;
        }
        DRW_DBG("ext data pos: "); DRW_DBG(tmpExtDataBuf.getPosition()); DRW_DBG("."); DRW_DBG(tmpExtDataBuf.getBitPos()); DRW_DBG("\n");
        extDataSize = buf->getBitShort(); //BS
        DRW_DBG(" ext data size: "); DRW_DBG(extDataSize);
    } //end parsing extData (EED)
    if (version < DRW::AC1015) {//14-
        objSize = buf->getRawLong32();  //RL 32bits size in bits
    }
    DRW_DBG(" objSize in bits: "); DRW_DBG(objSize);

    numReactors = buf->getBitLong(); //BL
    DRW_DBG(", numReactors: "); DRW_DBG(numReactors); DRW_DBG("\n");
    if (version > DRW::AC1015) {//2004+
        xDictFlag = buf->getBit();
        DRW_DBG("xDictFlag: "); DRW_DBG(xDictFlag);
    }
    if (version > DRW::AC1024) {//2013+
        std::uint8_t bd = buf->getBit();
        DRW_DBG(" Have binary data: "); DRW_DBG(bd); DRW_DBG("\n");
    }
    return buf->isGood();
}

//! Class to handle dimstyle entries
/*!
*  Class to handle ldim style symbol table entries
*  @author Rallaz
*/
bool DRW_Dimstyle::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 105:
        handle = reader->getHandleString();
        break;
    case 3:
        dimpost = reader->getUtf8String();
        break;
    case 4:
        dimapost = reader->getUtf8String();
        break;
    case 5:
        dimblk = reader->getUtf8String();
        break;
    case 6:
        dimblk1 = reader->getUtf8String();
        break;
    case 7:
        dimblk2 = reader->getUtf8String();
        break;
    case 40:
        dimscale = reader->getDouble();
        break;
    case 41:
        dimasz = reader->getDouble();
        break;
    case 42:
        dimexo = reader->getDouble();
        break;
    case 43:
        dimdli = reader->getDouble();
        break;
    case 44:
        dimexe = reader->getDouble();
        break;
    case 45:
        dimrnd = reader->getDouble();
        break;
    case 46:
        dimdle = reader->getDouble();
        break;
    case 47:
        dimtp = reader->getDouble();
        break;
    case 48:
        dimtm = reader->getDouble();
        break;
    case 49:
        dimfxl = reader->getDouble();
        break;
    case 140:
        dimtxt = reader->getDouble();
        break;
    case 141:
        dimcen = reader->getDouble();
        break;
    case 142:
        dimtsz = reader->getDouble();
        break;
    case 143:
        dimaltf = reader->getDouble();
        break;
    case 144:
        dimlfac = reader->getDouble();
        break;
    case 145:
        dimtvp = reader->getDouble();
        break;
    case 146:
        dimtfac = reader->getDouble();
        break;
    case 147:
        dimgap = reader->getDouble();
        break;
    case 148:
        dimaltrnd = reader->getDouble();
        break;
    case 71:
        dimtol = reader->getInt32();
        break;
    case 72:
        dimlim = reader->getInt32();
        break;
    case 73:
        dimtih = reader->getInt32();
        break;
    case 74:
        dimtoh = reader->getInt32();
        break;
    case 75:
        dimse1 = reader->getInt32();
        break;
    case 76:
        dimse2 = reader->getInt32();
        break;
    case 77:
        dimtad = reader->getInt32();
        break;
    case 78:
        dimzin = reader->getInt32();
        break;
    case 79:
        dimazin = reader->getInt32();
        break;
    case 170:
        dimalt = reader->getInt32();
        break;
    case 171:
        dimaltd = reader->getInt32();
        break;
    case 172:
        dimtofl = reader->getInt32();
        break;
    case 173:
        dimsah = reader->getInt32();
        break;
    case 174:
        dimtix = reader->getInt32();
        break;
    case 175:
        dimsoxd = reader->getInt32();
        break;
    case 176:
        dimclrd = reader->getInt32();
        break;
    case 177:
        dimclre = reader->getInt32();
        break;
    case 178:
        dimclrt = reader->getInt32();
        break;
    case 179:
        dimadec = reader->getInt32();
        break;
    case 270:
        dimunit = reader->getInt32();
        break;
    case 271:
        dimdec = reader->getInt32();
        break;
    case 272:
        dimtdec = reader->getInt32();
        break;
    case 273:
        dimaltu = reader->getInt32();
        break;
    case 274:
        dimalttd = reader->getInt32();
        break;
    case 275:
        dimaunit = reader->getInt32();
        break;
    case 276:
        dimfrac = reader->getInt32();
        break;
    case 277:
        dimlunit = reader->getInt32();
        break;
    case 278:
        dimdsep = reader->getInt32();
        break;
    case 279:
        dimtmove = reader->getInt32();
        break;
    case 280:
        dimjust = reader->getInt32();
        break;
    case 281:
        dimsd1 = reader->getInt32();
        break;
    case 282:
        dimsd2 = reader->getInt32();
        break;
    case 283:
        dimtolj = reader->getInt32();
        break;
    case 284:
        dimtzin = reader->getInt32();
        break;
    case 285:
        dimaltz = reader->getInt32();
        break;
    case 286:
        dimaltttz = reader->getInt32();
        break;
    case 287:
        dimfit = reader->getInt32();
        break;
    case 288:
        dimupt = reader->getInt32();
        break;
    case 289:
        dimatfit = reader->getInt32();
        break;
    case 290:
        dimfxlon = reader->getInt32();
        break;
    case 340:
        dimtxsty = reader->getUtf8String();
        break;
    case 341:
        dimldrblk = reader->getUtf8String();
        break;
    case 342:
        dimblk = reader->getUtf8String();
        break;
    case 343:
        dimblk1 = reader->getUtf8String();
        break;
    case 344:
        dimblk2 = reader->getUtf8String();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }

    return true;
}

// Populate the vars map from the parsed struct fields using EXACTLY the $DIM
// keys / DXF codes the LibreCAD createDimStyle consumer reads. Idempotent: the
// `if (!get(key))` guard never clobbers a value already present (DWG override
// path or DXF group-105 handles). (Phase 3A.0)
void DRW_Dimstyle::syncStructToVars() {
    auto addDouble = [&](const char* key, int code, double value) {
        if (!get(key)) add(key, code, value);
    };
    auto addInt = [&](const char* key, int code, int value) {
        if (!get(key)) add(key, code, value);
    };
    auto addStr = [&](const char* key, int code, const std::string& value) {
        if (!get(key)) add(key, code, value);
    };

    // Doubles.
    addDouble("$DIMSCALE", 40, dimscale);
    addDouble("$DIMASZ", 41, dimasz);
    addDouble("$DIMEXO", 42, dimexo);
    addDouble("$DIMEXE", 44, dimexe);
    addDouble("$DIMFXL", 49, dimfxl);
    addDouble("$DIMDLE", 46, dimdle);
    addDouble("$DIMDLI", 43, dimdli);
    addDouble("$DIMGAP", 147, dimgap);
    addDouble("$DIMTXT", 140, dimtxt);
    addDouble("$DIMTVP", 145, dimtvp);
    addDouble("$DIMRND", 45, dimrnd);
    addDouble("$DIMALTRND", 148, dimaltrnd);
    addDouble("$DIMCEN", 141, dimcen);
    addDouble("$DIMTM", 48, dimtm);
    addDouble("$DIMTP", 47, dimtp);
    addDouble("$DIMTFAC", 146, dimtfac);
    addDouble("$DIMTSZ", 142, dimtsz);
    addDouble("$DIMLFAC", 144, dimlfac);
    addDouble("$DIMALTF", 143, dimaltf);

    // Ints.
    addInt("$DIMSOXD", 175, dimsoxd);
    addInt("$DIMSAH", 173, dimsah);
    addInt("$DIMFXLON", 290, dimfxlon);
    addInt("$DIMSE1", 75, dimse1);
    addInt("$DIMSE2", 76, dimse2);
    addInt("$DIMTOFL", 172, dimtofl);
    addInt("$DIMTOH", 74, dimtoh);
    addInt("$DIMTIH", 73, dimtih);
    addInt("$DIMJUST", 280, dimjust);
    addInt("$DIMTAD", 77, dimtad);
    addInt("$DIMTIX", 174, dimtix);
    addInt("$DIMUPT", 288, dimupt);
    addInt("$DIMTMOVE", 279, dimtmove);
    addInt("$DIMATFIT", 289, dimatfit);
    addInt("$DIMZIN", 78, dimzin);
    addInt("$DIMAZIN", 79, dimazin);
    addInt("$DIMTZIN", 284, dimtzin);
    addInt("$DIMALTZ", 285, dimaltz);
    addInt("$DIMALTTZ", 286, dimaltttz);
    addInt("$DIMLUNIT", 277, dimlunit);
    addInt("$DIMDSEP", 278, dimdsep);
    addInt("$DIMDEC", 271, dimdec);
    addInt("$DIMALT", 170, dimalt);
    addInt("$DIMALTU", 273, dimaltu);
    addInt("$DIMALTD", 171, dimaltd);
    addInt("$DIMAUNIT", 275, dimaunit);
    addInt("$DIMADEC", 179, dimadec);
    addInt("$DIMFRAC", 276, dimfrac);
    addInt("$DIMTDEC", 272, dimtdec);
    addInt("$DIMALTTD", 274, dimalttd);
    addInt("$DIMTOL", 71, dimtol);
    addInt("$DIMTOLJ", 283, dimtolj);
    addInt("$DIMLIM", 72, dimlim);
    addInt("$DIMLWD", 371, dimlwd);
    addInt("$DIMLWE", 372, dimlwe);
    // Color ints — createDimStyle calls numberToColor(var->i_val()).
    addInt("$DIMCLRD", 176, dimclrd);
    addInt("$DIMCLRE", 177, dimclre);
    addInt("$DIMCLRT", 178, dimclrt);

    // Strings.
    addStr("$DIMPOST", 3, dimpost);
    addStr("$DIMAPOST", 4, dimapost);
    addStr("$DIMTXSTY", 340, dimtxsty);   // NAME; consumer runs prepareTextStyleName
    addStr("$DIMLDRBLK", 341, dimldrblk);
    addStr("$DIMBLK", 5, dimblk);
    addStr("$DIMBLK1", 6, dimblk1);
    addStr("$DIMBLK2", 7, dimblk2);
}

bool DRW_Dimstyle::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing dimension style **************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    DRW_DBG("dimension style name: "); DRW_DBG(name.c_str()); DRW_DBG("\n");

//    handleObj = shpControlH.ref;
    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    //    RS crc;   //RS */
    return buf->isGood();
}


//! Class to handle line type entries
/*!
*  Class to handle line type symbol table entries
*  @author Rallaz
*/
bool DRW_LType::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 3:
        desc = reader->getUtf8String();
        break;
    case 73:
        size = reader->getInt32();
        path.clear();
        if (!DRW::reserve( path, size)) {
            return false;
        }
        break;
    case 40:
        length = reader->getDouble();
        break;
    case 49:
        path.push_back(reader->getDouble());
        pathIdx++;
        break;
/*    case 74:
        haveShape = reader->getInt32();
        break;*/
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }

    return true;
}

//! Update line type
/*!
*  Update the size and length of line type according to the path
*  @author Rallaz
*/
/*TODO: control max length permited */
void DRW_LType::update(){
    double d =0;
    size = path.size();
    for (int i = 0;  i< size; i++){
        d += fabs(path.at(i));
    }
    length = d;
}

bool DRW_LType::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing line type *********************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    DRW_DBG("linetype name: "); DRW_DBG(name.c_str()); DRW_DBG("\n");
    flags = buf->getBit()<< 6;
    DRW_DBG("flags: "); DRW_DBG(flags);
    if (version > DRW::AC1018) {//2007+
    } else {//2004- //RLZ: verify in 2004, 2010 &2013
        std::int16_t xrefindex = buf->getBitShort();
        DRW_DBG(" xrefindex: "); DRW_DBG(xrefindex);
    }
    std::uint8_t xdep = buf->getBit();
    DRW_DBG(" xdep: "); DRW_DBG(xdep);
    flags |= xdep<< 4;
    DRW_DBG(" flags: "); DRW_DBG(flags);
    desc = sBuf->getVariableText(version, false);
    DRW_DBG(" desc: "); DRW_DBG(desc.c_str());
    length = buf->getBitDouble();
    DRW_DBG(" pattern length: "); DRW_DBG(length);
    char align = buf->getRawChar8();
    DRW_DBG(" align: "); DRW_DBG(std::string(&align, 1));
    size = buf->getRawChar8();
    DRW_DBG(" num dashes, size: "); DRW_DBG(size);
    DRW_DBG("\n    dashes:\n");
    bool haveStrArea = false;
    for (int i=0; i< size; i++){
        path.push_back(buf->getBitDouble());
        /*int bs1 =*/ buf->getBitShort();
        /*double d1= */buf->getRawDouble();
        /*double d2=*/ buf->getRawDouble();
        /*double d3= */buf->getBitDouble();
        /*double d4= */buf->getBitDouble();
        int bs2 = buf->getBitShort();
        if((bs2 & 2) !=0) haveStrArea = true;
    }
    for (unsigned i=0; i<path.size() ; i++){
        DRW_DBG(", "); DRW_DBG(path[i]); DRW_DBG("\n");
    }
    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    if (version < DRW::AC1021) { //2004-
        std::uint8_t strarea[256];
        buf->getBytes(strarea, 256);
        DRW_DBG("string area 256 bytes:\n"); DRW_DBG(reinterpret_cast<char*>(strarea)); DRW_DBG("\n");
    } else { //2007+
        //first verify flag
        if (haveStrArea) {
            std::uint8_t strarea[512];
            buf->getBytes(strarea, 512);
            DRW_DBG("string area 512 bytes:\n"); DRW_DBG(reinterpret_cast<char*>(strarea)); DRW_DBG("\n");
        } else
            DRW_DBG("string area 512 bytes not present\n");
    }

    if (version > DRW::AC1021) {//2007+ skip string area
        DRW_DBG(" ltype end of object data pos 2010: "); DRW_DBG(buf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(buf->getBitPos()); DRW_DBG("\n");
    }
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }

    if (version > DRW::AC1021) {//2007+ skip string area
        DRW_DBG(" ltype start of handles data pos 2010: "); DRW_DBG(buf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(buf->getBitPos()); DRW_DBG("\n");
    }

    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    dwgHandle ltControlH = buf->getHandle();
    DRW_DBG("linetype control Handle: "); DRW_DBGHL(ltControlH.code, ltControlH.size, ltControlH.ref);
    parentHandle = ltControlH.ref;
    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    for (int i=0; i< numReactors;++i) {
        dwgHandle reactorsH = buf->getHandle();
        DRW_DBG(" reactorsH control Handle: "); DRW_DBGHL(reactorsH.code, reactorsH.size, reactorsH.ref); DRW_DBG("\n");
    }
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref); DRW_DBG("\n");
        DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    }
    // ODA 20.4.58 LTYPE handle stream: external-reference block handle (always
    // present per COMMON_TABLE_FLAGS) followed by one shapefile/style handle
    // (DXF 340) PER DASH.  The per-dash handle is emitted unconditionally in the
    // DWG binary stream (NULL when the dash has no shape); only the DXF 340
    // group is gated by the dash shapeflag.  The legacy code read a fixed 2-or-1
    // trailing handles, which over-read for 1 dash and under-read for >=3 dashes,
    // desyncing the next object.  Cross-checked: libreDWG dwg.spec LTYPE REPEAT
    // block (per-dash SUB_FIELD_HANDLE style, code 340).
    dwgHandle xRefH = buf->getHandle();
    DRW_DBG(" XRefH control Handle: "); DRW_DBGHL(xRefH.code, xRefH.size, xRefH.ref); DRW_DBG("\n");
    for (int i = 0; i < size; ++i) {
        dwgHandle dashStyleH = buf->getHandle();
        DRW_DBG(" dash["); DRW_DBG(i); DRW_DBG("] style Handle: ");
        DRW_DBGHL(dashStyleH.code, dashStyleH.size, dashStyleH.ref); DRW_DBG("\n");
    }

    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

//! Class to handle layer entries
/*!
*  Class to handle layer symbol table entries
*  @author Rallaz
*/
bool DRW_Layer::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 6:
        lineType = reader->getUtf8String();
        break;
    case DRW::dxfCode::COLOR:
        color = reader->getInt32();
        break;
    case 290:
        plotF = reader->getBool();
        break;
    case DRW::dxfCode::LINEWEIGHT:
        lWeight = DRW_LW_Conv::dxfInt2lineWidth(reader->getInt32());
        break;
    case DRW::dxfCode::PLOTSTYLE:
        handlePlotS = reader->getString();
        break;
    case 347:
        handleMaterialS = reader->getString();
        break;
    case 420:
        color24 = reader->getInt32();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }

    return true;
}

bool DRW_Layer::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing layer *********************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    DRW_DBG("layer name: "); DRW_DBG(name.c_str());

    flags |= buf->getBit()<< 6;//layer have entity
    if (version < DRW::AC1021) {//2004-
        DRW_DBG(", xrefindex = "); DRW_DBG(buf->getBitShort()); DRW_DBG("\n");
        //std::int16_t xrefindex = buf->getBitShort();
    }
    flags |= buf->getBit() << 4;//is refx dependent
    if (version < DRW::AC1015) {//14-
        flags |= buf->getBit(); //layer frozen
        /*flags |=*/ buf->getBit(); //unused, negate the color
        flags |= buf->getBit() << 1;//frozen in new
        flags |= buf->getBit()<< 3;//locked
    }
    if (version > DRW::AC1014) {//2000+
        std::int16_t f = buf->getSBitShort();//bit2 are layer on
        DRW_DBG(", flags 2000+: "); DRW_DBG(f); DRW_DBG("\n");
        flags |= f & 0x0001; //layer frozen
        flags |= ( f>> 1) & 0x0002;//frozen in new
        flags |= ( f>> 1) & 0x0004;//locked
        plotF = ( f>> 4) & 0x0001;
        lWeight = DRW_LW_Conv::dwgInt2lineWidth( (f & 0x03E0) >> 5 );
    }
    {
        UTF8STRING cmcName, cmcBookName;
        color = buf->getCmColor(version, &color24, sBuf, &cmcName, &cmcBookName);
        if (!cmcName.empty()) {
            // libreDWG-style join: "BOOK$ENTRY" when book name is present,
            // otherwise just the entry name. Matches the format used by
            // dwgReader::dbColorMap / DRW_DbColor's bookName + "$" + name.
            colorName = cmcBookName.empty()
                ? cmcName
                : (cmcBookName + "$" + cmcName);
            DRW_DBG(" CMC name resolved: "); DRW_DBG(colorName.c_str()); DRW_DBG("\n");
        }
    }
    DRW_DBG(", entity color: "); DRW_DBG(color); DRW_DBG(", color24: "); DRW_DBG(color24); DRW_DBG("\n");

    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
    dwgHandle layerControlH = buf->getHandle();
    DRW_DBG("layer control Handle: "); DRW_DBGHL(layerControlH.code, layerControlH.size, layerControlH.ref);
    parentHandle = layerControlH.ref;

    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref); DRW_DBG("\n");
    }
    dwgHandle XRefH = buf->getHandle();
    DRW_DBG(" XRefH control Handle: "); DRW_DBGHL(XRefH.code, XRefH.size, XRefH.ref); DRW_DBG("\n");
    if (version > DRW::AC1014) {//2000+
        dwgHandle plotStyH = buf->getHandle();
        DRW_DBG(" PLot style control Handle: "); DRW_DBGHL(plotStyH.code, plotStyH.size, plotStyH.ref); DRW_DBG("\n");
        handlePlotS = DRW::toHexStr(plotStyH.ref);// std::string(plotStyH.ref);//RLZ: verify conversion
    }
    if (version > DRW::AC1018) {//2007+
        dwgHandle materialH = buf->getHandle();
        DRW_DBG(" Material control Handle: "); DRW_DBGHL(materialH.code, materialH.size, materialH.ref); DRW_DBG("\n");
        handleMaterialS = DRW::toHexStr(materialH.ref);//RLZ: verify conversion
    }
    //lineType handle
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    lTypeH = buf->getHandle();
    DRW_DBG("line type Handle: "); DRW_DBGHL(lTypeH.code, lTypeH.size, lTypeH.ref);
    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Block_Record::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing block record ******************************************\n");
    if (!ret)
        return ret;
    std::uint32_t insertCount = 0;//only 2000+
    std::uint32_t objectCount = 0; //only 2004+

    name = sBuf->getVariableText(version, false);
    DRW_DBG("block record name: "); DRW_DBG(name.c_str()); DRW_DBG("\n");

    flags |= buf->getBit()<< 6;//referenced external reference, block code 70, bit 7 (64)
    if (version > DRW::AC1018) {//2007+
    } else {//2004- //RLZ: verify in 2004, 2010 &2013
        std::int16_t xrefindex = buf->getBitShort();
        DRW_DBG(" xrefindex: "); DRW_DBG(xrefindex); DRW_DBG("\n");
    }
    flags |= buf->getBit() << 4;//is refx dependent, block code 70, bit 5 (16)
    flags |= buf->getBit(); //if is anonymous block (*U) block code 70, bit 1 (1)
    flags |= buf->getBit() << 1; //if block contains attdefs, block code 70, bit 2 (2)
    bool blockIsXref = buf->getBit(); //if is a Xref, block code 70, bit 3 (4)
    bool xrefOverlaid = buf->getBit(); //if is a overlaid Xref, block code 70, bit 4 (8)
    flags |= blockIsXref << 2; //if is a Xref, block code 70, bit 3 (4)
    flags |= xrefOverlaid << 3; //if is a overlaid Xref, block code 70, bit 4 (8)
    if (version > DRW::AC1014) {//2000+
        flags |= buf->getBit() << 5; //if is a loaded Xref, block code 70, bit 6 (32)
    }
    DRW_DBG("flags: "); DRW_DBG(flags); DRW_DBG(", ");
    // Per ODA spec / libreDWG dwg.spec (SINCE R_2004a), num_owned is only
    // present when the block_record is neither an xref nor an overlaid xref.
    // Reading it unconditionally for XREFs misaligns the rest of the parse.
    if (version > DRW::AC1015 && !blockIsXref && !xrefOverlaid) {
        objectCount = buf->getBitLong(); //Number of objects owned by this block
        if (!DRW::reserve( entMap, objectCount)) {
            return false;
        }
    }
    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    DRW_DBG("insertion point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z); DRW_DBG("\n");
    xrefPath = sBuf->getVariableText(version, false);
    DRW_DBG("XRef path name: "); DRW_DBG(xrefPath.c_str()); DRW_DBG("\n");

    if (version > DRW::AC1014) {//2000+
        insertCount = 0;
        while (std::uint8_t i = buf->getRawChar8() != 0)
            insertCount +=i;
        description = sBuf->getVariableText(version, false);  // 2a.6: DXF 4
        DRW_DBG("Block description: "); DRW_DBG(description.c_str()); DRW_DBG("\n");

        std::uint32_t prevData = buf->getBitLong();
        for (unsigned int j= 0; j < prevData; ++j)
            buf->getRawChar8();
    }
    if (version > DRW::AC1018) {//2007+
        insUnits = buf->getBitShort();
        canExplode = buf->getBit(); //if block can be exploded (DXF 280)
        blockScaling = buf->getRawChar8(); //2a.6: DXF 281
    }

    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }

    dwgHandle blockControlH = buf->getHandle();
    DRW_DBG("block control Handle: "); DRW_DBGHL(blockControlH.code, blockControlH.size, blockControlH.ref); DRW_DBG("\n");
    parentHandle = blockControlH.ref;

    for (int i=0; i<numReactors; i++){
        dwgHandle reactorH = buf->getHandle();
        DRW_DBG(" reactor Handle #"); DRW_DBG(i); DRW_DBG(": "); DRW_DBGHL(reactorH.code, reactorH.size, reactorH.ref); DRW_DBG("\n");
    }
    if (xDictFlag !=1) {//R14+ //seems present in 2000
        dwgHandle XDicObjH = buf->getHandle();
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref); DRW_DBG("\n");
    }
    if (version != DRW::AC1021) {//2007+ XDicObjH or NullH not present
    }
    dwgHandle NullH = buf->getHandle();
    DRW_DBG(" NullH control Handle: "); DRW_DBGHL(NullH.code, NullH.size, NullH.ref); DRW_DBG("\n");
    dwgHandle blockH = buf->getOffsetHandle(handle);
    DRW_DBG(" blockH Handle: "); DRW_DBGHL(blockH.code, blockH.size, blockH.ref); DRW_DBG("\n");
    block = blockH.ref;

    // Per ODA spec / libreDWG dwg.spec — entities handle vector is gated on
    // num_owned, which itself is only present for non-XREF blocks. Mirror the
    // num_owned guard above so the loop is skipped explicitly for XREFs.
    if (version > DRW::AC1015 && !blockIsXref && !xrefOverlaid) {//2004+, non-XREF
        for (unsigned int i=0; i< objectCount; i++){
            dwgHandle entityH = buf->getHandle();
            DRW_DBG(" entityH Handle #"); DRW_DBG(i); DRW_DBG(": "); DRW_DBGHL(entityH.code, entityH.size, entityH.ref); DRW_DBG("\n");
            entMap.push_back(entityH.ref);
        }
    } else if (version <= DRW::AC1015) {//2000-
        if(!blockIsXref && !xrefOverlaid){
            dwgHandle firstH = buf->getHandle();
            DRW_DBG(" firstH entity Handle: "); DRW_DBGHL(firstH.code, firstH.size, firstH.ref); DRW_DBG("\n");
            firstEH = firstH.ref;
            dwgHandle lastH = buf->getHandle();
            DRW_DBG(" lastH entity Handle: "); DRW_DBGHL(lastH.code, lastH.size, lastH.ref); DRW_DBG("\n");
            lastEH = lastH.ref;
        }
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    dwgHandle endBlockH = buf->getOffsetHandle(handle);
    DRW_DBG(" endBlockH Handle: "); DRW_DBGHL(endBlockH.code, endBlockH.size, endBlockH.ref); DRW_DBG("\n");
    endBlock = endBlockH.ref;

    if (version > DRW::AC1014) {//2000+
        for (unsigned int i=0; i< insertCount; i++){
            dwgHandle insertsH = buf->getHandle();
            DRW_DBG(" insertsH Handle #"); DRW_DBG(i); DRW_DBG(": "); DRW_DBGHL(insertsH.code, insertsH.size, insertsH.ref); DRW_DBG("\n");
        }
        DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        dwgHandle layoutH = buf->getHandle();
        DRW_DBG(" layoutH Handle: "); DRW_DBGHL(layoutH.code, layoutH.size, layoutH.ref); DRW_DBG("\n");
        layoutHandle = layoutH.ref;  // 2a.6: soft ptr to owning LAYOUT (DXF 340)
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n\n");
//    RS crc;   //RS */
    return buf->isGood();
}

//! Class to handle text style entries
/*!
*  Class to handle text style symbol table entries
*  @author Rallaz
*/
bool DRW_Textstyle::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 3:
        font = reader->getUtf8String();
        break;
    case 4:
        bigFont = reader->getUtf8String();
        break;
    case 40:
        height = reader->getDouble();
        break;
    case 41:
        width = reader->getDouble();
        break;
    case 50:
        oblique = reader->getDouble();
        break;
    case 42:
        lastHeight = reader->getDouble();
        break;
    case 71:
        genFlag = reader->getInt32();
        break;
    case 1071:
        fontFamily = reader->getInt32();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }

    return true;
}

bool DRW_Textstyle::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing text style *********************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    DRW_DBG("text style name: "); DRW_DBG(name.c_str()); DRW_DBG("\n");
    flags |= buf->getBit()<< 6;//style are referenced for a entity, style code 70, bit 7 (64)
    /*std::int16_t xrefindex =*/ buf->getBitShort();
    flags |= buf->getBit() << 4; //is refx dependent, style code 70, bit 5 (16)
    flags |= buf->getBit() << 2; //vertical text, stile code 70, bit 3 (4)
    flags |= buf->getBit(); //if is a shape file instead of text, style code 70, bit 1 (1)
    height = buf->getBitDouble();
    width = buf->getBitDouble();
    oblique = buf->getBitDouble();
    genFlag = buf->getRawChar8();
    lastHeight = buf->getBitDouble();
    font = sBuf->getVariableText(version, false);
    bigFont = sBuf->getVariableText(version, false);
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
    dwgHandle shpControlH = buf->getHandle();
    DRW_DBG(" parentControlH Handle: "); DRW_DBGHL(shpControlH.code, shpControlH.size, shpControlH.ref); DRW_DBG("\n");
    parentHandle = shpControlH.ref;
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref); DRW_DBG("\n");
        DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    }
/*RLZ: fails verify this part*/    dwgHandle XRefH = buf->getHandle();
    DRW_DBG(" XRefH control Handle: "); DRW_DBGHL(XRefH.code, XRefH.size, XRefH.ref); DRW_DBG("\n");

    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n\n");
    //    RS crc;   //RS */
    return buf->isGood();
}

//! Class to handle vport entries
/*!
*  Class to handle vport symbol table entries
*  @author Rallaz
*/
bool DRW_Vport::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 10:
        lowerLeft.x = reader->getDouble();
        break;
    case 20:
        lowerLeft.y = reader->getDouble();
        break;
    case 11:
        UpperRight.x = reader->getDouble();
        break;
    case 21:
        UpperRight.y = reader->getDouble();
        break;
    case 12:
        center.x = reader->getDouble();
        break;
    case 22:
        center.y = reader->getDouble();
        break;
    case 13:
        snapBase.x = reader->getDouble();
        break;
    case 23:
        snapBase.y = reader->getDouble();
        break;
    case 14:
        snapSpacing.x = reader->getDouble();
        break;
    case 24:
        snapSpacing.y = reader->getDouble();
        break;
    case 15:
        gridSpacing.x = reader->getDouble();
        break;
    case 25:
        gridSpacing.y = reader->getDouble();
        break;
    case 16:
        viewDir.x = reader->getDouble();
        break;
    case 26:
        viewDir.y = reader->getDouble();
        break;
    case 36:
        viewDir.z = reader->getDouble();
        break;
    case 17:
        viewTarget.x = reader->getDouble();
        break;
    case 27:
        viewTarget.y = reader->getDouble();
        break;
    case 37:
        viewTarget.z = reader->getDouble();
        break;
    case 40:
        height = reader->getDouble();
        break;
    case 41:
        ratio = reader->getDouble();
        break;
    case 42:
        lensHeight = reader->getDouble();
        break;
    case 43:
        frontClip = reader->getDouble();
        break;
    case 44:
        backClip = reader->getDouble();
        break;
    case 50:
        snapAngle = reader->getDouble();
        break;
    case 51:
        twistAngle = reader->getDouble();
        break;
    case 71:
        viewMode = reader->getInt32();
        break;
    case 72:
        circleZoom = reader->getInt32();
        break;
    case 73:
        fastZoom = reader->getInt32();
        break;
    case 74:
        ucsIcon = reader->getInt32();
        break;
    case 75:
        snap = reader->getInt32();
        break;
    case 76:
        grid = reader->getInt32();
        break;
    case 77:
        snapStyle = reader->getInt32();
        break;
    case 78:
        snapIsopair = reader->getInt32();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }

    return true;
}

bool DRW_Vport::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing VPort ************************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    DRW_DBG("vport name: "); DRW_DBG(name.c_str()); DRW_DBG("\n");
    flags |= buf->getBit()<< 6;// code 70, bit 7 (64)
    if (version < DRW::AC1021) { //2004-
        /*std::int16_t xrefindex =*/ buf->getBitShort();
    }
    flags |= buf->getBit() << 4; //is refx dependent, style code 70, bit 5 (16)
    height = buf->getBitDouble();
    ratio = buf->getBitDouble();
    DRW_DBG("flags: "); DRW_DBG(flags); DRW_DBG(" height: "); DRW_DBG(height);
    DRW_DBG(" ratio: "); DRW_DBG(ratio);
    center = buf->get2RawDouble();
    DRW_DBG("\nview center: "); DRW_DBGPT(center.x, center.y, center.z);
    viewTarget.x = buf->getBitDouble();
    viewTarget.y = buf->getBitDouble();
    viewTarget.z = buf->getBitDouble();
    DRW_DBG("\nview target: "); DRW_DBGPT(viewTarget.x, viewTarget.y, viewTarget.z);
    viewDir.x = buf->getBitDouble();
    viewDir.y = buf->getBitDouble();
    viewDir.z = buf->getBitDouble();
    DRW_DBG("\nview dir: "); DRW_DBGPT(viewDir.x, viewDir.y, viewDir.z);
    twistAngle = buf->getBitDouble();
    lensHeight = buf->getBitDouble();
    frontClip = buf->getBitDouble();
    backClip = buf->getBitDouble();
    DRW_DBG("\ntwistAngle: "); DRW_DBG(twistAngle); DRW_DBG(" lensHeight: "); DRW_DBG(lensHeight);
    DRW_DBG(" frontClip: "); DRW_DBG(frontClip); DRW_DBG(" backClip: "); DRW_DBG(backClip);
    viewMode = buf->getBit(); //view mode, code 71, bit 0 (1)
    viewMode |= buf->getBit() << 1; //view mode, code 71, bit 1 (2)
    viewMode |= buf->getBit() << 2; //view mode, code 71, bit 2 (4)
    viewMode |= buf->getBit() << 4; //view mode, code 71, bit 4 (16)
    if (version > DRW::AC1014) { //2000+
        //std::uint8_t renderMode = buf->getRawChar8();
        DRW_DBG("\n renderMode: "); DRW_DBG(buf->getRawChar8());
        if (version > DRW::AC1018) { //2007+
            DRW_DBG("\n use default lights: "); DRW_DBG(buf->getBit());
            DRW_DBG(" default lighting type: "); DRW_DBG(buf->getRawChar8());
            DRW_DBG(" brightness: "); DRW_DBG(buf->getBitDouble());
            DRW_DBG("\n contrast: "); DRW_DBG(buf->getBitDouble()); DRW_DBG("\n");
            DRW_DBG(" ambient color CMC: "); DRW_DBG(buf->getCmColor(version));
        }
    }
    lowerLeft = buf->get2RawDouble();
    DRW_DBG("\nlowerLeft: "); DRW_DBGPT(lowerLeft.x, lowerLeft.y, lowerLeft.z);
    UpperRight = buf->get2RawDouble();
    DRW_DBG("\nUpperRight: "); DRW_DBGPT(UpperRight.x, UpperRight.y, UpperRight.z);
    viewMode |= buf->getBit() << 3; //UCSFOLLOW, view mode, code 71, bit 3 (8)
    circleZoom = buf->getBitShort();
    fastZoom = buf->getBit();
    DRW_DBG("\nviewMode: "); DRW_DBG(viewMode); DRW_DBG(" circleZoom: ");
    DRW_DBG(circleZoom); DRW_DBG(" fastZoom: "); DRW_DBG(fastZoom);
    ucsIcon = buf->getBit(); //ucs Icon, code 74, bit 0 (1)
    ucsIcon |= buf->getBit() << 1; //ucs Icon, code 74, bit 1 (2)
    grid = buf->getBit();
    DRW_DBG("\nucsIcon: "); DRW_DBG(ucsIcon); DRW_DBG(" grid: "); DRW_DBG(grid);
    gridSpacing = buf->get2RawDouble();
    DRW_DBG("\ngrid Spacing: "); DRW_DBGPT(gridSpacing.x, gridSpacing.y, gridSpacing.z);
    snap = buf->getBit();
    snapStyle = buf->getBit();
    DRW_DBG("\nsnap on/off: "); DRW_DBG(snap); DRW_DBG(" snap Style: "); DRW_DBG(snapStyle);
    snapIsopair = buf->getBitShort();
    snapAngle = buf->getBitDouble();
    DRW_DBG("\nsnap Isopair: "); DRW_DBG(snapIsopair); DRW_DBG(" snap Angle: "); DRW_DBG(snapAngle);
    snapBase = buf->get2RawDouble();
    DRW_DBG("\nsnap Base: "); DRW_DBGPT(snapBase.x, snapBase.y, snapBase.z);
    snapSpacing = buf->get2RawDouble();
    DRW_DBG("\nsnap Spacing: "); DRW_DBGPT(snapSpacing.x, snapSpacing.y, snapSpacing.z);
    if (version > DRW::AC1014) { //2000+
        // Sequenced one-statement-per-field reads (NOT inside DRW_DBGPT macro
        // args — argument evaluation order is unspecified there).
        buf->getBit(); // ucs_at_origin (unknown/unused)
        ucsPerVP = buf->getBit();
        ucsOrigin.x = buf->getBitDouble();
        ucsOrigin.y = buf->getBitDouble();
        ucsOrigin.z = buf->getBitDouble();
        ucsXAxis.x = buf->getBitDouble();
        ucsXAxis.y = buf->getBitDouble();
        ucsXAxis.z = buf->getBitDouble();
        ucsYAxis.x = buf->getBitDouble();
        ucsYAxis.y = buf->getBitDouble();
        ucsYAxis.z = buf->getBitDouble();
        ucsElevation = buf->getBitDouble();
        ucsOrthoType = buf->getBitShort();
        DRW_DBG("\n UCS per Viewport: "); DRW_DBG(ucsPerVP);
        DRW_DBG("\nUCS origin: "); DRW_DBGPT(ucsOrigin.x, ucsOrigin.y, ucsOrigin.z);
        DRW_DBG("\nUCS X Axis: "); DRW_DBGPT(ucsXAxis.x, ucsXAxis.y, ucsXAxis.z);
        DRW_DBG("\nUCS Y Axis: "); DRW_DBGPT(ucsYAxis.x, ucsYAxis.y, ucsYAxis.z);
        DRW_DBG("\nUCS elevation: "); DRW_DBG(ucsElevation);
        DRW_DBG(" UCS Orthographic type: "); DRW_DBG(ucsOrthoType);
        if (version > DRW::AC1018) { //2007+
            gridBehavior = buf->getBitShort();
            DRW_DBG(" gridBehavior (flags): "); DRW_DBG(gridBehavior);
            DRW_DBG(" Grid major: "); DRW_DBG(buf->getBitShort());
        }
    }

    //common handles
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
    dwgHandle vpControlH = buf->getHandle();
    DRW_DBG("\n parentControlH Handle: "); DRW_DBGHL(vpControlH.code, vpControlH.size, vpControlH.ref); DRW_DBG("\n");
    parentHandle = vpControlH.ref;
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    if (xDictFlag !=1){
        dwgHandle XDicObjH = buf->getHandle();
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref); DRW_DBG("\n");
        DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    }
/*RLZ: fails verify this part*/    dwgHandle XRefH = buf->getHandle();
    DRW_DBG(" XRefH control Handle: "); DRW_DBGHL(XRefH.code, XRefH.size, XRefH.ref);

    if (version > DRW::AC1014) { //2000+
        DRW_DBG("\nRemaining bytes: "); DRW_DBG(buf->numRemainingBytes());
        if (version > DRW::AC1018) { //2007+
            dwgHandle bkgrdH = buf->getHandle();
            backgroundHandle = bkgrdH.ref;
            DRW_DBG(" background Handle: "); DRW_DBGHL(bkgrdH.code, bkgrdH.size, bkgrdH.ref);
            DRW_DBG("\nRemaining bytes: "); DRW_DBG(buf->numRemainingBytes());
            dwgHandle visualStH = buf->getHandle();
            visualStyleHandle = visualStH.ref;
	            DRW_DBG(" visual style Handle: "); DRW_DBGHL(visualStH.code, visualStH.size, visualStH.ref);
	            DRW_DBG("\nRemaining bytes: "); DRW_DBG(buf->numRemainingBytes());
	            dwgHandle sunH = buf->getHandle();
	            m_sunHandle = sunH.ref;
	            DRW_DBG(" sun Handle: "); DRW_DBGHL(sunH.code, sunH.size, sunH.ref);
            DRW_DBG("\nRemaining bytes: "); DRW_DBG(buf->numRemainingBytes());
        }
        dwgHandle namedUCSH = buf->getHandle();
        namedUcsHandle = namedUCSH.ref;
        DRW_DBG(" named UCS Handle: "); DRW_DBGHL(namedUCSH.code, namedUCSH.size, namedUCSH.ref);
        DRW_DBG("\nRemaining bytes: "); DRW_DBG(buf->numRemainingBytes());
        dwgHandle baseUCSH = buf->getHandle();
        baseUcsHandle = baseUCSH.ref;
        DRW_DBG(" base UCS Handle: "); DRW_DBGHL(baseUCSH.code, baseUCSH.size, baseUCSH.ref);
    }

    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    //    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_ImageDef::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 1:
        name = reader->getUtf8String();
        break;
    case 5:
        handle = reader->getHandleString();
        break;
    case 10:
        u = reader->getDouble();
        break;
    case 20:
        v = reader->getDouble();
        break;
    case 11:
        up = reader->getDouble();
        break;
    case 12:
        vp = reader->getDouble();
        break;
    case 21:
        vp = reader->getDouble();
        break;
    case 280:
        loaded = reader->getInt32();
        break;
    case 281:
        resolution = reader->getInt32();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }

    return true;
}

bool DRW_ImageDef::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing Image Def *********************************************\n");
    if (!ret)
        return ret;
    std::int32_t imgVersion = buf->getBitLong();
    DRW_DBG("class Version: "); DRW_DBG(imgVersion);
    DRW_Coord size = buf->get2RawDouble();
    u = size.x;  // P4-13: image size in pixels (DXF 10), was discarded
    v = size.y;  // DXF 20
    name = sBuf->getVariableText(version, false);
    DRW_DBG("appId name: "); DRW_DBG(name.c_str()); DRW_DBG("\n");
    loaded = buf->getBit();
    resolution = buf->getRawChar8();
    up = buf->getRawDouble();
    vp = buf->getRawDouble();

    dwgHandle parentH = buf->getHandle();
    DRW_DBG(" parentH Handle: "); DRW_DBGHL(parentH.code, parentH.size, parentH.ref); DRW_DBG("\n");
    parentHandle = parentH.ref;
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    //RLZ: Reactors handles
    if (xDictFlag !=1){
        dwgHandle XDicObjH = buf->getHandle();
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref); DRW_DBG("\n");
        DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    }
/*RLZ: fails verify this part*/    dwgHandle XRefH = buf->getHandle();
    DRW_DBG(" XRefH control Handle: "); DRW_DBGHL(XRefH.code, XRefH.size, XRefH.ref); DRW_DBG("\n");

    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n\n");
    //    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_PlotSettings::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 5:
        handle = reader->getHandleString();
        break;
    case 1:
        pageSetupName = reader->getUtf8String();
        break;
    case 2:
        printerConfig = reader->getUtf8String();
        break;
    case 4:
        paperSize = reader->getUtf8String();
        break;
    case 6:
        plotViewName = reader->getUtf8String();
        break;
    case 7:
        currentStyleSheet = reader->getUtf8String();
        break;
    case 40:
        marginLeft = reader->getDouble();
        break;
    case 41:
        marginBottom = reader->getDouble();
        break;
    case 42:
        marginRight = reader->getDouble();
        break;
    case 43:
        marginTop = reader->getDouble();
        break;
    case 44:
        paperWidth = reader->getDouble();
        break;
    case 45:
        paperHeight = reader->getDouble();
        break;
    case 46:
        plotOriginX = reader->getDouble();
        break;
    case 47:
        plotOriginY = reader->getDouble();
        break;
    case 48:
        windowMinX = reader->getDouble();
        break;
    case 49:
        windowMinY = reader->getDouble();
        break;
    case 140:
        windowMaxX = reader->getDouble();
        break;
    case 141:
        windowMaxY = reader->getDouble();
        break;
    case 142:
        realWorldUnits = reader->getDouble();
        break;
    case 143:
        drawingUnits = reader->getDouble();
        break;
    case 147:
        scaleFactor = reader->getDouble();
        break;
    case 148:
        paperImageOriginX = reader->getDouble();
        break;
    case 149:
        paperImageOriginY = reader->getDouble();
        break;
    case 70:
        plotLayoutFlags = reader->getInt32();
        break;
    case 72:
        paperUnits = reader->getInt32();
        break;
    case 73:
        plotRotation = reader->getInt32();
        break;
    case 74:
        plotType = reader->getInt32();
        break;
    case 75:
        scaleType = reader->getInt32();
        break;
    case 76:
        shadePlotMode = reader->getInt32();
        break;
    case 77:
        shadePlotResLevel = reader->getInt32();
        break;
    case 78:
        shadePlotCustomDPI = reader->getInt32();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }

    return true;
}

bool DRW_PlotSettings::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n********************** parsing Plot Settings **************************\n");
    if (!ret)
        return ret;

    // P4-02: AcDbPlotSettings body. Same wire layout as the PLOTSETTINGS
    // prefix embedded in DRW_Layout (libreDWG dwg.spec DWG_OBJECT(PLOTSETTINGS)
    // == LAYOUT plot prefix). Member names here mirror DRW_Layout so the decode
    // is a verbatim copy of the Layout path (drw_objects.cpp:3416-3452).
    pageSetupName    = sBuf->getVariableText(version, false); // printer_cfg_file (1)
    printerConfig    = sBuf->getVariableText(version, false); // paper_size (2)
    plotLayoutFlags  = buf->getBitShort();
    marginLeft       = buf->getBitDouble();
    marginBottom     = buf->getBitDouble();
    marginRight      = buf->getBitDouble();
    marginTop        = buf->getBitDouble();
    paperWidth       = buf->getBitDouble();
    paperHeight      = buf->getBitDouble();
    paperSize        = sBuf->getVariableText(version, false); // canonical_media_name
    plotOriginX      = buf->getBitDouble();
    plotOriginY      = buf->getBitDouble();
    paperUnits       = buf->getBitShort();
    plotRotation     = buf->getBitShort();
    plotType         = buf->getBitShort();
    windowMinX       = buf->getBitDouble();
    windowMinY       = buf->getBitDouble();
    windowMaxX       = buf->getBitDouble();
    windowMaxY       = buf->getBitDouble();

    if (version < DRW::AC1018) { // R13-R2002: plotview_name in the data section
        plotViewName = sBuf->getVariableText(version, false);
    }

    realWorldUnits    = buf->getBitDouble(); // paper_units (142)
    drawingUnits      = buf->getBitDouble(); // drawing_units (143)
    currentStyleSheet = sBuf->getVariableText(version, false); // stylesheet (7)
    scaleType         = buf->getBitShort();
    scaleFactor       = buf->getBitDouble();
    paperImageOriginX = buf->getBitDouble();
    paperImageOriginY = buf->getBitDouble();

    if (version >= DRW::AC1018) { // R2004+
        shadePlotMode      = buf->getBitShort();
        shadePlotResLevel  = buf->getBitShort();
        shadePlotCustomDPI = buf->getBitShort();
    }

    // Common handle tail (parent/reactors/xdict). PLOTSETTINGS has no further
    // data-consumed handles we track (R2007+ shadeplot handle is left in the
    // stream; not needed for the margins/paper/scale data we surface).
    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag,
                            &parentHandle);

    return buf->isGood();
}

bool DRW_UCS::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 10:
        origin.x = reader->getDouble();
        break;
    case 20:
        origin.y = reader->getDouble();
        break;
    case 30:
        origin.z = reader->getDouble();
        break;
    case 11:
        xAxisDirection.x = reader->getDouble();
        break;
    case 21:
        xAxisDirection.y = reader->getDouble();
        break;
    case 31:
        xAxisDirection.z = reader->getDouble();
        break;
    case 12:
        yAxisDirection.x = reader->getDouble();
        break;
    case 22:
        yAxisDirection.y = reader->getDouble();
        break;
    case 32:
        yAxisDirection.z = reader->getDouble();
        break;
    case 13:
        orthoOrigin.x = reader->getDouble();
        break;
    case 23:
        orthoOrigin.y = reader->getDouble();
        break;
    case 33:
        orthoOrigin.z = reader->getDouble();
        break;
    case 71:
        orthoType = reader->getInt32();
        break;
    case 79: //always 0 in DXF
        break;
    case 146:
        elevation = reader->getDouble();
        break;
    case 346: //base UCS handle, optional, only when 79 != 0
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_UCS::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    // Full UCS table-record decode (P4-04). Field order = libreDWG
    // dwg.spec:4293-4342 DWG_TABLE(UCS), binary (non-DXF) order:
    //   COMMON_TABLE_FLAGS(name); 3BD ucsorg/ucsxdir/ucsydir;
    //   SINCE R2000b: BD ucs_elevation FIRST, then BS UCSORTHOVIEW,
    //   then BS num_orthopts + repeat[BS type, 3BD pt] (DATA section);
    //   handle stream: owner, reactors, xdict, base_ucs, named_ucs.
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing UCS **************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    DRW_DBG("ucs name: "); DRW_DBG(name); DRW_DBG("\n");

    // COMMON_TABLE_FLAGS tail (mirror DRW_Vport: 64-flag, xrefindex, 16-flag).
    flags |= buf->getBit() << 6; // code 70, bit 7 (64)
    if (version < DRW::AC1021) { //2004-
        /*std::int16_t xrefindex =*/ buf->getBitShort();
    }
    flags |= buf->getBit() << 4; // code 70, bit 5 (16), xref-dependent

    origin = buf->get3BitDouble();
    xAxisDirection = buf->get3BitDouble();
    yAxisDirection = buf->get3BitDouble();

    if (version > DRW::AC1014) { //R2000+ (SINCE R_2000b)
        elevation = buf->getBitDouble();   // BD ucs_elevation FIRST
        orthoType = buf->getBitShort();    // BS UCSORTHOVIEW
        // num_orthopts array stays in the DATA section (before the handle
        // stream). Consume it to keep alignment; retain the first point.
        std::int16_t numOrthopts = buf->getBitShort();
        for (std::int16_t i = 0; i < numOrthopts; ++i) {
            std::int16_t oType = buf->getBitShort();
            DRW_Coord pt = buf->get3BitDouble();
            if (i == 0) {
                orthoOrigin = pt;
                if (orthoType == 0)
                    orthoType = oType;
            }
        }
    }

    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }

    // Handle stream: owner, reactors, xdict (common prefix), then the UCS
    // FIELD_HANDLEs base_ucs, named_ucs.
    dwgHandle ucsControlH = buf->getHandle();
    parentHandle = ucsControlH.ref;
    for (std::int32_t i = 0; i < numReactors; ++i)
        buf->getHandle();
    if (xDictFlag != 1)
        buf->getHandle();

    if (version > DRW::AC1014) { //R2000+
        baseUcsHandle = buf->getHandle();
        namedUcsHandle = buf->getHandle();
        DRW_DBG(" base UCS Handle: "); DRW_DBGHL(baseUcsHandle.code, baseUcsHandle.size, baseUcsHandle.ref); DRW_DBG("\n");
        DRW_DBG(" named UCS Handle: "); DRW_DBGHL(namedUcsHandle.code, namedUcsHandle.size, namedUcsHandle.ref); DRW_DBG("\n");
    }

    return buf->isGood();
}

bool DRW_View::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 40:
        size.y = reader->getDouble();
        break;
    case 41:
        size.x = reader->getDouble();
        break;
    case 10:
        center.x = reader->getDouble();
        break;
    case 20:
        center.y = reader->getDouble();
        break;
    case 11:
        viewDirectionFromTarget.x = reader->getDouble();
        break;
    case 21:
        viewDirectionFromTarget.y = reader->getDouble();
        break;
    case 31:
        viewDirectionFromTarget.z = reader->getDouble();
        break;
    case 12:
        targetPoint.x = reader->getDouble();
        break;
    case 22:
        targetPoint.y = reader->getDouble();
        break;
    case 32:
        targetPoint.z = reader->getDouble();
        break;
    case 42:
        lensLen = reader->getDouble();
        break;
    case 43:
        frontClippingPlaneOffset = reader->getDouble();
        break;
    case 44:
        backClippingPlaneOffset = reader->getDouble();
        break;
    case 50:
        twistAngle = reader->getDouble();
        break;
    case 71:
        viewMode = reader->getInt32();
        break;
    case 281:
        renderMode = reader->getInt32();
        break;
    case 72:
        hasUCS = reader->getBool();
        break;
    case 73:
        cameraPlottable = reader->getBool();
        break;
    case 110:
        ucsOrigin.x = reader->getDouble();
        break;
    case 120:
        ucsOrigin.y = reader->getDouble();
        break;
    case 130:
        ucsOrigin.z = reader->getDouble();
        break;
    case 111:
        ucsXAxis.x = reader->getDouble();
        break;
    case 121:
        ucsXAxis.y = reader->getDouble();
        break;
    case 131:
        ucsXAxis.z = reader->getDouble();
        break;
    case 112:
        ucsYAxis.x = reader->getDouble();
        break;
    case 122:
        ucsYAxis.y = reader->getDouble();
        break;
    case 132:
        ucsYAxis.z = reader->getDouble();
        break;
    case 79:
        ucsOrthoType = reader->getInt32();
        break;
    case 146:
        ucsElevation = reader->getDouble();
        break;
    case 345:
        namedUCS_ID = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    case 346:
        baseUCS_ID = static_cast<std::uint32_t>(reader->getHandleString());
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_View::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing VIEW *********************************************\n");
    if (!ret)
        return ret;

    name = sBuf->getVariableText(version, false);
    DRW_DBG("view name: "); DRW_DBG(name); DRW_DBG("\n");

    flags |= buf->getBit() << 6; // code 70, bit 7 (64)
    if (version < DRW::AC1021) { //2004-
        DRW_DBG("xrefindex = "); DRW_DBG(buf->getBitShort()); DRW_DBG("\n");
    }
    flags |= buf->getBit() << 4; // code 70, bit 5 (16), xref-dependent

    size.y = buf->getBitDouble();
    size.x = buf->getBitDouble();
    center = buf->get2RawDouble();
    targetPoint = buf->get3BitDouble();
    viewDirectionFromTarget = buf->get3BitDouble();
    twistAngle = buf->getBitDouble();
    lensLen = buf->getBitDouble();
    frontClippingPlaneOffset = buf->getBitDouble();
    backClippingPlaneOffset = buf->getBitDouble();

    viewMode = 0;
    viewMode |= buf->getBit();       // DXF 71 bit 0 (1)
    viewMode |= buf->getBit() << 1;  // DXF 71 bit 1 (2)
    viewMode |= buf->getBit() << 2;  // DXF 71 bit 2 (4)
    // ODA 20.4.60: stored bit 3 is the opposite of DXF 71 bit 4 (16).
    if (!buf->getBit())
        viewMode |= 16;

    if (version > DRW::AC1014) { //2000+
        renderMode = buf->getRawChar8();
    }
    if (version > DRW::AC1018) { //2007+
        m_useDefaultLights = buf->getBit();
        m_defaultLightingType = buf->getRawChar8();
        m_brightness = buf->getBitDouble();
        m_contrast = buf->getBitDouble();
        m_ambientColor = buf->getCmColor(version, nullptr, sBuf);
    }

    if (buf->getBit())
        flags |= 1; // paper-space flag, code 70 bit 0
    else
        flags &= ~1;

    if (version > DRW::AC1014) { //2000+
        hasUCS = buf->getBit();
        if (hasUCS) {
            ucsOrigin = buf->get3BitDouble();
            ucsXAxis = buf->get3BitDouble();
            ucsYAxis = buf->get3BitDouble();
            ucsElevation = buf->getBitDouble();
            ucsOrthoType = buf->getBitShort();
        }
    }
    if (version > DRW::AC1018) { //2007+
        cameraPlottable = buf->getBit();
    }

    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }

    dwgHandle viewControlH = buf->getHandle();
    DRW_DBG(" view control Handle: "); DRW_DBGHL(viewControlH.code, viewControlH.size, viewControlH.ref); DRW_DBG("\n");
    parentHandle = viewControlH.ref;

    for (std::int32_t i = 0; i < numReactors; ++i) {
        dwgHandle reactorH = buf->getHandle();
        DRW_DBG(" reactor Handle: "); DRW_DBGHL(reactorH.code, reactorH.size, reactorH.ref); DRW_DBG("\n");
    }

    if (xDictFlag != 1) {
        dwgHandle xDictH = buf->getHandle();
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(xDictH.code, xDictH.size, xDictH.ref); DRW_DBG("\n");
    }

    dwgHandle xrefH = buf->getHandle();
    DRW_DBG(" XRefH control Handle: "); DRW_DBGHL(xrefH.code, xrefH.size, xrefH.ref); DRW_DBG("\n");

    if (version > DRW::AC1018) { //2007+
        dwgHandle backgroundH = buf->getHandle();
        m_backgroundHandle = backgroundH.ref;
        DRW_DBG(" background Handle: "); DRW_DBGHL(backgroundH.code, backgroundH.size, backgroundH.ref); DRW_DBG("\n");
        dwgHandle visualStyleH = buf->getHandle();
        m_visualStyleHandle = visualStyleH.ref;
        DRW_DBG(" visual style Handle: "); DRW_DBGHL(visualStyleH.code, visualStyleH.size, visualStyleH.ref); DRW_DBG("\n");
        dwgHandle sunH = buf->getHandle();
        m_sunHandle = sunH.ref;
        DRW_DBG(" sun Handle: "); DRW_DBGHL(sunH.code, sunH.size, sunH.ref); DRW_DBG("\n");
    }

    if (version > DRW::AC1014) { //2000+
        dwgHandle baseUcsH = buf->getHandle();
        baseUCS_ID = baseUcsH.ref;
        DRW_DBG(" base UCS Handle: "); DRW_DBGHL(baseUcsH.code, baseUcsH.size, baseUcsH.ref); DRW_DBG("\n");
        dwgHandle namedUcsH = buf->getHandle();
        namedUCS_ID = namedUcsH.ref;
        DRW_DBG(" named UCS Handle: "); DRW_DBGHL(namedUcsH.code, namedUcsH.size, namedUcsH.ref); DRW_DBG("\n");
    }

    if (version > DRW::AC1018) { //2007+
        dwgHandle liveSectionH = buf->getHandle();
        m_liveSectionHandle = liveSectionH.ref;
        DRW_DBG(" live section Handle: "); DRW_DBGHL(liveSectionH.code, liveSectionH.size, liveSectionH.ref); DRW_DBG("\n");
    }

    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    return buf->isGood();
}

bool DRW_AppId::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing app Id *********************************************\n");
    if (!ret)
        return ret;
    name = sBuf->getVariableText(version, false);
    DRW_DBG("appId name: "); DRW_DBG(name); DRW_DBG("\n");
    flags |= buf->getBit()<< 6;// code 70, bit 7 (64)
    /*std::int16_t xrefindex =*/ buf->getBitShort();
    flags |= buf->getBit() << 4; //is refx dependent, style code 70, bit 5 (16)
    std::uint8_t unknown = buf->getRawChar8(); // unknown code 71
    DRW_DBG("unknown code 71: "); DRW_DBG(unknown); DRW_DBG("\n");
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }
    dwgHandle appIdControlH = buf->getHandle();
    DRW_DBG(" parentControlH Handle: "); DRW_DBGHL(appIdControlH.code, appIdControlH.size, appIdControlH.ref); DRW_DBG("\n");
    parentHandle = appIdControlH.ref;
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref); DRW_DBG("\n");
        DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    }
/*RLZ: fails verify this part*/    dwgHandle XRefH = buf->getHandle();
    DRW_DBG(" XRefH control Handle: "); DRW_DBGHL(XRefH.code, XRefH.size, XRefH.ref); DRW_DBG("\n");

    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n\n");
    //    RS crc;   //RS */
    return buf->isGood();
}

// ---------------------------------------------------------------------------
// encodeDwg implementations for table entry classes
// ---------------------------------------------------------------------------

// Reserved control-object handles (mirrors dwgwriter15.cpp constants).
namespace {
    constexpr std::uint32_t kLtypeControl   = 0x05;
    constexpr std::uint32_t kLayerControl   = 0x02;
    constexpr std::uint32_t kStyleControl   = 0x03;
    constexpr std::uint32_t kUcsControl     = 0x07;
    constexpr std::uint32_t kViewControl    = 0x06;
    constexpr std::uint32_t kVportControl   = 0x08;
    constexpr std::uint32_t kAppIdControl   = 0x09;

    bool isNonZeroCoord(const DRW_Coord& coord) {
        return coord.x != 0.0 || coord.y != 0.0 || coord.z != 0.0;
    }
}

bool DRW_LType::encodeDwg(DRW::Version version, dwgBufferW *buf,
                           dwgBufferW *strBuf, dwgBufferW *hdlBuf) const {
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (hdlBuf && version > DRW::AC1018) ? hdlBuf : buf;

    sb->putVariableText(version, name);
    buf->putBit(static_cast<std::uint8_t>((flags >> 6) & 1));
    if (version <= DRW::AC1018)
        buf->putBitShort(0);  // xrefindex
    buf->putBit(static_cast<std::uint8_t>((flags >> 4) & 1));
    sb->putVariableText(version, desc);
    buf->putBitDouble(length);
    buf->putRawChar8(0x41);  // align = 'A'
    buf->putRawChar8(static_cast<std::uint8_t>(size));

    bool haveStrArea = false;
    for (int i = 0; i < size; ++i) {
        double dashLen = (i < static_cast<int>(path.size())) ? path[i] : 0.0;
        buf->putBitDouble(dashLen);
        buf->putBitShort(0);   // complexFlags1
        buf->putRawDouble(0.0); buf->putRawDouble(0.0); // offset x,y
        buf->putBitDouble(0.0); // scale
        buf->putBitDouble(0.0); // rotation
        buf->putBitShort(0);   // complexFlags2 (0 = no shape)
    }

    if (version < DRW::AC1021) {
        // Pre-2007: 256-byte string area
        std::uint8_t strarea[256] = {};
        buf->putBytes(strarea, 256);
    } else if (haveStrArea) {
        std::uint8_t strarea[512] = {};
        buf->putBytes(strarea, 512);
    }

    // Handles
    hb->putHandle(makeRefW(kLtypeControl, 4));  // owner = soft pointer (4)
    // xDictFlag == 0 → write XDic null
    hb->putHandle(makeSoftOwnerW(0));
    // ODA 20.4.58: XRefH is always present (COMMON_TABLE_FLAGS), then one style
    // handle per dash.  The DWG binary stream emits the per-dash handle
    // unconditionally (NULL here -- libdxfrw does not yet model complex dashes);
    // the shape-flag gating only applies to the DXF 340 group.  Mirrors the
    // per-dash reader loop above.  (Byte-identical to the legacy encoder for
    // size==0 and size==2; correct for all other dash counts.)
    hb->putHandle(makeNullHandleW());      // XRefH (always)
    for (int i = 0; i < size; ++i) {
        hb->putHandle(makeNullHandleW());  // dash[i] style (340)
    }
    return true;
}

bool DRW_Layer::encodeDwg(DRW::Version version, dwgBufferW *buf,
                           dwgBufferW *strBuf, dwgBufferW *hdlBuf) const {
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (hdlBuf && version > DRW::AC1018) ? hdlBuf : buf;

    sb->putVariableText(version, name);
    buf->putBit(static_cast<std::uint8_t>((flags >> 6) & 1));
    if (version < DRW::AC1021)
        buf->putBitShort(0);  // xrefindex (2004-)
    buf->putBit(static_cast<std::uint8_t>((flags >> 4) & 1));

    if (version < DRW::AC1015) {
        buf->putBit(static_cast<std::uint8_t>(flags & 1));         // frozen
        buf->putBit(0);                                        // negate color
        buf->putBit(static_cast<std::uint8_t>((flags >> 1) & 1));  // frozen in new
        buf->putBit(static_cast<std::uint8_t>((flags >> 3) & 1));  // locked
    }
    if (version > DRW::AC1014) {
        int lwIdx = DRW_LW_Conv::lineWidth2dwgInt(lWeight);
        std::int16_t f = static_cast<std::int16_t>(
            (flags & 0x01) |
            ((flags & 0x02) << 1) |
            ((flags & 0x04) << 1) |
            (plotF ? 0x10 : 0) |
            ((lwIdx & 0x1F) << 5));
        buf->putSBitShort(f);
    }
    // P4-08: emit 24-bit truecolor + color/book name on R2004+ when present.
    // colorName is stored joined as "BOOK$ENTRY" by parseDwg (or just "ENTRY"
    // when no book); split it back so the CMC name/book flags round-trip. The
    // name strings go to the string buffer (sb), matching the reader which
    // reads them from sBuf.
    if (version > DRW::AC1014 && color24 >= 0) {
        UTF8STRING bookName, entryName = colorName;
        const std::string::size_type sep = colorName.find('$');
        if (sep != std::string::npos) {
            bookName = colorName.substr(0, sep);
            entryName = colorName.substr(sep + 1);
        }
        buf->putCmColor(version, static_cast<std::uint16_t>(color > 0 ? color : 7),
                        color24, entryName, bookName, sb);
    } else {
        buf->putCmColor(version, static_cast<std::uint16_t>(color > 0 ? color : 7));
    }

    // Handles
    hb->putHandle(makeRefW(kLayerControl, 4));  // owner = soft pointer (4)
    hb->putHandle(makeSoftOwnerW(0));   // XDic null
    hb->putHandle(makeNullHandleW());   // XRef null
    if (version > DRW::AC1014)
        hb->putHandle(makeNullHandleW());  // plotStyH null
    if (version > DRW::AC1018)
        hb->putHandle(makeNullHandleW());  // materialH null
    hb->putHandle(lTypeH);               // linetype handle (set by caller)
    return true;
}

bool DRW_Textstyle::encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf, dwgBufferW *hdlBuf) const {
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (hdlBuf && version > DRW::AC1018) ? hdlBuf : buf;

    sb->putVariableText(version, name);
    buf->putBit(static_cast<std::uint8_t>((flags >> 6) & 1));
    buf->putBitShort(0);  // xrefindex (always, all versions)
    buf->putBit(static_cast<std::uint8_t>((flags >> 4) & 1));
    buf->putBit(static_cast<std::uint8_t>((flags >> 2) & 1));  // vertical
    buf->putBit(static_cast<std::uint8_t>(flags & 1));          // shape file
    buf->putBitDouble(height);
    buf->putBitDouble(width);
    buf->putBitDouble(oblique);
    buf->putRawChar8(static_cast<std::uint8_t>(genFlag));
    buf->putBitDouble(lastHeight);
    sb->putVariableText(version, font);
    sb->putVariableText(version, bigFont);

    hb->putHandle(makeRefW(kStyleControl, 4));  // owner = soft pointer (4)
    hb->putHandle(makeSoftOwnerW(0));  // XDic null
    hb->putHandle(makeNullHandleW());  // XRef null
    return true;
}

bool DRW_Dimstyle::encodeDwg(DRW::Version version, dwgBufferW *buf,
                              dwgBufferW *strBuf, dwgBufferW *hdlBuf) const {
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    // Minimal encoder: name only (parseDwg reads only name and returns)
    sb->putVariableText(version, name);
    (void)buf; (void)hdlBuf;
    return true;
}

bool DRW_UCS::encodeDwg(DRW::Version version, dwgBufferW *buf,
                        dwgBufferW *strBuf, dwgBufferW *hdlBuf) const {
    if (buf == nullptr)
        return false;
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (hdlBuf && version > DRW::AC1018) ? hdlBuf : buf;

    sb->putVariableText(version, name);
    buf->putBit(static_cast<std::uint8_t>((flags >> 6) & 1));
    if (version < DRW::AC1021)
        buf->putBitShort(0);  // xrefindex (2004-)
    buf->putBit(static_cast<std::uint8_t>((flags >> 4) & 1));

    buf->put3BitDouble(origin);
    buf->put3BitDouble(xAxisDirection);
    buf->put3BitDouble(yAxisDirection);

    if (version > DRW::AC1014) {
        buf->putBitDouble(elevation);
        buf->putBitShort(static_cast<std::uint16_t>(orthoType));
        if (isNonZeroCoord(orthoOrigin)) {
            buf->putBitShort(1);
            buf->putBitShort(static_cast<std::uint16_t>(orthoType));
            buf->put3BitDouble(orthoOrigin);
        } else {
            buf->putBitShort(0);
        }
    }

    hb->putHandle(makeRefW(kUcsControl, 4));  // owner = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i)
        hb->putHandle(makeSoftOwnerW(0));
    if (xDictFlag != 1)
        hb->putHandle(makeSoftOwnerW(0));

    if (version > DRW::AC1014) {
        hb->putHandle(writeHandleOrHardPtr(baseUcsHandle));
        hb->putHandle(writeHandleOrHardPtr(namedUcsHandle));
    }
    return true;
}

bool DRW_Vport::encodeDwg(DRW::Version version, dwgBufferW *buf,
                           dwgBufferW *strBuf, dwgBufferW *hdlBuf) const {
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (hdlBuf && version > DRW::AC1018) ? hdlBuf : buf;

    sb->putVariableText(version, name);
    buf->putBit(static_cast<std::uint8_t>((flags >> 6) & 1));
    if (version < DRW::AC1021)
        buf->putBitShort(0);  // xrefindex (2004-)
    buf->putBit(static_cast<std::uint8_t>((flags >> 4) & 1));
    buf->putBitDouble(height);
    buf->putBitDouble(ratio);
    buf->put2RawDouble(center);
    buf->putBitDouble(viewTarget.x); buf->putBitDouble(viewTarget.y); buf->putBitDouble(viewTarget.z);
    buf->putBitDouble(viewDir.x);    buf->putBitDouble(viewDir.y);    buf->putBitDouble(viewDir.z);
    buf->putBitDouble(twistAngle);
    buf->putBitDouble(lensHeight);
    buf->putBitDouble(frontClip);
    buf->putBitDouble(backClip);
    buf->putBit(static_cast<std::uint8_t>(viewMode & 1));
    buf->putBit(static_cast<std::uint8_t>((viewMode >> 1) & 1));
    buf->putBit(static_cast<std::uint8_t>((viewMode >> 2) & 1));
    buf->putBit(static_cast<std::uint8_t>((viewMode >> 4) & 1));
    if (version > DRW::AC1014) {
        buf->putRawChar8(0);  // renderMode = 0
        if (version > DRW::AC1018) {
            buf->putBit(1);         // useDefLight
            buf->putRawChar8(1);    // defLightType
            buf->putBitDouble(1.0); // brightness
            buf->putBitDouble(0.0); // contrast
            buf->putCmColor(version, 0);  // ambientColor
        }
    }
    buf->put2RawDouble(lowerLeft);
    buf->put2RawDouble(UpperRight);
    buf->putBit(static_cast<std::uint8_t>((viewMode >> 3) & 1));
    buf->putBitShort(static_cast<std::uint16_t>(circleZoom));
    buf->putBit(static_cast<std::uint8_t>(fastZoom & 1));
    buf->putBit(static_cast<std::uint8_t>(ucsIcon & 1));
    buf->putBit(static_cast<std::uint8_t>((ucsIcon >> 1) & 1));
    buf->putBit(static_cast<std::uint8_t>(grid & 1));
    buf->put2RawDouble(gridSpacing);
    buf->putBit(static_cast<std::uint8_t>(snap & 1));
    buf->putBit(static_cast<std::uint8_t>(snapStyle & 1));
    buf->putBitShort(static_cast<std::uint16_t>(snapIsopair));
    buf->putBitDouble(snapAngle);
    buf->put2RawDouble(snapBase);
    buf->put2RawDouble(snapSpacing);
    if (version > DRW::AC1014) {
        buf->putBit(0);                     // ucs_at_origin (unknown)
        buf->putBit(ucsPerVP ? 1 : 0);      // ucsPerVP
        buf->putBitDouble(ucsOrigin.x); buf->putBitDouble(ucsOrigin.y); buf->putBitDouble(ucsOrigin.z); // ucsOrigin
        buf->putBitDouble(ucsXAxis.x);  buf->putBitDouble(ucsXAxis.y);  buf->putBitDouble(ucsXAxis.z);  // ucsX
        buf->putBitDouble(ucsYAxis.x);  buf->putBitDouble(ucsYAxis.y);  buf->putBitDouble(ucsYAxis.z);  // ucsY
        buf->putBitDouble(ucsElevation);    // ucsElev
        buf->putBitShort(static_cast<std::uint16_t>(ucsOrthoType)); // ucsOrthoType
        if (version > DRW::AC1018) {
            buf->putBitShort(static_cast<std::uint16_t>(gridBehavior));
            buf->putBitShort(1); // gridMajor
        }
    }

    hb->putHandle(makeRefW(kVportControl, 4));  // owner = soft pointer (4)
    hb->putHandle(makeSoftOwnerW(0));  // XDic null
    hb->putHandle(makeNullHandleW());  // XRef null
    if (version > DRW::AC1018) {
        hb->putHandle(makeNullHandleW());  // backgroundH
        hb->putHandle(makeNullHandleW());  // visualStyleH
        hb->putHandle(makeNullHandleW());  // sunH
    }
    if (version > DRW::AC1014) {
        hb->putHandle(makeNullHandleW());  // namedUCSH
        hb->putHandle(makeNullHandleW());  // baseUCSH
    }
    return true;
}

bool DRW_View::encodeDwg(DRW::Version version, dwgBufferW *buf,
                          dwgBufferW *strBuf, dwgBufferW *hdlBuf) const {
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (hdlBuf && version > DRW::AC1018) ? hdlBuf : buf;

    sb->putVariableText(version, name);
    buf->putBit(static_cast<std::uint8_t>((flags >> 6) & 1));
    if (version < DRW::AC1021)
        buf->putBitShort(0);  // xrefindex (2004-)
    buf->putBit(static_cast<std::uint8_t>((flags >> 4) & 1));
    buf->putBitDouble(size.y);
    buf->putBitDouble(size.x);
    buf->put2RawDouble(center);
    buf->put3BitDouble(targetPoint);
    buf->put3BitDouble(viewDirectionFromTarget);
    buf->putBitDouble(twistAngle);
    buf->putBitDouble(lensLen);
    buf->putBitDouble(frontClippingPlaneOffset);
    buf->putBitDouble(backClippingPlaneOffset);
    buf->putBit(static_cast<std::uint8_t>(viewMode & 1));
    buf->putBit(static_cast<std::uint8_t>((viewMode >> 1) & 1));
    buf->putBit(static_cast<std::uint8_t>((viewMode >> 2) & 1));
    // DWG stores the fourth view-mode bit inverted from DXF group 71 bit 4.
    buf->putBit(static_cast<std::uint8_t>((viewMode & 16) ? 0 : 1));
    if (version > DRW::AC1014) {
        buf->putRawChar8(static_cast<std::uint8_t>(renderMode));
        if (version > DRW::AC1018) {
            buf->putBit(static_cast<std::uint8_t>(m_useDefaultLights ? 1 : 0));
            buf->putRawChar8(m_defaultLightingType);
            buf->putBitDouble(m_brightness);
            buf->putBitDouble(m_contrast);
            buf->putCmColor(version, static_cast<std::uint16_t>(m_ambientColor));
        }
    }
    buf->putBit(static_cast<std::uint8_t>(flags & 1));  // paper-space flag
    if (version > DRW::AC1014) {
        buf->putBit(static_cast<std::uint8_t>(hasUCS ? 1 : 0));
        if (hasUCS) {
            buf->put3BitDouble(ucsOrigin);
            buf->put3BitDouble(ucsXAxis);
            buf->put3BitDouble(ucsYAxis);
            buf->putBitDouble(ucsElevation);
            buf->putBitShort(static_cast<std::uint16_t>(ucsOrthoType));
        }
    }
    if (version > DRW::AC1018)
        buf->putBit(static_cast<std::uint8_t>(cameraPlottable ? 1 : 0));

    hb->putHandle(makeRefW(kViewControl, 4));  // owner = soft pointer (4)
    hb->putHandle(makeSoftOwnerW(0));  // XDic null
    hb->putHandle(makeNullHandleW());  // XRef null
    if (version > DRW::AC1018) {
        hb->putHandle(makeNullHandleW());  // backgroundH null
        hb->putHandle(makeNullHandleW());  // visualStyleH null
        hb->putHandle(makeNullHandleW());  // sunH null
    }
    if (version > DRW::AC1014) {
        hb->putHandle(makeNullHandleW());  // baseUCSH null
        hb->putHandle(makeNullHandleW());  // namedUCSH null
    }
    if (version > DRW::AC1018)
        hb->putHandle(makeNullHandleW());  // liveSectionH null
    return true;
}

bool DRW_AppId::encodeDwg(DRW::Version version, dwgBufferW *buf,
                           dwgBufferW *strBuf, dwgBufferW *hdlBuf) const {
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (hdlBuf && version > DRW::AC1018) ? hdlBuf : buf;

    sb->putVariableText(version, name);
    buf->putBit(static_cast<std::uint8_t>((flags >> 6) & 1));
    buf->putBitShort(0);  // xrefindex (always)
    buf->putBit(static_cast<std::uint8_t>((flags >> 4) & 1));
    buf->putRawChar8(0);  // unknown code 71

    hb->putHandle(makeRefW(kAppIdControl, 4));  // owner = soft pointer (4)
    hb->putHandle(makeSoftOwnerW(0));  // XDic null
    hb->putHandle(makeNullHandleW());  // XRef null
    return true;
}

//Early OBJECTS support: keep DWG import from failing on common non-graphical
//objects that do not affect LibreCAD's 2D model directly. These parsers read
//enough sample-validated metadata to preserve references without promoting
//metadata-tail uncertainty to a geometry read failure.

bool DRW_Dictionary::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 280:
        hardOwner = reader->getInt32();
        break;
    case 281:
        cloning = reader->getInt32();
        break;
    case 3: {
        Entry entry;
        entry.m_name = reader->getUtf8String();
        m_entries.push_back(entry);   //handle (350/360) follows
        break;
    }
    case 350:   //soft-owned entry handle
    case 360:   //hard-owned entry handle
        if (!m_entries.empty())
            m_entries.back().m_handle = reader->getHandleString();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_Dictionary::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff;
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing Dictionary (base) ***************************************\n");
    if (!ret)
        return ret;
    std::uint32_t numItems = buf->getBitLong();
    if (version == DRW::AC1014)
        buf->getRawChar8();
    if (version > DRW::AC1014) {
        cloning = buf->getBitShort();
        hardOwner = buf->getRawChar8();
    }
    if (numItems > 100000)
        return false;

    std::vector<UTF8STRING> names;
    names.reserve(numItems);
    for (std::uint32_t i = 0; i < numItems; ++i)
        names.push_back(sBuf->getVariableText(version, false));

    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);

    m_entries.clear();
    m_entries.reserve(numItems);
    for (std::uint32_t i = 0; i < numItems; ++i) {
        dwgHandle itemH = buf->getOffsetHandle(handle);
        if (!names[i].empty() || itemH.ref != 0)
            m_entries.push_back({names[i], itemH.ref});
    }
    DRW_DBG("dictionary entries: "); DRW_DBG(static_cast<int>(m_entries.size())); DRW_DBG("\n");
    return true;
}

bool DRW_DictionaryWithDefault::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    if (code == 340) {   //hard pointer to the default object
        m_defaultEntryHandle = reader->getHandleString();
        return true;
    }
    return DRW_Dictionary::parseCode(code, reader);
}

bool DRW_DictionaryWithDefault::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    bool ret = DRW_Dictionary::parseDwg(version, buf, bs);
    DRW_DBG("\n***************************** parsing DictionaryWithDefault ******************************\n");
    if (!ret)
        return ret;
    dwgHandle defaultH = buf->getOffsetHandle(handle);
    m_defaultEntryHandle = defaultH.ref;
    DRW_DBG("default entry Handle: "); DRW_DBGHL(defaultH.code, defaultH.size, defaultH.ref); DRW_DBG("\n");
    return true;
}

bool DRW_DictionaryVar::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 280:
        m_schema = reader->getInt32();
        break;
    case 1:
        m_value = reader->getUtf8String();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_DictionaryVar::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {
        sBuf = &sBuff;
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing DictionaryVar ***************************************\n");
    if (!ret)
        return ret;
    m_schema = buf->getRawChar8();
    m_value = sBuf->getVariableText(version, false);

    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
    DRW_DBG("dictionary var schema: "); DRW_DBG(m_schema);
    DRW_DBG(" value: "); DRW_DBG(m_value); DRW_DBG("\n");
    return true;
}

bool DRW_XRecord::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    bool ret = DRW_TableEntry::parseDwg(version, buf, nullptr, bs);
    DRW_DBG("\n***************************** parsing XRecord ***************************************\n");
    if (!ret)
        return ret;

    std::uint32_t numDataBytes = buf->getBitLong();
    const std::uint64_t dataEnd = buf->getPosition() + numDataBytes;
    m_values.clear();
    m_handleValues.clear();

    while (buf->isGood() && buf->getPosition() < dataEnd) {
        const int code = static_cast<int>(buf->getRawShort16());
        if (xRecordCodeIsString(code)) {
            m_values.emplace_back(code, readXRecordText(version, buf));
        } else if (xRecordCodeIsPoint3D(code)) {
            DRW_Coord c;
            c.x = buf->getRawDouble();
            c.y = buf->getRawDouble();
            c.z = buf->getRawDouble();
            m_values.emplace_back(code, c);
        } else if (xRecordCodeIsDouble(code)) {
            m_values.emplace_back(code, buf->getRawDouble());
        } else if (xRecordCodeIsByte(code)) {
            m_values.emplace_back(code, static_cast<int>(buf->getRawChar8()));
        } else if (xRecordCodeIsBool(code)) {
            m_values.emplace_back(code, static_cast<int>(buf->getRawChar8() != 0));
        } else if (xRecordCodeIsInt16(code)) {
            m_values.emplace_back(code, static_cast<int>(buf->getRawShort16()));
        } else if (xRecordCodeIsInt32(code)) {
            m_values.emplace_back(code, static_cast<std::int32_t>(buf->getRawLong32()));
        } else if (code >= 160 && code <= 169) {
            m_values.emplace_back(code, static_cast<std::int64_t>(buf->getRawLong64()));
        } else if (xRecordCodeIsBinary(code)) {
            std::uint8_t len = buf->getRawChar8();
            std::vector<std::uint8_t> raw(len);
            if (len > 0)
                buf->getBytes(raw.data(), raw.size());
            m_values.emplace_back(code, std::move(raw));
        } else if (xRecordCodeIsHandle(code)) {
            std::uint64_t raw = buf->getRawLong64();
            m_handleValues.emplace_back(code, static_cast<std::uint32_t>(raw & 0xffffffffu));
        } else {
            DRW_DBG("unhandled XRECORD group code: "); DRW_DBG(code); DRW_DBG("\n");
            buf->setPosition(dataEnd);
            buf->setBitPos(0);
            break;
        }
    }

    if (version > DRW::AC1014)
        m_cloning = buf->getBitShort();

    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
    while (buf->isGood() && buf->numRemainingBytes() > 0) {
        dwgHandle h = buf->getOffsetHandle(handle);
        if (!buf->isGood())
            break;
        if (h.ref != 0)
            m_handleValues.emplace_back(0, h.ref);
    }

    DRW_DBG("xrecord values: "); DRW_DBG(static_cast<int>(m_values.size()));
    DRW_DBG(" handles: "); DRW_DBG(static_cast<int>(m_handleValues.size())); DRW_DBG("\n");
    return true;
}

// Mirror of readXRecordText.  Emits the on-disk form of a single string
// XRECORD value (excluding the leading RS group code, which the caller
// writes).  For R2007+ uses a UTF-16LE char-count + chars * 2 bytes; for
// earlier versions uses a byte-length + code-page byte + bytes.  No
// trailing null is appended — matches the parser's null-trim behavior.
static void writeXRecordText(DRW::Version version, dwgBufferW *buf,
                              const UTF8STRING& s) {
    if (version > DRW::AC1018) {
        // UTF-16LE encoding for R2007+ — mirror dwgBufferW::putUCSText
        // logic, but with the char count emitted via RS (raw short) and
        // without a trailing 0-terminator.
        std::vector<std::uint16_t> units;
        for (size_t i = 0; i < s.size(); ) {
            unsigned char c = static_cast<unsigned char>(s[i]);
            std::uint32_t cp = 0;
            if (c < 0x80) {
                cp = c; ++i;
            } else if ((c & 0xE0) == 0xC0 && i + 1 < s.size()) {
                cp = (c & 0x1F);
                cp = (cp << 6) | (static_cast<unsigned char>(s[i+1]) & 0x3F);
                i += 2;
            } else if ((c & 0xF0) == 0xE0 && i + 2 < s.size()) {
                cp = (c & 0x0F);
                cp = (cp << 6) | (static_cast<unsigned char>(s[i+1]) & 0x3F);
                cp = (cp << 6) | (static_cast<unsigned char>(s[i+2]) & 0x3F);
                i += 3;
            } else if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) {
                cp = (c & 0x07);
                cp = (cp << 6) | (static_cast<unsigned char>(s[i+1]) & 0x3F);
                cp = (cp << 6) | (static_cast<unsigned char>(s[i+2]) & 0x3F);
                cp = (cp << 6) | (static_cast<unsigned char>(s[i+3]) & 0x3F);
                i += 4;
            } else {
                cp = '?'; ++i;  // malformed UTF-8 -> replacement char
            }
            // Emit UTF-16LE: a surrogate pair for astral code points (> 0xFFFF),
            // matching dwgBufferW::putUCSText and AutoCAD/ODA. (Previously a
            // 4-byte UTF-8 sequence fell into the else and emitted one bogus unit
            // per byte -> mojibake.) NOTE: readXRecordText decodes via
            // DRW_ConvUTF16::toUtf8, which does NOT recombine surrogate pairs, so
            // an astral char is lossy on a libdxfrw->libdxfrw round-trip (re-read
            // as two WTF-8 surrogates); the emitted count/payload stay consistent
            // and match what conforming writers produce.
            if (cp > 0xFFFF) {
                cp -= 0x10000;
                units.push_back(static_cast<std::uint16_t>(0xD800 + ((cp >> 10) & 0x3FF)));
                units.push_back(static_cast<std::uint16_t>(0xDC00 + (cp & 0x3FF)));
            } else {
                units.push_back(static_cast<std::uint16_t>(cp & 0xFFFF));
            }
        }
        buf->putRawShort16(static_cast<std::uint16_t>(units.size()));
        for (std::uint16_t u : units) {
            buf->putRawChar8(static_cast<std::uint8_t>(u & 0xFF));
            buf->putRawChar8(static_cast<std::uint8_t>((u >> 8) & 0xFF));
        }
    } else {
        // Pre-R2007: byte-length + code-page byte + bytes.  Pass-through
        // UTF-8 here; the round-trip test owns the encoding choice.
        const auto& bytes = s;
        buf->putRawShort16(static_cast<std::uint16_t>(bytes.size()));
        buf->putRawChar8(0);                     // code page (0 = default)
        if (!bytes.empty())
            buf->putBytes(reinterpret_cast<const std::uint8_t*>(bytes.data()),
                          bytes.size());
    }
}

// XRECORD (AcDbXrecord) encoder.  Inverts parseDwg above.  Body: BL
// numDataBytes + variable typed-pair sequence + (R2000+) BS cloning.
// Handle stream: common prefix (parentHandle + reactors + xdic) + handles
// from m_handleValues where the recorded DXF code is 0 (the post-body
// handle refs the parser appends with code 0 to flag their stream origin).
// Data-block handles (entries in m_handleValues with code != 0) are
// emitted inside the data block at their position derived from order:
// they are interleaved with the m_values vector via the original code
// ordering preserved by the parser, but as the parser only records
// the value-vs-handle dichotomy (not strict ordering between them), the
// encoder emits all m_values in order first, then all data-block
// handles (code != 0).  Matches the parser's iteration-until-dataEnd
// loop which doesn't enforce code ordering either.
bool DRW_XRecord::encodeDwg(DRW::Version version, dwgBufferW *buf,
                             dwgBufferW *strBuf,
                             dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    (void)strBuf;                                // XRECORD uses inline strings
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018)
        ? handleBuf : buf;

    // Build the data block into a scratch buffer first so we can prefix
    // it with its size in bytes (a BL count).  The scratch buffer starts
    // byte-aligned; emitting bit-bound primitives into it would skew the
    // byte count, but XRECORD's data section uses raw* primitives only,
    // so the scratch byte size is exactly what the parser's dataEnd
    // arithmetic expects.
    dwgBufferW data;

    auto emitValue = [&](const DRW_Variant& v) {
        const int code = v.code();
        data.putRawShort16(static_cast<std::uint16_t>(code));
        if (xRecordCodeIsString(code)) {
            const char *p = (v.type() == DRW_Variant::STRING) ? v.c_str() : "";
            writeXRecordText(version, &data, std::string(p));
        } else if (xRecordCodeIsPoint3D(code)) {
            DRW_Coord *c = v.coord();
            data.putRawDouble(c ? c->x : 0.0);
            data.putRawDouble(c ? c->y : 0.0);
            data.putRawDouble(c ? c->z : 0.0);
        } else if (xRecordCodeIsDouble(code)) {
            data.putRawDouble(v.d_val());
        } else if (xRecordCodeIsByte(code)) {
            data.putRawChar8(static_cast<std::uint8_t>(v.i_val() & 0xFF));
        } else if (xRecordCodeIsBool(code)) {
            data.putRawChar8(v.i_val() ? 1 : 0);
        } else if (xRecordCodeIsInt16(code)) {
            data.putRawShort16(static_cast<std::uint16_t>(v.i_val() & 0xFFFF));
        } else if (xRecordCodeIsInt32(code)) {
            data.putRawLong32(static_cast<std::uint32_t>(v.i_val()));
        } else if (code >= 160 && code <= 169) {
            data.putRawLong64(static_cast<std::uint64_t>(v.i64_val()));
        } else if (xRecordCodeIsBinary(code)) {
            const std::vector<std::uint8_t>* raw = v.binary();
            // A DWG binary chunk is a single RC length (max 255) + that many
            // bytes.  Writing (size & 0xFF) as the length but raw->size() bytes
            // desynced the stream when size > 255 (the reader consumed only
            // `len` bytes, then mis-read the remainder as the next group). Cap
            // the chunk to 255 and write exactly `len` bytes to match.
            if (raw && raw->size() > 255u) {
                DRW_DBG("XRECORD binary chunk truncated to 255 bytes (was ");
                DRW_DBG(static_cast<int>(raw->size())); DRW_DBG(")\n");
            }
            std::uint8_t len = raw
                ? static_cast<std::uint8_t>(std::min(raw->size(), static_cast<std::size_t>(255u)))
                : 0;
            data.putRawChar8(len);
            if (raw && len != 0)
                data.putBytes(raw->data(), len);
        } else {
            // Unhandled — emit zero bytes so the size still matches.  The
            // parser logs the same case and bails out.
        }
    };

    for (const auto& v : m_values)
        emitValue(v);

    // Data-block handles — entries in m_handleValues with non-zero code.
    // The parser reads these via RLL (8 bytes) and stores the low 32 bits.
    for (const auto& hv : m_handleValues) {
        if (hv.first == 0)
            continue;                            // belongs to handle stream
        data.putRawShort16(static_cast<std::uint16_t>(hv.first));
        data.putRawLong64(static_cast<std::uint64_t>(hv.second));
    }

    // Emit numDataBytes then the accumulated data section.
    buf->putBitLong(static_cast<std::int32_t>(data.size()));
    if (data.size() > 0)
        buf->putBytes(data.data().data(), data.size());

    // Cloning short (R2000+).
    if (version > DRW::AC1014)
        buf->putBitShort(static_cast<std::int16_t>(m_cloning));

    // Common handle prefix per object base spec.
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i)
        hb->putHandle(makeSoftOwnerW(0));
    if (xDictFlag != 1)
        hb->putHandle(makeSoftOwnerW(0));

    // Trailing handle refs — those m_handleValues entries the parser
    // tagged with code 0 (handle-stream origin).  Emit as soft pointers
    // so getOffsetHandle in the parser resolves to the same ref.
    for (const auto& hv : m_handleValues) {
        if (hv.first != 0)
            continue;
        hb->putHandle(makeSoftOwnerW(hv.second));
    }
    return true;
}

bool DRW_Field::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018)
        sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing Field **********************************\n");
    if (!ret)
        return ret;

    auto finishSoft = [&]() {
        if (buf->isGood()) {
            seekObjectHandleStream(version, buf, objSize);
            readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
        }
        return true;
    };

    m_evaluatorId = sBuf->getVariableText(version, false);
    m_fieldCode = sBuf->getVariableText(version, false);

    const std::uint32_t numChildren = buf->getBitLong();
    const std::uint32_t numObjects = buf->getBitLong();
    if (numChildren > 100000 || numObjects > 100000) {
        DRW_DBG("FIELD handle counts out of range; keeping FIELD shell\n");
        return finishSoft();
    }

    if (version < DRW::AC1021)
        m_formatString = sBuf->getVariableText(version, false);

    m_evaluationOptionFlags = buf->getBitLong();
    m_filingOptionFlags = buf->getBitLong();
    m_fieldStateFlags = buf->getBitLong();
    m_evaluationStatusFlags = buf->getBitLong();
    m_evaluationErrorCode = buf->getBitLong();
    m_evaluationErrorMessage = sBuf->getVariableText(version, false);

    if (!readCadValue(version, buf, sBuf, m_value)) {
        DRW_DBG("FIELD value payload unsupported; keeping FIELD shell\n");
        return finishSoft();
    }

    m_valueString = sBuf->getVariableText(version, false);
    m_valueStringLength = buf->getBitLong();

    const std::uint32_t numChildValues = buf->getBitLong();
    if (numChildValues > 100000) {
        DRW_DBG("FIELD child value count out of range; keeping FIELD shell\n");
        return finishSoft();
    }

    m_childValues.clear();
    m_childValues.reserve(numChildValues);
    for (std::uint32_t i = 0; i < numChildValues && buf->isGood(); ++i) {
        ChildValue childValue;
        childValue.m_key = sBuf->getVariableText(version, false);
        if (!readCadValue(version, buf, sBuf, childValue.m_value)) {
            DRW_DBG("FIELD child value payload unsupported; keeping FIELD shell\n");
            return finishSoft();
        }
        m_childValues.push_back(std::move(childValue));
    }

    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);

    m_childHandles.clear();
    m_childHandles.reserve(numChildren);
    for (std::uint32_t i = 0; i < numChildren && buf->isGood(); ++i) {
        dwgHandle childH = buf->getOffsetHandle(handle);
        if (childH.ref != 0)
            m_childHandles.push_back(childH.ref);
    }

    m_objectHandles.clear();
    m_objectHandles.reserve(numObjects);
    for (std::uint32_t i = 0; i < numObjects && buf->isGood(); ++i) {
        dwgHandle objectH = buf->getOffsetHandle(handle);
        if (objectH.ref != 0)
            m_objectHandles.push_back(objectH.ref);
    }

    DRW_DBG("field evaluator: "); DRW_DBG(m_evaluatorId.c_str());
    DRW_DBG(" children: "); DRW_DBG(static_cast<int>(m_childHandles.size()));
    DRW_DBG(" objects: "); DRW_DBG(static_cast<int>(m_objectHandles.size()));
    DRW_DBG(" child values: "); DRW_DBG(static_cast<int>(m_childValues.size())); DRW_DBG("\n");
    return buf->isGood();
}

bool DRW_FieldList::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    bool ret = DRW_TableEntry::parseDwg(version, buf, nullptr, bs);
    DRW_DBG("\n***************************** parsing FieldList ******************************\n");
    if (!ret)
        return ret;

    std::uint32_t numFields = buf->getBitLong();
    m_unknown = buf->getBit();
    if (numFields > 100000)
        return false;

    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);

    m_fieldHandles.clear();
    m_fieldHandles.reserve(numFields);
    for (std::uint32_t i = 0; i < numFields && buf->isGood(); ++i) {
        dwgHandle fieldH = buf->getOffsetHandle(handle);
        if (fieldH.ref != 0)
            m_fieldHandles.push_back(fieldH.ref);
    }
    DRW_DBG("fieldlist fields: "); DRW_DBG(static_cast<int>(m_fieldHandles.size())); DRW_DBG("\n");
    return true;
}

// DATATABLE (AcDbDataTable) — ODA / libreDWG dwg2.spec DATATABLE, cross-checked
// against dwgTs parseDataTable.  Layout (both oracles agree):
//   BS flags; BL num_cols; BL num_rows; T table_name;
//   REPEAT num_cols { BL type; T text; REPEAT num_rows { BL data_long;
//                                                        BD data_double;
//                                                        T data_string } }
// The body is a DEBUG_CLASS in libreDWG ("(varies) TODO"): its cell layout is
// only partially documented, so a misaligned/truncated body can over-read.  We
// decode the reliable prefix unconditionally, walk the body best-effort with
// per-iteration isGood() guards + capped counts, and MANDATORY graceful-degrade
// (always return true — the record is delivered and the caller keeps the raw
// shelf for a lossless round-trip).
bool DRW_DataTable::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018)     // 2007+ keep strings in a separate stream
        sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing DataTable ******************************\n");
    if (!ret)
        return ret;

    auto finishSoft = [&]() {
        if (buf->isGood()) {
            seekObjectHandleStream(version, buf, objSize);
            readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
        }
        return true;
    };

    flags = buf->getBitShort();
    const std::int32_t rawCols = buf->getBitLong();
    const std::int32_t rawRows = buf->getBitLong();
    tableName = sBuf->getVariableText(version, false);

    columnCount = rawCols < 0 ? 0 : rawCols;
    rowCount = rawRows < 0 ? 0 : rawRows;

    // Bound both counts before the body walk.  isValidAssocCount rejects
    // negatives and absurd counts (>100000) that a corrupt/misaligned body
    // would otherwise turn into an unbounded loop.  Prefix stays decoded.
    if (!isValidAssocCount(columnCount) || !isValidAssocCount(rowCount)) {
        DRW_DBG("DATATABLE column/row count out of range; keeping shell\n");
        return finishSoft();
    }

    columns.clear();
    columns.reserve(static_cast<std::size_t>(columnCount));
    for (std::int32_t c = 0; c < columnCount && buf->isGood(); ++c) {
        Column col;
        col.type = buf->getBitLong();
        col.text = sBuf->getVariableText(version, false);
        col.rows.reserve(static_cast<std::size_t>(rowCount));
        for (std::int32_t r = 0; r < rowCount && buf->isGood(); ++r) {
            Row row;
            row.dataLong = buf->getBitLong();
            row.dataDouble = buf->getBitDouble();
            row.dataString = sBuf->getVariableText(version, false);
            col.rows.push_back(std::move(row));
        }
        columns.push_back(std::move(col));
    }

    DRW_DBG("datatable flags: "); DRW_DBG(flags);
    DRW_DBG(" cols: "); DRW_DBG(columnCount);
    DRW_DBG(" rows: "); DRW_DBG(rowCount);
    DRW_DBG(" name: "); DRW_DBG(tableName.c_str()); DRW_DBG("\n");

    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
    return true;
}

bool DRW_RasterVariables::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 90:
        m_classVersion = reader->getInt32();
        break;
    case 70:
        m_imageFrame = reader->getInt32();
        break;
    case 71:
        m_imageQuality = reader->getInt32();
        break;
    case 72:
        m_units = reader->getInt32();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_RasterVariables::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    bool ret = DRW_TableEntry::parseDwg(version, buf, nullptr, bs);
    DRW_DBG("\n***************************** parsing RasterVariables ************************\n");
    if (!ret)
        return ret;

    m_classVersion = buf->getBitLong();
    m_imageFrame = buf->getBitShort();
    m_imageQuality = buf->getBitShort();
    m_units = buf->getBitShort();

    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);

    DRW_DBG("raster variables classVersion: "); DRW_DBG(m_classVersion);
    DRW_DBG(" frame: "); DRW_DBG(m_imageFrame);
    DRW_DBG(" quality: "); DRW_DBG(m_imageQuality);
    DRW_DBG(" units: "); DRW_DBG(m_units); DRW_DBG("\n");
    return true;
}

bool DRW_WipeoutVariables::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    if (code == 70) {   //global display-frame flag
        m_displayFrame = reader->getInt32();
        return true;
    }
    return DRW_TableEntry::parseCode(code, reader);
}

bool DRW_WipeoutVariables::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    bool ret = DRW_TableEntry::parseDwg(version, buf, nullptr, bs);
    DRW_DBG("\n***************************** parsing WipeoutVariables ***********************\n");
    if (!ret)
        return ret;
    // ODA / libreDWG WIPEOUTVARIABLES: a single BS display-frame flag (DXF 70)
    // before START_OBJECT_HANDLE_STREAM.
    m_displayFrame = buf->getBitShort();
    if (version > DRW::AC1018)
        seekObjectHandleStream(version, buf, objSize);
    if (buf->numRemainingBytes() > 0)
        readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
    DRW_DBG("wipeout display_frame: "); DRW_DBG(m_displayFrame); DRW_DBG("\n");
    return true;
}

bool DRW_WipeoutVariables::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                     dwgBufferW *strBuf,
                                     dwgBufferW *handleBuf) const {
    if (buf == nullptr)
        return false;
    DRW_UNUSED(strBuf);
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018)
        ? handleBuf
        : buf;

    buf->putBitShort(m_displayFrame);
    putCommonObjectHandlePrefix(hb, static_cast<std::uint32_t>(parentHandle),
                                numReactors, xDictFlag);
    return true;
}

bool DRW_SortEntsTable::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    bool ret = DRW_TableEntry::parseDwg(version, buf, nullptr, bs);
    DRW_DBG("\n***************************** parsing SortEntsTable **************************\n");
    if (!ret)
        return ret;

    std::uint32_t numEntries = buf->getBitLong();
    if (numEntries > 100000)
        return false;

    m_sortHandles.clear();
    m_sortHandles.reserve(numEntries);
    for (std::uint32_t i = 0; i < numEntries && buf->isGood(); ++i) {
        dwgHandle sortH = buf->getOffsetHandle(handle);
        if (sortH.ref != 0)
            m_sortHandles.push_back(sortH.ref);
    }

    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);

    dwgHandle ownerH = buf->getOffsetHandle(handle);
    m_blockOwnerHandle = ownerH.ref;

    m_entityHandles.clear();
    m_entityHandles.reserve(numEntries);
    for (std::uint32_t i = 0; i < numEntries && buf->isGood(); ++i) {
        dwgHandle entH = buf->getOffsetHandle(handle);
        if (entH.ref != 0)
            m_entityHandles.push_back(entH.ref);
    }

    DRW_DBG("sortents entries: "); DRW_DBG(static_cast<int>(m_entityHandles.size())); DRW_DBG("\n");
    return true;
}

bool DRW_Section::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // SECTION_MANAGER / SECTION_SETTINGS DXF.  Body code 330 reuses the owner
    // handle code, so the "100 AcDbSection*" marker gates the body.  Code map
    // per dwgTs parseObjectsSectionFamilyDxf.ts.
    if (code == 100) { m_dxfInBody = true; return true; }
    if (!m_dxfInBody)
        return DRW_TableEntry::parseCode(code, reader);
    if (m_kind == Manager) {
        switch (code) {
        case 70:  m_isLive = (reader->getInt32() != 0); return true;
        case 90:  m_sectionCount = reader->getInt32(); return true;
        case 330: {
            const std::uint32_t h = reader->getHandleString();
            if (h != 0) m_sectionHandles.push_back(h);
            return true;
        }
        default: break;
        }
    } else { // Settings
        switch (code) {
        case 90:
        case 91: {
            m_settingsInts.push_back(reader->getInt32());
            const std::size_t n = m_settingsInts.size();
            if (n == 1) m_classVersion = m_settingsInts[0];
            else if (n == 2) m_sectionType = m_settingsInts[1];
            else if (n == 3) m_generationOptions = m_settingsInts[2];
            return true;
        }
        case 331: m_destinationBlockHandle = reader->getHandleString(); return true;
        default: break;
        }
    }
    return DRW_TableEntry::parseCode(code, reader);
}

bool DRW_RenderSettings::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // Positional record: accumulate per-code value vectors (the named meaning is
    // by order, resolved per-kind in finalize()).  Code map per dwgTs
    // parseObjectsRenderDxf.ts.
    switch (code) {
    case 90:
        m_longs.push_back(reader->getInt32());
        if (m_longs.size() == 1) m_classVersion = m_longs.front();
        break;
    case 1:
        m_strings.push_back(reader->getUtf8String());
        if (m_strings.size() == 1) m_name = m_strings.front();
        break;
    case 290: m_bools.push_back(reader->getInt32() != 0); break;
    case 70:  m_shorts.push_back(reader->getInt32()); break;
    case 280: m_bytes.push_back(reader->getInt32()); break;
    case 40:  m_doubles.push_back(reader->getDouble()); break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

void DRW_RenderSettings::finalize(){
    const auto lng = [&](std::size_t i, std::int32_t d){
        return i < m_longs.size() ? m_longs[i] : d; };
    const auto bl  = [&](std::size_t i){ return i < m_bools.size() ? m_bools[i] : false; };
    const auto byt = [&](std::size_t i){ return i < m_bytes.size() ? m_bytes[i] : 0; };
    const auto dbl = [&](std::size_t i){ return i < m_doubles.size() ? m_doubles[i] : 0.0; };
    if (m_kind == Environment) {
        m_fogEnabled = bl(0);
        m_fogBackgroundEnabled = bl(1);
        m_environmentImageEnabled = bl(2);
        m_fogColorR = byt(0); m_fogColorG = byt(1); m_fogColorB = byt(2);
        m_fogDensityNear = dbl(0); m_fogDensityFar = dbl(1);
        m_fogDistanceNear = dbl(2); m_fogDistanceFar = dbl(3);
    } else if (m_kind == Global) {
        m_procedure = lng(1, 0);
        m_destination = lng(2, 0);
    }
}

bool DRW_SunStudy::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbSunStudy DXF: scalar configuration only.  Codes 90/91/290 are reused
    // by the date/hour lists (a positional state machine in dwgTs); here only
    // the FIRST 90 (class version) and FIRST 290 (use_subset) are taken, the
    // rest of those lists are left to the raw-net.  291-294 are distinct codes.
    switch (code) {
    case 90:  if (m_seen90++ == 0) m_classVersion = reader->getInt32(); break;
    case 1:   m_setupName = reader->getUtf8String(); break;
    case 2:   m_description = reader->getUtf8String(); break;
    case 3:   m_sheetSetName = reader->getUtf8String(); break;
    case 4:   m_sheetSubsetName = reader->getUtf8String(); break;
    case 70:  m_outputType = static_cast<std::int16_t>(reader->getInt32()); break;
    case 290: if (m_seen290++ == 0) m_useSubset = (reader->getInt32() != 0); break;
    case 291: m_selectDatesFromCalendar = (reader->getInt32() != 0); break;
    case 292: m_selectRangeOfDates = (reader->getInt32() != 0); break;
    case 293: m_lockViewports = (reader->getInt32() != 0); break;
    case 294: m_labelViewports = (reader->getInt32() != 0); break;
    case 93:  m_startTime = reader->getInt32(); break;
    case 94:  m_endTime = reader->getInt32(); break;
    case 95:  m_interval = reader->getInt32(); break;
    case 74:  m_shadePlotType = static_cast<std::int16_t>(reader->getInt32()); break;
    case 75:  m_viewportCount = static_cast<std::int16_t>(reader->getInt32()); break;
    case 76:  m_rowCount = static_cast<std::int16_t>(reader->getInt32()); break;
    case 77:  m_columnCount = static_cast<std::int16_t>(reader->getInt32()); break;
    case 40:  m_spacing = reader->getDouble(); break;
    case 341: m_viewHandle = reader->getHandleString(); break;
    case 342: m_visualStyleHandle = reader->getHandleString(); break;
    case 343: m_textStyleHandle = reader->getHandleString(); break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_PointCloudDef::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbPointCloudDef / ...DefEx DXF.  Reactors carry only the class version.
    // Code map per dwgTs parseObjectsPointCloudDxf.ts (code 160 is a reader-
    // "skipped" range, left to the raw-net).
    switch (code) {
    case 90:  m_classVersion = reader->getInt32(); break;
    case 1:   m_sourceFilename = reader->getUtf8String(); break;
    case 280: m_isLoaded = (reader->getInt32() != 0); break;
    case 10:  m_extentsMin.x = reader->getDouble(); break;
    case 20:  m_extentsMin.y = reader->getDouble(); break;
    case 30:  m_extentsMin.z = reader->getDouble(); break;
    case 11:  m_extentsMax.x = reader->getDouble(); break;
    case 21:  m_extentsMax.y = reader->getDouble(); break;
    case 31:  m_extentsMax.z = reader->getDouble(); break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_Material::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbMaterial: only the name (1) and description (2) are decoded — the
    // visual-property fields (color/ambient/diffuse/specular/maps/...) are not
    // modeled here.  This matches dwgTs's MATERIAL decode (name + description);
    // the full record is still preserved by the raw-net for DXF round-trip.
    switch (code) {
    case 1:
        m_name = reader->getUtf8String();
        break;
    case 2:
        m_description = reader->getUtf8String();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_Background::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDb*Background DXF.  Group codes are overloaded across the six variants
    // (90 in particular appears twice for solid/gradient/ground-plane), so they
    // are interpreted per m_kind (set from the entity name).  Code map per dwgTs
    // parseObjectsBackgroundSunLightDxf.ts.
    switch (code) {
    case 90:
        if (m_kind == Gradient) {
            if (m_seen90 == 0) m_classVersion = reader->getInt32();
            else if (m_seen90 == 1) m_colorTop = reader->getInt32();
            ++m_seen90;
        } else if (m_kind == GroundPlane) {
            if (m_seen90 == 0) m_classVersion = reader->getInt32();
            else if (m_seen90 == 1) m_colorSkyZenith = reader->getInt32();
            ++m_seen90;
        } else if (m_kind == Solid) {
            if (m_seen90 == 0) m_classVersion = reader->getInt32();
            else if (m_seen90 == 1) m_solidColor = reader->getInt32();
            ++m_seen90;
        } else {
            m_classVersion = reader->getInt32();  // image/ibl/skylight: single 90
        }
        break;
    case 91:
        if (m_kind == Gradient) m_colorMiddle = reader->getInt32();
        else if (m_kind == GroundPlane) m_colorSkyHorizon = reader->getInt32();
        break;
    case 92:
        if (m_kind == Gradient) m_colorBottom = reader->getInt32();
        else if (m_kind == GroundPlane) m_colorUndergroundHorizon = reader->getInt32();
        break;
    case 93: if (m_kind == GroundPlane) m_colorUndergroundAzimuth = reader->getInt32(); break;
    case 94: if (m_kind == GroundPlane) m_colorNear = reader->getInt32(); break;
    case 95: if (m_kind == GroundPlane) m_colorFar = reader->getInt32(); break;
    case 140:
        if (m_kind == Gradient) m_horizon = reader->getDouble();
        else if (m_kind == Image) m_offset.x = reader->getDouble();
        break;
    case 141: if (m_kind == Gradient) m_height = reader->getDouble(); break;
    case 142:
        if (m_kind == Gradient) m_rotation = reader->getDouble();
        else if (m_kind == Image) m_scale.x = reader->getDouble();
        break;
    // NOTE: image-background offset.y/scale.y use DXF codes 240/242, which
    // libdxfrw's dxfReader does not classify (the 240-269 range is an unhandled
    // gap in dxfreader.cpp readRec) — those Y components are left to the raw-net
    // until the reader's code-range table is extended.
    case 40:  if (m_kind == Ibl) m_rotation = reader->getDouble(); break;
    case 1:   if (m_kind == Ibl) m_iblName = reader->getUtf8String(); break;
    case 300: if (m_kind == Image) m_fileName = reader->getUtf8String(); break;
    case 290:
        if (m_kind == Image) m_fitToScreen = (reader->getInt32() != 0);
        else if (m_kind == Ibl) {
            if (m_seen290 == 0) m_enabled = (reader->getInt32() != 0);
            else m_displayImage = (reader->getInt32() != 0);
            ++m_seen290;
        }
        break;
    case 291: if (m_kind == Image) m_maintainAspect = (reader->getInt32() != 0); break;
    case 292: if (m_kind == Image) m_useTiling = (reader->getInt32() != 0); break;
    case 340:
        if (m_kind == Ibl) m_secondaryBackgroundHandle = reader->getHandleString();
        else if (m_kind == Skylight) m_sunHandle = reader->getHandleString();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_Material::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018)
        sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing Material *******************************\n");
    if (!ret)
        return ret;

    m_name = sBuf->getVariableText(version, false);
    m_description = sBuf->getVariableText(version, false);

    if (version > DRW::AC1018) {
        seekObjectHandleStream(version, buf, objSize);
        readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
    }

    DRW_DBG("material name: "); DRW_DBG(m_name.c_str());
    DRW_DBG(" description: "); DRW_DBG(m_description.c_str()); DRW_DBG("\n");
    return true;
}

bool DRW_TableStyle::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbTableStyle DXF: the top-level scalar fields.  The nested per-cell
    // styles / row styles (data/header/title) need a sub-record state machine
    // and are left to the raw-net (round-tripped, not structurally decoded) —
    // matching dwgTs's TABLESTYLE DXF decode depth.
    switch (code) {
    case 3:   m_name = reader->getUtf8String(); break;
    case 70:  m_flowDirection = reader->getInt32(); break;
    case 71:  m_flags = reader->getInt32(); break;
    case 40:  m_horizontalCellMargin = reader->getDouble(); break;
    case 41:  m_verticalCellMargin = reader->getDouble(); break;
    case 280: m_titleSuppressed = (reader->getInt32() != 0); break;
    case 281: m_headerSuppressed = (reader->getInt32() != 0); break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_TableStyle::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018)
        sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing TableStyle *****************************\n");
    if (!ret)
        return ret;

    // Common handle stream: parentHandle + reactors + xdic must be read
    // unconditionally — seekObjectHandleStream is a no-op pre-R2007+ but
    // the inline handle prefix still needs consuming.
    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag, &parentHandle);

    m_rowStyles.clear();
    m_cellStyles.clear();

    if (version > DRW::AC1021) {
        buf->getRawChar8();
        m_name = sBuf->getVariableText(version, false);
        buf->getBitLong();
        buf->getBitLong();
        m_unknownHandle = readObjectHandleRef(hBuf);

        if (!readTableStyleCellStyle(version, buf, sBuf, hBuf,
                                     m_tableCellStyle, &m_subrecordRanges)) {
            DRW_DBG("TABLESTYLE table cell style parse incomplete\n");
            return true;
        }
        m_tableCellStyle.m_id = buf->getBitLong();
        m_tableCellStyle.m_styleClass = buf->getBitLong();
        m_tableCellStyle.m_name = sBuf->getVariableText(version, false);

        const std::uint32_t cellStyleCount = static_cast<std::uint32_t>(buf->getBitLong());
        if (cellStyleCount > kMaxTableStyleItems) {
            DRW_DBG("TABLESTYLE cell style count too large\n");
            return true;
        }
        m_cellStyles.reserve(cellStyleCount);
        for (std::uint32_t i = 0; i < cellStyleCount; ++i) {
            buf->getBitLong();
            DRW_TableStyleCellStyle style;
            if (!readTableStyleCellStyle(version, buf, sBuf, hBuf,
                                         style, &m_subrecordRanges)) {
                DRW_DBG("TABLESTYLE custom cell style parse incomplete\n");
                return true;
            }
            style.m_id = buf->getBitLong();
            style.m_styleClass = buf->getBitLong();
            style.m_name = sBuf->getVariableText(version, false);
            m_cellStyles.push_back(style);
        }

        DRW_DBG("table style name: "); DRW_DBG(m_name.c_str()); DRW_DBG("\n");
        return true;
    }

    m_name = sBuf->getVariableText(version, false);
    if (version <= DRW::AC1021) {
        m_flowDirection = buf->getBitShort();
        m_flags = buf->getBitShort();
        m_horizontalCellMargin = buf->getBitDouble();
        m_verticalCellMargin = buf->getBitDouble();
        m_titleSuppressed = buf->getBit();
        m_headerSuppressed = buf->getBit();
        m_rowStyles.reserve(3);
        for (int i = 0; i < 3; ++i) {
            DRW_TableStyleRowStyle rowStyle;
            if (!readLegacyTableStyleRowStyle(version, buf, sBuf, hBuf, rowStyle)) {
                DRW_DBG("TABLESTYLE row style parse incomplete\n");
                break;
            }
            m_rowStyles.push_back(rowStyle);
        }
    }

    DRW_DBG("table style name: "); DRW_DBG(m_name.c_str()); DRW_DBG("\n");
    return true;
}

bool DRW_CellStyleMap::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018)
        sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing CellStyleMap ***************************\n");
    if (!ret)
        return ret;

    // Common handle stream: parentHandle + reactors + xdic must be read
    // unconditionally — seekObjectHandleStream is a no-op pre-R2007+ but
    // the inline handle prefix still needs consuming.
    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag, &parentHandle);

    const std::uint32_t cellStyleCount = static_cast<std::uint32_t>(buf->getBitLong());
    if (cellStyleCount > kMaxTableStyleItems) {
        DRW_DBG("CELLSTYLEMAP cell style count too large\n");
        return true;
    }

    m_cellStyles.clear();
    m_cellStyles.reserve(cellStyleCount);
    for (std::uint32_t i = 0; i < cellStyleCount; ++i) {
        DRW_TableStyleCellStyle style;
        if (!readTableStyleCellStyle(version, buf, sBuf, hBuf,
                                     style, &m_subrecordRanges)) {
            DRW_DBG("CELLSTYLEMAP cell style parse incomplete\n");
            return true;
        }
        style.m_id = buf->getBitLong();
        style.m_styleClass = buf->getBitLong();
        style.m_name = sBuf->getVariableText(version, false);
        m_cellStyles.push_back(style);
    }

    DRW_DBG("cell style map entries: "); DRW_DBG(static_cast<int>(m_cellStyles.size())); DRW_DBG("\n");
    return true;
}

bool DRW_Layout::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    //LAYOUT embeds an AcDbPlotSettings prefix then an AcDbLayout body; codes
    //1/70/76/330 occur in both, disambiguated by the current 100-subclass.
    switch (code) {
    case 100: {
        const std::string sub = reader->getUtf8String();
        if (sub == "AcDbLayout")
            m_dxfSubclass = 1;
        else if (sub == "AcDbPlotSettings")
            m_dxfSubclass = 0;
        return true;
    }
    // ---- codes shared between the two subclasses ----
    case 1:
        if (m_dxfSubclass == 1) name = reader->getUtf8String();
        else pageSetupName = reader->getUtf8String();
        break;
    case 70:
        if (m_dxfSubclass == 1) layoutFlags = reader->getInt32();
        else plotLayoutFlags = reader->getInt32();
        break;
    case 76:
        if (m_dxfSubclass == 1) orthoViewType = reader->getInt32();
        else shadePlotMode = reader->getInt32();
        break;
    case 330:
        if (m_dxfSubclass == 1) paperSpaceBlockRecordHandle.ref = reader->getHandleString();
        else parentHandle = reader->getHandleString();   //structural owner
        break;
    // ---- AcDbPlotSettings prefix (unambiguous) ----
    case 2:  printerConfig = reader->getUtf8String(); break;
    case 4:  paperSize = reader->getUtf8String(); break;
    case 6:  plotViewName = reader->getUtf8String(); break;
    case 7:  currentStyleSheet = reader->getUtf8String(); break;
    case 40: marginLeft = reader->getDouble(); break;
    case 41: marginBottom = reader->getDouble(); break;
    case 42: marginRight = reader->getDouble(); break;
    case 43: marginTop = reader->getDouble(); break;
    case 44: paperWidth = reader->getDouble(); break;
    case 45: paperHeight = reader->getDouble(); break;
    case 46: plotOriginX = reader->getDouble(); break;
    case 47: plotOriginY = reader->getDouble(); break;
    case 48: windowMinX = reader->getDouble(); break;
    case 49: windowMinY = reader->getDouble(); break;
    case 140: windowMaxX = reader->getDouble(); break;
    case 141: windowMaxY = reader->getDouble(); break;
    case 142: realWorldUnits = reader->getDouble(); break;
    case 143: drawingUnits = reader->getDouble(); break;
    case 72: paperUnits = reader->getInt32(); break;
    case 73: plotRotation = reader->getInt32(); break;
    case 74: plotType = reader->getInt32(); break;
    case 75: scaleType = reader->getInt32(); break;
    case 77: shadePlotResLevel = reader->getInt32(); break;
    case 78: shadePlotCustomDPI = reader->getInt32(); break;
    case 147: scaleFactor = reader->getDouble(); break;
    case 148: paperImageOriginX = reader->getDouble(); break;
    case 149: paperImageOriginY = reader->getDouble(); break;
    // ---- AcDbLayout body (unambiguous) ----
    case 71: tabOrder = reader->getInt32(); break;
    case 10: limMinX = reader->getDouble(); break;
    case 20: limMinY = reader->getDouble(); break;
    case 11: limMaxX = reader->getDouble(); break;
    case 21: limMaxY = reader->getDouble(); break;
    case 12: insPoint.x = reader->getDouble(); break;
    case 22: insPoint.y = reader->getDouble(); break;
    case 32: insPoint.z = reader->getDouble(); break;
    case 13: ucsOrigin.x = reader->getDouble(); break;
    case 23: ucsOrigin.y = reader->getDouble(); break;
    case 33: ucsOrigin.z = reader->getDouble(); break;
    case 14: extMin.x = reader->getDouble(); break;
    case 24: extMin.y = reader->getDouble(); break;
    case 34: extMin.z = reader->getDouble(); break;
    case 15: extMax.x = reader->getDouble(); break;
    case 25: extMax.y = reader->getDouble(); break;
    case 35: extMax.z = reader->getDouble(); break;
    case 16: ucsXAxis.x = reader->getDouble(); break;
    case 26: ucsXAxis.y = reader->getDouble(); break;
    case 36: ucsXAxis.z = reader->getDouble(); break;
    case 17: ucsYAxis.x = reader->getDouble(); break;
    case 27: ucsYAxis.y = reader->getDouble(); break;
    case 37: ucsYAxis.z = reader->getDouble(); break;
    case 146: elevation = reader->getDouble(); break;
    case 331: lastActiveViewportHandle.ref = reader->getHandleString(); break;
    case 345: namedUcsHandle.ref = reader->getHandleString(); break;
    case 346: baseUcsHandle.ref = reader->getHandleString(); break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_Layout::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff;
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing Layout *******************************************\n");
    if (!ret)
        return ret;

    // ODA §20.4.84: LAYOUT body embeds the PLOTSETTINGS prefix inline,
    // then layout-specific fields, then a common handle tail.

    // --- PLOTSETTINGS prefix ---
    pageSetupName    = sBuf->getVariableText(version, false);
    printerConfig    = sBuf->getVariableText(version, false);
    plotLayoutFlags  = buf->getBitShort();
    marginLeft       = buf->getBitDouble();
    marginBottom     = buf->getBitDouble();
    marginRight      = buf->getBitDouble();
    marginTop        = buf->getBitDouble();
    paperWidth       = buf->getBitDouble();
    paperHeight      = buf->getBitDouble();
    paperSize        = sBuf->getVariableText(version, false);
    plotOriginX      = buf->getBitDouble();
    plotOriginY      = buf->getBitDouble();
    paperUnits       = buf->getBitShort();
    plotRotation     = buf->getBitShort();
    plotType         = buf->getBitShort();
    windowMinX       = buf->getBitDouble();
    windowMinY       = buf->getBitDouble();
    windowMaxX       = buf->getBitDouble();
    windowMaxY       = buf->getBitDouble();

    if (version < DRW::AC1018) { // R13-R2000: plotViewName precedes scale fields
        plotViewName = sBuf->getVariableText(version, false);
    }

    realWorldUnits    = buf->getBitDouble();
    drawingUnits      = buf->getBitDouble();
    currentStyleSheet = sBuf->getVariableText(version, false);
    scaleType         = buf->getBitShort();
    scaleFactor       = buf->getBitDouble();
    paperImageOriginX = buf->getBitDouble();
    paperImageOriginY = buf->getBitDouble();

    if (version >= DRW::AC1018) { // R2004+
        shadePlotMode      = buf->getBitShort();
        shadePlotResLevel  = buf->getBitShort();
        shadePlotCustomDPI = buf->getBitShort();
    }

    // --- LAYOUT-specific fields ---
    name          = sBuf->getVariableText(version, false);
    tabOrder      = buf->getBitLong();
    layoutFlags   = buf->getBitShort();
    ucsOrigin     = buf->get3BitDouble();
    limMinX       = buf->getRawDouble();
    limMinY       = buf->getRawDouble();
    limMaxX       = buf->getRawDouble();
    limMaxY       = buf->getRawDouble();
    insPoint      = buf->get3BitDouble();
    ucsXAxis      = buf->get3BitDouble();
    ucsYAxis      = buf->get3BitDouble();
    elevation     = buf->getBitDouble();
    orthoViewType = buf->getBitShort();
    extMin        = buf->get3BitDouble();
    extMax        = buf->get3BitDouble();

    if (version >= DRW::AC1018) {
        // ODA §20.4.84 / libreDWG dwg.spec FIELD_BL (num_viewports) is a BitLong,
        // not a RawLong32.  Reading it as RL desynced the rest of the body stream
        // for any AC1018+ LAYOUT with viewport handles.
        viewportCount = buf->getBitLong();
        // Bound a corrupt/hostile count before the reserve() below — mirrors the
        // file-wide guard convention (SortEntsTable numEntries, IDBUFFER numIds).
        // An unbounded viewportCount (e.g. 0x7FFFFFFF) would reserve() ~8.5 GB.
        if (viewportCount < 0 || viewportCount > 100000) {
            DRW_DBG("LAYOUT viewportCount out of range: "); DRW_DBG(viewportCount); DRW_DBG("\n");
            viewportCount = 0;
        }
    }

    // --- Handle stream ---
    // For AC1015/AC1018, the handle stream is inline (hBuf == buf); for
    // AC1024+ it lives in a separate offset.  seekObjectHandleStream is a
    // no-op pre-R2007+.  readCommonObjectHandles must be called
    // unconditionally — for R2000/R2004 the parentHandle + reactor + xdic
    // handles still sit at the head of the inline stream, even though they
    // share the byte stream with the body.
    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag,
                            &parentHandle);

    if (version >= DRW::AC1018) {
        plotViewHandle = hBuf->getOffsetHandle(handle);
    }
    if (version > DRW::AC1018) {
        visualStyleHandle = hBuf->getOffsetHandle(handle);
    }
    paperSpaceBlockRecordHandle = hBuf->getOffsetHandle(handle);
    lastActiveViewportHandle    = hBuf->getOffsetHandle(handle);
    baseUcsHandle               = hBuf->getOffsetHandle(handle);
    namedUcsHandle              = hBuf->getOffsetHandle(handle);

    if (version >= DRW::AC1018) {
        viewportHandles.clear();
        viewportHandles.reserve(viewportCount > 0 ? viewportCount : 0);
        for (std::int32_t i = 0; i < viewportCount && hBuf->isGood(); ++i) {
            viewportHandles.push_back(hBuf->getOffsetHandle(handle).ref);
        }
    }

    DRW_DBG("layout name: "); DRW_DBG(name);
    DRW_DBG(" tabOrder: "); DRW_DBG(tabOrder);
    DRW_DBG(" viewportCount: "); DRW_DBG(viewportCount); DRW_DBG("\n");

    return buf->isGood();
}

bool DRW_MLineStyle::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 70:
        flags = reader->getInt32();
        break;
    case 3:
        description = reader->getUtf8String();
        break;
    case 62:
        //Before the first element (no 49 seen) 62 is the style fill color;
        //after a 49 it is the current element's color.
        if (elements.empty())
            fillColor = reader->getInt32();
        else
            elements.back().color = reader->getInt32();
        break;
    case 51:
        startAngle = reader->getDouble();
        break;
    case 52:
        endAngle = reader->getDouble();
        break;
    case 71:
        break;  //element count; elements are appended on each 49
    case 49: {
        DRW_MLineElement element;
        element.offset = reader->getDouble();
        elements.push_back(element);
        break;
    }
    case 6:
        if (!elements.empty())
            elements.back().linetype = reader->getUtf8String();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_MLineStyle::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff;
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing MLineStyle ***************************************\n");
    if (!ret) return ret;
    // Per ODA spec §19.4.73 / libreDWG dwg_decode_MLINESTYLE:
    //   TV name, TV description, BS flags, CMC fill_color, BD start_angle,
    //   BD end_angle, RC num_lines, then per-line: BD offset, CMC color,
    //   BS lt_index OR H lt_handle (handle in handle stream R2007+).
    name = sBuf->getVariableText(version, false);
    description = sBuf->getVariableText(version, false);
    flags = buf->getBitShort();
    // Fill color: read as CMC. The reader returns the index; rgb stays
    // -1 unless CMC method is RGB. We only keep the index for now.
    std::int32_t fillRgb = -1;
    UTF8STRING dummyName, dummyBook;
    fillColor = static_cast<int>(buf->getCmColor(version, &fillRgb, sBuf, &dummyName, &dummyBook));
    startAngle = buf->getBitDouble();
    endAngle = buf->getBitDouble();
    std::uint8_t numLines = buf->getRawChar8();
    DRW_DBG("mlinestyle name: "); DRW_DBG(name);
    DRW_DBG(" desc: "); DRW_DBG(description);
    DRW_DBG(" flags: "); DRW_DBG(flags);
    DRW_DBG(" fill: "); DRW_DBG(fillColor);
    DRW_DBG(" lines: "); DRW_DBG(numLines); DRW_DBG("\n");
    if (numLines > 100) return true;  // sanity, preserve alignment
    elements.reserve(numLines);
    for (int i = 0; i < numLines; ++i) {
        DRW_MLineElement e;
        e.offset = buf->getBitDouble();
        std::int32_t elRgb = -1;
        UTF8STRING n2, b2;
        e.color = static_cast<int>(buf->getCmColor(version, &elRgb, sBuf, &n2, &b2));
        if (elRgb != -1) e.color24 = elRgb;
        // Per ODA/libreDWG (dwg.spec MLINESTYLE): PRE-R2018 stores the
        // per-element linetype as an inline signed BS index (SUB_FIELD_BSd
        // lt.index); R2018+ uses a linetype HANDLE in the object handle
        // stream instead. The previous gate was INVERTED — it read the BS
        // only for R2018+ and skipped it pre-R2018, desyncing every element
        // by one BS for the common R2000-R2013 case. Read the signed inline
        // index pre-R2018; getSBitShort preserves the negative BYLAYER(32767)/
        // BYBLOCK(32766)-style sentinels. The R2018+ handle read is 0B.4b.
        if (version < DRW::AC1032) {  // pre-R2018
            e.linetypeIndex = buf->getSBitShort();  // BSd inline lt index
        }
        elements.push_back(std::move(e));
    }
    if (version > DRW::AC1018)
        seekObjectHandleStream(version, buf, objSize);
    if (buf->numRemainingBytes() > 0)
        readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
    if (version >= DRW::AC1032) {  // R2018+: linetypes are handles
        for (DRW_MLineElement& e : elements) {
            if (buf->numRemainingBytes() <= 0)
                break;
            dwgHandle lineTypeH = buf->getHandle();
            e.linetypeHandle = lineTypeH.ref;
            DRW_DBG(" mlinestyle element linetype Handle: ");
            DRW_DBGHL(lineTypeH.code, lineTypeH.size, lineTypeH.ref); DRW_DBG("\n");
        }
    }
    return buf->isGood();
}

// MLINESTYLE fixed object type 73. Inverse of parseDwg above:
// name/description strings, scalar fields, element bodies, common object
// handles, then R2018+ per-element linetype handles.
bool DRW_MLineStyle::encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf, dwgBufferW *hdlBuf) const {
    if (buf == nullptr)
        return false;
    dwgBufferW *sb = (strBuf && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (hdlBuf && version > DRW::AC1018) ? hdlBuf : buf;

    sb->putVariableText(version, name);
    sb->putVariableText(version, description);
    buf->putBitShort(static_cast<std::uint16_t>(flags));
    buf->putCmColor(version,
                    static_cast<std::uint16_t>(fillColor < 0 ? 256 : fillColor));
    buf->putBitDouble(startAngle);
    buf->putBitDouble(endAngle);

    std::size_t numLines = elements.size();
    if (numLines > 255)
        numLines = 255;
    buf->putRawChar8(static_cast<std::uint8_t>(numLines));
    for (std::size_t i = 0; i < numLines; ++i) {
        const DRW_MLineElement& e = elements[i];
        buf->putBitDouble(e.offset);
        if (e.color24 >= 0) {
            buf->putCmColor(version,
                            static_cast<std::uint16_t>(e.color < 0 ? 256 : e.color),
                            e.color24, "", "");
        } else {
            buf->putCmColor(version,
                            static_cast<std::uint16_t>(e.color < 0 ? 256 : e.color));
        }
        if (version < DRW::AC1032)
            buf->putSBitShort(static_cast<std::int16_t>(e.linetypeIndex));
    }
    putCommonObjectHandlePrefix(hb, static_cast<std::uint32_t>(parentHandle),
                                numReactors, xDictFlag);
    if (version >= DRW::AC1032) {
        for (std::size_t i = 0; i < numLines; ++i)
            hb->putHandle(makeRefW(elements[i].linetypeHandle, 4));
    }
    return true;
}

// MLEADERSTYLE per ODA spec. Defensive parser following the MLEADER
// convention: bound-check counts and treat misalignment as non-fatal so the
// OBJECTS-section scan stays aligned even if a particular style record drifts.
bool DRW_MLeaderStyle::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbMLeaderStyle DXF.  All-scalar record (handles via 340-343); code map
    // matches the per-field comments on the class + dwgTs's MLEADERSTYLE decode.
    switch (code) {
    case 179: styleVersion = reader->getInt32(); break;
    case 170: contentType = reader->getInt32(); break;
    case 171: drawMLeaderOrder = reader->getInt32(); break;
    case 172: drawLeaderOrder = reader->getInt32(); break;
    case 90:  maxLeaderPoints = reader->getInt32(); break;
    case 40:  firstSegmentAngle = reader->getDouble(); break;
    case 41:  secondSegmentAngle = reader->getDouble(); break;
    case 173: leaderType = reader->getInt32(); break;
    case 91:  leaderColor = reader->getInt32(); break;
    case 92:  leaderLineWeight = reader->getInt32(); break;
    case 290: landingEnabled = (reader->getInt32() != 0); break;
    case 42:  landingGap = reader->getDouble(); break;
    case 291: autoIncludeLanding = (reader->getInt32() != 0); break;
    case 43:  landingDistance = reader->getDouble(); break;
    case 3:   description = reader->getUtf8String(); break;
    case 44:  arrowHeadSize = reader->getDouble(); break;
    case 300: textDefault = reader->getUtf8String(); break;
    case 174: leftAttachment = reader->getInt32(); break;
    case 178: rightAttachment = reader->getInt32(); break;
    case 175: textAngleType = reader->getInt32(); break;
    case 176: textAlignmentType = reader->getInt32(); break;
    case 93:  textColor = reader->getInt32(); break;
    case 45:  textHeight = reader->getDouble(); break;
    case 292: textFrameEnabled = (reader->getInt32() != 0); break;
    case 297: alwaysAlignTextLeft = (reader->getInt32() != 0); break;
    case 46:  alignSpace = reader->getDouble(); break;
    case 94:  blockColor = reader->getInt32(); break;
    case 47:  blockScale.x = reader->getDouble(); break;
    case 49:  blockScale.y = reader->getDouble(); break;
    case 140: blockScale.z = reader->getDouble(); break;
    case 293: blockScaleEnabled = (reader->getInt32() != 0); break;
    case 141: blockRotation = reader->getDouble(); break;
    case 294: blockRotationEnabled = (reader->getInt32() != 0); break;
    case 177: blockConnectionType = reader->getInt32(); break;
    case 142: scaleFactor = reader->getDouble(); break;
    case 295: propertyChanged = (reader->getInt32() != 0); break;
    case 296: isAnnotative = (reader->getInt32() != 0); break;
    case 143: breakSize = reader->getDouble(); break;
    case 271: attachmentDirection = reader->getInt32(); break;
    case 273: topAttachment = reader->getInt32(); break;
    case 272: bottomAttachment = reader->getInt32(); break;
    case 298: textExtended = (reader->getInt32() != 0); break;
    case 340: leaderLineTypeHandle.ref = reader->getHandleString(); break;
    case 341: arrowHeadBlockHandle.ref = reader->getHandleString(); break;
    case 342: textStyleHandle.ref = reader->getHandleString(); break;
    case 343: blockHandle.ref = reader->getHandleString(); break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_MLeaderStyle::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {  // 2007+
        sBuf = &sBuff;
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing MLeaderStyle ***************\n");
    if (!ret) return ret;

    // Per spec page 217: R2010 adds a leading style version (BS 179, expected
    // value 2).  Earlier files don't write it.  The IsNewFormat predicate the
    // spec mentions also flips on with an ACAD_MLEADERVER appid extension —
    // that's read via EED, which is already consumed by the entity preamble.
    if (version >= DRW::AC1024) {
        styleVersion = buf->getBitShort();
    }
    contentType         = buf->getBitShort();
    drawMLeaderOrder    = buf->getBitShort();
    drawLeaderOrder     = buf->getBitShort();
    maxLeaderPoints     = buf->getBitLong();
    firstSegmentAngle   = buf->getBitDouble();
    secondSegmentAngle  = buf->getBitDouble();
    leaderType          = buf->getBitShort();
    leaderColor         = buf->getCmColor(version);
    // leaderLineTypeHandle 340 — handle stream
    leaderLineWeight    = buf->getBitLong();
    landingEnabled      = buf->getBit();
    landingGap          = buf->getBitDouble();
    autoIncludeLanding  = buf->getBit();
    landingDistance     = buf->getBitDouble();
    description         = sBuf->getVariableText(version, false);
    // arrowHeadBlockHandle 341 — handle stream
    arrowHeadSize       = buf->getBitDouble();
    textDefault         = sBuf->getVariableText(version, false);
    // textStyleHandle 342 — handle stream
    leftAttachment      = buf->getBitShort();
    rightAttachment     = buf->getBitShort();
    // textAngleType(175) + alwaysAlignTextLeft(297) are gated on the object's
    // own class_version >= 2, NOT the file version. libreDWG (dwg2.spec
    // MLEADERSTYLE): R2010+ reads class_version (179) from the stream, while
    // R2007 (AC1021) does NOT write it but forces class_version = 2 — so these
    // fields ARE present on R2007. styleVersion defaults to 2 (drw_objects.h),
    // so styleVersion>=2 is true for both AC1021 and AC1024+. The old
    // `version>=AC1024` gate dropped them on AC1021 → whole-record desync for
    // ODA/AutoCAD. (write-review #21)
    if (styleVersion >= 2) {
        textAngleType   = buf->getBitShort();
    }
    textAlignmentType   = buf->getBitShort();
    textColor           = buf->getCmColor(version);
    textHeight          = buf->getBitDouble();
    textFrameEnabled    = buf->getBit();
    if (styleVersion >= 2) {
        alwaysAlignTextLeft = buf->getBit();
    }
    alignSpace          = buf->getBitDouble();
    // blockHandle 343 — handle stream
    blockColor          = buf->getCmColor(version);
    blockScale          = buf->get3BitDouble();
    blockScaleEnabled   = buf->getBit();
    blockRotation       = buf->getBitDouble();
    blockRotationEnabled = buf->getBit();
    blockConnectionType = buf->getBitShort();
    scaleFactor         = buf->getBitDouble();
    propertyChanged     = buf->getBit();
    isAnnotative        = buf->getBit();
    breakSize           = buf->getBitDouble();
    if (version >= DRW::AC1024) {
        attachmentDirection = buf->getBitShort();
        topAttachment       = buf->getBitShort();
        bottomAttachment    = buf->getBitShort();
    }
    if (version >= DRW::AC1027) {
        textExtended = buf->getBit() != 0;
    }

    // Common handle stream: parentHandle + reactors + xdic must be read
    // unconditionally — seekObjectHandleStream is a no-op pre-R2007+ but
    // the inline handle prefix still needs consuming.
    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag,
                            &parentHandle);
    leaderLineTypeHandle.ref = readObjectHandleRef(hBuf);
    arrowHeadBlockHandle.ref = readObjectHandleRef(hBuf);
    textStyleHandle.ref = readObjectHandleRef(hBuf);
    blockHandle.ref = readObjectHandleRef(hBuf);

    DRW_DBG("mleader style version: "); DRW_DBG(styleVersion);
    DRW_DBG(" contentType: "); DRW_DBG(contentType);
    DRW_DBG(" name: "); DRW_DBG(name); DRW_DBG("\n");
    return buf->isGood() && (!sBuf || sBuf->isGood())
        && (!hBuf || hBuf->isGood());
}

// IDBUFFER (AcDbIdBuffer) — ODA §20.4.79.
bool DRW_IDBuffer::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing IDBUFFER ***************\n");
    if (!ret) return ret;

    classVersion = buf->getRawChar8();             // unknown RC, always 0
    const std::uint32_t numIds = buf->getBitLong();
    if (numIds > 100000) {
        DRW_DBG("IDBUFFER numIds too large: "); DRW_DBG(numIds); DRW_DBG("\n");
        return true;       // preserve alignment for subsequent objects
    }

    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag,
                            &parentHandle);

    objIds.clear();
    objIds.reserve(numIds);
    for (std::uint32_t i = 0; i < numIds && hBuf->isGood(); ++i) {
        dwgHandle h = hBuf->getOffsetHandle(handle);
        objIds.push_back(h.ref);
    }

    DRW_DBG("IDBUFFER count: "); DRW_DBG(static_cast<int>(objIds.size()));
    DRW_DBG("\n");
    return buf->isGood();
}

// LAYER_INDEX (AcDbLayerIndex) — ODA §20.4.83.
bool DRW_LayerIndex::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing LAYER_INDEX ***********\n");
    if (!ret) return ret;

    timestamp1 = buf->getBitLong();
    timestamp2 = buf->getBitLong();
    const std::uint32_t numEntries = buf->getBitLong();
    if (numEntries > 100000) {
        DRW_DBG("LAYER_INDEX numEntries too large: ");
        DRW_DBG(numEntries); DRW_DBG("\n");
        return true;
    }

    entries.clear();
    entries.reserve(numEntries);
    for (std::uint32_t i = 0; i < numEntries && buf->isGood(); ++i) {
        DRW_LayerIndexEntry e;
        e.indexLong = buf->getBitLong();
        e.name      = sBuf->getVariableText(version, false);
        entries.push_back(std::move(e));
    }

    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag,
                            &parentHandle);

    for (auto& e : entries) {
        if (!hBuf->isGood()) break;
        dwgHandle h = hBuf->getOffsetHandle(handle);
        e.entryHandle = h.ref;
    }

    DRW_DBG("LAYER_INDEX entries: "); DRW_DBG(static_cast<int>(entries.size()));
    DRW_DBG("\n");
    return buf->isGood();
}

// SPATIAL_INDEX (AcDbSpatialIndex) — ODA §20.4.95.  Body beyond timestamps
// is unspecified ("rest of bits to handles"); only timestamps + common
// handles are captured. For AC1015/AC1018 (inline handle stream) we can't
// safely skip the opaque blob, so handle reading is gated to AC1024+
// where seekObjectHandleStream jumps to the absolute position.
bool DRW_SpatialIndex::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing SPATIAL_INDEX *********\n");
    if (!ret) return ret;

    timestamp1 = buf->getBitLong();
    timestamp2 = buf->getBitLong();

    if (version > DRW::AC1018) {  // R2007+ — seek to the handle stream
        dwgBuffer hBuff = *buf;
        seekObjectHandleStream(version, &hBuff, objSize);
        readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag,
                                &parentHandle);
    }
    // R2000/R2004: parentHandle and other handles left unset — opaque
    // unknown-blob length is not specified by ODA.
    DRW_DBG("SPATIAL_INDEX timestamps: "); DRW_DBG(timestamp1);
    DRW_DBG(" / "); DRW_DBG(timestamp2); DRW_DBG("\n");
    return buf->isGood();
}

// SCALE (AcDbScale) encoder — ODA §20.4.92.  Body fields only; no handle
// stream (parser intentionally leaves the post-body handles to the caller).
bool DRW_Scale::encodeDwg(DRW::Version version, dwgBufferW *buf,
                           dwgBufferW *strBuf) const {
    if (buf == nullptr) return false;
    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018) ? strBuf : buf;
    buf->putBitShort(static_cast<std::int16_t>(flag));
    sb->putVariableText(version, name);
    buf->putBitDouble(paperUnits);
    buf->putBitDouble(drawingUnits);
    buf->putBit(isUnitScale ? 1 : 0);
    return true;
}

// GROUP encoder — ODA §20.4.72.  Inverts parseDwg above.  Body: description
// TV + isUnnamed BS + selectable BS + handleCount BL.  Handle stream:
// common prefix + entity handles.
bool DRW_Group::encodeDwg(DRW::Version version, dwgBufferW *buf,
                           dwgBufferW *strBuf,
                           dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    sb->putVariableText(version, m_description);
    buf->putBitShort(m_isUnnamed ? 1 : 0);
    buf->putBitShort(m_selectable ? 1 : 0);
    buf->putBitLong(static_cast<std::int32_t>(m_entityHandles.size()));

    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    for (std::uint32_t h : m_entityHandles) {
        hb->putHandle(makeRefW(h, 5));     // GROUP members = hard pointer (5) per libreDWG dwg.spec:4936
    }
    return true;
}

// LAYOUT (AcDbLayout) encoder — ODA §20.4.84.  Inverts parseDwg above.
// Body lays out the PLOTSETTINGS prefix inline, then layout-specific
// fields, then the common handle prefix and type-specific handle tail.
// Handle ordering must match the parser: common prefix
// (parentHandle + reactors + xdic) comes FIRST, then the type-specific
// plotView/visualStyle/paperSpaceBlockRecord/lastActiveViewport/baseUcs/
// namedUcs handles, then per-viewport handles (R2004+).
bool DRW_Layout::encodeDwg(DRW::Version version, dwgBufferW *buf,
                            dwgBufferW *strBuf,
                            dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    // --- PLOTSETTINGS prefix ---
    sb->putVariableText(version, pageSetupName);
    sb->putVariableText(version, printerConfig);
    buf->putBitShort(static_cast<std::int16_t>(plotLayoutFlags));
    buf->putBitDouble(marginLeft);
    buf->putBitDouble(marginBottom);
    buf->putBitDouble(marginRight);
    buf->putBitDouble(marginTop);
    buf->putBitDouble(paperWidth);
    buf->putBitDouble(paperHeight);
    sb->putVariableText(version, paperSize);
    buf->putBitDouble(plotOriginX);
    buf->putBitDouble(plotOriginY);
    buf->putBitShort(static_cast<std::int16_t>(paperUnits));
    buf->putBitShort(static_cast<std::int16_t>(plotRotation));
    buf->putBitShort(static_cast<std::int16_t>(plotType));
    buf->putBitDouble(windowMinX);
    buf->putBitDouble(windowMinY);
    buf->putBitDouble(windowMaxX);
    buf->putBitDouble(windowMaxY);

    if (version < DRW::AC1018) {  // R13-R2000: plotViewName precedes scale fields
        sb->putVariableText(version, plotViewName);
    }

    buf->putBitDouble(realWorldUnits);
    buf->putBitDouble(drawingUnits);
    sb->putVariableText(version, currentStyleSheet);
    buf->putBitShort(static_cast<std::int16_t>(scaleType));
    buf->putBitDouble(scaleFactor);
    buf->putBitDouble(paperImageOriginX);
    buf->putBitDouble(paperImageOriginY);

    if (version >= DRW::AC1018) {  // R2004+
        buf->putBitShort(static_cast<std::int16_t>(shadePlotMode));
        buf->putBitShort(static_cast<std::int16_t>(shadePlotResLevel));
        buf->putBitShort(static_cast<std::int16_t>(shadePlotCustomDPI));
    }

    // --- LAYOUT-specific fields ---
    sb->putVariableText(version, name);
    buf->putBitLong(tabOrder);
    buf->putBitShort(static_cast<std::int16_t>(layoutFlags));
    buf->put3BitDouble(ucsOrigin);
    buf->putRawDouble(limMinX);
    buf->putRawDouble(limMinY);
    buf->putRawDouble(limMaxX);
    buf->putRawDouble(limMaxY);
    buf->put3BitDouble(insPoint);
    buf->put3BitDouble(ucsXAxis);
    buf->put3BitDouble(ucsYAxis);
    buf->putBitDouble(elevation);
    buf->putBitShort(static_cast<std::int16_t>(orthoViewType));
    buf->put3BitDouble(extMin);
    buf->put3BitDouble(extMax);

    if (version >= DRW::AC1018) {
        // BitLong per ODA §20.4.84 / libreDWG FIELD_BL (num_viewports); see the
        // matching getBitLong() in parseDwg.
        buf->putBitLong(viewportCount);
    }

    // --- Handle stream ---
    // Common prefix (parentHandle + reactors + xdic) FIRST, matching the
    // post-PR-2 parser's readCommonObjectHandles call ordering.
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));    // reactor refs — null
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));    // xdic
    }

    // Type-specific tail per ODA §20.4.84.  plotViewHandle and visualStyleHandle
    // are gated on the same version branches as the parser.
    if (version >= DRW::AC1018) {
        hb->putHandle(writeHandleOrHardPtr(plotViewHandle));
    }
    if (version > DRW::AC1018) {
        hb->putHandle(writeHandleOrHardPtr(visualStyleHandle));
    }
    hb->putHandle(writeHandleOrHardPtr(paperSpaceBlockRecordHandle));
    hb->putHandle(writeHandleOrHardPtr(lastActiveViewportHandle));
    hb->putHandle(writeHandleOrHardPtr(baseUcsHandle));
    hb->putHandle(writeHandleOrHardPtr(namedUcsHandle));

    if (version >= DRW::AC1018) {
        for (std::int32_t i = 0; i < viewportCount; ++i) {
            std::uint32_t ref = (i < static_cast<std::int32_t>(viewportHandles.size()))
                ? viewportHandles[static_cast<size_t>(i)]
                : 0;
            hb->putHandle(makeSoftOwnerW(ref));
        }
    }
    return true;
}

// DICTIONARY (AcDbDictionary) encoder — ODA §20.4.44.  Inverts parseDwg
// above.  Body: numItems BL + (AC1014 RC=0 OR AC1015+ cloning BS +
// hardOwner RC) + per-entry name TV.  Handle stream: common prefix
// (parent + reactors + xdic) + per-entry handle.
bool DRW_Dictionary::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                dwgBufferW *strBuf,
                                dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    const std::int32_t numItems = static_cast<std::int32_t>(m_entries.size());
    buf->putBitLong(numItems);
    if (version == DRW::AC1014) {
        buf->putRawChar8(0);
    } else if (version > DRW::AC1014) {
        buf->putBitShort(static_cast<std::int16_t>(cloning));
        buf->putRawChar8(static_cast<std::uint8_t>(hardOwner));
    }
    for (const auto& e : m_entries) {
        sb->putVariableText(version, e.m_name);
    }

    // Common handle prefix (parentHandle + reactors + xdic) — must precede
    // per-entry handles per ODA §20.4.44.
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));    // reactor refs — null
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));    // xdic
    }
    for (const auto& e : m_entries) {
        hb->putHandle(makeRefW(e.m_handle, 2));  // DICTIONARY itemhandle = soft owner (2) per libreDWG dwg.spec:3232
    }
    return true;
}

// RASTERVARIABLES (AcDbRasterVariables) encoder — ODA §20.4.91.  Inverts
// parseDwg above.  Body: classVersion BL + frame BS + quality BS + units BS.
// Handle stream: common prefix only (no type-specific handles).
bool DRW_RasterVariables::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                     dwgBufferW *strBuf,
                                     dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    DRW_UNUSED(strBuf);
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    buf->putBitLong(static_cast<std::int32_t>(m_classVersion));
    buf->putBitShort(static_cast<std::int16_t>(m_imageFrame));
    buf->putBitShort(static_cast<std::int16_t>(m_imageQuality));
    buf->putBitShort(static_cast<std::int16_t>(m_units));

    // Common handle prefix (parentHandle + reactors + xdic).
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    return true;
}

// SORTENTSTABLE (AcDbSortentsTable) encoder — ODA §20.4.93.  Inverts
// parseDwg above.  Layout: numEntries BL + N sort handles (INLINE in body
// via getOffsetHandle in the parser), then common handle prefix
// (parent + reactors + xdic), then block-owner handle, then N entity handles.
//
// The parser reads sort handles via `buf->getOffsetHandle` BEFORE the
// common prefix — so we must mirror that by emitting them inline in the
// body section, not in the handle stream tail.
bool DRW_SortEntsTable::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                   dwgBufferW *strBuf,
                                   dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    DRW_UNUSED(strBuf);
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    // The parser reads numEntries handles in BOTH the sort loop and the entity
    // loop (one shared count), storing only the non-zero refs — so m_sortHandles
    // and m_entityHandles each hold <= numEntries and can differ in size. Emit a
    // single count and EXACTLY that many handles in each loop, padding the shorter
    // with null handles (ref 0, which the parser reads-but-skips). A count that
    // matched only one loop would desync the block-owner + following handles.
    const std::size_t entryCount = std::max(m_sortHandles.size(), m_entityHandles.size());
    buf->putBitLong(static_cast<std::int32_t>(entryCount));

    // Sort handles — emitted inline in body to match parser's
    // `buf->getOffsetHandle(handle)` reads (pre-common-prefix).
    for (std::size_t i = 0; i < entryCount; ++i) {
        buf->putHandle(makeSoftOwnerW(i < m_sortHandles.size() ? m_sortHandles[i] : 0));
    }

    // Common handle prefix.
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }

    // Block-owner handle (soft pointer to the parent BlockRecord).
    hb->putHandle(makeRefW(m_blockOwnerHandle, 4));  // SORTENTSTABLE block_owner = soft pointer (4) per libreDWG dwg2.spec:160

    // Entity handles — same count as the sort loop (pad the shorter with null).
    for (std::size_t i = 0; i < entryCount; ++i) {
        hb->putHandle(makeSoftOwnerW(i < m_entityHandles.size() ? m_entityHandles[i] : 0));
    }
    return true;
}

// SPATIAL_FILTER (AcDbSpatialFilter) encoder — ODA §20.4.94 / libreDWG
// dwg2.spec DWG_OBJECT(SPATIAL_FILTER).  Inverts parseDwg above.  Body order:
// point-count BS + N 2RD points + 3BD normal + 3BD origin + display-boundary BS
// + (front/back) clip flags BS + optional clip distances BD + two 4x3 BD
// transform matrices, THEN the common handle prefix (START_OBJECT_HANDLE_STREAM)
// LAST.  For AC1015/AC1018 the handle stream is inline (hb == buf), so writing
// the common prefix first would interleave its bits into the body BitShort the
// parser expects.
bool DRW_SpatialFilter::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                   dwgBufferW *strBuf,
                                   dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    DRW_UNUSED(strBuf);
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    const std::int16_t pointCount = static_cast<std::int16_t>(m_boundaryPoints.size());
    buf->putBitShort(pointCount);
    for (const auto& p : m_boundaryPoints) {
        buf->put2RawDouble(p);
    }

    buf->put3BitDouble(m_normal);
    buf->put3BitDouble(m_origin);
    buf->putBitShort(m_displayBoundary ? 1 : 0);
    buf->putBitShort(m_clipFrontPlane ? 1 : 0);
    if (m_clipFrontPlane) {
        buf->putBitDouble(m_frontDistance);
    }
    buf->putBitShort(m_clipBackPlane ? 1 : 0);
    if (m_clipBackPlane) {
        buf->putBitDouble(m_backDistance);
    }

    // 4x3 transforms — 12 bit-doubles each.  Pad with 0.0 if the parsed
    // vector is short (defensive: parser always reads exactly 12).
    auto writeMatrix12 = [&](const std::vector<double>& m) {
        for (size_t i = 0; i < 12; ++i) {
            buf->putBitDouble(i < m.size() ? m[i] : 0.0);
        }
    };
    writeMatrix12(m_inverseInsertTransform);
    writeMatrix12(m_insertTransform);

    // Common handle prefix LAST (matches parser's post-body readCommonObjectHandles).
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    return true;
}

// GEODATA (AcDbGeoData) encoder — ODA §20.4.78.  Inverts parseDwg above.
// Body order (per the post-fix parser): common handle prefix FIRST, then
// version BL, then host-block handle (in handle stream), then body fields
// gated on m_version (1 = legacy R2009, 2/3 = R2010+).
//
// Mesh points + faces always follow the version-specific body when
// m_version is 2 or 3 (for legacy v1, the parser falls through to
// observation tags + mesh blocks too).
bool DRW_GeoData::encodeDwg(DRW::Version version, dwgBufferW *buf,
                             dwgBufferW *strBuf,
                             dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    // Common handle prefix FIRST.
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }

    buf->putBitLong(m_version);
    // Host block handle reads from the handle stream immediately after the
    // common prefix in the parser.
    hb->putHandle(makeSoftOwnerW(m_hostBlockHandle));
    buf->putBitShort(m_coordinatesType);

    if (m_version == 1) {
        buf->put3BitDouble(m_referencePoint);
        buf->putBitLong(m_horizontalUnits);
        buf->put3BitDouble(m_designPoint);
        buf->put3BitDouble(DRW_Coord{0.0, 0.0, 0.0});   // obsolete
        buf->put3BitDouble(m_upDirection);
        // Parser computes angle = pi/2 - bitDouble, then
        // northDirection = (cos(angle), sin(angle), 0).  Round-trip the
        // angle directly by inverting: angle = atan2(north.y, north.x).
        const double angle = std::atan2(m_northDirection.y, m_northDirection.x);
        buf->putBitDouble(M_PI / 2.0 - angle);
        buf->put3BitDouble(DRW_Coord{1.0, 1.0, 1.0});   // obsolete
        sb->putVariableText(version, m_coordinateSystemDefinition);
        sb->putVariableText(version, m_geoRssTag);
        buf->putBitDouble(m_horizontalUnitScale);
        sb->putVariableText(version, std::string());     // obsolete datum
        sb->putVariableText(version, std::string());     // obsolete WKT
    } else if (m_version == 2 || m_version == 3) {
        buf->put3BitDouble(m_designPoint);
        buf->put3BitDouble(m_referencePoint);
        buf->putBitDouble(m_horizontalUnitScale);
        buf->putBitLong(m_horizontalUnits);
        buf->putBitDouble(m_verticalUnitScale);
        buf->putBitLong(m_verticalUnits);
        buf->put3BitDouble(m_upDirection);
        buf->put2RawDouble(m_northDirection);
        buf->putBitLong(m_scaleEstimationMethod);
        buf->putBitDouble(m_userSpecifiedScaleFactor);
        buf->putBit(m_enableSeaLevelCorrection ? 1 : 0);
        buf->putBitDouble(m_seaLevelElevation);
        buf->putBitDouble(m_coordinateProjectionRadius);
        sb->putVariableText(version, m_coordinateSystemDefinition);
        sb->putVariableText(version, m_geoRssTag);
    } else {
        return true;   // unknown version: encoder stops where the parser does
    }

    sb->putVariableText(version, m_observationFromTag);
    sb->putVariableText(version, m_observationToTag);
    sb->putVariableText(version, m_observationCoverageTag);

    const std::int32_t pointCount = static_cast<std::int32_t>(m_points.size());
    buf->putBitLong(pointCount);
    for (const auto& p : m_points) {
        buf->put2RawDouble(p.m_source);
        buf->put2RawDouble(p.m_destination);
    }

    const std::int32_t faceCount = static_cast<std::int32_t>(m_faces.size());
    buf->putBitLong(faceCount);
    for (const auto& f : m_faces) {
        buf->putBitLong(f.m_index1);
        buf->putBitLong(f.m_index2);
        buf->putBitLong(f.m_index3);
    }
    return true;
}

// DICTIONARYVAR (AcDbDictionaryVar) encoder — ODA §20.4.46.  Body: schema RC
// + value TV.  Handle stream: common prefix only.
bool DRW_DictionaryVar::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                   dwgBufferW *strBuf,
                                   dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    buf->putRawChar8(static_cast<std::uint8_t>(m_schema));
    sb->putVariableText(version, m_value);

    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    return true;
}

// DICTIONARYWDFLT (AcDbDictionaryWithDefault) encoder — ODA §20.4.45.
// Inherits DICTIONARY's body + handle stream and appends a single default-
// entry handle at the tail of the handle stream.
bool DRW_DictionaryWithDefault::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                           dwgBufferW *strBuf,
                                           dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    if (!DRW_Dictionary::encodeDwg(version, buf, strBuf, handleBuf))
        return false;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;
    hb->putHandle(makeRefW(m_defaultEntryHandle, 5));  // WDFLT defaultid = hard pointer (5) per libreDWG dwg.spec:3284
    return true;
}

// IDBUFFER (AcDbIdBuffer) encoder — ODA §20.4.79.  Body: class_version RC +
// numIds BL.  Handle stream: common prefix + N object handles.
bool DRW_IDBuffer::encodeDwg(DRW::Version version, dwgBufferW *buf,
                              dwgBufferW *strBuf,
                              dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    DRW_UNUSED(strBuf);
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    buf->putRawChar8(static_cast<std::uint8_t>(classVersion));
    buf->putBitLong(static_cast<std::int32_t>(objIds.size()));

    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    for (std::uint32_t ref : objIds) {
        hb->putHandle(makeSoftOwnerW(ref));
    }
    return true;
}

// LAYER_INDEX (AcDbLayerIndex) encoder — ODA §20.4.83.  Body: timestamp1 BL +
// timestamp2 BL + numEntries BL + N (indexLong BL + name TV).  Handle
// stream: common prefix + N entry handles (one per entry, soft pointers to
// the matching IDBUFFER).
bool DRW_LayerIndex::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                dwgBufferW *strBuf,
                                dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    buf->putBitLong(static_cast<std::int32_t>(timestamp1));
    buf->putBitLong(static_cast<std::int32_t>(timestamp2));
    buf->putBitLong(static_cast<std::int32_t>(entries.size()));
    for (const auto& e : entries) {
        buf->putBitLong(e.indexLong);
        sb->putVariableText(version, e.name);
    }

    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    for (const auto& e : entries) {
        hb->putHandle(makeSoftOwnerW(e.entryHandle));
    }
    return true;
}

// SPATIAL_INDEX (AcDbSpatialIndex) encoder — ODA §20.4.95.  Body beyond the
// timestamps is opaque per the spec; the parser captures only timestamps
// (and at R2007+ also the common handle prefix).  Encoder mirrors that —
// timestamps only at pre-R2007, plus common handles at R2007+.
bool DRW_SpatialIndex::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                  dwgBufferW *strBuf,
                                  dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    DRW_UNUSED(strBuf);
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    buf->putBitLong(static_cast<std::int32_t>(timestamp1));
    buf->putBitLong(static_cast<std::int32_t>(timestamp2));

    // R2007+: parser reads the common handle prefix.  Pre-R2007: parser
    // leaves the handles unset, so we don't write them either (the opaque
    // tail makes any inline write unsafe to round-trip).
    if (version > DRW::AC1018) {
        hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
        for (std::int32_t i = 0; i < numReactors; ++i) {
            hb->putHandle(makeSoftOwnerW(0));
        }
        if (xDictFlag != 1) {
            hb->putHandle(makeSoftOwnerW(0));
        }
    }
    return true;
}

// Helper: invert readCadValue (drw_objects.cpp:608).  Inverts the
// dataType-switch, preserving raw bytes + value.  Only dataType branches
// the parser populates round-trip cleanly; unsupported types (128, 256)
// are passed through with their raw_data only.
bool writeCadValue(DRW::Version version, dwgBufferW *buf, dwgBufferW *strBuf,
                   const DRW_CadValue& value) {
    if (version > DRW::AC1018)
        buf->putBitLong(value.m_formatFlags);
    buf->putBitLong(value.m_dataType);
    const bool emptyR2007Value = version > DRW::AC1018 && (value.m_formatFlags & 3);
    if (!emptyR2007Value) {
        switch (value.m_dataType) {
        case 0:
        case 1:
            buf->putBitLong(value.m_value.i_val());
            break;
        case 2:
            buf->putBitDouble(value.m_value.d_val());
            break;
        case 4:
        case 512:
        case 8:
            buf->putBitLong(static_cast<std::int32_t>(value.m_rawData.size()));
            for (std::uint8_t b : value.m_rawData)
                buf->putRawChar8(b);
            break;
        case 16:
        case 32: {
            buf->putBitLong(static_cast<std::int32_t>(value.m_dataSize));
            // Round-trip the bytes we have: if the parser stored a coord
            // payload via addCoord, the dataSize covers the original RD
            // values and we emit them via the coord; otherwise emit the
            // raw bytes directly (binary fallback path).
            const int dims = value.m_dataType == 16 ? 2 : 3;
            const std::uint32_t expected = static_cast<std::uint32_t>(dims) * 8;
            if (value.m_dataSize >= expected && value.m_value.type() == DRW_Variant::COORD) {
                DRW_Coord *c = value.m_value.coord();
                buf->putRawDouble(c->x);
                buf->putRawDouble(c->y);
                if (dims == 3) buf->putRawDouble(c->z);
                for (std::uint8_t b : value.m_rawData)
                    buf->putRawChar8(b);
            } else {
                for (std::uint8_t b : value.m_rawData)
                    buf->putRawChar8(b);
            }
            break;
        }
        case 64:
            buf->putHandle(makeSoftOwnerW(value.m_handle));
            break;
        default:
            // Unsupported value type — caller is expected to handle the
            // FIELD shell as best as possible; encoder returns false.
            return false;
        }
    }
    if (version > DRW::AC1018) {
        dwgBufferW *textBuf = strBuf ? strBuf : buf;
        buf->putBitLong(value.m_unitType);
        textBuf->putVariableText(version, value.m_formatString);
        textBuf->putVariableText(version, value.m_valueString);
    }
    return true;
}

// FIELD (AcDbField) encoder — ODA §20.4.66.  Body: evaluator + fieldCode TVs
// + numChildren + numObjects BLs + (pre-R2007 only) formatString TV + flag
// BLs + error message TV + value Variant + valueString TV + length BL +
// child values vector + child handles + object handles.
bool DRW_Field::encodeDwg(DRW::Version version, dwgBufferW *buf,
                           dwgBufferW *strBuf,
                           dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    sb->putVariableText(version, m_evaluatorId);
    sb->putVariableText(version, m_fieldCode);

    buf->putBitLong(static_cast<std::int32_t>(m_childHandles.size()));
    buf->putBitLong(static_cast<std::int32_t>(m_objectHandles.size()));

    if (version < DRW::AC1021)
        sb->putVariableText(version, m_formatString);

    buf->putBitLong(m_evaluationOptionFlags);
    buf->putBitLong(m_filingOptionFlags);
    buf->putBitLong(m_fieldStateFlags);
    buf->putBitLong(m_evaluationStatusFlags);
    buf->putBitLong(m_evaluationErrorCode);
    sb->putVariableText(version, m_evaluationErrorMessage);

    if (!writeCadValue(version, buf, sb, m_value))
        return false;

    sb->putVariableText(version, m_valueString);
    buf->putBitLong(m_valueStringLength);

    buf->putBitLong(static_cast<std::int32_t>(m_childValues.size()));
    for (const auto& cv : m_childValues) {
        sb->putVariableText(version, cv.m_key);
        if (!writeCadValue(version, buf, sb, cv.m_value))
            return false;
    }

    // Common handle prefix + child handles + object handles.  Note: parser
    // reads child + object handles via `buf->getOffsetHandle(handle)` after
    // `readCommonObjectHandles(buf, ...)`, i.e. all inline at AC1018.
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    for (std::uint32_t ref : m_childHandles) {
        hb->putHandle(makeSoftOwnerW(ref));
    }
    for (std::uint32_t ref : m_objectHandles) {
        hb->putHandle(makeSoftOwnerW(ref));
    }
    return true;
}

// FIELDLIST (AcDbFieldList) encoder — ODA §20.4.67.  Body: numFields BL +
// unknown bit B.  Handle stream: common prefix + N field handles.
bool DRW_FieldList::encodeDwg(DRW::Version version, dwgBufferW *buf,
                               dwgBufferW *strBuf,
                               dwgBufferW *handleBuf) const {
    if (buf == nullptr) return false;
    DRW_UNUSED(strBuf);
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    buf->putBitLong(static_cast<std::int32_t>(m_fieldHandles.size()));
    buf->putBit(m_unknown ? 1 : 0);

    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    for (std::int32_t i = 0; i < numReactors; ++i) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    if (xDictFlag != 1) {
        hb->putHandle(makeSoftOwnerW(0));
    }
    for (std::uint32_t ref : m_fieldHandles) {
        hb->putHandle(makeSoftOwnerW(ref));
    }
    return true;
}

bool DRW_MLeaderStyle::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                 dwgBufferW *strBuf,
                                 dwgBufferW *handleBuf) const {
    if (buf == nullptr || version < DRW::AC1021)
        return false;

    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018)
        ? strBuf
        : buf;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018)
        ? handleBuf
        : buf;

    if (version >= DRW::AC1024)
        buf->putBitShort(static_cast<std::int16_t>(styleVersion));
    buf->putBitShort(static_cast<std::int16_t>(contentType));
    buf->putBitShort(static_cast<std::int16_t>(drawMLeaderOrder));
    buf->putBitShort(static_cast<std::int16_t>(drawLeaderOrder));
    buf->putBitLong(maxLeaderPoints);
    buf->putBitDouble(firstSegmentAngle);
    buf->putBitDouble(secondSegmentAngle);
    buf->putBitShort(static_cast<std::int16_t>(leaderType));
    buf->putCmColor(version, static_cast<std::uint16_t>(leaderColor));
    buf->putBitLong(leaderLineWeight);
    buf->putBit(landingEnabled ? 1 : 0);
    buf->putBitDouble(landingGap);
    buf->putBit(autoIncludeLanding ? 1 : 0);
    buf->putBitDouble(landingDistance);
    sb->putVariableText(version, description);
    buf->putBitDouble(arrowHeadSize);
    sb->putVariableText(version, textDefault);
    buf->putBitShort(static_cast<std::int16_t>(leftAttachment));
    buf->putBitShort(static_cast<std::int16_t>(rightAttachment));
    // Gate on class_version>=2 (styleVersion), not file version — see parseDwg.
    // styleVersion defaults to 2 so AC1021 emits these too (libreDWG parity).
    if (styleVersion >= 2)
        buf->putBitShort(static_cast<std::int16_t>(textAngleType));
    buf->putBitShort(static_cast<std::int16_t>(textAlignmentType));
    buf->putCmColor(version, static_cast<std::uint16_t>(textColor));
    buf->putBitDouble(textHeight);
    buf->putBit(textFrameEnabled ? 1 : 0);
    if (styleVersion >= 2)
        buf->putBit(alwaysAlignTextLeft ? 1 : 0);
    buf->putBitDouble(alignSpace);
    buf->putCmColor(version, static_cast<std::uint16_t>(blockColor));
    buf->putBitDouble(blockScale.x);
    buf->putBitDouble(blockScale.y);
    buf->putBitDouble(blockScale.z);
    buf->putBit(blockScaleEnabled ? 1 : 0);
    buf->putBitDouble(blockRotation);
    buf->putBit(blockRotationEnabled ? 1 : 0);
    buf->putBitShort(static_cast<std::int16_t>(blockConnectionType));
    buf->putBitDouble(scaleFactor);
    buf->putBit(propertyChanged ? 1 : 0);
    buf->putBit(isAnnotative ? 1 : 0);
    buf->putBitDouble(breakSize);
    if (version >= DRW::AC1024) {
        buf->putBitShort(static_cast<std::int16_t>(attachmentDirection));
        buf->putBitShort(static_cast<std::int16_t>(topAttachment));
        buf->putBitShort(static_cast<std::int16_t>(bottomAttachment));
    }
    if (version >= DRW::AC1027)
        buf->putBit(textExtended ? 1 : 0);

    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    hb->putHandle(makeSoftOwnerW(0));  // XDic null
    hb->putHandle(writeHandleOrHardPtr(leaderLineTypeHandle));
    hb->putHandle(writeHandleOrHardPtr(arrowHeadBlockHandle));
    hb->putHandle(writeHandleOrHardPtr(textStyleHandle));
    hb->putHandle(writeHandleOrHardPtr(blockHandle));
    return true;
}

// UNDERLAYDEFINITION (AcDb{Pdf,Dgn,Dwf}Definition) — custom-class object.
// Layout: common preamble + TV filename + TV sheetName.
bool DRW_UnderlayDefinition::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {
        sBuf = &sBuff;
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing UNDERLAYDEFINITION ***************\n");
    if (!ret) return ret;
    filename  = sBuf->getVariableText(version, false);
    sheetName = sBuf->getVariableText(version, false);
    DRW_DBG(" filename: "); DRW_DBG(filename);
    DRW_DBG(" sheet: "); DRW_DBG(sheetName); DRW_DBG("\n");
    return buf->isGood();
}

bool DRW_UnderlayDefinition::parseCode(int code,
                                       const std::unique_ptr<dxfReader>& reader) {
    switch (code) {
    case 1:
        filename = reader->getUtf8String();
        break;
    case 2:
        sheetName = reader->getUtf8String();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_UnderlayDefinition::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                       dwgBufferW *strBuf,
                                       dwgBufferW *handleBuf) const {
    if (buf == nullptr)
        return false;
    dwgBufferW *sb = (strBuf != nullptr && version > DRW::AC1018) ? strBuf : buf;
    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018) ? handleBuf : buf;

    sb->putVariableText(version, filename);
    sb->putVariableText(version, sheetName);

    // emitRecordPreamble writes numReactors=0 and xDictFlag=0 for this path.
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));
    hb->putHandle(makeRefW(0, 3));
    return true;
}

// SCALE (AcDbScale) — annotation-scale entry, ODA §20.4.93,
// libreDWG dwg2.spec:1195-1203:
//   BS  flag           (always 0, group code 70)
//   T   name           (e.g. "1:48", group code 300)
//   BD  paperUnits     (numerator,  group code 140)
//   BD  drawingUnits   (denominator, group code 141)
//   B   isUnitScale    (true for the 1:1 entry, group code 290)
//   START_OBJECT_HANDLE_STREAM (parent dictionary, reactors, xdic)
bool DRW_Scale::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 70:
        flag = reader->getInt32();
        break;
    case 140:
        paperUnits = reader->getDouble();
        break;
    case 141:
        drawingUnits = reader->getDouble();
        break;
    case 290:
        isUnitScale = reader->getBool();
        break;
    case 300:
        name = reader->getUtf8String();  //user-visible scale label
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_Scale::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {  // 2007+ uses separate string stream
        sBuf = &sBuff;
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing SCALE (AcDbScale) ******************\n");
    if (!ret) return ret;

    flag         = buf->getBitShort();
    name         = sBuf->getVariableText(version, false);
    paperUnits   = buf->getBitDouble();
    drawingUnits = buf->getBitDouble();
    isUnitScale  = buf->getBit();

    DRW_DBG("SCALE name='"); DRW_DBG(name.c_str());
    DRW_DBG("' paper="); DRW_DBG(paperUnits);
    DRW_DBG(" drawing="); DRW_DBG(drawingUnits);
    DRW_DBG(" factor="); DRW_DBG(scaleFactor());
    DRW_DBG(" unitScale="); DRW_DBG(isUnitScale ? 1 : 0);
    DRW_DBG("\n");

    // Trailing handle stream (parent dictionary, reactors, xdic) — left to
    // the caller; the OBJECTS dispatch hands us a size-bounded slice.
    return buf->isGood();
}

bool DRW_ObjectContextData::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing OBJECTCONTEXTDATA shell ***********\n");
    if (!ret) return ret;

    auto currentBit = [&]() -> std::uint64_t {
        return currentObjectDwgBit(buf);
    };
    auto hasBody = [&]() -> bool {
        return objSize == 0 || currentBit() < objSize;
    };
    auto read2RD = [&]() {
        DRW_Coord c;
        c.x = buf->getRawDouble();
        c.y = buf->getRawDouble();
        c.z = 0.0;
        return c;
    };
    auto readDimensionBody = [&]() {
        m_definitionPoint = read2RD();
        m_isDefaultTextLocation = buf->getBit() != 0;
        m_textRotation = buf->getBitDouble();
        m_unknown293 = buf->getBit() != 0;
        m_dimTofl = buf->getBit() != 0;
        m_dimOsxd = buf->getBit() != 0;
        m_dimAtFit = buf->getBit() != 0;
        m_dimTix = buf->getBit() != 0;
        m_dimTMove = buf->getBit() != 0;
        m_overrideCode = buf->getRawChar8();
        m_hasArrow2 = buf->getBit() != 0;
        m_flipArrow2 = buf->getBit() != 0;
        m_flipArrow1 = buf->getBit() != 0;
    };

    if (hasBody())
        m_classVersion = static_cast<std::uint16_t>(buf->getBitShort());
    if (hasBody())
        m_defaultFlag = buf->getBit() != 0;

    switch (m_kind) {
    case Kind::Text:
        if (hasBody()) m_horizontalMode = static_cast<std::uint16_t>(buf->getBitShort());
        if (hasBody()) m_rotation = buf->getBitDouble();
        if (hasBody()) m_insertionPoint = read2RD();
        if (hasBody()) m_alignmentPoint = read2RD();
        break;
    case Kind::MText:
        if (hasBody()) m_attachment = buf->getBitLong();
        if (hasBody()) m_xAxisDir = buf->get3BitDouble();
        if (hasBody()) m_insertionPoint = buf->get3BitDouble();
        if (hasBody()) m_rectangleWidth = buf->getBitDouble();
        if (hasBody()) m_rectangleHeight = buf->getBitDouble();
        if (hasBody()) m_extentsWidth = buf->getBitDouble();
        if (hasBody()) m_extentsHeight = buf->getBitDouble();
        // Column data follows in some MTEXT contexts.  It is deliberately left
        // in the raw body for now; handle-stream anchoring below keeps the read
        // aligned without committing to that optional layout.
        break;
    case Kind::MTextAttribute:
        if (hasBody()) m_horizontalMode = static_cast<std::uint16_t>(buf->getBitShort());
        if (hasBody()) m_rotation = buf->getBitDouble();
        if (hasBody()) m_insertionPoint = read2RD();
        if (hasBody()) m_alignmentPoint = read2RD();
        if (hasBody()) {
            const bool isEnabled = buf->getBit() != 0;
            if (isEnabled && hasBody()) {
                buf->getBitShort();                 // embedded SCALE flag
                (sBuf ? sBuf : buf)->getVariableText(version, false);
                buf->getBitDouble();                // paper units
                buf->getBitDouble();                // drawing units
                buf->getBit();                      // has unit scale
            }
        }
        break;
    case Kind::OrdinateDimension:
        if (hasBody()) readDimensionBody();
        if (hasBody()) m_featureLocationPoint = buf->get3BitDouble();
        if (hasBody()) m_leaderEndpoint = buf->get3BitDouble();
        break;
    case Kind::AlignedDimension:
    case Kind::AngularDimension:
    case Kind::RadialDimension:
    case Kind::LargeRadialDimension:
    case Kind::DiameterDimension:
        if (hasBody()) readDimensionBody();
        break;
    case Kind::Leader:
        // AcDbLeaderObjectContextData. Field order per LibreDWG
        // dwg2.spec DWG_OBJECT(LEADEROBJECTCONTEXTDATA) (empirically the
        // ground truth; note x_direction precedes b290 — dwgTs has these two
        // swapped): BL num_points, num_points x 3BD points, 3BD x_direction,
        // B b290, 3BD inspt_offset, 3BD endptproj.
        if (hasBody()) {
            std::int32_t pointCount = buf->getBitLong();
            if (pointCount < 0) pointCount = 0;
            // Each point is three bit-doubles; cap so a corrupt count cannot
            // run away (the per-iteration hasBody()/isGood() guards remain the
            // primary stop and never truncate a valid list).
            const std::int32_t cap = 1 << 20;
            if (pointCount > cap) pointCount = cap;
            m_leaderPoints.reserve(
                static_cast<std::size_t>(std::min<std::int32_t>(pointCount, 4096)));
            for (std::int32_t i = 0; i < pointCount && hasBody() && buf->isGood(); ++i)
                m_leaderPoints.push_back(buf->get3BitDouble());
        }
        if (hasBody()) m_leaderXDir = buf->get3BitDouble();
        if (hasBody()) m_leaderUnknown290 = buf->getBit() != 0;
        if (hasBody()) m_leaderInsertionOffset = buf->get3BitDouble();
        if (hasBody()) m_leaderEndpointProjection = buf->get3BitDouble();
        break;
    case Kind::BlockReference:
        // AcDbBlkrefObjectContextData (dwgTs parseBlockReferenceObjectContextData):
        // BD rotation, 3RD insertionPoint, BD xScale, BD yScale, BD zScale.
        if (hasBody()) m_blkRefRotation = buf->getBitDouble();
        if (hasBody()) m_blkRefInsertionPoint = buf->get3BitDouble();
        if (hasBody()) m_blkRefScale.x = buf->getBitDouble();
        if (hasBody()) m_blkRefScale.y = buf->getBitDouble();
        if (hasBody()) m_blkRefScale.z = buf->getBitDouble();
        break;
    case Kind::AnnotScale:
    case Kind::Unknown:
    default:
        break;
    }

    // Object-context readers intentionally tolerate body under-decode: the
    // stable metadata is in the common preamble and trailing handles, while the
    // original raw bytes are still delivered through addUnsupportedObject.  Re-
    // anchor to objSize whenever present (R2000+ and R2010+), matching the TS
    // oracle's readObjectHandleData behavior and avoiding handle-shift bugs.
    dwgBuffer hBuff = *buf;
    if (objSize > 0) {
        hBuff.setPosition(objSize >> 3);
        hBuff.setBitPos(objSize & 7);
    }
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);
    m_annotatedHandle = static_cast<std::uint32_t>(parentHandle);
    if (hBuff.isGood())
        m_scaleHandle = hBuff.getHandle().ref;
    if (isDimensionFamily() && hBuff.isGood())
        m_blockHandle = hBuff.getHandle().ref;

    DRW_DBG("OBJECTCONTEXTDATA kind="); DRW_DBG(static_cast<int>(m_kind));
    DRW_DBG(" handle="); DRW_DBG(handle);
    DRW_DBG(" scale="); DRW_DBG(m_scaleHandle);
    DRW_DBG("\n");
    // Graceful-degrade: always return true. The dispatcher adds BOTH the typed
    // object and the raw-preservation shelf only if this returns true, so a
    // bounds/overrun (isGood()==false) on a malformed record must not drop the
    // raw shelf -- the body is under-decode-tolerant, the raw bytes round-trip.
    return true;
}

bool DRW_Group::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 300:
        m_description = reader->getUtf8String();
        break;
    case 70:
        m_isUnnamed = reader->getInt32() != 0;
        break;
    case 71:
        m_selectable = reader->getInt32() != 0;
        break;
    case 340:
        m_entityHandles.push_back(reader->getHandleString());
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_Group::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing GROUP *****************************\n");
    if (!ret)
        return ret;

    m_description = sBuf->getVariableText(version, false);
    m_isUnnamed = buf->getBitShort() > 0;
    m_selectable = buf->getBitShort() > 0;

    const std::int32_t handleCount = buf->getBitLong();
    if (handleCount < 0 || handleCount > 100000)
        return false;

    // Use the inline-vs-snapshot pattern matching LAYOUT/MLeaderStyle: for
    // pre-R2007+ the handle stream is inline so operate on buf directly;
    // for R2007+ snapshot buf and seek to the absolute handle-stream offset.
    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag, &parentHandle);

    m_entityHandles.clear();
    m_entityHandles.reserve(static_cast<size_t>(handleCount));
    for (std::int32_t i = 0; i < handleCount; ++i)
        m_entityHandles.push_back(readObjectHandleRef(hBuf));

    DRW_DBG("GROUP description='"); DRW_DBG(m_description.c_str());
    DRW_DBG("' handles="); DRW_DBG(static_cast<int>(m_entityHandles.size())); DRW_DBG("\n");
    return buf->isGood() && hBuf->isGood();
}

bool DRW_DimensionAssociation::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbDimAssoc DXF.  Gated by "100 AcDbDimAssoc" so body 330 is the
    // dimension handle (not the object owner).  Osnap-ref blocks open with code
    // 1 (className) then 72 (osnap type) + 331 (object handle); refs are kept in
    // arrival order (mapping to first/second/... point by associativity-flag bit
    // is a refinement).  Code map per dwgTs parseObjectsSortEntsAndDimAssocDxf.ts.
    if (code == 100) {
        if (reader->getString() == "AcDbDimAssoc")
            m_dxfInBody = true;
        return true;
    }
    if (m_dxfInBody) {
        switch (code) {
        case 330: m_dimensionHandle = reader->getHandleString(); return true;
        case 90:  m_associativityFlags = reader->getInt32(); return true;
        case 70:  m_rotatedDimensionType =
                      static_cast<std::uint8_t>(reader->getInt32()); return true;
        case 71:  m_isTransSpace = (reader->getInt32() != 0); return true;
        case 1: { // opens a new osnap reference block
            DRW_DimensionAssociationOsnapRef ref;
            ref.m_className = reader->getUtf8String();
            m_osnapRefs.push_back(ref);
            return true;
        }
        case 72:
            if (!m_osnapRefs.empty())
                m_osnapRefs.back().m_objectOsnapType =
                    static_cast<std::uint8_t>(reader->getInt32());
            return true;
        case 331:
            if (!m_osnapRefs.empty())
                m_osnapRefs.back().m_objectHandle = reader->getHandleString();
            return true;
        default: break;
        }
    }
    return DRW_TableEntry::parseCode(code, reader);
}

bool DRW_SortEntsTable::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbSortentsTable DXF.  Body codes reuse the header handle codes, so the
    // "100 AcDbSortentsTable" marker gates them: before it, 5/330 are the
    // object's own handle/owner (base); after it, 330=block owner and the
    // 331(entity)/5(sort) pairs build the draw-order map.  Code map per dwgTs
    // parseObjectsSortEntsAndDimAssocDxf.ts.
    if (code == 100) {
        if (reader->getString() == "AcDbSortentsTable")
            m_dxfInBody = true;
        return true;
    }
    if (m_dxfInBody) {
        switch (code) {
        case 330: m_blockOwnerHandle = reader->getHandleString(); return true;
        case 331: m_entityHandles.push_back(reader->getHandleString()); return true;
        case 5:   m_sortHandles.push_back(reader->getHandleString()); return true;
        default: break;
        }
    }
    return DRW_TableEntry::parseCode(code, reader);
}

bool DRW_ImageDefinitionReactor::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbRasterImageDefReactor DXF: a single class-version field (90).
    switch (code) {
    case 90: m_classVersion = reader->getInt32(); break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_ImageDefinitionReactor::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing IMAGEDEF_REACTOR ******************\n");
    if (!ret)
        return ret;

    // Read the data-stream body (class_version) BEFORE the handle stream, then
    // seek buf itself to the handle stream: a no-op for <=AC1018 (handles are
    // inline right after the body) and a reposition for R2007+. The previous
    // order read parent/reactor/xdict handles from a body-start copy, which the
    // no-op <=AC1018 seek left pointing at the class_version bytes -> garbage
    // owner handles in every pre-R2007 drawing with a raster image. This mirrors
    // the EvaluationGraph fix (7edf47978) and the DRW_Dictionary pattern.
    m_classVersion = buf->getBitLong();
    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
    DRW_UNUSED(sBuf);
    return true;  // graceful-degrade: typed body + raw shelf both delivered
}

bool DRW_SpatialFilter::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbSpatialFilter / AcDbFilter DXF: the clip boundary polygon, the OCS
    // normal + origin, clip flags/distances, and the two 4x3 transform matrices.
    auto parseCode40Stream = [this]() {
        if (m_dxfCode40Values.size() < 24)
            return;
        const std::size_t matrixStart = m_dxfCode40Values.size() - 24;
        if (matrixStart > 0)
            m_frontDistance = m_dxfCode40Values.front();
        m_inverseInsertTransform.assign(
            m_dxfCode40Values.begin() + static_cast<std::ptrdiff_t>(matrixStart),
            m_dxfCode40Values.begin() + static_cast<std::ptrdiff_t>(matrixStart + 12));
        m_insertTransform.assign(
            m_dxfCode40Values.begin() + static_cast<std::ptrdiff_t>(matrixStart + 12),
            m_dxfCode40Values.begin() + static_cast<std::ptrdiff_t>(matrixStart + 24));
    };
    switch (code) {
    case 10: m_boundaryPoints.push_back(DRW_Coord(reader->getDouble(), 0.0, 0.0)); break;
    case 20: if (!m_boundaryPoints.empty()) m_boundaryPoints.back().y = reader->getDouble(); break;
    case 210: m_normal.x = reader->getDouble(); break;
    case 220: m_normal.y = reader->getDouble(); break;
    case 230: m_normal.z = reader->getDouble(); break;
    case 11: m_origin.x = reader->getDouble(); break;
    case 21: m_origin.y = reader->getDouble(); break;
    case 31: m_origin.z = reader->getDouble(); break;
    case 71: m_displayBoundary = (reader->getInt32() != 0); break;
    case 72: m_clipFrontPlane = (reader->getInt32() != 0); break;
    case 73: m_clipBackPlane = (reader->getInt32() != 0); break;
    case 40:
        m_dxfCode40Values.push_back(reader->getDouble());
        parseCode40Stream();
        break;
    case 41: m_backDistance = reader->getDouble(); break;
    case 70: reader->getInt32(); break;  // boundary vertex count (advisory)
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_SpatialFilter::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing SPATIAL_FILTER ********************\n");
    if (!ret)
        return ret;

    // Body fields FIRST (per libreDWG dwg2.spec DWG_OBJECT(SPATIAL_FILTER) and
    // ODA 20.4.94), then the common handle prefix via START_OBJECT_HANDLE_STREAM
    // -- matching the in-tree convention in DRW_DictionaryVar/DRW_PlotSettings.
    // For AC1015/AC1018 the handle stream is inline (hBuf == buf), so reading
    // the common handle prefix before the body would desync the bit position
    // and corrupt every subsequent body BitShort.
    const std::int32_t pointCount = buf->getBitShort();
    if (pointCount < 0 || pointCount > 100000)
        return false;
    m_boundaryPoints.clear();
    m_boundaryPoints.reserve(static_cast<size_t>(pointCount));
    for (std::int32_t i = 0; i < pointCount; ++i)
        m_boundaryPoints.push_back(buf->get2RawDouble());

    m_normal = buf->get3BitDouble();
    m_origin = buf->get3BitDouble();
    m_displayBoundary = buf->getBitShort() != 0;
    m_clipFrontPlane = buf->getBitShort() != 0;
    if (m_clipFrontPlane)
        m_frontDistance = buf->getBitDouble();
    m_clipBackPlane = buf->getBitShort() != 0;
    if (m_clipBackPlane)
        m_backDistance = buf->getBitDouble();
    m_inverseInsertTransform = readObject4x3Matrix(buf);
    m_insertTransform = readObject4x3Matrix(buf);

    // Common handle prefix LAST.  For AC1015/AC1018 hBuf == buf (inline); for
    // AC1024+ hBuf is a separate cursor seeked to objSize.
    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag, &parentHandle);

    DRW_UNUSED(sBuf);
    DRW_DBG("SPATIAL_FILTER boundary points: ");
    DRW_DBG(static_cast<int>(m_boundaryPoints.size())); DRW_DBG("\n");
    return buf->isGood() && hBuf->isGood();
}

bool DRW_GeoData::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbGeoData DXF (R2010+ version 2/3).  Decodes the scalar geolocation
    // fields; the coordinate-mesh point/face lists (codes 13/23/14/24 ... 93/96)
    // are left to the raw-net (their 13/23/14/24 interleave needs a point cursor
    // — a follow-up).  Code mapping cross-checked against dwgTs
    // parseObjectsGeoSpatialDxf.ts.
    auto ensurePoint = [this](std::size_t index) {
        if (index >= 100000)
            return false;
        if (m_points.size() <= index)
            m_points.resize(index + 1);
        return true;
    };
    auto ensureFace = [this](std::size_t index) {
        if (index >= 100000)
            return false;
        if (m_faces.size() <= index)
            m_faces.resize(index + 1);
        return true;
    };
    switch (code) {
    case 100:
        m_dxfInGeoDataSubclass = reader->getUtf8String() == "AcDbGeoData";
        break;
    case 330:
        if (m_dxfInGeoDataSubclass)
            m_hostBlockHandle = reader->getHandleString();
        else
            parentHandle = reader->getHandleString();
        break;
    case 90:  m_version = reader->getInt32(); break;
    case 70:  m_coordinatesType = reader->getInt32(); break;
    case 10:  m_designPoint.x = reader->getDouble(); break;
    case 20:  m_designPoint.y = reader->getDouble(); break;
    case 30:  m_designPoint.z = reader->getDouble(); break;
    case 11:  m_referencePoint.x = reader->getDouble(); break;
    case 21:  m_referencePoint.y = reader->getDouble(); break;
    case 31:  m_referencePoint.z = reader->getDouble(); break;
    case 40:  m_horizontalUnitScale = reader->getDouble(); break;
    case 41:  m_verticalUnitScale = reader->getDouble(); break;
    case 91:  m_horizontalUnits = reader->getInt32(); break;
    case 92:  m_verticalUnits = reader->getInt32(); break;
    case 210: m_upDirection.x = reader->getDouble(); break;
    case 220: m_upDirection.y = reader->getDouble(); break;
    case 230: m_upDirection.z = reader->getDouble(); break;
    case 12:  m_northDirection.x = reader->getDouble(); break;
    case 22:  m_northDirection.y = reader->getDouble(); break;
    case 95:  m_scaleEstimationMethod = reader->getInt32(); break;
    case 141: m_userSpecifiedScaleFactor = reader->getDouble(); break;
    case 294: m_enableSeaLevelCorrection = (reader->getInt32() != 0); break;
    case 142: m_seaLevelElevation = reader->getDouble(); break;
    case 143: m_coordinateProjectionRadius = reader->getDouble(); break;
    case 301: // coordinate-system definition (chunked; ^J -> newline)
    case 303: m_coordinateSystemDefinition += reader->getUtf8String(); break;
    case 302: m_geoRssTag = reader->getUtf8String(); break;
    case 305: m_observationFromTag = reader->getUtf8String(); break;
    case 306: m_observationToTag = reader->getUtf8String(); break;
    case 307: m_observationCoverageTag = reader->getUtf8String(); break;
    case 93: {
        const int count = reader->getInt32();
        if (count < 0 || count > 100000)
            return false;
        m_points.clear();
        m_points.reserve(static_cast<std::size_t>(count));
        m_dxfSourcePointIndex = 0;
        m_dxfTargetPointIndex = 0;
        break;
    }
    case 13:
        if (!ensurePoint(m_dxfSourcePointIndex))
            return false;
        m_points[m_dxfSourcePointIndex].m_source.x = reader->getDouble();
        break;
    case 23:
        if (!ensurePoint(m_dxfSourcePointIndex))
            return false;
        m_points[m_dxfSourcePointIndex].m_source.y = reader->getDouble();
        ++m_dxfSourcePointIndex;
        break;
    case 14:
        if (!ensurePoint(m_dxfTargetPointIndex))
            return false;
        m_points[m_dxfTargetPointIndex].m_destination.x = reader->getDouble();
        break;
    case 24:
        if (!ensurePoint(m_dxfTargetPointIndex))
            return false;
        m_points[m_dxfTargetPointIndex].m_destination.y = reader->getDouble();
        ++m_dxfTargetPointIndex;
        break;
    case 96: {
        const int count = reader->getInt32();
        if (count < 0 || count > 100000)
            return false;
        m_faces.clear();
        m_faces.reserve(static_cast<std::size_t>(count));
        m_dxfFaceIndex = 0;
        break;
    }
    case 97:
        if (!ensureFace(m_dxfFaceIndex))
            return false;
        m_faces[m_dxfFaceIndex].m_index1 = reader->getInt32();
        break;
    case 98:
        if (!ensureFace(m_dxfFaceIndex))
            return false;
        m_faces[m_dxfFaceIndex].m_index2 = reader->getInt32();
        break;
    case 99:
        if (!ensureFace(m_dxfFaceIndex))
            return false;
        m_faces[m_dxfFaceIndex].m_index3 = reader->getInt32();
        ++m_dxfFaceIndex;
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_GeoData::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing GEODATA ***************************\n");
    if (!ret)
        return ret;

    // For AC1015/AC1018 the handle stream is inline (hBuf == buf); for
    // AC1024+ it lives in a separate offset reached via
    // seekObjectHandleStream(objSize).  Per the PR-2 convention, the common
    // handle prefix must be read unconditionally so that AC1015/AC1018 also
    // consumes parent + reactors + xdic before any subsequent body reads.
    dwgBuffer hBuff = *buf;
    dwgBuffer *hBuf = (version > DRW::AC1018) ? &hBuff : buf;
    seekObjectHandleStream(version, hBuf, objSize);
    readCommonObjectHandles(hBuf, handle, numReactors, xDictFlag, &parentHandle);

    m_version = buf->getBitLong();
    m_hostBlockHandle = readObjectHandleRef(hBuf);
    m_coordinatesType = buf->getBitShort();

    if (m_version == 1) {
        m_referencePoint = buf->get3BitDouble();
        m_horizontalUnits = buf->getBitLong();
        m_verticalUnits = m_horizontalUnits;
        m_designPoint = buf->get3BitDouble();
        buf->get3BitDouble(); // obsolete, ODA writes (0,0,0)
        m_upDirection = buf->get3BitDouble();
        const double angle = M_PI / 2.0 - buf->getBitDouble();
        m_northDirection = DRW_Coord(std::cos(angle), std::sin(angle), 0.0);
        buf->get3BitDouble(); // obsolete, ODA writes (1,1,1)
        m_coordinateSystemDefinition = sBuf->getVariableText(version, false);
        m_geoRssTag = sBuf->getVariableText(version, false);
        m_horizontalUnitScale = buf->getBitDouble();
        m_verticalUnitScale = m_horizontalUnitScale;
        sBuf->getVariableText(version, false); // obsolete datum name
        sBuf->getVariableText(version, false); // obsolete WKT
    } else if (m_version == 2 || m_version == 3) {
        m_designPoint = buf->get3BitDouble();
        m_referencePoint = buf->get3BitDouble();
        m_horizontalUnitScale = buf->getBitDouble();
        m_horizontalUnits = buf->getBitLong();
        m_verticalUnitScale = buf->getBitDouble();
        m_verticalUnits = buf->getBitLong();
        m_upDirection = buf->get3BitDouble();
        m_northDirection = buf->get2RawDouble();
        m_scaleEstimationMethod = buf->getBitLong();
        m_userSpecifiedScaleFactor = buf->getBitDouble();
        m_enableSeaLevelCorrection = buf->getBit() != 0;
        m_seaLevelElevation = buf->getBitDouble();
        m_coordinateProjectionRadius = buf->getBitDouble();
        m_coordinateSystemDefinition = sBuf->getVariableText(version, false);
        m_geoRssTag = sBuf->getVariableText(version, false);
    } else {
        return buf->isGood() && hBuf->isGood();
    }

    m_observationFromTag = sBuf->getVariableText(version, false);
    m_observationToTag = sBuf->getVariableText(version, false);
    m_observationCoverageTag = sBuf->getVariableText(version, false);

    const std::int32_t pointCount = buf->getBitLong();
    if (pointCount < 0 || pointCount > 100000)
        return false;
    m_points.clear();
    m_points.reserve(static_cast<size_t>(pointCount));
    for (std::int32_t i = 0; i < pointCount; ++i) {
        DRW_GeoMeshPoint point;
        point.m_source = buf->get2RawDouble();
        point.m_destination = buf->get2RawDouble();
        m_points.push_back(point);
    }

    const std::int32_t faceCount = buf->getBitLong();
    if (faceCount < 0 || faceCount > 100000)
        return false;
    m_faces.clear();
    m_faces.reserve(static_cast<size_t>(faceCount));
    for (std::int32_t i = 0; i < faceCount; ++i) {
        DRW_GeoMeshFace face;
        face.m_index1 = buf->getBitLong();
        face.m_index2 = buf->getBitLong();
        face.m_index3 = buf->getBitLong();
        m_faces.push_back(face);
    }

    DRW_DBG("GEODATA version: "); DRW_DBG(m_version);
    DRW_DBG(" points: "); DRW_DBG(static_cast<int>(m_points.size()));
    DRW_DBG(" faces: "); DRW_DBG(static_cast<int>(m_faces.size())); DRW_DBG("\n");
    return buf->isGood() && sBuf->isGood() && hBuf->isGood();
}

bool DRW_TableGeometry::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing TABLEGEOMETRY *********************\n");
    if (!ret)
        return ret;

    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);

    m_rowCount = buf->getBitLong();
    m_columnCount = buf->getBitLong();
    m_cellCount = buf->getBitLong();
    if (m_rowCount < 0 || m_columnCount < 0 || m_cellCount < 0 || m_cellCount > 100000)
        return false;

    const long long expectedCells = static_cast<long long>(m_rowCount) * static_cast<long long>(m_columnCount);
    if (expectedCells > 100000 || (expectedCells != 0 && expectedCells != m_cellCount))
        return false;

    m_cells.clear();
    m_cells.reserve(static_cast<size_t>(m_cellCount));
    for (std::int32_t i = 0; i < m_cellCount; ++i) {
        DRW_TableGeometryCell cell;
        cell.m_flags = buf->getBitLong();
        cell.m_widthWithGap = buf->getBitDouble();
        cell.m_heightWithGap = buf->getBitDouble();
        cell.m_unknownHandle = readObjectHandleRef(&hBuff);
        const std::int32_t contentCount = buf->getBitLong();
        if (contentCount < 0 || contentCount > 100000)
            return false;
        cell.m_contents.reserve(static_cast<size_t>(contentCount));
        for (std::int32_t j = 0; j < contentCount; ++j) {
            DRW_TableGeometryContent content;
            content.m_topLeft = buf->get3BitDouble();
            content.m_center = buf->get3BitDouble();
            content.m_contentWidth = buf->getBitDouble();
            content.m_contentHeight = buf->getBitDouble();
            content.m_width = buf->getBitDouble();
            content.m_height = buf->getBitDouble();
            content.m_unknown = buf->getBitLong();
            cell.m_contents.push_back(content);
        }
        m_cells.push_back(cell);
    }

    DRW_UNUSED(sBuf);
    DRW_DBG("TABLEGEOMETRY cells: "); DRW_DBG(static_cast<int>(m_cells.size())); DRW_DBG("\n");
    return buf->isGood() && hBuff.isGood();
}

bool DRW_DimensionAssociation::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018)
        sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing DIMASSOC **************************\n");
    if (!ret)
        return ret;

    if (version <= DRW::AC1018)
        return false;

    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);

    m_dimensionHandle = readObjectHandleRef(&hBuff);
    m_associativityFlags = static_cast<std::uint32_t>(buf->getBitLong());
    m_isTransSpace = buf->getBit() != 0;
    m_rotatedDimensionType = buf->getRawChar8();

    auto readOsnapRef = [&]() {
        DRW_DimensionAssociationOsnapRef ref;
        ref.m_className = sBuf->getVariableText(version, false);
        ref.m_objectOsnapType = buf->getRawChar8();
        ref.m_objectHandle = readObjectHandleRef(&hBuff);
        return ref;
    };

    m_osnapRefs.clear();
    m_osnapRefs.reserve(4);
    for (std::uint32_t flag = 1; flag <= 8; flag <<= 1) {
        if ((m_associativityFlags & flag) != 0)
            m_osnapRefs.push_back(readOsnapRef());
    }

    DRW_DBG("DIMASSOC dimension handle: "); DRW_DBG(m_dimensionHandle);
    DRW_DBG(" refs: "); DRW_DBG(static_cast<int>(m_osnapRefs.size())); DRW_DBG("\n");
    return buf->isGood() && sBuf->isGood() && hBuff.isGood();
}

bool DRW_EvaluationGraph::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018)
        sBuf = &sBuff;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing ACAD_EVALUATION_GRAPH *************\n");
    if (!ret)
        return ret;

    // Body layout (value96/97, nodes, edges) is identical at every version
    // (matches dwgTs parseEvaluationGraph + LibreDWG DWG_OBJECT(EVALUATION_GRAPH)),
    // and the body holds no strings, so sBuf is unused here. Read the whole body
    // from buf, then the handle stream — seeking buf itself the same way
    // DRW_Dictionary does — so node expression handles resolve on R2000/R2004
    // (handles inline, seek is a no-op) and R2007+ (separate handle stream) alike.
    m_value96 = buf->getBitLong();
    m_value97 = buf->getBitLong();

    std::int32_t nodeCount = buf->getBitLong();
    if (nodeCount < 0 || nodeCount > 100000)
        nodeCount = 0;  // graceful-degrade: drop the typed body, stay raw-shelved
    m_nodes.clear();
    m_nodes.reserve(static_cast<size_t>(nodeCount));
    for (std::int32_t i = 0; i < nodeCount && buf->isGood(); ++i) {
        DRW_EvaluationGraphNode node;
        node.m_index = buf->getBitLong();
        node.m_flags = buf->getBitLong();
        node.m_nextNodeIndex = buf->getBitLong();
        node.m_data1 = buf->getBitLong();
        node.m_data2 = buf->getBitLong();
        node.m_data3 = buf->getBitLong();
        node.m_data4 = buf->getBitLong();
        m_nodes.push_back(node);
    }

    std::int32_t edgeCount = buf->getBitLong();
    if (edgeCount < 0 || edgeCount > 100000)
        edgeCount = 0;  // graceful-degrade: never drop the delivered object
    m_edges.clear();
    m_edges.reserve(static_cast<size_t>(edgeCount));
    for (std::int32_t i = 0; i < edgeCount && buf->isGood(); ++i) {
        DRW_EvaluationGraphEdge edge;
        edge.m_value92 = buf->getBitLong();
        edge.m_value93 = buf->getBitLong();
        edge.m_value94 = buf->getBitLong();
        edge.m_value91a = buf->getBitLong();
        edge.m_value91b = buf->getBitLong();
        edge.m_value92a = buf->getBitLong();
        edge.m_value92b = buf->getBitLong();
        edge.m_value92c = buf->getBitLong();
        edge.m_value92d = buf->getBitLong();
        edge.m_value92e = buf->getBitLong();
        m_edges.push_back(edge);
    }

    seekObjectHandleStream(version, buf, objSize);
    readCommonObjectHandles(buf, handle, numReactors, xDictFlag, &parentHandle);
    for (auto &node : m_nodes) {
        if (!buf->isGood())
            break;
        node.m_expressionHandle = readObjectHandleRef(buf);
    }

    DRW_DBG("EVALUATION_GRAPH nodes: "); DRW_DBG(static_cast<int>(m_nodes.size()));
    DRW_DBG(" edges: "); DRW_DBG(static_cast<int>(m_edges.size())); DRW_DBG("\n");
    DRW_UNUSED(sBuf);
    return true;  // graceful-degrade: always delivered (typed body + raw shelf)
}

bool DRW_AcDbPlaceholder::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing ACDBPLACEHOLDER *******************\n");
    if (!ret)
        return ret;

    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);
    DRW_UNUSED(sBuf);
    return ret;
}

bool DRW_AcDbPlaceholder::encodeDwg(DRW::Version version, dwgBufferW *buf,
                                    dwgBufferW *handleBuf) const {
    if (buf == nullptr)
        return false;

    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018)
        ? handleBuf
        : buf;
    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    hb->putHandle(makeSoftOwnerW(0));  // XDic null
    return true;
}

bool DRW_Sun::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 90:
        m_classVersion = reader->getInt32();
        break;
    case 290:
        m_isOn = reader->getBool();
        break;
    case 63:
        m_color = reader->getInt32();
        break;
    case 421:
        m_color24 = reader->getInt32();  // 24-bit true color (optional)
        break;
    case 40:
        m_intensity = reader->getDouble();
        break;
    case 291:
        m_hasShadow = reader->getBool();
        break;
    case 91:
        m_julianDay = reader->getInt32();
        break;
    case 92:
        m_milliseconds = reader->getInt32();
        break;
    case 292:
        m_isDaylightSavings = reader->getBool();
        break;
    case 70:
        m_shadowType = reader->getInt32();
        break;
    case 71:
        m_shadowMapSize = reader->getInt32();
        break;
    case 280:
        m_shadowSoftness = reader->getInt32();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

bool DRW_Sun::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing SUN *******************************\n");
    if (!ret)
        return ret;

    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);

    m_classVersion = static_cast<std::uint32_t>(buf->getBitLong());
    m_isOn = buf->getBit() != 0;
    {
        std::int32_t rgb24 = -1;
        m_color = buf->getCmColor(version, &rgb24);  // capture optional 24-bit true color
        m_color24 = rgb24;
    }
    m_intensity = buf->getBitDouble();
    m_hasShadow = buf->getBit() != 0;
    m_julianDay = buf->getBitLong();
    m_milliseconds = buf->getBitLong();
    m_isDaylightSavings = buf->getBit() != 0;
    m_shadowType = static_cast<std::uint32_t>(buf->getBitLong());
    m_shadowMapSize = buf->getBitShort();
    m_shadowSoftness = buf->getRawChar8();

    DRW_UNUSED(sBuf);
    DRW_DBG("SUN on: "); DRW_DBG(m_isOn ? 1 : 0); DRW_DBG("\n");
    return ret;
}

bool DRW_Sun::encodeDwg(DRW::Version version, dwgBufferW *buf,
                        dwgBufferW *handleBuf) const {
    if (buf == nullptr || version < DRW::AC1021)
        return false;

    dwgBufferW *hb = (handleBuf != nullptr && version > DRW::AC1018)
        ? handleBuf
        : buf;

    buf->putBitLong(static_cast<std::int32_t>(m_classVersion));
    buf->putBit(m_isOn ? 1 : 0);
    // Carry the 24-bit true color (m_color24); putCmColor falls back to the
    // index-only encoding (byte-identical) when m_color24 < 0 (unset).
    buf->putCmColor(version, static_cast<std::uint16_t>(m_color), m_color24, {}, {});
    buf->putBitDouble(m_intensity);
    buf->putBit(m_hasShadow ? 1 : 0);
    buf->putBitLong(m_julianDay);
    buf->putBitLong(m_milliseconds);
    buf->putBit(m_isDaylightSavings ? 1 : 0);
    buf->putBitLong(static_cast<std::int32_t>(m_shadowType));
    buf->putBitShort(static_cast<std::int16_t>(m_shadowMapSize));
    buf->putRawChar8(m_shadowSoftness);

    hb->putHandle(makeRefW(static_cast<std::uint32_t>(parentHandle), 4));  // ownerhandle = soft pointer (4)
    hb->putHandle(makeSoftOwnerW(0));  // XDic null
    return true;
}

bool DRW_AssociativeObject::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing ACDBASSOC shell *******************\n");
    if (!ret)
        return ret;

    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);

    auto appendPrefixStatus = [&](DRW_AssociativePrefixStatus::Kind kind,
                                  std::uint64_t startBit,
                                  DRW_AssociativePrefixStatus::ParseStatus status,
                                  std::uint16_t classVersion,
                                  size_t decodedHandleCount = 0,
                                  size_t decodedValueCount = 0,
                                  std::int32_t decodedCountValue = 0) {
        m_prefixStatuses.push_back(makeAssocPrefixStatus(
            kind, startBit, currentObjectDwgBit(buf), status, classVersion,
            decodedHandleCount, decodedValueCount, decodedCountValue));
    };
    auto readHandleVector = [&](std::vector<std::uint32_t>& target, std::int32_t count) {
        if (!isValidAssocCount(count))
            return false;
        target.reserve(target.size() + static_cast<size_t>(count));
        for (std::int32_t i = 0; i < count; ++i)
            target.push_back(readObjectHandleRef(&hBuff));
        return hBuff.isGood();
    };
    auto readOwnedRefs = [&](std::vector<DRW_AssociativeHandleRef>& target, std::int32_t count) {
        if (!isValidAssocCount(count))
            return false;
        target.reserve(target.size() + static_cast<size_t>(count));
        for (std::int32_t i = 0; i < count; ++i) {
            DRW_AssociativeHandleRef ref;
            ref.m_isOwned = buf->getBit() != 0;
            ref.m_handle = readObjectHandleRef(&hBuff);
            target.push_back(ref);
        }
        return buf->isGood() && hBuff.isGood();
    };
    auto readActionFields = [&]() {
        const std::uint64_t startBit = currentObjectDwgBit(buf);
        size_t decodedHandleCount = 0;
        m_classVersion = buf->getBitShort();
        m_geometryStatus = buf->getBitLong();
        m_owningNetworkHandle = readObjectHandleRef(&hBuff);
        m_actionBodyHandle = readObjectHandleRef(&hBuff);
        decodedHandleCount += 2;
        m_actionIndex = buf->getBitLong();
        m_maxDependencyIndex = buf->getBitLong();
        const std::int32_t dependencyCount = buf->getBitLong();
        if (!isValidAssocCount(dependencyCount)) {
            appendPrefixStatus(
                DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
                startBit,
                DRW_AssociativePrefixStatus::ParseStatus::BoundedCountOverflow,
                m_classVersion, decodedHandleCount, m_valueParamCount,
                dependencyCount);
            return false;
        }
        if (!readOwnedRefs(m_dependencies, dependencyCount)) {
            appendPrefixStatus(
                DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
                startBit, DRW_AssociativePrefixStatus::ParseStatus::Partial,
                m_classVersion, decodedHandleCount, m_valueParamCount,
                dependencyCount);
            return false;
        }
        decodedHandleCount += m_dependencies.size();
        if (m_classVersion > 1) {
            buf->getBitShort();
            const std::int32_t ownedParamCount = buf->getBitLong();
            if (!isValidAssocCount(ownedParamCount)) {
                appendPrefixStatus(
                    DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
                    startBit,
                    DRW_AssociativePrefixStatus::ParseStatus::BoundedCountOverflow,
                    m_classVersion, decodedHandleCount, m_valueParamCount,
                    ownedParamCount);
                return false;
            }
            if (!readHandleVector(m_ownedParams, ownedParamCount)) {
                appendPrefixStatus(
                    DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
                    startBit, DRW_AssociativePrefixStatus::ParseStatus::Partial,
                    m_classVersion, decodedHandleCount, m_valueParamCount,
                    ownedParamCount);
                return false;
            }
            decodedHandleCount += m_ownedParams.size();
            m_ownedParamPrefixCount = static_cast<size_t>(ownedParamCount);
            buf->getBitShort();
            const std::int32_t valueCount = buf->getBitLong();
            if (!isValidAssocCount(valueCount, kMaxAssocValueParams)) {
                appendPrefixStatus(
                    DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
                    startBit,
                    DRW_AssociativePrefixStatus::ParseStatus::BoundedCountOverflow,
                    m_classVersion, decodedHandleCount, m_valueParamCount,
                    valueCount);
                return false;
            }
            m_valueParamCount = static_cast<size_t>(valueCount);
            for (std::int32_t i = 0; i < valueCount; ++i) {
                const std::uint64_t valueStartBit = currentObjectDwgBit(buf);
                if (!skipValueParam(version, buf, sBuf, &hBuff)) {
                    m_prefixStatuses.push_back(makeAssocPrefixStatus(
                        DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
                        valueStartBit, currentObjectDwgBit(buf),
                        DRW_AssociativePrefixStatus::ParseStatus::Partial,
                        m_classVersion, 0, 1, valueCount));
                    appendPrefixStatus(
                        DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
                        startBit,
                        DRW_AssociativePrefixStatus::ParseStatus::Partial,
                        m_classVersion, decodedHandleCount, m_valueParamCount,
                        valueCount);
                    return false;
                }
                m_prefixStatuses.push_back(makeAssocPrefixStatus(
                    DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
                    valueStartBit, currentObjectDwgBit(buf),
                    DRW_AssociativePrefixStatus::ParseStatus::Complete,
                    m_classVersion, 0, 1, valueCount));
            }
            m_valueParamsParsed = true;
        }
        const bool complete = buf->isGood() && hBuff.isGood() && (!sBuf || sBuf->isGood());
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbAssocAction, startBit,
            prefixStatusFromGood(complete), m_classVersion, decodedHandleCount,
            m_valueParamCount, dependencyCount);
        return complete;
    };
    auto readDependencyFields = [&]() {
        const std::uint64_t startBit = currentObjectDwgBit(buf);
        m_classVersion = buf->getBitShort();
        m_status = buf->getBitLong();
        buf->getBit();
        buf->getBit();
        buf->getBit();
        buf->getBit();
        buf->getBitLong();
        m_dependencyHandle = readObjectHandleRef(&hBuff);
        const bool hasName = buf->getBit() != 0;
        if (hasName)
            sBuf->getVariableText(version, false);
        m_readDependencyHandle = readObjectHandleRef(&hBuff);
        m_writeDependencyHandle = readObjectHandleRef(&hBuff);
        readObjectHandleRef(&hBuff);
        buf->getBitLong();
        const bool complete = buf->isGood() && hBuff.isGood() && (!sBuf || sBuf->isGood());
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbAssocDependency, startBit,
            prefixStatusFromGood(complete), m_classVersion, 4, 0, 0);
        return complete;
    };

    // Route the shell body by class-name suffix, mirroring dwgTs
    // readAssociativityBody (which infers behaviour from the class /
    // objectType and defaults unknown subclasses to a raw shell). The
    // exact-name branches below keep the extended-field decoders
    // (network / geom-dependency / aligned-dim action body / vertex &
    // osnap action params / perssubent); every other ACDBASSOC* falls
    // through to a suffix-inferred common prefix and is preserved verbatim
    // by the raw shelf. m_recordName (== the associativity class name) is
    // always set, so the kind stays queryable even for undecoded subclasses.
    const std::string& rn = m_recordName;
    const auto endsWith = [](const std::string& s, const char* suffix) {
        const std::string suf(suffix);
        return s.size() >= suf.size()
            && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
    };

    if (rn == "ACDBASSOCNETWORK") {
        if (readActionFields()) {
            const std::uint64_t networkStartBit = currentObjectDwgBit(buf);
            buf->getBitShort();
            m_actionIndex = buf->getBitLong();
            const std::int32_t actionCount = buf->getBitLong();
            if (!isValidAssocCount(actionCount)) {
                appendPrefixStatus(
                    DRW_AssociativePrefixStatus::Kind::AcDbAssocNetwork,
                    networkStartBit,
                    DRW_AssociativePrefixStatus::ParseStatus::BoundedCountOverflow,
                    m_classVersion, 0, 0, actionCount);
            } else if (readOwnedRefs(m_actions, actionCount)) {
                const std::int32_t ownedActionCount = buf->getBitLong();
                if (!isValidAssocCount(ownedActionCount)) {
                    appendPrefixStatus(
                        DRW_AssociativePrefixStatus::Kind::AcDbAssocNetwork,
                        networkStartBit,
                        DRW_AssociativePrefixStatus::ParseStatus::BoundedCountOverflow,
                        m_classVersion, m_actions.size(), 0,
                        ownedActionCount);
                } else {
                    const bool complete = readHandleVector(
                        m_ownedActions, ownedActionCount);
                    appendPrefixStatus(
                        DRW_AssociativePrefixStatus::Kind::AcDbAssocNetwork,
                        networkStartBit, prefixStatusFromGood(complete),
                        m_classVersion,
                        m_actions.size() + m_ownedActions.size(), 0,
                        actionCount);
                }
            } else {
                appendPrefixStatus(
                    DRW_AssociativePrefixStatus::Kind::AcDbAssocNetwork,
                    networkStartBit,
                    DRW_AssociativePrefixStatus::ParseStatus::Partial,
                    m_classVersion, 0, 0, actionCount);
            }
        }
    } else if (rn == "ACDBASSOCGEOMDEPENDENCY") {
        if (readDependencyFields()) {
            const std::uint64_t geomStartBit = currentObjectDwgBit(buf);
            buf->getBitShort();
            buf->getBit();
            (sBuf ? sBuf : buf)->getVariableText(version, false);
            buf->getBit();
            appendPrefixStatus(
                DRW_AssociativePrefixStatus::Kind::AcDbAssocGeomDependency,
                geomStartBit,
                prefixStatusFromGood(buf->isGood() && (!sBuf || sBuf->isGood())),
                m_classVersion, 0, 0, 0);
        }
    } else if (m_recordName == "ACDBASSOCALIGNEDDIMACTIONBODY") {
        const std::uint64_t startBit = currentObjectDwgBit(buf);
        m_classVersion = buf->getBitShort();
        m_dependencyHandle = readObjectHandleRef(&hBuff);
        buf->getBitLong();
        m_rNodeHandle = readObjectHandleRef(&hBuff);
        m_dNodeHandle = readObjectHandleRef(&hBuff);
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbAssocActionBody, startBit,
            prefixStatusFromGood(buf->isGood() && hBuff.isGood()),
            m_classVersion, 3, 0, 0);
    } else if (m_recordName == "ACDBASSOCVERTEXACTIONPARAM") {
        const std::uint64_t prefixStartBit = currentObjectDwgBit(buf);
        m_actionParamPrefixParsed = skipAssocActionParamPrefix(version, buf, sBuf);
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbAssocActionParam,
            prefixStartBit, prefixStatusFromGood(m_actionParamPrefixParsed),
            m_classVersion, 0, 0, 0);
        if (m_actionParamPrefixParsed
            && skipAssocSingleDependencyActionParam(buf, &hBuff, &m_dependencyHandle)) {
            m_singleDependencyActionParamParsed = true;
            m_classVersion = static_cast<std::uint16_t>(buf->getBitLong());
            m_point = buf->get3BitDouble();
        }
    } else if (m_recordName == "ACDBASSOCOSNAPPOINTREFACTIONPARAM") {
        const std::uint64_t prefixStartBit = currentObjectDwgBit(buf);
        m_actionParamPrefixParsed = skipAssocActionParamPrefix(version, buf, sBuf);
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbAssocActionParam,
            prefixStartBit, prefixStatusFromGood(m_actionParamPrefixParsed),
            m_classVersion, 0, 0, 0);
        if (m_actionParamPrefixParsed
            && skipAssocCompoundActionParam(buf, &hBuff)) {
            m_compoundActionParamParsed = true;
            m_status = buf->getBitShort();
            m_osnapMode = buf->getRawChar8();
            m_parameter = buf->getBitDouble();
        }
    } else if (rn == "ACDBASSOCPERSSUBENTMANAGER"
               || rn == "ACDBPERSSUBENTMANAGER") {
        m_classVersion = static_cast<std::uint16_t>(buf->getBitLong());
        buf->getBitLong();
        buf->getBitLong();
        buf->getBitLong();
        const std::int32_t stepCount = buf->getBitLong();
        if (stepCount >= 0 && stepCount <= 100000) {
            for (std::int32_t i = 0; i < stepCount; ++i)
                buf->getBitLong();
        }
    } else if (endsWith(rn, "ACTIONPARAM")) {
        // Generic action-param subclass (edge / path / face / object /
        // asmbody / ...): decode the shared AcDbAssocActionParam prefix, then
        // stop — the single/compound tail layouts vary by subclass and are
        // preserved by the raw shelf. Mirrors the VERTEX/OSNAP branches but
        // without the variant-specific tail.
        const std::uint64_t prefixStartBit = currentObjectDwgBit(buf);
        m_actionParamPrefixParsed = skipAssocActionParamPrefix(version, buf, sBuf);
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbAssocActionParam,
            prefixStartBit, prefixStatusFromGood(m_actionParamPrefixParsed),
            m_classVersion, 0, 0, 0);
    } else if (endsWith(rn, "DEPENDENCY")) {
        // ACDBASSOCDEPENDENCY + ACDBASSOCVALUEDEPENDENCY share the
        // AcDbAssocDependency body (dwgTs cases 1228 / 1256).
        readDependencyFields();
    } else if (endsWith(rn, "ACTION") || endsWith(rn, "NETWORK")
               || endsWith(rn, "VARIABLE")
               || rn.find("2DCONSTRAINTGROUP") != std::string::npos) {
        // Action-bodied subclass (ACDBASSOCACTION / ACDBASSOCVARIABLE /
        // ACDBASSOC2DCONSTRAINTGROUP / ...): decode the AcDbAssocAction
        // prefix, then stop. The per-subclass extension (workplane, group
        // nodes, variable value, ...) is undocumented / polymorphic and is
        // preserved verbatim by the raw shelf.
        readActionFields();
    } else {
        // Any other ACDBASSOC* (surface / array action bodies, dependency
        // bodies, array parameters, ...): the shared table-entry prefix has
        // already run; record a Missing marker and let the raw shelf preserve
        // the opaque body verbatim (matches dwgTs readAssociativityBody's
        // default -> undefined -> raw-shell path).
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbAssocActionBody,
            currentObjectDwgBit(buf),
            DRW_AssociativePrefixStatus::ParseStatus::Missing,
            m_classVersion, 0, 0, 0);
    }

    DRW_DBG("ACDBASSOC shell: "); DRW_DBG(m_recordName.c_str()); DRW_DBG("\n");
    return ret;
}

bool DRW_AcShHistoryObject::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing ACSH shell ************************\n");
    if (!ret)
        return ret;

    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);

    auto appendPrefixStatus = [&](DRW_AssociativePrefixStatus::Kind kind,
                                  std::uint64_t startBit,
                                  DRW_AssociativePrefixStatus::ParseStatus status,
                                  std::uint16_t classVersion,
                                  size_t decodedHandleCount = 0,
                                  size_t decodedValueCount = 0,
                                  std::int32_t decodedCountValue = 0) {
        m_prefixStatuses.push_back(makeAssocPrefixStatus(
            kind, startBit, currentObjectDwgBit(buf), status, classVersion,
            decodedHandleCount, decodedValueCount, decodedCountValue));
    };
    if (m_recordName == "ACSH_HISTORY_CLASS") {
        const std::uint64_t startBit = currentObjectDwgBit(buf);
        m_major = static_cast<std::uint32_t>(buf->getBitLong());
        m_minor = static_cast<std::uint32_t>(buf->getBitLong());
        m_ownerHandle = readObjectHandleRef(&hBuff);
        m_historyNodeId = static_cast<std::uint32_t>(buf->getBitLong());
        m_showHistory = buf->getBit() != 0;
        m_recordHistory = buf->getBit() != 0;
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode, startBit,
            prefixStatusFromGood(buf->isGood() && hBuff.isGood()),
            static_cast<std::uint16_t>(m_major), 1, 0, 0);
    } else if (m_recordName == "ACSH_SWEEP_CLASS"
               || m_recordName == "ACSH_EXTRUSION_CLASS") {
        // ACSH_EXTRUSION_CLASS (AcDbShExtrusion) shares AcDbShSweepBase with
        // ACSH_SWEEP_CLASS (dwg2.spec:4257 vs 4210 — only the trailing SUBCLASS
        // marker differs), so it decodes through the same arm. The reliable
        // fields are major/minor/direction; the tail past `direction` is an
        // undocumented embedded sweep/path entity blob + SweepOptions and is
        // best-effort only (matches dwgTs parseAcshSweepBaseBody).
        const std::uint64_t evalStartBit = currentObjectDwgBit(buf);
        const bool evalParsed = skipEvalExpr(version, buf, sBuf, &hBuff);
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbEvalExpr, evalStartBit,
            prefixStatusFromGood(evalParsed), 0, 1, 1, 0);
        const std::uint64_t historyStartBit = currentObjectDwgBit(buf);
        const bool historyParsed = evalParsed
            && skipShHistoryNode(version, buf, sBuf, &hBuff);
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode,
            historyStartBit, prefixStatusFromGood(historyParsed), 0, 1, 0, 0);
        if (evalParsed && historyParsed) {
            const std::uint64_t actionBodyStartBit = currentObjectDwgBit(buf);
            m_major = static_cast<std::uint32_t>(buf->getBitLong());
            m_minor = static_cast<std::uint32_t>(buf->getBitLong());
            m_direction = buf->get3BitDouble();
            buf->getBitLong();
            const std::int32_t blob1Size = buf->getBitLong();
            bool sweepTailOk = blob1Size >= 0
                && readBinaryBytes(buf, m_binaryBlob1, static_cast<std::uint32_t>(blob1Size), kMaxAcShBlobBytes);
            if (sweepTailOk) {
                buf->getBitLong();
                const std::int32_t blob2Size = buf->getBitLong();
                sweepTailOk = blob2Size >= 0
                    && readBinaryBytes(buf, m_binaryBlob2, static_cast<std::uint32_t>(blob2Size), kMaxAcShBlobBytes);
            }
            if (sweepTailOk) {
                m_draftAngle = buf->getBitDouble();
                m_startDraftDistance = buf->getBitDouble();
                m_endDraftDistance = buf->getBitDouble();
                m_scaleFactor = buf->getBitDouble();
                m_twistAngle = buf->getBitDouble();
                m_alignAngle = buf->getBitDouble();
            }
            appendPrefixStatus(
                DRW_AssociativePrefixStatus::Kind::AcShActionBody,
                actionBodyStartBit,
                blob1Size < 0 ?
                    DRW_AssociativePrefixStatus::ParseStatus::BoundedCountOverflow :
                    prefixStatusFromGood(sweepTailOk),
                static_cast<std::uint16_t>(m_major), 0,
                m_binaryBlob1.size() + m_binaryBlob2.size(), blob1Size);
        }
    } else if (m_recordName == "ACSH_BOX_CLASS"
               || m_recordName == "ACSH_WEDGE_CLASS"
               || m_recordName == "ACSH_SPHERE_CLASS"
               || m_recordName == "ACSH_CYLINDER_CLASS"
               || m_recordName == "ACSH_CONE_CLASS") {
        // Primitive-shape body = AcDbEvalExpr_fields + AcDbShHistoryNode_fields
        // + major BL + minor BL + per-class dims (BD). Reuses the shared prefix
        // skips from the SWEEP arm; matches dwgTs parseAcshBoxOrWedgeBody /
        // parseAcshSphereClass / parseAcshCylinderOrConeBody.
        const std::uint64_t evalStartBit = currentObjectDwgBit(buf);
        const bool evalParsed = skipEvalExpr(version, buf, sBuf, &hBuff);
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbEvalExpr, evalStartBit,
            prefixStatusFromGood(evalParsed), 0, 1, 1, 0);
        const std::uint64_t historyStartBit = currentObjectDwgBit(buf);
        const bool historyParsed = evalParsed
            && skipShHistoryNode(version, buf, sBuf, &hBuff);
        appendPrefixStatus(
            DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode,
            historyStartBit, prefixStatusFromGood(historyParsed), 0, 1, 0, 0);
        if (evalParsed && historyParsed && buf->isGood()) {
            const std::uint64_t shapeStartBit = currentObjectDwgBit(buf);
            m_major = static_cast<std::uint32_t>(buf->getBitLong());
            m_minor = static_cast<std::uint32_t>(buf->getBitLong());
            int dimCount = 3;                                   // box / wedge
            if (m_recordName == "ACSH_SPHERE_CLASS")
                dimCount = 1;                                   // radius
            else if (m_recordName == "ACSH_CYLINDER_CLASS"
                     || m_recordName == "ACSH_CONE_CLASS")
                dimCount = 4;   // height, majorRadius, minorRadius, xRadius
            m_shapeParams.clear();
            m_shapeParams.reserve(static_cast<size_t>(dimCount));
            for (int i = 0; i < dimCount && buf->isGood(); ++i)
                m_shapeParams.push_back(buf->getBitDouble());
            const bool shapeOk = buf->isGood()
                && static_cast<int>(m_shapeParams.size()) == dimCount;
            appendPrefixStatus(
                DRW_AssociativePrefixStatus::Kind::AcShActionBody,
                shapeStartBit, prefixStatusFromGood(shapeOk),
                static_cast<std::uint16_t>(m_major), 0,
                m_shapeParams.size(), dimCount);
        }
    } else if (m_recordName == "ACSH_BOOLEAN_CLASS"
               || m_recordName == "ACSH_CHAMFER_CLASS"
               || m_recordName == "ACSH_FILLET_CLASS"
               || m_recordName == "ACSH_TORUS_CLASS"
               || m_recordName == "ACSH_REVOLVE_CLASS"
               || m_recordName == "ACSH_LOFT_CLASS") {
        // ACSH shape bodies whose fields are scalar and oracle-validatable
        // (dwgread -O JSON). All share the AcDbEvalExpr_fields +
        // AcDbShHistoryNode_fields prefix, then per-class dims. Matches dwgTs
        // parseAcshBooleanClass / parseAcshChamferClass / parseAcshFilletClass /
        // parseAcshTorusClass / parseAcshRevolveClass / parseAcshLoftClass.
        // (ACSH_BREP_CLASS is deliberately excluded: its body past the prefix is
        //  an ACIS solid payload, not scalar fields — dwg2.spec even flags
        //  "major // also in DWG?" — so it stays prefix-only + raw-shelved.)
        auto parseSharedPrefix = [&]() -> bool {
            const std::uint64_t evalStartBit = currentObjectDwgBit(buf);
            const bool evalParsed = skipEvalExpr(version, buf, sBuf, &hBuff);
            appendPrefixStatus(
                DRW_AssociativePrefixStatus::Kind::AcDbEvalExpr, evalStartBit,
                prefixStatusFromGood(evalParsed), 0, 1, 1, 0);
            const std::uint64_t historyStartBit = currentObjectDwgBit(buf);
            const bool historyParsed = evalParsed
                && skipShHistoryNode(version, buf, sBuf, &hBuff);
            appendPrefixStatus(
                DRW_AssociativePrefixStatus::Kind::AcDbShHistoryNode,
                historyStartBit, prefixStatusFromGood(historyParsed), 0, 1, 0, 0);
            return evalParsed && historyParsed && buf->isGood();
        };
        // Bounded vector<double> reader (BD elements), graceful on short read.
        auto readBoundedDoubles = [&](std::int32_t count,
                                      std::vector<double>& out) {
            out.clear();
            if (!isValidAssocCount(count))
                return;
            out.reserve(static_cast<size_t>(count));
            for (std::int32_t i = 0; i < count && buf->isGood(); ++i)
                out.push_back(buf->getBitDouble());
        };

        if (parseSharedPrefix()) {
            const std::uint64_t bodyStartBit = currentObjectDwgBit(buf);
            m_major = static_cast<std::uint32_t>(buf->getBitLong());
            m_minor = static_cast<std::uint32_t>(buf->getBitLong());

            if (m_recordName == "ACSH_BOOLEAN_CLASS") {
                m_operation = static_cast<std::int32_t>(
                    static_cast<std::int8_t>(buf->getRawChar8()));  // RCd
                m_operand1 = static_cast<std::uint32_t>(buf->getBitLong());
                m_operand2 = static_cast<std::uint32_t>(buf->getBitLong());
            } else if (m_recordName == "ACSH_CHAMFER_CLASS") {
                m_bl92 = static_cast<std::uint32_t>(buf->getBitLong());
                m_baseDist = buf->getBitDouble();
                m_otherDist = buf->getBitDouble();
                const std::int32_t numEdges = buf->getBitLong();
                m_edges.clear();
                if (isValidAssocCount(numEdges)) {
                    m_edges.reserve(static_cast<size_t>(numEdges));
                    for (std::int32_t i = 0; i < numEdges && buf->isGood(); ++i)
                        m_edges.push_back(static_cast<std::uint32_t>(buf->getBitLong()));
                    m_bl95 = static_cast<std::uint32_t>(buf->getBitLong());
                }
            } else if (m_recordName == "ACSH_FILLET_CLASS") {
                m_bl92 = static_cast<std::uint32_t>(buf->getBitLong());
                const std::int32_t numEdges = buf->getBitLong();
                m_edges.clear();
                if (isValidAssocCount(numEdges)) {
                    m_edges.reserve(static_cast<size_t>(numEdges));
                    for (std::int32_t i = 0; i < numEdges && buf->isGood(); ++i)
                        m_edges.push_back(static_cast<std::uint32_t>(buf->getBitLong()));
                    const std::int32_t numRadiuses = buf->getBitLong();
                    readBoundedDoubles(numRadiuses, m_radiuses);
                    const std::int32_t numStartsetbacks = buf->getBitLong();
                    const std::int32_t numEndsetbacks = buf->getBitLong();
                    // spec order: endsetbacks[num_endsetbacks] then
                    // startsetbacks[num_startsetbacks].
                    readBoundedDoubles(numEndsetbacks, m_endSetbacks);
                    readBoundedDoubles(numStartsetbacks, m_startSetbacks);
                }
            } else if (m_recordName == "ACSH_TORUS_CLASS") {
                m_shapeParams.clear();
                m_shapeParams.push_back(buf->getBitDouble());  // major_radius
                m_shapeParams.push_back(buf->getBitDouble());  // minor_radius
            } else if (m_recordName == "ACSH_REVOLVE_CLASS") {
                m_axisPoint = buf->get3BitDouble();
                m_revolveDirection = buf->get2RawDouble();      // 2RD (finite-but-wrong if degenerate)
                m_revolveAngle = buf->getBitDouble();
                m_startAngle = buf->getBitDouble();
                m_draftAngle = buf->getBitDouble();
                m_bd44 = buf->getBitDouble();
                m_bd45 = buf->getBitDouble();
                m_twistAngle = buf->getBitDouble();
                m_b290 = buf->getBit() != 0;
                m_isCloseToAxis = buf->getBit() != 0;
            } else if (m_recordName == "ACSH_LOFT_CLASS") {
                const std::int32_t numCross = buf->getBitLong();
                const std::int32_t numGuides = buf->getBitLong();
                // crosssects[] / guides[] are CALL_SUBENT decode NOPs — the
                // per-element handles are not in the data stream (dwgTs).
                m_numCrossSections = isValidAssocCount(numCross)
                    ? static_cast<std::uint32_t>(numCross) : 0;
                m_numGuides = isValidAssocCount(numGuides)
                    ? static_cast<std::uint32_t>(numGuides) : 0;
            }

            appendPrefixStatus(
                DRW_AssociativePrefixStatus::Kind::AcShActionBody, bodyStartBit,
                prefixStatusFromGood(buf->isGood()),
                static_cast<std::uint16_t>(m_major), 0, 0, 0);
        }
    }

    DRW_UNUSED(sBuf);
    DRW_DBG("ACSH shell: "); DRW_DBG(m_recordName.c_str()); DRW_DBG("\n");
    return ret;
}

namespace {

bool dynBlockContains(const std::string& s, const char* sub) {
    return s.find(sub) != std::string::npos;
}

bool dynBlockEndsWith(const std::string& s, const char* suf) {
    const std::size_t n = std::char_traits<char>::length(suf);
    return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
}

} // namespace

// True for every dynamic-block custom-class recName.  The dynamic-block classes
// all carry a PARAMETER / ACTION / GRIP suffix (or COMPONENT), or are one of the
// named singletons; the structural BLOCK_HEADER / BLOCK_CONTROL / BLOCK_RECORD
// table objects (which never reach the custom-class dispatch anyway) are rejected
// explicitly.
bool DRW_DynamicBlockObject::isDynamicBlockRecName(const UTF8STRING& rn) {
    if (rn.empty() || rn == "BLOCK" || rn == "BLOCK_HEADER"
        || rn == "BLOCK_CONTROL" || rn == "BLOCK_RECORD")
        return false;
    if (!dynBlockContains(rn, "BLOCK"))
        return false;
    return dynBlockEndsWith(rn, "PARAMETER")
        || dynBlockEndsWith(rn, "ACTION")
        || dynBlockEndsWith(rn, "GRIP")
        || dynBlockEndsWith(rn, "COMPONENT")
        || dynBlockContains(rn, "GRIPLOCATIONCOMPONENT")
        || dynBlockContains(rn, "PROXYNODE")
        || dynBlockContains(rn, "PURGEPREVENTER")
        || dynBlockContains(rn, "PARAMDEPENDENCYBODY")
        || dynBlockContains(rn, "PROPERTIESTABLE")
        || dynBlockContains(rn, "REPRESENTATION");
}

// Map a dynamic-block recName to how much of its body layout is reliably known.
DRW_DynamicBlockObject::Kind DRW_DynamicBlockObject::classify(const UTF8STRING& rn) {
    // Bare first: HANDLE_UNKNOWN_BITS classes (leading unknown bits would
    // misalign any body walk) and the non-evalexpr singletons -> shell only.
    if (dynBlockContains(rn, "PURGEPREVENTER")
        || dynBlockContains(rn, "PARAMDEPENDENCYBODY")
        || dynBlockContains(rn, "REPRESENTATION")
        || dynBlockContains(rn, "USERPARAMETER")           // AcDbBlockUserParameter
        || dynBlockContains(rn, "PROPERTIESTABLEGRIP")     // HANDLE_UNKNOWN_BITS
        || rn == "BLOCKPROPERTIESTABLE")
        return Kind::Bare;
    // Full-decode validation classes (both oracles / dwgread agree on the body).
    if (rn == "BLOCKMOVEACTION")
        return Kind::MoveAction;
    if (rn == "BLOCKVISIBILITYPARAMETER")
        return Kind::VisibilityParameter;
    // Evalexpr-only.
    if (dynBlockContains(rn, "PROXYNODE"))
        return Kind::EvalExprOnly;
    if (dynBlockContains(rn, "GRIPLOCATIONCOMPONENT")
        || dynBlockEndsWith(rn, "COMPONENT"))
        return Kind::GripLocationComponent;
    // Element-based families (evalexpr + AcDbBlockElement prefix).
    if (dynBlockEndsWith(rn, "GRIP"))
        return Kind::Grip;
    if (dynBlockEndsWith(rn, "ACTION"))
        return Kind::Action;
    if (dynBlockEndsWith(rn, "PARAMETER"))
        return Kind::Parameter;
    return Kind::EvalExprOnly;
}

// Dynamic-block object body decode.  Layout per libreDWG dwg2.spec
// (AcDbEvalExpr_fields / AcDbBlockElement_fields / AcDbBlockParameter_fields /
// AcDbBlockAction_fields / BLOCKVISIBILITYPARAMETER / BLOCKMOVEACTION),
// ground-truthed against dwgread -O JSON.  Handle-reference vectors (deps /
// blocks / params) live in the handle stream and are left deferred (their COUNT
// is read inline in the data stream; the handles stay raw-shelved).  Every read
// is isGood()-guarded, every count is bounded, and the parse ALWAYS returns true
// (graceful degrade) — the caller keeps the raw shelf for a lossless round-trip.
bool DRW_DynamicBlockObject::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;   // 2007+ string stream
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing DynamicBlock shell ****************\n");
    if (!ret)
        return ret;

    // Common + object-specific handles live in the handle stream; read a copy so
    // the data-stream walk is unaffected.  This also positions hBuff for the rare
    // evalexpr value_code==91 handle case.  Seek hBuff to objSize explicitly for
    // BOTH versions (not seekObjectHandleStream, which is a no-op for <=AC1018):
    // the body here is decoded only partially (Bare early-return, deferred handle
    // vectors), so buf never reaches the handle stream on its own, and the no-op
    // seek would leave hBuff at body-start -> garbage owner/reactor handles on
    // R2000/R2004. objSize is the handle-stream bit offset at every version
    // (same technique as the ACAD_TABLE <=AC1018 fix).
    dwgBuffer hBuff = *buf;
    hBuff.setPosition(objSize >> 3);
    hBuff.setBitPos(objSize & 7);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);

    // Bare classes: shell + handle only (leading unknown bits would misalign a
    // body walk); the raw shelf preserves the full byte image regardless.
    if (m_kind == Kind::Bare)
        return true;

    // ---- AcDbEvalExpr prefix (every non-Bare class) --------------------------
    m_parentId  = static_cast<std::uint32_t>(buf->getBitLong());   // BLd parentid
    m_major     = static_cast<std::uint32_t>(buf->getBitLong());   // BL major (98)
    m_minor     = static_cast<std::uint32_t>(buf->getBitLong());   // BL minor (99)
    m_valueCode = buf->getSBitShort();                             // BSd value_code (70)
    switch (m_valueCode) {                                         // value-code switch
    case 40: buf->getBitDouble(); break;
    case 10:
    case 11: buf->get2RawDouble(); break;
    case 1:  sBuf->getVariableText(version, false); break;
    case 90: buf->getBitLong(); break;
    case 91: readObjectHandleRef(&hBuff); break;   // handle stream, no data bits
    case 70: buf->getBitShort(); break;
    default: break;
    }
    m_nodeId = static_cast<std::uint32_t>(buf->getBitLong());      // BL nodeid
    m_evalExprParsed = buf->isGood();
    if (!m_evalExprParsed)
        return true;

    if (m_kind == Kind::EvalExprOnly)
        return true;

    if (m_kind == Kind::GripLocationComponent) {
        // AcDbBlockGripExpr: grip_type BL(91) + grip_expr T(300).
        m_gripType = buf->getBitLong();
        m_gripExpr = sBuf->getVariableText(version, false);
        return true;
    }

    // ---- AcDbBlockElement prefix (grip / parameter / action classes) ---------
    m_elementName  = sBuf->getVariableText(version, false);        // T name (300)
    m_elementMajor = static_cast<std::uint32_t>(buf->getBitLong()); // BL be_major (98)
    m_elementMinor = static_cast<std::uint32_t>(buf->getBitLong()); // BL be_minor (99)
    m_eed1071      = buf->getBitLong();                            // BL eed1071 (1071)
    m_elementParsed = buf->isGood();
    if (!m_elementParsed)
        return true;

    // Grip: element prefix only; deeper AcDbBlockGrip body left raw-shelved.
    if (m_kind == Kind::Grip)
        return true;

    // AcDbBlockAction classes extend AcDbBlockElement DIRECTLY (no
    // AcDbBlockParameter bits) — display_location follows element immediately.
    if (m_kind == Kind::Action || m_kind == Kind::MoveAction) {
        // Prefix-only for a plain action; the full body is decoded only for the
        // BLOCKMOVEACTION validation class.
        if (m_kind == Kind::Action)
            return true;
        // AcDbBlockAction (decoder/else branch): display_location 3BD, num_deps
        // BL, deps handles (deferred), num_actions BL, actions[] BL.
        m_displayLocation = buf->get3BitDouble();
        m_dependencyCount = buf->getBitLong();                    // BL num_deps (71)
        if (!isValidAssocCount(m_dependencyCount))
            return true;
        // deps[] handles are deferred (handle stream).
        m_actionCount = buf->getBitLong();                        // BL num_actions (70)
        if (!isValidAssocCount(m_actionCount))
            return true;
        m_actionIndexes.clear();
        m_actionIndexes.reserve(static_cast<std::size_t>(m_actionCount));
        for (std::int32_t a = 0; a < m_actionCount && buf->isGood(); ++a)
            m_actionIndexes.push_back(buf->getBitLong());         // BL action index (91)
        // AcDbBlockMoveAction: 2 connection points {code BL, name T}.
        m_connectionNames.clear();
        for (int cp = 0; cp < 2 && buf->isGood(); ++cp) {
            buf->getBitLong();                                    // connection code BL
            m_connectionNames.push_back(
                sBuf->getVariableText(version, false));           // connection name T
        }
        // AcDbBlockAction_doubles: action_offset_x/y BD + angle_offset BD.
        m_actionOffsetX = buf->getBitDouble();                    // BD (140)
        m_actionOffsetY = buf->getBitDouble();                    // BD (141)
        m_angleOffset   = buf->getBitDouble();                    // BD
        m_bodyFullyDecoded = buf->isGood();
        return true;
    }

    // AcDbBlockParameter: show_properties B(280) + chain_actions B(281).
    // (Parameter / VisibilityParameter both extend AcDbBlockParameter.)
    m_showProperties = buf->getBit() != 0;
    m_chainActions   = buf->getBit() != 0;
    if (!buf->isGood())
        return true;

    if (m_kind == Kind::Parameter)
        return true;

    if (m_kind == Kind::VisibilityParameter) {
        // AcDbBlock1PtParameter: def_pt 3BD + prop1/prop2 PropInfo + num_propinfos.
        m_defPoint = buf->get3BitDouble();
        for (int p = 0; p < 2 && buf->isGood(); ++p) {
            const std::int32_t numConn = buf->getBitLong();       // BL num_connections
            if (!isValidAssocCount(numConn))
                return true;
            for (std::int32_t c = 0; c < numConn && buf->isGood(); ++c) {
                buf->getBitLong();                                // connection code BL
                sBuf->getVariableText(version, false);            // connection name T
            }
        }
        buf->getBitLong();                                        // num_propinfos BL
        // AcDbBlockVisibilityParameter body.
        m_isInitialized = buf->getBit() != 0;                     // B is_initialized (281)
        m_visibilityName = sBuf->getVariableText(version, false); // T blockvisi_name (301)
        m_visibilityDescription =
            sBuf->getVariableText(version, false);                // T blockvisi_desc (302)
        m_unknownBool = buf->getBit() != 0;                       // B (91)
        m_blockCount = buf->getBitLong();                         // BL num_blocks (93)
        if (!isValidAssocCount(m_blockCount))
            return true;
        // blocks[] handles are deferred (handle stream) — count only here.
        m_stateCount = buf->getBitLong();                         // BL num_states (92)
        if (!isValidAssocCount(m_stateCount))
            return true;
        m_stateNames.clear();
        m_stateNames.reserve(static_cast<std::size_t>(m_stateCount));
        for (std::int32_t s = 0; s < m_stateCount && buf->isGood(); ++s) {
            m_stateNames.push_back(sBuf->getVariableText(version, false)); // T name (303)
            const std::int32_t stateBlocks = buf->getBitLong();  // BL num_blocks (94)
            if (!isValidAssocCount(stateBlocks))
                return true;
            const std::int32_t stateParams = buf->getBitLong();  // BL num_params (95)
            if (!isValidAssocCount(stateParams))
                return true;
            // per-state blocks[]/params[] handles are deferred (handle stream).
        }
        m_bodyFullyDecoded = buf->isGood();
        return true;
    }

    return true;
}

void DRW_DetailViewStyle::reset() {
    tType = DRW::DETAILVIEWSTYLE;
    m_modelDoc = DRW_ModelDocViewStyle();
    m_classVersion = 0;
    m_flags = 0;
    m_identifierStyleHandle = 0;
    m_identifierColor = 256;
    m_identifierHeight = 0.0;
    m_identifierExcludeCharacters.clear();
    m_identifierOffset = 0.0;
    m_identifierPlacement = 0;
    m_arrowSymbolHandle = 0;
    m_arrowSymbolColor = 256;
    m_arrowSymbolSize = 0.0;
    m_boundaryLineTypeHandle = 0;
    m_boundaryLineWeight = 0;
    m_boundaryLineColor = 256;
    m_viewLabelTextStyleHandle = 0;
    m_viewLabelTextColor = 256;
    m_viewLabelTextHeight = 0.0;
    m_viewLabelAttachment = 0;
    m_viewLabelOffset = 0.0;
    m_viewLabelAlignment = 0;
    m_viewLabelPattern.clear();
    m_connectionLineTypeHandle = 0;
    m_connectionLineWeight = 0;
    m_connectionLineColor = 256;
    m_borderLineTypeHandle = 0;
    m_borderLineWeight = 0;
    m_borderLineColor = 256;
    m_modelEdge = 0;
    m_dxfSubclass.clear();
    m_dxfGroup = -1;
    m_dxfHandleCount = 0;
    m_dxfColorCount = 0;
    m_dxfDoubleCount = 0;
    m_dxfLongCount = 0;
    DRW_TableEntry::reset();
}

bool DRW_DetailViewStyle::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    if (code == 100) {
        m_dxfSubclass = reader->getUtf8String();
        m_dxfGroup = -1;
        m_dxfHandleCount = m_dxfColorCount = m_dxfDoubleCount = m_dxfLongCount = 0;
        return true;
    }
    if (m_dxfSubclass == "AcDbModelDocViewStyle") {
        switch (code) {
        case 70: m_modelDoc.m_modelDocClassVersion = reader->getInt32(); return true;
        case 3:  m_modelDoc.m_description = reader->getUtf8String(); return true;
        case 290: m_modelDoc.m_modifiedForRecompute = reader->getBool(); return true;
        case 300: m_modelDoc.m_displayName = reader->getUtf8String(); return true;
        case 90: m_modelDoc.m_viewStyleFlags = static_cast<std::uint32_t>(reader->getInt32()); return true;
        default: return DRW_TableEntry::parseCode(code, reader);
        }
    }
    if (m_dxfSubclass != "AcDbDetailViewStyle")
        return DRW_TableEntry::parseCode(code, reader);

    if (code == 71) {
        m_dxfGroup = reader->getInt32();
        m_dxfHandleCount = m_dxfColorCount = m_dxfDoubleCount = m_dxfLongCount = 0;
        return true;
    }
    switch (code) {
    case 70:
        m_classVersion = reader->getInt32();
        return true;
    case 340: {
        const std::uint32_t ref = static_cast<std::uint32_t>(reader->getHandleString());
        if (m_dxfGroup == 1) {
            if (m_dxfHandleCount++ == 0) m_identifierStyleHandle = ref;
            else m_arrowSymbolHandle = ref;
        } else if (m_dxfGroup == 2) {
            m_boundaryLineTypeHandle = ref;
        } else if (m_dxfGroup == 3) {
            m_viewLabelTextStyleHandle = ref;
        } else if (m_dxfGroup == 4) {
            if (m_dxfHandleCount++ == 0) m_connectionLineTypeHandle = ref;
            else m_borderLineTypeHandle = ref;
        }
        return true;
    }
    case 62: {
        const int color = reader->getInt32();
        if (m_dxfGroup == 1) {
            if (m_dxfColorCount++ == 0) m_identifierColor = color;
            else m_arrowSymbolColor = color;
        } else if (m_dxfGroup == 2) {
            m_boundaryLineColor = color;
        } else if (m_dxfGroup == 3) {
            m_viewLabelTextColor = color;
        } else if (m_dxfGroup == 4) {
            if (m_dxfColorCount++ == 0) m_connectionLineColor = color;
            else m_borderLineColor = color;
        }
        return true;
    }
    case 40: {
        const double value = reader->getDouble();
        if (m_dxfGroup == 1) {
            if (m_dxfDoubleCount == 0) m_identifierHeight = value;
            else if (m_dxfDoubleCount == 1) m_arrowSymbolSize = value;
            else m_identifierOffset = value;
            ++m_dxfDoubleCount;
        } else if (m_dxfGroup == 3) {
            if (m_dxfDoubleCount++ == 0) m_viewLabelTextHeight = value;
            else m_viewLabelOffset = value;
        }
        return true;
    }
    case 90: {
        const std::int32_t value = reader->getInt32();
        if (m_dxfGroup == 0 || m_dxfGroup == -1) m_flags = static_cast<std::uint32_t>(value);
        else if (m_dxfGroup == 2) m_boundaryLineWeight = value;
        else if (m_dxfGroup == 3) {
            if (m_dxfLongCount++ == 0) m_viewLabelAttachment = static_cast<std::uint32_t>(value);
            else m_viewLabelAlignment = static_cast<std::uint32_t>(value);
        } else if (m_dxfGroup == 4) {
            if (m_dxfLongCount++ == 0) m_connectionLineWeight = value;
            else m_borderLineWeight = value;
        }
        return true;
    }
    case 280:
        if (m_dxfGroup == 1) m_identifierPlacement = static_cast<std::uint8_t>(reader->getInt32());
        else if (m_dxfGroup == 4) m_modelEdge = static_cast<std::uint8_t>(reader->getInt32());
        else reader->getInt32();
        return true;
    case 300:
        if (m_dxfGroup == 1) m_identifierExcludeCharacters = reader->getUtf8String();
        else if (m_dxfGroup == 3) m_viewLabelPattern = reader->getUtf8String();
        else reader->getUtf8String();
        return true;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
}

bool DRW_DetailViewStyle::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing DETAILVIEWSTYLE ******************\n");
    if (!ret) return ret;

    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);

    m_modelDoc.m_modelDocClassVersion = buf->getBitShort();
    m_modelDoc.m_description = sBuf->getVariableText(version, false);
    m_modelDoc.m_modifiedForRecompute = buf->getBit() != 0;
    if (version >= DRW::AC1032) {
        m_modelDoc.m_displayName = sBuf->getVariableText(version, false);
        m_modelDoc.m_viewStyleFlags = static_cast<std::uint32_t>(buf->getBitLong());
    }

    m_classVersion = buf->getBitShort();
    m_flags = static_cast<std::uint32_t>(buf->getBitLong());
    m_identifierStyleHandle = readObjectHandleRef(&hBuff);
    m_identifierColor = readObjectCmColor(version, buf, sBuf);
    m_identifierHeight = buf->getBitDouble();
    m_identifierExcludeCharacters = sBuf->getVariableText(version, false);
    m_identifierOffset = buf->getBitDouble();
    m_identifierPlacement = buf->getRawChar8();
    m_arrowSymbolHandle = readObjectHandleRef(&hBuff);
    m_arrowSymbolColor = readObjectCmColor(version, buf, sBuf);
    m_arrowSymbolSize = buf->getBitDouble();
    m_boundaryLineTypeHandle = readObjectHandleRef(&hBuff);
    m_boundaryLineWeight = buf->getBitLong();
    m_boundaryLineColor = readObjectCmColor(version, buf, sBuf);
    m_viewLabelTextStyleHandle = readObjectHandleRef(&hBuff);
    m_viewLabelTextColor = readObjectCmColor(version, buf, sBuf);
    m_viewLabelTextHeight = buf->getBitDouble();
    m_viewLabelAttachment = static_cast<std::uint32_t>(buf->getBitLong());
    m_viewLabelOffset = buf->getBitDouble();
    m_viewLabelAlignment = static_cast<std::uint32_t>(buf->getBitLong());
    m_viewLabelPattern = sBuf->getVariableText(version, false);
    m_connectionLineTypeHandle = readObjectHandleRef(&hBuff);
    m_connectionLineWeight = buf->getBitLong();
    m_connectionLineColor = readObjectCmColor(version, buf, sBuf);
    m_borderLineTypeHandle = readObjectHandleRef(&hBuff);
    m_borderLineWeight = buf->getBitLong();
    m_borderLineColor = readObjectCmColor(version, buf, sBuf);
    m_modelEdge = buf->getRawChar8();
    return buf->isGood() && sBuf->isGood() && hBuff.isGood();
}

void DRW_SectionViewStyle::reset() {
    tType = DRW::SECTIONVIEWSTYLE;
    m_modelDoc = DRW_ModelDocViewStyle();
    m_classVersion = 0;
    m_flags = 0;
    m_identifierStyleHandle = 0;
    m_identifierColor = 256;
    m_identifierHeight = 0.0;
    m_arrowStartSymbolHandle = 0;
    m_arrowEndSymbolHandle = 0;
    m_arrowSymbolColor = 256;
    m_arrowSymbolSize = 0.0;
    m_identifierExcludeCharacters.clear();
    m_identifierPosition = 0;
    m_identifierOffset = 0.0;
    m_arrowPosition = 0;
    m_arrowSymbolExtensionLength = 0.0;
    m_planeLineTypeHandle = 0;
    m_planeLineWeight = 0;
    m_planeLineColor = 256;
    m_bendLineTypeHandle = 0;
    m_bendLineWeight = 0;
    m_bendLineColor = 256;
    m_bendLineLength = 0.0;
    m_endLineOvershoot = 0.0;
    m_endLineLength = 0.0;
    m_viewLabelTextStyleHandle = 0;
    m_viewLabelTextColor = 256;
    m_viewLabelTextHeight = 0.0;
    m_viewLabelAttachment = 0;
    m_viewLabelOffset = 0.0;
    m_viewLabelAlignment = 0;
    m_viewLabelPattern.clear();
    m_hatchColor = 256;
    m_hatchBackgroundColor = 257;
    m_hatchPattern.clear();
    m_hatchScale = 1.0;
    m_hatchTransparency = 0;
    m_unknownB1 = false;
    m_unknownB2 = false;
    m_hatchAngles.clear();
    m_dxfSubclass.clear();
    m_dxfGroup = -1;
    m_dxfHandleCount = m_dxfColorCount = m_dxfDoubleCount = m_dxfLongCount = 0;
    m_dxfBoolCount = 0;
    m_dxfExpectedHatchAngles = 0;
    DRW_TableEntry::reset();
}

bool DRW_SectionViewStyle::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    if (code == 100) {
        m_dxfSubclass = reader->getUtf8String();
        m_dxfGroup = -1;
        m_dxfHandleCount = m_dxfColorCount = m_dxfDoubleCount = m_dxfLongCount = 0;
        m_dxfBoolCount = 0;
        return true;
    }
    if (m_dxfSubclass == "AcDbModelDocViewStyle") {
        switch (code) {
        case 70: m_modelDoc.m_modelDocClassVersion = reader->getInt32(); return true;
        case 3:  m_modelDoc.m_description = reader->getUtf8String(); return true;
        case 290: m_modelDoc.m_modifiedForRecompute = reader->getBool(); return true;
        case 300: m_modelDoc.m_displayName = reader->getUtf8String(); return true;
        case 90: m_modelDoc.m_viewStyleFlags = static_cast<std::uint32_t>(reader->getInt32()); return true;
        default: return DRW_TableEntry::parseCode(code, reader);
        }
    }
    if (m_dxfSubclass != "AcDbSectionViewStyle")
        return DRW_TableEntry::parseCode(code, reader);

    if (code == 71) {
        m_dxfGroup = reader->getInt32();
        m_dxfHandleCount = m_dxfColorCount = m_dxfDoubleCount = m_dxfLongCount = 0;
        m_dxfBoolCount = 0;
        return true;
    }
    switch (code) {
    case 70:
        m_classVersion = reader->getInt32();
        return true;
    case 340: {
        const std::uint32_t ref = static_cast<std::uint32_t>(reader->getHandleString());
        if (m_dxfGroup == 1) {
            if (m_dxfHandleCount == 0) m_identifierStyleHandle = ref;
            else if (m_dxfHandleCount == 1) m_arrowStartSymbolHandle = ref;
            else m_arrowEndSymbolHandle = ref;
            ++m_dxfHandleCount;
        } else if (m_dxfGroup == 2) {
            if (m_dxfHandleCount++ == 0) m_planeLineTypeHandle = ref;
            else m_bendLineTypeHandle = ref;
        } else if (m_dxfGroup == 3) {
            m_viewLabelTextStyleHandle = ref;
        }
        return true;
    }
    case 62: {
        const int color = reader->getInt32();
        if (m_dxfGroup == 1) {
            if (m_dxfColorCount++ == 0) m_identifierColor = color;
            else m_arrowSymbolColor = color;
        } else if (m_dxfGroup == 2) {
            if (m_dxfColorCount++ == 0) m_planeLineColor = color;
            else m_bendLineColor = color;
        } else if (m_dxfGroup == 3) {
            m_viewLabelTextColor = color;
        } else if (m_dxfGroup == 4) {
            if (m_dxfColorCount++ == 0) m_hatchColor = color;
            else m_hatchBackgroundColor = color;
        }
        return true;
    }
    case 40: {
        const double value = reader->getDouble();
        if (m_dxfGroup == 1) {
            if (m_dxfDoubleCount == 0) m_identifierHeight = value;
            else if (m_dxfDoubleCount == 1) m_arrowSymbolSize = value;
            else if (m_dxfDoubleCount == 2) m_arrowSymbolExtensionLength = value;
            else m_identifierOffset = value;
            ++m_dxfDoubleCount;
        } else if (m_dxfGroup == 2) {
            if (m_dxfDoubleCount == 0) m_bendLineLength = value;
            else if (m_dxfDoubleCount == 1) m_endLineOvershoot = value;
            else m_endLineLength = value;
            ++m_dxfDoubleCount;
        } else if (m_dxfGroup == 3) {
            if (m_dxfDoubleCount++ == 0) m_viewLabelTextHeight = value;
            else m_viewLabelOffset = value;
        } else if (m_dxfGroup == 4) {
            if (m_dxfDoubleCount++ == 0) m_hatchScale = value;
            else m_hatchAngles.push_back(value);
        }
        return true;
    }
    case 90: {
        const std::int32_t value = reader->getInt32();
        if (m_dxfGroup == 0 || m_dxfGroup == -1) m_flags = static_cast<std::uint32_t>(value);
        else if (m_dxfGroup == 1) {
            if (m_dxfLongCount++ == 0) m_identifierPosition = value;
            else m_arrowPosition = value;
        } else if (m_dxfGroup == 2) {
            if (m_dxfLongCount++ == 0) m_planeLineWeight = value;
            else m_bendLineWeight = value;
        } else if (m_dxfGroup == 3) {
            if (m_dxfLongCount++ == 0) m_viewLabelAttachment = static_cast<std::uint32_t>(value);
            else m_viewLabelAlignment = static_cast<std::uint32_t>(value);
        } else if (m_dxfGroup == 4) {
            if (m_dxfLongCount++ == 0) m_hatchTransparency = value;
            else m_dxfExpectedHatchAngles = static_cast<std::uint32_t>(value);
        }
        return true;
    }
    case 290:
        if (m_dxfGroup == 4) {
            if (m_dxfBoolCount++ == 0) m_unknownB1 = reader->getBool();
            else m_unknownB2 = reader->getBool();
        } else {
            reader->getBool();
        }
        return true;
    case 300:
        if (m_dxfGroup == 1) m_identifierExcludeCharacters = reader->getUtf8String();
        else if (m_dxfGroup == 3) m_viewLabelPattern = reader->getUtf8String();
        else if (m_dxfGroup == 4) m_hatchPattern = reader->getUtf8String();
        else reader->getUtf8String();
        return true;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
}

bool DRW_SectionViewStyle::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = version > DRW::AC1018 ? &sBuff : buf;
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing SECTIONVIEWSTYLE *****************\n");
    if (!ret) return ret;

    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);

    m_modelDoc.m_modelDocClassVersion = buf->getBitShort();
    m_modelDoc.m_description = sBuf->getVariableText(version, false);
    m_modelDoc.m_modifiedForRecompute = buf->getBit() != 0;
    if (version >= DRW::AC1032) {
        m_modelDoc.m_displayName = sBuf->getVariableText(version, false);
        m_modelDoc.m_viewStyleFlags = static_cast<std::uint32_t>(buf->getBitLong());
    }

    m_classVersion = buf->getBitShort();
    m_flags = static_cast<std::uint32_t>(buf->getBitLong());
    m_identifierStyleHandle = readObjectHandleRef(&hBuff);
    m_identifierColor = readObjectCmColor(version, buf, sBuf);
    m_identifierHeight = buf->getBitDouble();
    m_arrowStartSymbolHandle = readObjectHandleRef(&hBuff);
    m_arrowEndSymbolHandle = readObjectHandleRef(&hBuff);
    m_arrowSymbolColor = readObjectCmColor(version, buf, sBuf);
    m_arrowSymbolSize = buf->getBitDouble();
    m_identifierExcludeCharacters = sBuf->getVariableText(version, false);
    m_arrowSymbolExtensionLength = buf->getBitDouble();
    m_planeLineTypeHandle = readObjectHandleRef(&hBuff);
    m_planeLineWeight = buf->getBitLong();
    m_planeLineColor = readObjectCmColor(version, buf, sBuf);
    m_bendLineTypeHandle = readObjectHandleRef(&hBuff);
    m_bendLineWeight = buf->getBitLong();
    m_bendLineColor = readObjectCmColor(version, buf, sBuf);
    m_bendLineLength = buf->getBitDouble();
    m_endLineLength = buf->getBitDouble();
    m_viewLabelTextStyleHandle = readObjectHandleRef(&hBuff);
    m_viewLabelTextColor = readObjectCmColor(version, buf, sBuf);
    m_viewLabelTextHeight = buf->getBitDouble();
    m_viewLabelAttachment = static_cast<std::uint32_t>(buf->getBitLong());
    m_viewLabelOffset = buf->getBitDouble();
    m_viewLabelAlignment = static_cast<std::uint32_t>(buf->getBitLong());
    m_viewLabelPattern = sBuf->getVariableText(version, false);
    m_hatchColor = readObjectCmColor(version, buf, sBuf);
    m_hatchBackgroundColor = readObjectCmColor(version, buf, sBuf);
    m_hatchPattern = sBuf->getVariableText(version, false);
    m_hatchScale = buf->getBitDouble();
    m_hatchTransparency = buf->getBitLong();
    m_unknownB1 = buf->getBit() != 0;
    m_unknownB2 = buf->getBit() != 0;
    m_identifierPosition = buf->getBitLong();
    m_identifierOffset = buf->getBitDouble();
    m_arrowPosition = buf->getBitLong();
    m_endLineOvershoot = buf->getBitDouble();
    const std::int32_t hatchAngleCount = buf->getBitLong();
    if (hatchAngleCount < 0 || hatchAngleCount > 100000)
        return false;
    m_hatchAngles.clear();
    m_hatchAngles.reserve(static_cast<size_t>(hatchAngleCount));
    for (std::int32_t i = 0; i < hatchAngleCount; ++i)
        m_hatchAngles.push_back(buf->getBitDouble());

    return buf->isGood() && sBuf->isGood() && hBuff.isGood();
}

void DRW_BreakData::reset() {
    tType = DRW::BREAKDATA;
    m_pointRefHandles.clear();
    m_dimensionHandle = 0;
    m_dxfInBreakData = false;
    DRW_TableEntry::reset();
}

bool DRW_BreakData::parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
    if (code == 100) {
        m_dxfInBreakData = reader->getUtf8String() == "AcDbBreakData";
        return true;
    }
    if (!m_dxfInBreakData)
        return DRW_TableEntry::parseCode(code, reader);
    switch (code) {
    case 90:
        reader->getInt32();
        return true;
    case 330:
        m_pointRefHandles.push_back(static_cast<std::uint32_t>(reader->getHandleString()));
        return true;
    case 331:
        m_dimensionHandle = static_cast<std::uint32_t>(reader->getHandleString());
        return true;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
}

bool DRW_BreakData::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    bool ret = DRW_TableEntry::parseDwg(version, buf, nullptr, bs);
    DRW_DBG("\n***************************** parsing BREAKDATA ************************\n");
    if (!ret) return ret;

    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);

    const std::int32_t pointRefCount = buf->getBitLong();
    if (pointRefCount < 0 || pointRefCount > 100000)
        return buf->isGood();
    m_pointRefHandles.clear();
    m_pointRefHandles.reserve(static_cast<size_t>(pointRefCount));
    for (std::int32_t i = 0; i < pointRefCount; ++i)
        m_pointRefHandles.push_back(readObjectHandleRef(&hBuff));
    m_dimensionHandle = readObjectHandleRef(&hBuff);
    return buf->isGood() && hBuff.isGood();
}

bool DRW_BreakPointRef::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs) {
    bool ret = DRW_TableEntry::parseDwg(version, buf, nullptr, bs);
    DRW_DBG("\n***************************** parsing BREAKPOINTREF ********************\n");
    if (!ret) return ret;
    dwgBuffer hBuff = *buf;
    seekObjectHandleStream(version, &hBuff, objSize);
    readCommonObjectHandles(&hBuff, handle, numReactors, xDictFlag, &parentHandle);
    return buf->isGood() && hBuff.isGood();
}

// VISUALSTYLE (AcDbVisualStyle) — stub parser per ODA spec §20.4.95.
// Reads only what's needed for round-trip identity; full visual-style
// data (60+ fields) is irrelevant to LibreCAD's 2D rendering. Each
bool DRW_VisualStyle::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // AcDbVisualStyle DXF: description (2 / 4) + style type (70).  Matches
    // dwgTs's VISUALSTYLE decode; the per-property render settings (face/edge/
    // display) stay round-tripped raw only.
    switch (code) {
    case 2:
    case 4:
        desc = reader->getUtf8String();
        break;
    case 70:
        type = reader->getInt32();
        break;
    default:
        return DRW_TableEntry::parseCode(code, reader);
    }
    return true;
}

// object is parsed from a size-bounded buffer so any unread tail is
// safely discarded by the caller.
//
// Full AcDbVisualStyle body decode. Field order transcribed 1:1 from dwgTs
// parseObjects.ts parseVisualStyle / readVisualStyleLegacyBody (1587-1627) /
// readVisualStyleR2010bBody (1762-1832) / readVisualStyleR2013bExpansion
// (1651-1710), cross-checked against libreDWG dwg2.spec:2119-2289.
// Bit-primitive map: readBitLong->getBitLong, readBitDouble->getBitDouble,
// readBitShort->getBitShort, readBit->getBit, readRawChar->getRawChar8,
// readCmColor->readObjectCmColor, readObjectText->getVariableText.
// Graceful degrade: always return true once the common preamble parses, so
// dwgreader's `if(ret)` still delivers BOTH addVisualStyle and the raw shelf.
bool DRW_VisualStyle::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {  // 2007+ uses separate string stream
        sBuf = &sBuff;
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing VISUALSTYLE *********************\n");
    if (!ret) return ret;

    // hasBodyBytesRemaining (dwgTs :8033): for R2007+ the body ends at objSize
    // (the handle-stream start, in bits); guard every optional read against it.
    // Pre-R2007 uses numRemainingBytes() to match dwgTs's buf.remaining() > 0
    // (isGood() stays true at EOF, giving false-positive tail admits).
    auto bodyBytesRemaining = [&]() -> bool {
        if (version > DRW::AC1018 && objSize > 0)
            return currentObjectDwgBit(buf) < objSize;
        return buf->numRemainingBytes() > 0;
    };

    if (bodyBytesRemaining())
        desc = sBuf->getVariableText(version, false);   // FIELD_T description (2)
    if (bodyBytesRemaining())
        type = buf->getBitLong();                       // FIELD_BL style_type (70)

    if (version < DRW::AC1024 && bodyBytesRemaining()) {
        // ---- legacy body (< AC1024) : parseObjects.ts:1587-1627 / dwg2.spec PRE(R_2010b) ----
        m_body.faceLightingModel     = buf->getBitLong();
        m_body.faceLightingQuality   = buf->getBitLong();
        m_body.faceColorMode         = buf->getBitLong();
        m_body.faceOpacity           = buf->getBitDouble();
        m_body.faceSpecular          = buf->getBitDouble();
        m_body.faceMonoColor         = readObjectCmColor(version, buf, sBuf);
        m_body.faceModifier          = buf->getBitLong();

        m_body.edgeModel             = buf->getBitLong();
        m_body.edgeStyle             = buf->getBitLong();
        m_body.edgeIntersectionColor = readObjectCmColor(version, buf, sBuf);
        m_body.edgeObscuredColor     = readObjectCmColor(version, buf, sBuf);
        m_body.edgeObscuredLtype     = buf->getBitLong();
        m_body.edgeCreaseAngle       = buf->getBitDouble();
        m_body.edgeModifier          = buf->getBitLong();
        m_body.edgeColor             = readObjectCmColor(version, buf, sBuf);
        m_body.edgeOpacity           = buf->getBitDouble();
        m_body.edgeWidth             = buf->getBitShort();   // FIELD_CAST BS->BL (2149)
        m_body.edgeOverhang          = buf->getBitShort();   // FIELD_CAST BS->BL (2150)
        m_body.edgeJitter            = buf->getBitLong();
        m_body.edgeSilhouetteColor   = readObjectCmColor(version, buf, sBuf);
        m_body.edgeSilhouetteWidth   = buf->getBitShort();   // FIELD_CAST BS->BL (2153)
        m_body.edgeHaloGap           = buf->getRawChar8();   // FIELD_CAST RC->BL (2154)
        m_body.edgeIsolines          = buf->getBitShort();   // FIELD_CAST BS->BL (2155)
        m_body.edgeDoHidePrecision   = buf->getBit() != 0;
        m_body.edgeStyleApply        = buf->getBitShort();   // FIELD_CAST BS->BL (2157)
        m_body.edgeIntersectionLtype = buf->getBitShort();   // FIELD_CAST BS->BL (2158)

        m_body.displaySettings       = buf->getBitLong();
        m_body.displayBrightness     = static_cast<double>(buf->getBitLong()); // FIELD_BLd (2160)

        if (bodyBytesRemaining()) {                          // dwg2.spec:2162 tail guard
            m_body.displayShadowType = buf->getBitLong();
            if (version >= DRW::AC1021)
                buf->getBitDouble();                         // bd2007_45 (45), discarded
            m_body.internalOnly      = buf->getBit() != 0;
        }
    } else if (version >= DRW::AC1024 && bodyBytesRemaining()) {
        // ---- r2010b body (>= AC1024) : parseObjects.ts:1762-1827 / dwg2.spec SINCE(R_2010b) ----
        m_body.extLightingModel      = buf->getBitShort();   // FIELD_BS ext_lighting_model (177)
        m_body.internalOnly          = buf->getBit() != 0;   // FIELD_B internal_only (291)

        // value + BS "_int" companion (companion consumed/discarded):
        m_body.faceLightingModel     = buf->getBitLong();   buf->getBitShort();
        m_body.faceLightingQuality   = buf->getBitLong();   buf->getBitShort();
        m_body.faceColorMode         = buf->getBitLong();   buf->getBitShort();
        m_body.faceModifier          = buf->getBitShort();  buf->getBitShort();  // FIELD_CAST BS->BL (2189)
        m_body.faceOpacity           = buf->getBitDouble(); buf->getBitShort();
        m_body.faceSpecular          = buf->getBitDouble(); buf->getBitShort();
        m_body.faceMonoColor         = readObjectCmColor(version, buf, sBuf); buf->getBitShort();

        m_body.edgeModel             = buf->getBitLong();   buf->getBitShort();
        m_body.edgeStyle             = buf->getBitLong();   buf->getBitShort();
        m_body.edgeIntersectionColor = readObjectCmColor(version, buf, sBuf); buf->getBitShort();
        m_body.edgeObscuredColor     = readObjectCmColor(version, buf, sBuf); buf->getBitShort();
        m_body.edgeObscuredLtype     = buf->getBitLong();   buf->getBitShort();
        m_body.edgeIntersectionLtype = buf->getBitLong();   buf->getBitShort();
        m_body.edgeCreaseAngle       = buf->getBitDouble(); buf->getBitShort();
        m_body.edgeModifier          = buf->getBitLong();   buf->getBitShort();
        m_body.edgeColor             = readObjectCmColor(version, buf, sBuf); buf->getBitShort();
        m_body.edgeOpacity           = buf->getBitDouble(); buf->getBitShort();
        m_body.edgeWidth             = buf->getBitLong();   buf->getBitShort();
        m_body.edgeOverhang          = buf->getBitLong();   buf->getBitShort();
        m_body.edgeJitter            = buf->getBitLong();   buf->getBitShort();
        m_body.edgeSilhouetteColor   = readObjectCmColor(version, buf, sBuf); buf->getBitShort();
        m_body.edgeSilhouetteWidth   = buf->getBitLong();   buf->getBitShort();
        m_body.edgeHaloGap           = buf->getBitLong();   buf->getBitShort();
        {
            const std::int32_t isolines = buf->getBitLong();
            m_body.edgeIsolines      = isolines > 5000 ? 5000 : isolines;  // VALUEOUTOFBOUNDS (2210)
            buf->getBitShort();
        }
        m_body.edgeDoHidePrecision   = buf->getBit() != 0;  buf->getBitShort();

        m_body.displaySettings       = buf->getBitLong();   buf->getBitShort();
        m_body.displayBrightness     = buf->getBitDouble(); buf->getBitShort();
        m_body.displayShadowType     = buf->getBitLong();   buf->getBitShort();

        if (version >= DRW::AC1027) {
            // ---- r2013b expansion : parseObjects.ts:1651-1710 / dwg2.spec SINCE(R_2013b) ----
            m_body.hasR2013bExpansion = true;
            m_body.bProp1c = buf->getBit() != 0;   buf->getBitShort();
            m_body.bProp1d = buf->getBit() != 0;   buf->getBitShort();
            m_body.bProp1e = buf->getBit() != 0;   buf->getBitShort();
            m_body.bProp1f = buf->getBit() != 0;   buf->getBitShort();
            m_body.bProp20 = buf->getBit() != 0;   buf->getBitShort();
            m_body.bProp21 = buf->getBit() != 0;   buf->getBitShort();
            m_body.bProp22 = buf->getBit() != 0;   buf->getBitShort();
            m_body.bProp23 = buf->getBit() != 0;   buf->getBitShort();
            m_body.bProp24 = buf->getBit() != 0;   buf->getBitShort();
            m_body.blProp25 = buf->getBitLong();   buf->getBitShort();
            m_body.bdProp26 = buf->getBitDouble(); buf->getBitShort();
            m_body.bdProp27 = buf->getBitDouble(); buf->getBitShort();
            m_body.blProp28 = buf->getBitLong();   buf->getBitShort();
            m_body.cProp29  = readObjectCmColor(version, buf, sBuf); buf->getBitShort();
            m_body.blProp2a = buf->getBitLong();   buf->getBitShort();
            m_body.blProp2b = buf->getBitLong();   buf->getBitShort();
            m_body.cProp2c  = readObjectCmColor(version, buf, sBuf); buf->getBitShort();
            m_body.bProp2d  = buf->getBit() != 0;  buf->getBitShort();
            m_body.blProp2e = buf->getBitLong();   buf->getBitShort();
            m_body.blProp2f = buf->getBitLong();   buf->getBitShort();
            m_body.blProp30 = buf->getBitLong();   buf->getBitShort();
            m_body.bProp31  = buf->getBit() != 0;  buf->getBitShort();
            m_body.blProp32 = buf->getBitLong();   buf->getBitShort();
            m_body.cProp33  = readObjectCmColor(version, buf, sBuf); buf->getBitShort();
            m_body.bdProp34 = buf->getBitDouble(); buf->getBitShort();
            m_body.edgeWiggle = buf->getBitLong(); buf->getBitShort();       // prop 0x35
            m_body.strokes  = sBuf->getVariableText(version, false); buf->getBitShort(); // prop 0x36
            m_body.bProp37  = buf->getBit() != 0;  buf->getBitShort();
            m_body.bdProp38 = buf->getBitDouble(); buf->getBitShort();
            m_body.bdProp39 = buf->getBitDouble(); buf->getBitShort();
        }
    }

    // m_bodyDecoded reflects whether the walk stayed inside the object; the
    // object (typed + raw shelf) is delivered regardless (graceful degrade).
    m_bodyDecoded = buf->isGood();
    return true;
}

// DBCOLOR (AcDbColor) per ODA spec §20.4 / libreDWG dwg2.spec:2404-2408.
// Layout: common preamble + FIELD_CMC + START_OBJECT_HANDLE_STREAM.
bool DRW_DbColor::parseDwg(DRW::Version version, dwgBuffer *buf, std::uint32_t bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {  // 2007+ uses separate string stream
        sBuf = &sBuff;
    }
    bool ret = DRW_TableEntry::parseDwg(version, buf, sBuf, bs);
    DRW_DBG("\n***************************** parsing DBCOLOR (AcDbColor) ***************\n");
    if (!ret) return ret;

    // Single FIELD_CMC per the spec — let getCmColor handle the bit layout
    // (BS index, BL rgb-with-method, RC method byte, optional TV name +
    // book name from sBuf). Out-params populate our fields directly.
    std::int32_t rgb24 = -1;
    UTF8STRING cmcName, cmcBookName;
    std::uint32_t colorRet = buf->getCmColor(version, &rgb24, sBuf, &cmcName, &cmcBookName);
    colorIndex = static_cast<std::uint16_t>(colorRet);
    if (rgb24 != -1) rgb = rgb24;
    name = std::move(cmcName);
    bookName = std::move(cmcBookName);
    // colorMethod isn't directly returned by getCmColor; deduce from rgb24
    // presence (true RGB) or fall back to ByLayer/ByBlock per colorRet.
    if (rgb24 != -1)        colorMethod = 0xC2;  // true color
    else if (colorRet == 0) colorMethod = 0xC1;  // ByBlock
    else if (colorRet == 256) colorMethod = 0xC0;  // ByLayer
    else                    colorMethod = 0xC3;  // ACIS index
    DRW_DBG("DBCOLOR method: "); DRW_DBGH(colorMethod);
    DRW_DBG(" idx: "); DRW_DBG(colorIndex);
    DRW_DBG(" rgb: "); DRW_DBGH(rgb); DRW_DBG("\n");
    // START_OBJECT_HANDLE_STREAM follows — the parent/reactors/xdic handles.
    // libdxfrw's object dispatch hands us a slice; the OBJECTS section
    // parser stays aligned regardless of remainder.
    return buf->isGood();
}
