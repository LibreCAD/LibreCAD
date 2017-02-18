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

#include "textfileviewer.h"
#include "ui_textfileviewer.h"

#include <QFile>
#include <QTextStream>
#include <QPlainTextEdit>
#include <QListWidget>

TextFileViewer::TextFileViewer(QWidget* parent) :
    QFrame(parent),
    ui(new Ui::TextFileViewer)
{
    ui->setupUi(this);
    ui->text_edit->setReadOnly(true);

    connect(ui->list, &QListWidget::itemClicked, this, &TextFileViewer::loadFile);
}

TextFileViewer::~TextFileViewer()
{
    delete ui;
}

bool TextFileViewer::addFile(QString name, QString path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream txt_stream(&file);
    auto txt = txt_stream.readAll();

    auto item = new QListWidgetItem(name, ui->list);
    item->setWhatsThis(txt);
    return true;
}

void TextFileViewer::loadFile(QListWidgetItem* item)
{
    ui->text_edit->setPlainText(item->whatsThis());
}

void TextFileViewer::setFile(QString name)
{
    auto item = ui->list->findItems(name, Qt::MatchExactly)[0];
    ui->list->setCurrentItem(item);
    loadFile(item);
}
