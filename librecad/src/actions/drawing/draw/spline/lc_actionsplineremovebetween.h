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

#ifndef LC_ACTIONSPLINEREMOVEBETWEEN_H
#define LC_ACTIONSPLINEREMOVEBETWEEN_H

#include "lc_actionsplinemodifybase.h"

class LC_ActionSplineRemoveBetween:public LC_ActionSplineModifyBase{
    Q_OBJECT
public:
    LC_ActionSplineRemoveBetween(LC_ActionContext *actionContext);
    ~LC_ActionSplineRemoveBetween() override = default;
protected:
    bool m_splineIsClosed = false;
    RS_Entity *createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, bool startDirection) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMove(RS_Vector mouse, int status, LC_MouseEvent *e) override;
    void updateMouseButtonHints() override;
    void collectPointsThatRemainsAfterDeletion(
        const RS_Vector &controlPoint, unsigned int splinePointsCount, bool deleteNotFoundPoints, std::vector<RS_Vector> &pointsVector,
        std::vector<RS_Vector> &remainingPoints) const;
    bool isValidSplinePointsData(unsigned long long int size, bool closed);
    bool isValidSplineData(unsigned long long int size, bool closed, int degree);
    void doOnEntityNotCreated() override;
    void doCompleteTrigger() override;
};

#endif // LC_ACTIONSPLINEREMOVEBETWEEN_H
