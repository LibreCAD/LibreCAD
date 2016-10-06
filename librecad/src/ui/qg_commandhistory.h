/*
**********************************************************************************
**
** This file is part of the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2015 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/

// -- https://github.com/LibreCAD/LibreCAD --

#ifndef QG_COMMANDHISTORY_H
#define QG_COMMANDHISTORY_H
#include <QTextEdit>

/**
 * @brief The QG_CommandHistory class holds commands and messages.
 * It's a read only textedit widget.
 * \author ravas
 */
class QG_CommandHistory : public QTextEdit
{
    Q_OBJECT

public:
	QG_CommandHistory()=delete;
    explicit QG_CommandHistory(QWidget* parent);

private slots:

	void mouseReleaseEvent(QMouseEvent* event);
	void slotTextChanged();

private:
	/*menu item for Copy*/
	QAction* m_pCopy;
	/*menu item for Select All*/
	QAction* m_pSelectAll;
};

#endif // QG_COMMANDHISTORY_H
