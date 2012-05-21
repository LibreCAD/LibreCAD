/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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


#ifndef RS_SYSTEM_H
#define RS_SYSTEM_H

#include <QDir>
#include <QList>
#include <QSharedPointer>

#include "rs_debug.h"
#include "rs_locale.h"

#define RS_SYSTEM RS_System::instance()

/**
 * Class for some system methods such as file system operations.
 * Implemented as singleton. Use init to Initialize the class
 * before you use it.
 *
 * @author Andrew Mustun
 */
class RS_System {
protected:
    RS_System() {
        initialized = false;
    }
//    ~RS_System() {
//        while (!allKnownLocales.isEmpty())
//             delete allKnownLocales.takeFirst();
//    }

public:
    /**
     * @return Instance to the unique system object.
     */
    static RS_System* instance() {
        if (uniqueInstance==NULL) {
            uniqueInstance = new RS_System();
        }
        return uniqueInstance;
    }

    void init(const QString& appName, const QString& appVersion,
                  const QString& appDirName, const QString& appDir="");
	void initLanguageList();
    void initAllLanguagesList();

    bool checkInit();
    bool createPaths(const QString& p);

    /**
     * @return Users home directory.
     */
    QString getHomeDir() {
        return QDir::homePath();
    }

    /**
     * @return Current directory.
     */
    QString getCurrentDir() {
        return QDir::currentPath();
    }

	/**
	 * @return Application directory.
	 */
    QString getAppDir() {
		return appDir;
	}

    /**
     * @return Application Data directory.
    */
    QString getAppDataDir();

    /**
     * @return A list of absolute paths to all font files found.
     */
    QStringList getFontList() {
        QStringList ret = getFileList("fonts", "cxf");
		return ret;
    }
	
    /**
     * @return A list of absolute paths to all NEW font files found.
     */
    QStringList getNewFontList() {
        QStringList ret = getFileList("fonts", "lff");
                return ret;
    }

    /**
     * @return A list of absolute paths to all hatch pattern files found.
     */
    QStringList getPatternList() {
        QStringList ret = getFileList("patterns", "dxf");
		return ret;
    }

    /**
     * @return A list of absolute paths to all script files found.
     */
    QStringList getScriptList() {
        QStringList ret = getFileList("scripts/qsa", "qs");
		return ret;
    }
	
    /**
     * @return A list of absolute paths to all machine configuration files found.
     */
    QStringList getMachineList() {
        QStringList ret = getFileList("machines", "cxm");
		return ret;
    }
	
    /**
     * @return Absolute path to the documentation.
     */
    QString getDocPath() {
        QStringList lst = getDirectoryList("doc");

        if( !(lst.isEmpty()) ){
            return lst.first();
        } else return QString();
    }

	/**
	 * @return The application name.
	 */
        QString getAppName() {
		return appName;
	}

	/**
	 * @return The application version.
	 */
        QString getAppVersion() {
		return appVersion;
	}

    QStringList getFileList(const QString& subDirectory,
                              const QString& fileExtension);
							  
    QStringList getDirectoryList(const QString& subDirectory);
							  
    QStringList getLanguageList() {
		return languageList;
	}
	
        static QString languageToSymbol(const QString& lang);
        static QString symbolToLanguage(const QString& symb);

        static QString getEncoding(const QString& str);

        void loadTranslation(const QString& lang, const QString& langCmd);

    static bool test();

	/** Returns ISO code for given locale. Needed for win32 to convert 
	 from system encodings. */
        static QByteArray localeToISO(const QByteArray& locale);

    private:
    void addLocale(RS_Locale *locale);

protected:
    static RS_System* uniqueInstance;

    QString appName;
    QString appVersion;
    QString appDirName;
    QString appDir;
	
	//! List of available translations

    QStringList languageList;
    bool initialized;
    QList<QSharedPointer<RS_Locale> > allKnownLocales;

};

#endif

