/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textfontregistry.h"
#include <catch2/catch_test_macros.hpp>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <future>
#include <utility>
#include <vector>

namespace {
#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
QString fontPath() {
#ifdef LC_OPENTYPE_TEST_FONT_PATH
    return QString::fromUtf8(LC_OPENTYPE_TEST_FONT_PATH);
#else
    return QFileInfo(QString::fromUtf8(__FILE__)).absoluteDir().absoluteFilePath(
        "../../../../../../support/fonts/opentype/amiri-1.003/Amiri-Regular.ttf");
#endif
}
void install(const QString &root) {
    const auto path = QDir(root).absoluteFilePath(LC_TextFontRegistry::amiriEntry().relativeFontPath);
    REQUIRE(QDir().mkpath(QFileInfo(path).absolutePath()));
    REQUIRE(QFile::copy(fontPath(), path));
}
void requireCertifiedEngine() {
    if (LC_OpenTypeFont::shapingEngineIdentity() != LC_TextFontRegistry::amiriEntry().identity.engineIdentity)
        SKIP("Installed resource acceptance requires the certified HarfBuzz engine; this lane is not passing coverage");
}
#endif
}

TEST_CASE("Versioned profiles reject ambiguity and engine reinterpretation", "[opentype][registry][headless]") {
    using Registry = LC_TextFontRegistry;
    const auto entry = Registry::amiriEntry();
    Registry registry({});
    for (const auto &id : {QStringLiteral("amiri-regular"), QStringLiteral("Amiri"), QStringLiteral("lc-amiri-1.003-ot-v2")}) {
        auto identity = entry.identity;
        identity.profileId = id;
        REQUIRE(registry.resolve(identity).error == Registry::ErrorCode::UnknownProfile);
    }
    auto identity = entry.identity;
    identity.fontSha256[0] = '0';
    REQUIRE(registry.resolve(identity).error == Registry::ErrorCode::IdentityMismatch);
    identity = entry.identity;
    identity.faceIndex = 1;
    REQUIRE(registry.resolve(identity).error == Registry::ErrorCode::IdentityMismatch);
    identity = entry.identity;
    identity.metricPolicy += "changed";
    REQUIRE(registry.resolve(identity).error == Registry::ErrorCode::IdentityMismatch);
    identity = entry.identity;
    identity.shapingPolicy += "changed";
    REQUIRE(registry.resolve(identity).error == Registry::ErrorCode::IdentityMismatch);
    identity = entry.identity;
    identity.engineIdentity = "HarfBuzz-OT/unknown";
    REQUIRE(registry.resolve(identity).error == Registry::ErrorCode::IncompatibleEngine);
    REQUIRE(Registry::installedFontRoots(QStringLiteral("relative/path")).isEmpty());
    REQUIRE(Registry::installedFontRoots(QString()).isEmpty());
}

TEST_CASE("Registry distinguishes unavailable backends from incompatible engines", "[opentype][registry][headless]") {
    using Registry = LC_TextFontRegistry;
    const auto entry = Registry::amiriEntry();
    Registry registry({});
    const auto result = registry.resolve(entry.identity);
    REQUIRE_FALSE(result);
    REQUIRE_FALSE(result.diagnostic.isEmpty());
    REQUIRE(result.identity == entry.identity);
#if !defined(LC_WITH_OPENTYPE) || !LC_WITH_OPENTYPE
    REQUIRE(result.error == Registry::ErrorCode::BackendUnavailable);
#else
    if (LC_OpenTypeFont::shapingEngineIdentity() != entry.identity.engineIdentity)
        REQUIRE(result.error == Registry::ErrorCode::IncompatibleEngine);
    else
        REQUIRE(result.error == Registry::ErrorCode::InvalidSearchRoot);
#endif
}

TEST_CASE("Installed root policy is deterministic without initializing a GUI", "[opentype][registry][headless]") {
    using Registry = LC_TextFontRegistry;
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto app = temporary.filePath("install/bin");
    const auto roots = Registry::installedFontRoots(app);
    REQUIRE(roots == Registry::installedFontRoots(app));
    REQUIRE(roots.size() == 7);
    REQUIRE(roots[0] == temporary.filePath("install/Resources/fonts"));
    REQUIRE(roots[1] == temporary.filePath("install/bin/resources/fonts"));
    REQUIRE(roots[2] == temporary.filePath("install/bin/fonts"));
    REQUIRE(roots[3] == temporary.filePath("install/share/librecad/fonts"));
    REQUIRE(roots[4] == temporary.filePath("install/share/doc/librecad/fonts"));
    REQUIRE(roots[5] == temporary.filePath("install/lib64/librecad/fonts"));
    REQUIRE(roots[6] == temporary.filePath("install/lib/librecad/fonts"));
    for (const auto &root : roots) REQUIRE(QDir::isAbsolutePath(root));
}

#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
TEST_CASE("All supported installed font layouts resolve the pinned resource", "[opentype][registry][installed][headless]") {
    using Registry = LC_TextFontRegistry;
    const auto entry = Registry::amiriEntry();
    requireCertifiedEngine();
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const std::vector<std::pair<QString, QString>> layouts{
        {"mac/LibreCAD.app/Contents/MacOS", "mac/LibreCAD.app/Contents/Resources/fonts"},
        {"windows", "windows/resources/fonts"},
        {"unix/usr/bin", "unix/usr/share/librecad/fonts"},
        {"unix-doc/usr/bin", "unix-doc/usr/share/doc/librecad/fonts"},
        {"unix-lib64/usr/bin", "unix-lib64/usr/lib64/librecad/fonts"},
        {"unix-lib/usr/bin", "unix-lib/usr/lib/librecad/fonts"}
    };
    for (const auto &layout : layouts) {
        INFO(layout.first.toStdString());
        const auto root = temporary.filePath(layout.second);
        install(root);
        Registry installed({{}, temporary.filePath(layout.first)});
        const auto loaded = installed.resolve(entry.identity);
        REQUIRE(loaded);
        REQUIRE(loaded.font->checksum() == entry.identity.fontSha256);
        REQUIRE(loaded.resourcePath == QDir(root).absoluteFilePath(entry.relativeFontPath));
    }
}

TEST_CASE("Installed font lookup has explicit roots and immutable resource caching", "[opentype][registry][headless]") {
    using Registry = LC_TextFontRegistry;
    const auto entry = Registry::amiriEntry();
    requireCertifiedEngine();
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto app = temporary.filePath("LibreCAD.app/Contents/MacOS");
    const auto root = temporary.filePath("LibreCAD.app/Contents/Resources/fonts");
    install(root);
    Registry installed({{}, app});
    const auto loaded = installed.resolve(entry.identity);
    REQUIRE(loaded);
    REQUIRE(loaded.font->checksum() == entry.identity.fontSha256);
    REQUIRE(loaded.resourcePath == QDir(root).absoluteFilePath(entry.relativeFontPath));
    REQUIRE(installed.resolve(entry.identity).font == loaded.font);
    // A hot cache must not bypass any saved-identity or engine check.
    for (int field = 0; field < 6; ++field) {
        auto changed = entry.identity;
        if (field == 0) changed.profileId += "unknown";
        else if (field == 1) changed.fontSha256[0] = '0';
        else if (field == 2) changed.faceIndex = 1;
        else if (field == 3) changed.metricPolicy += "unknown";
        else if (field == 4) changed.shapingPolicy += "unknown";
        else changed.engineIdentity += "unknown";
        const auto rejected = installed.resolve(changed);
        REQUIRE_FALSE(rejected);
        REQUIRE(rejected.error == (field == 0 ? Registry::ErrorCode::UnknownProfile :
            field == 5 ? Registry::ErrorCode::IncompatibleEngine : Registry::ErrorCode::IdentityMismatch));
    }
    QFile changedFont(loaded.resourcePath);
    REQUIRE(changedFont.open(QIODevice::WriteOnly | QIODevice::Truncate));
    REQUIRE(changedFont.write("changed after load") > 0);
    changedFont.close();
    REQUIRE(installed.resolve(entry.identity).font == loaded.font);
    REQUIRE(loaded.font->outline(3999));
    installed.clear();
    REQUIRE(installed.resolve(entry.identity).error == Registry::ErrorCode::FontFailure);
    REQUIRE(QFile::remove(loaded.resourcePath));
    REQUIRE(installed.resolve(entry.identity).error == Registry::ErrorCode::MissingResource);
    REQUIRE(loaded.font->outline(3999));
    REQUIRE(QFile::copy(fontPath(), loaded.resourcePath));
    installed.clear();
    const auto reloaded = installed.resolve(entry.identity);
    REQUIRE(reloaded);
    REQUIRE(reloaded.font != loaded.font);
    REQUIRE(loaded.font->checksum() == entry.identity.fontSha256);
    Registry missing({{temporary.filePath("missing")}, {}});
    REQUIRE(missing.resolve(entry.identity).error == Registry::ErrorCode::MissingResource);
    // Explicit roots replace installed lookup even when a valid bundled font
    // exists beside the application; there is no implicit developer fallback.
    Registry overrideMissing({{temporary.filePath("missing")}, app});
    REQUIRE(overrideMissing.resolve(entry.identity).error == Registry::ErrorCode::MissingResource);
    Registry relative({{QStringLiteral("fonts")}, {}});
    REQUIRE(relative.resolve(entry.identity).error == Registry::ErrorCode::InvalidSearchRoot);
    Registry emptyRoot({{QString()}, app});
    REQUIRE(emptyRoot.resolve(entry.identity).error == Registry::ErrorCode::InvalidSearchRoot);
    Registry invalidApplication({{}, QStringLiteral("relative/app")});
    REQUIRE(invalidApplication.resolve(entry.identity).error == Registry::ErrorCode::InvalidSearchRoot);
    const auto corruptRoot = temporary.filePath("corrupt");
    const auto corruptPath = QDir(corruptRoot).absoluteFilePath(entry.relativeFontPath);
    REQUIRE(QDir().mkpath(QFileInfo(corruptPath).absolutePath()));
    QFile corrupt(corruptPath);
    REQUIRE(corrupt.open(QIODevice::WriteOnly));
    REQUIRE(corrupt.write("wrong bytes") > 0);
    corrupt.close();
    Registry priority({{corruptRoot, root}, {}});
    const auto failed = priority.resolve(entry.identity);
    REQUIRE(failed.error == Registry::ErrorCode::FontFailure);
    REQUIRE(failed.fontError.code == LC_OpenTypeFont::ErrorCode::ChecksumMismatch);
    Registry fallbackRoot({{temporary.filePath("missing"), root}, {}});
    REQUIRE(fallbackRoot.resolve(entry.identity));
}

#ifdef Q_OS_UNIX
TEST_CASE("A dangling higher priority font link is not silently substituted", "[opentype][registry][symlink][headless]") {
    using Registry = LC_TextFontRegistry;
    requireCertifiedEngine();
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto firstRoot = temporary.filePath("broken");
    const auto secondRoot = temporary.filePath("valid");
    const auto entry = Registry::amiriEntry();
    const auto brokenPath = QDir(firstRoot).absoluteFilePath(entry.relativeFontPath);
    REQUIRE(QDir().mkpath(QFileInfo(brokenPath).absolutePath()));
    REQUIRE(QFile::link(temporary.filePath("missing-font.ttf"), brokenPath));
    REQUIRE(QFileInfo(brokenPath).isSymLink());
    REQUIRE_FALSE(QFileInfo::exists(brokenPath));
    install(secondRoot);
    Registry registry({{firstRoot, secondRoot}, {}});
    const auto failed = registry.resolve(entry.identity);
    REQUIRE_FALSE(failed);
    REQUIRE(failed.error == Registry::ErrorCode::FontFailure);
    REQUIRE(failed.resourcePath == brokenPath);
}
#endif

TEST_CASE("Registry resources remain valid during concurrent resolve and clear", "[opentype][registry][cache][headless]") {
    using Registry = LC_TextFontRegistry;
    requireCertifiedEngine();
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto root = temporary.filePath("fonts");
    install(root);
    const auto entry = Registry::amiriEntry();
    Registry registry({{root}, {}});
    const auto held = registry.resolve(entry.identity);
    REQUIRE(held);
    std::vector<std::future<bool>> workers;
    for (unsigned worker = 0; worker < 8; ++worker) {
        workers.push_back(std::async(std::launch::async, [&registry, entry, worker] {
            for (unsigned iteration = 0; iteration < 32; ++iteration) {
                if ((iteration + worker) % 7 == 0) registry.clear();
                const auto loaded = registry.resolve(entry.identity);
                if (!loaded || loaded.font->checksum() != entry.identity.fontSha256) return false;
                const auto outline = loaded.font->outline(3999);
                if (!outline || !outline.outline->hasInk) return false;
            }
            return true;
        }));
    }
    for (auto &worker : workers) REQUIRE(worker.get());
    REQUIRE(held.font->checksum() == entry.identity.fontSha256);
    REQUIRE(held.font->outline(3999));
}
#endif
