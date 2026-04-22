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

#ifndef LC_PROPERTYSHEETWIDGET_H
#define LC_PROPERTYSHEETWIDGET_H

#include "lc_actioncontext.h"
#include "lc_entitypropertyvaluedelegate.h"
#include "lc_graphicviewawarewidget.h"
#include "lc_graphicviewport.h"
#include "lc_latecompletionrequestor.h"
#include "lc_property.h"
#include "lc_property_rsvector.h"
#include "lc_property_sheet_interface.h"
#include "lc_propertysheet_widget_options.h"
#include "lc_selectedsetlistener.h"
#include "lc_tool_options_properties_container_provider.h"
#include "lc_ucslist.h"
#include "lc_viewslist.h"
#include "rs_entity.h"
#include "rs_layerlistlistener.h"

class LC_ActionGroupManager;
class LC_ActionContext;
class LC_EntityPropertyContainerProvider;
class QToolButton;

namespace Ui {
    class LC_PropertySheetWidget;
}

class LC_PropertySheetWidget : public LC_GraphicViewAwareWidget, public LC_SelectedSetListener, public LC_UCSListListener,
                               public RS_LayerListListener, public LC_ViewListListener, public LC_EntitiesModificationContext,
                               public LC_LateCompletionRequestor, public LC_PropertySheetInterface {
    Q_OBJECT

public:
    explicit LC_PropertySheetWidget(QWidget* parent, LC_ActionContext* actionContext, LC_ActionGroupManager* actionGroupManager);
    ~LC_PropertySheetWidget() override;
    void loadCollapsedSections();
    void saveCollapsedSections();
    void setGraphicView(RS_GraphicView* gv) override;
    void stopInplaceEdit() const;
    void selectionChanged() override {refill();}
    void refill() override;
    void showToolOptions(LC_ToolOptionsPropertiesContainerProvider* provider) override;
    void setShouldHandleSelectionChange(bool value);
    void updateFormats();
    void doProcessLateRequest(const LC_ActionContext::InteractiveInputInfo& interactiveInputInfo);
    void onLateRequestCompleted(bool shouldBeSkipped) override;
    bool isCollapsedSection(const QString& name) const;
    void markContainerCollapsed(const QString& name, bool collapse);
    void checkSectionCollapsed(LC_PropertyContainer* result);
    void entityModified(RS_Entity* originalEntity, RS_Entity* entityClone) override;
    void ucsListModified([[maybe_unused]]bool changed) override {refill();}
    void layerListModified(bool) override {refill();}
    void viewsListModified([[maybe_unused]]bool changed) override {refill();}
    LC_PropertySheetWidgetOptions* getOptions() const {return m_propertySheetOptions.get();}
public slots :
    void onUcsChanged(LC_UCS* ucs);
    void onViewDefaultActionActivated(bool defaultActionActivated, RS2::ActionType actionRtti, RS2::ActionType prevActionRtti);
    void onRelativeZeroChanged(const RS_Vector&);
    void onBeforePropertyEdited(LC_Property* property, LC_Property::PropertyValuePtr newValue, int typeId);
    void onPropertyEdited(LC_Property* property);
    void onSelectionIndexChanged(int index);
    void onSectionPropertyChanged(LC_PropertyChangeReason reason);
    void onActivePropertyChanged(LC_Property* activeProperty);
    void highlightVectorPropertyPosition(const LC_PropertyRSVector* vectorProperty) const;
    void checkIfVectorAndGetLocation(LC_Property* property);
    void invalidateCached() const;
    void onDockVisibilityChanged(bool visible);
    void onActivePenChanged(RS_Pen pen);
    void onSettingsClicked();
    void setCurrentQAction(const QAction *a);
protected:
    enum OperationMode {
        MODE_SELECTION,
        MODE_TOOL_OPTIONS
    };
    void setupSelectionButton(QToolButton* selectionButton, QAction* selectionPointerAction, LC_ActionGroupManager* actionGroupManager);
    void updatePropertiesSheetFont() const;
    void setupSelectionTypeCombobox(RS2::EntityType entityTypeTryToSet, QString propertyTryToSet);
    void clearContextEntities();
    void collectEntitiesToModify(RS2::EntityType entityType, QList<RS_Entity*>& entitiesToModify) const;
    LC_PropertyContainer* createPropertiesContainer(RS2::EntityType entityType, const QList<RS_Entity*>& list);
    LC_PropertyContainer* preparePropertiesContainer(RS2::EntityType entityType);
    LC_PropertyContainer* prepareToolOptionsContainer(LC_ToolOptionsPropertiesContainerProvider* toolOptionsContainerProvider);
    void destroyContainer(LC_PropertyContainer* previousContainer) const;
    void setPickedPointPropertyValue(const QString& propertyName, const RS_Vector& ucsVector) const;
    void setPickedPropertyValue(const QString& propertyName, double interactiveInputValue,
                                LC_ActionContext::InteractiveInputInfo::InputType input) const;
    void setPickedPropertyCoordinateValue(const QString& propertyName, double interactiveInputValue, bool forX) const;
    bool isVirtualProperty(const LC_Property* property);
    int getCurrentlySelectedEntityType(int index) const;
    QLayout* getTopLevelLayout() const override;
    void replaceTopLevelContainer(LC_PropertyContainer* newContainer);
    void doAdjustForDockLocation(Qt::DockWidgetArea area) override;
private:
    Ui::LC_PropertySheetWidget* ui;
    void initPropertySheet();
    RS_GraphicView* m_graphicView = nullptr; // fixme - sand - review dependency
    RS_Document* m_document = nullptr;
    LC_GraphicViewport* m_viewport = nullptr;
    bool m_handleSelectionChange = true;
    LC_ActionContext* m_actionContext{nullptr};
    std::unique_ptr<LC_EntityPropertyContainerProvider> m_entityContainerProvider;
    std::unique_ptr<LC_PropertySheetWidgetOptions> m_propertySheetOptions;
    LC_ToolOptionsPropertiesContainerProvider* m_toolOptionsPropertiesContainerProvider = nullptr;
    QSet<QString> m_collapsedContainerNames;
    QList<RS_Entity*> m_orginalEntities;
    QList<RS_Entity*> m_modifiedEntities;
    OperationMode m_operationMode = MODE_SELECTION;
};

#endif
