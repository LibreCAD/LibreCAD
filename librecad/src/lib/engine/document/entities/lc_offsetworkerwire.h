/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_OFFSETWORKERWIRE_H
#define LC_OFFSETWORKERWIRE_H

#include <QByteArray>
#include <QCborArray>
#include <cstddef>

/** Offset-helper transport only. A decoded frame is not a geometry certificate. */
namespace LC_OffsetWorkerWire {
struct Limits {
    qsizetype frameBytes = 16 * 1024 * 1024;
    std::size_t items = 262144;
    quint64 arrayItems = 65536;
    unsigned depth = 32;
};
enum class Error {
    None, InvalidLimits, FrameLength, ExtraData, Truncated, InvalidCbor,
    UnsupportedType, NonFinite, Depth, ArrayItems, Items
};
struct Work {
    qsizetype payloadBytes = 0;
    std::size_t items = 0;
    unsigned deepest = 0;
};
struct Message {
    Error error = Error::InvalidCbor;
    QCborArray fields;
    Work work;
    explicit operator bool() const noexcept { return error == Error::None; }
};

/** Fixed arrays, finite double/int64 scalars, bool and null only; no maps,
 * strings, tags, byte strings, indefinite containers or trailing CBOR items.
 * Operation-specific field/type/count validation follows in the typed codec.
 */
Message decode(const QByteArray& payload, const Limits& = {});
QByteArray encode(const QCborArray& fields, Error* = nullptr, const Limits& = {});

/** Exactly one big-endian uint32-length-prefixed frame per helper process.
 * A refusal is sticky. Empty chunks are harmless; bytes after completion refuse.
 * Call finish() at EOF; a partial header or body cannot be accepted.
 */
class Receiver {
public:
    explicit Receiver(const Limits& = {});
    bool append(const QByteArray& chunk);
    Message finish();
    Error error() const noexcept { return m_error; }
    bool complete() const noexcept { return m_expected != 0 && m_payload.size() == m_expected; }
    qsizetype bufferedBytes() const noexcept { return m_header.size() + m_payload.size(); }
private:
    bool refuse(Error);
    Limits m_limits;
    QByteArray m_header, m_payload;
    qsizetype m_expected = 0;
    Error m_error = Error::None;
    bool m_finished = false;
};
}
#endif
