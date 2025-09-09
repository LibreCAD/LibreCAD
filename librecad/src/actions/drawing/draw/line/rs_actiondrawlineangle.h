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

#ifndef RS_ACTIONDRAWLINEANGLE_H
#define RS_ACTIONDRAWLINEANGLE_H

#include "rs_previewactioninterface.h"

/**
 * This action class can handle user events to draw 
 * simple lines at a given angle.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawLineAngle : public RS_PreviewActionInterface {
	Q_OBJECT
public:
    RS_ActionDrawLineAngle(LC_ActionContext *actionContext, bool fixedAngle = false, RS2::ActionType actionType = RS2::ActionDrawLineAngle);
    ~RS_ActionDrawLineAngle() override;
    void reset();
    void init(int status) override;
    QStringList getAvailableCommands() override;
    void setSnapPoint(int sp);
    int getSnapPoint() const;
    void setUcsAngleDegrees(double ucsRelAngle);
    double getUcsAngleDegrees() const;
    void setLength(double l);
    double getLength() const;
    bool hasFixedAngle() const;
    void setInAngleBasis(bool b);
    bool isInAngleBasis(){return m_orthoToAnglesBasis;}
protected:
    /**
 * Action States.
 */
    enum Status {
        SetPos = InitialActionStatus,       /**< Setting the position.  */
        SetAngle,     /**< Setting angle in the command line. */
        SetLength     /**< Setting length in the command line. */
    };
    enum SnapMode {
        SNAP_START, SNAP_MIDDLE, SNAP_END
    };
    struct Points;
    std::unique_ptr<Points> m_actionData;
    bool m_persistRelativeZero = false;
    bool m_alternateDirection = false;
    bool m_orthoToAnglesBasis = false;

    void preparePreview();
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    bool doProcessCommand(int status, const QString &command) override;
    void updateMouseButtonHints() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void doTrigger() override;
    void initFromSettings() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
};
#endif
