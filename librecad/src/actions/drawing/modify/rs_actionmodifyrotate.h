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

#ifndef RS_ACTIONMODIFYROTATE_H
#define RS_ACTIONMODIFYROTATE_H

#include "lc_actionmodifybase.h"

struct RS_RotateData;

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyRotate: public LC_ActionModifyBase {
    Q_OBJECT
public:
    RS_ActionModifyRotate(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionModifyRotate() override;
    void init(int status) override;
    void trigger() override;
    double getAngle();
    void setAngle(double angle);
    void setFreeAngle(bool enable);
    bool isFreeAngle() const {return freeAngle;};
    double getRefPointAngle();
    void setRefPointAngle(double angle);
    void setFreeRefPointAngle(bool value);
    bool isFreeRefPointAngle() const{return freeRefPointAngle;};
    bool isRefPointAngleAbsolute();
    void setRefPointAngleAbsolute(bool val);
    bool isRotateAlsoAroundReferencePoint();
    void setRotateAlsoAroundReferencePoint(bool value);
    double getCurrentAngle(){return currentAngle;}
    double getCurrentAngle2(){return currentAngle2;};
    void keyPressEvent(QKeyEvent *e) override;
protected:
    /**
     * Action States.
     */
    enum Status {
        SetReferencePoint,    /**< Setting the reference point. */
        SetCenterPoint,    /**< Setting the rotation center */
        SetTargetPoint,    /**< Setting the target to rotation to*/
        SetTargetPoint2ndRotation,    /**< Setting the target to rotation around ref point*/
    };
    // fixme - sand -  review whether it's practical to select rotation center first... it's less convenient
    // support of old mode, most probably it should be removed and one selection mode should remain
    bool selectRefPointFirst = true;
    bool freeAngle = false;
    bool freeRefPointAngle = false;
    std::unique_ptr<RS_RotateData> data;
    double currentAngle = 0.0;
    double currentAngle2 = 0.0;

    void previewRotationCircleAndPoints(const RS_Vector &center,const RS_Vector &refPoint, double angle);
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseRightButtonReleaseEventSelected(int status, QMouseEvent *pEvent) override;
    void mouseMoveEventSelected(QMouseEvent *e) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void selectionCompleted(bool singleEntity, bool fromInit) override;
    void tryTrigger();
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
};

#endif
