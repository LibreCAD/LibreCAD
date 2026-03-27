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

#ifndef LC_ACTIONSELECTQUICK_H
#define LC_ACTIONSELECTQUICK_H

#include "lc_actionpreselectionawarebase.h"
#include "lc_dlgquickselection.h"

struct LC_QuickSearchSelectionDialogState;

class LC_ActionSelectQuick: public LC_ActionPreSelectionAwareBase {
    Q_OBJECT
public:
    explicit LC_ActionSelectQuick(LC_ActionContext* actionContext)
        : LC_ActionPreSelectionAwareBase("ActionSelectQuick", actionContext,  RS2::ActionSelectQuick) {
    }

    ~LC_ActionSelectQuick() override;
    void onLateRequestCompleted(bool shouldBeSkipped) override;
    void trigger() override;
    void init(int status) override;
    bool mayBeTerminatedExternally() override {return m_allowExternalTermination;}

protected:
    bool doTriggerModifications([[maybe_unused]]LC_DocumentModificationBatch& ctx) override {return false;}
    void updateActionPromptForSelection() override;
    void onSelectionCompleted(bool singleEntity, bool fromInit) override;
private:
    enum Status {
        SHOW_DIALOG,
        EDITING
    };
    void showSelectionDialog();
    void performSelection(LC_DlgQuickSelection* dlg) const;
    void showDialog();

    bool m_allowExternalTermination{true};
    LC_QuickSearchSelectionDialogState* m_savedState {nullptr};
};

#endif
