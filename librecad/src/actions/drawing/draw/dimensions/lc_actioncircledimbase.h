/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/

#ifndef LC_ACTIONCIRCLEDIMBASE_H
#define LC_ACTIONCIRCLEDIMBASE_H

#include "rs_actiondimension.h"
#include "rs_dimension.h"

class LC_ActionCircleDimBase:public RS_ActionDimension {
    Q_OBJECT

public:
    LC_ActionCircleDimBase(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView,
        RS2::ActionType type);

    ~LC_ActionCircleDimBase() override;
    void trigger() override;
    void updateMouseButtonHints() override;
    QStringList getAvailableCommands() override;

    void mouseMoveEvent(QMouseEvent *event) override;

    double getAngle() const;

    void setAngle(double angle);

    bool isAngleIsFree() const;

    void setAngleIsFree(bool angleIsFree);

    double getCurrentAngle(){return currentAngle;}

protected:

    enum Status {
        SetEntity,     /**< Choose entity. */
        SetPos,      /**< Choose point. */
        SetText        /**< Setting text label in the command line. */
    };

    /** Chosen entity (arc / circle) */
    RS_Entity *entity = nullptr;
    /** Last status before entering text. */
    Status lastStatus = SetEntity;
    /** Chosen position */
    std::unique_ptr<RS_Vector> pos;

    double angle = 0;
    bool angleIsFree = false;
    bool alternateAngle = false;

    double currentAngle = 0.0;

    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    virtual RS_Vector preparePreview(RS_Entity *en, RS_Vector &position, bool forcePosition) = 0;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    virtual RS_Dimension* createDim(RS_EntityContainer *parent) const = 0;
};

#endif // LC_ACTIONCIRCLEDIMBASE_H
