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

#ifndef RS_ACTIONSELECTSINGLE_H
#define RS_ACTIONSELECTSINGLE_H

#include "rs_actionselectbase.h"

/**
 * This action class can handle user events to select entities.
 *
 * @author Andrew Mustun
 */
class LC_ActionSelectSingle:public RS_ActionSelectBase {
    Q_OBJECT
public:
    explicit LC_ActionSelectSingle(LC_ActionContext *actionContext,
                                   RS_ActionInterface *actionSelect = nullptr,
                                   const QList<RS2::EntityType> &entityTypeList = {});
    LC_ActionSelectSingle(RS2::EntityType typeToSelect,LC_ActionContext *actionContext,
                    RS_ActionInterface *actionSelect = nullptr,
                    const QList<RS2::EntityType> &entityTypeList = {});
    void trigger() override;
    RS2::EntityType getTypeToSelect() const;
protected:
    bool m_selectContour = false;
    RS_Entity *m_entityToSelect = nullptr;
    RS_ActionInterface *m_actionSelect = nullptr;
    RS2::EntityType m_typeToSelect = RS2::EntityType::EntityUnknown;
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* event) override;
    bool isEntityAllowedToSelect(RS_Entity* ent) const override;
    void selectionFinishedByKey(QKeyEvent *e, bool escape) override;
    void doSelectEntity(RS_Entity *entityToSelect, bool selectContour) const override;
    void updateActionPrompt() override;
    bool doTriggerModifications([[maybe_unused]]LC_DocumentModificationBatch& modificationData) override{return true;}
};

#endif
