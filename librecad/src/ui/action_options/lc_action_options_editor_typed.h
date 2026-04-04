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

#ifndef LC_ACTIONOPTIONSEDITORTYPE_H
#define LC_ACTIONOPTIONSEDITORTYPE_H

#include "lc_action_options_editor.h"
#include "lc_action_options_properties_filler.h"
#include "lc_action_options_widget.h"

using FunOptionsWidgetCreator = std::function<LC_ActionOptionsWidget*()>;
using FunOptionsFillerCreator = std::function<LC_ActionOptionsPropertiesFiller*()>;

class RS_ActionInterface;

class LC_ActionOptionsEditorTyped: public LC_ActionOptionsEditor{
public:
    [[deprecated]] // fixme - move to lib
    explicit LC_ActionOptionsEditorTyped(RS_ActionInterface *action, const FunOptionsWidgetCreator& funOptionsWidgetCreator,
        const FunOptionsFillerCreator& funOptionsFillerCreator):m_action{action},
        m_funCreateOptionsWidget{funOptionsWidgetCreator}, m_funCreateOptionsFiller{funOptionsFillerCreator} {}
    ~LC_ActionOptionsEditorTyped() override = default;
    void showOptions() override;
    void hideOptions() override;
    void updateOptions(const QString& tagToFocus = "") override;
    void updateOptionsUI(int mode, const QVariant *value) override;
    void setup(bool showOptionsInToolOptionsWidget, bool showOptionsInPropertySheetWidget) override;
protected:
    LC_ActionOptionsWidget* createOptionsWidget() const;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() const;

    std::unique_ptr<LC_ActionOptionsWidget> m_optionWidget {nullptr};
    std::unique_ptr<LC_ActionOptionsPropertiesFiller> m_optionFiller {nullptr};
    bool m_showOptionsInToolOptionsWidget = true;
    bool m_showOptionsInPropertySheetWidget = true;
    FunOptionsWidgetCreator m_funCreateOptionsWidget;
    FunOptionsFillerCreator m_funCreateOptionsFiller;
    void showOptionsInToolOptionsWidget();
    void showOptionsInPropertySheetWidget();
    void updateOptionsUIInToolOptionsWidget(int mode,const QVariant *value);
    void updateOptionsUIInPropertySheetWidget(int mode, const QVariant *value);
    void hideOptionsInToolOptionsWidget();
    void hideOptionsInPropertySheetWidget();
    void updateOptionsInToolOptionsWidget(const QString& tagToFocus);
    void updateOptionsInPropertySheetWidget(const QString& tagToFocus);

    RS_ActionInterface* m_action;
};


#endif
