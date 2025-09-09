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

#ifndef RS_ACTIONDEFAULT_H
#define RS_ACTIONDEFAULT_H

#include <memory>
#include "lc_overlayboxaction.h"


/**
 * This action class can handle user events to select all entities.
 *
 * @author Andrew Mustun
 */
//todo - support of moving by keyboard
//todo - joint move of endpoint ref? So if moving ref will move refs for adjusent entities
class RS_ActionDefault : public LC_OverlayBoxAction {
    Q_OBJECT

    using BASE_CLASS = RS_PreviewActionInterface;

public:
    RS_ActionDefault(LC_ActionContext *actionContext);
    ~RS_ActionDefault() override;

    void finish(bool) override{}

    void init(int status) override;
    void resume() override;
    void suspend() override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;
    QStringList getAvailableCommands() override;

    // clear temporary entities for highlighting
    void clearHighLighting();
    enum RS2::EntityType getTypeToSelect();
protected:
    /**
    * Action States.
    */
    enum Status {
        Neutral = InitialActionStatus,        /**< we don't know what we do yet.  */
        Dragging,       /**< dragging (either an entity or the
                                             first part of a selection window) */
        SetCorner2,     /**< Setting the 2nd corner of a selection window. */
        Moving,         /**< Moving entities (drag'n'drop) */
        MovingRef,       /**< Moving a reference point of one or more selected
                                             entities */
        Panning /**< view panning triggered by Ctl- mouse dragging */
    };


    void checkSupportOfQuickEntityInfo();
    void clearQuickInfoWidget();
    void updateQuickInfoWidget(RS_Entity *pEntity);
    void goToNeutralStatus();
    RS2::CursorType doGetMouseCursor(int status) override;

    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onMouseLeftButtonPress(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonPress(int status, LC_MouseEvent *e) override;

    void highlightHoveredEntities(LC_MouseEvent* currentMousePosition);
    void highlightEntity(RS_Entity* entity);
    void updateMouseButtonHints() override;
    void createEditedLineDescription(RS_Line* clone, bool ctrlPressed, bool shiftPressed);
    void createEditedArcDescription(RS_Arc* clone, bool ctrlPressed, bool shiftPressed);
    void createEditedCircleDescription(RS_Circle* clone, bool ctrlPressed, bool shiftPressed);
    bool isShowEntityDescriptionOnHighlight();
    void forceUpdateInfoCursor(const LC_MouseEvent *event);
    RS_Entity* getClone(RS_Entity* e);

private:

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    RS2::SnapRestriction m_snapRestriction = RS2::RestrictNothing;
    RS2::EntityType m_typeToSelect = RS2::EntityType::EntityUnknown;

    bool allowEntityQuickInfoForCTRL = false;
    bool allowEntityQuickInfoAuto = false;
    bool m_selectWithPressedMouseOnly = true; // fixme - sand - retrieve from setting (for backward compatibility or rather historic bug support)
};
#endif
