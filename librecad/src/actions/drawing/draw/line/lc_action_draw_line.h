
/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef RS_ACTIONDRAWLINE_H
#define RS_ACTIONDRAWLINE_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

/**
 * This action class can handle user events to draw
 * simple lines with the start- and endpoint given.
 *
 * @author Andrew Mustun
 */
class LC_ActionDrawLine : public LC_SingleEntityCreationAction{
    Q_OBJECT
public:
    explicit LC_ActionDrawLine(LC_ActionContext *actionContext);
    ~LC_ActionDrawLine() override;
    void reset();
    void init(int status) override;
    QStringList getAvailableCommands() override;
    void close();
    void next();
    void undo();
    void redo();

    bool mayUndo() const;
    bool mayRedo() const;
    bool mayClose() const;
protected:
    /// Action States
    enum Status {
        SetStartpoint = InitialActionStatus,   ///< Setting the startpoint
        SetEndpoint      ///< Setting the endpoint
    };

    /// History Actions
    enum HistoryAction {
        HA_SetStartpoint,   ///< Setting the startpoint
        HA_SetEndpoint,     ///< Setting the endpoint
        HA_Close,           ///< Close group of lines
        HA_Next             ///< Start new group of lines
    };

    struct History;
    struct ActionData;
    std::unique_ptr<ActionData> m_actionData;
    RS2::CursorType doGetMouseCursor(int status) override;
    void addHistory(HistoryAction action, const RS_Vector& previous, const RS_Vector& current, int start) const;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void setStartPoint(const RS_Vector& v);
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) override;
    void updateActionPrompt() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
