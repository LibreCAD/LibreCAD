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

#ifndef LC_DLGENTITYPROPERTIES_H
#define LC_DLGENTITYPROPERTIES_H

#include <QPointer>
#include <functional>
#include "lc_dialog.h"
#include "lc_dimstyleslistmodel.h"
#include "lc_entitypropertieseditorwidget.h"

class RS_Layer;
class RS_Entity;
class LC_PropertiesEditingWidgetOffsetCurve;

namespace Ui {
    class LC_DlgEntityProperties;
}

class LC_DlgEntityProperties : public LC_Dialog{
    Q_OBJECT
public:
    LC_DlgEntityProperties(QWidget* parent, LC_GraphicViewport* viewport, RS_Entity* entity,
                           LC_ActionContext::InteractiveInputInfo::InputType inputType, const QString& tag,
                           double valueOne, double valueTwo, std::function<bool()> nativeGuard = {});
    ~LC_DlgEntityProperties() override;
    LC_ActionContext::InteractiveInputInfo::InputType isInteractiveInputRequested() const {return m_interactiveInputRequested;}
    QString getInteractiveInputTag(){return m_inputTag;}
public slots:
    void accept() override;
    void reject() override;
protected slots:
    void onLayerChanged(RS_Layer* layer) const;
    void onPenChanged() const;
    void onInteractiveInputRequested(LC_ActionContext::InteractiveInputInfo::InputType inputType, const QString& tag);
private:
    friend class LC_EntityPropertiesEditor;
    void detachNativePreparation();
    Ui::LC_DlgEntityProperties *ui;
    RS_Entity* m_entity{nullptr};
    LC_ActionContext::InteractiveInputInfo::InputType  m_interactiveInputRequested{LC_ActionContext::InteractiveInputInfo::NOTNEEDED};
    QString m_inputTag{""};
    QPointer<LC_PropertiesEditingWidgetOffsetCurve> m_offsetEditor;
    void prepareTypeSpecificUI(LC_EntityPropertiesEditorWidget*& primaryWidget,
                               LC_EntityPropertiesEditorWidget*& secondaryWidget,
                               QString& dlgName,
                               QString& windowTitle, RS2::EntityType entityType);
    void setupEntityLayerAndAttributesUI(RS_Entity* entity);
};

#endif
