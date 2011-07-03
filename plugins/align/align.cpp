/*****************************************************************************/
/*  align.cpp - move and rotate entities using align points                  */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/


#include "document_interface.h"
#include "align.h"
#include <math.h>

QString LC_Align::name() const
 {
     return (tr("Align"));
 }

PluginCapabilities LC_Align::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("Modify", tr("Align"));
    return pluginCapabilities;
}

void LC_Align::execComm(Document_Interface *doc,
                             QWidget *parent, QString cmd)
{
    Q_UNUSED(parent);
    QPointF base1, base2, target1, target2;
    QList<Plug_Entity *> obj;
    bool yes  = doc->getSelect(&obj);
    if (!yes || obj.isEmpty()) return;
    yes = doc->getPoint(&base1, QString("first base point:"));
    if (yes) {
        yes = doc->getPoint(&target1, QString("first target point:"), &base1);
        if (yes) {
            yes = doc->getPoint(&base2, QString("second base point:"));
            if (yes) {
                yes = doc->getPoint(&target2, QString("second target point:"), &base2);
            }
        }

    }
    if (yes) {
        //first, move selection
        QPointF movev = target1 - base1;
        for (int i = 0; i < obj.size(); ++i) {
            obj.at(i)->move(movev);
        }
        //calculate angle
        double incx1, incx2, incy1, incy2, abase, atarget, angle;
        incx1 = base2.x() - base1.x();
        incx2 = target2.x() - target1.x();
        incy1 = base2.y() - base1.y();
        incy2 = target2.y() - target1.y();
        if (incx1 == 0) abase = M_PI/4;
        else abase = atan(incy1/incx1);
        if (incx2 == 0) atarget = M_PI/4;
        else atarget = atan(incy2/incx2);
        angle = atarget - abase;
        //end, rotate selection
        for (int i = 0; i < obj.size(); ++i) {
            obj.at(i)->rotate(target1, angle);
        }

    }

//selection cleanup
    while (!obj.isEmpty())
        delete obj.takeFirst();
}


Q_EXPORT_PLUGIN2(lc_align, LC_Align);
