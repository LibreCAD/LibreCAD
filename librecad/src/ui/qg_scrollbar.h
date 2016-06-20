/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
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

#ifndef QG_SCROLLBAR_H
#define QG_SCROLLBAR_H

#include <QScrollBar>
#include <QWheelEvent>

/**
 * A small wrapper for the Qt scrollbar. This class offers a slot
 * for scroll events.
 */
class QG_ScrollBar: public QScrollBar {
    Q_OBJECT

public:
    QG_ScrollBar(QWidget* parent=0)
            : QScrollBar(parent) {
        sizeHintCache = QScrollBar::sizeHint();
    }
    QG_ScrollBar(Qt::Orientation orientation,
                 QWidget* parent=0)
            : QScrollBar(orientation, parent) {
        sizeHintCache = QScrollBar::sizeHint();
    }

    // This sizeHint caches the height value. Out of profiling (see #727),
    // it appears that sizeHint calls all the way down to the underlying
    // windowing system to check the size.
    QSize sizeHint() const override final {
        return sizeHintCache;
    }

public slots:
    void slotWheelEvent(QWheelEvent* e) {
        wheelEvent(e);
    }

    void resizeEvent(QResizeEvent* event) override {
        sizeHintCache = QScrollBar::sizeHint();
        QScrollBar::resizeEvent(event);
    }

private:
    QSize sizeHintCache;

};

#endif

