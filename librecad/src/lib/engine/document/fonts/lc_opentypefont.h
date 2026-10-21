/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_OPENTYPEFONT_H
#define LC_OPENTYPEFONT_H

#include <QByteArray>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <cstdint>
#include <memory>
#include <vector>

class LC_TextShaper;

/** An exact, static font resource. No system font lookup or GUI is involved. */
class LC_OpenTypeFont final {
public:
    enum class ErrorCode {
        None, BackendUnavailable, MissingFile, UnreadableFile, ResourceLimit,
        ChecksumMismatch, InvalidFont, UnsupportedFont, InvalidProfile,
        MissingTables, InvalidGlyph, OutlineFailure, InvalidInput, ShapingFailure
    };
    struct Error {
        ErrorCode code = ErrorCode::None;
        QString message;
        explicit operator bool() const { return code != ErrorCode::None; }
    };
    struct Profile {
        QString id;
        QByteArray sha256;
        unsigned int faceIndex = 0;
        unsigned int unitsPerEm = 1000;
        double designHeight = 647.0;
        int ascender = 1124;
        int descender = -634;
        int lineHeight = 1758;
        bool requireArabicTables = true;
    };
    struct Command {
        enum class Type { MoveTo, LineTo, QuadraticTo, CubicTo, Close };
        Type type = Type::Close;
        // Move/Line: first is destination. Quadratic: first control, second
        // destination. Cubic: first/second controls, third destination.
        QPointF first;
        QPointF second;
        QPointF third;
    };
    struct Outline {
        std::vector<Command> commands;
        QRectF inkBounds; // Original, y-up font coordinates, never normalized.
        bool hasInk = false;
    };
    struct OutlineResult {
        std::shared_ptr<const Outline> outline;
        Error error;
        explicit operator bool() const { return outline && !error; }
    };
    struct LoadResult {
        std::shared_ptr<const LC_OpenTypeFont> font;
        Error error;
        explicit operator bool() const { return font && !error; }
    };
    struct CacheStats {
        std::size_t entries = 0;
        std::size_t bytes = 0;
        std::size_t hits = 0;
        std::size_t misses = 0;
    };

    static Profile amiriProfile();
    static bool backendAvailable();
    static QString shapingEngineIdentity();
    static LoadResult load(const QString &path, const Profile &profile = amiriProfile());
    ~LC_OpenTypeFont();
    LC_OpenTypeFont(const LC_OpenTypeFont &) = delete;
    LC_OpenTypeFont &operator=(const LC_OpenTypeFont &) = delete;

    const Profile &profile() const;
    const QByteArray &checksum() const;
    unsigned int glyphCount() const;
    double normalization() const; // Font units -> nominal 9-unit CAD cell.
    OutlineResult outline(std::uint32_t glyphId) const;
    CacheStats cacheStats() const;
    void clearOutlineCache() const;

private:
    struct Impl;
    explicit LC_OpenTypeFont(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
    friend class LC_TextShaper;
};

#endif
