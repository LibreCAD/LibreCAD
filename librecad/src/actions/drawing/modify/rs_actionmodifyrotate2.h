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

#ifndef RS_ACTIONMODIFYROTATE2_H
#define RS_ACTIONMODIFYROTATE2_H

#include "rs_modification.h"
#include "lc_actionmodifybase.h"

/**
 * This action class can handle user events to rotate entities around
 * two entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyRotate2 : public LC_ActionModifyBase {
    Q_OBJECT
public:
    RS_ActionModifyRotate2(RS_EntityContainer& container,
                           RS_GraphicView& graphicView);
    ~RS_ActionModifyRotate2() override;

    void init(int status) override;
    void trigger() override;
    void setAngle1(double d);
    double getAngle1();
    void setAngle2(double d);
    double getAngle2();
    void setUseSameAngle2ForCopies(bool b);
    bool isUseSameAngle2ForCopies();
    bool isMirrorAngles();
    void setMirrorAngles(bool b);
protected:
    /**
     * Action States.
     */
    enum Status {
        SetReferencePoint1,    /**< Setting the reference point. */
        SetReferencePoint2,    /**< Setting the target point. */
        ShowDialog             /**< Showing the options dialog. */
    };

    std::unique_ptr<RS_Rotate2Data> data;
    void previewRefPointsForMultipleCopies(const RS_Vector& mouse);
    void doTrigger();
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseRightButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseMoveEventSelected(QMouseEvent *e) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    bool isAllowTriggerOnEmptySelection() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
};
#endif
