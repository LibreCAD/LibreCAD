/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)
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

#ifndef RS_LOCALE_H
#define RS_LOCALE_H

#include <QLocale>
#include "rs.h"

/**
  * Store Locale information in a class
  * (c) 2011 R. van Twisk
  *
  **/
class RS_Locale :public QLocale {
public:
    RS_Locale();
    RS_Locale(const QString &_canonical);
	/* without virtual destructor => warning: deleting object of polymorphic  
	 * class type 'RS_Locale' which has non-virtual destructor might cause 
	 * undefined behaviour [-Wdelete-non-virtual-dtor]
	 * 
	 * TNick <nicu.tofan@gmail.com>
	 */
	virtual ~RS_Locale(){}
	

    virtual void setCanonical(const QString &_canonical);
    virtual void setDirection(RS2::TextLocaleDirection direction);
    virtual void setName(const QString &_name);
    virtual QString name() const;
    static QString toCanonical(const QString &canonical);

    QString getCanonical();
    QString getName();

protected:
    QString canonical;
    QString localeName;
    int direction;
};



#endif
