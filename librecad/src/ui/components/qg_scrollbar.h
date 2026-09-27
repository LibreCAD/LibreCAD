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
 * A small wrapper for the Qt scrollbar used by drawing views.
 *
 * A wheel event over the bar is always consumed: QScrollBar ignores a wheel it cannot
 * apply (at a range end), and Qt would then pass it on to the parent view, which zooms.
 */
class QG_ScrollBar: public QScrollBar {
    Q_OBJECT
public:
    explicit QG_ScrollBar(QWidget* parent=nullptr)
            : QScrollBar(parent) {
        init();
    }
   explicit  QG_ScrollBar(const Qt::Orientation orientation,
                 QWidget* parent=nullptr)
            : QScrollBar(orientation, parent) {
        init();
    }

    // This sizeHint caches the height value. Out of profiling (see #727),
    // it appears that sizeHint calls all the way down to the underlying
    // windowing system to check the size.
    QSize sizeHint() const final {
        return m_sizeHintCache;
    }

protected:
    void wheelEvent(QWheelEvent* e) override {
        QScrollBar::wheelEvent(e);
        e->accept();
    }

    void resizeEvent(QResizeEvent* event) override {
        m_sizeHintCache = QScrollBar::sizeHint();
        QScrollBar::resizeEvent(event);
    }

private:
    void init() {
        m_sizeHintCache = QScrollBar::sizeHint();
        // mouse events over the bar never reach the drawing view
        setAttribute(Qt::WA_NoMousePropagation);
        // drawing coordinates run left to right in every locale
        setLayoutDirection(Qt::LeftToRight);
        // QG_GraphicView::addScrollbars() relies on sliderReleased firing after the final
        // SliderMove (tracking on is Qt's default, but enforce it here so the invariant
        // holds in every build, not just where Q_ASSERT is compiled in).
        setTracking(true);
    }

    QSize m_sizeHintCache{};
};

#endif
