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

#ifndef RS_SIMPLEPYTHON_H
#define RS_SIMPLEPYTHON_H

#ifdef RS_OPT_SIMPLEPYTHON

#include "Python.h"

#include "rs_graphic.h"

#define RS_SIMPLEPYTHON RS_SimplePython::instance()

/**
 * Python scripting support.
 *
 * OBSOLETE
 *
 * @author Andrew Mustun
 */
class RS_SimplePython {
private:
    RS_SimplePython() {
        graphic = NULL;
    }

public:
    static RS_SimplePython* instance();

    void setGraphic(RS_Graphic* g) {
        graphic = g;
    }

    RS_Graphic* getGraphic() {
        return graphic;
    }

    int launch(const QString& script);

private:
    static RS_SimplePython* uniqueInstance;

    RS_Graphic* graphic;
};



/**
 * Global method needed by the python lib for initialisation.
 */
void init_pyextension();

/**
 * Test method.
 */
long inc(long i);

/**
 * Adds a line to the current graphic document.
 */
void rsPyAddLine(double x1, double y1, double x2, double y2);

#endif

#endif
