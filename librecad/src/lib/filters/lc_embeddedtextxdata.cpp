/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_embeddedtextxdata.h"
#include <cstring>
#include <limits>

namespace {
using Codec = LC_EmbeddedTextXData;
using Code = Codec::ErrorCode;
constexpr std::uint32_t magic = 0x5843544c; // Little-endian "LTCX".
constexpr std::uint8_t version = 2;
enum Wire : std::uint8_t { Null, Invalid, String, Integer, Integer64, Double, Coordinate, Binary };
static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559, "Capsules require IEEE binary64");
Codec::Error error(Code code, const char *message) { return {code, message}; }
bool valid(const Codec::Limits &limits) {
    return limits.maximumItems <= 1000000u && limits.maximumBytes >= 9 &&
        limits.maximumBytes <= 64u * 1024u * 1024u;
}
bool add(std::size_t &size, std::size_t amount, std::size_t maximum) {
    if (size > maximum || amount > maximum - size) return false;
    size += amount;
    return true;
}
void integer(Codec::Bytes &out, std::uint64_t value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; ++i) out.push_back(static_cast<std::uint8_t>(value >> (8u * i)));
}
void real(Codec::Bytes &out, double value) {
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    integer(out, bits, 8);
}
std::int32_t signed32(std::uint32_t bits) {
    std::int32_t value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
struct Reader {
    const Codec::Bytes &bytes;
    std::size_t position = 0;
    bool take(std::uint64_t &value, unsigned length) {
        if (position > bytes.size() || length > bytes.size() - position) return false;
        value = 0;
        for (unsigned i = 0; i < length; ++i) value |= std::uint64_t(bytes[position++]) << (8u * i);
        return true;
    }
    bool real(double &value) {
        std::uint64_t bits;
        if (!take(bits, 8)) return false;
        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }
    bool length(std::size_t &size) {
        std::uint64_t value;
        if (!take(value, 4) || value > bytes.size() - position) return false;
        size = static_cast<std::size_t>(value);
        return true;
    }
};
}

LC_EmbeddedTextXData::Encoded LC_EmbeddedTextXData::encode(const Tags &input, const Limits &limits) {
    if (!valid(limits)) return {std::nullopt, error(Code::InvalidLimits, "Invalid embedded XDATA retention limits")};
    if (input.size() > limits.maximumItems) return {std::nullopt, error(Code::ResourceLimit, "Embedded XDATA item retention limit exceeded")};
    std::size_t size = 9;
    for (const auto &value : input) {
        if (!add(size, 5, limits.maximumBytes)) return {std::nullopt, error(Code::ResourceLimit, "Embedded XDATA byte retention limit exceeded")};
        if (!value) continue;
        std::size_t payload = 0;
        switch (value->type()) {
        case DRW_Variant::INVALID: break;
        case DRW_Variant::STRING:
            if (!value->content.s) return {std::nullopt, error(Code::InvalidPayload, "Embedded XDATA string has invalid storage")};
            if (const auto reference = value->activeDwgLayerReference(); reference &&
                (reference->sourceVersion < DRW::UNKNOWNV || reference->sourceVersion > DRW::AC1032))
                return {std::nullopt, error(Code::InvalidPayload, "Embedded XDATA has an unknown active replay source version")};
            payload = value->activeDwgLayerReference() ? 18 : 6; // length, flags, optional version/u64.
            if (!add(payload, value->content.s->size(), limits.maximumBytes)) return {std::nullopt, error(Code::ResourceLimit, "Embedded XDATA string retention limit exceeded")};
            break;
        case DRW_Variant::INTEGER: payload = 4; break;
        case DRW_Variant::INTEGER64: case DRW_Variant::DOUBLE: payload = 8; break;
        case DRW_Variant::COORD:
            if (!value->coord()) return {std::nullopt, error(Code::InvalidPayload, "Embedded XDATA coordinate has invalid storage")};
            payload = 24; break;
        case DRW_Variant::BINARY:
            if (!value->content.b) return {std::nullopt, error(Code::InvalidPayload, "Embedded XDATA binary has invalid storage")};
            payload = 4;
            if (!add(payload, value->binary()->size(), limits.maximumBytes)) return {std::nullopt, error(Code::ResourceLimit, "Embedded XDATA binary retention limit exceeded")};
            break;
        }
        if (!add(size, payload, limits.maximumBytes)) return {std::nullopt, error(Code::ResourceLimit, "Embedded XDATA byte retention limit exceeded")};
    }
    Bytes out; out.reserve(size);
    integer(out, magic, 4); integer(out, version, 1); integer(out, input.size(), 4);
    for (const auto &value : input) {
        const auto kind = !value ? Null : value->type() == DRW_Variant::INVALID ? Invalid :
            value->type() == DRW_Variant::STRING ? String : value->type() == DRW_Variant::INTEGER ? Integer :
            value->type() == DRW_Variant::INTEGER64 ? Integer64 : value->type() == DRW_Variant::DOUBLE ? Double :
            value->type() == DRW_Variant::COORD ? Coordinate : Binary;
        integer(out, kind, 1); integer(out, value ? std::uint32_t(value->code()) : 0, 4);
        if (!value) continue;
        switch (kind) {
        case String: {
            const auto &text = *value->content.s;
            integer(out, text.size(), 4); out.insert(out.end(), text.begin(), text.end());
            integer(out, value->isLayerRef() ? 1 : 0, 1);
            const auto reference = value->activeDwgLayerReference();
            integer(out, reference ? 1 : 0, 1);
            if (reference) {integer(out, std::uint32_t(reference->sourceVersion), 4); integer(out, reference->reference, 8);}
            break;
        }
        case Integer: integer(out, std::uint32_t(value->i_val()), 4); break;
        case Integer64: integer(out, std::uint64_t(value->i64_val()), 8); break;
        case Double: real(out, value->d_val()); break;
        case Coordinate: real(out, value->coord()->x); real(out, value->coord()->y); real(out, value->coord()->z); break;
        case Binary: integer(out, value->binary()->size(), 4); out.insert(out.end(), value->binary()->begin(), value->binary()->end()); break;
        case Null: case Invalid: break;
        }
    }
    return {std::move(out), {}};
}

LC_EmbeddedTextXData::Decoded LC_EmbeddedTextXData::decode(const Bytes &input, const Limits &limits) {
    if (!valid(limits)) return {std::nullopt, error(Code::InvalidLimits, "Invalid embedded XDATA retention limits")};
    if (input.size() > limits.maximumBytes) return {std::nullopt, error(Code::ResourceLimit, "Embedded XDATA byte retention limit exceeded")};
    Reader reader{input};
    std::uint64_t signature = 0, schema = 0, count = 0;
    if (!reader.take(signature, 4) || signature != magic || !reader.take(schema, 1))
        return {std::nullopt, error(Code::Malformed, "Malformed embedded XDATA capsule header")};
    if (schema != version) return {std::nullopt, error(Code::UnknownVersion, "Unknown embedded XDATA capsule version")};
    if (!reader.take(count, 4) || count > limits.maximumItems || count > (input.size() - reader.position) / 5)
        return {std::nullopt, error(count > limits.maximumItems ? Code::ResourceLimit : Code::Malformed, "Invalid embedded XDATA capsule count")};
    Tags result; result.reserve(static_cast<std::size_t>(count));
    const auto malformed = [] { return Decoded{std::nullopt, error(Code::Malformed, "Malformed embedded XDATA capsule value")}; };
    for (std::uint64_t index = 0; index < count; ++index) {
        std::uint64_t kind, rawCode;
        if (!reader.take(kind, 1) || kind > Binary || !reader.take(rawCode, 4)) return malformed();
        const int code = signed32(static_cast<std::uint32_t>(rawCode));
        std::shared_ptr<DRW_Variant> value;
        if (kind == Null || kind == Invalid) {
            if (code != 0) return malformed(); // INVALID currently has only its default constructor.
            if (kind == Invalid) value = std::make_shared<DRW_Variant>();
        } else if (kind == String || kind == Binary) {
            std::size_t length;
            if (!reader.length(length)) return malformed();
            if (kind == String) {
                const std::string text(reinterpret_cast<const char *>(input.data() + reader.position), length);
                reader.position += length;
                std::uint64_t layer, active;
                if (!reader.take(layer, 1) || layer > 1 || !reader.take(active, 1) || active > 1) return malformed();
                value = std::make_shared<DRW_Variant>(code, text, layer != 0);
                if (active) {
                    std::uint64_t sourceVersion, reference;
                    if (code != 1003 || !reader.take(sourceVersion, 4) || sourceVersion > DRW::AC1032 || !reader.take(reference, 8)) return malformed();
                    value->setDwgRawLayerReference(reference, static_cast<DRW::Version>(sourceVersion));
                }
            } else {
                Bytes bytes(input.begin() + static_cast<std::ptrdiff_t>(reader.position), input.begin() + static_cast<std::ptrdiff_t>(reader.position + length));
                reader.position += length;
                value = std::make_shared<DRW_Variant>(code, std::move(bytes));
            }
        } else if (kind == Integer || kind == Integer64) {
            std::uint64_t bits;
            if (!reader.take(bits, kind == Integer ? 4 : 8)) return malformed();
            if (kind == Integer) value = std::make_shared<DRW_Variant>(code, signed32(static_cast<std::uint32_t>(bits)));
            else {std::int64_t number; std::memcpy(&number, &bits, sizeof(number)); value = std::make_shared<DRW_Variant>(code, number);}
        } else if (kind == Double) {
            double number;
            if (!reader.real(number)) return malformed();
            value = std::make_shared<DRW_Variant>(code, number);
        } else if (kind == Coordinate) {
            DRW_Coord coordinate;
            if (!reader.real(coordinate.x) || !reader.real(coordinate.y) || !reader.real(coordinate.z)) return malformed();
            value = std::make_shared<DRW_Variant>(code, coordinate);
        }
        result.push_back(std::move(value));
    }
    if (reader.position != input.size()) return malformed();
    return {std::move(result), {}};
}
