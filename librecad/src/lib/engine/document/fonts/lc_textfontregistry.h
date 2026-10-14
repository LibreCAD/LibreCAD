/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTFONTREGISTRY_H
#define LC_TEXTFONTREGISTRY_H

#include "lc_opentypefont.h"
#include <QStringList>
#include <memory>
#include <mutex>

/** Explicit versioned resources, never a system-font or LFF-name fallback. */
class LC_TextFontRegistry final {
public:
    struct Identity {
        QString profileId;
        QByteArray fontSha256;
        unsigned int faceIndex = 0;
        QString metricPolicy;
        QString shapingPolicy;
        QString engineIdentity;
        bool operator==(const Identity &other) const;
    };
    struct Entry {
        Identity identity;
        QString displayName;
        QString relativeFontPath;
        LC_OpenTypeFont::Profile font;
    };
    struct Options {
        // Absolute font directories, in deterministic priority order. Supplying
        // these replaces installed lookup, rather than appending cwd/user dirs.
        QStringList fontRoots;
        QString applicationDirectory;
    };
    enum class ErrorCode {
        None, UnknownProfile, IdentityMismatch, IncompatibleEngine, BackendUnavailable,
        InvalidSearchRoot, MissingResource, FontFailure
    };
    struct Result {
        std::shared_ptr<const LC_OpenTypeFont> font;
        Identity identity;
        QString resourcePath; // Runtime-only; never serialize this path.
        ErrorCode error = ErrorCode::None;
        QString diagnostic;
        LC_OpenTypeFont::Error fontError;
        explicit operator bool() const { return font && error == ErrorCode::None; }
    };

    explicit LC_TextFontRegistry(Options options);
    static Entry amiriEntry();
    static QStringList installedFontRoots(const QString &applicationDirectory);
    Result resolve(const Identity &identity) const;
    void clear() const;

private:
    Options m_options;
    mutable std::mutex m_mutex;
    mutable std::shared_ptr<const LC_OpenTypeFont> m_amiri;
    mutable QString m_amiriPath;
};

#endif
