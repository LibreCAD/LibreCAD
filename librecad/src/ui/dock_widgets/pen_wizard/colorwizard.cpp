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

#include <QColorDialog>
#include <QSettings>

#include "ui_colorwizard.h"

namespace {
    constexpr int PIXMAP_SIZE = 32;
}

ColorWizard::ColorWizard(QWidget* parent) : QFrame(parent), ui(std::make_unique<Ui::ColorWizard>()) {
    ui->setupUi(this);

    connect(ui->colorwin_button, &QToolButton::clicked, this, &ColorWizard::invokeColorDialog);

    connect(ui->add_button, &QToolButton::clicked, this, &ColorWizard::addOrRemove);

    const auto favList = ui->fav_list;
    connect(favList, &QListWidget::itemDoubleClicked, this, &ColorWizard::handleDoubleClick);

    favList->setContextMenuPolicy(Qt::ActionsContextMenu);
    favList->setDragDropMode(QAbstractItemView::InternalMove);

    const auto select = new QAction(QObject::tr("Select objects"), favList);
    connect(select, &QAction::triggered, this, &ColorWizard::requestSelection);
    favList->addAction(select);

    const auto apply = new QAction(QObject::tr("Apply to selected"), favList);
    connect(apply, &QAction::triggered, this, &ColorWizard::requestColorChange);
    favList->addAction(apply);

    const auto remove = new QAction(QObject::tr("Remove"), favList);
    connect(remove, &QAction::triggered, this, &ColorWizard::removeFavorite);
    favList->addAction(remove);

    const QSettings settings;
    auto favs = settings.value("Widgets/FavoriteColors").toStringList();
    foreach(auto fav, favs) {
        addFavorite(fav);
    }
    ui->combo->setCurrentIndex(-1);
}

ColorWizard::~ColorWizard() {
    const auto favs = getFavList();
    QSettings settings;
    if (favs.empty()) {
        settings.remove("Widgets/FavoriteColors");
    }
    else {
        settings.setValue("Widgets/FavoriteColors", favs);
    }
}

void ColorWizard::requestColorChange() {
    const auto i = ui->fav_list->currentItem();
    if (i != nullptr) {
        emit requestingColorChange(QColor(i->text()));
    }
}

void ColorWizard::requestSelection() {
    const auto i = ui->fav_list->currentItem();
    if (i != nullptr) {
        emit requestingSelection(QColor(i->text()));
    }
}

void ColorWizard::invokeColorDialog() {
    QColor current;
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    current = QColor::fromString(ui->combo->currentText());
#else
    current.setNamedColor(ui->combo->currentText());
#endif

    QColorDialog dlg;
    dlg.setCustomColor(0, current);

    const QColor color = dlg.getColor(current, this, "Select Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
        QPixmap pixmap(32, 32);
        pixmap.fill(color);
        ui->combo->insertItem(0, QIcon(pixmap), color.name());
        ui->combo->setCurrentIndex(0);
    }
}

void ColorWizard::addOrRemove() const {
    const auto color = ui->combo->currentText();
    if (color.isEmpty()) {
        return;
    }

    const auto list = ui->fav_list->findItems(color, Qt::MatchExactly);
    if (list.size() > 0) {
        qDeleteAll(list);
    }
    else {
        addFavorite(color);
    }
}

QStringList ColorWizard::getFavList() const {
    QStringList s_list;

    for (int i = 0; i < ui->fav_list->count(); ++i) {
        s_list << ui->fav_list->item(i)->text();
    }
    return s_list;
}

void ColorWizard::addFavorite(const QString& color) const {
    const auto item = new QListWidgetItem;
    item->setText(color);
    QPixmap pixmap(PIXMAP_SIZE, PIXMAP_SIZE);
    pixmap.fill(color);
    item->setIcon(QIcon(pixmap));
    ui->fav_list->addItem(item);
}

void ColorWizard::removeFavorite() const {
    delete ui->fav_list->currentItem();
}

void ColorWizard::handleDoubleClick(const QListWidgetItem* item) {
    const auto name = item->text();
    const auto color = QColor(name);
    const auto i = ui->combo->findText(name);
    if (i != -1) {
        ui->combo->setCurrentIndex(i);
    }
    else {
        QPixmap pixmap(PIXMAP_SIZE, PIXMAP_SIZE);
        pixmap.fill(color);
        ui->combo->insertItem(0, QIcon(pixmap), name);
        ui->combo->setCurrentIndex(0);
    }
    emit colorDoubleClicked(color);
}
