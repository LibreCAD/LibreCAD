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

#ifndef QG_QT2RS_H
#define QG_QT2RS_H

#include <qnamespace.h>

#include "rs_mouseevent.h"

/**
 * Translates Qt stuff into caduntu stuff.
 */
class QG_Qt2Rs {

public:
    QG_Qt2Rs() {}
    ~QG_Qt2Rs() {}

    /*static RS_MouseEvent mouseEvent(QMouseEvent* e) {
    	RS_MouseButton button;
    	switch (e->button()) {
    		case Qt::LeftButton:
    			button = LEFT;
    			break;
    		case Qt::RightButton:
    			button = RIGHT;
    			break;
    		case Qt::MidButton:
    			button = MIDDLE;
    			break;
    		default:
    			button = NONE;
    			break;
    	}

    	return RS_MouseEvent(e->pos().x(), 
                                e->pos().y(), 
                                button );
}*/
}
;

#endif

