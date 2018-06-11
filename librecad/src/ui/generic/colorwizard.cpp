/*
**********************************************************************************
**
** This file was created for LibreCAD (https://github.com/LibreCAD/LibreCAD).
**
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
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
** http://www.gnu.org/licenses/gpl-2.0.html
**
**********************************************************************************
*/

#include "colorwizard.h"
#include "ui_colorwizard.h"

#include <QAction>
#include <QColorDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QSettings>

ColorWizard::ColorWizard(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ColorWizard)
{
    ui->setupUi(this);

    connect(ui->colorwin_button, &QToolButton::clicked,
            this, &ColorWizard::invokeColorDialog);

    connect(ui->add_button, &QToolButton::clicked,
            this, &ColorWizard::addOrRemove);

    connect(ui->fav_list, &QListWidget::itemDoubleClicked,
            this, &ColorWizard::handleDoubleClick);

    ui->fav_list->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->fav_list->setDragDropMode(QAbstractItemView::InternalMove);

    auto select = new QAction(QObject::tr("Select objects"), ui->fav_list);
    connect(select, &QAction::triggered, this, &ColorWizard::requestSelection);
    ui->fav_list->addAction(select);

    auto apply = new QAction(QObject::tr("Apply to selected"), ui->fav_list);
    connect(apply, &QAction::triggered, this, &ColorWizard::requestColorChange);
    ui->fav_list->addAction(apply);

    auto remove = new QAction(QObject::tr("Remove"), ui->fav_list);
    connect(remove, &QAction::triggered, this, &ColorWizard::removeFavorite);
    ui->fav_list->addAction(remove);

    QSettings settings;
    auto favs = settings.value("Widgets/FavoriteColors").toStringList();
    foreach (auto fav, favs)
    {
        addFavorite(fav);
    }
    ui->combo->setCurrentIndex(-1);
}

ColorWizard::~ColorWizard()
{
    auto favs = getFavList();
    QSettings settings;
    if (favs.size() > 0)
        settings.setValue("Widgets/FavoriteColors", favs);
    else
        settings.remove("Widgets/FavoriteColors");
    delete ui;

}

void ColorWizard::requestColorChange()
{
    auto i = ui->fav_list->currentItem();
    if (i)
        emit requestingColorChange(QColor(i->text()));
}

void ColorWizard::requestSelection()
{
    auto i = ui->fav_list->currentItem();
    if (i)
        emit requestingSelection(QColor(i->text()));
}

void ColorWizard::invokeColorDialog()
{
    QColor current;
    current.setNamedColor(ui->combo->currentText());

    QColorDialog dlg;
    dlg.setCustomColor(0, current);

    QColor color = dlg.getColor(current, this, "Select Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid())
    {
        QPixmap pixmap(32, 32);
        pixmap.fill(color);
        ui->combo->insertItem(0, QIcon(pixmap), color.name());
        ui->combo->setCurrentIndex(0);
    }
}

void ColorWizard::addOrRemove()
{
    auto color = ui->combo->currentText();
    if (color.isEmpty()) return;

    auto list = ui->fav_list->findItems(color, Qt::MatchExactly);
    if (list.size() > 0)
        qDeleteAll(list);
    else
        addFavorite(color);

}

QStringList ColorWizard::getFavList()
{
    QStringList s_list;

    for (int i = 0; i < ui->fav_list->count(); ++i)
    {
        s_list << ui->fav_list->item(i)->text();
    }
    return s_list;
}

void ColorWizard::addFavorite(QString color)
{
    auto item = new QListWidgetItem;
    item->setText(color);
    QPixmap pixmap(32, 32);
    pixmap.fill(color);
    item->setIcon(QIcon(pixmap));
    ui->fav_list->addItem(item);
}

void ColorWizard::removeFavorite()
{
    delete ui->fav_list->currentItem();
}

void ColorWizard::handleDoubleClick(QListWidgetItem* item)
{
    auto name = item->text();
    auto color = QColor(name);
    auto i = ui->combo->findText(name);
    if (i != -1)
    {
        ui->combo->setCurrentIndex(i);
    }
    else
    {
        QPixmap pixmap(32, 32);
        pixmap.fill(color);
        ui->combo->insertItem(0, QIcon(pixmap), name);
        ui->combo->setCurrentIndex(0);
    }
    emit colorDoubleClicked(color);
}
