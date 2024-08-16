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

#include <QObject>
#include <QtCore/QtContainerFwd>
#include <QKeyEvent>
#include <QInputEvent>
#include <QMouseEvent>

#include "rs.h"
#include "rs_snapper.h"
#include "lc_modifiersinfo.h"

class RS_CommandEvent;
class RS_CoordinateEvent;
class RS_Graphic;
class RS_Document;
class QAction;
class QString;
class LC_ActionOptionsWidget; // todo - think about depencency - options in in ui, while this action in lib... quite artificial separation, actually

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
class RS_ActionInterface : public QObject, public RS_Snapper {
    Q_OBJECT
public:
    RS_ActionInterface(const char* name,
                       RS_EntityContainer& container,
                       RS_GraphicView& graphicView,
                       RS2::ActionType actionType = RS2::ActionNone);
	   ~RS_ActionInterface() override;

    virtual RS2::ActionType rtti() const;

    void setName(const char* _name);
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
    virtual void finish(bool updateTB = true );
    virtual void setPredecessor(RS_ActionInterface* pre);
    void suspend() override;
    void resume() override;
    virtual void hideOptions();
    virtual void showOptions();
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
    int status = 0;
protected:
    /** Action name. Used internally for debugging */
    QString name;

    /**
     * This flag is set when the action has terminated and
     * can be deleted.
     */
    bool finished = false;

    /**
     * Pointer to the graphic is this container is a graphic.
     * NULL otherwise
     */
    RS_Graphic *graphic = nullptr;

    /**
    * Pointer to the document (graphic or block) or NULL.
    */

    RS_Document *document = nullptr;
    /**
     * Predecessor of this action or NULL.
     */
    RS_ActionInterface* predecessor = nullptr;


    RS2::ActionType actionType = RS2::ActionNone;

    std::unique_ptr<LC_ActionOptionsWidget> m_optionWidget;

    double snapToAngleStep = 15.0;

    QString msgAvailableCommands();
    void setActionType(RS2::ActionType actionType);
    // Accessor for drawing keys
    int getGraphicVariableInt(const QString& key, int def) const;

    void updateSelectionWidget() const;
    void updateSelectionWidget(int countSelected, double selectedLength) const;

    virtual LC_ActionOptionsWidget* createOptionsWidget();
    void updateOptions();
    void updateOptionsUI(int mode);

    virtual RS2::CursorType doGetMouseCursor(int status);
    void updateMouseCursor();
    void setMouseCursor(const RS2::CursorType &cursor);

    virtual void updateMouseButtonHints();

    // simplified mouse widget and command message operations
    void updateMouseWidgetTRBack(const QString &msg,const LC_ModifiersInfo& modifiers = LC_ModifiersInfo::NONE());
    void updateMouseWidgetTRCancel(const QString &msg,const LC_ModifiersInfo& modifiers = LC_ModifiersInfo::NONE());
    void updateMouseWidget(const QString& = QString(),const QString& = QString(), const LC_ModifiersInfo& modifiers = LC_ModifiersInfo::NONE());


    static bool isControl(const QInputEvent *e);
    static bool isShift(const QInputEvent *e);

    virtual void onMouseLeftButtonRelease(int status, QMouseEvent * e);
    virtual void onMouseRightButtonRelease(int status, QMouseEvent * e);

    void updateSnapAngleStep();
    /**
 * Method should be overridden in inherited actions to process command. Should return true if command event should be accepted.
 * @param status status
 * @param c command
 * @return true if event should be accepted, false otherwise
 */
    virtual bool doProcessCommand([[maybe_unused]]int status, const QString &command);

    bool checkCommand(const QString& cmd, const QString& str,
                      RS2::ActionType action=RS2::ActionNone);
    QString command(const QString& cmd);
    virtual QString getAdditionalHelpMessage();
    virtual QString prepareCommand(RS_CommandEvent *e) const;

    void commandMessage(const QString &msg) const;

    void fireCoordinateEvent(const RS_Vector& coord);
    void fireCoordinateEventForSnap(QMouseEvent *e);

    virtual void onCoordinateEvent(int status, bool isZero, const RS_Vector& pos);
    void initPrevious(int status);
};
#endif
