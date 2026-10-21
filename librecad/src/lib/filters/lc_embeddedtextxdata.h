/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_EMBEDDEDTEXTXDATA_H
#define LC_EMBEDDEDTEXTXDATA_H

#include "drw_base.h"
#include <cstddef>
#include <optional>

/** Lossless private in-memory capsule for embedded MTEXT XDATA.
 * Not the LibreCADText schema and not permission to emit malformed wire data.
 * The filter retains its v1 reader; this independent capsule has version 2.
 * Exact bytes, type/code, null versus INVALID, IEEE64 storage and active DWG
 * layer-reference provenance survive. Inputs must not be mutated concurrently.
 * Encoding/decoding is transactional; errors leave input and output unchanged.
 * The caller must visibly reject import/export if retention is unavailable.
 */
class LC_EmbeddedTextXData final {
public:
    using Tags = std::vector<std::shared_ptr<DRW_Variant>>;
    using Bytes = std::vector<std::uint8_t>;
    struct Limits {
        std::size_t maximumItems = 1000000; // DRW MaxEedTotalItems.
        // Resource ceiling, not a DXF/DWG semantic limit. Includes the complete
        // capsule header and all per-tag/provenance overhead, not just strings.
        std::size_t maximumBytes = 64u * 1024u * 1024u;
    };
    enum class ErrorCode { None, InvalidLimits, ResourceLimit, InvalidPayload, Malformed, UnknownVersion };
    struct Error {
        ErrorCode code = ErrorCode::None;
        std::string message;
        explicit operator bool() const { return code != ErrorCode::None; }
    };
    struct Encoded { std::optional<Bytes> bytes; Error error; explicit operator bool() const { return bytes.has_value() && !error; } };
    struct Decoded { std::optional<Tags> tags; Error error; explicit operator bool() const { return tags.has_value() && !error; } };
    static Encoded encode(const Tags &input, const Limits &limits);
    static Encoded encode(const Tags &input) { return encode(input, Limits{}); }
    static Decoded decode(const Bytes &input, const Limits &limits);
    static Decoded decode(const Bytes &input) { return decode(input, Limits{}); }
};

#endif
