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
#include "qg_commandwidget.h"
#include <QKeyEvent>

#include "qg_actionhandler.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_system.h"
#include "rs_utility.h"

/*
 *  Constructs a QG_CommandWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CommandWidget::QG_CommandWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, fl), actionHandler(nullptr)
{
    setObjectName(name);
    setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CommandWidget::~QG_CommandWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CommandWidget::languageChange()
{
    retranslateUi(this);
}

bool QG_CommandWidget::eventFilter(QObject */*obj*/, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent* e=static_cast<QKeyEvent*>(event);
		//		qDebug()<<QString::number(e->key(), 16);
		switch(e->key()){
		case Qt::Key_Return:
		case Qt::Key_Enter:
			if(!leCommand->text().size())
				return false;
			else
				break;
		case Qt::Key_Escape:
			//			DEBUG_HEADER
			//			qDebug()<<"Not filtered";
			return false;
		default:
			break;
		}
		//detect Ctl- Alt- modifier, but not Shift
		//This should avoid filtering shortcuts, such as Ctl-C
		if(e->modifiers() & (Qt::KeyboardModifierMask ^ Qt::ShiftModifier)) return false;
		event->accept();
		QKeyEvent * newEvent = new QKeyEvent(*static_cast<QKeyEvent*>(event));
		QApplication::postEvent(leCommand, newEvent);
		this->setFocus();
		//			DEBUG_HEADER
		//			qDebug()<<"Filtered";
		return true;
	}
	return false;
}

void QG_CommandWidget::setFocus() {
    //setCommandMode();
	QFocusEvent* newEvent=new QFocusEvent(QEvent::FocusIn);
	QApplication::postEvent(leCommand, newEvent);
    leCommand->setFocus();
}

void QG_CommandWidget::setCommand(const QString& cmd) {
    if (cmd!="") {
        lCommand->setText(cmd);
    } else {
        lCommand->setText(tr("Command:"));
    }
    leCommand->setText("");
}

void QG_CommandWidget::appendHistory(const QString& msg) {
    teHistory->append(msg);
}

void QG_CommandWidget::trigger() {
    QString cmd = leCommand->text();
    cmd = cmd.simplified();
    bool isAction=false;
    if (cmd=="") {
        cmd="\n";
    } else {
        appendHistory(cmd);
    }

    if (actionHandler) {
        isAction=actionHandler->command(cmd);
    }

    if (!isAction && cmd!="\n" && !(cmd.contains(',') || cmd.at(0)=='@')) {
       appendHistory(tr("Unknown command: %1").arg(cmd));
    }

    leCommand->setText("");
}

void QG_CommandWidget::tabPressed() {
    if (actionHandler) {
        QStringList reducedChoice;
        QString typed = leCommand->text();
        QStringList choice;

        // check current command:
        choice = actionHandler->getAvailableCommands();
        if (choice.count()==0) {
            choice = RS_COMMANDS->complete(typed);
        }

        for (QStringList::Iterator it = choice.begin(); it != choice.end(); ++it) {
            if (typed.isEmpty() || (*it).startsWith(typed)) {
                reducedChoice << (*it);
            }
        }

        // command found:
        if (reducedChoice.count()==1) {
            leCommand->setText(reducedChoice.first());
        }
        else if (reducedChoice.count()>0) {
        //TODO: unix-like behaviour for autocompletion
            QString longestString = "";
            QString shortestString = "";
            int lengthShortestString(0);
            int lengthReducedChoice = reducedChoice.count();

            // Finding which is the longest string
            for(QStringList::Iterator it = reducedChoice.begin(); it != reducedChoice.end(); ++it) {
                if((*it).length() > longestString.length()) {
                    longestString = (*it);
                }
            }
            int lengthLongestString = longestString.length();

            // Finding which is the shortest string
            lengthShortestString = longestString.length();
            for(QStringList::Iterator it = reducedChoice.begin(); it != reducedChoice.end(); ++it) {
                if((*it).length() < lengthShortestString) {
                    shortestString = (*it);
                }
            }
            lengthShortestString = shortestString.length();

            // Now we parse the reducedChoice list, character of each item by character.
            int i(0);
            int pos(0);
            bool common = true;

            for(i = 0; i < lengthShortestString; ++i) {
                for(QStringList::Iterator it = reducedChoice.begin(); it != reducedChoice.end(); ++it) {
                    if(longestString.at(i) != (*it).at(i)) {
                        common = false;
                        break;
                    }
                }
                if(common == true) {
                    ++pos;
                }
            }

            QString proposal = longestString.left(pos);
            appendHistory(reducedChoice.join(", "));
            leCommand -> setText(proposal);
        }
    }
}

void QG_CommandWidget::escape() {
    //leCommand->clearFocus();
    if (actionHandler) {
        actionHandler->slotFocusNormal();
        actionHandler->command(QString(tr("escape", "escape, go back from action steps")));
    }
}

void QG_CommandWidget::setActionHandler(QG_ActionHandler* ah) {
    actionHandler = ah;
}

void QG_CommandWidget::setCommandMode() {
    QPalette palette;
    palette.setColor(lCommand->foregroundRole(), Qt::blue);
    lCommand->setPalette(palette);
}

void QG_CommandWidget::setNormalMode() {
    QPalette palette;
    palette.setColor(lCommand->foregroundRole(), Qt::black);
    lCommand->setPalette(palette);
}
