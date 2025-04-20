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

class LC_GraphicViewport;

/**
 * This class supports previewing. The RS_Snapper class uses
 * an instance of RS_Preview to preview entities, ranges, 
 * lines, arcs, ... on the fly.
 *
 * @author Andrew Mustun
 */
class RS_Preview : public RS_EntityContainer {
public:
    RS_Preview(RS_EntityContainer* parent, LC_GraphicViewport* viewport);
    RS2::EntityType rtti() const override{
        return RS2::EntityPreview;
    }
    void addEntity(RS_Entity* entity) override;
    void calcRectCorners(const RS_Vector& worldCorner1, const RS_Vector& worldCorner3, RS_Vector& worldCorner2,
                         RS_Vector& worldCorner4) const;
    void addCloneOf(RS_Entity* entity, LC_GraphicViewport* view);
    void addSelectionFrom(RS_EntityContainer& container, LC_GraphicViewport* view);
    void addAllFrom(RS_EntityContainer& container, LC_GraphicViewport* view);
    void addStretchablesFrom(RS_EntityContainer& container, LC_GraphicViewport* view,
                                     const RS_Vector& v1, const RS_Vector& v2);
    void draw(RS_Painter* painter) override;
    void addReferenceEntitiesToContainer(RS_EntityContainer* container);
    void clear() override;
    int getMaxAllowedEntities();
private:
    unsigned int m_maxEntities {0};
    QList<RS_Entity*> m_referenceEntities;
    LC_GraphicViewport* m_viewport {nullptr};
};
#endif
