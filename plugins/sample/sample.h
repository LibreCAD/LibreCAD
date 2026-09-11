/*****************************************************************************/
/*  sample.h - plugin example for LibreCAD                                   */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#ifndef SAMPLE_H
#define SAMPLE_H

#include "qc_plugininterface.h"
#include <QDialog>
class QLineEdit;

class LC_Sample : public QObject, QC_PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QC_PluginInterface)
    Q_PLUGIN_METADATA(IID LC_DocumentInterface_iid FILE  "sample.json")

 public:
    PluginCapabilities getCapabilities() const override;
    QString name() const override;
    void init(Document_Interface *doc, QWidget *parent) override;
    void execComm(QString cmd) override;
};

class lc_Sampledlg : public QDialog
{
    Q_OBJECT
public:
    explicit lc_Sampledlg(QWidget *parent = 0);
    ~lc_Sampledlg() override;

public slots:
//    void processAction(QStringList *commandList);
    void processAction(Document_Interface *doc) const;
    void checkAccept();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void readSettings();
    void writeSettings() const;
    bool failGUI(QString *msg) const;

private:
    QString errmsg;
    QLineEdit *startxedit;
    QLineEdit *startyedit;
    QLineEdit *endxedit;
    QLineEdit *endyedit;
};

#endif
