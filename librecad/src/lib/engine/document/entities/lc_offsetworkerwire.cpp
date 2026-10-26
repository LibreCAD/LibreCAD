/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerwire.h"

#include <QCborStreamReader>
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
    Work work;
    Error error = Error::None;
    bool fail(Error value) { error = value; return false; }
    bool admit(unsigned depth) {
        if (work.items == limits.items) return fail(Error::Items);
        ++work.items;
        work.deepest = std::max(work.deepest, depth);
        return depth <= limits.depth || fail(Error::Depth);
    }
    bool read(QCborStreamReader& reader, unsigned depth) {
        if (!admit(depth)) return false;
        if (reader.isArray()) {
            if (!reader.isLengthKnown()) return fail(Error::UnsupportedType);
            const auto count = reader.length();
            if (count > limits.arrayItems) return fail(Error::ArrayItems);
            if (count > limits.items - work.items) return fail(Error::Items);
            if (!reader.enterContainer()) return fail(Error::InvalidCbor);
            for (quint64 i = 0; i < count; ++i)
                if (!reader.hasNext() || !read(reader, depth + 1)) return failIfUnset();
            if (reader.hasNext() || !reader.leaveContainer()) return fail(Error::InvalidCbor);
            return true;
        }
        if (reader.isDouble()) {
            if (!std::isfinite(reader.toDouble())) return fail(Error::NonFinite);
        } else if (reader.isUnsignedInteger()) {
            if (reader.toUnsignedInteger() > quint64(std::numeric_limits<qint64>::max()))
                return fail(Error::UnsupportedType);
        } else if (reader.isNegativeInteger()) {
            // Qt exposes magnitude+1, which wraps to zero for CBOR -2^64.
            const auto magnitude = quint64(reader.toNegativeInteger());
            if (magnitude == 0 || magnitude > (quint64{1} << 63))
                return fail(Error::UnsupportedType);
        } else if (!reader.isBool() && !reader.isNull()) return fail(Error::UnsupportedType);
        reader.next(0);
        return reader.lastError() == QCborError::NoError || fail(Error::InvalidCbor);
    }
    bool failIfUnset() { return error == Error::None ? fail(Error::InvalidCbor) : false; }
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
    Scan scan{limits};
    scan.work.payloadBytes = payload.size();
    QCborStreamReader reader(payload);
    if (!reader.isArray()) result.error = Error::UnsupportedType;
    else if (!scan.read(reader, 1)) result.error = scan.error;
    else if (reader.lastError() != QCborError::NoError || reader.currentOffset() != payload.size())
        result.error = Error::InvalidCbor;
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
                frame.resize(4);
                qToBigEndian(quint32(payload.size()), frame.data());
                frame.append(payload);
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
