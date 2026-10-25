// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 librecad.org
// Copyright (C) 2026 Dongxu Li (github.com/dxli)

// The language combos of the Welcome and Options dialogs hold the canonical
// codes RS_System::initLanguageList() makes of the catalog names ("en_US" for
// librecad_en.qm), so their defaults must be chosen by code, not by the text of
// a bare code ("English" matches no item).

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QLocale>
#include <QSettings>
#include <QStringList>
#include <QVariant>

#include <utility>

#include "lc_actiontestsupport.h"
#include "qg_dlginitial.h"
#include "qg_dlgoptionsgeneral.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {

// Puts back the settings, default locale and language list a test case changes.
class LanguageFixture {
public:
    LanguageFixture() {
        for (const QString& key : m_keys) {
            m_values << RS_SETTINGS->getSettings()->value(key);
        }
    }

    ~LanguageFixture() {
        for (qsizetype i = 0; i < m_keys.size(); ++i) {
            const QStringList groupAndKey = m_keys.at(i).split('/');
            const auto guard = RS_SETTINGS->beginGroupGuard(groupAndKey.first());
            if (m_values.at(i).isValid()) {
                RS_SETTINGS->write(groupAndKey.last(), m_values.at(i));
            }
            else {
                RS_SETTINGS->remove(groupAndKey.last());
            }
        }
        QLocale::setDefault(m_locale);
        RS_SYSTEM->initLanguageList();
    }

    // initLanguageList() reads only the names of the catalogs in Paths/Translations,
    // so the files need not exist.
    static void listCatalogs(const QStringList& names) {
        QStringList files;
        for (const QString& name : names) {
            files << QStringLiteral("/nonexistent/librecad_%1.qm").arg(name);
        }
        LC_SET_ONE("Paths", "Translations", files.join(';'));
        RS_SYSTEM->initLanguageList();
    }

    // Some of the catalogs a release ships.
    static void listShippedCatalogs() {
        listCatalogs({"ar", "de", "en", "en_au", "es", "es_ar", "zh_cn", "zh_tw"});
    }

private:
    const QLocale m_locale;
    const QStringList m_keys{"Paths/Translations", "Appearance/Language",
                             "Appearance/LanguageCmd", "Defaults/Unit"};
    QVariantList m_values;
};

} // namespace

TEST_CASE("Language codes match the canonical language list", "[translation][language-selection]") {
    const QStringList listed{"ar_EG", "de_DE", "en_US", "en_AU", "es_ES", "es_AR", "zh_CN", "zh_TW"};

    CHECK(RS_System::matchLanguage(listed, "en_US") == "en_US");
    CHECK(RS_System::matchLanguage(listed, "en-au") == "en_AU");
    CHECK(RS_System::matchLanguage(listed, "es_AR") == "es_AR");

    // bare codes, as stored before the list held canonical codes
    CHECK(RS_System::matchLanguage(listed, "en") == "en_US");
    CHECK(RS_System::matchLanguage(listed, "de") == "de_DE");
    CHECK(RS_System::matchLanguage(listed, "es") == "es_ES");
    CHECK(RS_System::matchLanguage(listed, "ar") == "ar_EG");

    // regions without a catalog of their own, as QLocale::uiLanguages() gives them
    CHECK(RS_System::matchLanguage(listed, "de-AT") == "de_DE");
    CHECK(RS_System::matchLanguage(listed, "ar_SA") == "ar_EG");
    CHECK(RS_System::matchLanguage(listed, "zh-Hant-HK") == "zh_TW");
    CHECK(RS_System::matchLanguage(listed, "zh-Hans-CN") == "zh_CN");
    CHECK(RS_System::matchLanguage(QStringList{"en_AU"}, "en") == "en_AU");

    CHECK(RS_System::matchLanguage(listed, "fr_FR").isEmpty());
    CHECK(RS_System::matchLanguage(listed, "").isEmpty());
    CHECK(RS_System::matchLanguage(listed, "C").isEmpty());
    CHECK(RS_System::matchLanguage(QStringList{}, "en").isEmpty());
}

TEST_CASE("Welcome dialog preselects and saves a listed language", "[gui][language-selection]") {
    REQUIRE(lc::test::application() != nullptr);
    LanguageFixture fixture;
    LanguageFixture::listShippedCatalogs();
    const QStringList languages = RS_SYSTEM->getLanguageList();
    REQUIRE(languages.contains("en_US"));
    REQUIRE(RS_System::matchLanguage(languages, "sw").isEmpty());

    // the GUI follows the preferred language, falling back to English; commands stay English
    const std::pair<const char*, const char*> cases[] = {
        {"en_US", "en_US"}, {"de_AT", "de_DE"}, {"sw_KE", "en_US"}};
    for (const auto& [locale, gui] : cases) {
        INFO("default locale " << locale);
        QLocale::setDefault(QLocale(locale));
        QG_DlgInitial dialog;

        CHECK(dialog.cbLanguage->currentIndex() >= 0);
        CHECK(dialog.cbLanguageCmd->currentIndex() >= 0);
        CHECK(dialog.cbLanguage->currentData().toString() == gui);
        CHECK(dialog.cbLanguageCmd->currentData().toString() == "en_US");

        LC_SET_ONE("Appearance", "Language", QString{});
        LC_SET_ONE("Appearance", "LanguageCmd", QString{});
        dialog.ok();
        CHECK(LC_GET_ONE_STR("Appearance", "Language", "") == gui);
        CHECK(LC_GET_ONE_STR("Appearance", "LanguageCmd", "") == "en_US");
    }
}

TEST_CASE("Options dialog selects the stored language by its code", "[gui][language-selection]") {
    REQUIRE(lc::test::application() != nullptr);
    LanguageFixture fixture;
    LanguageFixture::listShippedCatalogs();

    // "en" is the item the Options dialog adds ahead of the listed languages
    const std::pair<const char*, const char*> cases[] = {
        {"de_DE", "de_DE"}, {"de", "de_DE"}, {"en_US", "en_US"},
        {"en", "en"}, {"", "en"}, {"xx", "en"}};
    for (const auto& [stored, selected] : cases) {
        INFO("stored language \"" << stored << '"');
        LC_SET_ONE("Appearance", "Language", QString(stored));
        LC_SET_ONE("Appearance", "LanguageCmd", QString(stored));
        QG_DlgOptionsGeneral dialog;

        CHECK(dialog.cbLanguage->currentData().toString() == selected);
        CHECK(dialog.cbLanguageCmd->currentData().toString() == selected);
    }
}

TEST_CASE("A canonical language code loads the catalog of its language", "[translation][language-selection]") {
    REQUIRE(lc::test::application() != nullptr);
    LanguageFixture fixture;
    LC_SET_ONE("Paths", "Translations", QStringLiteral(LIBRECAD_TEST_TRANSLATION_DIR));

    // "de_DE" is what either dialog stores for librecad_de.qm
    RS_SYSTEM->loadTranslation("de_DE", "de_DE");
    CHECK(QCoreApplication::translate("QG_DlgInitial", "Welcome") == "Willkommen");

    RS_SYSTEM->loadTranslation("en_US", "en_US");
    CHECK(QCoreApplication::translate("QG_DlgInitial", "Welcome") == "Welcome");
}
