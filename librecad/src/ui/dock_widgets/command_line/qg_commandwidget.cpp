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

#include <QDockWidget>
#include <QFileDialog>
#include <QKeyEvent>

#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "rs_commands.h"
#include "rs_settings.h"
/*
 *  Constructs a QG_CommandWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CommandWidget::QG_CommandWidget(QG_ActionHandler *action_handler, QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, fl)
    , m_actionHandler(action_handler)
{
    setObjectName(name);
    setupUi(this);
    connect(leCommand, &QG_CommandEdit::command, this, &QG_CommandWidget::handleCommand);
    connect(leCommand, &QG_CommandEdit::escape, this, &QG_CommandWidget::escape);
    connect(leCommand, &QG_CommandEdit::focusOut, this, &QG_CommandWidget::setNormalMode);
    connect(leCommand, &QG_CommandEdit::focusIn, this, &QG_CommandWidget::setCommandMode);
    connect(leCommand, &QG_CommandEdit::spacePressed, this, &QG_CommandWidget::spacePressed);
    connect(leCommand, &QG_CommandEdit::tabPressed, this, &QG_CommandWidget::tabPressed);
    connect(leCommand, &QG_CommandEdit::clearCommandsHistory, teHistory, &QG_CommandHistory::clear);
    connect(leCommand, &QG_CommandEdit::message, this, &QG_CommandWidget::appendHistory);
    connect(leCommand, &QG_CommandEdit::keycode, this, &QG_CommandWidget::handleKeycode);

    auto a1 = new QAction(QObject::tr("Keycode mode"), this);
    a1->setObjectName("keycode_action");
    a1->setCheckable(true);
    connect(a1, &QAction::toggled, this, &QG_CommandWidget::setKeycodeMode);
    options_button->addAction(a1);

    if (LC_GET_ONE_BOOL("Widgets","KeycodeMode", false)){
        leCommand->m_keycode_mode = true;
        a1->setChecked(true);
    }

    auto a2 = new QAction(QObject::tr("Load command file"), this);
    connect(a2, &QAction::triggered, this, &QG_CommandWidget::chooseCommandFile);
    options_button->addAction(a2);

    auto a3 = new QAction(QObject::tr("Paste multiple commands"), this);
    connect(a3, &QAction::triggered, leCommand, &QG_CommandEdit::modifiedPaste);
    options_button->addAction(a3);

    options_button->setStyleSheet("QToolButton::menu-indicator { image: none; }");

    // For convenience of re-docking a floating command widget. Without this button,
    // the title bar may not have a "dock" button.
    // The m_docking button allows user to re-dock the command widget
    m_docking = new QAction(tr("Dock"), this);
    addAction(m_docking);
    connect(m_docking, &QAction::triggered, this, &QG_CommandWidget::dockingButtonTriggered);

    options_button->addAction(m_docking);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CommandWidget::~QG_CommandWidget(){
    auto action = findChild<QAction*>("keycode_action");
    LC_SET_ONE("Widgets", "KeycodeMode", action->isChecked());
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CommandWidget::languageChange(){
    retranslateUi(this);
}

bool QG_CommandWidget::eventFilter(QObject */*obj*/, QEvent *event){
    if (event != nullptr && event->type() == QEvent::KeyPress) {
        auto e=static_cast<QKeyEvent*>(event);

        int key {e->key()};
        switch(key) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if(!leCommand->text().size())
                return false;
            else
                break;
        case Qt::Key_Escape:
            return false;
        case Qt::Key_Space:
            if (!hasFocus() && LC_GET_BOOL("Keyboard/ToggleFreeSnapOnSpace", false)) {
                // do not take focus here
                spacePressed();
                e->accept();
                return true;
            }
            break;
        default:
            break;
        }

        //detect Ctl- Alt- modifier, but not Shift
        //This should avoid filtering shortcuts, such as Ctl-C
        Qt::KeyboardModifiers  modifiers {e->modifiers()};
        if ( !(Qt::GroupSwitchModifier == modifiers && Qt::Key_At == key) // let '@' key pass for relative coords
          && modifiers != Qt::KeypadModifier
          && modifiers & (Qt::KeyboardModifierMask ^ Qt::ShiftModifier)) {
            return false;
        }

        this->setFocus();
        QApplication::postEvent(leCommand, e->clone());
        e->accept();

        return true;
    }

    return false;
}

void QG_CommandWidget::setFocus(){
    if (!isActiveWindow())
        activateWindow();

    auto newEvent = new QFocusEvent(QEvent::FocusIn);
	QApplication::postEvent(leCommand, newEvent);
    leCommand->setFocus();
}

void QG_CommandWidget::setCommand(const QString& cmd) {
    if (cmd.isEmpty()) {
        lCommand->setText(tr("Enter Command:"));
    }
    else {
        if (!cmd.endsWith(":")){
            lCommand->setText(cmd+":");
        }
        else{
            lCommand->setText(cmd);
        }
    }
}

void QG_CommandWidget::setInput(const QString& cmd) {
    leCommand->setText(cmd);
    leCommand->setFocus();
}

void QG_CommandWidget::appendHistory(const QString& msg) {
    teHistory->append(msg);
}

void QG_CommandWidget::handleCommand(QString cmd){
    cmd = cmd.simplified();
    bool isAction=false;
    if (!cmd.isEmpty()) {
        appendHistory(cmd);
    }

    if (m_actionHandler) {
        isAction= m_actionHandler->command(cmd);
    }

    if (!isAction && !(cmd.contains(',') || cmd.at(0)=='@')) {
       appendHistory(tr("Unknown command: %1").arg(cmd));
    }

    leCommand->setText("");
}

void QG_CommandWidget::spacePressed() {
    if (m_actionHandler)
        m_actionHandler->command({});
}
// fixme - review ouptput to command widget
//fixme - add generic help command (as TAB for empy)

void QG_CommandWidget::tabPressed() {
    if (m_actionHandler) {
        QString typed = leCommand->text();

        // check current command:
        QStringList choices = m_actionHandler->getAvailableCommands();
        if (choices.empty()) {
            choices = RS_COMMANDS->complete(typed);
        }

        QStringList reducedChoices;
        std::copy_if(choices.cbegin(), choices.cend(), std::back_inserter(reducedChoices), [&typed](const QString& cmd) {
            return typed.isEmpty() || cmd.startsWith(typed, Qt::CaseInsensitive);
        });

        // command found:
        if (reducedChoices.count()==1) {
            leCommand->setText(reducedChoices.first());
        }
        else if (!reducedChoices.isEmpty()) {
            const QString proposal = getRootCommand(reducedChoices, typed);
            appendHistory(reducedChoices.join(", "));
            const QString aliasFile = RS_Commands::getAliasFile();
            if (!aliasFile.isEmpty())
                appendHistory(tr("Command Alias File: %1").arg(aliasFile));
            leCommand -> setText(proposal);
        }
    }
}

void QG_CommandWidget::escape() {
    //leCommand->clearFocus();
    if (m_actionHandler) {
        m_actionHandler->command(QString(tr("escape", "escape, go back from action steps")));
    }
}

void QG_CommandWidget::setActionHandler(QG_ActionHandler* ah) {
    m_actionHandler = ah;
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

QString QG_CommandWidget::getRootCommand( const QStringList & cmdList, const QString & typed ) {
	//do we have to check for empty cmdList?
	if(cmdList.empty()) return QString();

	//find the shortest string in cmdList
	auto const& shortestString = * std::min_element(cmdList.begin(), cmdList.end(),
													[](QString const& a, QString const& b) -> bool
			{
				return a.size() < b.size();
			}
			);
	int const lengthShortestString = shortestString.size();

	// Now we parse the cmdList list, character of each item by character.
	int low = typed.length();
	int high = lengthShortestString + 1;

    while(high > low + 1) {
		int mid = (high + low)/2;
		bool common = true;

		QString const& proposal = shortestString.left(mid);
        for(auto const& substring: cmdList) {
            if(!substring.startsWith(proposal)) {
                common = false;
                break;
            }
        }
        if(common) {
            low = mid;
        }
        else {
            high = mid;
        }
    }

    // As we assign just before mid value to low (if strings are common), we can use it as parameter for left.
    // If not common -> low value does not changes, even if escaping from the while. This avoids weird behaviors like continuing completion when pressing tab.
	return shortestString.left(low);

}

void QG_CommandWidget::chooseCommandFile(){
    QString path = QFileDialog::getOpenFileName(this);
    if (!path.isEmpty()){
        leCommand->readCommandFile(path);
    }
}

void QG_CommandWidget::handleKeycode(QString code){
    if (m_actionHandler->keycode(code)){
        leCommand->clear();
    }
}

void QG_CommandWidget::setKeycodeMode(bool state){
    leCommand->m_keycode_mode = state;
}

void QG_CommandWidget::dockingButtonTriggered(bool /*docked*/){
    auto* cmd_dockwidget = QC_ApplicationWindow::getAppWindow()->findChild<QDockWidget*>("command_dockwidget");
    cmd_dockwidget->setFloating(!cmd_dockwidget->isFloating());
    m_docking->setText(cmd_dockwidget->isFloating() ? tr("Dock") : tr("Float"));
    setWindowTitle(cmd_dockwidget->isFloating() ? tr("Command Line") : tr("Cmd"));
}
