/*
**********************************************************************************
**
** LibreCAD is a cross-platform 2D CAD program (github.com/LibreCAD/LibreCAD).
**
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License Version 2
** as published by the Free Software Foundation.
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
** http://www.gnu.org/licenses/gpl-2.0.html
**
**********************************************************************************
*/

#ifndef TEXTFILEVIEWER_H
#define TEXTFILEVIEWER_H

#include <QFrame>
#include <QString>

class QListWidgetItem;

namespace Ui {
class TextFileViewer;
}

class TextFileViewer : public QFrame
{
    Q_OBJECT

public:
    explicit TextFileViewer(QWidget* parent = 0);
    ~TextFileViewer();

    bool addFile(QString name, QString path);
    void setFile(QString name);

public slots:
    void loadFile(QListWidgetItem* item);

private:
    Ui::TextFileViewer* ui;
};

#endif // TEXTFILEVIEWER_H
