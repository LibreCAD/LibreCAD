/*****************************************************************************/
/*  sample.cpp - plugin example for LibreCAD                                 */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSettings>
#include <QMessageBox>
#include <QDoubleValidator>

#include "document_interface.h"
#include "sample.h"

QString LC_Sample::name() const
 {
     return (tr("Sample plugin"));
 }

PluginCapabilities LC_Sample::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Sample plugin"));
    return pluginCapabilities;
}

void LC_Sample::execComm(Document_Interface *doc,
                             QWidget *parent, QString cmd)
{
    Q_UNUSED(doc);
    Q_UNUSED(cmd);
    lc_Sampledlg pdt(parent);
    int result =  pdt.exec();
    if (result == QDialog::Accepted)
        pdt.processAction(doc);
}




/*****************************/
lc_Sampledlg::lc_Sampledlg(QWidget *parent) :  QDialog(parent)
{
    setWindowTitle(tr("Draw line"));
    QLabel *label;

    QDoubleValidator *val = new QDoubleValidator(0);
    QGridLayout *mainLayout = new QGridLayout;

    label = new QLabel(tr("Start X:"));
    mainLayout->addWidget(label, 0, 0);
    startxedit = new QLineEdit();
    startxedit->setValidator(val);
    mainLayout->addWidget(startxedit, 1, 0);

    label = new QLabel(tr("Start Y:"));
    mainLayout->addWidget(label, 0, 1);
    startyedit = new QLineEdit();
    startyedit->setValidator(val);
    mainLayout->addWidget(startyedit, 1, 1);

    label = new QLabel(tr("End X:"));
    mainLayout->addWidget(label, 2, 0);
    endxedit = new QLineEdit();
    endxedit->setValidator(val);
    mainLayout->addWidget(endxedit, 3, 0);

    label = new QLabel(tr("End Y:"));
    mainLayout->addWidget(label, 2, 1);
    endyedit = new QLineEdit();
    endyedit->setValidator(val);
    mainLayout->addWidget(endyedit, 3, 1);


    QHBoxLayout *loaccept = new QHBoxLayout;
    QPushButton *acceptbut = new QPushButton(tr("Accept"));
    loaccept->addStretch();
    loaccept->addWidget(acceptbut);
    mainLayout->addLayout(loaccept, 4, 0);

    QPushButton *cancelbut = new QPushButton(tr("Cancel"));
    QHBoxLayout *locancel = new QHBoxLayout;
    locancel->addWidget(cancelbut);
    locancel->addStretch();
    mainLayout->addLayout(locancel, 4, 1);

    setLayout(mainLayout);
    readSettings();

    connect(cancelbut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptbut, SIGNAL(clicked()), this, SLOT(checkAccept()));
}


bool lc_Sampledlg::failGUI(QString *msg)
{
    if (startxedit->text().isEmpty()) {msg->insert(0, tr("Start X is empty")); return true;}
    if (startyedit->text().isEmpty()) {msg->insert(0, tr("Start Y is empty")); return true;}
    if (endxedit->text().isEmpty()) {msg->insert(0, tr("End X is empty")); return true;}
    if (endyedit->text().isEmpty()) {msg->insert(0, tr("End Y is empty")); return true;}
    return false;
}


void lc_Sampledlg::processAction(Document_Interface *doc)
{
    Q_UNUSED(doc);
    QPointF start, end;
    start.setX(startxedit->text().toDouble());
    start.setY(startyedit->text().toDouble());
    end.setX(endxedit->text().toDouble());
    end.setY(endyedit->text().toDouble());

    doc->addLine(&start, &end);
}

void lc_Sampledlg::checkAccept()
{

    errmsg.clear();
    if (failGUI(&errmsg)) {
        QMessageBox::critical ( this, tr("Sample plugin"), errmsg );
        errmsg.clear();
        return;
    }
    accept();
}


lc_Sampledlg::~lc_Sampledlg()
{
}
void lc_Sampledlg::closeEvent(QCloseEvent *event)
 {
    writeSettings();
    QWidget::closeEvent(event);
 }


void lc_Sampledlg::readSettings()
 {
    QString str;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "sample_plugin");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(430,140)).toSize();

    startxedit->setText( settings.value("startx", 0.5).toString() );
    startyedit->setText( settings.value("starty", 0.5).toString() );
    endxedit->setText( settings.value("endx", 3.5).toString() );
    endyedit->setText( settings.value("endy", 3.5).toString() );

    resize(size);
    move(pos);
 }

void lc_Sampledlg::writeSettings()
 {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "sample_plugin");
    settings.setValue("pos", pos());
    settings.setValue("size", size());

    settings.setValue("startx", startxedit->text());
    settings.setValue("starty", startyedit->text());
    settings.setValue("endx", endxedit->text());
    settings.setValue("endy", endyedit->text());
 }
