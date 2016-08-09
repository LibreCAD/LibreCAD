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
#include <cmath>

QString LC_Align::name() const
 {
     return (tr("Align"));
 }

PluginCapabilities LC_Align::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Align"));
    return pluginCapabilities;
}

void LC_Align::execComm(Document_Interface *doc,
                             QWidget *parent, QString cmd)
{
    Q_UNUSED(parent);
    Q_UNUSED(cmd);
    QPointF base1, base2, target1, target2;
    QList<Plug_Entity *> obj;
    bool yes  = doc->getSelect(&obj);
    if (!yes || obj.isEmpty()) return;
    yes = doc->getPoint(&base1, QString(tr("first base point:")));
    if (yes) {
        yes = doc->getPoint(&target1, QString(tr("first target point:")), &base1);
        if (yes) {
            yes = doc->getPoint(&base2, QString(tr("second base point:")));
            if (yes) {
                yes = doc->getPoint(&target2, QString(tr("second target point:")), &base2);
            }
        }

    }
    if (yes) {
        //first, move selection
        QPointF movev = target1 - base1;

        //calculate angle
        double abase, atarget, angle;
        abase = atan2( base2.y() - base1.y(),
                       base2.x() - base1.x());
        atarget = atan2( target2.y() - target1.y(),
                         target2.x() - target1.x());
        angle = atarget - abase;
        //end, rotate selection
        for (int i = 0; i < obj.size(); ++i) {
			obj.at(i)->moveRotate(movev, target1, angle);
        }

    }

//selection cleanup
    while (!obj.isEmpty())
        delete obj.takeFirst();
}
