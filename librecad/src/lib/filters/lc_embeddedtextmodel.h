/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_EMBEDDEDTEXTMODEL_H
#define LC_EMBEDDEDTEXTMODEL_H

#include <QString>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

/** Inert provisional full-model codec, NOT a DXF/DWG transport or activation API.
 * Every field owns its exact bytes. Unsupported schema/fields/kinds are opaque;
 * malformed bounded input is retained invalid, never published as a model.
 * Decode establishes neither dictionary ownership nor native-baseline authority.
 * A bridge must reconcile live STYLE ownership without changing shared defaults,
 * and initialize current source/profile independently of historical provenance.
 * Reading/saving must never reset source or profile edit authority.
 *
 * Envelope: "LCEM", u16 envelope=1, u16 schema=2, u32 headerBytes=64,
 * u64 bodyBytes, u32 topFieldCount, SHA256(body)[32], u64 reserved=0.
 * Field: u16 ID, u8 kind, u8 flags=0, u64 payloadBytes, payload. IDs ascend.
 * All integers and IEEE754 binary64 storage are little-endian. Record payloads
 * start with u64 fieldCount; arrays start with u64 count. RecordArray and
 * RawTagArrays elements have u64 byte length followed by their encoded payload.
 * Utf16/NullableBytes start with u8 nullness (0=null, 1=nonnull), u64 count,
 * then exact units/bytes. Null requires count=0; omitted and present-null differ.
 * RawTags is the committed LC_EmbeddedTextXData version-2 capsule, NOT wire XDATA.
 * Bytes have no null state; Utf16 retains malformed UTF16 and strings retain NUL.
 *
 * Top IDs: 1 CurrentSource, 2 optional ImportedSource, 3 OuterAttribute,
 * 4 EmbeddedMText, 5 Legacy, 6 TargetWireBaseline, 7 optional OpaqueExtras array.
 * The complete field registry and native mappings are documented in
 * docs/development/embedded_text_model_registry.md. Schema2 adds required exact
 * source handle/link/presence state; schema1 and unknown future schemas remain
 * opaque with exact bounded raw, never zero-filled or activated. This unreleased
 * service migration is not a native-wire or LibreCADText profile schema freeze.
 * Source handle components and semantic fields may conflict: retain both inertly,
 * never coerce a wide reference into a live 32-bit graph relationship. Unsupported
 * native enum-like values are retained as I32/U8, not cast into renderer enums.
 * CurrentSource18 is required canonical local deformation, a record of exact
 * Real fields1xx/2xy/3yx/4yy; ImportedSource18 is optional captured history only.
 * Identity(1,0,0,1) is authority only when actually held by the host, never guessed
 * from scalar height/angle/width. The renderer separately checks invertibility
 * and finiteness; the codec retains every bit without activating it.
 *
 * Final encode/decode limits include framing and all payload bytes; their item
 * count is aggregate over fields, array elements and raw variants, not per field.
 * Framing-only builders enforce packed bytes and LOCAL immediate counts, not
 * nested item/depth budgets: builder success is never bounded-model acceptance.
 * These are resource ceilings, not native wire allowances. Known model fields
 * may retain nonfinite IEEE bits; native geometry/wire validation is separate.
 * All operations are transactional. Inputs must not mutate concurrently.
 */
class LC_EmbeddedTextModel final {
public:
    using Bytes = std::vector<std::uint8_t>;
    static constexpr std::uint16_t EnvelopeVersion = 1;
    static constexpr std::uint16_t ModelSchemaVersion = 2;
    enum class Kind : std::uint8_t {
        Bool = 1, U8, U16, U32, U64, I32, I64, Real, Coord, Bytes,
        Utf16, Record, U32Array, RealArray, RecordArray, RawTags,
        RawTagArrays, NullableBytes
    };
    struct Field {
        std::uint16_t id = 0;
        Kind kind = Kind::Bytes;
        Bytes payload;
        bool operator==(const Field &other) const;
    };
    using Record = std::vector<Field>;
    struct Limits {
        std::size_t maximumBytes = 64u * 1024u * 1024u;
        std::size_t maximumItems = 1000000;
        unsigned maximumDepth = 16;
    };
    enum class ErrorCode {
        None, InvalidLimits, ResourceLimit, Malformed, DigestMismatch,
        UnknownVersion, UnknownField, UnknownKind, InvalidField, MissingField
    };
    struct Error {
        ErrorCode code = ErrorCode::None;
        QString message;
        std::vector<std::uint16_t> path;
        explicit operator bool() const { return code != ErrorCode::None; }
    };
    struct Encoded {
        std::optional<Bytes> bytes;
        Error error;
        explicit operator bool() const { return bytes.has_value() && !error; }
    };
    enum class Status { Known, Opaque, Invalid, Refused };
    struct Decoded {
        Status status = Status::Refused;
        std::optional<Record> model;
        // Present only for bounded Opaque/Invalid input. Refused means retention
        // was unavailable; the bridge must reject import and preserve its input.
        std::optional<Bytes> raw;
        Error error;
        bool known() const { return status == Status::Known && model.has_value() && !error; }
    };

    static Encoded encode(const Record &, const Limits &);
    static Encoded encode(const Record &record) { return encode(record, Limits{}); }
    static Decoded decode(const Bytes &, const Limits &);
    static Decoded decode(const Bytes &bytes) { return decode(bytes, Limits{}); }
    // Framing-only owning builders for a bridge. They check byte limits and
    // immediate record fields (plus immediate elements/fields for RecordArray),
    // but do not inspect nested payloads, apply maximumDepth or enforce nested
    // item counts. They validate neither schema nor authority. Always perform
    // final encode/decode for complete-model budgets and accepted model status.
    static Encoded packRecord(const Record &, const Limits &);
    static Encoded packRecord(const Record &record) { return packRecord(record, Limits{}); }
    static Encoded packRecordArray(const std::vector<Record> &, const Limits &);
    static Encoded packRecordArray(const std::vector<Record> &records) { return packRecordArray(records, Limits{}); }
};
#endif
