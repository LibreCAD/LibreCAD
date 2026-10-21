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

#include <functional>

#include <QColor>
#include <QRegion>
#include <QScrollBar>
#include <QString>
#include <QWheelEvent>

#include "lc_scrollmodel.h"

class QPalette;
class QStyleOptionSlider;

/**
 * A small wrapper for the Qt scrollbar used by drawing views.
 *
 * A wheel event over the bar is always consumed: QScrollBar ignores a wheel it cannot
 * apply (at a range end), and Qt would then pass it on to the parent view, which zooms.
 *
 * The bar can also show where the drawing lies along it (issue #2945): a thin stripe
 * along the groove's outer edge from the drawing's first to its last tick, mapped
 * through the style's own thumb geometry (LC_ScrollModel::bandPixels()), so that the
 * thumb lies inside the band whenever the view lies inside the drawing. It is painted
 * whenever it is switched on and the drawing has content, after the style's own
 * painting, with the thumb cut out, so it shows the parts of the drawing's range
 * outside the view, on either side of the thumb; it never changes hit-testing. A
 * tooltip provider can add the distance the clamped thumb can no longer show.
 */
class QG_ScrollBar: public QScrollBar {
    Q_OBJECT
public:
    /** Default of the Appearance/ScrollBarContentBand setting. */
    static constexpr bool kContentBandDefault = LC_ScrollModel::kContentBandDefault;

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

    /**
     * The drawing's extents along this bar, in the bar's ticks (the same ticks as its
     * value, see LC_ScrollModel::State::tickFor()). Repaints only on a change.
     */
    void setContentBand(bool hasContent, double startTick, double endTick);
    void setContentBandEnabled(bool enabled);
    bool isContentBandEnabled() const {
        return m_bandEnabled;
    }
    /**
     * The stripe in this bar's logical pixels, before the thumb is cut out of it; a null
     * rect when there is no band (disabled, empty drawing, bar too small).
     */
    QRect contentBandRect() const;
    /**
     * Whether any of the stripe is painted now: there is a band, and some of it lies
     * outside the thumb cut-out (paintedBandRegion()). Zoomed in, that is the drawing's
     * range on either side of the thumb; at zoom extents the band lies under the thumb
     * and nothing is painted.
     */
    bool isContentBandPainted() const;
    /** the stripe colour for this bar's palette (cached, see contentBandColorFor()) */
    QColor contentBandColor() const;
    /**
     * The stripe colour for \p palette: the first of Accent (Qt >= 6.6), Highlight and
     * WindowText blended toward the accent that reaches kMinBandContrast against
     * Window (and against Window shaded slightly, as styles draw the groove); the
     * candidates are lightened first on a dark palette. Always opaque, 8 bits per channel.
     */
    static QColor contentBandColorFor(const QPalette& palette);
    /** WCAG 2.1 minimum contrast for non-text graphics (see RS_Color::contrastRatio()) */
    static constexpr double kMinBandContrast = 3.0;

    /**
     * Supplies the bar's tooltip text on demand (an empty string shows none). The
     * drawing views set one that gives the drawing's and the view's ranges and their
     * distance; bars without a provider show the ordinary QWidget tooltip.
     */
    void setToolTipProvider(std::function<QString()> provider);
    /** the provider's current text, or an empty string without a provider */
    QString providedToolTip() const;

protected:
    void wheelEvent(QWheelEvent* e) override {
        QScrollBar::wheelEvent(e);
        e->accept();
    }

    void resizeEvent(QResizeEvent* event) override {
        m_sizeHintCache = QScrollBar::sizeHint();
        QScrollBar::resizeEvent(event);
    }

    bool event(QEvent* event) override;
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

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

    QRectF contentBandStripe(const QStyleOptionSlider& option) const;
    /**
     * What paintEvent() paints of the band now: the stripe (returned in \p stripe) minus
     * the thumb, grown one pixel along the bar on each side. Empty when nothing is
     * painted; the setting and the content are checked before any style geometry.
     */
    QRegion paintedBandRegion(QRectF* stripe = nullptr) const;

    QSize m_sizeHintCache{};
    bool m_bandEnabled = kContentBandDefault;
    bool m_bandHasContent = false;
    double m_bandStart = 0.0;
    double m_bandEnd = 0.0;
    //! contentBandColor() cache, cleared on a palette or style change
    mutable QColor m_bandColor;
    std::function<QString()> m_toolTipProvider;
};

#endif
