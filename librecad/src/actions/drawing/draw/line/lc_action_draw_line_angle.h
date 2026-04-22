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

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_line.h"

/**
 * This action class can handle user events to draw 
 * simple lines at a given angle.
 *
 * @author Andrew Mustun
 */
class LC_ActionDrawLineAngle : public LC_SingleEntityCreationAction {
    Q_OBJECT
public:
    enum LengthType {
        LINE = 0,
        BY_X,
        BY_Y,
        FREE
    };

    enum SnapMode {
        SNAP_START, SNAP_MIDDLE, SNAP_END
    };

    explicit LC_ActionDrawLineAngle(LC_ActionContext* actionContext, bool fixedAngle = false,
                                    RS2::ActionType actionType = RS2::ActionDrawLineAngle);
    ~LC_ActionDrawLineAngle() override;
    void reset();
    void init(int status) override;
    QStringList getAvailableCommands() override;
    void setLineSnapMode(int sp) ;
    int getLineSnapMode() const;
    void setUcsAngleDegrees(double ucsRelAngleDegrees);
    double getUcsAngleDegrees() const;
    void setLength(double l);
    double getLength() const;
    bool hasFixedAngle() const;
    void setInAngleBasis(bool b);

    bool isInAngleBasis() const {
        return m_orthoToAnglesBasis;
    }

    LengthType getLengthType() const {
        return m_lengthType;
    }

    void setLengthType(LengthType type, bool doUpdateOptions = true);
    bool isFreeLineMode() const;

protected:
    /**
 * Action States.
 */
    enum Status {
        SetPos = InitialActionStatus, /**< Setting the position.  */
        SetPoint2,
        SetAngle, /**< Setting angle in the command line. */
        SetLength, /**< Setting length in the command line. */
        SetLengthType, /**< Setting length type in the command line. */
        SetSnapPoint /**< Setting type of snap point in the command line. */
    };



    /**
    * Line data defined so far.
     */
    RS_LineData m_lineData;
    /**
    * Position.
    */
    RS_Vector m_pos;
    RS_Vector m_secondPoint;
    /**
    * Line angle. Stored in radians and in UCS basis coordinate system - to ensure that change of the UCS or Angle Basis when actions's is active
    * is reflected properly
    */
    double m_ucsBasisAngleRad{0.0};
    /**
    * Line length.
    */
    double m_lineLength{1.};
    /**
    * Is the angle fixed?
    */
    bool m_fixedAngle{false};
    /**
    * Snap point (start, middle, end).
    */
    SnapMode m_lineSnapMode{SNAP_START};
    bool m_persistRelativeZero = false;
    bool m_alternateDirection = false;
    bool m_orthoToAnglesBasis = false;
    LengthType m_lengthType = LengthType::LINE;

    void preparePreview();
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    bool doProcessCommand(int status, const QString& command) override;
    void updateActionPrompt() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& pos) override;
    void initFromSettings() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
