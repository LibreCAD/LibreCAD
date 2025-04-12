/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 librecad.org
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

#include "lc_ucsstatewidget.h"
#include "ui_lc_ucsstatewidget.h"

LC_UCSStateWidget::LC_UCSStateWidget(QWidget* parent, const char* name)
    : QWidget(parent)
    , ui(new Ui::LC_UCSStateWidget){
    setObjectName(name);
    ui->setupUi(this);
}

LC_UCSStateWidget::~LC_UCSStateWidget(){
    delete ui;
}

void LC_UCSStateWidget::update(QIcon icon, QString ucsName, QString ucsInfo) {
    ui->lblName->setText(ucsName);
    ui->lblInfo->setText(ucsInfo);
    ui->lblType->setPixmap(icon.pixmap(m_iconSize));
    m_savedIcon = icon;
}

void LC_UCSStateWidget::onIconsRefreshed(){
    ui->lblType->setPixmap(m_savedIcon.pixmap(m_iconSize));
}
