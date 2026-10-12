/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_textstyle.h"

#include <stdexcept>

namespace {
QString owned(const QString &text) {
    return text.isNull() ? QString{} : QString(text.constData(), text.size());
}
}

void LC_TextStyle::setFixedTextHeight(const double fixedTextHeight) {
    m_fixedTextHeight = fixedTextHeight;
}

void LC_TextStyle::setWidthFactor(const double widthFactor) {
    m_widthFactor = widthFactor;
}

void LC_TextStyle::setObliqueAngle(const double obliqueAngle) {
    m_obliqueAngle = obliqueAngle;
}

void LC_TextStyle::setGenFlag(const int genFlag) {
    m_genFlag = genFlag;
}

void LC_TextStyle::setLastHeight(const double lastHeight) {
    m_lastHeight = lastHeight;
}

void LC_TextStyle::setFontName(const QString& fontName) {
    m_fontName = owned(fontName);
}

void LC_TextStyle::setBigFont(const QString& bigFont) {
    m_bigFontName = owned(bigFont);
}

void LC_TextStyle::setFontFamilyItalicBold(const int fontFamilyItalicBold) {
    m_fontFamilyItalicBold = fontFamilyItalicBold;
}

void LC_TextStyle::setName(const QString& name) {
    m_name = owned(name);
}

LC_TextProfile::Error LC_TextStyle::importTextProfile(const std::optional<LC_TextProfile::Block> &block,
                                                     ProfileTransport transport,
                                                     const QString &unavailableDiagnostic,
                                                     const std::optional<QByteArray> &originalNameUtf8) {
    using Profile = LC_TextProfile;
    if (m_importedName || m_textProfile.authority() != LC_TextProfileBinding::Authority::Untouched)
        return {Profile::ErrorCode::InvalidField, QStringLiteral("A text STYLE can acquire imported profile provenance only once before editing"), QStringLiteral("profile")};
    const bool unavailable = transport == ProfileTransport::DuplicateNamespace ||
        transport == ProfileTransport::UnrepresentableNamespace;
    if ((!unavailable && transport != ProfileTransport::Representable) ||
        (unavailable && (block || unavailableDiagnostic.isEmpty())) ||
        (!unavailable && !unavailableDiagnostic.isEmpty()) ||
        (block && !LC_TextProfileBinding::supportsImport(*block)) ||
        (originalNameUtf8 && QString::fromUtf8(originalNameUtf8->constData(), originalNameUtf8->size()) != m_name))
        return {Profile::ErrorCode::InvalidType, QStringLiteral("Unrepresentable profile raw must remain in the transport sidecar"), QStringLiteral("profile")};
    // Prepare every allocating payload before committing import authority.
    auto name = std::optional<QString>(owned(m_name));
    const auto &bytes = originalNameUtf8;
    auto nameUtf8 = bytes ? std::optional<QByteArray>(bytes->isNull() ? QByteArray{} :
        QByteArray(bytes->constData(), bytes->size())) : std::nullopt;
    auto diagnostic = owned(unavailableDiagnostic);
    try {
        auto profile = block ? LC_TextProfileBinding(Profile::Scope::StyleDefaults, *block)
                             : LC_TextProfileBinding(Profile::Scope::StyleDefaults);
        m_textProfile = std::move(profile);
        m_importedName.swap(name);
        m_importedNameUtf8.swap(nameUtf8);
        m_importedProfileDiagnostic.swap(diagnostic);
        m_importedProfileTransport = transport;
    } catch (const std::invalid_argument &) {
        return {Profile::ErrorCode::InvalidType, QStringLiteral("Unrepresentable profile raw must remain in the transport sidecar"), QStringLiteral("profile")};
    }
    return {};
}

QString LC_TextStyle::getTextProfileDiagnostic() const {
    if (m_textProfile.authority() != LC_TextProfileBinding::Authority::Untouched) return {};
    if (!m_importedProfileDiagnostic.isEmpty()) return m_importedProfileDiagnostic;
    return m_textProfile.imported().error.message;
}

LC_TextStyle::CurrentProfile LC_TextStyle::getCurrentTextProfile() const {
    CurrentProfile result;
    if (m_textProfile.authority() == LC_TextProfileBinding::Authority::Untouched &&
        m_importedProfileTransport != ProfileTransport::Representable) {
        result.transport = m_importedProfileTransport;
        result.diagnostic = m_importedProfileDiagnostic;
    } else {
        result.metadata = m_textProfile.current();
        result.diagnostic = result.metadata->error.message;
    }
    return result;
}
