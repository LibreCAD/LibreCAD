/*****************************************************************************/
/*  test.cpp - Change the properties to be the same of first selected        */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#include <QMessageBox>
#include <QVariant>

#include "document_interface.h"
#include "sameprop.h"

QString LC_SameProp::name() const
 {
     return (tr("Same properties"));
 }

PluginCapabilities LC_SameProp::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Same properties"));
    return pluginCapabilities;
}

void LC_SameProp::execComm(Document_Interface *doc,
                             QWidget *parent, QString cmd)
{
    Q_UNUSED(parent);
    Q_UNUSED(cmd);
    QHash<int, QVariant> data, moddata;
    QList<Plug_Entity *> obj;
    QVariant lay, col, ltype, lwidth;
    Plug_Entity *ent, *modent;
    ent =  doc->getEnt(tr("select original entity:"));
    if (!ent) return;
    bool yes  = doc->getSelect(&obj, tr("select entities to change"));
    if (!yes || obj.isEmpty()) {
        delete ent;
        return;
    }

    ent->getData(&data);
    lay = data.value(DPI::LAYER);
    col = data.value(DPI::COLOR);
    ltype = data.value(DPI::LTYPE);
    lwidth = data.value(DPI::LWIDTH);
    for (int i = 0; i < obj.size(); ++i) {
        modent = obj.at(i);
        modent->getData(&moddata);
        moddata.insert(DPI::LAYER, lay );
        moddata.insert(DPI::LTYPE, ltype );
        moddata.insert(DPI::LWIDTH, lwidth );
        moddata.insert(DPI::COLOR, col );
        modent->updateData(&moddata);
    }
    while (!obj.isEmpty())
        delete obj.takeFirst();
}
