/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Shawn Curry (noneyabiz@mail.wasent.cz)
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
#include <QApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QFontDatabase>
#include "rs_settings.h"
#include "rs_font.h"
#include "rs_fontlist.h"
#include "rs_system.h"
#include "lc_fontcreation.h"
#include "qc_applicationwindow.h"
#include "rs_document.h"
#include "lc_telemetry.h"

LC_FontCreation::LC_FontCreation()
{
	RS_SETTINGS->beginGroup("/Paths");
	fontFolder = RS_SETTINGS->readEntry("/Fonts", "").trimmed();
	if (fontFolder.isEmpty() || !RS_SYSTEM->isWritable(fontFolder)) {
		QFileInfo info(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)), "LibreCAD/fonts");
		QDir dir;		
		fontFolder = info.filePath();		
		dir.mkpath(fontFolder);		
		RS_SETTINGS->writeEntry("/Fonts", QDir::toNativeSeparators(fontFolder));
	}
	RS_SETTINGS->endGroup();
}

bool LC_FontCreation::createFont(const QString & fontFamily)
{
	if (fontFolder.isEmpty() || fontFamily.isEmpty() || invalidFonts.contains(fontFamily))
		return false;
	if (RS_FONTLIST->containsFont(fontFamily))
		return true;
	QFileInfo info = QFileInfo(QDir(fontFolder), fontFamily + ".lff");
	QString lff = info.filePath();
	if (createFont(getFontFileName(fontFamily), lff)) {
		RS_FONTLIST->addFont(new RS_Font(lff, true));
		return true;
	}
	invalidFonts.push_back(fontFamily);
	return false;
}

bool LC_FontCreation::createFont(const QString& fontFileName, const QString& lff, const QString& author, const QString& license,
	double letter, double word, double line)
{
	if (!fontFileName.isEmpty() && !lff.isEmpty()) {
		QProcess proc;
		QDir dir = QDir::cleanPath(QCoreApplication::applicationDirPath());
		bool shx = fontFileName.toLower().endsWith(".shp");
		QFileInfo info = QFileInfo(dir, shx ? "s2f.exe" : "ttf2lff.exe");
		QString command = "\"" + info.filePath() + "\"";
		
		if (shx) {//S2F -i=<input filename> [-o=<output filename>] [-s=<character spacing>] [-silent]
			command +=
				QString(" -i=\"%1\"").arg(fontFileName) +
				QString(" -o=\"%1\"").arg(lff) +
				QString(" -s=%1").arg(letter);
		} else {
			command +=
				QString(" -l %1").arg(letter) +
				QString(" -w %1").arg(word) +
				QString(" -f %1").arg(line);

			if (!author.isEmpty())
				command += QString(" -a \"%1\"").arg(author);
			if (!license.isEmpty())
				command += QString(" -L \"%1\"").arg(license);

			command +=
				QString(" \"%1\"").arg(fontFileName) +
				QString(" \"%1\"").arg(lff);
		}
		proc.start(command);
		proc.waitForFinished();
	}

	bool result = QFile::exists(lff);
	if (result) {
		RS_Document *doc = QC_ApplicationWindow::getAppWindow()->getDocument();
		if (fontFileName.endsWith(".shp", Qt::CaseInsensitive))
			doc->getTelemetryData().shxFontsConverted++;
		else
			doc->getTelemetryData().ttfFontsConverted++;
	}

	return QFile::exists(lff);
}

/**
 * Locate the file name for the given font family, in the specified font folder.
 * Sub folders are searched recursively.  If the font folder is blank, the standard
 * font folders are searched.
 *
 * This is an expensive operation.  Each file must be read into the QFontDatabase to
 * determine if it supports the given font family.  The name must be an exact match.
 * In order to avoid scanning every font file in the directory tree, a first pass is
 * made to scan for any likely candidates, before resorting to a brute force approach.
 */
QString LC_FontCreation::getFontFileName(const QString & family, const QString & fontDir, QHash<QString, int>* checked)
{
	QString result = "";

	if (fontDir.isEmpty()) {
		QHash<QString, int> map;
		result = getFontFileName(family, fontFolder, &map);
		if (!result.isEmpty())
			return result;
		for (auto dirName : QStandardPaths::standardLocations(QStandardPaths::FontsLocation)) {
			result = getFontFileName(family, dirName, &map);
			if (!result.isEmpty())
				return result;
		}
	} else {
		QDir dir(fontDir);
		QString guess = family.chopped(family.size() - 3);
		checked->insert(fontDir, 1);
		// make a couple of guesses before scanning every font in the folder tree
		for (auto dirInfo : dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::System)) {
			QString path = dirInfo.filePath();
			if (path.contains(guess, Qt::CaseInsensitive)) {
				checked->insert(path, 1);
				result = getFontFileName(family, path, checked);
				if (!result.isEmpty())
					return result;
			}
		}
		for (auto fileInfo : dir.entryInfoList(QDir::Files)) {
			QString filePath = fileInfo.filePath();
			if (fileInfo.baseName().contains(guess, Qt::CaseInsensitive)) {
				checked->insert(filePath, 1);
				if (containsFamily(filePath, family))
					return filePath;
			}
		}

		// brute force
		for (auto fileInfo : dir.entryInfoList(QDir::Files)) {
			QString filePath = fileInfo.filePath();
			if (!checked->contains(filePath)) {
				if (containsFamily(filePath, family))
					return filePath;
			}
		}
		for (auto dirInfo : dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::System)) {
			QString path = dirInfo.filePath();
			if (!checked->contains(path)) {
				result = getFontFileName(family, path, checked);
				if (!result.isEmpty())
					return result;
			}
		}
	}

	return result;
}

bool LC_FontCreation::containsFamily(const QString & fontFile, const QString & family)
{
	if (fontFile.endsWith("shp", Qt::CaseInsensitive)) {
		QFileInfo info(fontFile);
		return info.baseName().toLower() == family.toLower();
	}	

	int id = QFontDatabase::addApplicationFont(fontFile);
	for (auto fam : QFontDatabase::applicationFontFamilies(id)) {
		if (family.compare(fam, Qt::CaseInsensitive) == 0) {
			QFontDatabase::removeApplicationFont(id);
			return true;
		}
	}
	QFontDatabase::removeApplicationFont(id);
	return false;
}
