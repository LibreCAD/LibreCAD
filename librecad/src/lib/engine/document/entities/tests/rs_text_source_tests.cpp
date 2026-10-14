/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textentitystate.h"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>
#ifndef LC_TEXT_STATE_ONLY
#include "rs_text.h"
#include "rs_mtext.h"
#endif

namespace {
using State = LC_TextEntityState;
using Profile = LC_TextProfile;
using Syntax = State::Syntax;
using Authority = LC_TextProfileBinding::Authority;
State::Import imported(Syntax syntax, const QString &source = QStringLiteral("raw")) {
    State::Import result;
    result.source = source;
    result.convention = {syntax, true, true};
    result.origin = State::SourceOrigin::Dxf;
    result.style = QStringLiteral("Original STYLE");
    result.compatibilityStyle = QStringLiteral("legacy-alias");
    return result;
}
Profile::Value profile(Profile::Scope scope = Profile::Scope::EntityOverride) {
    return {scope, Profile::Selection{}, QByteArray("ar"), Profile::Direction::RightToLeft};
}
Profile::Block encoded(const Profile::Value &value) {
    const auto result = Profile::encode(value);
    REQUIRE(result);
    return result.block;
}
void initial(State &state, Syntax syntax, const QString &input, const QString &finalText) {
    REQUIRE_FALSE(state.acceptLegacyText(input, finalText, syntax, true));
    CHECK(state.initialized());
    CHECK_FALSE(state.textEdited());
    CHECK(state.compatibilityInput() == input);
    CHECK(state.compatibilityText() == finalText);
}
} // namespace

TEST_CASE("Entity source state separates canonical source and legacy alignment baseline", "[text-source][state][headless]") {
    for (const auto syntax : {Syntax::Plain, Syntax::MText}) {
        State state;
        const auto raw = QStringLiteral("\\A1;\\A2;ب\u200d\u00a0\ufe8f");
        const auto legacy = raw.mid(4);
        initial(state, syntax, raw, legacy);
        CHECK(state.source() == raw);
        CHECK(state.convention() == State::Convention{syntax, false, false});
        CHECK(state.origin() == State::SourceOrigin::Native);
        CHECK_FALSE(state.imported());
        State rawAware = state;
        REQUIRE_FALSE(rawAware.editSource(raw, raw, rawAware.convention()));
        CHECK(rawAware.source() == raw);
        CHECK(rawAware.compatibilityText() == legacy);
        CHECK_FALSE(rawAware.textEdited());
        REQUIRE_FALSE(rawAware.editSource(QStringLiteral("raw-only edit"), raw, rawAware.convention()));
        CHECK(rawAware.source() == QStringLiteral("raw-only edit"));
        CHECK(rawAware.compatibilityText() == legacy);
        CHECK(rawAware.textEdited());
        const auto copy = state;
        REQUIRE_FALSE(state.acceptLegacyText(legacy, legacy, syntax, false));
        REQUIRE_FALSE(state.acceptLegacyText(legacy, legacy, syntax, true));
        CHECK(state.source() == raw);
        CHECK_FALSE(state.textEdited());
        REQUIRE_FALSE(state.acceptLegacyText(QStringLiteral("edited"), QStringLiteral("edited"), syntax, true));
        CHECK(state.source() == QStringLiteral("edited"));
        CHECK(state.textEdited());
        CHECK(copy.source() == raw);
        State moved(std::move(state));
        CHECK(moved.source() == state.source());
        REQUIRE_FALSE(state.editSource(raw, raw, {syntax, false, false}));
        CHECK(state.compatibilityText() == legacy);
        CHECK(moved.source() == QStringLiteral("edited"));
    }
}

TEST_CASE("Imported source text style and profile retain independent immutable authority", "[text-source][state][headless]") {
    for (const auto syntax : {Syntax::Plain, Syntax::MText}) {
        State state;
        auto input = imported(syntax, QStringLiteral("\\A2;{\\H2x;ب}\\P\\U+062A"));
        input.profile = encoded(profile());
        const auto compatibility = QStringLiteral("\\A2;ب\nت");
        REQUIRE_FALSE(state.initializeImport(input, compatibility));
        const auto original = state.imported();
        const auto rawProfile = state.profile().imported().raw;
        REQUIRE(original);
        REQUIRE(rawProfile);
        REQUIRE_FALSE(state.acceptLegacyText(compatibility, compatibility.mid(4), syntax, true));
        CHECK_FALSE(state.textEdited());
        CHECK_FALSE(state.styleEdited(QStringLiteral("legacy-alias")));
        CHECK(state.styleEdited(QStringLiteral("new-font")));
        CHECK(original->style == QStringLiteral("Original STYLE"));
        CHECK(original->compatibilityInput == compatibility);
        CHECK(original->compatibilityText == compatibility.mid(4));
        REQUIRE_FALSE(state.acceptLegacyText(compatibility.mid(4), compatibility.mid(4), syntax, false));
        CHECK(state.source() == input.source);
        REQUIRE_FALSE(state.editSource(QStringLiteral("raw-only edit"), compatibility, input.convention));
        CHECK(state.textEdited());
        CHECK(state.compatibilityText() == original->compatibilityText);
        CHECK(state.origin() == State::SourceOrigin::Native);
        CHECK(state.imported() == original);
        REQUIRE_FALSE(state.replaceProfile(profile()));
        CHECK(state.profile().authority() == Authority::Replace);
        CHECK(state.profile().imported().raw == rawProfile);
        CHECK(state.imported() == original);
        REQUIRE_FALSE(state.removeProfile());
        CHECK(state.profile().authority() == Authority::Remove);
        state.resetProfile();
        CHECK(state.profile().authority() == Authority::Untouched);
        CHECK(state.profile().imported().raw == rawProfile);
        REQUIRE_FALSE(state.editSource(input.source, compatibility, input.convention));
        CHECK_FALSE(state.textEdited()); // Restored exact imported semantics, not reimported.
        CHECK(state.initializeImport(input, compatibility).code == State::ErrorCode::AlreadyImported);
    }
}

TEST_CASE("Entity state deeply owns import buffers including borrowed and QVariant payload storage", "[text-source][state][headless][alias]") {
    QString source = QStringLiteral("ب\u200d\u00a0\ufe8f");
    QString style = QStringLiteral("Original STYLE");
    QString alias = QStringLiteral("legacy-alias");
    QString compatibility = QStringLiteral("\\A1;legacy");
    QByteArray bytes = source.toUtf8();
    auto *sourcePointer = source.data();
    auto *stylePointer = style.data();
    auto *aliasPointer = alias.data();
    auto *compatibilityPointer = compatibility.data();
    auto *bytePointer = bytes.data();
    auto input = imported(Syntax::MText);
    input.source = QString::fromRawData(source.constData(), source.size());
    input.style = QString::fromRawData(style.constData(), style.size());
    input.compatibilityStyle = QString::fromRawData(alias.constData(), alias.size());
    input.utf8 = QByteArray::fromRawData(bytes.constData(), bytes.size());
    input.profile = encoded(profile());
    auto *profilePointer = static_cast<QString *>(input.profile->front().value.data())->data();
    State state;
    REQUIRE_FALSE(state.initializeImport(input, QString::fromRawData(compatibility.constData(), compatibility.size())));
    const auto original = state.imported();
    const auto originalSource = state.source();
    const auto originalBytes = *original->utf8;
    const auto originalProfile = *state.profile().imported().raw;
    sourcePointer[0] = 'X'; stylePointer[0] = 'X'; aliasPointer[0] = 'X';
    compatibilityPointer[0] = 'X'; bytePointer[0] = 'X'; profilePointer[0] = 'X';
    CHECK(state.source() == originalSource);
    CHECK(original->source == originalSource);
    CHECK(*original->utf8 == originalBytes);
    CHECK(original->style == QStringLiteral("Original STYLE"));
    CHECK(original->compatibilityStyle == QStringLiteral("legacy-alias"));
    CHECK(original->compatibilityInput == QStringLiteral("\\A1;legacy"));
    CHECK(original->compatibilityText == QStringLiteral("legacy"));
    CHECK(*state.profile().imported().raw == originalProfile);
    input = {};
    CHECK(state.source() == originalSource);
    CHECK(state.profile().current().known());
}

TEST_CASE("Explicit-length UTF8 retains NUL and invalid sequences or rejects mismatch atomically", "[text-source][state][headless][utf8]") {
    for (const auto &bytes : {QByteArray("A\0B", 3), QByteArray("A\xFF\xC0\0B", 5), QByteArray{}}) {
        auto input = imported(Syntax::Plain, QString::fromUtf8(bytes.constData(), bytes.size()));
        input.utf8 = bytes;
        State state;
        REQUIRE_FALSE(state.initializeImport(input, input.source));
        REQUIRE(state.imported()->utf8);
        CHECK(*state.imported()->utf8 == bytes);
        CHECK(state.source() == input.source);
    }
    State state;
    initial(state, Syntax::Plain, QStringLiteral("before"), QStringLiteral("before"));
    auto input = imported(Syntax::Plain, QStringLiteral("A"));
    input.utf8 = QByteArray("A\0B", 3);
    CHECK(state.initializeImport(input, QStringLiteral("A")).code == State::ErrorCode::Utf8Mismatch);
    CHECK(state.source() == QStringLiteral("before"));
    CHECK_FALSE(state.imported());
    QString malformed;
    for (const auto unit : {0xD800, 0x0628, 0xDC00, 0x200D, 0x00A0, 0xFE8F, 0x0000}) malformed += QChar(unit);
    REQUIRE_FALSE(state.editSource(malformed, malformed, {Syntax::Plain, false, false}));
    CHECK(state.source() == malformed);
    CHECK(state.source().size() == 7);
}

TEST_CASE("Marker and payload provenance validation does not merge old and logical RTL authority", "[text-source][state][headless][negative]") {
    for (const auto marker : {Profile::RtlMarker::LegacyReversed, Profile::RtlMarker::LogicalRtl}) {
        for (const auto payload : {State::PayloadOrigin::Standalone, State::PayloadOrigin::AttributeText, State::PayloadOrigin::EmbeddedMText}) {
            auto input = imported(Syntax::MText);
            input.marker = marker;
            input.payload = payload;
            input.markerOrigin = State::MarkerOrigin::Entity;
            State state;
            REQUIRE_FALSE(state.initializeImport(input, QStringLiteral("legacy")));
            CHECK(state.imported()->marker == marker);
            CHECK(state.imported()->payload == payload);
            CHECK(state.imported()->markerOrigin == State::MarkerOrigin::Entity);
            input.markerOrigin = State::MarkerOrigin::AttributeOuter;
            State outer;
            const auto result = outer.initializeImport(input, QStringLiteral("legacy"));
            CHECK(static_cast<bool>(result) == (payload != State::PayloadOrigin::EmbeddedMText));
            CHECK(outer.initialized() == (payload == State::PayloadOrigin::EmbeddedMText));
        }
    }
    for (const auto bad : {State::MarkerOrigin::Entity, State::MarkerOrigin::AttributeOuter, static_cast<State::MarkerOrigin>(99)}) {
        State state;
        auto input = imported(Syntax::Plain);
        input.markerOrigin = bad;
        CHECK(state.initializeImport(input, QStringLiteral("legacy")).code == State::ErrorCode::InvalidProvenance);
        CHECK_FALSE(state.initialized());
    }
    auto badOrigin = imported(Syntax::Plain);
    auto badPayload = badOrigin;
    auto badMarker = badOrigin;
    auto badSyntax = badOrigin;
    badOrigin.origin = static_cast<State::SourceOrigin>(99);
    badPayload.payload = static_cast<State::PayloadOrigin>(99);
    badMarker.marker = static_cast<Profile::RtlMarker>(99);
    badSyntax.convention.syntax = static_cast<Syntax>(99);
    for (const auto &input : {badOrigin, badPayload, badMarker, badSyntax}) {
        State state;
        initial(state, Syntax::Plain, QStringLiteral("before"), QStringLiteral("before"));
        CHECK(state.initializeImport(input, QStringLiteral("legacy")));
        CHECK(state.source() == QStringLiteral("before"));
        CHECK(state.compatibilityText() == QStringLiteral("before"));
        CHECK_FALSE(state.imported());
        CHECK_FALSE(state.textEdited());
    }
}

TEST_CASE("Invalid scope and unsupported raw types cannot partially replace entity state", "[text-source][state][headless][negative]") {
    State state;
    auto input = imported(Syntax::Plain);
    input.profile = encoded(profile(Profile::Scope::StyleDefaults));
    REQUIRE_FALSE(state.initializeImport(input, QStringLiteral("legacy")));
    CHECK(state.profile().imported().status == Profile::DecodeStatus::Invalid);
    const auto snapshot = state.imported();
    const auto raw = state.profile().imported().raw;
    REQUIRE_FALSE(state.replaceProfile(profile()));
    const auto pending = state.profile().pendingEncoding();
    REQUIRE(pending);
    CHECK(state.replaceProfile(profile(Profile::Scope::StyleDefaults)).code == Profile::ErrorCode::InvalidScope);
    CHECK(state.profile().pendingEncoding() == pending);
    CHECK(state.profile().imported().raw == raw);
    CHECK(state.imported() == snapshot);
    CHECK(state.source() == input.source);
    State unsupported;
    input.profile->push_back({1004, QVariantList{QStringLiteral("borrowed?"), 7}});
    CHECK(unsupported.initializeImport(input, QStringLiteral("legacy")).code == State::ErrorCode::UnsupportedProfileType);
    CHECK_FALSE(unsupported.initialized());
    CHECK_FALSE(unsupported.imported());
    REQUIRE(input.profile);
    CHECK(input.profile->back().value.toList().size() == 2);
    input = imported(Syntax::Plain);
    input.compatibilityStyle.reset();
    CHECK(unsupported.initializeImport(input, QStringLiteral("legacy")).code == State::ErrorCode::InvalidProvenance);
    initial(unsupported, Syntax::Plain, QStringLiteral("native"), QStringLiteral("native"));
    REQUIRE_FALSE(unsupported.acceptLegacyText(QStringLiteral("edit"), QStringLiteral("edit"), Syntax::Plain, false));
    input.compatibilityStyle = QStringLiteral("legacy-alias");
    CHECK(unsupported.initializeImport(input, QStringLiteral("legacy")).code == State::ErrorCode::AlreadyEdited);
    CHECK(unsupported.source() == QStringLiteral("edit"));
    CHECK(unsupported.editSource(QStringLiteral("wrong"), QStringLiteral("wrong"), {Syntax::MText, false, false}).code == State::ErrorCode::InvalidConvention);
    CHECK(unsupported.source() == QStringLiteral("edit"));
}

TEST_CASE("Unavailable transport profiles never masquerade as absent inherited metadata", "[text-source][state][headless][transport]") {
    State absent;
    REQUIRE(absent.currentProfile().available());
    REQUIRE(absent.currentProfile().metadata);
    CHECK(absent.currentProfile().metadata->status == Profile::DecodeStatus::Absent);
    for (const auto transport : {State::ProfileTransport::DuplicateNamespace, State::ProfileTransport::UnrepresentableNamespace}) {
        auto input = imported(Syntax::MText);
        input.profileTransport = transport;
        QString diagnostic = QStringLiteral("Original raw retained in DRW sidecar");
        auto *pointer = diagnostic.data();
        input.profileDiagnostic = QString::fromRawData(diagnostic.constData(), diagnostic.size());
        State state;
        REQUIRE_FALSE(state.initializeImport(input, QStringLiteral("legacy")));
        const auto snapshot = state.imported();
        pointer[0] = 'X';
        CHECK(snapshot->profileTransport == transport);
        CHECK(snapshot->profileDiagnostic == QStringLiteral("Original raw retained in DRW sidecar"));
        CHECK_FALSE(state.currentProfile().available());
        CHECK_FALSE(state.currentProfile().metadata);
        CHECK(state.currentProfile().transport == transport);
        CHECK(state.currentProfile().diagnostic == snapshot->profileDiagnostic);
        REQUIRE_FALSE(state.editSource(QStringLiteral("edited"), QStringLiteral("edited"), {Syntax::MText, false, false}));
        CHECK_FALSE(state.currentProfile().available());
        CHECK(state.replaceProfile(profile(Profile::Scope::StyleDefaults)));
        CHECK_FALSE(state.currentProfile().available());
        REQUIRE_FALSE(state.replaceProfile(profile()));
        REQUIRE(state.currentProfile().available());
        CHECK(state.currentProfile().metadata->known());
        CHECK(state.currentProfile().diagnostic.isEmpty());
        CHECK(state.imported() == snapshot);
        state.resetProfile();
        CHECK_FALSE(state.currentProfile().available());
        CHECK(state.currentProfile().diagnostic == snapshot->profileDiagnostic);
        REQUIRE_FALSE(state.removeProfile());
        REQUIRE(state.currentProfile().available());
        CHECK(state.currentProfile().metadata->status == Profile::DecodeStatus::Absent);
        CHECK(state.imported()->profileTransport == transport);
        State copy = state;
        copy.resetProfile();
        CHECK_FALSE(copy.currentProfile().available());
        CHECK(state.currentProfile().available());
        state.resetProfile();
        CHECK_FALSE(state.currentProfile().available());
        CHECK(state.imported() == snapshot);
        State invalid;
        input.profile = encoded(profile());
        CHECK(invalid.initializeImport(input, QStringLiteral("legacy")).code == State::ErrorCode::InvalidProvenance);
        CHECK_FALSE(invalid.initialized());
        input.profile.reset(); input.profileDiagnostic.clear();
        CHECK(invalid.initializeImport(input, QStringLiteral("legacy")).code == State::ErrorCode::InvalidProvenance);
        CHECK_FALSE(invalid.initialized());
    }
    for (const auto transport : {State::ProfileTransport::Representable, static_cast<State::ProfileTransport>(99)}) {
        State state;
        auto input = imported(Syntax::Plain);
        input.profileTransport = transport;
        input.profileDiagnostic = QStringLiteral("inconsistent diagnostic");
        CHECK(state.initializeImport(input, QStringLiteral("legacy")).code == State::ErrorCode::InvalidProvenance);
        CHECK_FALSE(state.initialized());
        CHECK(state.currentProfile().available());
    }
}

TEST_CASE("Compatibility-input edits cannot disappear behind identical final legacy text", "[text-source][state][headless][compatibility-input]") {
    for (const auto syntax : {Syntax::Plain, Syntax::MText}) {
        State state;
        const auto input = imported(syntax);
        const auto before = QStringLiteral("\\A1;legacy");
        const auto after = QStringLiteral("\\A2;legacy");
        REQUIRE_FALSE(state.initializeImport(input, before));
        const auto original = state.imported();
        REQUIRE_FALSE(state.editSource(input.source, after, input.convention));
        CHECK(state.textEdited());
        CHECK(state.origin() == State::SourceOrigin::Native);
        CHECK(state.compatibilityInput() == after);
        CHECK(state.compatibilityText() == QStringLiteral("legacy"));
        CHECK(state.source() == input.source);
        CHECK(state.imported() == original);
        CHECK(original->compatibilityInput == before);
        CHECK(state.profile().authority() == Authority::Untouched);
        REQUIRE_FALSE(state.acceptLegacyText(state.compatibilityText(), state.compatibilityText(), syntax, true));
        CHECK(state.compatibilityInput() == after);
        CHECK(state.textEdited());
        REQUIRE_FALSE(state.editSource(input.source, before, input.convention));
        CHECK(state.compatibilityInput() == before);
        CHECK_FALSE(state.textEdited());
    }
}

TEST_CASE("Current compatibility input owns exact buffers independently of immutable import history", "[text-source][state][headless][compatibility-input][alias]") {
    for (const auto syntax : {Syntax::Plain, Syntax::MText}) {
        State state;
        auto input = imported(syntax);
        const auto before = QStringLiteral("\\A1;\\A2;A");
        REQUIRE_FALSE(state.initializeImport(input, before));
        CHECK(state.compatibilityInput() == before);
        State copy = state;
        QString buffer = QStringLiteral("\\A0;\\A2;B");
        buffer += QChar(0); buffer += QLatin1Char('C');
        const QString expected(buffer.constData(), buffer.size());
        auto *borrowed = buffer.data();
        REQUIRE_FALSE(state.editSource(input.source, QString::fromRawData(buffer.constData(), buffer.size()), input.convention));
        borrowed[0] = QLatin1Char('X');
        CHECK(state.compatibilityInput() == expected);
        CHECK(state.compatibilityText() == expected.mid(4));
        CHECK(state.compatibilityInput().at(state.compatibilityInput().size() - 2).unicode() == 0);
        CHECK(copy.compatibilityInput() == before);
        CHECK(state.imported() == copy.imported());
        CHECK(state.imported()->compatibilityInput == before);
        State moved(std::move(state));
        CHECK(moved.compatibilityInput() == expected);
        CHECK(state.compatibilityInput() == expected);
        REQUIRE_FALSE(state.acceptLegacyText(state.compatibilityText(), state.compatibilityText(), syntax, true));
        CHECK(state.compatibilityInput() == expected);
        REQUIRE_FALSE(state.replaceProfile(profile()));
        REQUIRE_FALSE(state.removeProfile());
        state.resetProfile();
        CHECK(state.compatibilityInput() == expected);
        auto wrong = input.convention;
        wrong.syntax = syntax == Syntax::Plain ? Syntax::MText : Syntax::Plain;
        CHECK(state.editSource(input.source, before, wrong).code == State::ErrorCode::InvalidConvention);
        CHECK(state.compatibilityInput() == expected);
        REQUIRE_FALSE(state.editSource(input.source, before, input.convention));
        CHECK(state.compatibilityInput() == before);
        CHECK(moved.compatibilityInput() == expected);
        CHECK_FALSE(state.textEdited());
        CHECK(moved.textEdited());
        State nullInput;
        REQUIRE_FALSE(nullInput.initializeImport(input, QString{}));
        CHECK(nullInput.compatibilityInput().isNull());
        REQUIRE_FALSE(nullInput.editSource(input.source, QStringLiteral(""), input.convention));
        CHECK_FALSE(nullInput.compatibilityInput().isNull());
        CHECK(nullInput.textEdited());
        CHECK(nullInput.imported()->compatibilityInput.isNull());
    }
}

#ifndef LC_TEXT_STATE_ONLY
namespace {
RS_TextData textData(const QString &text) {
    return {RS_Vector(1, 2), RS_Vector(1, 2), 9, 1, RS_TextData::VABaseline, RS_TextData::HALeft,
            RS_TextData::None, text, QString{}, 0, RS2::NoUpdate};
}
RS_MTextData mtextData(const QString &text) {
    return {RS_Vector(1, 2), 9, 100, RS_MTextData::VATop, RS_MTextData::HALeft,
            RS_MTextData::LeftToRight, RS_MTextData::AtLeast, 1, text, QString{}, 0, RS2::NoUpdate};
}
template<typename Entity> void unchanged(Entity &entity) {
    const auto raw = entity.getCanonicalText();
    const auto compatibility = entity.getText();
    const auto compatibilityInput = entity.getTextState().compatibilityInput();
    const auto provenance = entity.getTextState().imported();
    const auto profileRaw = entity.getTextProfile().imported().raw;
    const auto alignment = entity.getVAlign();
    for (int i = 0; i != 3; ++i) {
        entity.setText(entity.getText());
        entity.update(); // Empty legacy style: no font resource or GUI dependency.
        CHECK(entity.getCanonicalText() == raw);
        CHECK(entity.getText() == compatibility);
        CHECK(entity.getTextState().compatibilityInput() == compatibilityInput);
        CHECK(entity.getVAlign() == alignment);
        CHECK(entity.getTextState().imported() == provenance);
        CHECK(entity.getTextProfile().imported().raw == profileRaw);
        CHECK_FALSE(entity.getTextStateError());
    }
    std::unique_ptr<RS_Entity> clone(entity.clone());
    const auto *typed = dynamic_cast<const Entity *>(clone.get());
    REQUIRE(typed);
    CHECK(typed->getCanonicalText() == raw);
    CHECK(typed->getTextState().compatibilityInput() == compatibilityInput);
    CHECK(typed->getTextState().imported() == provenance);
    Entity reconstructed(nullptr, entity.getData());
    CHECK(reconstructed.getCanonicalText() == raw);
    CHECK(reconstructed.getText() == compatibility);
    CHECK(reconstructed.getTextState().compatibilityInput() == compatibilityInput);
    CHECK(reconstructed.getVAlign() == alignment);
    CHECK(reconstructed.getTextState().imported() == provenance);
}
template<typename Entity> void importedEntity(Entity &entity, Syntax syntax, State::PayloadOrigin payload, Profile::RtlMarker marker) {
    auto input = imported(syntax, QStringLiteral("\\A1;{\\H2x;ب}\\U+062A"));
    input.compatibilityStyle.reset(); // Host captures the actual resolved legacy style.
    input.payload = payload;
    input.marker = marker;
    input.markerOrigin = payload == State::PayloadOrigin::EmbeddedMText ? State::MarkerOrigin::AttributeOuter : State::MarkerOrigin::Entity;
    input.profile = encoded(profile());
    const auto compatibility = QStringLiteral("\\A1;بت");
    REQUIRE_FALSE(entity.importText(input, compatibility));
    CHECK(entity.getCanonicalText() == input.source);
    CHECK(entity.getText() == compatibility.mid(4));
    CHECK_FALSE(entity.getTextState().textEdited());
    CHECK_FALSE(entity.getTextState().styleEdited(entity.getStyle()));
    CHECK(entity.getTextState().imported()->style == input.style);
    CHECK(entity.getTextState().imported()->payload == payload);
    CHECK(entity.getTextState().imported()->marker == marker);
    CHECK(entity.getTextProfile().current().known());
    CHECK(entity.getTextProfile().authority() == Authority::Untouched);
    unchanged(entity);
    const auto original = entity.getTextState().imported();
    entity.setStyle(QStringLiteral("new-style"));
    CHECK(entity.getTextState().styleEdited(entity.getStyle()));
    CHECK(original->style == input.style);
    auto data = entity.getData();
    data.text = QStringLiteral("copied data edit");
    Entity edited(nullptr, data);
    CHECK(edited.getCanonicalText() == data.text);
    CHECK(edited.getTextState().textEdited());
    CHECK(edited.getTextState().origin() == State::SourceOrigin::Native);
    CHECK(edited.getTextState().imported() == original);
    CHECK(entity.getCanonicalText() == input.source);
    REQUIRE_FALSE(entity.setCanonicalText(QStringLiteral("raw-only"), compatibility, input.convention));
    CHECK(entity.getText() == compatibility.mid(4));
    CHECK(entity.getCanonicalText() == QStringLiteral("raw-only"));
    CHECK(entity.getTextState().textEdited());
    CHECK(entity.getTextState().imported() == original);
    const auto before = entity.getData();
    auto wrong = input.convention;
    wrong.syntax = syntax == Syntax::Plain ? Syntax::MText : Syntax::Plain;
    CHECK(entity.setCanonicalText(QStringLiteral("wrong"), QStringLiteral("wrong"), wrong).code == State::ErrorCode::InvalidConvention);
    CHECK(entity.getText() == before.text);
    CHECK(entity.getCanonicalText() == before.textState.source());
    CHECK(entity.getTextState().imported() == original);
    CHECK(entity.replaceTextProfile(profile(Profile::Scope::StyleDefaults)).code == Profile::ErrorCode::InvalidScope);
    CHECK(entity.getTextProfile().authority() == Authority::Untouched);
    CHECK(entity.importText(input, compatibility).code == State::ErrorCode::AlreadyImported);
}
} // namespace

TEST_CASE("TEXT and MTEXT native constructors preserve alignment raw source through no-op and copied-data edits", "[text-source][entity]") {
    const auto raw = QStringLiteral("\\A1;\\A2;ب\u200d\u00a0\ufe8f");
    RS_Text text(nullptr, textData(raw));
    CHECK(text.getVAlign() == RS_TextData::VAMiddle);
    CHECK(text.getCanonicalText() == raw);
    CHECK(text.getText() == raw.mid(4));
    unchanged(text);
    RS_MText mtext(nullptr, mtextData(raw));
    CHECK(mtext.getVAlign() == RS_MTextData::VAMiddle);
    CHECK(mtext.getCanonicalText() == raw);
    CHECK(mtext.getText() == raw.mid(4));
    unchanged(mtext);
    auto data = text.getData(); data.text = QStringLiteral("new");
    RS_Text changed(nullptr, data);
    CHECK(changed.getCanonicalText() == QStringLiteral("new"));
    CHECK(changed.getTextState().textEdited());
    CHECK(text.getCanonicalText() == raw);
}

TEST_CASE("Entity import APIs retain standalone attribute and embedded source provenance without activating profiles", "[text-source][entity]") {
    for (const auto marker : {Profile::RtlMarker::LegacyReversed, Profile::RtlMarker::LogicalRtl}) {
        RS_Text text(nullptr, textData(QStringLiteral("before")));
        importedEntity(text, Syntax::Plain, State::PayloadOrigin::Standalone, marker);
        RS_Text attribute(nullptr, textData(QStringLiteral("before")));
        importedEntity(attribute, Syntax::Plain, State::PayloadOrigin::AttributeText, marker);
        RS_MText mtext(nullptr, mtextData(QStringLiteral("before")));
        importedEntity(mtext, Syntax::MText, State::PayloadOrigin::Standalone, marker);
        RS_MText embedded(nullptr, mtextData(QStringLiteral("before")));
        importedEntity(embedded, Syntax::MText, State::PayloadOrigin::EmbeddedMText, marker);
    }
}

TEST_CASE("Raw-aware entity edits consume the explicit pre-alignment input stage once", "[text-source][entity][alignment]") {
    const auto raw = QStringLiteral("\\A1;\\A2;ب");
    auto check = [&](auto &entity, Syntax syntax, auto middle) {
        const State::Convention convention{syntax, false, false};
        REQUIRE_FALSE(entity.setCanonicalText(raw, raw, convention));
        CHECK(entity.getCanonicalText() == raw);
        CHECK(entity.getText() == raw.mid(4));
        CHECK(entity.getVAlign() == middle);
        CHECK_FALSE(entity.getTextState().textEdited());
        REQUIRE_FALSE(entity.setCanonicalText(QStringLiteral("raw-only edit"), raw, convention));
        CHECK(entity.getCanonicalText() == QStringLiteral("raw-only edit"));
        CHECK(entity.getText() == raw.mid(4));
        CHECK(entity.getVAlign() == middle);
        CHECK(entity.getTextState().textEdited());
    };
    RS_Text text(nullptr, textData(raw));
    check(text, Syntax::Plain, RS_TextData::VAMiddle);
    RS_MText mtext(nullptr, mtextData(raw));
    check(mtext, Syntax::MText, RS_MTextData::VAMiddle);
}

TEST_CASE("Preconstructed data import consumes initial alignment exactly once and owns public text buffers", "[text-source][entity][alias]") {
    auto input = imported(Syntax::MText, QStringLiteral("raw\\U+0628"));
    QString compatibility = QStringLiteral("\\A0;ب");
    auto *pointer = compatibility.data();
    auto data = mtextData(QString::fromRawData(compatibility.constData(), compatibility.size()));
    REQUIRE_FALSE(data.textState.initializeImport(input, data.text));
    RS_MText entity(nullptr, data);
    pointer[4] = 'X';
    CHECK(entity.getText() == QStringLiteral("ب"));
    CHECK(entity.getVAlign() == RS_MTextData::VABottom);
    CHECK(entity.getCanonicalText() == input.source);
    CHECK_FALSE(entity.getTextState().textEdited());
    unchanged(entity);
    QString native = QStringLiteral("native");
    auto *nativePointer = native.data();
    RS_Text text(nullptr, textData(QString::fromRawData(native.constData(), native.size())));
    nativePointer[0] = 'X';
    CHECK(text.getText() == QStringLiteral("native"));
    CHECK(text.getCanonicalText() == QStringLiteral("native"));
    QString next = QStringLiteral("edited");
    auto *nextPointer = next.data();
    text.setText(QString::fromRawData(next.constData(), next.size()));
    nextPointer[0] = 'X';
    CHECK(text.getText() == QStringLiteral("edited"));
    CHECK(text.getCanonicalText() == QStringLiteral("edited"));
    auto changedData = data;
    changedData.text = QStringLiteral("modified before constructor");
    RS_MText changed(nullptr, changedData);
    CHECK(changed.getCanonicalText() == changedData.text);
    CHECK(changed.getTextState().textEdited());
    CHECK(changed.getTextState().imported() == data.textState.imported());
}

TEST_CASE("Entity accessors distinguish unavailable transport profiles and explicit edit authority", "[text-source][entity][transport]") {
    RS_Text text(nullptr, textData(QStringLiteral("before")));
    RS_MText mtext(nullptr, mtextData(QStringLiteral("before")));
    auto check = [](auto &entity, Syntax syntax) {
        auto input = imported(syntax);
        input.compatibilityStyle.reset();
        input.profileTransport = State::ProfileTransport::DuplicateNamespace;
        input.profileDiagnostic = QStringLiteral("Duplicate namespace retained in transport");
        REQUIRE_FALSE(entity.importText(input, QStringLiteral("legacy")));
        const auto original = entity.getTextState().imported();
        CHECK_FALSE(entity.getCurrentTextProfile().available());
        CHECK_FALSE(entity.getCurrentTextProfile().metadata);
        entity.setText(entity.getText());
        CHECK_FALSE(entity.getCurrentTextProfile().available());
        CHECK(entity.replaceTextProfile(profile(Profile::Scope::StyleDefaults)));
        CHECK_FALSE(entity.getCurrentTextProfile().available());
        REQUIRE_FALSE(entity.replaceTextProfile(profile()));
        REQUIRE(entity.getCurrentTextProfile().available());
        CHECK(entity.getCurrentTextProfile().metadata->known());
        entity.resetTextProfile();
        CHECK_FALSE(entity.getCurrentTextProfile().available());
        REQUIRE_FALSE(entity.removeTextProfile());
        REQUIRE(entity.getCurrentTextProfile().available());
        CHECK(entity.getCurrentTextProfile().metadata->status == Profile::DecodeStatus::Absent);
        entity.resetTextProfile();
        CHECK_FALSE(entity.getCurrentTextProfile().available());
        CHECK(entity.getTextState().imported() == original);
        CHECK(original->profileTransport == State::ProfileTransport::DuplicateNamespace);
        CHECK(original->profileDiagnostic == input.profileDiagnostic);
    };
    check(text, Syntax::Plain);
    check(mtext, Syntax::MText);
}
#endif
