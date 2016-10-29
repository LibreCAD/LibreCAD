/*****************************************************************************/
/*  gear.h - gear plugin for LibreCAD                                      */
/*                                                                           */
/*  Copyright (C) 2016 Cédric Bosdonnat cedric@bosdonnat.fr                  */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#ifndef WHEEL_H
#define WHEEL_H

#include "qc_plugininterface.h"
#include <QDialog>

class QPointF;
class QSpinBox;
class QDoubleSpinBox;

class LC_Gear : public QObject, QC_PluginInterface
{
    Q_OBJECT
     Q_INTERFACES(QC_PluginInterface)
     Q_PLUGIN_METADATA(IID "org.librecad.gear" FILE  "gear.json")

 public:
    virtual PluginCapabilities getCapabilities() const;
    virtual QString name() const;
    virtual void execComm(Document_Interface *doc,
                          QWidget *parent, QString cmd);
};

class lc_Geardlg : public QDialog
{
    Q_OBJECT

public:
    explicit lc_Geardlg(QWidget *parent, QPointF *center);
    ~lc_Geardlg();

public slots:
    void processAction(Document_Interface *doc);
    void checkAccept();
    void pitchChanged(double d);

protected:
    void closeEvent(QCloseEvent *event);

private:
    void readSettings();
    void writeSettings();

private:

    QPointF *center;
    QSpinBox *nteethBox;
    QDoubleSpinBox *pitchBox;
    QDoubleSpinBox *pressureBox;
    QDoubleSpinBox *addendumBox;
    QDoubleSpinBox *dedendumBox;
};

#endif // WHEEL_H
