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

#ifndef RS_ACTIONMODIFYSTRETCH_H
#define RS_ACTIONMODIFYSTRETCH_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
// fixme - rework to
class LC_ActionModifyStretch : public LC_UndoableDocumentModificationAction {
    Q_OBJECT
public:
    explicit LC_ActionModifyStretch(LC_ActionContext *actionContext);
    ~LC_ActionModifyStretch() override;

    void init(int status) override;
    bool isRemoveOriginals() const {return m_removeOriginals;}
    void setRemoveOriginals(const bool val){m_removeOriginals = val;}
protected:
    /**
     * Action States.
     */
    enum Status {
        SetFirstCorner,       /**< Setting first corner of selection. */
        SetSecondCorner,      /**< Setting second corner of selection. */
        SetReferencePoint,    /**< Setting the reference point. */
        SetTargetPoint        /**< Setting the target point. */
    };
    struct StretchActionData;
    std::unique_ptr<StretchActionData> m_actionData;
    QList<RS_Entity*> m_entitiesList;
    bool m_removeOriginals = true;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void previewStretchRect(bool selected) const;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) override;
    void updateActionPrompt() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;

    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
