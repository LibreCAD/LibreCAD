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
#include <QHeaderView>
#include <QScreen>
#include <QShowEvent>
#include <QTreeWidget>

#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "rs_commands.h"
#include "rs_settings.h"
/*
 *  Constructs a QG_CommandWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CommandWidget::QG_CommandWidget(QG_ActionHandler* actionHandler, QWidget* parent, const char* name, const Qt::WindowFlags fl)
    : LC_GraphicViewAwareWidget(parent, name, fl)
      , m_actionHandler(actionHandler) {
    setObjectName(name);
    setupUi(this);
    historySplitter->setCollapsible(0, true);
    historySplitter->setCollapsible(1, false);
    historySplitter->setStretchFactor(0, 1);
    historySplitter->setStretchFactor(1, 0);
    historySplitter->setSizes({72, commandRow->sizeHint().height()});
    const QByteArray historyState = LC_GET_ONE_STR("Widgets", "CommandHistorySplitter", "").toLatin1();
    if (!historyState.isEmpty()) historySplitter->restoreState(QByteArray::fromBase64(historyState));
    setCommand({});
    m_suggestions = new QTreeWidget(this);
    m_suggestions->setObjectName("command_suggestions");
    // Keyboard focus belongs to the command editor, including mouse completion.
    m_suggestions->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    m_suggestions->setAttribute(Qt::WA_ShowWithoutActivating);
    m_suggestions->setFocusPolicy(Qt::NoFocus);
    m_suggestions->viewport()->setFocusPolicy(Qt::NoFocus);
    m_suggestions->setRootIsDecorated(false);
    m_suggestions->setUniformRowHeights(true);
    m_suggestions->setHeaderLabels({tr("Command / alias"), tr("Description")});
    m_suggestions->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_suggestions->setSelectionMode(QAbstractItemView::SingleSelection);
    leCommand->installEventFilter(this);
    connect(leCommand, &QLineEdit::textEdited, this, &QG_CommandWidget::updateSuggestions);
    connect(leCommand, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text != m_suggestionPrefix) dismissSuggestions();
    });
    connect(leCommand, &QG_CommandEdit::focusOut, this, &QG_CommandWidget::dismissSuggestions);
    connect(m_suggestions, &QTreeWidget::itemClicked, this, [this] {acceptSuggestion(false);});
    setActionHandler(actionHandler);
    connect(leCommand, &QG_CommandEdit::command, this, &QG_CommandWidget::handleCommand);
    connect(leCommand, &QG_CommandEdit::escape, this, &QG_CommandWidget::escape);
    connect(leCommand, &QG_CommandEdit::focusOut, this, &QG_CommandWidget::setNormalMode);
    connect(leCommand, &QG_CommandEdit::focusIn, this, &QG_CommandWidget::setCommandMode);
    connect(leCommand, &QG_CommandEdit::spacePressed, this, &QG_CommandWidget::spacePressed);
    connect(leCommand, &QG_CommandEdit::tabPressed, this, &QG_CommandWidget::tabPressed);
    connect(leCommand, &QG_CommandEdit::clearCommandsHistory, teHistory, &QG_CommandHistory::clear);
    connect(leCommand, &QG_CommandEdit::message, this, &QG_CommandWidget::appendHistory);
    connect(leCommand, &QG_CommandEdit::keycode, this, &QG_CommandWidget::handleKeycode);

    const auto a1 = new QAction(QObject::tr("Keycode mode"), this);
    a1->setObjectName("keycode_action");
    a1->setCheckable(true);
    connect(a1, &QAction::toggled, this, &QG_CommandWidget::setKeycodeMode);
    options_button->addAction(a1);

    if (LC_GET_ONE_BOOL("Widgets", "KeycodeMode", false)) {
        leCommand->setKeyCodeMode(true);
        a1->setChecked(true);
    }

    const auto a2 = new QAction(QObject::tr("Load command file"), this);
    connect(a2, &QAction::triggered, this, &QG_CommandWidget::chooseCommandFile);
    options_button->addAction(a2);

    const auto a3 = new QAction(QObject::tr("Paste multiple commands"), this);
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
QG_CommandWidget::~QG_CommandWidget() {
    LC_SET_ONE("Widgets", "CommandHistorySplitter", QString::fromLatin1(historySplitter->saveState().toBase64()));
    const auto action = findChild<QAction*>("keycode_action");
    LC_SET_ONE("Widgets", "KeycodeMode", action->isChecked());
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CommandWidget::languageChange() {
    dismissSuggestions();
    retranslateUi(this);
    m_suggestions->setHeaderLabels({tr("Command / alias"), tr("Description")});
}

bool QG_CommandWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_suggestionWindow && obj != leCommand) {
        if (event->type() == QEvent::Move || event->type() == QEvent::Resize) positionSuggestions();
        if (event->type() == QEvent::Hide || event->type() == QEvent::WindowDeactivate) dismissSuggestions();
        return false;
    }
    if (obj == leCommand) {
        if (event->type() == QEvent::KeyPress && m_suggestions->isVisible()) {
            const auto* key = static_cast<QKeyEvent*>(event);
            if (key->modifiers() == Qt::NoModifier || key->modifiers() == Qt::KeypadModifier) {
                if (!canSuggest() || !leCommand->allowsCompletion()) {
                    dismissSuggestions();
                    return false;
                }
                const int row = m_suggestions->indexOfTopLevelItem(m_suggestions->currentItem());
                switch (key->key()) {
                    case Qt::Key_Up:
                    case Qt::Key_Down: {
                        const int next = qBound(0, row + (key->key() == Qt::Key_Up ? -1 : 1), m_suggestions->topLevelItemCount() - 1);
                        m_suggestions->setCurrentItem(m_suggestions->topLevelItem(next));
                        m_suggestions->scrollToItem(m_suggestions->currentItem());
                        return true;
                    }
                    case Qt::Key_Tab: acceptSuggestion(false); return true;
                    case Qt::Key_Return:
                    case Qt::Key_Enter: acceptSuggestion(true); return true;
                    case Qt::Key_Space:
                        if (LC_GET_ONE_BOOL("Keyboard", "EvaluateCommandOnSpace", true)) {
                            acceptSuggestion(true);
                            return true;
                        }
                        break;
                    case Qt::Key_Escape: dismissSuggestions(); return true;
                    default: break;
                }
            }
        }
        return false;
    }
    if (event != nullptr && event->type() == QEvent::KeyPress) {
        const auto e = static_cast<QKeyEvent*>(event);

        const int key{e->key()};
        switch (key) {
            case Qt::Key_Return:
            case Qt::Key_Enter: {
                if (leCommand->text().size() == 0) {
                    return false;
                }
                break;
            }
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
        const Qt::KeyboardModifiers modifiers{e->modifiers()};
        if (!(Qt::GroupSwitchModifier == modifiers && Qt::Key_At == key) // let '@' key pass for relative coords
            && modifiers != Qt::KeypadModifier && modifiers & (Qt::KeyboardModifierMask ^ Qt::ShiftModifier) & (Qt::KeyboardModifierMask ^ Qt::ControlModifier)) {
            return false;
        }
        const bool isGraphicViewEvent = (key != Qt::Key_Shift) &&
            (key != Qt::Key_Control) &&
            (key != Qt::Key_Up) &&
            (key != Qt::Key_Left) &&
            (key != Qt::Key_Right) &&
            (key != Qt::Key_Down) &&
            (key != Qt::Key_Plus) &&
            (key != Qt::Key_Minus) &&
            (key != Qt::Key_Tab);
        // prevent focus for graphic-view specific keys
        if (isGraphicViewEvent) {
            focusWidget();
            QApplication::postEvent(leCommand, e->clone());
            e->accept();
            return true;
        }
        const auto eventClone = e->clone();
        QApplication::postEvent(QC_ApplicationWindow::getAppWindow().get(), eventClone);
        if (!eventClone->isAccepted()) {
            QApplication::postEvent(leCommand, e->clone());
        }
        e->accept();
        return true;
    }

    return false;
}

void QG_CommandWidget::focusWidget() {
    if (!isActiveWindow()) {
        activateWindow();
    }

    const auto newEvent = new QFocusEvent(QEvent::FocusIn);
    QApplication::postEvent(leCommand, newEvent);
    leCommand->setFocus();
}

void QG_CommandWidget::setCommand(const QString& cmd) const {
    // Mouse enter/focus can refresh an idle action's prompt while typing. Only
    // an actual argument/action context invalidates command-name suggestions.
    if (!canSuggest()) dismissSuggestions();
    if (cmd.isEmpty() || canSuggest()) {
        lCommand->setPrompt(tr("Command:"));
    }
    else {
        if (!cmd.endsWith(":")) {
            lCommand->setPrompt(cmd + ":");
        }
        else {
            lCommand->setPrompt(cmd);
        }
    }
    positionSuggestions();
}

void QG_CommandWidget::setInput(const QString& cmd) const {
    dismissSuggestions();
    leCommand->setText(cmd);
    leCommand->setFocus();
}

void QG_CommandWidget::appendHistory(const QString& msg) const {
    teHistory->append(msg);
}

void QG_CommandWidget::handleCommand(QString cmd) const {
    dismissSuggestions();
    cmd = cmd.simplified();
    bool isAction = false;
    if (!cmd.isEmpty()) {
        appendHistory(cmd);
    }

    if (m_actionHandler != nullptr) {
        isAction = m_actionHandler->command(cmd);
    }

    if (!isAction && !cmd.isEmpty() && !(cmd.contains(',') || cmd.startsWith('@'))) {
        appendHistory(tr("Unknown command: %1").arg(cmd));
    }

    leCommand->setText("");
}

void QG_CommandWidget::spacePressed() const {
    if (m_actionHandler != nullptr) {
        m_actionHandler->command({});
    }
}

// fixme - review ouptput to command widget
//fixme - add generic help command (as TAB for empy)

void QG_CommandWidget::tabPressed() const {
    if (m_actionHandler != nullptr) {
        QString typed = leCommand->text();
        if (!leCommand->allowsCompletion()) return;
        if (canSuggest()) {
            const_cast<QG_CommandWidget*>(this)->updateSuggestions();
            if (m_suggestions->topLevelItemCount() == 1) acceptSuggestion(false);
            return;
        }

        // check current command:
        QStringList choices = m_actionHandler->getAvailableCommands();

        QStringList reducedChoices;
        std::copy_if(choices.cbegin(), choices.cend(), std::back_inserter(reducedChoices),
                     [&typed](const QString& cmd) {
                         return typed.isEmpty() || cmd.startsWith(typed, Qt::CaseInsensitive);
                     });

        // command found:
        if (reducedChoices.count() == 1) {
            leCommand->setText(reducedChoices.first());
        }
        else if (!reducedChoices.isEmpty()) {
            const QString proposal = getRootCommand(reducedChoices, typed);
            appendHistory(reducedChoices.join(", "));
            const QString aliasFile = RS_Commands::getAliasFile();
            if (!aliasFile.isEmpty()) {
                appendHistory(tr("Command Alias File: %1").arg(aliasFile));
            }
            leCommand->setText(proposal);
        }
    }
}

void QG_CommandWidget::escape() const {
    //leCommand->clearFocus();
    if (m_actionHandler != nullptr) {
        m_actionHandler->command(RS_Commands::localizedCommand("escape", "escape, go back from action steps",
                                                                "QG_ActionHandler"));
    }
}

void QG_CommandWidget::setActionHandler(QG_ActionHandler* ah) {
    disconnect(m_completionConnection);
    m_actionHandler = ah;
    if (ah != nullptr) m_completionConnection = connect(ah, &QG_ActionHandler::commandContextChanged,
                                                        this, &QG_CommandWidget::dismissSuggestions);
}

void QG_CommandWidget::setCommandMode() const {
    QPalette promptPalette = palette();
    promptPalette.setColor(lCommand->foregroundRole(), promptPalette.color(QPalette::Link));
    lCommand->setPalette(promptPalette);
}

void QG_CommandWidget::setNormalMode() const {
    lCommand->setPalette(palette());
}

void QG_CommandWidget::resizeEvent(QResizeEvent* event) {
    LC_GraphicViewAwareWidget::resizeEvent(event);
    lCommand->setMaximumWidth(qMax(60, width() * 2 / 5));
    positionSuggestions();
}

void QG_CommandWidget::showEvent(QShowEvent* event) {
    LC_GraphicViewAwareWidget::showEvent(event);
    if (m_suggestionWindow) m_suggestionWindow->removeEventFilter(this);
    m_suggestionWindow = window();
    if (m_suggestionWindow != this) m_suggestionWindow->installEventFilter(this);
}

bool QG_CommandWidget::canSuggest() const {
    return m_actionHandler != nullptr && m_actionHandler->isCommandEntry();
}

void QG_CommandWidget::dismissSuggestions() const {
    if (m_suggestions != nullptr) m_suggestions->hide();
}

void QG_CommandWidget::updateSuggestions() {
    dismissSuggestions();
    if (!canSuggest() || !leCommand->hasFocus() || !leCommand->allowsCompletion()) return;
    m_suggestionPrefix = leCommand->text();
    m_suggestions->clear();
    for (const auto& candidate : RS_COMMANDS->suggestions(m_suggestionPrefix)) {
        // Alias files can contain reserved parser syntax (variables/sequences).
        // Completion must insert a single command-name token, not execute that syntax.
        if (!QG_CommandEdit::isCommandToken(candidate.text)) continue;
        const QString name = candidate.text == candidate.canonical ? candidate.text
                           : candidate.text + "  →  " + candidate.canonical;
        auto* item = new QTreeWidgetItem(m_suggestions, {name, candidate.description});
        item->setData(0, Qt::UserRole, candidate.text);
        item->setToolTip(0, candidate.alias ? tr("Alias for %1").arg(candidate.canonical) : candidate.canonical);
        item->setToolTip(1, candidate.description);
    }
    if (m_suggestions->topLevelItemCount() == 0) return;
    m_suggestions->setCurrentItem(m_suggestions->topLevelItem(0));
    m_suggestions->resizeColumnToContents(0);
    positionSuggestions();
    m_suggestions->show();
    m_suggestions->raise();
}

void QG_CommandWidget::positionSuggestions() const {
    if (m_suggestions == nullptr || m_suggestions->topLevelItemCount() == 0) return;
    const QPoint anchor = leCommand->mapToGlobal(QPoint(0, 0));
    const auto* screen = QGuiApplication::screenAt(anchor);
    if (screen == nullptr) screen = leCommand->screen();
    const QRect available = screen->availableGeometry();
    const int width = qMin(available.width(), qMax(420, qMin(640, leCommand->width())));
    const int rowHeight = qMax(1, m_suggestions->sizeHintForRow(0));
    const int preferredHeight = qMin(8, m_suggestions->topLevelItemCount()) * rowHeight
                              + m_suggestions->header()->sizeHint().height() + 2 * m_suggestions->frameWidth();
    const int above = anchor.y() - available.top();
    const int below = available.bottom() - anchor.y() - leCommand->height();
    const bool upwards = above >= preferredHeight || above >= below;
    const int height = qMax(1, qMin(preferredHeight, upwards ? above : below));
    const int x = qBound(available.left(), anchor.x(), available.right() - width + 1);
    const int y = upwards ? anchor.y() - height : anchor.y() + leCommand->height();
    m_suggestions->setGeometry(x, y, width, height);
}

void QG_CommandWidget::acceptSuggestion(bool execute) const {
    const auto* item = m_suggestions->currentItem();
    if (item == nullptr || !canSuggest() || !leCommand->allowsCompletion()) {
        dismissSuggestions();
        return;
    }
    const QString command = item->data(0, Qt::UserRole).toString();
    dismissSuggestions();
    leCommand->setText(command);
    leCommand->setFocus();
    if (execute) leCommand->acceptCommand(command);
}

QString QG_CommandWidget::getRootCommand(const QStringList& cmdList, const QString& typed) {
    //do we have to check for empty cmdList?
    if (cmdList.empty()) {
        return QString();
    }

    //find the shortest string in cmdList
    const auto& shortestString = *std::min_element(cmdList.begin(), cmdList.end(),
                                                   [](const QString& a, const QString& b) -> bool {
                                                       return a.size() < b.size();
                                                   }
        );
    const int lengthShortestString = shortestString.size();

    // Now we parse the cmdList list, character of each item by character.
    int low = typed.length();
    int high = lengthShortestString + 1;

    while (high > low + 1) {
        const int mid = (high + low) / 2;
        bool common = true;

        const QString& proposal = shortestString.left(mid);
        for (const auto& substring : cmdList) {
            if (!substring.startsWith(proposal)) {
                common = false;
                break;
            }
        }
        if (common) {
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

void QG_CommandWidget::chooseCommandFile() {
    const QString path = QFileDialog::getOpenFileName(this);
    if (!path.isEmpty()) {
        leCommand->readCommandFile(path);
    }
}

void QG_CommandWidget::handleKeycode(const QString& code) const {
    if (m_actionHandler->keycode(code)) {
        leCommand->clear();
    }
}

void QG_CommandWidget::setKeycodeMode(const bool state) const {
    leCommand->setKeyCodeMode(state);
}

void QG_CommandWidget::dockingButtonTriggered(bool /*docked*/) {
    auto* cmdDockWidget = QC_ApplicationWindow::getAppWindow()->findChild<QDockWidget*>("command_dockwidget");
    cmdDockWidget->setFloating(!cmdDockWidget->isFloating());
    m_docking->setText(cmdDockWidget->isFloating() ? tr("Dock") : tr("Float"));
    setWindowTitle(cmdDockWidget->isFloating() ? tr("Command Line") : tr("Cmd"));
}
