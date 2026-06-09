/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef RS_PYTHON_H
#define RS_PYTHON_H

#ifdef RS_OPT_PYTHON

#include "Python.h"

#include "rs_graphic.h"

#define RS_PYTHON RS_Python::instance()

/**
 * Python scripting support.
 *
 * OBSOLETE
 *
 * @author Andrew Mustun
 */
class RS_Python {
private:
    RS_Python();

public:
    static RS_Python* instance();

    void setGraphic(RS_Graphic* g) {
        graphic = g;
    }

    RS_Graphic* getGraphic() {
        return graphic;
    }

    int launch(const QString& script);

private:
    static RS_Python* uniqueInstance;
    RS_Graphic* graphic;
};

#endif

#endif
