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

#ifndef LC_ACTIONDIMSTYLEAPPLY_H
#define LC_ACTIONDIMSTYLEAPPLY_H
#include "rs_previewactioninterface.h"

class RS_Dimension;

class LC_ActionDimStyleApply:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    // statuses of action
    enum {
        SelectEntity = InitialActionStatus,
        ApplyToEntity
    };
    LC_ActionDimStyleApply(LC_ActionContext *actionContext);
    ~LC_ActionDimStyleApply() override = default;
    void init(int status) override;
    void finish(bool updateTB) override;
    void setSourceEntity(RS_Entity* en);
private:
    RS_Dimension* m_srcEntity {nullptr};
    RS2::EntityType m_srcEntityStyleType {RS2::EntityUnknown};
    QString m_srcEntityBaseStyleName;
protected:
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void updateMouseButtonHints() override;
};

#endif // LC_ACTIONDIMSTYLEAPPLY_H
