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


#include "lc_graphicviewawarewidget.h"
#include "ui_qg_commandwidget.h"

class QG_ActionHandler;
class QAction;

class QG_CommandWidget : public LC_GraphicViewAwareWidget, public Ui::QG_CommandWidget{
    Q_OBJECT
public:
    explicit QG_CommandWidget(QG_ActionHandler *actionHandler, QWidget *parent, const char *name, Qt::WindowFlags fl = {});
    ~QG_CommandWidget() override;

    bool eventFilter(QObject *obj, QEvent *event) override;
    QAction* getDockingAction() const {
        return m_docking;
    }
    void setInput(const QString &cmd) const;
    void setGraphicView([[maybe_unused]]RS_GraphicView* gview) override {}
protected:
    QLayout* getTopLevelLayout() const override {return layout();}
public slots:
    virtual void focusWidget();
    void setCommand( const QString & cmd ) const;
    void appendHistory( const QString & msg ) const;
    void handleCommand(QString cmd) const;
    void handleKeycode(const QString& code) const;
    void spacePressed() const;
    void tabPressed() const;
    void escape() const;
    void setActionHandler( QG_ActionHandler * ah );
    void setCommandMode() const;
    void setNormalMode() const;
    static QString getRootCommand( const QStringList & cmdList, const QString & typed );
    void setKeycodeMode(bool state) const;
protected slots:
    void languageChange();
    void chooseCommandFile();
private slots:
    void dockingButtonTriggered(bool);
private:
    QG_ActionHandler* m_actionHandler = nullptr;
    QAction* m_docking = nullptr;
};

#endif
