/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_ACTIONOPTIONSWIDGET_H
#define LC_ACTIONOPTIONSWIDGET_H

#include <QToolButton>

#include "lc_action_options_support.h"
#include "lc_actioncontext.h"

class QLineEdit;
class RS_ActionInterface;

/**
 * Utility base class for widgets that represents options for actions.
 * Method contains several utility methods as well as default workflows, and it's purpose is
 * simply creation of options UI and reduce code repetition there.
 */
class LC_ActionOptionsWidget:public QWidget, public LC_ActionOptionsSupport{
    Q_OBJECT
public:
    explicit LC_ActionOptionsWidget(QWidget *parent = nullptr, Qt::WindowFlags fl = {});
    ~LC_ActionOptionsWidget() override;
    void requestFocusForTag(const QString& tag) const;
    void hideOptions() override;
protected:
    void preSetupByAction(RS_ActionInterface* a) override;
    void cleanup() override;
    LC_ActionContext* m_actionContext{nullptr};
    LC_LateCompletionRequestor* m_laterCompletionRequestor{nullptr};

    bool m_interactiveInputControlsVisible {true};
    bool m_interactiveInputControlsAutoRaise {true};

    void connectInteractiveInputButton(QToolButton* button, LC_ActionContext::InteractiveInputInfo::InputType inputType,
                                       const QString& tag) const;
    void pickDistanceSetup(const QString& tag, QToolButton* button, QLineEdit* lineedit) const;
    void pickAngleSetup(const QString& tag, QToolButton* button, QLineEdit* editor) const;
    void onInteractiveInputButtonClicked(bool checked) const;
protected slots:
    virtual void languageChange() {}
};
#endif
