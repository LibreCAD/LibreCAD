/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETCURVEXDATA_H
#define LC_OFFSETCURVEXDATA_H

#include "drw_base.h"
#include "lc_offsetcurve.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <string_view>

namespace LC_OffsetCurveXData {

using XData = std::vector<std::shared_ptr<DRW_Variant>>;
using OwnedTail = LC_OffsetCurve::OwnedTail;
inline constexpr const char *AppId = "LibreCad";
inline constexpr const char *Marker = "LC_OFFSET_CURVE";
inline constexpr unsigned ReservedFlags = 0x7ff0u;
inline constexpr std::size_t MaximumTailItems = 64;
inline constexpr std::size_t MaximumTailBytes = 4096;
inline constexpr unsigned MaximumTailDepth = 8;

enum class Status { Invalid, Schema1, Other, NoSubList };
enum class Reason {
    ResourceLimit, InvalidOptions, MalformedKnown, DuplicateKnown,
    MalformedUnknown, MalformedGroup, InvalidDefinition, TailUnsupported,
    TailInvalid, TailLimit, Encoding, UnsafeMerge, EedChunkLimit
};
struct Diagnostic {
    Reason reason;
    std::size_t item;
};
using Diagnostics = std::vector<Diagnostic>;

enum class TextEncoding { Utf8, Utf16, Legacy };
struct Options {
    TextEncoding textEncoding = TextEncoding::Utf8;
    bool eed = false;
    // Mirrors dxfRW's !binFile branch; ignored for EED. Only ASCII 1000
    // values caret-escape bytes below 0x20 and literal '^' before encoding.
    bool asciiDxf = true;
    // Binary R12 extended group codes add the writer's 0xff escape byte.
    // Only affects binary-chunk framing; ignored for ASCII DXF and EED.
    bool binaryDxfR12 = false;
    // For Legacy, return the exact safely encoded byte count, excluding framing.
    // Use the selected writer's codec; nullopt refuses. ASCII 1000 caret
    // escaping has already been applied to this view; do not apply it again.
    // Called only on bounded valid UTF-8, and never for braces/binary/handles.
    std::function<std::optional<std::size_t>(std::string_view)> legacyEncodedBytes;
    // Callers may lower, never raise, these hard resource ceilings. Export also
    // admits emitted items (including split DXF binary chunks) and encoded
    // typed-payload bytes, independently of the source pointer/payload envelope.
    std::size_t maximumItems = 1000000;
    std::size_t maximumBytes = 64u * 1024u * 1024u;
    std::size_t maximumDiagnostics = 32;
};

struct Definition {
    LC_EllipseOffsetDefinition geometry;
    unsigned reservedFlags = 0;
    OwnedTail tail;
};
struct ParseResult {
    Status status = Status::NoSubList;
    std::optional<Definition> definition;
    // nullopt is a resource/options failure, NOT an empty successfully stripped
    // stream. Import must not silently discard the original in that case.
    // Remaining foreign variants are shared read-only; no input tag is modified.
    std::optional<XData> remaining;
    Diagnostics diagnostics;
    std::size_t knownCandidates = 0;
};

ParseResult parseAndStrip(const XData &input, const Options &options = {});

// null current means plain/undefined/inconsistent: strip stale schema 1 only.
// A supplied current definition must be eligible and valid, including its tail.
// All compatible LibreCad groups merge into the first, retaining its spelling.
// Foreign payloads/order and unknown schemas survive. No shared variant mutates.
// Tail byte limits include typed payload framing, not ASCII group-code lines.
// Foreign DXF binary tags stay shared; their emitted 127-byte chunks are sized
// using hexadecimal ASCII or raw binary payloads, with framing per chunk.
// EED packs scalar/complete top-level braced units into <=65535-byte packets,
// never splitting a braced unit; at most 65536 packets. APPID headers and layer
// names are resolved handles, not encoded payload text. Compatible merged
// payloads precede the newly appended definition in this same simulation.
// EED requires Utf16 or Legacy; layer/APPID handle resolution stays in the codec.
// Returns nullopt on any unsafe merge/encoding/resource/current-definition error.
// Diagnostics, if supplied, are replaced, bounded, and contain no payload text.
std::optional<XData> forExport(const XData &input,
    const std::optional<Definition> &current, const Options &options = {},
    Diagnostics *diagnostics = nullptr);

} // namespace LC_OffsetCurveXData
#endif
