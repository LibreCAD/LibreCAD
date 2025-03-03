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

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to select all entities.
 *
 * @author Andrew Mustun
 */
//todo - support of moving by keyboard
//todo - joint move of endpoint ref? So if moving ref will move refs for adjusent entities
class RS_ActionDefault : public RS_PreviewActionInterface {
    Q_OBJECT

    using BASE_CLASS = RS_PreviewActionInterface;

public:
    RS_ActionDefault(RS_EntityContainer& container,
                     RS_GraphicView& graphicView);
    ~RS_ActionDefault() override;

    void finish(bool) override{}

    void init(int status) override;
    void resume() override;
    void suspend() override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
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
        Neutral,        /**< we don't know what we do yet.  */
        Dragging,       /**< dragging (either an entity or the
                                             first part of a selection window) */
        SetCorner2,     /**< Setting the 2nd corner of a selection window. */
        Moving,         /**< Moving entities (drag'n'drop) */
        MovingRef,       /**< Moving a reference point of one or more selected
                                             entities */
        Panning /**< view panning triggered by Ctl- mouse dragging */
    };

    struct Points;
    std::unique_ptr<Points> pPoints;
    RS2::SnapRestriction snapRestriction;
    RS2::EntityType typeToSelect = RS2::EntityType::EntityUnknown;

    bool allowEntityQuickInfoForCTRL = false;
    bool allowEntityQuickInfoAuto = false;
    void checkSupportOfQuickEntityInfo();
    void clearQuickInfoWidget();
    void updateQuickInfoWidget(RS_Entity *pEntity);
    void goToNeutralStatus();
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;

    void highlightHoveredEntities(QMouseEvent* currentMousePosition);
    void highlightEntity(RS_Entity* entity);
    void updateMouseButtonHints() override;
};
#endif
