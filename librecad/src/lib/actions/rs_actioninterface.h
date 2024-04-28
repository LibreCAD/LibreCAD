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

#include "rs_snapper.h"
#include "lc_actionoptionswidget.h"

class RS_CommandEvent;
class RS_CoordinateEvent;
class RS_Graphic;
class RS_Document;
class QAction;
class QKeyEvent;
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
class RS_ActionInterface : public QObject, public RS_Snapper {
    Q_OBJECT
public:
    RS_ActionInterface(const char* name,
                       RS_EntityContainer& container,
                       RS_GraphicView& graphicView,
                       RS2::ActionType actionType = RS2::ActionNone);
	   virtual ~RS_ActionInterface();

    virtual RS2::ActionType rtti() const;

    void setName(const char* _name);
    QString getName();

    virtual void init(int status=0);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mousePressEvent(QMouseEvent*);

    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void keyReleaseEvent(QKeyEvent* e);
    virtual void coordinateEvent(RS_CoordinateEvent*);
    virtual void commandEvent(RS_CommandEvent*);
    virtual QStringList getAvailableCommands();
    virtual void setStatus(int status);
    virtual int getStatus() const;
    virtual void trigger();
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual bool isFinished() const;
    virtual void setFinished();
    virtual void finish(bool updateTB = true );
    virtual void setPredecessor(RS_ActionInterface* pre);
    void suspend() override;
    void resume() override;
    void hideOptions() override;
    void showOptions() override;
    void setActionType(RS2::ActionType actionType);
    bool checkCommand(const QString& cmd, const QString& str,
                             RS2::ActionType action=RS2::ActionNone);
    QString command(const QString& cmd);
    QString msgAvailableCommands();

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
     * Pointer to the default mouse cursor for this action or NULL.
     */
    //RS2::CursorType cursor;

    /**
     * Predecessor of this action or NULL.
     */
    RS_ActionInterface* predecessor = nullptr;

    /**
     * String prepended to the help text for currently available commands.
     */
    //static QString msgAvailableCommands;

    /**
     * Command used for showing help for every action.
     */
    //static QString cmdHelp;

    /**
     * Command for answering yes to a question.
     */
    //static QString cmdYes;
    //static QString cmdYes2;

     /**
     * Command for answering no to a question.
     */
    //static QString cmdNo;
    //static QString cmdNo2;
    RS2::ActionType actionType = RS2::ActionNone;

    std::unique_ptr<LC_ActionOptionsWidget> m_optionWidget;

    virtual void createOptionsWidget();
    void updateOptions();
    void updateOptionsUI(int mode);
};


#endif
