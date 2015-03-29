/****************************************************************************
**
 * Draw a common tangent circle of 3 existing circles
 * Problem of Appollonius

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

#ifndef RS_ACTIONDRAWCIRCLETAN3_H
#define RS_ACTIONDRAWCIRCLETAN3_H

#include <vector>
#include <QVector>
#include "rs_previewactioninterface.h"

struct RS_CircleData;
class RS_AtomicEntity;

/**
 * Draw Common tangential circle of 3 given circles, i.e. Appollonius's problem
 *
 * @author Dongxu Li
 */
class RS_ActionDrawCircleTan3 : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        SetCircle1,   //  Setting the First Circle.  */
        SetCircle2,   //  Setting the Second Circle.  */
        SetCircle3,   //  Setting the Third Circle.  */
        SetCenter   //  select the closest tangential Circle.  */
    };

public:
    RS_ActionDrawCircleTan3(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView);
	~RS_ActionDrawCircleTan3()=default;

    static QAction* createGUIAction(RS2::ActionType type, QObject* /*parent*/);

    virtual RS2::ActionType rtti() {
        return RS2::ActionDrawCircleTan3;
    }
    virtual void init(int status=0);

    virtual void trigger();
    virtual bool getData();
    virtual bool preparePreview();

    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);

    //        virtual void coordinateEvent(RS_CoordinateEvent* e);
    //    virtual void commandEvent(RS_CommandEvent* e);
    virtual QStringList getAvailableCommands();
    virtual void finish(bool updateTB=true);
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
//    virtual void updateToolBar();

//    virtual void showOptions();
//    virtual void hideOptions();



//protected:
    private:
	std::vector<double> verifyCenter(const RS_Vector& center) const;
	std::vector<double> getRadii(RS_AtomicEntity* entity, const RS_Vector& center) const;
    RS_Entity* catchCircle(QMouseEvent* e);
	std::vector<RS_AtomicEntity*> circles;
	std::shared_ptr<RS_CircleData> cData;
    RS_Vector coord;
    bool valid;
	const QVector<RS2::EntityType> enTypeList={RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
	//keep a list of centers found
	std::vector<std::shared_ptr<RS_CircleData> > candidates;
    RS_VectorSolutions centers;

};

#endif
