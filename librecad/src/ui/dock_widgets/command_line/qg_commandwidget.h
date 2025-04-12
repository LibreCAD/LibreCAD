/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
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
#ifndef QG_COMMANDWIDGET_H
#define QG_COMMANDWIDGET_H

#include "ui_qg_commandwidget.h"

class QG_ActionHandler;
class QAction;

class QG_CommandWidget : public QWidget, public Ui::QG_CommandWidget{
    Q_OBJECT
public:
    QG_CommandWidget(QG_ActionHandler *action_handler, QWidget *parent = nullptr, const char *name = nullptr, Qt::WindowFlags fl = {});
    ~QG_CommandWidget();

    bool eventFilter(QObject *obj, QEvent *event) override;
    QAction* getDockingAction() const {
        return m_docking;
    }
    void setInput(const QString &cmd);
public slots:
    virtual void setFocus();
    void setCommand( const QString & cmd );
    void appendHistory( const QString & msg );
    void handleCommand(QString cmd);
    void handleKeycode(QString code);
    void spacePressed();
    void tabPressed();
    void escape();
    void setActionHandler( QG_ActionHandler * ah );
    void setCommandMode();
    void setNormalMode();
    static QString getRootCommand( const QStringList & cmdList, const QString & typed );
    void setKeycodeMode(bool state);
protected slots:
    void languageChange();
    void chooseCommandFile();
private slots:
    void dockingButtonTriggered(bool);
private:
    QG_ActionHandler* m_actionHandler = nullptr;
    QAction* m_docking = nullptr;
};

#endif // QG_COMMANDWIDGET_H
