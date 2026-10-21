/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textprofilebinding.h"

#include <QMetaType>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace {
using Profile = LC_TextProfile;
bool supportedType(int type) noexcept {
    switch (type) {
    case QMetaType::UnknownType:
    case QMetaType::QString:
    case QMetaType::QByteArray:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Double:
    case QMetaType::Bool:
        return true;
    default:
        return false;
    }
}
QString owned(const QString &text) {
    return text.isNull() ? QString{} : QString(text.constData(), text.size());
}
QByteArray owned(const QByteArray &bytes) {
    return bytes.isNull() ? QByteArray{} : QByteArray(bytes.constData(), bytes.size());
}
Profile::Value owned(const Profile::Value &value) {
    auto result = value;
    if (result.language) result.language = owned(*value.language);
    if (result.selection) {
        auto &id = result.selection->identity;
        const auto &source = value.selection->identity;
        id.profileId = owned(source.profileId);
        id.fontSha256 = owned(source.fontSha256);
        id.metricPolicy = owned(source.metricPolicy);
        id.shapingPolicy = owned(source.shapingPolicy);
        id.engineIdentity = owned(source.engineIdentity);
    }
    return result;
}
QVariant owned(const QVariant &value) {
    // Typed-null variants contain only default payloads. Preserve QVariant's
    // null flag; constructing a variant from a default payload would clear it.
    if (!value.isNull()) {
        if (value.metaType().id() == QMetaType::QString) return owned(value.toString());
        if (value.metaType().id() == QMetaType::QByteArray) return owned(value.toByteArray());
    }
    // Fixed scalars (and typed null/invalid variants) have no nested buffers.
    // Detach QVariant itself, isolating even a previously retained data pointer
    // without conversions that could lose exact types or floating payload bits.
    auto result = value;
    result.detach();
    return result;
}
Profile::Block owned(const Profile::Block &block) {
    if (!LC_TextProfileBinding::supportsImport(block))
        throw std::invalid_argument("A text-profile binding cannot own this raw QVariant metatype");
    Profile::Block result;
    result.reserve(block.size());
    for (const auto &tag : block) result.push_back({tag.code, owned(tag.value)});
    return result;
}
bool validScope(Profile::Scope scope) {
    return scope == Profile::Scope::StyleDefaults || scope == Profile::Scope::EntityOverride;
}
Profile::Error scopeError() {
    return {Profile::ErrorCode::InvalidScope, QStringLiteral("The profile binding requires its locked style/entity scope"), QStringLiteral("scope")};
}
Profile::Decoded checkedImport(Profile::Scope scope, Profile::Decoded imported) {
    if (!validScope(scope) || (imported.known() && imported.value->scope != scope)) {
        imported.status = Profile::DecodeStatus::Invalid;
        imported.value.reset();
        imported.error = scopeError();
    }
    return imported;
}
} // namespace

LC_TextProfileBinding::LC_TextProfileBinding(Scope scope)
    : m_scope(scope), m_imported(checkedImport(scope, {})) {}

LC_TextProfileBinding::LC_TextProfileBinding(Scope scope, const Profile::Block &importedBlock)
    : m_scope(scope), m_imported(checkedImport(scope, Profile::decode(owned(importedBlock)))) {}

LC_TextProfileBinding::LC_TextProfileBinding(Scope scope, const std::shared_ptr<const Profile::Block> &importedBlock)
    : m_scope(scope), m_imported(checkedImport(scope, Profile::decode(importedBlock
          ? std::make_shared<const Profile::Block>(owned(*importedBlock)) : nullptr))) {}

bool LC_TextProfileBinding::supportsImport(const Profile::Block &block) noexcept {
    for (const auto &tag : block) if (!supportedType(tag.value.metaType().id())) return false;
    return true;
}

LC_TextProfileBinding::LC_TextProfileBinding(LC_TextProfileBinding &&other)
    : m_scope(other.m_scope), m_imported(other.m_imported), m_authority(other.m_authority),
      m_replacement(std::move(other.m_replacement)), m_encoding(std::move(other.m_encoding)) {
    // The import copy is prepared before either pending payload moves. Those
    // moves cannot throw, so a failed import copy leaves the source unchanged.
    static_assert(std::is_nothrow_move_constructible_v<decltype(m_replacement)>);
    static_assert(std::is_nothrow_move_constructible_v<decltype(m_encoding)>);
    other.reset();
}

LC_TextProfileBinding &LC_TextProfileBinding::operator=(LC_TextProfileBinding &&other) {
    if (this == &other) return *this;
    LC_TextProfileBinding incoming(std::move(other));
    swapState(incoming);
    return *this;
}

LC_TextProfileBinding &LC_TextProfileBinding::operator=(const LC_TextProfileBinding &other) {
    if (this == &other) return *this;
    // In particular, copying pending encoding may allocate. Do not change
    // authority, current value or provenance until the entire copy succeeds.
    LC_TextProfileBinding incoming(other);
    swapState(incoming);
    return *this;
}

void LC_TextProfileBinding::swapState(LC_TextProfileBinding &other) noexcept {
    // Prepare before touching the destination, then commit without throwing.
    static_assert(std::is_nothrow_swappable_v<decltype(m_scope)>);
    static_assert(std::is_nothrow_swappable_v<decltype(m_imported)>);
    static_assert(std::is_nothrow_swappable_v<decltype(m_authority)>);
    static_assert(std::is_nothrow_swappable_v<decltype(m_replacement)>);
    static_assert(std::is_nothrow_swappable_v<decltype(m_encoding)>);
    using std::swap;
    swap(m_scope, other.m_scope);
    swap(m_imported, other.m_imported);
    swap(m_authority, other.m_authority);
    swap(m_replacement, other.m_replacement);
    swap(m_encoding, other.m_encoding);
}

LC_TextProfile::Decoded LC_TextProfileBinding::current() const {
    Profile::Decoded result;
    if (m_authority == Authority::Replace) {
        result.status = Profile::DecodeStatus::Known;
        result.value = m_replacement;
    } else if (m_authority == Authority::Untouched) {
        result.status = m_imported.status;
        result.value = m_imported.value;
        result.error = m_imported.error;
    }
    return result;
}

const LC_TextProfile::Encoded *LC_TextProfileBinding::pendingEncoding() const {
    return m_authority == Authority::Replace && m_encoding ? &*m_encoding : nullptr;
}

LC_TextProfile::Error LC_TextProfileBinding::replace(const Profile::Value &value) {
    if (!validScope(m_scope) || value.scope != m_scope) return scopeError();
    auto snapshot = owned(value);
    auto encoded = Profile::encode(snapshot);
    if (!encoded) return encoded.error;
    // Prepare both payloads before committing. Qt/value allocation failures may
    // throw; no current binding state is changed while preparing these copies.
    auto replacement = std::optional<Profile::Value>(std::move(snapshot));
    auto encoding = std::optional<Profile::Encoded>(std::move(encoded));
    static_assert(std::is_nothrow_swappable_v<decltype(m_replacement)>);
    static_assert(std::is_nothrow_swappable_v<decltype(m_encoding)>);
    m_replacement.swap(replacement);
    m_encoding.swap(encoding);
    m_authority = Authority::Replace;
    return {};
}

LC_TextProfile::Error LC_TextProfileBinding::remove() {
    if (!validScope(m_scope)) return scopeError();
    m_replacement.reset();
    m_encoding.reset();
    m_authority = Authority::Remove;
    return {};
}

void LC_TextProfileBinding::reset() {
    m_replacement.reset();
    m_encoding.reset();
    m_authority = Authority::Untouched;
}
