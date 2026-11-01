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
#include <memory>
#include <QPointer>


#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

class LC_EntityPropertiesEditor;
class RS_Entity;
struct LC_EntityPropertiesEditSession;

/**
 * This action class can handle user events to select entities.
 *
 * @author Andrew Mustun
 */
class LC_ActionModifyEntity : public LC_UndoableDocumentModificationAction {
    Q_OBJECT
public:
    explicit LC_ActionModifyEntity(LC_ActionContext *actionContext);
    ~LC_ActionModifyEntity() override;
    void init(int status) override;
    void trigger() override;
    void notifyFinished() const;
    bool mayBeTerminatedExternally() override {return m_allowExternalTermination;}
    void onLateRequestCompleted(bool shouldBeSkipped) override;
protected:
    enum State {
        ShowDialog = InitialActionStatus,
        InEditing,
        EditComplete
    };

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void updateActionPrompt() override;
    void doTrigger() override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;
private:
    struct NativeDocumentLifetime;
    struct NativeDialogSource;
    bool nativeDialogDocumentIsCurrent() const;
    bool dialogSourceIsMember(const RS_Entity* source) const;
    void abandonDialogEdit();
    void nativeDialogDocumentDestroyed();
    bool captureNativeDialogSource();
    bool nativeDialogSourceIsCurrent() const;
    void discardNativeDialogClone();
    void finishNativeDialog(bool refused);
    std::unique_ptr<NativeDocumentLifetime> m_nativeDocumentLifetime;
    std::unique_ptr<NativeDialogSource> m_nativeDialogSource;
    RS_Entity* m_entity = nullptr;
    RS_Entity* m_clonedEntity = nullptr;
    std::shared_ptr<LC_EntityPropertiesEditSession> m_pendingNativeClone;
    bool m_modifyCursor{true};
    bool m_invokedForSingleEntity{false};
    bool m_allowExternalTermination{true};
    QPointer<LC_EntityPropertiesEditor> m_propertiesEditor;
};

#endif
