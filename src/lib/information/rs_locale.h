/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include <QString>
#include "rs.h"

/**
  * Store Locale information in a class
  * (c) 2011 R. van Twisk
  *
  **/
class RS_Locale {
public:
    RS_Locale();

    virtual void setCanonical(const QString &_canonical);
    virtual void setDirection(RS2::TextLocaleDirection direction);
    virtual void setName(const QString &_name);

    QString getCanonical();
    QString getName();

protected:
    QString canonical;
    QString name;
    int direction;
};



#endif
