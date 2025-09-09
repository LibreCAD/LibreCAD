/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#ifndef RS_ACTIONMODIFYENTITY_H
#define RS_ACTIONMODIFYENTITY_H

#include "lc_latecompletionrequestor.h"
#include "rs_previewactioninterface.h"

class LC_EntityPropertiesEditor;
class RS_Entity;

/**
 * This action class can handle user events to select entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyEntity : public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionModifyEntity(LC_ActionContext *actionContext);
    ~RS_ActionModifyEntity() override;
    void init(int status) override;
    void notifyFinished();
    bool mayBeTerminatedExternally() override {return m_allowExternalTermination;};
protected:
    enum State {
        ShowDialog = InitialActionStatus,
        InEditing,
        EditComplete
    };

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void updateMouseButtonHints() override;
    void doTrigger() override;
    // display the entity as selected
    void setDisplaySelected(bool selected);
    void completeEditing();
    void onLateRequestCompleted(bool shouldBeSkipped) override;
private:
    RS_Entity* m_entity = nullptr;
    RS_Entity* m_clonedEntity = nullptr;
    bool m_modifyCursor{true};
    bool m_invokedForSingleEntity{false};
    bool m_allowExternalTermination{true};
    LC_EntityPropertiesEditor *m_propertiesEditor{nullptr};
};

#endif
