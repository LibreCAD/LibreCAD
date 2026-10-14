/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTENTITYSTATE_H
#define LC_TEXTENTITYSTATE_H

#include "lc_textdocument.h"
#include "lc_textprofilebinding.h"

/** Source/edit authority carried by TEXT/MTEXT data, not a renderer or codec.
 * All accepted input buffers are owned. Copies share immutable state until an
 * edit; imported provenance is never rewritten by text/profile edits. Inputs
 * must not be mutated concurrently with an API call. No profile is activated.
 */
class LC_TextEntityState final {
public:
    using Profile = LC_TextProfile;
    using Syntax = LC_TextDocument::Syntax;
    enum class SourceOrigin { Native, Dxf, Dwg };
    enum class PayloadOrigin { Standalone, AttributeText, EmbeddedMText };
    enum class MarkerOrigin { None, Entity, AttributeOuter };
    // Nonrepresentable/duplicated namespaces remain in the transport's raw
    // sidecar. They are NOT safely absent and must not inherit STYLE defaults.
    // Representable only describes this ownership boundary, not wire validity,
    // schema permission, an available font or permission to activate a profile.
    enum class ProfileTransport { Representable, DuplicateNamespace, UnrepresentableNamespace };
    struct CurrentProfile {
        ProfileTransport transport = ProfileTransport::Representable;
        QString diagnostic;
        // Absent is a present Decoded with status Absent. An unavailable import
        // has no modeled metadata at all, so callers must explicitly gate it.
        std::optional<Profile::Decoded> metadata;
        bool available() const { return transport == ProfileTransport::Representable && metadata.has_value(); }
    };
    struct Convention {
        Syntax syntax = Syntax::Plain;
        bool decodeCaretCodes = false;
        bool decodeDxfEscapes = false;
        bool operator==(const Convention &other) const;
    };
    struct Import {
        QString source; // Before generic decoding or leading alignment stripping.
        // Optional exact original UTF-8, including NUL or invalid sequences.
        // Its explicit-length Qt decoding must equal source; never truncated.
        std::optional<QByteArray> utf8;
        Convention convention;
        SourceOrigin origin = SourceOrigin::Native;
        PayloadOrigin payload = PayloadOrigin::Standalone;
        QString style; // Original STYLE identity, not the resolved legacy alias.
        // Resolved Data.style at import. Required for direct state import;
        // entity importText() supplies its current style when this is absent.
        std::optional<QString> compatibilityStyle;
        Profile::RtlMarker marker = Profile::RtlMarker::None;
        MarkerOrigin markerOrigin = MarkerOrigin::None;
        // One isolated namespace only. Unsupported metatypes are an error;
        // transport retains unsupported/unrelated raw outside this service.
        std::optional<Profile::Block> profile;
        ProfileTransport profileTransport = ProfileTransport::Representable;
        QString profileDiagnostic;
    };
    struct Imported {
        QString source;
        std::optional<QByteArray> utf8;
        Convention convention;
        SourceOrigin origin = SourceOrigin::Native;
        PayloadOrigin payload = PayloadOrigin::Standalone;
        QString style;
        QString compatibilityStyle;
        Profile::RtlMarker marker = Profile::RtlMarker::None;
        MarkerOrigin markerOrigin = MarkerOrigin::None;
        QString compatibilityInput; // Generic decoder output, before A stripping.
        QString compatibilityText; // Final legacy getText() baseline.
        ProfileTransport profileTransport = ProfileTransport::Representable;
        QString profileDiagnostic;
        // Profile provenance lives only in profile().imported().raw, not a
        // second potentially shallow Import.profile snapshot.
    };
    enum class ErrorCode {
        None, InvalidConvention, InvalidProvenance, Utf8Mismatch,
        UnsupportedProfileType, AlreadyImported, AlreadyEdited
    };
    struct Error {
        ErrorCode code = ErrorCode::None;
        QString message;
        explicit operator bool() const { return code != ErrorCode::None; }
    };

    LC_TextEntityState();
    LC_TextEntityState(const LC_TextEntityState &) = default;
    LC_TextEntityState &operator=(const LC_TextEntityState &) = default;
    // Immutable value sharing also leaves moved-from objects fully usable.
    LC_TextEntityState(LC_TextEntityState &&other) noexcept;
    LC_TextEntityState &operator=(LC_TextEntityState &&other) noexcept;

    bool initialized() const;
    const QString &source() const;
    const Convention &convention() const;
    SourceOrigin origin() const;
    // Exact owned current generic-decoder output before one leading A-strip.
    // Distinct from both final compatibilityText() and imported() history.
    const QString &compatibilityInput() const;
    const QString &compatibilityText() const;
    std::shared_ptr<const Imported> imported() const;
    // Includes raw-only edits and convention changes, not merely decoded text.
    bool textEdited() const;
    bool styleEdited(const QString &currentStyle) const;
    // Low-level edit/raw binding only; use currentProfile() for inheritance or
    // activation decisions because raw may instead be held in a DRW sidecar.
    // References returned by state accessors remain valid until this value is
    // edited/assigned; imported() provides its own stable snapshot lifetime.
    const LC_TextProfileBinding &profile() const;
    CurrentProfile currentProfile() const;

    // One import into an unedited host. May precede entity construction or
    // initialize a freshly constructed, unedited host. No re-import mutator.
    // Invalid/opaque/wrong-scope wire profiles are retained inert by the binding.
    Error initializeImport(const Import &, const QString &compatibilityInput);
    // Explicit raw-aware edit. compatibilityInput is the caller's generic
    // legacy interpretation BEFORE stripping one leading \A0;/\A1;/\A2;.
    // A host's final getText() is not interchangeable when it still starts
    // with another alignment directive. No decoding/source-order guessing.
    Error editSource(const QString &, const QString &compatibilityInput, const Convention &);
    // Host-only reconciliation. Constructor finalization consumes the expected
    // imported A-strip without editing raw; copied public Data.text changes are
    // ordinary native edits. The host supplies its actual final legacy text.
    Error acceptLegacyText(const QString &input, const QString &finalText, Syntax, bool constructor);

    Profile::Error replaceProfile(const Profile::Value &);
    Profile::Error removeProfile();
    void resetProfile();

private:
    struct Data;
    std::shared_ptr<const Data> m_data;
};

#endif
