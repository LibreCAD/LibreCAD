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

#ifndef LC_TABPROXYWIDGET_H
#define LC_TABPROXYWIDGET_H

#include <QApplication>
#include <QStackedLayout>
#include <QWidget>

class LC_TabProxyWidget : public QWidget {
public:
    LC_TabProxyWidget();

    /** Constructor
     * @param optTargetWidget if non-NULL, this will be passed to SetTargetWidget().  Defaults to NULL.
     */
    LC_TabProxyWidget(QWidget* optTargetWidget = nullptr)
        : _layout(new QStackedLayout(this)), m_targetWidget(nullptr) {
        setTargetWidget(optTargetWidget);
        setFocusPolicy(Qt::ClickFocus);
    }

    ~LC_TabProxyWidget() override { setTargetWidget(nullptr); }

    /** Set the widget that we want to be a proxy for
     * @param optTargetWidget the widget we will proxy for, or NULL to disassociate us from any target widget
     * @note the same pointer for (optTargetWidget) can (and should!) be passed to multiple TabProxyWidget objects
     */
    void setTargetWidget(QWidget* optTargetWidget);
    void showEvent(QShowEvent*) override;
    bool eventFilter(QObject* o, QEvent* e) override;
private:
    void adoptTargetWidget();
    void updateSizeConstraints();

    QStackedLayout* _layout {nullptr};
    QWidget*    m_targetWidget {nullptr};
};

#endif // LC_TABPROXYWIDGET_H
