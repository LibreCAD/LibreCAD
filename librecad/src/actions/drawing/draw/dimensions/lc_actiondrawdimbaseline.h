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

#ifndef LC_ACTIONDRAWDIMBASELINE_H
#define LC_ACTIONDRAWDIMBASELINE_H

#include "lc_actiondimlinearbase.h"
#include "rs_dimlinear.h"

struct RS_DimLinearData;

class LC_ActionDrawDimBaseline:public LC_ActionDimLinearBase{
    Q_OBJECT
public:
    LC_ActionDrawDimBaseline(LC_ActionContext *actionContext,RS2::ActionType type);
    bool isFreeBaselineDistance() const;
    void setFreeBaselineDistance(bool freeDistance);
    double getBaselineDistance() const;
    void setBaselineDistance(double distance);
    void reset() override;
    double getCurrentBaselineDistance() const;
    QStringList getAvailableCommands() override;
protected:
    std::unique_ptr<RS_DimLinearData> m_edata;
    RS_Vector m_baseDefPoint;
    RS_Vector m_prevExtensionPointEnd; // fixme - probably remove
    double m_dimDirectionAngle = 0.0;
    bool m_freeBaselineDistance = false;
    double m_baselineDistance = 20;
    bool m_alternateMode = false;
    double m_currentDistance = 0.0;
    Status m_lastStatus = SetExtPoint1;
    bool isBaseline();
    RS_Entity *createDim(RS_EntityContainer* parent) override;
    RS_Vector getExtensionPoint1() override;
    void setExtensionPoint1(RS_Vector p) override;
    void setExtensionPoint2(RS_Vector p) override;
    RS_Vector getExtensionPoint2() override;
    void preparePreview() override;
    double getDimAngle() override;
    bool doProcessCommand(int status, const QString &command) override;
    void doTrigger() override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void updateMouseButtonHints() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
};

#endif // LC_ACTIONDRAWDIMBASELINE_H
