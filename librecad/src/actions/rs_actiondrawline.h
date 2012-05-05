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

#ifndef RS_ACTIONDRAWLINE_H
#define RS_ACTIONDRAWLINE_H

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to draw
 * simple lines with the start- and endpoint given.
 *
 * @author Andrew Mustun
 */
class RS_ActionDrawLine : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetStartpoint,   /**< Setting the startpoint.  */
        SetEndpoint      /**< Setting the endpoint. */
    };

public:
    RS_ActionDrawLine(RS_EntityContainer& container,
                      RS_GraphicView& graphicView);
    virtual ~RS_ActionDrawLine();

        virtual RS2::ActionType rtti() {
                return RS2::ActionDrawLine;
        }

        static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

    void reset();

    virtual void init(int status=0);
    virtual void trigger();

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

        virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();

        virtual void showOptions();
        virtual void hideOptions();

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
//    virtual void updateToolBar();
    void addHistory(const RS_Vector& v);//add history after the current point

        void close();
        void undo();
        void redo();

protected:
    RS_Vector snapToAngle(const RS_Vector& currentCoord);
     /**
    * Line data defined so far.
    */
    RS_LineData data;
        /**
         * Start point of the series of lines. Used for close function.
         */
        RS_Vector start;

        /**
         * Point history (for undo)
         */
        int historyIndex;
        QVector<RS_Vector> history;

};

#endif
