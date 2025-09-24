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

#include "lc_tabproxywidget.h"

#include "rs_debug.h"

LC_TabProxyWidget::LC_TabProxyWidget() {
}

#include <QApplication>
#include <QLabel>
#include <QMap>
#include <QSet>
#include <QStackedLayout>
#include <QTabWidget>
#include <QWidget>

static QMap<QWidget*, QSet<LC_TabProxyWidget*>> m_targetWidgetToProxies;

void LC_TabProxyWidget::setTargetWidget(QWidget* targetWidget) {
    if (targetWidget != m_targetWidget) {
        if (m_targetWidget) {
            m_targetWidget->removeEventFilter(this);

            QSet<LC_TabProxyWidget*>* proxiesForTargetWidget = m_targetWidgetToProxies.contains(m_targetWidget)
                                                                   ? &m_targetWidgetToProxies[m_targetWidget]
                                                                   : nullptr;
            if ((proxiesForTargetWidget == nullptr) || (proxiesForTargetWidget->isEmpty())) {
                // fixme - sand - well, that's ugly enought... yet if we are here - this a severe developer's bug, so let it be for now...
                printf("LC_TabProxyWidget::setTargetWidget(NULL):  can't proxies-table for target widget %p is %s!\n",
                       targetWidget, proxiesForTargetWidget ? "empty" : "missing");
                exit(10);
            }

            proxiesForTargetWidget->remove(this);
            if (proxiesForTargetWidget->isEmpty()) {
                m_targetWidgetToProxies.remove(m_targetWidget);
                delete m_targetWidget;
            }
            else if (dynamic_cast<LC_TabProxyWidget*>(m_targetWidget->parentWidget()) == this) {
                proxiesForTargetWidget->values()[0]->adoptTargetWidget();
                // hand him off to another proxy to for safekeeping
            }
        }

        m_targetWidget = targetWidget;

        if (m_targetWidget != nullptr) {
            if (m_targetWidgetToProxies.contains(m_targetWidget) == false) {
                m_targetWidgetToProxies[m_targetWidget] = QSet<LC_TabProxyWidget*>();
            }
            m_targetWidgetToProxies[m_targetWidget].insert(this);

            if ((isHidden() == false) || (m_targetWidget->parentWidget() == nullptr) || (dynamic_cast<LC_TabProxyWidget*>(
                m_targetWidget->parentWidget()) == nullptr)) {
                adoptTargetWidget();
            }

            updateSizeConstraints();
            m_targetWidget->installEventFilter(this);
        }
    }
}

bool LC_TabProxyWidget::eventFilter(QObject* o, QEvent* e) {
    if (o == m_targetWidget) {
        if (e->type() == QEvent::Resize) {
            updateSizeConstraints();
        }
        /*if (e->type() == QEvent::MouseMove) {
            // LC_ERR << "Mouse MOve";
        }
        if (e->type() == QEvent::NativeGesture) {
            // LC_ERR << "Native Guesture";
        }
        if (e->type() == QEvent::Wheel) {
            // LC_ERR << "Wheel";
        }
        else {
            // LC_ERR << e->type();
        }*/
    }
    return QWidget::eventFilter(o, e);
}

void LC_TabProxyWidget::updateSizeConstraints() {
    if (m_targetWidget != nullptr) {
        setMinimumSize(m_targetWidget->minimumSize());
        setMaximumSize(m_targetWidget->maximumSize());
        setSizePolicy(m_targetWidget->sizePolicy());
    }
    else {
        setMinimumSize(QSize(0, 0));
        setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
    }
}

void LC_TabProxyWidget::showEvent(QShowEvent* e) {
    adoptTargetWidget();
    QWidget::showEvent(e);
    if (m_targetWidget != nullptr) {
        m_targetWidget->show();
    }
}

void LC_TabProxyWidget::adoptTargetWidget() {
    if ((m_targetWidget) && (m_targetWidget->parentWidget() != this)) {
        QLayout* layout = m_targetWidget->layout();
        if (layout != nullptr) {
            layout->removeWidget(m_targetWidget);
        }

        m_targetWidget->setParent(this);
        _layout->addWidget(m_targetWidget);
    }
}

static void setWidgetBackgroundColor(QWidget* w, const QColor bc) {
    QPalette p = w->palette();
    p.setColor(QPalette::Window, bc);
    w->setAutoFillBackground(true);
    w->setPalette(p);
}
