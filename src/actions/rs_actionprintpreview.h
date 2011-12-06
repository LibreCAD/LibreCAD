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

#ifndef RS_ACTIONPRINTPREVIEW_H
#define RS_ACTIONPRINTPREVIEW_H

#include "rs_actioninterface.h"

/**
 * Default action for print preview.
 *
 * @author Andrew Mustun
 */
class RS_ActionPrintPreview : public RS_ActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
                Neutral,
                Moving
    };

public:
    RS_ActionPrintPreview(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
    ~RS_ActionPrintPreview();

        static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

        virtual RS2::ActionType rtti() {
                return RS2::ActionPrintPreview;
        }

    virtual void init(int status=0);
    virtual void resume();

    virtual void trigger();

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

    virtual void coordinateEvent(RS_CoordinateEvent* e);
    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();

        virtual void showOptions();
        virtual void hideOptions();

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

        void center();
        void fit();
        bool setScale(double f);
        double getScale();
        //print warning message to command widget
        //should we add this as virtual function to rs_actioninterface?
        void printWarning(const QString& s);

        void setBlackWhite(bool bw);
        //bool isBlackWhite() {
        //	return blackWhite;
        //}
        RS2::Unit getUnit();

protected:
        //bool blackWhite;
        bool hasOptions;
        RS_Vector v1;
        RS_Vector v2;
};

#endif
