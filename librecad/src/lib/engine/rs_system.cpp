/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2021 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "rs_system.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTranslator>

#include "rs_debug.h"
#include "rs_locale.h"
#include "rs_settings.h"

class QString;

RS_System* RS_System::instance() {
    static auto uniqueInstance = new RS_System();
    return uniqueInstance;
}

/**
 * Initializes the system.
 *
 * @param appName Application name (e.g. "librecad II")
 * @param appVersion Application version (e.g. "1.2.3")
 * @param appDirName Application directory name used for
 *     subdirectories in /usr, /etc ~/.
 * @param arg0
 */
void RS_System::init(const QString& appName, const QString& appVersion, const QString& appDirName, const char* arg0) {
    this->m_appName = appName;
    this->m_appVersion = appVersion;
    this->m_appDirName = appDirName;
    if (QFile::decodeName( arg0).contains( "/.mount")) {
        // in AppImage QCoreApplication::applicationDirPath() directs to /lib64 of mounted AppImage
        // thus use argv[0] to extract the correct path to librecad executable
        m_appDir = QFileInfo( QFile::decodeName( arg0)).absoluteFilePath();
        RS_DEBUG->print("%s\n", (QString("arg0:")+ QString(arg0)).toUtf8().constData());
        RS_DEBUG->print("%s\n", (QString("appDir:")+ m_appDir).toUtf8().constData());
    }
    else {
        // in regular application QCoreApplication::applicationDirPath() is preferred, see GitHub #1488
        m_appDir = QCoreApplication::applicationDirPath();
        RS_DEBUG->print("%s\n", (QString("appDir2:")+ m_appDir).toUtf8().constData());
    }

    // when appDir is not HOME or CURRENT dir, search appDir too in getDirectoryList()
    m_externalAppDir = (!m_appDir.isEmpty()
                   && "/" != m_appDir
                   && getHomeDir() != m_appDir
                   && getCurrentDir() != m_appDir);

    RS_DEBUG->print("RS_System::init: System %s initialized.", appName.toLatin1().data());
    RS_DEBUG->print("RS_System::init: App dir: %s", m_appDir.toLatin1().data());
    m_initialized = true;

    initAllLanguagesList();
    initLanguageList();
}


void RS_System::init(const QString& appName,
                     const QString& appVersion,
                     const QString& appDirName,
                     const QString& arg0){
    init(appName, appVersion, appDirName, arg0.toLatin1().data());
}

/**
 * Initializes the list of available translations.
 */
void RS_System::initLanguageList() {
    RS_DEBUG->print("RS_System::initLanguageList");
    QStringList lst = getFileList("qm", "qm");

    LC_GROUP("Paths"); // fixme settings
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    lst += LC_GET_STR("Translations", "").split(";", Qt::SkipEmptyParts);
#else
    lst += (RS_SETTINGS->readEntry("/Translations", "")).split(";", QString::SkipEmptyParts);
#endif
    LC_GROUP_END();

    for (auto& it : lst) {

        RS_DEBUG->print("RS_System::initLanguageList: qm file: %s", it.toLatin1().data());

       const int i0 = it.lastIndexOf(QString("librecad"), -1, Qt::CaseInsensitive);
        if (i0 == -1){
            continue;
       }
       const int i1 = it.indexOf('_', i0);
       const int i2 = it.indexOf('.', i1);
        if (i1 == -1 || i2 == -1) {
            continue;
        }
        QString l = it.mid(i1 + 1, i2 - i1 - 1);

        if (!m_languageList.contains(l) ) {
            RS_DEBUG->print("RS_System::initLanguageList: append language: %s",
                            l.toLatin1().data());
            m_languageList.append(l);
        }
    }
    RS_DEBUG->print("RS_System::initLanguageList: OK");
}

void RS_System::addLocale(RS_Locale *locale) {
    m_allKnownLocales.push_back( QSharedPointer<RS_Locale>( locale));
}

#define LNG(canonical, direction, name) \
    locale = new RS_Locale(); \
    locale->setCanonical( canonical); \
    locale->setDirection( direction); \
    locale->setName( name); \
    addLocale( locale);

void RS_System::initAllLanguagesList() {
    // RVT uk_AU renamed to uk so that we don't have to change the pootle server

    m_allKnownLocales.clear();
    RS_Locale *locale;
    LNG( "ab"   , RS2::locLeftToRight, "Abkhazian")
    LNG( "aa"   , RS2::locLeftToRight, "Afar")
    LNG( "af_ZA", RS2::locLeftToRight, "Afrikaans")
    LNG( "sq_AL", RS2::locLeftToRight, "Albanian")
    LNG( "am"   , RS2::locLeftToRight, "Amharic")
    LNG( "ar"   , RS2::locRightToLeft, "Arabic")
    LNG( "ar_DZ", RS2::locRightToLeft, "Arabic (Algeria)")
    LNG( "ar_BH", RS2::locRightToLeft, "Arabic (Bahrain)")
    LNG( "ar_EG", RS2::locRightToLeft, "Arabic (Egypt)")
    LNG( "ar_IQ", RS2::locRightToLeft, "Arabic (Iraq)")
    LNG( "ar_JO", RS2::locRightToLeft, "Arabic (Jordan)")
    LNG( "ar_KW", RS2::locRightToLeft, "Arabic (Kuwait)")
    LNG( "ar_LB", RS2::locRightToLeft, "Arabic (Lebanon)")
    LNG( "ar_LY", RS2::locRightToLeft, "Arabic (Libya)")
    LNG( "ar_MA", RS2::locRightToLeft, "Arabic (Morocco)")
    LNG( "ar_OM", RS2::locRightToLeft, "Arabic (Oman)")
    LNG( "ar_QA", RS2::locRightToLeft, "Arabic (Qatar)")
    LNG( "ar_SA", RS2::locRightToLeft, "Arabic (Saudi Arabia)")
    LNG( "ar_SD", RS2::locRightToLeft, "Arabic (Sudan)")
    LNG( "ar_SY", RS2::locRightToLeft, "Arabic (Syria)")
    LNG( "ar_TN", RS2::locRightToLeft, "Arabic (Tunisia)")
    LNG( "ar_AE", RS2::locRightToLeft, "Arabic (Uae)")
    LNG( "ar_YE", RS2::locRightToLeft, "Arabic (Yemen)")
    LNG( "hy"   , RS2::locLeftToRight, "Armenian")
    LNG( "as"   , RS2::locLeftToRight, "Assamese")
    LNG( "ay"   , RS2::locLeftToRight, "Aymara")
    LNG( "az"   , RS2::locLeftToRight, "Azeri")
    LNG( "az"   , RS2::locLeftToRight, "Azeri (Cyrillic)")
    LNG( "az"   , RS2::locLeftToRight, "Azeri (Latin)")
    LNG( "ba"   , RS2::locLeftToRight, "Bashkir")
    LNG( "eu_ES", RS2::locLeftToRight, "Basque")
    LNG( "be_BY", RS2::locLeftToRight, "Belarusian")
    LNG( "bn"   , RS2::locLeftToRight, "Bengali")
    LNG( "dz"   , RS2::locLeftToRight, "Bhutani")
    LNG( "bh"   , RS2::locLeftToRight, "Bihari")
    LNG( "bi"   , RS2::locLeftToRight, "Bislama")
    LNG( "br"   , RS2::locLeftToRight, "Breton")
    LNG( "bg_BG", RS2::locLeftToRight, "Bulgarian")
    LNG( "my"   , RS2::locLeftToRight, "Burmese")
    LNG( "km"   , RS2::locLeftToRight, "Cambodian")
    LNG( "ca_ES", RS2::locLeftToRight, "Catalan")
    LNG( "zh_TW", RS2::locLeftToRight, "Chinese")
    LNG( "zh_CN", RS2::locLeftToRight, "Chinese (Simplified)")
    LNG( "zh_TW", RS2::locLeftToRight, "Chinese (Traditional)")
    LNG( "zh_HK", RS2::locLeftToRight, "Chinese (Hongkong)")
    LNG( "zh_MO", RS2::locLeftToRight, "Chinese (Macau)")
    LNG( "zh_SG", RS2::locLeftToRight, "Chinese (Singapore)")
    LNG( "zh_TW", RS2::locLeftToRight, "Chinese (Taiwan)")
    LNG( "co"   , RS2::locLeftToRight, "Corsican")
    LNG( "hr_HR", RS2::locLeftToRight, "Croatian")
    LNG( "cs_CZ", RS2::locLeftToRight, "Czech")
    LNG( "da_DK", RS2::locLeftToRight, "Danish")
    LNG( "nl_NL", RS2::locLeftToRight, "Dutch")
    LNG( "nl_BE", RS2::locLeftToRight, "Dutch (Belgian)")
    LNG( "en_GB", RS2::locLeftToRight, "English")
    LNG( "en_GB", RS2::locLeftToRight, "English (U.K.)")
    LNG( "en_US", RS2::locLeftToRight, "English (U.S.)")
    LNG( "en_AU", RS2::locLeftToRight, "English (Australia)")
    LNG( "en_BZ", RS2::locLeftToRight, "English (Belize)")
    LNG( "en_BW", RS2::locLeftToRight, "English (Botswana)")
    LNG( "en_CA", RS2::locLeftToRight, "English (Canada)")
    LNG( "en_CB", RS2::locLeftToRight, "English (Caribbean)")
    LNG( "en_DK", RS2::locLeftToRight, "English (Denmark)")
    LNG( "en_IE", RS2::locLeftToRight, "English (Eire)")
    LNG( "en_JM", RS2::locLeftToRight, "English (Jamaica)")
    LNG( "en_NZ", RS2::locLeftToRight, "English (New Zealand)")
    LNG( "en_PH", RS2::locLeftToRight, "English (Philippines)")
    LNG( "en_ZA", RS2::locLeftToRight, "English (South Africa)")
    LNG( "en_TT", RS2::locLeftToRight, "English (Trinidad)")
    LNG( "en_ZW", RS2::locLeftToRight, "English (Zimbabwe)")
    LNG( "eo"   , RS2::locLeftToRight, "Esperanto")
    LNG( "et_EE", RS2::locLeftToRight, "Estonian")
    LNG( "fo_FO", RS2::locLeftToRight, "Faeroese")
    LNG( "fa_IR", RS2::locLeftToRight, "Farsi")
    LNG( "fj"   , RS2::locLeftToRight, "Fiji")
    LNG( "fi_FI", RS2::locLeftToRight, "Finnish")
    LNG( "fr_FR", RS2::locLeftToRight, "French")
    LNG( "fr_BE", RS2::locLeftToRight, "French (Belgian)")
    LNG( "fr_CA", RS2::locLeftToRight, "French (Canadian)")
    LNG( "fr_LU", RS2::locLeftToRight, "French (Luxembourg)")
    LNG( "fr_MC", RS2::locLeftToRight, "French (Monaco)")
    LNG( "fr_CH", RS2::locLeftToRight, "French (Swiss)")
    LNG( "fy"   , RS2::locLeftToRight, "Frisian")
    LNG( "gl_ES", RS2::locLeftToRight, "Galician")
    LNG( "ka_GE", RS2::locLeftToRight, "Georgian")
    LNG( "de_DE", RS2::locLeftToRight, "German")
    LNG( "de_AT", RS2::locLeftToRight, "German (Austrian)")
    LNG( "de_BE", RS2::locLeftToRight, "German (Belgium)")
    LNG( "de_LI", RS2::locLeftToRight, "German (Liechtenstein)")
    LNG( "de_LU", RS2::locLeftToRight, "German (Luxembourg)")
    LNG( "de_CH", RS2::locLeftToRight, "German (Swiss)")
    LNG( "el_GR", RS2::locLeftToRight, "Greek")
    LNG( "kl_GL", RS2::locLeftToRight, "Greenlandic")
    LNG( "gn"   , RS2::locLeftToRight, "Guarani")
    LNG( "gu"   , RS2::locLeftToRight, "Gujarati")
    LNG( "ha"   , RS2::locLeftToRight, "Hausa")
    LNG( "he_IL", RS2::locRightToLeft, "Hebrew")
    LNG( "hi_IN", RS2::locLeftToRight, "Hindi")
    LNG( "hu_HU", RS2::locLeftToRight, "Hungarian")
    LNG( "is_IS", RS2::locLeftToRight, "Icelandic")
    LNG( "id_ID", RS2::locLeftToRight, "Indonesian")
    LNG( "ia"   , RS2::locLeftToRight, "Interlingua")
    LNG( "ie"   , RS2::locLeftToRight, "Interlingue")
    LNG( "iu"   , RS2::locLeftToRight, "Inuktitut")
    LNG( "ik"   , RS2::locLeftToRight, "Inupiak")
    LNG( "ga_IE", RS2::locLeftToRight, "Irish")
    LNG( "it_IT", RS2::locLeftToRight, "Italian")
    LNG( "it_CH", RS2::locLeftToRight, "Italian (Swiss)")
    LNG( "ja_JP", RS2::locLeftToRight, "Japanese")
    LNG( "jw"   , RS2::locLeftToRight, "Javanese")
    LNG( "kn"   , RS2::locLeftToRight, "Kannada")
    LNG( "ks"   , RS2::locLeftToRight, "Kashmiri")
    LNG( "ks_IN", RS2::locLeftToRight, "Kashmiri (India)")
    LNG( "kk"   , RS2::locLeftToRight, "Kazakh")
    LNG( "kw_GB", RS2::locLeftToRight, "Kernewek")
    LNG( "rw"   , RS2::locLeftToRight, "Kinyarwanda")
    LNG( "ky"   , RS2::locLeftToRight, "Kirghiz")
    LNG( "rn"   , RS2::locLeftToRight, "Kirundi")
    LNG( ""     , RS2::locLeftToRight, "Konkani")
    LNG( "ko_KR", RS2::locLeftToRight, "Korean")
    LNG( "ku_TR", RS2::locLeftToRight, "Kurdish")
    LNG( "lo"   , RS2::locLeftToRight, "Laothian")
    LNG( "la"   , RS2::locLeftToRight, "Latin")
    LNG( "lv_LV", RS2::locLeftToRight, "Latvian")
    LNG( "ln"   , RS2::locLeftToRight, "Lingala")
    LNG( "lt_LT", RS2::locLeftToRight, "Lithuanian")
    LNG( "mk_MK", RS2::locLeftToRight, "Macedonian")
    LNG( "mg"   , RS2::locLeftToRight, "Malagasy")
    LNG( "ms_MY", RS2::locLeftToRight, "Malay")
    LNG( "ml"   , RS2::locLeftToRight, "Malayalam")
    LNG( "ms_BN", RS2::locLeftToRight, "Malay (Brunei Darussalam)")
    LNG( "ms_MY", RS2::locLeftToRight, "Malay (Malaysia)")
    LNG( "mt_MT", RS2::locLeftToRight, "Maltese")
    LNG( ""     , RS2::locLeftToRight, "Manipuri")
    LNG( "mi"   , RS2::locLeftToRight, "Maori")
    LNG( "mr_IN", RS2::locLeftToRight, "Marathi")
    LNG( "mo"   , RS2::locLeftToRight, "Moldavian")
    LNG( "mn"   , RS2::locLeftToRight, "Mongolian")
    LNG( "na"   , RS2::locLeftToRight, "Nauru")
    LNG( "ne_NP", RS2::locLeftToRight, "Nepali")
    LNG( "ne_IN", RS2::locLeftToRight, "Nepali (India)")
    LNG( "nb_NO", RS2::locLeftToRight, "Norwegian (Bokmal)")
    LNG( "nn_NO", RS2::locLeftToRight, "Norwegian (Nynorsk)")
    LNG( "oc"   , RS2::locLeftToRight, "Occitan")
    LNG( "or"   , RS2::locLeftToRight, "Oriya")
    LNG( "om"   , RS2::locLeftToRight, "(Afan) Oromo")
    LNG( "ps"   , RS2::locLeftToRight, "Pashto, Pushto")
    LNG( "pl_PL", RS2::locLeftToRight, "Polish")
    LNG( "pt_PT", RS2::locLeftToRight, "Portuguese")
    LNG( "pt_BR", RS2::locLeftToRight, "Portuguese (Brazilian)")
    LNG( "pa"   , RS2::locLeftToRight, "Punjabi")
    LNG( "qu"   , RS2::locLeftToRight, "Quechua")
    LNG( "rm"   , RS2::locLeftToRight, "Rhaeto-Romance")
    LNG( "ro_RO", RS2::locLeftToRight, "Romanian")
    LNG( "ru_RU", RS2::locLeftToRight, "Russian")
    LNG( "ru_UA", RS2::locLeftToRight, "Russian (Ukraine)")
    LNG( "sm"   , RS2::locLeftToRight, "Samoan")
    LNG( "sg"   , RS2::locLeftToRight, "Sangho")
    LNG( "sa"   , RS2::locLeftToRight, "Sanskrit")
    LNG( "gd"   , RS2::locLeftToRight, "Scots Gaelic")
    LNG( "se_NO", RS2::locLeftToRight, "Northern Sami")
    LNG( "sr_SR", RS2::locLeftToRight, "Serbian")
    LNG( "sr_SR", RS2::locLeftToRight, "Serbian (Cyrillic)")
    LNG( "sr_SR@latin", RS2::locLeftToRight, "Serbian (Latin)")
    LNG( "sr_YU", RS2::locLeftToRight, "Serbian (Cyrillic)")
    LNG( "sr_YU@latin", RS2::locLeftToRight, "Serbian (Latin)")
    LNG( "sh"   , RS2::locLeftToRight, "Serbo-Croatian")
    LNG( "st"   , RS2::locLeftToRight, "Sesotho")
    LNG( "tn"   , RS2::locLeftToRight, "Setswana")
    LNG( "sn"   , RS2::locLeftToRight, "Shona")
    LNG( "sd"   , RS2::locLeftToRight, "Sindhi")
    LNG( "si"   , RS2::locLeftToRight, "Sinhalese")
    LNG( "ss"   , RS2::locLeftToRight, "Siswati")
    LNG( "sk_SK", RS2::locLeftToRight, "Slovak")
    LNG( "sl_SI", RS2::locLeftToRight, "Slovenian")
    LNG( "so"   , RS2::locLeftToRight, "Somali")
    LNG( "es_ES", RS2::locLeftToRight, "Spanish")
    LNG( "es_AR", RS2::locLeftToRight, "Spanish (Argentina)")
    LNG( "es_BO", RS2::locLeftToRight, "Spanish (Bolivia)")
    LNG( "es_CL", RS2::locLeftToRight, "Spanish (Chile)")
    LNG( "es_CO", RS2::locLeftToRight, "Spanish (Colombia)")
    LNG( "es_CR", RS2::locLeftToRight, "Spanish (Costa Rica)")
    LNG( "es_DO", RS2::locLeftToRight, "Spanish (Dominican republic)")
    LNG( "es_EC", RS2::locLeftToRight, "Spanish (Ecuador)")
    LNG( "es_SV", RS2::locLeftToRight, "Spanish (El Salvador)")
    LNG( "es_GT", RS2::locLeftToRight, "Spanish (Guatemala)")
    LNG( "es_HN", RS2::locLeftToRight, "Spanish (Honduras)")
    LNG( "es_MX", RS2::locLeftToRight, "Spanish (Mexican)")
    LNG( "es_ES", RS2::locLeftToRight, "Spanish (Modern)")
    LNG( "es_NI", RS2::locLeftToRight, "Spanish (Nicaragua)")
    LNG( "es_PA", RS2::locLeftToRight, "Spanish (Panama)")
    LNG( "es_PY", RS2::locLeftToRight, "Spanish (Paraguay)")
    LNG( "es_PE", RS2::locLeftToRight, "Spanish (Peru)")
    LNG( "es_PR", RS2::locLeftToRight, "Spanish (Puerto Rico)")
    LNG( "es_UY", RS2::locLeftToRight, "Spanish (Uruguay)")
    LNG( "es_US", RS2::locLeftToRight, "Spanish (U.S.)")
    LNG( "es_VE", RS2::locLeftToRight, "Spanish (Venezuela)")
    LNG( "su"   , RS2::locLeftToRight, "Sundanese")
    LNG( "sw_KE", RS2::locLeftToRight, "Swahili")
    LNG( "sv_SE", RS2::locLeftToRight, "Swedish")
    LNG( "sv_FI", RS2::locLeftToRight, "Swedish (Finland)")
    LNG( "tl_PH", RS2::locLeftToRight, "Tagalog")
    LNG( "tg"   , RS2::locLeftToRight, "Tajik")
    LNG( "ta"   , RS2::locLeftToRight, "Tamil")
    LNG( "tt"   , RS2::locLeftToRight, "Tatar")
    LNG( "te"   , RS2::locLeftToRight, "Telugu")
    LNG( "th_TH", RS2::locLeftToRight, "Thai")
    LNG( "bo"   , RS2::locLeftToRight, "Tibetan")
    LNG( "ti"   , RS2::locLeftToRight, "Tigrinya")
    LNG( "to"   , RS2::locLeftToRight, "Tonga")
    LNG( "ts"   , RS2::locLeftToRight, "Tsonga")
    LNG( "tr_TR", RS2::locLeftToRight, "Turkish")
    LNG( "tk"   , RS2::locLeftToRight, "Turkmen")
    LNG( "tw"   , RS2::locLeftToRight, "Twi")
    LNG( "ug"   , RS2::locLeftToRight, "Uighur")
    LNG( "uk"   , RS2::locLeftToRight, "Ukrainian")
    LNG( "ur"   , RS2::locLeftToRight, "Urdu")
    LNG( "ur_IN", RS2::locLeftToRight, "Urdu (India)")
    LNG( "ur_PK", RS2::locLeftToRight, "Urdu (Pakistan)")
    LNG( "uz"   , RS2::locLeftToRight, "Uzbek")
    LNG( "uz"   , RS2::locLeftToRight, "Uzbek (Cyrillic)")
    LNG( "uz"   , RS2::locLeftToRight, "Uzbek (Latin)")
    LNG( "ca_ES@valencia", RS2::locLeftToRight, "Valencian")
    LNG( "vi_VN", RS2::locLeftToRight, "Vietnamese")
    LNG( "vo"   , RS2::locLeftToRight, "Volapuk")
    LNG( "cy"   , RS2::locLeftToRight, "Welsh")
    LNG( "wo"   , RS2::locLeftToRight, "Wolof")
    LNG( "xh"   , RS2::locLeftToRight, "Xhosa")
    LNG( "yi"   , RS2::locLeftToRight, "Yiddish")
    LNG( "yo"   , RS2::locLeftToRight, "Yoruba")
    LNG( "za"   , RS2::locLeftToRight, "Zhuang")
    LNG( "zu"   , RS2::locLeftToRight, "Zulu")
}

/**
 * Loads a different translation for the application GUI.
 *
 *fixme, need to support command language
 */
void RS_System::loadTranslation(const QString& lang, const QString& /*langCmd*/) {
    static QTranslator* tQt = nullptr;
    static QTranslator* tLibreCAD = nullptr;
    static QTranslator* tPlugIns = nullptr;

    //make translation filenames case insensitive, #276
    QString langLower("");
    QString langUpper("");
    const int i0 = lang.indexOf('_');
    if (i0 >= 2 && lang.size() - i0 >= 2) {
        //contains region code
        langLower = lang.left( i0) + '_' + lang.mid( i0 + 1).toLower();
        langUpper = lang.left( i0) + '_' + lang.mid( i0 + 1).toUpper();
    }
    else {
        langLower = lang;
        langUpper.clear();
    }
    // search in various directories for translations
    QStringList lst = getDirectoryList( "qm");

    LC_GROUP( "Paths");
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    lst += LC_GET_STR("Translations", "").split(";", Qt::SkipEmptyParts);
#else
    lst += (RS_SETTINGS->readEntry( "/Translations", "")).split( ";", QString::SkipEmptyParts);
#endif
    LC_GROUP_END();

    if( tLibreCAD != nullptr) {
        qApp->removeTranslator( tLibreCAD);
        delete tLibreCAD;
    }
    if( tPlugIns != nullptr) {
        qApp->removeTranslator( tPlugIns);
        delete tPlugIns;
    }
    if( tQt != nullptr) {
        qApp->removeTranslator( tQt);
        delete tQt;
    }
    const QString langFileLower = "librecad_" + langLower + ".qm";
    const QString langFileUpper = "librecad_" + langUpper + ".qm";
    const QString langPlugInsLower = "plugins_" + langLower + ".qm";
    const QString langPlugInsUpper = "plugins_" + langUpper + ".qm";
    const QString langQtLower = "qt_" + langLower + ".qm";
    const QString langQtUpper = "qt_" + langUpper + ".qm";
    auto t = new QTranslator(nullptr);
    for (QStringList::Iterator it = lst.begin();
         it != lst.end();
         ++it) {

        // load LibreCAD translations
        if (nullptr == tLibreCAD) {
            if (t->load(langFileLower, *it) || (!langUpper.isEmpty() && t->load(langFileUpper, *it))) {
                tLibreCAD = t;
                qApp->installTranslator( tLibreCAD);
                t = new QTranslator(nullptr);
            }
        }

        // load PlugIns translations
        if (nullptr == tPlugIns) {
            if (t->load(langPlugInsLower, *it) || (!langUpper.isEmpty() && t->load(langPlugInsUpper, *it) == true)) {
                tPlugIns = t;
                qApp->installTranslator( tPlugIns);
                t = new QTranslator(nullptr);
            }
        }

        // load Qt standard dialog translations
        if (nullptr == tQt) {
            if (t->load(langQtLower, *it) || (!langUpper.isEmpty() && t->load(langQtUpper, *it))) {
                tQt = t;
                qApp->installTranslator( tQt);
                t = new QTranslator(nullptr);
            }
        }
        if (nullptr != tLibreCAD && nullptr != tPlugIns && nullptr != tQt) {
            break;
        }
    }

    delete t;
}


/**
 * Checks if the system has been initialized and prints a warning
 * otherwise to stderr.
 */
bool RS_System::checkInit() const{
    if (!m_initialized) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Syste.m::checkInit: System not initialized.\n"
                        "Use RS_SYSTEM->init(appname, appdirname) to do so.");
    }
    return m_initialized;
}

/**
 * Creates all given directories in the user's home.
 */
bool RS_System::createPaths(const QString& directory) {
    QDir dir;
    dir.cd(QDir::homePath());
    dir.mkpath(directory);
    return true;
}

/**
 * Create if not exist and return the Application data directory.
 * In OS_WIN32 "c:\documents&settings\<user>\local configuration\application data\LibreCAD"
 * In OS_MAC "/Users/<user>/Library/Application Support/LibreCAD"
 * In OS_LINUX "/home/<user>/.local/share/data/LibreCAD"
 *
 * @return Application data directory.
 */
QString RS_System::getAppDataDir() const{
    QString appData = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation);
    const QDir dir( appData);
    if (!dir.exists()) {
        if (!dir.mkpath( appData)) {
            return QString();
        }
    }
    RS_DEBUG->print("%s\n", (QString("appData: ") + appData).toUtf8().constData());
    return appData;
}

/**
 * Searches for files in an application shared directory in the given
 * subdirectory with the given extension.
 *
 * @return List of the absolute paths of the files found.
 */
QStringList RS_System::getFileList(const QString& subDirectory,
                                   const QString& fileExtension) const{
    checkInit();

    RS_DEBUG->print( "RS_System::getFileList: subdirectory %s ", subDirectory.toLatin1().data());
    RS_DEBUG->print( "RS_System::getFileList: appDirName %s ", m_appDirName.toLatin1().data());
    RS_DEBUG->print( "RS_System::getFileList: getCurrentDir %s ", getCurrentDir().toLatin1().data());

    QStringList fileList;

    auto directoryList = getDirectoryList( subDirectory);

    foreach(const QString& path, directoryList) {
        QDir dir {path};

        if (dir.exists() && dir.isReadable()) {
            QStringList files = dir.entryList( QStringList( "*." + fileExtension));
            for(QString& file: files)
            {
                fileList += path + "/" + file;
            }
        }
    }

    LC_LOG<<__func__<<"():: fileList:";
    foreach(const auto& file, fileList)
        LC_LOG<<file;

    return fileList;
}

/**
 * @return List of all directories in subdirectory 'subDirectory' in
 * all possible LibreCAD directories.
 */
QStringList RS_System::getDirectoryList(const QString& subDir) const{
    QStringList dirList;
    const QString subDirectory = QDir::fromNativeSeparators( subDir);

#ifdef Q_OS_MAC
    dirList.append(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + m_appDirName + "/" + subDirectory);
#endif // Q_OS_MAC

#if (defined(Q_OS_WIN32) || defined(Q_OS_WIN64))
    dirList.append(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + m_appDirName + "/" + subDirectory);
#endif // Q_OS_WIN32 or Q_OS_WIN64

    // Unix home directory, it's old style but some people might have stuff there.
    dirList.append( getHomeDir() + "/." + m_appDirName + "/" + subDirectory);

    //local (application) directory has priority over other dirs:
    if (subDirectory.compare( "plugins") == 0) {
        // 17 Aug, 2011, Dongxu Li, do not look for plugins in the current folder,
        // we should install plugins to system or ~/.librecad/plugins/
        if (m_externalAppDir) {
            dirList.append( m_appDir + "/" + subDirectory);
        }
    }

    RS_DEBUG->print("%s\n", QString("%1(): line %2: dir=%3").arg(__func__).arg(__LINE__).arg(m_appDir).toUtf8().constData());

#if (defined(Q_OS_WIN32) || defined(Q_OS_WIN64) || defined(Q_OS_UNIX))
    // for AppImage use relative paths from executable
    // from package manager the executable is in /usr/bin
    // in AppImage the executable is APPDIR/usr/bin
    // so this should work for package manager and AppImage distribution
    dirList.append( QDir::cleanPath( m_appDir + "/../share/doc/" + m_appDirName + "/" + subDirectory));

    // try various locations for different Linux distributions
    dirList.append( QDir::cleanPath( m_appDir + "/../share/" + m_appDirName + "/" + subDirectory));
    dirList.append( QDir::cleanPath( m_appDir + "/../lib64/" + m_appDirName + "/" + subDirectory));
    dirList.append( QDir::cleanPath( m_appDir + "/../lib/" + m_appDirName + "/" + subDirectory));

    if (QStringLiteral( "plugins") == subDirectory) {
        dirList.append( QDir::cleanPath( m_appDir + "/../lib64/" + m_appDirName));
        dirList.append( QDir::cleanPath( m_appDir + "/../lib/" + m_appDirName));
    }
#endif
    for (auto& dir: dirList) {
        RS_DEBUG->print("%s\n", QString("%1(): line %2: dir=%3\n").arg(__func__).arg(__LINE__).arg(dir).toUtf8().constData());
    }

#ifdef Q_OS_MAC
    // Apple uses the resource directory
    if (!m_appDir.isEmpty() && m_appDir!="/") {
        dirList.append( QDir::cleanPath( m_appDir + "/../Resources/" + subDirectory));
    }
#endif

#ifndef Q_OS_MAC
    // Add support directory if librecad is run-in-place,
    // not for Apple because it uses resources this is more for unix systems
    dirList.append( m_appDir + "/resources/" + subDirectory);
#endif

    // Individual directories:
    {
        LC_GROUP_GUARD( "Paths");
        {
            constexpr auto emptyBehavior = Qt::SkipEmptyParts;

            static const QRegularExpression SEP("[;]");

            if (subDirectory == "fonts") {
                const QString savedFonts = LC_GET_STR("Fonts", "");
                RS_DEBUG->print("saved fonts: %s\n", savedFonts.toUtf8().constData());
                dirList += LC_GET_STR("Fonts", "").split(SEP, emptyBehavior);
            }
            else if (subDirectory == "patterns") {
                dirList += LC_GET_STR("Patterns", "").split(SEP, emptyBehavior);
            }
            else if (subDirectory.startsWith("scripts")) {
                dirList += LC_GET_STR("Scripts", "").split(SEP, emptyBehavior);
            }
            else if (subDirectory.startsWith("library")) {
                dirList += LC_GET_STR("Library", "").split(SEP, emptyBehavior);
            }
            else if (subDirectory.startsWith("qm")) {
                dirList += LC_GET_STR("Translations", "").split(SEP, emptyBehavior);
            }
        }
    }

    QStringList ret;

    RS_DEBUG->print("RS_System::getDirectoryList: Paths:");
    for (const QString& dir: std::as_const(dirList)) {
        if (QFileInfo{dir}.isDir()) {
            ret += dir;
        }
    }

    for (const QString& dir: std::as_const(ret)) {
        RS_DEBUG->print("%s\n", QString("%1(): line %2: dir=%3").arg(__func__).arg(__LINE__).arg(dir).toUtf8().constData());
    }

    return ret;
}

/**
 * Converts a language string to a symbol (e.g. Deutsch or German to 'de').
 * Languages taken from RFC3066
 */
QString RS_System::languageToSymbol(const QString& lang) {
    const int i1 = lang.indexOf( ' ');
    QString l = lang;
    if (i1 >= 2){
        l = lang.mid( 0, i1);
    }
    return l;

//    RS_Locale *locale;
//    foreach (locale, *RS_SYSTEM->allKnownLocales) {
//        if (locale->getName().toLower() == l) {
//            return locale->getCanonical();
//        }
//    }

//    return "";
}

/**
 * Converts a locale code into a readable string
 * (e.g. 'de' to 'German Deutsch'
 * (e.g. 'en_au' to 'English (Australia)'
 */
QString RS_System::symbolToLanguage(const QString& symb) {
    const RS_Locale loc( symb);
    QString ret;
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    const QString territory = RS_Locale::territoryToString(loc.territory());
#else
    QString territory = RS_Locale::countryToString(loc.country());
#endif

    static QRegularExpression englishLang{"^en"};
    if (symb.contains(englishLang)) {
        ret = RS_Locale::languageToString(loc.language());
        if( symb.contains('_') ) {
            ret += " (" + territory + ')';
        }
    }
    else {
        ret = RS_Locale::languageToString(loc.language()) + ' ' + loc.nativeLanguageName();
        if( symb.contains( '_') ) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
            ret += " (" + territory + ' ' + loc.nativeTerritoryName() + ')';
#else
            ret += " (" + territory + ' ' + loc.nativeCountryName() + ')';
#endif
        }
    }
    return ret;
}

/**
 * Tries to convert the given encoding string to an encoding Qt knows.
 */
QString RS_System::getEncoding(const QString& str) {
    const QString l=str.toLower();

    if (l=="latin1" || l=="ansi_1252" || l=="iso-8859-1" ||
            l=="cp819" || l=="csiso" || l=="ibm819" || l=="iso_8859-1" ||
            l=="iso8859-1" || l=="iso-ir-100" || l=="l1") {
        return "Latin1";
    }
    if (l=="big5" || l=="ansi_950" || l=="cn-big5" || l=="csbig5" ||
        l=="x-x-big5") {
        return "Big5";
    }
    if (l=="big5-hkscs") {
        return "Big5-HKSCS";
    }
    if (l=="eucjp" || l=="euc-jp" || l=="cseucpkdfmtjapanese" ||
        l=="x-euc" || l=="x-euc-jp") {
        return "eucJP";
    }
    if (l=="euckr") {
        return "eucKR";
    }
    if (l=="gb2312" || l=="chinese" || l=="cn-gb" ||
        l=="csgb2312" || l=="csgb231280" || l=="csiso58gb231280" ||
        l=="gb_2312-80" || l=="gb231280" || l=="gb2312-80" || l=="gbk" ||
        l=="iso-ir-58") {
        return "GB2312";
    }
   /* if (l=="gbk") { // fixme - sand - same condition as above, dead code
        return "GBK";
    }*/
    if (l=="gb18030") {
        return "GB18030";
    }
    if (l=="jis7") {
        return "JIS7";
    }
    if (l=="shift-jis" || l=="ansi_932" || l=="shift_jis" || l=="csShiftJIS" ||
        l=="cswindows31j" || l=="ms_kanji" || l=="x-ms-cp932" || l=="x-sjis") {
        return "Shift-JIS";
    }
    if (l=="tscii") {
        return "TSCII";
    }
    if (l=="utf88-bit") {
        return "utf88-bit";
    }
    if (l=="utf16") {
        return "utf16";
    }
    if (l=="utf8" || l=="utf-8") {
        return "utf-8";
    }
    if (l=="koi8-r") {
        return "KOI8-R";
    }
    if (l=="koi8-u") {
        return "KOI8-U";
    }
  /*  if (l=="iso8859-1") { // fixme - sand - same condition as above, dead code
        return "ISO8859-1";
    }*/
    if (l=="iso8859-2") {
        return "ISO8859-2";
    }
    if (l=="iso8859-3") {
        return "ISO8859-3";
    }
    if (l=="iso8859-4" || l=="ansi_1257") {
        return "ISO8859-4";
    }
    if (l=="iso8859-5") {
        return "ISO8859-5";
    }
    if (l=="iso8859-6" || l=="ansi_1256") {
        return "ISO8859-6";
    }
    if (l=="iso8859-7" || l=="ansi_1253") {
        return "ISO8859-7";
    }
    if (l=="iso8859-8") {
        return "ISO8859-8";
    }
    if (l=="iso8859-8-i" || l=="ansi_1255") {
        return "ISO8859-8-i";
    }
    if (l=="iso8859-9" || l=="ansi_1254") {
        return "ISO8859-9";
    }
    if (l=="iso8859-10") {
        return "ISO8859-10";
    }
    if (l=="iso8859-13") {
        return "ISO8859-13";
    }
    if (l=="iso8859-14") {
        return "ISO8859-14";
    }
    if (l=="iso8859-15") {
        return "ISO8859-15";
    }
    if (l=="ibm 850") {
        return "IBM 850";
    }
    if (l=="ibm 866") {
        return "IBM 866";
    }
    if (l=="cp874") {
        return "CP874";
    }
    if (l=="cp1250") {
        return "CP1250";
    }
    if (l=="cp1251" || l=="ansi_1251") {
        return "CP1251";
    }
    if (l=="cp1252") {
        return "CP1252";
    }
    if (l=="cp1253") {
        return "CP1253";
    }
    if (l=="cp1254") {
        return "CP1254";
    }
    if (l=="cp1255") {
        return "CP1255";
    }
    if (l=="cp1256") {
        return "CP1256";
    }
    if (l=="cp1257") {
        return "CP1257";
    }
    if (l=="cp1258") {
        return "CP1258";
    }
    if (l=="apple roman") {
        return "Apple Roman";
    }
    if (l=="tis-620") {
        return "TIS-620";
    }

    return "latin1";
}


/** Returns ISO code for given locale. Needed for win32 to convert
 from system encodings.
 Locale names mostly copied from XFree86.

 The code may be incomplete (chinese/japanese locales, etc.)

 2004-05-13, J Staniek
*/
static QMap<QByteArray,QByteArray> g_locMap;

QByteArray RS_System::localeToISO(const QByteArray& locale) {
    if (g_locMap.isEmpty()) {
        g_locMap["croatian"]="ISO8859-2";
        g_locMap["cs"]="ISO8859-2";
        g_locMap["cs_CS"]="ISO8859-2";
        g_locMap["cs_CZ"]="ISO8859-2";
        g_locMap["cz"]="ISO8859-2";
        g_locMap["cz_CZ"]="ISO8859-2";
        g_locMap["czech"]="ISO8859-2";
        g_locMap["hr"]="ISO8859-2";
        g_locMap["hr_HR"]="ISO8859-2";
        g_locMap["hu"]="ISO8859-2";
        g_locMap["hu_HU"]="ISO8859-2";
        g_locMap["hungarian"]="ISO8859-2";
        g_locMap["pl"]="ISO8859-2";
        g_locMap["pl_PL"]="ISO8859-2";
        g_locMap["polish"]="ISO8859-2";
        g_locMap["ro"]="ISO8859-2";
        g_locMap["ro_RO"]="ISO8859-2";
        g_locMap["rumanian"]="ISO8859-2";
        g_locMap["serbocroatian"]="ISO8859-2";
        g_locMap["sh"]="ISO8859-2";
        g_locMap["sh_SP"]="ISO8859-2";
        g_locMap["sh_YU"]="ISO8859-2";
        g_locMap["sk"]="ISO8859-2";
        g_locMap["sk_SK"]="ISO8859-2";
        g_locMap["sl"]="ISO8859-2";
        g_locMap["sl_CS"]="ISO8859-2";
        g_locMap["sl_SI"]="ISO8859-2";
        g_locMap["slovak"]="ISO8859-2";
        g_locMap["slovene"]="ISO8859-2";
        g_locMap["sr_SP"]="ISO8859-2";

        g_locMap["eo"]="ISO8859-3";

        g_locMap["ee"]="ISO8859-4";
        g_locMap["ee_EE"]="ISO8859-4";

        g_locMap["mk"]="ISO8859-5";
        g_locMap["mk_MK"]="ISO8859-5";
        g_locMap["sp"]="ISO8859-5";
        g_locMap["sp_YU"]="ISO8859-5";

        g_locMap["ar_AA"]="ISO8859-6";
        g_locMap["ar_SA"]="ISO8859-6";
        g_locMap["arabic"]="ISO8859-6";

        g_locMap["el"]="ISO8859-7";
        g_locMap["el_GR"]="ISO8859-7";
        g_locMap["greek"]="ISO8859-7";

        g_locMap["hebrew"]="ISO8859-8";
        g_locMap["he"]="ISO8859-8";
        g_locMap["he_IL"]="ISO8859-8";
        g_locMap["iw"]="ISO8859-8";
        g_locMap["iw_IL"]="ISO8859-8";

        g_locMap["tr"]="ISO8859-9";
        g_locMap["tr_TR"]="ISO8859-9";
        g_locMap["turkish"]="ISO8859-9";

        g_locMap["lt"]="ISO8859-13";
        g_locMap["lt_LT"]="ISO8859-13";
        g_locMap["lv"]="ISO8859-13";
        g_locMap["lv_LV"]="ISO8859-13";

        g_locMap["et"]="ISO8859-15";
        g_locMap["et_EE"]="ISO8859-15";
        g_locMap["br_FR"]="ISO8859-15";
        g_locMap["ca_ES"]="ISO8859-15";
        g_locMap["de"]="ISO8859-15";
        g_locMap["de_AT"]="ISO8859-15";
        g_locMap["de_BE"]="ISO8859-15";
        g_locMap["de_DE"]="ISO8859-15";
        g_locMap["de_LU"]="ISO8859-15";
        g_locMap["en_IE"]="ISO8859-15";
        g_locMap["es"]="ISO8859-15";
        g_locMap["es_EC"]="ISO8859-15";
        g_locMap["es_ES"]="ISO8859-15";
        g_locMap["eu_ES"]="ISO8859-15";
        g_locMap["fi"]="ISO8859-15";
        g_locMap["fi_FI"]="ISO8859-15";
        g_locMap["finnish"]="ISO8859-15";
        g_locMap["fr"]="ISO8859-15";
        g_locMap["fr_FR"]="ISO8859-15";
        g_locMap["fr_BE"]="ISO8859-15";
        g_locMap["fr_LU"]="ISO8859-15";
        g_locMap["french"]="ISO8859-15";
        g_locMap["ga_IE"]="ISO8859-15";
        g_locMap["gl_ES"]="ISO8859-15";
        g_locMap["it"]="ISO8859-15";
        g_locMap["it_IT"]="ISO8859-15";
        g_locMap["oc_FR"]="ISO8859-15";
        g_locMap["nl"]="ISO8859-15";
        g_locMap["nl_BE"]="ISO8859-15";
        g_locMap["nl_NL"]="ISO8859-15";
        g_locMap["pt"]="ISO8859-15";
        g_locMap["pt_PT"]="ISO8859-15";
        g_locMap["sv_FI"]="ISO8859-15";
        g_locMap["wa_BE"]="ISO8859-15";

        g_locMap["uk"]="KOI8-U";
        g_locMap["uk_UA"]="KOI8-U";
        g_locMap["ru_YA"]="KOI8-U";
        g_locMap["ukrainian"]="KOI8-U";

        g_locMap["be"]="KOI8-R";
        g_locMap["be_BY"]="KOI8-R";
        g_locMap["bg"]="KOI8-R";
        g_locMap["bg_BG"]="KOI8-R";
        g_locMap["bulgarian"]="KOI8-R";
        g_locMap["ba_RU"]="KOI8-R";
        g_locMap["ky"]="KOI8-R";
        g_locMap["ky_KG"]="KOI8-R";
        g_locMap["kk"]="KOI8-R";
        g_locMap["kk_KZ"]="KOI8-R";
    }
    QByteArray l = g_locMap[locale];
    if (l.isEmpty()) {
        return "ISO8859-1";
    }
    return l;
}

QString RS_System::getHomeDir() {
    return QDir::homePath();
}

/**
 * @return Users home directory.
 */
QString RS_System::getTempDir() {
    return QDir::tempPath();
}

/**
 * @return Current directory.
 */
QString RS_System::getCurrentDir() {
    return QDir::currentPath();
}
