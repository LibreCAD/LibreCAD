/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_DEFAULTACTIONCONTEXT_H
#define LC_DEFAULTACTIONCONTEXT_H
#include "lc_actioncontext.h"

class QG_ActionHandler;
class LC_QTStatusbarManager;
class QG_MouseWidget;
class QG_SelectionWidget;
class QG_CommandWidget;
class QG_CoordinateWidget;
class LC_ActionOptionsManager;

class LC_DefaultActionContext: public LC_ActionContext{
public:
    LC_DefaultActionContext(QG_ActionHandler* actionHandler);
    virtual ~LC_DefaultActionContext() = default;
    void addOptionsWidget(LC_ActionOptionsWidget *widget) override;
    void removeOptionsWidget(LC_ActionOptionsWidget *widget) override;
    void requestSnapDistOptions(double *dist, bool on) override;
    void requestSnapMiddleOptions(int *middlePoints, bool on) override;
    void hideSnapOptions() override;
    void updateSelectionWidget(int countSelected, double selectedLength) override;

    void updateMouseWidget(const QString &, const QString &, const LC_ModifiersInfo &modifiers) override;
    void commandMessage(const QString &message) override;
    void commandPrompt(const QString &message) override;
    void updateCoordinateWidget(const RS_Vector &abs, const RS_Vector &rel, bool updateFormat) override;

    void setActionOptionsManager(LC_ActionOptionsManager *m_action_options_manager){
        m_actionOptionsManager = m_action_options_manager;
    }

    void setCoordinateWidget(QG_CoordinateWidget *coordinate_widget){
        m_coordinateWidget = coordinate_widget;
    }

    void setCommandWidget(QG_CommandWidget *command_widget){
        m_commandWidget = command_widget;
    }

    void setSelectionWidget(QG_SelectionWidget *selection_widget){
        m_selectionWidget = selection_widget;
    }

    void setMouseWidget(QG_MouseWidget *mouse_widget){
        m_mouseWidget = mouse_widget;
    }

    void setStatusBarManager(LC_QTStatusbarManager *status_bar_manager){
        m_statusBarManager = status_bar_manager;
    }
    void setDocumentAndView(RS_Document *document, RS_GraphicView *view) override;
    void setSnapMode(const RS_SnapMode& mode) override;
    void setCurrentAction(RS2::ActionType, void* data) override;
    RS_ActionInterface* getCurrentAction() override;
protected:
    void deleteActionHandler();
private:
    LC_ActionOptionsManager* m_actionOptionsManager {nullptr};
    QG_CoordinateWidget* m_coordinateWidget{nullptr};
    QG_CommandWidget* m_commandWidget{nullptr};
    QG_SelectionWidget* m_selectionWidget{nullptr};
    QG_MouseWidget* m_mouseWidget{nullptr};
    LC_QTStatusbarManager* m_statusBarManager{nullptr};
    QG_ActionHandler* m_actionHandler{nullptr};
};

#endif // LC_DEFAULTACTIONCONTEXT_H
