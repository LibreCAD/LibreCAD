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
#ifndef RS_ACTIONPOLYLINESEGMENT_H
#define RS_ACTIONPOLYLINESEGMENT_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

class RS_Entity;
class RS_EntityContainer;
class RS_GraphicView;
class RS_Polyline;
class RS_Vector;

/**
 * This action class can handle Create Polyline Existing from Segments
 *
 * @author Andrew Mustun
 */
class LC_ActionPolylineFromSegment:public LC_UndoableDocumentModificationAction {
    Q_OBJECT
public:
    explicit LC_ActionPolylineFromSegment(LC_ActionContext *actionContext);
    LC_ActionPolylineFromSegment(LC_ActionContext *actionContext, RS_Entity *target);
    void init(int status) override;
    void drawSnapper() override;
protected:
    /**
     * Action States.
     */
    enum Status {
        ChooseEntity = InitialActionStatus /**< Choosing one of the polyline segments. */
    };

    RS_Entity *m_targetEntity = nullptr;
    bool m_initWithTarget{false};

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;

    //! create polyline from segments
//! @param selectedEntity
//! @param useSelected only create from selected entities
//! @param ctx
    void convertPolyline(RS_Entity* selectedEntity, bool useSelected, LC_DocumentModificationBatch& ctx) const;
    RS_Vector appendPol(RS_Polyline *current, const RS_Polyline* toAdd, bool reversed) const;

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* event) override;
    void updateActionPrompt() override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;
};
#endif
