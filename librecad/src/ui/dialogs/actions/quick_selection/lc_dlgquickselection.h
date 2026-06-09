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

#ifndef LC_DLGQUICKSELECTION_H
#define LC_DLGQUICKSELECTION_H

#include <qlistwidget.h>

#include "lc_actioncontext.h"
#include "lc_dialog.h"
#include "lc_entitymatchdescriptor.h"
#include "lc_graphicviewport.h"
#include "rs_selection.h"

namespace Ui {
    class LC_DlgQuickSelection;
}


struct LC_QuickSearchConditions {
    bool applyToSelection {false};
    RS2::EntityType entityType {RS2::EntityUnknown};
    QString selectedPropertyName;
    LC_PropertyMatchOperation matchOperation{MATCH_OPERATION_EQUALS};
    bool includeIntoNewSet  {false};
    bool appendToCurrentSet {false};
};

struct LC_QuickSearchSelectionDialogState;

class LC_DlgQuickSelection : public LC_Dialog {
    Q_OBJECT
public:
    LC_DlgQuickSelection(QWidget* parent, LC_ActionContext* actionContext, LC_ActionContext::InteractiveInputInfo::InputType interactiveInputType, const LC_QuickSearchSelectionDialogState* savedState, double interactiveInputValue1,
                         double interactiveInputValue2);
    ~LC_DlgQuickSelection() override;
    LC_ActionContext::InteractiveInputInfo::InputType isInteractiveInputRequested() const {return m_interactiveInputRequested;}
    LC_QuickSearchSelectionDialogState* getSavedState() const;
    bool isAdditionalSelectionRequested() const {return m_selectionRequested;}
    RS_Selection::ConditionalSelectionOptions getSelectionOptions(RS_Selection::ConditionalSelectionOptions& result) const;
    void accept() override;
protected slots:
    void onApplyToCurrentIndexChanged(int index);
    void onOperatorChanged(int index);
    void onEntityTypeIndexChanged(int index);
    void onPropertyListRowChanged(int currentRow);
    void onAppendToCurrentSetClicked() const;
    void onPickLengthClicked();
    void onPickCoord();
    void onPickAngle();
    void onManualSelection();
    void onPrecisionTextEditingFinished();
    void onUpdatePrecisionByDocumentSettings();
private:
    Ui::LC_DlgQuickSelection *ui;
    LC_ActionContext* m_actionContext;
    RS_Document *m_document {nullptr};
    LC_GraphicViewport* m_viewport {nullptr};
    RS_Selection::CurrentSelectionState m_selectionState;
    RS2::EntityType m_entityType = RS2::EntityUnknown;
    LC_EntityMatchDescriptor* m_entityMatchDescriptor {nullptr};
    LC_PropertyMatchDescriptor* m_propertyDescriptor {nullptr};
    LC_PropertyMatchOperation m_operationType = MATCH_OPERATION_EQUALS;

    bool m_selectionRequested{false};
    LC_ActionContext::InteractiveInputInfo::InputType  m_interactiveInputRequested{LC_ActionContext::InteractiveInputInfo::NOTNEEDED};
    QString m_inputTag{""};

    QString m_precisionLength;
    QString m_precisionAngle;

    LC_EntityMatcher *obtainEntityMatcher() const;
    RS2::EntityType getEntityTypeToMatch() const;

    void saveState() const;
    void setupEntitiesTypesList(const QMap<RS2::EntityType, int>& map) const;
    void addOperationItem(LC_PropertyMatchDescriptor* propertyDescriptor, LC_PropertyMatchOperation type, const QString& label) const;
    void setupOperationsCombobox(LC_PropertyMatchDescriptor* propertyDescriptor) const;
    void enablePrecisionLength();
    void enablePrecisionAngle();
    void disablePrecision() const;
    void setPropertyValueInput(LC_PropertyMatchDescriptor* propertyDescriptor);
    void restoreFromSavedState(const LC_QuickSearchSelectionDialogState* savedState,
        LC_ActionContext::InteractiveInputInfo::InputType inputType, double interactiveInputValue1, double interactiveInputValue2);
    QString obtainPropertyName(int currentRow) const;
    QString obtainCurrentPropertyName() const;
    RS2::EntityType obtainEntityType(int index) const;
    RS2::EntityType obtainCurrentEntityType() const;
    LC_PropertyMatchOperation obtainCurrentOperation() const;
    LC_PropertyMatchOperation obtainOperation(int index) const;
    void updateWidgetSettings() const;
    bool setCurrentEntityType(int entityType) const;
    void setCurrentPropertyName(const QString& propertyName) const;
    void setCurrentOperation(int operation) const;
    void tryToRestorePreviousState();
    void updatePrecisionLength();
    void updatePrecisionAngle();
};

#endif
