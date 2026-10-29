/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "lc_offsetworkerwire.h"

#include <catch2/catch_test_macros.hpp>
#include <QCborMap>
#include <QCborValue>
#include <QtEndian>
#include <cmath>
#include <limits>

namespace wire = LC_OffsetWorkerWire;
namespace {
QByteArray framed(const QByteArray& payload) {
    QByteArray bytes(4, '\0');
    qToBigEndian(quint32(payload.size()), bytes.data());
    return bytes + payload;
}
wire::Message receive(const QByteArray& bytes, const wire::Limits& limits = {}) {
    wire::Receiver receiver(limits);
    receiver.append(bytes);
    return receiver.finish();
}
}

TEST_CASE("offset worker wire handles every transport split without changing typed scalars",
          "[lc-offsetcurve][worker-protocol][wire]") {
    const QCborArray fields{1, qint64{42}, QCborArray{0.25, -0.0, true, false, nullptr},
        std::numeric_limits<qint64>::min(), std::numeric_limits<qint64>::max()};
    wire::Error error;
    const auto bytes = wire::encode(fields, &error);
    REQUIRE(error == wire::Error::None);
    REQUIRE(bytes.size() > 4);
    for (qsizetype split = 0; split <= bytes.size(); ++split) {
        CAPTURE(split);
        wire::Receiver receiver;
        REQUIRE(receiver.append(bytes.first(split)));
        REQUIRE(receiver.append({}));
        REQUIRE(receiver.append(bytes.sliced(split)));
        REQUIRE(receiver.complete());
        const auto message = receiver.finish();
        REQUIRE(message);
        CHECK(message.fields == fields);
        REQUIRE(message.fields.at(2).isArray());
        const auto nested = message.fields.at(2).toArray();
        CHECK(nested.at(0).isDouble());
        CHECK(nested.at(1).isDouble());
        CHECK(std::signbit(nested.at(1).toDouble()));
        CHECK(message.fields.at(3).toInteger() == std::numeric_limits<qint64>::min());
        CHECK(message.fields.at(4).toInteger() == std::numeric_limits<qint64>::max());
        CHECK(message.fields.at(3).isInteger());
        CHECK(message.fields.at(4).isInteger());
        CHECK(message.work.items == 11);
        CHECK(message.work.deepest == 3);
        CHECK(message.work.payloadBytes == bytes.size() - 4);
    }
    wire::Receiver bytewise;
    for (const auto byte : bytes) REQUIRE(bytewise.append(QByteArray(1, byte)));
    REQUIRE(bytewise.finish());
}

TEST_CASE("offset worker wire refuses incomplete zero oversized and duplicate frames",
          "[lc-offsetcurve][worker-protocol][wire]") {
    const auto bytes = wire::encode(QCborArray{1});
    for (qsizetype size = 0; size < bytes.size(); ++size) {
        CAPTURE(size);
        CHECK(receive(bytes.first(size)).error == wire::Error::Truncated);
    }
    CHECK(receive(QByteArray(4, '\0')).error == wire::Error::FrameLength);
    QByteArray oversized(4, '\0');
    qToBigEndian(quint32(wire::Limits{}.frameBytes + 1), oversized.data());
    wire::Receiver receiver;
    CHECK_FALSE(receiver.append(oversized));
    CHECK(receiver.bufferedBytes() == 0);
    CHECK_FALSE(receiver.append(bytes));
    CHECK(receiver.error() == wire::Error::FrameLength);
    qToBigEndian(quint32(wire::Limits{}.frameBytes), oversized.data());
    wire::Receiver exactHeader;
    REQUIRE(exactHeader.append(oversized));
    CHECK(exactHeader.bufferedBytes() == 4);
    CHECK(exactHeader.finish().error == wire::Error::Truncated);
    CHECK(receive(bytes + bytes).error == wire::Error::ExtraData);
    wire::Receiver duplicate;
    REQUIRE(duplicate.append(bytes));
    CHECK_FALSE(duplicate.append(bytes));
    CHECK(duplicate.finish().error == wire::Error::ExtraData);
    wire::Receiver finished;
    REQUIRE(finished.append(bytes));
    REQUIRE(finished.finish());
    CHECK_FALSE(finished.append(bytes));
    CHECK(finished.finish().error == wire::Error::ExtraData);
    CHECK_FALSE(finished.append(bytes));
    wire::Receiver finishedTwice;
    REQUIRE(finishedTwice.append(bytes));
    REQUIRE(finishedTwice.finish());
    CHECK(finishedTwice.finish().error == wire::Error::ExtraData);
}

TEST_CASE("offset worker wire retains the first refusal through all later input and EOF",
          "[lc-offsetcurve][worker-protocol][wire]") {
    const auto bytes = wire::encode(QCborArray{1});
    for (const auto malformed : {false, true}) {
        CAPTURE(malformed);
        wire::Receiver receiver;
        const auto error = malformed ? wire::Error::UnsupportedType : wire::Error::Truncated;
        REQUIRE(receiver.append(malformed ? framed(QByteArray::fromHex("81f7")) : bytes.first(5)));
        const auto first = receiver.finish();
        CHECK(first.error == error);
        CHECK(first.fields.empty());
        CHECK(receiver.bufferedBytes() == 0);
        CHECK_FALSE(receiver.complete());
        CHECK_FALSE(receiver.append({}));
        CHECK_FALSE(receiver.append(bytes));
        CHECK(receiver.error() == error);
        CHECK(receiver.finish().error == error);
        CHECK(receiver.bufferedBytes() == 0);
    }
}

TEST_CASE("offset worker wire bounds actual CBOR structure before building value trees",
          "[lc-offsetcurve][worker-protocol][wire]") {
    const QCborArray fields{QCborArray{QCborArray{1}}};
    const auto payload = QCborValue(fields).toCbor();
    wire::Limits limits;
    limits.depth = 4; limits.items = 4; limits.arrayItems = 1;
    REQUIRE(wire::decode(payload, limits));
    limits.depth = 3;
    CHECK(wire::decode(payload, limits).error == wire::Error::Depth);
    wire::Error error;
    CHECK(wire::encode(fields, &error, limits).isEmpty());
    CHECK(error == wire::Error::Depth);
    limits.depth = 4; limits.items = 3;
    CHECK(wire::decode(payload, limits).error == wire::Error::Items);
    CHECK(wire::encode(fields, &error, limits).isEmpty());
    CHECK(error == wire::Error::Items);
    limits.items = 4;
    CHECK(wire::decode(QCborValue(QCborArray{1, 2}).toCbor(), limits).error == wire::Error::ArrayItems);
    CHECK(wire::encode(QCborArray{1, 2}, &error, limits).isEmpty());
    CHECK(error == wire::Error::ArrayItems);
    limits = {};
    limits.frameBytes = payload.size();
    REQUIRE(receive(framed(payload), limits));
    --limits.frameBytes;
    CHECK(receive(framed(payload), limits).error == wire::Error::FrameLength);
    CHECK(wire::encode(fields, &error, limits).isEmpty());
    CHECK(error == wire::Error::FrameLength);
}

TEST_CASE("offset worker wire rejects higher limits instead of enlarging its hard allowance",
          "[lc-offsetcurve][worker-protocol][wire]") {
    const auto bytes = wire::encode(QCborArray{1});
    for (unsigned which = 0; which < 8; ++which) {
        CAPTURE(which);
        wire::Limits limits;
        if (which == 0) limits.frameBytes = 0;
        if (which == 1) ++limits.frameBytes;
        if (which == 2) limits.items = 0;
        if (which == 3) ++limits.items;
        if (which == 4) limits.arrayItems = 0;
        if (which == 5) ++limits.arrayItems;
        if (which == 6) limits.depth = 0;
        if (which == 7) ++limits.depth;
        CHECK(receive(bytes, limits).error == wire::Error::InvalidLimits);
        CHECK(wire::decode(bytes.sliced(4), limits).error == wire::Error::InvalidLimits);
        wire::Error error;
        CHECK(wire::encode(QCborArray{1}, &error, limits).isEmpty());
        CHECK(error == wire::Error::InvalidLimits);
    }
}

TEST_CASE("offset worker wire admits hard structural boundaries and rejects exactly one more",
          "[lc-offsetcurve][worker-protocol][wire]") {
    QCborArray row;
    for (quint64 i = 0; i < wire::Limits{}.arrayItems; ++i) row.append(nullptr);
    REQUIRE(receive(wire::encode(row)));
    row.append(nullptr);
    wire::Error error;
    CHECK(wire::encode(row, &error).isEmpty());
    CHECK(error == wire::Error::ArrayItems);
    CHECK(wire::decode(QCborValue(row).toCbor()).error == wire::Error::ArrayItems);

    row.removeLast(); row.removeLast(); // 65535, so four subarrays can reach the item cap.
    auto last = row;
    last.removeLast();
    QCborArray full{row, row, row, last};
    const auto exact = receive(wire::encode(full));
    REQUIRE(exact);
    CHECK(exact.work.items == wire::Limits{}.items);
    last.append(nullptr); full[3] = last;
    CHECK(wire::encode(full, &error).isEmpty());
    CHECK(error == wire::Error::Items);
    CHECK(wire::decode(QCborValue(full).toCbor()).error == wire::Error::Items);

    QByteArray exactDepth(int(wire::Limits{}.depth - 1), char(0x81));
    exactDepth.append(char(0x00));
    const auto exactDepthValue = wire::decode(exactDepth);
    REQUIRE(exactDepthValue);
    CHECK(exactDepthValue.work.deepest == wire::Limits{}.depth);
    REQUIRE_FALSE(wire::encode(exactDepthValue.fields).isEmpty());
    QByteArray excessiveDepth = exactDepth;
    excessiveDepth.prepend(char(0x81));
    CHECK(wire::decode(excessiveDepth).error == wire::Error::Depth);
    CHECK(wire::encode(QCborArray{exactDepthValue.fields}, &error).isEmpty());
    CHECK(error == wire::Error::Depth);
}

TEST_CASE("offset worker wire rejects nonfinite ambiguous and extensible CBOR encodings",
          "[lc-offsetcurve][worker-protocol][wire]") {
    for (const auto number : {std::numeric_limits<double>::infinity(),
                             -std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN()}) {
        const QCborArray fields{number};
        CHECK(wire::decode(QCborValue(fields).toCbor()).error == wire::Error::NonFinite);
        wire::Error error;
        CHECK(wire::encode(fields, &error).isEmpty());
        CHECK(error == wire::Error::NonFinite);
    }
    for (const auto& bytes : {QByteArray::fromHex("9f01ff"), // indefinite array
                             QByteArray::fromHex("81a201000101"), // duplicate map keys
                             QByteArray::fromHex("816178"), // string
                             QByteArray::fromHex("814100"), // bytes
                             QByteArray::fromHex("81c001"), // tag
                             QByteArray::fromHex("81f7"), // undefined
                             QByteArray::fromHex("81f93c00"), // float16
                             QByteArray::fromHex("81fa3f800000"), // float32
                             QByteArray::fromHex("819affffffff"), // oversized declared array
                             QByteArray::fromHex("811b8000000000000000"), // INT64_MAX + 1
                             QByteArray::fromHex("813b8000000000000000"), // INT64_MIN - 1
                             QByteArray::fromHex("811bffffffffffffffff"),
                             QByteArray::fromHex("813bffffffffffffffff")}) {
        CAPTURE(bytes.toHex().constData());
        CHECK_FALSE(wire::decode(bytes));
    }
    const auto payload = QCborValue(QCborArray{1}).toCbor();
    CHECK_FALSE(wire::decode(payload + QByteArray(1, '\0')));
    CHECK_FALSE(wire::decode(QByteArray::fromHex("8201")));
    CHECK_FALSE(wire::decode(QByteArray::fromHex("01")));
    for (const QCborValue& value : {QCborValue(QStringLiteral("text")), QCborValue(QByteArray("bytes")),
                                    QCborValue(QCborMap{}), QCborValue(QCborValue::Undefined)}) {
        wire::Error error;
        CHECK(wire::encode(QCborArray{value}, &error).isEmpty());
        CHECK(error == wire::Error::UnsupportedType);
    }
}
