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

#ifndef RS_SELECTION_H
#define RS_SELECTION_H

class QString;

class RS_EntityContainer;
class RS_Graphic;
class RS_GraphicView;
class RS_Vector;
class RS_Entity;

/**
 * API Class for selecting entities. 
 * There's no interaction handled in this class.
 * This class is connected to an entity container and
 * can be connected to a graphic view.
 *
 * @author Andrew Mustun
 */
class RS_Selection {
public:
    RS_Selection(RS_EntityContainer& entityContainer,
                 RS_GraphicView* graphicView=nullptr);

    void selectSingle(RS_Entity* e);
    void selectAll(bool select=true);
    void deselectAll() {
        selectAll(false);
    }
    void invertSelection();
    void selectWindow(enum RS2::EntityType typeToSelect, const RS_Vector& v1, const RS_Vector& v2,
                      bool select=true, bool cross=false);
    void deselectWindow(enum RS2::EntityType typeToSelect,const RS_Vector& v1, const RS_Vector& v2) {
        selectWindow(typeToSelect,v1, v2, false);
    }
    void selectIntersected(const RS_Vector& v1, const RS_Vector& v2,
                      bool select=true);
    void deselectIntersected(const RS_Vector& v1, const RS_Vector& v2) {
		selectIntersected(v1, v2, false);
	}
    void selectContour(RS_Entity* e);
	
    void selectLayer(RS_Entity* e);
    void selectLayer(const QString& layerName, bool select=true);
    void deselectLayer(QString& layerName) {
		selectLayer(layerName, false);
	}

protected:
    RS_EntityContainer* container = nullptr;
    RS_Graphic* graphic = nullptr;
    RS_GraphicView* graphicView = nullptr;
};

#endif
