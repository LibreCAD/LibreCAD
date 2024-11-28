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

#ifndef QG_LSP_COMMANDEDIT_H
#define QG_LSP_COMMANDEDIT_H

#include <QLineEdit>
#include <QMap>
#include <QFile>
#include <QString>

#ifdef DEVELOPER

/**
 * A lisp command line edit with some typical console features
 * (uparrow for the history, tab, ..).
 */
class QG_Lsp_CommandEdit: public QLineEdit {
    Q_OBJECT

public:
    QG_Lsp_CommandEdit(QWidget* parent=nullptr);
    virtual ~QG_Lsp_CommandEdit() { writeHistoryFile(); }

    virtual QString text() const ;

    void runFile(const QString& path);
    void processInput(QString input);
    void setPrombt(const QString& p) { prom = p; prombt(); }
    void doProcess(bool proc) { m_doProcess = proc; }
    QString dockName() const { return parentWidget()->objectName(); }

    bool keycode_mode = false;

protected:
    bool event(QEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

    QString relative_ray;
    QMap<QString, QString> variables;

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

private:
    /**
      * @brief extractCliCal, filter cli calculator math expression
      * @param cmd, cli string
      * @return an empty string, if calculation is performed; the input string, otherwise
      */
    QString filterCliCal(const QString& cmd);
    QStringList historyList;
    QStringList::const_iterator it = historyList.cbegin();
    bool acceptCoordinates = false;
    bool calculator_mode = false;
    QString prom = QObject::tr("Command: ");
    /* save history for next session */
    QString m_path;
    QFile m_histFile;
    QTextStream  m_histFileStream;
    /* for input mode */
    bool m_doProcess = true;

    int prombtSize() const { return (int) prom.size(); }
    void prombt() { QLineEdit::setText(prom); }
    void readHistoryFile();
    void writeHistoryFile();

public slots:
    void modifiedPaste();

};

#endif // DEVELOPER

#endif // QG_LSP_COMMANDEDIT_H

