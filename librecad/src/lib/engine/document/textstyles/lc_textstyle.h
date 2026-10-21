/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_TEXTSTYLE_H
#define LC_TEXTSTYLE_H
#include <QString>

#include "lc_textprofilebinding.h"
#include "lc_textentitystate.h"
#include "rs_flags.h"

class LC_TextStyle{
public:
    using ProfileTransport = LC_TextEntityState::ProfileTransport;
    using CurrentProfile = LC_TextEntityState::CurrentProfile;
    LC_TextStyle() = default;
    double getFixedTextHeight() const {return m_fixedTextHeight;}
    double getWidthFactor() const {return m_widthFactor;}
    double getObliqueAngle() const {return m_obliqueAngle;}
    int getGenFlag() const {return m_genFlag;}
    double getLastHeight() const {return m_lastHeight;}
    QString getFontName() const {return m_fontName;}
    QString getBigFont() const {return m_bigFontName;}
    int getFontFamilyItalicBold() const {return m_fontFamilyItalicBold;}
    void setFixedTextHeight(double fixedTextHeight);
    void setWidthFactor(double widthFactor);
    void setObliqueAngle(double obliqueAngle);
    void setGenFlag(int genFlag);
    void setLastHeight(double lastHeight);
    void setFontName(const QString& fontName);
    void setBigFont(const QString& bigFont);
    void setFontFamilyItalicBold(int fontFamilyItalicBold);
    void setName(const QString& name);
    QString getName() const {return m_name;}
    void setFlags(const unsigned flags) {m_flags.setFlags(flags);}
    unsigned getFlags() const {return m_flags.getFlags();}
    // Import provenance is separate from explicit profile edit authority. Raw
    // duplicate/unrepresentable namespaces remain in the transport sidecar and
    // must supply an unavailable diagnostic, never masquerade as absent data.
    LC_TextProfile::Error importTextProfile(const std::optional<LC_TextProfile::Block> &block,
                                           ProfileTransport transport = ProfileTransport::Representable,
                                           const QString &unavailableDiagnostic = {},
                                           const std::optional<QByteArray> &originalNameUtf8 = std::nullopt);
    CurrentProfile getCurrentTextProfile() const;
    // Low-level raw/edit binding, not an inheritance/activation decision.
    const LC_TextProfileBinding &getTextProfile() const {return m_textProfile;}
    LC_TextProfile::Error replaceTextProfile(const LC_TextProfile::Value &value) {return m_textProfile.replace(value);}
    LC_TextProfile::Error removeTextProfile() {return m_textProfile.remove();}
    void resetTextProfile() {m_textProfile.reset();}
    QString getTextProfileDiagnostic() const;
    const std::optional<QString> &importedName() const {return m_importedName;}
    const std::optional<QByteArray> &importedNameUtf8() const {return m_importedNameUtf8;}
private:
    QString m_name;
    RS_Flags m_flags {0};
    double m_fixedTextHeight {0};          /*!< Fixed text height (0 not set), code 40 */
    double m_widthFactor {1};              /*!< Width factor, code 41 */
    double m_obliqueAngle {0.0};           /*!< Oblique angle, code 50 */
    int m_genFlag {0};                     /*!< Text generation flags, code 71 */
    double m_lastHeight {1.0};             /*!< Last height used, code 42 */
    QString  m_fontName {QStringLiteral("txt")}; /*!< primary font file name, code 3 */
    QString  m_bigFontName;                /*!< bigfont file name or blank if none, code 4 */
    int m_fontFamilyItalicBold {0};         /*!< ttf font family, italic and bold flags, code 1071 */
    LC_TextProfileBinding m_textProfile{LC_TextProfile::Scope::StyleDefaults};
    std::optional<QString> m_importedName;
    std::optional<QByteArray> m_importedNameUtf8;
    QString m_importedProfileDiagnostic;
    ProfileTransport m_importedProfileTransport = ProfileTransport::Representable;
};

#endif
