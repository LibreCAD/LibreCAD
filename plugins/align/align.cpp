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

#include <cmath>

#include "document_interface.h"
#include "align.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QSettings>

QString LC_Align::name() const
 {
     return (tr("Align"));
 }

PluginCapabilities LC_Align::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Align"))
	    << PluginMenuLocation("plugins_menu", tr("Align settings..."));
    return pluginCapabilities;
}

void LC_Align::execComm(Document_Interface *doc,
                             QWidget *parent, QString cmd)
{
    Q_UNUSED(parent);

    /* First load the settings */
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
		       "LibreCAD", "align_plugin");
    bool keep_orig = settings.value("keep_original", false).toBool();
    bool base_first = settings.value("base_first", false).toBool();
    bool acting = true;

    if (cmd.length() > 6) { // cheapo settings dialog, could be improved
        QCheckBox* cbkeep = new QCheckBox(tr("Keep original objects"));
        QCheckBox* cbbase = new QCheckBox(tr("Specify base points first"));
	cbkeep->setChecked(keep_orig);
	cbbase->setChecked(base_first);
        QMessageBox setdlg;
	setdlg.setWindowTitle(tr("Align Settings"));
	setdlg.addButton(cbkeep, QMessageBox::ActionRole);
	setdlg.addButton(cbbase, QMessageBox::ActionRole);
	setdlg.setText(tr("Click on options to set/unset,\n"
			  "Ok to accept and start alignment."));
	setdlg.setDetailedText(
	       tr("If 'Keep original objects' is checked,\n"
		  "Align will copy rather than move the selected objects.\n\n"
		  "If 'Specify base points first' is checked,\n"
		  "Align will prompt for the alignment points in the order\n"
		  "first base, second base, first target, second target.")
			       );
	QPushButton* okpushbtn = setdlg.addButton(QMessageBox::Ok);
	QAbstractButton* okbtn = (QAbstractButton*)okpushbtn;
	while (acting) {
	    setdlg.exec();
	    keep_orig = cbkeep->isChecked();
	    base_first = cbbase->isChecked();
	    if (setdlg.clickedButton() == okbtn) acting = false;
	}
	settings.setValue("keep_original", keep_orig);
	settings.setValue("base_first", base_first);
    }
    QPointF base1, base2, target1, target2;
    QList<Plug_Entity *> obj;
    bool yes  = doc->getSelect(&obj);
    if (!yes || obj.isEmpty()) return;
    yes = doc->getPoint(&base1, QString(tr("first base point:")));
    while (yes) {
        if (base_first)
	     yes = doc->getPoint(&base2, QString(tr("second base point:")));
	if (!yes) break;
        yes = doc->getPoint(&target1, QString(tr("first target point:")), &base1);
	if (!yes) break;
	if (!base_first)
	    yes = doc->getPoint(&base2, QString(tr("second base point:")));
	if (!yes) break;
	yes = doc->getPoint(&target2, QString(tr("second target point:")), &base2);
	break;
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
	DPI::Disposition whattodo =
	    keep_orig ? DPI::KEEP_ORIGINAL : DPI::DELETE_ORIGINAL;
        for (int i = 0; i < obj.size(); ++i) {
	    obj.at(i)->moveRotate(movev, target1, angle, whattodo);
        }

    }

//selection cleanup
    while (!obj.isEmpty())
        delete obj.takeFirst();
}
