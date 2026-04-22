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
    explicit LC_DefaultActionContext(QG_ActionHandler* actionHandler);
    ~LC_DefaultActionContext() override = default;
    void addOptionsWidget(LC_ActionOptionsWidget *widget) override;
    void removeOptionsWidget(LC_ActionOptionsWidget *widget) override;
    void requestSnapDistOptions(double *dist, bool on) override;
    void requestSnapMiddleOptions(int *middlePoints, bool on) override;
    void hideSnapOptions() override;
    void updateActionPrompt(const QString &, const QString &, const LC_ModifiersInfo &modifiers) override;
    void commandMessage(const QString &message) override;
    void commandPrompt(const QString &message) override;
    void updateCoordinateWidget(const RS_Vector &abs, const RS_Vector &rel, bool updateFormat) override;

    void setActionOptionsManager(LC_ActionOptionsManager *actionOptionsManager){
        m_actionOptionsManager = actionOptionsManager;
    }

    void setCoordinateWidget(QG_CoordinateWidget *coordinateWidget){
        m_coordinateWidget = coordinateWidget;
    }

    void setCommandWidget(QG_CommandWidget *commandWidget){
        m_commandWidget = commandWidget;
    }

    void setMouseWidget(QG_MouseWidget *mouseWidget){
        m_mouseWidget = mouseWidget;
    }

    void setStatusBarManager(LC_QTStatusbarManager *statusBarManager){
        m_statusBarManager = statusBarManager;
    }
    void setDocumentAndView(RS_Document *document, RS_GraphicView *view) override;
    void setSnapMode(const RS_SnapMode& mode) override;
    void setCurrentAction(RS2::ActionType, void* data) override;
    RS_ActionInterface* getCurrentAction() override;
protected:
    void deleteActionHandler() const;
private:
    LC_ActionOptionsManager* m_actionOptionsManager {nullptr};
    QG_CoordinateWidget* m_coordinateWidget{nullptr};
    QG_CommandWidget* m_commandWidget{nullptr};
    QG_MouseWidget* m_mouseWidget{nullptr};
    LC_QTStatusbarManager* m_statusBarManager{nullptr};
    QG_ActionHandler* m_actionHandler{nullptr};
};

#endif
