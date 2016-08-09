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


#ifndef RS_PREVIEW_H
#define RS_PREVIEW_H

#include "rs_entitycontainer.h"

/**
 * This class supports previewing. The RS_Snapper class uses
 * an instance of RS_Preview to preview entities, ranges, 
 * lines, arcs, ... on the fly.
 *
 * @author Andrew Mustun
 */
class RS_Preview : public RS_EntityContainer {
public:
    RS_Preview(RS_EntityContainer* parent=nullptr);
	~RS_Preview() = default;
    virtual RS2::EntityType rtti() const override{
        return RS2::EntityPreview;
    }
    virtual void addEntity(RS_Entity* entity) override;
    void addCloneOf(RS_Entity* entity);
    virtual void addSelectionFrom(RS_EntityContainer& container);
    virtual void addAllFrom(RS_EntityContainer& container);
    virtual void addStretchablesFrom(RS_EntityContainer& container,
           const RS_Vector& v1, const RS_Vector& v2);

    void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;

private:
	int maxEntities;
};

#endif
