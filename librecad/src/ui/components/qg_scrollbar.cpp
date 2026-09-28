/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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
**********************************************************************/

#include "qg_scrollbar.h"

#include <algorithm>
#include <cmath>

#include <QHelpEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QToolTip>

#include "lc_scrollmodel.h"

namespace {
    // stripe thickness as a share of the groove thickness, and its floor (logical px)
    constexpr double kStripeShare = 0.25;
    constexpr double kMinStripePixels = 2.0;
    // a candidate colour is lightened by this factor (QColor::lighter) on a dark palette
    constexpr int kLighterOnDark = 140;
    // styles shade the groove slightly off Window (Fusion light: #e6e6e6 on #efefef), so
    // contrast is also required against Window shaded this much (QColor::darker/lighter)
    constexpr int kGrooveShade = 106;

    double relativeLuminance(const QColor& color) {
        auto channel = [](const double c) {
            return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
        };
        const QColor rgb = color.toRgb();
        return 0.2126 * channel(rgb.redF()) + 0.7152 * channel(rgb.greenF()) + 0.0722 * channel(rgb.blueF());
    }

    QColor opaque(QColor color) {
        color.setAlpha(255);
        return color;
    }

    QColor blend(const QColor& from, const QColor& to, const double share) {
        const QColor a = from.toRgb();
        const QColor b = to.toRgb();
        return QColor::fromRgbF(static_cast<float>(a.redF() + (b.redF() - a.redF()) * share),
                                static_cast<float>(a.greenF() + (b.greenF() - a.greenF()) * share),
                                static_cast<float>(a.blueF() + (b.blueF() - a.blueF()) * share));
    }
}

void QG_ScrollBar::setContentBand(const bool hasContent, const double startTick, const double endTick) {
    if (hasContent == m_bandHasContent && (!hasContent || (startTick == m_bandStart && endTick == m_bandEnd))) {
        return;
    }
    m_bandHasContent = hasContent;
    m_bandStart = startTick;
    m_bandEnd = endTick;
    if (m_bandEnabled) {
        update();
    }
}

void QG_ScrollBar::setContentBandEnabled(const bool enabled) {
    if (enabled == m_bandEnabled) {
        return;
    }
    m_bandEnabled = enabled;
    update();
}

double QG_ScrollBar::contrastRatio(const QColor& a, const QColor& b) {
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

QColor QG_ScrollBar::contentBandColorFor(const QPalette& palette) {
    const QColor window = opaque(palette.color(QPalette::Active, QPalette::Window));
    const bool dark = window.lightnessF() < 0.5;
    const QColor groove = dark ? window.lighter(kGrooveShade) : window.darker(kGrooveShade);
    auto prepared = [dark](const QColor& color) {
        // 8 bits per channel, as painted
        return QColor((dark ? opaque(color).lighter(kLighterOnDark) : opaque(color)).rgb());
    };
    auto contrasts = [&window, &groove](const QColor& color) {
        return std::min(contrastRatio(color, window), contrastRatio(color, groove)) >= kMinBandContrast;
    };
    const QColor highlight = palette.color(QPalette::Active, QPalette::Highlight);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    // macOS's Highlight is the pale selection colour; Accent is the system accent
    const QColor accent = palette.color(QPalette::Active, QPalette::Accent);
#else
    const QColor accent = highlight;
#endif
    for (const QColor& candidate : {accent, highlight}) {
        const QColor color = prepared(candidate);
        if (contrasts(color)) {
            return color;
        }
    }
    // e.g. a user-picked yellow accent on a light palette: the text colour, tinted with
    // as much of the accent as keeps the contrast
    const QColor text(opaque(palette.color(QPalette::Active, QPalette::WindowText)).rgb());
    const QColor tint = prepared(accent);
    for (int percent = 75; percent > 0; percent -= 5) {
        const QColor color(blend(text, tint, percent / 100.0).rgb());
        if (contrasts(color)) {
            return color;
        }
    }
    if (contrasts(text)) {
        return text;
    }
    return dark ? QColor(Qt::white) : QColor(Qt::black);
}

QColor QG_ScrollBar::contentBandColor() const {
    if (!m_bandColor.isValid()) {
        m_bandColor = contentBandColorFor(palette());
    }
    return m_bandColor;
}

QRect QG_ScrollBar::contentBandRect() const {
    QStyleOptionSlider option;
    initStyleOption(&option);
    return contentBandStripe(option).toAlignedRect();
}

bool QG_ScrollBar::isContentBandPainted() const {
    return !contentBandRect().isNull()
        && LC_ScrollModel::bandIsInformative(m_bandStart - minimum(), m_bandEnd - minimum(), value() - minimum(),
                                             pageStep());
}

/**
 * Measures the thumb geometry from the style itself (slider rect at value 0 and at the
 * maximum; macOS reads sliderValue, the common styles sliderPosition, so both are set),
 * maps the content ticks through LC_ScrollModel::bandPixels(), and returns the stripe:
 * along the groove's bottom edge (horizontal) or right edge (vertical; the bar is always
 * left to right), a quarter of the groove thickness but at least kMinStripePixels,
 * snapped to device pixels so that it is painted crisp without antialiasing.
 */
QRectF QG_ScrollBar::contentBandStripe(const QStyleOptionSlider& option) const {
    if (!m_bandEnabled || !m_bandHasContent) {
        return {};
    }
    const bool horizontal = orientation() == Qt::Horizontal;
    QStyleOptionSlider probe = option;
    probe.sliderPosition = probe.sliderValue = probe.minimum;
    const QRect atMin = style()->subControlRect(QStyle::CC_ScrollBar, &probe, QStyle::SC_ScrollBarSlider, this);
    probe.sliderPosition = probe.sliderValue = probe.maximum;
    const QRect atMax = style()->subControlRect(QStyle::CC_ScrollBar, &probe, QStyle::SC_ScrollBarSlider, this);
    const QRect groove = style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarGroove, this);
    if (atMin.isEmpty() || groove.isEmpty()) {
        return {}; // e.g. NSScroller hides the knob of a bar too short for it
    }

    LC_ScrollModel::ThumbGeometry geometry;
    geometry.start0 = horizontal ? atMin.left() : atMin.top();
    geometry.travel = (horizontal ? atMax.left() : atMax.top()) - geometry.start0;
    geometry.length = horizontal ? atMin.width() : atMin.height();
    geometry.maximum = option.maximum - option.minimum;
    geometry.pageStep = option.pageStep;
    const LC_ScrollModel::Band band = LC_ScrollModel::bandPixels(geometry, true, m_bandStart - option.minimum,
                                                                 m_bandEnd - option.minimum);
    if (!band.visible) {
        return {};
    }

    const double dpr = std::max(1.0, devicePixelRatioF());
    const int grooveThickness = horizontal ? groove.height() : groove.width();
    const double thickness = std::min<double>(grooveThickness,
                                              std::max(kMinStripePixels, std::round(kStripeShare * grooveThickness)));
    const double thicknessDevice = std::max(1.0, std::round(thickness * dpr));
    const double startDevice = std::round(band.start * dpr);
    const double endDevice = std::round(band.end * dpr);
    if (horizontal) {
        const double outerDevice = std::floor((groove.y() + groove.height()) * dpr);
        return {startDevice / dpr, (outerDevice - thicknessDevice) / dpr, (endDevice - startDevice) / dpr,
                thicknessDevice / dpr};
    }
    const double outerDevice = std::floor((groove.x() + groove.width()) * dpr);
    return {(outerDevice - thicknessDevice) / dpr, startDevice / dpr, thicknessDevice / dpr,
            (endDevice - startDevice) / dpr};
}

void QG_ScrollBar::setToolTipProvider(std::function<QString()> provider) {
    m_toolTipProvider = std::move(provider);
}

QString QG_ScrollBar::providedToolTip() const {
    return m_toolTipProvider ? m_toolTipProvider() : QString();
}

bool QG_ScrollBar::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip && m_toolTipProvider) {
        // built on demand: nothing is formatted while the bar merely scrolls
        const QString text = m_toolTipProvider();
        if (text.isEmpty()) {
            QToolTip::hideText();
            event->ignore();
        } else {
            QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), text, this);
        }
        return true;
    }
    return QScrollBar::event(event);
}

void QG_ScrollBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        m_bandColor = QColor();
        update();
    }
    QScrollBar::changeEvent(event);
}

/**
 * The style paints the whole bar first (so hover, pressed and style-sheet states are
 * the style's own); the stripe then goes over the groove only, with the thumb cut out
 * (plus one pixel along the bar for Fusion's outline, drawn just outside its rect), so
 * the thumb is never tinted. Painting after the style keeps this independent of which
 * sub-controls a style honours (a QScrollBar style sheet repaints the whole bar on every
 * sub-control call, windowsvista/windows11 may paint cached whole-bar images).
 *
 * Nothing is painted while any of the drawing is in view along the bar (the view lies
 * inside it, covers it or straddles one of its edges; LC_ScrollModel::bandIsInformative()):
 * the bar then looks exactly as without the band.
 */
void QG_ScrollBar::paintEvent(QPaintEvent* event) {
    QScrollBar::paintEvent(event);
    if (!m_bandEnabled || !m_bandHasContent
        || !LC_ScrollModel::bandIsInformative(m_bandStart - minimum(), m_bandEnd - minimum(), value() - minimum(),
                                              pageStep())) {
        return;
    }
    QStyleOptionSlider option;
    initStyleOption(&option);
    const QRectF stripe = contentBandStripe(option);
    if (stripe.isEmpty()) {
        return;
    }
    const QRect thumb = style()->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, this);
    QRegion clip(stripe.toAlignedRect());
    if (thumb.isValid()) {
        clip -= orientation() == Qt::Horizontal ? QRect(thumb.left() - 1, 0, thumb.width() + 2, height())
                                                : QRect(0, thumb.top() - 1, width(), thumb.height() + 2);
    }
    clip &= event->region();
    if (clip.isEmpty()) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setClipRegion(clip);
    painter.fillRect(stripe, contentBandColor());
}
