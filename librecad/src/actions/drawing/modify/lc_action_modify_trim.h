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

#ifndef RS_ACTIONMODIFYTRIM_H
#define RS_ACTIONMODIFYTRIM_H

#include "lc_undoabledocumentmodificationaction.h"

class RS_AtomicEntity;
/**
 * This action class can handle user events to trim entities.
 *
 * @author Andrew Mustun
 */
class LC_ActionModifyTrim:public LC_UndoableDocumentModificationAction {
    Q_OBJECT
public:
    explicit LC_ActionModifyTrim(LC_ActionContext *actionContext,bool both = false);
    ~LC_ActionModifyTrim() override;
    void init(int status) override;
    void finish() override;
protected:
    /**
     * Action States.
     */
    enum Status {
        ChooseLimitEntity = InitialActionStatus, /**< Choosing the limiting entity. */
        ChooseTrimEntity /**< Choosing the entity to trim. */
    };

    RS_AtomicEntity *m_trimEntity = nullptr;
    RS_Entity *m_limitEntity = nullptr;
    struct TrimActionData;
    std::unique_ptr<TrimActionData> m_actionData;
    bool m_both = false;

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    void previewRefTrimmedEntity(RS_Entity *trimmed, const RS_Entity *original) const;
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateActionPrompt() override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void previewTrim(RS_Entity* entityToTrimCandidate, RS_Entity* limitingEntity, const RS_Vector& trimCoordinates, const RS_Vector& limitCoordinates, bool& trimInvalid) const;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;
};
#endif
