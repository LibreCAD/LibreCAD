/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2021 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/


#include "libdxfrw.h"
#include <cerrno>
#include <cctype>
#include <charconv>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <set>
#include <sstream>
#include <cassert>
#include <chrono>
#include <unordered_set>
#include "intern/drw_textcodec.h"
#include "intern/dxfparserlimits.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/drw_dbg.h"
#include "intern/drw_reserve.h"
#include "intern/dwg_dxf_output_transaction.h"
#include "intern/dwgsafety.h"
#include "intern/dwgutil.h"
#include "intern/proxygraphicdecoder.h"

#define FIRSTHANDLE 48

namespace {

bool updateRawDxfApplicationDepth(const DRW_Variant& value, int& depth);
bool validateCapturedRawDxfObject(const DRW_RawDxfObject& object,
                                  bool binaryOutput);

bool isSupportedDxfWriteVersion(DRW::Version version) {
    switch (version) {
    case DRW::AC1009:
    case DRW::AC1014:
    case DRW::AC1015:
    case DRW::AC1018:
    case DRW::AC1021:
    case DRW::AC1024:
    case DRW::AC1027:
    case DRW::AC1032:
        return true;
    default:
        return false;
    }
}

bool isDxfHexText(const std::string& text) {
    if ((text.size() & 1u) != 0)
        return false;
    return std::all_of(text.begin(), text.end(), [](char value) {
        return std::isxdigit(static_cast<unsigned char>(value)) != 0;
    });
}

bool isValidDxfEedVariant(const DRW_Variant *value) {
    if (value == nullptr)
        return false;

    switch (value->code()) {
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1005:
        return value->type() == DRW_Variant::STRING
            && value->content.s != nullptr;
    case 1004:
        if (value->type() == DRW_Variant::BINARY) {
            return value->content.b != nullptr
                && value->content.b->size()
                    <= DRW::kMaxDxfBinaryPayloadBytes;
        }
        return value->type() == DRW_Variant::STRING
            && value->content.s != nullptr
            && isDxfHexText(*value->content.s)
            && value->content.s->size() / 2u
                <= DRW::kMaxDxfBinaryPayloadBytes;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
        return value->type() == DRW_Variant::COORD
            && value->content.v != nullptr
            && std::isfinite(value->content.v->x)
            && std::isfinite(value->content.v->y)
            && std::isfinite(value->content.v->z);
    case 1040:
    case 1041:
    case 1042:
        return value->type() == DRW_Variant::DOUBLE
            && std::isfinite(value->content.d);
    case 1070:
        return value->type() == DRW_Variant::INTEGER;
    case 1071:
        if (value->type() == DRW_Variant::INTEGER)
            return true;
        return value->type() == DRW_Variant::INTEGER64
            && value->content.i64 >= std::numeric_limits<std::int32_t>::min()
            && value->content.i64 <= std::numeric_limits<std::int32_t>::max();
    default:
        return false;
    }
}

bool isSafeDxfRecordText(const std::string& text) {
    return text.find('\0') == std::string::npos
        && text.find('\r') == std::string::npos
        && text.find('\n') == std::string::npos;
}

bool isFiniteDxfCoord(const DRW_Coord& point) {
    return std::isfinite(point.x) && std::isfinite(point.y)
        && std::isfinite(point.z);
}

bool isValidDxfLineWeight(DRW_LW_Conv::lineWidth value) {
    switch (value) {
    case DRW_LW_Conv::width00:
    case DRW_LW_Conv::width01:
    case DRW_LW_Conv::width02:
    case DRW_LW_Conv::width03:
    case DRW_LW_Conv::width04:
    case DRW_LW_Conv::width05:
    case DRW_LW_Conv::width06:
    case DRW_LW_Conv::width07:
    case DRW_LW_Conv::width08:
    case DRW_LW_Conv::width09:
    case DRW_LW_Conv::width10:
    case DRW_LW_Conv::width11:
    case DRW_LW_Conv::width12:
    case DRW_LW_Conv::width13:
    case DRW_LW_Conv::width14:
    case DRW_LW_Conv::width15:
    case DRW_LW_Conv::width16:
    case DRW_LW_Conv::width17:
    case DRW_LW_Conv::width18:
    case DRW_LW_Conv::width19:
    case DRW_LW_Conv::width20:
    case DRW_LW_Conv::width21:
    case DRW_LW_Conv::width22:
    case DRW_LW_Conv::width23:
    case DRW_LW_Conv::widthByLayer:
    case DRW_LW_Conv::widthByBlock:
    case DRW_LW_Conv::widthDefault:
        return true;
    }
    return false;
}

bool isValidDxfEntityFields(const DRW_Entity& entity) {
    if (entity.space != DRW::ModelSpace && entity.space != DRW::PaperSpace)
        return false;
    if (!isSafeDxfRecordText(entity.layer)
        || !isSafeDxfRecordText(entity.lineType)
        || !isSafeDxfRecordText(entity.colorName))
        return false;
    if (!std::isfinite(entity.ltypeScale) || entity.ltypeScale <= 0.0
        || entity.color < -255 || entity.color > DRW::ColorByLayer
        || (entity.color24 < -1 || entity.color24 > 0xFFFFFF)
        || !isValidDxfLineWeight(entity.lWeight)
        || entity.numProxyGraph < 0)
        return false;
    if (std::any_of(entity.reactorHandles.cbegin(),
                    entity.reactorHandles.cend(),
                    [](std::uint32_t handle) { return handle == 0; }))
        return false;
    if (std::any_of(entity.extData.cbegin(), entity.extData.cend(),
                    [](const std::shared_ptr<DRW_Variant>& value) {
                        return !isValidDxfEedVariant(value.get());
                    }))
        return false;

    if (const auto *point = dynamic_cast<const DRW_Point*>(&entity)) {
        if (!isFiniteDxfCoord(point->basePoint)
            || !isFiniteDxfCoord(point->extPoint)
            || !std::isfinite(point->thickness)
            || !std::isfinite(point->xAxisAngle))
            return false;
    }
    if (const auto *line = dynamic_cast<const DRW_Line*>(&entity)) {
        if (!isFiniteDxfCoord(line->secPoint))
            return false;
    }
    if (const auto *trace = dynamic_cast<const DRW_Trace*>(&entity)) {
        if (!isFiniteDxfCoord(trace->thirdPoint)
            || !isFiniteDxfCoord(trace->fourPoint))
            return false;
    }
    if (const auto *circle = dynamic_cast<const DRW_Circle*>(&entity)) {
        if (!std::isfinite(circle->radious))
            return false;
    }
    if (const auto *arc = dynamic_cast<const DRW_Arc*>(&entity)) {
        if (!std::isfinite(arc->staangle)
            || !std::isfinite(arc->endangle))
            return false;
    }
    if (const auto *ellipse = dynamic_cast<const DRW_Ellipse*>(&entity)) {
        if (!std::isfinite(ellipse->ratio)
            || !std::isfinite(ellipse->staparam)
            || !std::isfinite(ellipse->endparam))
            return false;
    }
    if (const auto *text = dynamic_cast<const DRW_Text*>(&entity)) {
        if (!std::isfinite(text->height) || !std::isfinite(text->angle)
            || !std::isfinite(text->widthscale)
            || !std::isfinite(text->oblique)
            || !isSafeDxfRecordText(text->text)
            || !isSafeDxfRecordText(text->style))
            return false;
    }
    if (const auto *insert = dynamic_cast<const DRW_Insert*>(&entity)) {
        if (!std::isfinite(insert->xscale)
            || !std::isfinite(insert->yscale)
            || !std::isfinite(insert->zscale)
            || !std::isfinite(insert->angle)
            || !std::isfinite(insert->colspace)
            || !std::isfinite(insert->rowspace))
            return false;
    }
    return true;
}

bool isValidDxfAppData(const std::list<std::list<DRW_Variant>>& appData) {
    for (const std::list<DRW_Variant>& group : appData) {
        bool opened = false;
        int depth = 0;
        for (const DRW_Variant& data : group) {
            if (data.code() == 102) {
                if (data.type() != DRW_Variant::STRING
                    || data.content.s == nullptr)
                    return false;
                const std::string& marker = *data.content.s;
                if (!opened) {
                    if (marker.empty() || marker == "}")
                        return false;
                    opened = true;
                    depth = 1;
                } else if (marker == "}") {
                    if (--depth < 0)
                        return false;
                } else if (!marker.empty() && marker.front() == '{') {
                    if (++depth > DRW::kMaxDxfApplicationGroupNesting)
                        return false;
                } else {
                    return false;
                }
                continue;
            }
            if (!opened || depth == 0)
                return false;
            switch (data.type()) {
            case DRW_Variant::STRING:
                if (data.content.s == nullptr)
                    return false;
                break;
            case DRW_Variant::INTEGER:
            case DRW_Variant::INTEGER64:
                break;
            case DRW_Variant::DOUBLE:
                if (!std::isfinite(data.content.d))
                    return false;
                break;
            default:
                return false;
            }
        }
        if (!group.empty() && (!opened || depth != 0))
            return false;
    }
    return true;
}

struct DxfDimstyleVariableSpec {
    int code;
    DRW_Variant::TYPE type;
    DRW::Version minimumVersion;
};

// DIMSTYLE's dynamic map is an override mechanism, not an arbitrary DXF
// record. Keep the type and version contract in one table so a malformed
// variant cannot be emitted through an inactive union member.
constexpr DxfDimstyleVariableSpec kDxfDimstyleVariableSpecs[] = {
    {3, DRW_Variant::STRING, DRW::AC1009},
    {4, DRW_Variant::STRING, DRW::AC1009},
    {5, DRW_Variant::STRING, DRW::AC1009},
    {6, DRW_Variant::STRING, DRW::AC1009},
    {7, DRW_Variant::STRING, DRW::AC1009},
    {40, DRW_Variant::DOUBLE, DRW::AC1009},
    {41, DRW_Variant::DOUBLE, DRW::AC1009},
    {42, DRW_Variant::DOUBLE, DRW::AC1009},
    {43, DRW_Variant::DOUBLE, DRW::AC1009},
    {44, DRW_Variant::DOUBLE, DRW::AC1009},
    {45, DRW_Variant::DOUBLE, DRW::AC1009},
    {46, DRW_Variant::DOUBLE, DRW::AC1009},
    {47, DRW_Variant::DOUBLE, DRW::AC1009},
    {48, DRW_Variant::DOUBLE, DRW::AC1009},
    {49, DRW_Variant::DOUBLE, DRW::AC1021},
    {50, DRW_Variant::DOUBLE, DRW::AC1021},
    {69, DRW_Variant::INTEGER, DRW::AC1021},
    {70, DRW_Variant::INTEGER, DRW::AC1021},
    {71, DRW_Variant::INTEGER, DRW::AC1009},
    {72, DRW_Variant::INTEGER, DRW::AC1009},
    {73, DRW_Variant::INTEGER, DRW::AC1009},
    {74, DRW_Variant::INTEGER, DRW::AC1009},
    {75, DRW_Variant::INTEGER, DRW::AC1009},
    {76, DRW_Variant::INTEGER, DRW::AC1009},
    {77, DRW_Variant::INTEGER, DRW::AC1009},
    {78, DRW_Variant::INTEGER, DRW::AC1009},
    {79, DRW_Variant::INTEGER, DRW::AC1015},
    {90, DRW_Variant::INTEGER, DRW::AC1021},
    {140, DRW_Variant::DOUBLE, DRW::AC1009},
    {141, DRW_Variant::DOUBLE, DRW::AC1009},
    {142, DRW_Variant::DOUBLE, DRW::AC1009},
    {143, DRW_Variant::DOUBLE, DRW::AC1009},
    {144, DRW_Variant::DOUBLE, DRW::AC1009},
    {145, DRW_Variant::DOUBLE, DRW::AC1009},
    {146, DRW_Variant::DOUBLE, DRW::AC1009},
    {147, DRW_Variant::DOUBLE, DRW::AC1009},
    {148, DRW_Variant::DOUBLE, DRW::AC1015},
    {170, DRW_Variant::INTEGER, DRW::AC1009},
    {171, DRW_Variant::INTEGER, DRW::AC1009},
    {172, DRW_Variant::INTEGER, DRW::AC1009},
    {173, DRW_Variant::INTEGER, DRW::AC1009},
    {174, DRW_Variant::INTEGER, DRW::AC1009},
    {175, DRW_Variant::INTEGER, DRW::AC1009},
    {176, DRW_Variant::INTEGER, DRW::AC1009},
    {177, DRW_Variant::INTEGER, DRW::AC1009},
    {178, DRW_Variant::INTEGER, DRW::AC1009},
    {179, DRW_Variant::INTEGER, DRW::AC1015},
    {270, DRW_Variant::INTEGER, DRW::AC1012},
    {271, DRW_Variant::INTEGER, DRW::AC1012},
    {272, DRW_Variant::INTEGER, DRW::AC1012},
    {273, DRW_Variant::INTEGER, DRW::AC1012},
    {274, DRW_Variant::INTEGER, DRW::AC1012},
    {275, DRW_Variant::INTEGER, DRW::AC1012},
    {276, DRW_Variant::INTEGER, DRW::AC1015},
    {277, DRW_Variant::INTEGER, DRW::AC1015},
    {278, DRW_Variant::INTEGER, DRW::AC1015},
    {279, DRW_Variant::INTEGER, DRW::AC1015},
    {280, DRW_Variant::INTEGER, DRW::AC1012},
    {281, DRW_Variant::INTEGER, DRW::AC1012},
    {282, DRW_Variant::INTEGER, DRW::AC1012},
    {283, DRW_Variant::INTEGER, DRW::AC1012},
    {284, DRW_Variant::INTEGER, DRW::AC1012},
    {285, DRW_Variant::INTEGER, DRW::AC1012},
    {286, DRW_Variant::INTEGER, DRW::AC1012},
    {287, DRW_Variant::INTEGER, DRW::AC1012},
    {288, DRW_Variant::INTEGER, DRW::AC1012},
    {289, DRW_Variant::INTEGER, DRW::AC1015},
    {290, DRW_Variant::INTEGER, DRW::AC1021},
    {295, DRW_Variant::INTEGER, DRW::AC1024},
    {340, DRW_Variant::STRING, DRW::AC1012},
    {341, DRW_Variant::STRING, DRW::AC1015},
    {342, DRW_Variant::STRING, DRW::AC1015},
    {343, DRW_Variant::STRING, DRW::AC1015},
    {344, DRW_Variant::STRING, DRW::AC1015},
    {345, DRW_Variant::STRING, DRW::AC1021},
    {346, DRW_Variant::STRING, DRW::AC1021},
    {347, DRW_Variant::STRING, DRW::AC1021},
    {371, DRW_Variant::INTEGER, DRW::AC1015},
    {372, DRW_Variant::INTEGER, DRW::AC1015},
};

const DxfDimstyleVariableSpec* findDxfDimstyleVariableSpec(int code) {
    for (const DxfDimstyleVariableSpec& spec : kDxfDimstyleVariableSpecs) {
        if (spec.code == code)
            return &spec;
    }
    return nullptr;
}

class DxfRawHandleLexemeScope final {
public:
    explicit DxfRawHandleLexemeScope(dxfReader& reader)
        : m_reader(reader), m_previous(reader.allowsWideHandleLexemes()) {
        m_reader.setAllowWideHandleLexemes(true);
    }

    ~DxfRawHandleLexemeScope() {
        m_reader.setAllowWideHandleLexemes(m_previous);
    }

    DxfRawHandleLexemeScope(const DxfRawHandleLexemeScope&) = delete;
    DxfRawHandleLexemeScope& operator=(const DxfRawHandleLexemeScope&) = delete;

private:
    dxfReader& m_reader;
    bool m_previous;
};

bool dxfKeywordEquals(const std::string& value, const char* keyword) {
    if (keyword == nullptr || value.size() != std::strlen(keyword))
        return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        const unsigned char valueChar =
            static_cast<unsigned char>(value[i]);
        const unsigned char keywordChar =
            static_cast<unsigned char>(keyword[i]);
        if (std::toupper(valueChar) != std::toupper(keywordChar))
            return false;
    }
    return true;
}

bool dxfTableEntryComplete(const DRW_TableEntry& entry,
                           DRW::Version sourceVersion) {
    if (entry.name.empty())
        return false;
    return sourceVersion == DRW::UNKNOWNV || sourceVersion <= DRW::AC1009
        || entry.handle != DRW::NoHandle;
}

std::string dxfSymbolNameKey(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::toupper(ch));
                   });
    return value;
}

bool isSafeDxfClassMetadata(const DRW_Class& cls) {
    return !cls.className.empty()
        && cls.recName.find_first_of("\r\n") == std::string::npos
        && cls.className.find_first_of("\r\n") == std::string::npos
        && cls.appName.find_first_of("\r\n") == std::string::npos
        && cls.recName.find('\0') == std::string::npos
        && cls.className.find('\0') == std::string::npos
        && cls.appName.find('\0') == std::string::npos
        && cls.proxyFlag >= 0
        && cls.proxyFlag <= std::numeric_limits<std::uint16_t>::max()
        && cls.instanceCount >= 0
        && cls.wasaProxyFlag >= 0 && cls.wasaProxyFlag <= 1
        && cls.entityFlag >= 0 && cls.entityFlag <= 1;
}

bool checkedDxfHandleOffset(std::uint32_t base, std::uint32_t delta,
                            std::uint32_t& result) noexcept {
    if (base > std::numeric_limits<std::uint32_t>::max() - delta)
        return false;
    result = base + delta;
    return true;
}

bool canWriteDxfDimensionAssociation(
    const DRW_DimensionAssociation& association) {
    if ((association.m_associativityFlags & ~0x0Fu) != 0
        || association.m_osnapRefs.size() > 4
        || association.m_hasUnrepresentableDetail)
        return false;

    std::size_t activeSlots = 0;
    for (std::uint32_t flag = 1; flag <= 8; flag <<= 1) {
        if ((association.m_associativityFlags & flag) != 0)
            ++activeSlots;
    }
    return activeSlots == association.m_osnapRefs.size()
        && std::all_of(association.m_osnapRefs.begin(),
                       association.m_osnapRefs.end(),
                       [](const DRW_DimensionAssociationOsnapRef& ref) {
                           return ref.m_objectOsnapType == 0;
                       });
}

bool canWriteDxfEvaluationGraph(
    DRW::Version version, const DRW_EvaluationGraph& graph) {
    return version >= DRW::AC1021
        && graph.m_nodes.size() <= DRW_EvaluationGraph::kMaxEntries
        && graph.m_edges.size() <= DRW_EvaluationGraph::kMaxEntries;
}

void writeDxfBinaryChunks(dxfWriter* writer,
                          const std::vector<std::uint8_t>& data);

std::string dxfFieldHandleText(std::uint32_t handle) {
    std::ostringstream stream;
    stream << std::uppercase << std::hex << handle;
    return stream.str();
}

bool dxfFieldValuePayloadIsSuppressed(
    DRW::Version version, const DRW_CadValue& value) {
    return version > DRW::AC1018 && (value.m_formatFlags & 3) != 0;
}

bool dxfFieldValueHasData(const DRW_CadValue& value) {
    return value.m_dataType != 0 || value.m_formatFlags != 0
        || value.m_dataSize != 0 || value.m_unitType != 0
        || value.m_value.type() != DRW_Variant::INVALID
        || !value.m_formatString.empty() || !value.m_valueString.empty()
        || value.m_handle != 0 || !value.m_rawData.empty();
}

const std::vector<std::uint8_t>* dxfFieldBinaryData(
    const DRW_CadValue& value) {
    if (!value.m_rawData.empty())
        return &value.m_rawData;
    if (value.m_value.type() == DRW_Variant::BINARY)
        return value.m_value.binary();
    return nullptr;
}

bool canWriteDxfFieldValue(
    DRW::Version version, const DRW_CadValue& value) {
    switch (value.m_dataType) {
    case 0:
    case 1:
        return dxfFieldValuePayloadIsSuppressed(version, value)
            || value.m_value.type() == DRW_Variant::INTEGER;
    case 2:
        return dxfFieldValuePayloadIsSuppressed(version, value)
            || value.m_value.type() == DRW_Variant::DOUBLE;
    case 4:
        return dxfFieldValuePayloadIsSuppressed(version, value)
            || value.m_value.type() == DRW_Variant::STRING;
    case 8: {
        const std::vector<std::uint8_t>* data = dxfFieldBinaryData(value);
        return dxfFieldValuePayloadIsSuppressed(version, value)
            || (data != nullptr
                && (value.m_dataSize == 0 || value.m_dataSize == data->size()));
    }
    case 16:
    case 32: {
        const std::uint32_t expectedSize =
            static_cast<std::uint32_t>(value.m_dataType == 16 ? 16 : 24);
        return dxfFieldValuePayloadIsSuppressed(version, value)
            || (value.m_value.type() == DRW_Variant::COORD
                && value.m_value.coord() != nullptr
                && value.m_rawData.empty()
                && (value.m_dataSize == 0 || value.m_dataSize == expectedSize));
    }
    case 64:
        if (dxfFieldValuePayloadIsSuppressed(version, value)
            || value.m_value.type() == DRW_Variant::INVALID)
            return true;
        return value.m_value.type() == DRW_Variant::INTEGER
            && (value.m_handle == 0 || value.m_handle
                == static_cast<std::uint32_t>(value.m_value.i_val()));
    default:
        return false;
    }
}

bool writeDxfFieldValue(
    dxfWriter* writer, DRW::Version version, const DRW_CadValue& value) {
    if (writer == nullptr || !canWriteDxfFieldValue(version, value))
        return false;

    bool written = (version <= DRW::AC1018
                    || writer->writeInt32(93, value.m_formatFlags))
        && writer->writeInt32(90, value.m_dataType);
    if (!dxfFieldValuePayloadIsSuppressed(version, value)) {
        switch (value.m_dataType) {
        case 0:
        case 1:
            written = written && writer->writeInt32(91, value.m_value.i_val());
            break;
        case 2:
            written = written && writer->writeDouble(140, value.m_value.d_val());
            break;
        case 4:
            written = written && writer->writeUtf8String(1, value.m_value.c_str());
            break;
        case 8: {
            const std::vector<std::uint8_t>* data = dxfFieldBinaryData(value);
            written = written && writer->writeInt32(
                92, static_cast<std::int32_t>(data->size()));
            if (written && !data->empty())
                writeDxfBinaryChunks(writer, *data);
            written = written && !writer->hasWriteError();
            break;
        }
        case 16:
        case 32: {
            const std::uint32_t dataSize = value.m_dataSize != 0
                ? value.m_dataSize
                : static_cast<std::uint32_t>(value.m_dataType == 16 ? 16 : 24);
            const DRW_Coord* coord = value.m_value.coord();
            written = written && writer->writeInt32(
                92, static_cast<std::int32_t>(dataSize))
                && writer->writeDouble(11, coord->x)
                && writer->writeDouble(21, coord->y);
            if (value.m_dataType == 32)
                written = written && writer->writeDouble(31, coord->z);
            break;
        }
        case 64: {
            const std::uint32_t handle = value.m_handle != 0
                ? value.m_handle
                : static_cast<std::uint32_t>(value.m_value.type()
                                               == DRW_Variant::INTEGER
                    ? value.m_value.i_val() : 0);
            written = written && writer->writeString(
                330, dxfFieldHandleText(handle));
            break;
        }
        default:
            return false;
        }
    }
    if (version > DRW::AC1018) {
        written = written && writer->writeInt32(94, value.m_unitType)
            && writer->writeUtf8String(300, value.m_formatString)
            && (value.m_unitType == 12
                || writer->writeUtf8String(302, value.m_valueString));
    }
    return written && writer->writeUtf8String(304, "ACVALUE_END");
}

bool canWriteDxfField(DRW::Version version, const DRW_Field& field) {
    if (field.m_childHandles.size() > DRW_Field::kMaxItems
        || field.m_objectHandles.size() > DRW_Field::kMaxItems
        || field.m_childValues.size() > DRW_Field::kMaxItems)
        return false;
    return std::all_of(field.m_childHandles.begin(), field.m_childHandles.end(),
                       [](std::uint32_t handle) { return handle != 0; })
        && std::all_of(field.m_objectHandles.begin(), field.m_objectHandles.end(),
                       [](std::uint32_t handle) { return handle != 0; })
        && (!dxfFieldValueHasData(field.m_value)
            || (version > DRW::AC1018
                && canWriteDxfFieldValue(version, field.m_value)))
        && std::all_of(field.m_childValues.begin(), field.m_childValues.end(),
                       [version](const DRW_Field::ChildValue& child) {
                           return canWriteDxfFieldValue(version, child.m_value);
                       });
}

bool canWriteDxfFieldList(const DRW_FieldList& list) {
    return list.m_fieldHandles.size() <= DRW_Field::kMaxItems;
}

void writeDxfSplineBody(dxfWriter *writer, DRW_Spline *ent) {
    // Normal vector is optional; omit when it is the default (0,0,1).
    if (ent->normalVec.x != 0.0 || ent->normalVec.y != 0.0 || ent->normalVec.z != 1.0) {
        writer->writeDouble(210, ent->normalVec.x);
        writer->writeDouble(220, ent->normalVec.y);
        writer->writeDouble(230, ent->normalVec.z);
    }
    int flags = ent->flags;
    if (std::any_of(ent->weightlist.begin(), ent->weightlist.end(),
                    [](double weight) { return std::fabs(weight - 1.0) > 1e-12; })) {
        flags |= 0x04;
    }
    writer->writeInt16(70, flags);
    writer->writeInt16(71, ent->degree);
    writer->writeInt16(72, static_cast<int>(ent->knotslist.size()));
    writer->writeInt16(73, static_cast<int>(ent->controllist.size()));
    writer->writeInt16(74, static_cast<int>(ent->fitlist.size()));
    writer->writeDouble(42, ent->tolknot);
    writer->writeDouble(43, ent->tolcontrol);
    writer->writeDouble(44, ent->tolfit);
    for (double k : ent->knotslist)
        writer->writeDouble(40, k);
    // Control points with interleaved weights (when present)
    for (std::size_t i = 0; i < ent->controllist.size(); ++i) {
        const auto& crd = ent->controllist[i];
        writer->writeDouble(10, crd->x);
        writer->writeDouble(20, crd->y);
        writer->writeDouble(30, crd->z);
        if (i < ent->weightlist.size())
            writer->writeDouble(41, ent->weightlist[i]);
    }
    for (const auto& crd : ent->fitlist) {
        writer->writeDouble(11, crd->x);
        writer->writeDouble(21, crd->y);
        writer->writeDouble(31, crd->z);
    }
    // Start/end tangent vectors (fit-point splines, codes 12/22/32 and 13/23/33)
    if (ent->tgStart.x != 0.0 || ent->tgStart.y != 0.0 || ent->tgStart.z != 0.0) {
        writer->writeDouble(12, ent->tgStart.x);
        writer->writeDouble(22, ent->tgStart.y);
        writer->writeDouble(32, ent->tgStart.z);
    }
    if (ent->tgEnd.x != 0.0 || ent->tgEnd.y != 0.0 || ent->tgEnd.z != 0.0) {
        writer->writeDouble(13, ent->tgEnd.x);
        writer->writeDouble(23, ent->tgEnd.y);
        writer->writeDouble(33, ent->tgEnd.z);
    }
}

std::size_t tableColumnCount(const DRW_Table& table) {
    std::size_t columns = table.m_content.m_columns.size();
    for (const auto& row : table.m_content.m_rows)
        columns = std::max(columns, row.m_cells.size());
    return columns;
}

UTF8STRING tableCellText(const DRW_TableCell& cell) {
    for (const DRW_TableCellContent& content : cell.m_contents) {
        if (!content.m_text.empty())
            return content.m_text;
        if (!content.m_value.m_valueString.empty())
            return content.m_value.m_valueString;
        if (content.m_value.m_value.type() == DRW_Variant::STRING)
            return content.m_value.m_value.c_str();
    }
    return UTF8STRING();
}

const char *modelerGeometryDxfName(DRW::ETYPE type) {
    switch (type) {
    case DRW::E3DSOLID:
        return "3DSOLID";
    case DRW::REGION:
        return "REGION";
    case DRW::BODY:
        return "BODY";
    default:
        return nullptr;
    }
}

const char *modelerGeometryDxfSubclass(DRW::ETYPE type) {
    switch (type) {
    case DRW::E3DSOLID:
        return "AcDb3dSolid";
    case DRW::REGION:
        return "AcDbRegion";
    case DRW::BODY:
        return "AcDbBody";
    default:
        return nullptr;
    }
}

bool isTextAcisPayload(const std::vector<std::uint8_t>& data) {
    return std::all_of(data.begin(), data.end(), [](std::uint8_t byte) {
        return byte == '\n' || byte == '\r' || byte == '\t' ||
               (byte >= 0x20 && byte < 0x7f);
    });
}

void writeDxfTextChunks(dxfWriter *writer, const std::vector<std::uint8_t>& data) {
    const std::string text(data.begin(), data.end());
    constexpr std::size_t kChunkSize = 255;
    for (std::size_t off = 0; off < text.size(); off += kChunkSize) {
        const std::size_t n = std::min(kChunkSize, text.size() - off);
        writer->writeString(off == 0 ? 1 : 3, text.substr(off, n));
    }
}

void writeDxfBinaryChunks(dxfWriter *writer, const std::vector<std::uint8_t>& data) {
    static const char hexd[] = "0123456789ABCDEF";
    constexpr std::size_t kChunkBytes = 127;
    for (std::size_t off = 0; off < data.size(); off += kChunkBytes) {
        const std::size_t n = std::min(kChunkBytes, data.size() - off);
        std::string chunk;
        chunk.reserve(n * 2);
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint8_t byte = data[off + i];
            chunk.push_back(hexd[byte >> 4]);
            chunk.push_back(hexd[byte & 0x0F]);
        }
        writer->writeString(310, chunk);
    }
}

bool appendDxfHexChunk(const std::string& text,
                       std::vector<std::uint8_t>& target) {
    if (target.size() > DRW::kMaxDxfBinaryPayloadBytes
        || (text.size() & 1u) != 0
        || text.size() / 2u >
               DRW::kMaxDxfBinaryPayloadBytes - target.size()) {
        return false;
    }
    const auto hexDigit = [](char ch) -> int {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < text.size(); i += 2) {
        const int high = hexDigit(text[i]);
        const int low = hexDigit(text[i + 1]);
        if (high < 0 || low < 0)
            return false;
    }
    const std::size_t newSize = target.size() + text.size() / 2u;
    if (newSize > static_cast<std::size_t>(std::numeric_limits<int>::max())
        || !DRW::reserve(target, static_cast<int>(newSize)))
        return false;
    for (std::size_t i = 0; i < text.size(); i += 2) {
        const int high = hexDigit(text[i]);
        const int low = hexDigit(text[i + 1]);
        target.push_back(static_cast<std::uint8_t>((high << 4) | low));
    }
    return true;
}

// Keep BLOCK child callbacks out of the public interface until the complete
// child sequence and its ENDBLK footer have been validated. Values are copied
// because most entity processors use stack-local objects and child pointers.
class DxfBlockEventSink final : public DRW_Interface {
public:
    explicit DxfBlockEventSink(DRW_Interface& target) : m_target(target) {}

    bool failed() const noexcept { return m_failed; }

    bool flush() {
        if (m_failed)
            return false;
        for (auto& event : m_events)
            event(m_target);
        return !m_failed;
    }

    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(int) override {}
    void endBlock() override {}

#define DRW_BLOCK_EVENT_VALUE(method, type) \
    void method(const type& data) override { \
        recordValue(data, [](DRW_Interface& target, const type& value) { \
            target.method(value); \
        }); \
    }

#define DRW_BLOCK_EVENT_POINTER(method, type) \
    void method(const type* data) override { \
        recordPointer(data, [](DRW_Interface& target, const type* value) { \
            target.method(value); \
        }); \
    }

    DRW_BLOCK_EVENT_VALUE(addPoint, DRW_Point)
    DRW_BLOCK_EVENT_VALUE(addLine, DRW_Line)
    DRW_BLOCK_EVENT_VALUE(add3DLine, DRW_3DLine)
    DRW_BLOCK_EVENT_VALUE(addRay, DRW_Ray)
    DRW_BLOCK_EVENT_VALUE(addXline, DRW_Xline)
    DRW_BLOCK_EVENT_VALUE(addArc, DRW_Arc)
    DRW_BLOCK_EVENT_VALUE(addCircle, DRW_Circle)
    DRW_BLOCK_EVENT_VALUE(addEllipse, DRW_Ellipse)
    DRW_BLOCK_EVENT_VALUE(addLWPolyline, DRW_LWPolyline)
    DRW_BLOCK_EVENT_POINTER(addMLine, DRW_MLine)
    DRW_BLOCK_EVENT_POINTER(addUnderlay, DRW_Underlay)
    DRW_BLOCK_EVENT_VALUE(addShape, DRW_Shape)
    DRW_BLOCK_EVENT_VALUE(addOle2Frame, DRW_Ole2Frame)
    DRW_BLOCK_EVENT_VALUE(addOleFrame, DRW_OleFrame)
    DRW_BLOCK_EVENT_VALUE(addProxyEntity, DRW_ProxyEntity)
    DRW_BLOCK_EVENT_VALUE(addPolyline, DRW_Polyline)
    DRW_BLOCK_EVENT_POINTER(addSpline, DRW_Spline)
    DRW_BLOCK_EVENT_POINTER(addHelix, DRW_Helix)
    DRW_BLOCK_EVENT_VALUE(addMesh, DRW_Mesh)
    void addKnot(const DRW_Entity&) override {}
    DRW_BLOCK_EVENT_VALUE(addInsert, DRW_Insert)
    DRW_BLOCK_EVENT_VALUE(addTable, DRW_Table)
    DRW_BLOCK_EVENT_VALUE(addTrace, DRW_Trace)
    DRW_BLOCK_EVENT_VALUE(add3dFace, DRW_3Dface)
    DRW_BLOCK_EVENT_VALUE(addSolid, DRW_Solid)
    DRW_BLOCK_EVENT_VALUE(addMText, DRW_MText)
    DRW_BLOCK_EVENT_VALUE(addText, DRW_Text)
    DRW_BLOCK_EVENT_VALUE(addAttDef, DRW_Attdef)
    DRW_BLOCK_EVENT_POINTER(addDimAlign, DRW_DimAligned)
    DRW_BLOCK_EVENT_POINTER(addDimLinear, DRW_DimLinear)
    DRW_BLOCK_EVENT_POINTER(addDimRadial, DRW_DimRadial)
    DRW_BLOCK_EVENT_POINTER(addDimDiametric, DRW_DimDiametric)
    DRW_BLOCK_EVENT_POINTER(addDimAngular, DRW_DimAngular)
    DRW_BLOCK_EVENT_POINTER(addDimAngular3P, DRW_DimAngular3p)
    DRW_BLOCK_EVENT_POINTER(addDimOrdinate, DRW_DimOrdinate)
    DRW_BLOCK_EVENT_POINTER(addDimArc, DRW_DimArc)
    DRW_BLOCK_EVENT_POINTER(addLeader, DRW_Leader)
    DRW_BLOCK_EVENT_POINTER(addHatch, DRW_Hatch)
    DRW_BLOCK_EVENT_POINTER(addMPolygon, DRW_MPolygon)
    DRW_BLOCK_EVENT_VALUE(addViewport, DRW_Viewport)
    DRW_BLOCK_EVENT_POINTER(addImage, DRW_Image)
    DRW_BLOCK_EVENT_POINTER(addWipeout, DRW_Wipeout)
    DRW_BLOCK_EVENT_POINTER(addPointCloud, DRW_PointCloud)
    DRW_BLOCK_EVENT_POINTER(addPointCloudEx, DRW_PointCloudEx)
    DRW_BLOCK_EVENT_POINTER(addNavisworksModel, DRW_NavisworksModel)
    DRW_BLOCK_EVENT_POINTER(addSurface, DRW_Surface)
    DRW_BLOCK_EVENT_POINTER(addMLeader, DRW_MLeader)
    DRW_BLOCK_EVENT_VALUE(addModelerGeometry, DRW_ModelerGeometry)
    DRW_BLOCK_EVENT_VALUE(addRawDxfEntity, DRW_RawDxfObject)

#undef DRW_BLOCK_EVENT_POINTER
#undef DRW_BLOCK_EVENT_VALUE

    void linkImage(const DRW_ImageDef*) override {}
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

private:
    using Event = std::function<void(DRW_Interface&)>;

    template <typename T, typename Callback>
    void recordValue(const T& data, Callback callback) {
        if (m_failed)
            return;
        try {
            auto copy = std::make_shared<T>(data);
            m_events.emplace_back(
                [copy = std::move(copy), callback](DRW_Interface& target) {
                    callback(target, *copy);
                });
        } catch (...) {
            m_failed = true;
        }
    }

    template <typename T, typename Callback>
    void recordPointer(const T* data, Callback callback) {
        if (m_failed)
            return;
        try {
            std::shared_ptr<T> copy;
            if (data != nullptr)
                copy = std::make_shared<T>(*data);
            m_events.emplace_back(
                [copy = std::move(copy), callback](DRW_Interface& target) {
                    callback(target, copy.get());
                });
        } catch (...) {
            m_failed = true;
        }
    }

    DRW_Interface& m_target;
    std::vector<Event> m_events;
    bool m_failed {false};
};

}

class dxfRW::RecordStateScope final {
public:
    explicit RecordStateScope(dxfRW& owner, DRW_TableEntry *entry = nullptr)
        : m_owner(owner), m_entity(nullptr), m_entry(entry),
          m_handle(entry != nullptr ? entry->handle : 0),
          m_name(entry != nullptr ? entry->name : std::string()),
          m_mutationCheckpoint(owner.m_dxfWriteMutations.size()),
          m_lineTypesBefore(owner.m_writingContext.lineTypesMap.size()),
          m_pendingBlockRecordsBefore(owner.m_pendingBlockRecords.size()),
          m_wlayer0Before(owner.wlayer0),
          m_dimstyleStdBefore(owner.dimstyleStd),
          m_writingBlockBefore(owner.writingBlock),
          m_collectingBlockRecordsBefore(owner.m_collectingBlockRecords),
          m_imageDefBefore(owner.imageDef.size()),
          m_currHandleBefore(owner.currHandle) {
        ++m_owner.m_recordStateScopeDepth;
    }

    explicit RecordStateScope(dxfRW& owner, DRW_Entity *entity)
        : m_owner(owner), m_entity(entity), m_entry(nullptr),
          m_handle(entity != nullptr ? entity->handle : 0),
          m_name(),
          m_mutationCheckpoint(owner.m_dxfWriteMutations.size()),
          m_lineTypesBefore(owner.m_writingContext.lineTypesMap.size()),
          m_pendingBlockRecordsBefore(owner.m_pendingBlockRecords.size()),
          m_wlayer0Before(owner.wlayer0),
          m_dimstyleStdBefore(owner.dimstyleStd),
          m_writingBlockBefore(owner.writingBlock),
          m_collectingBlockRecordsBefore(owner.m_collectingBlockRecords),
          m_imageDefBefore(owner.imageDef.size()),
          m_currHandleBefore(owner.currHandle) {
        ++m_owner.m_recordStateScopeDepth;
    }

    RecordStateScope(const RecordStateScope&) = delete;
    RecordStateScope& operator=(const RecordStateScope&) = delete;

    ~RecordStateScope() {
        rollback();
        --m_owner.m_recordStateScopeDepth;
    }

    void commit() noexcept { m_committed = true; }

private:
    void rollback() noexcept {
        if (m_committed)
            return;
        if (m_entry != nullptr) {
            m_entry->handle = m_handle;
            using std::swap;
            swap(m_entry->name, m_name);
        }
        if (m_entity != nullptr)
            m_entity->handle = m_handle;
        while (m_owner.m_dxfWriteMutations.size() > m_mutationCheckpoint) {
            const DxfWriteMutation& mutation =
                m_owner.m_dxfWriteMutations.back();
            switch (mutation.kind) {
            case DxfWriteMutationKind::BlockMapInsert:
                m_owner.blockMap.erase(mutation.key);
                break;
            case DxfWriteMutationKind::TextStyleMapSet: {
                const auto it = m_owner.textStyleMap.find(mutation.key);
                if (mutation.hadPrevious) {
                    if (it != m_owner.textStyleMap.end())
                        it->second = mutation.previousHandle;
                } else {
                    m_owner.textStyleMap.erase(mutation.key);
                }
                break;
            }
            case DxfWriteMutationKind::SourceHandleInsert:
                m_owner.m_writingContext.sourceHandleToMintedMap.erase(
                    mutation.handle);
                break;
            case DxfWriteMutationKind::SourceHandleErase:
                m_owner.m_writingContext.sourceHandleToMintedMap.emplace(
                    mutation.handle, mutation.previousHandle);
                break;
            case DxfWriteMutationKind::AmbiguousSourceHandleInsert:
                m_owner.m_writingContext.ambiguousSourceHandles.erase(
                    mutation.handle);
                break;
            case DxfWriteMutationKind::ImageReactorInsert:
                if (mutation.imageDef != nullptr)
                    mutation.imageDef->reactors.erase(mutation.key);
                break;
            }
            m_owner.m_dxfWriteMutations.pop_back();
        }
        m_owner.m_writingContext.lineTypesMap.resize(m_lineTypesBefore);
        m_owner.m_pendingBlockRecords.resize(m_pendingBlockRecordsBefore);
        m_owner.wlayer0 = m_wlayer0Before;
        m_owner.dimstyleStd = m_dimstyleStdBefore;
        m_owner.writingBlock = m_writingBlockBefore;
        m_owner.m_collectingBlockRecords = m_collectingBlockRecordsBefore;
        while (m_owner.imageDef.size() > m_imageDefBefore) {
            delete m_owner.imageDef.back();
            m_owner.imageDef.pop_back();
        }
        m_owner.currHandle = m_currHandleBefore;
        m_committed = true;
    }

    dxfRW& m_owner;
    DRW_Entity *m_entity;
    DRW_TableEntry *m_entry;
    std::uint32_t m_handle;
    std::string m_name;
    std::size_t m_mutationCheckpoint;
    std::size_t m_lineTypesBefore;
    std::size_t m_pendingBlockRecordsBefore;
    bool m_wlayer0Before;
    bool m_dimstyleStdBefore;
    bool m_writingBlockBefore;
    bool m_collectingBlockRecordsBefore;
    std::size_t m_imageDefBefore;
    std::uint32_t m_currHandleBefore;
    bool m_committed {false};
};

// Ordinary entity writers historically emitted directly into the parent
// stream, while a small set of compound writers used explicit scopes. Keep
// the ordinary paths uniform: bytes and record-local mutations are committed
// together only when the function leaves a clean writer state. The explicit
// commit is also needed by APIs returning an object owned by the write session:
// callers must never receive a pointer that a scope destructor can roll back.
class dxfRW::EntityRecordScope final {
public:
    EntityRecordScope(dxfRW& owner, DRW_Entity *entity)
        : m_owner(owner), m_writer(owner.writer.get()), m_state(owner, entity),
          m_record(*owner.writer) {}

    EntityRecordScope(dxfRW& owner, DRW_TableEntry *entry)
        : m_owner(owner), m_writer(owner.writer.get()), m_state(owner, entry),
          m_record(*owner.writer) {}

    EntityRecordScope(const EntityRecordScope&) = delete;
    EntityRecordScope& operator=(const EntityRecordScope&) = delete;

    bool commit() noexcept {
        if (m_finished)
            return m_succeeded;
        m_finished = true;
        if (m_owner.m_writeError || m_writer == nullptr
            || m_writer->hasWriteError()) {
            return false;
        }
        try {
            if (!m_record.commit()) {
                m_owner.m_writeError = true;
                return false;
            }
            m_state.commit();
            m_succeeded = true;
            return true;
        } catch (...) {
            m_owner.m_writeError = true;
            return false;
        }
    }

    ~EntityRecordScope() noexcept {
        commit();
    }

private:
    dxfRW& m_owner;
    dxfWriter *m_writer;
    RecordStateScope m_state;
    DxfWriterRecordScope m_record;
    bool m_finished {false};
    bool m_succeeded {false};
};

/*enum sections {
    secUnknown,
    secHeader,
    secTables,
    secBlocks,
    secEntities,
    secObjects
};*/

dxfRW::dxfRW(const char* name){
    DRW_DBGSL(DRW_dbg::Level::None);
    fileName = name;
}


dxfRW::~dxfRW(){
    for (std::vector<DRW_ImageDef*>::iterator it=imageDef.begin(); it!=imageDef.end(); ++it)
        delete *it;

    imageDef.clear();
}

void dxfRW::setDebug(DRW::DebugLevel lvl){
    switch (lvl){
    case DRW::DebugLevel::Debug:
        DRW_DBGSL(DRW_dbg::Level::Debug);
        break;
    case DRW::DebugLevel::None:
        DRW_DBGSL(DRW_dbg::Level::None);
    }
}

bool dxfRW::read(DRW_Interface *interface_, bool ext){
    drw_assert(fileName.empty() == false);
    version = DRW::UNKNOWNV;
    error = DRW::BAD_NONE;
    nextentity.clear();
    m_hasPendingEntityBoundary = false;
    m_readingBlockEntities = false;
    m_readingContext.lineTypeNameMap.clear();
    m_readingContext.blockRecordMap.clear();
    m_readRawHandles.clear();
    applyExt = ext;
    std::ifstream filestr;
    if (nullptr == interface_) {
        return setError(DRW::BAD_UNKNOWN);
    }
    DRW_DBG("dxfRW::read 1def\n");
    filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
    if (!filestr.is_open()
        || !filestr.good()) {
        return setError(DRW::BAD_OPEN);
    }

    char line[24]{};
    char line2[22] = "AutoCAD Binary DXF\r\n";
    line2[20] = (char)26;
    line2[21] = '\0';
    // Read the 22-byte sentinel plus the first 2 bytes of the group stream so
    // the binary sub-format can be detected before any group is parsed.
    filestr.read (line, 24);
    filestr.close();
    iface = interface_;
    DRW_DBG("dxfRW::read 2\n");
    // `line` is filled by an unterminated read; compare the sentinel by exact
    // length to avoid strcmp reading past the buffer when the sentinel
    // bytes don't include an embedded NUL.
    if (std::memcmp(line, line2, 22) == 0) {
        filestr.open (fileName.c_str(), std::ios_base::in | std::ios::binary);
        if (!filestr.is_open() || !filestr.good())
            return setError(DRW::BAD_OPEN);
        binFile = true;
        //skip sentinel
        filestr.seekg (22, std::ios::beg);
        if (!filestr.good())
            return setError(DRW::BAD_OPEN);
        // R12/AC1009 binary uses 1-byte group codes; R13+ uses 2-byte LE. The
        // first group is always code 0 (SECTION): R12 => bytes 00 'S' (byte[1]
        // != 0); R13+ => bytes 00 00. So a non-zero second byte selects the
        // 1-byte reader.
        if (static_cast<unsigned char>(line[23]) != 0)
            reader = std::make_unique<dxfReaderBinaryR12>(&filestr);
        else
            reader = std::make_unique<dxfReaderBinary>(&filestr);
        DRW_DBG("dxfRW::read binary file\n");
    } else {
        binFile = false;
        filestr.open (fileName.c_str(), std::ios_base::in);
        if (!filestr.is_open() || !filestr.good())
            return setError(DRW::BAD_OPEN);
        reader = std::make_unique<dxfReaderAscii>(&filestr);
    }

    bool isOk {processDxf()};
    filestr.close();
    version = (DRW::Version) reader->getVersion();
    reader.reset();
    return isOk;
}


bool dxfRW::readAscii(DRW_Interface *interface_, bool ext, std::string& content) {
    if (nullptr == interface_) {
        return setError(DRW::BAD_UNKNOWN);
    }
    version = DRW::UNKNOWNV;
    error = DRW::BAD_NONE;
    nextentity.clear();
    m_hasPendingEntityBoundary = false;
    m_readingBlockEntities = false;
    m_readingContext.lineTypeNameMap.clear();
    m_readingContext.blockRecordMap.clear();
    m_readRawHandles.clear();
    applyExt = ext;
    iface = interface_;
    std::istringstream strstream(content);
    reader = std::make_unique<dxfReaderAscii>(&strstream);
    bool isOk {processDxf()};
    version = (DRW::Version) reader->getVersion();
    reader.reset();
    return isOk;
}

std::uint32_t dxfRW::getBlockRecordHandleToWrite(const std::string& blockName) const {
    auto it = blockMap.find(dxfSymbolNameKey(blockName));
    return (it != blockMap.end()) ? it->second : DRW::NoHandle;
}

std::uint32_t dxfRW::preallocateEntityHandle(std::uint32_t sourceHandle) {
    if (sourceHandle == 0 || writer == nullptr
        || m_writingContext.ambiguousSourceHandles.count(sourceHandle) != 0)
        return 0;
    const auto existing = m_writingContext.sourceHandleToMintedMap.find(
        sourceHandle);
    if (existing != m_writingContext.sourceHandleToMintedMap.end())
        return existing->second;
    std::uint32_t emittedHandle = 0;
    if (!allocateDxfHandle(emittedHandle))
        return 0;
    const std::size_t mutationCheckpoint = m_dxfWriteMutations.size();
    if (m_recordStateScopeDepth != 0) {
        m_dxfWriteMutations.push_back({
            DxfWriteMutationKind::SourceHandleInsert, {}, sourceHandle,
            emittedHandle, false});
    }
    try {
        const auto inserted = m_writingContext.sourceHandleToMintedMap.emplace(
            sourceHandle, emittedHandle);
        if (!inserted.second)
            m_dxfWriteMutations.resize(mutationCheckpoint);
    } catch (...) {
        m_dxfWriteMutations.resize(mutationCheckpoint);
        throw;
    }
    return emittedHandle;
}

bool dxfRW::allocateDxfHandle(std::uint32_t& handle) noexcept {
    handle = 0;
    try {
        handle = m_handleAllocator.next();
    } catch (...) {
        m_writeError = true;
        return false;
    }
    return handle != 0;
}

std::uint32_t dxfRW::allocHandle() {
    std::uint32_t handle = 0;
    return allocateDxfHandle(handle) ? handle : 0;
}

bool dxfRW::bindSourceEntityHandle(std::uint32_t sourceHandle,
                                   std::uint32_t emittedHandle) {
    if (sourceHandle == 0 || emittedHandle == 0 || writer == nullptr
        || m_writingContext.ambiguousSourceHandles.count(sourceHandle) != 0)
        return false;

    const auto existing = m_writingContext.sourceHandleToMintedMap.find(
        sourceHandle);
    if (existing == m_writingContext.sourceHandleToMintedMap.end()) {
        const std::size_t mutationCheckpoint = m_dxfWriteMutations.size();
        if (m_recordStateScopeDepth != 0) {
            m_dxfWriteMutations.push_back({
                DxfWriteMutationKind::SourceHandleInsert, {}, sourceHandle,
                emittedHandle, false});
        }
        try {
            const auto inserted =
                m_writingContext.sourceHandleToMintedMap.emplace(
                    sourceHandle, emittedHandle);
            if (!inserted.second)
                m_dxfWriteMutations.resize(mutationCheckpoint);
        } catch (...) {
            m_dxfWriteMutations.resize(mutationCheckpoint);
            throw;
        }
        return true;
    }
    if (existing->second == emittedHandle)
        return true;

    // A source handle is a global identity. Once two output handles claim it,
    // remove the key so deferred references cannot silently choose one.
    const std::uint32_t previousHandle = existing->second;
    const std::size_t mutationCheckpoint = m_dxfWriteMutations.size();
    if (m_recordStateScopeDepth != 0) {
        m_dxfWriteMutations.push_back({
            DxfWriteMutationKind::SourceHandleErase, {}, sourceHandle,
            previousHandle, true});
        m_dxfWriteMutations.push_back({
            DxfWriteMutationKind::AmbiguousSourceHandleInsert, {},
            sourceHandle, 0, false});
    }
    try {
        m_writingContext.sourceHandleToMintedMap.erase(existing);
        const auto inserted =
            m_writingContext.ambiguousSourceHandles.insert(sourceHandle);
        if (!inserted.second) {
            m_writingContext.sourceHandleToMintedMap.emplace(
                sourceHandle, previousHandle);
            m_dxfWriteMutations.resize(mutationCheckpoint);
        }
    } catch (...) {
        m_writingContext.sourceHandleToMintedMap.emplace(
            sourceHandle, previousHandle);
        m_dxfWriteMutations.resize(mutationCheckpoint);
        throw;
    }
    return false;
}

void dxfRW::markSourceHandleAmbiguous(std::uint32_t sourceHandle) {
    if (sourceHandle == 0)
        return;
    const std::size_t mutationCheckpoint = m_dxfWriteMutations.size();
    if (m_recordStateScopeDepth != 0) {
        m_dxfWriteMutations.push_back({
            DxfWriteMutationKind::AmbiguousSourceHandleInsert, {},
            sourceHandle, 0, false});
    }
    try {
        const auto inserted =
            m_writingContext.ambiguousSourceHandles.insert(sourceHandle);
        if (!inserted.second)
            m_dxfWriteMutations.resize(mutationCheckpoint);
    } catch (...) {
        m_dxfWriteMutations.resize(mutationCheckpoint);
        throw;
    }
}

std::uint32_t dxfRW::getTextStyleHandle(const std::string& styleName) const {
    if (!styleName.empty()) {
        auto it = textStyleMap.find(dxfSymbolNameKey(styleName));
        if (it != textStyleMap.end()) return it->second;
    }
    return DRW::NoHandle;
}

void dxfRW::resetDxfWriteSession() {
    // Reset only state derived from the current callback traversal. Persistent
    // inputs (classes, raw sections, dictionaries, and groups) remain available
    // for a retry or a later versioned write.
    m_writeError = false;
    m_handleReservationFailed = false;
    error = DRW::BAD_NONE;
    m_handleAllocator.resetGenerated();
    m_dxfWriteMutations.clear();
    // Image definitions are generated by the preceding callback traversal,
    // not persistent input. A failed attempt leaves them owned here; discard
    // them before a retry so reactor entries cannot accumulate across writes.
    for (DRW_ImageDef *id : imageDef)
        delete id;
    imageDef.clear();
    blockMap.clear();
    m_writingContext.lineTypesMap.clear();
    textStyleMap.clear();
    m_pendingBlockRecords.clear();
    m_collectingBlockRecords = false;
    writingBlock = false;
    currHandle = DRW::NoHandle;
    m_writingContext.sourceHandleToMintedMap.clear();
    m_writingContext.ambiguousSourceHandles.clear();
    m_dxfClassesFrozen = false;
}

bool dxfRW::write(DRW_Interface *interface_, DRW::Version ver, bool bin) try {
    if (!isSupportedDxfWriteVersion(ver)) {
        error = DRW::BAD_VERSION;
        return false;
    }
    if (interface_ == nullptr) {
        error = DRW::BAD_OPEN;
        return false;
    }
    if (m_reservationFailureGeneration
        != m_consumedReservationFailureGeneration) {
        m_consumedReservationFailureGeneration =
            m_reservationFailureGeneration;
        error = DRW::BAD_OPEN;
        return false;
    }
    resetDxfWriteSession();
    version = ver;
    binFile = bin;
    iface = interface_;
    DwgDxfOutputTransaction output(
        fileName, binFile ? std::ios::binary : std::ios::openmode(0));
    if (!output.open()) {
        m_writeError = true;
        error = DRW::BAD_OPEN;
        return false;
    }
    std::ofstream& filestr = output.stream();
    const auto failWrite = [&]() {
        writer.reset();
        output.abort();
        m_writeError = true;
        error = DRW::BAD_OPEN;
        return false;
    };
    if (binFile) {
        //write sentinel
        filestr << "AutoCAD Binary DXF\r\n" << (char)26 << '\0';
        if (!filestr.good())
            return failWrite();
        if (version <= DRW::AC1009)
            writer = std::make_unique<dxfWriterBinaryR12>(&filestr);
        else
        writer = std::make_unique<dxfWriterBinary>(&filestr);
        DRW_DBG("dxfRW::read binary file\n");
    } else {
        writer = std::make_unique<dxfWriterAscii>(&filestr);
        std::string comm = std::string("dxfrw ") + std::string(DRW_VERSION);
        if (!writer->writeString(999, comm))
            return failWrite();
    }
    //Reserve the codec's fixed structural code-5 literals (table heads, mandatory
    //records, BLOCK_RECORDs, *Model/*Paper BLOCK+ENDBLK, root dict C / ACAD_GROUP
    //D) so the minted-handle stream (m_handleAllocator.next()) skips them. Any raw
    //code-5 handle preserved by the filter was already reserve()d before write(),
    //so a re-emitted raw OBJECT/ENTITY can collide with neither a minted handle
    //nor a fixed-low structural handle. The first next() yields FIRSTHANDLE (0x30)
    //exactly as the legacy ++entCount did, keeping a fresh write byte-identical.
    seedReservedDxf();
    if (!preflightDxfClasses())
        return failWrite();

    if (version > DRW::AC1009) {
        // BLOCK_RECORD handles can be referenced by HEADER and DIMSTYLE
        // records, both of which precede the physical BLOCK_RECORD table.
        // Collect all records before streaming either dependent section.
        RecordStateScope blockRecordState(*this);
        m_collectingBlockRecords = true;
        iface->writeBlockRecords();
        m_collectingBlockRecords = false;
        if (m_writeError || writer->hasWriteError()) {
            return failWrite();
        }
        blockRecordState.commit();
    }

    DRW_Header header;
    iface->writeHeader(header);
    if (!writeRequiredString(0, "SECTION"))
        return failWrite();
    header.write(writer, version);
    if (writer->hasWriteError())
        return failWrite();
    if (!writeRequiredString(0, "ENDSEC"))
        return failWrite();
    if (ver > DRW::AC1009) {
        if (!writeRequiredString(0, "SECTION")
            || !writeRequiredString(2, "CLASSES"))
            return failWrite();
        //Emit a CLASS record for each custom (non-fixed) object class actually
        //present in the output. Without these, AutoCAD/ODA silently drop the
        //corresponding OBJECTS instances (the entry and instance must co-exist).
        //The filter registers them from the raw-net objects before write().
        for (const DRW_Class &cls : m_dxfClasses) {
            if (!cls.write(writer.get(), version) || writer->hasWriteError())
                return failWrite();
        }
        if (!writeRequiredString(0, "ENDSEC"))
            return failWrite();
    }
    if (!writeRequiredString(0, "SECTION")
        || !writeRequiredString(2, "TABLES"))
        return failWrite();
    if (!writeTables() || m_writeError || writer->hasWriteError())
        return failWrite();
    if (!writeRequiredString(0, "ENDSEC")
        || !writeRequiredString(0, "SECTION")
        || !writeRequiredString(2, "BLOCKS"))
        return failWrite();
    if (!writeBlocks() || m_writeError || writer->hasWriteError())
        return failWrite();
    if (!writeRequiredString(0, "ENDSEC"))
        return failWrite();

    if (!writeRequiredString(0, "SECTION")
        || !writeRequiredString(2, "ENTITIES"))
        return failWrite();
    iface->writeEntities();
    if (m_writeError || writer->hasWriteError())
        return failWrite();
    if (!writeRequiredString(0, "ENDSEC"))
        return failWrite();

    if (version > DRW::AC1009) {
        if (!writeRequiredString(0, "SECTION")
            || !writeRequiredString(2, "OBJECTS"))
            return failWrite();
        if (!writeObjects() || writer->hasWriteError())
            return failWrite();
        if (!writeRequiredString(0, "ENDSEC"))
            return failWrite();
    }
    for (const DRW_RawDxfSection &section : m_rawDxfSections) {
        if (!writeRawDxfSection(section) || writer->hasWriteError())
            return failWrite();
    }
    if (!writeRequiredString(0, "EOF"))
        return failWrite();
    // Back-patch $HANDSEED with the final handle high-water mark. The header was
    // streamed first (before any table/block/entity/object handle was minted),
    // so it wrote a fixed-width placeholder and recorded the value-field offset.
    // Now that the whole body is written, m_handleAllocator.current() is one past
    // the largest handle reserved or minted, i.e. strictly above every emitted
    // code-5 handle — exactly what a $HANDSEED needs to be.
    if (header.m_handseedValueOffset != std::streampos(-1)) {
        std::uint32_t seed = highWaterHandle();
        char buf[DRW_Header::kHandseedFieldWidth + 1];
        snprintf(buf, sizeof(buf), "%0*X",
                 DRW_Header::kHandseedFieldWidth, seed);
        std::streampos resume = filestr.tellp();
        if (resume == std::streampos(-1))
            return failWrite();
        filestr.seekp(header.m_handseedValueOffset);
        if (!filestr.good())
            return failWrite();
        filestr.write(buf, DRW_Header::kHandseedFieldWidth);
        if (!filestr.good())
            return failWrite();
        filestr.seekp(resume);
        if (!filestr.good())
            return failWrite();
    }
    filestr.flush();
    const bool isOk = filestr.good() && !writer->hasWriteError();
    writer.reset();
    if (!isOk)
        return failWrite();
    if (!output.commit())
        return failWrite();
    return true;
}
catch (const std::exception&) {
    writer.reset();
    m_writeError = true;
    error = DRW::BAD_OPEN;
    return false;
}
catch (...) {
    writer.reset();
    m_writeError = true;
    error = DRW::BAD_OPEN;
    return false;
}

void dxfRW::seedReservedDxf() {
    // Fixed structural code-5 handles the codec writes as literals (see
    // writeTables/writeBlocks/writeBlockRecord/writeObjects). These DIFFER from
    // the DWG seedReserved() set — they are the DXF codec's own canonical values
    // and stay verbatim. Reserving them up front lets m_handleAllocator.next()
    // skip them while preserving FIRSTHANDLE (0x30) as the first minted handle.
    static const std::uint32_t fixed[] = {
        0x1,   // BLOCK_RECORD table head
        0x2,   // LAYER table head
        0x3,   // STYLE table head
        0x5,   // LTYPE table head
        0x6,   // VIEW table head
        0x7,   // UCS table head
        0x8,   // VPORT table head
        0x9,   // APPID table head
        0xA,   // DIMSTYLE table head
        0xC,   // NamedObjectsDictionary (root dict)
        0xD,   // ACAD_GROUP dictionary
        0x10,  // LAYER "0"
        0x12,  // APPID "ACAD"
        0x14,  // LTYPE "ByBlock"
        0x15,  // LTYPE "ByLayer"
        0x16,  // LTYPE "Continuous"
        0x1C,  // BLOCK "*Paper_Space"
        0x1D,  // ENDBLK "*Paper_Space"
        0x1E,  // BLOCK_RECORD "*Paper_Space"
        0x1F,  // BLOCK_RECORD "*Model_Space"
        0x20,  // BLOCK "*Model_Space"
        0x21,  // ENDBLK "*Model_Space"
    };
    for (std::uint32_t h : fixed)
        m_handleAllocator.reserve(h);
}

bool dxfRW::writeRequiredString(int code, const std::string& value) {
    if (writer == nullptr || m_writeError || !writer->writeString(code, value)) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::preflightEntity(const DRW_Entity *ent) {
    if (ent == nullptr || writer == nullptr || writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    if (!isValidDxfEntityFields(*ent)) {
        m_writeError = true;
        return false;
    }
    if (version >= DRW::AC1014 && !isValidDxfAppData(ent->appData)) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::preflightDxfClasses() {
    // R12 has no CLASSES section; retain the supplied metadata for a later
    // version rather than rejecting an otherwise valid legacy export.
    if (version <= DRW::AC1009) {
        m_dxfClassesFrozen = false;
        return true;
    }

    std::unordered_set<std::string> recordNames;
    std::unordered_set<std::string> classNames;
    try {
        recordNames.reserve(m_dxfClasses.size());
        classNames.reserve(m_dxfClasses.size());
        for (const DRW_Class& cls : m_dxfClasses) {
            if (!isSafeDxfClassMetadata(cls)) {
                m_writeError = true;
                return false;
            }
            if (!cls.recName.empty()
                && !recordNames.insert(dxfSymbolNameKey(cls.recName)).second) {
                m_writeError = true;
                return false;
            }
            if (!classNames.insert(dxfSymbolNameKey(cls.className)).second) {
                m_writeError = true;
                return false;
            }
        }
    } catch (...) {
        m_writeError = true;
        return false;
    }
    m_dxfClassesFrozen = true;
    return true;
}

bool dxfRW::rejectUnsupportedDxfWrite() noexcept {
    m_writeError = true;
    return false;
}

bool dxfRW::failDxfWrite() noexcept {
    m_writeError = true;
    return false;
}

bool dxfRW::validateHatchPayload(const DRW_Hatch *ent) const {
    const auto validInt16 = [](std::size_t value) {
        return value <= static_cast<std::size_t>(
            std::numeric_limits<std::uint16_t>::max());
    };
    if (ent == nullptr || !ent->validateDxf()
        || !isSafeDxfRecordText(ent->name)
        || !isSafeDxfRecordText(ent->gradName)
        || !isFiniteDxfCoord(ent->basePoint)
        || !isFiniteDxfCoord(ent->extPoint)
        || ent->solid < 0 || ent->solid > 1
        || ent->associative < 0 || ent->associative > 1
        || ent->hstyle < 0
        || ent->hstyle > std::numeric_limits<std::uint16_t>::max()
        || ent->hpattern < 0
        || ent->hpattern > std::numeric_limits<std::uint16_t>::max()
        || ent->doubleflag < 0 || ent->doubleflag > 1
        || !std::isfinite(ent->angle) || !std::isfinite(ent->scale)
        || !std::isfinite(ent->pixelSize)
        || ent->looplist.size() > static_cast<std::size_t>(DRW_Hatch::kMaxDxfItems)
        || ent->patternLines.size() > static_cast<std::size_t>(DRW_Hatch::kMaxDxfItems)
        || ent->seedPoints.size() > static_cast<std::size_t>(DRW_Hatch::kMaxDxfItems)
        || ent->gradColors.size() > static_cast<std::size_t>(DRW_Hatch::kMaxDxfItems)
        || ent->seedPoints.size()
               > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())
        || ent->gradColors.size()
               > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        return false;

    const auto validBoundaryEntity = [](const std::shared_ptr<DRW_Entity>& value) {
        if (!value || !isValidDxfEntityFields(*value))
            return false;
        switch (value->eType) {
        case DRW::LINE:
            return dynamic_cast<const DRW_Line*>(value.get()) != nullptr;
        case DRW::ARC:
            return dynamic_cast<const DRW_Arc*>(value.get()) != nullptr;
        case DRW::ELLIPSE:
            return dynamic_cast<const DRW_Ellipse*>(value.get()) != nullptr;
        case DRW::SPLINE: {
            const auto *spline = dynamic_cast<const DRW_Spline*>(value.get());
            return spline != nullptr
                && spline->validatePayloadFields(/*allowMixedLists=*/true);
        }
        default:
            return false;
        }
    };

    for (const auto& loop : ent->looplist) {
        if (!loop || loop->type < 0
            || loop->type > std::numeric_limits<std::uint16_t>::max()
            || !validInt16(loop->m_boundaryHandles.size())
            || std::any_of(loop->m_boundaryHandles.cbegin(),
                           loop->m_boundaryHandles.cend(),
                           [](std::uint32_t handle) { return handle == 0; }))
            return false;

        if ((loop->type & 2) != 0) {
            if (loop->objlist.size() != 1)
                return false;
            const auto *polyline =
                dynamic_cast<const DRW_LWPolyline*>(loop->objlist.front().get());
            if (polyline == nullptr || !polyline->validatePayloadFields())
                return false;
            continue;
        }

        if (loop->objlist.size() > static_cast<std::size_t>(DRW_Hatch::kMaxDxfItems))
            return false;
        for (const auto& edge : loop->objlist) {
            if (!validBoundaryEntity(edge))
                return false;
        }
    }

    for (const auto& line : ent->patternLines) {
        if (!std::isfinite(line.angle) || !std::isfinite(line.baseX)
            || !std::isfinite(line.baseY) || !std::isfinite(line.offsetX)
            || !std::isfinite(line.offsetY) || !validInt16(line.dashList.size())
            || !std::all_of(line.dashList.cbegin(), line.dashList.cend(),
                            [](double value) { return std::isfinite(value); }))
            return false;
    }
    for (const auto& point : ent->seedPoints) {
        if (!isFiniteDxfCoord(point))
            return false;
    }
    for (const auto& stop : ent->gradColors) {
        if (!std::isfinite(stop.value) || stop.rgb < -1
            || stop.rgb > 0xFFFFFF || stop.aciColor < 0
            || stop.aciColor > std::numeric_limits<std::uint16_t>::max()
            || !isSafeDxfRecordText(stop.colorName)
            || !isSafeDxfRecordText(stop.colorBookName))
            return false;
    }

    if (const auto *polygon = dynamic_cast<const DRW_MPolygon*>(ent)) {
        if (!std::isfinite(polygon->xDirX) || !std::isfinite(polygon->xDirY)
            || polygon->fillColorAci < 0
            || polygon->fillColorAci > std::numeric_limits<std::uint16_t>::max()
            || polygon->fillColorRgb < -1
            || polygon->fillColorRgb > 0xFFFFFF
            || polygon->degenerateLoops < 0
            || !isSafeDxfRecordText(polygon->fillColorName))
            return false;
    }
    return true;
}

bool dxfRW::preflightTableEntry(const DRW_TableEntry *ent) {
    if (ent == nullptr || writer == nullptr || writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    if (version < DRW::AC1014) {
        if (!ent->appData.empty() || !ent->reactorHandles.empty()
            || ent->xDictHandle != 0) {
            m_writeError = true;
            return false;
        }
    } else if (!isValidDxfAppData(ent->appData)) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeEntity(DRW_Entity *ent, bool captureSourceHandle,
                        std::uint32_t ownerOverride) {
    if (!preflightEntity(ent))
        return false;
    // On entry, ent->handle is a SOURCE-handle key seeded by the filter from
    // RS_Entity::sourceHandle() (getEntityAttributes). A unique source may
    // already have a planned handle so BLOCK_RECORD {BLKREFS} can refer to it.
    const std::uint32_t sourceHandle = ent->handle;
    std::uint32_t emittedHandle = 0;
    if (captureSourceHandle && sourceHandle != 0
        && m_writingContext.ambiguousSourceHandles.count(sourceHandle) == 0) {
        const auto planned = m_writingContext.sourceHandleToMintedMap.find(
            sourceHandle);
        if (planned != m_writingContext.sourceHandleToMintedMap.end())
            emittedHandle = planned->second;
    }
    if (emittedHandle == 0) {
        if (!allocateDxfHandle(emittedHandle))
            return false;
    }
    ent->handle = emittedHandle;
    const auto hasAppGroup = [ent](const char *name) {
        for (const auto &group : ent->appData) {
            if (group.empty())
                continue;
            const DRW_Variant &opener = group.front();
            if (opener.code() != 102
                || opener.type() != DRW_Variant::STRING)
                continue;
            const std::string marker = opener.content.s
                ? *opener.content.s
                : std::string{};
            const std::string normalized =
                (!marker.empty() && marker.front() == '{')
                    ? marker.substr(1)
                    : marker;
            if (normalized == name)
                return true;
        }
        return false;
    };
    const bool hasReactorsAppGroup = hasAppGroup("ACAD_REACTORS");
    const bool hasXDictionaryAppGroup = hasAppGroup("ACAD_XDICTIONARY");
    if (captureSourceHandle && sourceHandle != 0
        && m_writingContext.ambiguousSourceHandles.count(sourceHandle) == 0) {
        // emplace (NOT operator[]): keeps the FIRST-seen source->minted mapping.
        // captureSourceHandle is false on the VERTEX/SEQEND parent re-entries from
        // writePolyline/writeInsert, which call writeEntity(ent) AGAIN on the SAME
        // parent whose handle was already minted -- so sourceHandle there is a
        // stale MINTED handle (>= FIRSTHANDLE), not a real source. Recording those
        // would POLLUTE the map: a real source handle (also commonly >= FIRSTHANDLE)
        // can numerically equal a stale minted key, and emplace keeping the
        // first-seen would then SHADOW the genuine mapping -> GROUP 340 (resolved
        // via sourceHandleToMintedMap) would mis-point or drop a member. Gating on
        // the call SITE (not the handle value) is correct because a real source
        // handle is indistinguishable from a minted one by value alone.
        const std::size_t mutationCheckpoint = m_dxfWriteMutations.size();
        if (m_recordStateScopeDepth != 0) {
            m_dxfWriteMutations.push_back({
                DxfWriteMutationKind::SourceHandleInsert, {}, sourceHandle,
                ent->handle, false});
        }
        try {
            const auto inserted =
                m_writingContext.sourceHandleToMintedMap.emplace(
                    sourceHandle, ent->handle);
            if (!inserted.second)
                m_dxfWriteMutations.resize(mutationCheckpoint);
        } catch (...) {
            m_dxfWriteMutations.resize(mutationCheckpoint);
            throw;
        }
    }
    writer->writeString(5, toHexStr(ent->handle));
    // R2000+ DXF requires a code-330 owner handle (soft-pointer to the owning
    // BLOCK_RECORD) on every entity. Without it ezdxf/AutoCAD treat the entity
    // as an orphan and emit a recover/audit warning. Resolution priority:
    //   1) ent->parentHandle when explicitly seeded (e.g. raw-replay paths);
    //   2) the active BLOCK_RECORD (currHandle) while writingBlock is true --
    //      writeBlock() latches it for every user block in the BLOCKS section;
    //   3) the fixed Model_Space (0x1F) / Paper_Space (0x1E) BLOCK_RECORD
    //      handles by ent->space, for entities in the ENTITIES section.
    if (version > DRW::AC1014) {
        std::uint32_t ownerHandle = ownerOverride != DRW::NoHandle
            ? ownerOverride : ent->parentHandle;
        if (ownerHandle == 0) {
            if (writingBlock)
                ownerHandle = static_cast<std::uint32_t>(currHandle);
            else
                ownerHandle = (ent->space == DRW::PaperSpace)
                    ? DRW::DxfPaperSpaceBlockRecordHandle
                    : DRW::DxfModelSpaceBlockRecordHandle;
        }
        writer->writeString(330, toHexStr(ownerHandle));
    }
    if (!ent->reactorHandles.empty() && !hasReactorsAppGroup) {
        writer->writeString(102, "{ACAD_REACTORS");
        for (const std::uint32_t reactor : ent->reactorHandles) {
            writer->writeString(330, toHexStr(reactor));
        }
        writer->writeString(102, "}");
    }
    if (ent->xDictHandle != 0 && !hasXDictionaryAppGroup) {
        writer->writeString(102, "{ACAD_XDICTIONARY");
        writer->writeString(360, toHexStr(ent->xDictHandle));
        writer->writeString(102, "}");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbEntity");
    }
    if (ent->space == 1)
        writer->writeInt16(67, 1);
    if (version > DRW::AC1009) {
        writer->writeUtf8String(8, ent->layer);
        writer->writeUtf8String(6, ent->lineType);
    } else {
        writer->writeUtf8Caps(8, ent->layer);
        writer->writeUtf8Caps(6, ent->lineType);
    }
    writer->writeInt16(62, ent->color);
    if (version > DRW::AC1015 && ent->color24 >= 0) {
        writer->writeInt32(420, ent->color24);
    }
    if (version > DRW::AC1015 && !ent->colorName.empty()) {
        writer->writeUtf8String(430, ent->colorName);
    }
    // linetype scale(48) + visibility(60) — both read by DRW_Entity::parseCode
    // and symmetric on the DWG path; previously dropped on DXF save, so a
    // per-entity linetype scale or an invisible entity was lost on DWG→DXF.
    // 60: 0=visible (default, omitted), 1=invisible. (write-review pass-2 #11)
    if (version > DRW::AC1009 && ent->ltypeScale != 1.0) {
        writer->writeDouble(48, ent->ltypeScale);
    }
    if (version > DRW::AC1009 && !ent->visible) {
        writer->writeInt16(60, 1);
    }
    if (version > DRW::AC1018 && ent->shadow != DRW::CastAndReceieveShadows) {
        writer->writeInt16(284, static_cast<int>(ent->shadow));
    }
    // Material (347) is an R2007+ AcDbEntity field (ezdxf acdb_entity:347 ->
    // DXF2007); emitting it at R2004 is non-conformant.
    if (version > DRW::AC1018 && ent->material != DRW::MaterialByLayer) {
        writer->writeUtf8String(347, toHexStr(ent->material));
    }
    if (version > DRW::AC1018 && ent->fullVisualStyleHandle != 0) {
        writer->writeUtf8String(348,
                                toHexStr(ent->fullVisualStyleHandle));
    }
    if (version > DRW::AC1014) {
        writer->writeInt16(370, DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight));
    }
    // Plot-style handle (390) is likewise R2007+ (ezdxf acdb_entity:390 ->
    // DXF2007).
    if (version > DRW::AC1018 && ent->plotStyle != DRW::DefaultPlotStyle) {
        writer->writeUtf8String(390, toHexStr(ent->plotStyle));
    }
    if (version > DRW::AC1015 && ent->transparency != DRW::Opaque) {
        writer->writeInt32(440, ent->transparency);
    }
    if (version >= DRW::AC1014 && !writeAppData(ent->appData)) {
        m_writeError = true;
        return false;
    }
    if (writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeSequenceEnd(std::uint32_t ownerHandle) {
    if (writer == nullptr || ownerHandle == DRW::NoHandle
        || writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }

    std::uint32_t sequenceHandle = 0;
    if (!allocateDxfHandle(sequenceHandle))
        return false;
    if (!writer->writeString(0, "SEQEND")
        || !writer->writeString(5, toHexStr(sequenceHandle))) {
        m_writeError = true;
        return false;
    }
    if (version > DRW::AC1014
        && !writer->writeString(330, toHexStr(ownerHandle))) {
        m_writeError = true;
        return false;
    }
    if (version > DRW::AC1009
        && (!writer->writeString(100, "AcDbEntity")
            || !writer->writeString(8, "0")
            || !writer->writeString(100, "AcDbSequenceEnd"))) {
        m_writeError = true;
        return false;
    }
    return !writer->hasWriteError();
}

bool dxfRW::writeAppData(const std::list<std::list<DRW_Variant>>& appData) {
    // Validate every application-data group before writing its opener.  This
    // keeps malformed nesting or union storage from producing a partial 102
    // group in the containing record.
    const auto validGroup = [](const std::list<DRW_Variant>& group) {
        bool opened = false;
        int depth = 0;
        for (const DRW_Variant& data : group) {
            if (data.code() == 102) {
                if (data.type() != DRW_Variant::STRING
                    || data.content.s == nullptr)
                    return false;
                const std::string& marker = *data.content.s;
                if (!opened) {
                    if (marker.empty() || marker == "}")
                        return false;
                    opened = true;
                    depth = 1;
                } else if (marker == "}") {
                    if (--depth < 0)
                        return false;
                } else if (!marker.empty() && marker.front() == '{') {
                    if (++depth > DRW::kMaxDxfApplicationGroupNesting)
                        return false;
                } else {
                    return false;
                }
                continue;
            }
            if (!opened || depth == 0)
                return false;
            switch (data.type()) {
            case DRW_Variant::STRING:
                if (data.content.s == nullptr)
                    return false;
                break;
            case DRW_Variant::INTEGER:
            case DRW_Variant::INTEGER64:
                break;
            case DRW_Variant::DOUBLE:
                if (!std::isfinite(data.content.d))
                    return false;
                break;
            default:
                return false;
            }
        }
        return group.empty() || (opened && depth == 0);
    };

    for (const auto& group : appData) {
        if (!validGroup(group)) {
            m_writeError = true;
            return false;
        }
    }

    for (const auto &group : appData) {
        bool opened = false;
        int depth = 0;

        for (const auto &data : group) {
            if (data.code() == 102) {
                if (data.type() != DRW_Variant::STRING
                    || data.content.s == nullptr) {
                    m_writeError = true;
                    return false;
                }
                std::string marker = *data.content.s;
                if (!opened) {
                    if (marker.empty() || marker == "}") {
                        m_writeError = true;
                        return false;
                    }
                    if (marker.front() != '{')
                        marker.insert(marker.begin(), '{');
                    if (marker.size() == 1) {
                        m_writeError = true;
                        return false;
                    }
                    opened = true;
                    depth = 1;
                } else if (marker == "}") {
                    if (--depth < 0) {
                        m_writeError = true;
                        return false;
                    }
                } else if (!marker.empty() && marker.front() == '{') {
                    if (++depth > DRW::kMaxDxfApplicationGroupNesting) {
                        m_writeError = true;
                        return false;
                    }
                } else {
                    m_writeError = true;
                    return false;
                }
                if (!writer->writeString(102, marker)) {
                    m_writeError = true;
                    return false;
                }
                continue;
            }
            if (!opened)
                continue;

            if (depth == 0) {
                m_writeError = true;
                return false;
            }

            bool written = false;
            switch (data.type()) {
            case DRW_Variant::STRING:
                written = data.content.s != nullptr
                    && writer->writeString(data.code(), *data.content.s);
                break;
            case DRW_Variant::INTEGER:
                written = writer->writeInt32(data.code(), data.content.i);
                break;
            case DRW_Variant::INTEGER64:
                written = writer->writeInt64(data.code(), data.content.i64);
                break;
            case DRW_Variant::DOUBLE:
                written = std::isfinite(data.content.d)
                    && writer->writeDouble(data.code(), data.content.d);
                break;
            default:
                m_writeError = true;
                return false;
            }
            if (!written) {
                m_writeError = true;
                return false;
            }
        }

        if (opened && depth != 0) {
            m_writeError = true;
            return false;
        }
        if (!group.empty() && !opened) {
            m_writeError = true;
            return false;
        }
    }
    return true;
}

bool dxfRW::writeTableEntryAppData(const DRW_TableEntry& entry) {
    if (version < DRW::AC1014) {
        if (!entry.appData.empty() || !entry.reactorHandles.empty()
            || entry.xDictHandle != 0) {
            m_writeError = true;
            return false;
        }
        return true;
    }

    const auto hasAppGroup = [&entry](const char *name) {
        for (const auto &group : entry.appData) {
            if (group.empty())
                continue;
            const DRW_Variant &opener = group.front();
            if (opener.code() != 102
                || opener.type() != DRW_Variant::STRING)
                continue;
            const std::string marker = opener.content.s
                ? *opener.content.s
                : std::string{};
            const std::string normalized =
                (!marker.empty() && marker.front() == '{')
                    ? marker.substr(1)
                    : marker;
            if (normalized == name)
                return true;
        }
        return false;
    };

    if (!entry.reactorHandles.empty() && !hasAppGroup("ACAD_REACTORS")) {
        if (!writer->writeString(102, "{ACAD_REACTORS")) {
            m_writeError = true;
            return false;
        }
        for (const std::uint32_t reactor : entry.reactorHandles) {
            if (!writer->writeString(330, toHexStr(reactor))) {
                m_writeError = true;
                return false;
            }
        }
        if (!writer->writeString(102, "}")) {
            m_writeError = true;
            return false;
        }
    }
    if (entry.xDictHandle != 0 && !hasAppGroup("ACAD_XDICTIONARY")) {
        if (!writer->writeString(102, "{ACAD_XDICTIONARY")
            || !writer->writeString(360, toHexStr(entry.xDictHandle))
            || !writer->writeString(102, "}")) {
            m_writeError = true;
            return false;
        }
    }
    if (!writeAppData(entry.appData)) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeLineType(DRW_LType *ent){
    if (!preflightTableEntry(ent))
        return false;
    std::string strname = ent->name;

    transform(strname.begin(), strname.end(), strname.begin(),
              [](unsigned char ch) {
                  return static_cast<char>(std::toupper(ch));
              });
    //do not write linetypes handled by library
    if (strname == "BYLAYER" || strname == "BYBLOCK" || strname == "CONTINUOUS") {
        // These mandatory records are emitted before the interface callback.
        // Replaying application data here would otherwise report success while
        // silently dropping the payload.
        if (!ent->appData.empty() || !ent->extData.empty()
            || !ent->reactorHandles.empty() || ent->xDictHandle != 0) {
            m_writeError = true;
            return false;
        }
        return true;
    }
    if (ent->path.size() >
            static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max())) {
        m_writeError = true;
        return false;
    }
    // update() normalizes path/segments and recomputes size/length, but it is
    // mutating. Work on a copy so a failed write never changes caller state.
    DRW_LType normalized = *ent;
    normalized.update();
    if (normalized.path.size() >
            static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max())
        || !normalized.validateDxf()) {
        m_writeError = true;
        return false;
    }
    RecordStateScope state(*this, ent);
    DxfWriterRecordScope record(*writer);
    bool result = true;
    const auto write = [&result](bool ok) { result = ok && result; };
    std::uint32_t allocatedHandle = 0;
    if (version > DRW::AC1009 && !allocateDxfHandle(allocatedHandle))
        return false;
    write(writer->writeString(0, "LTYPE"));
    if (version > DRW::AC1009) {
        write(writer->writeString(5, toHexStr(allocatedHandle)));
        m_writingContext.lineTypesMap.emplace_back(
            strname, allocatedHandle);
        if (version > DRW::AC1012) {
            write(writer->writeString(330, "5"));
        }
        write(writer->writeString(100, "AcDbSymbolTableRecord"));
        write(writer->writeString(100, "AcDbLinetypeTableRecord"));
        write(writer->writeUtf8String(2, normalized.name));
    } else
        write(writer->writeUtf8Caps(2, normalized.name));
    write(writer->writeInt16(70, normalized.flags));
    write(writer->writeUtf8String(3, normalized.desc));
    write(writer->writeInt16(72, normalized.alignment));
    write(writer->writeInt16(73, normalized.size));
    write(writer->writeDouble(40, normalized.length));

    for (std::size_t i = 0; i < normalized.segments.size(); ++i) {
        const DRW_LTypeSegment& segment = normalized.segments[i];
        write(writer->writeDouble(49, segment.length));
        if (version > DRW::AC1009) {
            write(writer->writeInt16(74, segment.shapeFlags));
            if (segment.shapeFlags != 0) {
                write(writer->writeInt16(75, segment.complexShapeCode));
                write(writer->writeString(
                    340, toHexStr(segment.styleHandle.ref)));
                write(writer->writeDouble(46, segment.scale));
                write(writer->writeDouble(50, segment.rotation * ARAD));
                write(writer->writeDouble(44, segment.xOffset));
                write(writer->writeDouble(45, segment.yOffset));
                if ((segment.shapeFlags & 0x02) != 0)
                    write(writer->writeUtf8String(9, segment.text));
            }
        }
    }
    if (!writeTableEntryAppData(*ent))
        result = false;
    if (!ent->extData.empty())
        result = writeExtData(ent->extData) && result;
    if (!result || writer->hasWriteError())
        m_writeError = true;
    if (!result || writer->hasWriteError())
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeLayer(DRW_Layer *ent){
    if (!preflightTableEntry(ent)
        || !ent->validateDxf()) {
        m_writeError = true;
        return false;
    }
    RecordStateScope state(*this, static_cast<DRW_TableEntry*>(ent));
    DxfWriterRecordScope record(*writer);
    bool result = true;
    const auto write = [&result](bool ok) { result = ok && result; };
    std::uint32_t allocatedHandle = 0;
    const bool isLayerZero = !wlayer0 && ent->name == "0";
    if (version > DRW::AC1009 && !isLayerZero
        && !allocateDxfHandle(allocatedHandle))
        return false;
    write(writer->writeString(0, "LAYER"));
    if (isLayerZero) {
        wlayer0 = true;
        if (version > DRW::AC1009) {
            write(writer->writeString(5, "10"));
        }
    } else {
        if (version > DRW::AC1009) {
            write(writer->writeString(5, toHexStr(allocatedHandle)));
        }
    }
    if (version > DRW::AC1012) {
        write(writer->writeString(330, "2"));
    }
    if (version > DRW::AC1009) {
        write(writer->writeString(100, "AcDbSymbolTableRecord"));
        write(writer->writeString(100, "AcDbLayerTableRecord"));
        write(writer->writeUtf8String(2, ent->name));
    } else {
        write(writer->writeUtf8Caps(2, ent->name));
    }
    write(writer->writeInt16(70, ent->flags));
    write(writer->writeInt16(62, ent->color));
    if (version > DRW::AC1015 && ent->color24 >= 0) {
        write(writer->writeInt32(420, ent->color24));
    }
    if (version > DRW::AC1009) {
        write(writer->writeUtf8String(6, ent->lineType));
        // plot (290), lineweight (370) and plotstyle handle (390) are R2000+
        // LAYER fields (ezdxf acdb_symbol_table_record: all DXF2000); they did
        // not exist in R13/R14, so emitting them there is non-conformant.
        if (version > DRW::AC1014) {
            // Emit the plot flag unconditionally, matching lineweight (370) and
            // plotstyle (390) below and AutoCAD/ezdxf, which always write every
            // R2000+ LAYER field. The previous code wrote 290 only when plotF
            // was false and relied on "absent => true"; that dropped an explicit
            // "290 1" written by a strict external tool on re-save and was
            // inconsistent with the sibling fields. Reading is unaffected:
            // DRW_Layer defaults plotF=true, so legacy files without 290 still
            // load as plot-on.
            write(writer->writeBool(290, ent->plotF));
            write(writer->writeInt16(370,
                                     DRW_LW_Conv::lineWidth2dxfInt(ent->lWeight)));
            write(writer->writeString(390, "F"));
        }
    } else
        write(writer->writeUtf8Caps(6, ent->lineType));
    if (!writeTableEntryAppData(*ent))
        result = false;
    if (!ent->extData.empty()){
        write(writeExtData(ent->extData));
    }
    if (!result || writer->hasWriteError())
        m_writeError = true;
    if (!result || writer->hasWriteError())
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeTextstyle(DRW_Textstyle *ent){
    if (!preflightTableEntry(ent)
        || !ent->validateDxf()) {
        m_writeError = true;
        return false;
    }
    RecordStateScope state(*this, static_cast<DRW_TableEntry*>(ent));
    DxfWriterRecordScope record(*writer);
    bool result = true;
    const auto write = [&result](bool ok) { result = ok && result; };
    std::uint32_t allocatedHandle = 0;
    if (version > DRW::AC1009 && !allocateDxfHandle(allocatedHandle))
        return false;
    write(writer->writeString(0, "STYLE"));
    //stringstream cause crash in OS/X, bug#3597944
    const std::string name = dxfSymbolNameKey(ent->name);
    if (!dimstyleStd) {
        if (name == "STANDARD"){
            dimstyleStd = true;
        }
    }
    if (version > DRW::AC1009) {
        write(writer->writeString(5, toHexStr(allocatedHandle)));
        const auto existing = textStyleMap.find(name);
        const std::size_t mutationCheckpoint = m_dxfWriteMutations.size();
        if (m_recordStateScopeDepth != 0) {
            m_dxfWriteMutations.push_back({
                DxfWriteMutationKind::TextStyleMapSet, name, 0,
                existing == textStyleMap.end() ? 0 : existing->second,
                existing != textStyleMap.end()});
        }
        try {
            textStyleMap[name] = allocatedHandle;
        } catch (...) {
            m_dxfWriteMutations.resize(mutationCheckpoint);
            throw;
        }
        }

    if (version > DRW::AC1012) {
        write(writer->writeString(330, "2"));
    }
    if (version > DRW::AC1009) {
        write(writer->writeString(100, "AcDbSymbolTableRecord"));
        write(writer->writeString(100, "AcDbTextStyleTableRecord"));
        write(writer->writeUtf8String(2, ent->name));
    } else {
        write(writer->writeUtf8Caps(2, ent->name));
    }
    write(writer->writeInt16(70, ent->flags));
    write(writer->writeDouble(40, ent->height));
    write(writer->writeDouble(41, ent->width));
    write(writer->writeDouble(50, ent->oblique));
    write(writer->writeInt16(71, ent->genFlag));
    write(writer->writeDouble(42, ent->lastHeight));
    if (version > DRW::AC1009) {
        write(writer->writeUtf8String(3, ent->font));
        write(writer->writeUtf8String(4, ent->bigFont));
        if (ent->fontFamily != 0)
            write(writer->writeInt32(1071, ent->fontFamily));
    } else {
        write(writer->writeUtf8Caps(3, ent->font));
        write(writer->writeUtf8Caps(4, ent->bigFont));
    }
    if (!writeTableEntryAppData(*ent))
        result = false;
    if (!ent->extData.empty())
        result = writeExtData(ent->extData) && result;
    if (!result || writer->hasWriteError())
        m_writeError = true;
    if (!result || writer->hasWriteError())
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeVport(DRW_Vport *ent){
    if (!preflightTableEntry(ent)
        || !ent->validateDxf()) {
        m_writeError = true;
        return false;
    }
    RecordStateScope state(*this, ent);
    DxfWriterRecordScope record(*writer);
    const std::string emittedName = !dimstyleStd ? "*ACTIVE" : ent->name;
    if (!dimstyleStd) {
        dimstyleStd = true;
    }
    std::uint32_t allocatedHandle = 0;
    if (version > DRW::AC1009 && !allocateDxfHandle(allocatedHandle))
        return false;
    writer->writeString(0, "VPORT");
    if (version > DRW::AC1009) {
        writer->writeString(5, toHexStr(allocatedHandle));
        if (version > DRW::AC1012)
            writer->writeString(330, "2");
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbViewportTableRecord");
        writer->writeUtf8String(2, emittedName);
    } else
        writer->writeUtf8Caps(2, emittedName);
    writer->writeInt16(70, ent->flags);
    writer->writeDouble(10, ent->lowerLeft.x);
    writer->writeDouble(20, ent->lowerLeft.y);
    writer->writeDouble(11, ent->UpperRight.x);
    writer->writeDouble(21, ent->UpperRight.y);
    writer->writeDouble(12, ent->center.x);
    writer->writeDouble(22, ent->center.y);
    writer->writeDouble(13, ent->snapBase.x);
    writer->writeDouble(23, ent->snapBase.y);
    writer->writeDouble(14, ent->snapSpacing.x);
    writer->writeDouble(24, ent->snapSpacing.y);
    writer->writeDouble(15, ent->gridSpacing.x);
    writer->writeDouble(25, ent->gridSpacing.y);
    writer->writeDouble(16, ent->viewDir.x);
    writer->writeDouble(26, ent->viewDir.y);
    writer->writeDouble(36, ent->viewDir.z);
    writer->writeDouble(17, ent->viewTarget.x);
    writer->writeDouble(27, ent->viewTarget.y);
    writer->writeDouble(37, ent->viewTarget.z);
    writer->writeDouble(40, ent->height);
    writer->writeDouble(41, ent->ratio);
    writer->writeDouble(42, ent->lensHeight);
    writer->writeDouble(43, ent->frontClip);
    writer->writeDouble(44, ent->backClip);
    writer->writeDouble(50, ent->snapAngle);
    writer->writeDouble(51, ent->twistAngle);
    writer->writeInt16(71, ent->viewMode);
    writer->writeInt16(72, ent->circleZoom);
    writer->writeInt16(73, ent->fastZoom);
    writer->writeInt16(74, ent->ucsIcon);
    writer->writeInt16(75, ent->snap);
    writer->writeInt16(76, ent->grid);
    writer->writeInt16(77, ent->snapStyle);
    writer->writeInt16(78, ent->snapIsopair);
    if (version > DRW::AC1014) {
        writer->writeInt16(281, static_cast<int>(ent->renderMode));
        writer->writeInt16(65, ent->ucsPerVP ? 1 : 0);
        writer->writeDouble(110, ent->ucsOrigin.x);
        writer->writeDouble(120, ent->ucsOrigin.y);
        writer->writeDouble(130, ent->ucsOrigin.z);
        writer->writeDouble(111, ent->ucsXAxis.x);
        writer->writeDouble(121, ent->ucsXAxis.y);
        writer->writeDouble(131, ent->ucsXAxis.z);
        writer->writeDouble(112, ent->ucsYAxis.x);
        writer->writeDouble(122, ent->ucsYAxis.y);
        writer->writeDouble(132, ent->ucsYAxis.z);
        writer->writeInt16(79, ent->ucsOrthoType);
        writer->writeDouble(146, ent->ucsElevation);
        if (ent->namedUcsHandle != 0)
            writer->writeString(345, toHexStr(ent->namedUcsHandle));
        if (ent->baseUcsHandle != 0)
            writer->writeString(346, toHexStr(ent->baseUcsHandle));
        if (version > DRW::AC1018) {
            writer->writeInt16(60, ent->gridBehavior);
            writer->writeInt16(61, ent->gridMajorLines);
            writer->writeBool(292, ent->useDefaultLighting ? 1 : 0);
            writer->writeInt16(282, ent->defaultLightingType);
            writer->writeDouble(141, ent->brightness);
            writer->writeDouble(142, ent->contrast);
            writer->writeInt16(63, static_cast<int>(ent->ambientColor));
            if (ent->ambientColorRgb >= 0)
                writer->writeInt32(421, ent->ambientColorRgb);
            if (!ent->ambientColorName.empty())
                writer->writeUtf8String(431, ent->ambientColorName);
            if (ent->backgroundHandle != 0)
                writer->writeString(332, toHexStr(ent->backgroundHandle));
            if (ent->visualStyleHandle != 0)
                writer->writeString(348, toHexStr(ent->visualStyleHandle));
            if (ent->m_sunHandle != 0)
                writer->writeString(361, toHexStr(ent->m_sunHandle));
    }
    }
    bool result = !writer->hasWriteError();
    if (!writeTableEntryAppData(*ent))
        result = false;
    if (!ent->extData.empty())
        result = writeExtData(ent->extData) && result;
    if (!result || writer->hasWriteError())
        m_writeError = true;
    if (!result || writer->hasWriteError())
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeDimstyle(DRW_Dimstyle *ent){
    if (!preflightTableEntry(ent))
        return false;

    // Validate dynamic DIMSTYLE overrides before the record prefix is
    // emitted.  Unknown codes are rejected rather than silently dropped;
    // callers that need a new DIMSTYLE field must extend the descriptor table
    // with its DXF type and version first.
    for (const auto& kv : ent->vars) {
        const DRW_Variant* value = kv.second;
        if (value == nullptr) {
            m_writeError = true;
            return false;
        }
        const DxfDimstyleVariableSpec* spec =
            findDxfDimstyleVariableSpec(value->code());
        if (spec == nullptr) {
            m_writeError = true;
            return false;
        }
        if (version < spec->minimumVersion)
            continue;
        if (value->type() != spec->type
            || (value->type() == DRW_Variant::STRING
                && value->content.s == nullptr)
            || (value->type() == DRW_Variant::DOUBLE
                && !std::isfinite(value->content.d))) {
            m_writeError = true;
            return false;
        }
    }

    RecordStateScope state(*this, ent);
    DxfWriterRecordScope record(*writer);
    std::uint32_t allocatedHandle = 0;
    if (version > DRW::AC1009 && !allocateDxfHandle(allocatedHandle))
        return false;
    writer->writeString(0, "DIMSTYLE");
    if (!dimstyleStd) {
        std::string name = ent->name;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::toupper(ch));
                       });
        if (name == "STANDARD")
            dimstyleStd = true;
    }
    if (version > DRW::AC1009) {
        writer->writeString(105, toHexStr(allocatedHandle));
    }

    if (version > DRW::AC1012) {
        writer->writeString(330, "A");
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbDimStyleTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    if ( version == DRW::AC1009 || !(ent->dimpost.empty()) )
        writer->writeUtf8String(3, ent->dimpost);
    if ( version == DRW::AC1009 || !(ent->dimapost.empty()) )
        writer->writeUtf8String(4, ent->dimapost);
    if ( version == DRW::AC1009 || !(ent->dimblk.empty()) )
        writer->writeUtf8String(5, ent->dimblk);
    if ( version == DRW::AC1009 || !(ent->dimblk1.empty()) )
        writer->writeUtf8String(6, ent->dimblk1);
    if ( version == DRW::AC1009 || !(ent->dimblk2.empty()) )
        writer->writeUtf8String(7, ent->dimblk2);
    // De-dup: the prepareDRWDimStyle* helpers populate ent->vars (via d.add)
    // with the real modified values; the POD members below keep reset() defaults.
    // Emitting both produced DUPLICATE DXF group codes (default + real) that ezdxf
    // flags. Route the POD writes through guards that skip any code already in
    // ent->vars, so the vars loop below emits the single real value. (P3 #6)
    std::set<int> dimVarCodes;
    for (const auto& kv : ent->vars) {
        const DRW_Variant* value = kv.second;
        const DxfDimstyleVariableSpec* spec = value == nullptr
            ? nullptr : findDxfDimstyleVariableSpec(value->code());
        if (spec != nullptr && version >= spec->minimumVersion)
            dimVarCodes.insert(value->code());
    }
    auto wD = [&](int code, double val) {
        if (!dimVarCodes.count(code)) writer->writeDouble(code, val); };
    auto wI = [&](int code, int val) {
        if (!dimVarCodes.count(code)) writer->writeInt16(code, val); };
    wD(40, ent->dimscale);
    wD(41, ent->dimasz);
    wD(42, ent->dimexo);
    wD(43, ent->dimdli);
    wD(44, ent->dimexe);
    wD(45, ent->dimrnd);
    wD(46, ent->dimdle);
    wD(47, ent->dimtp);
    wD(48, ent->dimtm);
    if ( version > DRW::AC1018 || ent->dimfxl !=0 )
        wD(49, ent->dimfxl);
    wD(140, ent->dimtxt);
    wD(141, ent->dimcen);
    wD(142, ent->dimtsz);
    wD(143, ent->dimaltf);
    wD(144, ent->dimlfac);
    wD(145, ent->dimtvp);
    wD(146, ent->dimtfac);
    wD(147, ent->dimgap);
    if (version > DRW::AC1014) {
        wD(148, ent->dimaltrnd);
    }
    wI(71, ent->dimtol);
    wI(72, ent->dimlim);
    wI(73, ent->dimtih);
    wI(74, ent->dimtoh);
    wI(75, ent->dimse1);
    wI(76, ent->dimse2);
    wI(77, ent->dimtad);
    wI(78, ent->dimzin);
    if (version > DRW::AC1014) {
        wI(79, ent->dimazin);
    }
    wI(170, ent->dimalt);
    wI(171, ent->dimaltd);
    wI(172, ent->dimtofl);
    wI(173, ent->dimsah);
    wI(174, ent->dimtix);
    wI(175, ent->dimsoxd);
    wI(176, ent->dimclrd);
    wI(177, ent->dimclre);
    wI(178, ent->dimclrt);
    if (version > DRW::AC1014) {
        wI(179, ent->dimadec);
    }
    if (version > DRW::AC1009) {
        if (version < DRW::AC1015)
            wI(270, ent->dimunit);
        wI(271, ent->dimdec);
        wI(272, ent->dimtdec);
        wI(273, ent->dimaltu);
        wI(274, ent->dimalttd);
        wI(275, ent->dimaunit);
    }
    if (version > DRW::AC1014) {
        wI(276, ent->dimfrac);
        wI(277, ent->dimlunit);
        wI(278, ent->dimdsep);
        wI(279, ent->dimtmove);
    }
    if (version > DRW::AC1009) {
        wI(280, ent->dimjust);
        wI(281, ent->dimsd1);
        wI(282, ent->dimsd2);
        wI(283, ent->dimtolj);
        wI(284, ent->dimtzin);
        wI(285, ent->dimaltz);
        wI(286, ent->dimaltttz);
        if (version < DRW::AC1015)
            wI(287, ent->dimfit);
        wI(288, ent->dimupt);
    }
    if (version > DRW::AC1014) {
        wI(289, ent->dimatfit);
    }
    if ( version > DRW::AC1018 && ent->dimfxlon !=0 )
        wI(290, ent->dimfxlon);
    if (version > DRW::AC1009) {
        const std::string txstyname = dxfSymbolNameKey(ent->dimtxsty);
        if (!dimVarCodes.count(340) && textStyleMap.count(txstyname) > 0) {
            std::uint32_t txstyHandle = (*(textStyleMap.find(txstyname))).second;
            writer->writeUtf8String(340, toHexStr(txstyHandle));
        }
    }
    if (version > DRW::AC1014) {
        const auto writeBlockHandle = [this, &dimVarCodes](int code,
                                                            const UTF8STRING& name) {
            if (dimVarCodes.count(code))
                return;
            const auto block = blockMap.find(dxfSymbolNameKey(name));
            if (block != blockMap.end())
                writer->writeUtf8String(code, toHexStr(block->second));
        };
        writeBlockHandle(341, ent->dimldrblk);
        writeBlockHandle(342, ent->dimblk);
        writeBlockHandle(343, ent->dimblk1);
        writeBlockHandle(344, ent->dimblk2);
        wI(371, ent->dimlwd);
        wI(372, ent->dimlwe);
    }
    for (auto& kv : ent->vars) {
        DRW_Variant* v = kv.second;
        if (v == nullptr)
            continue;
        const DxfDimstyleVariableSpec* spec =
            findDxfDimstyleVariableSpec(v->code());
        if (spec == nullptr || version < spec->minimumVersion)
            continue;
        switch (v->type()) {
            case DRW_Variant::STRING:  writer->writeUtf8String(v->code(), v->c_str()); break;
            case DRW_Variant::INTEGER: writer->writeInt16(v->code(), v->i_val()); break;
            case DRW_Variant::INTEGER64: writer->writeInt64(v->code(), v->i64_val()); break;
            case DRW_Variant::DOUBLE:  writer->writeDouble(v->code(), v->d_val()); break;
            default: break;
        }
    }
    if (!writeTableEntryAppData(*ent)
        || (!ent->extData.empty() && !writeExtData(ent->extData))) {
        m_writeError = true;
        return false;
    }
    if (writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeView(DRW_View *ent){
    if (!preflightTableEntry(ent)
        || !ent->validateDxf()) {
        m_writeError = true;
        return false;
    }
    RecordStateScope state(*this, ent);
    DxfWriterRecordScope record(*writer);
    std::uint32_t allocatedHandle = 0;
    if (version > DRW::AC1009 && !allocateDxfHandle(allocatedHandle))
        return false;
    bool result = writer->writeString(0, "VIEW");
    if (version > DRW::AC1009) {
        writer->writeString(5, toHexStr(allocatedHandle));
        if (version > DRW::AC1012)
            writer->writeString(330, "6");
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbViewTableRecord");
        writer->writeUtf8String(2, ent->name);
    } else
        writer->writeUtf8Caps(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeDouble(40, ent->size.y);
    writer->writeDouble(10, ent->center.x);
    writer->writeDouble(20, ent->center.y);
    writer->writeDouble(41, ent->size.x);
    writer->writeDouble(11, ent->viewDirectionFromTarget.x);
    writer->writeDouble(21, ent->viewDirectionFromTarget.y);
    writer->writeDouble(31, ent->viewDirectionFromTarget.z);
    writer->writeDouble(12, ent->targetPoint.x);
    writer->writeDouble(22, ent->targetPoint.y);
    writer->writeDouble(32, ent->targetPoint.z);
    writer->writeDouble(42, ent->lensLen);
    writer->writeDouble(43, ent->frontClippingPlaneOffset);
    writer->writeDouble(44, ent->backClippingPlaneOffset);
    writer->writeDouble(50, ent->twistAngle);
    writer->writeInt16(71, ent->viewMode);
    if (version > DRW::AC1009) {
        writer->writeInt16(281, static_cast<int>(ent->renderMode));
        writer->writeBool(72, ent->hasUCS);
        if (ent->hasUCS) {
            writer->writeDouble(110, ent->ucsOrigin.x);
            writer->writeDouble(120, ent->ucsOrigin.y);
            writer->writeDouble(130, ent->ucsOrigin.z);
            writer->writeDouble(111, ent->ucsXAxis.x);
            writer->writeDouble(121, ent->ucsXAxis.y);
            writer->writeDouble(131, ent->ucsXAxis.z);
            writer->writeDouble(112, ent->ucsYAxis.x);
            writer->writeDouble(122, ent->ucsYAxis.y);
            writer->writeDouble(132, ent->ucsYAxis.z);
            writer->writeInt16(79, ent->ucsOrthoType);
            writer->writeDouble(146, ent->ucsElevation);
        }
        if (version > DRW::AC1018)
            writer->writeBool(73, ent->cameraPlottable);
    }
    if (version > DRW::AC1018) {
        writer->writeBool(292, ent->m_useDefaultLights);
        writer->writeInt16(282, ent->m_defaultLightingType);
        writer->writeDouble(141, ent->m_brightness);
        writer->writeDouble(142, ent->m_contrast);
        writer->writeInt16(63, static_cast<int>(ent->m_ambientColor));
        if (ent->m_ambientColorRgb >= 0)
            writer->writeInt32(421, ent->m_ambientColorRgb);
        if (!ent->m_ambientColorName.empty())
            writer->writeUtf8String(431, ent->m_ambientColorName);
        if (ent->m_backgroundHandle != 0)
            writer->writeString(332, toHexStr(ent->m_backgroundHandle));
        if (ent->m_visualStyleHandle != 0)
            writer->writeString(348, toHexStr(ent->m_visualStyleHandle));
        if (ent->m_sunHandle != 0)
            writer->writeString(361, toHexStr(ent->m_sunHandle));
        if (ent->m_liveSectionHandle != 0)
            writer->writeString(334, toHexStr(ent->m_liveSectionHandle));
    }
    if (version > DRW::AC1009 && ent->hasUCS) {
        if (ent->baseUCS_ID != 0)
            writer->writeString(346, toHexStr(ent->baseUCS_ID));
        if (ent->namedUCS_ID != 0)
            writer->writeString(345, toHexStr(ent->namedUCS_ID));
    }
    if (!writeTableEntryAppData(*ent))
        result = false;
    if (!ent->extData.empty())
        result = writeExtData(ent->extData) && result;
    if (!result || writer->hasWriteError())
        m_writeError = true;
    if (!result || writer->hasWriteError())
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeUCS(DRW_UCS *ent){
    if (!preflightTableEntry(ent)
        || !ent->validateDxf()) {
        m_writeError = true;
        return false;
    }
    RecordStateScope state(*this, ent);
    DxfWriterRecordScope record(*writer);
    bool result = true;
    const auto write = [&result](bool ok) { result = ok && result; };
    std::uint32_t allocatedHandle = 0;
    if (version > DRW::AC1009 && !allocateDxfHandle(allocatedHandle))
        return false;
    write(writer->writeString(0, "UCS"));
    if (version > DRW::AC1009) {
        write(writer->writeString(5, toHexStr(allocatedHandle)));
        if (version > DRW::AC1012)
            write(writer->writeString(330, "7"));
        write(writer->writeString(100, "AcDbSymbolTableRecord"));
        write(writer->writeString(100, "AcDbUCSTableRecord"));
        write(writer->writeUtf8String(2, ent->name));
    } else
        write(writer->writeUtf8Caps(2, ent->name));
    write(writer->writeInt16(70, ent->flags));
    write(writer->writeDouble(10, ent->origin.x));
    write(writer->writeDouble(20, ent->origin.y));
    write(writer->writeDouble(30, ent->origin.z));
    write(writer->writeDouble(11, ent->xAxisDirection.x));
    write(writer->writeDouble(21, ent->xAxisDirection.y));
    write(writer->writeDouble(31, ent->xAxisDirection.z));
    write(writer->writeDouble(12, ent->yAxisDirection.x));
    write(writer->writeDouble(22, ent->yAxisDirection.y));
    write(writer->writeDouble(32, ent->yAxisDirection.z));
    if (version > DRW::AC1014) {
        write(writer->writeDouble(13, ent->orthoOrigin.x));
        write(writer->writeDouble(23, ent->orthoOrigin.y));
        write(writer->writeDouble(33, ent->orthoOrigin.z));
        write(writer->writeInt16(79, ent->orthoType));
        write(writer->writeDouble(146, ent->elevation));
        if (ent->baseUcsHandle.ref != 0)
            write(writer->writeString(346, toHexStr(ent->baseUcsHandle.ref)));
        if (ent->namedUcsHandle.ref != 0)
            write(writer->writeString(345, toHexStr(ent->namedUcsHandle.ref)));
    }
    if (!writeTableEntryAppData(*ent))
        result = false;
    if (!ent->extData.empty())
        result = writeExtData(ent->extData) && result;
    if (!result || writer->hasWriteError())
        m_writeError = true;
    if (!result || writer->hasWriteError())
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}


bool dxfRW::writeAppId(DRW_AppId *ent){
    if (!preflightTableEntry(ent)
        || !ent->validateDxf()) {
        m_writeError = true;
        return false;
    }
    std::string strname = ent->name;
    transform(strname.begin(), strname.end(), strname.begin(),
              [](unsigned char ch) {
                  return static_cast<char>(std::toupper(ch));
              });
    //do not write mandatory ACAD appId, handled by library
    if (strname == "ACAD")
        return true;
    RecordStateScope state(*this, ent);
    DxfWriterRecordScope record(*writer);
    std::uint32_t allocatedHandle = 0;
    if (version > DRW::AC1009 && !allocateDxfHandle(allocatedHandle))
        return false;
    bool result = writer->writeString(0, "APPID");
    if (version > DRW::AC1009) {
        result = writer->writeString(5, toHexStr(allocatedHandle)) && result;
        if (version > DRW::AC1014) {
            result = writer->writeString(330, "9") && result;
        }
        result = writer->writeString(100, "AcDbSymbolTableRecord") && result;
        result = writer->writeString(100, "AcDbRegAppTableRecord") && result;
        result = writer->writeUtf8String(2, ent->name) && result;
    } else {
        result = writer->writeUtf8Caps(2, ent->name) && result;
    }
    result = writer->writeInt16(70, ent->flags) && result;
    if (ent->unknown71 != 0)
        result = writer->writeInt16(71, ent->unknown71) && result;
    if (!writeTableEntryAppData(*ent))
        result = false;
    if (!ent->extData.empty())
        result = writeExtData(ent->extData) && result;
    if (!result || writer->hasWriteError())
        m_writeError = true;
    if (!result || writer->hasWriteError())
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

std::uint32_t dxfRW::remapEntityHandle(std::uint32_t sourceHandle) const {
    const auto it = m_writingContext.sourceHandleToMintedMap.find(sourceHandle);
    return it == m_writingContext.sourceHandleToMintedMap.end()
        ? sourceHandle : it->second;
}

bool dxfRW::writePoint(DRW_Point *ent) {
    if (!preflightEntity(ent))
        return false;
    RecordStateScope state(*this, ent);
    DxfWriterRecordScope record(*writer);
    writer->writeString(0, "POINT");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbPoint");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    if (ent->thickness != 0.0) {
        writer->writeDouble(39, ent->thickness);
    }
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0
        || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    if (ent->xAxisAngle != 0.0)
        writer->writeDouble(50, ent->xAxisAngle * ARAD);  // radians -> DXF degrees (rad * 180/pi)
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeLine(DRW_Line *ent) {
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "LINE");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbLine");
    }
    // thickness(39) — reader + DWG encoder both preserve it; omitting it
    // flattened thick 2.5D profiles on DXF save.
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    } else {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
    }
    // extrusion(210/220/230) — default 0,0,1; reader consumes it. Omitting it
    // re-imported out-of-plane lines in the WCS XY plane.
    DRW_Coord crd = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::write3DLine(DRW_3DLine *ent) {
    if (!preflightEntity(ent))
        return false;
    if (version < DRW::AC1015)
        return rejectUnsupportedDxfWrite();

    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "3DLINE");
    if (!writeEntity(ent))
        return false;
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(39, ent->thickness);
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeRay(DRW_Ray *ent) {
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "RAY");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbRay");
    }
    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
        writer->writeDouble(31, crd.z);
    } else {
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeXline(DRW_Xline *ent) {
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "XLINE");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbXline");
    }
    DRW_Coord crd = ent->secPoint;
    crd.unitize();
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0 || ent->secPoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
        writer->writeDouble(31, crd.z);
    } else {
        writer->writeDouble(11, crd.x);
        writer->writeDouble(21, crd.y);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeCircle(DRW_Circle *ent) {
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "CIRCLE");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbCircle");
    }
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    writer->writeDouble(40, ent->radious);
    // Extrusion (AcDbCircle subclass) — default 0,0,1. Omitting it flattened
    // non-Z-up circles on DXF export; the reader (DRW_Point::parseCode) already
    // consumes 210/220/230, so this completes the round trip.
    DRW_Coord crd = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeArc(DRW_Arc *ent) {
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "ARC");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbCircle");
    }
    // thickness(39) belongs to the AcDbCircle subclass (precedes the AcDbArc
    // marker); reader + DWG encoder preserve it.
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0) {
        writer->writeDouble(30, ent->basePoint.z);
    }
    writer->writeDouble(40, ent->radious);
    // Extrusion belongs to the AcDbCircle subclass, so it must precede the
    // AcDbArc marker. Default 0,0,1; reader consumes 210/220/230.
    DRW_Coord crd = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbArc");
    }
    writer->writeDouble(50, ent->staangle*ARAD);
    writer->writeDouble(51, ent->endangle*ARAD);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeEllipse(DRW_Ellipse *ent){
    if (!preflightEntity(ent))
        return false;
    //verify axis/ratio and params for full ellipse
    DRW_Ellipse normalized = *ent;
    normalized.correctAxis();
    if (version > DRW::AC1009) {
        EntityRecordScope scope(*this, ent);
        writer->writeString(0, "ELLIPSE");
        if (!writeEntity(ent))
            return false;
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbEllipse");
        }
        writer->writeDouble(10, normalized.basePoint.x);
        writer->writeDouble(20, normalized.basePoint.y);
        writer->writeDouble(30, normalized.basePoint.z);
        writer->writeDouble(11, normalized.secPoint.x);
        writer->writeDouble(21, normalized.secPoint.y);
        writer->writeDouble(31, normalized.secPoint.z);
        writer->writeDouble(40, normalized.ratio);
        writer->writeDouble(41, normalized.staparam);
        writer->writeDouble(42, normalized.endparam);
        // extrusion(210/220/230) — a tilted-plane ellipse loses its orientation
        // (and partial arcs can flip sweep) without it; reader + applyExtrusion
        // depend on it. Default 0,0,1.
        DRW_Coord crd = normalized.extPoint;
        if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
            writer->writeDouble(210, crd.x);
            writer->writeDouble(220, crd.y);
            writer->writeDouble(230, crd.z);
        }
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    } else {
        DRW_Polyline pol;
        //RLZ: copy properties
        normalized.toPolyline(&pol, elParts);
        return writePolyline(&pol);
    }
    return !writer->hasWriteError();
}

bool dxfRW::writeTrace(DRW_Trace *ent){
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "TRACE");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbTrace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    if (ent->thickness != 0.0)
        writer->writeDouble(39, ent->thickness);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeSolid(DRW_Solid *ent){
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "SOLID");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbTrace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    if (ent->thickness != 0.0)
        writer->writeDouble(39, ent->thickness);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::write3dface(DRW_3Dface *ent){
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "3DFACE");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbFace");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->thirdPoint.x);
    writer->writeDouble(22, ent->thirdPoint.y);
    writer->writeDouble(32, ent->thirdPoint.z);
    writer->writeDouble(13, ent->fourPoint.x);
    writer->writeDouble(23, ent->fourPoint.y);
    writer->writeDouble(33, ent->fourPoint.z);
    writer->writeInt16(70, ent->invisibleflag);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeLWPolyline(DRW_LWPolyline *ent){
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (writer == nullptr || ent == nullptr || !ent->validatePayloadFields()) {
        m_writeError = true;
        return false;
    }

    {
        EntityRecordScope scope(*this, ent);
        writer->writeString(0, "LWPOLYLINE");
        if (!writeEntity(ent))
            return false;
        writer->writeString(100, "AcDbPolyline");
        const int vertexCount = static_cast<int>(ent->vertlist.size());
        writer->writeInt32(90, vertexCount);
        writer->writeInt16(70, ent->flags);
        writer->writeDouble(43, ent->width);
        if (ent->elevation != 0)
            writer->writeDouble(38, ent->elevation);
        if (ent->thickness != 0)
            writer->writeDouble(39, ent->thickness);
        for (const auto& v : ent->vertlist) {
            writer->writeDouble(10, v->x);
            writer->writeDouble(20, v->y);
            if (v->stawidth != 0)
                writer->writeDouble(40, v->stawidth);
            if (v->endwidth != 0)
                writer->writeDouble(41, v->endwidth);
            if (v->bulge != 0)
                writer->writeDouble(42, v->bulge);
            if (version > DRW::AC1021 && v->identifier != 0)
                writer->writeInt32(91, v->identifier);
        }
        // extrusion(210/220/230) — UCS/extruded plines flatten to WCS without it
        // (reader reads it, DWG encoder preserves it). Default 0,0,1.
        DRW_Coord crd = ent->extPoint;
        if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
            writer->writeDouble(210, crd.x);
            writer->writeDouble(220, crd.y);
            writer->writeDouble(230, crd.z);
        }
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    }
    return !writer->hasWriteError();
}

bool dxfRW::writePolyline(DRW_Polyline *ent) {
    if (writer == nullptr || ent == nullptr || !ent->validatePayloadFields()
        || std::any_of(ent->vertlist.cbegin(), ent->vertlist.cend(),
                       [](const auto& vertex) {
                           return vertex == nullptr
                               || !vertex->validatePayloadFields();
                       })) {
        m_writeError = true;
        return false;
    }
    RecordStateScope state(*this, ent);
    DxfWriterRecordScope record(*writer);
    writer->writeString(0, "POLYLINE");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        if (ent->flags & 8 || ent->flags & 16)
            writer->writeString(100, "AcDb3dPolyline");
        else
            writer->writeString(100, "AcDb2dPolyline");
    } else
        writer->writeInt16(66, 1);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, ent->basePoint.z);
    if (ent->thickness != 0) {
        writer->writeDouble(39, ent->thickness);
        }
    writer->writeInt16(70, ent->flags);
    if (ent->defstawidth != 0) {
        writer->writeDouble(40, ent->defstawidth);
        }
    if (ent->defendwidth != 0) {
        writer->writeDouble(41, ent->defendwidth);
    }
    if (ent->flags & 16 || ent->flags & 32) {
        writer->writeInt16(71, ent->vertexcount);
        writer->writeInt16(72, ent->facecount);
    }
    if (ent->smoothM != 0) {
        writer->writeInt16(73, ent->smoothM);
    }
    if (ent->smoothN != 0) {
        writer->writeInt16(74, ent->smoothN);
    }
    if (ent->curvetype != 0) {
        writer->writeInt16(75, ent->curvetype);
    }
    DRW_Coord crd  = ent->extPoint;
    if (crd.x != 0 || crd.y != 0 || crd.z != 1) {
        writer->writeDouble(210, crd.x);
        writer->writeDouble(220, crd.y);
        writer->writeDouble(230, crd.z);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;

    const std::uint32_t parentHandle = ent->handle;
    for (const auto& vertex : ent->vertlist) {
        DRW_Vertex *v = vertex.get();
        writer->writeString(0, "VERTEX");
        if (!writeEntity(v, /*captureSourceHandle=*/true, parentHandle))
            return false;
        if (version > DRW::AC1009) {
            // R2000+ requires a type-specific second subclass marker after
            // AcDbVertex (a face record uses ONLY AcDbFaceRecord). Mirrors
            // ezdxf polyline.py vertex classification; without it AutoCAD/ezdxf
            // mis-type 3D/mesh/polyface vertices.
            if ((v->flags & 128) && (v->flags & 64)) {
                writer->writeString(100, "AcDbFaceRecord");
            } else {
                writer->writeString(100, "AcDbVertex");
                if (v->flags & 128)
                    writer->writeString(100, "AcDbPolyFaceMeshVertex");
                else if (ent->flags & 16)
                    writer->writeString(100, "AcDbPolyFaceMeshVertex");
                else if (ent->flags & 32)
                    writer->writeString(100, "AcDbPolygonMeshVertex");
                else if (ent->flags & 8)
                    writer->writeString(100, "AcDb3dPolylineVertex");
                else
                    writer->writeString(100, "AcDb2dVertex");
            }
        }
        if ( (v->flags & 128) && !(v->flags & 64) ) {
            writer->writeDouble(10, 0);
            writer->writeDouble(20, 0);
            writer->writeDouble(30, 0);
        } else {
            writer->writeDouble(10, v->basePoint.x);
            writer->writeDouble(20, v->basePoint.y);
            writer->writeDouble(30, v->basePoint.z);
        }
        if (v->stawidth != 0)
            writer->writeDouble(40, v->stawidth);
        if (v->endwidth != 0)
            writer->writeDouble(41, v->endwidth);
        if (v->bulge != 0)
            writer->writeDouble(42, v->bulge);
        if (v->flags != 0) {
            writer->writeInt16(70, v->flags);
        }
        if (v->flags & 2) {
            writer->writeDouble(50, v->tgdir);
        }
        if ( v->flags & 128 ) {
            if (v->vindex1 != 0) {
                writer->writeInt16(71, v->vindex1);
            }
            if (v->vindex2 != 0) {
                writer->writeInt16(72, v->vindex2);
            }
            if (v->vindex3 != 0) {
                writer->writeInt16(73, v->vindex3);
            }
            if (v->vindex4 != 0) {
                writer->writeInt16(74, v->vindex4);
            }
            if ( !(v->flags & 64) ) {
                writer->writeInt32(91, v->identifier);
            }
        }
    }
    if (!writeSequenceEnd(parentHandle))
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeSpline(DRW_Spline *ent){
    if (version > DRW::AC1009) {
        if (writer == nullptr || ent == nullptr
            || !ent->validatePayloadFields(/*allowMixedLists=*/true)
            || ent->knotslist.size()
                   > static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())
            || ent->controllist.size()
                   > static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())
            || ent->fitlist.size()
                   > static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())) {
            m_writeError = true;
            return false;
        }
        EntityRecordScope scope(*this, ent);
        writer->writeString(0, "SPLINE");
        if (!writeEntity(ent))
            return false;
        writer->writeString(100, "AcDbSpline");
        writeDxfSplineBody(writer.get(), ent);
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    } else {
        // R12 has no SPLINE record and this writer does not approximate it.
        return rejectUnsupportedDxfWrite();
    }
    return !writer->hasWriteError();
}

bool dxfRW::writeHelix(DRW_Helix *ent){
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (version > DRW::AC1009) {
        if (writer == nullptr || ent == nullptr
            || !ent->validatePayloadFields(/*allowMixedLists=*/true)
            || ent->knotslist.size()
                   > static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())
            || ent->controllist.size()
                   > static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())
            || ent->fitlist.size()
                   > static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())
            || !std::isfinite(ent->axisBasePt.x)
            || !std::isfinite(ent->axisBasePt.y)
            || !std::isfinite(ent->axisBasePt.z)
            || !std::isfinite(ent->startPt.x)
            || !std::isfinite(ent->startPt.y)
            || !std::isfinite(ent->startPt.z)
            || !std::isfinite(ent->axisVector.x)
            || !std::isfinite(ent->axisVector.y)
            || !std::isfinite(ent->axisVector.z)
            || !std::isfinite(ent->radius)
            || !std::isfinite(ent->turns)
            || !std::isfinite(ent->turnHeight)) {
            m_writeError = true;
            return false;
        }
        EntityRecordScope scope(*this, ent);
        writer->writeString(0, "HELIX");
        if (!writeEntity(ent))
            return false;
        writer->writeString(100, "AcDbSpline");
        writeDxfSplineBody(writer.get(), ent);
        writer->writeString(100, "AcDbHelix");
        writer->writeInt32(90, ent->m_majorVersion);
        writer->writeInt32(91, ent->m_maintVersion);
        writer->writeDouble(10, ent->axisBasePt.x);
        writer->writeDouble(20, ent->axisBasePt.y);
        writer->writeDouble(30, ent->axisBasePt.z);
        writer->writeDouble(11, ent->startPt.x);
        writer->writeDouble(21, ent->startPt.y);
        writer->writeDouble(31, ent->startPt.z);
        writer->writeDouble(12, ent->axisVector.x);
        writer->writeDouble(22, ent->axisVector.y);
        writer->writeDouble(32, ent->axisVector.z);
        writer->writeDouble(40, ent->radius);
        writer->writeDouble(41, ent->turns);
        writer->writeDouble(42, ent->turnHeight);
        writer->writeBool(290, ent->handedness);
        writer->writeInt16(280, static_cast<int>(ent->constraintType));
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    }
    return !writer->hasWriteError();
}

bool dxfRW::writeHatch(DRW_Hatch *ent){
    if (version > DRW::AC1009) {
        if (!preflightEntity(ent) || !validateHatchPayload(ent)) {
            m_writeError = true;
            return false;
        }
        EntityRecordScope scope(*this, ent);
        writer->writeString(0, "HATCH");
        if (!writeEntity(ent))
            return false;
        writer->writeString(100, "AcDbHatch");
        writer->writeDouble(10, 0.0);
        writer->writeDouble(20, 0.0);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeString(2, ent->name);
        writer->writeInt16(70, ent->solid);
        writer->writeInt16(71, ent->associative);
        const int loopCount = static_cast<int>(ent->looplist.size());
        writer->writeInt32(91, loopCount);
        //write paths data
        for (int i = 0; i < loopCount; i++){
            DRW_HatchLoop *loop = ent->looplist.at(i).get();
            writer->writeInt32(92, loop->type);
            if ((loop->type & 2) == 2) {
                // Polyline boundary path
                DRW_LWPolyline *pl = nullptr;
                if (!loop->objlist.empty())
                    pl = dynamic_cast<DRW_LWPolyline*>(loop->objlist.at(0).get());
                const bool hasBulge = pl && std::any_of(
                    pl->vertlist.begin(), pl->vertlist.end(),
                    [](const std::shared_ptr<DRW_Vertex2D>& v){ return v && v->bulge != 0.0; });
                writer->writeInt16(72, hasBulge ? 1 : 0);
                writer->writeInt16(73, pl ? (pl->flags & 1) : 0); // is-closed
                const int nv = pl ? static_cast<int>(pl->vertlist.size()) : 0;
                writer->writeInt32(93, nv);
                for (int v = 0; v < nv; ++v) {
                    const auto &vtx = pl->vertlist.at(v);
                    writer->writeDouble(10, vtx->x);
                    writer->writeDouble(20, vtx->y);
                    if (hasBulge)
                        writer->writeDouble(42, vtx->bulge);
                }
                // Emit source boundary handles (associative hatch) or 0.
                if (!loop->m_boundaryHandles.empty()) {
                    writer->writeInt32(97, static_cast<int>(loop->m_boundaryHandles.size()));
                    for (std::uint32_t h : loop->m_boundaryHandles)
                        writer->writeString(330, toHexStr(h));
                } else {
                    writer->writeInt32(97, 0);
                }
            } else {
                //boundary path
                const int edgeCount = static_cast<int>(loop->objlist.size());
                writer->writeInt32(93, edgeCount);
                for (int j = 0; j < edgeCount; ++j) {
                    switch ((loop->objlist.at(j))->eType) {
                        case DRW::LINE: {
                        writer->writeInt16(72, 1);
                        DRW_Line* l = (DRW_Line*)loop->objlist.at(j).get();
                        writer->writeDouble(10, l->basePoint.x);
                        writer->writeDouble(20, l->basePoint.y);
                        writer->writeDouble(11, l->secPoint.x);
                        writer->writeDouble(21, l->secPoint.y);
                            break;
                        }
                        case DRW::ARC: {
                        writer->writeInt16(72, 2);
                        DRW_Arc* a = (DRW_Arc*)loop->objlist.at(j).get();
                        writer->writeDouble(10, a->basePoint.x);
                        writer->writeDouble(20, a->basePoint.y);
                        writer->writeDouble(40, a->radious);
                        writer->writeDouble(50, a->staangle*ARAD);
                        writer->writeDouble(51, a->endangle*ARAD);
                        writer->writeInt16(73, a->isccw);
                            break;
                        }
                        case DRW::ELLIPSE: {
                        writer->writeInt16(72, 3);
                        DRW_Ellipse* a = (DRW_Ellipse*)loop->objlist.at(j).get();
                        DRW_Ellipse normalized = *a;
                        normalized.correctAxis();
                        writer->writeDouble(10, normalized.basePoint.x);
                        writer->writeDouble(20, normalized.basePoint.y);
                        writer->writeDouble(11, normalized.secPoint.x);
                        writer->writeDouble(21, normalized.secPoint.y);
                        writer->writeDouble(40, normalized.ratio);
                        writer->writeDouble(50, normalized.staparam*ARAD);
                        writer->writeDouble(51, normalized.endparam*ARAD);
                        writer->writeInt16(73, normalized.isccw);
                            break;
                        }
                        case DRW::SPLINE:{
                        writer->writeInt16(72, 4);
                            DRW_Spline* sp = (DRW_Spline*)loop->objlist.at(j).get();
                        writer->writeInt32(94, sp->degree);
                            const bool rational = (sp->flags & 0x4) != 0;
                            const bool periodic = (sp->flags & 0x2) != 0;
                        writer->writeInt16(73, rational ? 1 : 0);
                        writer->writeInt16(74, periodic ? 1 : 0);
                        writer->writeInt32(95, static_cast<int>(sp->knotslist.size()));
                        writer->writeInt32(96, static_cast<int>(sp->controllist.size()));
                            for (double k : sp->knotslist) {
                            writer->writeDouble(40, k);
                            }
                        for (size_t k = 0; k < sp->controllist.size(); ++k) {
                                const auto& cp = sp->controllist[k];
                                if (!cp) continue;
                            writer->writeDouble(10, cp->x);
                            writer->writeDouble(20, cp->y);
                                if (rational) {
                                    double w = (k < sp->weightlist.size()) ? sp->weightlist[k] : 1.0;
                                writer->writeDouble(42, w);
                                }
                            }
                        writer->writeInt32(97, static_cast<int>(sp->fitlist.size()));
                            for (const auto& fp : sp->fitlist) {
                                if (!fp) continue;
                            writer->writeDouble(11, fp->x);
                            writer->writeDouble(21, fp->y);
                            }
                        // start/end tangents (codes 12/22, 13/23)
                        if (sp->tgStart.x != 0.0 || sp->tgStart.y != 0.0) {
                            writer->writeDouble(12, sp->tgStart.x);
                            writer->writeDouble(22, sp->tgStart.y);
                        }
                        if (sp->tgEnd.x != 0.0 || sp->tgEnd.y != 0.0) {
                            writer->writeDouble(13, sp->tgEnd.x);
                            writer->writeDouble(23, sp->tgEnd.y);
                        }
                            break;
                        }
                        default:
                            break;
                    }
                }
                // Emit source boundary handles (associative hatch) or 0.
                if (!loop->m_boundaryHandles.empty()) {
                    writer->writeInt32(97, static_cast<int>(loop->m_boundaryHandles.size()));
                    for (std::uint32_t h : loop->m_boundaryHandles)
                        writer->writeString(330, toHexStr(h));
                } else {
                    writer->writeInt32(97, 0);
                }
            }
        }
        writer->writeInt16(75, ent->hstyle);
        writer->writeInt16(76, ent->hpattern);
        if (!ent->solid){
            writer->writeDouble(52, ent->angle);
            writer->writeDouble(41, ent->scale);
            writer->writeInt16(77, ent->doubleflag);
        }
        // code 78 (def-line count) is written for both solid and pattern fills
        const int nDefLines = static_cast<int>(ent->patternLines.size());
        writer->writeInt16(78, nDefLines);
        for (const DRW_Hatch::PatternLine &pl : ent->patternLines) {
            writer->writeDouble(53, pl.angle);
            writer->writeDouble(43, pl.baseX);
            writer->writeDouble(44, pl.baseY);
            writer->writeDouble(45, pl.offsetX);
            writer->writeDouble(46, pl.offsetY);
            writer->writeInt16(79, static_cast<int>(pl.dashList.size()));
            for (double d : pl.dashList)
                writer->writeDouble(49, d);
        }
        if (ent->pixelSize != 0.0)
            writer->writeDouble(47, ent->pixelSize);
        // Seed points (group 98 = count, then 10/20 pairs).
        const int seedCount = static_cast<int>(ent->seedPoints.size());
        writer->writeInt32(98, seedCount);
        for (const DRW_Coord &pt : ent->seedPoints) {
            writer->writeDouble(10, pt.x);
            writer->writeDouble(20, pt.y);
        }
        // Gradient block (R2004+ DXF; codes 450-470 + 463/421/63 per stop).
        if (ent->isGradient) {
            writer->writeInt32(450, ent->isGradient);
            writer->writeInt32(451, ent->gradReserved);
            writer->writeDouble(460, ent->gradAngle);
            writer->writeDouble(461, ent->gradShift);
            writer->writeInt32(452, ent->singleColor);
            writer->writeDouble(462, ent->gradTint);
            writer->writeInt32(453, static_cast<int>(ent->gradColors.size()));
            for (const DRW_Hatch::GradientStop &stop : ent->gradColors) {
                writer->writeDouble(463, stop.value);
                if (stop.aciColor != 0)
                    writer->writeInt16(63, stop.aciColor);
                if (stop.rgb >= 0)
                    writer->writeInt32(421, stop.rgb);
                if (stop.colorMethod != 0)
                    writer->writeInt32(431, stop.colorMethod);
                if (!stop.colorName.empty())
                    writer->writeUtf8String(432, stop.colorName);
                if (!stop.colorBookName.empty())
                    writer->writeUtf8String(433, stop.colorBookName);
            }
            writer->writeUtf8String(470, ent->gradName);
        }
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    } else {
        return rejectUnsupportedDxfWrite();
    }
    return !writer->hasWriteError();
}

bool dxfRW::writeMPolygon(DRW_MPolygon *ent){
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (version > DRW::AC1009) {
        if (!preflightEntity(ent) || !validateHatchPayload(ent)) {
            m_writeError = true;
            return false;
        }
        EntityRecordScope scope(*this, ent);
        writer->writeString(0, "MPOLYGON");
        if (!writeEntity(ent))
            return false;
        writer->writeString(100, "AcDbMPolygon");
        writer->writeInt16(70, ent->solid);
        writer->writeDouble(10, 0.0);
        writer->writeDouble(20, 0.0);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeString(2, ent->name);
        writer->writeInt16(71, ent->associative);
        const int loopCount = static_cast<int>(ent->looplist.size());
        writer->writeInt32(91, loopCount);
        for (int i = 0; i < loopCount; ++i) {
            DRW_HatchLoop *loop = ent->looplist.at(i).get();
            writer->writeInt32(92, loop->type);
            if ((loop->type & 2) == 2) {
                DRW_LWPolyline *pl = nullptr;
                if (!loop->objlist.empty())
                    pl = dynamic_cast<DRW_LWPolyline*>(loop->objlist.at(0).get());
                const bool hasBulge = pl && std::any_of(
                    pl->vertlist.begin(), pl->vertlist.end(),
                    [](const std::shared_ptr<DRW_Vertex2D>& v){ return v && v->bulge != 0.0; });
                writer->writeInt16(72, hasBulge ? 1 : 0);
                writer->writeInt16(73, pl ? (pl->flags & 1) : 0);
                const int nv = pl ? static_cast<int>(pl->vertlist.size()) : 0;
                writer->writeInt32(93, nv);
                for (int v = 0; v < nv; ++v) {
                    const auto &vtx = pl->vertlist.at(v);
                    writer->writeDouble(10, vtx->x);
                    writer->writeDouble(20, vtx->y);
                    if (hasBulge)
                        writer->writeDouble(42, vtx->bulge);
                }
                writer->writeInt32(97, static_cast<int>(loop->m_boundaryHandles.size()));
                for (std::uint32_t h : loop->m_boundaryHandles)
                    writer->writeString(330, toHexStr(h));
            } else {
                const int edgeCount = static_cast<int>(loop->objlist.size());
                writer->writeInt32(93, edgeCount);
                for (int j = 0; j < edgeCount; ++j) {
                    switch ((loop->objlist.at(j))->eType) {
                    case DRW::LINE: {
                        writer->writeInt16(72, 1);
                        DRW_Line* l = (DRW_Line*)loop->objlist.at(j).get();
                        writer->writeDouble(10, l->basePoint.x);
                        writer->writeDouble(20, l->basePoint.y);
                        writer->writeDouble(11, l->secPoint.x);
                        writer->writeDouble(21, l->secPoint.y);
                        break; }
                    case DRW::ARC: {
                        writer->writeInt16(72, 2);
                        DRW_Arc* a = (DRW_Arc*)loop->objlist.at(j).get();
                        writer->writeDouble(10, a->basePoint.x);
                        writer->writeDouble(20, a->basePoint.y);
                        writer->writeDouble(40, a->radious);
                        writer->writeDouble(50, a->staangle*ARAD);
                        writer->writeDouble(51, a->endangle*ARAD);
                        writer->writeInt16(73, a->isccw);
                        break; }
                    case DRW::ELLIPSE: {
                        writer->writeInt16(72, 3);
                        DRW_Ellipse* a = (DRW_Ellipse*)loop->objlist.at(j).get();
                        DRW_Ellipse normalized = *a;
                        normalized.correctAxis();
                        writer->writeDouble(10, normalized.basePoint.x);
                        writer->writeDouble(20, normalized.basePoint.y);
                        writer->writeDouble(11, normalized.secPoint.x);
                        writer->writeDouble(21, normalized.secPoint.y);
                        writer->writeDouble(40, normalized.ratio);
                        writer->writeDouble(50, normalized.staparam*ARAD);
                        writer->writeDouble(51, normalized.endparam*ARAD);
                        writer->writeInt16(73, normalized.isccw);
                        break; }
                    case DRW::SPLINE: {
                        writer->writeInt16(72, 4);
                        DRW_Spline* sp = (DRW_Spline*)loop->objlist.at(j).get();
                        writer->writeInt32(94, sp->degree);
                        const bool rational = (sp->flags & 0x4) != 0;
                        const bool periodic = (sp->flags & 0x2) != 0;
                        writer->writeInt16(73, rational ? 1 : 0);
                        writer->writeInt16(74, periodic ? 1 : 0);
                        writer->writeInt32(95, static_cast<int>(sp->knotslist.size()));
                        writer->writeInt32(96, static_cast<int>(sp->controllist.size()));
                        for (double k : sp->knotslist)
                            writer->writeDouble(40, k);
                        for (size_t k = 0; k < sp->controllist.size(); ++k) {
                            const auto& cp = sp->controllist[k];
                            if (!cp) continue;
                            writer->writeDouble(10, cp->x);
                            writer->writeDouble(20, cp->y);
                            if (rational) {
                                double w = (k < sp->weightlist.size()) ? sp->weightlist[k] : 1.0;
                                writer->writeDouble(42, w);
                            }
                        }
                        writer->writeInt32(97, static_cast<int>(sp->fitlist.size()));
                        for (const auto& fp : sp->fitlist) {
                            if (!fp) continue;
                            writer->writeDouble(11, fp->x);
                            writer->writeDouble(21, fp->y);
                        }
                        if (sp->tgStart.x != 0.0 || sp->tgStart.y != 0.0) {
                            writer->writeDouble(12, sp->tgStart.x);
                            writer->writeDouble(22, sp->tgStart.y);
                        }
                        if (sp->tgEnd.x != 0.0 || sp->tgEnd.y != 0.0) {
                            writer->writeDouble(13, sp->tgEnd.x);
                            writer->writeDouble(23, sp->tgEnd.y);
                        }
                        break; }
                    default:
                        break;
                    }
                }
                writer->writeInt32(97, static_cast<int>(loop->m_boundaryHandles.size()));
                for (std::uint32_t h : loop->m_boundaryHandles)
                    writer->writeString(330, toHexStr(h));
            }
        }
        writer->writeInt16(75, ent->hstyle);
        writer->writeInt16(76, ent->hpattern);
        if (!ent->solid) {
            writer->writeDouble(52, ent->angle);
            writer->writeDouble(41, ent->scale);
            writer->writeInt16(77, ent->doubleflag);
        }
        const int nDefLines = static_cast<int>(ent->patternLines.size());
        writer->writeInt16(78, nDefLines);
        for (const DRW_Hatch::PatternLine &pl : ent->patternLines) {
            writer->writeDouble(53, pl.angle);
            writer->writeDouble(43, pl.baseX);
            writer->writeDouble(44, pl.baseY);
            writer->writeDouble(45, pl.offsetX);
            writer->writeDouble(46, pl.offsetY);
            writer->writeInt16(79, static_cast<int>(pl.dashList.size()));
            for (double d : pl.dashList)
                writer->writeDouble(49, d);
        }
        if (ent->fillColorAci != 0)
            writer->writeInt16(63, ent->fillColorAci);
        if (ent->fillColorRgb >= 0)
            writer->writeInt32(421, ent->fillColorRgb);
        if (!ent->fillColorName.empty())
            writer->writeUtf8String(430, ent->fillColorName);
        writer->writeDouble(11, ent->xDirX);
        writer->writeDouble(21, ent->xDirY);
        writer->writeInt32(99, ent->degenerateLoops);
        if (ent->isGradient) {
            writer->writeInt32(450, ent->isGradient);
            writer->writeInt32(451, ent->gradReserved);
            writer->writeDouble(460, ent->gradAngle);
            writer->writeDouble(461, ent->gradShift);
            writer->writeInt32(452, ent->singleColor);
            writer->writeDouble(462, ent->gradTint);
            writer->writeInt32(453, static_cast<int>(ent->gradColors.size()));
            for (const DRW_Hatch::GradientStop &stop : ent->gradColors) {
                writer->writeDouble(463, stop.value);
                if (stop.aciColor != 0)
                    writer->writeInt16(63, stop.aciColor);
                if (stop.rgb >= 0)
                    writer->writeInt32(421, stop.rgb);
                if (stop.colorMethod != 0)
                    writer->writeInt32(431, stop.colorMethod);
                if (!stop.colorName.empty())
                    writer->writeUtf8String(432, stop.colorName);
                if (!stop.colorBookName.empty())
                    writer->writeUtf8String(433, stop.colorBookName);
            }
            writer->writeUtf8String(470, ent->gradName);
        }
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    }
    return !writer->hasWriteError();
}

bool dxfRW::writeLeader(DRW_Leader *ent){
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (writer == nullptr || ent == nullptr || !ent->validatePayloadFields()
        || ent->vertexlist.size()
               > static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max())) {
        m_writeError = true;
        return false;
    }

    {
        EntityRecordScope scope(*this, ent);
        writer->writeString(0, "LEADER");
        if (!writeEntity(ent))
            return false;
        writer->writeString(100, "AcDbLeader");
        writer->writeUtf8String(3, ent->style);
        writer->writeInt16(71, ent->arrow);
        writer->writeInt16(72, ent->leadertype);
        writer->writeInt16(73, ent->flag);
        writer->writeInt16(74, ent->hookline);
        writer->writeInt16(75, ent->hookflag);
        writer->writeDouble(40, ent->textheight);
        writer->writeDouble(41, ent->textwidth);
        writer->writeInt16(76, static_cast<int>(ent->vertexlist.size()));
        for (const auto& vert : ent->vertexlist) {
            writer->writeDouble(10, vert->x);
            writer->writeDouble(20, vert->y);
            writer->writeDouble(30, vert->z);
        }
        // block_color (77): color used when the leader annotation color is
        // BYBLOCK; ezdxf acdb_leader emits it unconditionally (default 7).
        writer->writeInt16(77, ent->coloruse);
        // annotation_handle (340): hard ref to the associated annotation,
        // resolved through the source->minted map; skip when unresolved so we
        // never emit a dangling handle (mirrors the GROUP-340 policy).
        if (ent->annotHandle != 0) {
            auto it = m_writingContext.sourceHandleToMintedMap.find(ent->annotHandle);
            if (it != m_writingContext.sourceHandleToMintedMap.end())
                writer->writeString(340, toHexStr(it->second));
        }
        if (ent->extrusionPoint.x != 0.0 || ent->extrusionPoint.y != 0.0 ||
            ent->extrusionPoint.z != 1.0) {
            writer->writeDouble(210, ent->extrusionPoint.x);
            writer->writeDouble(220, ent->extrusionPoint.y);
            writer->writeDouble(230, ent->extrusionPoint.z);
        }
        // horizontal_direction (211), offset-from-block (212), offset-from-
        // annotation (213) — emitted after 210 per ezdxf order, when non-default.
        if (ent->horizdir.x != 1.0 || ent->horizdir.y != 0.0 || ent->horizdir.z != 0.0) {
            writer->writeDouble(211, ent->horizdir.x);
            writer->writeDouble(221, ent->horizdir.y);
            writer->writeDouble(231, ent->horizdir.z);
        }
        if (ent->offsetblock.x != 0.0 || ent->offsetblock.y != 0.0 || ent->offsetblock.z != 0.0) {
            writer->writeDouble(212, ent->offsetblock.x);
            writer->writeDouble(222, ent->offsetblock.y);
            writer->writeDouble(232, ent->offsetblock.z);
        }
        if (ent->offsettext.x != 0.0 || ent->offsettext.y != 0.0 || ent->offsettext.z != 0.0) {
            writer->writeDouble(213, ent->offsettext.x);
            writer->writeDouble(223, ent->offsettext.y);
            writer->writeDouble(233, ent->offsettext.z);
        }
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    }
    return !writer->hasWriteError();
}
bool dxfRW::writeArcDimension(DRW_DimArc *d) {
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (!preflightEntity(d))
        return false;
    EntityRecordScope scope(*this, d);
    writer->writeString(0, "ARC_DIMENSION");
    if (!writeEntity(d))
        return false;
    writer->writeString(100, "AcDbDimension");
    if (version >= DRW::AC1024)
        writer->writeInt16(280, 0);   // AcDbDimension version, 0 = R2010+
    if (!d->getName().empty())
        writer->writeString(2, d->getName());
    writer->writeDouble(10, d->getArcDefPoint().x);
    writer->writeDouble(20, d->getArcDefPoint().y);
    writer->writeDouble(30, d->getArcDefPoint().z);
    writer->writeDouble(11, d->getTextPoint().x);
    writer->writeDouble(21, d->getTextPoint().y);
    writer->writeDouble(31, d->getTextPoint().z);
    // ARC_DIMENSION: subtype 5 in low 3 bits (same as angular3p); preserve high bits.
    const int dimType = (d->type & ~0x07) | 5 | 32;
    writer->writeInt16(70, dimType);
    if (!d->getText().empty())
        writer->writeUtf8String(1, d->getText());
    writer->writeInt16(71, d->getAlign());
    if (d->getTextLineStyle() != 1)
        writer->writeInt16(72, d->getTextLineStyle());
    if (d->getTextLineFactor() != 1)
        writer->writeDouble(41, d->getTextLineFactor());
    writer->writeUtf8String(3, d->getStyle());
    if (d->getDir() != 0)
        writer->writeDouble(53, d->getDir());
    writer->writeDouble(210, d->getExtrusion().x);
    writer->writeDouble(220, d->getExtrusion().y);
    writer->writeDouble(230, d->getExtrusion().z);
    writer->writeString(100, "AcDbArcDimension");
    writer->writeDouble(13, d->getExtLine1().x);
    writer->writeDouble(23, d->getExtLine1().y);
    writer->writeDouble(33, d->getExtLine1().z);
    writer->writeDouble(14, d->getExtLine2().x);
    writer->writeDouble(24, d->getExtLine2().y);
    writer->writeDouble(34, d->getExtLine2().z);
    writer->writeDouble(15, d->getArcCenter().x);
    writer->writeDouble(25, d->getArcCenter().y);
    writer->writeDouble(35, d->getArcCenter().z);
    writer->writeInt16(70, d->arcSymbol);
    writer->writeDouble(40, d->arcStartAngle);
    writer->writeDouble(41, d->arcEndAngle);
    writer->writeInt16(71, d->isPartial ? 1 : 0);
    DRW_Coord lp1 = d->hasLeader ? d->getLeaderPt1() : d->getExtLine1();
    DRW_Coord lp2 = d->hasLeader ? d->leaderPt2      : d->getExtLine2();
    writer->writeDouble(16, lp1.x); writer->writeDouble(26, lp1.y); writer->writeDouble(36, lp1.z);
    writer->writeDouble(17, lp2.x); writer->writeDouble(27, lp2.y); writer->writeDouble(37, lp2.z);
    if (!d->extData.empty() && !writeExtData(d->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeLargeRadialDimension(DRW_DimLargeRadial *d) {
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (!preflightEntity(d))
        return false;
    EntityRecordScope scope(*this, d);
    writer->writeString(0, "LARGE_RADIAL_DIMENSION");
    if (!writeEntity(d))
        return false;
    writer->writeString(100, "AcDbDimension");
    if (version >= DRW::AC1024)
        writer->writeInt16(280, 0);
    if (!d->getName().empty())
        writer->writeString(2, d->getName());
    writer->writeDouble(10, d->getCenterPoint().x);
    writer->writeDouble(20, d->getCenterPoint().y);
    writer->writeDouble(30, d->getCenterPoint().z);
    writer->writeDouble(11, d->getTextPoint().x);
    writer->writeDouble(21, d->getTextPoint().y);
    writer->writeDouble(31, d->getTextPoint().z);
    int dimType = (d->type & ~0x07) | 4;
    if (!(dimType & 32))
        dimType += 32;
    writer->writeInt16(70, dimType);
    if (!d->getText().empty())
        writer->writeUtf8String(1, d->getText());
    writer->writeInt16(71, d->getAlign());
    if (d->getTextLineStyle() != 1)
        writer->writeInt16(72, d->getTextLineStyle());
    if (d->getTextLineFactor() != 1)
        writer->writeDouble(41, d->getTextLineFactor());
    writer->writeUtf8String(3, d->getStyle());
    if (d->measureValue != 0)
        writer->writeDouble(42, d->measureValue);
    if (d->getDir() != 0)
        writer->writeDouble(53, d->getDir());
    if (d->hdir != 0)
        writer->writeDouble(51, d->hdir);
    if (d->getFlipArrow1())
        writer->writeInt16(74, 1);
    if (d->getFlipArrow2())
        writer->writeInt16(75, 1);
    if (d->genTol)
        writer->writeInt16(76, 1);
    if (d->limGen)
        writer->writeInt16(77, 1);
    if (d->tolPlus != 0)
        writer->writeDouble(43, d->tolPlus);
    if (d->tolMinus != 0)
        writer->writeDouble(44, d->tolMinus);
    if (d->tolScale != 0)
        writer->writeDouble(45, d->tolScale);
    if (d->tolDecimals != 0)
        writer->writeInt16(78, d->tolDecimals);
    if (d->tolAlign != 0)
        writer->writeInt16(79, d->tolAlign);
    if (d->tolZero != 0)
        writer->writeInt16(80, d->tolZero);
    if (d->altTolDecimals != 0)
        writer->writeInt16(81, d->altTolDecimals);
    if (d->altZero != 0)
        writer->writeInt16(82, d->altZero);
    if (d->altTolZero != 0)
        writer->writeInt16(83, d->altTolZero);
    if (d->textMove != 0)
        writer->writeInt16(84, d->textMove);
    writer->writeDouble(210, d->getExtrusion().x);
    writer->writeDouble(220, d->getExtrusion().y);
    writer->writeDouble(230, d->getExtrusion().z);
    writer->writeString(100, "AcDbRadialDimensionLarge");
    writer->writeDouble(13, d->getChordPoint().x);
    writer->writeDouble(23, d->getChordPoint().y);
    writer->writeDouble(33, d->getChordPoint().z);
    writer->writeDouble(14, d->overrideCenterPoint.x);
    writer->writeDouble(24, d->overrideCenterPoint.y);
    writer->writeDouble(34, d->overrideCenterPoint.z);
    writer->writeDouble(15, d->jogPoint.x);
    writer->writeDouble(25, d->jogPoint.y);
    writer->writeDouble(35, d->jogPoint.z);
    writer->writeDouble(40, d->jogAngle);
    if (!d->extData.empty() && !writeExtData(d->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeDimension(DRW_Dimension *ent) {
    if (!preflightEntity(ent))
        return false;
    if (ent->eType == DRW::DIMARC)
        return writeArcDimension(static_cast<DRW_DimArc*>(ent));
    if (auto *largeRadial = dynamic_cast<DRW_DimLargeRadial*>(ent))
        return writeLargeRadialDimension(largeRadial);
    if (version > DRW::AC1009) {
        EntityRecordScope scope(*this, ent);
        writer->writeString(0, "DIMENSION");
        if (!writeEntity(ent))
            return false;
        writer->writeString(100, "AcDbDimension");
        if (version >= DRW::AC1024)
            writer->writeInt16(280, 0);   // AcDbDimension version, 0 = R2010+
        if (!ent->getName().empty()){
            writer->writeString(2, ent->getName());
        }
        writer->writeDouble(10, ent->getDefPoint().x);
        writer->writeDouble(20, ent->getDefPoint().y);
        writer->writeDouble(30, ent->getDefPoint().z);
        writer->writeDouble(11, ent->getTextPoint().x);
        writer->writeDouble(21, ent->getTextPoint().y);
        writer->writeDouble(31, ent->getTextPoint().z);
        const int dimType = ent->type | 32;
        writer->writeInt16(70, dimType);
        if ( !(ent->getText().empty()) )
            writer->writeUtf8String(1, ent->getText());
        writer->writeInt16(71, ent->getAlign());
        if ( ent->getTextLineStyle() != 1)
            writer->writeInt16(72, ent->getTextLineStyle());
        if ( ent->getTextLineFactor() != 1)
            writer->writeDouble(41, ent->getTextLineFactor());
        writer->writeUtf8String(3, ent->getStyle());
        // Measurement(42), horizontal direction(51) and flip-arrow flags(74/75)
        // — the DXF reader consumes all four (drw_entities.cpp parseCode) and the
        // DWG encoder writes hdir/flipArrow1/flipArrow2; emit them for symmetry.
        if (ent->measureValue != 0)
            writer->writeDouble(42, ent->measureValue);
        if (ent->getDir() != 0)
            writer->writeDouble(53, ent->getDir());
        if (ent->hdir != 0)
            writer->writeDouble(51, ent->hdir);
        if (ent->getFlipArrow1())
            writer->writeInt16(74, 1);
        if (ent->getFlipArrow2())
            writer->writeInt16(75, 1);
        if (ent->genTol)
            writer->writeInt16(76, 1);
        if (ent->limGen)
            writer->writeInt16(77, 1);
        if (ent->tolPlus != 0)
            writer->writeDouble(43, ent->tolPlus);
        if (ent->tolMinus != 0)
            writer->writeDouble(44, ent->tolMinus);
        if (ent->tolScale != 0)
            writer->writeDouble(45, ent->tolScale);
        if (ent->tolDecimals != 0)
            writer->writeInt16(78, ent->tolDecimals);
        if (ent->tolAlign != 0)
            writer->writeInt16(79, ent->tolAlign);
        if (ent->tolZero != 0)
            writer->writeInt16(80, ent->tolZero);
        if (ent->altTolDecimals != 0)
            writer->writeInt16(81, ent->altTolDecimals);
        if (ent->altZero != 0)
            writer->writeInt16(82, ent->altZero);
        if (ent->altTolZero != 0)
            writer->writeInt16(83, ent->altTolZero);
        if (ent->textMove != 0)
            writer->writeInt16(84, ent->textMove);
        writer->writeDouble(210, ent->getExtrusion().x);
        writer->writeDouble(220, ent->getExtrusion().y);
        writer->writeDouble(230, ent->getExtrusion().z);

        switch (ent->eType) {
            case DRW::DIMALIGNED:
            case DRW::DIMLINEAR: {
            DRW_DimAligned * dd = (DRW_DimAligned*)ent;
            writer->writeString(100, "AcDbAlignedDimension");
                DRW_Coord crd = dd->getClonepoint();
                if (crd.x != 0 || crd.y != 0 || crd.z != 0) {
                writer->writeDouble(12, crd.x);
                writer->writeDouble(22, crd.y);
                writer->writeDouble(32, crd.z);
                }
            writer->writeDouble(13, dd->getDef1Point().x);
            writer->writeDouble(23, dd->getDef1Point().y);
            writer->writeDouble(33, dd->getDef1Point().z);
            writer->writeDouble(14, dd->getDef2Point().x);
            writer->writeDouble(24, dd->getDef2Point().y);
            writer->writeDouble(34, dd->getDef2Point().z);
                if (ent->eType == DRW::DIMLINEAR) {
                DRW_DimLinear * dl = (DRW_DimLinear*)ent;
                if (dl->getAngle() != 0)
                    writer->writeDouble(50, dl->getAngle());
                if (dl->getOblique() != 0)
                    writer->writeDouble(52, dl->getOblique());
                writer->writeString(100, "AcDbRotatedDimension");
                }
                break;
            }
            case DRW::DIMRADIAL: {
            DRW_DimRadial * dd = (DRW_DimRadial*)ent;
            writer->writeString(100, "AcDbRadialDimension");
            writer->writeDouble(15, dd->getDiameterPoint().x);
            writer->writeDouble(25, dd->getDiameterPoint().y);
            writer->writeDouble(35, dd->getDiameterPoint().z);
            writer->writeDouble(40, dd->getLeaderLength());
                break;
            }
            case DRW::DIMDIAMETRIC: {
            DRW_DimDiametric * dd = (DRW_DimDiametric*)ent;
            writer->writeString(100, "AcDbDiametricDimension");
            writer->writeDouble(15, dd->getDiameter1Point().x);
            writer->writeDouble(25, dd->getDiameter1Point().y);
            writer->writeDouble(35, dd->getDiameter1Point().z);
            writer->writeDouble(40, dd->getLeaderLength());
                break;
            }
            case DRW::DIMANGULAR: {
            DRW_DimAngular * dd = (DRW_DimAngular*)ent;
            writer->writeString(100, "AcDb2LineAngularDimension");
            writer->writeDouble(13, dd->getFirstLine1().x);
            writer->writeDouble(23, dd->getFirstLine1().y);
            writer->writeDouble(33, dd->getFirstLine1().z);
            writer->writeDouble(14, dd->getFirstLine2().x);
            writer->writeDouble(24, dd->getFirstLine2().y);
            writer->writeDouble(34, dd->getFirstLine2().z);
            writer->writeDouble(15, dd->getSecondLine1().x);
            writer->writeDouble(25, dd->getSecondLine1().y);
            writer->writeDouble(35, dd->getSecondLine1().z);
            writer->writeDouble(16, dd->getDimPoint().x);
            writer->writeDouble(26, dd->getDimPoint().y);
            writer->writeDouble(36, dd->getDimPoint().z);
            break; }
            case DRW::DIMANGULAR3P: {
            DRW_DimAngular3p * dd = (DRW_DimAngular3p*)ent;
            writer->writeString(100, "AcDb3PointAngularDimension");
            writer->writeDouble(13, dd->getFirstLine().x);
            writer->writeDouble(23, dd->getFirstLine().y);
            writer->writeDouble(33, dd->getFirstLine().z);
            writer->writeDouble(14, dd->getSecondLine().x);
            writer->writeDouble(24, dd->getSecondLine().y);
            writer->writeDouble(34, dd->getSecondLine().z);
            writer->writeDouble(15, dd->getVertexPoint().x);
            writer->writeDouble(25, dd->getVertexPoint().y);
            writer->writeDouble(35, dd->getVertexPoint().z);
            break; }
            case DRW::DIMORDINATE: {
            DRW_DimOrdinate * dd = (DRW_DimOrdinate*)ent;
            writer->writeString(100, "AcDbOrdinateDimension");
            writer->writeDouble(13, dd->getFirstLine().x);
            writer->writeDouble(23, dd->getFirstLine().y);
            writer->writeDouble(33, dd->getFirstLine().z);
            writer->writeDouble(14, dd->getSecondLine().x);
            writer->writeDouble(24, dd->getSecondLine().y);
            writer->writeDouble(34, dd->getSecondLine().z);
            break; }
            default:
                break;
        }
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    } else  {
        return rejectUnsupportedDxfWrite();
    }
    return !writer->hasWriteError();
}

bool dxfRW::writeInsert(DRW_Insert *ent){
    if (!preflightEntity(ent)
        || ent->name.empty()
        || !isSafeDxfRecordText(ent->name)
        || ent->colcount <= 0
        || ent->rowcount <= 0
        || ent->colcount > std::numeric_limits<std::uint16_t>::max()
        || ent->rowcount > std::numeric_limits<std::uint16_t>::max()
        || std::any_of(ent->attlist.cbegin(), ent->attlist.cend(),
                       [](const auto& attribute) {
                           return attribute == nullptr
                               || !isValidDxfEntityFields(*attribute)
                               || !isSafeDxfRecordText(attribute->tag);
                       }))
    {
        m_writeError = true;
        return false;
    }
    RecordStateScope state(*this, ent);
    DxfWriterRecordScope record(*writer);
    const bool hasAttribs = !ent->attlist.empty();
    const bool isMInsert = ent->isMInsert();
    // DXF represents a multiple insert as an INSERT record with the
    // AcDbMInsertBlock subclass and grid groups.  MINSERT is the distinct DWG
    // object type, not the standard DXF group-0 entity name.
    writer->writeString(0, "INSERT");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, isMInsert ? "AcDbMInsertBlock" : "AcDbBlockReference");
        if (hasAttribs)
            writer->writeInt16(66, 1); //attributes-follow flag
        writer->writeUtf8String(2, ent->name);
    } else {
        if (hasAttribs)
            writer->writeInt16(66, 1);
        writer->writeUtf8Caps(2, ent->name);
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(41, ent->xscale);
    writer->writeDouble(42, ent->yscale);
    writer->writeDouble(43, ent->zscale);
    writer->writeDouble(50, (ent->angle)*ARAD); //in dxf angle is writed in degrees
    writer->writeInt16(70, ent->colcount);
    writer->writeInt16(71, ent->rowcount);
    writer->writeDouble(44, ent->colspace);
    writer->writeDouble(45, ent->rowspace);
    // extrusion(210/220/230) — OCS inserts re-import in the wrong plane without
    // it (reader reads it, DWG encoder preserves it). Default 0,0,1.
    if (ent->extPoint.x != 0 || ent->extPoint.y != 0 || ent->extPoint.z != 1) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    //Trailing block attributes + terminating SEQEND (mirrors writePolyline).
    if (hasAttribs) {
        const std::uint32_t insertHandle = ent->handle;
        for (const auto &att : ent->attlist) {
            if (att && !writeAttrib(att.get(), insertHandle))
                return false;
        }
        if (!writeSequenceEnd(insertHandle))
            return false;
    }
    if (writer->hasWriteError() || !record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeTable(DRW_Table *ent){
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "ACAD_TABLE");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbBlockReference");

    const UTF8STRING blockName = ent->name.empty() ? UTF8STRING("*T1") : ent->name;
    writer->writeUtf8String(2, blockName);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    if (ent->xscale != 1.0)
        writer->writeDouble(41, ent->xscale);
    if (ent->yscale != 1.0)
        writer->writeDouble(42, ent->yscale);
    if (ent->zscale != 1.0)
        writer->writeDouble(43, ent->zscale);
    if (ent->angle != 0.0)
        writer->writeDouble(50, ent->angle * ARAD);
    if (ent->extPoint.x != 0 || ent->extPoint.y != 0 || ent->extPoint.z != 1) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }

    writer->writeString(100, "AcDbTable");
    if (ent->m_tableStyleHandle != 0)
        writer->writeString(342, toHexStr(ent->m_tableStyleHandle));

    DRW_Coord horizontal = ent->m_horizontalDirection;
    if (horizontal.x == 0.0 && horizontal.y == 0.0 && horizontal.z == 0.0)
        horizontal.x = 1.0;
    writer->writeDouble(11, horizontal.x);
    writer->writeDouble(21, horizontal.y);
    writer->writeDouble(31, horizontal.z);

    const std::size_t rows = ent->m_content.m_rows.size();
    const std::size_t columns = tableColumnCount(*ent);
    writer->writeInt32(90, ent->m_valueFlag);
    writer->writeInt32(91, static_cast<int>(rows));
    writer->writeInt32(92, static_cast<int>(columns));
    writer->writeInt32(93, 0);
    writer->writeInt32(94, 0);
    writer->writeInt32(95, 0);
    writer->writeInt32(96, 0);

    for (const auto& row : ent->m_content.m_rows)
        writer->writeDouble(141, row.m_height);
    for (std::size_t column = 0; column < columns; ++column) {
        const double width = column < ent->m_content.m_columns.size()
            ? ent->m_content.m_columns[column].m_width
            : 0.0;
        writer->writeDouble(142, width);
    }

    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            const DRW_TableCell *cell = nullptr;
            if (column < ent->m_content.m_rows[row].m_cells.size())
                cell = &ent->m_content.m_rows[row].m_cells[column];
            const UTF8STRING text = cell == nullptr ? UTF8STRING() : tableCellText(*cell);
            writer->writeInt16(171, 1);
            writer->writeInt16(172, 0);
            writer->writeInt16(173, 0);
            writer->writeInt16(174, 0);
            writer->writeInt16(175, 1);
            writer->writeInt16(176, 1);
            writer->writeInt32(91, 0);
            writer->writeInt16(178, 0);
            writer->writeDouble(145, 0.0);
            writer->writeInt32(92, 0);
            writer->writeUtf8String(301, "CELL_VALUE");
            writer->writeInt32(93, text.empty() ? 3 : 2);
            writer->writeInt32(90, text.empty() ? 0 : 4);
            if (!text.empty())
                writer->writeUtf8String(1, text);
            else
                writer->writeInt32(91, 0);
            writer->writeInt32(94, 0);
            writer->writeUtf8String(300, "");
            writer->writeUtf8String(302, text);
            writer->writeUtf8String(304, "ACVALUE_END");
        }
    }

    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeEmbeddedMText(DRW_MText *ent) {
    if (ent == nullptr)
        return false;

    writer->writeString(100, "Embedded Object");
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeDouble(41, ent->widthscale);
    if (ent->m_r2018RectHeight != 0.0)
        writer->writeDouble(46, ent->m_r2018RectHeight);
    writer->writeInt16(71, ent->textgen);
    writer->writeInt16(72, ent->alignH);
    writer->writeUtf8String(1, ent->text);
    if (!ent->style.empty())
        writer->writeUtf8String(7, ent->style);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0
        || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    if (ent->secPoint.x != 0.0 || ent->secPoint.y != 0.0
        || ent->secPoint.z != 0.0) {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    }
    if (ent->m_r2018ExtentsWidth != 0.0)
        writer->writeDouble(42, ent->m_r2018ExtentsWidth);
    if (ent->m_r2018ExtentsHeight != 0.0)
        writer->writeDouble(43, ent->m_r2018ExtentsHeight);
    if (ent->angle != 0.0)
        writer->writeDouble(50, ent->angle);
    writer->writeInt16(73, ent->linespacingStyle);
    writer->writeDouble(44, ent->interlin);
    if (ent->m_backgroundFlags != 0) {
        writer->writeDouble(45, ent->m_backgroundScale);
        writer->writeInt32(90, ent->m_backgroundFlags);
        writer->writeInt16(63, ent->m_backgroundColor);
        writer->writeInt32(441, ent->m_backgroundTransparency);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeAttributeR2018Features(DRW_Attrib *ent) {
    if (ent == nullptr || version < DRW::AC1032 || !ent->mtext)
        return true;
    std::uint8_t attributeType = ent->m_attributeType;
    if (attributeType == 1)
        attributeType = ent->eType == DRW::ATTDEF ? 4 : 2;
    writer->writeInt16(71, attributeType);
    writer->writeInt16(72, 0); // R2018 keep-duplicate-records placeholder.
    writer->writeDouble(11, ent->mtext->basePoint.x);
    writer->writeDouble(21, ent->mtext->basePoint.y);
    writer->writeDouble(31, ent->mtext->basePoint.z);
    return writeEmbeddedMText(ent->mtext.get());
}

bool dxfRW::writeAttrib(DRW_Attrib *ent,
                        std::uint32_t ownerOverride){
    if (!preflightEntity(ent)
        || !isSafeDxfRecordText(ent->tag)
        || ent->m_fieldLength < std::numeric_limits<std::int16_t>::min()
        || ent->m_fieldLength > std::numeric_limits<std::uint16_t>::max()
        || (ent->mtext != nullptr
            && (!isValidDxfEntityFields(*ent->mtext)
                || !isSafeDxfRecordText(ent->mtext->text))))
    {
        m_writeError = true;
        return false;
    }
    if (ent->eType == DRW::ATTDEF) {
        if (auto *attdef = dynamic_cast<DRW_Attdef *>(ent))
            return writeAttdef(attdef, ownerOverride);
    }

    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "ATTRIB");
    if (!writeEntity(ent, true, ownerOverride))
        return false;
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbText");
    if (ent->thickness != 0)  // reader + DWG encoder preserve thickness(39)
        writer->writeDouble(39, ent->thickness);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(50, ent->angle);
    writer->writeDouble(41, ent->widthscale);
    writer->writeDouble(51, ent->oblique);
    if (version > DRW::AC1009)
        writer->writeUtf8String(7, ent->style);
    else
        writer->writeUtf8Caps(7, ent->style);
    writer->writeInt16(71, ent->textgen);
    if (ent->alignH != DRW_Text::HLeft)
        writer->writeInt16(72, ent->alignH);
    if (ent->alignH != DRW_Text::HLeft || ent->alignV != DRW_Text::VBaseLine) {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    }
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbAttribute");
    writer->writeUtf8String(2, ent->tag);
    writer->writeInt16(70, ent->attribFlags);
    writer->writeInt16(73, ent->m_fieldLength);
    if (ent->alignV != DRW_Text::VBaseLine)
        writer->writeInt16(74, ent->alignV);
    if (version > DRW::AC1014)
        writer->writeInt16(280, ent->lockPosition ? 1 : 0);
    if (!writeAttributeR2018Features(ent))
        return false;
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeAttdef(DRW_Attdef *ent,
                        std::uint32_t ownerOverride){
    if (!preflightEntity(ent)
        || !isSafeDxfRecordText(ent->tag)
        || !isSafeDxfRecordText(ent->prompt)
        || ent->m_fieldLength < std::numeric_limits<std::int16_t>::min()
        || ent->m_fieldLength > std::numeric_limits<std::uint16_t>::max()
        || (ent->mtext != nullptr
            && (!isValidDxfEntityFields(*ent->mtext)
                || !isSafeDxfRecordText(ent->mtext->text))))
    {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "ATTDEF");
    if (!writeEntity(ent, true, ownerOverride))
        return false;
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbText");
    if (ent->thickness != 0)
        writer->writeDouble(39, ent->thickness);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(50, ent->angle);
    writer->writeDouble(41, ent->widthscale);
    writer->writeDouble(51, ent->oblique);
    if (version > DRW::AC1009)
        writer->writeUtf8String(7, ent->style);
    else
        writer->writeUtf8Caps(7, ent->style);
    writer->writeInt16(71, ent->textgen);
    if (ent->alignH != DRW_Text::HLeft)
        writer->writeInt16(72, ent->alignH);
    if (ent->alignH != DRW_Text::HLeft || ent->alignV != DRW_Text::VBaseLine) {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    }
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbAttributeDefinition");
    writer->writeUtf8String(2, ent->tag);
    writer->writeUtf8String(3, ent->prompt);
    writer->writeInt16(70, ent->attribFlags);
    writer->writeInt16(73, ent->m_fieldLength);
    if (ent->alignV != DRW_Text::VBaseLine)
        writer->writeInt16(74, ent->alignV);
    if (version > DRW::AC1014)
        writer->writeInt16(280, ent->lockPosition ? 1 : 0);
    if (!writeAttributeR2018Features(ent))
        return false;
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeText(DRW_Text *ent) {
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "TEXT");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbText");
    }
    if (ent->thickness != 0)  // reader + DWG encoder preserve thickness(39)
        writer->writeDouble(39, ent->thickness);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(50, ent->angle);
    writer->writeDouble(41, ent->widthscale);
    writer->writeDouble(51, ent->oblique);
    if (version > DRW::AC1009)
        writer->writeUtf8String(7, ent->style);
    else
        writer->writeUtf8Caps(7, ent->style);
    writer->writeInt16(71, ent->textgen);
    if (ent->alignH != DRW_Text::HLeft) {
        writer->writeInt16(72, ent->alignH);
    }
    if (ent->alignH != DRW_Text::HLeft || ent->alignV != DRW_Text::VBaseLine) {
        writer->writeDouble(11, ent->secPoint.x);
        writer->writeDouble(21, ent->secPoint.y);
        writer->writeDouble(31, ent->secPoint.z);
    }
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbText");
    }
    if (ent->alignV != DRW_Text::VBaseLine) {
        writer->writeInt16(73, ent->alignV);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

static double arcAlignedDxfValue(const UTF8STRING& value, double fallback) {
    if (value.empty())
        return fallback;
    try {
        return std::stod(value);
    } catch (...) {
        return fallback;
    }
}

bool dxfRW::writeRText(DRW_RText *ent) {
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "RTEXT");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "RText");
    writer->writeUtf8String(7, ent->style.empty() ? "Standard" : ent->style);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->height);
    writer->writeDouble(50, ent->angle);
    writer->writeInt32(70, ent->m_rTextFlags);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeArcAlignedText(DRW_ArcAlignedText *ent) {
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "ARCALIGNEDTEXT");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbArcAlignedText");
    writer->writeUtf8String(1, ent->text);
    if (!ent->m_fontName.empty())
        writer->writeUtf8String(2, ent->m_fontName);
    if (!ent->m_bigFontName.empty())
        writer->writeUtf8String(3, ent->m_bigFontName);
    writer->writeUtf8String(7, ent->style.empty() ? "Standard" : ent->style);
    writer->writeDouble(10, ent->m_center.x);
    writer->writeDouble(20, ent->m_center.y);
    writer->writeDouble(30, ent->m_center.z);
    writer->writeDouble(40, ent->m_radius);
    writer->writeDouble(41, arcAlignedDxfValue(
        ent->m_xScale, ent->widthscale > 0.0 ? ent->widthscale : 1.0));
    writer->writeDouble(42, arcAlignedDxfValue(
        ent->m_textSize, ent->height > 0.0 ? ent->height : 0.0));
    writer->writeDouble(43, arcAlignedDxfValue(ent->m_charSpacing, 1.0));
    writer->writeDouble(44, arcAlignedDxfValue(ent->m_offsetFromArc, 0.0));
    writer->writeDouble(45, arcAlignedDxfValue(ent->m_rightOffset, 0.0));
    writer->writeDouble(46, arcAlignedDxfValue(ent->m_leftOffset, 0.0));
    writer->writeDouble(50, ent->m_startAngle * ARAD);
    writer->writeDouble(51, ent->m_endAngle * ARAD);
    writer->writeInt32(90, ent->m_rawColor);
    writer->writeInt32(77, ent->m_characterSet);
    writer->writeInt32(78, ent->m_pitchAndFamily);
    writer->writeInt32(79, ent->m_isShx);
    writer->writeInt32(74, ent->m_isBold);
    writer->writeInt32(75, ent->m_isItalic);
    writer->writeInt32(76, ent->m_isUnderlined);
    writer->writeInt32(72, ent->m_alignment);
    writer->writeInt32(70, ent->m_isReverse);
    writer->writeInt32(280, ent->m_wizardFlag);
    writer->writeInt32(73, ent->m_textPosition);
    writer->writeInt32(71, ent->m_textDirection);
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeTolerance(DRW_Tolerance *ent){
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "TOLERANCE");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbFcf");
    writer->writeUtf8String(3, ent->dimStyleName);
    writer->writeDouble(10, ent->insertionPoint.x);
    writer->writeDouble(20, ent->insertionPoint.y);
    writer->writeDouble(30, ent->insertionPoint.z);
    writer->writeUtf8String(1, ent->text);
    writer->writeDouble(210, ent->extPoint.x);
    writer->writeDouble(220, ent->extPoint.y);
    writer->writeDouble(230, ent->extPoint.z);
    writer->writeDouble(11, ent->xAxisDirectionVector.x);
    writer->writeDouble(21, ent->xAxisDirectionVector.y);
    writer->writeDouble(31, ent->xAxisDirectionVector.z);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeMLine(DRW_MLine *ent) {
    if (version <= DRW::AC1009) return rejectUnsupportedDxfWrite();
    if (ent == nullptr || writer == nullptr || !ent->validatePayloadFields()) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "MLINE");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbMline");
    writer->writeUtf8String(2, ent->styleName);
    if (ent->styleHandle != 0) {
        writer->writeString(340, toHexStr(ent->styleHandle));
    }
    writer->writeDouble(40, ent->scale);
    writer->writeInt16(70, ent->justification);
    writer->writeInt16(71, ent->openClosed);
    writer->writeInt16(72, static_cast<int>(ent->vertlist.size()));
    writer->writeInt16(73, static_cast<int>(ent->numLines));
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    for (const auto& v : ent->vertlist) {
        writer->writeDouble(11, v.position.x);
        writer->writeDouble(21, v.position.y);
        writer->writeDouble(31, v.position.z);
        writer->writeDouble(12, v.vertexDir.x);
        writer->writeDouble(22, v.vertexDir.y);
        writer->writeDouble(32, v.vertexDir.z);
        writer->writeDouble(13, v.miterDir.x);
        writer->writeDouble(23, v.miterDir.y);
        writer->writeDouble(33, v.miterDir.z);
        for (int li = 0; li < ent->numLines; ++li) {
            const auto& seg = (li < static_cast<int>(v.segParms.size()))
                                  ? v.segParms[li] : std::vector<double>{};
            const auto& fill = (li < static_cast<int>(v.areaFillParms.size()))
                                   ? v.areaFillParms[li] : std::vector<double>{};
            writer->writeInt16(74, static_cast<int>(seg.size()));
            for (double p : seg) writer->writeDouble(41, p);
            writer->writeInt16(75, static_cast<int>(fill.size()));
            for (double p : fill) writer->writeDouble(42, p);
        }
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeUnderlay(DRW_Underlay *ent) {
    if (version <= DRW::AC1009) return rejectUnsupportedDxfWrite();
    if (ent == nullptr || writer == nullptr || !ent->validatePayloadFields()
        || ent->clipBoundary.size() > DRW_Underlay::kMaxClipVertices
        || ent->inverseClipBoundary.size() > DRW_Underlay::kMaxClipVertices
        || (version <= DRW::AC1021
            && (!ent->inverseClipBoundary.empty() || (ent->flags & 0x10) != 0))) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    const bool hasInverseClip = !ent->inverseClipBoundary.empty()
                                || (ent->flags & 0x10) != 0;
    const std::uint8_t wireFlags =
        hasInverseClip ? static_cast<std::uint8_t>(ent->flags | 0x10) : ent->flags;
    const char* tag = (ent->kind == DRW_Underlay::DGN) ? "DGNUNDERLAY"
                    : (ent->kind == DRW_Underlay::DWF) ? "DWFUNDERLAY"
                    : "PDFUNDERLAY";
    writer->writeString(0, tag);
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbUnderlayReference");
    if (ent->definitionHandle != 0) {
        writer->writeString(340, toHexStr(ent->definitionHandle));
    }
    writer->writeDouble(10, ent->position.x);
    writer->writeDouble(20, ent->position.y);
    writer->writeDouble(30, ent->position.z);
    if (ent->scale.x != 1.0 || ent->scale.y != 1.0 || ent->scale.z != 1.0) {
        writer->writeDouble(41, ent->scale.x);
        writer->writeDouble(42, ent->scale.y);
        writer->writeDouble(43, ent->scale.z);
    }
    writer->writeDouble(50, ent->rotation);
    if (ent->extPoint.x != 0.0 || ent->extPoint.y != 0.0 || ent->extPoint.z != 1.0) {
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
    }
    writer->writeInt16(280, wireFlags);
    writer->writeInt16(281, ent->contrast);
    writer->writeInt16(282, ent->fade);
    for (const auto& v : ent->clipBoundary) {
        writer->writeDouble(11, v.x);
        writer->writeDouble(21, v.y);
    }
    if (hasInverseClip) {
        writer->writeInt16(170, static_cast<int>(ent->inverseClipBoundary.size()));
        for (const auto& v : ent->inverseClipBoundary) {
            writer->writeDouble(12, v.x);
            writer->writeDouble(22, v.y);
        }
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeUnderlayDefinition(DRW_UnderlayDefinition *ent) {
    if (version <= DRW::AC1009) return rejectUnsupportedDxfWrite();
    if (!preflightTableEntry(ent))
        return false;
    const char* tag = (ent->kind == DRW_UnderlayDefinition::DGN) ? "DGNDEFINITION"
                    : (ent->kind == DRW_UnderlayDefinition::DWF) ? "DWFDEFINITION"
                    : "PDFDEFINITION";
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, tag);
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbUnderlayDefinition");
    writer->writeUtf8String(1, ent->filename);
    writer->writeUtf8String(2, ent->sheetName);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeMText(DRW_MText *ent){
    if (version > DRW::AC1009) {
        if (!preflightEntity(ent))
            return false;
        EntityRecordScope scope(*this, ent);
        writer->writeString(0, "MTEXT");
        if (!writeEntity(ent))
            return false;
        writer->writeString(100, "AcDbMText");
        writer->writeDouble(10, ent->basePoint.x);
        writer->writeDouble(20, ent->basePoint.y);
        writer->writeDouble(30, ent->basePoint.z);
        writer->writeDouble(40, ent->height);
        writer->writeDouble(41, ent->widthscale);
        writer->writeInt16(71, ent->textgen);
        writer->writeInt16(72, ent->alignH);
        // Chunk on UTF-8 codepoint boundaries so a multi-byte character (or
        // its codepage/DBCS encoding, or a \U+/\M+ escape) is never split
        // across the group-3/group-1 records. The previous code split the
        // post-codec byte string at a fixed 250 bytes, corrupting DBCS pairs
        // and escape sequences. Encoded per-codepoint chunks stay <=250 bytes;
        // for pure-ASCII text the output is byte-identical to the old 250-split.
        const std::string& utf8 = ent->text;
        std::vector<std::string> chunks;
        std::string cur;
        for (std::size_t p = 0; p < utf8.size(); ) {
            unsigned char c = static_cast<unsigned char>(utf8[p]);
            std::size_t cl = (c < 0x80) ? 1 : ((c >> 5) == 0x6) ? 2
                           : ((c >> 4) == 0xE) ? 3 : ((c >> 3) == 0x1E) ? 4 : 1;
            if (p + cl > utf8.size()) cl = utf8.size() - p;
            std::string enc = writer->fromUtf8String(utf8.substr(p, cl));
            if (!cur.empty() && cur.size() + enc.size() > 250) {
                chunks.push_back(cur);
                cur.clear();
            }
            cur += enc;
            p += cl;
        }
        chunks.push_back(cur);  // final (group 1); empty when text is empty
        for (std::size_t k = 0; k + 1 < chunks.size(); ++k)
            writer->writeString(3, chunks[k]);
        writer->writeString(1, chunks.back());
        writer->writeString(7, ent->style);
        writer->writeDouble(210, ent->extPoint.x);
        writer->writeDouble(220, ent->extPoint.y);
        writer->writeDouble(230, ent->extPoint.z);
        writer->writeDouble(50, ent->angle);
        // MTEXT may carry an explicit WCS x-axis direction (groups 11/21/31)
        // instead of deriving its rotation from group 50. Preserve it when
        // present; a zero vector means the optional field was absent.
        if (ent->secPoint.x != 0.0 || ent->secPoint.y != 0.0
            || ent->secPoint.z != 0.0) {
            writer->writeDouble(11, ent->secPoint.x);
            writer->writeDouble(21, ent->secPoint.y);
            writer->writeDouble(31, ent->secPoint.z);
        }
        writer->writeInt16(73, ent->linespacingStyle);  // linespacing style (was: alignV)
        writer->writeDouble(44, ent->interlin);
//RLZ ... 11, 21, 31 needed?
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    } else {
        return rejectUnsupportedDxfWrite();
    }
    return !writer->hasWriteError();
}

bool dxfRW::writeLight(DRW_Light *ent) {
    // AcDbLight is an R2007+ (AC1021+) entity; pre-R2007 DXF has no LIGHT entity,
    // so skip rather than emit something AutoCAD/ezdxf would reject. Lights read
    // from a DWG are carried on LibreCAD's metadata shelf and would otherwise be
    // dropped on DWG->DXF export; this re-emits them (D4 write-path preservation).
    if (version < DRW::AC1021)
        return rejectUnsupportedDxfWrite();
    if (!preflightEntity(ent))
        return false;
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "LIGHT");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbLight");
    writer->writeInt32(90, static_cast<int>(ent->m_classVersion));
    writer->writeUtf8String(1, ent->m_name);
    writer->writeInt16(70, static_cast<int>(ent->m_type));
    writer->writeBool(290, ent->m_status);
    // ACI index in 63; a packed true-color value goes in 421 instead.
    if (ent->m_color < 256)
        writer->writeInt16(63, static_cast<int>(ent->m_color));
    else
        writer->writeInt32(421, static_cast<int>(ent->m_color));
    writer->writeBool(291, ent->m_plotGlyph);
    writer->writeDouble(40, ent->m_intensity);
    writer->writeDouble(10, ent->m_position.x);
    writer->writeDouble(20, ent->m_position.y);
    writer->writeDouble(30, ent->m_position.z);
    writer->writeDouble(11, ent->m_target.x);
    writer->writeDouble(21, ent->m_target.y);
    writer->writeDouble(31, ent->m_target.z);
    writer->writeInt16(72, static_cast<int>(ent->m_attenuationType));
    writer->writeBool(292, ent->m_useAttenuationLimits);
    writer->writeDouble(41, ent->m_attenuationStartLimit);
    writer->writeDouble(42, ent->m_attenuationEndLimit);
    writer->writeDouble(50, ent->m_hotspotAngle);
    writer->writeDouble(51, ent->m_falloffAngle);
    writer->writeBool(293, ent->m_castShadows);
    writer->writeInt16(73, static_cast<int>(ent->m_shadowType));
    writer->writeInt32(91, static_cast<int>(ent->m_shadowMapSize));
    writer->writeInt16(280, static_cast<int>(ent->m_shadowMapSoftness));
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeCamera(DRW_Camera *ent) {
    if (!preflightEntity(ent))
        return false;
    if (version < DRW::AC1015)
        return rejectUnsupportedDxfWrite();
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "CAMERA");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbCamera");
    if (ent->m_viewHandle != 0)
        writer->writeString(340, toHexStr(ent->m_viewHandle));
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeGeoPositionMarker(DRW_GeoPositionMarker *ent) {
    if (!preflightEntity(ent))
        return false;
    if (version < DRW::AC1027)
        return rejectUnsupportedDxfWrite();
    if (ent->m_enableFrameText && ent->mtext == nullptr) {
        m_writeError = true;
        return false;
    }

    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "GEOPOSITIONMARKER");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbGeoPositionMarker");
    writer->writeInt32(90, static_cast<int>(ent->m_classVersion));
    writer->writeDouble(10, ent->m_position.x);
    writer->writeDouble(20, ent->m_position.y);
    writer->writeDouble(30, ent->m_position.z);
    writer->writeDouble(40, ent->m_radius);
    writer->writeUtf8String(1, ent->m_notes);
    writer->writeDouble(40, ent->m_landingGap);
    writer->writeBool(290, ent->m_mtextVisible);
    writer->writeInt16(280, ent->m_textAlignment);
    writer->writeBool(290, ent->m_enableFrameText);
    if (ent->m_enableFrameText) {
        // The DXF form stores the embedded object's insertion point separately
        // before the standard Embedded Object MTEXT subclass.
        writer->writeDouble(11, ent->m_position.x);
        writer->writeDouble(21, ent->m_position.y);
        writer->writeDouble(31, ent->m_position.z);
        DRW_MText embedded(*ent->mtext);
        embedded.extData.clear();
        if (!writeEmbeddedMText(&embedded))
            return false;
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeSectionObject(DRW_SectionObject *ent) {
    if (!preflightEntity(ent))
        return false;
    if (version < DRW::AC1021)
        return rejectUnsupportedDxfWrite();
    if (ent->m_verts.size() > DRW_SectionObject::kMaxVertices
        || ent->m_blVerts.size() > DRW_SectionObject::kMaxVertices) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "SECTIONOBJECT");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbSection");
    writer->writeInt32(90, static_cast<int>(ent->m_state));
    writer->writeInt32(91, static_cast<int>(ent->m_flags));
    writer->writeUtf8String(1, ent->m_name);
    writer->writeDouble(10, ent->m_vertDir.x);
    writer->writeDouble(20, ent->m_vertDir.y);
    writer->writeDouble(30, ent->m_vertDir.z);
    writer->writeDouble(40, ent->m_topHeight);
    writer->writeDouble(41, ent->m_bottomHeight);
    writer->writeInt16(70, static_cast<int>(ent->m_indicatorAlpha));
    writer->writeInt32(62, static_cast<int>(ent->m_indicatorColor));
    writer->writeInt32(92, static_cast<int>(ent->m_verts.size()));
    for (const DRW_Coord& point : ent->m_verts) {
        writer->writeDouble(11, point.x);
        writer->writeDouble(21, point.y);
        writer->writeDouble(31, point.z);
    }
    writer->writeInt32(93, static_cast<int>(ent->m_blVerts.size()));
    for (const DRW_Coord& point : ent->m_blVerts) {
        writer->writeDouble(12, point.x);
        writer->writeDouble(22, point.y);
        writer->writeDouble(32, point.z);
    }
    if (ent->m_sectionSettingsHandle != 0)
        writer->writeString(360, toHexStr(ent->m_sectionSettingsHandle));
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeMesh(DRW_Mesh *ent) {
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (ent == nullptr || writer == nullptr || !ent->validateDxfOutput()) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "MESH");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbSubDMesh");
    writer->writeInt16(71, static_cast<int>(ent->version));
    writer->writeInt16(72, ent->blendCrease ? 1 : 0);
    writer->writeInt32(91, ent->subdivisionLevel);
    writer->writeInt32(92, static_cast<int>(ent->vertices.size()));
    for (const DRW_Coord& vertex : ent->vertices) {
        writer->writeDouble(10, vertex.x);
        writer->writeDouble(20, vertex.y);
        writer->writeDouble(30, vertex.z);
    }

    std::int32_t faceStreamCount = 0;
    for (const auto& face : ent->faces)
        faceStreamCount += static_cast<std::int32_t>(face.size() + 1);
    writer->writeInt32(93, faceStreamCount);
    for (const auto& face : ent->faces) {
        writer->writeInt32(90, static_cast<int>(face.size()));
        for (std::int32_t index : face)
            writer->writeInt32(90, index);
    }

    writer->writeInt32(94, static_cast<int>(ent->edges.size()));
    for (const auto& edge : ent->edges) {
        writer->writeInt32(90, edge.first);
        writer->writeInt32(90, edge.second);
    }

    writer->writeInt32(95, static_cast<int>(ent->creases.size()));
    for (double crease : ent->creases)
        writer->writeDouble(140, crease);
    writer->writeInt32(90, static_cast<int>(ent->propertyOverrides.size()));
    for (const auto& overrideData : ent->propertyOverrides) {
        writer->writeInt32(91, overrideData.subEntityMarker);
        writer->writeInt32(92, static_cast<int>(overrideData.propertyTypes.size()));
        for (std::int32_t propertyType : overrideData.propertyTypes)
            writer->writeInt32(90, propertyType);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeShape(DRW_Shape *ent) {
    if (!preflightEntity(ent))
        return false;
    // DXF SHAPE (AcDbShape). The DWG stores only a glyph index; the glyph name
    // lives in the external .shx and is unrecoverable, so group 2 carries the
    // SHAPEFILE/STYLE record name (resolved on read), matching libredwg/ACadSharp.
    // m_rotation/m_oblique are radians (DRW_Shape::parseDwg keeps them un-scaled,
    // unlike DRW_Text) -> convert to DXF degrees.
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
               && std::isfinite(point.z);
    };
    if (!finite(ent->m_insertionPoint) || !finite(ent->m_extrusion)
        || !std::isfinite(ent->m_scale) || !std::isfinite(ent->m_rotation)
        || !std::isfinite(ent->m_widthFactor) || !std::isfinite(ent->m_oblique)
        || !std::isfinite(ent->m_thickness)) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "SHAPE");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbShape");
    if (ent->m_thickness != 0.0)
        writer->writeDouble(39, ent->m_thickness);
    writer->writeDouble(10, ent->m_insertionPoint.x);
    writer->writeDouble(20, ent->m_insertionPoint.y);
    writer->writeDouble(30, ent->m_insertionPoint.z);
    writer->writeDouble(40, ent->m_scale);            // size
    if (!ent->m_styleName.empty())
        writer->writeUtf8String(2, ent->m_styleName); // shape (style) name
    writer->writeDouble(50, ent->m_rotation * ARAD);  // radians -> degrees
    if (ent->m_widthFactor != 1.0)
        writer->writeDouble(41, ent->m_widthFactor);
    if (ent->m_oblique != 0.0)
        writer->writeDouble(51, ent->m_oblique * ARAD);
    if (ent->m_extrusion.x != 0.0 || ent->m_extrusion.y != 0.0 || ent->m_extrusion.z != 1.0) {
        writer->writeDouble(210, ent->m_extrusion.x);
        writer->writeDouble(220, ent->m_extrusion.y);
        writer->writeDouble(230, ent->m_extrusion.z);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeOle2Frame(DRW_Ole2Frame *ent) {
    if (!preflightEntity(ent))
        return false;
    // DXF OLE2FRAME (AcDbOle2Frame). Field order/codes per ACadSharp + dwgread:
    // 70 version, 3 client, 10/11 frame corners, 71 type, 72 mode, 73 lock
    // aspect, 90 length, 310 binary (hex chunks), 1 "OLE" trailer. pt1/pt2
    // were decoded from the OLE payload header on read; the payload is replayed
    // verbatim.
    if (ent->m_payloadTooLarge || ent->m_payloadTruncated
        || ent->m_payloadBytes.size() > DRW_Ole2Frame::kMaxOlePayloadBytes
        || (ent->m_payloadPresent
            && ent->m_declaredPayloadLength != ent->m_payloadBytes.size())) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "OLE2FRAME");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbOle2Frame");
    writer->writeInt16(70, static_cast<int>(ent->m_oleVersion));
    writer->writeUtf8String(3, ent->m_oleClient);
    writer->writeDouble(10, ent->m_pt1.x);
    writer->writeDouble(20, ent->m_pt1.y);
    writer->writeDouble(30, ent->m_pt1.z);
    writer->writeDouble(11, ent->m_pt2.x);
    writer->writeDouble(21, ent->m_pt2.y);
    writer->writeDouble(31, ent->m_pt2.z);
    writer->writeInt16(71, static_cast<int>(ent->m_flags));  // OLE object type
    writer->writeInt16(72, static_cast<int>(ent->m_mode));   // tile/paper-space mode
    writer->writeInt16(73, ent->m_lockAspect);                // lock aspect ratio
    writer->writeInt32(90, static_cast<int>(ent->m_payloadBytes.size()));
    // group 310: payload as hex, 127 bytes (254 hex chars) per record (AutoCAD/
    // dwgread convention; the binary-DXF writer hex-decodes and re-chunks).
    static const char hexd[] = "0123456789ABCDEF";
    const std::vector<std::uint8_t>& data = ent->m_payloadBytes;
    for (std::size_t off = 0; off < data.size(); off += 127) {
        const std::size_t n = std::min<std::size_t>(127, data.size() - off);
        std::string chunk;
        chunk.reserve(n * 2);
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint8_t b = data[off + i];
            chunk.push_back(hexd[b >> 4]);
            chunk.push_back(hexd[b & 0x0F]);
        }
        writer->writeString(310, chunk);
    }
    writer->writeString(1, "OLE");
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeOleFrame(DRW_OleFrame *ent) {
    if (!preflightEntity(ent))
        return false;
    // Legacy OLEFRAME has no OLE2 frame rectangle or DXF mode field. Its
    // interoperable DXF payload is the bounded 70/90/310 contract, followed
    // by the historical OLE trailer.
    if (ent->m_payloadTooLarge || ent->m_payloadTruncated
        || ent->m_payloadBytes.size() > DRW_OleFrame::kMaxOlePayloadBytes
        || (ent->m_payloadPresent
            && ent->m_declaredPayloadLength != ent->m_payloadBytes.size())) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "OLEFRAME");
    if (!writeEntity(ent))
        return false;
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbOleFrame");
    writer->writeInt16(70, static_cast<int>(ent->m_flags));
    writer->writeInt32(90, static_cast<int>(ent->m_payloadBytes.size()));
    static const char hexd[] = "0123456789ABCDEF";
    const std::vector<std::uint8_t>& data = ent->m_payloadBytes;
    for (std::size_t off = 0; off < data.size(); off += 127) {
        const std::size_t n = std::min<std::size_t>(127, data.size() - off);
        std::string chunk;
        chunk.reserve(n * 2);
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint8_t b = data[off + i];
            chunk.push_back(hexd[b >> 4]);
            chunk.push_back(hexd[b & 0x0F]);
        }
        writer->writeString(310, chunk);
    }
    writer->writeString(1, "OLE");
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeViewport(DRW_Viewport *ent) {
    if (ent == nullptr || writer == nullptr || !ent->validatePayloadFields()) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    if (!writer->writeString(0, "VIEWPORT")) {
        m_writeError = true;
        return false;
    }
    if (!writeEntity(ent))
        return false;
    bool result = true;
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbViewport");
    }
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    if (ent->basePoint.z != 0.0)
        writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(40, ent->pswidth);
    writer->writeDouble(41, ent->psheight);
    writer->writeInt16(68, ent->vpstatus);
    writer->writeInt16(69, ent->vpID);
    writer->writeDouble(12, ent->centerPX);
    writer->writeDouble(22, ent->centerPY);
    if (version > DRW::AC1009) {
        writer->writeDouble(13, ent->snapPX);
        writer->writeDouble(23, ent->snapPY);
        writer->writeDouble(14, ent->snapSpPX);
        writer->writeDouble(24, ent->snapSpPY);
        writer->writeDouble(15, ent->gridSpX);
        writer->writeDouble(25, ent->gridSpY);
        writer->writeDouble(16, ent->viewDir.x);
        writer->writeDouble(26, ent->viewDir.y);
        writer->writeDouble(36, ent->viewDir.z);
        writer->writeDouble(17, ent->viewTarget.x);
        writer->writeDouble(27, ent->viewTarget.y);
        writer->writeDouble(37, ent->viewTarget.z);
        writer->writeDouble(42, ent->viewLength);
        writer->writeDouble(43, ent->frontClip);
        writer->writeDouble(44, ent->backClip);
        writer->writeDouble(45, ent->viewHeight);
        writer->writeDouble(50, ent->snapAngle);
        writer->writeDouble(51, ent->twistAngle);
        writer->writeInt16(72, static_cast<int>(ent->circleZoom));
        if (version > DRW::AC1018)
            writer->writeInt16(61, ent->majorGridLines);
        writer->writeInt32(90, ent->statusFlags);
        writer->writeUtf8String(1, ent->styleSheet);
        writer->writeInt16(281, ent->renderMode);
        writer->writeInt16(71, ent->ucsPerViewport ? 1 : 0);
        writer->writeInt16(74, ent->ucsAtOrigin ? 1 : 0);
        writer->writeDouble(110, ent->ucsOrigin.x);
        writer->writeDouble(120, ent->ucsOrigin.y);
        writer->writeDouble(130, ent->ucsOrigin.z);
        writer->writeDouble(111, ent->ucsXAxis.x);
        writer->writeDouble(121, ent->ucsXAxis.y);
        writer->writeDouble(131, ent->ucsXAxis.z);
        writer->writeDouble(112, ent->ucsYAxis.x);
        writer->writeDouble(122, ent->ucsYAxis.y);
        writer->writeDouble(132, ent->ucsYAxis.z);
        writer->writeDouble(146, ent->ucsElevation);
        writer->writeInt16(79, ent->ucsOrthographicType);
        for (const std::uint32_t ref : ent->frozenLayerHandles) {
            if (ref != 0)
                writer->writeString(331, toHexStr(ref));
        }
    }
    if (version > DRW::AC1015)
        writer->writeInt16(170, ent->shadePlotMode);
    if (version > DRW::AC1018) {
        writer->writeInt16(292, ent->useDefaultLighting ? 1 : 0);
        writer->writeInt16(282, ent->defaultLightingType);
        writer->writeDouble(141, ent->brightness);
        writer->writeDouble(142, ent->contrast);
        writer->writeInt16(63, static_cast<int>(ent->ambientColor));
        if (ent->ambientColorRgb >= 0)
            writer->writeInt32(421, ent->ambientColorRgb);
        if (!ent->ambientColorName.empty())
            writer->writeUtf8String(431, ent->ambientColorName);
        if (ent->backgroundHandle != 0)
            writer->writeString(332, toHexStr(ent->backgroundHandle));
        if (ent->shadePlotHandle != 0)
            writer->writeString(333, toHexStr(ent->shadePlotHandle));
    }
    if (version > DRW::AC1014) {
        if (ent->clipBoundaryHandle != 0)
            writer->writeString(340, toHexStr(ent->clipBoundaryHandle));
        if (ent->namedUcsHandle != 0)
            writer->writeString(345, toHexStr(ent->namedUcsHandle));
        if (ent->baseUcsHandle != 0)
            writer->writeString(346, toHexStr(ent->baseUcsHandle));
    }
    if (version > DRW::AC1018) {
        if (ent->visualStyleHandle != 0)
            writer->writeString(348, toHexStr(ent->visualStyleHandle));
        if (ent->m_sunHandle != 0)
            writer->writeString(361, toHexStr(ent->m_sunHandle));
    }
    if (!ent->extData.empty())
        result = writeExtData(ent->extData) && result;
    if (!result || writer->hasWriteError())
        m_writeError = true;
    return result && !writer->hasWriteError();
}

DRW_ImageDef* dxfRW::writeImage(DRW_Image *ent, std::string name){
    if (version <= DRW::AC1009) {
        m_writeError = true;
        return nullptr; // IMAGE is not available in ACAD R12 / earlier.
    }
    if (!preflightEntity(ent))
        return nullptr;
    if (ent == nullptr || writer == nullptr || !ent->validatePayloadFields()) {
        m_writeError = true;
        return nullptr;
    }
    EntityRecordScope scope(*this, ent);
    // A DXF image definition is a named dictionary entry. Normalize an
    // unnamed source before lookup so repeated direct calls cannot create
    // duplicate empty or fallback names.
    if (name.empty()) {
        if (ent->handle != 0) {
            name = std::string("librecad_image_") + toHexStr(ent->handle);
        } else {
            name = "librecad_image";
            const auto hasImageDefinition = [this](const std::string &candidate) {
                return std::any_of(
                    imageDef.cbegin(), imageDef.cend(),
                    [&candidate](const DRW_ImageDef *definition) {
                        return definition != nullptr
                               && definition->name == candidate;
                    });
            };
            for (std::size_t suffix = 1; hasImageDefinition(name); ++suffix)
                name = "librecad_image_" + std::to_string(suffix);
        }
    }
    //search if exist imagedef with this mane (image inserted more than 1 time)
    //RLZ: imagedef_reactor seem needed to read in acad
    DRW_ImageDef *id = NULL;
    std::unique_ptr<DRW_ImageDef> newId;
    for (unsigned int i=0; i<imageDef.size(); i++) {
        if (imageDef.at(i)->name == name ) {
            id = imageDef.at(i);
            continue;
        }
    }
    if (id == NULL) {
        newId = std::make_unique<DRW_ImageDef>();
        id = newId.get();
        if (!allocateDxfHandle(id->handle))
            return nullptr;
    }
    id->name = name;
    std::uint32_t reactorHandle = 0;
    if (!allocateDxfHandle(reactorHandle))
        return nullptr;
    const std::string idReactor = toHexStr(reactorHandle);

    writer->writeString(0, "IMAGE");
    if (!writeEntity(ent)) {
        m_writeError = true;
        return nullptr;
    }
    writer->writeString(100, "AcDbRasterImage");
    writer->writeInt32(90, ent->m_classVersion);
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->vVector.x);
    writer->writeDouble(22, ent->vVector.y);
    writer->writeDouble(32, ent->vVector.z);
    writer->writeDouble(13, ent->sizeu);
    writer->writeDouble(23, ent->sizev);
    writer->writeString(340, toHexStr(id->handle));
    writer->writeInt16(70, ent->m_displayProps);
    writer->writeInt16(280, ent->clip);
    writer->writeInt16(281, ent->brightness);
    writer->writeInt16(282, ent->contrast);
    writer->writeInt16(283, ent->fade);
    writer->writeString(360, idReactor);
    // Clip boundary (ezdxf acdb_raster_image order): type 71, count 91,
    // then 14/24 vertices. A polygonal path (>=3 pts) is type 2; otherwise
    // emit the rectangular default (type 1, two opposite corners in
    // image-pixel coords) so consumers never see count=0.
    if (ent->clipPath.size() == 2) {
        writer->writeInt16(71, 1);
        writer->writeInt32(91, 2);
        for (const DRW_Coord& v : ent->clipPath) {
            writer->writeDouble(14, v.x);
            writer->writeDouble(24, v.y);
        }
    } else if (ent->clipPath.size() >= 3) {
        writer->writeInt16(71, 2);
        writer->writeInt32(91, static_cast<std::int32_t>(ent->clipPath.size()));
        for (const DRW_Coord& v : ent->clipPath) {
            writer->writeDouble(14, v.x);
            writer->writeDouble(24, v.y);
        }
    } else {
        writer->writeInt16(71, 1);
        writer->writeInt32(91, 2);
        writer->writeDouble(14, -0.5);
        writer->writeDouble(24, -0.5);
        writer->writeDouble(14, ent->sizeu - 0.5);
        writer->writeDouble(24, ent->sizev - 0.5);
    }
    if (version >= DRW::AC1024) {
        writer->writeBool(290, ent->clipMode);  // R2010+ clip mode
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData)) {
        return nullptr;
    }
    if (writer->hasWriteError()) {
        m_writeError = true;
        return nullptr;
    }
    try {
        if (newId) {
            // Transfer ownership only after vector insertion succeeds;
            // otherwise a bad_alloc would leak the definition.
            imageDef.push_back(newId.get());
            newId.release();
        }
        if (id->reactors.find(idReactor) != id->reactors.end()) {
            m_writeError = true;
            return nullptr;
        }
        if (m_recordStateScopeDepth != 0) {
            DxfWriteMutation mutation;
            mutation.kind = DxfWriteMutationKind::ImageReactorInsert;
            mutation.key = idReactor;
            mutation.imageDef = id;
            m_dxfWriteMutations.push_back(std::move(mutation));
        }
        const auto inserted = id->reactors.emplace(
            idReactor, toHexStr(ent->handle));
        if (!inserted.second) {
            m_writeError = true;
            return nullptr;
        }
    } catch (...) {
        m_writeError = true;
        return nullptr;
    }
    if (!scope.commit())
        return nullptr;
    return id;
}

// MULTILEADER DXF write.  Mirrors the entity-level field set captured by
// DRW_MLeader::parseCode.  The CONTEXT_DATA{} block is NOT emitted yet —
// a full faithful round-trip requires walking all roots/leader-lines
// with their control-flow markers (302/304 open, 305/303/301 close);
// follow-up.  For now the entity is written as a recognisable
// AcDbMLeader stub plus its scalar fields; consumers that read it back
// see all the override flags + style fields preserved.
bool dxfRW::writeMultiLeader(DRW_MLeader *ent){
    if (!preflightEntity(ent))
        return false;
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "MULTILEADER");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbMLeader");

    const DRW_MLeaderAnnotContext &ctx = ent->context;
    const bool hasContext =
        ctx.hasTextContents || ctx.hasContentsBlock || !ctx.roots.empty();
    auto writeCoord = [&](int xCode, const DRW_Coord &coord) {
        writer->writeDouble(xCode, coord.x);
        writer->writeDouble(xCode + 10, coord.y);
        writer->writeDouble(xCode + 20, coord.z);
    };
    if (hasContext) {
        writer->writeString(300, "CONTEXT_DATA{");
        writer->writeDouble(40, ctx.overallScale);
        writeCoord(10, ctx.contentBasePoint);
        writer->writeDouble(41, ctx.textHeight);
        writer->writeDouble(140, ctx.arrowHeadSize);
        writer->writeDouble(145, ctx.landingGap);
        writer->writeInt16(174, ctx.styleLeftAttach);
        writer->writeInt16(175, ctx.styleRightAttach);
        writer->writeInt16(176, ctx.textAlignType);
        writer->writeInt16(177, ctx.attachmentType);
        writer->writeBool(290, ctx.hasTextContents);
        if (ctx.hasTextContents) {
            writer->writeUtf8String(304, ctx.textLabel);
            writeCoord(11, ctx.textNormal);
            writeCoord(12, ctx.textLocation);
            writeCoord(13, ctx.textDirection);
            writer->writeDouble(42, ctx.textRotation);
            writer->writeDouble(43, ctx.boundaryWidth);
            writer->writeDouble(44, ctx.boundaryHeight);
            writer->writeDouble(45, ctx.lineSpacingFactor);
            writer->writeInt16(170, ctx.lineSpacingStyle);
            writer->writeInt32(90, ctx.textColor);
            writer->writeInt16(171, ctx.alignment);
            writer->writeInt16(172, ctx.flowDirection);
            writer->writeInt32(91, ctx.bgFillColor);
            writer->writeDouble(141, ctx.bgScaleFactor);
            writer->writeInt32(92, ctx.bgTransparency);
            writer->writeBool(291, ctx.bgFillEnabled);
            writer->writeBool(292, ctx.bgMaskFillOn);
            writer->writeInt16(173, ctx.columnType);
            writer->writeBool(293, ctx.textHeightAuto);
            writer->writeDouble(142, ctx.columnWidth);
            writer->writeDouble(143, ctx.columnGutter);
            writer->writeBool(294, ctx.columnFlowReversed);
            for (double columnSize : ctx.columnSizes)
                writer->writeDouble(144, columnSize);
            writer->writeBool(295, ctx.wordBreak);
        }
        writer->writeBool(296, ctx.hasContentsBlock);
        if (ctx.hasContentsBlock) {
            writeCoord(14, ctx.blockNormal);
            writeCoord(15, ctx.blockLocation);
            writeCoord(16, ctx.blockScale);
            writer->writeDouble(46, ctx.blockRotation);
            writer->writeInt32(93, ctx.blockColor);
        }

        for (const DRW_MLeaderRoot &root : ctx.roots) {
            writer->writeString(302, "LEADER{");
            writer->writeBool(290, root.isContentValid);
            writer->writeBool(291, root.unknown291);
            writeCoord(10, root.connectionPoint);
            writeCoord(11, root.direction);
            writer->writeInt32(90, root.leaderIndex);
            writer->writeDouble(40, root.landingDistance);
            for (const DRW_MLeaderLeaderLine &line : root.leaderLines) {
                writer->writeString(304, "LEADER_LINE{");
                for (const DRW_Coord &point : line.points)
                    writeCoord(10, point);
                writer->writeInt32(90, line.segmentIndex);
                writer->writeInt32(91, line.leaderLineIndex);
                writer->writeInt32(93, line.overrideFlags);
                writer->writeInt16(170, line.leaderType);
                writer->writeInt32(92, line.color);
                writer->writeInt32(171, line.lineWeight);
                writer->writeDouble(40, line.arrowSize);
                writer->writeString(305, "}");
            }
            writer->writeInt16(271, root.attachmentDirection);
            writer->writeString(303, "}");
        }

        writeCoord(110, ctx.basePoint);
        writeCoord(111, ctx.baseDirection);
        writeCoord(112, ctx.baseVertical);
        writer->writeBool(297, ctx.isNormalReversed);
        writer->writeInt16(273, ctx.styleTopAttach);
        writer->writeInt16(272, ctx.styleBottomAttach);
        writer->writeString(301, "}");
    }

    writer->writeInt32(90, ent->overrideFlags);
    writer->writeInt16(170, ent->leaderType);
    writer->writeInt32(91, ent->leaderColor);
    writer->writeInt16(171, ent->leaderLineWeight);
    writer->writeBool(290, ent->landingEnabled);
    writer->writeBool(291, ent->doglegEnabled);
    writer->writeDouble(41, ent->landingDistance);
    writer->writeDouble(42, ent->defaultArrowHeadSize);
    writer->writeInt16(172, ent->styleContentType);
    writer->writeInt16(173, ent->styleLeftAttach);
    writer->writeInt16(95, ent->styleRightAttach);
    writer->writeInt16(174, ent->styleTextAngleType);
    writer->writeInt16(175, ent->unknown175);
    writer->writeInt32(92, ent->styleTextColor);
    writer->writeBool(292, ent->styleTextFrameEnabled);
    writer->writeInt32(93, ent->styleBlockColor);
    writer->writeDouble(43, ent->styleBlockRotation);
    writer->writeInt16(176, ent->styleAttachmentType);
    writer->writeBool(293, ent->isAnnotative);
    writer->writeBool(294, ent->isTextDirectionNegative);
    writer->writeInt16(178, ent->ipeAlign);
    writer->writeInt16(179, ent->justification);
    writer->writeDouble(45, ent->scaleFactor);
    writer->writeInt16(271, ent->attachmentDirection);
    writer->writeInt16(273, ent->styleTopAttach);
    writer->writeInt16(272, ent->styleBottomAttach);
    writer->writeBool(295, ent->leaderExtendedToText);
        if (!ent->extData.empty() && !writeExtData(ent->extData))
            return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeWipeout(DRW_Wipeout *ent) {
    // WIPEOUT inherits AcDbRasterImage's group codes plus an AcDbWipeout
    // subclass marker carrying the polygon (91 + 14/24) and frame flag (290).
    // No AcDbRasterImageDef is written: WIPEOUT carries no actual raster.
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (ent == nullptr || writer == nullptr || !ent->validatePayloadFields()
        || !ent->hasValidBoundary()) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "WIPEOUT");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbRasterImage");
    writer->writeDouble(10, ent->basePoint.x);
    writer->writeDouble(20, ent->basePoint.y);
    writer->writeDouble(30, ent->basePoint.z);
    writer->writeDouble(11, ent->secPoint.x);
    writer->writeDouble(21, ent->secPoint.y);
    writer->writeDouble(31, ent->secPoint.z);
    writer->writeDouble(12, ent->vVector.x);
    writer->writeDouble(22, ent->vVector.y);
    writer->writeDouble(32, ent->vVector.z);
    writer->writeDouble(13, ent->sizeu);
    writer->writeDouble(23, ent->sizev);
    writer->writeInt16(70, ent->m_displayProps);
    writer->writeString(340, toHexStr(ent->ref));
    writer->writeString(360, toHexStr(ent->m_imageDefReactorHandle));
    writer->writeInt16(280, ent->clip);    // 1 = clipping enabled
    writer->writeInt16(281, ent->brightness);
    writer->writeInt16(282, ent->contrast);
    writer->writeInt16(283, ent->fade);
    writer->writeString(100, "AcDbWipeout");
    writer->writeInt32(90, 0);             // class version
    writer->writeInt16(71, ent->m_clipBoundaryType);
    writer->writeInt32(91, static_cast<std::int32_t>(ent->clipPath.size()));
    for (const DRW_Coord& v : ent->clipPath) {
        writer->writeDouble(14, v.x);
        writer->writeDouble(24, v.y);
    }
    // Group 290 is the R2010+ Clip mode (0 = mask outside, 1 = mask inside);
    // this is shared with IMAGE and is NOT a frame-display flag.  WIPEOUTFRAME
    // (whether the polygon outline is drawn) is global, in WIPEOUTVARIABLES.
    if (version > DRW::AC1021)
        writer->writeBool(290, ent->clipMode);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writePointCloud(DRW_PointCloud *ent){
    if (!preflightEntity(ent))
        return false;
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    if (ent == nullptr || version <= DRW::AC1018
        || ent->classVersion > std::numeric_limits<std::uint16_t>::max()
        || ent->sourceFileCount < 0
        || ent->sourceFileCount > static_cast<int>(DRW_PointCloud::kMaxItems)
        || ent->sourceFiles.size() != static_cast<std::size_t>(ent->sourceFileCount)
        || ent->pointCount > static_cast<std::uint64_t>(
               std::numeric_limits<std::int32_t>::max())
        || ent->showIntensity || ent->showClipping
        || ent->clippingCount != 0 || !ent->clippings.empty()) {
        m_writeError = true;
        return false;
    }
    if (!finite(ent->origin) || !finite(ent->extentsMin)
        || !finite(ent->extentsMax) || !finite(ent->ucsOrigin)
        || !finite(ent->ucsXDirection) || !finite(ent->ucsYDirection)
        || !finite(ent->ucsZDirection)
        || !std::isfinite(ent->intensityStyle.minIntensity)
        || !std::isfinite(ent->intensityStyle.maxIntensity)
        || !std::isfinite(ent->intensityStyle.lowThreshold)
        || !std::isfinite(ent->intensityStyle.highThreshold)
        || ent->intensityScheme < 0
        || ent->intensityScheme > std::numeric_limits<std::uint16_t>::max()) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "POINTCLOUD");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbEntity");
    writer->writeString(100, "AcDbPointCloud");
    writer->writeInt16(70, ent->classVersion);
    writer->writeDouble(10, ent->origin.x);
    writer->writeDouble(20, ent->origin.y);
    writer->writeDouble(30, ent->origin.z);
    writer->writeUtf8String(1, ent->savedFilename);
    writer->writeInt32(90, ent->sourceFileCount);
    for (const UTF8STRING& srcFile : ent->sourceFiles) {
        writer->writeUtf8String(2, srcFile);
    }
    // DXF carries the geometry/reference fields for both source-file forms.
    // The source-count conditional applies to the DWG bit stream only.
    writer->writeDouble(11, ent->extentsMin.x);
    writer->writeDouble(21, ent->extentsMin.y);
    writer->writeDouble(31, ent->extentsMin.z);
    writer->writeDouble(12, ent->extentsMax.x);
    writer->writeDouble(22, ent->extentsMax.y);
    writer->writeDouble(32, ent->extentsMax.z);
    writer->writeInt32(92, static_cast<int>(ent->pointCount));
    writer->writeUtf8String(3, ent->ucsName);
    writer->writeDouble(13, ent->ucsOrigin.x);
    writer->writeDouble(23, ent->ucsOrigin.y);
    writer->writeDouble(33, ent->ucsOrigin.z);
    writer->writeDouble(210, ent->ucsXDirection.x);
    writer->writeDouble(220, ent->ucsXDirection.y);
    writer->writeDouble(230, ent->ucsXDirection.z);
    writer->writeDouble(211, ent->ucsYDirection.x);
    writer->writeDouble(221, ent->ucsYDirection.y);
    writer->writeDouble(231, ent->ucsYDirection.z);
    writer->writeDouble(212, ent->ucsZDirection.x);
    writer->writeDouble(222, ent->ucsZDirection.y);
    writer->writeDouble(232, ent->ucsZDirection.z);
    if (version > DRW::AC1024) {
        writer->writeString(330, toHexStr(ent->definitionHandle));
        writer->writeString(360, toHexStr(ent->reactorHandle));
        writer->writeInt16(71, ent->intensityScheme);
        writer->writeDouble(40, ent->intensityStyle.minIntensity);
        writer->writeDouble(41, ent->intensityStyle.maxIntensity);
        writer->writeDouble(42, ent->intensityStyle.lowThreshold);
        writer->writeDouble(43, ent->intensityStyle.highThreshold);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writePointCloudEx(DRW_PointCloudEx *ent){
    if (!preflightEntity(ent))
        return false;
    const auto finite = [](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z);
    };
    const auto fitsUint16 = [](int value) {
        return value >= 0
            && value <= std::numeric_limits<std::uint16_t>::max();
    };
    const auto integral32 = [](double value) {
        return std::isfinite(value)
            && value >= static_cast<double>(std::numeric_limits<std::int32_t>::min())
            && value <= static_cast<double>(std::numeric_limits<std::int32_t>::max())
            && std::floor(value) == value;
    };
    if (ent == nullptr || version <= DRW::AC1024
        || ent->classVersion > std::numeric_limits<std::uint16_t>::max()
        || ent->croppingCount < 0
        || ent->croppingCount > static_cast<int>(DRW_PointCloudEx::kMaxItems)
        || ent->croppings.size() != static_cast<std::size_t>(ent->croppingCount)) {
        m_writeError = true;
        return false;
    }
    if (!finite(ent->extentsMin) || !finite(ent->extentsMax)
        || !finite(ent->ucsOrigin) || !finite(ent->ucsXDirection)
        || !finite(ent->ucsYDirection) || !finite(ent->ucsZDirection)
        || !std::isfinite(ent->elevationMin)
        || !std::isfinite(ent->elevationMax)
        || !integral32(ent->intensityMin) || !integral32(ent->intensityMax)
        || !fitsUint16(ent->stylizationType)
        || !fitsUint16(ent->intensityOutOfRangeBehavior)
        || !fitsUint16(ent->elevationOutOfRangeBehavior)) {
        m_writeError = true;
        return false;
    }
    for (const DRW_PointCloudExCropping& cropping : ent->croppings) {
        if (cropping.type < 0 || cropping.type > 0xffff
            || cropping.pointCount < 0
            || cropping.pointCount > static_cast<int>(DRW_PointCloudEx::kMaxItems)
            || cropping.points.size() != static_cast<std::size_t>(cropping.pointCount)
            || !finite(cropping.cropPlane)
            || !finite(cropping.cropXDirection)
            || !finite(cropping.cropYDirection)) {
            m_writeError = true;
            return false;
        }
        for (const DRW_Coord& point : cropping.points) {
            if (!finite(point)) {
                m_writeError = true;
                return false;
            }
        }
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "POINTCLOUDEX");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbEntity");
    writer->writeString(100, "AcDbPointCloud");
    writer->writeInt16(70, ent->classVersion);
    writer->writeDouble(10, ent->extentsMin.x);
    writer->writeDouble(20, ent->extentsMin.y);
    writer->writeDouble(30, ent->extentsMin.z);
    writer->writeDouble(11, ent->extentsMax.x);
    writer->writeDouble(21, ent->extentsMax.y);
    writer->writeDouble(31, ent->extentsMax.z);
    writer->writeDouble(12, ent->ucsOrigin.x);
    writer->writeDouble(22, ent->ucsOrigin.y);
    writer->writeDouble(32, ent->ucsOrigin.z);
    writer->writeDouble(210, ent->ucsXDirection.x);
    writer->writeDouble(220, ent->ucsXDirection.y);
    writer->writeDouble(230, ent->ucsXDirection.z);
    writer->writeDouble(211, ent->ucsYDirection.x);
    writer->writeDouble(221, ent->ucsYDirection.y);
    writer->writeDouble(231, ent->ucsYDirection.z);
    writer->writeDouble(212, ent->ucsZDirection.x);
    writer->writeDouble(222, ent->ucsZDirection.y);
    writer->writeDouble(232, ent->ucsZDirection.z);
    writer->writeBool(290, ent->isLocked);
    writer->writeString(330, toHexStr(ent->definitionHandle));
    writer->writeString(360, toHexStr(ent->reactorHandle));
    writer->writeUtf8String(1, ent->name);
    writer->writeBool(291, ent->showIntensity);
    writer->writeInt16(71, ent->stylizationType);
    writer->writeUtf8String(1, ent->intensityColorScheme);
    writer->writeUtf8String(1, ent->currentColorScheme);
    writer->writeUtf8String(1, ent->classificationColorScheme);
    writer->writeDouble(40, ent->elevationMin);
    writer->writeDouble(41, ent->elevationMax);
    writer->writeInt32(90, static_cast<std::int32_t>(ent->intensityMin));
    writer->writeInt32(91, static_cast<std::int32_t>(ent->intensityMax));
    writer->writeInt16(71, ent->intensityOutOfRangeBehavior);
    writer->writeInt16(72, ent->elevationOutOfRangeBehavior);
    writer->writeBool(292, ent->elevationApplyToFixedRange);
    writer->writeBool(293, ent->intensityAsGradient);
    writer->writeBool(294, ent->elevationAsGradient);
    writer->writeBool(295, ent->showCropping);
    writer->writeInt32(92, ent->croppingCount);
    if (ent->croppingCount == 0) {
        writer->writeInt32(93, ent->unknownInt0);
    writer->writeInt32(93, ent->unknownInt1);
    }
    for (const DRW_PointCloudExCropping& cropping : ent->croppings) {
        writer->writeInt16(280, cropping.type);
        writer->writeBool(290, cropping.isInside);
        writer->writeBool(290, cropping.isInverted);
        writer->writeDouble(13, cropping.cropPlane.x);
        writer->writeDouble(23, cropping.cropPlane.y);
        writer->writeDouble(33, cropping.cropPlane.z);
        writer->writeDouble(213, cropping.cropXDirection.x);
        writer->writeDouble(223, cropping.cropXDirection.y);
        writer->writeDouble(233, cropping.cropXDirection.z);
        writer->writeDouble(213, cropping.cropYDirection.x);
        writer->writeDouble(223, cropping.cropYDirection.y);
        writer->writeDouble(233, cropping.cropYDirection.z);
        writer->writeInt32(93, cropping.pointCount);
        for (const DRW_Coord& point : cropping.points) {
            writer->writeDouble(13, point.x);
            writer->writeDouble(23, point.y);
            writer->writeDouble(33, point.z);
        }
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writePointCloudDef(DRW_PointCloudDef *ent) {
    if (!preflightTableEntry(ent))
        return false;
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();

    const char *recordName = "POINTCLOUDDEFINITION";
    const char *subclassName = "AcDbPointCloudDef";
    switch (ent->m_kind) {
    case DRW_PointCloudDef::Definition:
        break;
    case DRW_PointCloudDef::DefinitionEx:
        recordName = "POINTCLOUDDEFINITIONEX";
        subclassName = "AcDbPointCloudDefEx";
        break;
    case DRW_PointCloudDef::Reactor:
        recordName = "POINTCLOUDDEFREACTOR";
        subclassName = "AcDbPointCloudDefReactor";
        break;
    case DRW_PointCloudDef::ReactorEx:
        recordName = "POINTCLOUDDEFREACTOREX";
        subclassName = "AcDbPointCloudDefReactorEx";
        break;
    default:
        m_writeError = true;
        return false;
    }
    if (ent->m_classVersion < 0
        || ent->m_classVersion > DRW_PointCloudDef::kMaxClassVersion
        || ent->m_pointCount > DRW_PointCloudDef::kMaxPointCount) {
        m_writeError = true;
        return false;
    }

    EntityRecordScope scope(*this, ent);
    writer->writeString(0, recordName);
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, subclassName);
    writer->writeInt32(90, ent->m_classVersion);
    if (ent->m_kind == DRW_PointCloudDef::Definition
        || ent->m_kind == DRW_PointCloudDef::DefinitionEx) {
        writer->writeUtf8String(1, ent->m_sourceFilename);
        writer->writeBool(280, ent->m_isLoaded);
        writer->writeInt64(160, static_cast<std::int64_t>(ent->m_pointCount));
        writer->writeDouble(10, ent->m_extentsMin.x);
        writer->writeDouble(20, ent->m_extentsMin.y);
        writer->writeDouble(30, ent->m_extentsMin.z);
        writer->writeDouble(11, ent->m_extentsMax.x);
        writer->writeDouble(21, ent->m_extentsMax.y);
        writer->writeDouble(31, ent->m_extentsMax.z);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeNavisworksModelDef(DRW_NavisworksModelDef *ent) {
    if (!preflightTableEntry(ent))
        return false;
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (ent->m_flags < 0 || ent->m_flags > DRW_NavisworksModelDef::kMaxFlags
        || ent->m_path.size() > DRW_NavisworksModelDef::kMaxPathLength) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "NAVISWORKSMODELDEF");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbNavisworksModelDef");
    writer->writeInt16(70, static_cast<std::int16_t>(ent->m_flags));
    writer->writeUtf8String(1, ent->m_path);
    writer->writeBool(290, ent->m_status);
    writer->writeDouble(10, ent->m_minExtent.x);
    writer->writeDouble(20, ent->m_minExtent.y);
    writer->writeDouble(30, ent->m_minExtent.z);
    writer->writeDouble(11, ent->m_maxExtent.x);
    writer->writeDouble(21, ent->m_maxExtent.y);
    writer->writeDouble(31, ent->m_maxExtent.z);
    writer->writeBool(291, ent->m_hostDrawingVisibility);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writePointCloudColorMap(DRW_PointCloudColorMap *ent) {
    if (!preflightTableEntry(ent))
        return false;
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (ent->m_classVersion < 0
        || ent->m_classVersion > DRW_PointCloudColorMap::kMaxClassVersion
        || ent->m_colorRamps.size() > DRW_PointCloudColorMap::kMaxRamps
        || ent->m_classificationColorRamps.size() > DRW_PointCloudColorMap::kMaxRamps
        || ent->m_colorRampCount != ent->m_colorRamps.size()
        || ent->m_classificationColorRampCount
               != ent->m_classificationColorRamps.size())
    {
        m_writeError = true;
        return false;
    }
    const auto validScheme = [](const UTF8STRING& scheme) {
        return scheme.size() <= DRW_PointCloudColorMap::kMaxColorSchemeLength;
    };
    if (!validScheme(ent->m_defaultIntensityColorScheme)
        || !validScheme(ent->m_defaultElevationColorScheme)
        || !validScheme(ent->m_defaultClassificationColorScheme)) {
        m_writeError = true;
        return false;
    }

    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "POINTCLOUDCOLORMAP");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbPointCloudColorMap");
    writer->writeInt32(90, ent->m_classVersion);
    writer->writeUtf8String(1, ent->m_defaultIntensityColorScheme);
    writer->writeUtf8String(1, ent->m_defaultElevationColorScheme);
    writer->writeUtf8String(1, ent->m_defaultClassificationColorScheme);
    const auto writeRamps = [&](const std::vector<DRW_PointCloudColorMapRamp>& ramps) {
        for (const auto& ramp : ramps) {
            if (ramp.m_classVersion < 0
                || ramp.m_classVersion > DRW_PointCloudColorMap::kMaxClassVersion
                || ramp.m_rampCount < ramp.m_colorSchemes.size()
                || ramp.m_rampCount > DRW_PointCloudColorMap::kMaxColorsPerRamp)
            {
                m_writeError = true;
                return false;
            }
            writer->writeInt32(91, static_cast<std::int32_t>(ramp.m_rampCount));
            writer->writeInt32(90, ramp.m_classVersion);
            for (const UTF8STRING& scheme : ramp.m_colorSchemes) {
                if (!validScheme(scheme)) {
                    m_writeError = true;
                    return false;
                }
                writer->writeUtf8String(1, scheme);
            }
        }
        return true;
    };
    if (!writeRamps(ent->m_colorRamps))
        return false;
    if (!ent->m_classificationColorRamps.empty()) {
        writer->writeInt16(70, 1);
        if (!writeRamps(ent->m_classificationColorRamps))
            return false;
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeNavisworksModel(DRW_NavisworksModel *ent) {
    if (!preflightEntity(ent))
        return false;
    if (version < DRW::AC1015)
        return rejectUnsupportedDxfWrite();
    if (!std::isfinite(ent->unitFactor)
        || !std::all_of(ent->transform.begin(), ent->transform.end(),
                        [](double value) { return std::isfinite(value); })) {
        m_writeError = true;
        return false;
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, "NAVISWORKSMODEL");
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbNavisworksModel");
    writer->writeInt32(70, static_cast<std::int32_t>(ent->flags));
    if (ent->definitionHandle != 0)
        writer->writeString(340, toHexStr(ent->definitionHandle));
    for (double value : ent->transform)
        writer->writeDouble(40, value);
    writer->writeDouble(40, ent->unitFactor);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeSurface(DRW_Surface *ent){
    if (ent == nullptr) {
        m_writeError = true;
        return false;
    }
    if (version <= DRW::AC1018)
        return rejectUnsupportedDxfWrite();
    if (!preflightEntity(ent))
        return false;
    const auto fail = [this]() {
        m_writeError = true;
        return false;
    };
    std::string entType;
    const char *subclassType = nullptr;
    switch (ent->eType) {
    case DRW::PLANESURFACE: entType = "PLANESURFACE"; break;
    case DRW::EXTRUDEDSURFACE:
        entType = "EXTRUDEDSURFACE";
        subclassType = "AcDbExtrudedSurface";
        break;
    case DRW::REVOLVEDSURFACE:
        entType = "REVOLVEDSURFACE";
        subclassType = "AcDbRevolvedSurface";
        break;
    case DRW::SWEPTSURFACE:
        entType = "SWEPTSURFACE";
        subclassType = "AcDbSweptSurface";
        break;
    case DRW::LOFTEDSURFACE:
        entType = "LOFTEDSURFACE";
        subclassType = "AcDbLoftedSurface";
        break;
    case DRW::NURBSURFACE:
        entType = "NURBSSURFACE";
        subclassType = "AcDbNurbSurface";
        break;
    default: return fail();
    }
    EntityRecordScope scope(*this, ent);
    writer->writeString(0, entType);
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbEntity");
    writer->writeString(100, "AcDbModelerGeometry");
    if (!ent->rawAcisData.empty()) {
        if (isTextAcisPayload(ent->rawAcisData))
            writeDxfTextChunks(writer.get(), ent->rawAcisData);
        else
            writeDxfBinaryChunks(writer.get(), ent->rawAcisData);
    }
    writer->writeInt16(70, ent->modelerFormatVersion);
    writer->writeString(100, "AcDbSurface");
    writer->writeInt16(71, ent->uIsolines);
    writer->writeInt16(72, ent->vIsolines);
    if (subclassType != nullptr)
        writer->writeString(100, subclassType);
    if (ent->eType == DRW::REVOLVEDSURFACE) {
        const auto *revolved = dynamic_cast<const DRW_RevolvedSurface *>(ent);
        if (revolved == nullptr
            || revolved->classId >
                   static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())
            || !std::isfinite(revolved->axisPoint.x)
            || !std::isfinite(revolved->axisPoint.y)
            || !std::isfinite(revolved->axisPoint.z)
            || !std::isfinite(revolved->axisVector.x)
            || !std::isfinite(revolved->axisVector.y)
            || !std::isfinite(revolved->axisVector.z)
            || !std::isfinite(revolved->revolveAngle)
            || !std::isfinite(revolved->startAngle)
            || !std::isfinite(revolved->draftAngle)
            || !std::isfinite(revolved->draftStartDistance)
            || !std::isfinite(revolved->draftEndDistance)
            || !std::isfinite(revolved->twistAngle)
            || !std::all_of(revolved->transform.begin(), revolved->transform.end(),
                            [](double value) { return std::isfinite(value); })) {
            return fail();
        }
        writer->writeInt32(90, static_cast<std::int32_t>(revolved->classId));
        writer->writeDouble(10, revolved->axisPoint.x);
        writer->writeDouble(20, revolved->axisPoint.y);
        writer->writeDouble(30, revolved->axisPoint.z);
        writer->writeDouble(11, revolved->axisVector.x);
        writer->writeDouble(21, revolved->axisVector.y);
        writer->writeDouble(31, revolved->axisVector.z);
        writer->writeDouble(40, revolved->revolveAngle);
        writer->writeDouble(41, revolved->startAngle);
        for (double value : revolved->transform)
            writer->writeDouble(42, value);
        writer->writeDouble(43, revolved->draftAngle);
        writer->writeDouble(44, revolved->draftStartDistance);
        writer->writeDouble(45, revolved->draftEndDistance);
        writer->writeDouble(46, revolved->twistAngle);
        writer->writeInt16(290, revolved->solid ? 1 : 0);
        writer->writeInt16(291, revolved->closeToAxis ? 1 : 0);
    } else if (ent->eType == DRW::EXTRUDEDSURFACE) {
        const auto *extruded = dynamic_cast<const DRW_ExtrudedSurface *>(ent);
        const auto finiteCoord = [](const DRW_Coord& value) {
            return std::isfinite(value.x) && std::isfinite(value.y)
                && std::isfinite(value.z);
        };
        const auto validFlags = [](std::int32_t value) {
            return value >= 0 && value <=
                static_cast<std::int32_t>(std::numeric_limits<std::uint16_t>::max());
        };
        if (extruded == nullptr
            || extruded->classId >
                   static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())
            || !finiteCoord(extruded->sweepVector)
            || !finiteCoord(extruded->referenceVector)
            || !validFlags(extruded->sweepAlignmentFlags)
            || !validFlags(extruded->pathFlags)
            || !std::all_of(extruded->extrudedTransform.begin(),
                            extruded->extrudedTransform.end(),
                            [](double value) { return std::isfinite(value); })
            || !std::all_of(extruded->sweepEntityTransform.begin(),
                            extruded->sweepEntityTransform.end(),
                            [](double value) { return std::isfinite(value); })
            || !std::all_of(extruded->pathEntityTransform.begin(),
                            extruded->pathEntityTransform.end(),
                            [](double value) { return std::isfinite(value); })
            || !std::isfinite(extruded->draftAngle)
            || !std::isfinite(extruded->draftStartDistance)
            || !std::isfinite(extruded->draftEndDistance)
            || !std::isfinite(extruded->twistAngle)
            || !std::isfinite(extruded->scaleFactor)
            || !std::isfinite(extruded->alignAngle)) {
            return fail();
        }
        writer->writeInt32(90, static_cast<std::int32_t>(extruded->classId));
        writer->writeDouble(10, extruded->sweepVector.x);
        writer->writeDouble(20, extruded->sweepVector.y);
        writer->writeDouble(30, extruded->sweepVector.z);
        for (double value : extruded->extrudedTransform)
            writer->writeDouble(40, value);
        writer->writeDouble(42, extruded->draftAngle);
        writer->writeDouble(43, extruded->draftStartDistance);
        writer->writeDouble(44, extruded->draftEndDistance);
        writer->writeDouble(45, extruded->twistAngle);
        writer->writeDouble(48, extruded->scaleFactor);
        writer->writeDouble(49, extruded->alignAngle);
        for (double value : extruded->sweepEntityTransform)
            writer->writeDouble(46, value);
        for (double value : extruded->pathEntityTransform)
            writer->writeDouble(47, value);
        writer->writeInt16(290, extruded->solid ? 1 : 0);
        writer->writeInt16(70, extruded->sweepAlignmentFlags);
        writer->writeInt16(71, extruded->pathFlags);
        writer->writeInt16(292, extruded->alignStart ? 1 : 0);
        writer->writeInt16(293, extruded->bank ? 1 : 0);
        writer->writeInt16(294, extruded->basePointSet ? 1 : 0);
        writer->writeInt16(295, extruded->sweepEntityTransformComputed ? 1 : 0);
        writer->writeInt16(296, extruded->pathEntityTransformComputed ? 1 : 0);
        writer->writeDouble(11, extruded->referenceVector.x);
        writer->writeDouble(21, extruded->referenceVector.y);
        writer->writeDouble(31, extruded->referenceVector.z);
    } else if (ent->eType == DRW::SWEPTSURFACE) {
        const auto *swept = dynamic_cast<const DRW_SweptSurface *>(ent);
        const auto finiteCoord = [](const DRW_Coord& value) {
            return std::isfinite(value.x) && std::isfinite(value.y)
                && std::isfinite(value.z);
        };
        const auto validFlags = [](std::int32_t value) {
            return value >= 0 && value <=
                static_cast<std::int32_t>(std::numeric_limits<std::uint16_t>::max());
        };
        const auto finiteMatrix = [](const auto& matrix) {
            return std::all_of(matrix.begin(), matrix.end(),
                               [](double value) { return std::isfinite(value); });
        };
        if (swept == nullptr
            || swept->sweepEntityId >
                   static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())
            || swept->pathEntityId >
                   static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())
            || swept->sweepData.size() > DRW_SweptSurface::kMaxSweepDataSize
            || swept->pathData.size() > DRW_SweptSurface::kMaxSweepDataSize
            || !finiteCoord(swept->referenceVector)
            || !validFlags(swept->sweepAlignmentFlags)
            || !validFlags(swept->pathFlags)
            || !finiteMatrix(swept->sweepEntityTransformed)
            || !finiteMatrix(swept->pathEntityTransformed)
            || !finiteMatrix(swept->sweepEntityTransform)
            || !finiteMatrix(swept->pathEntityTransform)
            || !std::isfinite(swept->draftAngle)
            || !std::isfinite(swept->draftStartDistance)
            || !std::isfinite(swept->draftEndDistance)
            || !std::isfinite(swept->twistAngle)
            || !std::isfinite(swept->scaleFactor)
            || !std::isfinite(swept->alignAngle)) {
            return fail();
        }
        writer->writeInt32(90, static_cast<std::int32_t>(swept->sweepEntityId));
        if (!swept->sweepData.empty())
            writeDxfBinaryChunks(writer.get(), swept->sweepData);
        writer->writeInt32(91, static_cast<std::int32_t>(swept->pathEntityId));
        if (!swept->pathData.empty())
            writeDxfBinaryChunks(writer.get(), swept->pathData);
        for (double value : swept->sweepEntityTransformed)
            writer->writeDouble(40, value);
        for (double value : swept->pathEntityTransformed)
            writer->writeDouble(41, value);
        writer->writeDouble(42, swept->draftAngle);
        writer->writeDouble(43, swept->draftStartDistance);
        writer->writeDouble(44, swept->draftEndDistance);
        writer->writeDouble(45, swept->twistAngle);
        writer->writeDouble(48, swept->scaleFactor);
        writer->writeDouble(49, swept->alignAngle);
        for (double value : swept->sweepEntityTransform)
            writer->writeDouble(46, value);
        for (double value : swept->pathEntityTransform)
            writer->writeDouble(47, value);
        writer->writeInt16(290, swept->solid ? 1 : 0);
        writer->writeInt16(70, swept->sweepAlignmentFlags);
        writer->writeInt16(71, swept->pathFlags);
        writer->writeInt16(292, swept->alignStart ? 1 : 0);
        writer->writeInt16(293, swept->bank ? 1 : 0);
        writer->writeInt16(294, swept->basePointSet ? 1 : 0);
        writer->writeInt16(295, swept->sweepEntityTransformComputed ? 1 : 0);
        writer->writeInt16(296, swept->pathEntityTransformComputed ? 1 : 0);
        writer->writeDouble(11, swept->referenceVector.x);
        writer->writeDouble(21, swept->referenceVector.y);
        writer->writeDouble(31, swept->referenceVector.z);
    } else if (ent->eType == DRW::LOFTEDSURFACE) {
        const auto *lofted = dynamic_cast<const DRW_LoftedSurface *>(ent);
        const auto finiteMatrix = [](const auto& matrix) {
            return std::all_of(matrix.begin(), matrix.end(),
                               [](double value) { return std::isfinite(value); });
        };
        if (lofted == nullptr
            || lofted->numCrossSections < 0
            || lofted->numGuideCurves < 0
            || lofted->numCrossSections >
                   static_cast<std::int32_t>(DRW_LoftedSurface::kMaxReferenceTokenCount)
            || lofted->numGuideCurves >
                   static_cast<std::int32_t>(DRW_LoftedSurface::kMaxReferenceTokenCount)
            || lofted->dxfReferenceData.size() >
                   DRW_LoftedSurface::kMaxReferenceTokenCount
            || !finiteMatrix(lofted->loftEntityTransform)
            || !std::isfinite(lofted->startDraftAngle)
            || !std::isfinite(lofted->endDraftAngle)
            || !std::isfinite(lofted->startDraftMagnitude)
            || !std::isfinite(lofted->endDraftMagnitude)) {
            return fail();
        }
        std::size_t referenceDataSize = 0;
        for (const DRW_Variant& value : lofted->dxfReferenceData) {
            if (value.code() == 90 && value.type() == DRW_Variant::INTEGER) {
                if (value.i_val() < 0
                    || value.i_val() > static_cast<std::int32_t>(
                           DRW_LoftedSurface::kMaxReferenceTokenCount)) {
                    return fail();
                }
            } else if (value.code() == 310
                       && value.type() == DRW_Variant::BINARY) {
                const auto *bytes = value.binary();
                if (bytes == nullptr
                    || bytes->size() > DRW_LoftedSurface::kMaxReferenceDataSize
                    || referenceDataSize >
                           DRW_LoftedSurface::kMaxReferenceDataSize - bytes->size()) {
                    return fail();
                }
                referenceDataSize += bytes->size();
            } else {
                return fail();
            }
        }
        for (double value : lofted->loftEntityTransform)
            writer->writeDouble(40, value);
        for (const DRW_Variant& value : lofted->dxfReferenceData) {
            if (value.code() == 90)
                writer->writeInt32(90, value.i_val());
            else
                writeDxfBinaryChunks(writer.get(), *value.binary());
        }
        writer->writeInt32(70, lofted->planeNormalLoftingType);
        writer->writeDouble(41, lofted->startDraftAngle);
        writer->writeDouble(42, lofted->endDraftAngle);
        writer->writeDouble(43, lofted->startDraftMagnitude);
        writer->writeDouble(44, lofted->endDraftMagnitude);
        writer->writeInt16(290, lofted->arcLengthParameterization ? 1 : 0);
        writer->writeInt16(291, lofted->noTwist ? 1 : 0);
        writer->writeInt16(292, lofted->alignDirection ? 1 : 0);
        writer->writeInt16(293, lofted->simpleSurfaces ? 1 : 0);
        writer->writeInt16(294, lofted->closedSurfaces ? 1 : 0);
        writer->writeInt16(295, lofted->solid ? 1 : 0);
        writer->writeInt16(296, lofted->ruledSurface ? 1 : 0);
        writer->writeInt16(297, lofted->virtualGuide ? 1 : 0);
        if (lofted->pathCurveHandle != 0)
            writer->writeString(5, toHexStr(lofted->pathCurveHandle));
    } else if (ent->eType == DRW::NURBSURFACE) {
        const auto *nurbs = dynamic_cast<const DRW_NurbsSurface *>(ent);
        const auto finiteCoord = [](const DRW_Coord& value) {
            return std::isfinite(value.x) && std::isfinite(value.y)
                && std::isfinite(value.z);
        };
        if (nurbs == nullptr
            || !finiteCoord(nurbs->uvec1) || !finiteCoord(nurbs->vvec1)
            || !finiteCoord(nurbs->uvec2) || !finiteCoord(nurbs->vvec2)) {
            return fail();
        }
        if (version >= DRW::AC1027) {
            writer->writeInt16(170, nurbs->short170);
            writer->writeInt16(290, nurbs->cvHullDisplay ? 1 : 0);
            writer->writeDouble(10, nurbs->uvec1.x);
            writer->writeDouble(20, nurbs->uvec1.y);
            writer->writeDouble(30, nurbs->uvec1.z);
            writer->writeDouble(11, nurbs->vvec1.x);
            writer->writeDouble(21, nurbs->vvec1.y);
            writer->writeDouble(31, nurbs->vvec1.z);
            writer->writeDouble(12, nurbs->uvec2.x);
            writer->writeDouble(22, nurbs->uvec2.y);
            writer->writeDouble(32, nurbs->uvec2.z);
            writer->writeDouble(13, nurbs->vvec2.x);
            writer->writeDouble(23, nurbs->vvec2.y);
            writer->writeDouble(33, nurbs->vvec2.z);
        }
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return fail();
    return !writer->hasWriteError();
}

bool dxfRW::writeModelerGeometry(DRW_ModelerGeometry *ent) {
    if (version <= DRW::AC1009)
        return rejectUnsupportedDxfWrite();
    if (!preflightEntity(ent))
        return false;

    const char *recordName = modelerGeometryDxfName(ent->eType);
    const char *subclassName = modelerGeometryDxfSubclass(ent->eType);
    if (recordName == nullptr || subclassName == nullptr)
        return rejectUnsupportedDxfWrite();

    EntityRecordScope scope(*this, ent);
    writer->writeString(0, recordName);
    if (!writeEntity(ent))
        return false;
    writer->writeString(100, "AcDbModelerGeometry");
    writer->writeString(100, subclassName);
    writer->writeInt16(70, ent->m_modelerVersion);
    if (ent->m_historyHandle != 0)
        writer->writeString(350, toHexStr(ent->m_historyHandle));
    if (!ent->m_rawBytes.empty()) {
        if (version <= DRW::AC1018 && isTextAcisPayload(ent->m_rawBytes))
            writeDxfTextChunks(writer.get(), ent->m_rawBytes);
        else
            writeDxfBinaryChunks(writer.get(), ent->m_rawBytes);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeBlockRecord(std::string name, int insUnits) {
    return writeBlockRecord(std::move(name), insUnits, {});
}

bool dxfRW::writeBlockRecord(
    std::string name, int insUnits,
    const std::vector<std::uint8_t>& previewData) {
    return writeBlockRecord(std::move(name), insUnits, previewData, {});
}

bool dxfRW::writeBlockRecord(
    std::string name, int insUnits,
    const std::vector<std::uint8_t>& previewData,
    const std::vector<std::uint32_t>& insertHandles) {
    if (version > DRW::AC1009
        && previewData.size() > DRW::kMaxDxfBinaryPayloadBytes) {
        m_writeError = true;
        return false;
    }
    if (version <= DRW::AC1014 && !insertHandles.empty()) {
        m_writeError = true;
        return false;
    }
    if (insertHandles.size() > DRW::kMaxDxfBlockRecordInsertHandles) {
        m_writeError = true;
        return false;
    }
    std::set<std::uint32_t> uniqueInsertHandles;
    for (const std::uint32_t insertHandle : insertHandles) {
        if (insertHandle == DRW::NoHandle
            || !uniqueInsertHandles.insert(insertHandle).second) {
            m_writeError = true;
            return false;
        }
    }
    if (version > DRW::AC1009) {
        const std::string key = dxfSymbolNameKey(name);
        if (writer == nullptr || key.empty() || blockMap.count(key) != 0) {
            m_writeError = true;
            return false;
        }
        PendingDxfBlockRecord record;
        try {
            if (!allocateDxfHandle(record.handle))
                return false;
            std::uint32_t blockHandle = 0;
            std::uint32_t endBlockHandle = 0;
            if (!checkedDxfHandleOffset(record.handle, 1, blockHandle)
                || !checkedDxfHandleOffset(record.handle, 2,
                                            endBlockHandle)) {
                throw std::overflow_error("DXF block handle range exhausted");
            }
            m_handleAllocator.reserve(blockHandle);  // BLOCK
            m_handleAllocator.reserve(endBlockHandle);  // ENDBLK
        } catch (...) {
            m_writeError = true;
            return false;
        }
        record.name = std::move(name);
        record.insUnits = insUnits;
        record.previewData = previewData;
        record.insertHandles = insertHandles;

        if (m_collectingBlockRecords) {
            const std::size_t mutationCheckpoint = m_dxfWriteMutations.size();
            try {
                if (m_recordStateScopeDepth != 0) {
                    m_dxfWriteMutations.push_back({
                        DxfWriteMutationKind::BlockMapInsert, key, 0, 0,
                        false});
                }
                const auto inserted = blockMap.emplace(key, record.handle);
                if (!inserted.second) {
                    m_dxfWriteMutations.resize(mutationCheckpoint);
                    m_writeError = true;
                    return false;
                }
                m_pendingBlockRecords.push_back(std::move(record));
            } catch (...) {
                blockMap.erase(key);
                if (m_pendingBlockRecords.size() > 0
                    && m_pendingBlockRecords.back().handle == record.handle)
                    m_pendingBlockRecords.pop_back();
                m_dxfWriteMutations.resize(mutationCheckpoint);
                m_writeError = true;
                return false;
            }
            return true;
        }
        RecordStateScope state(*this);
        DxfWriterRecordScope bytes(*writer);
        const std::size_t mutationCheckpoint = m_dxfWriteMutations.size();
        try {
            m_dxfWriteMutations.push_back({
                DxfWriteMutationKind::BlockMapInsert, key, 0, 0, false});
            const auto inserted = blockMap.emplace(key, record.handle);
            if (!inserted.second) {
                m_dxfWriteMutations.resize(mutationCheckpoint);
                m_writeError = true;
                return false;
            }
        } catch (...) {
            m_writeError = true;
            return false;
        }
        if (!emitBlockRecord(record) || writer->hasWriteError())
            return false;
        if (!bytes.commit()) {
            m_writeError = true;
            return false;
        }
        state.commit();
        return true;
    }
    return true;
}

bool dxfRW::emitBlockRecord(const PendingDxfBlockRecord& record) {
    if (writer == nullptr || version <= DRW::AC1009 || record.handle == 0
        || record.name.empty()) {
        m_writeError = true;
        return false;
    }
    writer->writeString(0, "BLOCK_RECORD");
    writer->writeString(5, toHexStr(record.handle));
    if (version > DRW::AC1014)
        writer->writeString(330, "1");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbBlockTableRecord");
    writer->writeUtf8String(2, record.name);
    if (version > DRW::AC1018) {
        writer->writeInt16(70, record.insUnits);
        writer->writeInt16(280, 1);
        writer->writeInt16(281, 0);
    }
    if (!record.previewData.empty())
        writeDxfBinaryChunks(writer.get(), record.previewData);
    if (version > DRW::AC1014 && !record.insertHandles.empty()) {
        writer->writeString(102, "{BLKREFS");
        for (const std::uint32_t insertHandle : record.insertHandles)
            writer->writeString(331, toHexStr(insertHandle));
        writer->writeString(102, "}");
    }
    return !writer->hasWriteError();
}

bool dxfRW::writeBlock(DRW_Block *bk){
    if (!preflightEntity(bk) || m_collectingBlockRecords) {
        m_writeError = true;
        return false;
    }

    const bool wasWritingBlock = writingBlock;
    const std::uint32_t previousHandle = currHandle;

    std::uint32_t blockRecordHandle = 0;
    std::uint32_t blockHandle = 0;
    std::uint32_t endBlockHandle = 0;
    std::uint32_t previousEndBlockHandle = 0;
    if (version > DRW::AC1009) {
        const auto blockRecord = blockMap.find(dxfSymbolNameKey(bk->name));
        if (blockRecord == blockMap.end()) {
            m_writeError = true;
            return false;
        }
        blockRecordHandle = static_cast<std::uint32_t>(blockRecord->second);
        if (!checkedDxfHandleOffset(blockRecordHandle, 1, blockHandle)
            || !checkedDxfHandleOffset(blockRecordHandle, 2,
                                       endBlockHandle)) {
            m_writeError = true;
            return false;
        }
        DRW_UNUSED(endBlockHandle);
    }
    if (writingBlock && version > DRW::AC1009
        && !checkedDxfHandleOffset(previousHandle, 2,
                                   previousEndBlockHandle)) {
        writingBlock = wasWritingBlock;
        currHandle = previousHandle;
        m_writeError = true;
        return false;
    }

    if (writingBlock) {
        writer->writeString(0, "ENDBLK");
        if (version > DRW::AC1009) {
            writer->writeString(5, toHexStr(previousEndBlockHandle));
            if (version > DRW::AC1014) {
                writer->writeString(330, toHexStr(currHandle));
            }
            writer->writeString(100, "AcDbEntity");
        }
        writer->writeString(8, "0");
        if (version > DRW::AC1009) {
            writer->writeString(100, "AcDbBlockEnd");
        }
    }
    writingBlock = true;
    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        currHandle = blockRecordHandle;
        writer->writeString(5, toHexStr(blockHandle));
        if (version > DRW::AC1014) {
            writer->writeString(330, toHexStr(currHandle));
    }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockBegin");
        writer->writeUtf8String(2, bk->name);
    } else
        writer->writeUtf8Caps(2, bk->name);
    writer->writeInt16(70, bk->flags);
    writer->writeDouble(10, bk->basePoint.x);
    writer->writeDouble(20, bk->basePoint.y);
    if (bk->basePoint.z != 0.0) {
        writer->writeDouble(30, bk->basePoint.z);
    }
    if (version > DRW::AC1009)
        writer->writeUtf8String(3, bk->name);
    else
        writer->writeUtf8Caps(3, bk->name);
    if (version >= DRW::AC1014 && !writeAppData(bk->appData)) {
        writingBlock = wasWritingBlock;
        currHandle = previousHandle;
        return false;
    }
    writer->writeString(1, "");

    if (writer->hasWriteError()) {
        writingBlock = wasWritingBlock;
        currHandle = previousHandle;
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeTables() {
    if (writer == nullptr) {
        m_writeError = true;
        return false;
    }
    RecordStateScope state(*this);
    DxfWriterRecordScope record(*writer);
    const auto hasWriteFailure = [this]() {
        return m_writeError || writer == nullptr || writer->hasWriteError();
    };
    const auto writeEndTable = [&]() {
        writer->writeString(0, "ENDTAB");
        return !hasWriteFailure();
    };

    writer->writeString(0, "TABLE");
    writer->writeString(2, "VPORT");
    if (version > DRW::AC1009) {
        writer->writeString(5, "8");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
/*** VPORT ***/
    if (hasWriteFailure())
        return false;
    dimstyleStd =false;
    iface->writeVports();
    if (hasWriteFailure())
        return false;
    if (!dimstyleStd) {
        DRW_Vport portact;
        portact.name = "*ACTIVE";
        if (!writeVport(&portact))
            return false;
    }
    if (hasWriteFailure())
        return false;
    if (!writeEndTable())
        return false;
/*** LTYPE ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "5");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 4); //end table def
//Mandatory linetypes
    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "14");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "ByBlock");
    } else
        writer->writeString(2, "BYBLOCK");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "15");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
}
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "ByLayer");
    } else
        writer->writeString(2, "BYLAYER");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);

    writer->writeString(0, "LTYPE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "16");
        if (version > DRW::AC1014) {
            writer->writeString(330, "5");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbLinetypeTableRecord");
        writer->writeString(2, "Continuous");
    } else {
        writer->writeString(2, "CONTINUOUS");
    }
    writer->writeInt16(70, 0);
    writer->writeString(3, "Solid line");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);
//Application linetypes
    if (hasWriteFailure())
        return false;
    iface->writeLTypes();
    if (hasWriteFailure())
        return false;
    if (!writeEndTable())
        return false;
/*** LAYER ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "LAYER");
    if (version > DRW::AC1009) {
        writer->writeString(5, "2");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 1); //end table def
    if (hasWriteFailure())
        return false;
    wlayer0 = false;
    iface->writeLayers();
    if (hasWriteFailure())
        return false;
    if (!wlayer0) {
        DRW_Layer lay0;
        lay0.name = "0";
        if (!writeLayer(&lay0))
            return false;
    }
    if (hasWriteFailure())
        return false;
    if (!writeEndTable())
        return false;
/*** STYLE ***/
    writer->writeString(0, "TABLE");
    writer->writeString(2, "STYLE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "3");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
}
        writer->writeString(100, "AcDbSymbolTable");
}
    writer->writeInt16(70, 3); //end table def
    if (hasWriteFailure())
        return false;
    dimstyleStd =false;
    iface->writeTextstyles();
    if (hasWriteFailure())
        return false;
    if (!dimstyleStd) {
        DRW_Textstyle tsty;
        tsty.name = "Standard";
        if (!writeTextstyle(&tsty))
            return false;
    }
    if (hasWriteFailure())
        return false;
    if (!writeEndTable())
        return false;

    writer->writeString(0, "TABLE");
    writer->writeString(2, "VIEW");
    if (version > DRW::AC1009) {
        writer->writeString(5, "6");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
}
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0);
    if (hasWriteFailure())
        return false;
    iface->writeViews();
    if (hasWriteFailure())
        return false;
    if (!writeEndTable())
        return false;

    writer->writeString(0, "TABLE");
    writer->writeString(2, "UCS");
    if (version > DRW::AC1009) {
        writer->writeString(5, "7");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
    }
    writer->writeInt16(70, 0);
    if (hasWriteFailure())
        return false;
    iface->writeUCSs();
    if (hasWriteFailure())
        return false;
    if (!writeEndTable())
        return false;

    writer->writeString(0, "TABLE");
    writer->writeString(2, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "9");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
        }
    writer->writeInt16(70, 1); //end table def
    writer->writeString(0, "APPID");
    if (version > DRW::AC1009) {
        writer->writeString(5, "12");
        if (version > DRW::AC1014) {
            writer->writeString(330, "9");
    }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbRegAppTableRecord");
    }
    writer->writeString(2, "ACAD");
    writer->writeInt16(70, 0);
    if (hasWriteFailure())
        return false;
    iface->writeAppId();
    if (hasWriteFailure())
        return false;
    if (!writeEndTable())
        return false;

    writer->writeString(0, "TABLE");
    writer->writeString(2, "DIMSTYLE");
    if (version > DRW::AC1009) {
        writer->writeString(5, "A");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
        }
    writer->writeInt16(70, 1); //end table def
    if (version > DRW::AC1014) {
        writer->writeString(100, "AcDbDimStyleTable");
        writer->writeInt16(71, 1); //end table def
        }
    if (hasWriteFailure())
        return false;
    dimstyleStd =false;
    iface->writeDimstyles();
    if (hasWriteFailure())
        return false;
    if (!dimstyleStd) {
        DRW_Dimstyle dsty;
        dsty.name = "Standard";
        if (!writeDimstyle(&dsty))
            return false;
        }
    if (hasWriteFailure())
        return false;
    if (!writeEndTable())
        return false;

    if (version > DRW::AC1009) {
        writer->writeString(0, "TABLE");
        writer->writeString(2, "BLOCK_RECORD");
        writer->writeString(5, "1");
        if (version > DRW::AC1014) {
            writer->writeString(330, "0");
        }
        writer->writeString(100, "AcDbSymbolTable");
        writer->writeInt16(
            70, static_cast<int>(2 + m_pendingBlockRecords.size()));
        writer->writeString(0, "BLOCK_RECORD");
        writer->writeString(5, "1F");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeString(2, "*Model_Space");
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, 0);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
        writer->writeString(0, "BLOCK_RECORD");
        writer->writeString(5, "1E");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1");
        }
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbBlockTableRecord");
        writer->writeString(2, "*Paper_Space");
        if (version > DRW::AC1018) {
            //    writer->writeInt16(340, 22);
            writer->writeInt16(70, 0);
            writer->writeInt16(280, 1);
            writer->writeInt16(281, 0);
        }
    }
    if (hasWriteFailure())
        return false;
    for (const PendingDxfBlockRecord& record : m_pendingBlockRecords) {
        if (!emitBlockRecord(record))
            return false;
    }
    if (version > DRW::AC1009) {
        writer->writeString(0, "ENDTAB");
    } else {
        // R12 has no BLOCK_RECORD table or record handles. Preserve the
        // historical callback order without unnecessary pre-collection.
        iface->writeBlockRecords();
        if (hasWriteFailure())
            return false;
    }
    if (hasWriteFailure() || !record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeBlocks() {
    RecordStateScope state(*this);
    DxfWriterRecordScope record(*writer);
    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "20");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockBegin");
        writer->writeString(2, "*Model_Space");
    } else
        writer->writeString(2, "$MODEL_SPACE");
    writer->writeInt16(70, 0);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, 0.0);
    if (version > DRW::AC1009)
        writer->writeString(3, "*Model_Space");
    else
        writer->writeString(3, "$MODEL_SPACE");
    writer->writeString(1, "");
    writer->writeString(0, "ENDBLK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "21");
        if (version > DRW::AC1014) {
            writer->writeString(330, "1F");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbBlockEnd");

    writer->writeString(0, "BLOCK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "1C");
        if (version > DRW::AC1014) {
            // Paper_Space BLOCK (handle 1C) is owned by the Paper_Space
            // BLOCK_RECORD (1E, reserved above), not the nonexistent handle 1B.
            writer->writeString(330, "1E");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009) {
        writer->writeString(100, "AcDbBlockBegin");
        writer->writeString(2, "*Paper_Space");
    } else
        writer->writeString(2, "$PAPER_SPACE");
    writer->writeInt16(70, 0);
    writer->writeDouble(10, 0.0);
    writer->writeDouble(20, 0.0);
    writer->writeDouble(30, 0.0);
    if (version > DRW::AC1009)
        writer->writeString(3, "*Paper_Space");
    else
        writer->writeString(3, "$PAPER_SPACE");
    writer->writeString(1, "");
    writer->writeString(0, "ENDBLK");
    if (version > DRW::AC1009) {
        writer->writeString(5, "1D");
        if (version > DRW::AC1014) {
            // Paper_Space ENDBLK (handle 1D) is owned by the Paper_Space
            // BLOCK_RECORD (1E), not the Model_Space BLOCK_RECORD (1F).
            writer->writeString(330, "1E");
        }
        writer->writeString(100, "AcDbEntity");
    }
    writer->writeString(8, "0");
    if (version > DRW::AC1009)
        writer->writeString(100, "AcDbBlockEnd");

    if (m_writeError || writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    writingBlock = false;
    iface->writeBlocks();
    if (m_writeError || writer->hasWriteError())
        return false;
    if (writingBlock) {
        std::uint32_t autoEndBlockHandle = 0;
        if (version > DRW::AC1009
            && !checkedDxfHandleOffset(currHandle, 2,
                                       autoEndBlockHandle)) {
            m_writeError = true;
            return false;
        }
        writingBlock = false;
        writer->writeString(0, "ENDBLK");
        if (version > DRW::AC1009) {
            writer->writeString(5, toHexStr(autoEndBlockHandle));
//            writer->writeString(5, "1D");
            if (version > DRW::AC1014) {
                writer->writeString(330, toHexStr(currHandle));
            }
            writer->writeString(100, "AcDbEntity");
        }
        writer->writeString(8, "0");
        if (version > DRW::AC1009)
            writer->writeString(100, "AcDbBlockEnd");
    }
    if (m_writeError || writer->hasWriteError() || !record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();
    return true;
}

bool dxfRW::writeObjects() {
    RecordStateScope state(*this);
    DxfWriterRecordScope record(*writer);
    std::string imgDictH;
    std::vector<std::uint32_t> groupHandles;
    try {
        if (!imageDef.empty()) {
            std::uint32_t imageDictionaryHandle = 0;
            if (!allocateDxfHandle(imageDictionaryHandle))
                return false;
            imgDictH = toHexStr(imageDictionaryHandle);
        }
        groupHandles.reserve(m_groups.size());
        for (std::size_t i = 0; i < m_groups.size(); ++i) {
            std::uint32_t groupHandle = 0;
            if (!allocateDxfHandle(groupHandle))
                return false;
            groupHandles.push_back(groupHandle);
        }
    } catch (...) {
        m_writeError = true;
        return false;
    }
    writer->writeString(0, "DICTIONARY");
    writer->writeString(5, "C");
    if (version > DRW::AC1014) {
        writer->writeString(330, "0");
    }
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
    writer->writeString(3, "ACAD_GROUP");
    writer->writeString(350, "D");
    if (imageDef.size() != 0) {
        writer->writeString(3, "ACAD_IMAGE_DICT");
        writer->writeString(350, imgDictH);
    }
    //Slice (spine-dicts): re-attach raw-net-routed named dictionaries to the
    //regenerated root NamedObjectsDictionary so they are reachable (not pruned as
    //orphans). The filter populates (name, hex-handle) from the source root dict;
    //each handle matches the verbatim code-5 of a dictionary re-emitted later in
    //this OBJECTS section. ACAD_GROUP / root / C-D collisions are excluded there.
    for (const std::pair<std::string, std::string> &entry : m_rootDictEntries) {
        writer->writeString(3, entry.first);
        writer->writeString(350, entry.second);
    }
    //F3: mint a fresh code-5 handle for each GROUP BEFORE the ACAD_GROUP D dict
    //so the D dict body can list each group as a (name, handle) entry (the GROUP
    //objects themselves are emitted after the named-dict block below, owned by
    //D). Allocator-minted handles skip the reserved/fixed set, so they cannot
    //collide. A group with no resolvable members is still listed/emitted (a valid
    //empty group is harmless); members are filtered at emit time.
    writer->writeString(0, "DICTIONARY");
    writer->writeString(5, "D");
    writer->writeString(330, "C");
    writer->writeString(100, "AcDbDictionary");
    writer->writeInt16(281, 1);
    for (std::size_t i = 0; i < m_groups.size(); ++i) {
        //Unnamed groups carry a generated "*An" name; named groups carry their
        //real name. The D-dict entry name mirrors the GROUP's name field.
        writer->writeUtf8String(3, m_groups[i].name);
        writer->writeString(350, toHexStr(groupHandles[i]));
    }
//write IMAGEDEF_REACTOR
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        for (auto it=id->reactors.begin() ; it != id->reactors.end(); ++it ) {
            writer->writeString(0, "IMAGEDEF_REACTOR");
            writer->writeString(5, (*it).first);
            writer->writeString(330, (*it).second);
            writer->writeString(100, "AcDbRasterImageDefReactor");
            writer->writeInt16(90, 2); //version 2=R14 to v2010
            writer->writeString(330, (*it).second);
        }
    }
    if (imageDef.size() != 0) {
        writer->writeString(0, "DICTIONARY");
        writer->writeString(5, imgDictH);
        writer->writeString(330, "C");
        writer->writeString(100, "AcDbDictionary");
        writer->writeInt16(281, 1);
        for (unsigned int i=0; i<imageDef.size(); i++) {
            size_t f1, f2;
            f1 = imageDef.at(i)->name.find_last_of("/\\");
            f2 =imageDef.at(i)->name.find_last_of('.');
            ++f1;
            writer->writeString(3, imageDef.at(i)->name.substr(f1,f2-f1));
            writer->writeString(350, toHexStr(imageDef.at(i)->handle) );
        }
    }
    for (unsigned int i=0; i<imageDef.size(); i++) {
        DRW_ImageDef *id = imageDef.at(i);
        writer->writeString(0, "IMAGEDEF");
        writer->writeString(5, toHexStr(id->handle) );
        // Owner 330 = the AcDbRasterImageDef dictionary (imgDictH). A missing
        // owner triggers an INVALID_OWNER_HANDLE audit/repair in ezdxf/AutoCAD.
        // imgDictH is non-empty whenever imageDef is non-empty (set above).
        if (version > DRW::AC1014 && !imgDictH.empty()) {
            writer->writeString(330, imgDictH);
        }
        writer->writeString(102, "{ACAD_REACTORS");
        for (auto it=id->reactors.begin() ; it != id->reactors.end(); ++it ) {
            writer->writeString(330, (*it).first);
        }
        writer->writeString(102, "}");
        writer->writeString(100, "AcDbRasterImageDef");
        writer->writeInt16(90, id->imgVersion);
        writer->writeUtf8String(1, id->name);
        writer->writeDouble(10, id->u);
        writer->writeDouble(20, id->v);
        writer->writeDouble(11, id->up);
        writer->writeDouble(21, id->vp);
        writer->writeInt16(280, id->loaded);
        writer->writeInt16(281, id->resolution);
    }
    //F4-followup: emit the named-dictionary OBJECTS the filter routed via
    //setNamedDictObjects (DWG->DXF). The root C dict already references these by
    //handle (setRootDictEntries spliced their (name, handle) into C above); this
    //makes them exist as reachable objects with a valid owner, clearing the
    //INVALID_OWNER_HANDLE fixes ezdxf otherwise applies to the dangling 350s.
    for (const DRW_Dictionary &dict : m_namedDictObjects) {
        writer->writeString(0, "DICTIONARY");
        writer->writeString(5, toHexStr(dict.handle));
        writeObjectOwner(dict.parentHandle != 0
                             ? static_cast<std::uint32_t>(dict.parentHandle)
                             : 0);
        writer->writeString(100, "AcDbDictionary");
        // Preserve the full duplicate-record cloning policy (valid values include
        // 0,1,2,3,4,5,11,12,13 — e.g. 12 = keep, sort). parseCode reads the full
        // int (code 281); collapsing nonzero->1 silently rewrote the policy.
        writer->writeInt16(281, dict.cloning);
        for (const DRW_Dictionary::Entry &entry : dict.m_entries) {
            writer->writeUtf8String(3, entry.m_name);
            writer->writeString(350, toHexStr(entry.m_handle));
        }
    }

    //F3: emit each GROUP object, owned by the ACAD_GROUP D dict (which already
    //lists it as a (name, minted-handle) entry above). Member 340 references are
    //resolved through the writeEntity source->minted map; a member whose SOURCE
    //handle was not written (consumed / filtered entity) is SKIPPED — never an
    //emitted dangling 340.
    const auto &srcToMinted = m_writingContext.sourceHandleToMintedMap;
    for (std::size_t i = 0; i < m_groups.size(); ++i) {
        const DRW_Group &grp = m_groups[i];
        writer->writeString(0, "GROUP");
        writer->writeString(5, toHexStr(groupHandles[i]));
        writer->writeString(330, "D");
        writer->writeString(100, "AcDbGroup");
        writer->writeUtf8String(300, grp.m_description);
        writer->writeInt16(70, grp.m_isUnnamed ? 1 : 0);
        writer->writeInt16(71, grp.m_selectable ? 1 : 0);
        for (std::uint32_t memberSrc : grp.m_entityHandles) {
            auto it = srcToMinted.find(memberSrc);
            if (it == srcToMinted.end())
                continue;  // member not written -> skip (no dangling 340)
            writer->writeString(340, toHexStr(it->second));
        }
    }

    if (m_writeError || writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    iface->writeObjects();
    if (m_writeError || writer->hasWriteError())
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    state.commit();

    // IMAGEDEF pointers are owned by this write session. Keep them alive until
    // every OBJECTS byte has committed; resetDxfWriteSession discards carriers
    // from a failed attempt before the next callback traversal.
    for (DRW_ImageDef *id : imageDef)
        delete id;
    imageDef.clear();
    return true;
}

bool dxfRW::writeExtData(
    const std::vector<std::shared_ptr<DRW_Variant>> &ed) {
    // Re-pack as raw pointers so we share the existing implementation. The
    // raw pointers do not own — same lifetime as the shared_ptrs in @p ed.
    std::vector<DRW_Variant*> raw;
    try {
        raw.reserve(ed.size());
        for (const auto &sp : ed) {
            if (!sp) {
                m_writeError = true;
                return false;
            }
            raw.push_back(sp.get());
        }
    } catch (...) {
        m_writeError = true;
        return false;
    }
    return writeExtData(raw);
}

bool dxfRW::writeExtData(const std::vector<DRW_Variant*> &ed){
    if (writer == nullptr) {
        m_writeError = true;
        return false;
    }

    // Validate the complete EED vector before emitting its first group.  EED
    // is appended to an already-open record, so validating as we write can
    // leave a syntactically incomplete record when a later variant is bad.
    const auto isValid = [](const DRW_Variant* value) {
        if (value == nullptr)
            return false;

        switch (value->code()) {
        case 1000:
        case 1001:
        case 1002:
        case 1003:
        case 1005:
            return value->type() == DRW_Variant::STRING
                && value->content.s != nullptr;
        case 1004:
            if (value->type() == DRW_Variant::BINARY) {
                return value->content.b != nullptr
                    && value->content.b->size()
                        <= DRW::kMaxDxfBinaryPayloadBytes;
            }
            return value->type() == DRW_Variant::STRING
                && value->content.s != nullptr
                && isDxfHexText(*value->content.s)
                && value->content.s->size() / 2u
                    <= DRW::kMaxDxfBinaryPayloadBytes;
        case 1010:
        case 1011:
        case 1012:
        case 1013:
            return value->type() == DRW_Variant::COORD
                && value->content.v != nullptr
                && std::isfinite(value->content.v->x)
                && std::isfinite(value->content.v->y)
                && std::isfinite(value->content.v->z);
        case 1040:
        case 1041:
        case 1042:
            return value->type() == DRW_Variant::DOUBLE
                && std::isfinite(value->content.d);
        case 1070:
            return value->type() == DRW_Variant::INTEGER;
        case 1071:
            if (value->type() == DRW_Variant::INTEGER)
                return true;
            return value->type() == DRW_Variant::INTEGER64
                && value->content.i64
                    >= std::numeric_limits<std::int32_t>::min()
                && value->content.i64
                    <= std::numeric_limits<std::int32_t>::max();
        default:
            return false;
        }
    };
    for (const DRW_Variant* value : ed) {
        if (!isValid(value)) {
            m_writeError = true;
            return false;
        }
    }

    bool success = true;
    const auto recordResult = [&success, this](bool result) {
        if (!result) {
            success = false;
            m_writeError = true;
        }
    };
    const auto writeBinaryHex = [this](const std::string& hex) {
        if (!isDxfHexText(hex)
            || hex.size() / 2u > DRW::kMaxDxfBinaryPayloadBytes)
            return false;
        if (hex.empty())
            return writer->writeUtf8String(1004, std::string{});
        constexpr std::size_t kHexChunkLength = 2u * 127u;
        for (std::size_t offset = 0; offset < hex.size();
             offset += kHexChunkLength) {
            if (!writer->writeUtf8String(
                    1004, hex.substr(offset, kHexChunkLength)))
                return false;
        }
        return true;
    };

    try {
        for (std::vector<DRW_Variant*>::const_iterator it=ed.begin();
             it!=ed.end(); ++it){
            if (*it == nullptr) {
                recordResult(false);
                continue;
            }
            switch ((*it)->code()) {
                case 1000:
                case 1001:
                case 1002:
                case 1003:
                case 1005: {
                    const int cc = (*it)->code();
                    if ((*it)->type() != DRW_Variant::STRING
                        || (*it)->content.s == nullptr) {
                        recordResult(false);
                        break;
                    }
                    recordResult(writer->writeUtf8String(
                        cc, *(*it)->content.s));
                    break;
                }
                case 1004:
                    // DXF code 1004 is binary chunk data; emitted as a
                    // hex-encoded string. Both BINARY (from DWG path) and
                    // STRING (from a DXF round-trip that already hex-encoded
                    // the bytes) variants are accepted.
                    if ((*it)->type() == DRW_Variant::BINARY) {
                        const std::vector<std::uint8_t>* bytes =
                            (*it)->content.b;
                        if (bytes == nullptr
                            || bytes->size() > DRW::kMaxDxfBinaryPayloadBytes) {
                            recordResult(false);
                            break;
                        }
                        std::string hex;
                        static const char hexDigits[] = "0123456789ABCDEF";
                        hex.reserve(bytes->size() * 2);
                        for (std::uint8_t b : *bytes) {
                            hex.push_back(hexDigits[(b >> 4) & 0xF]);
                            hex.push_back(hexDigits[b & 0xF]);
                        }
                        recordResult(writeBinaryHex(hex));
                    } else if ((*it)->type() == DRW_Variant::STRING
                               && (*it)->content.s != nullptr
                               && isDxfHexText(*(*it)->content.s)) {
                        recordResult(writeBinaryHex(*(*it)->content.s));
                    } else {
                        recordResult(false);
                    }
                    break;
                case 1010:
                case 1011:
                case 1012:
                case 1013:
                    if ((*it)->type() != DRW_Variant::COORD
                        || (*it)->content.v == nullptr
                        || !std::isfinite((*it)->content.v->x)
                        || !std::isfinite((*it)->content.v->y)
                        || !std::isfinite((*it)->content.v->z)) {
                        recordResult(false);
                        break;
                    }
                    recordResult(writer->writeDouble(
                        (*it)->code(), (*it)->content.v->x));
                    recordResult(writer->writeDouble(
                        (*it)->code()+10, (*it)->content.v->y));
                    recordResult(writer->writeDouble(
                        (*it)->code()+20, (*it)->content.v->z));
                    break;
                case 1040:
                case 1041:
                case 1042:
                    recordResult((*it)->type() == DRW_Variant::DOUBLE
                        && std::isfinite((*it)->content.d)
                        && writer->writeDouble((*it)->code(),
                                               (*it)->content.d));
                    break;
                case 1070:
                    recordResult((*it)->type() == DRW_Variant::INTEGER
                        && writer->writeInt16((*it)->code(),
                                              (*it)->content.i));
                    break;
                case 1071:
                    if ((*it)->type() == DRW_Variant::INTEGER) {
                        recordResult(writer->writeInt32(
                            (*it)->code(), (*it)->content.i));
                    } else if ((*it)->type() == DRW_Variant::INTEGER64) {
                        const std::int64_t value = (*it)->content.i64;
                        recordResult(
                            value >= std::numeric_limits<std::int32_t>::min()
                            && value <= std::numeric_limits<std::int32_t>::max()
                            && writer->writeInt32(
                                (*it)->code(), static_cast<std::int32_t>(value)));
                    } else {
                        recordResult(false);
                    }
                    break;
                default:
                    break;
            }
        }
    } catch (...) {
        m_writeError = true;
        return false;
    }
    return success && !writer->hasWriteError();
}

/********* Reader Process *********/

bool dxfRW::processDxf() {
    DRW_DBG("dxfRW::processDxf() start processing dxf\n");
    int code {-1};
    bool inSection {false};

    reader->setIgnoreComments( false);
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG(" code\n");
        /* at this level we should only get:
         999 - Comment
         0 - SECTION or EOF
         2 - section name
         everything else between "2 - section name" and "0 - ENDSEC" is handled in process() methods
        */
        switch (code) {
        case 999: // when DXF was created by libdxfrw, first record is a comment with dxfrw version info
            header.addComment( reader->getString());
                continue;

            case 0:
                // DXF comment policy: retain 999 records before the first SECTION
                // in DRW_Header; comments inside sections are deliberately skipped
                // by dxfReader and are not part of typed or raw-record replay.
                reader->setIgnoreComments(true);
                if (!inSection) {
                const std::string& sectionstr = reader->getString();

                    if (dxfKeywordEquals(sectionstr, "SECTION")) {
                        DRW_DBG(sectionstr);
                        DRW_DBG(" new section\n");
                        inSection = true;
                        continue;
                    }
                    if (dxfKeywordEquals(sectionstr, "EOF")) {
                        return true; //found EOF terminate
                    }
                }
                else {
                    // in case SECTION was unknown or not supported
                if (dxfKeywordEquals(reader->getString(), "ENDSEC")) {
                        inSection = false;
                    }
                }
                break;

        case 2:
                if (inSection) {
                    bool processed{false};
                const std::string sectionname = reader->getString();

                    DRW_DBG(sectionname);
                    DRW_DBG(" process section\n");
                    if (dxfKeywordEquals(sectionname, "HEADER")) {
                        processed = processHeader();
                    }
                else if (dxfKeywordEquals(sectionname, "CLASSES")) {
                    processed = processClasses();
                    }
                    else if (dxfKeywordEquals(sectionname, "TABLES")) {
                        processed = processTables();
                    }
                    else if (dxfKeywordEquals(sectionname, "BLOCKS")) {
                        processed = processBlocks();
                    }
                    else if (dxfKeywordEquals(sectionname, "ENTITIES")) {
                        processed = processEntities(false);
                        if (processed
                            && !dxfKeywordEquals(nextentity, "ENDSEC")) {
                            processed = setError(DRW::BAD_READ_ENTITIES);
                        }
                    }
                    else if (dxfKeywordEquals(sectionname, "OBJECTS")) {
                        processed = processObjects();
                    }
                    else {
                        DRW_DBG(" section unknown or not supported\n");
                        processed = processRawDxfSection(sectionname);
                        if (!processed)
                            return false;
                    }

                    if (!processed) {
                        DRW_DBG("  failed\n");
                        // Keep the specific section error (for example
                        // BAD_VERSION from an invalid $ACADVER) instead of
                        // replacing it with the generic section failure.
                        return error == DRW::BAD_NONE
                            ? setError(DRW::BAD_READ_SECTION)
                            : false;
                    }

                    inSection = false;
                }
                continue;

            default:
                // landing here means an unknown or not supported SECTION
                inSection = false;
                break;
        }
    }

    if (0 == code && "EOF" == reader->getString()) {
        // in case the final EOF has no newline we end up here!
        // this is caused by filestr->good() which is false for missing newline on EOF
        return true;
    }

    return setError(DRW::BAD_UNKNOWN);
}

bool dxfRW::processRawDxfSection(const std::string& sectionName) {
    if (sectionName.empty())
        return setError(DRW::BAD_READ_SECTION);

    DRW_RawDxfSection section;
    section.m_name = sectionName;
    section.m_hasRawValues = !binFile;
    DxfRawHandleLexemeScope handleLexemes(*reader);
    int applicationDepth = 0;
    int code = 0;
    std::size_t pairCount = 0;
    while (reader->readRec(&code)) {
        if (++pairCount > DRW::kMaxDxfApplicationGroupPairs)
            return setError(DRW::BAD_CODE_PARSED);
        if (code == 0 && dxfKeywordEquals(reader->getString(), "ENDSEC")) {
            if (applicationDepth != 0)
                return setError(DRW::BAD_CODE_PARSED);
            section.m_version = reader->getSourceVersion();
            iface->addRawDxfSection(section);
            return true;
        }

        DRW_RawDxfObject group;
        if (!captureRawGroup(group, code, /*validateHandles=*/true)
            || group.groups.empty()
            || !updateRawDxfApplicationDepth(group.groups.back(),
                                               applicationDepth))
            return setError(DRW::BAD_CODE_PARSED);
        try {
            section.m_groups.push_back(std::move(group.groups.back()));
            section.m_rawValues.push_back(std::move(group.rawValues.back()));
        } catch (...) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_SECTION);
}

/********* Header Section *********/

bool dxfRW::processHeader() {
    DRW_DBG("dxfRW::processHeader\n");
    int code;
    std::string sectionstr;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG(" processHeader\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG(" processHeader\n\n");
            if (dxfKeywordEquals(sectionstr, "ENDSEC")) {
                iface->addHeader(&header);
                return true;  //found ENDSEC terminate
            }

            DRW_DBG("unexpected 0 code in header!\n");
            return setError(DRW::BAD_READ_HEADER);
        }

        if (!header.parseCode(code, reader)) {
            return setError(reader->getSourceVersion() == DRW::UNKNOWNV
                                ? DRW::BAD_VERSION
                                : DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_HEADER);
}

/********* Classes Section *********/

bool dxfRW::processClasses() {
    DRW_DBG("dxfRW::processClasses\n");
    int code = 0;
    bool reading = false;
    DRW_Class cls;
    std::vector<DRW_Class> pendingClasses;
    bool hasRecName = false;
    bool hasClassName = false;
    bool hasAppName = false;
    bool hasProxyFlag = false;
    bool hasInstanceCount = false;
    bool hasWasaProxyFlag = false;
    bool hasEntityFlag = false;

    auto finishClass = [&]() -> bool {
        if (!reading)
            return true;
        // Every CLASS group defined by the DXF reference is mandatory. An
        // internal proxy class may have an empty record name, but it must still
        // carry code 1; otherwise default-initialized metadata would be
        // published as a plausible class. Group 91 was introduced with the
        // R2004 class-instance trailer and is absent from older records.
        if (!hasRecName || !hasClassName || !hasAppName || !hasProxyFlag
            || !hasWasaProxyFlag || !hasEntityFlag
            || (reader->getSourceVersion() > DRW::AC1015
                && !hasInstanceCount)) {
            DRW_DBG("malformed CLASS record: missing required group\n");
            return false;
        }
        if (pendingClasses.size() >= DRW::kMaxDxfClasses)
            return false;
        pendingClasses.push_back(cls);
        return true;
    };

    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG(" processClasses\n");
        if (code == 0) {
            if (!finishClass())
                return setError(DRW::BAD_CODE_PARSED);
            const std::string sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG(" processClasses\n\n");
            if (dxfKeywordEquals(sectionstr, "CLASS")) {
                reading = true;
                cls = DRW_Class{};
                hasRecName = false;
                hasClassName = false;
                hasAppName = false;
                hasProxyFlag = false;
                hasInstanceCount = false;
                hasWasaProxyFlag = false;
                hasEntityFlag = false;
                continue;
            }
            if (dxfKeywordEquals(sectionstr, "ENDSEC")) {
                for (const DRW_Class &pendingClass : pendingClasses)
                    iface->addDxfClass(pendingClass);
                return true;
            }

            DRW_DBG("unexpected 0 code in classes section!\n");
            return setError(DRW::BAD_READ_HEADER);
        }

        if (!reading)
            return setError(DRW::BAD_CODE_PARSED);

        switch (code) {
        case 1:
            if (hasRecName)
                return setError(DRW::BAD_CODE_PARSED);
            hasRecName = true;
            cls.recName = reader->getUtf8String();
            break;
        case 2:
            if (hasClassName)
                return setError(DRW::BAD_CODE_PARSED);
            hasClassName = true;
            cls.className = reader->getUtf8String();
            break;
        case 3:
            if (hasAppName)
                return setError(DRW::BAD_CODE_PARSED);
            hasAppName = true;
            cls.appName = reader->getUtf8String();
            break;
        case 90:
            if (hasProxyFlag)
                return setError(DRW::BAD_CODE_PARSED);
            hasProxyFlag = true;
            cls.proxyFlag = reader->getInt32();
            if (cls.proxyFlag < 0)
                return setError(DRW::BAD_CODE_PARSED);
            break;
        case 91:
            if (hasInstanceCount)
                return setError(DRW::BAD_CODE_PARSED);
            hasInstanceCount = true;
            cls.instanceCount = reader->getInt32();
            if (cls.instanceCount < 0)
                return setError(DRW::BAD_CODE_PARSED);
            break;
        case 280:
            if (hasWasaProxyFlag)
                return setError(DRW::BAD_CODE_PARSED);
            hasWasaProxyFlag = true;
            cls.wasaProxyFlag = reader->getInt32();
            if (cls.wasaProxyFlag < 0 || cls.wasaProxyFlag > 1)
                return setError(DRW::BAD_CODE_PARSED);
            break;
        case 281:
            if (hasEntityFlag)
                return setError(DRW::BAD_CODE_PARSED);
            cls.entityFlag = reader->getInt32();
            if (cls.entityFlag < 0 || cls.entityFlag > 1)
                return setError(DRW::BAD_CODE_PARSED);
            hasEntityFlag = true;
            break;
        default:
            break;
        }
    }

    return setError(DRW::BAD_READ_HEADER);
}

/********* Tables Section *********/

bool dxfRW::processTables() {
    DRW_DBG("dxfRW::processTables\n");
    int code;
    std::string sectionstr;
    bool more = true;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG(" processHeader\n\n");
            if (dxfKeywordEquals(sectionstr, "TABLE")) {
                more = reader->readRec(&code);
                DRW_DBG(code); DRW_DBG("\n");
                if (!more || code != 2) {
                    return setError(DRW::BAD_READ_TABLES); //wrong dxf file
                }
                sectionstr = reader->getString();
                DRW_DBG(sectionstr); DRW_DBG(" processHeader\n\n");
                //found section, process it
                bool processed = true;
                if (dxfKeywordEquals(sectionstr, "LTYPE")) {
                    processed = processLType();
                } else if (dxfKeywordEquals(sectionstr, "LAYER")) {
                    processed = processLayer();
                } else if (dxfKeywordEquals(sectionstr, "STYLE")) {
                    processed = processTextStyle();
                } else if (dxfKeywordEquals(sectionstr, "VPORT")) {
                    processed = processVports();
                } else if (dxfKeywordEquals(sectionstr, "VIEW")) {
                    processed = processView();
                } else if (dxfKeywordEquals(sectionstr, "UCS")) {
                    processed = processUCS();
                } else if (dxfKeywordEquals(sectionstr, "APPID")) {
                    processed = processAppId();
                } else if (dxfKeywordEquals(sectionstr, "DIMSTYLE")) {
                    processed = processDimStyle();
                } else if (dxfKeywordEquals(sectionstr, "BLOCK_RECORD")) {
                    processed = processBlockRecord();
                } else {
                    // Unknown tables are allowed, but their terminator still
                    // belongs to the section grammar.  Consume the table as
                    // an opaque stream and reject ENDSEC before ENDTAB so a
                    // malformed table cannot be mistaken for a valid one.
                    bool ended = false;
                    while (reader->readRec(&code)) {
                        if (code != 0)
                            continue;
                        const std::string tableRecord = reader->getString();
                        if (dxfKeywordEquals(tableRecord, "ENDTAB")) {
                            ended = true;
                            break;
                        }
                        if (dxfKeywordEquals(tableRecord, "ENDSEC"))
                            return setError(DRW::BAD_READ_TABLES);
                    }
                    if (!ended)
                        return setError(DRW::BAD_READ_TABLES);
                }
                if (!processed)
                    return false;
            } else if (dxfKeywordEquals(sectionstr, "ENDSEC")) {
                return true;  //found ENDSEC terminate
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processBlockRecord() {
    DRW_DBG("dxfRW::processBlockRecord\n");
    int code = 0;
    bool reading = false;
    bool readingBlkRefs = false;
    int ignoredApplicationDepth = 0;
    std::uint32_t handle = DRW::NoHandle;
    DRW_ParsingContext::BlockRecordInfo record;
    std::unordered_set<std::uint32_t> recordInsertHandles;
    std::unordered_map<std::uint32_t, DRW_ParsingContext::BlockRecordInfo>
        pendingBlockRecords;

    auto finishRecord = [&]() -> bool {
        if (!reading)
            return true;
        if (readingBlkRefs || ignoredApplicationDepth != 0)
            return false;
        const DRW::Version sourceVersion = reader->getSourceVersion();
        if (record.name.empty()
            || (sourceVersion != DRW::UNKNOWNV
                && sourceVersion > DRW::AC1009
                && handle == DRW::NoHandle))
            return false;
        if (handle != DRW::NoHandle) {
            const auto inserted = pendingBlockRecords.emplace(handle, record);
            if (!inserted.second)
                return false;
        }
        return true;
    };
    const auto publishBlockRecords = [&]() -> bool {
        for (const auto &entry : pendingBlockRecords) {
            if (m_readingContext.blockRecordMap.find(entry.first)
                    != m_readingContext.blockRecordMap.end())
                return false;
        }
        for (const auto &entry : pendingBlockRecords)
            m_readingContext.blockRecordMap.emplace(entry.first, entry.second);
        return true;
    };

    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!finishRecord())
                return setError(DRW::BAD_CODE_PARSED);
            const std::string sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (dxfKeywordEquals(sectionstr, "BLOCK_RECORD")) {
                reading = true;
                readingBlkRefs = false;
                ignoredApplicationDepth = 0;
                handle = DRW::NoHandle;
                record = DRW_ParsingContext::BlockRecordInfo{};
                recordInsertHandles.clear();
            } else if (dxfKeywordEquals(sectionstr, "ENDTAB")) {
                if (!publishBlockRecords())
                    return setError(DRW::BAD_CODE_PARSED);
                return true;
            } else {
                return setError(DRW::BAD_READ_TABLES);
            }
        } else if (reading) {
            switch (code) {
            case 102: {
                const std::string control = reader->getString();
                if (readingBlkRefs) {
                    if (control != "}")
                        return setError(DRW::BAD_CODE_PARSED);
                    readingBlkRefs = false;
                } else if (ignoredApplicationDepth != 0) {
                    if (control == "}") {
                        --ignoredApplicationDepth;
                    } else if (!control.empty() && control.front() == '{') {
                        if (ignoredApplicationDepth ==
                                DRW::kMaxDxfApplicationGroupNesting)
                            return setError(DRW::BAD_CODE_PARSED);
                        ++ignoredApplicationDepth;
                    } else {
                        return setError(DRW::BAD_CODE_PARSED);
                    }
                } else {
                    if (control == "{BLKREFS") {
                        if (reader->getSourceVersion() != DRW::UNKNOWNV
                            && reader->getSourceVersion() <= DRW::AC1014)
                            return setError(DRW::BAD_CODE_PARSED);
                        readingBlkRefs = true;
                    } else if (!control.empty() && control.front() == '{') {
                        ignoredApplicationDepth = 1;
                    } else if (control == "}") {
                        return setError(DRW::BAD_CODE_PARSED);
                    }
                }
                break;
            }
            case 331:
                if (ignoredApplicationDepth != 0)
                    break;
                if (!readingBlkRefs || !reader->isValidHandleString())
                    return setError(DRW::BAD_CODE_PARSED);
                if (recordInsertHandles.size()
                        >= DRW::kMaxDxfBlockRecordInsertHandles)
                    return setError(DRW::BAD_CODE_PARSED);
                {
                    const std::uint32_t insertHandle =
                        reader->getHandleString();
                    if (insertHandle == DRW::NoHandle
                        || !recordInsertHandles.insert(insertHandle).second)
                        return setError(DRW::BAD_CODE_PARSED);
                    record.insertHandles.push_back(insertHandle);
                }
                break;
            case 340:
                if (!reader->isValidHandleString())
                    return setError(DRW::BAD_CODE_PARSED);
                record.layoutHandle = reader->getHandleString();
                break;
            case 2:
                record.name = reader->getUtf8String();
                break;
            case 5:
                if (!reader->isValidHandleString())
                    return setError(DRW::BAD_CODE_PARSED);
                handle = reader->getHandleString();
                break;
            case 70:
                record.insUnits = reader->getInt32();
                break;
            case 280: {
                const std::int32_t canExplode = reader->getInt32();
                if (canExplode < 0 || canExplode > 1)
                    return setError(DRW::BAD_CODE_PARSED);
                record.canExplode = canExplode != 0;
                break;
            }
            case 281: {
                const std::int32_t blockScaling = reader->getInt32();
                if (blockScaling < 0 || blockScaling > 1)
                    return setError(DRW::BAD_CODE_PARSED);
                record.blockScaling = static_cast<std::uint8_t>(blockScaling);
                break;
            }
            case 310:
                if (!appendDxfHexChunk(reader->getString(), record.previewData))
                    return setError(DRW::BAD_CODE_PARSED);
                break;
            default:
                break;
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processLType() {
    DRW_DBG("dxfRW::processLType\n");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_LType ltype;
    std::vector<DRW_LType> pendingLTypes;
    std::map<std::uint32_t, std::string> pendingLineTypeNames;
    auto finishLType = [&]() -> bool {
        if (!reading)
            return true;
        if (!dxfTableEntryComplete(ltype, reader->getSourceVersion()))
            return false;
        ltype.update();
        if (!ltype.validateDxf())
            return false;
        if (ltype.handle != 0 && !ltype.name.empty()) {
            if (pendingLineTypeNames.find(ltype.handle)
                    != pendingLineTypeNames.end()
                || m_readingContext.lineTypeNameMap.find(ltype.handle)
                    != m_readingContext.lineTypeNameMap.end())
                return false;
            pendingLineTypeNames.emplace(ltype.handle, ltype.name);
        }
        pendingLTypes.push_back(ltype);
        return true;
    };
    const auto publishLTypes = [&]() -> bool {
        for (const auto &entry : pendingLineTypeNames) {
            if (m_readingContext.lineTypeNameMap.find(entry.first)
                    != m_readingContext.lineTypeNameMap.end())
                return false;
        }
        for (const DRW_LType &entry : pendingLTypes)
            iface->addLType(entry);
        for (const auto &entry : pendingLineTypeNames)
            m_readingContext.lineTypeNameMap.emplace(entry.first,
                                                     entry.second);
        return true;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!finishLType())
                return setError(DRW::BAD_CODE_PARSED);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (dxfKeywordEquals(sectionstr, "LTYPE")) {
                reading = true;
                ltype.reset();
            } else if (dxfKeywordEquals(sectionstr, "ENDTAB")) {
                if (!publishLTypes())
                    return setError(DRW::BAD_CODE_PARSED);
                return true;  //found ENDTAB terminate
            } else {
                return setError(DRW::BAD_READ_TABLES);
            }
        } else if (reading) {
            if (!ltype.parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processLayer() {
    DRW_DBG("dxfRW::processLayer\n");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Layer layer;
    std::vector<DRW_Layer> pendingLayers;
    std::set<std::uint32_t> seenHandles;
    const auto publishLayer = [&]() -> bool {
        if (!dxfTableEntryComplete(layer, reader->getSourceVersion())
            || !layer.validateDxf())
            return false;
        if (layer.handle != DRW::NoHandle
            && !seenHandles.insert(layer.handle).second)
            return false;
        pendingLayers.push_back(layer);
        return true;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading && !publishLayer())
                return setError(DRW::BAD_CODE_PARSED);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (dxfKeywordEquals(sectionstr, "LAYER")) {
                reading = true;
                layer.reset();
            } else if (dxfKeywordEquals(sectionstr, "ENDTAB")) {
                for (const DRW_Layer &entry : pendingLayers)
                    iface->addLayer(entry);
                return true;  //found ENDTAB terminate
            } else {
                return setError(DRW::BAD_READ_TABLES);
            }
        } else if (reading) {
            if (!layer.parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
}

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processDimStyle() {
    DRW_DBG("dxfRW::processDimStyle");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Dimstyle dimSty;
    std::vector<DRW_Dimstyle> pendingDimStyles;
    std::set<std::uint32_t> seenHandles;
    const auto publishDimStyle = [&]() -> bool {
        if (!dxfTableEntryComplete(dimSty, reader->getSourceVersion()))
            return false;
        if (dimSty.handle != DRW::NoHandle
            && !seenHandles.insert(dimSty.handle).second)
            return false;
        dimSty.syncStructToVars();
        pendingDimStyles.push_back(dimSty);
        return true;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading) {
                // Phase 3A.0: populate the vars map from the parsed struct so
                // the LibreCAD createDimStyle consumer (reads $DIM* keys) gets
                // the imported values, not reset() defaults. Copy-free (called
                // on dimSty before it is reset() for the next record).
                if (!publishDimStyle())
                    return setError(DRW::BAD_CODE_PARSED);
            }
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (dxfKeywordEquals(sectionstr, "DIMSTYLE")) {
                reading = true;
                // Start from a pristine record. reset() leaves the $DIM
                // override map and the optional string codes populated, and
                // syncStructToVars() keeps whatever the map already holds, so
                // reusing it imports every style with the first one's values.
                dimSty = DRW_Dimstyle{};
            } else if (dxfKeywordEquals(sectionstr, "ENDTAB")) {
                for (const DRW_Dimstyle &entry : pendingDimStyles)
                    iface->addDimStyle(entry);
                return true;  //found ENDTAB terminate
            } else {
                return setError(DRW::BAD_READ_TABLES);
            }
        } else if (reading) {
            if (!dimSty.parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }
    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processTextStyle(){
    DRW_DBG("dxfRW::processTextStyle");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Textstyle TxtSty;
    std::vector<DRW_Textstyle> pendingTextStyles;
    std::set<std::uint32_t> seenHandles;
    const auto publishTextStyle = [&]() -> bool {
        if (!dxfTableEntryComplete(TxtSty, reader->getSourceVersion())
            || !TxtSty.validateDxf())
            return false;
        if (TxtSty.handle != DRW::NoHandle
            && !seenHandles.insert(TxtSty.handle).second)
            return false;
        pendingTextStyles.push_back(TxtSty);
        return true;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading && !publishTextStyle())
                return setError(DRW::BAD_CODE_PARSED);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (dxfKeywordEquals(sectionstr, "STYLE")) {
                reading = true;
                TxtSty.reset();
            } else if (dxfKeywordEquals(sectionstr, "ENDTAB")) {
                for (const DRW_Textstyle &entry : pendingTextStyles)
                    iface->addTextStyle(entry);
                return true;  //found ENDTAB terminate
            } else {
                return setError(DRW::BAD_READ_TABLES);
            }
        } else if (reading) {
            if (!TxtSty.parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processVports(){
    DRW_DBG("dxfRW::processVports");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_Vport vp;
    std::vector<DRW_Vport> pendingVports;
    std::set<std::uint32_t> seenHandles;
    const auto publishVport = [&]() -> bool {
        if (!dxfTableEntryComplete(vp, reader->getSourceVersion())
            || !vp.validateDxf())
            return false;
        if (vp.handle != DRW::NoHandle
            && !seenHandles.insert(vp.handle).second)
            return false;
        pendingVports.push_back(vp);
        return true;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading && !publishVport())
                return setError(DRW::BAD_CODE_PARSED);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (dxfKeywordEquals(sectionstr, "VPORT")) {
                reading = true;
                vp.reset();
            } else if (dxfKeywordEquals(sectionstr, "ENDTAB")) {
                for (const DRW_Vport &entry : pendingVports)
                    iface->addVport(entry);
                return true;  //found ENDTAB terminate
            } else {
                return setError(DRW::BAD_READ_TABLES);
            }
        } else if (reading) {
            if (!vp.parseCode(code, reader)) {
                return setError( DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processView(){
    DRW_DBG("dxfRW::processView");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_View v;
    std::vector<DRW_View> pendingViews;
    std::set<std::uint32_t> seenHandles;
    const auto publishView = [&]() -> bool {
        if (!dxfTableEntryComplete(v, reader->getSourceVersion())
            || !v.validateDxf())
            return false;
        if (v.handle != DRW::NoHandle
            && !seenHandles.insert(v.handle).second)
            return false;
        pendingViews.push_back(v);
        return true;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading && !publishView())
                return setError(DRW::BAD_CODE_PARSED);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (dxfKeywordEquals(sectionstr, "VIEW")) {
                reading = true;
                v.reset();
            } else if (dxfKeywordEquals(sectionstr, "ENDTAB")) {
                for (const DRW_View &entry : pendingViews)
                    iface->addView(entry);
                return true;
            } else {
                return setError(DRW::BAD_READ_TABLES);
            }
        } else if (reading) {
            if (!v.parseCode(code, reader)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
        }
    }
    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processUCS(){
    DRW_DBG("dxfRW::processUCS");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_UCS u;
    std::vector<DRW_UCS> pendingUcs;
    std::set<std::uint32_t> seenHandles;
    const auto publishUcs = [&]() -> bool {
        if (!dxfTableEntryComplete(u, reader->getSourceVersion())
            || !u.validateDxf())
            return false;
        if (u.handle != DRW::NoHandle
            && !seenHandles.insert(u.handle).second)
            return false;
        pendingUcs.push_back(u);
        return true;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading && !publishUcs())
                return setError(DRW::BAD_CODE_PARSED);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (dxfKeywordEquals(sectionstr, "UCS")) {
                reading = true;
                u.reset();
            } else if (dxfKeywordEquals(sectionstr, "ENDTAB")) {
                for (const DRW_UCS &entry : pendingUcs)
                    iface->addUCS(entry);
                return true;
            } else {
                return setError(DRW::BAD_READ_TABLES);
            }
        } else if (reading) {
            if (!u.parseCode(code, reader)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
        }
    }
    return setError(DRW::BAD_READ_TABLES);
}

bool dxfRW::processAppId(){
    DRW_DBG("dxfRW::processAppId");
    int code;
    std::string sectionstr;
    bool reading = false;
    DRW_AppId vp;
    std::vector<DRW_AppId> pendingAppIds;
    std::set<std::uint32_t> seenHandles;
    const auto publishAppId = [&]() -> bool {
        if (!dxfTableEntryComplete(vp, reader->getSourceVersion())
            || !vp.validateDxf())
            return false;
        if (vp.handle != DRW::NoHandle
            && !seenHandles.insert(vp.handle).second)
            return false;
        pendingAppIds.push_back(vp);
        return true;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (reading && !publishAppId())
                return setError(DRW::BAD_CODE_PARSED);
            sectionstr = reader->getString();
            DRW_DBG(sectionstr); DRW_DBG("\n");
            if (dxfKeywordEquals(sectionstr, "APPID")) {
                reading = true;
                vp.reset();
            } else if (dxfKeywordEquals(sectionstr, "ENDTAB")) {
                for (const DRW_AppId &entry : pendingAppIds)
                    iface->addAppId(entry);
                return true;  //found ENDTAB terminate
            } else {
                return setError(DRW::BAD_READ_TABLES);
            }
        } else if (reading) {
            if (!vp.parseCode(code, reader)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
        }
    }

    return setError(DRW::BAD_READ_TABLES);
}

/********* Block Section *********/

dxfRW::DxfEntityBoundary dxfRW::setEntityBoundary(int code) {
    if (code != 0) {
        // A failed probe must not leave an earlier code-0 token available to
        // a later dispatcher.
        nextentity.clear();
        m_hasPendingEntityBoundary = false;
        return DxfEntityBoundary::Error;
    }

    nextentity = reader->getString();
    m_hasPendingEntityBoundary = true;
    DRW_DBG(nextentity); DRW_DBG(" entity boundary\n");
    return classifyEntityBoundary();
}

dxfRW::DxfEntityBoundary dxfRW::classifyEntityBoundary() const {
    if (!m_hasPendingEntityBoundary || nextentity.empty())
        return DxfEntityBoundary::Error;
    if (dxfKeywordEquals(nextentity, "ENDSEC"))
        return DxfEntityBoundary::EndSection;
    if (dxfKeywordEquals(nextentity, "ENDBLK"))
        return DxfEntityBoundary::EndBlock;
    return DxfEntityBoundary::NextEntity;
}

bool dxfRW::acceptEntityCallbackBoundary() const {
    switch (classifyEntityBoundary()) {
    case DxfEntityBoundary::NextEntity:
        return true;
    case DxfEntityBoundary::EndSection:
        return !m_readingBlockEntities;
    case DxfEntityBoundary::EndBlock:
        return m_readingBlockEntities;
    case DxfEntityBoundary::Error:
        return false;
    }
    return false;
}

bool dxfRW::acceptObjectBoundary(int code) {
    const DxfEntityBoundary boundary = setEntityBoundary(code);
    return boundary == DxfEntityBoundary::NextEntity
        || boundary == DxfEntityBoundary::EndSection;
}

dxfRW::DxfEntityBoundary dxfRW::readEntityBoundary() {
    int code;
    if (!reader->readRec(&code)) {
        nextentity.clear();
        m_hasPendingEntityBoundary = false;
        return DxfEntityBoundary::Error;
    }
    return setEntityBoundary(code);
}

dxfRW::DxfEntityBoundary dxfRW::consumeEntityFooter() {
    int code;
    while (reader->readRec(&code)) {
        if (code != 0)
            continue;
        return setEntityBoundary(code);
    }
    nextentity.clear();
    m_hasPendingEntityBoundary = false;
    return DxfEntityBoundary::Error;
}

bool dxfRW::processBlocks() {
    DRW_DBG("dxfRW::processBlocks\n");
    nextentity.clear();
    m_hasPendingEntityBoundary = false;
    while (true) {
        if (!m_hasPendingEntityBoundary) {
            const DxfEntityBoundary boundary = readEntityBoundary();
            if (boundary == DxfEntityBoundary::EndSection)
                return true;
            if (boundary != DxfEntityBoundary::NextEntity)
                return setError(DRW::BAD_READ_BLOCKS);
        }
        const DxfEntityBoundary boundary = classifyEntityBoundary();
        if (boundary == DxfEntityBoundary::EndSection)
            return true;
        if (boundary != DxfEntityBoundary::NextEntity
            || !dxfKeywordEquals(nextentity, "BLOCK"))
            return setError(DRW::BAD_READ_BLOCKS);
        try {
            if (!processBlock()) {
                nextentity.clear();
                m_hasPendingEntityBoundary = false;
                return false;
            }
        } catch (...) {
            nextentity.clear();
            m_hasPendingEntityBoundary = false;
            throw;
        }
    }
}

bool dxfRW::processBlock() {
    DRW_DBG("dxfRW::processBlock");
    int code;
    DRW_Block block;
    bool hasName = false;
    bool hasHandle = false;
    bool blockPublished = false;
    bool hasStagedBlockRecord = false;
    DRW_ParsingContext::BlockRecordInfo stagedBlockRecord;

    // addBlock() establishes the destination for subsequent entity callbacks.
    // Once that scope is visible, every failure path must balance it so a
    // caller cannot remain attached to a malformed block.
    const auto closePublishedBlock = [&]() {
        if (!blockPublished)
            return;
        iface->endBlock();
        blockPublished = false;
    };
    const auto publishBlockRecord = [&]() {
        if (hasStagedBlockRecord) {
            m_readingContext.blockRecordMap[block.handle] =
                std::move(stagedBlockRecord);
        }
    };

    auto consumeEndBlock = [&]() -> bool {
        DRW_Block footer;
        footer.setIsEnd(true);
        bool hasEndHandle = false;
        bool hasEndOwner = false;
        bool hasLayer = false;
        bool hasEntitySubclass = false;
        bool hasBlockEndSubclass = false;
        std::uint32_t endOwner = DRW::NoHandle;

        const auto isXdataCode = [](int value) {
            return value == 1000 || value == 1001 || value == 1002
                || value == 1003 || value == 1004 || value == 1005
                || (value >= 1010 && value <= 1013)
                || (value >= 1020 && value <= 1023)
                || (value >= 1030 && value <= 1033)
                || (value >= 1040 && value <= 1042)
                || value == 1070 || value == 1071;
        };
        const auto isAllowedCode = [isXdataCode](int value) {
            return value == DRW::dxfCode::HANDLE
                || value == DRW::dxfCode::OWNER_HANDLE
                || value == DRW::dxfCode::LAYER
                || value == 100 || value == 102 || isXdataCode(value);
        };

        while (reader->readRec(&code)) {
            if (code == DRW::dxfCode::HANDLE) {
                // Code 5 is deliberately not validated by readRec(): a few
                // table records use it as a name.  ENDBLK is an object
                // record, so its handle is a real drawing-wide identity.
                if (hasEndHandle || !reader->isValidHandleString())
                    return false;
                const std::uint32_t handle = reader->getHandleString();
                if (handle == DRW::NoHandle || !reader->registerSelfHandle())
                    return false;
                footer.handle = handle;
                hasEndHandle = true;
                continue;
            }
            if (code == DRW::dxfCode::OWNER_HANDLE) {
                if (hasEndOwner || !reader->isValidHandleString())
                    return false;
                endOwner = reader->getHandleString();
                if (endOwner == DRW::NoHandle)
                    return false;
                footer.parentHandle = endOwner;
                hasEndOwner = true;
                continue;
            }
            if (code != 0) {
                if (!isAllowedCode(code))
                    return false;
                if (code == DRW::dxfCode::LAYER) {
                    if (hasLayer)
                        return false;
                    hasLayer = true;
                } else if (code == 100) {
                    const std::string marker = reader->getString();
                    if (marker == "AcDbEntity") {
                        if (hasEntitySubclass)
                            return false;
                        hasEntitySubclass = true;
                    } else if (marker == "AcDbBlockEnd") {
                        if (hasBlockEndSubclass)
                            return false;
                        hasBlockEndSubclass = true;
                    } else {
                        return false;
                    }
                }
                if (code != 100 && !footer.parseCode(code, reader))
                    return false;
                continue;
            }

            const DxfEntityBoundary boundary = setEntityBoundary(code);
            if (boundary == DxfEntityBoundary::Error
                || boundary == DxfEntityBoundary::EndBlock)
                return false;

            const DRW::Version sourceVersion = reader->getSourceVersion();
            const bool requiresHandle =
                sourceVersion != DRW::UNKNOWNV && sourceVersion > DRW::AC1009;
            // Minimal modern BLOCK fixtures may omit the optional-looking
            // owner pair entirely. Once BLOCK carries a BLOCK_RECORD owner,
            // however, ENDBLK must carry the same owner as well.
            const bool requiresOwner = reader->hasSourceVersion()
                && sourceVersion > DRW::AC1014
                && block.parentHandle != DRW::NoHandle;
            if ((requiresHandle && !hasEndHandle)
                || (requiresOwner && !hasEndOwner))
                return false;
            if (hasEndOwner && block.parentHandle != DRW::NoHandle
                && endOwner != block.parentHandle)
                return false;

            return boundary == DxfEntityBoundary::NextEntity
                || boundary == DxfEntityBoundary::EndSection;
        }
        return false;
    };

    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            if (boundary == DxfEntityBoundary::Error ||
                boundary == DxfEntityBoundary::EndSection) {
                return setError(DRW::BAD_READ_BLOCKS);
            }
            const bool requiresHandle =
                reader->getSourceVersion() != DRW::UNKNOWNV
                && reader->getSourceVersion() > DRW::AC1009;
            if (!hasName || block.name.empty()
                || (requiresHandle && (!hasHandle
                                       || block.handle == DRW::NoHandle))) {
                return setError(DRW::BAD_READ_BLOCKS);
            }
            if (block.handle != DRW::NoHandle
                && m_readingContext.blockRecordMap.find(block.handle)
                       != m_readingContext.blockRecordMap.end()) {
                // BLOCK begin handles are drawing-wide object handles. Do not
                // let a later duplicate overwrite the first block's
                // resolution entry or publish a second scope with the same id.
                return setError(DRW::BAD_READ_BLOCKS);
            }
            if (block.parentHandle != DRW::NoHandle) {
                const auto recordName = m_readingContext.resolveBlockRecordName(block.parentHandle);
                if (!recordName.empty())
                    block.name = recordName;
                block.insUnits = m_readingContext.resolveBlockRecordInsUnits(block.parentHandle);
                block.previewData =
                    m_readingContext.resolveBlockRecordPreview(block.parentHandle);
            }
            if (block.handle != DRW::NoHandle && !block.name.empty()) {
                stagedBlockRecord.name = block.name;
                stagedBlockRecord.insUnits = block.insUnits;
                stagedBlockRecord.previewData = block.previewData;
                hasStagedBlockRecord = true;
            }
            iface->addBlock(block);
            blockPublished = true;
            if (boundary == DxfEntityBoundary::EndBlock) {
                if (!consumeEndBlock()) {
                    closePublishedBlock();
                    return setError(DRW::BAD_READ_BLOCKS);
                }
                publishBlockRecord();
                closePublishedBlock();
                return true;  //found ENDBLK, terminate
            } else {
                const bool previousBlockContext = m_readingBlockEntities;
                DRW_Interface *const previousInterface = iface;
                DxfBlockEventSink blockEvents(*previousInterface);
                m_readingBlockEntities = true;
                iface = &blockEvents;
                bool entitiesProcessed = false;
                try {
                    entitiesProcessed = processEntities(true);
                } catch (...) {
                    iface = previousInterface;
                    m_readingBlockEntities = previousBlockContext;
                    closePublishedBlock();
                    throw;
                }
                iface = previousInterface;
                m_readingBlockEntities = previousBlockContext;
                if (!entitiesProcessed || blockEvents.failed()) {
                    closePublishedBlock();
                    return setError(DRW::BAD_READ_BLOCKS);
                }
                if (classifyEntityBoundary() != DxfEntityBoundary::EndBlock) {
                    closePublishedBlock();
                    return setError(DRW::BAD_READ_BLOCKS);
                }
                if (!consumeEndBlock()) {
                    closePublishedBlock();
                    return setError(DRW::BAD_READ_BLOCKS);
                }
                if (!blockEvents.flush()) {
                    closePublishedBlock();
                    return setError(DRW::BAD_READ_BLOCKS);
                }
                publishBlockRecord();
                closePublishedBlock();
                return true;  //found ENDBLK, terminate
            }
            }

        if (code == 2)
            hasName = true;
        else if (code == DRW::dxfCode::HANDLE)
            hasHandle = true;
        if (!block.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
}

    return setError(DRW::BAD_READ_BLOCKS);
}


/********* Entities Section *********/

bool dxfRW::processEntities(bool isblock) {
    DRW_DBG("dxfRW::processEntities\n");
    if (!isblock || !m_hasPendingEntityBoundary) {
        const DxfEntityBoundary boundary = readEntityBoundary();
        if (boundary == DxfEntityBoundary::Error ||
            (boundary == DxfEntityBoundary::EndBlock && !isblock)) {
            return setError(DRW::BAD_READ_ENTITIES);
        }
    }

    while (true) {
        const DxfEntityBoundary boundary = classifyEntityBoundary();
        if (boundary == DxfEntityBoundary::Error)
            return setError(DRW::BAD_READ_ENTITIES);
        if (boundary == DxfEntityBoundary::EndSection)
            return true;
        if (boundary == DxfEntityBoundary::EndBlock) {
            if (isblock)
                return true;
            return setError(DRW::BAD_READ_ENTITIES);
        }

        bool processed {false};
        if (dxfKeywordEquals(nextentity, "POINT")) {
            processed = processPoint();
        } else if (dxfKeywordEquals(nextentity, "LINE")) {
            processed = processLine();
        } else if (dxfKeywordEquals(nextentity, "3DLINE")) {
            processed = process3DLine();
        }  else if (dxfKeywordEquals(nextentity, "CIRCLE")) {
            processed = processCircle();
        } else if (dxfKeywordEquals(nextentity, "ARC")) {
            processed = processArc();
        } else if (dxfKeywordEquals(nextentity, "ELLIPSE")) {
            processed = processEllipse();
        } else if (dxfKeywordEquals(nextentity, "TRACE")) {
            processed = processTrace();
        } else if (dxfKeywordEquals(nextentity, "SOLID")) {
            processed = processSolid();
        } else if (dxfKeywordEquals(nextentity, "SHAPE")) {
            processed = processShape();
        } else if (dxfKeywordEquals(nextentity, "OLEFRAME")) {
            processed = processOleFrame();
        } else if (dxfKeywordEquals(nextentity, "OLE2FRAME")) {
            processed = processOle2Frame();
        } else if (dxfKeywordEquals(nextentity, "INSERT")
                   || dxfKeywordEquals(nextentity, "MINSERT")) {
            processed = processInsert();
        } else if (dxfKeywordEquals(nextentity, "ACAD_TABLE")) {
            processed = processTable();
        } else if (dxfKeywordEquals(nextentity, "LWPOLYLINE")) {
            processed = processLWPolyline();
        } else if (dxfKeywordEquals(nextentity, "POLYLINE")) {
            processed = processPolyline();
        } else if (dxfKeywordEquals(nextentity, "TEXT")) {
            processed = processText();
        } else if (dxfKeywordEquals(nextentity, "ATTDEF")) {
            processed = processAttdef();
        } else if (dxfKeywordEquals(nextentity, "MTEXT")) {
            processed = processMText();
        } else if (dxfKeywordEquals(nextentity, "RTEXT")) {
            processed = processRText();
        } else if (dxfKeywordEquals(nextentity, "CAMERA")) {
            processed = processCamera();
        } else if (dxfKeywordEquals(nextentity, "GEOPOSITIONMARKER")
                   || dxfKeywordEquals(nextentity, "POSITIONMARKER")) {
            processed = processGeoPositionMarker();
        } else if (dxfKeywordEquals(nextentity, "SECTIONOBJECT")
                   || dxfKeywordEquals(nextentity, "SECTION_OBJECT")) {
            processed = processSectionObject();
        } else if (dxfKeywordEquals(nextentity, "ARCALIGNEDTEXT")
                   || dxfKeywordEquals(nextentity, "ARC_ALIGNED_TEXT")) {
            processed = processArcAlignedText();
        } else if (dxfKeywordEquals(nextentity, "MLINE")) {
            processed = processMLine();
        } else if (dxfKeywordEquals(nextentity, "PDFUNDERLAY")
                   || dxfKeywordEquals(nextentity, "PDFREFERENCE")
                   || dxfKeywordEquals(nextentity, "DGNUNDERLAY")
                   || dxfKeywordEquals(nextentity, "DWFUNDERLAY")) {
            processed = processUnderlay(nextentity);
        } else if (dxfKeywordEquals(nextentity, "HATCH")) {
            processed = processHatch();
        } else if (dxfKeywordEquals(nextentity, "MPOLYGON")) {
            processed = processMPolygon();
        } else if (dxfKeywordEquals(nextentity, "SPLINE")) {
            processed = processSpline();
        } else if (dxfKeywordEquals(nextentity, "HELIX")) {
            processed = processHelix();
        } else if (dxfKeywordEquals(nextentity, "3DFACE")) {
            processed = process3dface();
        } else if (dxfKeywordEquals(nextentity, "MESH")) {
            processed = processMesh();
        } else if (dxfKeywordEquals(nextentity, "VIEWPORT")) {
            processed = processViewport();
        } else if (dxfKeywordEquals(nextentity, "IMAGE")) {
            processed = processImage();
        } else if (dxfKeywordEquals(nextentity, "WIPEOUT")) {
            processed = processWipeout();
        } else if (dxfKeywordEquals(nextentity, "POINTCLOUD")) {
            processed = processPointCloud();
        } else if (dxfKeywordEquals(nextentity, "POINTCLOUDEX")) {
            processed = processPointCloudEx();
        } else if (dxfKeywordEquals(nextentity, "NAVISWORKSMODEL")) {
            processed = processNavisworksModel();
        } else if (dxfKeywordEquals(nextentity, "PLANESURFACE")
                   || dxfKeywordEquals(nextentity, "EXTRUDEDSURFACE")
                   || dxfKeywordEquals(nextentity, "REVOLVEDSURFACE")
                   || dxfKeywordEquals(nextentity, "SWEPTSURFACE")
                   || dxfKeywordEquals(nextentity, "LOFTEDSURFACE")
                   || dxfKeywordEquals(nextentity, "NURBSSURFACE")) {
            processed = processSurface();
        } else if (dxfKeywordEquals(nextentity, "3DSOLID")
                   || dxfKeywordEquals(nextentity, "REGION")
                   || dxfKeywordEquals(nextentity, "BODY")) {
            processed = processModelerGeometry();
        } else if (dxfKeywordEquals(nextentity, "MULTILEADER")) {
            processed = processMultiLeader();
        } else if (dxfKeywordEquals(nextentity, "DIMENSION")) {
            processed = processDimension();
        } else if (dxfKeywordEquals(nextentity, "ARC_DIMENSION")) {
            processed = processArcDimension();
        } else if (dxfKeywordEquals(nextentity, "LARGE_RADIAL_DIMENSION")) {
            processed = processLargeRadialDimension();
        } else if (dxfKeywordEquals(nextentity, "LEADER")) {
            processed = processLeader();
        } else if (dxfKeywordEquals(nextentity, "RAY")) {
            processed = processRay();
        } else if (dxfKeywordEquals(nextentity, "XLINE")) {
            processed = processXline();
        } else if (dxfKeywordEquals(nextentity, "TOLERANCE")) {
            processed = processTolerance();
        } else if (dxfKeywordEquals(nextentity, "ACAD_PROXY_ENTITY")) {
            processed = processProxyEntity();
        } else {
            // Slice A4: capture an unmodeled entity verbatim rather than dropping it.
            processed = processRawEntity();
        }
        if (!processed)
            return error == DRW::BAD_NONE
                ? setError(DRW::BAD_READ_ENTITIES)
                : false;

        const DxfEntityBoundary nextBoundary = classifyEntityBoundary();
        if (nextBoundary == DxfEntityBoundary::Error)
            return setError(DRW::BAD_READ_ENTITIES);
        if (nextBoundary == DxfEntityBoundary::EndSection)
            return true;
        if (nextBoundary == DxfEntityBoundary::EndBlock) {
            if (isblock)
                return true;
            return setError(DRW::BAD_READ_ENTITIES);
        }
    }
}

bool dxfRW::processEllipse() {
    DRW_DBG("dxfRW::processEllipse");
    int code;
    DRW_Ellipse ellipse;
    bool hasCenterX = false;
    bool hasCenterY = false;
    bool hasMajorAxisX = false;
    bool hasMajorAxisY = false;
    bool hasRatio = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasCenterX || !hasCenterY || !hasMajorAxisX
                || !hasMajorAxisY || !hasRatio)
                return setError(DRW::BAD_CODE_PARSED);
            if (applyExt)
                ellipse.applyExtrusion();
            iface->addEllipse(ellipse);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasCenterX = hasCenterX || code == 10;
        hasCenterY = hasCenterY || code == 20;
        hasMajorAxisX = hasMajorAxisX || code == 11;
        hasMajorAxisY = hasMajorAxisY || code == 21;
        hasRatio = hasRatio || code == 40;

        if (!ellipse.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processTrace() {
    DRW_DBG("dxfRW::processTrace");
    int code;
    DRW_Trace trace;
    bool hasFirstX = false;
    bool hasFirstY = false;
    bool hasSecondX = false;
    bool hasSecondY = false;
    bool hasThirdX = false;
    bool hasThirdY = false;
    bool hasFourthX = false;
    bool hasFourthY = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasFirstX || !hasFirstY || !hasSecondX || !hasSecondY
                || !hasThirdX || !hasThirdY
                || hasFourthX != hasFourthY)
                return setError(DRW::BAD_CODE_PARSED);
            if (!hasFourthX)
                trace.fourPoint = trace.thirdPoint;
            if (applyExt)
                trace.applyExtrusion();
            iface->addTrace(trace);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasFirstX = hasFirstX || code == 10;
        hasFirstY = hasFirstY || code == 20;
        hasSecondX = hasSecondX || code == 11;
        hasSecondY = hasSecondY || code == 21;
        hasThirdX = hasThirdX || code == 12;
        hasThirdY = hasThirdY || code == 22;
        hasFourthX = hasFourthX || code == 13;
        hasFourthY = hasFourthY || code == 23;

        if (!trace.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processSolid() {
    DRW_DBG("dxfRW::processSolid");
    int code;
    DRW_Solid solid;
    bool hasFirstX = false;
    bool hasFirstY = false;
    bool hasSecondX = false;
    bool hasSecondY = false;
    bool hasThirdX = false;
    bool hasThirdY = false;
    bool hasFourthX = false;
    bool hasFourthY = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasFirstX || !hasFirstY || !hasSecondX || !hasSecondY
                || !hasThirdX || !hasThirdY
                || hasFourthX != hasFourthY)
                return setError(DRW::BAD_CODE_PARSED);
            if (!hasFourthX)
                solid.fourPoint = solid.thirdPoint;
            if (applyExt)
                solid.applyExtrusion();
            iface->addSolid(solid);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasFirstX = hasFirstX || code == 10;
        hasFirstY = hasFirstY || code == 20;
        hasSecondX = hasSecondX || code == 11;
        hasSecondY = hasSecondY || code == 21;
        hasThirdX = hasThirdX || code == 12;
        hasThirdY = hasThirdY || code == 22;
        hasFourthX = hasFourthX || code == 13;
        hasFourthY = hasFourthY || code == 23;

        if (!solid.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processShape() {
    DRW_DBG("dxfRW::processShape\n");
    int code;
    DRW_Shape shape;
    bool hasStyleName = false;
    bool hasInsertionX = false;
    bool hasInsertionY = false;
    bool hasSize = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasStyleName || !hasInsertionX || !hasInsertionY || !hasSize)
                return setError(DRW::BAD_CODE_PARSED);
            const auto finite = [](const DRW_Coord& point) {
                return std::isfinite(point.x) && std::isfinite(point.y)
                       && std::isfinite(point.z);
            };
            if (!finite(shape.m_insertionPoint)
                || !finite(shape.m_extrusion)
                || !std::isfinite(shape.m_scale)
                || !std::isfinite(shape.m_rotation)
                || !std::isfinite(shape.m_widthFactor)
                || !std::isfinite(shape.m_oblique)
                || !std::isfinite(shape.m_thickness)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
            iface->addShape(shape);
            return true;
        }

        if (code == 2)
            hasStyleName = !reader->getUtf8String().empty();
        hasInsertionX = hasInsertionX || code == 10;
        hasInsertionY = hasInsertionY || code == 20;
        hasSize = hasSize || code == 40;

        if (!shape.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processOle2Frame() {
    DRW_DBG("dxfRW::processOle2Frame\n");
    int code;
    DRW_Ole2Frame frame;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!frame.m_dxfPayloadLengthSpecified
                || frame.m_payloadTooLarge
                || frame.m_payloadTruncated
                || frame.m_declaredPayloadLength != frame.m_payloadBytes.size()
                || !std::isfinite(frame.m_pt1.x)
                || !std::isfinite(frame.m_pt1.y)
                || !std::isfinite(frame.m_pt1.z)
                || !std::isfinite(frame.m_pt2.x)
                || !std::isfinite(frame.m_pt2.y)
                || !std::isfinite(frame.m_pt2.z)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
            frame.m_payloadByteCount = static_cast<std::uint32_t>(
                frame.m_payloadBytes.size());
            frame.m_payloadPresent = !frame.m_payloadBytes.empty();
            iface->addOle2Frame(frame);
            return true;
        }
        if (!frame.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processOleFrame() {
    DRW_DBG("dxfRW::processOleFrame\n");
    int code;
    DRW_OleFrame frame;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!frame.m_dxfPayloadLengthSpecified
                || frame.m_payloadTooLarge
                || frame.m_payloadTruncated
                || frame.m_declaredPayloadLength != frame.m_payloadBytes.size()) {
                return setError(DRW::BAD_CODE_PARSED);
            }
            frame.m_payloadByteCount = static_cast<std::uint32_t>(
                frame.m_payloadBytes.size());
            frame.m_payloadPresent = !frame.m_payloadBytes.empty();
            iface->addOleFrame(frame);
            return true;
        }
        if (!frame.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processMesh() {
    DRW_DBG("dxfRW::processMesh");
    int code;
    DRW_Mesh mesh;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!mesh.validateDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addMesh(mesh);
            return true;
        }
        if (!mesh.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::process3dface() {
    DRW_DBG("dxfRW::process3dface");
    int code;
    DRW_3Dface face;
    bool hasFirstX = false;
    bool hasFirstY = false;
    bool hasSecondX = false;
    bool hasSecondY = false;
    bool hasThirdX = false;
    bool hasThirdY = false;
    bool hasFourthX = false;
    bool hasFourthY = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasFirstX || !hasFirstY || !hasSecondX || !hasSecondY
                || !hasThirdX || !hasThirdY
                || hasFourthX != hasFourthY)
                return setError(DRW::BAD_CODE_PARSED);
            if (!hasFourthX)
                face.fourPoint = face.thirdPoint;
            iface->add3dFace(face);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasFirstX = hasFirstX || code == 10;
        hasFirstY = hasFirstY || code == 20;
        hasSecondX = hasSecondX || code == 11;
        hasSecondY = hasSecondY || code == 21;
        hasThirdX = hasThirdX || code == 12;
        hasThirdY = hasThirdY || code == 22;
        hasFourthX = hasFourthX || code == 13;
        hasFourthY = hasFourthY || code == 23;

        if (!face.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processViewport() {
    DRW_DBG("dxfRW::processViewport");
    int code;
    DRW_Viewport vp;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!vp.validateDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addViewport(vp);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!vp.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPoint() {
    DRW_DBG("dxfRW::processPoint\n");
    int code;
    DRW_Point point;
    bool hasX = false;
    bool hasY = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasX || !hasY)
                return setError(DRW::BAD_CODE_PARSED);
            iface->addPoint(point);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasX = hasX || code == 10;
        hasY = hasY || code == 20;

        if (!point.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processLine() {
    DRW_DBG("dxfRW::processLine\n");
    int code;
    DRW_Line line;
    bool hasStartX = false;
    bool hasStartY = false;
    bool hasEndX = false;
    bool hasEndY = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasStartX || !hasStartY || !hasEndX || !hasEndY)
                return setError(DRW::BAD_CODE_PARSED);
            iface->addLine(line);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasStartX = hasStartX || code == 10;
        hasStartY = hasStartY || code == 20;
        hasEndX = hasEndX || code == 11;
        hasEndY = hasEndY || code == 21;

        if (!line.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
}
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::process3DLine() {
    DRW_DBG("dxfRW::process3DLine\n");
    int code;
    DRW_3DLine line;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->add3DLine(line);
            return true;
        }

        if (!line.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processMLine() {
    DRW_DBG("dxfRW::processMLine\n");
    int code;
    DRW_MLine mline;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!mline.validateDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addMLine(&mline);
            return true;
        }
        if (!mline.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processUnderlay(const std::string& kind) {
    DRW_DBG("dxfRW::processUnderlay\n");
    int code;
    DRW_Underlay u;
    if (dxfKeywordEquals(kind, "DGNUNDERLAY")) u.kind = DRW_Underlay::DGN;
    else if (dxfKeywordEquals(kind, "DWFUNDERLAY")) u.kind = DRW_Underlay::DWF;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addUnderlay(&u);
            return true;
        }
        if (!u.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processRay() {
    DRW_DBG("dxfRW::processRay\n");
    int code;
    DRW_Ray line;
    bool hasStartX = false;
    bool hasStartY = false;
    bool hasDirectionX = false;
    bool hasDirectionY = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasStartX || !hasStartY || !hasDirectionX || !hasDirectionY)
                return setError(DRW::BAD_CODE_PARSED);
            iface->addRay(line);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasStartX = hasStartX || code == 10;
        hasStartY = hasStartY || code == 20;
        hasDirectionX = hasDirectionX || code == 11;
        hasDirectionY = hasDirectionY || code == 21;

        if (!line.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processXline() {
    DRW_DBG("dxfRW::processXline\n");
    int code;
    DRW_Xline line;
    bool hasStartX = false;
    bool hasStartY = false;
    bool hasDirectionX = false;
    bool hasDirectionY = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasStartX || !hasStartY || !hasDirectionX || !hasDirectionY)
                return setError(DRW::BAD_CODE_PARSED);
            iface->addXline(line);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasStartX = hasStartX || code == 10;
        hasStartY = hasStartY || code == 20;
        hasDirectionX = hasDirectionX || code == 11;
        hasDirectionY = hasDirectionY || code == 21;

        if (!line.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processCircle() {
    DRW_DBG("dxfRW::processPoint\n");
    int code;
    DRW_Circle circle;
    bool hasCenterX = false;
    bool hasCenterY = false;
    bool hasRadius = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasCenterX || !hasCenterY || !hasRadius)
                return setError(DRW::BAD_CODE_PARSED);
            if (applyExt)
                circle.applyExtrusion();
            iface->addCircle(circle);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasCenterX = hasCenterX || code == 10;
        hasCenterY = hasCenterY || code == 20;
        hasRadius = hasRadius || code == 40;

        if (!circle.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processArc() {
    DRW_DBG("dxfRW::processPoint\n");
    int code;
    DRW_Arc arc;
    bool hasCenterX = false;
    bool hasCenterY = false;
    bool hasRadius = false;
    bool hasStartAngle = false;
    bool hasEndAngle = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasCenterX || !hasCenterY || !hasRadius
                || !hasStartAngle || !hasEndAngle)
                return setError(DRW::BAD_CODE_PARSED);
            if (applyExt)
                arc.applyExtrusion();
            iface->addArc(arc);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasCenterX = hasCenterX || code == 10;
        hasCenterY = hasCenterY || code == 20;
        hasRadius = hasRadius || code == 40;
        hasStartAngle = hasStartAngle || code == 50;
        hasEndAngle = hasEndAngle || code == 51;

        if (!arc.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processInsert() {
    DRW_DBG("dxfRW::processInsert");
    int code;
    DRW_Insert insert;
    bool hasName = false;
    bool hasInsertionX = false;
    bool hasInsertionY = false;
    const auto validInsert = [&]() {
        return hasName && !insert.name.empty() && hasInsertionX && hasInsertionY;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (boundary == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            if (!validInsert())
                return setError(DRW::BAD_CODE_PARSED);
            // Attribute flag (66=1) signals trailing ATTRIB entities; mirror
            // the POLYLINE/VERTEX/SEQEND pattern and gate on the next entity
            // name rather than the flag (some writers omit code 66).
            if (!dxfKeywordEquals(nextentity, "ATTRIB")) {
                if (!acceptEntityCallbackBoundary())
                    return setError(DRW::BAD_READ_ENTITIES);
                iface->addInsert(insert);
                return true;  //found new entity or ENDSEC, terminate
            }
            if (!processAttrib(&insert))  //fills insert.attlist until SEQEND
                return false;
            if (!dxfKeywordEquals(nextentity, "SEQEND"))
                return setError(DRW::BAD_READ_ENTITIES);
            if (consumeEntityFooter() == DxfEntityBoundary::Error
                || !acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addInsert(insert);
            return true;
        }

        hasName = hasName || code == 2;
        hasInsertionX = hasInsertionX || code == 10;
        hasInsertionY = hasInsertionY || code == 20;

        if (!insert.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processTable() {
    DRW_DBG("dxfRW::processTable");
    int code;
    DRW_Table table;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (boundary == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            if (!dxfKeywordEquals(nextentity, "ATTRIB")) {
                if (!acceptEntityCallbackBoundary())
                    return setError(DRW::BAD_READ_ENTITIES);
                iface->addTable(table);
                return true;  //found new entity or ENDSEC, terminate
            }
            if (!processAttrib(&table))
                return false;
            if (!dxfKeywordEquals(nextentity, "SEQEND"))
                return setError(DRW::BAD_READ_ENTITIES);
            if (consumeEntityFooter() == DxfEntityBoundary::Error
                || !acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addTable(table);
            return true;
        }

        if (!table.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processAttrib(DRW_Insert *insert) {
    DRW_DBG("dxfRW::processAttrib");
    int code;
    auto att = std::make_shared<DRW_Attrib>();
    bool hasText = false;
    bool hasTag = false;
    bool hasInsertionX = false;
    bool hasInsertionY = false;
    bool hasHeight = false;
    const auto validAttribute = [&]() {
        return hasText && hasTag && !att->tag.empty() && hasInsertionX
            && hasInsertionY && hasHeight;
    };
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (boundary != DxfEntityBoundary::NextEntity)
                return setError(DRW::BAD_READ_ENTITIES);
            if (dxfKeywordEquals(nextentity, "SEQEND")) {
                if (!validAttribute())
                    return setError(DRW::BAD_CODE_PARSED);
                insert->attlist.push_back(att);
                return true;  //found SEQEND, no more attribs, terminate
            }
            if (dxfKeywordEquals(nextentity, "ATTRIB")) {
                if (!validAttribute())
                    return setError(DRW::BAD_CODE_PARSED);
                insert->attlist.push_back(att);
                att = std::make_shared<DRW_Attrib>(); //another attrib
                hasText = false;
                hasTag = false;
                hasInsertionX = false;
                hasInsertionY = false;
                hasHeight = false;
                continue;
            }
            return setError(DRW::BAD_READ_ENTITIES);
        }

        hasText = hasText || code == 1;
        hasTag = hasTag || code == 2;
        hasInsertionX = hasInsertionX || code == 10;
        hasInsertionY = hasInsertionY || code == 20;
        hasHeight = hasHeight || code == 40;

        if (!att->parseCode(code, reader)) { //members of att are reinitialized here
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processAttdef() {
    DRW_DBG("dxfRW::processAttdef");
    int code;
    DRW_Attdef attdef;
    bool hasText = false;
    bool hasTag = false;
    bool hasInsertionX = false;
    bool hasInsertionY = false;
    bool hasHeight = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasText || !hasTag || attdef.tag.empty() || !hasInsertionX
                || !hasInsertionY || !hasHeight)
                return setError(DRW::BAD_CODE_PARSED);
            if (applyExt)
                attdef.applyExtrusion();
            iface->addAttDef(attdef);
            return true;
        }
        hasText = hasText || code == 1;
        hasTag = hasTag || code == 2;
        hasInsertionX = hasInsertionX || code == 10;
        hasInsertionY = hasInsertionY || code == 20;
        hasHeight = hasHeight || code == 40;

        if (!attdef.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processLWPolyline() {
    DRW_DBG("dxfRW::processLWPolyline");
    int code;
    DRW_LWPolyline pl;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!pl.validateDxf())
                return setError(DRW::BAD_CODE_PARSED);
            if (applyExt)
                pl.applyExtrusion();
            iface->addLWPolyline(pl);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!pl.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPolyline() {
    DRW_DBG("dxfRW::processPolyline");
    int code;
    DRW_Polyline pl;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (boundary != DxfEntityBoundary::NextEntity
                || !dxfKeywordEquals(nextentity, "VERTEX")) {
                // POLYLINE owns a VERTEX...SEQEND chain.  A section or block
                // boundary before SEQEND is a truncated child sequence; do
                // not publish the parent with partial geometry.
                return setError(DRW::BAD_READ_ENTITIES);
            }
            if (!processVertex(&pl))
                return false;
            if (!dxfKeywordEquals(nextentity, "SEQEND"))
                return setError(DRW::BAD_READ_ENTITIES);
            if (consumeEntityFooter() == DxfEntityBoundary::Error
                || !acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addPolyline(pl);
            return true;
        }

        if (!pl.parseCode(code, reader)) { //parseCode just initialize the members of pl
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processVertex(DRW_Polyline *pl) {
    DRW_DBG("dxfRW::processVertex");
    int code;
    auto v = std::make_shared<DRW_Vertex>();
    bool hasInsertionX = false;
    bool hasInsertionY = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if(0 == code)  {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (boundary != DxfEntityBoundary::NextEntity)
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasInsertionX || !hasInsertionY
                || !v->validatePayloadFields())
                return setError(DRW::BAD_CODE_PARSED);
            if (dxfKeywordEquals(nextentity, "SEQEND")) {
                pl->appendVertex(v);
                return true;  //found SEQEND no more vertex, terminate
            }
            if (dxfKeywordEquals(nextentity, "VERTEX")){
                pl->appendVertex(v);
                v = std::make_shared<DRW_Vertex>(); //another vertex
                continue;
            }
            return setError(DRW::BAD_READ_ENTITIES);
        }

        hasInsertionX = hasInsertionX || code == 10;
        hasInsertionY = hasInsertionY || code == 20;

        if (!v->parseCode(code, reader)) { //the members of v are reinitialized here
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processTolerance() {
    DRW_DBG("dxfRW::processTolerance");
    int code;
    DRW_Tolerance tol;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addTolerance(tol);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!tol.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processText() {
    DRW_DBG("dxfRW::processText");
    int code;
    DRW_Text txt;
    bool hasText = false;
    bool hasInsertionX = false;
    bool hasInsertionY = false;
    bool hasHeight = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasText || !hasInsertionX || !hasInsertionY || !hasHeight)
                return setError(DRW::BAD_CODE_PARSED);
            iface->addText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasText = hasText || code == 1;
        hasInsertionX = hasInsertionX || code == 10;
        hasInsertionY = hasInsertionY || code == 20;
        hasHeight = hasHeight || code == 40;

        if (!txt.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processMText() {
    DRW_DBG("dxfRW::processMText");
    int code;
    DRW_MText txt;
    bool hasText = false;
    bool hasInsertionX = false;
    bool hasInsertionY = false;
    bool hasHeight = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasText || !hasInsertionX || !hasInsertionY || !hasHeight)
                return setError(DRW::BAD_CODE_PARSED);
            txt.updateAngle();
            iface->addMText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasText = hasText || code == 1 || code == 3;
        hasInsertionX = hasInsertionX || code == 10;
        hasInsertionY = hasInsertionY || code == 20;
        hasHeight = hasHeight || code == 40;

        if (!txt.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

// RTEXT (Express Tools reactive text) — read-only, mapped onto DRW_Text and
// delivered via addText.  Its DXF group codes are a TEXT subset plus a flags
// long (70); DRW_RText::parseCode handles the flag and delegates the rest.
bool dxfRW::processRText() {
    DRW_DBG("dxfRW::processRText");
    int code;
    DRW_RText txt;
    bool hasText = false;
    bool hasInsertionX = false;
    bool hasInsertionY = false;
    bool hasHeight = false;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hasText || !hasInsertionX || !hasInsertionY || !hasHeight)
                return setError(DRW::BAD_CODE_PARSED);
            iface->addText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }

        hasText = hasText || code == 1;
        hasInsertionX = hasInsertionX || code == 10;
        hasInsertionY = hasInsertionY || code == 20;
        hasHeight = hasHeight || code == 40;

        if (!txt.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

// CAMERA (AcDbCamera) carries only a VIEW table soft pointer in the public
// DXF representation. The entity is metadata-only for LibreCAD.
bool dxfRW::processCamera() {
    DRW_DBG("dxfRW::processCamera");
    int code;
    DRW_Camera camera;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addCamera(camera);
            return true;
        }
        if (!camera.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processGeoPositionMarker() {
    DRW_DBG("dxfRW::processGeoPositionMarker");
    int code;
    DRW_GeoPositionMarker marker;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            marker.handle = raw.handle;
            marker.parentHandle = raw.parentHandle;
            iface->addGeoPositionMarker(marker);
            iface->addRawDxfEntity(raw);
            return true;
        }

        if (code == 102) {
            if (!captureRawDxfApplicationGroup(raw, marker))
                return setError(DRW::BAD_CODE_PARSED);
            continue;
        }
        if (!captureRawGroup(raw, code, /*validateHandles=*/true)
            || raw.groups.empty()
            || !marker.parseDxfVariant(raw.groups.back())) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processSectionObject() {
    DRW_DBG("dxfRW::processSectionObject");
    int code;
    DRW_SectionObject section;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (section.m_verts.size() > DRW_SectionObject::kMaxVertices
                || section.m_blVerts.size() > DRW_SectionObject::kMaxVertices)
                return setError(DRW::BAD_CODE_PARSED);
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            iface->addSectionObject(section);
            iface->addRawDxfEntity(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, section))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

// ARCALIGNEDTEXT (Express Tools arc-aligned text) — read-only, mapped onto
// DRW_Text as a 2D approximation (text at the arc mid-point).  applyArcApprox-
// imation() derives basePoint / angle / height from the arc parameters once all
// group codes are read.
bool dxfRW::processArcAlignedText() {
    DRW_DBG("dxfRW::processArcAlignedText");
    int code;
    DRW_ArcAlignedText txt;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            txt.applyArcApproximation();
            iface->addText(txt);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!txt.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processHatch() {
    DRW_DBG("dxfRW::processHatch");
    int code;
    DRW_Hatch hatch;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!hatch.validateDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addHatch(&hatch);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!hatch.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
}
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processSpline() {
    DRW_DBG("dxfRW::processSpline");
    int code;
    DRW_Spline sp;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!sp.validateDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addSpline(&sp);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!sp.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
}
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processHelix() {
    DRW_DBG("dxfRW::processHelix");
    int code;
    DRW_Helix helix;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!helix.validateDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addHelix(&helix);
            return true;
        }

        if (!helix.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processImage() {
    DRW_DBG("dxfRW::processImage");
    int code;
    DRW_Image img;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addImage(&img);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!img.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

// MULTILEADER DXF read.  Captures the entity-level scalar fields via
// DRW_MLeader::parseCode.  Nested CONTEXT_DATA{} / LEADER{} / LEADER_LINE{}
// blocks use control-flow group codes (300/302/304 open + 301/303/305 close)
// — Phase 8 keeps the body capture minimal; Phase 9 / follow-up will wire
// the full nested-block state machine.
bool dxfRW::processMultiLeader() {
    DRW_DBG("dxfRW::processMultiLeader");
    int code;
    DRW_MLeader e;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!e.isDxfContextClosed())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addMLeader(&e);
            return true;
        }
        if (!e.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processWipeout() {
    DRW_DBG("dxfRW::processWipeout");
    int code;
    DRW_Wipeout wipeout;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!wipeout.hasValidBoundary()) {
                return setError(DRW::BAD_CODE_PARSED);
            }
            iface->addWipeout(&wipeout);
            return true;
        }

        if (!wipeout.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPointCloud() {
    DRW_DBG("dxfRW::processPointCloud");
    int code;
    DRW_PointCloud pc;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!pc.finalizeDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addPointCloud(&pc);
            return true;
        }

        if (!pc.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processPointCloudEx() {
    DRW_DBG("dxfRW::processPointCloudEx");
    int code;
    DRW_PointCloudEx pce;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!pce.finalizeDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addPointCloudEx(&pce);
            return true;
        }

        if (!pce.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processNavisworksModel() {
    DRW_DBG("dxfRW::processNavisworksModel");
    int code;
    DRW_NavisworksModel model;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!model.finalizeDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addNavisworksModel(&model);
            return true;
        }
        if (!model.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processSurface() {
    DRW_DBG("dxfRW::processSurface");
    int code;
    std::unique_ptr<DRW_Surface> surf;
    if (dxfKeywordEquals(nextentity, "PLANESURFACE")) {
        surf = std::make_unique<DRW_PlaneSurface>();
    } else if (dxfKeywordEquals(nextentity, "EXTRUDEDSURFACE")) {
        surf = std::make_unique<DRW_ExtrudedSurface>();
    } else if (dxfKeywordEquals(nextentity, "REVOLVEDSURFACE")) {
        surf = std::make_unique<DRW_RevolvedSurface>();
    } else if (dxfKeywordEquals(nextentity, "SWEPTSURFACE")) {
        surf = std::make_unique<DRW_SweptSurface>();
    } else if (dxfKeywordEquals(nextentity, "LOFTEDSURFACE")) {
        surf = std::make_unique<DRW_LoftedSurface>();
    } else if (dxfKeywordEquals(nextentity, "NURBSSURFACE")) {
        surf = std::make_unique<DRW_NurbsSurface>();
    } else {
        return setError(DRW::BAD_READ_ENTITIES);
    }

    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!surf->finalizeDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addSurface(surf.get());
            return true;
        }

        if (!surf->parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processModelerGeometry() {
    DRW_DBG("dxfRW::processModelerGeometry");
    int code;
    DRW::ETYPE type = DRW::UNKNOWN;
    if (dxfKeywordEquals(nextentity, "3DSOLID")) {
        type = DRW::E3DSOLID;
    } else if (dxfKeywordEquals(nextentity, "REGION")) {
        type = DRW::REGION;
    } else if (dxfKeywordEquals(nextentity, "BODY")) {
        type = DRW::BODY;
    } else {
        return setError(DRW::BAD_READ_ENTITIES);
    }

    DRW_ModelerGeometry geom(type);
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addModelerGeometry(geom);
            return true;
        }

        if (!geom.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

// MPOLYGON (AcDbMPolygon) DXF read.  Boundary loops, solid flag and pattern share
// HATCH's group codes; DRW_MPolygon::parseCode delegates those to DRW_Hatch and
// additionally captures the MPOLYGON-only fill-color / degenerate-count trailer.
// Delivered via addMPolygon (defaults to addHatch, so it renders as a hatch).
bool dxfRW::processMPolygon() {
    DRW_DBG("dxfRW::processMPolygon");
    int code;
    DRW_MPolygon poly;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!poly.validateDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addMPolygon(&poly);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!poly.parseCode(code, reader)) {
            return setError(DRW::BAD_CODE_PARSED);
}
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


bool dxfRW::processDimension() {
    DRW_DBG("dxfRW::processDimension");
    int code;
    DRW_Dimension dim;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            int type = dim.type & 0x0F;
        switch (type) {
            case 0: {
                DRW_DimLinear d(dim);
                iface->addDimLinear(&d);
                break;
            }
            case 1: {
                DRW_DimAligned d(dim);
                iface->addDimAlign(&d);
                break;
            }
            case 2: {
                DRW_DimAngular d(dim);
                iface->addDimAngular(&d);
                break;
            }
            case 3: {
                DRW_DimDiametric d(dim);
                iface->addDimDiametric(&d);
                break;
            }
            case 4: {
                DRW_DimRadial d(dim);
                iface->addDimRadial(&d);
                break;
            }
            case 5: {
                DRW_DimAngular3p d(dim);
                iface->addDimAngular3P(&d);
                break;
            }
            case 6: {
                DRW_DimOrdinate d(dim);
                iface->addDimOrdinate(&d);
                break; }
            }
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!dim.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processArcDimension() {
    DRW_DBG("dxfRW::processArcDimension");
    int code;
    DRW_DimArc d;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addDimArc(&d);
            return true;
        }
        if (!d.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

// LARGE_RADIAL_DIMENSION (AcDbRadialDimensionLarge) is a top-level entity token,
// NOT a DIMENSION subtype, so it gets its own reader.  Delivered via the existing
// addDimRadial callback (DRW_DimLargeRadial is-a DRW_DimRadial).
bool dxfRW::processLargeRadialDimension() {
    DRW_DBG("dxfRW::processLargeRadialDimension");
    int code;
    DRW_DimLargeRadial d;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            iface->addDimRadial(&d);
            return true;
        }
        if (!d.parseCode(code, reader))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

bool dxfRW::processLeader() {
    DRW_DBG("dxfRW::processLeader");
    int code;
    DRW_Leader leader;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (setEntityBoundary(code) == DxfEntityBoundary::Error)
                return setError(DRW::BAD_READ_ENTITIES);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (!leader.validateDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addLeader(&leader);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!leader.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
}
    }

    return setError(DRW::BAD_READ_ENTITIES);
}


/********* Objects Section *********/

bool dxfRW::processObjects() {
    DRW_DBG("dxfRW::processObjects\n");
    int code;
    if (!reader->readRec(&code)) {
        return setError(DRW::BAD_READ_OBJECTS); //first record in objects must be 0
    }

    const DxfEntityBoundary firstBoundary = setEntityBoundary(code);
    if (firstBoundary == DxfEntityBoundary::Error
        || firstBoundary == DxfEntityBoundary::EndBlock) {
        return setError(DRW::BAD_READ_OBJECTS);
    }

    while (true) {
        const DxfEntityBoundary boundary = classifyEntityBoundary();
        if (boundary == DxfEntityBoundary::EndSection)
            return true;
        if (boundary == DxfEntityBoundary::Error
            || boundary == DxfEntityBoundary::EndBlock) {
            return setError(DRW::BAD_READ_OBJECTS);
        }

        const std::string normalizedEntity = dxfSymbolNameKey(nextentity);
        bool processed {false};
        if (normalizedEntity == "ACDBDETAILVIEWSTYLE"
            || normalizedEntity == "DETAILVIEWSTYLE") {
            processed = processDetailViewStyle();
        }
        else if (normalizedEntity == "ACDBSECTIONVIEWSTYLE"
                 || normalizedEntity == "SECTIONVIEWSTYLE") {
            processed = processSectionViewStyle();
        }
        else if (normalizedEntity == "BREAKDATA") {
            processed = processBreakData();
        }
        else if (normalizedEntity == "BREAKPOINTREF") {
            processed = processBreakPointRef();
        }
        else if (normalizedEntity == "IMAGEDEF") {
            processed = processImageDef();
        }
        else if (normalizedEntity == "PDFDEFINITION"
                 || normalizedEntity == "ACDBPDFDEFINITION"
                 || normalizedEntity == "DGNDEFINITION"
                 || normalizedEntity == "ACDBDGNDEFINITION"
                 || normalizedEntity == "DWFDEFINITION"
                 || normalizedEntity == "ACDBDWFDEFINITION") {
            processed = processUnderlayDefinition();
        }
        else if (normalizedEntity == "PLOTSETTINGS") {
            processed = processPlotSettings();
        }
        else if (normalizedEntity == "GROUP") {
            processed = processGroup();
        }
        else if (normalizedEntity == "LIGHTLIST"
                 || normalizedEntity == "ACDBLIGHTLIST") {
            processed = processLightList();
        }
        else if (normalizedEntity == "DATALINK") {
            processed = processDataLink();
        }
        else if (normalizedEntity == "GEOMAPIMAGE"
                 || normalizedEntity == "ACDBGEOMAPIMAGE") {
            processed = processGeoMapImage();
        }
        else if (normalizedEntity == "LAYERFILTER"
                 || normalizedEntity == "LAYER_FILTER") {
            processed = processLayerFilter();
        }
        else if (normalizedEntity == "DICTIONARY") {
            processed = processDictionary();
        }
        else if (normalizedEntity == "SCALE") {
            processed = processScale();
        }
        else if (normalizedEntity == "MLINESTYLE") {
            processed = processMLineStyle();
        }
        else if (normalizedEntity == "DICTIONARYVAR") {
            processed = processDictionaryVar();
        }
        else if (normalizedEntity == "XRECORD") {
            processed = processXRecord();
        }
        else if (normalizedEntity == "ACDBDICTIONARYWDFLT"
                 || normalizedEntity == "DICTIONARYWDFLT") {
            processed = processDictionaryWithDefault();
        }
        else if (normalizedEntity == "RASTERVARIABLES"
                 || normalizedEntity == "ACDBRASTERVARIABLES") {
            processed = processRasterVariables();
        }
        else if (normalizedEntity == "FIELD"
                 || normalizedEntity == "ACDBFIELD") {
            processed = processField();
        }
        else if (normalizedEntity == "FIELDLIST"
                 || normalizedEntity == "ACDBFIELDLIST") {
            processed = processFieldList();
        }
        else if (normalizedEntity == "SUN"
                 || normalizedEntity == "ACDBSUN") {
            processed = processSun();
        }
        else if (normalizedEntity == "LAYOUT") {
            processed = processLayout();
        }
        else if (normalizedEntity == "WIPEOUTVARIABLES"
                 || normalizedEntity == "ACDBWIPEOUTVARIABLES") {
            processed = processWipeoutVariables();
        }
        else if (normalizedEntity == "MATERIAL") {
            processed = processMaterial();
        }
        else if (normalizedEntity == "DBCOLOR"
                 || normalizedEntity == "ACDBCOLOR") {
            processed = processDbColor();
        }
        else if (normalizedEntity == "GEODATA"
                 || normalizedEntity == "ACDBGEODATA") {
            processed = processGeoData();
        }
        else if (normalizedEntity == "INDEX"
                 || normalizedEntity == "ACDBINDEX") {
            processed = processIndex();
        }
        else if (normalizedEntity == "LAYER_INDEX"
                 || normalizedEntity == "LAYERINDEX"
                 || normalizedEntity == "ACDBLAYERINDEX") {
            processed = processLayerIndex();
        }
        else if (normalizedEntity == "SPATIAL_INDEX"
                 || normalizedEntity == "SPATIALINDEX"
                 || normalizedEntity == "ACDBSPATIALINDEX") {
            processed = processSpatialIndex();
        }
        else if (normalizedEntity == "IDBUFFER"
                 || normalizedEntity == "ACDBIDBUFFER") {
            processed = processIDBuffer();
        }
        else if (normalizedEntity == "VISUALSTYLE"
                 || normalizedEntity == "ACDBVISUALSTYLE"
                 || normalizedEntity == "ACDB_VISUALSTYLE_CLASS") {
            processed = processVisualStyle();
        }
        else if (normalizedEntity == "IMAGEDEF_REACTOR") {
            processed = processImageDefReactor();
        }
        else if (normalizedEntity == "SPATIAL_FILTER"
                 || normalizedEntity == "SPATIALFILTER") {
            processed = processSpatialFilter();
        }
        else if (normalizedEntity == "TABLESTYLE") {
            processed = processTableStyle();
        }
        else if (normalizedEntity == "MLEADERSTYLE") {
            processed = processMLeaderStyle();
        }
        else if (normalizedEntity == "SORTENTSTABLE") {
            processed = processSortEntsTable();
        }
        else if (normalizedEntity == "DIMASSOC") {
            processed = processDimAssoc();
        }
        else if (normalizedEntity == "PERSISTENTSUBENTITYMANAGER"
                 || normalizedEntity == "PERSUBENTMGR"
                 || normalizedEntity == "ACDBPERSSUBENTMANAGER"
                 || normalizedEntity.rfind("ACDBASSOC", 0) == 0
                 || normalizedEntity.rfind("ASSOC", 0) == 0) {
            processed = processAssociativeObject();
        }
        else if (DRW_DynamicBlockObject::isDynamicBlockRecName(normalizedEntity)) {
            processed = processDynamicBlockObject();
        }
        else if (normalizedEntity.rfind("ACSH_", 0) == 0
                 || normalizedEntity == "ACDBHISTORYITEM"
                 || normalizedEntity == "HISTORYNODE") {
            processed = processAcShHistoryObject();
        }
        else if (normalizedEntity == "EVALUATION_GRAPH"
                 || normalizedEntity == "EVALUATIONGRAPH"
                 || normalizedEntity == "ACDBEVALGRAPH"
                 || normalizedEntity == "ACAD_EVALUATION_GRAPH") {
            processed = processEvaluationGraph();
        }
        else if (normalizedEntity == "SOLIDBACKGROUND"
                 || normalizedEntity == "SOLID_BACKGROUND"
                 || normalizedEntity == "GRADIENTBACKGROUND"
                 || normalizedEntity == "GRADIENT_BACKGROUND"
                 || normalizedEntity == "GROUNDPLANEBACKGROUND"
                 || normalizedEntity == "GROUND_PLANE_BACKGROUND"
                 || normalizedEntity == "IMAGEBACKGROUND"
                 || normalizedEntity == "IMAGE_BACKGROUND"
                 || normalizedEntity == "IBLBACKGROUND"
                 || normalizedEntity == "IBL_BACKGROUND"
                 || normalizedEntity == "SKYLIGHTBACKGROUND"
                 || normalizedEntity == "SKYLIGHT_BACKGROUND") {
            processed = processBackground();
        }
        else if (normalizedEntity == "POINTCLOUDDEFINITION"
                 || normalizedEntity == "ACDBPOINTCLOUDDEF"
                 || normalizedEntity == "POINTCLOUDDEFINITIONEX"
                 || normalizedEntity == "ACDBPOINTCLOUDDEFEX"
                 || normalizedEntity == "POINTCLOUDDEFREACTOR"
                 || normalizedEntity == "ACDBPOINTCLOUDDEFREACTOR"
                 || normalizedEntity == "POINTCLOUDDEFREACTOREX"
                 || normalizedEntity == "ACDBPOINTCLOUDDEFREACTOREX") {
            processed = processPointCloudDef();
        }
        else if (normalizedEntity == "NAVISWORKSMODELDEF") {
            processed = processNavisworksModelDef();
        }
        else if (normalizedEntity == "POINTCLOUDCOLORMAP"
                 || normalizedEntity == "ACDBPOINTCLOUDCOLORMAP") {
            processed = processPointCloudColorMap();
        }
        else if (normalizedEntity == "SUNSTUDY"
                 || normalizedEntity == "ACDBSUNSTUDY") {
            processed = processSunStudy();
        }
        else if (normalizedEntity == "MOTIONPATH"
                 || normalizedEntity == "ACDBMOTIONPATH") {
            processed = processMotionPath();
        }
        else if (normalizedEntity == "CURVEPATH"
                 || normalizedEntity == "ACDBCURVEPATH") {
            processed = processCurvePath();
        }
        else if (normalizedEntity == "POINTPATH"
                 || normalizedEntity == "ACDBPOINTPATH") {
            processed = processPointPath();
        }
        else if (normalizedEntity == "TVDEVICEPROPERTIES"
                 || normalizedEntity == "ACDBTVDEVICEPROPERTIES") {
            processed = processTvDeviceProperties();
        }
        else if (normalizedEntity == "CSACDOCUMENTOPTIONS") {
            processed = processCsacDocumentOptions();
        }
        else if (normalizedEntity == "OBJECT_PTR"
                 || normalizedEntity == "OBJECTPTR"
                 || normalizedEntity == "ACDBOBJECTPTR") {
            processed = processObjectPtr();
        }
        else if (normalizedEntity == "PARTIAL_VIEWING_INDEX"
                 || normalizedEntity == "PARTIALVIEWINGINDEX"
                 || normalizedEntity == "ACDBPARTIALVIEWINGINDEX") {
            processed = processPartialViewingIndex();
        }
        else if (normalizedEntity == "RENDERSETTINGS"
                 || normalizedEntity == "ACDBRENDERSETTINGS"
                 || normalizedEntity == "RENDERGLOBAL"
                 || normalizedEntity == "ACDBRENDERGLOBAL"
                 || normalizedEntity == "RENDERENVIRONMENT"
                 || normalizedEntity == "ACDBRENDERENVIRONMENT"
                 || normalizedEntity == "RENDERENTRY"
                 || normalizedEntity == "ACDBRENDERENTRY"
                 || normalizedEntity == "RAPIDRTRENDERSETTINGS"
                 || normalizedEntity == "ACDBRAPIDRTRENDERSETTINGS"
                 || normalizedEntity == "MENTALRAYRENDERSETTINGS"
                 || normalizedEntity == "ACDBMENTALRAYRENDERSETTINGS") {
            processed = processRenderSettings();
        }
        else if (normalizedEntity == "SECTIONMANAGER"
                 || normalizedEntity == "ACDBSECTIONMANAGER"
                 || normalizedEntity == "SECTION_MANAGER"
                 || normalizedEntity == "SECTIONSETTINGS"
                 || normalizedEntity == "ACDBSECTIONSETTINGS"
                 || normalizedEntity == "SECTION_SETTINGS") {
            processed = processSection();
        }
        else if (normalizedEntity == "ACAD_PROXY_OBJECT") {
            processed = processProxyObject();
        }
        else {
            //Slice A1: never silently drop an unmodeled object — capture its
            //group codes verbatim for lossless re-emit instead of skipping.
            processed = processRawObject();
        }
        if (!processed)
            return error == DRW::BAD_NONE
                ? setError(DRW::BAD_READ_OBJECTS)
                : false;

        const DxfEntityBoundary nextBoundary = classifyEntityBoundary();
        if (nextBoundary == DxfEntityBoundary::EndSection)
            return true;
        if (nextBoundary == DxfEntityBoundary::Error
            || nextBoundary == DxfEntityBoundary::EndBlock) {
            return setError(DRW::BAD_READ_OBJECTS);
        }
    }

}

// ACDBASSOC* and PERSUBENTMGR records have many class-specific tails. Parse
// the common shell and preserve the complete DXF group for exact replay.
bool dxfRW::processAssociativeObject() {
    DRW_DBG("dxfRW::processAssociativeObject");
    int code;
    DRW_AssociativeObject data(nextentity);
    DRW_RawDxfObject raw;
    raw.name = nextentity;

    while (reader->readRec(&code)) {
        if (code == 0) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            if (boundary == DxfEntityBoundary::Error
                || boundary == DxfEntityBoundary::EndBlock) {
                return setError(DRW::BAD_READ_OBJECTS);
            }
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            iface->addAssociativeObject(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (code == 102) {
            if (!captureRawDxfApplicationGroup(raw, data))
                return setError(DRW::BAD_CODE_PARSED);
            continue;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// Dynamic-block DXF records are opaque shell objects in dwgTs.  Decode only
// the common object references and keep the complete group stream in the raw
// net; the individual AcDbEvalExpr/AcDbBlockElement layouts vary by class and
// are not reliable enough to infer from a producer-specific DXF sample.
bool dxfRW::processDynamicBlockObject() {
    DRW_DBG("dxfRW::processDynamicBlockObject");
    int code;
    DRW_DynamicBlockObject data(nextentity);
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            if (boundary == DxfEntityBoundary::Error
                || boundary == DxfEntityBoundary::EndBlock) {
                return setError(DRW::BAD_READ_OBJECTS);
            }
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            data.handle = raw.handle;
            data.parentHandle = raw.parentHandle;
            iface->addDynamicBlockObject(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureRawGroup(raw, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// ACSH history/action records follow the same DXF shell policy as dynamic
// blocks: common handles are useful to consumers, while the class-specific
// body remains authoritative in the raw record for round-trip preservation.
bool dxfRW::processAcShHistoryObject() {
    DRW_DBG("dxfRW::processAcShHistoryObject");
    int code;
    DRW_AcShHistoryObject data(nextentity);
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            if (boundary == DxfEntityBoundary::Error
                || boundary == DxfEntityBoundary::EndBlock) {
                return setError(DRW::BAD_READ_OBJECTS);
            }
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            data.handle = raw.handle;
            data.parentHandle = raw.parentHandle;
            iface->addAcShHistoryObject(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureRawGroup(raw, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processDetailViewStyle() {
    DRW_DBG("dxfRW::processDetailViewStyle");
    int code;
    DRW_DetailViewStyle style;
    //Also route to the raw net so this typed-read OBJECT survives DXF->DXF (it
    //has no typed DXF writer). Without it the object is dropped and any extension
    //dictionary it owns is orphaned (dangling 330). CLASS is registered via
    //dxfClassForRecordName(ACDB(DETAIL|SECTION)VIEWSTYLE).
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            iface->addDetailViewStyle(style);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, style))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processSectionViewStyle() {
    DRW_DBG("dxfRW::processSectionViewStyle");
    int code;
    DRW_SectionViewStyle style;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            iface->addSectionViewStyle(style);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, style))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processBreakData() {
    DRW_DBG("dxfRW::processBreakData");
    int code;
    DRW_BreakData data;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addBreakData(data);
            iface->addRawDxfObject(raw);  // else dropped on DXF->DXF (no typed writer)
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// MATERIAL (AcDbMaterial): structured DXF read of name/description (matching the
// DWG parser + dwgTs), plus full raw-net preservation for lossless DXF re-emit
// (the visual-property fields are not modeled, only round-tripped).
bool dxfRW::processMaterial() {
    DRW_DBG("dxfRW::processMaterial");
    int code;
    DRW_Material data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMaterial(data);
            iface->addRawDxfObject(raw);  // no typed writer: raw re-emits on DXF->DXF
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// DBCOLOR (AcDbColor): structured color/book fields plus raw-net preservation
// for any producer-specific groups not represented by DRW_DbColor.
bool dxfRW::processDbColor() {
    DRW_DBG("dxfRW::processDbColor");
    int code;
    DRW_DbColor data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addDbColor(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// GEODATA (AcDbGeoData): structured DXF read of the scalar geolocation fields,
// plus full raw-net preservation (the coordinate-mesh lists are round-tripped
// raw only — see DRW_GeoData::parseCode).
bool dxfRW::processGeoData() {
    DRW_DBG("dxfRW::processGeoData");
    int code;
    DRW_GeoData data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addGeoData(data);
            iface->addRawDxfObject(raw);  // no typed writer: raw re-emits on DXF->DXF
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// VISUALSTYLE (AcDbVisualStyle): structured DXF read of description + style type,
// plus full raw-net preservation (the per-property face/edge/display settings are
// round-tripped raw only — matches dwgTs's VISUALSTYLE decode depth).
bool dxfRW::processVisualStyle() {
    DRW_DBG("dxfRW::processVisualStyle");
    int code;
    DRW_VisualStyle data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addVisualStyle(data);
            iface->addRawDxfObject(raw);  // no typed writer: raw re-emits on DXF->DXF
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// IMAGEDEF_REACTOR (AcDbRasterImageDefReactor): structured DXF read of the
// class-version field + raw-net preservation for lossless DXF re-emit.
bool dxfRW::processImageDefReactor() {
    DRW_DBG("dxfRW::processImageDefReactor");
    int code;
    DRW_ImageDefinitionReactor data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addImageDefinitionReactor(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// SPATIAL_FILTER (AcDbSpatialFilter): structured DXF read of the clip boundary +
// planes + raw-net preservation.
bool dxfRW::processSpatialFilter() {
    DRW_DBG("dxfRW::processSpatialFilter");
    int code;
    DRW_SpatialFilter data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addSpatialFilter(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// TABLESTYLE (AcDbTableStyle): structured DXF read of the top-level fields +
// raw-net preservation (nested row/cell styles round-tripped raw only).
bool dxfRW::processTableStyle() {
    DRW_DBG("dxfRW::processTableStyle");
    int code;
    DRW_TableStyle data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addTableStyle(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// MLEADERSTYLE (AcDbMLeaderStyle): structured DXF read of the full scalar record
// + raw-net preservation.  Delivered via addMLeaderStyle (pointer callback).
bool dxfRW::processMLeaderStyle() {
    DRW_DBG("dxfRW::processMLeaderStyle");
    int code;
    DRW_MLeaderStyle data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addMLeaderStyle(&data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// SORTENTSTABLE (AcDbSortentsTable): structured DXF read of the draw-order map
// (block owner + entity/sort handle pairs) + raw-net preservation.
bool dxfRW::processSortEntsTable() {
    DRW_DBG("dxfRW::processSortEntsTable");
    int code;
    DRW_SortEntsTable data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!data.finalizeDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addSortEntsTable(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// DIMASSOC (AcDbDimAssoc): structured DXF read of the associative-dimension
// metadata (dimension handle, flags, osnap refs) + raw-net preservation.
bool dxfRW::processDimAssoc() {
    DRW_DBG("dxfRW::processDimAssoc");
    int code;
    DRW_DimensionAssociation data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addDimensionAssociation(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// EVALUATION_GRAPH (AcDbEvalGraph): preserve the repeated node/edge payload in
// typed form while retaining the raw record for fields outside the C++ model.
bool dxfRW::processEvaluationGraph() {
    DRW_DBG("dxfRW::processEvaluationGraph");
    int code;
    DRW_EvaluationGraph data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!data.finalizeDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addEvaluationGraph(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// AcDb*Background OBJECTS (solid/gradient/ground-plane/image/IBL/skylight):
// structured DXF read into DRW_Background (kind set from the entity name) + raw-
// net preservation.  DWG stays raw (no DWG parser).  Not rendered by LibreCAD.
bool dxfRW::processBackground() {
    DRW_DBG("dxfRW::processBackground");
    int code;
    DRW_Background data;
    if (dxfKeywordEquals(nextentity, "GRADIENTBACKGROUND")
        || dxfKeywordEquals(nextentity, "GRADIENT_BACKGROUND"))
        data.m_kind = DRW_Background::Gradient;
    else if (dxfKeywordEquals(nextentity, "GROUNDPLANEBACKGROUND")
             || dxfKeywordEquals(nextentity, "GROUND_PLANE_BACKGROUND"))
        data.m_kind = DRW_Background::GroundPlane;
    else if (dxfKeywordEquals(nextentity, "IMAGEBACKGROUND")
             || dxfKeywordEquals(nextentity, "IMAGE_BACKGROUND"))
        data.m_kind = DRW_Background::Image;
    else if (dxfKeywordEquals(nextentity, "IBLBACKGROUND")
             || dxfKeywordEquals(nextentity, "IBL_BACKGROUND"))
        data.m_kind = DRW_Background::Ibl;
    else if (dxfKeywordEquals(nextentity, "SKYLIGHTBACKGROUND")
             || dxfKeywordEquals(nextentity, "SKYLIGHT_BACKGROUND"))
        data.m_kind = DRW_Background::Skylight;
    else
        data.m_kind = DRW_Background::Solid;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addBackground(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// AcDbPointCloudDef / ...DefEx and reactors: structured DXF read into
// DRW_PointCloudDef (kind from the entity name) + raw-net preservation.
bool dxfRW::processPointCloudDef() {
    DRW_DBG("dxfRW::processPointCloudDef");
    int code;
    DRW_PointCloudDef data;
    if (dxfKeywordEquals(nextentity, "POINTCLOUDDEFINITIONEX")
        || dxfKeywordEquals(nextentity, "ACDBPOINTCLOUDDEFEX"))
        data.m_kind = DRW_PointCloudDef::DefinitionEx;
    else if (dxfKeywordEquals(nextentity, "POINTCLOUDDEFREACTOREX")
             || dxfKeywordEquals(nextentity, "ACDBPOINTCLOUDDEFREACTOREX"))
        data.m_kind = DRW_PointCloudDef::ReactorEx;
    else if (dxfKeywordEquals(nextentity, "POINTCLOUDDEFREACTOR")
             || dxfKeywordEquals(nextentity, "ACDBPOINTCLOUDDEFREACTOR"))
        data.m_kind = DRW_PointCloudDef::Reactor;
    else
        data.m_kind = DRW_PointCloudDef::Definition;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (data.m_classVersion < 0
                || data.m_classVersion > DRW_PointCloudDef::kMaxClassVersion
                || ((data.m_kind == DRW_PointCloudDef::Definition
                     || data.m_kind == DRW_PointCloudDef::DefinitionEx)
                    && data.m_pointCount > DRW_PointCloudDef::kMaxPointCount))
                return setError(DRW::BAD_CODE_PARSED);
            iface->addPointCloudDef(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processNavisworksModelDef() {
    DRW_DBG("dxfRW::processNavisworksModelDef");
    int code;
    DRW_NavisworksModelDef data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addNavisworksModelDef(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processPointCloudColorMap() {
    DRW_DBG("dxfRW::processPointCloudColorMap");
    int code;
    DRW_PointCloudColorMap data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (data.m_colorRampCount != data.m_colorRamps.size()
                || data.m_classificationColorRampCount
                       != data.m_classificationColorRamps.size())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addPointCloudColorMap(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// SUNSTUDY (AcDbSunStudy): structured DXF read of the scalar study config +
// raw-net preservation (date/hour lists left raw).
bool dxfRW::processSunStudy() {
    DRW_DBG("dxfRW::processSunStudy");
    int code;
    DRW_SunStudy data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addSunStudy(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// MOTIONPATH (AcDbMotionPath): positional DXF fields plus raw-net
// preservation.  The record name is accepted in both forms emitted by
// current producers.
bool dxfRW::processMotionPath() {
    DRW_DBG("dxfRW::processMotionPath");
    int code;
    DRW_MotionPath data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addMotionPath(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processCurvePath() {
    DRW_DBG("dxfRW::processCurvePath");
    int code;
    DRW_CurvePath data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addCurvePath(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processPointPath() {
    DRW_DBG("dxfRW::processPointPath");
    int code;
    DRW_PointPath data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addPointPath(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processObjectPtr() {
    DRW_DBG("dxfRW::processObjectPtr");
    int code;
    DRW_ObjectPtr data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addObjectPtr(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processPartialViewingIndex() {
    DRW_DBG("dxfRW::processPartialViewingIndex");
    int code;
    DRW_PartialViewingIndex data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!data.finalizeDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addPartialViewingIndex(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// AcDbRenderSettings family (settings/global/environment/entry/mentalray/
// rapidrt): positional DXF capture into DRW_RenderSettings (kind from the entity
// name; vectors finalized into named fields) + raw-net preservation.
bool dxfRW::processRenderSettings() {
    DRW_DBG("dxfRW::processRenderSettings");
    int code;
    DRW_RenderSettings data;
    if (dxfKeywordEquals(nextentity, "RENDERGLOBAL")
        || dxfKeywordEquals(nextentity, "ACDBRENDERGLOBAL"))
        data.m_kind = DRW_RenderSettings::Global;
    else if (dxfKeywordEquals(nextentity, "RENDERENVIRONMENT")
             || dxfKeywordEquals(nextentity, "ACDBRENDERENVIRONMENT"))
        data.m_kind = DRW_RenderSettings::Environment;
    else if (dxfKeywordEquals(nextentity, "RENDERENTRY")
             || dxfKeywordEquals(nextentity, "ACDBRENDERENTRY"))
        data.m_kind = DRW_RenderSettings::Entry;
    else if (dxfKeywordEquals(nextentity, "RAPIDRTRENDERSETTINGS")
             || dxfKeywordEquals(nextentity, "ACDBRAPIDRTRENDERSETTINGS"))
        data.m_kind = DRW_RenderSettings::RapidRT;
    else if (dxfKeywordEquals(nextentity, "MENTALRAYRENDERSETTINGS")
             || dxfKeywordEquals(nextentity, "ACDBMENTALRAYRENDERSETTINGS"))
        data.m_kind = DRW_RenderSettings::MentalRay;
    else data.m_kind = DRW_RenderSettings::Settings;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            data.finalize();
            iface->addRenderSettings(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

// SECTION_MANAGER / SECTION_SETTINGS: structured DXF read into DRW_Section
// (kind from the entity name) + raw-net preservation.
bool dxfRW::processSection() {
    DRW_DBG("dxfRW::processSection");
    int code;
    DRW_Section data;
    if (dxfKeywordEquals(nextentity, "SECTIONSETTINGS")
        || dxfKeywordEquals(nextentity, "ACDBSECTIONSETTINGS")
        || dxfKeywordEquals(nextentity, "SECTION_SETTINGS"))
        data.m_kind = DRW_Section::Settings;
    else
        data.m_kind = DRW_Section::Manager;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    const auto validSection = [&data]() {
        if (data.m_kind == DRW_Section::Manager) {
            return data.m_sectionCount >= 0
                && data.m_sectionCount <= DRW_Section::kMaxSectionCount
                && data.m_sectionHandles.size()
                    == static_cast<std::size_t>(data.m_sectionCount);
        }
        // Legacy flat SECTIONSETTINGS records reuse the same scalar codes
        // without nested type markers, so their populated typeCount field is
        // ambiguous and must remain tolerant. A negative count is unambiguously
        // malformed even when no nested records were present.
        if (data.m_types.empty())
            return data.m_typeCount >= 0;
        if (data.m_typeCount < 0
            || data.m_typeCount > DRW_Section::kMaxSectionTypeCount
            || data.m_types.size()
                != static_cast<std::size_t>(data.m_typeCount))
            return false;
        for (const DRW_SectionTypeSettings& type : data.m_types) {
            if (type.m_numSources < 0
                || type.m_numSources > DRW_Section::kMaxSectionSourceCount
                || type.m_sourceHandles.size()
                    != static_cast<std::size_t>(type.m_numSources)
                || type.m_numGeometrySettings < 0
                || type.m_numGeometrySettings
                    > DRW_Section::kMaxSectionGeometryCount
                || type.m_geometry.size()
                    != static_cast<std::size_t>(type.m_numGeometrySettings))
                return false;
        }
        return true;
    };
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!validSection())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addSection(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processBreakPointRef() {
    DRW_DBG("dxfRW::processBreakPointRef");
    int code;
    DRW_BreakPointRef ref;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addBreakPointRef(ref);
            iface->addRawDxfObject(raw);  // else dropped on DXF->DXF (no typed writer)
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, ref))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processUnderlayDefinition() {
    DRW_DBG("dxfRW::processUnderlayDefinition");
    int code;
    DRW_UnderlayDefinition definition;
    if (dxfKeywordEquals(nextentity, "DGNDEFINITION")
        || dxfKeywordEquals(nextentity, "ACDBDGNDEFINITION"))
        definition.kind = DRW_UnderlayDefinition::DGN;
    else if (dxfKeywordEquals(nextentity, "DWFDEFINITION")
             || dxfKeywordEquals(nextentity, "ACDBDWFDEFINITION"))
        definition.kind = DRW_UnderlayDefinition::DWF;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->linkUnderlay(&definition);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, definition))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processImageDef() {
    DRW_DBG("dxfRW::processImageDef");
    int code;
    DRW_ImageDef img;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->linkImage(&img);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!img.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processPlotSettings() {
    DRW_DBG("dxfRW::processPlotSettings");
    int code;
    DRW_PlotSettings ps;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addPlotSettings(&ps);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!ps.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processGroup() {
    DRW_DBG("dxfRW::processGroup");
    int code;
    DRW_Group group;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addGroup(group);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!group.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processLightList() {
    DRW_DBG("dxfRW::processLightList");
    int code;
    DRW_LightList lightList;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addLightList(lightList);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, lightList))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processLayerFilter() {
    DRW_DBG("dxfRW::processLayerFilter");
    int code;
    DRW_LayerFilter layerFilter;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addLayerFilter(layerFilter);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, layerFilter))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processDataLink() {
    DRW_DBG("dxfRW::processDataLink");
    int code;
    DRW_DataLink dataLink;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addDataLink(dataLink);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, dataLink))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processIndex() {
    DRW_DBG("dxfRW::processIndex");
    int code;
    DRW_Index index;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addIndex(index);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, index))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processLayerIndex() {
    DRW_DBG("dxfRW::processLayerIndex");
    int code;
    DRW_LayerIndex index;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!index.finalizeDxf())
                return setError(DRW::BAD_CODE_PARSED);
            iface->addLayerIndex(index);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, index))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processSpatialIndex() {
    DRW_DBG("dxfRW::processSpatialIndex");
    int code;
    DRW_SpatialIndex index;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addSpatialIndex(index);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, index))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processIDBuffer() {
    DRW_DBG("dxfRW::processIDBuffer");
    int code;
    DRW_IDBuffer data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addIDBuffer(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, data))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processGeoMapImage() {
    DRW_DBG("dxfRW::processGeoMapImage");
    int code;
    DRW_GeoMapImage geoMapImage;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addGeoMapImage(geoMapImage);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (!captureAndParseRawDxfGroup(raw, code, geoMapImage))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processDictionary() {
    DRW_DBG("dxfRW::processDictionary");
    int code;
    DRW_Dictionary dict;
    //Route NON-ROOT named dictionaries through the raw net so they round-trip
    //DXF->DXF (re-attached to the regenerated root via setRootDictEntries). The
    //source root dict (330==0) is NOT routed — the codec regenerates it at fixed
    //handle C; re-emitting it would duplicate the NamedObjectsDictionary.
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!dict.hasCompleteDxfEntries())
                return setError(DRW::BAD_CODE_PARSED);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addDictionary(dict);
            //Skip the root (330==0) and the fixed root/group handles C/D — the
            //codec always regenerates those; routing them would duplicate the
            //NamedObjectsDictionary / ACAD_GROUP dict.
            if (raw.parentHandle != 0 && raw.handle != 0xCu && raw.handle != 0xDu)
                iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!captureAndParseRawDxfGroup(raw, code, dict)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processScale() {
    DRW_DBG("dxfRW::processScale");
    int code;
    DRW_Scale scale;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addScale(scale);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!captureAndParseRawDxfGroup(raw, code, scale)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processMLineStyle() {
    DRW_DBG("dxfRW::processMLineStyle");
    int code;
    DRW_MLineStyle style;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addMLineStyle(style);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!captureAndParseRawDxfGroup(raw, code, style)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processDictionaryVar() {
    DRW_DBG("dxfRW::processDictionaryVar");
    int code;
    DRW_DictionaryVar var;
    DRW_RawDxfObject raw;       //data-only type: also preserved for DXF re-emit
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addDictionaryVar(var);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!captureAndParseRawDxfGroup(raw, code, var)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processXRecord() {
    DRW_DBG("dxfRW::processXRecord");
    int code;
    DRW_XRecord record;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            iface->addXRecord(record);
            iface->addRawDxfObject(raw);
            return true;
        }

        if (!captureAndParseRawDxfGroup(raw, code, record))
            return setError(DRW::BAD_CODE_PARSED);
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processDictionaryWithDefault() {
    DRW_DBG("dxfRW::processDictionaryWithDefault");
    int code;
    DRW_DictionaryWithDefault dict;
    //Same as processDictionary: route non-root WDFLT dicts (e.g. ACAD_PLOTSTYLENAME)
    //through the raw net; its 340 default points at a raw-net-preserved placeholder.
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!dict.hasCompleteDxfPayload())
                return setError(DRW::BAD_CODE_PARSED);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addDictionaryWithDefault(dict);
            if (raw.parentHandle != 0 && raw.handle != 0xCu && raw.handle != 0xDu)
                iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!captureAndParseRawDxfGroup(raw, code, dict)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

// Data-only OBJECTS (no inter-object handle refs beyond base-class 5/330) are
// ALSO captured into the raw-passthrough net so the DXF writer re-emits their
// bodies verbatim on a DXF->DXF round-trip. The typed object still populates
// LC_DwgAdvancedMetadata for the DWG write path; the raw net is DXF-write-only
// (the DWG path ignores it), so there is no double-emit. The dictionary/handle
// "spine" types (DICTIONARY/GROUP/LAYOUT/ACDBDICTIONARYWDFLT) are deliberately
// NOT routed here — verbatim re-emit of their handle graph would corrupt the
// regenerated dictionary tree; those await typed DXF writers.
bool dxfRW::processRasterVariables() {
    DRW_DBG("dxfRW::processRasterVariables");
    int code;
    DRW_RasterVariables rv;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addRasterVariables(rv);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!captureAndParseRawDxfGroup(raw, code, rv)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

// FIELD / FIELDLIST (AcDbField / AcDbFieldList).  The DWG read path already
// decodes these (dwgReader OBJECTS dispatch -> DRW_Field / DRW_FieldList ->
// addField / addFieldList); this adds the matching DXF read.  The group-code
// layout below was verified against an ODA-converted FIELD-rich DWG
// (blocks_and_tables_-_imperial.dwg -> DXF, ODA File Converter 27.1.0):
//   1  evaluatorId          90  child-field count       360 child-field handle
//   2  fieldCode            97  object-id count         331 object-id handle
//   3  fieldCode overflow   91  evaluation option flags 300 evaluation error msg
//   92/94/95/96 flags       301 value string           98  value-string length
// After the field-level scalars, per-child-value records begin at code 6.  The
// FIELD's own typed value begins at code 7 "ACFD_FIELD_VALUE".  Both use the
// same value grammar and end at code 304 "ACVALUE_END".  Routing a group while
// a value is active prevents value-local codes from clobbering field scalars.
bool dxfRW::processField() {
    DRW_DBG("dxfRW::processField");
    int code;
    DRW_Field field;
    DRW_RawDxfObject raw;
    raw.name = nextentity;

    bool inSubclass = false;      // set once the AcDbField subclass marker is seen
    bool childOpen = false;
    bool primaryOpen = false;
    bool primarySeen = false;
    bool activeHasType = false;
    bool activeHasDataSize = false;
    bool activeHasX = false;
    bool activeHasY = false;
    bool activeHasZ = false;
    DRW_CadValue* activeValue = nullptr;
    int childHandleCount = -1;
    int objectHandleCount = -1;
    int childValueCount = -1;
    DRW_Field::ChildValue child;

    const auto resetActiveValue = [&]() {
        activeHasType = false;
        activeHasDataSize = false;
        activeHasX = false;
        activeHasY = false;
        activeHasZ = false;
    };
    const auto valueHasRequiredPayload = [&]() {
        if (activeValue == nullptr || !activeHasType
            || (activeValue->m_formatFlags & 3) != 0)
            return activeValue != nullptr && activeHasType;

        switch (activeValue->m_dataType) {
        case 0:
        case 1:
            return activeValue->m_value.type() == DRW_Variant::INTEGER;
        case 2:
            return activeValue->m_value.type() == DRW_Variant::DOUBLE;
        case 4:
            return activeValue->m_value.type() == DRW_Variant::STRING;
        case 8:
            return activeHasDataSize
                && activeValue->m_dataSize == activeValue->m_rawData.size();
        case 16:
            return activeValue->m_value.type() == DRW_Variant::COORD
                && activeHasX && activeHasY
                && (activeValue->m_dataSize == 0
                    || activeValue->m_dataSize == 16);
        case 32:
            return activeValue->m_value.type() == DRW_Variant::COORD
                && activeHasX && activeHasY && activeHasZ
                && (activeValue->m_dataSize == 0
                    || activeValue->m_dataSize == 24);
        case 64:
            return activeValue->m_value.type() == DRW_Variant::INTEGER;
        default:
            return false;
        }
    };
    const auto closeValue = [&]() {
        if (!valueHasRequiredPayload())
            return false;
        if (activeValue->m_dataType == 8)
            activeValue->m_value.addBinary(310, activeValue->m_rawData);
        if (primaryOpen) {
            primaryOpen = false;
        } else {
            if (!childOpen || field.m_childValues.size() >= DRW_Field::kMaxItems)
                return false;
            field.m_childValues.push_back(child);
            childOpen = false;
        }
        activeValue = nullptr;
        resetActiveValue();
        return true;
    };

    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (!inSubclass || activeValue != nullptr || field.handle == 0
                || childHandleCount < 0 || objectHandleCount < 0
                || childValueCount < 0
                || static_cast<std::size_t>(childHandleCount)
                    != field.m_childHandles.size()
                || static_cast<std::size_t>(objectHandleCount)
                    != field.m_objectHandles.size()
                || static_cast<std::size_t>(childValueCount)
                    != field.m_childValues.size()
                || !validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            field.setDwgCommonObjectState(
                static_cast<std::int32_t>(field.reactorHandles.size()),
                field.xDictHandle != 0 ? 1 : 0, false);
            iface->addField(field);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (code == 102) {
            if (!captureRawDxfApplicationGroup(raw, field))
                return setError(DRW::BAD_CODE_PARSED);
            continue;
        }
        if (!captureRawGroup(raw, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);

        if (code >= 1000 && code <= 1071) {
            if (!field.parseCode(code, reader))
                return setError(DRW::BAD_CODE_PARSED);
            continue;
        }

        if (100 == code) {
            if ("AcDbField" == reader->getString())
                inSubclass = true;
            continue;
        }
        if (!inSubclass) {
            // Common preamble: handle (5) and owner (330).
            if (5 == code)        field.handle = reader->getHandleString();
            else if (330 == code) field.parentHandle = reader->getHandleString();
            continue;
        }

        if (activeValue != nullptr) {
            switch (code) {
            case 1:
                if (activeValue->m_dataType != 4
                    || activeValue->m_value.type() != DRW_Variant::INVALID)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_value.addString(1, reader->getUtf8String());
                break;
            case 2:
                if (activeValue->m_dataType != 4
                    || activeValue->m_value.type() != DRW_Variant::STRING)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_value.addString(
                    1, UTF8STRING(activeValue->m_value.c_str())
                        + reader->getUtf8String());
                break;
            case 11:
                if (activeValue->m_dataType != 16
                    && activeValue->m_dataType != 32)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_value.addCoord(11, DRW_Coord(reader->getDouble(), 0.0, 0.0));
                activeHasX = true;
                break;
            case 21:
                if ((activeValue->m_dataType != 16 && activeValue->m_dataType != 32)
                    || !activeHasX || activeValue->m_value.coord() == nullptr)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_value.setCoordY(reader->getDouble());
                activeHasY = true;
                break;
            case 31:
                if (activeValue->m_dataType != 32 || !activeHasX || !activeHasY
                    || activeValue->m_value.coord() == nullptr)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_value.setCoordZ(reader->getDouble());
                activeHasZ = true;
                break;
            case 90:
                if (activeHasType)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_dataType = reader->getInt32();
                activeHasType = true;
                break;
            case 91:
                if (activeValue->m_dataType != 0 && activeValue->m_dataType != 1)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_value.addInt(91, reader->getInt32());
                break;
            case 92:
                if ((activeValue->m_dataType != 8
                     && activeValue->m_dataType != 16
                     && activeValue->m_dataType != 32)
                    || activeHasDataSize)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_dataSize = static_cast<std::uint32_t>(reader->getInt32());
                if (activeValue->m_dataSize > DRW::kMaxDxfBinaryPayloadBytes)
                    return setError(DRW::BAD_CODE_PARSED);
                activeHasDataSize = true;
                break;
            case 93:
                if (activeHasType)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_formatFlags = reader->getInt32();
                break;
            case 94:
                activeValue->m_unitType = reader->getInt32();
                break;
            case 140:
                if (activeValue->m_dataType != 2)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_value.addDouble(140, reader->getDouble());
                break;
            case 300:
                activeValue->m_formatString = reader->getUtf8String();
                break;
            case 302:
                activeValue->m_valueString = reader->getUtf8String();
                break;
            case 304:
                if (reader->getUtf8String() != "ACVALUE_END" || !closeValue())
                    return setError(DRW::BAD_CODE_PARSED);
                break;
            case 310:
                if (activeValue->m_dataType != 8
                    || !appendDxfHexChunk(reader->getString(), activeValue->m_rawData))
                    return setError(DRW::BAD_CODE_PARSED);
                break;
            case 330:
                if (activeValue->m_dataType != 64)
                    return setError(DRW::BAD_CODE_PARSED);
                activeValue->m_handle = reader->getHandleString();
                activeValue->m_value.addInt(330, activeValue->m_handle);
                break;
            default:
                return setError(DRW::BAD_CODE_PARSED);
            }
            continue;
        }

        switch (code) {
        case 1:   field.m_evaluatorId = reader->getUtf8String(); break;
        case 2:   field.m_fieldCode = reader->getUtf8String(); break;
        case 3:   field.m_fieldCode += reader->getUtf8String(); break;
        case 4:   field.m_formatString = reader->getUtf8String(); break;
        case 6:
            if (primarySeen || childValueCount < 0
                || field.m_childValues.size()
                    >= static_cast<std::size_t>(childValueCount))
                return setError(DRW::BAD_CODE_PARSED);
            child = DRW_Field::ChildValue();
            child.m_key = reader->getUtf8String();
            childOpen = true;
            activeValue = &child.m_value;
            resetActiveValue();
            break;
        case 7:
            if (primarySeen || childValueCount < 0
                || field.m_childValues.size()
                    != static_cast<std::size_t>(childValueCount)
                || reader->getUtf8String() != "ACFD_FIELD_VALUE")
                return setError(DRW::BAD_CODE_PARSED);
            field.m_value = DRW_CadValue();
            activeValue = &field.m_value;
            primaryOpen = true;
            primarySeen = true;
            resetActiveValue();
            break;
        case 90:
            if (childHandleCount >= 0)
                return setError(DRW::BAD_CODE_PARSED);
            childHandleCount = reader->getInt32();
            if (childHandleCount < 0
                || childHandleCount > static_cast<int>(DRW_Field::kMaxItems))
                return setError(DRW::BAD_CODE_PARSED);
            break;
        case 93:
            if (childValueCount >= 0)
                return setError(DRW::BAD_CODE_PARSED);
            childValueCount = reader->getInt32();
            if (childValueCount < 0
                || childValueCount > static_cast<int>(DRW_Field::kMaxItems))
                return setError(DRW::BAD_CODE_PARSED);
            break;
        case 97:
            if (objectHandleCount >= 0)
                return setError(DRW::BAD_CODE_PARSED);
            objectHandleCount = reader->getInt32();
            if (objectHandleCount < 0
                || objectHandleCount > static_cast<int>(DRW_Field::kMaxItems))
                return setError(DRW::BAD_CODE_PARSED);
            break;
        case 91:  field.m_evaluationOptionFlags = reader->getInt32(); break;
        case 92:  field.m_filingOptionFlags = reader->getInt32(); break;
        case 94:  field.m_fieldStateFlags = reader->getInt32(); break;
        case 95:  field.m_evaluationStatusFlags = reader->getInt32(); break;
        case 96:  field.m_evaluationErrorCode = reader->getInt32(); break;
        case 300: field.m_evaluationErrorMessage = reader->getUtf8String(); break;
        case 301: field.m_valueString = reader->getUtf8String(); break;
        case 98:  field.m_valueStringLength = reader->getInt32(); break;
        case 360: {
            const std::uint32_t h = reader->getHandleString();
            if (h == 0 || field.m_childHandles.size() >= DRW_Field::kMaxItems)
                return setError(DRW::BAD_CODE_PARSED);
            field.m_childHandles.push_back(h);
            break;
        }
        case 331: {
            const std::uint32_t h = reader->getHandleString();
            if (h == 0 || field.m_objectHandles.size() >= DRW_Field::kMaxItems)
                return setError(DRW::BAD_CODE_PARSED);
            field.m_objectHandles.push_back(h);
            break;
        }
        default: break;
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

// FIELDLIST (AcDbIdSet / AcDbFieldList): num_fields (90) + an "unknown" bool
// (290) + a soft-pointer per field. The canonical body is delimited by the
// two subclass markers so preamble owner group 330 never becomes a member.
bool dxfRW::processFieldList() {
    DRW_DBG("dxfRW::processFieldList");
    int code;
    DRW_FieldList list;
    DRW_RawDxfObject raw;
    raw.name = nextentity;

    enum class FieldListState {
        Preamble,
        IdSet,
        Complete
    };
    FieldListState state = FieldListState::Preamble;
    bool sawHandle = false;
    bool sawOwner = false;
    bool sawAcDbObject = false;
    bool sawApplicationData = false;
    bool sawCount = false;
    bool sawUnknown = false;
    int fieldCount = -1;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            if (state != FieldListState::Complete || !sawHandle
                || !sawCount || !sawUnknown || list.handle == 0 || fieldCount < 0
                || static_cast<std::size_t>(fieldCount)
                    != list.m_fieldHandles.size()
                || !validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            list.setDwgCommonObjectState(
                static_cast<std::int32_t>(list.reactorHandles.size()),
                list.xDictHandle != 0 ? 1 : 0, false);
            iface->addFieldList(list);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (code == 102) {
            if (state != FieldListState::Preamble)
                return setError(DRW::BAD_CODE_PARSED);
            if (!captureRawDxfApplicationGroup(raw, list))
                return setError(DRW::BAD_CODE_PARSED);
            sawApplicationData = true;
            continue;
        }
        if (!captureRawGroup(raw, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);

        if (code >= 1000 && code <= 1071) {
            if (state != FieldListState::Complete)
                return setError(DRW::BAD_CODE_PARSED);
            if (!list.parseCode(code, reader))
                return setError(DRW::BAD_CODE_PARSED);
            continue;
        }

        if (100 == code) {
            const std::string sub = reader->getString();
            if (sub == "AcDbObject" && state == FieldListState::Preamble
                && !sawAcDbObject && !sawApplicationData) {
                sawAcDbObject = true;
                continue;
            }
            if (sub == "AcDbIdSet" && state == FieldListState::Preamble) {
                state = FieldListState::IdSet;
                continue;
            }
            if (sub == "AcDbFieldList" && state == FieldListState::IdSet
                && sawCount && sawUnknown
                && static_cast<std::size_t>(fieldCount)
                       == list.m_fieldHandles.size()) {
                state = FieldListState::Complete;
                continue;
            }
            return setError(DRW::BAD_CODE_PARSED);
        }

        if (state == FieldListState::Complete)
            return setError(DRW::BAD_CODE_PARSED);

        if (state == FieldListState::Preamble) {
            if (sawApplicationData)
                return setError(DRW::BAD_CODE_PARSED);
            switch (code) {
            case 5:
                if (sawHandle)
                    return setError(DRW::BAD_CODE_PARSED);
                list.handle = reader->getHandleString();
                sawHandle = true;
                break;
            case 330:
                if (sawOwner)
                    return setError(DRW::BAD_CODE_PARSED);
                list.parentHandle = reader->getHandleString();
                sawOwner = true;
                break;
            default:
                return setError(DRW::BAD_CODE_PARSED);
            }
            continue;
        }

        switch (code) {
        case 90:
            if (sawCount || sawUnknown || !list.m_fieldHandles.empty())
                return setError(DRW::BAD_CODE_PARSED);
            fieldCount = reader->getInt32();
            if (fieldCount < 0
                || fieldCount > static_cast<int>(DRW_Field::kMaxItems))
                return setError(DRW::BAD_CODE_PARSED);
            sawCount = true;
            break;
        case 290:
            if (!sawCount || sawUnknown || !list.m_fieldHandles.empty())
                return setError(DRW::BAD_CODE_PARSED);
            list.m_unknown = reader->getInt32();
            if (list.m_unknown != 0 && list.m_unknown != 1)
                return setError(DRW::BAD_CODE_PARSED);
            sawUnknown = true;
            break;
        case 330: {
            const std::uint32_t h = reader->getHandleString();
            if (!sawCount || !sawUnknown || fieldCount < 0
                || list.m_fieldHandles.size() >= DRW_Field::kMaxItems
                || list.m_fieldHandles.size()
                       >= static_cast<std::size_t>(fieldCount))
                return setError(DRW::BAD_CODE_PARSED);
            list.m_fieldHandles.push_back(h);
            break;
        }
        default:
            return setError(DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processSun() {
    DRW_DBG("dxfRW::processSun");
    int code;
    DRW_Sun sun;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addSun(sun);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!captureAndParseRawDxfGroup(raw, code, sun)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processLayout() {
    DRW_DBG("dxfRW::processLayout");
    int code;
    DRW_Layout layout;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addLayout(layout);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!layout.parseCode(code, reader)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::processWipeoutVariables() {
    DRW_DBG("dxfRW::processWipeoutVariables");
    int code;
    DRW_WipeoutVariables wv;
    DRW_RawDxfObject raw;
    raw.name = nextentity;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            DRW_DBG(nextentity); DRW_DBG("\n");
            iface->addWipeoutVariables(wv);
            iface->addRawDxfObject(raw);
            return true;  //found new entity or ENDSEC, terminate
        }

        if (!captureAndParseRawDxfGroup(raw, code, wv)) {
            return setError( DRW::BAD_CODE_PARSED);
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

// TVDEVICEPROPERTIES is a producer-specific OBJECTS record rather than a
// standard DXF class.  Decode the positional fields used by dwgTs while
// retaining every group, so the source record remains authoritative on replay.
bool dxfRW::processTvDeviceProperties() {
    DRW_DBG("dxfRW::processTvDeviceProperties");
    int code;
    DRW_TvDeviceProperties data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;

    int seen90 = 0;
    int seen70 = 0;
    int seenBll = 0;
    int seen40 = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            data.handle = raw.handle;
            data.parentHandle = raw.parentHandle;
            iface->addTvDeviceProperties(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (code == 102) {
            if (!captureRawDxfApplicationGroup(raw, data))
                return setError(DRW::BAD_CODE_PARSED);
            continue;
        }

        if (!captureRawGroup(raw, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);
        switch (code) {
        case 90:
            if (seen90 == 0)
                data.flags = reader->getInt32();
            else if (seen90 == 1)
                data.useLutPalette = reader->getInt32();
            else if (seen90 == 2)
                data.blendingMode = reader->getInt32();
            ++seen90;
            break;
        case 70:
            if (seen70++ == 0)
                data.maxRegenThreads = reader->getInt32();
            break;
        case 160: case 161: case 162: case 163: case 164:
        case 165: case 166: case 167: case 168: case 169:
            if (seenBll == 0)
                data.alternateHighlight = reader->getInt64();
            else if (seenBll == 1)
                data.alternateHighlightColor = reader->getInt64();
            else if (seenBll == 2)
                data.geometryShaderUsage = reader->getInt64();
            ++seenBll;
            break;
        case 40:
            if (seen40 == 0)
                data.antialiasingLevel = reader->getDouble();
            else if (seen40 == 1)
                data.valueBd2 = reader->getDouble();
            ++seen40;
            break;
        default:
            break;
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

// CSACDOCUMENTOPTIONS has only two documented DXF body fields. Keep the
// complete source record in the raw carrier while decoding those fields.
bool dxfRW::processCsacDocumentOptions() {
    DRW_DBG("dxfRW::processCsacDocumentOptions");
    int code;
    DRW_CsacDocumentOptions data;
    DRW_RawDxfObject raw;
    raw.name = nextentity;

    int seen90 = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!acceptObjectBoundary(code))
                return setError(DRW::BAD_READ_OBJECTS);
            if (!validateCapturedRawDxfObject(raw, binFile))
                return setError(DRW::BAD_CODE_PARSED);
            data.handle = raw.handle;
            data.parentHandle = raw.parentHandle;
            iface->addCsacDocumentOptions(data);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (code == 102) {
            if (!captureRawDxfApplicationGroup(raw, data))
                return setError(DRW::BAD_CODE_PARSED);
            continue;
        }

        if (!captureRawGroup(raw, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);
        if (code == 90) {
            if (seen90 == 0)
                data.classVersion = static_cast<std::uint32_t>(
                    reader->getInt32());
            else if (seen90 == 1)
                data.flags = static_cast<std::uint32_t>(reader->getInt32());
            ++seen90;
        }
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

//Slice A1: lossless passthrough for an OBJECTS-section object libdxfrw does not
//model as a typed DXF object. Captures each group with its decoded typed value
//and, for ASCII input, the original value spelling so the object can be
//re-emitted without numeric normalization once the object-write spine (A2)
//consumes it.
namespace {
enum class RawValType { Str, Int16, Int32, Int64, Dbl, Bool };
//Mirror dxfReader::readRec's code->reader dispatch (intern/dxfreader.cpp) so a
//raw-captured group value is taken from the matching typed getter. readRec parses
//numeric codes into the typed members (intData/int64/doubleData) and leaves
//strData STALE, so getString() is wrong for them. The reader's public `type` is
//ALSO unreliable here: each numeric reader sets `type` then calls readString(&t)
//which resets it to STRING — hence we classify by code range, not reader->type.
RawValType classifyDxfCode(int code) {
    if (code < 10) return RawValType::Str;
    else if (code < 60) return RawValType::Dbl;
    else if (code < 80) return RawValType::Int16;
    else if (code < 90) return RawValType::Str;
    else if (code < 100) return RawValType::Int32;
    else if (code < 110) return RawValType::Str;
    else if (code < 150) return RawValType::Dbl;
    else if (code < 160) return RawValType::Str;
    else if (code < 170) return RawValType::Int64;
    else if (code < 180) return RawValType::Int16;
    else if (code < 210) return RawValType::Str;
    else if (code < 260) return RawValType::Dbl;
    else if (code < 290) return RawValType::Int16;
    else if (code < 300) return RawValType::Bool;
    else if (code < 310) return RawValType::Str;
    else if (code < 320) return RawValType::Str;            // readBinary -> string
    else if (code < 370) return RawValType::Str;            // incl. 330/340/350/360
    else if (code < 390) return RawValType::Int16;
    else if (code < 400) return RawValType::Str;
    else if (code < 410) return RawValType::Int16;
    else if (code < 420) return RawValType::Str;
    else if (code < 430) return RawValType::Int32;
    else if (code < 440) return RawValType::Str;
    else if (code < 450) return RawValType::Int32;
    else if (code < 460) return RawValType::Int32;
    else if (code < 470) return RawValType::Dbl;
    else if (code <= 481) return RawValType::Str;
    else if (code == 1004) return RawValType::Str;
    else if (code > 998 && code < 1009) return RawValType::Str;
    else if (code < 1060) return RawValType::Dbl;
    else if (code < 1071) return RawValType::Int16;
    else if (code == 1071) return RawValType::Int32;
    return RawValType::Str;
}

bool isDxfHandleReferenceCode(int code) {
    return code == 5 || code == 105 || code == 1005 ||
           (code >= 320 && code <= 369) ||
           (code >= 390 && code <= 399) ||
           (code >= 480 && code <= 481);
}

bool parseRawDxfHandleLexeme(const std::string& text, std::uint64_t& value) {
    if (text.empty() || text.size() > 16)
        return false;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, value, 16);
    return result.ec == std::errc{} && result.ptr == end;
}

bool requiresDxfSelfHandle(DRW::Version sourceVersion) {
    return sourceVersion != DRW::UNKNOWNV && sourceVersion > DRW::AC1009;
}

bool requiresDxfSelfHandle(const dxfReader& reader) {
    return requiresDxfSelfHandle(reader.getSourceVersion());
}

bool updateRawDxfApplicationDepth(const DRW_Variant& value, int& depth) {
    if (value.code() != 102)
        return true;
    if (value.type() != DRW_Variant::STRING || value.c_str() == nullptr)
        return false;

    const std::string marker(value.c_str());
    if (marker.size() > 1 && marker.front() == '{') {
        if (++depth > DRW::kMaxDxfApplicationGroupNesting)
            return false;
    } else if (marker == "}") {
        if (depth == 0)
            return false;
        --depth;
    } else {
        return false;
    }
    return true;
}

bool isDxfBinaryChunkCode(int code) {
    return (code >= 310 && code <= 319) || code == 1004;
}

bool isDxfHexString(const std::string& text) {
    if ((text.size() & 1u) != 0u || text.size() / 2u > 127u)
        return false;
    return std::all_of(text.begin(), text.end(), [](unsigned char ch) {
        return std::isxdigit(ch) != 0;
    });
}

bool isValidDxfHandleString(const std::string& text) {
    std::uint64_t value = 0;
    return parseRawDxfHandleLexeme(text, value);
}

bool hasRawDxfSelfHandle(const DRW_RawDxfObject& object) {
    for (const DRW_Variant& group : object.groups) {
        if (group.code() != 5 || group.type() != DRW_Variant::STRING
            || group.c_str() == nullptr) {
            continue;
        }
        std::uint64_t value = 0;
        if (parseRawDxfHandleLexeme(group.c_str(), value) && value != 0)
            return true;
    }
    return false;
}

bool parseDxfIntegerString(const std::string& text, std::int64_t minimum,
                           std::int64_t maximum) {
    const char* begin = text.data();
    const char* end = begin + text.size();
    while (begin != end && std::isspace(static_cast<unsigned char>(*begin)))
        ++begin;
    while (end != begin && std::isspace(static_cast<unsigned char>(end[-1])))
        --end;
    if (begin == end)
        return false;
    if (*begin == '+') {
        if (++begin == end)
            return false;
    }

    std::int64_t value = 0;
    const std::from_chars_result result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end
        && value >= minimum && value <= maximum;
}

bool isValidDxfDoubleString(const std::string& text) {
    char* end = nullptr;
    errno = 0;
    const double value = std::strtod(text.c_str(), &end);
    while (end != nullptr && *end != '\0'
           && std::isspace(static_cast<unsigned char>(*end))) {
        ++end;
    }
    return end != text.c_str() && end != nullptr && *end == '\0'
        && errno != ERANGE && std::isfinite(value);
}

bool isValidRawDxfNumericString(RawValType type, const std::string& text) {
    switch (type) {
    case RawValType::Int16:
        return parseDxfIntegerString(
            text, std::numeric_limits<std::int16_t>::min(),
            std::numeric_limits<std::uint16_t>::max());
    case RawValType::Int32:
    case RawValType::Bool:
        return parseDxfIntegerString(
            text, std::numeric_limits<std::int32_t>::min(),
            std::numeric_limits<std::int32_t>::max());
    case RawValType::Int64:
        return parseDxfIntegerString(
            text, std::numeric_limits<std::int64_t>::min(),
            std::numeric_limits<std::int64_t>::max());
    case RawValType::Dbl:
        return isValidDxfDoubleString(text);
    case RawValType::Str:
        return false;
    }
    return false;
}

bool isSafeAsciiDxfLexeme(const std::string& text) {
    return text.find_first_of("\r\n") == std::string::npos
        && text.find('\0') == std::string::npos;
}

bool isValidRawDxfLexeme(int code, RawValType type,
                         const std::string& text) {
    if (!isSafeAsciiDxfLexeme(text))
        return false;
    if (isDxfBinaryChunkCode(code))
        return isDxfHexString(text);
    if (isDxfHandleReferenceCode(code))
        return isValidDxfHandleString(text);
    return type == RawValType::Str || isValidRawDxfNumericString(type, text);
}

bool validateRawDxfGroups(const std::vector<DRW_Variant>& groups,
                          const std::vector<UTF8STRING>& rawValues,
                          bool hasRawValues,
                          bool binaryOutput,
                          bool allowRecordBoundaries) {
    if (groups.size() > DRW::kMaxDxfApplicationGroupPairs
        || (hasRawValues && rawValues.size() != groups.size())) {
        return false;
    }

    const bool writesRawValues = hasRawValues && !binaryOutput;
    int applicationDepth = 0;
    for (std::size_t groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
        const DRW_Variant& group = groups[groupIndex];
        const int code = group.code();
        if (code < 0 || code > 1071
            || (!allowRecordBoundaries && code == 0)) {
            return false;
        }

        const RawValType type = classifyDxfCode(code);
        if (writesRawValues
            && !isValidRawDxfLexeme(code, type, rawValues[groupIndex])) {
            return false;
        }

        switch (type) {
        case RawValType::Str: {
            if (group.type() != DRW_Variant::STRING || group.c_str() == nullptr)
                return false;
            const std::string value(group.c_str());
            if ((isDxfBinaryChunkCode(code) && !isDxfHexString(value))
                || (isDxfHandleReferenceCode(code)
                    && !isValidDxfHandleString(value))) {
                return false;
            }
            break;
        }
        case RawValType::Int16:
            if (group.type() == DRW_Variant::STRING && !binaryOutput
                && isValidRawDxfNumericString(
                    RawValType::Int16, group.c_str())) {
                break;
            }
            if (group.type() != DRW_Variant::INTEGER
                || group.i_val() < std::numeric_limits<std::int16_t>::min()
                || group.i_val() > std::numeric_limits<std::uint16_t>::max()) {
                return false;
            }
            break;
        case RawValType::Int32:
        case RawValType::Bool:
            if (group.type() == DRW_Variant::STRING && !binaryOutput
                && isValidRawDxfNumericString(classifyDxfCode(code),
                                               group.c_str())) {
                break;
            }
            if (group.type() != DRW_Variant::INTEGER)
                return false;
            break;
        case RawValType::Int64:
            if (group.type() == DRW_Variant::STRING && !binaryOutput
                && isValidRawDxfNumericString(
                    RawValType::Int64, group.c_str())) {
                break;
            }
            if (group.type() != DRW_Variant::INTEGER64)
                return false;
            break;
        case RawValType::Dbl:
            if (group.type() == DRW_Variant::STRING && !binaryOutput
                && isValidRawDxfNumericString(
                    RawValType::Dbl, group.c_str())) {
                break;
            }
            if (group.type() != DRW_Variant::DOUBLE
                || !std::isfinite(group.d_val())) {
                return false;
            }
            break;
        }

        if (code == 102 && writesRawValues) {
            const DRW_Variant rawMarker(code, rawValues[groupIndex]);
            if (!updateRawDxfApplicationDepth(rawMarker, applicationDepth))
                return false;
        } else if (!updateRawDxfApplicationDepth(group, applicationDepth)) {
            return false;
        }
    }
    return applicationDepth == 0;
}

bool validateCapturedRawDxfObject(const DRW_RawDxfObject& object,
                                  bool binaryOutput) {
    return validateRawDxfGroups(object.groups, object.rawValues,
                                object.hasRawValues, binaryOutput,
                                /*allowRecordBoundaries=*/false);
}

enum class DxfProxyPayloadSlot { Primary, Body, Unknown };

struct DxfProxyCapture {
    DxfProxyPayloadSlot slot = DxfProxyPayloadSlot::Primary;
    bool inProxyRecord = false;
    int applicationDepth = 0;
    std::vector<std::string> applicationGroups;
    bool hasOwner = false;
    std::uint32_t ownerHandle = 0;
    std::vector<std::uint32_t> reactorHandles;
    std::uint32_t xDictHandle = 0;
    bool hasProxyClassId = false;
    std::int32_t proxyClassId = 0;
    bool hasProxyCarrierId = false;
    std::int32_t proxyCarrierId = 0;
    bool hasProxyDrawingFormat = false;
    std::uint32_t proxyDrawingFormat = 0;
    bool hasFromDxf = false;
    bool fromDxf = false;
    bool hasPrimaryByteSize = false;
    std::uint64_t primaryByteSize = 0;
    bool hasBodyBitSize = false;
    std::uint32_t bodyBitSize = 0;
    bool hasUnknownByteSize = false;
    std::uint64_t unknownByteSize = 0;
    std::vector<std::uint8_t> primary;
    std::vector<std::uint8_t> body;
    std::vector<std::uint8_t> unknown;
    std::vector<DRW_ProxyObjectIdRef> objectIdRefs;
};

bool proxyDxfInteger(const DRW_Variant& value, std::int64_t& result) {
    if (value.type() == DRW_Variant::INTEGER) {
        result = value.i_val();
        return true;
    }
    if (value.type() == DRW_Variant::INTEGER64) {
        result = value.i64_val();
        return true;
    }
    return false;
}

bool proxyDxfInt32(const DRW_Variant& value, std::int32_t& result) {
    std::int64_t integer = 0;
    if (!proxyDxfInteger(value, integer)
        || integer < std::numeric_limits<std::int32_t>::min()
        || integer > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    result = static_cast<std::int32_t>(integer);
    return true;
}

bool proxyDxfHandle(const DRW_Variant& value, std::uint64_t& result) {
    if (value.type() != DRW_Variant::STRING || value.c_str() == nullptr)
        return false;
    const std::string text(value.c_str());
    if (text.empty())
        return false;

    result = 0;
    for (char ch : text) {
        unsigned digit = 0;
        if (ch >= '0' && ch <= '9')
            digit = static_cast<unsigned>(ch - '0');
        else if (ch >= 'a' && ch <= 'f')
            digit = static_cast<unsigned>(ch - 'a' + 10);
        else if (ch >= 'A' && ch <= 'F')
            digit = static_cast<unsigned>(ch - 'A' + 10);
        else
            return false;
        if (result > (std::numeric_limits<std::uint64_t>::max() - digit) / 16u)
            return false;
        result = result * 16u + digit;
    }
    return true;
}

bool appendProxyDxfBytes(const DRW_Variant& value,
                         std::vector<std::uint8_t>& target) {
    if (target.size() > DRW::kMaxDxfBinaryPayloadBytes)
        return false;
    if (value.type() == DRW_Variant::BINARY) {
        const auto* bytes = value.binary();
        if (bytes == nullptr || bytes->size() >
                                  DRW::kMaxDxfBinaryPayloadBytes - target.size())
            return false;
        const std::size_t newSize = target.size() + bytes->size();
        if (newSize > static_cast<std::size_t>(std::numeric_limits<int>::max())
            || !DRW::reserve(target, static_cast<int>(newSize)))
            return false;
        try {
            target.insert(target.end(), bytes->begin(), bytes->end());
        } catch (...) {
            return false;
        }
        return true;
    }
    if (value.type() != DRW_Variant::STRING || value.c_str() == nullptr)
        return false;

    const std::string text(value.c_str());
    return appendDxfHexChunk(text, target);
}

bool validateProxyDxfPayloads(const DxfProxyCapture& capture) {
    const auto byteCountMatches = [](const std::vector<std::uint8_t>& data,
                                      bool declared,
                                      std::uint64_t expected) {
        return !declared || (expected <= DRW::kMaxDxfBinaryPayloadBytes
                             && data.size() == expected);
    };
    const auto bitCountMatches = [](const std::vector<std::uint8_t>& data,
                                    bool declared,
                                    std::uint64_t expected) {
        if (!declared)
            return true;
        if (expected > DRW::kMaxDxfBinaryPayloadBytes * 8u)
            return false;
        return (expected + 7u) / 8u == data.size();
    };

    return byteCountMatches(capture.primary, capture.hasPrimaryByteSize,
                            capture.primaryByteSize)
        && bitCountMatches(capture.body, capture.hasBodyBitSize,
                           capture.bodyBitSize)
        && byteCountMatches(capture.unknown, capture.hasUnknownByteSize,
                            capture.unknownByteSize);
}

int proxyDxfHandleCode(int code) {
    switch (code) {
    case 330: return 2;
    case 340: return 3;
    case 350: return 4;
    case 360: return 5;
    default: return 0;
    }
}

bool collectProxyDxfGroup(DxfProxyCapture& capture,
                          const DRW_Variant& value, bool entity) {
    const int code = value.code();
    if (code == 102) {
        if (value.type() != DRW_Variant::STRING || value.c_str() == nullptr)
            return false;
        const std::string marker(value.c_str());
        if (marker.size() > 1 && marker.front() == '{') {
            if (++capture.applicationDepth > DRW::kMaxDxfApplicationGroupNesting)
                return false;
            capture.applicationGroups.emplace_back(marker.substr(1));
        } else if (marker == "}") {
            if (capture.applicationDepth == 0
                || capture.applicationGroups.empty())
                return false;
            --capture.applicationDepth;
            capture.applicationGroups.pop_back();
        } else {
            return false;
        }
        return true;
    }

    if (code == 330 || code == 340 || code == 350 || code == 360) {
        std::uint64_t handle = 0;
        if (!proxyDxfHandle(value, handle))
            return false;
        if (capture.applicationDepth != 0) {
            const std::string& group = capture.applicationGroups.back();
            if (capture.applicationDepth == 1
                && dxfKeywordEquals(group, "ACAD_REACTORS")
                && code == 330) {
                if (handle != 0
                    && handle <= std::numeric_limits<std::uint32_t>::max()) {
                    capture.reactorHandles.push_back(
                        static_cast<std::uint32_t>(handle));
                }
            } else if (capture.applicationDepth == 1
                       && dxfKeywordEquals(group, "ACAD_XDICTIONARY")
                       && (code == 360 || code == 361 || code == 362)
                       && capture.xDictHandle == 0
                       && handle != 0
                       && handle <= std::numeric_limits<std::uint32_t>::max()) {
                capture.xDictHandle = static_cast<std::uint32_t>(handle);
            }
            return true;
        }
        if (capture.inProxyRecord) {
            DRW_ProxyObjectIdRef ref;
            ref.m_dxfCode = code;
            ref.m_handleCode = static_cast<std::uint8_t>(proxyDxfHandleCode(code));
            ref.m_handle = handle;
            ref.m_rawHandle = handle;
            capture.objectIdRefs.push_back(ref);
        } else if (code == 330 && !capture.hasOwner) {
            if (handle > std::numeric_limits<std::uint32_t>::max())
                return false;
            capture.hasOwner = true;
            capture.ownerHandle = static_cast<std::uint32_t>(handle);
        }
        return true;
    }

    std::int64_t integer = 0;
    std::int32_t integer32 = 0;
    switch (code) {
    case 5:
        // The self handle is consumed by the common raw carrier. No proxy
        // payload state is changed here.
        return true;
    case 90:
        if (!proxyDxfInt32(value, integer32)) return false;
        capture.inProxyRecord = true;
        capture.hasProxyCarrierId = true;
        capture.proxyCarrierId = integer32;
        return true;
    case 91:
        if (!proxyDxfInt32(value, integer32)) return false;
        capture.inProxyRecord = true;
        capture.hasProxyClassId = true;
        capture.proxyClassId = integer32;
        return true;
    case 92:
    case 160:
        if (capture.hasPrimaryByteSize
            || !proxyDxfInteger(value, integer)
            || integer < 0
            || static_cast<std::uint64_t>(integer)
                   > DRW::kMaxDxfBinaryPayloadBytes)
            return false;
        capture.inProxyRecord = true;
        capture.hasPrimaryByteSize = true;
        capture.primaryByteSize = static_cast<std::uint64_t>(integer);
        capture.slot = DxfProxyPayloadSlot::Primary;
        return true;
    case 93:
    case 161:
        if (!proxyDxfInteger(value, integer) || integer < 0
            || capture.hasBodyBitSize
            || static_cast<std::uint64_t>(integer)
                   > DRW::kMaxDxfBinaryPayloadBytes * 8u)
            return false;
        capture.inProxyRecord = true;
        capture.hasBodyBitSize = true;
        capture.bodyBitSize = static_cast<std::uint32_t>(integer);
        capture.slot = DxfProxyPayloadSlot::Body;
        return true;
    case 94:
        capture.inProxyRecord = true;
        return true;
    case 95:
        if (!proxyDxfInteger(value, integer)
            || integer < 0
            || static_cast<std::uint64_t>(integer)
                   > std::numeric_limits<std::uint32_t>::max())
            return false;
        capture.inProxyRecord = true;
        capture.hasProxyDrawingFormat = true;
        capture.proxyDrawingFormat = static_cast<std::uint32_t>(integer);
        return true;
    case 96:
    case 162:
        if (capture.hasUnknownByteSize
            || !proxyDxfInteger(value, integer)
            || integer < 0
            || static_cast<std::uint64_t>(integer)
                   > DRW::kMaxDxfBinaryPayloadBytes)
            return false;
        capture.inProxyRecord = true;
        capture.hasUnknownByteSize = true;
        capture.unknownByteSize = static_cast<std::uint64_t>(integer);
        capture.slot = DxfProxyPayloadSlot::Unknown;
        return true;
    case 70:
        if (!proxyDxfInteger(value, integer)) return false;
        capture.inProxyRecord = true;
        capture.hasFromDxf = true;
        capture.fromDxf = integer != 0;
        return true;
    case 71:
    case 97:
        capture.inProxyRecord = true;
        return true;
    case 310: {
        capture.inProxyRecord = true;
        auto* target = &capture.primary;
        if (capture.slot == DxfProxyPayloadSlot::Body)
            target = &capture.body;
        else if (capture.slot == DxfProxyPayloadSlot::Unknown)
            target = &capture.unknown;
        return appendProxyDxfBytes(value, *target);
    }
    case 311:
        capture.inProxyRecord = true;
        return appendProxyDxfBytes(value,
                                   entity ? capture.unknown : capture.body);
    default:
        return true;
    }
}

void applyProxyDxfCommon(DRW_ProxyEntity& entity,
                         const DRW_Variant& value) {
    switch (value.code()) {
    case 5:
        if (value.type() == DRW_Variant::STRING) {
            std::uint64_t handle = 0;
            if (proxyDxfHandle(value, handle))
                entity.handle = static_cast<std::uint32_t>(handle);
        }
        break;
    case 8:
        if (value.type() == DRW_Variant::STRING)
            entity.layer = value.c_str();
        break;
    case 6:
        if (value.type() == DRW_Variant::STRING)
            entity.lineType = value.c_str();
        break;
    case 62:
        if (value.type() == DRW_Variant::INTEGER)
            entity.color = value.i_val();
        break;
    case 370:
        if (value.type() == DRW_Variant::INTEGER)
            entity.lWeight = DRW_LW_Conv::dxfInt2lineWidth(value.i_val());
        break;
    case 48:
        if (value.type() == DRW_Variant::DOUBLE)
            entity.ltypeScale = value.d_val();
        break;
    case 60:
        if (value.type() == DRW_Variant::INTEGER)
            entity.visible = (value.i_val() & 1) == 0;
        break;
    case 67:
        if (value.type() == DRW_Variant::INTEGER)
            entity.space = static_cast<DRW::Space>(value.i_val());
        break;
    case 347:
        if (value.type() == DRW_Variant::STRING) {
            std::uint64_t handle = 0;
            if (proxyDxfHandle(value, handle))
                entity.material = static_cast<std::uint32_t>(handle);
        }
        break;
    case 390:
        if (value.type() == DRW_Variant::STRING) {
            std::uint64_t handle = 0;
            if (proxyDxfHandle(value, handle))
                entity.plotStyle = static_cast<std::uint32_t>(handle);
        }
        break;
    case 420:
        if (value.type() == DRW_Variant::INTEGER)
            entity.color24 = value.i_val();
        break;
    case 430:
        if (value.type() == DRW_Variant::STRING)
            entity.colorName = value.c_str();
        break;
    case 440:
        if (value.type() == DRW_Variant::INTEGER)
            entity.transparency = value.i_val();
        break;
    default:
        break;
    }
}

bool applyProxyDxfCapture(DRW_ProxyEntity& entity,
                          DxfProxyCapture&& capture) {
    entity.m_hasProxyClassId = capture.hasProxyClassId;
    entity.m_proxyClassId = capture.proxyClassId;
    entity.m_hasProxyCarrierId = capture.hasProxyCarrierId;
    entity.m_proxyCarrierId = capture.proxyCarrierId;
    entity.m_hasProxyDrawingFormat = capture.hasProxyDrawingFormat;
    entity.m_proxyDrawingFormat = capture.proxyDrawingFormat;
    entity.m_proxyDwgVersion = static_cast<std::uint16_t>(
        capture.proxyDrawingFormat & 0xFFFFu);
    entity.m_proxyMaintenanceVersion = static_cast<std::uint16_t>(
        capture.proxyDrawingFormat >> 16u);
    entity.m_hasFromDxf = capture.hasFromDxf;
    entity.m_fromDxf = capture.fromDxf;
    entity.m_hasProxyGraphicsByteSize = capture.hasPrimaryByteSize;
    entity.m_proxyGraphicsByteSize = capture.primaryByteSize;
    entity.m_hasEntityDataBitSize = capture.hasBodyBitSize;
    entity.m_entityDataBitSize = capture.bodyBitSize;
    entity.m_hasUnknownDataByteSize = capture.hasUnknownByteSize;
    entity.m_unknownDataByteSize = capture.unknownByteSize;
    entity.m_entityData = std::move(capture.body);
    entity.m_unknownData = std::move(capture.unknown);
    entity.proxyGraphics.clear();
    if (!capture.primary.empty()) {
        if (capture.primary.size()
                > static_cast<std::size_t>(std::numeric_limits<int>::max())
            || !DRW::reserve(entity.proxyGraphics,
                             static_cast<int>(capture.primary.size())))
            return false;
        try {
            entity.proxyGraphics.assign(
                reinterpret_cast<const char*>(capture.primary.data()),
                capture.primary.size());
        } catch (...) {
            return false;
        }
    }
    entity.numProxyGraph = capture.primary.size() >
                                   static_cast<std::size_t>(std::numeric_limits<int>::max())
        ? std::numeric_limits<int>::max()
        : static_cast<int>(capture.primary.size());
    entity.reactorHandles = std::move(capture.reactorHandles);
    entity.xDictHandle = capture.xDictHandle;
    entity.m_objectIdRefs = std::move(capture.objectIdRefs);
    return true;
}

void applyProxyDxfCapture(DRW_ProxyObject& object,
                          DxfProxyCapture&& capture) {
    object.m_hasProxyClassId = capture.hasProxyClassId;
    object.m_proxyClassId = capture.proxyClassId;
    object.m_hasProxyCarrierId = capture.hasProxyCarrierId;
    object.m_proxyCarrierId = capture.proxyCarrierId;
    object.m_hasProxyDrawingFormat = capture.hasProxyDrawingFormat;
    object.m_proxyDrawingFormat = capture.proxyDrawingFormat;
    object.m_proxyDwgVersion = static_cast<std::uint16_t>(
        capture.proxyDrawingFormat & 0xFFFFu);
    object.m_proxyMaintenanceVersion = static_cast<std::uint16_t>(
        capture.proxyDrawingFormat >> 16u);
    object.m_hasFromDxf = capture.hasFromDxf;
    object.m_fromDxf = capture.fromDxf;
    object.m_hasProxyGraphicsByteSize = capture.hasPrimaryByteSize;
    object.m_proxyGraphicsByteSize = capture.primaryByteSize;
    object.m_hasObjectDataBitSize = capture.hasBodyBitSize;
    object.m_objectDataBitSize = capture.bodyBitSize;
    object.m_hasUnknownDataByteSize = capture.hasUnknownByteSize;
    object.m_unknownDataByteSize = capture.unknownByteSize;
    object.m_binaryData = std::move(capture.primary);
    object.m_objectData = std::move(capture.body);
    object.m_unknownData = std::move(capture.unknown);
    object.reactorHandles = std::move(capture.reactorHandles);
    object.xDictHandle = capture.xDictHandle;
    object.m_objectIdRefs = std::move(capture.objectIdRefs);
}
}  // namespace

//Capture the current DXF record into a raw-passthrough carrier as a correctly
//TYPED DRW_Variant (see classifyDxfCode above for why getString()/reader->type
//cannot be trusted for numeric codes — that was the A1/A4 capture bug). The write
//side (writeRawDxfObject) re-emits binary values from their matching variant
//type and uses rawValues only for ASCII source spellings. Also latches code 5
//-> handle and code 330 -> parentHandle.
bool dxfRW::captureRawGroup(DRW_RawDxfObject &obj, int code,
                            bool validateHandles) {
    if (obj.m_version == DRW::UNKNOWNV)
        obj.m_version = reader->getSourceVersion();
    obj.hasRawValues = !binFile;
    if (validateHandles && isDxfHandleReferenceCode(code)) {
        const bool validHandle = reader->allowsWideHandleLexemes()
            ? reader->isValidHandleLexeme()
            : reader->isValidHandleString();
        if (!validHandle)
            return false;
    }
    if (code == DRW::dxfCode::HANDLE) {
        std::uint64_t rawHandle = 0;
        if (!parseRawDxfHandleLexeme(reader->getString(), rawHandle))
            return false;
        if (!reader->registerSelfHandle())
            return false;
        if (rawHandle != 0 && !m_readRawHandles.insert(rawHandle).second)
            return false;
    }
    try {
        switch (classifyDxfCode(code)) {
        case RawValType::Int16:
        case RawValType::Int32:
        case RawValType::Bool:
            obj.groups.emplace_back(code,
                                    static_cast<std::int32_t>(
                                        reader->getInt32()));
            break;
        case RawValType::Int64:
            obj.groups.emplace_back(code,
                                    static_cast<std::int64_t>(
                                        reader->getInt64()));
            break;
        case RawValType::Dbl:
            obj.groups.emplace_back(code, reader->getDouble());
            break;
        case RawValType::Str:
        default:
            obj.groups.emplace_back(code, reader->getString());
            break;
        }
        obj.rawValues.emplace_back(binFile ? std::string{}
                                            : reader->getRawValue());
    } catch (...) {
        return false;
    }
    if (5 == code && obj.handle == 0) {
        // The raw group string remains authoritative when the handle does not
        // fit this legacy 32-bit convenience field.
        obj.handle = reader->getHandleString();
    } else if (330 == code) {
        // Latch the OWNER 330 only — the one OUTSIDE any 102 {ACAD_REACTORS/
        // ACAD_XDICTIONARY} control group. Reactor 330s live at 102-group depth
        // >= 1; the prior code latched the LAST 330 unconditionally, so an object
        // whose only 330s are reactors (no owner 330) took a reactor handle as
        // its owner. Compute the depth from the groups captured so far (the
        // current 330 is the last element) and latch only at depth 0.
        int depth = 0;
        for (std::size_t i = 0; i + 1 < obj.groups.size(); ++i) {
            if (obj.groups[i].code() == 102
                && obj.groups[i].type() == DRW_Variant::STRING) {
                const char *v = obj.groups[i].c_str();
                if (v && v[0] == '{' && v[1] != '\0') ++depth;
                else if (v && v[0] == '}') --depth;
            }
        }
        if (depth == 0 && obj.parentHandle == 0)
        obj.parentHandle = reader->getHandleString();
    }
    return true;
}

bool dxfRW::captureRawDxfApplicationGroup(DRW_RawDxfObject &obj,
                                          std::list<std::list<DRW_Variant>> &appData,
                                          std::vector<std::uint32_t> &reactorHandles,
                                          std::uint32_t &xDictHandle) {
    if (!captureRawGroup(obj, 102, /*validateHandles=*/true)
        || obj.groups.empty()
        || appData.size() >= DRW::kMaxDxfApplicationGroups) {
        return false;
    }

    const DRW_Variant &openingGroup = obj.groups.back();
    if (openingGroup.type() != DRW_Variant::STRING
        || openingGroup.c_str() == nullptr) {
        return false;
    }
    const std::string openingText(openingGroup.c_str());
    if (openingText.size() <= 1 || openingText.front() != '{')
        return false;

    const bool isReactors = openingText == "{ACAD_REACTORS";
    const bool isXDictionary = openingText == "{ACAD_XDICTIONARY";
    std::list<DRW_Variant> applicationGroup;
    std::vector<std::uint32_t> reactors;
    std::uint32_t xDictionary = 0;
    int depth = 0;
    std::size_t pairCount = 1;

    try {
        DRW_Variant opener;
        opener.addString(102, openingText.substr(1));
        applicationGroup.push_back(std::move(opener));
        if (!updateRawDxfApplicationDepth(openingGroup, depth))
            return false;

        int code = 0;
        while (depth > 0 && reader->readRec(&code)) {
            if (code == 0 || pairCount >= DRW::kMaxDxfApplicationGroupPairs
                || !captureRawGroup(obj, code, /*validateHandles=*/true)
                || obj.groups.empty()) {
                return false;
            }
            ++pairCount;

            const DRW_Variant &group = obj.groups.back();
            const bool topLevelGroup = depth == 1;
            if (!updateRawDxfApplicationDepth(group, depth))
                return false;
            applicationGroup.push_back(group);

            if (topLevelGroup && isReactors && code == 330) {
                const std::uint32_t handle = reader->getHandleString();
                if (handle != 0) {
                    if (reactors.size() >= dwgSafety::MaxReactorCount)
                        return false;
                    reactors.push_back(handle);
                }
            } else if (topLevelGroup && isXDictionary && code == 360
                       && xDictionary == 0) {
                xDictionary = reader->getHandleString();
            }
        }
    } catch (...) {
        return false;
    }

    if (depth != 0)
        return false;
    try {
        appData.push_back(std::move(applicationGroup));
        if (isReactors)
            reactorHandles = std::move(reactors);
        if (isXDictionary && xDictionary != 0)
            xDictHandle = xDictionary;
    } catch (...) {
        return false;
    }
    return true;
}

bool dxfRW::captureRawDxfApplicationGroup(DRW_RawDxfObject &obj,
                                          DRW_TableEntry &entry) {
    return captureRawDxfApplicationGroup(obj, entry.appData,
                                         entry.reactorHandles,
                                         entry.xDictHandle);
}

bool dxfRW::captureRawDxfApplicationGroup(DRW_RawDxfObject &obj,
                                          DRW_Entity &entity) {
    return captureRawDxfApplicationGroup(obj, entity.appData,
                                         entity.reactorHandles,
                                         entity.xDictHandle);
}

bool dxfRW::captureAndParseRawDxfGroup(DRW_RawDxfObject &obj, int code,
                                       DRW_TableEntry &entry) {
    if (code == 102)
        return captureRawDxfApplicationGroup(obj, entry);
    return captureRawGroup(obj, code, /*validateHandles=*/true)
        && entry.parseCode(code, reader);
}

bool dxfRW::captureAndParseRawDxfGroup(DRW_RawDxfObject &obj, int code,
                                       DRW_Entity &entity) {
    if (code == 102)
        return captureRawDxfApplicationGroup(obj, entity);
    return captureRawGroup(obj, code, /*validateHandles=*/true)
        && entity.parseCode(code, reader);
}

bool dxfRW::processRawObject() {
    DRW_DBG("dxfRW::processRawObject");
    int code;
    DRW_RawDxfObject obj;
    obj.name = nextentity;
    DxfRawHandleLexemeScope handleLexemes(*reader);
    int applicationDepth = 0;
    std::size_t pairCount = 0;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            if (boundary == DxfEntityBoundary::Error
                || boundary == DxfEntityBoundary::EndBlock
                || applicationDepth != 0)
                return setError(DRW::BAD_READ_OBJECTS);
            if (requiresDxfSelfHandle(*reader) && !hasRawDxfSelfHandle(obj))
                return setError(DRW::BAD_READ_OBJECTS);
            iface->addRawDxfObject(obj);
            return true;  //found new entity or ENDSEC, terminate
        }
        if (++pairCount > DRW::kMaxDxfApplicationGroupPairs)
            return setError(DRW::BAD_CODE_PARSED);
        if (!captureRawGroup(obj, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);
        if (obj.groups.empty()
            || !updateRawDxfApplicationDepth(obj.groups.back(),
                                             applicationDepth))
            return setError(DRW::BAD_CODE_PARSED);
    }

    return setError(DRW::BAD_READ_OBJECTS);
}

//Slice A4: lossless passthrough for an unmodeled entity in the ENTITIES section
//or inside a BLOCK. Same verbatim capture as
//processRawObject but reports via addRawDxfEntity / the entities error code.
bool dxfRW::processRawEntity() {
    DRW_DBG("dxfRW::processRawEntity");
    int code;
    DRW_RawDxfObject ent;
    ent.name = nextentity;
    DxfRawHandleLexemeScope handleLexemes(*reader);
    // Accumulate any cached proxy graphics (codes 92/160 + 310) via the common
    // entity parser so we can decode it into render primitives — same path as
    // the DWG reader.  proxyHost is a throwaway carrier; the raw object is still
    // delivered verbatim for round-trip.
    DRW_Point proxyHost;
    int applicationDepth = 0;
    std::size_t pairCount = 0;
    while (reader->readRec(&code)) {
        DRW_DBG(code); DRW_DBG("\n");
        if (0 == code) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            if (boundary == DxfEntityBoundary::Error
                || !acceptEntityCallbackBoundary()
                || applicationDepth != 0)
                return setError(DRW::BAD_READ_ENTITIES);
            if (requiresDxfSelfHandle(*reader) && !hasRawDxfSelfHandle(ent))
                return setError(DRW::BAD_READ_ENTITIES);
            if (proxyHost.proxyGraphics.size() >= 16)
                DRW_ProxyGraphicDecoder::decode(
                    proxyHost.proxyGraphics, reader->getSourceVersion(),
                    *iface, proxyHost);
            iface->addRawDxfEntity(ent);
            return true;  //found new entity, ENDSEC or ENDBLK, terminate
        }
        if (++pairCount > DRW::kMaxDxfApplicationGroupPairs)
            return setError(DRW::BAD_CODE_PARSED);
        // Keep raw capture authoritative. Parsing every group through a typed
        // entity parser is unsafe here: code 102 recursively consumes its
        // child records, so the raw stream would lose nested groups. Only
        // parse the common fields needed by the proxy decoder's draw state.
        switch (code) {
        case DRW::dxfCode::HANDLE:
            // A wide raw handle has no typed proxy-host representation. Keep
            // its source lexeme in the raw carrier and do not turn it into a
            // misleading zero handle for proxy-graphic decoding.
            if (reader->isValidHandleString()
                && !proxyHost.parseCode(code, reader)) {
                return setError(DRW::BAD_CODE_PARSED);
            }
            break;
        case DRW::dxfCode::LAYER:
        case 6:
        case DRW::dxfCode::COLOR:
        case DRW::dxfCode::LINEWEIGHT:
        case 67:
        case 92:
        case 160:
        case 310:
            if (!proxyHost.parseCode(code, reader))
                return setError(DRW::BAD_CODE_PARSED);
            break;
        default:
            break;
        }
        if (!captureRawGroup(ent, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);
        if (ent.groups.empty()
            || !updateRawDxfApplicationDepth(ent.groups.back(),
                                             applicationDepth))
            return setError(DRW::BAD_CODE_PARSED);
    }

    return setError(DRW::BAD_READ_ENTITIES);
}

// ACAD_PROXY_ENTITY carries typed metadata around two independent binary
// payloads.  Keep the complete raw group stream alongside the typed view so
// DXF->DXF replay never depends on a guessed proxy writer.
bool dxfRW::processProxyEntity() {
    DRW_DBG("dxfRW::processProxyEntity\n");
    int code;
    DRW_ProxyEntity entity;
    DRW_RawDxfObject raw;
    DxfProxyCapture capture;
    raw.name = nextentity;
    // Proxy records are retained as raw carriers. Keep DWG-width code-5
    // lexemes lossless even when the typed convenience handle is uint32_t.
    DxfRawHandleLexemeScope handleLexemes(*reader);
    std::size_t pairCount = 0;

    while (reader->readRec(&code)) {
        if (code == 0) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            if (boundary == DxfEntityBoundary::Error
                || !acceptEntityCallbackBoundary())
                return setError(DRW::BAD_READ_ENTITIES);
            if (capture.applicationDepth != 0)
                return setError(DRW::BAD_CODE_PARSED);
            if (!validateProxyDxfPayloads(capture))
                return setError(DRW::BAD_CODE_PARSED);
            if (requiresDxfSelfHandle(*reader) && !hasRawDxfSelfHandle(raw))
                return setError(DRW::BAD_READ_ENTITIES);

            entity.handle = raw.handle;
            entity.parentHandle = capture.hasOwner ? capture.ownerHandle
                                                   : DRW::NoHandle;
            raw.parentHandle = entity.parentHandle;
            if (!applyProxyDxfCapture(entity, std::move(capture)))
                return setError(DRW::BAD_READ_ENTITIES);
            if (entity.proxyGraphics.size() >= 16)
                DRW_ProxyGraphicDecoder::decode(
                    entity.proxyGraphics, reader->getSourceVersion(),
                    *iface, entity);
            iface->addProxyEntity(entity);
            iface->addRawDxfEntity(raw);
            return true;
        }
        if (++pairCount > DRW::kMaxDxfApplicationGroupPairs)
            return setError(DRW::BAD_CODE_PARSED);

        if (!captureRawGroup(raw, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);
        if (raw.groups.empty()
            || !collectProxyDxfGroup(capture, raw.groups.back(), true)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
        applyProxyDxfCommon(entity, raw.groups.back());
    }
    return setError(DRW::BAD_READ_ENTITIES);
}

// ACAD_PROXY_OBJECT follows the same DXF proxy-data policy as the entity, but
// code 311 belongs to object data rather than the entity's unknown slot.
bool dxfRW::processProxyObject() {
    DRW_DBG("dxfRW::processProxyObject\n");
    int code;
    DRW_ProxyObject object;
    DRW_RawDxfObject raw;
    DxfProxyCapture capture;
    raw.name = nextentity;
    // Proxy records are retained as raw carriers. Keep DWG-width code-5
    // lexemes lossless even when the typed convenience handle is uint32_t.
    DxfRawHandleLexemeScope handleLexemes(*reader);
    std::size_t pairCount = 0;

    while (reader->readRec(&code)) {
        if (code == 0) {
            const DxfEntityBoundary boundary = setEntityBoundary(code);
            if (boundary == DxfEntityBoundary::Error
                || boundary == DxfEntityBoundary::EndBlock)
                return setError(DRW::BAD_READ_OBJECTS);
            if (capture.applicationDepth != 0)
                return setError(DRW::BAD_CODE_PARSED);
            if (!validateProxyDxfPayloads(capture))
                return setError(DRW::BAD_CODE_PARSED);
            if (requiresDxfSelfHandle(*reader) && !hasRawDxfSelfHandle(raw))
                return setError(DRW::BAD_READ_OBJECTS);

            object.handle = raw.handle;
            object.parentHandle = capture.hasOwner ? capture.ownerHandle
                                                   : DRW::NoHandle;
            raw.parentHandle = object.parentHandle;
            applyProxyDxfCapture(object, std::move(capture));
            iface->addProxyObject(object);
            iface->addRawDxfObject(raw);
            return true;
        }
        if (++pairCount > DRW::kMaxDxfApplicationGroupPairs)
            return setError(DRW::BAD_CODE_PARSED);

        if (!captureRawGroup(raw, code, /*validateHandles=*/true))
            return setError(DRW::BAD_CODE_PARSED);
        if (raw.groups.empty()
            || !collectProxyDxfGroup(capture, raw.groups.back(), false)) {
            return setError(DRW::BAD_CODE_PARSED);
        }
    }
    return setError(DRW::BAD_READ_OBJECTS);
}

bool dxfRW::writeRawDxfGroups(
    const std::vector<DRW_Variant> &groups,
    const std::vector<UTF8STRING> &rawValues,
    bool hasRawValues,
    DRW::Version sourceVersion,
    bool remapSourceHandles) {
    if (writer == nullptr
        || (sourceVersion != DRW::UNKNOWNV && sourceVersion != version)) {
        m_writeError = true;
        return false;
    }
    if (!validateRawDxfGroups(groups, rawValues, hasRawValues, binFile,
                              /*allowRecordBoundaries=*/true)) {
        m_writeError = true;
        return false;
    }

    auto writeString = [this](int code, const std::string &value) {
        if (!writer->writeString(code, value)) {
            m_writeError = true;
            return false;
        }
        return true;
    };
    auto writeInt16 = [this](int code, int value) {
        if (!writer->writeInt16(code, value)) {
            m_writeError = true;
            return false;
        }
        return true;
    };
    auto writeInt32 = [this](int code, int value) {
        if (!writer->writeInt32(code, value)) {
            m_writeError = true;
            return false;
        }
        return true;
    };
    auto writeBool = [this](int code, bool value) {
        if (!writer->writeBool(code, value)) {
            m_writeError = true;
            return false;
        }
        return true;
    };
    auto writeInt64 = [this](int code, std::int64_t value) {
        if (!writer->writeInt64(code, value)) {
            m_writeError = true;
            return false;
        }
        return true;
    };
    auto writeDouble = [this](int code, double value) {
        if (!writer->writeDouble(code, value)) {
            m_writeError = true;
            return false;
        }
        return true;
    };

    for (std::size_t groupIndex = 0; groupIndex < groups.size();
         ++groupIndex) {
        const DRW_Variant &v = groups[groupIndex];
        // Apply the structural-collision handle remap to handle-reference codes.
        // The value is a hex handle string; if it names a remapped handle, emit
        // the replacement so this object (and every reference to a remapped
        // object) stays internally consistent.
        if (v.type() == DRW_Variant::STRING
            && isDxfHandleReferenceCode(v.code())) {
            std::string s = v.c_str();
            std::uint32_t parsed = 0;
            const char* begin = s.data();
            const char* end = begin + s.size();
            const std::from_chars_result result =
                std::from_chars(begin, end, parsed, 16);
            if (result.ec == std::errc{} && result.ptr == end) {
                std::uint32_t remapped = parsed;
                const auto structural = m_handleRemap.find(parsed);
                if (structural != m_handleRemap.end()) {
                    remapped = structural->second;
                } else if (remapSourceHandles) {
                    const auto source =
                        m_writingContext.sourceHandleToMintedMap.find(parsed);
                    if (source != m_writingContext.sourceHandleToMintedMap.end())
                        remapped = source->second;
                }
                if (remapped != parsed) {
                    if (!writeString(v.code(),
                                     toHexStr(remapped)))
                        return false;
                    continue;
                }
            }
        }
        bool written = false;
        if (!binFile && hasRawValues && groupIndex < rawValues.size()) {
            written = writeString(v.code(), rawValues[groupIndex]);
        }
        if (written)
            continue;
        switch (v.type()) {
        case DRW_Variant::STRING:
            written = writeString(v.code(), std::string(v.c_str()));
            break;
        case DRW_Variant::INTEGER: {
            switch (classifyDxfCode(v.code())) {
            case RawValType::Int16:
                written = writeInt16(v.code(), v.i_val());
                break;
            case RawValType::Int32:
                written = writeInt32(v.code(), v.i_val());
                break;
            case RawValType::Bool:
                written = writeBool(v.code(), v.i_val() != 0);
                break;
            default:
                m_writeError = true;
                break;
            }
            break;
        }
        case DRW_Variant::INTEGER64:
            if (classifyDxfCode(v.code()) == RawValType::Int64)
                written = writeInt64(v.code(), v.i64_val());
            else
                m_writeError = true;
            break;
        case DRW_Variant::DOUBLE:
            if (classifyDxfCode(v.code()) == RawValType::Dbl)
                written = writeDouble(v.code(), v.d_val());
            else
                m_writeError = true;
            break;
        default:
            m_writeError = true;
            break;
        }
        if (!written)
            return false;
    }
    return true;
}

// Slice A2: re-emit a raw-captured object (from processRawObject) verbatim.
// The capture stores decoded typed values and, for ASCII input, parallel source
// spellings; the shared group writer also serves unknown DXF sections.
bool dxfRW::writeRawDxfObject(DRW_RawDxfObject *obj) {
    if (obj == nullptr || writer == nullptr
        || (obj->m_version != DRW::UNKNOWNV && obj->m_version != version)) {
        m_writeError = true;
        return false;
    }
    if (obj->name.empty()
        || !validateRawDxfGroups(obj->groups, obj->rawValues,
                                 obj->hasRawValues, binFile,
                                 /*allowRecordBoundaries=*/false)
        || (requiresDxfSelfHandle(version) && !hasRawDxfSelfHandle(*obj))) {
        m_writeError = true;
        return false;
    }
    auto writeString = [this](int code, const std::string &value) {
        if (!writer->writeString(code, value)) {
            m_writeError = true;
            return false;
        }
        return true;
    };
    DxfWriterRecordScope record(*writer);
    if (!writeString(0, obj->name))
        return false;
    if (!writeRawDxfGroups(obj->groups, obj->rawValues, obj->hasRawValues,
                            obj->m_version, /*remapSourceHandles=*/true))
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeRawDxfSection(const DRW_RawDxfSection &section) {
    if (writer == nullptr || section.m_name.empty()
        || dxfKeywordEquals(section.m_name, "HEADER")
        || dxfKeywordEquals(section.m_name, "CLASSES")
        || dxfKeywordEquals(section.m_name, "TABLES")
        || dxfKeywordEquals(section.m_name, "BLOCKS")
        || dxfKeywordEquals(section.m_name, "ENTITIES")
        || dxfKeywordEquals(section.m_name, "OBJECTS")
        || (section.m_version != DRW::UNKNOWNV
            && section.m_version != version)) {
        m_writeError = true;
        return false;
    }
    if (!validateRawDxfGroups(section.m_groups, section.m_rawValues,
                              section.m_hasRawValues, binFile,
                              /*allowRecordBoundaries=*/true)) {
        m_writeError = true;
        return false;
    }
    DxfWriterRecordScope record(*writer);
    if (!writer->writeString(0, "SECTION")
        || !writer->writeString(2, section.m_name)
        || !writeRawDxfGroups(section.m_groups, section.m_rawValues,
                              section.m_hasRawValues,
                              section.m_version,
                              /*remapSourceHandles=*/false)
        || !writer->writeString(0, "ENDSEC")) {
        m_writeError = true;
        return false;
    }
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    return true;
}

//Slice A3: canonical DXF CLASS metadata for the custom-class OBJECTS the raw net
//round-trips — both the routed data-only types (SUN/SCALE/...) and common
//unmodeled OBJECTS captured verbatim (MATERIAL/VISUALSTYLE/...). Values are the
//DXF-authoritative ezdxf CLASS_DEFINITIONS tuple {className(2), appName(3),
//flags(90), wasaProxy(280), isEntity(281)}. Every entry is a NON-fixed object
//class (ezdxf lists only classes that need a CLASS), so registering one can never
//mislabel a fixed built-in (DICTIONARY/GROUP/LAYOUT/MLINESTYLE are absent here);
//emission is instance-driven, so an entry that is never present never fires.
//instanceCount (91) is left 0 for the caller to fill. entityFlag 0 = object.
//Arbitrary/proprietary objects not in this table still round-trip losslessly
//LibreCAD<->LibreCAD but get no CLASS (a heuristic proxy is a deliberate TODO,
//since distinguishing a custom class from an unmodeled fixed type is unsafe).
bool dxfRW::dxfClassForRecordName(const std::string &recName, DRW_Class &out) {
    struct Entry { const char *rec; const char *cls; const char *app; int flag; int isEntity; };
    static const Entry table[] = {
        // Routed data-only OBJECTS (also captured into the raw net on read).
        {"SUN",              "AcDbSun",                 "SCENEOE",           1153, 0},
        {"ACDBSUN",          "AcDbSun",                 "SCENEOE",           1153, 0},
        {"SCALE",            "AcDbScale",               "ObjectDBX Classes", 1153, 0},
        {"DICTIONARYVAR",    "AcDbDictionaryVar",       "ObjectDBX Classes", 0, 0},
        {"RASTERVARIABLES",  "AcDbRasterVariables",     "ISM",               0, 0},
        {"ACDBRASTERVARIABLES", "AcDbRasterVariables",  "ISM",               0, 0},
        {"WIPEOUTVARIABLES", "AcDbWipeoutVariables",    "WipeOut",           0, 0},
        {"ACDBWIPEOUTVARIABLES", "AcDbWipeoutVariables", "WipeOut",          0, 0},
        {"PDFDEFINITION",    "AcDbPdfDefinition",       "ObjectDBX Classes", 1153, 0},
        {"ACDBPDFDEFINITION", "AcDbPdfDefinition",      "ObjectDBX Classes", 1153, 0},
        {"DGNDEFINITION",    "AcDbDgnDefinition",       "ObjectDBX Classes", 1153, 0},
        {"ACDBDGNDEFINITION", "AcDbDgnDefinition",     "ObjectDBX Classes", 1153, 0},
        {"DWFDEFINITION",    "AcDbDwfDefinition",       "ObjectDBX Classes", 1153, 0},
        {"ACDBDWFDEFINITION", "AcDbDwfDefinition",     "ObjectDBX Classes", 1153, 0},
        // Common unmodeled custom OBJECTS that reach the raw net verbatim.
        {"MATERIAL",         "AcDbMaterial",            "ObjectDBX Classes", 1153, 0},
        {"SOLIDBACKGROUND",  "AcDbSolidBackground",     "SCENEOE",           1153, 0},
        {"SOLID_BACKGROUND", "AcDbSolidBackground",     "SCENEOE",           1153, 0},
        {"GRADIENTBACKGROUND", "AcDbGradientBackground", "SCENEOE",         1153, 0},
        {"GRADIENT_BACKGROUND", "AcDbGradientBackground", "SCENEOE",       1153, 0},
        {"GROUNDPLANEBACKGROUND", "AcDbGroundPlaneBackground", "SCENEOE", 1153, 0},
        {"GROUND_PLANE_BACKGROUND", "AcDbGroundPlaneBackground", "SCENEOE", 1153, 0},
        {"IMAGEBACKGROUND",   "AcDbImageBackground",     "SCENEOE",           1153, 0},
        {"IMAGE_BACKGROUND",  "AcDbImageBackground",     "SCENEOE",           1153, 0},
        {"IBLBACKGROUND",     "AcDbIBLBackground",      "SCENEOE",           1153, 0},
        {"IBL_BACKGROUND",    "AcDbIBLBackground",      "SCENEOE",           1153, 0},
        {"SKYLIGHTBACKGROUND", "AcDbSkyBackground",      "SCENEOE",           1153, 0},
        {"SKYLIGHT_BACKGROUND", "AcDbSkyBackground",    "SCENEOE",           1153, 0},
        {"SUNSTUDY",          "AcDbSunStudy",            "SCENEOE",           1153, 0},
        {"ACDBSUNSTUDY",      "AcDbSunStudy",            "SCENEOE",           1153, 0},
        {"MOTIONPATH",        "AcDbMotionPath",          "ACTION",            1153, 0},
        {"ACDBMOTIONPATH",    "AcDbMotionPath",          "ACTION",            1153, 0},
        {"CURVEPATH",         "AcDbCurvePath",           "ACTION",            1153, 0},
        {"ACDBCURVEPATH",     "AcDbCurvePath",           "ACTION",            1153, 0},
        {"POINTPATH",         "AcDbPointPath",           "ACTION",            1153, 0},
        {"ACDBPOINTPATH",     "AcDbPointPath",           "ACTION",            1153, 0},
        {"OBJECT_PTR",        "AcDbObjectPtr",           "ObjectDBX Classes", 1153, 0},
        {"OBJECTPTR",         "AcDbObjectPtr",           "ObjectDBX Classes", 1153, 0},
        {"ACDBOBJECTPTR",     "AcDbObjectPtr",           "ObjectDBX Classes", 1153, 0},
        {"PARTIAL_VIEWING_INDEX", "AcDbPartialViewingIndex", "ObjectDBX Classes", 1153, 0},
        {"PARTIALVIEWINGINDEX", "AcDbPartialViewingIndex", "ObjectDBX Classes", 1153, 0},
        {"ACDBPARTIALVIEWINGINDEX", "AcDbPartialViewingIndex", "ObjectDBX Classes", 1153, 0},
        {"RENDERSETTINGS",    "AcDbRenderSettings",      "SCENEOE",           1153, 0},
        {"ACDBRENDERSETTINGS", "AcDbRenderSettings",     "SCENEOE",           1153, 0},
        {"RENDERGLOBAL",      "AcDbRenderGlobal",        "SCENEOE",           1153, 0},
        {"ACDBRENDERGLOBAL",  "AcDbRenderGlobal",        "SCENEOE",           1153, 0},
        {"RENDERENVIRONMENT", "AcDbRenderEnvironment",   "SCENEOE",           1153, 0},
        {"ACDBRENDERENVIRONMENT", "AcDbRenderEnvironment", "SCENEOE",       1153, 0},
        {"RENDERENTRY",       "AcDbRenderEntry",         "SCENEOE",           1153, 0},
        {"ACDBRENDERENTRY",   "AcDbRenderEntry",         "SCENEOE",           1153, 0},
        {"RAPIDRTRENDERSETTINGS", "AcDbRapidRTRenderSettings", "ObjectDBX Classes", 1153, 0},
        {"ACDBRAPIDRTRENDERSETTINGS", "AcDbRapidRTRenderSettings", "ObjectDBX Classes", 1153, 0},
        {"MENTALRAYRENDERSETTINGS", "AcDbMentalRayRenderSettings", "SCENEOE", 1153, 0},
        {"ACDBMENTALRAYRENDERSETTINGS", "AcDbMentalRayRenderSettings", "SCENEOE", 1153, 0},
        {"SECTIONMANAGER",    "AcDbSectionManager",      "ObjectDBX Classes", 1153, 0},
        {"ACDBSECTIONMANAGER", "AcDbSectionManager",     "ObjectDBX Classes", 1153, 0},
        {"SECTION_MANAGER",   "AcDbSectionManager",      "ObjectDBX Classes", 1153, 0},
        {"SECTIONSETTINGS",   "AcDbSectionSettings",     "ObjectDBX Classes", 1153, 0},
        {"ACDBSECTIONSETTINGS", "AcDbSectionSettings",    "ObjectDBX Classes", 1153, 0},
        {"SECTION_SETTINGS",  "AcDbSectionSettings",     "ObjectDBX Classes", 1153, 0},
        {"DBCOLOR",           "AcDbColor",               "ObjectDBX Classes", 1153, 0},
        {"ACDBCOLOR",         "AcDbColor",               "ObjectDBX Classes", 1153, 0},
        {"EVALUATION_GRAPH",  "AcDbEvalGraph",           "ObjectDBX Classes", 1153, 0},
        {"EVALUATIONGRAPH",   "AcDbEvalGraph",           "ObjectDBX Classes", 1153, 0},
        {"ACDBEVALGRAPH",     "AcDbEvalGraph",           "ObjectDBX Classes", 1153, 0},
        {"ACAD_EVALUATION_GRAPH", "AcDbEvalGraph",       "ObjectDBX Classes", 1153, 0},
        {"VISUALSTYLE",      "AcDbVisualStyle",         "ObjectDBX Classes", 4095, 0},
        {"ACDBVISUALSTYLE",  "AcDbVisualStyle",         "ObjectDBX Classes", 4095, 0},
        {"ACDB_VISUALSTYLE_CLASS", "AcDbVisualStyle",   "ObjectDBX Classes", 4095, 0},
        {"TABLESTYLE",       "AcDbTableStyle",          "ObjectDBX Classes", 4095, 0},
        {"MLEADERSTYLE",     "AcDbMLeaderStyle", "ACDB_MLEADERSTYLE_CLASS", 4095, 0},
        {"ACDBDETAILVIEWSTYLE",  "AcDbDetailViewStyle",  "ObjectDBX Classes", 1025, 0},
        {"DETAILVIEWSTYLE",      "AcDbDetailViewStyle",  "ObjectDBX Classes", 1025, 0},
        {"ACDBSECTIONVIEWSTYLE", "AcDbSectionViewStyle", "ObjectDBX Classes", 1025, 0},
        {"SECTIONVIEWSTYLE",     "AcDbSectionViewStyle", "ObjectDBX Classes", 1025, 0},
        {"ACDBPLACEHOLDER",  "AcDbPlaceHolder",         "ObjectDBX Classes", 0, 0},
        // Action-history and dynamic-block classes confirmed by
        // libreDWG's dxfclasses.in.  These are emitted by the raw shell path
        // and therefore need a matching CLASSES entry for strict readers.
        {"ACSH_BOOLEAN_CLASS", "AcDbShBoolean", "ObjectDBX Classes", 1153, 0},
        {"ACSH_BOX_CLASS", "AcDbShBox", "ObjectDBX Classes", 1153, 0},
        {"ACSH_BREP_CLASS", "AcDbShBrep", "ObjectDBX Classes", 1153, 0},
        {"ACSH_CHAMFER_CLASS", "AcDbShChamfer", "ObjectDBX Classes", 1153, 0},
        {"ACSH_CONE_CLASS", "AcDbShCone", "ObjectDBX Classes", 1153, 0},
        {"ACSH_CYLINDER_CLASS", "AcDbShCylinder", "ObjectDBX Classes", 1153, 0},
        {"ACSH_EXTRUSION_CLASS", "AcDbShExtrusion", "ObjectDBX Classes", 1153, 0},
        {"ACSH_FILLET_CLASS", "AcDbShFillet", "ObjectDBX Classes", 1153, 0},
        {"ACSH_HISTORY_CLASS", "AcDbShHistory", "ObjectDBX Classes", 1153, 0},
        {"ACSH_LOFT_CLASS", "AcDbShLoft", "ObjectDBX Classes", 1153, 0},
        {"ACSH_PYRAMID_CLASS", "AcDbShPyramid", "ObjectDBX Classes", 1153, 0},
        {"ACSH_REVOLVE_CLASS", "AcDbShRevolve", "ObjectDBX Classes", 1153, 0},
        {"ACSH_SPHERE_CLASS", "AcDbShSphere", "ObjectDBX Classes", 1153, 0},
        {"ACSH_SWEEP_CLASS", "AcDbShSweep", "ObjectDBX Classes", 1153, 0},
        {"ACSH_TORUS_CLASS", "AcDbShTorus", "ObjectDBX Classes", 1153, 0},
        {"ACSH_WEDGE_CLASS", "AcDbShWedge", "ObjectDBX Classes", 1153, 0},
        {"ACDBBLOCKPARAMDEPENDENCYBODY", "AcDbBlockParameterDependencyBody", "ObjectDBX Classes", 1153, 0},
        {"ACDB_BLOCKREPRESENTATION_DATA", "AcDbBlockRepresentationData", "ObjectDBX Classes", 1153, 0},
        {"BLOCKARRAYACTION", "AcDbBlockArrayAction", "ObjectDBX Classes", 1153, 0},
        {"BLOCKANGULARCONSTRAINTPARAMETER", "AcDbBlockAngularConstraintParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKDIAMETRICCONSTRAINTPARAMETER", "AcDbBlockDiametricConstraintParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKRADIALCONSTRAINTPARAMETER", "AcDbBlockRadialConstraintParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKALIGNEDCONSTRAINTPARAMETER", "AcDbBlockAlignedConstraintParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKLINEARCONSTRAINTPARAMETER", "AcDbBlockLinearConstraintParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKHORIZONTALCONSTRAINTPARAMETER", "AcDbBlockHorizontalConstraintParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKVERTICALCONSTRAINTPARAMETER", "AcDbBlockVerticalConstraintParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKMOVEACTION", "AcDbBlockMoveAction", "ObjectDBX Classes", 1153, 0},
        {"BLOCKFLIPACTION", "AcDbBlockFlipAction", "ObjectDBX Classes", 1153, 0},
        {"BLOCKLOOKUPACTION", "AcDbBlockLookupAction", "ObjectDBX Classes", 1153, 0},
        {"BLOCKROTATEACTION", "AcDbBlockRotateAction", "ObjectDBX Classes", 1153, 0},
        {"BLOCKSCALEACTION", "AcDbBlockScaleAction", "ObjectDBX Classes", 1153, 0},
        {"BLOCKPOLARSTRETCHACTION", "AcDbBlockPolarStretchAction", "ObjectDBX Classes", 1153, 0},
        {"BLOCKSTRETCHACTION", "AcDbBlockStretchAction", "ObjectDBX Classes", 1153, 0},
        {"BLOCKLOOKUPPARAMETER", "AcDbBlockLookupParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKPOINTPARAMETER", "AcDbBlockPointParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKROTATIONPARAMETER", "AcDbBlockRotationParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKPOLARPARAMETER", "AcDbBlockPolarParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKPOLARGRIP", "AcDbBlockPolarGrip", "ObjectDBX Classes", 1153, 0},
        {"BLOCKROTATIONGRIP", "AcDbBlockRotationGrip", "ObjectDBX Classes", 1153, 0},
        {"BLOCKVISIBILITYGRIP", "AcDbBlockVisibilityGrip", "ObjectDBX Classes", 1153, 0},
        {"BLOCKXYGRIP", "AcDbBlockXYGrip", "ObjectDBX Classes", 1153, 0},
        {"BLOCKGRIPLOCATIONCOMPONENT", "AcDbBlockGripExpr", "ObjectDBX Classes", 1153, 0},
        {"BLOCKALIGNMENTGRIP", "AcDbBlockAlignmentGrip", "ObjectDBX Classes", 1153, 0},
        {"BLOCKALIGNMENTPARAMETER", "AcDbBlockAlignmentParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKLINEARPARAMETER", "AcDbBlockLinearParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKBASEPOINTPARAMETER", "AcDbBlockBasepointParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKFLIPPARAMETER", "AcDbBlockFlipParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKFLIPGRIP", "AcDbBlockFlipGrip", "ObjectDBX Classes", 1153, 0},
        {"BLOCKLINEARGRIP", "AcDbBlockLinearGrip", "ObjectDBX Classes", 1153, 0},
        {"BLOCKLOOKUPGRIP", "AcDbBlockLookupGrip", "ObjectDBX Classes", 1153, 0},
        {"BLOCKUSERPARAMETER", "AcDbBlockUserParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKXYPARAMETER", "AcDbBlockXYParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKVISIBILITYPARAMETER", "AcDbBlockVisibilityParameter", "ObjectDBX Classes", 1153, 0},
        {"BLOCKPROPERTIESTABLE", "AcDbBlockPropertiesTable", "ObjectDBX Classes", 1153, 0},
        {"BLOCKPROPERTIESTABLEGRIP", "AcDbBlockPropertiesTableGrip", "ObjectDBX Classes", 1153, 0},
        {"ACDB_DYNAMICBLOCKPROXYNODE", "AcDbDynamicBlockProxyNode", "ObjectDBX Classes", 1153, 0},
        {"ACDB_DYNAMICBLOCKPURGEPREVENTER_VERSION", "AcDbDynamicBlockPurgePreventer", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCPERSSUBENTMANAGER", "AcDbAssocPersSubentManager", "ObjectDBX Classes", 1153, 0},
        {"ACDBPERSSUBENTMANAGER", "AcDbPersSubentManager", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCNETWORK", "AcDbAssocNetwork", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCACTION", "AcDbAssocAction", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCALIGNEDDIMACTIONBODY", "AcDbAssocAlignedDimActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCOSNAPPOINTREFACTIONPARAM", "AcDbAssocOsnapPointRefActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCVERTEXACTIONPARAM", "AcDbAssocVertexActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCGEOMDEPENDENCY", "AcDbAssocGeomDependency", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCDEPENDENCY", "AcDbAssocDependency", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCDIMDEPENDENCYBODY", "AcDbAssocDimDependencyBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOC3POINTANGULARDIMACTIONBODY", "AcDbAssoc3PointAngularDimActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCALIGNEDIMACTIONBODY", "AcDbAssocAlignedDimActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCORDINATEDIMACTIONBODY", "AcDbAssocOrdinatedDimActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCROTATEDDIMACTIONBODY", "AcDbAssocRotatedDimActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCARRAYMODIFYACTIONBODY", "AcDbAssocArrayModifyActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCARRAYACTIONBODY", "AcDbAssocArrayActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCEDGECHAMFERACTIONBODY", "AcDbAssocEdgeChamferActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCEDGEFILLETACTIONBODY", "AcDbAssocEdgeFilletActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCMLEADERACTIONBODY", "AcDbAssocMLeaderActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCBLENDSURFACEACTIONBODY", "AcDbAssocBlendSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCEXTENDSURFACEACTIONBODY", "AcDbAssocExtendSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCEXTRUDEDSURFACEACTIONBODY", "AcDbAssocExtrudedSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCFILLETSURFACEACTIONBODY", "AcDbAssocFilletSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCLOFTEDSURFACEACTIONBODY", "AcDbAssocLoftedSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCNETWORKSURFACEACTIONBODY", "AcDbAssocNetworkSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCOFFSETSURFACEACTIONBODY", "AcDbAssocOffsetSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCPLANESURFACEACTIONBODY", "AcDbAssocPlaneSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCPATCHSURFACEACTIONBODY", "AcDbAssocPatchSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCRESTOREENTITYSTATEACTIONBODY", "AcDbAssocRestoreEntityStateActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCREVOLVEDSURFACEACTIONBODY", "AcDbAssocRevolvedSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCSWEPTSURFACEACTIONBODY", "AcDbAssocSweptSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCTRIMSURFACEACTIONBODY", "AcDbAssocTrimSurfaceActionBody", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCACTIONPARAM", "AcDbAssocActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCASMBODYACTIONPARAM", "AcDbAssocAsmbodyActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCCOMPOUNDACTIONPARAM", "AcDbAssocCompoundActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCEDGEACTIONPARAM", "AcDbAssocEdgeActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCFACEACTIONPARAM", "AcDbAssocFaceActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCPATHACTIONPARAM", "AcDbAssocPathActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCOBJECTACTIONPARAM", "AcDbAssocObjectActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCPOINTREFACTIONPARAM", "AcDbAssocPointRefActionParam", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCARRAYMODIFYPARAMETERS", "AcDbAssocArrayModifyParameters", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCARRAYPATHPARAMETERS", "AcDbAssocArrayPathParameters", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCARRAYPOLARPARAMETERS", "AcDbAssocArrayPolarParameters", "ObjectDBX Classes", 1153, 0},
        {"ACDBASSOCARRAYRECTANGULARPARAMETERS", "AcDbAssocArrayRectangularParameters", "ObjectDBX Classes", 1153, 0},
        // Canonical ODBX custom-object classes used by annotation contexts
        // and proxy/VBA preservation. These records are raw-preserved when
        // no typed DXF body parser is available, but strict consumers still
        // require their exact CLASSES metadata.
        {"CONTEXTDATAMANAGER", "AcDbContextDataManager", "ObjectDBX Classes", 0, 0},
        {"VBA_PROJECT", "AcDbVbaProject", "ObjectDBX Classes", 0, 0},
        {"ACAD_PROXY_ENTITY_WRAPPER", "AcDbProxyEntityWrapper", "ObjectDBX Classes", 0, 0},
        {"ACAD_PROXY_OBJECT_WRAPPER", "AcDbProxyObjectWrapper", "ObjectDBX Classes", 0, 0},
        {"ACDB_ALDIMOBJECTCONTEXTDATA_CLASS", "AcDbAlignedDimensionObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_ANGDIMOBJECTCONTEXTDATA_CLASS", "AcDbAngularDimensionObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_ANNOTSCALEOBJECTCONTEXTDATA_CLASS", "AcDbAnnotScaleObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_BLKREFOBJECTCONTEXTDATA_CLASS", "AcDbBlkrefObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_DMDIMOBJECTCONTEXTDATA_CLASS", "AcDbDiametricDimensionObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_FCFOBJECTCONTEXTDATA_CLASS", "AcDbFcfObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_LEADEROBJECTCONTEXTDATA_CLASS", "AcDbLeaderObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_MLEADEROBJECTCONTEXTDATA_CLASS", "AcDbMLeaderObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_ORDDIMOBJECTCONTEXTDATA_CLASS", "AcDbOrdinateDimensionObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_RADIMLGOBJECTCONTEXTDATA_CLASS", "AcDbRadialDimensionLargeObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_RADIMOBJECTCONTEXTDATA_CLASS", "AcDbRadialDimensionObjectContextData", "ObjectDBX Classes", 0, 0},
        {"ACDB_TEXTOBJECTCONTEXTDATA_CLASS", "AcDbTextObjectContextData", "ObjectDBX Classes", 0, 0},
        {"CELLSTYLEMAP",     "AcDbCellStyleMap",        "ObjectDBX Classes", 1152, 0},
        {"FIELD",            "AcDbField",               "ObjectDBX Classes", 1152, 0},
        {"FIELDLIST",        "AcDbFieldList",           "ObjectDBX Classes", 1152, 0},
        {"GEODATA",          "AcDbGeoData",             "ObjectDBX Classes", 4095, 0},
        {"ACDBGEODATA",      "AcDbGeoData",             "ObjectDBX Classes", 4095, 0},
        {"SPATIAL_FILTER",   "AcDbSpatialFilter",       "ObjectDBX Classes", 0, 0},
        {"SPATIALFILTER",    "AcDbSpatialFilter",       "ObjectDBX Classes", 0, 0},
        {"SORTENTSTABLE",    "AcDbSortentsTable",       "ObjectDBX Classes", 0, 0},
        {"IDBUFFER",         "AcDbIdBuffer",            "ObjectDBX Classes", 0, 0},
        {"ACDBIDBUFFER",     "AcDbIdBuffer",            "ObjectDBX Classes", 0, 0},
        {"LAYER_INDEX",      "AcDbLayerIndex",          "ObjectDBX Classes", 0, 0},
        {"LAYERINDEX",       "AcDbLayerIndex",          "ObjectDBX Classes", 0, 0},
        {"ACDBLAYERINDEX",   "AcDbLayerIndex",          "ObjectDBX Classes", 0, 0},
        {"SPATIAL_INDEX",    "AcDbSpatialIndex",        "ObjectDBX Classes", 0, 0},
        {"SPATIALINDEX",     "AcDbSpatialIndex",        "ObjectDBX Classes", 0, 0},
        {"ACDBSPATIALINDEX", "AcDbSpatialIndex",        "ObjectDBX Classes", 0, 0},
        {"LAYER_FILTER",     "AcDbLayerFilter",         "ObjectDBX Classes", 0, 0},
        {"LAYERFILTER",      "AcDbLayerFilter",         "ObjectDBX Classes", 0, 0},
        {"DICTIONARYWDFLT",  "AcDbDictionaryWithDefault", "ObjectDBX Classes", 0, 0},
        {"ACDBDICTIONARYWDFLT", "AcDbDictionaryWithDefault", "ObjectDBX Classes", 0, 0},
        {"DIMASSOC",         "AcDbDimAssoc",            "AcDbDimAssoc",      0, 0},
        // Custom ENTITIES (isEntity=1). Typed direct writers and raw-net replay
        // both need these CLASS records; without them AutoCAD/ODA prune the
        // entities on load.
        {"ACAD_TABLE",       "AcDbTable",               "ObjectDBX Classes", 1025, 1},
        {"HELIX",            "AcDbHelix",               "ObjectDBX Classes", 4095, 1},
        {"MESH",             "AcDbSubDMesh",            "SCENEOE",           1025, 1},
        {"RTEXT",            "AcDbRText",               "EXPRESS",           1025, 1},
        {"ARCALIGNEDTEXT",   "AcDbArcAlignedText",      "EXPRESS",           1025, 1},
        {"MPOLYGON",         "AcDbMPolygon",            "AcMPolygonObj15",   1025, 1},
        {"LARGE_RADIAL_DIMENSION", "AcDbRadialDimensionLarge", "ACAD",       1025, 1},
        {"PDFUNDERLAY",      "AcDbPdfReference",        "ObjectDBX Classes", 4095, 1},
        {"DGNUNDERLAY",      "AcDbDgnReference",        "ObjectDBX Classes", 4095, 1},
        {"DWFUNDERLAY",      "AcDbDwfReference",        "ObjectDBX Classes", 4095, 1},
        {"SURFACE",          "AcDbSurface",             "ObjectDBX Classes", 4095, 1},
        {"EXTRUDEDSURFACE",  "AcDbExtrudedSurface",     "ObjectDBX Classes", 4095, 1},
        {"LOFTEDSURFACE",    "AcDbLoftedSurface",       "ObjectDBX Classes", 0, 1},
        {"REVOLVEDSURFACE",  "AcDbRevolvedSurface",     "ObjectDBX Classes", 0, 1},
        {"SWEPTSURFACE",     "AcDbSweptSurface",        "ObjectDBX Classes", 0, 1},
        {"PLANESURFACE",     "AcDbPlaneSurface",        "ObjectDBX Classes", 4095, 1},
        {"NURBSSURFACE",     "AcDbNurbSurface",         "ObjectDBX Classes", 4095, 1},
        {"POINTCLOUD",       "AcDbPointCloud",          "ObjectDBX Classes", 4095, 1},
        {"POINTCLOUDEX",     "AcDbPointCloudEx",        "ObjectDBX Classes", 4095, 1},
        {"NAVISWORKSMODEL",  "AcDbNavisworksModel",     "ACTION",           4095, 1},
        {"POINTCLOUDDEFINITION", "AcDbPointCloudDef", "ObjectDBX Classes", 4095, 0},
        {"ACDBPOINTCLOUDDEF", "AcDbPointCloudDef", "ObjectDBX Classes", 4095, 0},
        {"POINTCLOUDDEFINITIONEX", "AcDbPointCloudDefEx", "ObjectDBX Classes", 4095, 0},
        {"ACDBPOINTCLOUDDEFEX", "AcDbPointCloudDefEx", "ObjectDBX Classes", 4095, 0},
        {"POINTCLOUDDEFREACTOR", "AcDbPointCloudDefReactor", "ObjectDBX Classes", 4095, 0},
        {"ACDBPOINTCLOUDDEFREACTOR", "AcDbPointCloudDefReactor", "ObjectDBX Classes", 4095, 0},
        {"POINTCLOUDDEFREACTOREX", "AcDbPointCloudDefReactorEx", "ObjectDBX Classes", 4095, 0},
        {"ACDBPOINTCLOUDDEFREACTOREX", "AcDbPointCloudDefReactorEx", "ObjectDBX Classes", 4095, 0},
        {"NAVISWORKSMODELDEF", "AcDbNavisworksModelDef", "ObjectDBX Classes", 4095, 0},
        {"POINTCLOUDCOLORMAP", "AcDbPointCloudColorMap", "ObjectDBX Classes", 4095, 0},
        {"ACDBPOINTCLOUDCOLORMAP", "AcDbPointCloudColorMap", "ObjectDBX Classes", 4095, 0},
    };
    for (const Entry &e : table) {
        if (recName == e.rec) {
            out.recName = e.rec;
            out.className = e.cls;
            out.appName = e.app;
            out.proxyFlag = e.flag;
            out.wasaProxyFlag = 0;
            out.entityFlag = e.isEntity;
            out.instanceCount = 0;
            return true;
        }
    }
    return false;
}

bool dxfRW::writePlotSettings(DRW_PlotSettings *ent) {
    if (!preflightTableEntry(ent))
        return false;
    std::uint32_t handle = static_cast<std::uint32_t>(ent->handle);
    if (handle != 0) {
        if (!reserveHandle(handle))
            return false;
    } else if (!allocateDxfHandle(handle)) {
        return false;
    }

    bool result = true;
    const auto write = [&result](bool ok) { result = ok && result; };
    write(writer->writeString(0, "PLOTSETTINGS"));
    write(writer->writeString(5, toHexStr(handle)));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    write(writer->writeString(100, "AcDbPlotSettings"));
    writePlotSettingsFields(ent);
    if (ent->shadePlotHandle.ref != 0)
        write(writer->writeString(333, toHexStr(ent->shadePlotHandle.ref)));
    if (!writeTableEntryAppData(*ent))
        result = false;
    if (!ent->extData.empty())
        result = writeExtData(ent->extData) && result;
    if (!result || writer->hasWriteError())
        m_writeError = true;
    return result && !writer->hasWriteError();
}

void dxfRW::writePlotSettingsFields(const DRW_PlotSettings *ent) {
    // Full AcDbPlotSettings field set in ezdxf layout.py order. Previously only
    // 6/40/41/42/43 were emitted, so page size, margins, plot window, scale,
    // rotation, units and shade-plot settings were all lost on export.
    writer->writeUtf8String(1, ent->pageSetupName);
    writer->writeUtf8String(2, ent->printerConfig);
    writer->writeUtf8String(4, ent->paperSize);
    writer->writeUtf8String(6, ent->plotViewName);
    writer->writeDouble(40, ent->marginLeft);
    writer->writeDouble(41, ent->marginBottom);
    writer->writeDouble(42, ent->marginRight);
    writer->writeDouble(43, ent->marginTop);
    writer->writeDouble(44, ent->paperWidth);
    writer->writeDouble(45, ent->paperHeight);
    writer->writeDouble(46, ent->plotOriginX);
    writer->writeDouble(47, ent->plotOriginY);
    writer->writeDouble(48, ent->windowMinX);
    writer->writeDouble(49, ent->windowMinY);
    writer->writeDouble(140, ent->windowMaxX);
    writer->writeDouble(141, ent->windowMaxY);
    writer->writeDouble(142, ent->realWorldUnits);
    writer->writeDouble(143, ent->drawingUnits);
    writer->writeInt16(70, ent->plotLayoutFlags);
    writer->writeInt16(72, ent->paperUnits);
    writer->writeInt16(73, ent->plotRotation);
    writer->writeInt16(74, ent->plotType);
    writer->writeUtf8String(7, ent->currentStyleSheet);
    writer->writeInt16(75, ent->scaleType);
    if (version > DRW::AC1015) {   // shade-plot settings are R2004+
        writer->writeInt16(76, ent->shadePlotMode);
        writer->writeInt16(77, ent->shadePlotResLevel);
        writer->writeInt16(78, ent->shadePlotCustomDPI);
    }
    writer->writeDouble(147, ent->scaleFactor);
    writer->writeDouble(148, ent->paperImageOriginX);
    writer->writeDouble(149, ent->paperImageOriginY);
}

// LAYOUT is a fixed built-in OBJECT. It embeds the same AcDbPlotSettings prefix
// as PLOTSETTINGS, followed by AcDbLayout fields.
bool dxfRW::writeLayout(DRW_Layout *ent) {
    if (!preflightTableEntry(ent))
        return false;
    std::uint32_t handle = static_cast<std::uint32_t>(ent->handle);
    if (handle != 0) {
        if (!reserveHandle(handle))
            return false;
    } else if (!allocateDxfHandle(handle)) {
        return false;
    }
    writer->writeString(0, "LAYOUT");
    writer->writeString(5, toHexStr(handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbPlotSettings");
    DRW_PlotSettings plotSettings;
    plotSettings.pageSetupName = ent->pageSetupName;
    plotSettings.printerConfig = ent->printerConfig;
    plotSettings.plotLayoutFlags = ent->plotLayoutFlags;
    plotSettings.marginLeft = ent->marginLeft;
    plotSettings.marginBottom = ent->marginBottom;
    plotSettings.marginRight = ent->marginRight;
    plotSettings.marginTop = ent->marginTop;
    plotSettings.paperWidth = ent->paperWidth;
    plotSettings.paperHeight = ent->paperHeight;
    plotSettings.paperSize = ent->paperSize;
    plotSettings.plotOriginX = ent->plotOriginX;
    plotSettings.plotOriginY = ent->plotOriginY;
    plotSettings.paperUnits = ent->paperUnits;
    plotSettings.plotRotation = ent->plotRotation;
    plotSettings.plotType = ent->plotType;
    plotSettings.windowMinX = ent->windowMinX;
    plotSettings.windowMinY = ent->windowMinY;
    plotSettings.windowMaxX = ent->windowMaxX;
    plotSettings.windowMaxY = ent->windowMaxY;
    plotSettings.plotViewName = ent->plotViewName;
    plotSettings.realWorldUnits = ent->realWorldUnits;
    plotSettings.drawingUnits = ent->drawingUnits;
    plotSettings.currentStyleSheet = ent->currentStyleSheet;
    plotSettings.scaleType = ent->scaleType;
    plotSettings.scaleFactor = ent->scaleFactor;
    plotSettings.paperImageOriginX = ent->paperImageOriginX;
    plotSettings.paperImageOriginY = ent->paperImageOriginY;
    plotSettings.shadePlotMode = ent->shadePlotMode;
    plotSettings.shadePlotResLevel = ent->shadePlotResLevel;
    plotSettings.shadePlotCustomDPI = ent->shadePlotCustomDPI;
    writePlotSettingsFields(&plotSettings);

    writer->writeString(100, "AcDbLayout");
    writer->writeUtf8String(1, ent->name);
    writer->writeInt16(70, ent->layoutFlags);
    writer->writeInt32(71, ent->tabOrder);
    writer->writeDouble(10, ent->limMinX);
    writer->writeDouble(20, ent->limMinY);
    writer->writeDouble(11, ent->limMaxX);
    writer->writeDouble(21, ent->limMaxY);
    writer->writeDouble(12, ent->insPoint.x);
    writer->writeDouble(22, ent->insPoint.y);
    writer->writeDouble(32, ent->insPoint.z);
    writer->writeDouble(14, ent->extMin.x);
    writer->writeDouble(24, ent->extMin.y);
    writer->writeDouble(34, ent->extMin.z);
    writer->writeDouble(15, ent->extMax.x);
    writer->writeDouble(25, ent->extMax.y);
    writer->writeDouble(35, ent->extMax.z);
    writer->writeDouble(146, ent->elevation);
    writer->writeDouble(13, ent->ucsOrigin.x);
    writer->writeDouble(23, ent->ucsOrigin.y);
    writer->writeDouble(33, ent->ucsOrigin.z);
    writer->writeDouble(16, ent->ucsXAxis.x);
    writer->writeDouble(26, ent->ucsXAxis.y);
    writer->writeDouble(36, ent->ucsXAxis.z);
    writer->writeDouble(17, ent->ucsYAxis.x);
    writer->writeDouble(27, ent->ucsYAxis.y);
    writer->writeDouble(37, ent->ucsYAxis.z);
    writer->writeInt16(76, ent->orthoViewType);
    if (ent->shadePlotHandle.ref != 0)
        writer->writeString(333, toHexStr(ent->shadePlotHandle.ref));
    if (ent->paperSpaceBlockRecordHandle.ref != 0)
        writer->writeString(
            330, toHexStr(ent->paperSpaceBlockRecordHandle.ref));
    if (ent->lastActiveViewportHandle.ref != 0)
        writer->writeString(
            331, toHexStr(ent->lastActiveViewportHandle.ref));
    if (ent->namedUcsHandle.ref != 0)
        writer->writeString(
            345, toHexStr(ent->namedUcsHandle.ref));
    if (ent->baseUcsHandle.ref != 0)
        writer->writeString(
            346, toHexStr(ent->baseUcsHandle.ref));
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

//F4: typed DXF emitters for the routed data-only OBJECTS that the DWG reader
//populates only into typed metadata (NOT the DXF raw net). On DWG->DXF the filter
//pulls each from dwgAdvancedMetadata().suns()/scales()/dictionaryVars()/
//rasterVariables() and calls these so the object is present in the output (DXF->DXF
//already preserves them via the raw net; the filter dedups by handle to avoid a
//double-emit). The group-code shape is the inverse of each type's parseCode
//(DRW_Sun/Scale/DictionaryVar/RasterVariables::parseCode in drw_objects.cpp),
//cross-checked field-for-field against ezdxf 1.4.4. Each emits the verbatim
//code-5 handle (reserved by the filter's pre-write pass) and a 330 owner (the
//record's parentHandle when known, else root dict "C" to avoid an ownerless
//prune). The matching CLASS record is registered by the filter via
//dxfClassForRecordName.

//helper: emit 330 owner as a hex handle (record parentHandle when nonzero, else
//root dict "C" so the object is reachable and not pruned as an orphan).
void dxfRW::writeObjectOwner(std::uint32_t parentHandle) {
    if (writer == nullptr) {
        m_writeError = true;
        return;
    }
    if (version <= DRW::AC1014)
        return;  //pre-R2000 DXF has no 330 owner handles in OBJECTS
    if (parentHandle != 0)
        writer->writeString(330, toHexStr(parentHandle));
    else
        writer->writeString(330, "C");
}

bool dxfRW::writeSun(DRW_Sun *ent) {
    if (!preflightTableEntry(ent))
        return false;
    DxfWriterRecordScope record(*writer);
    writer->writeString(0, "SUN");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbSun");
    writer->writeInt32(90, static_cast<int>(ent->m_classVersion));
    writer->writeBool(290, ent->m_isOn);
    writer->writeInt16(63, static_cast<int>(ent->m_color));
    if (version > DRW::AC1015 && ent->m_color24 >= 0)
        writer->writeInt32(421, ent->m_color24);  // 24-bit true color (R2004+)
    writer->writeDouble(40, ent->m_intensity);
    writer->writeBool(291, ent->m_hasShadow);
    writer->writeInt32(91, ent->m_julianDay);
    writer->writeInt32(92, ent->m_milliseconds);
    writer->writeBool(292, ent->m_isDaylightSavings);
    writer->writeInt16(70, static_cast<int>(ent->m_shadowType));
    writer->writeInt16(71, static_cast<int>(ent->m_shadowMapSize));
    writer->writeInt16(280, static_cast<int>(ent->m_shadowSoftness));
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    if (writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeScale(DRW_Scale *ent) {
    if (!preflightTableEntry(ent))
        return false;
    DxfWriterRecordScope record(*writer);
    writer->writeString(0, "SCALE");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbScale");
    writer->writeInt16(70, static_cast<int>(ent->flag));
    writer->writeUtf8String(300, ent->name);
    writer->writeDouble(140, ent->paperUnits);
    writer->writeDouble(141, ent->drawingUnits);
    writer->writeBool(290, ent->isUnitScale);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    if (writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeDictionaryVar(DRW_DictionaryVar *ent) {
    if (!preflightTableEntry(ent))
        return false;
    DxfWriterRecordScope record(*writer);
    writer->writeString(0, "DICTIONARYVAR");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    //DICTIONARYVAR uses the literal subclass marker "DictionaryVariables"
    //(NOT "AcDbDictionaryVar"); confirmed against ezdxf 1.4.4.
    writer->writeString(100, "DictionaryVariables");
    writer->writeInt16(280, static_cast<int>(ent->m_schema));
    writer->writeUtf8String(1, ent->m_value);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    if (writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeRasterVariables(DRW_RasterVariables *ent) {
    if (!preflightTableEntry(ent))
        return false;
    DxfWriterRecordScope record(*writer);
    writer->writeString(0, "RASTERVARIABLES");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbRasterVariables");
    writer->writeInt32(90, ent->m_classVersion);
    writer->writeInt16(70, ent->m_imageFrame);
    writer->writeInt16(71, ent->m_imageQuality);
    writer->writeInt16(72, ent->m_units);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    if (writer->hasWriteError()) {
        m_writeError = true;
        return false;
    }
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    return true;
}

// GEODATA (AcDbGeoData, custom class). Inverse of DRW_GeoData::parseCode for
// scalar geolocation fields and mesh point/face lists.
bool dxfRW::writeGeoData(DRW_GeoData *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "GEODATA");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbGeoData");
    writer->writeInt32(90, ent->m_version != 0 ? ent->m_version : 3);
    if (ent->m_hostBlockHandle != 0)
        writer->writeString(330, toHexStr(ent->m_hostBlockHandle));
    writer->writeInt16(70, ent->m_coordinatesType);
    writer->writeDouble(10, ent->m_designPoint.x);
    writer->writeDouble(20, ent->m_designPoint.y);
    writer->writeDouble(30, ent->m_designPoint.z);
    writer->writeDouble(11, ent->m_referencePoint.x);
    writer->writeDouble(21, ent->m_referencePoint.y);
    writer->writeDouble(31, ent->m_referencePoint.z);
    writer->writeDouble(40, ent->m_horizontalUnitScale);
    writer->writeDouble(41, ent->m_verticalUnitScale);
    writer->writeInt32(91, ent->m_horizontalUnits);
    writer->writeInt32(92, ent->m_verticalUnits);
    writer->writeDouble(210, ent->m_upDirection.x);
    writer->writeDouble(220, ent->m_upDirection.y);
    writer->writeDouble(230, ent->m_upDirection.z);
    writer->writeDouble(12, ent->m_northDirection.x);
    writer->writeDouble(22, ent->m_northDirection.y);
    writer->writeInt32(95, ent->m_scaleEstimationMethod);
    writer->writeDouble(141, ent->m_userSpecifiedScaleFactor);
    writer->writeBool(294, ent->m_enableSeaLevelCorrection);
    writer->writeDouble(142, ent->m_seaLevelElevation);
    writer->writeDouble(143, ent->m_coordinateProjectionRadius);

    std::string definition = ent->m_coordinateSystemDefinition;
    for (std::string::size_type pos = definition.find('\n');
         pos != std::string::npos;
         pos = definition.find('\n', pos + 2)) {
        definition.replace(pos, 1, "^J");
    }
    std::string::size_type offset = 0;
    while (definition.size() - offset > 255) {
        writer->writeUtf8String(303, definition.substr(offset, 255));
        offset += 255;
    }
    writer->writeUtf8String(301, definition.substr(offset));
    writer->writeUtf8String(302, ent->m_geoRssTag);
    writer->writeUtf8String(305, ent->m_observationFromTag);
    writer->writeUtf8String(306, ent->m_observationToTag);
    writer->writeUtf8String(307, ent->m_observationCoverageTag);

    writer->writeInt32(93, static_cast<int>(ent->m_points.size()));
    for (const DRW_GeoMeshPoint &point : ent->m_points) {
        writer->writeDouble(13, point.m_source.x);
        writer->writeDouble(23, point.m_source.y);
        writer->writeDouble(14, point.m_destination.x);
        writer->writeDouble(24, point.m_destination.y);
    }
    writer->writeInt32(96, static_cast<int>(ent->m_faces.size()));
    for (const DRW_GeoMeshFace &face : ent->m_faces) {
        writer->writeInt32(97, face.m_index1);
        writer->writeInt32(98, face.m_index2);
        writer->writeInt32(99, face.m_index3);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

// SPATIAL_FILTER (AcDbSpatialFilter, custom class). Emits the clip boundary,
// normal/origin, clip flags/distances, and boundary-relative transforms.
bool dxfRW::writeSpatialFilter(DRW_SpatialFilter *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "SPATIAL_FILTER");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbFilter");
    writer->writeString(100, "AcDbSpatialFilter");
    writer->writeInt16(70, static_cast<int>(ent->m_boundaryPoints.size()));
    for (const DRW_Coord &point : ent->m_boundaryPoints) {
        writer->writeDouble(10, point.x);
        writer->writeDouble(20, point.y);
    }
    writer->writeDouble(210, ent->m_normal.x);
    writer->writeDouble(220, ent->m_normal.y);
    writer->writeDouble(230, ent->m_normal.z);
    writer->writeDouble(11, ent->m_origin.x);
    writer->writeDouble(21, ent->m_origin.y);
    writer->writeDouble(31, ent->m_origin.z);
    writer->writeInt16(71, ent->m_displayBoundary ? 1 : 0);
    writer->writeInt16(72, ent->m_clipFrontPlane ? 1 : 0);
    if (ent->m_clipFrontPlane)
        writer->writeDouble(40, ent->m_frontDistance);
    writer->writeInt16(73, ent->m_clipBackPlane ? 1 : 0);
    if (ent->m_clipBackPlane)
        writer->writeDouble(41, ent->m_backDistance);

    auto writeMatrix12 = [this](const std::vector<double> &matrix) {
        for (std::size_t i = 0; i < 12; ++i)
            writer->writeDouble(40, i < matrix.size() ? matrix[i] : 0.0);
    };
    writeMatrix12(ent->m_inverseInsertTransform);
    writeMatrix12(ent->m_insertTransform);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

// SORTENTSTABLE (AcDbSortentsTable, custom class). Entity handles are remapped
// through the source->minted map when this is emitted after ENTITIES; direct unit
// writers with an empty map preserve the caller-provided handles verbatim.
bool dxfRW::writeSortEntsTable(DRW_SortEntsTable *ent) {
    if (!preflightTableEntry(ent)
        || ent->m_blockOwnerHandle == DRW::NoHandle
        || ent->m_entityHandles.size() != ent->m_sortHandles.size()
        || ent->m_entityHandles.size() > DRW_SortEntsTable::kMaxEntries
        || std::any_of(ent->m_entityHandles.cbegin(),
                       ent->m_entityHandles.cend(),
                       [](std::uint32_t handle) {
                           return handle == DRW::NoHandle;
                       }))
        return false;
    writer->writeString(0, "SORTENTSTABLE");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbSortentsTable");
    writer->writeString(330, toHexStr(ent->m_blockOwnerHandle));

    const auto &srcToMinted = m_writingContext.sourceHandleToMintedMap;
    const bool remapEntities = !srcToMinted.empty();
    auto resolveEntity = [&](std::uint32_t source, std::uint32_t &resolved) {
        if (source == 0)
            return false;
        if (!remapEntities) {
            resolved = source;
            return true;
        }
        auto it = srcToMinted.find(source);
        if (it == srcToMinted.end())
            return false;
        resolved = it->second;
        return true;
    };
    auto resolveSort = [&](std::uint32_t source, std::uint32_t &resolved) {
        if (source == 0)
            return true;
        if (!remapEntities) {
            resolved = source;
            return true;
        }
        if (m_writingContext.ambiguousSourceHandles.count(source) != 0)
            return false;
        auto it = srcToMinted.find(source);
        // Sort keys can target an OBJECTS record as well as an entity. Only
        // entity source handles are remapped by this writer, so retain a
        // non-entity key that has no entity mapping.
        resolved = it == srcToMinted.end() ? source : it->second;
        return true;
    };

    const std::size_t entryCount = ent->m_entityHandles.size();
    for (std::size_t i = 0; i < entryCount; ++i) {
        const std::uint32_t entitySource = ent->m_entityHandles[i];
        std::uint32_t entityHandle = 0;
        if (!resolveEntity(entitySource, entityHandle))
            return failDxfWrite();
        const std::uint32_t sortSource = ent->m_sortHandles[i];
        std::uint32_t sortHandle = 0;
        if (!resolveSort(sortSource, sortHandle))
            return failDxfWrite();
        writer->writeString(331, toHexStr(entityHandle));
        writer->writeString(5, toHexStr(sortHandle));
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

// FIELD (AcDbField, custom class). Emits the scalar field state, child/object
// references, cached value string, and child value records preserved by the
// typed FIELD model.
bool dxfRW::writeField(DRW_Field *ent) {
    if (!preflightTableEntry(ent))
        return false;
    if (version < DRW::AC1015 || ent == nullptr || writer == nullptr
        || !canWriteDxfField(version, *ent)) {
        m_writeError = true;
        return false;
    }

    auto writeString = [this](int code, const std::string& value) {
        return writer->writeString(code, value);
    };
    auto writeUtf8String = [this](int code, const UTF8STRING& value) {
        return writer->writeUtf8String(code, value);
    };
    bool written = writeString(0, "FIELD")
        && writeString(5, toHexStr(ent->handle));
    if (version > DRW::AC1014) {
        written = written && writeString(
            330, ent->parentHandle != 0
                ? toHexStr(static_cast<std::uint32_t>(ent->parentHandle))
                : "C");
    }
    if (!writeTableEntryAppData(*ent))
        return false;
    written = written && writeString(100, "AcDbField")
        && writeUtf8String(1, ent->m_evaluatorId)
        && writeUtf8String(2, ent->m_fieldCode);
    if (!ent->m_formatString.empty())
        written = written && writeUtf8String(4, ent->m_formatString);
    written = written && writer->writeInt32(
        90, static_cast<int>(ent->m_childHandles.size()));
    for (std::uint32_t child : ent->m_childHandles)
        written = written && writeString(360, toHexStr(child));
    written = written && writer->writeInt32(
        97, static_cast<int>(ent->m_objectHandles.size()));
    for (std::uint32_t object : ent->m_objectHandles)
        written = written && writeString(331, toHexStr(object));
    written = written
        && writer->writeInt32(91, ent->m_evaluationOptionFlags)
        && writer->writeInt32(92, ent->m_filingOptionFlags)
        && writer->writeInt32(94, ent->m_fieldStateFlags)
        && writer->writeInt32(95, ent->m_evaluationStatusFlags)
        && writer->writeInt32(96, ent->m_evaluationErrorCode)
        && writeUtf8String(300, ent->m_evaluationErrorMessage)
        && writer->writeInt32(93, static_cast<int>(ent->m_childValues.size()));

    auto writeChildValue = [this](const DRW_Field::ChildValue &child) {
        return writer->writeUtf8String(6, child.m_key)
            && writeDxfFieldValue(writer.get(), version, child.m_value);
    };
    for (const DRW_Field::ChildValue &child : ent->m_childValues)
        written = written && writeChildValue(child);

    if (dxfFieldValueHasData(ent->m_value)) {
        written = written && writeUtf8String(7, "ACFD_FIELD_VALUE")
            && writeDxfFieldValue(writer.get(), version, ent->m_value);
    }

    written = written
        && writeUtf8String(301, ent->m_valueString)
        && writer->writeInt32(98, ent->m_valueStringLength);
    if (!written || (!ent->extData.empty() && !writeExtData(ent->extData))) {
        m_writeError = true;
        return false;
    }
    return true;
}

// FIELDLIST (AcDbIdSet / AcDbFieldList, custom class).
bool dxfRW::writeFieldList(DRW_FieldList *ent) {
    if (!preflightTableEntry(ent))
        return false;
    if (version < DRW::AC1015 || ent == nullptr || writer == nullptr
        || !canWriteDxfFieldList(*ent)) {
        m_writeError = true;
        return false;
    }

    auto writeString = [this](int code, const std::string& value) {
        return writer->writeString(code, value);
    };
    bool written = writeString(0, "FIELDLIST")
        && writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    if (!written || writer->hasWriteError() || !writeTableEntryAppData(*ent))
        return false;
    written = written && writeString(100, "AcDbIdSet")
        && writer->writeInt32(90, static_cast<int>(ent->m_fieldHandles.size()))
        && writer->writeBool(290, ent->m_unknown != 0);
    for (std::uint32_t field : ent->m_fieldHandles)
        written = written && writeString(330, toHexStr(field));
    written = written && writeString(100, "AcDbFieldList");
    if (!written || (!ent->extData.empty() && !writeExtData(ent->extData))) {
        m_writeError = true;
        return false;
    }
    return true;
}

// MLEADERSTYLE (AcDbMLeaderStyle, custom class). Inverse of
// DRW_MLeaderStyle::parseCode: common table-entry name/flags, scalar style
// fields, and handle references 340-343.
bool dxfRW::writeMLeaderStyle(DRW_MLeaderStyle *ent) {
    if (version < DRW::AC1021)
        return rejectUnsupportedDxfWrite();
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "MLEADERSTYLE");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbMLeaderStyle");
    writer->writeUtf8String(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeInt32(179, ent->styleVersion);
    writer->writeInt32(170, ent->contentType);
    writer->writeInt32(171, ent->drawMLeaderOrder);
    writer->writeInt32(172, ent->drawLeaderOrder);
    writer->writeInt32(90, ent->maxLeaderPoints);
    writer->writeDouble(40, ent->firstSegmentAngle);
    writer->writeDouble(41, ent->secondSegmentAngle);
    writer->writeInt32(173, ent->leaderType);
    writer->writeInt32(91, ent->leaderColor);
    if (ent->leaderLineTypeHandle.ref != 0)
    writer->writeString(340, toHexStr(ent->leaderLineTypeHandle.ref));
    writer->writeInt32(92, ent->leaderLineWeight);
    writer->writeBool(290, ent->landingEnabled);
    writer->writeDouble(42, ent->landingGap);
    writer->writeBool(291, ent->autoIncludeLanding);
    writer->writeDouble(43, ent->landingDistance);
    writer->writeUtf8String(3, ent->description);
    if (ent->arrowHeadBlockHandle.ref != 0)
    writer->writeString(341, toHexStr(ent->arrowHeadBlockHandle.ref));
    writer->writeDouble(44, ent->arrowHeadSize);
    writer->writeUtf8String(300, ent->textDefault);
    if (ent->textStyleHandle.ref != 0)
    writer->writeString(342, toHexStr(ent->textStyleHandle.ref));
    writer->writeInt32(174, ent->leftAttachment);
    writer->writeInt32(178, ent->rightAttachment);
    writer->writeInt32(175, ent->textAngleType);
    writer->writeInt32(176, ent->textAlignmentType);
    writer->writeInt32(93, ent->textColor);
    writer->writeDouble(45, ent->textHeight);
    writer->writeBool(292, ent->textFrameEnabled);
    writer->writeBool(297, ent->alwaysAlignTextLeft);
    writer->writeDouble(46, ent->alignSpace);
    if (ent->blockHandle.ref != 0)
    writer->writeString(343, toHexStr(ent->blockHandle.ref));
    writer->writeInt32(94, ent->blockColor);
    writer->writeDouble(47, ent->blockScale.x);
    writer->writeDouble(49, ent->blockScale.y);
    writer->writeDouble(140, ent->blockScale.z);
    writer->writeBool(293, ent->blockScaleEnabled);
    writer->writeDouble(141, ent->blockRotation);
    writer->writeBool(294, ent->blockRotationEnabled);
    writer->writeInt32(177, ent->blockConnectionType);
    writer->writeDouble(142, ent->scaleFactor);
    writer->writeBool(295, ent->propertyChanged);
    writer->writeBool(296, ent->isAnnotative);
    writer->writeDouble(143, ent->breakSize);
    writer->writeInt32(271, ent->attachmentDirection);
    writer->writeInt32(273, ent->topAttachment);
    writer->writeInt32(272, ent->bottomAttachment);
    writer->writeBool(298, ent->textExtended);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

//MLINESTYLE is a FIXED built-in (no CLASS record). The group-code shape is the
//inverse of DRW_MLineStyle::parseCode (drw_objects.cpp): name 2, flags 70,
//description 3, fill color 62 (before any element), start/end angle 51/52,
//element count 71, then per element offset 49 / color 62 / linetype 6.
//Cross-checked field-for-field against ezdxf 1.4.4 (AcDbMlineStyle).
bool dxfRW::writeMLineStyle(DRW_MLineStyle *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "MLINESTYLE");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbMlineStyle");
    writer->writeUtf8String(2, ent->name);
    writer->writeInt16(70, ent->flags);
    writer->writeUtf8String(3, ent->description);
    writer->writeInt16(62, ent->fillColor);
    writer->writeDouble(51, ent->startAngle);
    writer->writeDouble(52, ent->endAngle);
    writer->writeInt16(71, static_cast<int>(ent->elements.size()));
    for (const DRW_MLineElement &el : ent->elements) {
        writer->writeDouble(49, el.offset);
        writer->writeInt16(62, el.color);
        writer->writeUtf8String(6, el.linetype.empty() ? "BYLAYER" : el.linetype);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

//WIPEOUTVARIABLES (AcDbWipeoutVariables, custom class). Inverse of
//DRW_WipeoutVariables::parseCode: only the global display-frame flag (DXF 70).
bool dxfRW::writeWipeoutVariables(DRW_WipeoutVariables *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "WIPEOUTVARIABLES");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbWipeoutVariables");
    writer->writeInt16(70, ent->m_displayFrame);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeMaterial(DRW_Material *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "MATERIAL");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbMaterial");
    writer->writeUtf8String(1, ent->m_name);
    writer->writeUtf8String(2, ent->m_description);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeBackground(DRW_Background *ent, const char *recordName) {
    if (!preflightTableEntry(ent))
        return false;
    if (recordName == nullptr || *recordName == '\0')
        return failDxfWrite();
    DxfWriterRecordScope record(*writer);
    writer->writeString(0, recordName);
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    switch (ent->m_kind) {
    case DRW_Background::Gradient:
        writer->writeString(100, "AcDbGradientBackground");
        break;
    case DRW_Background::GroundPlane:
        writer->writeString(100, "AcDbGroundPlaneBackground");
        break;
    case DRW_Background::Image:
        writer->writeString(100, "AcDbImageBackground");
        break;
    case DRW_Background::Ibl:
        writer->writeString(100, "AcDbIBLBackground");
        break;
    case DRW_Background::Skylight:
        writer->writeString(100, "AcDbSkyBackground");
        break;
    case DRW_Background::Solid:
    default:
        writer->writeString(100, "AcDbSolidBackground");
        break;
    }
    writer->writeInt32(90, ent->m_classVersion);
    switch (ent->m_kind) {
    case DRW_Background::Solid:
        writer->writeInt32(90, ent->m_solidColor);
        break;
    case DRW_Background::Gradient:
        writer->writeInt32(90, ent->m_colorTop);
        writer->writeInt32(91, ent->m_colorMiddle);
        writer->writeInt32(92, ent->m_colorBottom);
        writer->writeDouble(140, ent->m_horizon);
        writer->writeDouble(141, ent->m_height);
        writer->writeDouble(142, ent->m_rotation);
        break;
    case DRW_Background::GroundPlane:
        writer->writeInt32(90, ent->m_colorSkyZenith);
        writer->writeInt32(91, ent->m_colorSkyHorizon);
        writer->writeInt32(92, ent->m_colorUndergroundHorizon);
        writer->writeInt32(93, ent->m_colorUndergroundAzimuth);
        writer->writeInt32(94, ent->m_colorNear);
        writer->writeInt32(95, ent->m_colorFar);
        break;
    case DRW_Background::Image:
        writer->writeUtf8String(300, ent->m_fileName);
        writer->writeBool(290, ent->m_fitToScreen);
        writer->writeBool(291, ent->m_maintainAspect);
        writer->writeBool(292, ent->m_useTiling);
        writer->writeDouble(140, ent->m_offset.x);
        writer->writeDouble(240, ent->m_offset.y);
        writer->writeDouble(142, ent->m_scale.x);
        writer->writeDouble(242, ent->m_scale.y);
        break;
    case DRW_Background::Ibl:
        writer->writeBool(290, ent->m_enabled);
        writer->writeUtf8String(1, ent->m_iblName);
        writer->writeDouble(40, ent->m_rotation);
        writer->writeBool(290, ent->m_displayImage);
        if (ent->m_secondaryBackgroundHandle != 0)
            writer->writeString(340, toHexStr(
                static_cast<int>(ent->m_secondaryBackgroundHandle)));
        break;
    case DRW_Background::Skylight:
        if (ent->m_sunHandle != 0)
            writer->writeString(340, toHexStr(ent->m_sunHandle));
        break;
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    if (!record.commit()) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeSunStudy(DRW_SunStudy *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "SUNSTUDY");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    if (!ent->reactorHandles.empty()) {
        writer->writeString(102, "{ACAD_REACTORS");
        for (const std::uint32_t reactor : ent->reactorHandles)
            writer->writeString(330, toHexStr(reactor));
        writer->writeString(102, "}");
    }
    if (ent->xDictHandle != 0) {
        writer->writeString(102, "{ACAD_XDICTIONARY");
        writer->writeString(360, toHexStr(ent->xDictHandle));
        writer->writeString(102, "}");
    }
    writer->writeString(100, "AcDbSunStudy");
    writer->writeInt32(90, ent->m_classVersion);
    writer->writeUtf8String(1, ent->m_setupName);
    writer->writeUtf8String(2, ent->m_description);
    writer->writeInt16(70, ent->m_outputType);
    if (ent->m_outputType == 0) {
        writer->writeBool(290, ent->m_useSubset);
        writer->writeUtf8String(3, ent->m_sheetSetName);
        writer->writeUtf8String(4, ent->m_sheetSubsetName);
    }
    writer->writeBool(291, ent->m_selectDatesFromCalendar);
    writer->writeInt32(91, static_cast<std::int32_t>(ent->m_dates.size()));
    for (const DRW_SunStudyDate& date : ent->m_dates) {
        writer->writeInt32(90, date.m_julianDay);
        writer->writeInt32(90, date.m_milliseconds);
    }
    writer->writeBool(292, ent->m_selectRangeOfDates);
    if (ent->m_selectRangeOfDates) {
        writer->writeInt32(93, ent->m_startTime);
        writer->writeInt32(94, ent->m_endTime);
        writer->writeInt32(95, ent->m_interval);
    }
    writer->writeInt32(91, static_cast<std::int32_t>(ent->m_hours.size()));
    for (bool hour : ent->m_hours)
        writer->writeBool(290, hour);
    writer->writeInt16(74, ent->m_shadePlotType);
    writer->writeInt16(75, ent->m_viewportCount);
    writer->writeInt16(76, ent->m_rowCount);
    writer->writeInt16(77, ent->m_columnCount);
    writer->writeDouble(40, ent->m_spacing);
    writer->writeBool(293, ent->m_lockViewports);
    writer->writeBool(294, ent->m_labelViewports);
    if (ent->m_pageSetupWizardHandle != 0)
        writer->writeString(340, toHexStr(ent->m_pageSetupWizardHandle));
    if (ent->m_viewHandle != 0)
        writer->writeString(341, toHexStr(ent->m_viewHandle));
    if (ent->m_visualStyleHandle != 0)
        writer->writeString(342, toHexStr(ent->m_visualStyleHandle));
    if (ent->m_textStyleHandle != 0)
        writer->writeString(343, toHexStr(ent->m_textStyleHandle));
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeMotionPath(DRW_MotionPath *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "MOTIONPATH");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    if (!ent->reactorHandles.empty()) {
        writer->writeString(102, "{ACAD_REACTORS");
        for (const std::uint32_t reactor : ent->reactorHandles)
            writer->writeString(330, toHexStr(reactor));
        writer->writeString(102, "}");
    }
    if (ent->xDictHandle != 0) {
        writer->writeString(102, "{ACAD_XDICTIONARY");
        writer->writeString(360, toHexStr(ent->xDictHandle));
        writer->writeString(102, "}");
    }
    writer->writeString(100, "AcDbMotionPath");
    writer->writeInt32(90, ent->m_classVersion);
    writer->writeString(340, toHexStr(ent->m_cameraPathHandle));
    writer->writeString(340, toHexStr(ent->m_targetPathHandle));
    writer->writeString(340, toHexStr(ent->m_viewTableHandle));
    writer->writeInt32(90, ent->m_frames);
    writer->writeInt32(90, ent->m_frameRate);
    writer->writeBool(290, ent->m_cornerDeceleration);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeCurvePath(DRW_CurvePath *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "ACDBCURVEPATH");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    if (!ent->reactorHandles.empty()) {
        writer->writeString(102, "{ACAD_REACTORS");
        for (const std::uint32_t reactor : ent->reactorHandles)
            writer->writeString(330, toHexStr(reactor));
        writer->writeString(102, "}");
    }
    if (ent->xDictHandle != 0) {
        writer->writeString(102, "{ACAD_XDICTIONARY");
        writer->writeString(360, toHexStr(ent->xDictHandle));
        writer->writeString(102, "}");
    }
    writer->writeString(100, "AcDbCurvePath");
    writer->writeInt32(90, ent->m_classVersion);
    writer->writeString(340, toHexStr(ent->m_entityHandle));
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writePointPath(DRW_PointPath *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "ACDBPOINTPATH");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    if (!ent->reactorHandles.empty()) {
        writer->writeString(102, "{ACAD_REACTORS");
        for (const std::uint32_t reactor : ent->reactorHandles)
            writer->writeString(330, toHexStr(reactor));
        writer->writeString(102, "}");
    }
    if (ent->xDictHandle != 0) {
        writer->writeString(102, "{ACAD_XDICTIONARY");
        writer->writeString(360, toHexStr(ent->xDictHandle));
        writer->writeString(102, "}");
    }
    writer->writeString(100, "AcDbPointPath");
    writer->writeInt32(90, ent->m_classVersion);
    writer->writeDouble(10, ent->m_point.x);
    writer->writeDouble(20, ent->m_point.y);
    writer->writeDouble(30, ent->m_point.z);
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeObjectPtr(DRW_ObjectPtr *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "OBJECTPTR");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    if (!ent->reactorHandles.empty()) {
        writer->writeString(102, "{ACAD_REACTORS");
        for (const std::uint32_t reactor : ent->reactorHandles)
            writer->writeString(330, toHexStr(reactor));
        writer->writeString(102, "}");
    }
    if (ent->xDictHandle != 0) {
        writer->writeString(102, "{ACAD_XDICTIONARY");
        writer->writeString(360, toHexStr(ent->xDictHandle));
        writer->writeString(102, "}");
    }
    writer->writeString(100, "AcDbObjectPtr");
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writePartialViewingIndex(DRW_PartialViewingIndex *ent) {
    if (!preflightTableEntry(ent))
        return false;
    writer->writeString(0, "PARTIALVIEWINGINDEX");
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    if (!ent->reactorHandles.empty()) {
        writer->writeString(102, "{ACAD_REACTORS");
        for (const std::uint32_t reactor : ent->reactorHandles)
            writer->writeString(330, toHexStr(reactor));
        writer->writeString(102, "}");
    }
    if (ent->xDictHandle != 0) {
        writer->writeString(102, "{ACAD_XDICTIONARY");
        writer->writeString(360, toHexStr(ent->xDictHandle));
        writer->writeString(102, "}");
    }
    writer->writeString(100, "AcDbPartialViewingIndex");
    for (const DRW_PartialViewingIndexEntry& entry : ent->m_entries) {
        writer->writeDouble(10, entry.extentsMin.x);
        writer->writeDouble(20, entry.extentsMin.y);
        writer->writeDouble(30, entry.extentsMin.z);
        writer->writeDouble(11, entry.extentsMax.x);
        writer->writeDouble(21, entry.extentsMax.y);
        writer->writeDouble(31, entry.extentsMax.z);
        writer->writeString(340,
                            toHexStr(entry.objectHandle));
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeRenderSettings(DRW_RenderSettings *ent,
                                const char *recordName) {
    if (!preflightTableEntry(ent))
        return false;
    if (recordName == nullptr || *recordName == '\0')
        return failDxfWrite();
    writer->writeString(0, recordName);
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    switch (ent->m_kind) {
    case DRW_RenderSettings::Global:
        writer->writeString(100, "AcDbRenderGlobal"); break;
    case DRW_RenderSettings::Environment:
        writer->writeString(100, "AcDbRenderEnvironment"); break;
    case DRW_RenderSettings::Entry:
        writer->writeString(100, "AcDbRenderEntry"); break;
    case DRW_RenderSettings::RapidRT:
        writer->writeString(100, "AcDbRapidRTRenderSettings"); break;
    case DRW_RenderSettings::MentalRay:
        writer->writeString(100, "AcDbMentalRayRenderSettings"); break;
    case DRW_RenderSettings::Settings:
    default:
        writer->writeString(100, "AcDbRenderSettings"); break;
    }
    const bool hasVectors = !ent->m_longs.empty() || !ent->m_strings.empty()
        || !ent->m_bools.empty() || !ent->m_shorts.empty()
        || !ent->m_bytes.empty() || !ent->m_doubles.empty();
    if (hasVectors) {
        for (std::int32_t value : ent->m_longs) writer->writeInt32(90, value);
        for (const UTF8STRING &value : ent->m_strings)
            writer->writeUtf8String(1, value);
        for (bool value : ent->m_bools) writer->writeBool(290, value);
        for (std::int32_t value : ent->m_shorts) writer->writeInt16(70, value);
        for (std::int32_t value : ent->m_bytes) writer->writeInt16(280, value);
        for (double value : ent->m_doubles) writer->writeDouble(40, value);
    } else {
        writer->writeInt32(90, ent->m_classVersion);
        writer->writeUtf8String(1, ent->m_name);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeSection(DRW_Section *ent, const char *recordName) {
    if (!preflightTableEntry(ent))
        return false;
    if (recordName == nullptr || *recordName == '\0')
        return failDxfWrite();
    writer->writeString(0, recordName);
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    if (ent->m_kind == DRW_Section::Manager) {
        writer->writeString(100, "AcDbSectionManager");
        writer->writeBool(70, ent->m_isLive);
        writer->writeInt32(90, ent->m_sectionCount);
        for (std::uint32_t handle : ent->m_sectionHandles)
            if (handle != 0)
                writer->writeString(330, toHexStr(handle));
    } else {
        writer->writeString(100, "AcDbSectionSettings");
        if (ent->m_types.empty()) {
            writer->writeInt32(90, ent->m_classVersion);
            writer->writeInt32(91, ent->m_sectionType);
            writer->writeInt32(91, ent->m_generationOptions);
            if (ent->m_destinationBlockHandle != 0)
                writer->writeString(331, toHexStr(
                    static_cast<int>(ent->m_destinationBlockHandle)));
        } else {
            writer->writeInt32(90, ent->m_currentType);
            writer->writeInt32(91, static_cast<int>(ent->m_types.size()));
            for (const DRW_SectionTypeSettings& type : ent->m_types) {
                writer->writeUtf8String(1, "SectionTypeSettings");
                writer->writeInt32(90, type.m_type);
                writer->writeInt32(91, type.m_generation);
                const std::size_t sourceCount = std::min(
                    type.m_sourceHandles.size(),
                    static_cast<std::size_t>(DRW_Section::kMaxSectionSourceCount));
                writer->writeInt32(92, static_cast<int>(sourceCount));
                for (std::size_t i = 0; i < sourceCount; ++i) {
                    writer->writeString(330, toHexStr(
                        static_cast<int>(type.m_sourceHandles[i])));
                }
                if (type.m_destinationBlockHandle != 0)
                    writer->writeString(331, toHexStr(
                        static_cast<int>(type.m_destinationBlockHandle)));
                writer->writeUtf8String(1, type.m_destinationFile);
                const std::size_t geometryCount = std::min(
                    type.m_geometry.size(),
                    static_cast<std::size_t>(DRW_Section::kMaxSectionGeometryCount));
                writer->writeInt32(93, static_cast<int>(geometryCount));
                for (std::size_t i = 0; i < geometryCount; ++i) {
                    const DRW_SectionGeometrySettings& geometry = type.m_geometry[i];
                    writer->writeUtf8String(2, "SectionGeometrySettings");
                    writer->writeInt32(90, geometry.m_numGeometries);
                    writer->writeInt32(91, geometry.m_hexIndex);
                    writer->writeInt32(92, geometry.m_flags);
                    writer->writeInt16(62, geometry.m_color);
                    if (geometry.m_hasRgbColor || geometry.m_rgbColor != 0)
                        writer->writeInt32(420, geometry.m_rgbColor);
                    writer->writeUtf8String(8, geometry.m_layer);
                    writer->writeUtf8String(6, geometry.m_lineType);
                    writer->writeDouble(40, geometry.m_lineTypeScale);
                    writer->writeUtf8String(1, geometry.m_plotStyle);
                    writer->writeInt16(370, geometry.m_lineWeight);
                    writer->writeInt16(70, geometry.m_faceTransparency);
                    writer->writeInt16(71, geometry.m_edgeTransparency);
                    writer->writeInt16(72, geometry.m_hatchType);
                    writer->writeUtf8String(2, geometry.m_hatchPattern);
                    writer->writeDouble(41, geometry.m_hatchAngle);
                    writer->writeDouble(42, geometry.m_hatchSpacing);
                    writer->writeDouble(43, geometry.m_hatchScale);
                    writer->writeUtf8String(3, "SectionGeometrySettingsEnd");
                }
                writer->writeUtf8String(3, "SectionTypeSettingsEnd");
            }
        }
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeDbColor(DRW_DbColor *ent, const char *recordName) {
    if (!preflightTableEntry(ent))
        return false;
    if (recordName == nullptr || *recordName == '\0')
        return failDxfWrite();
    writer->writeString(0, recordName);
    writer->writeString(5, toHexStr(ent->handle));
    writeObjectOwner(static_cast<std::uint32_t>(ent->parentHandle));
    writer->writeString(100, "AcDbColor");
    if (ent->rgb >= 0)
        writer->writeInt32(420, ent->rgb);
    else
        writer->writeInt16(62, static_cast<int>(ent->colorIndex));
    if (!ent->bookName.empty() || !ent->name.empty()) {
        const UTF8STRING value = ent->bookName.empty()
            ? ent->name
            : ent->bookName + "$" + ent->name;
        writer->writeUtf8String(430, value);
    }
    if (!ent->extData.empty() && !writeExtData(ent->extData))
        return false;
    return !writer->hasWriteError();
}

bool dxfRW::writeDimensionAssociation(DRW_DimensionAssociation *ent) {
    if (!preflightTableEntry(ent)
        || ent == nullptr || writer == nullptr
        || !canWriteDxfDimensionAssociation(*ent)) {
        m_writeError = true;
        return false;
    }

    auto writeString = [this](int code, const std::string& value) {
        return writer->writeString(code, value);
    };
    auto writeUtf8String = [this](int code, const UTF8STRING& value) {
        return writer->writeUtf8String(code, value);
    };
    bool written = writeString(0, "DIMASSOC")
        && writeString(5, toHexStr(ent->handle));
    if (version > DRW::AC1014) {
        written = written && writeString(
            330, ent->parentHandle != 0
                ? toHexStr(static_cast<std::uint32_t>(ent->parentHandle))
                : "C");
    }
    written = written && writeString(100, "AcDbDimAssoc");
    if (ent->m_dimensionHandle != 0)
        written = written && writeString(330, toHexStr(ent->m_dimensionHandle));
    written = written
        && writer->writeInt32(90, static_cast<int>(ent->m_associativityFlags))
        && writer->writeInt16(70, static_cast<int>(ent->m_rotatedDimensionType))
        && writer->writeBool(71, ent->m_isTransSpace);
    for (const DRW_DimensionAssociationOsnapRef &ref : ent->m_osnapRefs) {
        written = written && writeUtf8String(1, ref.m_className)
            && writer->writeInt16(72, static_cast<int>(ref.m_objectOsnapType));
        if (ref.m_objectHandle != 0)
            written = written && writeString(331, toHexStr(ref.m_objectHandle));
    }
    if (!written || (!ent->extData.empty() && !writeExtData(ent->extData))) {
        m_writeError = true;
        return false;
    }
    return true;
}

bool dxfRW::writeEvaluationGraph(DRW_EvaluationGraph *ent,
                                 const char *recordName) {
    if (!preflightTableEntry(ent)
        || ent == nullptr || writer == nullptr || recordName == nullptr
        || *recordName == '\0'
        || !canWriteDxfEvaluationGraph(version, *ent)) {
        m_writeError = true;
        return false;
    }

    auto writeString = [this](int code, const std::string& value) {
        return writer->writeString(code, value);
    };
    bool written = writeString(0, recordName)
        && writeString(5, toHexStr(ent->handle));
    if (version > DRW::AC1014) {
        written = written && writeString(
            330, ent->parentHandle != 0
                ? toHexStr(static_cast<std::uint32_t>(ent->parentHandle))
                : "C");
    }
    written = written && writeString(100, "AcDbEvalGraph")
        && writer->writeInt32(96, ent->m_value96)
        && writer->writeInt32(97, ent->m_value97);
    for (const DRW_EvaluationGraphNode &node : ent->m_nodes) {
        written = written
            && writer->writeInt32(91, node.m_index)
            && writer->writeInt32(93, node.m_flags)
            && writer->writeInt32(95, node.m_nextNodeIndex);
        written = written
            && writeString(360, toHexStr(node.m_expressionHandle));
        written = written
            && writer->writeInt32(92, node.m_data1)
            && writer->writeInt32(92, node.m_data2)
            && writer->writeInt32(92, node.m_data3)
            && writer->writeInt32(92, node.m_data4);
    }
    for (const DRW_EvaluationGraphEdge &edge : ent->m_edges) {
        written = written
            && writer->writeInt32(92, edge.m_value92)
            && writer->writeInt32(93, edge.m_value93)
            && writer->writeInt32(94, edge.m_value94)
            && writer->writeInt32(91, edge.m_value91a)
            && writer->writeInt32(91, edge.m_value91b)
            && writer->writeInt32(92, edge.m_value92a)
            && writer->writeInt32(92, edge.m_value92b)
            && writer->writeInt32(92, edge.m_value92c)
            && writer->writeInt32(92, edge.m_value92d)
            && writer->writeInt32(92, edge.m_value92e);
    }
    if (!written || (!ent->extData.empty() && !writeExtData(ent->extData))) {
        m_writeError = true;
        return false;
    }
    return true;
}

/** utility function
 * convert a int to string in hex
 **/
std::string dxfRW::toHexStr(std::uint32_t n){
#if defined(__APPLE__)
    char buffer[9]= {'\0'};
    snprintf(buffer,9, "%X", static_cast<unsigned int>(n));
    return std::string(buffer);
#else
    std::ostringstream Convert;
    Convert << std::uppercase << std::hex << n;
    return Convert.str();
#endif
}

std::string dxfRW::toHexStr(int n){
    return toHexStr(static_cast<std::uint32_t>(n));
}


DRW::Version dxfRW::getVersion() const {
    return version;
}

DRW::error dxfRW::getError() const{
    return error;
}

bool dxfRW::setError(const DRW::error lastError){
    error = lastError;
    return (DRW::BAD_NONE == error);
}
