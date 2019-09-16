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
#ifndef LC_FONTCREATION_H
#define LC_FONTCREATION_H
#include <QString>

/**
 * Class for the creation of new fonts.
 *
 * @author Shawn Curry
 */
class LC_FontCreation {
public:
    LC_FontCreation();
    virtual ~LC_FontCreation() = default;

	/**
	 * Creates an LFF for the specified font family, if a suitable font file can 
	 * be found in the standard font folders; and also adds it to the RS_FontList.
	 *
	 * @param fontFamily the font-family name
	 * @return true the font is ready to use in LibreCAD
	 */
	bool createFont(const QString& fontFamily);

	/**
	 * The "do-it" function.  Runs ttf2lff.exe with the specified parameters.
	 * @return true the LFF was created
	 */
	bool createFont(const QString& fontFileName, const QString& lff, const QString& author = "", const QString& license = "",
		double letter = 0.5, double word = 2, double line = 1);

	/**
	 * Search for a raw font file for the specified font-family.
	 */
	QString getFontFileName(const QString& family, const QString& fontDir = "", QHash<QString, int>* checked = nullptr);
	
	QString getFontFolder() { return fontFolder; }

private:
	bool containsFamily(const QString& fontFile, const QString& family);
	QString fontFolder;
	QStringList invalidFonts;
};

#endif // LC_FONTCREATION_H
