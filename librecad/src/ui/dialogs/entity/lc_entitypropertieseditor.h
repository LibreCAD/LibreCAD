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

#ifndef LC_ENTITYPROPERTIESEDITOR_H
#define LC_ENTITYPROPERTIESEDITOR_H
#include <QObject>

#include "lc_latecompletionrequestor.h"
#include "rs_entity.h"

class LC_ActionContext;
class LC_LateCompletionRequestor;
class LC_GraphicViewport;

class LC_EntityPropertiesEditor: public QObject, public LC_LateCompletionRequestor{
    Q_OBJECT
public:
    LC_EntityPropertiesEditor(LC_ActionContext* actionContext, LC_LateCompletionRequestor* lateCompletionRequestor);
    void editEntity(QWidget* parent, RS_Entity* entity, LC_GraphicViewport* viewport);
    void onLateRequestCompleted(bool shouldBeSkipped) override;
public slots:
    void showEntityPropertiesDialog();
protected:
    LC_ActionContext* m_actionContext {nullptr};
    RS_Entity* m_entity {nullptr};
    LC_GraphicViewport* m_viewport {nullptr};
    LC_LateCompletionRequestor* m_lateCompletionRequestor {nullptr};
    QWidget* m_parent {nullptr};
};

#endif // LC_ENTITYPROPERTIESEDITOR_H
