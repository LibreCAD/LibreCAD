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
#include <QPointer>
#include <memory>
#include <cstdint>
#include <functional>

#include "lc_latecompletionrequestor.h"
#include "rs_entity.h"

class LC_ActionContext;
class LC_LateCompletionRequestor;
class LC_GraphicViewport;
class QDialog;

// Value ownership outlives an action/editor destroyed inside the modal event loop.
struct LC_EntityPropertiesEditSession {
    std::unique_ptr<RS_Entity> entity;
    ~LC_EntityPropertiesEditSession();
};

class LC_EntityPropertiesEditor: public QObject, public LC_LateCompletionRequestor{
    Q_OBJECT
public:
    LC_EntityPropertiesEditor(LC_ActionContext* actionContext, LC_LateCompletionRequestor* requestor);
    ~LC_EntityPropertiesEditor() override;
    void editEntity(QWidget* parent, RS_Entity* entity, LC_GraphicViewport* viewport,
        std::function<bool()> guard = {}, std::shared_ptr<LC_EntityPropertiesEditSession> session = {});
    void cancelGuardedEdit();
    void onLateRequestCompleted(bool shouldBeSkipped) override;
public slots:
    void showEntityPropertiesDialog();
protected:
    LC_ActionContext* m_actionContext {nullptr};
    RS_Entity* m_entity {nullptr};
    LC_GraphicViewport* m_viewport {nullptr};
    LC_LateCompletionRequestor* m_lateCompletionRequestor {nullptr};
    QWidget* m_parent {nullptr};
private:
    void scheduleDialog(int delay);
    std::function<bool()> m_editGuard;
    std::shared_ptr<LC_EntityPropertiesEditSession> m_guardedSession;
    QPointer<QDialog> m_activeGuardedDialog;
    std::uint64_t m_editGeneration = 0;
};

#endif
