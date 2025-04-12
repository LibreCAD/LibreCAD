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

#ifndef LC_UCSSTATEWIDGET_H
#define LC_UCSSTATEWIDGET_H

#include <QWidget>

namespace Ui {
    class LC_UCSStateWidget;
}

class LC_UCSStateWidget : public QWidget{
    Q_OBJECT
public:
    explicit LC_UCSStateWidget(QWidget *parent,const char* name);
    ~LC_UCSStateWidget() override;
    void update(QIcon icon, QString ucsName, QString ucsInfo);
public slots:
    void onIconsRefreshed();
private:
    Ui::LC_UCSStateWidget *ui;
    int m_iconSize = 24;
    QIcon m_savedIcon;
};

#endif // LC_UCSSTATEWIDGET_H
