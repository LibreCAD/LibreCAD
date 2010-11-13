/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include <qapplication.h>
#include <qtextcodec.h>
//Added by qt3to4:
#include <Q3CString>
#include <QTranslator>
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_regexp.h"
#include "rs_translator.h"
#include "rs_fileinfo.h"
#include <QDesktopServices>

RS_System* RS_System::uniqueInstance = NULL;


/**
 * Initializes the system.
 *
 * @param appName Application name (e.g. "caduntu II")
 * @param appVersion Application version (e.g. "1.2.3")
 * @param appDirName Application directory name used for
 *     subdirectories in /usr, /etc ~/.
 * @param appDir Absolute application directory (e.g. /opt/qcad)
 *                 defaults to current directory.
 */
void RS_System::init(const RS_String& appName, const RS_String& appVersion,
                     const RS_String& appDirName, const RS_String& appDir) {
    this->appName = appName;
    this->appVersion = appVersion;
    this->appDirName = appDirName;
    if (appDir=="") {
        this->appDir = RS_Dir::currentDirPath();
    } else {
        this->appDir = appDir;
    }

    RS_DEBUG->print("RS_System::init: System %s initialized.", appName.latin1());
    RS_DEBUG->print("RS_System::init: App dir: %s", appDir.latin1());
    initialized = true;

    initLanguageList();
}



/**
 * Initializes the list of available translations.
 */
void RS_System::initLanguageList() {
    RS_DEBUG->print("RS_System::initLanguageList");
    RS_StringList lst = getFileList("qm", "qm");

    RS_SETTINGS->beginGroup("/Paths");
    lst += RS_StringList::split(";",
                                RS_SETTINGS->readEntry("/Translations", ""));
    RS_SETTINGS->endGroup();

    for (RS_StringList::Iterator it = lst.begin();
            it!=lst.end();
            ++it) {

        RS_DEBUG->print("RS_System::initLanguageList: qm file: %s",
                        (*it).latin1());

        int i1 = (*it).findRev('_');
        int i2 = (*it).find('.', i1);
        RS_String l = (*it).mid(i1+1, i2-i1-1);

        if (languageList.find(l)==languageList.end()) {
            RS_DEBUG->print("RS_System::initLanguageList: append language: %s",
                            l.latin1());
            languageList.append(l);
        }
    }
    RS_DEBUG->print("RS_System::initLanguageList: OK");
}



/**
 * Loads a different translation for the application GUI.
 */
void RS_System::loadTranslation(const RS_String& lang, const RS_String& langCmd) {
    static RS_Translator* tQt = NULL;
    static RS_Translator* tCaduntu = NULL;
    static RS_Translator* tCaduntuGuiQt = NULL;
    static RS_Translator* tCaduntuActions = NULL;
    static RS_Translator* tCaduntuCmd = NULL;
    static RS_Translator* tCaduntuLib = NULL;
    static RS_Translator* tCaduntuCam = NULL;
    static RS_Translator* tCaduntuProf = NULL;

    RS_String langFile;

    // search in various directories for translations
    RS_StringList lst = getDirectoryList("qm");

    RS_SETTINGS->beginGroup("/Paths");
    lst += RS_StringList::split(";",
                                RS_SETTINGS->readEntry("/Translations", ""));
    RS_SETTINGS->endGroup();

    for (RS_StringList::Iterator it = lst.begin();
            it!=lst.end();
            ++it) {

        langFile = "qt_" + lang + ".qm";
        if (tQt!=NULL) {
            qApp->removeTranslator(tQt);
            delete tQt;
        }
        tQt = new QTranslator(0);
        if (tQt->load(langFile, (*it))) {
            qApp->installTranslator(tQt);
        }


        langFile = "caduntu_" + lang + ".qm";
        if (tCaduntu!=NULL) {
            qApp->removeTranslator(tCaduntu);
            delete tCaduntu;
        }
        tCaduntu = new QTranslator(0);
        if (tCaduntu->load(langFile, (*it))) {
            qApp->installTranslator(tCaduntu);
        }


        langFile = "caduntuguiqt_" + lang + ".qm";
        if (tCaduntuGuiQt!=NULL) {
            qApp->removeTranslator(tCaduntuGuiQt);
            delete tCaduntuGuiQt;
        }
        tCaduntuGuiQt = new QTranslator(0);
        if (tCaduntuGuiQt->load(langFile, (*it))) {
            qApp->installTranslator(tCaduntuGuiQt);
        }


        langFile = "caduntuactions_" + lang + ".qm";
        if (tCaduntuActions!=NULL) {
            qApp->removeTranslator(tCaduntuActions);
            delete tCaduntuActions;
        }
        tCaduntuActions = new QTranslator(0);
        if (tCaduntuActions->load(langFile, (*it))) {
            qApp->installTranslator(tCaduntuActions);
        }

        langFile = "caduntucmd_" + langCmd + ".qm";
        if (tCaduntuCmd!=NULL) {
            qApp->removeTranslator(tCaduntuCmd);
            delete tCaduntuCmd;
        }
        tCaduntuCmd = new QTranslator(0);
        if (tCaduntuCmd->load(langFile, (*it))) {
            qApp->installTranslator(tCaduntuCmd);
        }

        langFile = "caduntulib_" + lang + ".qm";
        if (tCaduntuLib!=NULL) {
            qApp->removeTranslator(tCaduntuLib);
            delete tCaduntuLib;
        }
        tCaduntuLib = new QTranslator(0);
        if (tCaduntuLib->load(langFile, (*it))) {
            qApp->installTranslator(tCaduntuLib);
        }

        langFile = "caduntucam_" + lang + ".qm";
        if (tCaduntuLib!=NULL) {
            qApp->removeTranslator(tCaduntuCam);
            delete tCaduntuCam;
        }
        tCaduntuCam = new QTranslator(0);
        if (tCaduntuCam->load(langFile, (*it))) {
            qApp->installTranslator(tCaduntuCam);
        }

        langFile = "caduntuprof_" + lang + ".qm";
        if (tCaduntuProf!=NULL) {
            qApp->removeTranslator(tCaduntuProf);
            delete tCaduntuProf;
        }
        tCaduntuProf = new QTranslator(0);
        if (tCaduntuProf->load(langFile, (*it))) {
            qApp->installTranslator(tCaduntuProf);
        }
    }
}



/**
 * Checks if the system has been initialized and prints a warning
 * otherwise to stderr.
 */
bool RS_System::checkInit() {
    if (!initialized) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_System::checkInit: System not initialized.\n"
            "Use RS_SYSTEM->init(appname, appdirname) to do so.");
    }
    return initialized;
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
 * Searches for files in an application shared directory in the given
 * subdirectory with the given extension.
 *
 * @return List of the absolute paths of the files found.
 */
RS_StringList RS_System::getFileList(const RS_String& subDirectory,
                                     const RS_String& fileExtension) {

    checkInit();

        RS_DEBUG->print("RS_System::getFileList: subdirectory %s ", subDirectory.latin1());
        RS_DEBUG->print("RS_System::getFileList: appDirName %s ", appDirName.latin1());
        RS_DEBUG->print("RS_System::getFileList: getCurrentDir %s ", getCurrentDir().latin1());


    RS_StringList dirList = getDirectoryList(subDirectory);

    RS_StringList fileList;
    RS_String path;
    RS_Dir dir;

    for (RS_StringList::Iterator it = dirList.begin();
            it!=dirList.end();
            ++it ) {

        //path = RS_String(*it) + "/" + subDirectory;
        path = RS_String(*it);
        dir = RS_Dir(path);

        if (dir.exists() && dir.isReadable()) {
            RS_StringList files = dir.entryList("*." + fileExtension);
            for (RS_StringList::Iterator it2 = files.begin();
                    it2!=files.end();
                    it2++) {

                fileList += path + "/" + (*it2);
            }
        }
    }

    return fileList;
}



/**
 * @return List of all directories in subdirectory 'subDirectory' in
 * all possible QCad directories.
 */
RS_StringList RS_System::getDirectoryList(const RS_String& _subDirectory) {
    RS_StringList dirList;

    QString subDirectory=QDir::fromNativeSeparators(_subDirectory);

#ifdef Q_OS_MAC
        dirList.append(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/" + appDirName + "/" + subDirectory);
#endif
#ifdef Q_OS_WIN32
        dirList.append(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/" + appDirName + "/" + subDirectory);
#endif

        //local (application) directory has priority over other dirs:
        if (!appDir.isEmpty() && appDir!="/" && appDir!=getHomeDir()) {
            dirList.append(appDir + "/" + subDirectory);
        }

        // Redhat style:
        dirList.append("/usr/share/" + appDirName + "/" + subDirectory);

        // SuSE style:
        dirList.append("/usr/X11R6/" + appDirName + "/" + subDirectory);

        dirList.append("/usr/X11R6/share/" + appDirName + "/" + subDirectory);
        dirList.append(getHomeDir() + "/." + appDirName + "/" + subDirectory);


#ifdef Q_OS_MAC
    if (!appDir.isEmpty() && appDir!="/") {
        dirList.append(appDir + "/../Resources/" + subDirectory);
    }
#endif

#ifndef Q_OS_MAC
        // Add support directory if caduntu is run-in-place, not for Apple because it uses resources
        dirList.append(appDir + "/resources/" + subDirectory);
#endif

    // Individual directories:
    RS_SETTINGS->beginGroup("/Paths");
    if (subDirectory=="fonts") {
        dirList += RS_StringList::split(RS_RegExp("[;]"),
                                        RS_SETTINGS->readEntry("/Fonts", ""));
    } else if (subDirectory=="patterns") {
        dirList += RS_StringList::split(RS_RegExp("[;]"),
                                        RS_SETTINGS->readEntry("/Patterns", ""));
    } else if (subDirectory.startsWith("scripts")) {
        dirList += RS_StringList::split(RS_RegExp("[;]"),
                                        RS_SETTINGS->readEntry("/Scripts", ""));
    } else if (subDirectory.startsWith("library")) {
        dirList += RS_StringList::split(RS_RegExp("[;]"),
                                        RS_SETTINGS->readEntry("/Library", ""));
    } else if (subDirectory.startsWith("po")) {
        dirList += RS_StringList::split(RS_RegExp("[;]"),
                                        RS_SETTINGS->readEntry("/Translations", ""));
    }
    RS_SETTINGS->endGroup();

    RS_StringList ret;

    RS_DEBUG->print("RS_System::getDirectoryList: Paths:");
    for (RS_StringList::Iterator it = dirList.begin();
            it!=dirList.end(); ++it ) {

        if (RS_FileInfo(*it).isDir()) {
            ret += (*it);
            RS_DEBUG->print(*it);
        }
    }

    return ret;
}



/**
 * Converts a language string to a symbol (e.g. Deutsch or German to 'de').
 *
 * Supported languages: http://ftp.ics.uci.edu/pub/ietf/http/related/iso639.txt
 */
RS_String RS_System::languageToSymbol(const RS_String& lang) {
    RS_String l = lang.lower();

    // don't use else if.. M$ visual wannabe c++ can't handle it
    if (l=="afar") {
        return "aa";
    }
    if (l=="abkhazian") {
        return "ab";
    }
    if (l=="afrikaans") {
        return "af";
    }
    if (l=="amharic") {
        return "am";
    }
    if (l=="arabic") {
        return "ar";
    }
    if (l=="assamese") {
        return "as";
    }
    if (l=="aymara") {
        return "ay";
    }
    if (l=="azerbaijani") {
        return "az";
    }
    if (l=="bashkir") {
        return "ba";
    }
    if (l=="byelorussian") {
        return "be";
    }
    if (l=="bulgarian") {
        return "bg";
    }
    if (l=="bihari") {
        return "bh";
    }
    if (l=="bislama") {
        return "bi";
    }
    if (l=="bengali" || l=="bangla") {
        return "bn";
    }
    if (l=="tibetan") {
        return "bo";
    }
    if (l=="breton") {
        return "br";
    }
    if (l=="catalan") {
        return "ca";
    }
    if (l=="corsican") {
        return "co";
    }
    if (l=="czech") {
        return "cs";
    }
    if (l=="welsh") {
        return "cy";
    }
    if (l=="danish") {
        return "da";
    }
    if (l=="german" || l=="deutsch") {
        return "de";
    }
    if (l=="bhutani") {
        return "dz";
    }
    if (l=="greek") {
        return "el";
    }
    if (l=="english") {
        return "en";
    }
    if (l=="esperanto") {
        return "eo";
    }
    if (l=="spanish") {
        return "es";
    }
    if (l=="estonian") {
        return "et";
    }
    if (l=="basque") {
        return "eu";
    }
    if (l=="persian") {
        return "fa";
    }
    if (l=="finnish") {
        return "fi";
    }
    if (l=="fiji") {
        return "fj";
    }
    if (l=="faroese") {
        return "fo";
    }
    if (l=="french" || l=="francais") {
        return "fr";
    }
    if (l=="frisian") {
        return "fy";
    }
    if (l=="irish") {
        return "ga";
    }
    if (l=="scots gaelic" || l=="gaelic") {
        return "gd";
    }
    if (l=="galician") {
        return "gl";
    }
    if (l=="guarani") {
        return "gn";
    }
    if (l=="gujarati") {
        return "gu";
    }
    if (l=="hausa") {
        return "ha";
    }
    if (l=="hebrew") {
        return "he";
    }
    if (l=="hindi") {
        return "hi";
    }
    if (l=="croatian") {
        return "hr";
    }
    if (l=="hungarian") {
        return "hu";
    }
    if (l=="armenian") {
        return "hy";
    }
    if (l=="interlingua") {
        return "ia";
    }
    if (l=="indonesian") {
        return "id";
    }
    if (l=="interlingue") {
        return "ie";
    }
    if (l=="inupiak") {
        return "ik";
    }
    if (l=="icelandic") {
        return "is";
    }
    if (l=="italian") {
        return "it";
    }
    if (l=="inuktitut") {
        return "iu";
    }
    if (l=="japanese") {
        return "ja";
    }
    if (l=="javanese") {
        return "jw";
    }
    if (l=="georgian") {
        return "ka";
    }
    if (l=="kazakh") {
        return "kk";
    }
    if (l=="greenlandic") {
        return "kl";
    }
    if (l=="cambodian") {
        return "km";
    }
    if (l=="kannada") {
        return "kn";
    }
    if (l=="korean") {
        return "ko";
    }
    if (l=="kashmiri") {
        return "ks";
    }
    if (l=="kurdish") {
        return "ku";
    }
    if (l=="kirghiz") {
        return "ky";
    }
    if (l=="latin") {
        return "la";
    }
    if (l=="lingala") {
        return "ln";
    }
    if (l=="laothian") {
        return "lo";
    }
    if (l=="lithuanian") {
        return "lt";
    }
    if (l=="latvian" || l=="lettish") {
        return "lv";
    }
    if (l=="malagasy") {
        return "mg";
    }
    if (l=="maori") {
        return "mi";
    }
    if (l=="macedonian") {
        return "mk";
    }
    if (l=="malayalam") {
        return "ml";
    }
    if (l=="mongolian") {
        return "mn";
    }
    if (l=="moldavian") {
        return "mo";
    }
    if (l=="marathi") {
        return "mr";
    }
    if (l=="malay") {
        return "ms";
    }
    if (l=="maltese") {
        return "mt";
    }
    if (l=="burmese") {
        return "my";
    }
    if (l=="nauru") {
        return "na";
    }
    if (l=="nepali") {
        return "ne";
    }
    if (l=="dutch") {
        return "nl";
    }
    if (l=="norwegian") {
        return "no";
    }
    if (l=="occitan") {
        return "oc";
    }
    if (l=="afan" || l=="oromo" || l=="afan oromo") {
        return "om";
    }
    if (l=="oriya") {
        return "or";
    }
    if (l=="punjabi") {
        return "pa";
    }
    if (l=="polish") {
        return "pl";
    }
    if (l=="pashto" || l=="pushto") {
        return "ps";
    }
    if (l=="portuguese") {
        return "pt";
    }
    if (l=="brasilian portuguese") {
        return "pt-br";
    }
    if (l=="quechua") {
        return "qu";
    }
    if (l=="rhaeto-romance") {
        return "rm";
    }
    if (l=="kirundi") {
        return "rn";
    }
    if (l=="romanian") {
        return "ro";
    }
    if (l=="russian") {
        return "ru";
    }
    if (l=="kinyarwanda") {
        return "rw";
    }
    if (l=="sanskrit") {
        return "sa";
    }
    if (l=="sindhi") {
        return "sd";
    }
    if (l=="sangho") {
        return "sg";
    }
    if (l=="serbo-Croatian") {
        return "sh";
    }
    if (l=="sinhalese") {
        return "si";
    }
    if (l=="slovak") {
        return "sk";
    }
    if (l=="slovenian") {
        return "sl";
    }
    if (l=="samoan") {
        return "sm";
    }
    if (l=="shona") {
        return "sn";
    }
    if (l=="somali") {
        return "so";
    }
    if (l=="albanian") {
        return "sq";
    }
    if (l=="serbian") {
        return "sr";
    }
    if (l=="siswati") {
        return "ss";
    }
    if (l=="sesotho") {
        return "st";
    }
    if (l=="sundanese") {
        return "su";
    }
    if (l=="swedish") {
        return "sv";
    }
    if (l=="swahili") {
        return "sw";
    }
    if (l=="tamil") {
        return "ta";
    }
    if (l=="telugu") {
        return "te";
    }
    if (l=="tajik") {
        return "tg";
    }
    if (l=="thai") {
        return "th";
    }
    if (l=="tigrinya") {
        return "ti";
    }
    if (l=="turkmen") {
        return "tk";
    }
    if (l=="tagalog") {
        return "tl";
    }
    if (l=="setswana") {
        return "tn";
    }
    if (l=="tonga") {
        return "to";
    }
    if (l=="turkish") {
        return "tr";
    }
    if (l=="tsonga") {
        return "ts";
    }
    if (l=="tatar") {
        return "tt";
    }
    if (l=="twi") {
        return "tw";
    }
    if (l=="uighur") {
        return "ug";
    }
    if (l=="ukrainian") {
        return "uk";
    }
    if (l=="urdu") {
        return "ur";
    }
    if (l=="uzbek") {
        return "uz";
    }
    if (l=="vietnamese") {
        return "vi";
    }
    if (l=="volapuk") {
        return "vo";
    }
    if (l=="wolof") {
        return "wo";
    }
    if (l=="xhosa") {
        return "xh";
    }
    if (l=="yiddish") {
        return "yi";
    }
    if (l=="yoruba") {
        return "yo";
    }
    if (l=="zhuang") {
        return "za";
    }
    if (l=="chinese") {
        return "zh";
    }
    if (l=="zulu") {
        return "zu";
    }

    return "";
}



/**
 * Converst a language two-letter-code into a readable string
 * (e.g. 'de' to Deutsch)
 */
RS_String RS_System::symbolToLanguage(const RS_String& symb) {
    RS_String l = symb.lower();

    if (l=="aa") {
        return "Afar";
    }
    if (l=="ab") {
        return "Abkhazian";
    }
    if (l=="af") {
        return "Afrikaans";
    }
    if (l=="am") {
        return "Amharic";
    }
    if (l=="ar") {
        return "Arabic";
    }
    if (l=="as") {
        return "Assamese";
    }
    if (l=="ay") {
        return "Aymara";
    }
    if (l=="az") {
        return "Azerbaijani";
    }
    if (l=="ba") {
        return "Bashkir";
    }
    if (l=="be") {
        return "Byelorussian";
    }
    if (l=="bg") {
        return "Bulgarian";
    }
    if (l=="bh") {
        return "Bihari";
    }
    if (l=="bi") {
        return "Bislama";
    }
    if (l=="bn") {
        return "Bengali";
    }
    if (l=="bo") {
        return "Tibetan";
    }
    if (l=="br") {
        return "Breton";
    }
    if (l=="ca") {
        return "Catalan";
    }
    if (l=="co") {
        return "Corsican";
    }
    if (l=="cs") {
        return "Czech";
    }
    if (l=="cy") {
        return "Welsh";
    }
    if (l=="da") {
        return "Danish";
    }
    if (l=="de") {
        return "German";
    }
    if (l=="dz") {
        return "Bhutani";
    }
    if (l=="el") {
        return "Greek";
    }
    if (l=="en") {
        return "English";
    }
    if (l=="eo") {
        return "Esperanto";
    }
    if (l=="es") {
        return "Spanish";
    }
    if (l=="et") {
        return "Estonian";
    }
    if (l=="eu") {
        return "Basque";
    }
    if (l=="fa") {
        return "Persian";
    }
    if (l=="fi") {
        return "Finnish";
    }
    if (l=="fj") {
        return "Fiji";
    }
    if (l=="fo") {
        return "Faroese";
    }
    if (l=="fr") {
        return "French";
    }
    if (l=="fy") {
        return "Frisian";
    }
    if (l=="ga") {
        return "Irish";
    }
    if (l=="gd") {
        return "Scots Gaelic";
    }
    if (l=="gl") {
        return "Galician";
    }
    if (l=="gn") {
        return "Guarani";
    }
    if (l=="gu") {
        return "Gujarati";
    }
    if (l=="ha") {
        return "Hausa";
    }
    if (l=="he") {
        return "Hebrew";
    }
    if (l=="hi") {
        return "Hindi";
    }
    if (l=="hr") {
        return "Croatian";
    }
    if (l=="hu") {
        return "Hungarian";
    }
    if (l=="hy") {
        return "Armenian";
    }
    if (l=="ia") {
        return "Interlingua";
    }
    if (l=="id") {
        return "Indonesian";
    }
    if (l=="ie") {
        return "Interlingue";
    }
    if (l=="ik") {
        return "Inupiak";
    }
    if (l=="is") {
        return "Icelandic";
    }
    if (l=="it") {
        return "Italian";
    }
    if (l=="iu") {
        return "Inuktitut";
    }
    if (l=="ja") {
        return "Japanese";
    }
    if (l=="jw") {
        return "Javanese";
    }
    if (l=="ka") {
        return "Georgian";
    }
    if (l=="kk") {
        return "Kazakh";
    }
    if (l=="kl") {
        return "Greenlandic";
    }
    if (l=="km") {
        return "Cambodian";
    }
    if (l=="kn") {
        return "Kannada";
    }
    if (l=="ko") {
        return "Korean";
    }
    if (l=="ks") {
        return "Kashmiri";
    }
    if (l=="ku") {
        return "Kurdish";
    }
    if (l=="ky") {
        return "Kirghiz";
    }
    if (l=="la") {
        return "Latin";
    }
    if (l=="ln") {
        return "Lingala";
    }
    if (l=="lo") {
        return "Laothian";
    }
    if (l=="lt") {
        return "Lithuanian";
    }
    if (l=="lv") {
        return "Latvian";
    }
    if (l=="mg") {
        return "Malagasy";
    }
    if (l=="mi") {
        return "Maori";
    }
    if (l=="mk") {
        return "Macedonian";
    }
    if (l=="ml") {
        return "Malayalam";
    }
    if (l=="mn") {
        return "Mongolian";
    }
    if (l=="mo") {
        return "Moldavian";
    }
    if (l=="mr") {
        return "Marathi";
    }
    if (l=="ms") {
        return "Malay";
    }
    if (l=="mt") {
        return "Maltese";
    }
    if (l=="my") {
        return "Burmese";
    }
    if (l=="na") {
        return "Nauru";
    }
    if (l=="ne") {
        return "Nepali";
    }
    if (l=="nl") {
        return "Dutch";
    }
    if (l=="no") {
        return "Norwegian";
    }
    if (l=="oc") {
        return "Occitan";
    }
    if (l=="om") {
        return "Afan Oromo";
    }
    if (l=="or") {
        return "Oriya";
    }
    if (l=="pa") {
        return "Punjabi";
    }
    if (l=="pl") {
        return "Polish";
    }
    if (l=="ps") {
        return "Pashto";
    }
    if (l=="pt") {
        return "Portuguese";
    }
    if (l=="pt-br") {
        return "Brasilian Portuguese";
    }
    if (l=="qu") {
        return "Quechua";
    }
    if (l=="rm") {
        return "Rhaeto-Romance";
    }
    if (l=="rn") {
        return "Kirundi";
    }
    if (l=="ro") {
        return "Romanian";
    }
    if (l=="ru") {
        return "Russian";
    }
    if (l=="rw") {
        return "Kinyarwanda";
    }
    if (l=="sa") {
        return "Sanskrit";
    }
    if (l=="sd") {
        return "Sindhi";
    }
    if (l=="sg") {
        return "Sangho";
    }
    if (l=="sh") {
        return "Serbo-croatian";
    }
    if (l=="si") {
        return "Sinhalese";
    }
    if (l=="sk") {
        return "Slovak";
    }
    if (l=="sl") {
        return "Slovenian";
    }
    if (l=="sm") {
        return "Samoan";
    }
    if (l=="sn") {
        return "Shona";
    }
    if (l=="so") {
        return "Somali";
    }
    if (l=="sq") {
        return "Albanian";
    }
    if (l=="sr") {
        return "Serbian";
    }
    if (l=="ss") {
        return "Siswati";
    }
    if (l=="st") {
        return "Sesotho";
    }
    if (l=="su") {
        return "Sundanese";
    }
    if (l=="sv") {
        return "Swedish";
    }
    if (l=="sw") {
        return "Swahili";
    }
    if (l=="ta") {
        return "Tamil";
    }
    if (l=="te") {
        return "Telugu";
    }
    if (l=="tg") {
        return "Tajik";
    }
    if (l=="th") {
        return "Thai";
    }
    if (l=="ti") {
        return "Tigrinya";
    }
    if (l=="tk") {
        return "Turkmen";
    }
    if (l=="tl") {
        return "Tagalog";
    }
    if (l=="tn") {
        return "Setswana";
    }
    if (l=="to") {
        return "Tonga";
    }
    if (l=="tr") {
        return "Turkish";
    }
    if (l=="ts") {
        return "Tsonga";
    }
    if (l=="tt") {
        return "Tatar";
    }
    if (l=="tw") {
        return "Twi";
    }
    if (l=="ug") {
        return "Uighur";
    }
    if (l=="uk") {
        return "Ukrainian";
    }
    if (l=="ur") {
        return "Urdu";
    }
    if (l=="uz") {
        return "Uzbek";
    }
    if (l=="vi") {
        return "Vietnamese";
    }
    if (l=="vo") {
        return "Volapuk";
    }
    if (l=="wo") {
        return "Wolof";
    }
    if (l=="xh") {
        return "Xhosa";
    }
    if (l=="yi") {
        return "Yiddish";
    }
    if (l=="yo") {
        return "Yoruba";
    }
    if (l=="za") {
        return "Zhuang";
    }
    if (l=="zh") {
        return "Chinese";
    }
    if (l=="zu") {
        return "Zulu";
    }

    return "";
}



/**
 * Tries to convert the given encoding string to an encoding Qt knows.
 */
RS_String RS_System::getEncoding(const RS_String& str) {
    RS_String l=str.lower();

    if (l=="latin1" || l=="ansi_1252" || l=="iso-8859-1" ||
            l=="cp819" || l=="csiso" || l=="ibm819" || l=="iso_8859-1" ||
            l=="iso8859-1" || l=="iso-ir-100" || l=="l1") {
        return "Latin1";
    } else if (l=="big5" || l=="ansi_950" || l=="cn-big5" || l=="csbig5" ||
               l=="x-x-big5") {
        return "Big5";
    } else if (l=="big5-hkscs") {
        return "Big5-HKSCS";
    } else if (l=="eucjp" || l=="euc-jp" || l=="cseucpkdfmtjapanese" ||
               l=="x-euc" || l=="x-euc-jp") {
        return "eucJP";
    } else if (l=="euckr") {
        return "eucKR";
    } else if (l=="gb2312" || l=="gb2312" || l=="chinese" || l=="cn-gb" ||
               l=="csgb2312" || l=="csgb231280" || l=="csiso58gb231280" ||
               l=="gb_2312-80" || l=="gb231280" || l=="gb2312-80" || l=="gbk" ||
               l=="iso-ir-58") {
        return "GB2312";
    } else if (l=="gbk") {
        return "GBK";
    } else if (l=="gb18030") {
        return "GB18030";
    } else if (l=="jis7") {
        return "JIS7";
    } else if (l=="shift-jis" || l=="ansi_932" || l=="shift_jis" || l=="csShiftJIS" ||
               l=="cswindows31j" || l=="ms_kanji" || l=="x-ms-cp932" || l=="x-sjis") {
        return "Shift-JIS";
    } else if (l=="tscii") {
        return "TSCII";
    } else if (l=="utf88-bit") {
        return "utf88-bit";
    } else if (l=="utf16") {
        return "utf16";
    } else if (l=="koi8-r") {
        return "KOI8-R";
    } else if (l=="koi8-u") {
        return "KOI8-U";
    } else if (l=="iso8859-1") {
        return "ISO8859-1";
    } else if (l=="iso8859-2") {
        return "ISO8859-2";
    } else if (l=="iso8859-3") {
        return "ISO8859-3";
    } else if (l=="iso8859-4" || l=="ansi_1257") {
        return "ISO8859-4";
    } else if (l=="iso8859-5") {
        return "ISO8859-5";
    } else if (l=="iso8859-6" || l=="ansi_1256") {
        return "ISO8859-6";
    } else if (l=="iso8859-7" || l=="ansi_1253") {
        return "ISO8859-7";
    } else if (l=="iso8859-8") {
        return "ISO8859-8";
    } else if (l=="iso8859-8-i" || l=="ansi_1255") {
        return "ISO8859-8-i";
    } else if (l=="iso8859-9" || l=="ansi_1254") {
        return "ISO8859-9";
    } else if (l=="iso8859-10") {
        return "ISO8859-10";
    } else if (l=="iso8859-13") {
        return "ISO8859-13";
    } else if (l=="iso8859-14") {
        return "ISO8859-14";
    } else if (l=="iso8859-15") {
        return "ISO8859-15";
    } else if (l=="ibm 850") {
        return "IBM 850";
    } else if (l=="ibm 866") {
        return "IBM 866";
    } else if (l=="cp874") {
        return "CP874";
    } else if (l=="cp1250") {
        return "CP1250";
    } else if (l=="cp1251") {
        return "CP1251";
    } else if (l=="cp1252") {
        return "CP1252";
    } else if (l=="cp1253") {
        return "CP1253";
    } else if (l=="cp1254") {
        return "CP1254";
    } else if (l=="cp1255") {
        return "CP1255";
    } else if (l=="cp1256") {
        return "CP1256";
    } else if (l=="cp1257") {
        return "CP1257";
    } else if (l=="cp1258") {
        return "CP1258";
    } else if (l=="apple roman") {
        return "Apple Roman";
    } else if (l=="tis-620") {
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
static QMap<Q3CString,Q3CString> loc_map;

Q3CString RS_System::localeToISO(const Q3CString& locale) {
    if (loc_map.isEmpty()) {
        loc_map["croatian"]="ISO8859-2";
        loc_map["cs"]="ISO8859-2";
        loc_map["cs_CS"]="ISO8859-2";
        loc_map["cs_CZ"]="ISO8859-2";
        loc_map["cz"]="ISO8859-2";
        loc_map["cz_CZ"]="ISO8859-2";
        loc_map["czech"]="ISO8859-2";
        loc_map["hr"]="ISO8859-2";
        loc_map["hr_HR"]="ISO8859-2";
        loc_map["hu"]="ISO8859-2";
        loc_map["hu_HU"]="ISO8859-2";
        loc_map["hungarian"]="ISO8859-2";
        loc_map["pl"]="ISO8859-2";
        loc_map["pl_PL"]="ISO8859-2";
        loc_map["polish"]="ISO8859-2";
        loc_map["ro"]="ISO8859-2";
        loc_map["ro_RO"]="ISO8859-2";
        loc_map["rumanian"]="ISO8859-2";
        loc_map["serbocroatian"]="ISO8859-2";
        loc_map["sh"]="ISO8859-2";
        loc_map["sh_SP"]="ISO8859-2";
        loc_map["sh_YU"]="ISO8859-2";
        loc_map["sk"]="ISO8859-2";
        loc_map["sk_SK"]="ISO8859-2";
        loc_map["sl"]="ISO8859-2";
        loc_map["sl_CS"]="ISO8859-2";
        loc_map["sl_SI"]="ISO8859-2";
        loc_map["slovak"]="ISO8859-2";
        loc_map["slovene"]="ISO8859-2";
        loc_map["sr_SP"]="ISO8859-2";

        loc_map["eo"]="ISO8859-3";

        loc_map["ee"]="ISO8859-4";
        loc_map["ee_EE"]="ISO8859-4";

        loc_map["mk"]="ISO8859-5";
        loc_map["mk_MK"]="ISO8859-5";
        loc_map["sp"]="ISO8859-5";
        loc_map["sp_YU"]="ISO8859-5";

        loc_map["ar_AA"]="ISO8859-6";
        loc_map["ar_SA"]="ISO8859-6";
        loc_map["arabic"]="ISO8859-6";

        loc_map["el"]="ISO8859-7";
        loc_map["el_GR"]="ISO8859-7";
        loc_map["greek"]="ISO8859-7";

        loc_map["hebrew"]="ISO8859-8";
        loc_map["he"]="ISO8859-8";
        loc_map["he_IL"]="ISO8859-8";
        loc_map["iw"]="ISO8859-8";
        loc_map["iw_IL"]="ISO8859-8";

        loc_map["tr"]="ISO8859-9";
        loc_map["tr_TR"]="ISO8859-9";
        loc_map["turkish"]="ISO8859-9";

        loc_map["lt"]="ISO8859-13";
        loc_map["lt_LT"]="ISO8859-13";
        loc_map["lv"]="ISO8859-13";
        loc_map["lv_LV"]="ISO8859-13";

        loc_map["et"]="ISO8859-15";
        loc_map["et_EE"]="ISO8859-15";
        loc_map["br_FR"]="ISO8859-15";
        loc_map["ca_ES"]="ISO8859-15";
        loc_map["de"]="ISO8859-15";
        loc_map["de_AT"]="ISO8859-15";
        loc_map["de_BE"]="ISO8859-15";
        loc_map["de_DE"]="ISO8859-15";
        loc_map["de_LU"]="ISO8859-15";
        loc_map["en_IE"]="ISO8859-15";
        loc_map["es"]="ISO8859-15";
        loc_map["es_ES"]="ISO8859-15";
        loc_map["eu_ES"]="ISO8859-15";
        loc_map["fi"]="ISO8859-15";
        loc_map["fi_FI"]="ISO8859-15";
        loc_map["finnish"]="ISO8859-15";
        loc_map["fr"]="ISO8859-15";
        loc_map["fr_FR"]="ISO8859-15";
        loc_map["fr_BE"]="ISO8859-15";
        loc_map["fr_LU"]="ISO8859-15";
        loc_map["french"]="ISO8859-15";
        loc_map["ga_IE"]="ISO8859-15";
        loc_map["gl_ES"]="ISO8859-15";
        loc_map["it"]="ISO8859-15";
        loc_map["it_IT"]="ISO8859-15";
        loc_map["oc_FR"]="ISO8859-15";
        loc_map["nl"]="ISO8859-15";
        loc_map["nl_BE"]="ISO8859-15";
        loc_map["nl_NL"]="ISO8859-15";
        loc_map["pt"]="ISO8859-15";
        loc_map["pt_PT"]="ISO8859-15";
        loc_map["sv_FI"]="ISO8859-15";
        loc_map["wa_BE"]="ISO8859-15";

        loc_map["uk"]="KOI8-U";
        loc_map["uk_UA"]="KOI8-U";
        loc_map["ru_YA"]="KOI8-U";
        loc_map["ukrainian"]="KOI8-U";

        loc_map["be"]="KOI8-R";
        loc_map["be_BY"]="KOI8-R";
        loc_map["bg"]="KOI8-R";
        loc_map["bg_BG"]="KOI8-R";
        loc_map["bulgarian"]="KOI8-R";
        loc_map["ba_RU"]="KOI8-R";
        loc_map["ky"]="KOI8-R";
        loc_map["ky_KG"]="KOI8-R";
        loc_map["kk"]="KOI8-R";
        loc_map["kk_KZ"]="KOI8-R";
    }
    Q3CString l = loc_map[locale];
    if (l.isEmpty())
        return "ISO8859-1";
    return l;
}
