/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LC_FILEIDENTITY_H
#define LC_FILEIDENTITY_H

#include <QDateTime>
#include <QString>

/** Metadata and native object identity captured for a regular file. */
struct LC_FileIdentity {
    struct NativeState {
        bool valid = false;
        quint64 device = 0, inode = 0;
        qint64 size = 0, modifiedSeconds = 0, modifiedNanos = 0, changedSeconds = 0, changedNanos = 0;
        bool operator==(const NativeState& other) const noexcept;
        bool sameObject(const NativeState& other) const noexcept;
    };
    bool valid = false;
    QString sourcePath;
    QString canonicalPath;
    qint64 size = -1;
    QDateTime modified, metadataChanged, birth;
    NativeState native;

    static LC_FileIdentity capture(const QString& file);
    bool matches(const QString& file) const;
    bool matchesDescriptor(int descriptor) const noexcept;
    bool refersToPath(const QString& file) const;
    bool refersToSameObject(const QString& file) const;
};

#endif
