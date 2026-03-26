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

#ifndef RS_ACTIONDRAWLINERELANGLE_H
#define RS_ACTIONDRAWLINERELANGLE_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

class RS_Vector;

/**
 * This action class can handle user events to draw lines with a given angle
 * to a given entity.
 *
 * @author Andrew Mustun
 */
// fixme - add line snap mode (start/end, middle)
class LC_ActionDrawLineRelAngle:public LC_SingleEntityCreationAction {
    Q_OBJECT
public:
    explicit LC_ActionDrawLineRelAngle(LC_ActionContext *actionContext,double angle = 0.0,bool fixedAngle = false);
    ~LC_ActionDrawLineRelAngle() override;
    RS2::ActionType rtti() const override;
    void finish() override;
    QStringList getAvailableCommands() override;
    void setAngle(double angleDeg);
    double getAngle() const;
    void setLength(const double l){m_length = l;}
    double getLength() const{return m_length;}
    bool hasFixedAngle() const{return m_fixedAngle;}
protected:
    enum Status {
        SetEntity = InitialActionStatus,     /**< Choose entity. */
        SetPos,        /**< Choose position. */
        SetAngle,      /**< Set angle in console. */
        SetLength      /**< Set length in console. */
    };
    /** Chosen entity */
    RS_Entity *m_entity = nullptr;
    /** Chosen position */
    std::unique_ptr<RS_Vector> m_pos;
    /**
     * Line angle.
     */
    double m_relativeAngleRad = 0.;
    /**
     * Line length.
     */
    double m_length = 10.;
    /**
     * Is the angle fixed?
     */
    bool m_fixedAngle = false;

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void setEntity(RS_Entity* en);
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) override;
    void updateActionPrompt() override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};

#endif
