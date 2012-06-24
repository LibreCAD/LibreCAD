/****************************************************************************
**
 * Draw ellipse by foci and a point on ellipse

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#ifndef RS_ACTIONDRAWCIRCLETAN1_2P_H
#define RS_ACTIONDRAWCIRCLETAN1_2P_H

#include <QVector>
#include "rs_previewactioninterface.h"
//#include "rs_ellipse.h"

class RS_Circle;

/**
 * Draw ellipse by foci and a point on ellipse
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleTan1_2P : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetCircle1=0,   //  Setting the First Circle.  */
        SetPoint1=1,   //  Setting the Second Circle.  */
        SetPoint2=2,   //  select the closest tangential Circle.  */
        SetCenter   //  select the closest tangential Circle.  */
    };

public:
    RS_ActionDrawCircleTan1_2P(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView);
    ~RS_ActionDrawCircleTan1_2P();

    static QAction* createGUIAction(RS2::ActionType type, QObject* /*parent*/);

    virtual RS2::ActionType rtti() {
        return RS2::ActionDrawCircleTan1_2P;
    }
    virtual void init(int status=0);

    virtual void trigger();
    virtual bool getCenters();
    virtual bool preparePreview();

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

            virtual void coordinateEvent(RS_CoordinateEvent* e);
//        virtual void commandEvent(RS_CommandEvent* e);
    virtual QStringList getAvailableCommands();
    virtual void finish(bool updateTB=true);
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
//    virtual void updateToolBar();

//    virtual void showOptions();
//    virtual void hideOptions();
//    void setRadius(const double& r);
    double getRadius(){
        return cData.radius;
    }


protected:
    RS_Entity* catchCircle(QMouseEvent* e);
    RS_AtomicEntity* circle;
    QVector<RS_Vector> points;
    private:
    RS_CircleData cData;
    RS_Vector coord;
    double radius;
    bool valid;
    QVector<RS2::EntityType> enTypeList;
    //keep a list of centers found
    RS_VectorSolutions centers;

};

#endif
