/****************************************************************************
**
* Abstract base class for linear dimensions

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#ifndef LIBRECAD_LC_ACTIONDIMLINEARBASE_H
#define LIBRECAD_LC_ACTIONDIMLINEARBASE_H

#include "rs_constructionline.h"
#include "rs_actiondimension.h"

class LC_ActionDimLinearBase:public RS_ActionDimension {
Q_OBJECT
public:
    LC_ActionDimLinearBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionDimLinearBase() override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent *e) override;

protected:
    /**
   * Action States.
   */
    enum Status {
        SetExtPoint1,    /**< Setting the 1st ext point.  */
        SetExtPoint2,    /**< Setting the 2nd ext point. */
        SetDefPoint,     /**< Setting the common def point. */
        SetText,         /**< Setting the text label in the command line. */
        SetAngle         /**< Setting the angle in the command line. */
    };

    enum ActionMode{
        NORMAL,
        BASELINE,
        CONTINUE
    };

    ActionMode actionMode = NORMAL;

    virtual RS_Vector getExtensionPoint1() = 0;
    virtual void setExtensionPoint1(RS_Vector p) = 0;
    virtual void setExtensionPoint2(RS_Vector p) = 0;
    virtual RS_Vector getExtensionPoint2() = 0;
    virtual void preparePreview() = 0;
    virtual double getDimAngle() = 0;
    RS_Vector getAdjacentDimDimSnapPoint(const RS_Vector &ownDimPointToCheck, double snapRange);
    RS_Vector adjustDefPointByAdjacentDims(const RS_Vector &mouse, const RS_Vector &extPoint1, const RS_Vector &extPoint2, double ownDimLineAngle, bool forPreview);
    RS_Vector adjustByAdjacentDim(RS_Vector mouse, bool forPreview);
    virtual RS_Entity *createDim(RS_EntityContainer* parent) = 0;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;

};
#endif //LIBRECAD_LC_ACTIONDIMLINEARBASE_H
