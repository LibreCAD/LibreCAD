/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_ACTIONDIMORDINATE_H
#define LC_ACTIONDIMORDINATE_H


#include "rs_actiondimension.h"

class LC_DimOrdinate;
class RS_EntityContainer;
class LC_ActionDimOrdinate: public RS_ActionDimension{
    Q_OBJECT
public:
    explicit LC_ActionDimOrdinate(LC_ActionContext* context);
    ~LC_ActionDimOrdinate() override;
protected:
    enum State {
        SetFeaturePoint = InitialActionStatus,
        SetLeaderEnd,
        SetText
    };

    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    State m_lastStatus = SetFeaturePoint;

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    bool doProcessCommand(int status, const QString& command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& pos) override;
    void doTrigger() override;
    LC_DimOrdinate* createDim(RS_Vector leaderEndPoint, bool alternateOrdinate, RS_EntityContainer * container);
    void onMouseMoveEvent(int status, LC_MouseEvent* event) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent* e) override;
    QStringList doGetAvailableCommands(int status) override;
    void updateMouseButtonHints() override;
};

#endif // LC_ACTIONDIMORDINATE_H
