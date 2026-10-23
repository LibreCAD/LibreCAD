/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetcurvexdata.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <utility>

#include <QByteArrayView>
#include <QStringConverter>
#include <QStringView>

namespace LC_OffsetCurveXData {
namespace {
constexpr std::size_t HardItems = 1000000;
constexpr std::size_t HardBytes = 64u * 1024u * 1024u;
constexpr std::size_t HardDiagnostics = 32;

void diagnose(Diagnostics &out, const Options &options, Reason reason, std::size_t item) {
    if (out.size() < std::min(options.maximumDiagnostics, HardDiagnostics))
        out.push_back({reason, item});
}

bool charge(std::size_t &used, std::size_t amount, std::size_t cap) {
    if (used > cap || amount > cap - used) return false;
    used += amount;
    return true;
}

const std::string *text(const DRW_Variant *value, int code) {
    return value && value->code() == code && value->type() == DRW_Variant::STRING
        ? value->content.s : nullptr;
}
bool literal(const DRW_Variant *value, int code, std::string_view expected) {
    const auto *s = text(value, code);
    return s && *s == expected;
}
int brace(const DRW_Variant *value) {
    if (literal(value, 1002, "{")) return 1;
    if (literal(value, 1002, "}")) return -1;
    return 0;
}
bool application(const DRW_Variant *value) {
    const auto *s = text(value, 1001);
    if (!s || s->size() != 8) return false;
    constexpr std::string_view lower = "librecad";
    for (std::size_t i = 0; i < lower.size(); ++i) {
        unsigned char c = static_cast<unsigned char>((*s)[i]);
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (c != static_cast<unsigned char>(lower[i])) return false;
    }
    return true;
}
bool integer16(const DRW_Variant *v, std::int32_t &out) {
    if (!v || v->code() != 1070 || v->type() != DRW_Variant::INTEGER) return false;
    out = v->i_val();
    return out >= -32768 && out <= 32767;
}
bool number(const DRW_Variant *v, int code, double &out) {
    if (!v || v->code() != code || v->type() != DRW_Variant::DOUBLE) return false;
    out = v->d_val();
    return std::isfinite(out);
}
bool coordinate(const DRW_Variant *v, int code, RS_Vector &out) {
    if (!v || v->code() != code || v->type() != DRW_Variant::COORD || !v->coord()) return false;
    const auto &p = *v->coord();
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return false;
    out = RS_Vector{p.x, p.y, p.z};
    return true;
}

bool optionsValid(const Options &o) {
    if (o.textEncoding != TextEncoding::Utf8 && o.textEncoding != TextEncoding::Utf16 &&
        o.textEncoding != TextEncoding::Legacy) return false;
    return (!o.eed || o.textEncoding != TextEncoding::Utf8) &&
        (o.textEncoding != TextEncoding::Legacy || bool(o.legacyEncodedBytes));
}

// Admission precedes even the shallow pointer copy. Payload sizes are inspected
// without copying strings or allocating a removal bitmap proportional to input.
bool bounded(const XData &input, const Options &o) {
    if (input.size() > std::min(o.maximumItems, HardItems)) return false;
    const auto cap = std::min(o.maximumBytes, HardBytes);
    std::size_t used = 0;
    for (const auto &v : input) {
        if (!charge(used, sizeof(std::shared_ptr<DRW_Variant>), cap)) return false;
        if (!v) continue;
        if (v->type() == DRW_Variant::STRING && v->content.s &&
            !charge(used, v->content.s->size(), cap)) return false;
        if (v->type() == DRW_Variant::BINARY &&
            !charge(used, v->binary()->size(), cap)) return false;
    }
    return true;
}

struct TextSize { std::size_t utf8 = 0, utf16 = 0, controls = 0; };

bool textBound(const TextSize &size, const Options &o) {
    const auto units = o.eed && o.textEncoding == TextEncoding::Utf16 ? 65535u : 255u;
    return size.utf8 <= std::min(o.maximumBytes, HardBytes) && size.utf8 <= 4u * units;
}

std::optional<TextSize> textSize(std::string_view s, const Options &o) {
    TextSize size{s.size(), 0, 0};
    if (!textBound(size, o) || !QByteArrayView(s.data(), static_cast<qsizetype>(s.size())).isValidUtf8())
        return std::nullopt;
    for (const unsigned char c : s) {
        if (!c || c == '\r' || c == '\n') return std::nullopt;
        if ((c & 0xc0) != 0x80) size.utf16 += c >= 0xf0 ? 2 : 1;
        if (c < 0x20) ++size.controls;
    }
    return size;
}

std::optional<TextSize> textSize(const QString &s, const Options &o) {
    if (static_cast<std::size_t>(s.size()) > MaximumTailBytes || !QStringView{s}.isValidUtf16()) return std::nullopt;
    TextSize size{0, static_cast<std::size_t>(s.size()), 0};
    for (qsizetype i = 0; i < s.size(); ++i) {
        const auto c = s[i].unicode();
        if (!c || c == '\r' || c == '\n') return std::nullopt;
        if (c < 0x20) ++size.controls;
        if (QChar::isHighSurrogate(c)) { size.utf8 += 4; ++i; }
        else size.utf8 += c < 0x80 ? 1 : c < 0x800 ? 2 : 3;
    }
    return textBound(size, o) ? std::optional<TextSize>(size) : std::nullopt;
}

std::optional<std::size_t> encodedTextSize(const TextSize &size, const Options &o, bool caret) {
    const auto extra = caret && !o.eed && o.asciiDxf ? size.controls : 0;
    const auto bytes = o.textEncoding == TextEncoding::Utf16 ? 2*(size.utf16 + extra) : size.utf8 + extra;
    const auto maximum = o.eed && o.textEncoding == TextEncoding::Utf16 ? 65535u * 2u : 255u;
    return bytes <= maximum ? std::optional<std::size_t>(bytes) : std::nullopt;
}

std::optional<std::size_t> stringBytes(std::string_view s, const Options &o, bool caret = true) {
    const auto size = textSize(s, o);
    if (!size) return std::nullopt;
    if (o.textEncoding != TextEncoding::Legacy) return encodedTextSize(*size, o, caret);
    std::string escaped;
    if (caret && !o.eed && o.asciiDxf && size->controls) {
        const auto count = size->utf8 + size->controls;
        if (count > std::min(o.maximumBytes, HardBytes)) return std::nullopt;
        escaped.reserve(count);
        // Exact dxfCaretEncodedControls rule, before the writer's fromUtf8.
        for (const unsigned char c : s) {
            if (c < 0x20) { escaped.push_back('^'); escaped.push_back(static_cast<char>(c + 0x40)); }
            else escaped.push_back(static_cast<char>(c));
        }
        s = escaped;
    }
    try {
        const auto bytes = o.legacyEncodedBytes(s);
        return bytes && *bytes <= 255 ? bytes : std::nullopt;
    } catch (...) { return std::nullopt; }
}

std::optional<std::size_t> stringBytes(const QString &s, const Options &o) {
    const auto size = textSize(s, o);
    if (!size) return std::nullopt;
    if (o.textEncoding != TextEncoding::Legacy) return encodedTextSize(*size, o, true);
    const auto utf8 = s.toUtf8(); // The source envelope was admitted above.
    return stringBytes(std::string_view(utf8.constData(), static_cast<std::size_t>(utf8.size())), o);
}

bool hex(std::string_view s) {
    return std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    });
}

std::optional<std::size_t> binarySize(const DRW_Variant *v, const Options &o) {
    std::size_t count;
    if (v && v->type() == DRW_Variant::BINARY) {
        if (v->content.b != v->binary()) return std::nullopt;
        count = v->binary()->size();
    } else {
        const auto *s = text(v, 1004);
        if (!s || s->size() % 2 || !hex(*s)) return std::nullopt;
        count = s->size()/2;
    }
    // dxfRW's writer accepts a bounded payload and emits 127-byte chunks.
    if (count > (o.eed ? 255u : HardBytes)) return std::nullopt;
    return count;
}

std::optional<std::size_t> itemCount(const DRW_Variant *v, const Options &o) {
    if (!v || v->code() != 1004) return 1;
    const auto count = binarySize(v, o);
    if (!count) return std::nullopt;
    return o.eed ? 1 : std::max<std::size_t>(1, *count/127 + (*count % 127 != 0));
}

// Common typed payload sizes; no DWG buffers, resolver or writer APIs are used.
std::optional<std::size_t> itemBytes(const DRW_Variant *v, const Options &o, bool foreign) {
    if (!v) return std::nullopt;
    const int code = v->code();
    if (code == 1000) {
        const auto *s = text(v, code);
        if (!s) return std::nullopt;
        const auto bytes = stringBytes(*s, o);
        if (!bytes) return std::nullopt;
        return *bytes + (o.eed && o.textEncoding == TextEncoding::Legacy ? 4u : 3u);
    }
    if (code == 1002) return brace(v) ? std::optional<std::size_t>(2) : std::nullopt;
    if (code >= 1010 && code <= 1013) {
        RS_Vector p;
        return coordinate(v, code, p) ? std::optional<std::size_t>(25) : std::nullopt;
    }
    if (code >= 1040 && code <= 1042) {
        double n;
        return number(v, code, n) ? std::optional<std::size_t>(9) : std::nullopt;
    }
    if (code == 1070) {
        if (v->type() != DRW_Variant::INTEGER || v->i_val() < -32768 ||
            v->i_val() > (foreign ? 65535 : 32767)) return std::nullopt;
        return 3;
    }
    if (code == 1071) {
        if (v->type() == DRW_Variant::INTEGER) return 5;
        // Existing foreign 1071 transport also accepts a range-checked int64.
        if (foreign && v->type() == DRW_Variant::INTEGER64 &&
            v->i64_val() >= std::numeric_limits<std::int32_t>::min() &&
            v->i64_val() <= std::numeric_limits<std::int32_t>::max()) return 5;
        return std::nullopt;
    }
    if (!foreign) return std::nullopt;
    if (code == 1003) {
        const auto *s = text(v, code);
        if (!s) return std::nullopt;
        // EED resolves the spelling to an eight-byte handle; DXF writes the
        // encoded layer name without code-1000 caret escaping.
        if (o.eed) return 9;
        const auto bytes = stringBytes(std::string_view(*s), o, false);
        return bytes ? std::optional<std::size_t>(*bytes + 3u) : std::nullopt;
    }
    if (code == 1005) {
        const auto *s = text(v, code);
        if (!s) return std::nullopt;
        std::string_view value = *s;
        if (value.size() >= 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) value.remove_prefix(2);
        return !value.empty() && value.size() <= 16 && hex(value) ? std::optional<std::size_t>(9) : std::nullopt;
    }
    if (code == 1004) {
        const auto count = binarySize(v, o), items = itemCount(v, o);
        if (!count || !items) return std::nullopt;
        const auto framing = !o.eed && !o.asciiDxf ? (o.binaryDxfR12 ? 4u : 3u) : 2u;
        const auto payload = !o.eed && o.asciiDxf ? 2 * *count : *count;
        std::size_t bytes = payload;
        if (!charge(bytes, framing * *items, HardBytes)) return std::nullopt;
        return bytes;
    }
    return std::nullopt;
}

bool normalize(LC_EllipseOffsetDefinition &g) {
    const double tolerance = LC_CurveOffset::kEllipseRelativeOffsetTolerance;
    return LC_EllipseOffsetGeometry::isFinite2D(g.basis.center) &&
        LC_EllipseOffsetGeometry::isFinite2D(g.basis.majorP) &&
        std::isfinite(g.distance) && g.distance > 0 && std::isfinite(g.relTolerance) &&
        g.relTolerance >= .1 * tolerance && g.relTolerance <= 10 * tolerance &&
        LC_EllipseOffsetGeometry::normaliseBasis(g.basis);
}

struct CandidateEnd { std::size_t end; bool closed; bool validBraces; };
CandidateEnd candidateEnd(const XData &input, std::size_t open, std::size_t groupEnd) {
    std::size_t depth = 1;
    bool valid = true;
    for (std::size_t i = open + 1; i < groupEnd; ++i) {
        const auto *v = input[i].get();
        const int b = brace(v);
        if (b > 0) ++depth;
        else if (b < 0 && --depth == 0) return {i + 1, true, valid};
        if (v && v->code() == 1002 && !b) valid = false;
    }
    return {groupEnd, false, valid};
}

std::optional<LC_OffsetCurveExtension> extension(const DRW_Variant &v) {
    const int code = v.code();
    if (code == 1000 || code == 1002) {
        const auto *s = text(&v, code);
        if (!s || s->size() > 4 * MaximumTailBytes) return std::nullopt;
        QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::ConvertInitialBom);
        const QString value = decoder(QByteArrayView(s->data(), static_cast<qsizetype>(s->size())));
        if (decoder.hasError()) return std::nullopt;
        return LC_OffsetCurveExtension{code, value};
    }
    if (code >= 1010 && code <= 1013) {
        RS_Vector p;
        if (coordinate(&v, code, p)) return LC_OffsetCurveExtension{code, p};
    } else if (code >= 1040 && code <= 1042) {
        double d;
        if (number(&v, code, d)) return LC_OffsetCurveExtension{code, d};
    } else if (code == 1070 && v.type() == DRW_Variant::INTEGER && v.i_val() >= -32768 && v.i_val() <= 32767)
        return LC_OffsetCurveExtension{code, static_cast<std::int16_t>(v.i_val())};
    else if (code == 1071 && v.type() == DRW_Variant::INTEGER)
        return LC_OffsetCurveExtension{code, v.i_val()};
    return std::nullopt;
}

OwnedTail readTail(const XData &input, std::size_t begin, std::size_t end,
    const Options &o, Diagnostics &diagnostics) {
    if (end - begin > MaximumTailItems) {
        diagnose(diagnostics, o, Reason::TailLimit, begin);
        return {};
    }
    std::size_t bytes = 0;
    unsigned depth = 0;
    bool valid = true;
    for (std::size_t i = begin; i < end; ++i) {
        const auto *v = input[i].get();
        if (v && (v->code() == 1003 || v->code() == 1004 || v->code() == 1005)) {
            diagnose(diagnostics, o, Reason::TailUnsupported, i);
            continue;
        }
        const auto size = itemBytes(v, o, false);
        if (!size) { diagnose(diagnostics, o, Reason::TailInvalid, i); valid = false; continue; }
        if (!charge(bytes, *size, MaximumTailBytes)) {
            diagnose(diagnostics, o, Reason::TailLimit, i); valid = false;
        }
        const int b = brace(v);
        if (b > 0) {
            if (++depth > MaximumTailDepth) valid = false;
        } else if (b < 0) {
            if (!depth) valid = false;
            else --depth;
        }
    }
    if (!valid || depth) {
        diagnose(diagnostics, o, Reason::TailInvalid, begin);
        return {};
    }
    OwnedTail tail;
    tail.reserve(end - begin);
    for (std::size_t i = begin; i < end; ++i) {
        const auto &v = *input[i];
        if (v.code() == 1003 || v.code() == 1004 || v.code() == 1005) continue;
        auto item = extension(v);
        if (!item) { diagnose(diagnostics, o, Reason::TailInvalid, i); return {}; }
        tail.push_back(std::move(*item));
    }
    return tail;
}

std::optional<Definition> readKnown(const XData &input, std::size_t marker,
    const CandidateEnd &end, const Options &o, Diagnostics &diagnostics, bool decodeTail) {
    // Marker, open, schema, kind, center, displacement, ratio, a1, a2,
    // flags, distance, tolerance, [tail], close.
    if (!end.closed || !end.validBraces || end.end - marker < 13) return std::nullopt;
    Definition result;
    auto &g = result.geometry;
    std::int32_t flags;
    if (!literal(input[marker + 3].get(), 1000, "ELLIPSE") ||
        !coordinate(input[marker + 4].get(), 1011, g.basis.center) ||
        !coordinate(input[marker + 5].get(), 1012, g.basis.majorP) ||
        !number(input[marker + 6].get(), 1040, g.basis.ratio) ||
        !number(input[marker + 7].get(), 1040, g.basis.angle1) ||
        !number(input[marker + 8].get(), 1040, g.basis.angle2) ||
        !integer16(input[marker + 9].get(), flags) || flags < 0 ||
        !number(input[marker + 10].get(), 1041, g.distance) ||
        !number(input[marker + 11].get(), 1040, g.relTolerance)) return std::nullopt;
    g.basis.reversed = (flags & 1) != 0;
    g.towardCentre = (flags & 2) != 0;
    g.cacheReversed = (flags & 4) != 0;
    if (bool(flags & 8) != (g.towardCentre != g.basis.reversed) ||
        g.basis.center.z != 0 || g.basis.majorP.z != 0 || !normalize(g)) return std::nullopt;
    result.reservedFlags = static_cast<unsigned>(flags) & ReservedFlags;
    if (decodeTail) result.tail = readTail(input, marker + 12, end.end - 1, o, diagnostics);
    return result;
}

ParseResult parse(const XData &input, const Options &o, bool decodeTail = true) {
    ParseResult result;
    if (!optionsValid(o) || !bounded(input, o)) {
        result.status = Status::Invalid;
        diagnose(result.diagnostics, o, optionsValid(o) ? Reason::ResourceLimit : Reason::InvalidOptions, 0);
        return result;
    }
    XData remaining;
    remaining.reserve(input.size());
    bool invalid = false, other = false;
    std::size_t start = 0;
    while (start < input.size()) {
        const bool isGroup = input[start] && input[start]->code() == 1001;
        std::size_t end = start + 1;
        while (end < input.size() && (!input[end] || input[end]->code() != 1001)) ++end;
        if (!isGroup || !application(input[start].get())) {
            remaining.insert(remaining.end(), input.begin() + start, input.begin() + end);
            start = end;
            continue;
        }
        remaining.push_back(input[start]);
        std::size_t depth = 0;
        for (std::size_t i = start + 1; i < end;) {
            if (!depth && literal(input[i].get(), 1000, Marker) &&
                i + 1 < end && brace(input[i + 1].get()) == 1) {
                const auto boundary = candidateEnd(input, i + 1, end);
                std::int32_t schema = 0;
                const bool typed = i + 2 < end && integer16(input[i + 2].get(), schema);
                if (typed && schema != 1) {
                    other = true;
                    if (!boundary.closed || !boundary.validBraces)
                        diagnose(result.diagnostics, o, Reason::MalformedUnknown, i);
                    remaining.insert(remaining.end(), input.begin() + i, input.begin() + boundary.end);
                } else {
                    ++result.knownCandidates;
                    auto definition = typed ? readKnown(input, i, boundary, o, result.diagnostics, decodeTail) : std::nullopt;
                    if (!definition) {
                        invalid = true;
                        diagnose(result.diagnostics, o, Reason::MalformedKnown, i);
                    } else if (result.knownCandidates == 1) result.definition = std::move(definition);
                    if (result.knownCandidates > 1) {
                        invalid = true;
                        diagnose(result.diagnostics, o, Reason::DuplicateKnown, i);
                    }
                }
                i = boundary.end;
                continue;
            }
            const int b = brace(input[i].get());
            if (b > 0) ++depth;
            else if (b < 0) {
                if (depth) --depth;
                else { invalid = true; diagnose(result.diagnostics, o, Reason::MalformedGroup, i); }
            } else if (input[i] && input[i]->code() == 1002) {
                invalid = true; diagnose(result.diagnostics, o, Reason::MalformedGroup, i);
            }
            remaining.push_back(input[i++]);
        }
        if (depth) { invalid = true; diagnose(result.diagnostics, o, Reason::MalformedGroup, start); }
        start = end;
    }
    result.status = invalid ? Status::Invalid : result.definition ? Status::Schema1 :
        other ? Status::Other : Status::NoSubList;
    if (invalid) result.definition.reset();
    result.remaining = std::move(remaining);
    return result;
}

std::shared_ptr<DRW_Variant> writeExtension(const LC_OffsetCurveExtension &item) {
    const int code = item.code;
    if (code == 1000 || code == 1002) {
        const auto *s = std::get_if<QString>(&item.value);
        if (!s || s->size() > 4096 || s->contains(QChar(0)) || !QStringView{*s}.isValidUtf16()) return {};
        const auto utf8 = s->toUtf8();
        return std::make_shared<DRW_Variant>(code, std::string(utf8.constData(), static_cast<std::size_t>(utf8.size())));
    }
    if (code >= 1010 && code <= 1013) {
        const auto *p = std::get_if<RS_Vector>(&item.value);
        if (p && p->valid && std::isfinite(p->x) && std::isfinite(p->y) && std::isfinite(p->z))
            return std::make_shared<DRW_Variant>(code, DRW_Coord{p->x, p->y, p->z});
    } else if (code >= 1040 && code <= 1042) {
        const auto *d = std::get_if<double>(&item.value);
        if (d && std::isfinite(*d)) return std::make_shared<DRW_Variant>(code, *d);
    } else if (code == 1070) {
        if (const auto *i = std::get_if<std::int16_t>(&item.value))
            return std::make_shared<DRW_Variant>(code, static_cast<std::int32_t>(*i));
    } else if (code == 1071) {
        if (const auto *i = std::get_if<std::int32_t>(&item.value)) return std::make_shared<DRW_Variant>(code, *i);
    }
    return {};
}

std::optional<std::size_t> extensionBytes(const LC_OffsetCurveExtension &item, const Options &o, bool storage) {
    const int code = item.code;
    if (code == 1000 || code == 1002) {
        const auto *s = std::get_if<QString>(&item.value);
        if (!s) return std::nullopt;
        const auto size = textSize(*s, o);
        if (!size) return std::nullopt;
        if (code == 1002) {
            if (*s != QStringLiteral("{") && *s != QStringLiteral("}")) return std::nullopt;
            return storage ? 1 : 2;
        }
        if (storage) return size->utf8;
        const auto bytes = stringBytes(*s, o);
        if (bytes) return *bytes + (o.eed && o.textEncoding == TextEncoding::Legacy ? 4u : 3u);
        return std::nullopt;
    }
    if (code >= 1010 && code <= 1013) {
        const auto *p = std::get_if<RS_Vector>(&item.value);
        if (p && p->valid && std::isfinite(p->x) && std::isfinite(p->y) && std::isfinite(p->z)) return storage ? 0 : 25;
    } else if (code >= 1040 && code <= 1042) {
        const auto *d = std::get_if<double>(&item.value);
        if (d && std::isfinite(*d)) return storage ? 0 : 9;
    } else if (code == 1070 && std::holds_alternative<std::int16_t>(item.value)) return storage ? 0 : 3;
    else if (code == 1071 && std::holds_alternative<std::int32_t>(item.value)) return storage ? 0 : 5;
    return std::nullopt;
}

struct CurrentSize {
    LC_EllipseOffsetDefinition geometry;
    std::size_t items = 0, storage = 0, encoded = 0;
};

std::optional<CurrentSize> currentStorage(const Definition &current, const Options &o, Diagnostics &diagnostics) {
    CurrentSize size{current.geometry, 13 + current.tail.size(), 0, 0};
    if ((current.reservedFlags & ~ReservedFlags) || !normalize(size.geometry)) {
        diagnose(diagnostics, o, Reason::InvalidDefinition, 0); return std::nullopt;
    }
    const auto cap = std::min(o.maximumBytes, HardBytes);
    // Marker (15), kind (7), and the two one-byte brace strings.
    if (!charge(size.storage, size.items * sizeof(std::shared_ptr<DRW_Variant>) + 24, cap)) {
        diagnose(diagnostics, o, Reason::ResourceLimit, 0); return std::nullopt;
    }
    unsigned depth = 0;
    for (std::size_t i = 0; i < current.tail.size(); ++i) {
        const auto &item = current.tail[i];
        const auto bytes = extensionBytes(item, o, true);
        if (!bytes || !charge(size.storage, *bytes, cap)) {
            diagnose(diagnostics, o, bytes ? Reason::ResourceLimit : Reason::TailInvalid, i); return std::nullopt;
        }
        if (item.code == 1002) {
            const auto &s = std::get<QString>(item.value);
            if (s == QStringLiteral("{")) {
                if (++depth > MaximumTailDepth) { diagnose(diagnostics, o, Reason::TailLimit, i); return std::nullopt; }
            } else if (!depth) { diagnose(diagnostics, o, Reason::TailInvalid, i); return std::nullopt; }
            else --depth;
        }
    }
    if (depth) { diagnose(diagnostics, o, Reason::TailInvalid, 0); return std::nullopt; }
    return size;
}

bool currentEncoding(const Definition &current, CurrentSize &size, const Options &o, Diagnostics &diagnostics) {
    std::size_t tail = 0;
    for (std::size_t i = 0; i < current.tail.size(); ++i) {
        const auto bytes = extensionBytes(current.tail[i], o, false);
        if (!bytes || !charge(tail, *bytes, MaximumTailBytes)) {
            diagnose(diagnostics, o, bytes ? Reason::TailLimit : Reason::TailInvalid, i); return false;
        }
    }
    const auto marker = stringBytes(std::string_view(Marker), o);
    const auto kind = stringBytes(std::string_view("ELLIPSE"), o);
    if (!marker || !kind) { diagnose(diagnostics, o, Reason::Encoding, 0); return false; }
    const auto framing = o.eed && o.textEncoding == TextEncoding::Legacy ? 4u : 3u;
    // Two coordinates, five doubles, two int16 values, and two braces.
    size.encoded = 105 + 2*framing + *marker + *kind + tail;
    return true;
}

std::optional<XData> writeCurrent(const Definition &current, const CurrentSize &size) {
    const auto &g = size.geometry;
    const unsigned flags = current.reservedFlags | (g.basis.reversed ? 1u : 0u) |
        (g.towardCentre ? 2u : 0u) | (g.cacheReversed ? 4u : 0u) |
        (g.towardCentre != g.basis.reversed ? 8u : 0u);
    XData out;
    out.reserve(size.items);
    out.push_back(std::make_shared<DRW_Variant>(1000, std::string(Marker)));
    out.push_back(std::make_shared<DRW_Variant>(1002, std::string("{")));
    out.push_back(std::make_shared<DRW_Variant>(1070, std::int32_t{1}));
    out.push_back(std::make_shared<DRW_Variant>(1000, std::string("ELLIPSE")));
    out.push_back(std::make_shared<DRW_Variant>(1011, DRW_Coord{g.basis.center.x, g.basis.center.y, 0}));
    out.push_back(std::make_shared<DRW_Variant>(1012, DRW_Coord{g.basis.majorP.x, g.basis.majorP.y, 0}));
    out.push_back(std::make_shared<DRW_Variant>(1040, g.basis.ratio));
    out.push_back(std::make_shared<DRW_Variant>(1040, g.basis.angle1));
    out.push_back(std::make_shared<DRW_Variant>(1040, g.basis.angle2));
    out.push_back(std::make_shared<DRW_Variant>(1070, static_cast<std::int32_t>(flags)));
    out.push_back(std::make_shared<DRW_Variant>(1041, g.distance));
    out.push_back(std::make_shared<DRW_Variant>(1040, g.relTolerance));
    for (const auto &item : current.tail) {
        auto value = writeExtension(item);
        if (!value) return std::nullopt;
        out.push_back(std::move(value));
    }
    out.push_back(std::make_shared<DRW_Variant>(1002, std::string("}")));
    return out;
}

std::size_t groupEnd(const XData &data, std::size_t begin) {
    std::size_t end = begin + 1;
    while (end < data.size() && (!data[end] || data[end]->code() != 1001)) ++end;
    return end;
}
bool balanced(const XData &data, std::size_t begin, std::size_t end) {
    std::size_t depth = 0;
    for (std::size_t i = begin; i < end; ++i) {
        const int b = brace(data[i].get());
        if (b > 0) ++depth;
        else if (b < 0) { if (!depth) return false; --depth; }
        else if (data[i] && data[i]->code() == 1002) return false;
    }
    return depth == 0;
}

bool preflightEncoding(const XData &raw, std::size_t preamble, std::size_t first, bool keepOurs,
    const std::optional<CurrentSize> &owned, const Options &o, Diagnostics &diagnostics) {
    const auto cap = std::min(o.maximumBytes, HardBytes);
    std::size_t total = 0, ours = owned ? owned->encoded : 0;
    if (!charge(total, ours, cap)) { diagnose(diagnostics, o, Reason::ResourceLimit, 0); return false; }
    if (o.eed && (preamble || ours > 65535)) {
        diagnose(diagnostics, o, preamble ? Reason::Encoding : Reason::EedChunkLimit, 0); return false;
    }
    for (std::size_t i = 0; i < preamble; ++i) {
        const auto bytes = itemBytes(raw[i].get(), o, true);
        if (!bytes) { diagnose(diagnostics, o, Reason::Encoding, i); return false; }
        if (!charge(total, *bytes, cap)) { diagnose(diagnostics, o, Reason::ResourceLimit, i); return false; }
    }
    for (std::size_t begin = preamble; begin < raw.size();) {
        const auto end = groupEnd(raw, begin);
        const bool isOurs = application(raw[begin].get());
        if (isOurs && !keepOurs) { begin = end; continue; }
        if (!balanced(raw, begin + 1, end)) { diagnose(diagnostics, o, Reason::UnsafeMerge, begin); return false; }
        if (!isOurs || begin == first) {
            const auto *s = text(raw[begin].get(), 1001);
            const auto bytes = s && !s->empty() ? stringBytes(std::string_view(*s), o, false) : std::nullopt;
            if (!bytes) { diagnose(diagnostics, o, Reason::Encoding, begin); return false; }
            if (!charge(total, *bytes + 3, cap)) { diagnose(diagnostics, o, Reason::ResourceLimit, begin); return false; }
        }
        std::size_t chunk = 0;
        for (std::size_t i = begin + 1; i < end; ++i) {
            const auto bytes = itemBytes(raw[i].get(), o, true);
            if (!bytes) { diagnose(diagnostics, o, Reason::Encoding, i); return false; }
            if (!charge(total, *bytes, cap)) { diagnose(diagnostics, o, Reason::ResourceLimit, i); return false; }
            if (o.eed && !charge(isOurs ? ours : chunk, *bytes, 65535)) {
                diagnose(diagnostics, o, Reason::EedChunkLimit, i); return false;
            }
        }
        if (o.eed && !isOurs && chunk == 0) { diagnose(diagnostics, o, Reason::UnsafeMerge, begin); return false; }
        begin = end;
    }
    if (owned && first == raw.size()) {
        const auto bytes = stringBytes(std::string_view(AppId), o, false);
        if (!bytes || !charge(total, *bytes + 3, cap)) { diagnose(diagnostics, o, Reason::ResourceLimit, 0); return false; }
    }
    return true;
}

std::optional<XData> merge(const XData &input, const std::optional<Definition> &current,
    const Options &o, Diagnostics &diagnostics) {
    const auto itemCap = std::min(o.maximumItems, HardItems), byteCap = std::min(o.maximumBytes, HardBytes);
    if (!optionsValid(o)) { diagnose(diagnostics, o, Reason::InvalidOptions, 0); return std::nullopt; }
    if (current) {
        if (current->tail.size() > MaximumTailItems) { diagnose(diagnostics, o, Reason::TailLimit, 0); return std::nullopt; }
        const auto minimum = 14 + current->tail.size();
        if (minimum > itemCap || minimum*sizeof(std::shared_ptr<DRW_Variant>) + 32 > byteCap) {
            diagnose(diagnostics, o, Reason::ResourceLimit, 0); return std::nullopt;
        }
    }
    // Export needs stale sublists removed, not decoded owned tails or codec
    // callbacks for data it will discard.
    auto parsed = parse(input, o, false);
    diagnostics = std::move(parsed.diagnostics);
    if (!parsed.remaining) return std::nullopt;
    auto &raw = *parsed.remaining;
    std::optional<CurrentSize> ownedSize;
    if (current) {
        ownedSize = currentStorage(*current, o, diagnostics);
        if (!ownedSize) return std::nullopt;
    }
    std::size_t first = raw.size(), preamble = 0;
    std::size_t oursPayload = 0;
    while (preamble < raw.size() && (!raw[preamble] || raw[preamble]->code() != 1001)) ++preamble;
    for (std::size_t begin = preamble; begin < raw.size();) {
        const auto end = groupEnd(raw, begin);
        const bool ours = application(raw[begin].get());
        if (ours) {
            if (!balanced(raw, begin + 1, end)) {
                diagnose(diagnostics, o, Reason::UnsafeMerge, begin); return std::nullopt;
            }
            if (first == raw.size()) first = begin;
            oursPayload += end - begin - 1;
        }
        begin = end;
    }
    const bool keepOurs = oursPayload || bool(current);
    std::size_t count = ownedSize ? ownedSize->items : 0, emitted = count;
    std::size_t storage = ownedSize ? ownedSize->storage : 0;
    const auto admit = [&](const std::shared_ptr<DRW_Variant> &v, std::size_t i) {
        const auto items = itemCount(v.get(), o);
        if (!items || !charge(emitted, *items, itemCap) || !charge(count, 1, itemCap) ||
            !charge(storage, sizeof(std::shared_ptr<DRW_Variant>), byteCap)) {
            diagnose(diagnostics, o, items ? Reason::ResourceLimit : Reason::Encoding, i); return false;
        }
        const auto payload = v && v->type() == DRW_Variant::STRING && v->content.s ? v->content.s->size() :
            v && v->type() == DRW_Variant::BINARY ? v->binary()->size() : 0;
        if (!charge(storage, payload, byteCap)) { diagnose(diagnostics, o, Reason::ResourceLimit, i); return false; }
        return true;
    };
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (application(raw[i].get()) && (!keepOurs || i != first)) continue;
        if (!admit(raw[i], i)) return std::nullopt;
    }
    if (ownedSize && first == raw.size() &&
        (!charge(count, 1, itemCap) || !charge(emitted, 1, itemCap) ||
         !charge(storage, sizeof(std::shared_ptr<DRW_Variant>) + 8, byteCap))) {
        diagnose(diagnostics, o, Reason::ResourceLimit, 0); return std::nullopt;
    }
    if (count > XData{}.max_size()) { diagnose(diagnostics, o, Reason::ResourceLimit, 0); return std::nullopt; }
    if (ownedSize && !currentEncoding(*current, *ownedSize, o, diagnostics)) return std::nullopt;
    if (!preflightEncoding(raw, preamble, first, keepOurs, ownedSize, o, diagnostics)) return std::nullopt;
    std::optional<XData> owned;
    if (current) { owned = writeCurrent(*current, *ownedSize); if (!owned) return std::nullopt; }
    XData out;
    out.reserve(count);
    out.insert(out.end(), raw.begin(), raw.begin() + preamble);
    for (std::size_t begin = preamble; begin < raw.size();) {
        const auto end = groupEnd(raw, begin);
        const bool ours = application(raw[begin].get());
        if (ours && (!keepOurs || begin != first)) { begin = end; continue; }
        out.insert(out.end(), raw.begin() + begin, raw.begin() + end);
        if (begin == first) {
            for (std::size_t later = end; later < raw.size();) {
                const auto last = groupEnd(raw, later);
                if (application(raw[later].get())) out.insert(out.end(), raw.begin() + later + 1, raw.begin() + last);
                later = last;
            }
            if (owned) out.insert(out.end(), owned->begin(), owned->end());
        }
        begin = end;
    }
    if (owned && first == raw.size()) {
        out.push_back(std::make_shared<DRW_Variant>(1001, std::string(AppId)));
        out.insert(out.end(), owned->begin(), owned->end());
    }
    return out;
}
} // namespace

ParseResult parseAndStrip(const XData &input, const Options &options) {
    try { return parse(input, options); }
    catch (const std::bad_alloc &) {
        ParseResult failed;
        failed.status = Status::Invalid;
        return failed;
    }
}

std::optional<XData> forExport(const XData &input, const std::optional<Definition> &current,
    const Options &options, Diagnostics *diagnostics) {
    Diagnostics local;
    std::optional<XData> result;
    try { result = merge(input, current, options, local); }
    catch (const std::bad_alloc &) { result.reset(); }
    if (diagnostics) *diagnostics = std::move(local);
    return result;
}
} // namespace LC_OffsetCurveXData
