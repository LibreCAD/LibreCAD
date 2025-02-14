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

#ifndef COMMANDEDIT_H
#define COMMANDEDIT_H

#ifdef DEVELOPER

#include <QLineEdit>
#include <QString>
#include <QFile>

/**
 * A python command line edit with some typical console features
 * (uparrow for the history, tab, ..).
 */
class CommandEdit: public QLineEdit {
    Q_OBJECT

public:
    CommandEdit(QWidget* parent=nullptr);
    virtual ~CommandEdit() { writeHistoryFile(); }

    virtual QString text() const;
    virtual void resetPrompt() {}
    virtual void setCurrent() {}
    virtual void processInput(QString) {}

    void setPrompt(const QString &p) { m_prom = p; prompt(); }
    void prompt() { QLineEdit::setText(m_prom); }
    void doProcess(bool proc) { m_doProcess = proc; }
    void doProcessLc(bool proc) { m_doProcessLc = proc; }

    QString dockName() const { return parentWidget()->objectName(); }
    QString cmdLang() const { return dockName().contains("Python") ? "py" : "lisp"; }

    /* for input mode */
    bool m_doProcess = true;
    bool m_doProcessLc = false;
    bool keycode_mode = false;

    QStringList historyList;
    QStringList::const_iterator it = historyList.cbegin();

signals:
    void spacePressed();
    void tabPressed();
    void escape();
    void focusIn();
    void focusOut();
    void clearCommandsHistory();
    void command(QString cmd);
    void message(QString msg);
    void keycode(QString code);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    bool event(QEvent* e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    bool isForeignCommand(QString input);
    void processVariable(QString input);

    QMap<QString, QString> variables;

private:
    /*save history for next session*/
    QString m_path;
    QString m_prom;
    QFile m_histFile;
    QTextStream  m_histFileStream;

    int promptSize() const { return (int) m_prom.size(); }
    void readHistoryFile();
    void writeHistoryFile();

public slots:
    void modifiedPaste();
};

#endif // DEVELOPER

#endif // COMMANDEDIT_H

