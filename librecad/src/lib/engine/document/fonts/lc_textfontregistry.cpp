/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textfontregistry.h"

#include <QDir>
#include <QFileInfo>
#include <utility>

bool LC_TextFontRegistry::Identity::operator==(const Identity &other) const {
    return profileId == other.profileId && fontSha256 == other.fontSha256 &&
           faceIndex == other.faceIndex && metricPolicy == other.metricPolicy &&
           shapingPolicy == other.shapingPolicy && engineIdentity == other.engineIdentity;
}

LC_TextFontRegistry::LC_TextFontRegistry(Options options) : m_options(std::move(options)) {}

LC_TextFontRegistry::Entry LC_TextFontRegistry::amiriEntry() {
    Entry entry;
    entry.font = LC_OpenTypeFont::amiriProfile();
    entry.identity.profileId = entry.font.id;
    entry.identity.fontSha256 = entry.font.sha256;
    entry.identity.faceIndex = entry.font.faceIndex;
    entry.identity.metricPolicy = QStringLiteral("amiri-cap-y647-cell9-baseline-v1");
    entry.identity.shapingPolicy = QStringLiteral("hb-ot-default-monotone-characters-v1");
    // Compatibility is an explicit allowlist, not "whatever HarfBuzz is linked".
    // Adding another engine requires the profile's placement/geometry evidence;
    // a geometry-changing engine must get a new rendering profile.
    entry.identity.engineIdentity = QStringLiteral("HarfBuzz-OT/14.6.0");
    entry.displayName = QStringLiteral("Amiri 1.003 (OpenType)");
    entry.relativeFontPath = QStringLiteral("opentype/amiri-1.003/Amiri-Regular.ttf");
    return entry;
}

QStringList LC_TextFontRegistry::installedFontRoots(const QString &applicationDirectory) {
    // No cwd, source checkout, environment variable, font database or preference
    // access. Development/test roots have to be provided explicitly by the caller.
    if (applicationDirectory.isEmpty() || !QDir::isAbsolutePath(applicationDirectory)) return {};
    const QDir application(applicationDirectory);
    QStringList roots;
    for (const auto *relative : {"../Resources/fonts", "resources/fonts", "fonts",
                                "../share/librecad/fonts", "../share/doc/librecad/fonts",
                                "../lib64/librecad/fonts", "../lib/librecad/fonts"}) {
        const auto path = QDir::cleanPath(application.absoluteFilePath(QString::fromLatin1(relative)));
        if (!roots.contains(path)) roots.push_back(path);
    }
    return roots;
}

LC_TextFontRegistry::Result LC_TextFontRegistry::resolve(const Identity &identity) const {
    Result result;
    result.identity = identity;
    const auto entry = amiriEntry();
    if (identity.profileId != entry.identity.profileId) {
        result.error = ErrorCode::UnknownProfile;
        result.diagnostic = QStringLiteral("The exact OpenType rendering profile is unknown");
        return result;
    }
    auto withoutEngine = identity;
    withoutEngine.engineIdentity = entry.identity.engineIdentity;
    if (!(withoutEngine == entry.identity)) {
        result.error = ErrorCode::IdentityMismatch;
        result.diagnostic = QStringLiteral("Saved font or shaping/metric policy does not match the versioned profile");
        return result;
    }
    if (identity.engineIdentity != entry.identity.engineIdentity) {
        result.error = ErrorCode::IncompatibleEngine;
        result.diagnostic = QStringLiteral("The saved rendering profile requires an unavailable compatible shaping engine");
        return result;
    }
    if (!LC_OpenTypeFont::backendAvailable()) {
        result.error = ErrorCode::BackendUnavailable;
        result.diagnostic = QStringLiteral("OpenType rendering is disabled; the saved profile has not been substituted");
        return result;
    }
    if (LC_OpenTypeFont::shapingEngineIdentity() != entry.identity.engineIdentity) {
        result.error = ErrorCode::IncompatibleEngine;
        result.diagnostic = QStringLiteral("The runtime shaping engine is not certified for this saved profile");
        return result;
    }
    const auto roots = m_options.fontRoots.isEmpty()
        ? installedFontRoots(m_options.applicationDirectory) : m_options.fontRoots;
    if (roots.isEmpty()) {
        result.error = ErrorCode::InvalidSearchRoot;
        result.diagnostic = QStringLiteral("OpenType lookup requires an absolute application directory or explicit font roots");
        return result;
    }
    for (const auto &root : roots) {
        if (root.isEmpty() || !QDir::isAbsolutePath(root)) {
            result.error = ErrorCode::InvalidSearchRoot;
            result.diagnostic = QStringLiteral("OpenType resource roots must be explicit absolute directories");
            return result;
        }
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_amiri) {
        result.font = m_amiri;
        result.resourcePath = m_amiriPath;
        return result;
    }
    for (const auto &root : roots) {
        const auto path = QDir(root).absoluteFilePath(entry.relativeFontPath);
        const QFileInfo resource(path);
        // exists() follows links: a dangling link is a broken installation,
        // not permission to conceal it with a later resource root.
        if (!resource.exists() && !resource.isSymLink()) continue;
        const auto loaded = LC_OpenTypeFont::load(path, entry.font);
        if (!loaded) {
            // A corrupt higher-priority installation must not be silently
            // masked by a developer font or a different backend in a later root.
            result.error = ErrorCode::FontFailure;
            result.fontError = loaded.error;
            result.diagnostic = loaded.error.message;
            result.resourcePath = path;
            return result;
        }
        m_amiri = loaded.font;
        m_amiriPath = path;
        result.font = m_amiri;
        result.resourcePath = path;
        return result;
    }
    result.error = ErrorCode::MissingResource;
    result.diagnostic = QStringLiteral("The bundled font for the saved rendering profile is missing");
    return result;
}

void LC_TextFontRegistry::clear() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_amiri.reset();
    m_amiriPath.clear();
}
