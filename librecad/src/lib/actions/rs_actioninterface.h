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

#ifndef RS_ACTIONINTERFACE_H
#define RS_ACTIONINTERFACE_H

#include <rs_math.h>

#include "lc_action_options_base.h"
#include "lc_action_options_editor.h"
#include "lc_actioncontext.h"
#include "lc_latecompletionrequestor.h"
#include "lc_modifiersinfo.h"
#include "rs.h"
#include "rs_snapper.h"

class LC_ActionOptionsPropertiesFiller;
class LC_ActionOptionsWidget;
class RS_Undoable;
class LC_ModifiersInfo;
class QInputEvent;
class QKeyEvent;
class QMouseEvent;

class RS_CommandEvent;
class RS_CoordinateEvent;
class RS_Graphic;
class RS_Document;
class QAction;
class QString;



/**
 * This is the interface that must be implemented for all
 * action classes. Action classes handle actions such
 * as drawing lines, moving entities or zooming in.
 *
 * Inherited from QObject for Qt translation features.
 *
 * @author Andrew Mustun
 */
//fixme - actually, inheritance from snapper is rather bad design... not all actions (say, file open or print-preview) should be
// inherited from snapper - only ones that really work with drawing should be snap-aware
class RS_ActionInterface : public RS_Snapper, public LC_LateCompletionRequestor, public LC_ActionOptionsBase {
Q_OBJECT
public:
    ~RS_ActionInterface() override;
    enum ActionStatus {
        InitialActionStatus = 0
    };
    virtual RS2::ActionType rtti() const;
    virtual bool isSupportsPredecessorAction() const {return false;}
    QString getName();
    virtual void init(int status);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void keyReleaseEvent(QKeyEvent* e);
    virtual void coordinateEvent(RS_CoordinateEvent*);
    virtual void commandEvent(RS_CommandEvent*);
    virtual QStringList getAvailableCommands();
    virtual void setStatus(int status);
    int getStatus() const;
    virtual void trigger();
    virtual bool isFinished() const;
    virtual void setFinished();
    void finish() override;
    virtual void setPredecessor(std::shared_ptr<RS_ActionInterface> pre);
    std::shared_ptr<RS_ActionInterface> getPredecessor() const;

    void suspend() override;
    void resume() override;
    virtual bool mayBeTerminatedExternally() {return true;}
    void hideOptions();
    void showOptions() const;
    void onLateRequestCompleted(bool shouldBeSkipped) override;
    void updateOptions(const QString& tagToFocus = "") const;
    void postCreateInit();
    virtual void tryShowRelativeInput([[maybe_unused]]RS2::RelativePointParam type) {}
private:
    /**
     * Current status of the action. After an action has
     * been created the action status is set to 0. Actions
     * that are terminated have a stats of -1. Other status
     * numbers can be used to describe the stage this action
     * is in. E.g. a window zoom consists of selecting the
     * first corner (status 0), and selecting the second
     * corner (status 1).
     */
    int m_status = 0;
protected:
    RS_ActionInterface(const QString& actionName,
                       LC_ActionContext *actionContext,
                       RS2::ActionType actionType /*= RS2::ActionNone*/);

    /**
     * This flag is set when the action has terminated and
     * can be deleted.
     */
    bool m_finished = false; // fixme- sand - review!!! Hides one from super?

    /**
     * Pointer to the graphic is this container is a graphic.
     * NULL otherwise
     */
    RS_Graphic *m_graphic = nullptr; // // fixme- sand - review!!!

    /**
     * Predecessor of this action or NULL.
     */
    std::shared_ptr<RS_ActionInterface> m_predecessor = nullptr; // fixme - sand - review!!!
    RS2::ActionType m_actionType = RS2::ActionNone;
    std::unique_ptr<LC_ActionOptionsEditor> m_optionsEditor;

    bool m_restoreRelativeInput{false};

    virtual bool mayInitWithContextEntity(int status);
    virtual void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos);
    virtual void doInitialInit();

    void switchToAction(RS2::ActionType actionType, void* data = nullptr) const;
    QString msgAvailableCommands();
    void setActionType(RS2::ActionType actionType);
    // Accessor for drawing keys
    int getGraphicVariableInt(const QString& key, int def) const;

    virtual void createOptionsEditor();
    virtual LC_ActionOptionsWidget* createOptionsWidget();
    virtual LC_ActionOptionsPropertiesFiller* createOptionsFiller();

    void updateOptionsUI(int mode, const QVariant *value = nullptr) const;

    virtual RS2::CursorType doGetMouseCursor(int status);
    void updateMouseCursor();
    void setMouseCursor(RS2::CursorType cursor) const;

    virtual void updateActionPrompt();

    // simplified mouse widget and command message operations
    void updatePromptTRBack(const QString &msg,const LC_ModifiersInfo& modifiers = LC_ModifiersInfo::NONE()) const;
    void updatePromptTRCancel(const QString &msg,const LC_ModifiersInfo& modifiers = LC_ModifiersInfo::NONE()) const;
    void updatePrompt(const QString& = QString(),const QString& = QString(), const LC_ModifiersInfo& modifiers = LC_ModifiersInfo::NONE()) const;
    void clearMouseWidgetIcon() const;

    static bool isControl(const QInputEvent *e);
    static bool isShift(const QInputEvent *e);
    static bool isAlt(const QInputEvent *e);

    virtual void onMouseLeftButtonRelease(int status, QMouseEvent * e);
    virtual void onMouseRightButtonRelease(int status, QMouseEvent * e);
    virtual void onMouseLeftButtonPress(int status, QMouseEvent * e);
    virtual void onMouseRightButtonPress(int status, QMouseEvent * e);

    /**
    * Method should be overridden in inherited actions to process command. Should return true if command event should be accepted.
    * @param status status
    * @param command command
    * @return true if event should be accepted, false otherwise
    */
    virtual bool doProcessCommand([[maybe_unused]]int status, const QString &command);

    bool checkCommand(const QString& cmd, const QString& str,
                      RS2::ActionType action=RS2::ActionType::ActionNone);
    QString command(const QString& cmd);
    virtual QString getAdditionalHelpMessage();
    virtual QString prepareCommand(RS_CommandEvent *e) const;

    void commandMessage(const QString &msg) const;
    void commandPrompt(const QString &msg) const;

    void fireCoordinateEvent(const RS_Vector& coord);

    virtual void onCoordinateEvent(int status, bool isZero, const RS_Vector& coord);
    void initPrevious(int status);
    void preparePromptForInfoCursorOverlay(const QString &msg, const LC_ModifiersInfo &modifiers) const;

     void undoCycleReplace(RS_Entity* entityToReplace, RS_Entity* entityReplacing) const;

    void setPenAndLayerToActive(RS_Entity* e);
    void suspendRelativeInputWidget() override;
    void select(RS_Entity* e) const;
    void select(const QList<RS_Entity*>& entitiesList) const;
    void unselect(const QList<RS_Entity*>& entitiesList) const;
    void unselectAll() const;
    void unselect(RS_Entity* e) const;
    void clearVisualSnap() const override;
    bool isSnapExpected() override {return isInVisualSnapStatus(getStatus());}

virtual bool doUpdateAngleByInteractiveInput([[maybe_unused]]const QString& tag,[[maybe_unused]] double angleRad) {return false;}
    virtual bool doUpdateDistanceByInteractiveInput([[maybe_unused]]const QString& tag, [[maybe_unused]]double distance) {return false;}
    virtual bool doUpdatePointByInteractiveInput([[maybe_unused]]const QString& tag, [[maybe_unused]]RS_Vector &point) {return false;}
};
#endif
