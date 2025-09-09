/****************************************************************************
**
* Utility base class for widgets that represents options for actions

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

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
**********************************************************************/
#ifndef LC_ACTIONCONTEXT_H
#define LC_ACTIONCONTEXT_H

#include <QString>
#include "lc_latecompletionrequestor.h"
#include "rs.h"
#include "rs_vector.h"

class RS_Entity;
class QString;

struct RS_SnapMode;

class RS_Document;
class RS_EntityContainer;
class RS_GraphicView;
class RS_Vector;
class LC_ModifiersInfo;
class LC_ActionOptionsWidget;

class RS_ActionInterface;

class LC_ActionContext{
public:
    LC_ActionContext() = default;
    virtual ~LC_ActionContext() = default;
    virtual void addOptionsWidget([[maybe_unused]]LC_ActionOptionsWidget * widget){}
    virtual void removeOptionsWidget([[maybe_unused]]LC_ActionOptionsWidget * widget){}
    virtual void requestSnapDistOptions([[maybe_unused]]double* dist, [[maybe_unused]]bool on) {}
    virtual RS_ActionInterface* getCurrentAction() {return nullptr;}
    virtual void requestSnapMiddleOptions([[maybe_unused]]int* middlePoints, [[maybe_unused]]bool on) {}
    virtual void hideSnapOptions() {}
    virtual void updateSelectionWidget([[maybe_unused]]int countSelected, [[maybe_unused]]double selectedLength){}

    virtual void updateMouseWidget([[maybe_unused]]const QString& left,
                                  [[maybe_unused]]const QString& right,
                                  [[maybe_unused]]const LC_ModifiersInfo& modifiers){}

    virtual void commandMessage([[maybe_unused]]const QString& message) {}
    virtual void commandPrompt([[maybe_unused]]const QString& message) {}


    virtual void updateCoordinateWidget([[maybe_unused]]const RS_Vector& abs,
                                        [[maybe_unused]]const RS_Vector& rel,
                                        [[maybe_unused]]bool updateFormat) {}

    virtual RS_EntityContainer* getEntityContainer();
    virtual RS_GraphicView* getGraphicView();

    virtual void setDocumentAndView(RS_Document *document, RS_GraphicView *view);

    virtual void setSnapMode([[maybe_unused]]const RS_SnapMode &mode) {}
    virtual void setCurrentAction(RS2::ActionType, [[maybe_unused]]void* data){}
    int getSelectedEntitiesCount() const {return m_selectionCount;}
    void saveContextMenuActionContext(RS_Entity* entity, const RS_Vector &position, bool clearEntitySelection);
    void clearContextMenuActionContext();
    RS_Entity* getContextMenuActionContextEntity();
    RS_Vector getContextMenuActionClickPosition();

    struct InteractiveInputInfo {
        enum State {
            NONE,
            REQUESTED
        };

        enum InputType {
            POINT,
            DISTANCE,
            ANGLE,
            NOTNEEDED,
        };

        State m_state;
        double m_distance {0};
        double m_angleRad{0};
        RS_Vector m_wcsPoint;
        InputType m_inputType;
        QString m_requestorTag;
        LC_LateCompletionRequestor* m_requestor {nullptr};
    };

    void interactiveInputStart(InteractiveInputInfo::InputType inputType, LC_LateCompletionRequestor* m_requestor, const QString &tag);
    void interactiveInputRequestCancel();
    InteractiveInputInfo* getInteractiveInputInfo(){return &m_interactiveInputInfo;}
protected:
    InteractiveInputInfo m_interactiveInputInfo;
    RS_EntityContainer * m_entityContainer {nullptr};
    RS_GraphicView * m_graphicView {nullptr};
    int m_selectionCount{0};
    RS_Vector m_contextMenuClickPosition {false};
    RS_Entity* m_contextMenuActionEntity {nullptr};
    bool m_uselectContextMenuActionEntity {false};

    void interactiveInputInvoke(InteractiveInputInfo::InputType inputType);
    void interactiveInputRequest(InteractiveInputInfo::InputType inputType, LC_LateCompletionRequestor* m_requestor, const QString &tag);
};

#endif // LC_ACTIONCONTEXT_H
