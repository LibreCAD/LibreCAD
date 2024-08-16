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

#ifndef RS_ACTIONMODIFYMOVEROTATE_H
#define RS_ACTIONMODIFYMOVEROTATE_H


#include "lc_actionmodifybase.h"

struct RS_MoveRotateData;

/**
 * This action class can handle user events to move and at the same
 * time rotate entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyMoveRotate : public LC_ActionModifyBase {
    Q_OBJECT
public:
    RS_ActionModifyMoveRotate(RS_EntityContainer& container,
                              RS_GraphicView& graphicView);
    ~RS_ActionModifyMoveRotate() override;

    void trigger() override;
    QStringList getAvailableCommands() override;
    void setAngle(double a);
    double getAngle() const;
    void setUseSameAngleForCopies(bool b);
    bool isUseSameAngleForCopies();
    void setAngleIsFree(bool b);
    bool isAngleFree(){return !angleIsFixed;};
protected:
    /**
 * Action States.
 */
    enum Status {
        SetReferencePoint,    /**< Setting the reference point. */
        SetTargetPoint,       /**< Setting the target point. */
        SetAngle,              /**< Setting angle in command line. */
        ShowDialog           /**< Showing the options dialog. */
    };
    struct Points;
    std::unique_ptr<Points> pPoints;
/** Last status before entering angle. */
    Status lastStatus = SetReferencePoint;
    bool angleIsFixed = true;
/**
 * Commands
 */
    QString cmdAngle;
    QString cmdAngle2;
    QString cmdAngle3;

    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseRightButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseMoveEventSelected(QMouseEvent *e) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void previewRefPointsForMultipleCopies();
    void doTrigger();
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
};
#endif
