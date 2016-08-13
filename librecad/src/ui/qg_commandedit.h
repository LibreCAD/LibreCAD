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

#ifndef QG_COMMANDEDIT_H
#define QG_COMMANDEDIT_H

#include <QLineEdit>
#include <QString>
#include <QMap>

/**
 * A command line edit with some typical console features 
 * (uparrow for the history, tab, ..).
 */
class QG_CommandEdit: public QLineEdit {
    Q_OBJECT

public:
    QG_CommandEdit(QWidget* parent=0);
    virtual ~QG_CommandEdit()=default;

    void readCommandFile(const QString& path);

    bool keycode_mode;

protected:
	virtual bool event(QEvent* e);
	virtual void keyPressEvent(QKeyEvent* e);
	virtual void focusInEvent(QFocusEvent *e);
	virtual void focusOutEvent(QFocusEvent *e);
    void evaluateExpression(QString input);

    QString relative_ray;
    QMap<QString, QString> variables;

    void processInput(QString input);
    bool isForeignCommand(QString input);
    void processVariable(QString input);

signals:
	void tabPressed();
	void escape();
	void focusIn();
	void focusOut();
    void clearCommandsHistory();
    void command(QString cmd);
    void message(QString msg);
    void keycode(QString code);

private:
	QStringList historyList;
	QStringList::Iterator it;
	bool acceptCoordinates;
    bool calculator_mode;

public slots:
    void modifiedPaste();
};

#endif

