/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_DLGMENUASSIGNER_H
#define LC_DLGMENUASSIGNER_H

#include <QElapsedTimer>

#include "lc_dialog.h"
#include "lc_menuactivator.h"

namespace Ui {
    class LC_DlgMenuAssigner;
}

class LC_DlgMenuAssigner : public LC_Dialog {
    Q_OBJECT
public:
    void initEntityContextCombobox();
    explicit LC_DlgMenuAssigner(QWidget *parent, LC_MenuActivator* activator, QList<LC_MenuActivator*>* activators);
    ~LC_DlgMenuAssigner() override;
public slots:
    void onKeyModifierToggled(bool checked);
    void onContextEntityCurrentIndexChanged(int currentIndex);
    void onEventTypeToggled(bool checked);
    void onBtnTypeToggled(bool checked);
private:
    Ui::LC_DlgMenuAssigner *ui;
    LC_MenuActivator* m_activator;
    QList<LC_MenuActivator*>* m_activators;
    QElapsedTimer m_doubleClickTimer;

    bool eventFilter(QObject *object, QEvent *event) override;

    void updateShortcutView();
    QString findMenuForActivator();
    bool validateShortcut();
};

#endif // LC_DLGMENUASSIGNER_H
