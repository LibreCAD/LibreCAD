/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textprofile.h"
#include <catch2/catch_test_macros.hpp>
#include <QMetaType>
#include <algorithm>
#include <limits>

namespace {
using Profile = LC_TextProfile;
using Code = Profile::ErrorCode;
using Scope = Profile::Scope;
using Direction = Profile::Direction;
using Backend = Profile::Backend;
using Status = Profile::DecodeStatus;
using Marker = Profile::RtlMarker;
using Origin = Profile::Origin;

Profile::Selection openType() {
    return {Backend::HarfBuzzOpenType, {QStringLiteral("lc-amiri-1.003-ot-v1"),
        QByteArray("cd2550c0f4c05eb341bf97958211aaa39382bca96577ba3a67d4a3b4912c43c0"), 0,
        QStringLiteral("amiri-cap-y647-cell9-baseline-v1"),
        QStringLiteral("hb-ot-default-monotone-characters-v1"), QStringLiteral("HarfBuzz-OT/14.6.0")}};
}
Profile::Value style() { return {Scope::StyleDefaults, openType(), QByteArray("ar"), Direction::Auto}; }
Profile::Value entity() { return {Scope::EntityOverride, {}, {}, {}}; }
Profile::Block encoded(Profile::Value value = style()) {
    const auto result = Profile::encode(value);
    REQUIRE(result);
    return result.block;
}
std::size_t field(const Profile::Block &block, const QString &key) {
    for (std::size_t i = 2; i < block.size(); ++i)
        if (block[i].code == 1000 && block[i].value.metaType().id() == QMetaType::QString &&
            (block[i].value.toString() == key || block[i].value.toString().startsWith(key + '='))) return i;
    FAIL("Missing expected field " << key.toStdString());
    return 0;
}
void removeField(Profile::Block &block, const QString &key) {
    const auto index = field(block, key);
    const auto count = block[index].value.toString() == key ? 2 : 1;
    block.erase(block.begin() + index, block.begin() + index + count);
}
void reject(const Profile::Block &block, Code code) {
    const auto result = Profile::decode(block);
    CHECK(result.status == Status::Invalid);
    CHECK_FALSE(result.known());
    CHECK_FALSE(result.value);
    CHECK(result.error.code == code);
    REQUIRE(result.raw);
    CHECK(*result.raw == block);
    CHECK(Profile::inherit(result, Profile::Decoded{}, "ar").status == Profile::InheritanceStatus::Unavailable);
}
void opaque(const Profile::Block &block, Code code) {
    const auto raw = std::make_shared<const Profile::Block>(block);
    const auto result = Profile::decode(raw);
    CHECK(result.status == Status::Opaque);
    CHECK_FALSE(result.known());
    CHECK_FALSE(result.value);
    CHECK(result.error.code == code);
    CHECK(result.raw == raw);
    CHECK(*result.raw == block);
    const auto inherited = Profile::inherit(result, Profile::Decoded{}, "ar");
    CHECK(inherited.status == Profile::InheritanceStatus::Unavailable);
    CHECK_FALSE(inherited.openTypeSelected());
    CHECK_FALSE(inherited.requiresLegacyRendering());
    CHECK(inherited.style.raw == raw);
}
} // namespace

TEST_CASE("Provisional text profiles encode exact portable namespace fields", "[text-profile][codec][headless]") {
    const auto value = style();
    const auto block = encoded(value);
    REQUIRE(block.size() == 14);
    CHECK(block[0] == Profile::Tag{1001, QStringLiteral("LibreCADText")});
    CHECK(block[1] == Profile::Tag{1070, 1});
    CHECK(block[field(block, "face") + 1] == Profile::Tag{1070, 0});
    CHECK(block[field(block, "direction") + 1] == Profile::Tag{1070, 0});
    for (const auto &tag : block) {
        CHECK((tag.code == 1001 || tag.code == 1000 || tag.code == 1070));
        CHECK(tag.code != 1071);
        if (tag.code == 1070) CHECK(tag.value.metaType().id() == QMetaType::Int);
        else {
            CHECK(tag.value.metaType().id() == QMetaType::QString);
            CHECK(tag.value.toString().toUtf8().size() <= 255);
        }
    }
    const auto result = Profile::decode(block);
    REQUIRE(result.known());
    CHECK_FALSE(result.error);
    CHECK(result.value == std::optional<Profile::Value>(value));
    CHECK(*result.raw == block);
    CHECK(Profile::encode(*result.value).block == block);
    CHECK_FALSE(Profile::Tag{1070, 1} == Profile::Tag{1070, QStringLiteral("1")});
    CHECK_FALSE(Profile::Tag{1070, 1} == Profile::Tag{1070, 1.0});
}

TEST_CASE("Style defaults and partial entity overrides roundtrip without inference", "[text-profile][codec][headless]") {
    for (int present = 0; present < 8; ++present) {
        for (const auto backend : {Backend::LegacyLff, Backend::HarfBuzzOpenType}) {
            for (const auto direction : {Direction::Auto, Direction::LeftToRight, Direction::RightToLeft, Direction::ByStyle}) {
                auto value = entity();
                if (present & 1) value.selection = backend == Backend::LegacyLff ? Profile::Selection{} : openType();
                if (present & 2) value.language = "fa-IR";
                if (present & 4) value.direction = direction;
                const auto result = Profile::decode(encoded(value));
                REQUIRE(result.known());
                CHECK(result.value == std::optional<Profile::Value>(value));
            }
        }
    }
    auto legacy = style();
    legacy.selection = Profile::Selection{};
    CHECK(Profile::decode(encoded(legacy)).value == std::optional<Profile::Value>(legacy));
    for (int missing = 0; missing < 3; ++missing) {
        auto value = style();
        if (missing == 0) value.selection.reset();
        if (missing == 1) value.language.reset();
        if (missing == 2) value.direction.reset();
        CHECK(Profile::validate(value).code == Code::MissingField);
        const auto result = Profile::encode(value);
        CHECK_FALSE(result);
        CHECK(result.block.empty());
    }
    auto recursive = style(); recursive.direction = Direction::ByStyle;
    CHECK(Profile::validate(recursive).code == Code::InvalidField);
    CHECK_FALSE(Profile::encode(recursive));
    CHECK(Profile::decode(Profile::Block{}).status == Status::Absent);
    CHECK(Profile::decode(std::shared_ptr<const Profile::Block>{}).status == Status::Absent);
}

TEST_CASE("Known fields reject duplicates regardless of value or ordering", "[text-profile][codec][negative]") {
    const auto block = encoded();
    for (const auto *key : {"scope", "backend", "profile", "sha256", "face", "metrics", "shaping", "engine", "language", "direction"}) {
        CAPTURE(key);
        auto duplicate = block;
        const auto index = field(block, key);
        duplicate.push_back(block[index]);
        if (block[index].value.toString() == key) duplicate.push_back(block[index + 1]);
        reject(duplicate, Code::DuplicateField);
    }
    auto reordered = block;
    const auto face = field(reordered, "face");
    const auto key = reordered[face], number = reordered[face + 1];
    reordered.erase(reordered.begin() + face, reordered.begin() + face + 2);
    reordered.push_back(key); reordered.push_back(number);
    const auto decoded = Profile::decode(reordered);
    REQUIRE(decoded.known());
    CHECK(decoded.value == std::optional<Profile::Value>(style()));
    CHECK(*decoded.raw == reordered);
    CHECK(Profile::encode(*decoded.value).block == block);
    auto unknownDuplicate = block;
    unknownDuplicate.push_back({1000, QStringLiteral("future=first")});
    unknownDuplicate.push_back({1000, QStringLiteral("future=second")});
    reject(unknownDuplicate, Code::DuplicateField);
}

TEST_CASE("Incomplete atomic selections and malformed scope or field values cannot activate", "[text-profile][codec][negative]") {
    for (const auto *key : {"scope", "backend", "profile", "sha256", "face", "metrics", "shaping", "engine", "language", "direction"}) {
        CAPTURE(key);
        auto block = encoded(); removeField(block, key);
        reject(block, Code::MissingField);
    }
    auto scope = encoded(); scope[field(scope, "scope")].value = QStringLiteral("scope=font-name");
    reject(scope, Code::InvalidScope);
    auto legacyIdentity = encoded(); legacyIdentity[field(legacyIdentity, "backend")].value = QStringLiteral("backend=legacy-lff");
    reject(legacyIdentity, Code::InvalidField);
    for (const auto *key : {"scope", "backend", "profile", "sha256", "metrics", "shaping", "engine", "language"}) {
        auto block = encoded(); block[field(block, key)].value = QString::fromLatin1(key);
        reject(block, Code::InvalidType);
    }
    for (const auto *key : {"face", "direction"}) {
        auto block = encoded(); block[field(block, key)].value = QString(QString::fromLatin1(key) + "=0");
        reject(block, Code::InvalidType);
    }
    auto badName = encoded(); badName.push_back({1000, QStringLiteral("bad/name=value")});
    reject(badName, Code::InvalidField);
}

TEST_CASE("Wire group and variant types are exact rather than numerically coerced", "[text-profile][codec][negative]") {
    for (const auto &replacement : {QVariant(QStringLiteral("1")), QVariant(1.0), QVariant(true), QVariant(qlonglong(1)), QVariant(uint(1))}) {
        auto schema = encoded(); schema[1].value = replacement;
        reject(schema, Code::InvalidSchema);
        for (const auto *key : {"face", "direction"}) {
            auto block = encoded(); block[field(block, key) + 1].value = replacement;
            reject(block, Code::InvalidType);
        }
    }
    for (const auto *key : {"scope", "backend", "profile", "sha256", "metrics", "shaping", "engine", "language"}) {
        auto block = encoded(); block[field(block, key)].value = 0;
        reject(block, Code::InvalidType);
    }
    auto wrongSchemaCode = encoded(); wrongSchemaCode[1].code = 1071;
    reject(wrongSchemaCode, Code::InvalidSchema);
    auto wrongFaceCode = encoded(); wrongFaceCode[field(wrongFaceCode, "face") + 1].code = 1071;
    reject(wrongFaceCode, Code::InvalidType);
    auto looseInteger = encoded(); looseInteger.push_back({1070, 0});
    reject(looseInteger, Code::InvalidType);
    auto truncatedInteger = encoded(); removeField(truncatedInteger, "direction"); truncatedInteger.push_back({1000, QStringLiteral("direction")});
    reject(truncatedInteger, Code::InvalidType);
}

TEST_CASE("Recognized wire field values are validated before they can activate", "[text-profile][codec][negative]") {
    for (const auto *key : {"backend", "profile", "metrics", "shaping"}) {
        for (const auto &bad : {QString(), QStringLiteral("/absolute"), QStringLiteral("C:font"),
                               QStringLiteral("bad name"), QStringLiteral("عربي"), QString(128, 'x')}) {
            CAPTURE(key, bad.toStdString());
            auto block = encoded();
            block[field(block, key)].value = QString(QString::fromLatin1(key) + '=' + bad);
            reject(block, Code::InvalidField);
        }
    }
    for (const auto &bad : {QString(), QStringLiteral("/absolute"), QStringLiteral("C:/font"),
                           QStringLiteral("Engine//1"), QStringLiteral("../font"), QStringLiteral("Engine/..")}) {
        auto block = encoded(); block[field(block, "engine")].value = QString(QStringLiteral("engine=") + bad);
        reject(block, Code::InvalidField);
    }
    for (const auto &bad : {QString(), QString(63, 'a'), QString(65, 'a'), QString(64, 'A'), QString(64, 'g'), QString(64, QChar(0x0661))}) {
        auto block = encoded(); block[field(block, "sha256")].value = QString(QStringLiteral("sha256=") + bad);
        reject(block, Code::InvalidField);
    }
    for (const auto &bad : {QString(), QStringLiteral("ar_eg"), QStringLiteral("ar--EG"),
                           QStringLiteral("a123"), QStringLiteral("abcdefghi"), QStringLiteral("عربي")}) {
        auto block = encoded(); block[field(block, "language")].value = QString(QStringLiteral("language=") + bad);
        reject(block, Code::InvalidField);
    }
    auto recursive = encoded(); recursive[field(recursive, "direction") + 1].value = 3;
    reject(recursive, Code::InvalidField);
}

TEST_CASE("Schema backend and fields remain opaque and exact without engine certification", "[text-profile][codec][opaque]") {
    for (int version : {2, 32767}) {
        auto block = encoded(); block[1].value = version;
        block.push_back({1004, QByteArray("opaque\0bytes", 12)});
        opaque(block, Code::UnknownSchema);
    }
    for (int version : {-32768, -1, 0, 32768, std::numeric_limits<int>::max()}) {
        auto block = encoded(); block[1].value = version;
        reject(block, Code::InvalidSchema);
    }
    auto unknownBackend = encoded(); unknownBackend[field(unknownBackend, "backend")].value = QStringLiteral("backend=future-shaper");
    opaque(unknownBackend, Code::UnknownBackend);
    for (const auto &tag : {Profile::Tag{1000, QStringLiteral("future=retain-opaque")},
                           Profile::Tag{1071, 17}, Profile::Tag{1004, QByteArray("blob")}, Profile::Tag{1040, 0.5}}) {
        auto block = encoded(); block.push_back(tag);
        opaque(block, Code::UnknownField);
    }
    auto unknownPair = encoded(); unknownPair.push_back({1000, QStringLiteral("future")}); unknownPair.push_back({1070, 17});
    opaque(unknownPair, Code::UnknownField);
    auto futureProfile = style();
    futureProfile.selection->identity.profileId = "lc-amiri-1.003-ot-v999";
    futureProfile.selection->identity.engineIdentity = "FutureShaper/999.0";
    const auto known = Profile::decode(encoded(futureProfile));
    REQUIRE(known.known()); // Syntax validity is deliberately not registry compatibility.
    CHECK(known.value == std::optional<Profile::Value>(futureProfile));
    CHECK(*known.raw == encoded(futureProfile));
}

TEST_CASE("Namespaces raw records and explicitly edited values have separate authority", "[text-profile][codec][source]") {
    auto block = encoded();
    const auto raw = std::make_shared<const Profile::Block>(block);
    const auto decoded = Profile::decode(raw);
    REQUIRE(decoded.known());
    auto edited = *decoded.value;
    edited.language = "ur"; edited.direction = Direction::RightToLeft;
    const auto current = Profile::encode(edited);
    REQUIRE(current);
    CHECK(current.block != block);
    CHECK(decoded.raw == raw);
    CHECK(*decoded.raw == block);
    CHECK(decoded.value == std::optional<Profile::Value>(style()));
    CHECK(Profile::decode(current.block).value == std::optional<Profile::Value>(edited));
    const Profile::Block unrelated{{1001, QStringLiteral("OtherCAD")}, {1070, -7}, {1000, QStringLiteral("unrelated")}};
    const auto before = unrelated;
    CHECK(Profile::decode(unrelated).status == Status::Invalid);
    CHECK(unrelated == before);
    auto combined = block; combined.insert(combined.end(), unrelated.begin(), unrelated.end());
    reject(combined, Code::InvalidNamespace);
    auto duplicateApp = block; duplicateApp.push_back(block.front());
    reject(duplicateApp, Code::InvalidNamespace);
    auto wrongApp = block; wrongApp.front().value = QStringLiteral("librecadtext");
    reject(wrongApp, Code::InvalidNamespace);
    auto missingApp = block; missingApp.erase(missingApp.begin());
    reject(missingApp, Code::InvalidNamespace);
    auto appType = block; appType.front().value = 1;
    reject(appType, Code::InvalidNamespace);
}

TEST_CASE("Identifier hashes language ranges and typed enum values are bounded", "[text-profile][codec][negative]") {
    for (const auto &bad : {QString(), QStringLiteral("/absolute"), QStringLiteral("C:font"), QStringLiteral("a\\font"),
                           QStringLiteral("a/font"), QStringLiteral(".."), QStringLiteral(".hidden"), QStringLiteral("bad name"),
                           QStringLiteral("عربي"), QString(128, 'x'), QString(QString("bad") + QChar(0))}) {
        for (int which = 0; which < 3; ++which) {
            auto value = style();
            if (which == 0) value.selection->identity.profileId = bad;
            if (which == 1) value.selection->identity.metricPolicy = bad;
            if (which == 2) value.selection->identity.shapingPolicy = bad;
            CHECK(Profile::validate(value).code == Code::InvalidField);
            CHECK_FALSE(Profile::encode(value));
        }
    }
    for (const auto &bad : {QString(), QStringLiteral("/absolute/path"), QStringLiteral("C:/font"), QStringLiteral("C:font"),
                           QStringLiteral("a\\font"), QStringLiteral("../font"), QStringLiteral("Engine/.."), QStringLiteral("Engine/"),
                           QStringLiteral("Engine//1"), QStringLiteral("Engine/1/2"), QStringLiteral("bad engine"), QString(128, 'x')}) {
        auto value = style(); value.selection->identity.engineIdentity = bad;
        CHECK_FALSE(Profile::encode(value));
    }
    for (const auto &good : {QByteArray("ar"), QByteArray("ar-EG"), QByteArray("zh-Hans-CN"), QByteArray("x-private"), QByteArray("i-klingon"), QByteArray("und")}) {
        auto value = style(); value.language = good;
        CHECK(Profile::decode(encoded(value)).known());
    }
    for (const auto &bad : {QByteArray(), QByteArray(" ar"), QByteArray("ar_eg"), QByteArray("ar--EG"), QByteArray("-ar"),
                           QByteArray("ar-"), QByteArray("a123"), QByteArray("abcdefghi"), QByteArray("ar-123456789"),
                           QByteArray("ar\0EG", 5), QByteArray(64, 'a'), QStringLiteral("عربي").toUtf8()}) {
        auto value = style(); value.language = bad;
        CHECK_FALSE(Profile::encode(value));
    }
    for (const auto &bad : {QByteArray(), QByteArray(63, 'a'), QByteArray(65, 'a'), QByteArray(64, 'A'), QByteArray(64, 'g'), QByteArray(64, '\0')}) {
        auto value = style(); value.selection->identity.fontSha256 = bad;
        CHECK_FALSE(Profile::encode(value));
    }
    for (unsigned face : {0u, 32767u}) {
        auto value = style(); value.selection->identity.faceIndex = face;
        CHECK(Profile::decode(encoded(value)).value == std::optional<Profile::Value>(value));
    }
    for (unsigned face : {32768u, 65535u, std::numeric_limits<unsigned>::max()}) {
        auto value = style(); value.selection->identity.faceIndex = face;
        CHECK_FALSE(Profile::encode(value));
    }
    for (int face : {-1, -32768, 32768}) {
        auto block = encoded(); block[field(block, "face") + 1].value = face;
        reject(block, face > 32767 ? Code::InvalidType : Code::InvalidField);
    }
    for (int direction : {-1, 4, 32767}) {
        auto block = encoded(); block[field(block, "direction") + 1].value = direction;
        reject(block, Code::InvalidField);
    }
    auto scope = style(); scope.scope = static_cast<Scope>(99); CHECK_FALSE(Profile::encode(scope));
    auto direction = style(); direction.direction = static_cast<Direction>(99); CHECK_FALSE(Profile::encode(direction));
    auto backend = style(); backend.selection->backend = static_cast<Backend>(99); CHECK_FALSE(Profile::encode(backend));
    auto legacy = style(); legacy.selection->backend = Backend::LegacyLff; CHECK_FALSE(Profile::encode(legacy));
}

TEST_CASE("Byte tag and UTF16 limits fail deterministically while preserving raw", "[text-profile][codec][limits]") {
    auto tagLimit = encoded();
    while (tagLimit.size() <= Profile::MaximumTags) tagLimit.push_back({1040, 1.0});
    reject(tagLimit, Code::ComplexityLimit);
    auto byteLimit = encoded();
    for (int i = 0; i < 20; ++i) byteLimit.push_back({1000, QString(QString("future%1=").arg(i) + QString(240, 'x'))});
    reject(byteLimit, Code::ComplexityLimit);
    auto boundary = encoded(); boundary.push_back({1000, QString(QStringLiteral("future=") + QString(248, 'x'))});
    opaque(boundary, Code::UnknownField);
    boundary.back().value = QString(QStringLiteral("future=") + QString(249, 'x'));
    reject(boundary, Code::ComplexityLimit);
    auto utf8 = encoded(); utf8.push_back({1000, QString(QStringLiteral("future=") + QString(124, QChar(0x0628)))});
    opaque(utf8, Code::UnknownField); // Seven ASCII bytes +248 UTF8 bytes =255.
    utf8.back().value = QString(QStringLiteral("future=") + QString(125, QChar(0x0628)));
    reject(utf8, Code::ComplexityLimit);
    for (const auto ch : {QChar(0), QChar(0xd800), QChar(0xdc00)}) {
        auto block = encoded(); block.push_back({1000, QString(QStringLiteral("future=") + ch)});
        reject(block, Code::InvalidField);
    }
    auto huge = encoded(); huge.push_back({1000, QString(1024 * 1024, 'x')});
    reject(huge, Code::ComplexityLimit);
    auto binary = encoded(); binary.push_back({1004, QByteArray(256, 'x')});
    reject(binary, Code::ComplexityLimit);
    auto nested = encoded(); nested.push_back({1000, QVariantList{1, 2, 3}});
    reject(nested, Code::InvalidType);
}

TEST_CASE("Inheritance direction and language authority are explicit and never inferred", "[text-profile][inheritance]") {
    for (const auto styleDirection : {Direction::Auto, Direction::LeftToRight, Direction::RightToLeft}) {
        for (const auto marker : {Marker::None, Marker::LogicalRtl, Marker::LegacyReversed}) {
            for (int overrideDirection = -1; overrideDirection <= 3; ++overrideDirection) {
                for (int selection = 0; selection < 3; ++selection) {
                    for (bool overrideLanguage : {false, true}) {
                        auto defaults = style(); defaults.direction = styleDirection;
                        auto override = entity();
                        if (overrideDirection >= 0) override.direction = static_cast<Direction>(overrideDirection);
                        if (selection == 1) override.selection = Profile::Selection{};
                        if (selection == 2) override.selection = openType();
                        if (overrideLanguage) override.language = "fa";
                        const auto inherited = Profile::inherit(std::optional<Profile::Value>(defaults), std::optional<Profile::Value>(override), "und", marker);
                        const bool conflict = marker == Marker::LegacyReversed && selection != 1;
                        CHECK(inherited.status == (conflict ? Profile::InheritanceStatus::LegacyConflict : Profile::InheritanceStatus::Resolved));
                        CHECK(inherited.openTypeSelected() == (!conflict && selection != 1));
                        CHECK(inherited.requiresLegacyRendering() == (conflict || selection == 1));
                        CHECK(inherited.effective.selectionOrigin == (selection ? Origin::Entity : Origin::Style));
                        CHECK(inherited.effective.language == (overrideLanguage ? QByteArray("fa") : QByteArray("ar")));
                        CHECK(inherited.effective.languageOrigin == (overrideLanguage ? Origin::Entity : Origin::Style));
                        const bool explicitDirection = overrideDirection >= 0 && overrideDirection != 3;
                        const auto direction = explicitDirection ? static_cast<Direction>(overrideDirection) : marker == Marker::LogicalRtl ? Direction::RightToLeft : styleDirection;
                        const auto origin = explicitDirection ? Origin::Entity : marker == Marker::LogicalRtl ? Origin::LogicalRtlMarker : Origin::Style;
                        CHECK(inherited.effective.direction == direction);
                        CHECK(inherited.effective.directionOrigin == origin);
                        CHECK(inherited.style.value == std::optional<Profile::Value>(defaults));
                        CHECK(inherited.entity.value == std::optional<Profile::Value>(override));
                        CHECK(inherited.error.code == (conflict ? Code::LegacyOrderConflict : Code::None));
                    }
                }
            }
        }
    }
    const auto fallback = Profile::inherit(Profile::Decoded{}, Profile::Decoded{}, "und");
    CHECK(fallback.requiresLegacyRendering());
    CHECK(fallback.effective.direction == Direction::Auto);
    CHECK(fallback.effective.language == "und");
    CHECK(fallback.effective.selectionOrigin == Origin::Fallback);
    CHECK(fallback.effective.directionOrigin == Origin::Fallback);
    const auto marker = Profile::inherit(Profile::Decoded{}, Profile::Decoded{}, "und", Marker::LogicalRtl);
    CHECK(marker.effective.direction == Direction::RightToLeft);
    CHECK(marker.effective.directionOrigin == Origin::LogicalRtlMarker);
}

TEST_CASE("Conflicts opaque metadata malformed state and wrong scopes never rewrite source", "[text-profile][inheritance][negative]") {
    const auto defaults = Profile::decode(encoded());
    auto override = entity(); override.direction = Direction::LeftToRight;
    const auto original = Profile::decode(encoded(override));
    const auto conflict = Profile::inherit(defaults, original, "ar", Marker::LegacyReversed);
    CHECK(conflict.status == Profile::InheritanceStatus::LegacyConflict);
    CHECK(conflict.error.code == Code::LegacyOrderConflict);
    CHECK(conflict.effective.selection == openType());
    CHECK(conflict.style.value == defaults.value);
    CHECK(conflict.entity.value == original.value);
    CHECK(conflict.style.raw == defaults.raw);
    CHECK(conflict.entity.raw == original.raw);
    CHECK(*conflict.style.raw == encoded());
    auto future = encoded(override); future[1].value = 2;
    const auto unknown = Profile::decode(future);
    const auto blocked = Profile::inherit(defaults, unknown, "ar");
    CHECK(blocked.status == Profile::InheritanceStatus::Unavailable);
    CHECK(blocked.entity.raw == unknown.raw);
    CHECK_FALSE(blocked.openTypeSelected());
    CHECK(Profile::inherit(original, defaults, "ar").error.code == Code::InvalidScope);
    CHECK(Profile::inherit(defaults, original, "bad_language").status == Profile::InheritanceStatus::Unavailable);
    CHECK(Profile::inherit(defaults, original, "ar", static_cast<Marker>(99)).status == Profile::InheritanceStatus::Unavailable);
    Profile::Decoded absentWithValue; absentWithValue.value = style();
    CHECK(Profile::inherit(absentWithValue, {}, "ar").status == Profile::InheritanceStatus::Unavailable);
    Profile::Decoded absentWithRaw; absentWithRaw.raw = defaults.raw;
    CHECK(Profile::inherit(absentWithRaw, {}, "ar").status == Profile::InheritanceStatus::Unavailable);
    auto knownWithoutValue = defaults; knownWithoutValue.value.reset();
    CHECK(Profile::inherit(knownWithoutValue, {}, "ar").status == Profile::InheritanceStatus::Unavailable);
    auto invalidKnown = defaults; invalidKnown.value->language = "bad_language";
    CHECK(Profile::inherit(invalidKnown, {}, "ar").status == Profile::InheritanceStatus::Unavailable);
    auto invalidStatus = defaults; invalidStatus.status = static_cast<Status>(99);
    CHECK(Profile::inherit(invalidStatus, {}, "ar").status == Profile::InheritanceStatus::Unavailable);
}
