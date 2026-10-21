/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textentitystate.h"

#include <stdexcept>
#include <utility>

namespace {
using State = LC_TextEntityState;
QString owned(const QString &value) {
    return value.isNull() ? QString{} : QString(value.constData(), value.size());
}
QByteArray owned(const QByteArray &value) {
    return value.isNull() ? QByteArray{} : QByteArray(value.constData(), value.size());
}
bool sameBuffer(const QString &a, const QString &b) {
    return a.isNull() == b.isNull() && a == b;
}
bool valid(const State::Convention &convention) {
    return convention.syntax == State::Syntax::Plain || convention.syntax == State::Syntax::MText;
}
State::Error error(State::ErrorCode code, const char *message) {
    return {code, QString::fromLatin1(message)};
}
QString legacyText(const QString &input) {
    if (input.startsWith(QLatin1StringView("\\A0;")) || input.startsWith(QLatin1StringView("\\A1;")) ||
        input.startsWith(QLatin1StringView("\\A2;"))) return input.mid(4);
    return input;
}
bool validProvenance(const State::Import &input) {
    switch (input.profileTransport) {
    case State::ProfileTransport::Representable:
        if (!input.profileDiagnostic.isEmpty()) return false;
        break;
    case State::ProfileTransport::DuplicateNamespace:
    case State::ProfileTransport::UnrepresentableNamespace:
        if (input.profile || input.profileDiagnostic.isEmpty()) return false;
        break;
    default: return false;
    }
    switch (input.origin) {
    case State::SourceOrigin::Native: case State::SourceOrigin::Dxf: case State::SourceOrigin::Dwg: break;
    default: return false;
    }
    switch (input.payload) {
    case State::PayloadOrigin::Standalone: case State::PayloadOrigin::AttributeText: case State::PayloadOrigin::EmbeddedMText: break;
    default: return false;
    }
    switch (input.marker) {
    case State::Profile::RtlMarker::None: return input.markerOrigin == State::MarkerOrigin::None;
    case State::Profile::RtlMarker::LegacyReversed: case State::Profile::RtlMarker::LogicalRtl:
        return input.markerOrigin == State::MarkerOrigin::Entity ||
            (input.markerOrigin == State::MarkerOrigin::AttributeOuter && input.payload == State::PayloadOrigin::EmbeddedMText);
    default: return false;
    }
}
} // namespace

struct LC_TextEntityState::Data {
    QString source;
    Convention convention;
    SourceOrigin origin = SourceOrigin::Native;
    QString compatibilityInput;
    QString compatibilityText;
    bool initialized = false;
    bool nativeEdited = false;
    bool pendingConstructor = false;
    std::shared_ptr<const Imported> imported;
    LC_TextProfileBinding profile{Profile::Scope::EntityOverride};
};

bool LC_TextEntityState::Convention::operator==(const Convention &other) const {
    return syntax == other.syntax && decodeCaretCodes == other.decodeCaretCodes && decodeDxfEscapes == other.decodeDxfEscapes;
}

LC_TextEntityState::LC_TextEntityState() : m_data(std::make_shared<const Data>()) {}
LC_TextEntityState::LC_TextEntityState(LC_TextEntityState &&other) noexcept : m_data(other.m_data) {}
LC_TextEntityState &LC_TextEntityState::operator=(LC_TextEntityState &&other) noexcept {
    m_data = other.m_data;
    return *this;
}
bool LC_TextEntityState::initialized() const { return m_data->initialized; }
const QString &LC_TextEntityState::source() const { return m_data->source; }
const LC_TextEntityState::Convention &LC_TextEntityState::convention() const { return m_data->convention; }
LC_TextEntityState::SourceOrigin LC_TextEntityState::origin() const { return m_data->origin; }
const QString &LC_TextEntityState::compatibilityInput() const { return m_data->compatibilityInput; }
const QString &LC_TextEntityState::compatibilityText() const { return m_data->compatibilityText; }
std::shared_ptr<const LC_TextEntityState::Imported> LC_TextEntityState::imported() const { return m_data->imported; }
const LC_TextProfileBinding &LC_TextEntityState::profile() const { return m_data->profile; }
LC_TextEntityState::CurrentProfile LC_TextEntityState::currentProfile() const {
    if (m_data->profile.authority() == LC_TextProfileBinding::Authority::Untouched && m_data->imported &&
        m_data->imported->profileTransport != ProfileTransport::Representable)
        return {m_data->imported->profileTransport, m_data->imported->profileDiagnostic, std::nullopt};
    return {ProfileTransport::Representable, {}, m_data->profile.current()};
}
bool LC_TextEntityState::textEdited() const {
    if (!m_data->imported) return m_data->nativeEdited;
    const auto &original = *m_data->imported;
    return m_data->source != original.source || !(m_data->convention == original.convention) ||
        !sameBuffer(m_data->compatibilityInput, original.compatibilityInput) ||
        m_data->compatibilityText != original.compatibilityText;
}
bool LC_TextEntityState::styleEdited(const QString &currentStyle) const {
    return !m_data->imported || currentStyle != m_data->imported->compatibilityStyle;
}

LC_TextEntityState::Error LC_TextEntityState::initializeImport(const Import &input, const QString &compatibilityInput) {
    if (m_data->imported) return error(ErrorCode::AlreadyImported, "Text source already has immutable imported provenance");
    if (m_data->nativeEdited || m_data->profile.authority() != LC_TextProfileBinding::Authority::Untouched)
        return error(ErrorCode::AlreadyEdited, "An edited text host cannot acquire a new imported baseline");
    if (!valid(input.convention) || (initialized() && input.convention.syntax != convention().syntax))
        return error(ErrorCode::InvalidConvention, "The source convention does not match the text host");
    if (!validProvenance(input) || !input.compatibilityStyle)
        return error(ErrorCode::InvalidProvenance, "The source/RTL provenance or imported compatibility-style baseline is missing or inconsistent");
    if (input.utf8 && QString::fromUtf8(input.utf8->constData(), input.utf8->size()) != input.source)
        return error(ErrorCode::Utf8Mismatch, "Explicit-length UTF-8 decoding does not match the canonical source");
    if (input.profile && !LC_TextProfileBinding::supportsImport(*input.profile))
        return error(ErrorCode::UnsupportedProfileType, "The transport must retain this unsupported raw profile type outside the binding");
    try {
        auto next = std::make_shared<Data>();
        auto original = std::make_shared<Imported>();
        original->source = owned(input.source);
        if (input.utf8) original->utf8 = owned(*input.utf8);
        original->convention = input.convention;
        original->origin = input.origin;
        original->payload = input.payload;
        original->style = owned(input.style);
        original->compatibilityStyle = owned(*input.compatibilityStyle);
        original->marker = input.marker;
        original->markerOrigin = input.markerOrigin;
        original->compatibilityInput = owned(compatibilityInput);
        original->compatibilityText = owned(legacyText(compatibilityInput));
        original->profileTransport = input.profileTransport;
        original->profileDiagnostic = owned(input.profileDiagnostic);
        next->source = original->source;
        next->convention = original->convention;
        next->origin = original->origin;
        next->compatibilityInput = original->compatibilityInput;
        next->compatibilityText = original->compatibilityText;
        next->initialized = true;
        next->pendingConstructor = true;
        next->imported = std::move(original);
        if (input.profile) next->profile = LC_TextProfileBinding(Profile::Scope::EntityOverride, *input.profile);
        m_data = std::move(next);
        return {};
    } catch (const std::invalid_argument &) {
        return error(ErrorCode::UnsupportedProfileType, "The transport must retain this unsupported raw profile type outside the binding");
    }
}

LC_TextEntityState::Error LC_TextEntityState::editSource(const QString &source, const QString &compatibilityInput,
                                                        const Convention &inputConvention) {
    if (!valid(inputConvention) || (initialized() && inputConvention.syntax != convention().syntax))
        return error(ErrorCode::InvalidConvention, "The edited source convention does not match the text host");
    const auto finalText = legacyText(compatibilityInput);
    if (initialized() && source == m_data->source && sameBuffer(compatibilityInput, m_data->compatibilityInput) &&
        finalText == m_data->compatibilityText && inputConvention == convention()) return {};
    auto next = std::make_shared<Data>(*m_data);
    next->source = owned(source);
    next->convention = inputConvention;
    next->origin = SourceOrigin::Native;
    next->compatibilityInput = owned(compatibilityInput);
    next->compatibilityText = owned(finalText);
    next->initialized = true;
    next->nativeEdited = true;
    next->pendingConstructor = false;
    m_data = std::move(next);
    return {};
}

LC_TextEntityState::Error LC_TextEntityState::acceptLegacyText(const QString &input, const QString &finalText,
                                                              Syntax syntax, bool constructor) {
    if ((syntax != Syntax::Plain && syntax != Syntax::MText) || (initialized() && syntax != convention().syntax))
        return error(ErrorCode::InvalidConvention, "The data source convention does not match the text entity");
    const bool unchanged = initialized() && input == compatibilityText() && finalText == compatibilityText();
    const bool importedInitial = constructor && m_data->pendingConstructor && m_data->imported &&
        input == m_data->imported->compatibilityInput && finalText == m_data->imported->compatibilityText;
    if (unchanged || importedInitial) {
        if (constructor && m_data->pendingConstructor) {
            auto next = std::make_shared<Data>(*m_data);
            next->pendingConstructor = false;
            m_data = std::move(next);
        }
        return {};
    }
    if (finalText != legacyText(input)) return error(ErrorCode::InvalidConvention, "The host supplied an inconsistent legacy alignment-strip baseline");
    if (!initialized()) {
        auto next = std::make_shared<Data>(*m_data);
        next->source = owned(input);
        next->convention.syntax = syntax;
        next->compatibilityInput = owned(input);
        next->compatibilityText = owned(finalText);
        next->initialized = true;
        m_data = std::move(next);
        return {};
    }
    return editSource(input, input, Convention{syntax, false, false});
}

LC_TextProfile::Error LC_TextEntityState::replaceProfile(const Profile::Value &value) {
    auto next = std::make_shared<Data>(*m_data);
    const auto failure = next->profile.replace(value);
    if (failure) return failure;
    m_data = std::move(next);
    return {};
}
LC_TextProfile::Error LC_TextEntityState::removeProfile() {
    auto next = std::make_shared<Data>(*m_data);
    const auto failure = next->profile.remove();
    if (failure) return failure;
    m_data = std::move(next);
    return {};
}
void LC_TextEntityState::resetProfile() {
    if (m_data->profile.authority() == LC_TextProfileBinding::Authority::Untouched) return;
    auto next = std::make_shared<Data>(*m_data);
    next->profile.reset();
    m_data = std::move(next);
}
