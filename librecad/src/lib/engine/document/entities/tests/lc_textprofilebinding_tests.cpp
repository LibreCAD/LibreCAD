/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textprofilebinding.h"
#include <catch2/catch_test_macros.hpp>
#include <QMetaType>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

struct BindingAliasedPayload {
    std::shared_ptr<QByteArray> bytes;
};
Q_DECLARE_METATYPE(BindingAliasedPayload)

namespace {
using Binding = LC_TextProfileBinding;
using Profile = LC_TextProfile;
using Scope = Profile::Scope;
using Status = Profile::DecodeStatus;
using Code = Profile::ErrorCode;
using Authority = Binding::Authority;

Profile::Value value(Scope scope, bool legacy = false) {
    Profile::Selection selection;
    if (!legacy) selection = {Profile::Backend::HarfBuzzOpenType,
        {QStringLiteral("lc-amiri-1.003-ot-v1"), QByteArray(64, 'a'), 0,
         QStringLiteral("amiri-cap-y647-cell9-baseline-v1"),
         QStringLiteral("hb-ot-default-monotone-characters-v1"), QStringLiteral("HarfBuzz-OT/14.6.0")}};
    return {scope, selection, QByteArray("ar"), Profile::Direction::Auto};
}
Profile::Block encoded(const Profile::Value &input) {
    const auto result = Profile::encode(input);
    REQUIRE(result);
    return result.block;
}
std::size_t field(const Profile::Block &block, const QString &name) {
    for (std::size_t i = 2; i < block.size(); ++i) {
        const auto text = block[i].value.toString();
        if (block[i].code == 1000 && (text == name || text.startsWith(name + '='))) return i;
    }
    FAIL("Expected field " << name.toStdString());
    return 0;
}
void sameModeled(const Profile::Decoded &actual, const Profile::Decoded &expected) {
    CHECK_FALSE(actual.raw);
    CHECK(actual.status == expected.status);
    CHECK(actual.value == expected.value);
    CHECK(actual.error.code == expected.error.code);
    CHECK(actual.error.message == expected.error.message);
    CHECK(actual.error.field == expected.error.field);
}
void untouched(const Binding &binding) {
    CHECK(binding.authority() == Authority::Untouched);
    CHECK_FALSE(binding.replacement());
    CHECK(binding.pendingEncoding() == nullptr);
    sameModeled(binding.current(), binding.imported());
}
void replaced(const Binding &binding, const Profile::Value &expected) {
    CHECK(binding.authority() == Authority::Replace);
    CHECK(binding.replacement() == std::optional<Profile::Value>(expected));
    const auto current = binding.current();
    CHECK(current.known());
    CHECK(current.value == std::optional<Profile::Value>(expected));
    CHECK_FALSE(current.raw);
    CHECK_FALSE(current.error);
    REQUIRE(binding.pendingEncoding());
    CHECK_FALSE(binding.pendingEncoding()->error);
    CHECK(binding.pendingEncoding()->block == encoded(expected));
}
void removed(const Binding &binding) {
    CHECK(binding.authority() == Authority::Remove);
    CHECK_FALSE(binding.replacement());
    CHECK(binding.pendingEncoding() == nullptr);
    sameModeled(binding.current(), Profile::Decoded{});
}
} // namespace

TEST_CASE("Bindings distinguish untouched absent metadata from explicit namespace removal", "[text-profile-binding][headless]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        Binding binding(scope);
        CHECK(binding.scope() == scope);
        CHECK(binding.imported().status == Status::Absent);
        CHECK_FALSE(binding.imported().raw);
        untouched(binding);
        CHECK_FALSE(binding.remove());
        removed(binding);
        CHECK(binding.imported().status == Status::Absent);
        binding.reset();
        untouched(binding);
        Binding empty(scope, Profile::Block{});
        REQUIRE(empty.imported().raw);
        CHECK(empty.imported().raw->empty());
        untouched(empty);
        Binding null(scope, std::shared_ptr<const Profile::Block>{});
        CHECK_FALSE(null.imported().raw);
        untouched(null);
    }
}

TEST_CASE("Import raw provenance stays separate from modeled current metadata", "[text-profile-binding][source]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        const auto initial = value(scope);
        auto block = encoded(initial);
        std::rotate(block.begin() + 2, block.begin() + 3, block.end());
        const auto raw = std::make_shared<const Profile::Block>(block);
        Binding binding(scope, raw);
        const auto snapshotRaw = binding.imported().raw;
        CHECK(snapshotRaw != raw);
        CHECK(*binding.imported().raw == block);
        CHECK(binding.imported().value == std::optional<Profile::Value>(initial));
        untouched(binding);
        auto snapshot = binding.current(); snapshot.value->language = "fa";
        CHECK(binding.current().value->language == QByteArray("ar"));
        CHECK_FALSE(binding.replace(initial)); // Explicit even when semantics match import.
        replaced(binding, initial);
        CHECK(binding.pendingEncoding()->block != block);
        CHECK(binding.imported().raw == snapshotRaw);
        CHECK(*binding.imported().raw == block);
        binding.reset();
        untouched(binding);
        CHECK(binding.imported().raw == snapshotRaw);
        CHECK(*binding.imported().raw == block);
        Binding copiedInput(scope, block);
        block.front().value = QStringLiteral("OtherApplication");
        CHECK(copiedInput.imported().raw->front().value.toString() == Profile::applicationName());
    }
}

TEST_CASE("Shared input aliases cannot mutate the binding's owned import snapshot", "[text-profile-binding][source][alias]") {
    const auto before = encoded(value(Scope::EntityOverride));
    const auto mutableInput = std::make_shared<Profile::Block>(before);
    const std::shared_ptr<const Profile::Block> input = mutableInput;
    Binding binding(Scope::EntityOverride, input);
    const auto snapshot = binding.imported().raw;
    REQUIRE(snapshot);
    CHECK(snapshot != input);
    CHECK(*snapshot == before);
    mutableInput->at(field(*mutableInput, "language")).value = QStringLiteral("language=fa");
    mutableInput->front().value = QStringLiteral("OtherApplication");
    mutableInput->push_back({1001, QStringLiteral("SecondApplication")});
    CHECK(*snapshot == before);
    CHECK(binding.current().value == std::optional<Profile::Value>(value(Scope::EntityOverride)));
    mutableInput->clear();
    CHECK(*snapshot == before);
    CHECK_FALSE(binding.remove());
    CHECK_FALSE(binding.replace(value(Scope::EntityOverride, true)));
    binding.reset();
    untouched(binding);
    CHECK(binding.imported().raw == snapshot);
    CHECK(*snapshot == before);
}

TEST_CASE("Replacement snapshots isolate every retained mutable Qt buffer", "[text-profile-binding][alias-regression]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        auto input = value(scope);
        auto &id = input.selection->identity;
        auto *language = input.language->data();
        auto *profile = id.profileId.data();
        auto *checksum = id.fontSha256.data();
        auto *metrics = id.metricPolicy.data();
        auto *shaping = id.shapingPolicy.data();
        auto *engine = id.engineIdentity.data();
        Binding binding(scope);
        REQUIRE_FALSE(binding.replace(input));
        language[0] = '!';
        profile[0] = '/';
        checksum[0] = 'z';
        metrics[0] = '/';
        shaping[0] = '/';
        engine[0] = '/';
        CHECK(Profile::validate(input));
        REQUIRE(binding.current().value);
        CHECK_FALSE(Profile::validate(*binding.current().value));
        replaced(binding, value(scope));
    }
}

TEST_CASE("Imported snapshots isolate retained QString and QByteArray data pointers", "[text-profile-binding][alias-regression]") {
    for (bool shared : {false, true}) {
        auto input = std::make_shared<Profile::Block>(encoded(value(Scope::EntityOverride)));
        const auto languageIndex = field(*input, "language");
        auto *language = static_cast<QString *>(input->at(languageIndex).value.data())->data();
        Binding known = shared ? Binding(Scope::EntityOverride, std::shared_ptr<const Profile::Block>(input))
                               : Binding(Scope::EntityOverride, *input);
        language[9] = 'f';
        CHECK(known.imported().raw->at(languageIndex).value.toString() == QStringLiteral("language=ar"));
        CHECK(known.current().value == std::optional<Profile::Value>(value(Scope::EntityOverride)));

        auto opaqueInput = std::make_shared<Profile::Block>(encoded(value(Scope::EntityOverride)));
        opaqueInput->at(1).value = 2;
        opaqueInput->push_back({1004, QByteArray("A\0B", 3)});
        auto *bytes = static_cast<QByteArray *>(opaqueInput->back().value.data())->data();
        Binding opaque = shared ? Binding(Scope::EntityOverride, std::shared_ptr<const Profile::Block>(opaqueInput))
                                : Binding(Scope::EntityOverride, *opaqueInput);
        bytes[0] = 'Z';
        CHECK(opaque.imported().status == Status::Opaque);
        CHECK(opaque.imported().raw->back().value.metaType().id() == QMetaType::QByteArray);
        CHECK(opaque.imported().raw->back().value.toByteArray() == QByteArray("A\0B", 3));
    }
}

TEST_CASE("Import owning domain rejects unsupported Qt payloads explicitly without loss", "[text-profile-binding][ownership][negative]") {
    static_assert(noexcept(Binding::supportsImport(std::declval<const Profile::Block &>())));
    const auto customBytes = std::make_shared<QByteArray>("opaque");
    const std::vector<QVariant> unsupported{
        QVariantList{1, 2}, QVariantMap{{QStringLiteral("key"), QByteArray("bytes")}},
        QStringList{QStringLiteral("text")}, QVariant::fromValue(short(7)),
        QVariant::fromValue(1.0f), QVariant::fromValue(static_cast<void *>(customBytes.get())),
        QVariant::fromValue(BindingAliasedPayload{customBytes})};
    for (const auto &payload : unsupported) {
        auto raw = std::make_shared<Profile::Block>(encoded(value(Scope::EntityOverride)));
        raw->at(1).value = 2;
        raw->push_back({1004, payload});
        const auto before = raw->back().value;
        CHECK_FALSE(Binding::supportsImport(*raw));
        CHECK_THROWS_AS(Binding(Scope::EntityOverride, *raw), std::invalid_argument);
        CHECK_THROWS_AS(Binding(Scope::EntityOverride, std::shared_ptr<const Profile::Block>(raw)), std::invalid_argument);
        auto movedBlock = *raw;
        CHECK_THROWS_AS(Binding(Scope::EntityOverride, std::move(movedBlock)), std::invalid_argument);
        REQUIRE(movedBlock.size() == raw->size());
        CHECK(movedBlock.front() == raw->front());
        CHECK(movedBlock.back().value.metaType() == before.metaType());
        const std::shared_ptr<const Profile::Block> constRaw = raw;
        auto movedPointer = constRaw;
        CHECK_THROWS_AS(Binding(Scope::EntityOverride, std::move(movedPointer)), std::invalid_argument);
        CHECK(movedPointer == constRaw);
        CHECK(raw->back().code == 1004);
        CHECK(raw->back().value.metaType() == before.metaType());
        if (before.metaType() == QMetaType::fromType<BindingAliasedPayload>()) {
            CHECK(raw->back().value.value<BindingAliasedPayload>().bytes == customBytes);
            CHECK(*customBytes == QByteArray("opaque"));
        } else CHECK(raw->back().value == before);
        // The low-level codec can still retain exact caller-owned raw. The
        // binding domain never coerces, drops or pretends to clone this value.
        const auto decoded = Profile::decode(std::shared_ptr<const Profile::Block>(raw));
        CHECK(decoded.status == Status::Invalid);
        CHECK(decoded.raw == raw);
        CHECK(decoded.error.code == Code::InvalidType);
    }
    CHECK(Binding::supportsImport({}));
}

TEST_CASE("Owned raw preserves supported exact types nullness and floating payload storage", "[text-profile-binding][ownership][source]") {
    double nan;
    const std::uint64_t nanBits = UINT64_C(0x7ff8000000000042);
    std::memcpy(&nan, &nanBits, sizeof(nan));
    const std::vector<QVariant> payloads{
        QVariant{}, QVariant(QMetaType::fromType<int>()),
        QVariant(QMetaType::fromType<QString>()), QVariant(QMetaType::fromType<QByteArray>()),
        QVariant(QMetaType::fromType<uint>()), QVariant(QMetaType::fromType<qlonglong>()),
        QVariant(QMetaType::fromType<qulonglong>()), QVariant(QMetaType::fromType<double>()),
        QVariant(QMetaType::fromType<bool>()),
        QVariant(QString{}), QVariant(QStringLiteral("")),
        QVariant(QByteArray{}), QVariant(QByteArray("")),
        QVariant(QString::fromUtf16(u"a\0\U0001f600", 4)), QVariant(QString(1, QChar(0xdc00))),
        QVariant(QByteArray("A\0B", 3)),
        QVariant(-7), QVariant(7u), QVariant::fromValue(qlonglong(-12345678901)),
        QVariant::fromValue(qulonglong(12345678901)), QVariant(true),
        QVariant(nan), QVariant(-0.0)};
    for (const auto &payload : payloads) {
        auto block = encoded(value(Scope::EntityOverride));
        block[1].value = 2;
        block.push_back({1004, payload});
        REQUIRE(Binding::supportsImport(block));
        Binding binding(Scope::EntityOverride, block);
        const auto &snapshot = binding.imported().raw->back().value;
        CHECK(snapshot.metaType() == payload.metaType());
        CHECK(snapshot.isNull() == payload.isNull());
        if (payload.metaType().id() == QMetaType::QString) {
            CHECK(snapshot.toString() == payload.toString());
            CHECK(snapshot.toString().isNull() == payload.toString().isNull());
        } else if (payload.metaType().id() == QMetaType::QByteArray) {
            CHECK(snapshot.toByteArray() == payload.toByteArray());
            CHECK(snapshot.toByteArray().isNull() == payload.toByteArray().isNull());
        } else if (payload.isValid()) {
            REQUIRE(snapshot.constData());
            REQUIRE(payload.constData());
            CHECK(std::memcmp(snapshot.constData(), payload.constData(), std::size_t(payload.metaType().sizeOf())) == 0);
        }
        CHECK(binding.imported().status != Status::Known);
        CHECK_FALSE(binding.current().value);
    }
    auto block = encoded(value(Scope::EntityOverride));
    block[1].value = 2;
    block.push_back({1040, nan});
    auto *retained = static_cast<double *>(block.back().value.data());
    Binding binding(Scope::EntityOverride, block);
    *retained = 17.0;
    CHECK(std::memcmp(binding.imported().raw->back().value.constData(), &nanBits, sizeof(nanBits)) == 0);
}

TEST_CASE("Snapshots own external raw buffers and survive input lifetime end", "[text-profile-binding][ownership][source]") {
    QChar text[]{'f', 'u', 't', 'u', 'r', 'e', '=', 'a'};
    char bytes[]{'A', '\0', 'B'};
    auto block = encoded(value(Scope::EntityOverride));
    block[1].value = 2;
    block.push_back({1000, QString::fromRawData(text, 8)});
    block.push_back({1004, QByteArray::fromRawData(bytes, 3)});
    Binding binding(Scope::EntityOverride, block);
    block.clear();
    text[7] = 'z';
    bytes[0] = 'Z';
    const auto raw = binding.imported().raw;
    CHECK(raw->at(raw->size() - 2).value.toString() == QStringLiteral("future=a"));
    CHECK(raw->back().value.toByteArray() == QByteArray("A\0B", 3));
    CHECK(binding.imported().status == Status::Opaque);
    REQUIRE_FALSE(binding.replace(value(Scope::EntityOverride)));
    Binding moved(std::move(binding));
    CHECK(moved.imported().raw == raw);
    CHECK(binding.imported().raw == raw);
    untouched(binding);
}

TEST_CASE("Replace remove and reset form explicit repeatable edit transitions", "[text-profile-binding][authority]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        for (bool imported : {false, true}) {
            const auto initial = value(scope);
            Binding binding(scope, imported ? encoded(initial) : Profile::Block{});
            const auto raw = binding.imported().raw;
            const auto decoded = binding.imported();
            for (bool legacy : {false, true}) {
                auto edit = value(scope, legacy); edit.language = "fa-IR"; edit.direction = Profile::Direction::RightToLeft;
                CHECK_FALSE(binding.replace(edit));
                replaced(binding, edit);
                const auto encoding = binding.pendingEncoding();
                for (int read = 0; read < 3; ++read) {
                    sameModeled(binding.current(), binding.current());
                    CHECK(binding.pendingEncoding() == encoding);
                    CHECK(binding.authority() == Authority::Replace);
                }
                CHECK(binding.imported().raw == raw);
                CHECK(binding.imported().value == decoded.value);
                CHECK_FALSE(binding.replace(edit));
                replaced(binding, edit);
                CHECK_FALSE(binding.remove());
                removed(binding);
                for (int read = 0; read < 3; ++read) {
                    sameModeled(binding.current(), Profile::Decoded{});
                    CHECK(binding.pendingEncoding() == nullptr);
                    CHECK(binding.authority() == Authority::Remove);
                }
                CHECK_FALSE(binding.remove());
                removed(binding);
                CHECK(binding.imported().raw == raw);
                binding.reset();
                untouched(binding);
                binding.reset();
                untouched(binding);
                CHECK(binding.imported().value == decoded.value);
                CHECK_FALSE(binding.replace(edit));
                replaced(binding, edit);
                binding.reset();
                untouched(binding);
            }
        }
    }
    Binding entity(Scope::EntityOverride);
    Profile::Value inherited{Scope::EntityOverride, {}, {}, {}};
    CHECK_FALSE(entity.replace(inherited));
    replaced(entity, inherited);
    CHECK(entity.pendingEncoding()->block.size() == 3); // Scope-only is NOT namespace removal.
    CHECK_FALSE(entity.remove());
    removed(entity);
}

TEST_CASE("Unknown and malformed imports remain retained inactive until an explicit edit", "[text-profile-binding][negative][source]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        std::vector<Profile::Block> inputs;
        auto unknownSchema = encoded(value(scope)); unknownSchema[1].value = 2; inputs.push_back(unknownSchema);
        auto unknownBackend = encoded(value(scope)); unknownBackend[field(unknownBackend, "backend")].value = QStringLiteral("backend=future"); inputs.push_back(unknownBackend);
        auto unknownField = encoded(value(scope)); unknownField.push_back({1000, QStringLiteral("future=opaque")}); inputs.push_back(unknownField);
        auto duplicate = encoded(value(scope)); duplicate.push_back({1000, QStringLiteral("language=fa")}); inputs.push_back(duplicate);
        auto malformed = encoded(value(scope)); malformed[1].value = QStringLiteral("1"); inputs.push_back(malformed);
        auto wrongNamespace = encoded(value(scope)); wrongNamespace.front().value = QStringLiteral("OtherApplication"); inputs.push_back(wrongNamespace);
        auto badType = encoded(value(scope)); badType[field(badType, "language")].value = QByteArray("not a QString"); inputs.push_back(badType);
        auto overLimit = encoded(value(scope)); overLimit.push_back({1000, QString(1024 * 1024, 'x')}); inputs.push_back(overLimit);
        for (const auto &block : inputs) {
            const auto raw = std::make_shared<const Profile::Block>(block);
            Binding binding(scope, raw);
            const auto initial = binding.imported();
            CHECK((initial.status == Status::Opaque || initial.status == Status::Invalid));
            CHECK_FALSE(initial.value);
            CHECK(initial.error);
            CHECK(initial.raw != raw);
            CHECK(*initial.raw == block);
            untouched(binding);
            CHECK_FALSE(binding.replace(value(scope)));
            replaced(binding, value(scope));
            CHECK(binding.imported().raw == initial.raw);
            CHECK(binding.imported().error.code == initial.error.code);
            CHECK_FALSE(binding.remove());
            removed(binding);
            CHECK(binding.imported().raw == initial.raw);
            binding.reset();
            untouched(binding);
            CHECK(binding.imported().raw == initial.raw);
            CHECK(*binding.imported().raw == block);
        }
    }
}

TEST_CASE("Known imports in the wrong locked scope become invalid without losing raw data", "[text-profile-binding][scope]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        const auto other = scope == Scope::StyleDefaults ? Scope::EntityOverride : Scope::StyleDefaults;
        const auto raw = std::make_shared<const Profile::Block>(encoded(value(other)));
        Binding binding(scope, raw);
        const auto snapshotRaw = binding.imported().raw;
        CHECK(binding.scope() == scope);
        CHECK(binding.imported().status == Status::Invalid);
        CHECK(binding.imported().error.code == Code::InvalidScope);
        CHECK(binding.imported().error.field == QStringLiteral("scope"));
        CHECK_FALSE(binding.imported().value);
        CHECK(snapshotRaw != raw);
        CHECK(*snapshotRaw == *raw);
        untouched(binding);
        CHECK_FALSE(binding.replace(value(scope)));
        replaced(binding, value(scope));
        CHECK(binding.imported().status == Status::Invalid);
        CHECK(binding.imported().raw == snapshotRaw);
        binding.reset();
        untouched(binding);
        CHECK_FALSE(binding.remove());
        removed(binding);
        CHECK(binding.imported().raw == snapshotRaw);
    }
}

TEST_CASE("Failed replacements preserve every previous authority and provenance", "[text-profile-binding][atomic][negative]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        std::vector<Profile::Value> invalid;
        auto wrongScope = value(scope == Scope::StyleDefaults ? Scope::EntityOverride : Scope::StyleDefaults); invalid.push_back(wrongScope);
        auto badLanguage = value(scope); badLanguage.language = "not_valid"; invalid.push_back(badLanguage);
        auto badFace = value(scope); badFace.selection->identity.faceIndex = 32768; invalid.push_back(badFace);
        auto badHash = value(scope); badHash.selection->identity.fontSha256 = QByteArray(64, 'A'); invalid.push_back(badHash);
        auto badEngine = value(scope); badEngine.selection->identity.engineIdentity = "/tmp/font"; invalid.push_back(badEngine);
        auto badBackend = value(scope); badBackend.selection->backend = static_cast<Profile::Backend>(99); invalid.push_back(badBackend);
        auto badDirection = value(scope); badDirection.direction = static_cast<Profile::Direction>(99); invalid.push_back(badDirection);
        auto badScope = value(scope); badScope.scope = static_cast<Scope>(99); invalid.push_back(badScope);
        if (scope == Scope::StyleDefaults) {
            auto missing = value(scope); missing.selection.reset(); invalid.push_back(missing);
            auto recursive = value(scope); recursive.direction = Profile::Direction::ByStyle; invalid.push_back(recursive);
        }
        for (const auto authority : {Authority::Untouched, Authority::Replace, Authority::Remove}) {
            Binding binding(scope, encoded(value(scope)));
            auto pending = value(scope, true); pending.language = "ur";
            if (authority == Authority::Replace) REQUIRE_FALSE(binding.replace(pending));
            if (authority == Authority::Remove) REQUIRE_FALSE(binding.remove());
            const auto imported = binding.imported();
            const auto current = binding.current();
            const auto replacement = binding.replacement();
            const auto encoding = binding.pendingEncoding();
            for (const auto &bad : invalid) {
                CHECK(binding.replace(bad));
                CHECK(binding.scope() == scope);
                CHECK(binding.authority() == authority);
                CHECK(binding.imported().raw == imported.raw);
                CHECK(binding.imported().value == imported.value);
                CHECK(binding.replacement() == replacement);
                CHECK(binding.pendingEncoding() == encoding);
                sameModeled(binding.current(), current);
                if (encoding) CHECK(binding.pendingEncoding()->block == encoded(pending));
            }
        }
    }
}

TEST_CASE("Replacement accepts an internal value alias without changing imported provenance", "[text-profile-binding][atomic][alias]") {
    Binding binding(Scope::EntityOverride, encoded(value(Scope::EntityOverride)));
    const auto raw = binding.imported().raw;
    REQUIRE_FALSE(binding.replace(*binding.imported().value));
    replaced(binding, value(Scope::EntityOverride));
    auto edit = value(Scope::EntityOverride, true); edit.language = "ur";
    REQUIRE_FALSE(binding.replace(edit));
    REQUIRE(binding.replacement());
    REQUIRE_FALSE(binding.replace(*binding.replacement()));
    replaced(binding, edit);
    CHECK(binding.imported().raw == raw);
    CHECK(binding.imported().value == std::optional<Profile::Value>(value(Scope::EntityOverride)));
}

TEST_CASE("Invalid scope construction stays invalid and edits cannot normalize it", "[text-profile-binding][scope][negative]") {
    for (bool withRaw : {false, true}) {
        const auto raw = withRaw ? std::make_shared<const Profile::Block>(encoded(value(Scope::EntityOverride))) : nullptr;
        Binding binding(static_cast<Scope>(99), raw);
        const auto snapshotRaw = binding.imported().raw;
        if (withRaw) {
            CHECK(snapshotRaw != raw);
            CHECK(*snapshotRaw == *raw);
        } else CHECK_FALSE(snapshotRaw);
        CHECK(binding.imported().status == Status::Invalid);
        CHECK(binding.imported().error.code == Code::InvalidScope);
        untouched(binding);
        CHECK(binding.replace(value(Scope::EntityOverride)).code == Code::InvalidScope);
        CHECK(binding.replace(value(Scope::StyleDefaults)).code == Code::InvalidScope);
        CHECK(binding.remove().code == Code::InvalidScope);
        binding.reset();
        untouched(binding);
        CHECK(binding.imported().raw == snapshotRaw);
    }
    Binding absent(static_cast<Scope>(-1));
    CHECK(absent.current().status == Status::Invalid);
    CHECK(absent.current().error.code == Code::InvalidScope);
    CHECK_FALSE(absent.imported().raw);
}

TEST_CASE("Copy move and whole value assignment preserve independent edits and raw snapshots", "[text-profile-binding][value]") {
    static_assert(std::is_copy_constructible_v<Binding> && std::is_move_constructible_v<Binding>);
    static_assert(std::is_copy_assignable_v<Binding> && std::is_move_assignable_v<Binding>);
    Binding original(Scope::StyleDefaults, encoded(value(Scope::StyleDefaults)));
    auto edit = value(Scope::StyleDefaults, true); edit.language = "fa";
    REQUIRE_FALSE(original.replace(edit));
    Binding copy(original);
    CHECK(copy.imported().raw == original.imported().raw);
    replaced(copy, edit);
    CHECK_FALSE(copy.remove());
    removed(copy);
    replaced(original, edit);
    Binding moved(std::move(copy));
    removed(moved);
    CHECK(moved.imported().raw == original.imported().raw);
    Binding assigned(Scope::EntityOverride);
    assigned = original;
    CHECK(assigned.scope() == Scope::StyleDefaults); // Whole binding, NOT an in-place edit.
    CHECK(assigned.imported().raw == original.imported().raw);
    replaced(assigned, edit);
    assigned.reset();
    untouched(assigned);
    replaced(original, edit);
    Binding moveAssigned(Scope::EntityOverride);
    moveAssigned = std::move(moved);
    CHECK(moveAssigned.scope() == Scope::StyleDefaults);
    CHECK(moveAssigned.imported().raw == original.imported().raw);
    removed(moveAssigned);
    CHECK_FALSE(moveAssigned.replace(value(Scope::StyleDefaults)));
    replaced(moveAssigned, value(Scope::StyleDefaults));
    replaced(original, edit);
    auto detached = moveAssigned.current(); detached.value->selection->identity.profileId = "different";
    CHECK(moveAssigned.current().value->selection->identity.profileId != QStringLiteral("different"));
}

TEST_CASE("Whole value assignment transfers each authority without conflating scope or reset", "[text-profile-binding][value][scope]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        for (const auto authority : {Authority::Untouched, Authority::Replace, Authority::Remove}) {
            auto block = encoded(value(scope)); block[1].value = 2;
            Binding source(scope, block);
            if (authority == Authority::Replace) REQUIRE_FALSE(source.replace(value(scope)));
            if (authority == Authority::Remove) REQUIRE_FALSE(source.remove());
            const auto other = scope == Scope::StyleDefaults ? Scope::EntityOverride : Scope::StyleDefaults;
            Binding target(other, encoded(value(other)));
            target = source;
            CHECK(target.scope() == scope);
            CHECK(target.authority() == authority);
            CHECK(target.imported().raw == source.imported().raw);
            CHECK(target.imported().status == Status::Opaque);
            sameModeled(target.current(), source.current());
            target.reset();
            untouched(target);
            CHECK(source.authority() == authority);
            CHECK(target.imported().raw == source.imported().raw);
        }
    }
    Binding invalid(static_cast<Scope>(99));
    Binding valid(Scope::EntityOverride);
    invalid = valid;
    CHECK(invalid.scope() == Scope::EntityOverride);
    CHECK_FALSE(invalid.replace(value(Scope::EntityOverride)));
    valid = Binding(static_cast<Scope>(99));
    CHECK(valid.current().error.code == Code::InvalidScope);
    CHECK(valid.replace(value(Scope::EntityOverride)).code == Code::InvalidScope);
}

TEST_CASE("Move leaves a replacement source coherent and its imported provenance intact", "[text-profile-binding][move-regression]") {
    Binding source(Scope::EntityOverride, encoded(value(Scope::EntityOverride)));
    const auto raw = source.imported().raw;
    auto edit = value(Scope::EntityOverride, true); edit.language = "ur";
    REQUIRE_FALSE(source.replace(edit));
    SECTION("construction") {
        Binding destination(std::move(source));
        replaced(destination, edit);
        CHECK(source.authority() == Authority::Untouched);
        CHECK(source.pendingEncoding() == nullptr);
        CHECK(source.imported().raw == raw);
        untouched(source);
        REQUIRE(source.current().value);
        CHECK_FALSE(Profile::validate(*source.current().value));
    }
    SECTION("assignment") {
        Binding destination(Scope::StyleDefaults, encoded(value(Scope::StyleDefaults)));
        destination = std::move(source);
        replaced(destination, edit);
        CHECK(source.authority() == Authority::Untouched);
        CHECK(source.pendingEncoding() == nullptr);
        CHECK(source.imported().raw == raw);
        untouched(source);
        REQUIRE(source.current().value);
        CHECK_FALSE(Profile::validate(*source.current().value));
    }
}

TEST_CASE("Every moved import and edit authority retains a reusable coherent source", "[text-profile-binding][move][source]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        for (int importKind = 0; importKind < 5; ++importKind) {
            for (const auto authority : {Authority::Untouched, Authority::Replace, Authority::Remove}) {
                for (bool assign : {false, true}) {
                    Profile::Block block;
                    if (importKind) block = encoded(value(scope));
                    if (importKind == 2) block[1].value = 2;
                    if (importKind == 3) block[1].value = QStringLiteral("invalid");
                    if (importKind == 4) block = encoded(value(scope == Scope::StyleDefaults ? Scope::EntityOverride : Scope::StyleDefaults));
                    Binding source(scope, block);
                    const auto imported = source.imported();
                    auto edit = value(scope, true); edit.language = "fa";
                    if (authority == Authority::Replace) REQUIRE_FALSE(source.replace(edit));
                    if (authority == Authority::Remove) REQUIRE_FALSE(source.remove());
                    const auto before = source.current();
                    const auto otherScope = scope == Scope::StyleDefaults ? Scope::EntityOverride : Scope::StyleDefaults;
                    const auto moved = [&]() {
                        if (!assign) return Binding(std::move(source));
                        Binding target(otherScope, encoded(value(otherScope)));
                        REQUIRE_FALSE(target.replace(value(otherScope, true)));
                        target = std::move(source);
                        return target;
                    }();
                    CHECK(moved.scope() == scope);
                    CHECK(moved.authority() == authority);
                    CHECK(moved.imported().raw == imported.raw);
                    sameModeled(moved.current(), before);
                    if (authority == Authority::Replace) replaced(moved, edit);
                    if (authority == Authority::Remove) removed(moved);
                    if (authority == Authority::Untouched) untouched(moved);
                    CHECK(source.scope() == scope);
                    CHECK(source.imported().raw == imported.raw);
                    CHECK(source.imported().value == imported.value);
                    CHECK(source.imported().error.code == imported.error.code);
                    untouched(source);
                    // Reuse the moved-from source through every in-place edit.
                    REQUIRE_FALSE(source.replace(value(scope)));
                    replaced(source, value(scope));
                    REQUIRE_FALSE(source.remove());
                    removed(source);
                    source.reset();
                    untouched(source);
                    CHECK(source.imported().raw == imported.raw);
                }
            }
        }
    }
    Binding invalid(static_cast<Scope>(99), encoded(value(Scope::EntityOverride)));
    const auto raw = invalid.imported().raw;
    Binding moved(std::move(invalid));
    CHECK(moved.current().error.code == Code::InvalidScope);
    untouched(invalid);
    CHECK(invalid.imported().raw == raw);
    CHECK(invalid.remove().code == Code::InvalidScope);
    CHECK(invalid.replace(value(Scope::EntityOverride)).code == Code::InvalidScope);
}

TEST_CASE("Self move assignment leaves every authority and provenance unchanged", "[text-profile-binding][move][alias]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        for (bool unknown : {false, true}) {
            for (const auto authority : {Authority::Untouched, Authority::Replace, Authority::Remove}) {
                auto block = encoded(value(scope));
                if (unknown) block[1].value = 2;
                Binding binding(scope, block);
                auto edit = value(scope, true); edit.language = "ur";
                if (authority == Authority::Replace) REQUIRE_FALSE(binding.replace(edit));
                if (authority == Authority::Remove) REQUIRE_FALSE(binding.remove());
                const auto raw = binding.imported().raw;
                const auto before = binding.current();
                const auto encoding = binding.pendingEncoding();
                auto *same = &binding;
                binding = std::move(*same);
                CHECK(binding.scope() == scope);
                CHECK(binding.authority() == authority);
                CHECK(binding.imported().raw == raw);
                CHECK(binding.pendingEncoding() == encoding);
                sameModeled(binding.current(), before);
                if (authority == Authority::Replace) replaced(binding, edit);
                if (authority == Authority::Remove) removed(binding);
                if (authority == Authority::Untouched) untouched(binding);
            }
        }
    }
}

TEST_CASE("Self copy assignment preserves every field and pending pointer", "[text-profile-binding][copy][alias]") {
    for (const auto scope : {Scope::StyleDefaults, Scope::EntityOverride}) {
        for (bool unknown : {false, true}) {
            for (const auto authority : {Authority::Untouched, Authority::Replace, Authority::Remove}) {
                auto block = encoded(value(scope));
                if (unknown) block[1].value = 2;
                Binding binding(scope, block);
                auto edit = value(scope, true); edit.language = "fa";
                if (authority == Authority::Replace) REQUIRE_FALSE(binding.replace(edit));
                if (authority == Authority::Remove) REQUIRE_FALSE(binding.remove());
                const auto raw = binding.imported().raw;
                const auto before = binding.current();
                const auto encoding = binding.pendingEncoding();
                const auto *same = &binding;
                CHECK(&(binding = *same) == &binding);
                CHECK(binding.scope() == scope);
                CHECK(binding.authority() == authority);
                CHECK(binding.imported().raw == raw);
                CHECK(binding.pendingEncoding() == encoding);
                sameModeled(binding.current(), before);
                if (authority == Authority::Replace) replaced(binding, edit);
                if (authority == Authority::Remove) removed(binding);
                if (authority == Authority::Untouched) untouched(binding);
            }
        }
    }
}

TEST_CASE("Current metadata supports inheritance without treating provenance as edit authority", "[text-profile-binding][inheritance]") {
    Binding defaults(Scope::StyleDefaults, encoded(value(Scope::StyleDefaults)));
    auto unknown = encoded(value(Scope::EntityOverride)); unknown[1].value = 2;
    Binding entity(Scope::EntityOverride, unknown);
    const auto blocked = Profile::inherit(defaults.current(), entity.current(), "ar");
    CHECK(blocked.status == Profile::InheritanceStatus::Unavailable);
    CHECK_FALSE(blocked.style.raw);
    CHECK_FALSE(blocked.entity.raw);
    CHECK_FALSE(entity.remove());
    const auto inherited = Profile::inherit(defaults.current(), entity.current(), "ar");
    CHECK(inherited.openTypeSelected());
    CHECK(entity.imported().status == Status::Opaque);
    CHECK(*entity.imported().raw == unknown);
    Profile::Value direction{Scope::EntityOverride, {}, {}, Profile::Direction::LeftToRight};
    CHECK_FALSE(entity.replace(direction));
    const auto logicalRtl = Profile::inherit(defaults.current(), entity.current(), "ar", Profile::RtlMarker::LogicalRtl);
    CHECK(logicalRtl.effective.direction == Profile::Direction::LeftToRight);
    const auto legacy = Profile::inherit(defaults.current(), entity.current(), "ar", Profile::RtlMarker::LegacyReversed);
    CHECK(legacy.status == Profile::InheritanceStatus::LegacyConflict);
    CHECK(legacy.effective.selection.backend == Profile::Backend::HarfBuzzOpenType);
    CHECK(*entity.imported().raw == unknown);
    entity.reset();
    CHECK(Profile::inherit(defaults.current(), entity.current(), "ar").status == Profile::InheritanceStatus::Unavailable);
}

TEST_CASE("Opaque floating payloads retain exact storage without numeric raw equality", "[text-profile-binding][source][opaque]") {
    auto block = encoded(value(Scope::EntityOverride)); block[1].value = 2;
    block.push_back({1040, std::numeric_limits<double>::quiet_NaN()});
    block.push_back({1040, -0.0});
    const auto raw = std::make_shared<const Profile::Block>(std::move(block));
    Binding binding(Scope::EntityOverride, raw);
    const auto snapshotRaw = binding.imported().raw;
    CHECK(binding.imported().status == Status::Opaque);
    CHECK(snapshotRaw != raw);
    CHECK(std::isnan(binding.imported().raw->at(raw->size() - 2).value.toDouble()));
    CHECK(std::signbit(binding.imported().raw->back().value.toDouble()));
    CHECK(std::memcmp(snapshotRaw->at(raw->size() - 2).value.constData(), raw->at(raw->size() - 2).value.constData(), sizeof(double)) == 0);
    CHECK_FALSE(binding.replace(value(Scope::EntityOverride)));
    CHECK(binding.imported().raw == snapshotRaw);
    CHECK(std::memcmp(binding.imported().raw->back().value.constData(), raw->back().value.constData(), sizeof(double)) == 0);
    CHECK_FALSE(binding.remove());
    binding.reset();
    CHECK(binding.imported().raw == snapshotRaw);
    CHECK(std::isnan(binding.imported().raw->at(raw->size() - 2).value.toDouble()));
    CHECK(std::signbit(binding.imported().raw->back().value.toDouble()));
    CHECK_FALSE(binding.current().raw);
}
