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


#ifndef RS_FONTLIST_H
#define RS_FONTLIST_H


#include <QList>
class RS_Font;

#define RS_FONTLIST RS_FontList::instance()

/**
 * The global list of fonts. This is implemented as a singleton.
 * Use RS_FontList::instance() to get a pointer to the object.
 *
 * @author Andrew Mustun
 */
class RS_FontList {
protected:
    RS_FontList();

public:
    /**
     * @return Instance to the unique font list.
     */
    static RS_FontList* instance() {
        if (uniqueInstance==NULL) {
            uniqueInstance = new RS_FontList();
        }
        return uniqueInstance;
    }

    virtual ~RS_FontList() {}

    void init();

    void clearFonts();
    int countFonts() {
        return fonts.count();
    }
    virtual void removeFont(RS_Font* font);
    RS_Font* requestFont(const QString& name);

    //! @return a const iterator for the font list.
    QListIterator<RS_Font *> getIteretor(){
        return QListIterator<RS_Font *>(fonts);
    }

    friend std::ostream& operator << (std::ostream& os, RS_FontList& l);

protected:
    static RS_FontList* uniqueInstance;

private:
    //! fonts in the graphic
    QList<RS_Font *> fonts;
};

#endif
