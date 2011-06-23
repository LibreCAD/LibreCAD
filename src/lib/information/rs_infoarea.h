/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
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

#ifndef RS_INFOAREA_H
#define RS_INFOAREA_H

#include <QVector>
#include "rs_vector.h"


/**
 * Class for getting information about an area. 
 *
 * @author Andrew Mustun
 */
class RS_InfoArea {
public: 
	RS_InfoArea();
	~RS_InfoArea();

	void reset();
	void addPoint(const RS_Vector& p);
	void calculate();
	void close();
	bool isValid();
	bool isClosed();
	double getArea() { 
		return area; 
	}
	double getCircumference() { 
		return circumference; 
	}
	int count() { 
		return thePoints.count(); 
	}

private:
	double calcSubArea(const RS_Vector& p1, const RS_Vector& p2);

        QVector<RS_Vector> thePoints;
	double baseY;
	double area;
	double circumference;
};

#endif
