/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textstylelist.h"
#include <catch2/catch_test_macros.hpp>
#include <QDate>
#include <memory>

namespace {
using Profile = LC_TextProfile;
using Binding = LC_TextProfileBinding;
using Transport = LC_TextEntityState::ProfileTransport;
Profile::Value defaults() {
    return {Profile::Scope::StyleDefaults, Profile::Selection{}, QByteArray("ar"), Profile::Direction::Auto};
}
std::unique_ptr<LC_TextStyle> style(const QString &name) {
    auto result = std::make_unique<LC_TextStyle>();
    result->setName(name);
    return result;
}
}

TEST_CASE("STYLE defaults retain native scalar semantics", "[text-style-authority][headless]") {
    LC_TextStyle value;
    CHECK(value.getFixedTextHeight() == 0.0);
    CHECK(value.getWidthFactor() == 1.0);
    CHECK(value.getObliqueAngle() == 0.0);
    CHECK(value.getGenFlag() == 0);
    CHECK(value.getLastHeight() == 1.0);
    CHECK(value.getFontName() == QStringLiteral("txt"));
    CHECK(value.getBigFont().isEmpty());
    CHECK(value.getFontFamilyItalicBold() == 0);
    CHECK(value.getFlags() == 0);
    CHECK_FALSE(value.importedName());
    CHECK_FALSE(value.importedNameUtf8());
    REQUIRE(value.getCurrentTextProfile().available());
    REQUIRE(value.getCurrentTextProfile().metadata);
    CHECK(value.getCurrentTextProfile().metadata->status == Profile::DecodeStatus::Absent);
}

TEST_CASE("Copied STYLE bindings retain unavailable history across explicit edits",
          "[text-style-authority][ownership][headless]") {
    for (const auto transport : {Transport::DuplicateNamespace, Transport::UnrepresentableNamespace}) {
        LC_TextStyle original;
        original.setName(QStringLiteral("Original"));
        const auto diagnostic = QStringLiteral("original namespace retained externally");
        REQUIRE_FALSE(original.importTextProfile(std::nullopt, transport, diagnostic, QByteArray("Original")));
        for (const bool remove : {false, true}) {
            LC_TextStyle copied(original);
            LC_TextStyle assigned;
            assigned = original;
            for (LC_TextStyle *value : {&copied, &assigned}) {
                CHECK(value->getCurrentTextProfile().transport == transport);
                CHECK(value->getCurrentTextProfile().diagnostic == diagnostic);
                REQUIRE(value->importedName());
                REQUIRE(value->importedNameUtf8());
                CHECK(*value->importedName() == QStringLiteral("Original"));
                CHECK(*value->importedNameUtf8() == QByteArray("Original"));
                if (remove) REQUIRE_FALSE(value->removeTextProfile());
                else REQUIRE_FALSE(value->replaceTextProfile(defaults()));
                REQUIRE(value->getCurrentTextProfile().available());
                CHECK(value->getTextProfileDiagnostic().isEmpty());
                value->setName(QStringLiteral("Edited"));
                value->resetTextProfile();
                CHECK_FALSE(value->getCurrentTextProfile().available());
                CHECK(value->getCurrentTextProfile().transport == transport);
                CHECK(value->getCurrentTextProfile().diagnostic == diagnostic);
                CHECK(value->getTextProfileDiagnostic() == diagnostic);
                CHECK(*value->importedName() == QStringLiteral("Original"));
                CHECK(*value->importedNameUtf8() == QByteArray("Original"));
                CHECK(value->getName() == QStringLiteral("Edited"));
            }
            CHECK_FALSE(original.getCurrentTextProfile().available());
            CHECK(original.getCurrentTextProfile().diagnostic == diagnostic);
            CHECK(original.getName() == QStringLiteral("Original"));
        }
    }
}

TEST_CASE("STYLE profile import distinguishes unavailable raw from absent defaults", "[text-style-authority][headless]") {
    LC_TextStyle absent;
    absent.setName(QStringLiteral("ABSENT"));
    REQUIRE_FALSE(absent.importTextProfile(std::nullopt));
    REQUIRE(absent.getCurrentTextProfile().available());
    CHECK(absent.getCurrentTextProfile().metadata->status == Profile::DecodeStatus::Absent);
    for (const auto transport : {Transport::DuplicateNamespace, Transport::UnrepresentableNamespace}) {
        LC_TextStyle imported;
        imported.setName(QStringLiteral("SCOPED"));
        REQUIRE_FALSE(imported.importTextProfile(std::nullopt, transport, QStringLiteral("raw retained externally")));
        const auto before = imported.getCurrentTextProfile();
        CHECK_FALSE(before.available());
        CHECK_FALSE(before.metadata);
        CHECK(before.transport == transport);
        CHECK(before.diagnostic == QStringLiteral("raw retained externally"));
        CHECK(imported.getTextProfile().imported().status == Profile::DecodeStatus::Absent);
        REQUIRE_FALSE(imported.replaceTextProfile(defaults()));
        CHECK(imported.getCurrentTextProfile().available());
        CHECK(imported.getCurrentTextProfile().metadata->known());
        imported.resetTextProfile();
        CHECK_FALSE(imported.getCurrentTextProfile().available());
        REQUIRE_FALSE(imported.removeTextProfile());
        REQUIRE(imported.getCurrentTextProfile().available());
        CHECK(imported.getCurrentTextProfile().metadata->status == Profile::DecodeStatus::Absent);
        imported.resetTextProfile();
        CHECK(imported.getCurrentTextProfile().transport == transport);
        CHECK(imported.importTextProfile(std::nullopt));
    }
}

TEST_CASE("STYLE raw provenance stays inert and scoped through replacement and copying", "[text-style-authority][headless]") {
    auto known = Profile::encode(defaults());
    REQUIRE(known);
    for (const auto mode : {0, 1, 2, 3}) {
        auto block = known.block;
        if (mode == 1) block[1].value = 999;
        if (mode == 2) block.push_back({1000, QStringLiteral("future=value")});
        if (mode == 3) {
            auto wrong = defaults(); wrong.scope = Profile::Scope::EntityOverride;
            block = Profile::encode(wrong).block;
        }
        LC_TextStyle imported;
        imported.setName(QStringLiteral("Original"));
        REQUIRE_FALSE(imported.importTextProfile(block));
        const auto raw = imported.getTextProfile().imported().raw;
        REQUIRE(raw);
        CHECK(*raw == block);
        CHECK(imported.getTextProfile().scope() == Profile::Scope::StyleDefaults);
        CHECK(imported.getCurrentTextProfile().available());
        CHECK(imported.getCurrentTextProfile().metadata->known() == (mode == 0));
        LC_TextStyle copied(imported);
        CHECK(copied.getTextProfile().imported().raw == raw);
        REQUIRE_FALSE(copied.replaceTextProfile(defaults()));
        CHECK(imported.getTextProfile().authority() == Binding::Authority::Untouched);
        copied.setName(QStringLiteral("Edited"));
        CHECK(*copied.importedName() == QStringLiteral("Original"));
        CHECK(imported.getName() == QStringLiteral("Original"));
        auto wrong = defaults(); wrong.scope = Profile::Scope::EntityOverride;
        CHECK(copied.replaceTextProfile(wrong));
        CHECK(copied.getTextProfile().authority() == Binding::Authority::Replace);
        copied.resetTextProfile();
        CHECK(copied.getTextProfile().imported().raw == raw);
    }
}

TEST_CASE("STYLE accepts only safe imported transport states and owns name buffers", "[text-style-authority][headless]") {
    LC_TextStyle value;
    QString name = QStringLiteral("name"), diagnostic = QStringLiteral("held diagnostic");
    auto *nameData = name.data(); auto *diagnosticData = diagnostic.data();
    value.setName(name);
    nameData[0] = QChar('X');
    CHECK(value.getName() == QStringLiteral("name"));
    auto block = Profile::encode(defaults()).block;
    CHECK(value.importTextProfile(block, Transport::DuplicateNamespace, diagnostic));
    CHECK_FALSE(value.importedName());
    CHECK(value.importTextProfile(std::nullopt, Transport::DuplicateNamespace));
    CHECK(value.importTextProfile(std::nullopt, Transport::Representable, diagnostic));
    auto unsupported = block; unsupported.push_back({1000, QDate(2026, 1, 1)});
    CHECK(value.importTextProfile(unsupported));
    CHECK_FALSE(value.importedName());
    QByteArray wire = QByteArray::fromHex("6e616d65ff");
    value.setName(QString::fromUtf8(wire.constData(), wire.size()));
    auto *wireData = wire.data();
    REQUIRE_FALSE(value.importTextProfile(std::nullopt, Transport::UnrepresentableNamespace, diagnostic, wire));
    diagnosticData[0] = QChar('X'); wireData[0] = 'X';
    CHECK(value.getCurrentTextProfile().diagnostic == QStringLiteral("held diagnostic"));
    REQUIRE(value.importedNameUtf8());
    CHECK(*value.importedNameUtf8() == QByteArray::fromHex("6e616d65ff"));
}

TEST_CASE("STYLE lists own records and clone active bindings independently", "[text-style-authority][ownership][headless]") {
    LC_TextStyleList list;
    auto *first = list.upsertStyle(style(QStringLiteral("first")));
    auto *second = list.upsertStyle(style(QStringLiteral("second")));
    REQUIRE(first); REQUIRE(second);
    REQUIRE_FALSE(first->importTextProfile(Profile::encode(defaults()).block));
    CHECK(list.getActiveStyle() == first);
    list.addStyle(first);
    list.addStyle(style(QStringLiteral("first")).release());
    CHECK(list.count() == 2);
    LC_TextStyleList copied(list);
    REQUIRE(copied.getActiveStyle());
    CHECK(copied.getActiveStyle() != first);
    CHECK(copied.getActiveStyle()->getTextProfile().imported().raw == first->getTextProfile().imported().raw);
    REQUIRE_FALSE(copied.getActiveStyle()->removeTextProfile());
    CHECK(first->getTextProfile().authority() == Binding::Authority::Untouched);
    auto *replacement = list.upsertStyle(style(QStringLiteral("first")));
    CHECK(list.count() == 2);
    CHECK(list.getActiveStyle() == replacement);
    CHECK(copied.getActiveStyle()->getName() == QStringLiteral("first"));
    LC_TextStyleList assigned;
    assigned = copied;
    CHECK(assigned.getActiveStyle() != copied.getActiveStyle());
    const auto *same = &assigned;
    assigned = *same;
    CHECK(assigned.count() == 2);
    list.remove(replacement);
    CHECK(list.getActiveStyle() == second);
    list.clear();
    CHECK(list.count() == 0);
    CHECK(list.getActiveStyle() == nullptr);
    CHECK_FALSE(list.isModified());
    CHECK(copied.count() == 2);
}

TEST_CASE("STYLE replace and upsert never duplicate an already-owned pointer", "[text-style-authority][ownership][headless]") {
    LC_TextStyleList list;
    auto *first = list.upsertStyle(style(QStringLiteral("first")));
    auto *second = list.upsertStyle(style(QStringLiteral("second")));
    second->setName(first->getName());
    CHECK(list.upsertStyle(std::unique_ptr<LC_TextStyle>(second)) == second);
    CHECK(list.count() == 2);
    CHECK(list.at(0) == first);
    CHECK(list.at(1) == second);
    second->setName(QStringLiteral("second"));
    list.replace(*list.getStyles());
    CHECK(list.count() == 2);
    CHECK(list.getActiveStyle() == first);
    auto *duplicate = style(QStringLiteral("second")).release();
    list.replace(QList<LC_TextStyle *>{second, nullptr, second, duplicate});
    CHECK(list.count() == 1);
    CHECK(list.at(0) == second);
    CHECK(list.getActiveStyle() == second);
    list.clear();
}
