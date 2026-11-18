/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerwire.h"

#include <QCborValue>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <limits>

namespace LC_OffsetWorkerWire {
namespace {
bool valid(const Limits& limits) {
    const Limits hard;
    return limits.frameBytes > 0 && limits.frameBytes <= hard.frameBytes &&
        limits.items > 0 && limits.items <= hard.items &&
        limits.arrayItems > 0 && limits.arrayItems <= hard.arrayItems &&
        limits.depth > 0 && limits.depth <= hard.depth;
}
struct Scan {
    const Limits& limits;
    const QByteArray* payload = nullptr;
    qsizetype offset = 0;
    Work work;
    Error error = Error::None;
    bool fail(Error value) { error = value; return false; }
    bool admit(unsigned depth) {
        if (work.items == limits.items) return fail(Error::Items);
        ++work.items;
        work.deepest = std::max(work.deepest, depth);
        return depth <= limits.depth || fail(Error::Depth);
    }
    bool byte(quint8& value) {
        if (!payload || offset >= payload->size()) return fail(Error::InvalidCbor);
        value = quint8(static_cast<unsigned char>(payload->at(offset++)));
        return true;
    }
    bool argument(quint8 additional, quint64& value) {
        if (additional < 24) { value = additional; return true; }
        const auto remaining = payload->size() - offset;
        if (additional == 24) {
            quint8 raw;
            if (!byte(raw)) return false;
            value = raw;
        } else if (additional == 25) {
            if (remaining < 2) return fail(Error::InvalidCbor);
            value = qFromBigEndian<quint16>(payload->constData() + offset);
            offset += 2;
        } else if (additional == 26) {
            if (remaining < 4) return fail(Error::InvalidCbor);
            value = qFromBigEndian<quint32>(payload->constData() + offset);
            offset += 4;
        } else if (additional == 27) {
            if (remaining < 8) return fail(Error::InvalidCbor);
            value = qFromBigEndian<quint64>(payload->constData() + offset);
            offset += 8;
        } else {
            return fail(Error::UnsupportedType);
        }
        return true;
    }
    bool read(unsigned depth) {
        if (!admit(depth)) return false;
        quint8 head;
        if (!byte(head)) return false;
        const quint8 major = head >> 5;
        const quint8 additional = head & 0x1f;
        if (major == 4) {
            quint64 count;
            if (!argument(additional, count)) return false;
            if (count > limits.arrayItems) return fail(Error::ArrayItems);
            if (work.items > limits.items || count > limits.items - work.items) return fail(Error::Items);
            for (quint64 i = 0; i < count; ++i) if (!read(depth + 1)) return false;
            return true;
        }
        if (major == 0 || major == 1) {
            quint64 value;
            if (!argument(additional, value)) return false;
            return value <= quint64(std::numeric_limits<qint64>::max()) || fail(Error::UnsupportedType);
        }
        if (major != 7) return fail(Error::UnsupportedType);
        if (additional == 20 || additional == 21 || additional == 22) return true;
        if (additional == 25 || additional == 26) return fail(Error::UnsupportedType);
        if (additional == 27) {
            const auto remaining = payload->size() - offset;
            if (remaining < 8) return fail(Error::InvalidCbor);
            const auto bits = qFromBigEndian<quint64>(payload->constData() + offset);
            offset += 8;
            return (bits & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL || fail(Error::NonFinite);
        }
        return fail(Error::UnsupportedType);
    }
    bool value(const QCborValue& item, unsigned depth) {
        if (!admit(depth)) return false;
        if (item.isArray()) {
            const auto array = item.toArray();
            if (quint64(array.size()) > limits.arrayItems) return fail(Error::ArrayItems);
            if (std::size_t(array.size()) > limits.items - work.items) return fail(Error::Items);
            for (const auto& child : array) if (!value(child, depth + 1)) return false;
            return true;
        }
        if (item.isDouble()) return std::isfinite(item.toDouble()) || fail(Error::NonFinite);
        return item.isInteger() || item.isBool() || item.isNull() || fail(Error::UnsupportedType);
    }
};
}

Message decode(const QByteArray& payload, const Limits& limits) {
    Message result;
    if (!valid(limits)) { result.error = Error::InvalidLimits; return result; }
    if (payload.isEmpty() || payload.size() > limits.frameBytes) {
        result.error = Error::FrameLength; return result;
    }
    Scan scan{limits, &payload};
    scan.work.payloadBytes = payload.size();
    if (quint8(static_cast<unsigned char>(payload.at(0))) >> 5 != 4) result.error = Error::UnsupportedType;
    else if (!scan.read(1)) result.error = scan.error;
    else if (scan.offset != payload.size()) result.error = Error::InvalidCbor;
    else {
        QCborParserError parse;
        const auto value = QCborValue::fromCbor(payload, &parse);
        if (parse.error != QCborError::NoError || !value.isArray()) result.error = Error::InvalidCbor;
        else { result.fields = value.toArray(); result.error = Error::None; }
    }
    result.work = scan.work;
    return result;
}

QByteArray encode(const QCborArray& fields, Error* error, const Limits& limits) {
    Error failure = Error::None;
    QByteArray frame;
    if (!valid(limits)) failure = Error::InvalidLimits;
    else {
        Scan scan{limits};
        if (!scan.value(QCborValue(fields), 1)) failure = scan.error;
        else {
            const auto payload = QCborValue(fields).toCbor();
            if (payload.isEmpty() || payload.size() > limits.frameBytes) failure = Error::FrameLength;
            else {
                Scan encoded{limits, &payload};
                if (quint8(static_cast<unsigned char>(payload.at(0))) >> 5 != 4) failure = Error::InvalidCbor;
                else if (!encoded.read(1)) failure = encoded.error;
                else if (encoded.offset != payload.size()) failure = Error::InvalidCbor;
                else {
                    frame.resize(4);
                    qToBigEndian(quint32(payload.size()), frame.data());
                    frame.append(payload);
                }
            }
        }
    }
    if (error) *error = failure;
    return frame;
}

Receiver::Receiver(const Limits& limits) : m_limits(limits) {
    if (!valid(limits)) m_error = Error::InvalidLimits;
}
bool Receiver::refuse(Error error) {
    if (m_error == Error::None) m_error = error;
    m_header.clear(); m_payload.clear(); m_expected = 0;
    return false;
}
bool Receiver::append(const QByteArray& chunk) {
    if (m_error != Error::None) return false;
    if (chunk.isEmpty()) return true;
    if (m_finished || complete()) return refuse(Error::ExtraData);
    qsizetype offset = 0;
    if (m_header.size() < 4) {
        const auto count = std::min(qsizetype{4} - m_header.size(), chunk.size());
        m_header.append(chunk.constData(), count); offset += count;
        if (m_header.size() < 4) return true;
        const auto size = qFromBigEndian<quint32>(m_header.constData());
        if (size == 0 || size > quint32(m_limits.frameBytes)) return refuse(Error::FrameLength);
        m_expected = qsizetype(size);
    }
    const auto count = chunk.size() - offset;
    if (count > m_expected - m_payload.size()) return refuse(Error::ExtraData);
    m_payload.append(chunk.constData() + offset, count);
    return true;
}
Message Receiver::finish() {
    if (m_finished && m_error == Error::None) refuse(Error::ExtraData);
    m_finished = true;
    if (m_error != Error::None) return {m_error, {}, {}};
    if (!complete()) { refuse(Error::Truncated); return {m_error, {}, {}}; }
    auto result = decode(m_payload, m_limits);
    if (!result) refuse(result.error);
    return result;
}
}
