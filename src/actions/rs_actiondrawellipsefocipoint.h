/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2, or (at your option) any later
** versionas published by the Free Software
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

#ifndef RS_ACTIONDRAWELLIPSEFOCIPOINT_H
#define RS_ACTIONDRAWELLIPSEFOCIPOINT_H

#include "rs_previewactioninterface.h"
#include "rs_ellipse.h"

/**
 * Draw ellipse by foci and a point on ellipse
 *
 * @author Dongxu Li
 */
class RS_ActionDrawEllipseFociPoint : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetFocus1,   //  Setting the first focus.  */
        SetFocus2,    //  Setting the second focus. */
        SetPoint    //  Setting a point on ellipse
    };

public:
    RS_ActionDrawEllipseFociPoint(RS_EntityContainer& container,
                             RS_GraphicView& graphicView);
    ~RS_ActionDrawEllipseFociPoint();

        static QAction* createGUIAction(RS2::ActionType type, QObject* /*parent*/);

    virtual void init(int status=0);

    virtual void trigger();

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

        virtual void coordinateEvent(RS_CoordinateEvent* e);
//    virtual void commandEvent(RS_CommandEvent* e);
        virtual QStringList getAvailableCommands();

    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();

protected:
    // Foci of ellipse
    RS_Vector focus1,focus2;
    // A point on ellipse
    RS_Vector point;
    private:
    RS_Vector center,major;
    double c; //hold half of distance between foci
    double d; //hold half of distance
};

#endif
