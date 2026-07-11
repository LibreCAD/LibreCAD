/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_TITLEBARWIDGET_H
#define LC_TITLEBARWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QStyleOption>
#include <QPainter>
#include <QDockWidget>
#include <QResizeEvent>
#include <QPointer>
#include <QTimer>
#include <QBoxLayout>

class LC_CustomTitleBarWidget : public QWidget {
    Q_OBJECT public:
    enum DisplayMode {
        IconOnly,
        TextOnly,
        IconAndText
    };

    Q_ENUM(DisplayMode)

    // Конструктор с отдельными строками для горизонтальной и вертикальной ориентации
    explicit LC_CustomTitleBarWidget(const QString& horizontalTitle, const QString& verticalTitle, const QString& iconName = QString(),
                                     QWidget* parent = nullptr, DisplayMode mode = IconAndText);

    // Перегрузка конструктора для обратной совместимости (использует одну строку для обеих ориентаций)
    explicit LC_CustomTitleBarWidget(const QString& title, const QString& iconName = QString(), QWidget* parent = nullptr,
                                     DisplayMode mode = IconAndText);

    ~LC_CustomTitleBarWidget() override;

    // Getters (только для чтения)
    DisplayMode displayMode() const;
    QString horizontalTitle() const;
    QString verticalTitle() const;
    QString iconName() const;

    // Setter только для режима отображения
    void setDisplayMode(DisplayMode mode);

signals:
    void displayModeChanged(DisplayMode newMode);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void changeEvent(QEvent* event) override;
    bool event(QEvent* event) override;

private slots:
    void onDockWidgetFeaturesChanged();
    void onScreenChanged();
    void onLogicalDotsPerInchChanged(qreal dpi);
    void onApplicationFontChanged();
    void delayedUpdate();
    void showTooltip();

private:
    struct Constants {
        static constexpr int BASE_ICON_SIZE = 16;
        static constexpr int BASE_PADDING_SMALL = 2;
        static constexpr int BASE_PADDING_MEDIUM = 3;
        static constexpr int BASE_PADDING_LARGE = 5;
        static constexpr int BASE_PADDING_EXTRA = 4;
        static constexpr int MINIMUM_HEIGHT = 20;
        static constexpr int UPDATE_DELAY_MS = 50;
        static constexpr int TOOLTIP_DELAY_MS = 500;
        static constexpr int MAX_TEXT_LENGTH = 10000;
        static constexpr int CHAR_WIDTH_FACTOR = 2;
        static constexpr int CACHE_TOLERANCE_FACTOR = 1;
    };

    struct VerticalTextInfo {
        QString displayText;
        bool isElided;
    };

    QLabel* createTitleLabel(const QString& text);
    QString getCurrentTitle() const;
    void createIconLabel(const QString& iconName);
    void loadIcon(const QString& iconName);
    void updateIconForMode();
    void rebuildLayoutForMode();
    void setupConnections();
    void disconnectAllConnections();
    void updateIconSize();
    QSize getScaledIconSize() const;
    int scaleToDpi(int value) const;
    qreal getLogicalDpiX() const;
    void updateLayoutMargins();
    void updateFontMetrics();
    void updateDockWidgetPointer();
    void updateOrientation();
    void updateTitleForCurrentOrientation();
    void clearVerticalCache();
    void hideTooltip();
    void scheduleUpdate();
    VerticalTextInfo getVerticalDisplayText(const QString& text, int availableHeight) const;
    void createVerticalText(const QString& text);
    int getSafeAvailableHeight() const;
    QString safeUnicodeLeft(const QString& text, int maxChars) const;
    QPixmap createRotatedTextPixmap(const QString& text) const;
    bool isVerticalCacheValid() const;
    QString getElidedText(const QString& text, int width);
    QString safeElideText(const QString& text, int width) const;
    bool isTextElided() const;

    QLabel* m_titleLabel;
    QLabel* m_iconLabel;
    QBoxLayout* m_layout;
    QPointer<QDockWidget> m_dockWidget;

    const QString m_horizontalTitle;
    const QString m_verticalTitle;
    const QString m_iconName;
    Qt::Orientation m_currentOrientation;
    DisplayMode m_displayMode;
    bool m_isTextElided;
    QFontMetrics* m_fontMetrics;
    bool m_blockRebuild;
    QPixmap* m_verticalPixmapCache;
    QTimer* m_updateTimer;
    QTimer* m_tooltipTimer;
    QPoint m_lastTooltipPos;

    QList<QMetaObject::Connection> m_dockWidgetConnections;
    QList<QMetaObject::Connection> m_screenConnections;
    QList<QMetaObject::Connection> m_windowConnections;
    QList<QMetaObject::Connection> m_appConnections;
};

#endif
