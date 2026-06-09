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

#ifndef RS_ACTIONSELECTWINDOW_H
#define RS_ACTIONSELECTWINDOW_H

#include "lc_overlayboxaction.h"
#include "rs_actionselectbase.h"

/**
 * This action class can handle user events to select all entities.
 *
 * @author Andrew Mustun
 */
class LC_ActionSelectWindow:public RS_ActionSelectBase {
    Q_OBJECT
public:
    LC_ActionSelectWindow(LC_ActionContext *actionContext,bool select);
    LC_ActionSelectWindow(RS2::EntityType typeToSelect, LC_ActionContext* actionContext);
    ~LC_ActionSelectWindow() override;
    void init(int status) override;
    bool isSelectAllEntityTypes() const;
    void setSelectAllEntityTypes(bool val);
    QList<RS2::EntityType> getEntityTypesToSelect();
    void setEntityTypesToSelect(const QList<RS2::EntityType>& types);
protected:
    /**
 * Action States.
 */
    enum Status {
        SetCorner1,     /**< Setting the 1st corner of the window.  */
        SetCorner2      /**< Setting the 2nd corner of the window. */
    };

    struct Points;
    std::unique_ptr<Points> m_actionData;
    bool m_select = false;
    bool m_selectIntersecting = false;
    bool m_invertSelectionOperation = false;
    bool m_selectAllEntityTypes = true;
    QList<RS2::EntityType> m_entityTypesToSelect;

    LC_ActionSelectWindow(const QString& name, LC_ActionContext* actionContext, RS2::ActionType actionType, RS2::EntityType typeToSelect, bool select);
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonPress(int status, const LC_MouseEvent* e) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void updateActionPrompt() override;
    void doTrigger() override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void selectionFinishedByKey(QKeyEvent* e, bool escape) override;
    bool isAllowSelectionFinishByEnterForEmptySelection() override {return true;}
    bool doTriggerModifications([[maybe_unused]]LC_DocumentModificationBatch& modificationData) override {return true;}
    void doSaveOptions() override;
    void doLoadOptions() override;
};
#endif
