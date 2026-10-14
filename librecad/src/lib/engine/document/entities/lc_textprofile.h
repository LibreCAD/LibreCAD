/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTPROFILE_H
#define LC_TEXTPROFILE_H

#include <QByteArray>
#include <QString>
#include <QVariant>
#include <memory>
#include <optional>
#include <vector>

/** Provisional profile value/codec, not permission to activate saved profiles.
 * P0 transport proof must precede schema freeze/public import activation. This
 * QtCore-only service neither loads fonts nor certifies a shaping engine.
 */
class LC_TextProfile final {
public:
    static constexpr int SchemaVersion = 1;
    static constexpr std::size_t MaximumTags = 64;
    static constexpr std::size_t MaximumBlockBytes = 4096;
    static constexpr int MaximumStringBytes = 255;
    static QString applicationName();

    enum class Scope { StyleDefaults, EntityOverride };
    enum class Backend { LegacyLff, HarfBuzzOpenType };
    enum class Direction { Auto = 0, LeftToRight = 1, RightToLeft = 2, ByStyle = 3 };
    enum class RtlMarker { None, LegacyReversed, LogicalRtl };
    struct Identity {
        // Same value fields as LC_TextFontRegistry::Identity, without a runtime
        // registry dependency. The adapter must still resolve that exact identity.
        QString profileId;
        QByteArray fontSha256;
        unsigned int faceIndex = 0;
        QString metricPolicy;
        QString shapingPolicy;
        QString engineIdentity;
        bool operator==(const Identity &other) const;
    };
    struct Selection {
        Backend backend = Backend::LegacyLff;
        Identity identity; // Empty for an explicit LegacyLff opt-out.
        bool operator==(const Selection &other) const;
    };
    struct Value {
        Scope scope = Scope::EntityOverride;
        // Selection/identity are atomic. An omitted entity selection inherits;
        // explicit LegacyLff is an opt-out, not an absent OpenType profile.
        std::optional<Selection> selection;
        std::optional<QByteArray> language;
        std::optional<Direction> direction;
        bool operator==(const Value &other) const;
    };
    struct Tag {
        int code = 0;
        QVariant value;
        // Exact-type semantic equality, without numeric/string coercion.
        // Not bitwise wire/dirty authority: NaNs and signed zeros follow
        // QVariant value equality. Retained raw provenance is separate.
        bool operator==(const Tag &other) const;
    };
    using Block = std::vector<Tag>;
    enum class ErrorCode {
        None, InvalidNamespace, InvalidType, InvalidSchema, UnknownSchema,
        UnknownBackend, UnknownField, DuplicateField, MissingField,
        InvalidField, InvalidScope, ComplexityLimit, LegacyOrderConflict
    };
    struct Error {
        ErrorCode code = ErrorCode::None;
        QString message;
        QString field;
        explicit operator bool() const { return code != ErrorCode::None; }
    };
    enum class DecodeStatus { Absent, Known, Opaque, Invalid };
    struct Decoded {
        DecodeStatus status = DecodeStatus::Absent;
        std::optional<Value> value;
        // One isolated APPID block, not an entire XDATA stream. Keep this exact
        // block for no-op saves; unrelated namespaces remain caller-owned.
        // Invalid/over-limit input is retained too; it is never silently dropped.
        std::shared_ptr<const Block> raw;
        Error error;
        bool known() const { return status == DecodeStatus::Known && value.has_value(); }
    };
    struct Encoded {
        Block block;
        Error error;
        explicit operator bool() const { return !error; }
    };
    enum class Origin { Fallback, Style, Entity, LogicalRtlMarker };
    struct Effective {
        Selection selection;
        QByteArray language;
        Direction direction = Direction::Auto; // Never unresolved ByStyle.
        Origin selectionOrigin = Origin::Fallback;
        Origin languageOrigin = Origin::Fallback;
        Origin directionOrigin = Origin::Fallback;
    };
    enum class InheritanceStatus { Resolved, LegacyConflict, Unavailable };
    struct Inherited {
        InheritanceStatus status = InheritanceStatus::Unavailable;
        Effective effective;
        // Preserve modeled and raw inputs, including the stored OpenType
        // selection in a marker-1 conflict; do not rewrite it as LegacyLff.
        Decoded style;
        Decoded entity;
        Error error;
        bool openTypeSelected() const {
            return status == InheritanceStatus::Resolved &&
                   effective.selection.backend == Backend::HarfBuzzOpenType;
        }
        // LegacyConflict requests the existing legacy renderer, if available.
        // This says nothing about the availability of any font resource.
        bool requiresLegacyRendering() const {
            return status == InheritanceStatus::LegacyConflict ||
                   (status == InheritanceStatus::Resolved &&
                    effective.selection.backend == Backend::LegacyLff);
        }
    };

    // Provisional schema 1 is one 1001 "LibreCADText", one 1070 schema integer,
    // then keyed 1000 strings. scope=style-default|entity-override is required;
    // backend=legacy-lff|harfbuzz-ot is optional only for entity inheritance.
    // An OpenType selection atomically requires profile=, sha256=, metrics=,
    // shaping=, engine=, and the 1000 "face" /1070 integer pair. language= is a
    // string; direction is a 1000 "direction" /1070 enum pair (0/1/2/3 above).
    // Canonical output uses that order; decoding accepts reordered whole fields.
    // Style defaults require selection, language and direction. Omitted entity
    // fields inherit, and entity ByStyle resolves through marker 2 then style.
    // Unknown schema/backend/fields remain opaque and are never activated.
    // Tag 1070 values must be QVariant::Int, not strings/reals implicitly coerced
    // to numbers. Known encoding emits 1001/1000/1070 only, never STYLE's 1071.
    // Portable face indexes are conservatively limited to signed 16-bit 0..32767.
    // Language is bounded ASCII BCP47-shaped subtags, not registered-tag validation.
    static Error validate(const Value &value);
    // Low-level retention, not a deep ownership boundary: both overloads may
    // share Qt payload buffers with the caller, and the shared-pointer overload
    // retains the supplied Block. Keep borrowed storage alive and do not mutate
    // any reachable/shared payload through aliases or retained data pointers
    // for the entire lifetime of the returned Decoded/raw and its copies.
    // Entity-facing bindings must take their own supported-payload snapshot.
    static Decoded decode(Block block);
    static Decoded decode(std::shared_ptr<const Block> block);
    // Explicit typed edit: canonical encoding is not automatic raw replay.
    static Encoded encode(const Value &value);
    static Inherited inherit(const Decoded &style, const Decoded &entity,
                             const QByteArray &fallbackLanguage, RtlMarker marker = RtlMarker::None);
    static Inherited inherit(const std::optional<Value> &style, const std::optional<Value> &entity,
                             const QByteArray &fallbackLanguage, RtlMarker marker = RtlMarker::None);
};

#endif
